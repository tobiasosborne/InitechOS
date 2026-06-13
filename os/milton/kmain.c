/* kmain.c -- InitechDOS flat C kernel entry (the stage2 handoff landing).
 *
 * beads: initech-d00 ("stage2 -> C kernel handoff").
 * Ref:   PRD Sec 5 (hardware contract: VBE 2.0 LFB 640x480 8/32bpp, flat
 *        binary kernel handed off by stage2; ADR-0003 DEC-08);
 *        docs/research/boot-to-text-ground-truth.md Sec 2 (boot_info contract),
 *        Sec 4.1 (KERNEL serial marker), Sec 5 Risk 2 (the blitter MUST branch
 *        on lfb_bpp -- 32 vs 24 -- exactly as stage2 does, stage2.asm:272-296);
 *        stage2.asm SEAFOAM_R/G/B (lines 42-44) + fill32/fill24 (272-296).
 *        CLAUDE.md Law 2 (oracle is truth), Rule 2 (fail loud),
 *        Rule 11 (reproducible -- NO __DATE__/__TIME__/host paths),
 *        Rule 12 (ASCII only).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 *
 * This is the FIRST C that runs on InitechOS. Scope (initech-d00): prove the
 * handoff -- read boot_info, emit serial markers, and (CONSTRAINT) re-draw the
 * LFB seafoam so the existing test-tracer-boot screendump oracle stays green.
 * The text console + banner are the NEXT tasks (initech-yqb / initech-bea);
 * see the fb_clear seam below. Do NOT implement text here.
 */

#include <stdint.h>
#include "io.h"
#include "boot_info.h"
#include "console.h"
#include "idt.h"
#include "pic.h"
#include "int21.h"
#include "psp.h"         /* psp_build -> the kernel-context PSP (initech-509.3) */
#include "sft.h"         /* sft_init -> the system file table (initech-509.3) */
#include "loader.h"      /* load_program (beads initech-509.5) */
#include "test_prog.h"   /* the baked flat test/type/dir program bytes */
#include "ata.h"         /* ATA PIO backend (FAT-mount over the emulator) */
#include "fat12.h"       /* FAT12 mount + root-dir enumerate (proto-DIR)  */
#include "fileio_fat.h"  /* bind the FAT12 file backend into int21 (509.5) */
#include "kbd.h"         /* PS/2 keyboard IRQ1 driver (initech-3rs) */
#include "pit.h"         /* 8254 PIT IRQ0 tick (initech-3rs) */
#include "rtc.h"         /* MC146818 RTC clock source (initech-yv9) */
#include "command.h"     /* COMMAND.COM REPL (initech-7pc); BOOT_SHELL only */
#include "sysinit.h"     /* SYSINIT named bring-up phases (beads initech-509.2) */

/* Seafoam: ported VERBATIM from stage2.asm:42-44 (SEAFOAM_R/G/B). The pie
 * chart and the wristwatch are canon; so is this fill color. The screendump
 * oracle (tools/ppm_seafoam_check.c) asserts this exact RGB. */
#define SEAFOAM_R 0x6F
#define SEAFOAM_G 0xA0
#define SEAFOAM_B 0x8E

/* COM1 16550 UART. The MBR already programmed it (mbr.asm serial_init); we
 * only poll LSR THRE and write THR -- same protocol as stage2's serial_putc32
 * (stage2.asm:313-327). Ref: 16550 UART datasheet. */
#define COM1_THR 0x3F8u   /* transmit holding register (DLAB=0) */
#define COM1_LSR 0x3FDu   /* line status register               */
#define LSR_THRE 0x20u    /* transmit holding register empty     */
/* Bounded spin so a stuck/absent UART never hangs the boot path (Rule 2); a
 * working 16550 asserts THRE far below this poll count, so the bound is inert
 * on every emulator while still bailing on dead hardware. */
#define SERIAL_SPIN_MAX 100000u

/* The InitechDOS operator banner -- the FIRST text the OS prints at boot.
 * Ref: ADR-0003 DEC-12 + Appendix D.1; the LOCKED bytes are spec/dos_banner.txt
 * (two lines, byte-exact, incl. the controlled DOUBLE space in "InitechDOS  Version
 * 3.30"). The literals below MUST match spec/dos_banner.txt byte-for-byte; the
 * test-boot gate extracts these two lines from the serial capture and `diff`s
 * them against spec/dos_banner.txt, so any drift here goes RED (Law 1 / Rule 8).
 * ASCII only (Rule 12). */
#define BANNER_LINE1 "InitechDOS  Version 3.30"
#define BANNER_LINE2 "Copyright (C) 1991 Initech Systems Corporation.  All Rights Reserved."

static void serial_putc(char c)
{
    uint32_t spins = 0u;
    while ((inb(COM1_LSR) & LSR_THRE) == 0) {
        if (++spins >= SERIAL_SPIN_MAX) {
            return;             /* UART not draining -> drop the byte, never hang */
        }
    }
    outb(COM1_THR, (uint8_t)c);
}

static void serial_puts(const char *s)
{
    while (*s) {
        serial_putc(*s++);
    }
}

/* Emit an unsigned decimal (for the BI-BAD diagnostic). Freestanding: no
 * libc, so we format by hand into a small reversed buffer. */
