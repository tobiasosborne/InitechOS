/* mouse.c -- PS/2 aux mouse (IRQ12) driver. See mouse.h for the full citations.
 *
 * beads: initech-5l5z FO-6 (ADR-0006 E-D3(b) / FO-6). THE CLAUDE.md minefield:
 * "real mode -> protected/flat" + the dual-8259A cascade EOI. The load-bearing
 * correctness point is the DUAL-PIC EOI order (SLAVE then MASTER) in
 * mouse_irq_handler -- a master-only EOI leaves the slave's in-service bit set
 * and SILENTLY WEDGES all further IRQ12s (no second mouse event ever fires).
 *
 * Ref (Law 1):
 *   - Intel 8042 keyboard-controller datasheet / IBM PS/2 Technical Reference:
 *     status register at 0x64 (bit0 OBF = output-buffer-full, data ready to
 *     read from 0x60; bit1 IBF = input-buffer-full, wait before writing 0x60/
 *     0x64); controller commands 0xA8 (enable aux), 0x20 (read config byte),
 *     0x60 (write config byte), 0xD4 (route the next 0x60 write to the aux
 *     device); config-byte bit1 = aux-port (IRQ12) interrupt enable, bit5 =
 *     aux-port clock DISABLE (clear to enable). PS/2 mouse command 0xF4 =
 *     enable data reporting (device ACK = 0xFA). [osdev.org PS/2 Mouse / 8042.]
 *   - Intel 8259A datasheet: IRQ12 = slave IR4 (cascaded on master IRQ2); a
 *     cascaded ISR sends a non-specific EOI (OCW2 0x20) to the SLAVE command
 *     port (0xA0) THEN the MASTER command port (0x20).
 *   - spec/event_model.h Sec 5: flair_raw_event_t MOUSE payload layout.
 *   - os/milton/kbd.c / pit.c: the function-pointer-hook + bounded-spin idioms
 *     (FO-4/FO-5) mirrored here. CLAUDE.md Law 1/2/3, Rule 2/11/12.
 *
 * ARTIFACT code (Law 3): freestanding, io.h only, no libc.
 */

#include "mouse.h"
#include "io.h"

/* --- 8042 PS/2 controller ports (Intel 8042 datasheet; standard ISA) ------ */
#define PS2_DATA        0x60u   /* data port (R: output buffer / W: input buf) */
#define PS2_STATUS      0x64u   /* status register (read)                      */
#define PS2_CMD         0x64u   /* command register (write)                    */

/* Status-register bits (read from 0x64). */
#define PS2_ST_OBF      0x01u   /* bit0: output buffer full (data ready @0x60) */
#define PS2_ST_IBF      0x02u   /* bit1: input  buffer full (busy; wait to W)  */

/* 8042 controller commands (written to 0x64). */
#define PS2_CMD_ENABLE_AUX    0xA8u  /* enable the auxiliary (mouse) device    */
#define PS2_CMD_READ_CONFIG   0x20u  /* read controller config byte (-> 0x60)  */
#define PS2_CMD_WRITE_CONFIG  0x60u  /* write controller config byte (<- 0x60) */
#define PS2_CMD_WRITE_AUX     0xD4u  /* next 0x60 write goes to the aux device */

/* Controller configuration-byte bits. */
#define PS2_CFG_AUX_IRQ12     0x02u  /* bit1: aux-port interrupt (IRQ12) enable */
#define PS2_CFG_AUX_CLK_OFF   0x20u  /* bit5: aux-port CLOCK DISABLE (clear=on) */

/* PS/2 mouse device commands (sent via the 0xD4 prefix, then to 0x60). */
#define MOUSE_CMD_ENABLE_RPT  0xF4u  /* enable data reporting (ACK 0xFA)        */

/* PS/2 packet byte-0 fields. */
#define PKT_BTN_LEFT    0x01u
#define PKT_BTN_RIGHT   0x02u
#define PKT_BTN_MIDDLE  0x04u
#define PKT_BTN_MASK    0x07u
#define PKT_ALWAYS1     0x08u   /* byte0 bit3 is always 1 -- the resync anchor  */
#define PKT_OVERFLOW    0xC0u   /* byte0 bits 6,7 = X/Y overflow (data invalid) */

/* --- 8259A EOI (the dual-PIC minefield; Intel 8259A datasheet) ------------ */
#define PIC1_CMD        0x20u   /* master 8259A command port                   */
#define PIC2_CMD        0xA0u   /* slave  8259A command port                   */
#define PIC_EOI         0x20u   /* OCW2 non-specific end-of-interrupt          */

