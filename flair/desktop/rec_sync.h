/* os/milton/rec_sync.h -- included once, by os/milton/kmain.c (it uses kmain's
 * static serial and clock helpers). Bead initech-x8gsi.1. */
#ifndef INITECH_REC_SYNC_H
#define INITECH_REC_SYNC_H

/* ---- RECORD-SYNC: the clip recorder's capture handshake (bead initech-x8gsi.1)
 * FACTORY INSTRUMENTATION, compiled ONLY into the record kernels (-DFLAIR_RECORD_SYNC,
 * Makefile RECORD_SYNC_IMAGES); every shipped and gate kernel is byte-identical
 * without it.
 *
 * WHY. record-flair used to grab each clip frame a fixed wall-clock settle after an
 * injected event. A guest still busy at that instant (an Empty Trash purge on the
 * ATA disk, Open-All painting 21 windows, a lazy overlay load) was caught half
 * done, and WHICH half depended on host load: record-flair-repro went RED on
 * empty_trash / finder_entries (MEASURED 2026-10-25: frame 52 of empty_trash showed
 * the purged files still listed in one capture, the emptied Trash in the other).
 * And the guest's OWN clock ran on through every settle and every screendump, so
 * time-driven behaviour (a held scroll arrow repeats every 5 ticks after 30) saw
 * however much host time the recorder happened to spend (MEASURED: scroll_arrow
 * took 2, 3 or 4 repeat steps per press across six captures).
 *
 * THE HANDSHAKE (LOCKSTEP). The harness (harness/emu/qemu.c --record-sync) wires a
 * second UART, COM2, to a pipe it alone writes, and runs QEMU with -icount (guest
 * time = instructions retired, not host speed). Bytes it sends:
 *   1..250  FRAME: serve at least <n> ticks after the guest last consumed input;
 *   0xFE n  HOLD:  serve <n> ticks (1..250) after the guest last resumed --
 *                  the harness's "w<ms>" token, timed on the guest's clock;
 *   0xFF    RESUME.
 * The guest serves only at an INPUT-IDLE point -- WaitNextEvent's cooperative
 * yield (or the DOS blocking console read's wait) with the time condition met,
 * the 8042 output buffer empty (no PS/2 byte the ISR has not taken) and its
 * software input ring empty -- the 8042 first: an ISR that takes the last byte
 * between the two checks has already posted it to the ring. A WaitNextEvent yield
 * means the caller has finished everything the previous event (or nullEvent: the
 * cursor redraw after a pure move) asked of it; the cooperative model has no other
 * place to be. Serving = print "REC-SYNC n=<k> ..." and BLOCK with the guest's
 * clock STOPPED (the FLAIR PIT hook detached; the DOS clock subtracts the blocked
 * ticks). While it is blocked the harness grabs the frame, then injects the NEXT
 * token's input (it queues in the 8042/ring) and only then resumes it, and the
 * guest resumes on a PIT tick edge. So every stretch the guest runs between two
 * sync points starts from the same state, at the same clock phase, with the same
 * input already queued: a frame never depends on host timing.
 *
 * While blocked, a FRAME request whose settle is already met is served again at
 * once (the same frozen state); otherwise it, or a HOLD, resumes the guest with
 * the request pending. COM2 presence is probed once (scratch register): an image
 * booted without the pipe never blocks. contract: a malformed byte is shouted on
 * serial (and makes record-flair RED). */
#ifdef FLAIR_RECORD_SYNC
#define REC_SYNC_COM2      0x2F8u   /* COM2 RBR (DLAB=0)            */
#define REC_SYNC_COM2_LSR  0x2FDu   /* line status: bit 0 = DR      */
#define REC_SYNC_COM2_SCR  0x2FFu   /* scratch register (probe)     */
#define REC_SYNC_KBC_STAT  0x64u    /* 8042 status: bit 0 = OBF     */
#define REC_SYNC_HOLD      0xFEu
#define REC_SYNC_RESUME    0xFFu
static int      g_rec_sync_state;   /* 0 unprobed, 1 present, -1 absent  */
static uint32_t g_rec_sync_n;       /* sync points served                */
static uint32_t g_rec_req;          /* pending FRAME settle, 0 = none    */
static uint32_t g_rec_hold;         /* pending HOLD ticks, 0 = none      */
static uint32_t g_rec_resume;       /* guest clock at the last resume    */
static uint32_t g_rec_in_tail;      /* FLAIR ring tail at the last WNE entry   */
static uint32_t g_rec_in_tick;      /* FLAIR clock when input was last consumed */
static uint32_t g_rec_dos_in_tick;  /* DOS clock when CON last returned a char  */
static uint32_t g_rec_pit_frozen;   /* PIT ticks spent blocked (DOS clock)      */