static void serial_putu(uint32_t v)
{
    char buf[10];
    int n = 0;

    if (v == 0) {
        serial_putc('0');
        return;
    }
    while (v > 0 && n < (int)sizeof(buf)) {
        buf[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n > 0) {
        serial_putc(buf[--n]);
    }
}

/* fb_clear -- fill the entire linear framebuffer with one RGB color, branching
 * on bpp exactly as stage2 (stage2.asm:272-296). This is the seam for the
 * future text console (initech-yqb): the console will clear to a background
 * color and then blit glyphs from boot_info.font_addr. For initech-d00 we only
 * use it to re-paint seafoam so the screendump oracle stays green.
 *
 * Ref Risk 2 (ground-truth Sec 5): NEVER assume 32bpp -- Bochs/86Box may
 * present 24bpp first. We honor boot_info.lfb_bpp. */
static void fb_clear(const boot_info_t *bi, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t total = bi->lfb_pitch * bi->lfb_height; /* bytes, incl. padding */

    if (bi->lfb_bpp == 8) {
        /* Mode 0x13 fallback (initech-6pj): 1 byte/pixel = a palette INDEX. The
         * paletted framebuffer cannot encode an arbitrary (r,g,b); we fill with
         * the console background index, which the kernel maps to seafoam in the
         * VGA DAC (vga_set_dac). The (r,g,b) request is honored only on the
         * direct-color VBE paths below. */
        volatile uint8_t *fb = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
        (void)r; (void)g; (void)b;
        for (uint32_t i = 0; i < total; i++) {
            fb[i] = (uint8_t)CONSOLE_BG_IDX;
        }
    } else if (bi->lfb_bpp == 32) {
        /* 32bpp pixel = 0x00RRGGBB (XRGB8888; X/alpha byte = 0). */
        uint32_t px = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        volatile uint32_t *fb = (volatile uint32_t *)(uintptr_t)bi->lfb_addr;
        uint32_t dwords = total >> 2;
        for (uint32_t i = 0; i < dwords; i++) {
            fb[i] = px;
        }
    } else {
        /* 24bpp: B,G,R byte triples per pixel (stage2.asm fill24). */
        volatile uint8_t *fb = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
        uint32_t pixels = total / 3u;
        for (uint32_t i = 0; i < pixels; i++) {
            fb[i * 3u + 0u] = b;
            fb[i * 3u + 1u] = g;
            fb[i * 3u + 2u] = r;
        }
    }
}

/* vga_set_dac -- program one VGA DAC palette register (mode 0x13 fallback,
 * initech-6pj). In the paletted standard-VGA mode the console writes palette
 * INDICES, so the kernel maps CONSOLE_BG_IDX -> seafoam and CONSOLE_FG_IDX ->
 * light-gray ink here. The DAC is 6 bits/channel (0..63), so 8-bit RGB is
 * >> 2. Ports: 0x3C8 = write-index, 0x3C9 = R,G,B data in succession. The
 * direct-color VBE 24/32bpp paths need no palette and never call this. */
static void vga_set_dac(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    outb(0x3C8, index);
    outb(0x3C9, (uint8_t)(r >> 2));
    outb(0x3C9, (uint8_t)(g >> 2));
    outb(0x3C9, (uint8_t)(b >> 2));
}

/* Emit a signed decimal (for FAT-MOUNT-FAIL rc=NN, which carries a negative
 * fail-loud error code -- ATA_ERR_* / FAT12_ERR_*). Freestanding hand-format. */
static void serial_puti(int32_t v)
{
    if (v < 0) {
        serial_putc('-');
        /* Negate in unsigned space so INT32_MIN does not overflow. */
        serial_putu((uint32_t)(-(int64_t)v));
    } else {
        serial_putu((uint32_t)v);
    }
}

/* extern entrypoint of the live IDT self-test handler (isr.asm, vector 0x80). */
extern void isr_selftest(void);

/* extern entrypoint of the INT 21h trap stub (isr.asm, vector 0x21). */
extern void int21_entry(void);

/* extern entrypoint of the INT 20h legacy-terminate stub (isr.asm, vector 0x20;
 * beads initech-509.5). Free vector because the PIC master is at 0x28 (DEC-04a). */
extern void int20_entry(void);

/* extern entrypoints of the hardware IRQ stubs (isr.asm, beads initech-3rs):
 * PIT IRQ0 -> vector 0x28, keyboard IRQ1 -> vector 0x29 (post PIC remap). */
extern void irq0_entry(void);
extern void irq1_entry(void);

/* Called by isr_selftest (isr.asm) on `int 0x80`. Emits the self-test marker
 * to serial proving dispatch reached C; the handler then irets and the boot
 * resumes. Same-TU access to serial_puts. */
void selftest_report_c(void)
{
    serial_puts("IDT-SELFTEST-OK\n");
}

/* ---- INT 21h CON device wiring (beads initech-509.5) ----------------------
 * The dispatcher (int21.c) routes every "display" byte through a sink and
 * terminate through a hook. The kernel binds a sink that fans each byte out to
 * the LFB console AND COM1 serial (the CON device), and a terminate hook that
 * emits the exit line then cli;hlt (terminate == stop; no process model yet).
 * The console pointer is held at file scope so the sink (a plain function) can
 * reach the live console kernel_main set up -- mirrors panic_set_console. */
static console_t *g_int21_con = 0;

/* Kernel-context PSP (beads initech-509.3). Holds the standard Job File Table
 * for INT 21h handle functions issued from kernel context (before/after a
 * program runs). File scope (BSS) so it outlives kernel_main and stays bound
 * across the program load/return. The loader binds the loaded program's own PSP
 * during a run; the kernel re-binds this one when load_program returns. */
static psp_t g_kernel_psp;

static void int21_con_sink(char c)
{
    if (g_int21_con) {
        console_putc(g_int21_con, c);
    }
    serial_putc(c);
}

static void int21_exit_hook(uint8_t code)
{
    /* Emit the grep-able exit marker to serial + one console line, then halt
     * cleanly (Rule 2 -- never silent, never hang). */
    serial_puts("INT21-EXIT rc=");
    serial_putu((uint32_t)code);
    serial_putc('\n');
    if (g_int21_con) {
        console_puts(g_int21_con, "\nProgram terminated.\n");
    }
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

/* ---- INT 21h CON INPUT source wiring (beads initech-n62) ------------------
 * The dispatcher (int21.c) reads keystrokes through an input source it binds
 * via int21_set_conin: a BLOCKING get + a NON-blocking poll. The kernel binds
 * the source to the live PS/2 keyboard (kbd.c). The blocking get spins on `hlt`
 * until a key arrives -- the keyboard IRQ1 (vector 0x29) wakes the hlt, and the
 * 0x8F trap gate on INT 21h leaves IF set so the IRQ can fire WHILE an INT 21h
 * call is in flight (kbd.h REENTRANCY INVARIANT: IRQ1 touches only kbd state).
 * This glue MUST live here, not in int21.c, so int21.c stays free of hlt/I/O and
 * compiles hosted (Law 3). Bound near the sti, after kbd_init. */
static int int21_conin_get_kbd(void)
{
    int c;
    while ((c = kbd_getchar()) < 0) {
        __asm__ __volatile__("hlt");   /* sleep until IRQ1 (a keystroke) wakes us */
    }
    return c;
}

static int int21_conin_poll_kbd(void)
{
    return kbd_getchar();   /* non-blocking: char 0..255 or -1 if the ring is empty */
}

/* ---- INT 21h CLOCK source wiring (beads initech-yv9) ----------------------
 * The dispatcher (int21.c) reads/writes the wall clock through a CLOCK seam it
 * binds via int21_set_clock. The kernel binds the live MC146818 RTC (rtc.c,
 * ports 0x70/0x71). This glue lives here so int21.c stays free of port I/O and
 * compiles hosted (Law 3). The seam exchanges flat fields (not rtc_time_t) so
 * int21.c need not include rtc.h. */
static int int21_clock_get_rtc(uint16_t *year, uint8_t *month, uint8_t *day,
                               uint8_t *hour, uint8_t *minute, uint8_t *second,
                               uint8_t *dow)
{
    rtc_time_t t;
    if (!rtc_now(&t)) {
        return 0;   /* RTC unreadable -> int21 falls back to the DOS epoch */
    }
    *year   = t.year;
    *month  = t.month;
    *day    = t.day;
    *hour   = t.hour;
    *minute = t.minute;
    *second = t.second;
    *dow    = t.day_of_week;
    return 1;
}

static int int21_clock_set_rtc(uint16_t year, uint8_t month, uint8_t day,
                               uint8_t hour, uint8_t minute, uint8_t second,
                               uint8_t which)
{
    rtc_time_t t;
    t.year = year; t.month = month; t.day = day;
    t.hour = hour; t.minute = minute; t.second = second; t.day_of_week = 0u;
    /* Translate the int21 SET mask to the rtc mask (same bit layout, but keep
     * them decoupled). */
    uint8_t rmask = 0u;
    if (which & INT21_CLOCK_SET_DATE) rmask |= RTC_SET_DATE;
    if (which & INT21_CLOCK_SET_TIME) rmask |= RTC_SET_TIME;
    return rtc_set(&t, rmask);
}

/* ---- INT 21h VECTOR-TABLE source wiring (beads initech-509.8) -------------
 * AH=25h SETVECT / AH=35h GETVECT reach the live IDT through a seam int21.c
 * binds via int21_set_vectortable, so int21.c does not link idt.c and stays
 * hosted (Law 3). SETVECT installs a flat handler as a 0x8F TRAP gate on the
 * kernel CS (idt_install_trap -- the SAME gate type the DOS handlers use);
 * GETVECT reassembles the flat offset from the gate's split lo/hi fields
 * (idt.h idt_gate_t). */
static void int21_setvect_idt(uint8_t vec, uint32_t handler)
{
    idt_install_trap(vec, (void *)(uintptr_t)handler);
}

static uint32_t int21_getvect_idt(uint8_t vec)
{
    idt_gate_t g = idt_get_gate(vec);
    return (uint32_t)g.offset_lo | ((uint32_t)g.offset_hi << 16);
}

/* Issue AH=09h DISPLAY STRING via a literal `int 0x21`: EDX = flat ptr to a
 * '$'-terminated string. This is the LIVE end-to-end self-test of the syscall
 * path on the real boot -- the banner is printed THROUGH int 0x21, not via a
 * direct console_puts. */
static inline void dos_puts(const char *dollar_terminated)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x0900u), "d"((uint32_t)(uintptr_t)dollar_terminated)
        : "cc", "memory");
}