/* Bounded-spin budget for every 8042 status poll. Deterministic (Rule 11): the
 * wait is bounded by a COUNT, never wall-clock. ~100k inb()s is generously past
 * the 8042's microsecond-scale latch time on any emulated/real 386, and a
 * timeout is fail-soft (skip, never hang -- Rule 2). */
#define PS2_SPIN_BUDGET  100000u

/* --- packet-assembly state (ISR-private; single producer, no concurrency) -- */
static uint8_t  g_pkt[3];        /* the 3 PS/2 packet bytes                     */
static uint32_t g_phase;         /* next byte index 0..2                        */

/* Completed-packet hook (NULL default; mouse.h contract). Written once from
 * task context BEFORE sti, read in ISR context -- no concurrent writer. */
static void (*g_mouse_event_hook)(int dx, int dy, uint8_t buttons) = 0;

void mouse_set_event_hook(void (*fn)(int dx, int dy, uint8_t buttons))
{
    g_mouse_event_hook = fn;
}

/* Wait (bounded) until the 8042 input buffer is EMPTY (IBF clear) so it is safe
 * to write a command/data byte. Returns 1 if ready, 0 on timeout (fail-soft). */
static int ps2_wait_write(void)
{
    for (uint32_t i = 0u; i < PS2_SPIN_BUDGET; i++) {
        if ((inb(PS2_STATUS) & PS2_ST_IBF) == 0u) {
            return 1;
        }
    }
    return 0;   /* fail-soft: never hang (Rule 2) */
}

/* Wait (bounded) until the 8042 output buffer is FULL (OBF set) so a byte is
 * available to read from 0x60. Returns 1 if data is ready, 0 on timeout. */
static int ps2_wait_read(void)
{
    for (uint32_t i = 0u; i < PS2_SPIN_BUDGET; i++) {
        if ((inb(PS2_STATUS) & PS2_ST_OBF) != 0u) {
            return 1;
        }
    }
    return 0;
}

/* Write a byte to the 8042 COMMAND register (0x64), IBF-gated. */
static void ps2_write_cmd(uint8_t cmd)
{
    (void)ps2_wait_write();
    outb(PS2_CMD, cmd);
}

/* Write a byte to the 8042 DATA register (0x60), IBF-gated. */
static void ps2_write_data(uint8_t data)
{
    (void)ps2_wait_write();
    outb(PS2_DATA, data);
}

/* Read a byte from the 8042 DATA register (0x60), OBF-gated. Returns 0xFF on a
 * read timeout (the caller treats the ACK/config read as best-effort). */
static uint8_t ps2_read_data(void)
{
    if (!ps2_wait_read()) {
        return 0xFFu;   /* fail-soft sentinel; never hang */
    }
    return inb(PS2_DATA);
}

void mouse_init(void)
{
    g_phase = 0u;

    /* (1) Drain any stale byte the firmware left in the 8042 output buffer so
     * the first real IRQ12 is not preceded by a ghost byte (8042: a pending
     * byte blocks fresh data). Bounded (Rule 2). */
    for (int i = 0; i < 16; i++) {
        if ((inb(PS2_STATUS) & PS2_ST_OBF) != 0u) {
            (void)inb(PS2_DATA);
        } else {
            break;
        }
    }

    /* (2) Enable the auxiliary (mouse) device. */
    ps2_write_cmd(PS2_CMD_ENABLE_AUX);          /* 0xA8 */

    /* (3) Read the controller configuration byte. */
    ps2_write_cmd(PS2_CMD_READ_CONFIG);         /* 0x20 */
    uint8_t cfg = ps2_read_data();              /* <- 0x60 */

    /* (4) Enable IRQ12 (bit1 SET) and the aux clock (bit5 CLEAR). */
    cfg |= PS2_CFG_AUX_IRQ12;                    /* bit1 = 1: IRQ12 on   */
    cfg &= (uint8_t)~PS2_CFG_AUX_CLK_OFF;        /* bit5 = 0: clock on    */

    /* (5) Write the configuration byte back. */
    ps2_write_cmd(PS2_CMD_WRITE_CONFIG);        /* 0x60 (command) */
    ps2_write_data(cfg);                        /* -> 0x60 (data) */

    /* (6) Enable data reporting on the mouse: 0xD4 prefix, then 0xF4 to the aux
     * device; CONSUME the 0xFA ACK by polling (IRQ12 still masked here). The
     * device may take a moment to forward 0xF4 and reply, so wait (bounded)
     * SPECIFICALLY for the 0xFA byte across a few attempts, then drain any
     * residual byte. This is load-bearing: a 0xFA that is NOT consumed here is
     * later delivered via IRQ12 and mis-framed as a packet byte0 -- 0xFA has
     * bit3 (the always-1 resync anchor) SET, so the handler would wrongly accept
     * it and SLIP the first real packet (verified symptom). All bounded (Rule
     * 11): no wait is wall-clock or unbounded. */
    ps2_write_cmd(PS2_CMD_WRITE_AUX);           /* 0xD4 */
    ps2_write_data(MOUSE_CMD_ENABLE_RPT);       /* 0xF4 -> mouse */
    for (int t = 0; t < 8; t++) {
        if (ps2_read_data() == 0xFAu) {         /* ACK seen -> done */
            break;
        }
    }
    /* Drain anything still pending so the FIRST IRQ12 reads a fresh packet
     * byte0 (injection happens only AFTER FLAIR-HOOK-SET, well after this, so
     * no real movement packet can be eaten here). */
    for (int i = 0; i < 16; i++) {
        if ((inb(PS2_STATUS) & PS2_ST_OBF) != 0u) {
            (void)inb(PS2_DATA);
        } else {
            break;
        }
    }
    g_phase = 0u;
}