static void rec_sync_bad(const char *what, uint8_t b)
{
    serial_puts("REC-SYNC-BAD ");
    serial_puts(what);
    serial_putc('=');
    serial_putu(b);
    serial_putc('\n');
}

/* The DOS-side clock: PIT ticks less every tick spent blocked in a sync. */
static uint32_t rec_dos_now(void)
{
    return pit_ticks() - g_rec_pit_frozen;
}

static uint8_t rec_com2_wait_byte(void)
{
    while ((inb(REC_SYNC_COM2_LSR) & 1u) == 0u) {
        __asm__ __volatile__("hlt");
    }
    return inb(REC_SYNC_COM2);
}

/* Decode one harness byte b (and a HOLD's operand) into a pending request.
 * Returns 1 for RESUME, 0 otherwise. */
static int rec_sync_take(uint8_t b)
{
    if (b >= 1u && b <= 250u) {
        g_rec_req = b;
    } else if (b == REC_SYNC_HOLD) {
        uint8_t n = rec_com2_wait_byte();
        if (n == 0u || n > 250u) { rec_sync_bad("hold", n); n = 1u; }
        g_rec_hold = n;
    } else if (b == REC_SYNC_RESUME) {
        return 1;
    } else {
        rec_sync_bad("req", b);
    }
    return 0;
}

/* Nonzero while a FRAME or HOLD request is pending (taking one off COM2). */
static int rec_sync_pending(void)
{
    if (g_rec_sync_state == 0) {
        outb(REC_SYNC_COM2_SCR, 0x5Au);
        g_rec_sync_state = (inb(REC_SYNC_COM2_SCR) == 0x5Au) ? 1 : -1;
        if (g_rec_sync_state > 0) serial_puts("REC-SYNC-ON\n");
    }
    if (g_rec_sync_state > 0 && g_rec_req == 0u && g_rec_hold == 0u &&
        (inb(REC_SYNC_COM2_LSR) & 1u) != 0u) {
        if (rec_sync_take(inb(REC_SYNC_COM2))) rec_sync_bad("resume", REC_SYNC_RESUME);
    }
    return g_rec_req != 0u || g_rec_hold != 0u;
}

/* Is a pending request due at clock `now` (input last consumed at `in`)? */
static int rec_sync_due(uint32_t now, uint32_t in)
{
    if (g_rec_req) return now - in >= g_rec_req;
    return now - g_rec_resume >= g_rec_hold;
}

/* The 8042 holds no byte the keyboard/mouse ISR has yet to take. */
static int rec_sync_kbc_empty(void)
{
    return (inb(REC_SYNC_KBC_STAT) & 1u) == 0u;
}

static void rec_sync_announce(uint32_t now, uint32_t in)
{
    g_rec_sync_n++;
    serial_puts("REC-SYNC n=");
    serial_putu(g_rec_sync_n);
    serial_puts(g_rec_req ? " frame" : " hold");
    serial_puts(" t=");
    serial_putu(now);
    serial_puts(" after=");
    serial_putu(now - (g_rec_req ? in : g_rec_resume));
    serial_putc('\n');
    g_rec_req = 0u;
    g_rec_hold = 0u;
}

/* Serve the due request at clock `now` and block until the harness resumes us.
 * flair_clock: stop the FLAIR tick (event.c g_tick) while blocked; the DOS
 * clock is kept frozen by counting the blocked PIT ticks instead. Resume is on a
 * PIT tick edge, so the guest always restarts at the same clock phase. */