/* ---- Proto-DIR (beads initech-saw) ----------------------------------------
 * Walk the mounted FAT12 root directory and render a DOS-style "DIR" listing
 * to BOTH the LFB console and COM1 serial. This is the FIRST end-to-end
 * exercise of ata.c + fat12.c on the emulator: no INT 21h, no SFT -- a direct
 * call into the oracle-green FAT reader over the (newly live) ATA backend.
 * Ref: fs-mount-sft-ground-truth.md Sec 5.1.
 *
 * The callback fans each line to the console (when bound) and serial, so the
 * harness can grep the filenames on the wire AND a screendump shows the list. */
static console_t *g_dir_con = 0;

static void dir_putc(char c)
{
    if (g_dir_con) {
        console_putc(g_dir_con, c);
    }
    serial_putc(c);
}

static void dir_puts(const char *s)
{
    while (*s) {
        dir_putc(*s++);
    }
}

/* Emit an unsigned decimal right-justified into a field of `width`, fanned to
 * console+serial (so the size column lines up DOS-style). */
static void dir_putu_field(uint32_t v, int width)
{
    char buf[10];
    int n = 0;
    int pad;

    if (v == 0) {
        buf[n++] = '0';
    } else {
        while (v > 0 && n < (int)sizeof(buf)) {
            buf[n++] = (char)('0' + (v % 10u));
            v /= 10u;
        }
    }
    for (pad = width - n; pad > 0; pad--) {
        dir_putc(' ');
    }
    while (n > 0) {
        dir_putc(buf[--n]);
    }
}

/* fat12_read_root_dir visitor: print one "NAME.EXT    SIZE" line per regular
 * file. Volume labels and subdirectories are skipped (this milestone lists
 * plain files only). Returns 0 to continue enumeration. */
static int dir_visit(const dir_entry_t *e, void *user)
{
    char name[FAT12_NAME83_MAX];
    (void)user;

    /* Skip the volume label and directory entries; list regular files only. */
    if (e->attribute & (DIR_ATTR_VOLLABEL | DIR_ATTR_DIRECTORY)) {
        return 0;
    }
    if (fat12_format_83(e, name) != FAT12_OK) {
        return 0; /* defensive: a malformed entry is skipped, not fatal */
    }

    dir_puts(name);
    dir_putu_field(e->file_size, 13);
    dir_putc('\n');
    return 0;
}

/* ---- EXEC backend glue (beads initech-saw / INT 21h AH=4Bh) ---------------
 * int21's AH=4Bh reaches the FAT-sourced loader through the int21_exec_fn seam
 * (a DOS-error-returning callback). This adapter bridges that to loader.c's
 * load_program_from_fat (which returns a loader_status_t): it runs the named
 * .COM to completion and maps the loader status onto the DOS EXEC error codes.
 * It lives here, not in int21.c, so int21.c stays free of loader.c (Law 3).
 *
 * After the child returns, restore the kernel-context exit hook + PSP (the
 * loader rebinds both during the run; ground-truth Sec 4.5 / beads initech-509.3)
 * exactly as run_baked does -- otherwise a later kernel-context INT 21h would
 * dispatch through the child's stale hook/PSP. */
static uint16_t loader_exec_by_name(const char *name83, uint8_t *out_rc)
{
    uint8_t rc = 0;
    loader_status_t st = load_program_from_fat(name83, (const char *)0, 0u, &rc);
    int21_set_exit(int21_exit_hook);  /* restore kernel-context terminate */
    int21_set_psp(&g_kernel_psp);     /* restore kernel-context JFT */

    switch (st) {
        case LOADER_OK:
            if (out_rc) {
                *out_rc = rc;
            }
            return 0u;                          /* success */
        case LOADER_ERR_NO_VOLUME:
        case LOADER_ERR_NOT_FOUND:
            return INT21_ERR_FILE_NOT_FOUND;    /* 0x0002 */
        case LOADER_ERR_BUSY:
            return INT21_ERR_INSUFFICIENT_MEM;  /* 0x0008 -- nested EXEC (deferred) */
        case LOADER_ERR_TOO_BIG:
        case LOADER_ERR_ZERO_LEN:
            return INT21_ERR_BAD_FORMAT;        /* 0x000B -- bad/oversized image */
        case LOADER_ERR_READ:
        default:
            return INT21_ERR_FILE_NOT_FOUND;    /* read error reads as absent */
    }
}

/* Run a baked flat program through the loader, framed with serial markers
 * `<tag>-BEGIN` / `<tag>-EXIT rc=NN` / `<tag>-LOAD-FAIL st=NN` so a boot oracle
 * can assert load->run->return. Restores the kernel-context exit hook + PSP
 * after the run (the loader rebinds both during a program; ground-truth Sec
 * 4.5 / beads initech-509.3). Shared by the demo / TYPE / DIR programs. */
/* __attribute__((unused)): the SHELL build (initech-k6x) compiles none of the
 * baked-program call sites (demos gated #ifndef BOOT_SHELL; the EXEC/WRITE/...
 * self-tests #ifdef'd out), so run_baked is legitimately unused there. */
__attribute__((unused))
static void run_baked(const char *tag, const uint8_t *image, uint32_t image_len)
{
    serial_puts(tag);
    serial_puts("-BEGIN\n");

    uint8_t rc = 0;
    loader_status_t st = load_program(image, image_len, (const char *)0, 0u, &rc);
    int21_set_exit(int21_exit_hook);  /* restore kernel-context terminate */
    int21_set_psp(&g_kernel_psp);     /* restore kernel-context JFT (509.3) */

    if (st == LOADER_OK) {
        serial_puts(tag);
        serial_puts("-EXIT rc=");
        serial_putu((uint32_t)rc);
        serial_putc('\n');
    } else {
        /* Fail loud (Rule 2): the program was rejected before it ran. */
        serial_puts(tag);
        serial_puts("-LOAD-FAIL st=");
        serial_putu((uint32_t)st);
        serial_putc('\n');
    }
}