void mouse_irq_handler(void)
{
    /* Read ONE byte from the 8042 output buffer (port 0x60). The read clears
     * OBF and re-arms the next IRQ12 (8042 datasheet). ISR-enqueue-only
     * minimum (ADR-0004 D-4): no Toolbox, no alloc, no I/O beyond this read. */
    uint8_t data = inb(PS2_DATA);

    /* Accumulate the 3-byte PS/2 packet. RESYNC at phase 0: a valid movement
     * byte0 has bit3 (the always-1 anchor) SET and the overflow bits (bit6 X,
     * bit7 Y) CLEAR. Reject anything else and stay at phase 0. This rejects:
     *   - a mis-aligned dx/dy byte that happens to land at phase 0;
     *   - a PS/2 STATUS byte that is NOT packet data -- notably the 0xFA ACK
     *     (QEMU/real controllers can deliver a stray ACK via IRQ12 after the
     *     0xF4 enable; 0xFA = 1111_1010 has bit3 SET, so the bit3 anchor ALONE
     *     would wrongly accept it and slip the first real packet -- the verified
     *     FO-6 symptom). 0xFA has the overflow bits set, so the 0xC0 test
     *     rejects it. (0xAA BAT-OK is likewise rejected.)
     *   - an overflow packet (delta exceeded the int8 range): unusable data, so
     *     the ISR drops it rather than enqueue a corrupt jump (the pump never
     *     sees garbage; spec/event_model.h Sec 5 deltas are int8). */
    if (g_phase == 0u &&
        ((data & PKT_ALWAYS1) == 0u || (data & PKT_OVERFLOW) != 0u)) {
        /* not a valid packet byte0: drop, stay at phase 0 -- fall to the EOI. */
    } else {
        g_pkt[g_phase] = data;
        g_phase++;
        if (g_phase >= 3u) {
            g_phase = 0u;
            uint8_t  b0 = g_pkt[0];
            int8_t   dx = (int8_t)g_pkt[1];   /* signed PS/2 X delta (+ = right) */
            int8_t   dy = (int8_t)g_pkt[2];   /* signed PS/2 Y delta (+ = up)    */
            uint8_t  buttons = (uint8_t)(b0 & PKT_BTN_MASK);
            if (g_mouse_event_hook) {
                /* RAW deltas (ADR-0004 D-4: the ISR posts raw; the WaitNextEvent
                 * pump cooks deltas->cursor + the screen-coordinate Y flip in
                 * task context, FO-7 / spec/event_model.h Sec 5). */
                g_mouse_event_hook((int)dx, (int)dy, buttons);
            }
        }
    }

    /* DUAL-PIC EOI -- THE load-bearing minefield correctness point (ADR-0006
     * BC-3; CLAUDE.md "real mode -> protected is a minefield"). IRQ12 is the
     * SLAVE 8259A's IR4, cascaded behind the master IRQ2. The handler MUST send
     * a non-specific EOI to the SLAVE command port (0xA0) FIRST, THEN the MASTER
     * command port (0x20). A master-only EOI leaves the slave's in-service bit
     * set and SILENTLY WEDGES every further slave IRQ -- no second mouse event
     * ever fires (Bochs/real-hardware accurate; QEMU is more forgiving).
     * Ref: Intel 8259A datasheet (cascaded EOI). */
#ifndef FLAIR_LIVE_MUTATE_MASTER_ONLY_EOI
    outb(PIC2_CMD, PIC_EOI);   /* SLAVE EOI FIRST (0xA0)  -- the non-negotiable */
#endif
    outb(PIC1_CMD, PIC_EOI);   /* MASTER EOI (0x20)                              */
}