static void rec_sync_serve(int flair_clock, uint32_t now, uint32_t in)
{
    uint32_t p0 = pit_ticks(), p;
    if (flair_clock) pit_set_tick_hook((void (*)(void))0);
    rec_sync_announce(now, in);
    for (;;) {
        if (rec_sync_take(rec_com2_wait_byte())) break;
        if (g_rec_req && now - in >= g_rec_req) {
            rec_sync_announce(now, in);       /* same frozen state, again */
            continue;
        }
        break;                                /* run on with it pending */
    }
    p = pit_ticks();
    while (pit_ticks() == p) {
        __asm__ __volatile__("hlt");          /* resume on a tick edge */
    }
    g_rec_pit_frozen += pit_ticks() - p0;
    if (flair_clock) pit_set_tick_hook(flair_tick_advance);
    g_rec_resume = flair_clock ? flair_tick_count() : rec_dos_now();
}

/* The FLAIR input-idle point: WaitNextEvent's cooperative yield (event.c, the
 * ring just found empty; flair_live_yield is the hook), whichever translation
 * unit called it -- kmain.c, dialog.c's modal, or a pump living in the COLD
 * overlay (a call-site macro cannot see those: ADR-0014 Lane 2 moved the FILE
 * COPY pump there and record-flair went RED on the missing sync). */
static void rec_sync_yield(flair_raw_ring_t *ring)
{
    uint32_t now = flair_tick_count();
    if (ring->tail != g_rec_in_tail) {       /* the last call consumed input */
        g_rec_in_tail = ring->tail;
        g_rec_in_tick = now;
    }
    if (rec_sync_pending() && rec_sync_due(now, g_rec_in_tick) &&
        rec_sync_kbc_empty() && ring->head == ring->tail) {
        rec_sync_serve(1, now, g_rec_in_tick);
    }
}

/* The menu-bar clock of a record kernel (the pump's AH=2Ch poll): the time the
 * RTC read at the first poll (to the minute), advanced by the FLAIR clock since.
 * The RTC itself
 * runs on through every blocked capture (host time); this clock does not, so a
 * minute rolls over at the same frame in every capture. */
static void rec_sync_clock(int_frame_t *f)
{
    static int have;
    static uint32_t base_s, base_t;
    uint32_t s;
    if (!have) {
        have = 1;
        /* Whole minutes only: the boot's disk waits make the RTC's SECOND
         * at the first poll a matter of host I/O latency. */
        base_s = ((f->ecx >> 8) & 255u) * 3600u + (f->ecx & 255u) * 60u;
        base_t = flair_tick_count();
    }
    s = (base_s + (flair_tick_count() - base_t) / 100u) % 86400u;
    f->ecx = (f->ecx & 0xFFFF0000u) | ((s / 3600u) << 8) | ((s / 60u) % 60u);
    f->edx = (f->edx & 0xFFFF0000u) | ((s % 60u) << 8);
}

#define REC_SYNC_YIELD(ring)  rec_sync_yield(ring)
#define REC_SYNC_CLOCK(f)     rec_sync_clock(f)
/* The DOS console's input-idle point, timed on the DOS clock (the FLAIR clock is
 * unhooked at the DOS hand-off): the 8042 first, then the kbd ring; a char that
 * raced in is taken next pass. Expands inside the int21_conin_get_kbd wait loop. */
#define REC_SYNC_DOS_IDLE()                                         \
    if (rec_sync_pending() &&                                       \
        rec_sync_due(rec_dos_now(), g_rec_dos_in_tick) &&           \
        rec_sync_kbc_empty() && !kbd_haschar()) {                   \
        rec_sync_serve(0, rec_dos_now(), g_rec_dos_in_tick);        \
        continue;                                                   \
    }
#define REC_SYNC_DOS_INPUT(c) do { if ((c) >= 0) g_rec_dos_in_tick = rec_dos_now(); } while (0)
#else  /* every shipped and gate kernel: nothing at all */
#define REC_SYNC_YIELD(ring)  ((void)0)
#define REC_SYNC_CLOCK(f)     ((void)0)
#define REC_SYNC_DOS_IDLE()   ((void)0)
#define REC_SYNC_DOS_INPUT(c) ((void)0)
#endif /* FLAIR_RECORD_SYNC */

#endif /* INITECH_REC_SYNC_H */