void kernel_main(void)
{
    /* Marker: C kernel entered. This is the acceptance signal for the handoff
     * (ground-truth Sec 4.1) -- a missing KERNEL means the far jump failed. */
    serial_puts("KERNEL\n");

    /* Read the handoff block at its fixed physical address. Volatile so the
     * compiler cannot assume anything about memory it did not write. */
    const volatile boot_info_t *bi = (const volatile boot_info_t *)
        (uintptr_t)BOOT_INFO_ADDR;

    /* Snapshot to plain locals (the struct is small + stable after handoff). */
    boot_info_t b;
    b.lfb_addr   = bi->lfb_addr;
    b.lfb_pitch  = bi->lfb_pitch;
    b.lfb_bpp    = bi->lfb_bpp;
    b.lfb_width  = bi->lfb_width;
    b.lfb_height = bi->lfb_height;
    b.font_addr  = bi->font_addr;

    /* Sanity-check the handoff struct (Rule 2 -- fail loud, but stay live so
     * the screendump still captures). Emit BI-OK on success, or BI-BAD + the
     * offending value so a wrong field is diagnosable on serial. */
    int ok = 1;
    if (!(b.lfb_bpp == 8 || b.lfb_bpp == 24 || b.lfb_bpp == 32)) {
        serial_puts("BI-BAD bpp=");   serial_putu(b.lfb_bpp);    serial_putc('\n');
        ok = 0;
    }
    if (b.lfb_addr == 0) {
        serial_puts("BI-BAD addr=0\n");
        ok = 0;
    }
    /* Two valid geometries: the 640x480 VBE LFB (QEMU / real VESA) or the
     * 320x200 standard-VGA mode-0x13 fallback (initech-6pj; e.g. Bochs). */
    if (!((b.lfb_width == 640 && b.lfb_height == 480) ||
          (b.lfb_width == 320 && b.lfb_height == 200))) {
        serial_puts("BI-BAD geom w="); serial_putu(b.lfb_width);
        serial_puts(" h=");            serial_putu(b.lfb_height); serial_putc('\n');
        ok = 0;
    }
    if (ok) {
        serial_puts("BI-OK\n");
    }

    /* SYSINIT PHASE 1 -- interrupt + syscall foundation (beads initech-509.2;
     * beads initech-a5a / initech-509.5 / initech-509.3). This was a long ad-hoc
     * sequence inline in kernel_main; it is now the NAMED sysinit_early() phase,
     * which runs the EXACT same steps IN THE EXACT same order (sysinit.h documents
     * the ordering minefield): remap+MASK the 8259 -> "PIC" -> install the IDT ->
     * "IDT" -> bind the int21 CON sink + terminate hook -> sft_init + build the
     * kernel-context PSP + int21_set_psp -> install the 0x21 + 0x20 trap gates ->
     * "INT21". IF stays 0 here (no sti); the HW-IRQ tail is sysinit_enable_irqs()
     * far below, in its original position. The serial markers are emitted from the
     * phase so the capture is byte-for-byte identical. */
    sysinit_early(int21_con_sink, int21_exit_hook, &g_kernel_psp, serial_puts);

    /* LIVE self-test: install a trap gate on an UNUSED vector (0x80 -- not an
     * IRQ vector after the 0x28/0x30 remap), fire `int 0x80`, and confirm
     * control returns. Proves gate-install + dispatch + iret + resume on the
     * real boot (the host oracle proves the byte-level encode; this proves the
     * CPU actually honors it). The handler emits IDT-SELFTEST-OK; IDT-RESUMED
     * after the int proves the iret resumed us. */
    idt_install_trap(0x80u, (void *)isr_selftest);
    __asm__ __volatile__("int $0x80");
    serial_puts("IDT-RESUMED\n");

    /* Mode 0x13 fallback (initech-6pj): the framebuffer is paletted, so map the
     * console's two indices -- CONSOLE_BG_IDX -> seafoam, CONSOLE_FG_IDX ->
     * light gray (0xC0, matching console.c CONSOLE_FG_*) -- in the VGA DAC
     * BEFORE any fill or glyph blit. The direct-color VBE 24/32bpp paths skip
     * this. */
    if (b.lfb_bpp == 8) {
        vga_set_dac((uint8_t)CONSOLE_BG_IDX, SEAFOAM_R, SEAFOAM_G, SEAFOAM_B);
        vga_set_dac((uint8_t)CONSOLE_FG_IDX, 0xC0, 0xC0, 0xC0);
    }

    /* CONSTRAINT (initech-d00): the C kernel's first drawing act re-paints the
     * LFB seafoam so test-tracer-boot's screendump assertion still passes --
     * the handoff is now proven on the REAL boot chain end-to-end. */
    fb_clear(&b, SEAFOAM_R, SEAFOAM_G, SEAFOAM_B);

    /* Text console (beads initech-yqb): bind the framebuffer + the captured
     * VGA ROM font (b.font_addr) and render a SHORT test string. The console's
     * default background is SEAFOAM (console.c CONSOLE_BG_*), so console_init's
     * clear repaints the SAME seafoam -- the screen stays predominantly seafoam
     * and test-tracer-boot's 81-point screendump grid still reads seafoam (Law
     * 2 -- do not weaken the oracle to ship). Glyphs are inked on top in fg
     * (light gray). This is NOT the InitechDOS banner -- that is initech-bea.
     *
     * Fail loud (Rule 2): if console_init refuses (NULL/zero addr/bad bpp/no
     * font) emit CON-BAD + the code and skip drawing; the seafoam fill above
     * still satisfies the screendump so the handoff gate stays diagnosable. */
    /* `con` MUST live for the rest of kernel_main: g_int21_con / g_dir_con / the
     * panic console all hold &con and are dereferenced by the program demo and
     * the proto-DIR far below. A block-local here would dangle once the block
     * ends (its stack is reused by the loader/mount locals), so the console is
     * declared at function scope. Root-cause fix (Rule 3): a nested-scope `con`
     * rendered the banner by luck but the proto-DIR through clobbered state. */
    console_t con;
    {
        int crc = console_init(&con, &b);
        if (crc == CONSOLE_OK) {
            serial_puts("CONSOLE\n");

            /* Bind the live console so a CPU-exception panic can render its
             * one visible line (beads initech-a5a / panic.c). */
            panic_set_console(&con);

            /* Bind the SAME console to the INT 21h CON sink so `int 0x21` AH=09h
             * (below) drives the LFB console + serial (beads initech-509.5). */
            g_int21_con = &con;

            /* Print the InitechDOS operator banner -- the milestone (beads
             * initech-bea). The two LOCKED lines (spec/dos_banner.txt; ADR-0003
             * DEC-12 / Appendix D.1) at the top-left, each ending '\n'. The
             * console default bg is SEAFOAM (console.c), so the screen stays
             * predominantly seafoam with the banner inked on top in fg.
             *
             * ALSO mirror the banner to COM1, framed by BANNER-BEGIN/END
             * markers, so the harness has a strong literal serial signal
             * ("InitechDOS  Version 3.30" on the wire) and the test-boot gate
             * can diff the captured lines against spec/dos_banner.txt
             * byte-for-byte (Law 1 / Rule 8). */
            serial_puts("BANNER-BEGIN\n");

            /* Drive the banner THROUGH the new syscall path (beads
             * initech-509.5): `int 0x21` AH=09h with EDX -> a '$'-terminated
             * copy of each line. The dispatcher's CON sink fans every byte to
             * BOTH the LFB console and COM1 serial, so the screen output is
             * unchanged (test-tracer-boot/test-boot screendump stays green) AND
             * the serial bytes between BANNER-BEGIN/BANNER-END still match
             * spec/dos_banner.txt byte-for-byte. The '$' is the DOS terminator
             * and is NOT emitted; the '\n' before it IS. */
            dos_puts(BANNER_LINE1 "\n$");
            dos_puts(BANNER_LINE2 "\n$");

            serial_puts("BANNER-END\n");
            serial_puts("BANNER\n");
        } else {
            serial_puts("CON-BAD rc=");
            serial_putu((uint32_t)crc);
            serial_putc('\n');
        }
    }

#ifdef BOOT_SELFTEST_FAULT
    /* SELF-TEST BUILD ONLY (beads initech-a5a; make test-panic). Deliberately
     * trigger a CPU exception AFTER the banner to prove the panic path fires on
     * a REAL fault instead of triple-faulting (the minefield: a triple-fault
     * silently reboots in QEMU). #DE (vector 0): volatile divide-by-zero so the
     * optimizer cannot fold it away. The panic handler dumps "PANIC vec=00" to
     * serial and halts; the oracle asserts that marker AND triple_fault=0. The
     * NORMAL kernel image never defines this macro, so test-boot stays green. */
    serial_puts("SELFTEST-FAULT-ARMED\n");
    {
        volatile int z = 0;
        volatile int q = 1;
        q = q / z;            /* #DE -- divide error, vector 0 */
        serial_putu((uint32_t)q);  /* unreachable; defeats dead-store elision */
    }
#endif

    /* MOUNT A REAL FILESYSTEM (beads initech-saw) FIRST, so the file-handle
     * INT 21h functions (3Dh OPEN / 3Fh READ / 4Eh/4Fh FINDFIRST/NEXT) have a
     * bound volume BEFORE the TYPE/DIR programs run. Attach the FAT12 data disk
     * on the IDE primary SLAVE, read sector 0 via the ATA PIO backend (its FIRST
     * live run on the emulator -- beads initech-adf), mount it, render a proto-
     * DIR, and bind the FAT12 file backend into int21 (beads initech-509.5).
     *
     * The volume + its ATA ctx/blockdev live at FUNCTION scope (not a nested
     * block) because fileio_fat caches a pointer to `vol` that the TYPE/DIR
     * programs dereference through int21's OPEN/FINDFIRST -- a block-local would
     * dangle once the block ended (the same dangling-state lesson as `con`).
     *
     * Fail loud (Rule 2): on any failure -- no drive (floating bus), bad boot
     * signature, ATA error -- emit "FAT-MOUNT-FAIL rc=NN" and CONTINUE (do NOT
     * hang). The ATA floating-bus guard guarantees mount returns an error when
     * no data disk is attached, so a boot WITHOUT --disk2 still reaches the
     * programs (TYPE then reports TYPE-OPEN-FAIL; DIR finds nothing) and the
     * halt loop. Ref: fs-mount-sft-ground-truth.md Sec 5.1. */
    static uint8_t sector_buf[BLOCKDEV_SECTOR_SIZE]; /* caller scratch (BSS) */
    ata_ctx_t      ata_slave;
    blockdev_t     fatdev;
    fat12_volume_t vol;
    int            mounted = 0;

    serial_puts("FAT-MOUNT-BEGIN\n");
    {
        int rc;

        /* Fan the proto-DIR to the live console too (the SAME console the
         * banner used, already bound via g_int21_con). */
        g_dir_con = g_int21_con;

        ata_ctx_init_primary_slave(&ata_slave);
        ata_blockdev_init(&fatdev, &ata_slave);

        rc = fat12_mount(&vol, &fatdev, sector_buf);
        if (rc == FAT12_OK) {
            serial_puts("FAT-MOUNT-OK\n");
            mounted = 1;

            /* Proto-DIR: a header line, then one NAME.EXT + size per file. */
            dir_puts("Directory of A:\\\n");
            rc = fat12_read_root_dir(&vol, sector_buf, dir_visit, (void *)0);
            if (rc == FAT12_OK) {
                serial_puts("DIR-OK\n");
            } else {
                serial_puts("DIR-FAIL rc=");
                serial_puti((int32_t)rc);
                serial_putc('\n');
            }

            /* Bind the FAT12 file backend (beads initech-509.5): cache the FAT
             * and hand int21 the OPEN/CLOSE/dir-entry vtable, so the TYPE/DIR
             * programs below resolve real files through this volume. */
            rc = fileio_fat_bind(&vol);
            if (rc == 0) {
                serial_puts("FILEIO-BIND-OK\n");
            } else {
                serial_puts("FILEIO-BIND-FAIL rc=");
                serial_puti((int32_t)rc);
                serial_putc('\n');
            }

            /* Bind the SAME volume to the loader so AH=4Bh EXEC / the saw path
             * (load_program_from_fat) can load a .COM BY NAME off this disk, and
             * bind the EXEC backend seam so int21's AH=4Bh reaches the loader
             * (beads initech-saw). load_program_from_fat caches the FAT here; a
             * read error leaves it unbound (fail loud). */
            loader_bind_fat_volume(&vol);
            int21_set_exec_backend(loader_exec_by_name);
            serial_puts("LOADER-FAT-BIND-OK\n");
        } else {
            /* Fail loud + continue (do NOT hang). A missing disk yields
             * ATA_ERR_NO_DRIVE propagated as FAT12_ERR_READ. */
            serial_puts("FAT-MOUNT-FAIL rc=");
            serial_puti((int32_t)rc);
            serial_putc('\n');
        }
    }
    serial_puts("FAT-MOUNT-END\n");
    (void)mounted;

    /* Bind the INT 21h CLOCK source to the live MC146818 RTC (beads
     * initech-yv9). Polled CMOS reads (ports 0x70/0x71) -- NO IRQ dependency, so
     * we bind it HERE (before any baked program runs) rather than near the late
     * conin bind. From here AH=2Ah/2Bh/2Ch/2Dh read + set the real RTC; under the
     * harness's pinned `-rtc base` the readings are deterministic. */
    int21_set_clock(int21_clock_get_rtc, int21_clock_set_rtc);
    serial_puts("CLOCK-LIVE\n");

    /* Bind the INT 21h vector-table seam to the live IDT (beads initech-509.8)
     * so AH=25h SETVECT / AH=35h GETVECT read+write real IDT gates. The DOS
     * handler gates (0x20-0x24) are already installed by sysinit_early; this just
     * lets a program query/replace any vector via INT 21h. */
    int21_set_vectortable(int21_setvect_idt, int21_getvect_idt);
    serial_puts("VECT-LIVE\n");

    /* SYSINIT PHASE 2 -- CONFIG.SYS apply (beads initech-509.2). THE hook point:
     * after the FAT volume is mounted + bound (so CONFIG.SYS can be read off it)
     * and BEFORE any program loads (so the FILES= cap is in force for the demos +
     * the shell). Read CONFIG.SYS from the volume if present; else fall back to
     * the LOCKED baseline (spec/dos_config_sys_baseline.txt, embedded in
     * sysinit.c). Apply the honorable directives -- FILES=N -> the runtime SFT cap
     * (the one with teeth; sft_set_files_limit) -- and emit the "SYSINIT: ..."
     * serial summary the in-emulator oracle asserts. A FAT-sized scratch (>= one
     * 1.44 MB FAT == 9*512) caches the FAT for the file read; sector_buf is the
     * 512-byte dir-scan scratch (reused from the mount above). */
    {
        /* Reuse the file backend's already-cached FAT (fileio_fat_bind above)
         * rather than a SECOND ~4.6 KiB .bss buffer: the kernel .bss was butting
         * against PROGRAM_BASE (_kernel_end within ~160 bytes of the old 0x20000
         * window), so a dedicated cfg_fat_buf risked a kernel/program collision
         * (beads initech-509.2 fix). The window has since been raised to 0x30000
         * (beads initech-5pe), but reusing the resident FAT is still the right
         * call. The FAT is already resident from the bind. */
        uint32_t cfg_fat_len = 0u;
        void    *cfg_fat = fileio_fat_fat_buffer(&cfg_fat_len);
        (void)sysinit_apply_config(mounted ? &vol : (const fat12_volume_t *)0,
                                   sector_buf, cfg_fat, cfg_fat_len, serial_puts);
    }

    /* RUN A PROGRAM (beads initech-509.5). Lay out a PSP + a baked flat program
     * at PROGRAM_BASE/PROGRAM_IMAGE, JMP to it, let it call INT 21h, and regain
     * control on AH=4Ch -- the return-to-loader mechanism (ground-truth Sec 4).
     * Output fans to the LFB console + serial through the int21 CON sink.
     *
     * Three programs, in sequence (each framed by run_baked markers):
     *   1. the original demo (PROGRAM-*) -- AH=09h DISPLAY STRING + AH=4Ch;
     *      asserted by make test-program (no data disk required).
     *   2. TYPE (TYPE-*) -- OPEN HELLO.TXT, READ, WRITE to stdout, CLOSE; the
     *      file's contents reach serial+console; asserted by make test-type
     *      (requires --disk2; without it the program prints TYPE-OPEN-FAIL).
     *   3. DIR (DIR-PROG-*) -- FINDFIRST/FINDNEXT *.*, write each name; asserted
     *      by make test-dir (requires --disk2). */
    /* The baked PROGRAM/TYPE/DIR demos are M2 SCAFFOLDING, asserted by
     * test-program/test-type/test-dir on the DEMO image. They are gated OUT of
     * the SHELL build (initech-k6x): the real default boot drops to the
     * COMMAND.COM A:\> prompt right after the banner + proto-DIR (authentic
     * DOS), not after a parade of demos. The other self-test variants (EXEC /
     * WRITE / ... -- not BOOT_SHELL) still run these so their flow is unchanged. */
#ifndef BOOT_SHELL
    run_baked("PROGRAM", g_test_prog_image, g_test_prog_image_len);
    serial_puts("TYPE-OUTPUT-BEGIN\n");
    run_baked("TYPE", g_type_prog_image, g_type_prog_image_len);
    serial_puts("TYPE-OUTPUT-END\n");
    serial_puts("DIR-PROG-OUTPUT-BEGIN\n");
    run_baked("DIR-PROG", g_dir_prog_image, g_dir_prog_image_len);
    serial_puts("DIR-PROG-OUTPUT-END\n");
#endif

#ifdef BOOT_EXEC
    /* FAT-SOURCED LOAD + EXEC self-test (beads initech-saw; make test-exec).
     * Only in the -DBOOT_EXEC image so the normal boot is unchanged. Requires
     * --disk2 (the FAT12 data disk carrying GREET.COM). Two proofs:
     *
     *   1. THE SAW CORE: call load_program_from_fat("GREET.COM", ...) DIRECTLY.
     *      This proves a program was loaded FROM THE FAT VOLUME (not baked) and
     *      ran -- GREET.COM prints "GREETINGS FROM A:GREET.COM" via AH=09h and
     *      exits rc=7. Markers EXEC-SAW-BEGIN / EXEC-SAW-EXIT rc=NN frame it.
     *      Restore the kernel-context hook+PSP after (the loader rebinds them).
     *
     *   2. INT 21h AH=4Bh EXEC of "GREET.COM" from KERNEL/shell context, then
     *      AH=4Dh GET-RETURN-CODE -- proving the EXEC dispatch path + the child
     *      rc retrieval. (Nested EXEC -- 4Bh from inside a running program -- is
     *      NOT supported this milestone: g_loader_ctx is single-level; the guard
     *      returns LOADER_ERR_BUSY -> AX=0x0008. So EXEC-from-kernel is the
     *      binding oracle, per the brief.) */
    serial_puts("EXEC-SAW-BEGIN\n");
    {
        uint8_t saw_rc = 0;
        loader_status_t st = load_program_from_fat("GREET.COM",
                                                   (const char *)0, 0u, &saw_rc);
        int21_set_exit(int21_exit_hook);   /* restore kernel-context terminate */
        int21_set_psp(&g_kernel_psp);      /* restore kernel-context JFT */
        if (st == LOADER_OK) {
            serial_puts("EXEC-SAW-EXIT rc=");
            serial_putu((uint32_t)saw_rc);
            serial_putc('\n');
        } else {
            serial_puts("EXEC-SAW-FAIL st=");
            serial_putu((uint32_t)st);
            serial_putc('\n');
        }
    }

    /* AH=4Bh EXEC + AH=4Dh GET-RETURN-CODE via literal `int 0x21` from kernel
     * context (EDX -> "GREET.COM"). The dispatcher routes 4Bh through the EXEC
     * backend (loader_exec_by_name) which restores hook/PSP internally. */
    serial_puts("EXEC-4B-BEGIN\n");
    {
        static const char greet_path[] = "GREET.COM";
        uint32_t ret_ax = 0x4B00u;        /* AH=4Bh, AL=00h (load+execute) in/out */
        uint32_t carry  = 0;
        /* EDX = flat ptr to the path. Capture CF with `sbb` (carry -> -1/0) so the
         * asm needs only EAX + ECX (the path goes in EDX); keep the operand list
         * lean to avoid over-constraining the i386 register file. */
        __asm__ __volatile__(
            "int $0x21\n\t"
            "sbb %1, %1\n\t"              /* carry = (CF) ? 0xFFFFFFFF : 0 */
            : "+a"(ret_ax), "=c"(carry)
            : "d"((uint32_t)(uintptr_t)greet_path)
            : "cc", "memory");
        if (carry != 0u) {
            serial_puts("EXEC-4B-FAIL ax=");
            serial_putu(ret_ax & 0xFFFFu);
            serial_putc('\n');
        } else {
            /* AH=4Dh GET-RETURN-CODE: AL = the child's exit code (expect 7). */
            uint32_t rc_ax = 0;
            __asm__ __volatile__(
                "int $0x21\n\t"
                : "=a"(rc_ax)
                : "a"(0x4D00u)
                : "cc", "memory");
            serial_puts("EXEC-4B-RC=");
            serial_putu(rc_ax & 0xFFu);
            serial_putc('\n');
        }
    }
    serial_puts("EXEC-4B-END\n");
#endif

#ifdef BOOT_WRITE
    /* FAT WRITE round-trip self-test (beads initech-509.11; make test-fatwrite).
     * Only in the -DBOOT_WRITE image so the normal boot is unchanged. Requires a
     * WRITABLE FAT12 data disk on --disk2. Run the baked WRITE program: it
     * CREATs OUT.TXT, WRITEs "hello\r\n", CLOSEs (flush to disk), then re-OPENs +
     * READs OUT.TXT and echoes it to stdout -- proving write+read round-trips
     * THROUGH the OS on real ATA in one boot. The harness asserts the echoed
     * "hello" between WRITE-OUTPUT-BEGIN/END + WRITE-EXIT rc=0 + triple_fault=0. */
    serial_puts("WRITE-OUTPUT-BEGIN\n");
    run_baked("WRITE", g_write_prog_image, g_write_prog_image_len);
    serial_puts("WRITE-OUTPUT-END\n");
#endif

#ifdef BOOT_MULTIOPEN
    /* MULTI-OPEN capability self-test (beads initech-0qh; epic initech-6qy; make
     * test-multiopen). Only in the -DBOOT_MULTIOPEN image so the normal boot is
     * unchanged. Requires a FAT12 data disk on --disk2 carrying HELLO.TXT +
     * SECOND.TXT + a >64 KiB BIG.DAT. Run the baked MULTI-OPEN program: it OPENs
     * HELLO.TXT and SECOND.TXT (and BIG.DAT) CONCURRENTLY, does interleaved
     * positioned reads with LSEEK on both (proving independent per-handle
     * positions, no cross-talk), and LSEEKs PAST 64 KiB into BIG.DAT to read a
     * signature back (the old whole-file 64 KiB buffer could never hold it). The
     * harness asserts MO-A1=Hel / MO-B1=Sec / MO-A2=Hello / MO-BIG=BEYOND-64KiB!
     * between the markers + MULTIOPEN-EXIT rc=0 + triple_fault=0. */
    serial_puts("MULTIOPEN-OUTPUT-BEGIN\n");
    run_baked("MULTIOPEN", g_multiopen_prog_image, g_multiopen_prog_image_len);
    serial_puts("MULTIOPEN-OUTPUT-END\n");
#endif

#ifdef BOOT_DATETIME
    /* DATE/TIME + FREE-SPACE + PSP capability self-test (beads initech-yv9; make
     * test-datetime). Only in the -DBOOT_DATETIME image so the normal boot is
     * unchanged. The harness boots this with a PINNED RTC (-rtc base=
     * 2026-06-09T12:34:56) and a FAT12 data disk on --disk2. The baked program
     * issues AH=2Ah GET DATE, AH=2Ch GET TIME, AH=36h GET DISK FREE SPACE, and
     * AH=62h GET PSP and writes the decoded results to serial. The harness
     * asserts the EXACT pinned date (2026-06-09, Tuesday) + time (12:34:56) +
     * free space > 0 + PSP nonzero. The CLOCK seam is already bound (CLOCK-LIVE)
     * above, so AH=2Ah/2Ch read the real (pinned) MC146818 RTC. */
    serial_puts("DATETIME-OUTPUT-BEGIN\n");
    run_baked("DATETIME", g_datetime_prog_image, g_datetime_prog_image_len);
    serial_puts("DATETIME-OUTPUT-END\n");
#endif

#ifdef BOOT_EXITH
    /* EXIT-HANDLE TEARDOWN self-test (beads initech-6hk; epic initech-6qy; make
     * test-exit-handles). Only in the -DBOOT_EXITH image so the normal boot is
     * unchanged. Requires --disk2 carrying EXITH.COM + HELLO.TXT/SECOND.TXT/
     * CHAIN.TXT/BLOCK.BIN.
     *
     * THE LEAK PROOF: EXITH.COM OPENs FOUR files and TERMINATES (4Ch) WITHOUT
     * closing them -- 4 SFT file slots consumed per run. We EXEC it RUNS times
     * (RUNS * 4 > the 16 file slots, SFT_FIRST_FILE..SFT_MAX_ENTRIES). If EXIT
     * did NOT reclaim a process's handles, the leaked slots accumulate across
     * runs and by run 5 the table is exhausted -- the child's 5th-run OPEN
     * returns CF=1 and it prints EXITH-CHILD-OPENFAIL + exits rc=1. WITH the
     * teardown (sft_close_process on do_terminate) every run starts with a clean
     * file table, so all 6 runs' OPENs succeed (EXITH-CHILD-OK) and exit rc=0.
     *
     * We drive the EXEC loop from KERNEL context via load_program_from_fat (the
     * saw core), restoring the kernel hook + PSP after each child (the loader
     * rebinds both during a run), exactly as the BOOT_EXEC saw block does. Each
     * run emits EXITH-RUN<n>-OK on a clean rc=0 child, or EXITH-RUN<n>-FAIL with
     * the loader status / child rc on any failure (fail loud, Rule 2). A final
     * EXITH-DONE rc=0 marks a clean sweep; the harness asserts every run is OK. */
    serial_puts("EXITH-OUTPUT-BEGIN\n");
    {
        const int RUNS = 6;   /* 6 * 4 opens = 24 > 16 file slots */
        int run;
        int all_ok = 1;
        for (run = 1; run <= RUNS; run++) {
            uint8_t rc = 0;
            loader_status_t st = load_program_from_fat("EXITH.COM",
                                                       (const char *)0, 0u, &rc);
            int21_set_exit(int21_exit_hook);  /* restore kernel-context terminate */
            int21_set_psp(&g_kernel_psp);     /* restore kernel-context JFT */

            serial_puts("EXITH-RUN");
            serial_putu((uint32_t)run);
            if (st == LOADER_OK && rc == 0u) {
                serial_puts("-OK\n");
            } else {
                serial_puts("-FAIL st=");
                serial_putu((uint32_t)st);
                serial_puts(" rc=");
                serial_putu((uint32_t)rc);
                serial_putc('\n');
                all_ok = 0;
            }
        }
        serial_puts("EXITH-DONE rc=");
        serial_putu(all_ok ? 0u : 1u);
        serial_putc('\n');
    }
    serial_puts("EXITH-OUTPUT-END\n");
#endif

#ifdef BOOT_SYSINIT
    /* CONFIG.SYS FILES= CAP self-test (beads initech-509.2; make test-sysinit).
     * Only in the -DBOOT_SYSINIT image so the normal boot is unchanged. SYSINIT
     * Phase 2 (above) read CONFIG.SYS off --disk2 (minted with FILES=8 -> a 4-slot
     * file SFT) and emitted the "SYSINIT: source=CONFIG.SYS FILES=8 ..." summary.
     * Here we EXEC SYSI.COM, which OPENs HELLO.TXT over and over WITHOUT closing,
     * counting how many OPENs succeed before the SFT exhausts at the cap. It
     * reports SYSINIT-PROG-OPENS=<n>; the oracle asserts n == 4 (the cap bites at
     * exactly the limit -- FILES=8 -> slots 4..7). Loaded BY NAME from FAT (the
     * saw core), restoring the kernel hook + PSP after (the loader rebinds both). */
    serial_puts("SYSINIT-OUTPUT-BEGIN\n");
    {
        uint8_t rc = 0;
        loader_status_t st = load_program_from_fat("SYSI.COM",
                                                   (const char *)0, 0u, &rc);
        int21_set_exit(int21_exit_hook);  /* restore kernel-context terminate */
        int21_set_psp(&g_kernel_psp);     /* restore kernel-context JFT */
        if (st == LOADER_OK) {
            serial_puts("SYSINIT-PROG-EXIT rc=");
            serial_putu((uint32_t)rc);
            serial_putc('\n');
        } else {
            serial_puts("SYSINIT-PROG-FAIL st=");
            serial_putu((uint32_t)st);
            serial_putc('\n');
        }
    }
    serial_puts("SYSINIT-OUTPUT-END\n");
#endif

    /* ---- ENABLE HARDWARE INTERRUPTS (beads initech-3rs) -------------------
     * SYSINIT PHASE 3 -- the FIRST `sti` of the whole project, now the NAMED
     * sysinit_enable_irqs() phase. Up to here the kernel ran with IF=0 and every
     * IRQ masked; software ints (0x20/0x21/0x80) dispatched through the IDT with
     * IF=0. The phase runs the EXACT same steps IN THE EXACT same order: program
     * the 8254 PIT (100 Hz) -> init the PS/2 keyboard ring -> install the IRQ0
     * (0x28) + IRQ1 (0x29) interrupt gates -> unmask ONLY IRQ0+IRQ1 -> `sti` ->
     * "IRQ-LIVE".
     *
     * Order matters (the minefield -- CLAUDE.md "real mode -> protected is a
     * minefield" / Rule 5; sysinit.h): program the devices and install the gates
     * BEFORE unmasking, and unmask BEFORE sti, so a line can never fire into an
     * uninstalled gate (which would hit the spurious stub and panic).
     *
     * Placed AFTER the boot demos (its ORIGINAL position) so the existing gates
     * (banner / program / mount / proto-DIR / TYPE / DIR) run with the SAME IF=0
     * flow as before -- enabling interrupts here changes none of their behavior
     * (the keyboard ring simply stays empty in those gates). Ref: 8254/8042/8259A.
     *
     * REENTRANCY (beads initech-xk2): the IRQ handlers touch ONLY pit/kbd state,
     * never the INT 21h dispatcher globals, so an IRQ landing inside an INT 21h
     * trap (0x8F keeps IF set) is safe with no lock. */
    sysinit_enable_irqs(serial_puts);

    /* Bind the INT 21h CON input source to the now-live keyboard (beads
     * initech-n62). AFTER sti so the blocking get's `hlt` is actually woken by
     * IRQ1. From here, AH=01h/06h/07h/08h/0Ah/0Bh/0Ch read real keystrokes. */
    int21_set_conin(int21_conin_get_kbd, int21_conin_poll_kbd);
    serial_puts("CONIN-LIVE\n");
    /* (The CLOCK source was bound earlier, right after FAT-MOUNT-END -- it has no
     * IRQ dependency, beads initech-yv9.) */

#ifdef BOOT_KBD_ECHO
    /* ECHO SELF-TEST BUILD ONLY (beads initech-3rs / initech-43b; make
     * test-kbd). This is the BITING end-to-end oracle for BOTH the keyboard
     * driver and the harness --keys injection: emit a serial marker the harness
     * waits for, then echo every ASCII byte the IRQ1 path enqueues back to
     * serial framed by BEGIN/END markers, until N chars have been echoed, then
     * halt. The NORMAL image never defines this macro, so test-boot et al. are
     * unaffected. We `hlt` between polls so the CPU sleeps until the next IRQ
     * (PIT tick or a keystroke) -- proves IF is actually set and IRQ1 fires. */
    serial_puts("KBD-ECHO-READY\n");
    serial_puts("KBD-ECHO-BEGIN\n");
    {
        int echoed = 0;
        const int want = 3;          /* the test injects "d,i,r" -> 3 chars */
        while (echoed < want) {
            int ch = kbd_getchar();
            if (ch < 0) {
                __asm__ __volatile__("hlt");  /* sleep until an IRQ wakes us */
                continue;
            }
            serial_putc((char)ch);   /* echo the decoded ASCII to serial */
            echoed++;
        }
    }
    serial_puts("\nKBD-ECHO-END\n");
    for (;;) {
        __asm__ __volatile__("hlt");
    }
#elif defined(BOOT_SHELL)
    /* COMMAND.COM INTERACTIVE SHELL BUILD ONLY (beads initech-7pc; make
     * test-shell). The M2 capstone: instead of the demo+halt, enter the resident
     * COMMAND.COM REPL. It prints the $P$G prompt (A:\>), reads a line via INT 21h
     * AH=0Ah, and dispatches DIR / TYPE / CD / CLS / VER / ECHO / EXIT + external
     * .COM EXEC -- all by issuing REAL `int 0x21` calls (command.c dogfoods the
     * OS API, the authentic COMMAND.COM design). The blocking AH=0Ah can only make
     * progress because the oracle injects keys via QMP --keys (gated on
     * SHELL-READY); with no injection the prompt appears but no command runs (the
     * test goes RED). The NORMAL image never defines BOOT_SHELL, so test-boot et
     * al. (which run WITHOUT key injection) are unaffected and never block.
     *
     * Making the shell the DEFAULT boot + migrating the demo gates is a SEPARATE
     * downstream task (see the follow-up bead). Ref: ADR-0003 DEC-11/DEC-12. */
    serial_puts("SHELL-READY\n");
    command_repl();
    serial_puts("SHELL-DONE\n");   /* the REPL returned via EXIT (clean) */
    for (;;) {
        __asm__ __volatile__("hlt");
    }
#elif defined(BOOT_IRQSTORM)
    /* IRQ-STORM REENTRANCY SELF-TEST BUILD ONLY (beads initech-xk2; make
     * test-int21-irqstorm). THE binding oracle for the INT 21h reentrancy guard:
     * IRQs are LIVE here (sti above), so the free-running 100 Hz PIT (IRQ0) and
     * injected keystrokes (IRQ1) fire WHILE the baked IRQ-STORM program drives the
     * INT 21h dispatcher functions that USE its global state -- FINDFIRST/NEXT
     * (g_dta + g_find), a multi-cluster positioned READ (the FAT cache + cluster
     * scratch over slow ATA PIO, so a PIT IRQ WILL land mid-read), and a SECOND
     * concurrent open handle. Because INT 21h is a 0x8F TRAP gate (IF stays set),
     * an IRQ CAN land mid-syscall; the guard (irq.c / int21.c) makes a forbidden
     * ISR-issued reentry FAIL LOUD, and the harness asserts the enumeration order
     * + the read signature come back EXACTLY right (no async corruption).
     *
     * Emit IRQSTORM-READY so the harness injects the keystroke storm (--keys-after
     * IRQSTORM-READY), then run the program. The blocking-free program makes
     * progress on its own (it reads files, not the keyboard); the storm overlaps
     * its ATA reads. The NORMAL image never defines BOOT_IRQSTORM. */
    serial_puts("IRQSTORM-READY\n");
    serial_puts("IRQSTORM-OUTPUT-BEGIN\n");
    run_baked("IRQSTORM", g_irqstorm_prog_image, g_irqstorm_prog_image_len);
    serial_puts("IRQSTORM-OUTPUT-END\n");
    for (;;) {
        __asm__ __volatile__("hlt");
    }
#elif defined(BOOT_CONIN)
    /* CON-INPUT SELF-TEST BUILD ONLY (beads initech-n62; make test-conin). Run
     * the baked CON-input program: it prints a prompt, reads a LINE via INT 21h
     * AH=0Ah BUFFERED INPUT (which blocks on the keyboard -- the harness injects
     * the keys), then writes "CONIN-LINE=<line>" back through AH=40h. This is the
     * BITING end-to-end oracle that a real program reads a line from the keyboard
     * via the INT 21h CON input path. The blocking AH=0Ah can ONLY make progress
     * because --keys injects "d,i,r,ret"; with no injection it would wait
     * forever, which is why the in-emulator test MUST inject (Rule 2 / stop
     * condition). The NORMAL image never defines this macro. */
    serial_puts("CONIN-PROG-READY\n");
    serial_puts("CONIN-PROG-OUTPUT-BEGIN\n");
    run_baked("CONIN-PROG", g_conin_prog_image, g_conin_prog_image_len);
    serial_puts("CONIN-PROG-OUTPUT-END\n");
    for (;;) {
        __asm__ __volatile__("hlt");
    }
#elif defined(BOOT_VECT)
    /* SETVECT/GETVECT + INT 24h CRITICAL-ERROR SELF-TEST BUILD ONLY (beads
     * initech-509.8; make test-vect). THE in-emulator keystone for the acceptance
     * criteria: "A critical error presents MSG-DOS-0001 and processes Abort/Retry/
     * Fail; vectors saved/restored across EXEC/EXIT."
     *
     * The baked vect program (via run_baked -> load_program): GETVECTs 0x24 and
     * emits "V24PRE=<dec>"; announces "VECT-PROG-READY" so the harness injects the
     * operator key ('a'); does `int $0x24` -- the KERNEL int24 handler (NOT
     * overridden) prints MSG-DOS-0001 to CON (-> serial) and reads the injected
     * 'a' (Abort -> AL=1), which the program emits as "CRIT-AL=1"; then SETVECTs
     * 0x24 to a BOGUS handler and EXITs WITHOUT restoring.
     *
     * load_program's EXEC path saved the parent's 0x24 vector into the child PSP;
     * its EXIT path restored it into the live IDT -- despite the child's SETVECT.
     * After run_baked returns we GETVECT 0x24 (the SAME idt-backed seam) and emit
     * "V24POST=<dec>". The harness asserts MSG-DOS-0001 present + CRIT-AL=1 +
     * V24POST == V24PRE (the save/restore acceptance). The blocking int24 read can
     * only progress because --keys injects 'a' (gated on VECT-PROG-READY).
     *
     * MUTATION-PROOF NOTE (Rule 6): a clean in-emulator mutant for the restore is
     * impractical (the loader save/restore is inseparable from the boot image), so
     * this gate's restore acceptance leans on the HOST mutation-proof: the
     * PSP_MUTATE_VEC_OFFSET mutant in make test-int24-mutant perturbs exactly the
     * psp_save_vectors offset this restore depends on and is proven to bite. The
     * NORMAL image never defines BOOT_VECT. */
    serial_puts("VECT-PROG-OUTPUT-BEGIN\n");
    run_baked("VECT-PROG", g_vect_prog_image, g_vect_prog_image_len);
    /* The loader restored the parent's 0x24 vector on the child's EXIT. Read it
     * back through the SAME idt-backed GETVECT seam the program used and report it
     * so the harness can compare V24POST to V24PRE. */
    {
        uint32_t v24_post = int21_getvect_idt(0x24u);
        serial_puts("V24POST=");
        serial_putu(v24_post);
        serial_putc('\n');
    }
    serial_puts("VECT-PROG-OUTPUT-END\n");
    for (;;) {
        __asm__ __volatile__("hlt");
    }
#else
    /* Stay live (hlt-loop) so the QMP screendump captures the framebuffer. Do
     * NOT isa-debug-exit (that kills the guest before capture). The kstart.asm
     * hang guard also catches an unexpected return. With IRQs live the PIT tick
     * (IRQ0) wakes the CPU 100x/s; the halt loop just goes back to sleep. */
    for (;;) {
        __asm__ __volatile__("hlt");
    }
#endif
}
