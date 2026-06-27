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
#include "memory_map.h"  /* FLAIR_HEAP_* + flair_heap_ram_ok (ADR-0004 DEC-03) */
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
#include "mouse.h"       /* PS/2 mouse IRQ12 driver (initech-5l5z FO-6) */
#include "pit.h"         /* 8254 PIT IRQ0 tick (initech-3rs) */
#include "rtc.h"         /* MC146818 RTC clock source (initech-yv9) */
#include "command.h"     /* COMMAND.COM REPL (initech-7pc); BOOT_SHELL only */
#include "sysinit.h"     /* SYSINIT named bring-up phases (beads initech-509.2) */

#if defined(BOOT_FLAIR_SHELL) || defined(BOOT_FLAIR_LIVE)
/* The LIVE FLAIR desktop demo kernels (beads initech-re30.3 LANE 1; initech-5l5z
 * FO-4). These FLAIR Toolbox + spec headers are pulled in ONLY for the
 * BOOT_FLAIR_SHELL (static render-once) and BOOT_FLAIR_LIVE (cooperative pump,
 * ADR-0006) demo builds, so the NORMAL boot path (and every other -D demo
 * kernel) is byte-unchanged. The objects are ALREADY in KERNEL_FLAIR_OBJS
 * (linked at re30.2); these builds compose the Office Space desktop and present
 * it to the live LFB. BOOT_FLAIR_LIVE ADDITIONALLY drives the PIT tick into the
 * cooperative WaitNextEvent time base (FO-4). */
#include "heap.h"            /* flair_heap_t, flair_alloc (-Ios/flair)        */
#include "surface.h"         /* bitmap_t, surface_put_pixel (-Ios/flair)      */
#include "region_algebra.h"  /* region_t, RGN_ROWS_CAP/RGN_X_POOL_CAP (-Ispec)*/
#include "shell.h"           /* shell_scene_t, shell_build_scene/render       */
#include "desktop.h"         /* desktop_paint_damage (FO-7 minimal repaint)   */
#include "menu_canon.h"      /* FLAIR_CANON_PHOTOSHOP_MENU_COUNT (-Ispec/assets)*/
#include "palette.h"         /* flair_palette_rgb + INITECH_*_RGB (-Ispec/assets)*/
#include "event.h"           /* flair_tick_advance/count (FO-4) + flair_raw_post/
                              * flair_event_init / flair_raw_ring_t (FO-5);
                              * WaitNextEvent / flair_event_set_yield (FO-7)  */
#ifdef FLAIR_LIVE_TENANTS
/* The FLAIR App Contract (ADR-0013; Wave-4 Step 4): the -DFLAIR_LIVE_TENANTS demo
 * boots HELLO+NOTES as co-resident tenants on the live desktop and routes every
 * event through the SOLE Layer-5 spine flair_app_dispatch / flair_route_updates.
 * Pulled in ONLY for that NEW image (layered on BOOT_FLAIR_LIVE); the default boot
 * and the existing FLAIRLIVE/FLAIRSHELL/INTERACTIVE images stay byte-identical
 * (every tenant symbol is behind this guard; Rule 11). */
#include "process.h"             /* FlairProcessList, FlairProcess_launch,
                                  * flair_app_dispatch, flair_route_updates (-Ios/flair)*/
#include "ref_tenant.h"          /* hello_procs / notes_procs (-Ios/apps)            */
#include "flair_tenants_demo.h"  /* FLAIR_TEN_* demo layout + budget (-Ispec)        */
#endif
#endif

/* Seafoam: ported VERBATIM from stage2.asm:42-44 (SEAFOAM_R/G/B). The pie
 * chart (116%) and the HOURGLASS cursor are canon (the wristwatch is THE BUG,
 * PRD Appendix A); so is this fill color. The screendump oracle
 * (tools/ppm_seafoam_check.c) asserts this exact RGB. */
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

/* Emit a 32-bit value as 8 fixed-width hex nibbles (MSB first). Used by the
 * FLAIR-heap RAM-sufficiency panic to print the heap window addresses (more
 * readable than decimal). Freestanding: no libc; deterministic (Rule 11). */
static void serial_puthex32(uint32_t v)
{
    static const char H[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        serial_putc(H[(v >> i) & 0xFu]);
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
/* PS/2 mouse IRQ12 stub (isr.asm, beads initech-5l5z FO-6) -> vector 0x34
 * (slave 8259A IR4 = PIC_SLAVE_BASE 0x30 + 4). Installed + unmasked ONLY by the
 * BOOT_FLAIR_LIVE arm below; inert in every other kernel. */
extern void irq12_entry(void);

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

/* ---- ANSI.SYS console-ops adapters (beads initech-p96i) -------------------
 * Bridge the int21 ANSI vtable (con_putc -> ansi_feed -> these) to the live
 * console + serial.  CRITICAL: ansi_put_char fans to serial EXACTLY like
 * int21_con_sink, so the emu serial markers survive when ANSI.SYS is loaded --
 * the normal boot loads it (CONFIG.SYS DEVICE=ANSI.SYS), so con_putc routes
 * through the FSM and every plain (escape-free) byte becomes a put_char here.
 * ctx is the live console (g_int21_con). */
static void ansi_put_char(void *ctx, uint8_t ch, uint8_t cga_attr)
{
    console_t *con = (console_t *)ctx;
    if (con) {
        console_set_attr(con, cga_attr);
        console_putc(con, (char)ch);
    }
    serial_putc((char)ch);
}
static void ansi_set_cursor(void *ctx, int row, int col)
{
    if (ctx) {
        console_set_cursor((console_t *)ctx, row, col);
    }
}
static void ansi_cursor_rel(void *ctx, int drow, int dcol)
{
    console_t *con = (console_t *)ctx;
    if (con) {
        uint32_t r = 0, c = 0;
        int nr, nc;
        console_get_cursor(con, &r, &c);
        /* Clamp the LOW edge here: console_set_cursor takes uint32_t and only
         * clamps the HIGH edge, so a negative relative move (cursor up past row
         * 0) must be pinned to 0 before the cast (else it wraps to ~UINT32 and
         * clamps to the LAST row -- the wrong direction). */
        nr = (int)r + drow;
        nc = (int)c + dcol;
        if (nr < 0) { nr = 0; }
        if (nc < 0) { nc = 0; }
        console_set_cursor(con, (uint32_t)nr, (uint32_t)nc);
    }
}
static void ansi_erase_display(void *ctx, int mode)
{
    if (ctx) {
        console_erase_display((console_t *)ctx, mode);
    }
}
static void ansi_erase_line(void *ctx, int mode)
{
    if (ctx) {
        console_erase_line((console_t *)ctx, mode);
    }
}
static void ansi_set_attr(void *ctx, uint8_t cga_attr)
{
    if (ctx) {
        console_set_attr((console_t *)ctx, cga_attr);
    }
}
static void ansi_get_cursor(void *ctx, int *row, int *col)
{
    if (ctx) {
        uint32_t r = 0, c = 0;
        console_get_cursor((console_t *)ctx, &r, &c);
        *row = (int)r;
        *col = (int)c;
    }
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

/* ---- INT 25h/26h absolute-disk seam adapter (ADR-0003 DEC-15, initech-4mq7) -
 * The int21 absolute-disk seam (int21_absdisk_backend_t) wants read/write
 * thunks of shape (lba,count,buf); the blockdev_t members carry a ctx as their
 * first arg. These thunks bridge the two by capturing the mounted volume's
 * blockdev pointer (set on the mounted==1 path below). int21.c never sees the
 * blockdev/FAT types (Law 3); the binding lives here in the kernel. */
static const blockdev_t *g_absdisk_dev = 0;

static int int21_absdisk_read(uint32_t lba, uint32_t count, void *buf)
{
    if (g_absdisk_dev == 0 || g_absdisk_dev->read_sectors == 0) {
        return -1;   /* fail loud (Rule 2) -- never a silent short read */
    }
    return g_absdisk_dev->read_sectors(g_absdisk_dev->ctx, lba, count, buf);
}

static int int21_absdisk_write(uint32_t lba, uint32_t count, const void *buf)
{
    if (g_absdisk_dev == 0 || g_absdisk_dev->write_sectors == 0) {
        return -1;   /* read-only / unbound -> the seam maps this to fail-loud */
    }
    return g_absdisk_dev->write_sectors(g_absdisk_dev->ctx, lba, count, buf);
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
static uint16_t loader_exec_by_name(const char *name83, uint16_t dir_start,
                                    const char *cmd_tail, uint32_t cmd_tail_len,
                                    uint32_t env_block, uint8_t *out_rc)
{
    uint8_t rc = 0;
    /* Save the kernel/shell CWD before the child run (the loader resets the CWD
     * to root for the child; beads initech-mzxa) and restore it after, in
     * lockstep with the PSP/exit-hook restore, so the child's CWD never leaks
     * into kernel-context INT 21h. The CWD is saved/restored AROUND the run, so a
     * subdir EXEC (dir_start!=0, the leaf resolved relative to the parent's CWD)
     * cannot corrupt the parent's CWD (beads initech-zs24). env_block (beads
     * initech-1i0x inc 3) threads the shell's inherited env block through
     * unchanged (0 = inherit-empty). */
    int21_cwd_snapshot_t cwd_snap = int21_cwd_save();
    loader_status_t st = load_program_from_fat(name83, dir_start, cmd_tail,
                                               cmd_tail_len, env_block, &rc);
    int21_set_exit(int21_exit_hook);  /* restore kernel-context terminate */
    int21_set_psp(&g_kernel_psp);     /* restore kernel-context JFT */
    int21_cwd_restore(&cwd_snap);     /* restore kernel-context CWD */

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
        case LOADER_ERR_BAD_ENV:
            return INT21_ERR_BAD_FORMAT;        /* 0x000B -- bad/oversized image/env */
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
    int21_cwd_snapshot_t cwd_snap = int21_cwd_save();   /* save kernel CWD (mzxa) */
    loader_status_t st = load_program(image, image_len, (const char *)0, 0u, &rc);
    int21_set_exit(int21_exit_hook);  /* restore kernel-context terminate */
    int21_set_psp(&g_kernel_psp);     /* restore kernel-context JFT (509.3) */
    int21_cwd_restore(&cwd_snap);     /* restore kernel-context CWD */

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

#if defined(BOOT_FLAIR_SHELL) || defined(BOOT_FLAIR_LIVE)
/* ===========================================================================
 * THE LIVE FLAIR DESKTOP (beads initech-re30.3, LANE 1) -- render the Office
 * Space chimera desktop LIVE on the emulated 386.
 *
 * THE milestone: re30.2 linked the 12 FLAIR Managers into the kernel with NO
 * call sites; this is the FIRST call site. It mirrors the HOST oracle
 * harness/proptest/test_shell.c build_shell() EXACTLY (same 2 windows, the two
 * stacked menu bars, the modal FILE COPY box), with two and only two
 * differences: (a) every byte of scene storage comes from the FLAIR heap via
 * flair_alloc (NOT static/BSS -- the kernel window has only ~17 KiB of runway
 * under the B0 kernel.ld ASSERT, bead 5dr8), and (b) it renders to an indexed-8
 * OFFSCREEN then PRESENTS that to the live LFB (instead of malloc+PPM).
 *
 * THE INDEXED-8 SEAM (ADR-0004 OD-2): desktop.c/chrome.c/menu.c/dialog.c return
 * palette INDICES as the "color"; surface_put_pixel writes the raw index at
 * 8bpp but interprets the value as XRGB at 24/32bpp. So shell_render produces
 * CORRECT pixels only into an 8bpp bitmap. We therefore render the scene into an
 * 8bpp offscreen (EXACTLY as the host oracle does) and then convert index->RGB
 * via flair_palette_rgb (THE shared point of truth with the host oracle's
 * render_palette_rgb; spec/assets/palette.h) when the live LFB is direct-color.
 *
 * Ref: ADR-0004 D-1/D-2 (one surface module; Layer-5 desktop shell), D-5
 *      (z-order), OD-2/OD-4 (indexed-8 / seafoam). os/flair/shell.h
 *      (shell_build_scene/shell_render), os/flair/desktop.h
 *      (FLAIR_DESKTOP_BG_INDEX), spec/memory_map.h (FLAIR_HEAP_*),
 *      harness/render/render.c:56-75 (render_palette_rgb -- the index->RGB
 *      correspondence flair_palette_rgb mirrors). harness/proptest/test_shell.c
 *      build_shell (the scene this MIRRORS). CLAUDE.md Law 2 (oracle is truth),
 *      Law 4 (look like the frame), Rule 2 (fail loud), Rule 11 (deterministic),
 *      Rule 12 (ASCII-clean).
 * ===========================================================================*/

/* The scene geometry MIRRORS test_shell.c byte-for-byte (Law 2: the screendump
 * oracle reuses the host test's known band positions). */
#define FD_N_WINDOWS    2
#define FD_N_SYS_MENUS  4

/* Window bounds + titles -- IDENTICAL to test_shell.c W0_/W1_ bounds + titles. */
static const rgn_rect_t fd_win_bounds[FD_N_WINDOWS] = {
    {  80,  60, 300, 360 },   /* window 0 (back, upper-left -- overlaps the box)  */
    { 120, 300, 360, 560 }    /* window 1 (front, lower-right)                    */
};
static const char *const fd_win_titles[FD_N_WINDOWS] = {
    "untitled-1",
    "Saving tables to disk"
};
/* System-7 bar titles (File/Edit/View/Special) -- IDENTICAL to test_shell.c. */
static const char *const fd_sys_titles[FD_N_SYS_MENUS] = {
    "File", "Edit", "View", "Special"
};

/* The fail-loud sink for a NULL flair_alloc (Rule 2): emit a grep-able marker
 * with which allocation failed, then halt forever. Never proceed into FLAIR work
 * on a heap that could not back the scene (a silently-wrong framebuffer is worse
 * than a halt; CLAUDE.md Rule 2). */
static void flair_desktop_oom(const char *what)
{
    serial_puts("PANIC flair-desktop: flair_alloc returned NULL for ");
    serial_puts(what);
    serial_putc('\n');
    serial_puts("HALTED\n");
    if (g_int21_con) {
        console_puts(g_int21_con,
            "\nPC LOAD LETTER  (FLAIR heap exhausted)\n");
    }
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

/* Allocate ONE region from the FLAIR heap with its rows[]/x_pool pools attached
 * and set to the empty set -- the test_drag/test_shell store_attach() idiom, but
 * heap-backed. Fail loud (Rule 2) if any of the three allocations is NULL. */
static region_t *flair_desktop_alloc_region(flair_heap_t *h, const char *what)
{
    region_t  *r    = (region_t *)flair_alloc(h, FLAIR_CLASS_HANDLE,
                                              (uint32_t)sizeof(region_t));
    rgn_row_t *rows = (rgn_row_t *)flair_alloc(h, FLAIR_CLASS_REGION,
                          (uint32_t)(sizeof(rgn_row_t) * RGN_ROWS_CAP));
    int16_t   *pool = (int16_t *)flair_alloc(h, FLAIR_CLASS_REGION,
                          (uint32_t)(sizeof(int16_t) * RGN_X_POOL_CAP));
    if (!r || !rows || !pool) {
        flair_desktop_oom(what);
    }
    r->rows       = rows;
    r->cap_rows   = RGN_ROWS_CAP;
    r->x_pool     = pool;
    r->x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(r);
    return r;
}

/* Present the indexed-8 offscreen onto the live LFB, honoring lfb_bpp + the LFB
 * pitch (scanline padding). 8bpp: program the VGA DAC with the FLAIR palette
 * (only the indices the scene uses, 0..8) and copy indices row-by-row. 24/32bpp:
 * convert each index -> 0x00RRGGBB via flair_palette_rgb and write it through the
 * ONE surface module (surface_put_pixel) into a bitmap_t wrapping the LFB. */
static void flair_desktop_present(const boot_info_t *bi, const bitmap_t *off)
{
    uint32_t W = off->width;
    uint32_t H = off->height;

    if (bi->lfb_bpp == 8u) {
        /* Indexed-8 LFB present (the period-authentic SVGA depth, ADR-0004
         * OD-2/AM-7: VBE 0x101 on a Cirrus/ET4000-class card). The LFB stores
         * palette indices: program the DAC for every index flair_palette_rgb
         * defines (0..8; DAC is 6 bits/channel so 8-bit RGB is >> 2,
         * vga_set_dac) then copy indices row-by-row honoring lfb_pitch (not
         * width).
         *
         * KEEP -- do NOT delete as dead code (committee ruling re30.3-chair,
         * 2026-06-21). This path is correct + ratified but currently UNREACHED:
         * stage2 vbe_setup requests only 640x480x32/24, and the sole 8bpp the
         * boot yields is mode-0x13 320x200, which the 640x480 size guard below
         * rejects before present runs. Reached when a 640x480x8 LFB exists
         * (VESA 0x101 / 86Box at M4). Activation of the stage2 0x101 fallback
         * AND a test that REACHES this wrapper are tracked in bead initech-2gva
         * (host-modeled separately in harness/render/render.c). */
        for (uint32_t idx = 0; idx <= 8u; idx++) {
            uint32_t rgb = flair_palette_rgb((uint8_t)idx);
            vga_set_dac((uint8_t)idx,
                        (uint8_t)((rgb >> 16) & 0xFFu),
                        (uint8_t)((rgb >> 8) & 0xFFu),
                        (uint8_t)(rgb & 0xFFu));
        }
        volatile uint8_t *lfb = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
        for (uint32_t y = 0; y < H; y++) {
            const uint8_t *srow = (const uint8_t *)off->base + (uint32_t)y * off->pitch;
            volatile uint8_t *drow = lfb + (uint32_t)y * bi->lfb_pitch;
            for (uint32_t x = 0; x < W; x++) {
                drow[x] = srow[x];
            }
        }
        return;
    }

    /* Direct-color LFB (QEMU VBE leg, 24 or 32 bpp). Wrap the LFB as a bitmap_t
     * and write each converted pixel through surface_put_pixel (the ONE pixel
     * path; ADR-0004 D-2). The byte offset uses lfb_pitch so padded scanlines
     * are honored. */
    bitmap_t lbm;
    lbm.base            = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
    lbm.pitch           = bi->lfb_pitch;
    lbm.bpp             = bi->lfb_bpp;
    lbm.bytes_per_pixel = bi->lfb_bpp / 8u;
    lbm.width           = bi->lfb_width;
    lbm.height          = bi->lfb_height;
    for (uint32_t y = 0; y < H; y++) {
        const uint8_t *srow = (const uint8_t *)off->base + (uint32_t)y * off->pitch;
        for (uint32_t x = 0; x < W; x++) {
            uint32_t rgb = flair_palette_rgb(srow[x]) & 0x00FFFFFFu;
            uint32_t boff = (uint32_t)y * lbm.pitch + x * lbm.bytes_per_pixel;
            surface_put_pixel(&lbm, boff, rgb);
        }
    }
}

/* FO-7 (beads initech-5l5z; ADR-0006 E-D4): the live-loop context the FLAIR
 * WaitNextEvent pump drives. flair_desktop_run populates it (when ctx_out is
 * non-NULL) AFTER the initial present so the pump can keep driving the SAME
 * already-built scene -- the Window Manager (z-order + DiffRgn damage), the
 * document-window store (for FindWindow part-code routing), the compositor
 * scratch region, and the indexed-8 offscreen the damage repaints into and the
 * present blits to the LFB. BC-1 (single spine): the pump runs the IDENTICAL
 * window.c/desktop.c verbs the host suites mutation-prove; kmain is a thin
 * source/sink adapter (no event/geometry logic re-implemented here). */
typedef struct flair_live_ctx {
    WindowMgr          *wm;         /* &scene->wm (z-order + damage)             */
    shell_window_store *windows;    /* the document-window store (FindWindow)    */
    int                 n_windows;  /* document window count                     */
    region_t           *comp;       /* compositor visible-region scratch (D-5)   */
    bitmap_t            off;         /* the indexed-8 offscreen (descriptor copy) */
    shell_scene_t      *scene;      /* the whole scene (heap-backed)             */
#ifdef FLAIR_LIVE_TENANTS
    flair_heap_t       *master;     /* the function-static FLAIR heap, exposed so
                                     * the FLAIR_LIVE_TENANTS launch site can carve
                                     * tenant partitions from it (ADR-0013 Sec 3.6).
                                     * Behind the guard so the struct layout stays
                                     * byte-identical for non-tenant builds.       */
#endif
} flair_live_ctx_t;

/* Build + render + present the live FLAIR desktop. Returns nothing; fails loud
 * on a too-small LFB or any heap exhaustion (Rule 2). When ctx_out is non-NULL
 * (the BOOT_FLAIR_LIVE pump), the built scene handles are captured into it after
 * the initial present so the cooperative WaitNextEvent pump can drive them
 * (FO-7). BOOT_FLAIR_SHELL passes NULL -- byte-for-byte the same render + present
 * + (no capture), so the static screendump gate is unchanged (Rule 11). */
static void flair_desktop_run(const boot_info_t *bi, flair_live_ctx_t *ctx_out)
{
    enum { FD_W = 640, FD_H = 480 };

    /* The scene is rendered at 640x480 (ADR-0004 OD-3, the native desktop). The
     * mode-0x13 fallback (320x200) cannot hold it; fail loud rather than render
     * a clipped/garbage desktop (Rule 2). */
    if (bi->lfb_width < (uint32_t)FD_W || bi->lfb_height < (uint32_t)FD_H) {
        serial_puts("PANIC flair-desktop: LFB smaller than 640x480 w=");
        serial_putu(bi->lfb_width);
        serial_puts(" h=");
        serial_putu(bi->lfb_height);
        serial_putc('\n');
        serial_puts("HALTED\n");
        if (g_int21_con) {
            console_puts(g_int21_con,
                "\nPC LOAD LETTER  (FLAIR needs a 640x480 framebuffer)\n");
        }
        for (;;) {
            __asm__ __volatile__("cli; hlt");
        }
    }

    /* Bind the FLAIR heap to its fixed 4 MiB extended-memory window (NOT over
     * the LFB -- the heap is [FLAIR_HEAP_BASE, +SIZE); the LFB is the render
     * destination). The RAM-sufficiency gate above already proved this window is
     * backed by real RAM (FLAIR-HEAP-OK). */
    static flair_heap_t heap;   /* tiny bookkeeping struct; no buffer embedded */
    flair_heap_init(&heap, (void *)(uintptr_t)FLAIR_HEAP_BASE, FLAIR_HEAP_SIZE);

    /* --- The whole scene, allocated from the FLAIR heap (Law 3; bead 5dr8) --- */
    shell_scene_t *scene =
        (shell_scene_t *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                     (uint32_t)sizeof(shell_scene_t));
    if (!scene) { flair_desktop_oom("shell_scene_t"); }

    /* The five Window-Manager regions (desktop_update + 3 manager scratch + the
     * compositor's distinct visible-region carrier; desktop.h). */
    region_t *desk = flair_desktop_alloc_region(&heap, "wm.desktop_update");
    region_t *sa   = flair_desktop_alloc_region(&heap, "wm.scratch_a");
    region_t *sb   = flair_desktop_alloc_region(&heap, "wm.scratch_b");
    region_t *sc   = flair_desktop_alloc_region(&heap, "wm.scratch_c");
    region_t *comp = flair_desktop_alloc_region(&heap, "wm.comp_scratch");

    /* The document windows + their region trios. */
    shell_window_store *wins =
        (shell_window_store *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                          (uint32_t)(sizeof(shell_window_store) * FD_N_WINDOWS));
    if (!wins) { flair_desktop_oom("shell_window_store[]"); }
    for (int i = 0; i < FD_N_WINDOWS; i++) {
        wins[i].strucRgn  = flair_desktop_alloc_region(&heap, "win.strucRgn");
        wins[i].contRgn   = flair_desktop_alloc_region(&heap, "win.contRgn");
        wins[i].updateRgn = flair_desktop_alloc_region(&heap, "win.updateRgn");
    }

    /* The System-7 bar: 4 menus, each with a 2-item list (well-formed). Mirrors
     * test_shell.c. */
    MenuInfo *sys_menus =
        (MenuInfo *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                (uint32_t)(sizeof(MenuInfo) * FD_N_SYS_MENUS));
    MenuItem *sys_items =
        (MenuItem *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                            (uint32_t)(sizeof(MenuItem) * FD_N_SYS_MENUS * 2));
    if (!sys_menus || !sys_items) { flair_desktop_oom("sys menus/items"); }
    for (int mi = 0; mi < FD_N_SYS_MENUS; mi++) {
        MenuItem *it = &sys_items[mi * 2];
        it[0] = (MenuItem){ "About", 0, 0,   0, 1, 0 };
        it[1] = (MenuItem){ "Quit",  0, 'Q', 0, 1, 0 };
        sys_menus[mi].menuID    = (int16_t)(128 + mi);
        sys_menus[mi].title     = fd_sys_titles[mi];
        sys_menus[mi].items     = it;
        sys_menus[mi].n_items   = 2;
        sys_menus[mi].menuWidth = 0;
    }

    /* The Photoshop bar: 8 menus; the shell STAMPS the canon titles from
     * menu_canon.h (the caller leaves title NULL + supplies the item lists). */
    MenuInfo *ps_menus =
        (MenuInfo *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
            (uint32_t)(sizeof(MenuInfo) * FLAIR_CANON_PHOTOSHOP_MENU_COUNT));
    MenuItem *ps_items =
        (MenuItem *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
            (uint32_t)(sizeof(MenuItem) * FLAIR_CANON_PHOTOSHOP_MENU_COUNT * 2));
    if (!ps_menus || !ps_items) { flair_desktop_oom("photoshop menus/items"); }
    for (int mi = 0; mi < FLAIR_CANON_PHOTOSHOP_MENU_COUNT; mi++) {
        MenuItem *it = &ps_items[mi * 2];
        it[0] = (MenuItem){ "New",  0, 'N', 0, 1, 0 };
        it[1] = (MenuItem){ "Open", 0, 'O', 0, 1, 0 };
        ps_menus[mi].title     = (const char *)0;  /* set by shell from canon  */
        ps_menus[mi].items     = it;
        ps_menus[mi].n_items   = 2;
        ps_menus[mi].menuWidth = 0;
    }

    /* The modal FILE COPY dialog storage (DialogRecord + 2 items + progress
     * control + 3 regions). */
    DialogRecord *dlg =
        (DialogRecord *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                    (uint32_t)sizeof(DialogRecord));
    DialogItem *dlg_items =
        (DialogItem *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                  (uint32_t)(sizeof(DialogItem) * 2));
    ControlRecord *dlg_progress =
        (ControlRecord *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                     (uint32_t)sizeof(ControlRecord));
    if (!dlg || !dlg_items || !dlg_progress) { flair_desktop_oom("dialog storage"); }
    region_t *dlg_struc = flair_desktop_alloc_region(&heap, "dlg.strucRgn");
    region_t *dlg_cont  = flair_desktop_alloc_region(&heap, "dlg.contRgn");
    region_t *dlg_upd   = flair_desktop_alloc_region(&heap, "dlg.updateRgn");

    /* The indexed-8 offscreen the scene renders into (OD-2: 1 byte/pixel = a
     * palette index). FLAIR_CLASS_BITMAP. Tight pitch == width (no padding). */
    uint8_t *off_px =
        (uint8_t *)flair_alloc(&heap, FLAIR_CLASS_BITMAP,
                               (uint32_t)((uint32_t)FD_W * (uint32_t)FD_H));
    if (!off_px) { flair_desktop_oom("offscreen8 (640x480)"); }
    bitmap_t off;
    off.base            = (volatile uint8_t *)off_px;
    off.pitch           = (uint32_t)FD_W;     /* tight: width * (8/8) */
    off.bpp             = 8u;
    off.bytes_per_pixel = 1u;
    off.width           = (uint32_t)FD_W;
    off.height          = (uint32_t)FD_H;

    /* --- Wire the scene (IDENTICAL call shape to test_shell.c build_shell) --- */
    rgn_rect_t frame = { 0, 0, (int16_t)FD_H, (int16_t)FD_W };
    shell_build_scene(scene,
                      frame,
                      wins, FD_N_WINDOWS, fd_win_bounds, fd_win_titles,
                      desk, sa, sb, sc, comp,
                      sys_menus, FD_N_SYS_MENUS,
                      ps_menus,
                      dlg, dlg_items, dlg_progress,
                      dlg_struc, dlg_cont, dlg_upd,
#ifdef FLAIR_LIVE_TENANTS
                      0 /* FLAIR App Contract demo: NO modal -- the two canon doc
                         * windows are hidden by the tenant arm and the co-resident
                         * tenants are launched on top (ADR-0013); reuses shell.c
                         * byte-identically, only the show_modal flag differs. */);
#else
                      1 /* show_modal -- the canon Office Space frame */);
#endif

    /* --- Render the desktop in INDICES into the 8bpp offscreen (EXACTLY as the
     * host oracle does) --- */
    shell_render(scene, &off);

    /* --- PRESENT the offscreen onto the live LFB (8 -> DAC+copy; 24/32 -> convert) --- */
    flair_desktop_present(bi, &off);

    /* FO-7: hand the built scene to the live pump (BC-4: present FIRST, then the
     * pump keeps driving the SAME scene). The storage is all FLAIR-heap-backed and
     * outlives this frame; `off` is a value descriptor (its .base is heap) copied
     * by value. NULL for the BOOT_FLAIR_SHELL static frame (no behaviour change). */
    if (ctx_out) {
        ctx_out->wm        = &scene->wm;
        ctx_out->windows   = wins;
        ctx_out->n_windows = FD_N_WINDOWS;
        ctx_out->comp      = comp;
        ctx_out->off       = off;
        ctx_out->scene     = scene;
#ifdef FLAIR_LIVE_TENANTS
        /* Expose the function-static FLAIR heap so the FLAIR_LIVE_TENANTS launch
         * site can carve tenant partitions from it (it has static lifetime, so
         * &heap is valid after this returns; ADR-0013 Sec 3.6). */
        ctx_out->master    = &heap;
#endif
    }
}
#endif /* BOOT_FLAIR_SHELL || BOOT_FLAIR_LIVE */

#ifdef BOOT_FLAIR_LIVE
/* ===========================================================================
 * FO-5: FLAIR raw ring + kbd scancode hook (beads initech-5l5z; ADR-0006
 * E-D3(c) / FO-5; landing-sequence Step 3).
 *
 * Kernel-static SPSC ring for raw keyboard events. Zero-initialised in BSS;
 * flair_event_init() is called by the BOOT_FLAIR_LIVE arm of kernel_main
 * BEFORE any hook is installed or IRQ is unmasked (ADR-0004 D-4 contract).
 * The kbd IRQ1 producer posts into this ring; the bounded wake loop in
 * kernel_main drains it in task context and emits FLAIR-KEY on serial.
 *
 * Additive (ADR-0006 E-D3 / BC-2): the DOS g_kbd ASCII ring in kbd.c is
 * kept 100% intact; COMMAND.COM / test-shell / test-samir-boot are GREEN. */
static flair_raw_ring_t g_flair_kbd_ring;

/* flair_live_kbd_post -- kbd scancode hook installed by BOOT_FLAIR_LIVE.
 * Called from IRQ1 context (kbd_irq_handler, AFTER the DOS g_kbd ring post
 * and BEFORE the PIC EOI) with the RAW PS/2 scancode byte (scancode SET 1
 * make/break from port 0x60; NOT the ASCII translation). Posts ONE
 * FLAIR_RAW_KEYBOARD event to g_flair_kbd_ring.
 * ISR-enqueue-only minimum (ADR-0004 D-4): a single flair_raw_post() call;
 * no Toolbox, no alloc, no port I/O. The full ring drop case is the defined
 * non-panic behaviour for ISR context (event.h Sec 1; Rule 2 telemetry).
 * Ref: ADR-0006 E-D3(c), FO-5; kbd.h kbd_set_scancode_hook; event.h;
 *      spec/event_model.h flair_raw_event_t (kind/tick/payload layout).
 *
 * __attribute__((unused)): the FLAIR_LIVE_MUTATE_NO_KBD_HOOK Rule-6 mutant build
 * compiles out the SOLE call site (kbd_set_scancode_hook(flair_live_kbd_post),
 * #ifndef-guarded below), so this hook is legitimately unused there -- exactly
 * the mutant's point (no install => no FLAIR-KEY). Same idiom as run_baked above
 * for the BOOT_SHELL build. */
__attribute__((unused))
static void flair_live_kbd_post(uint8_t sc)
{
    flair_raw_event_t raw;
    raw.kind    = (uint32_t)FLAIR_RAW_KEYBOARD; /* FLAIR_RAW_KEYBOARD=0 */
    raw.tick    = flair_tick_count();            /* PIT tick count at IRQ time */
    raw.payload = (uint32_t)sc;                 /* low byte=raw scancode (SET 1) */
    (void)flair_raw_post(&g_flair_kbd_ring, &raw);
}

/* flair_live_mouse_post -- mouse completed-packet hook installed by
 * BOOT_FLAIR_LIVE (ADR-0006 E-D3(b) / FO-6; beads initech-5l5z). Called from
 * IRQ12 context (mouse_irq_handler, after the 3-byte PS/2 packet is assembled
 * and BEFORE the dual-PIC EOI) with the signed deltas + button bits. Posts ONE
 * FLAIR_RAW_MOUSE event into the SAME g_flair_kbd_ring the kbd path feeds (the
 * one SPSC raw ring; spec/event_model.h Sec 5 payload layout: buttons in bits
 * 0..7, signed dX in 8..15, signed dY in 16..23). ISR-enqueue-only minimum
 * (ADR-0004 D-4): a single flair_raw_post() call; no Toolbox, no alloc, no I/O.
 *
 * __attribute__((unused)): the FLAIR_LIVE_MUTATE_NO_MOUSE_HOOK Rule-6 mutant
 * compiles out the SOLE call site (mouse_set_event_hook(flair_live_mouse_post),
 * #ifndef-guarded below), so this hook is legitimately unused there -- exactly
 * the mutant's point (no install => no FLAIR-MOUSE). */
__attribute__((unused))
static void flair_live_mouse_post(int dx, int dy, uint8_t buttons)
{
    flair_raw_event_t raw;
    raw.kind    = (uint32_t)FLAIR_RAW_MOUSE;     /* FLAIR_RAW_MOUSE=1 */
    raw.tick    = flair_tick_count();            /* PIT tick count at IRQ time */
    raw.payload = (uint32_t)(buttons & 0x07u)               /* bits 0..7 buttons */
                | (((uint32_t)((uint8_t)(int8_t)dx)) << 8)  /* bits 8..15 dX     */
                | (((uint32_t)((uint8_t)(int8_t)dy)) << 16);/* bits 16..23 dY    */
    (void)flair_raw_post(&g_flair_kbd_ring, &raw);
}

/* ===========================================================================
 * FO-7/8: the WaitNextEvent pump support (beads initech-5l5z; ADR-0006 E-D4 /
 * FO-7; landing-sequence Step 6). These are the THIN source/sink glue around the
 * already-built, host-mutation-proven window.c/desktop.c verbs (BC-1 single
 * spine): NO event/geometry logic is re-implemented here.
 * ===========================================================================*/

/* The cooperative HLT yield hook (ADR-0004 D-6 / event.h Sec 3): on bare metal
 * WaitNextEvent calls this once per idle iteration to sleep until the next IRQ
 * (PIT tick / kbd / mouse). NOT preemption -- the pump holds the CPU and yields
 * voluntarily (BC-7; cooperative, not preemptive). */
static void flair_live_yield(void)
{
    __asm__ __volatile__("hlt");
}

/* FO-5/6 marker reconciliation: the pump now COOKS the raw ring via WaitNextEvent
 * (so the raw-drain FLAIR-KEY/FLAIR-MOUSE markers no longer apply). Emit ONE
 * coherent cooked marker per delivered EventRecord: the `what` code (mouseDown=1,
 * mouseUp=2, keyDown=3, keyUp=4; spec/event_model.h Sec 1) and the global cursor
 * `where` event.c stamps from the accumulated mouse deltas. A keyDown 'a' (no
 * mouse) reads `FLAIR-EVT what=3 where=320,240`; a mouseDown after injected
 * motion reads the advanced cursor -- on-metal proof the pump cooks the ring. */
static void flair_live_emit_evt(const EventRecord *ev)
{
    serial_puts("FLAIR-EVT what=");
    serial_putu((uint32_t)ev->what);
    serial_puts(" where=");
    serial_puti((int32_t)ev->where.h);
    serial_putc(',');
    serial_puti((int32_t)ev->where.v);
    serial_puts(" msg=");
    serial_puthex32(ev->message & 0xFFFFu);   /* key: (vkey<<8)|ascii; mouse: 0 */
    serial_putc('\n');
}

/* Map a hit WindowPtr back to its document-window store index (or -1). For the
 * FLAIR-DRAG marker only (the modal/dialog window is not in this store). */
static int flair_live_window_index(const flair_live_ctx_t *ctx, WindowPtr w)
{
    int i;
    for (i = 0; i < ctx->n_windows; i++) {
        if (&ctx->windows[i].rec == w) {
            return i;
        }
    }
    return -1;
}

/* FO-7 inDrag dispatch: track the live drag, then move + minimal-repaint + present.
 *
 * The net drag delta is the cursor displacement from button-down (where0) to
 * mouse-up: WaitNextEvent updates the global cursor on EVERY raw mouse move (a
 * pure move cooks to nullEvent so it is NOT delivered, but the cursor still
 * advances) and DELIVERS the mouseUp with `where` == the final cursor. So we
 * re-enter WaitNextEvent until mouseUp (bounded by a tick guard, Rule 11) and sum
 * the net (dh,dv). Then DragWindow translates the window + accrues the D-5 damage
 * (the vacated desktop into desktop_update, any re-exposed window behind into its
 * updateRgn); WindowMgr_invalidate seeds the moved window's OWN repaint (MoveWindow
 * clears its updateRgn -- the D-5 oracle scopes to OTHER windows + desktop, the
 * test_drag.c idiom); desktop_paint_damage repaints ONLY the damage; the present
 * blits the offscreen to the live LFB the same way the initial present does. */
static void flair_live_do_drag(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               WindowPtr w, flair_point_t where0)
{
    EventRecord up;
    flair_point_t where1 = where0;
    uint32_t guard = flair_tick_count() + 150u;   /* bounded drag wait (~1.5 s) */
    rgn_rect_t before, after;
    int16_t dh, dv;
    int wid;

    for (;;) {
        int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &up, 3u);
        if (g) {
            flair_live_emit_evt(&up);
            if (up.what == (uint16_t)mouseUp) { where1 = up.where; break; }
        } else {
            /* timeout: WaitNextEvent stamped `up.where` with the current cursor
             * (it advanced as the move packets cooked); keep tracking it. */
            where1 = up.where;
        }
        if (flair_tick_count() >= guard) { break; }
    }

    dh = (int16_t)(where1.h - where0.h);
    dv = (int16_t)(where1.v - where0.v);

    before = region_get_bbox(w->strucRgn);
#ifndef FLAIR_LIVE_MUTATE_DRAG_NOOP
    if (dh != 0 || dv != 0) {
        DragWindow(ctx->wm, w, dh, dv);
        /* seed the moved window's self-repaint at its NEW position. */
        WindowMgr_invalidate(ctx->wm, w, region_get_bbox(w->strucRgn));
    }
#else
    /* HER-14 drag-noop mutant (ADR-0006 M1 / BC-9): the inDrag dispatch does
     * NOTHING -- the window does NOT move. The WDEF-scan at the new pos then reads
     * bare teal and the OLD pos still shows chrome, so test-flair-drag goes RED.
     * This catches the "static frame dressed as interactive" heresy. NEVER define
     * in a real build. */
    (void)dh; (void)dv;
#endif

    /* D-5 minimal repaint of the damage, then PRESENT to the live LFB. */
    desktop_paint_damage(ctx->wm, &ctx->off, ctx->comp);
    flair_desktop_present(bi, &ctx->off);

    after = region_get_bbox(w->strucRgn);
    wid   = flair_live_window_index(ctx, w);
    serial_puts("FLAIR-DRAG win ");
    serial_puti((int32_t)wid);
    serial_puts(" (");
    serial_puti((int32_t)before.left);
    serial_putc(',');
    serial_puti((int32_t)before.top);
    serial_puts(")->(");
    serial_puti((int32_t)after.left);
    serial_putc(',');
    serial_puti((int32_t)after.top);
    serial_puts(")\n");
}

/* FO-7 inGoAway dispatch (wired, not gated -- the drag gate does not exercise it;
 * a focused close gate is the thin FO-8b follow-on). HideWindow accrues the
 * exposure damage of everything the window covered exactly like DisposeWindow
 * (window.h Sec 2); desktop_paint_damage then repaints the exposed area + any
 * re-exposed window behind, and the present blits it. */
static void flair_live_do_close(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                WindowPtr w)
{
    int wid = flair_live_window_index(ctx, w);
    HideWindow(ctx->wm, w);
    desktop_paint_damage(ctx->wm, &ctx->off, ctx->comp);
    flair_desktop_present(bi, &ctx->off);
    serial_puts("FLAIR-CLOSE win ");
    serial_puti((int32_t)wid);
    serial_putc('\n');
}

/* FO-8b dropped-pull-down colors (indexed-8 offscreen; OD-2). A BTNFACE-gray
 * body + a black 1px frame/text and an inverted (black) hilite band (the
 * GDI-facade pull-down body, the FO-D2-8/rmsr chimera). idx6 = canon BTNFACE
 * gray #C0C0C0 (CIDX_CONTROL), idx0 = canon black (spec/assets/color_canon.h):
 * the SAME indices the controls (idx6) and frame ink (idx0) use, so the present
 * path (flair_palette_rgb) and the INDEPENDENT ppm canon (flair_canon_rgb)
 * agree on VALUE while the load-bearing differential stays STRUCTURAL (a panel
 * where bare teal / menubar-white was). BTNFACE gray (not idx1 white) is chosen
 * so the body-fill leg BITES the menu-noop mutant: the System-7 menu bar is
 * canon WHITE (idx3), so a white panel body would be invisible against it; gray
 * is distinct from teal, white AND black. */
#define FLAIR_MENU_PANEL_BG_IDX   6u   /* BTNFACE gray body (CIDX_CONTROL)    */
#define FLAIR_MENU_PANEL_FG_IDX   0u   /* black frame/text/hilite (CIDX_BLACK)*/

/* Bounded cursor-point capture while the menu button is held (Rule 11): the
 * drop+track is a sub-second gesture, so a small cap is ample and never grows
 * unbounded; the most-recent (release) point is always kept in the last slot. */
#define FLAIR_MENU_TRACK_MAX      64

/* FO-8b inMenuBar dispatch (beads initech-5l5z; ADR-0004 D-3 / ADR-0006 FO-8 --
 * inMenuBar -> MenuSelect): the LIVE pull-down. On a mouseDown in the System-7
 * menu-bar band the pump calls this to DROP the menu live on the booted 386 (Law
 * 4's "draggable arrangement with WORKING MENUS"): MenuBar_hit the title, draw the
 * dropped panel into the offscreen + present, TRACK the cursor (bounded; the
 * test_drag re-enter-WaitNextEvent-until-mouseUp idiom) re-hiliting the item under
 * it, then MenuSelect the (menuID<<16|item) result and leave the chosen menu
 * visibly OPEN as the demo's final interactive frame.
 *
 * BC-1 single spine: this is THIN source/sink glue around the already-built,
 * host-mutation-proven menu.c verbs (MenuBar_hit / flair_draw_menu_panel /
 * MenuInfo_item_at / MenuSelect; test_menu.c) and the present the initial frame
 * uses -- NO menu geometry is re-implemented here.
 *
 * PERSISTENT, drag-analogous (ADR-0006 BC-4): the selected menu stays DROPPED as
 * the final frame so the post-trace screendump grades the live drop, EXACTLY as
 * the live drag leaves the window at its NEW position (the drag does not snap the
 * window back; the menu does not snap shut before the dump). The
 * desktop_paint_damage menu CLOSE (invalidate the panel rect -> D-5 minimal
 * repaint -> present) is the documented follow-on close path; running it before
 * the screendump would ERASE the very panel the oracle asserts (Law 2), so it is
 * not run in this bounded demo. The GDI-facade CombineRgn panel clip (FO-D2-8 /
 * initech-rmsr) stays open: the drop draws with clip=NULL (the panel spans free
 * desktop below the bar), noted for the rmsr follow-on.
 *
 * FLAIR_LIVE_MUTATE_MENU_NOOP (Rule 6; the HER-14 "menus do not work" heresy):
 * the dispatch does NOT drop a panel and does NOT track/select -- it only emits
 * the marker with sel=0. The desktop under the title stays bare teal, so
 * ppm_flair_menu_check sees teal (RED) and the sel=0 marker proves nothing was
 * chosen. NEVER define in a real build. */
static void flair_live_do_menu(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               MenuBar *bar, flair_point_t where0)
{
    int mi = MenuBar_hit(bar, (int)where0.h);
    int16_t menuID = (mi >= 0) ? bar->menus[mi].menuID : (int16_t)0;

#ifndef FLAIR_LIVE_MUTATE_MENU_NOOP
    if (mi < 0) {
        /* In the bar but not on a title (Apple slot / past the last title): no
         * menu drops. Emit the marker with sel=0 so the gate can tell. */
        serial_puts("FLAIR-MENU menu=0 item=0 (sel=0x00000000)\n");
        return;
    }

    /* A whole-bitmap GrafPort over the offscreen (mirrors shell.c make_bar_port;
     * flair_draw_menu_panel reads only port->portBits.bm + the supplied clip). */
    GrafPort port;
    rgn_rect_t whole = { 0, 0, (int16_t)ctx->off.height, (int16_t)ctx->off.width };
    port.portBits.bm     = ctx->off;
    port.portBits.bounds = whole;
    port.portRect        = whole;
    port.visRgn          = (region_t *)0;
    port.clipRgn         = (region_t *)0;
    port.pnLoc.v = 0; port.pnLoc.h = 0;
    port.pnSize.v = 1; port.pnSize.h = 1;
    port.pnVis = 0;
    port.grafProcs = (QDProcs *)0;

    /* DROP: draw the dropped panel (no hilite yet) + present -- the menu is down. */
    flair_draw_menu_panel(&port, bar, mi, -1,
                          FLAIR_MENU_PANEL_FG_IDX, FLAIR_MENU_PANEL_BG_IDX,
                          (const region_t *)0);
    flair_desktop_present(bi, &ctx->off);
    serial_puts("FLAIR-MENU-DROP menu=");
    serial_puti((int32_t)menuID);
    serial_putc('\n');

    /* TRACK: collect the cursor points (deduped, bounded) until mouseUp, re-hiliting
     * the item under the cursor as it moves. A pure move cooks to nullEvent (got=0)
     * but WaitNextEvent still stamps `where` with the advanced cursor (the drag
     * idiom). Bounded by a tick guard (Rule 11) in case mouseUp never arrives. */
    flair_point_t pts[FLAIR_MENU_TRACK_MAX];
    int n = 0;
    int last_hi = -1;
    uint32_t guard = flair_tick_count() + 150u;   /* ~1.5 s bound */
    for (;;) {
        EventRecord mev;
        int got = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &mev, 3u);
        if (got) {
            flair_live_emit_evt(&mev);
        }
        /* Append the cursor point, dedup consecutive identical; when full, keep
         * the most recent in the last slot so the RELEASE point is always pts[n-1]
         * for MenuSelect (deterministic selection, Rule 11). */
        if (n == 0 || pts[n - 1].h != mev.where.h || pts[n - 1].v != mev.where.v) {
            if (n < FLAIR_MENU_TRACK_MAX) {
                pts[n++] = mev.where;
            } else {
                pts[FLAIR_MENU_TRACK_MAX - 1] = mev.where;
            }
        }
        int hi = MenuInfo_item_at(bar, mi, (int)mev.where.h, (int)mev.where.v);
        if (hi != last_hi) {
            flair_draw_menu_panel(&port, bar, mi, hi,
                                  FLAIR_MENU_PANEL_FG_IDX, FLAIR_MENU_PANEL_BG_IDX,
                                  (const region_t *)0);
            flair_desktop_present(bi, &ctx->off);
            last_hi = hi;
        }
        if (got && mev.what == (uint16_t)mouseUp) {
            break;
        }
        if (flair_tick_count() >= guard) {
            break;
        }
    }

    /* SELECT: the IM (menuID<<16|item) result from the tracked sequence (the
     * release is pts[n-1]). Leave the chosen item hilited as the persistent final
     * frame; if nothing was chosen, keep the last tracked hilite. */
    uint32_t sel = MenuSelect(bar, where0, pts, n);
    int sel_item = (int)MenuResultItem(sel);
    int sel_hi   = (sel_item > 0) ? (sel_item - 1) : last_hi;
    flair_draw_menu_panel(&port, bar, mi, sel_hi,
                          FLAIR_MENU_PANEL_FG_IDX, FLAIR_MENU_PANEL_BG_IDX,
                          (const region_t *)0);
    flair_desktop_present(bi, &ctx->off);

    serial_puts("FLAIR-MENU menu=");
    serial_puti((int32_t)menuID);
    serial_puts(" item=");
    serial_putu((uint32_t)sel_item);
    serial_puts(" (sel=0x");
    serial_puthex32(sel);
    serial_puts(")\n");
#else
    /* HER-14 MENU-NOOP mutant: no drop, no track, no select -- only the marker
     * with sel=0. The desktop under the title stays bare teal (ppm RED) and sel=0
     * proves nothing was chosen. NEVER define in a real build. */
    serial_puts("FLAIR-MENU menu=");
    serial_puti((int32_t)menuID);
    serial_puts(" item=0 (sel=0x00000000)\n");
    (void)ctx; (void)bi;
#endif
}

/* ===========================================================================
 * VISIBLE SOFTWARE MOUSE CURSOR -- a SAVE-UNDER, DIRTY-RECT arrow overlay on the
 * indexed-8 offscreen (beads initech-5l5z usability follow-on).
 *
 * Guarded behind -DFLAIR_LIVE_INTERACTIVE so the DEFAULT build/flair_live.img
 * (the gate image: test-flair-live/key/mouse/drag/menu + their mutants) is
 * BYTE-FOR-BEHAVIOR unchanged -- still bounded (250-tick FLAIR-LIVE-OK), still
 * NO cursor. The cursor + the unbounded pump exist ONLY in the interactive image.
 *
 * THE ARROW IS THE LOCKED FLAIR_CURSOR_ARROW (spec/assets/cursors.h), used
 * VERBATIM (NOT re-authored -- Law 1/Law 4; the hourglass is canon, the arrow is
 * the period-standard companion, ADR-0004 AM-4 / ADR-0006 BC-8). CURS format:
 * data[r]/mask[r] are uint16_t per row, MSB = col 0, 1 = ink; hotspot = tip =
 * (hot_row,hot_col) = (0,0). NOTE: the locked arrow has mask == data (a solid
 * silhouette, NO white dilation), so it composites as pure INK = CIDX_BLACK; the
 * (mask set & data clear) -> CIDX_WHITE branch is kept for generality but never
 * fires for this asset.
 *
 * The composite writes palette INDICES only (ADR-0004 OD-2): no raw LFB color
 * literals -- cursor_present_rect does the 8bpp index-copy / 24-32bpp direct-color
 * conversion (flair_palette_rgb + surface_put_pixel), exactly as the full
 * flair_desktop_present does, but bounded to the ~16x16 dirty rect (smooth blit).
 *
 * Ref: spec/assets/cursors.h (FLAIR_CURSOR_ARROW -- data/mask/hotspot);
 *      spec/assets/color_canon.h (CIDX_BLACK=0 / CIDX_WHITE=1); ADR-0004 OD-2
 *      (indexed-8 seam) / D-2 (the one surface module); ADR-0006. CLAUDE.md
 *      Law 1/2/3/4, Rule 2 (clip/fail-safe) / Rule 11 / Rule 12 (ASCII-clean).
 * ===========================================================================*/
#ifdef FLAIR_LIVE_INTERACTIVE
#include "cursors.h"   /* spec/assets/cursors.h (LOCKED arrow; -Ispec/assets)   */

enum { CURSOR_DIM = 16 };               /* the CURS sprite is 16x16             */
#define CURSOR_CIDX_INK    0u           /* CIDX_BLACK  (color_canon.h idx0)     */
#define CURSOR_CIDX_PAPER  1u           /* CIDX_WHITE  (color_canon.h idx1)     */

/* The save-under: the offscreen indices the sprite overwrote, restored on hide.
 * Keyed by the FULL 16x16 sprite grid (local row*DIM+col); only in-bounds cells
 * are touched on BOTH save and restore (recomputed identically from g_cur_ox/oy),
 * so the two stay symmetric even when the sprite is partly off-screen. */
static uint8_t g_cur_save[CURSOR_DIM * CURSOR_DIM];
static int     g_cur_shown = 0;    /* is the sprite currently composited?       */
static int     g_cur_ox    = 0;    /* composite top-left x of the shown sprite  */
static int     g_cur_oy    = 0;    /* composite top-left y of the shown sprite  */

/* cursor_present_rect -- blit ONLY [rx,rx+rw) x [ry,ry+rh) of the indexed-8
 * offscreen to the live LFB, honoring lfb_bpp + lfb_pitch. Models
 * flair_desktop_present's two inner loops (8bpp index-copy; 24/32bpp
 * flair_palette_rgb -> surface_put_pixel) but bounded to the dirty rect. The DAC
 * is already programmed by the initial full present, and the cursor uses idx0/1
 * which are in the palette, so no DAC reprogram is needed here. */
static void cursor_present_rect(const boot_info_t *bi, const bitmap_t *off,
                                int rx, int ry, int rw, int rh)
{
    int x, y;
    /* Clip the rect to the offscreen bounds (Rule 2; callers pass the sprite
     * rect, which may straddle an edge). */
    if (rx < 0) { rw += rx; rx = 0; }
    if (ry < 0) { rh += ry; ry = 0; }
    if (rx + rw > (int)off->width)  { rw = (int)off->width  - rx; }
    if (ry + rh > (int)off->height) { rh = (int)off->height - ry; }
    if (rw <= 0 || rh <= 0) { return; }

    if (bi->lfb_bpp == 8u) {
        volatile uint8_t *lfb = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
        for (y = 0; y < rh; y++) {
            const uint8_t *srow = (const uint8_t *)off->base
                                + (uint32_t)(ry + y) * off->pitch;
            volatile uint8_t *drow = lfb + (uint32_t)(ry + y) * bi->lfb_pitch;
            for (x = 0; x < rw; x++) {
                drow[rx + x] = srow[rx + x];
            }
        }
        return;
    }

    bitmap_t lbm;
    lbm.base            = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
    lbm.pitch           = bi->lfb_pitch;
    lbm.bpp             = bi->lfb_bpp;
    lbm.bytes_per_pixel = bi->lfb_bpp / 8u;
    lbm.width           = bi->lfb_width;
    lbm.height          = bi->lfb_height;
    for (y = 0; y < rh; y++) {
        const uint8_t *srow = (const uint8_t *)off->base
                            + (uint32_t)(ry + y) * off->pitch;
        for (x = 0; x < rw; x++) {
            uint32_t rgb  = flair_palette_rgb(srow[rx + x]) & 0x00FFFFFFu;
            uint32_t boff = (uint32_t)(ry + y) * lbm.pitch
                          + (uint32_t)(rx + x) * lbm.bytes_per_pixel;
            surface_put_pixel(&lbm, boff, rgb);
        }
    }
}

/* cursor_show -- save the offscreen indices under the 16x16 arrow at hotspot
 * (cx,cy), composite the LOCKED FLAIR_CURSOR_ARROW as indices, then present just
 * that rect. No-op if already shown (the move protocol always hides first). */
static void cursor_show(flair_live_ctx_t *ctx, const boot_info_t *bi,
                        int cx, int cy)
{
    const FLAIRCursor *cur = &FLAIR_CURSOR_ARROW;
    bitmap_t *off  = &ctx->off;
    int       W    = (int)off->width;
    int       H    = (int)off->height;
    uint8_t  *base = (uint8_t *)off->base;     /* explicit cast drops volatile  */
    uint32_t  pitch = off->pitch;
    int       ox   = cx - (int)cur->hot_col;   /* tip hotspot (0,0) -> ox = cx  */
    int       oy   = cy - (int)cur->hot_row;
    int       r, c;

    if (g_cur_shown) { return; }

    for (r = 0; r < CURSOR_DIM; r++) {
        int      py   = oy + r;
        uint16_t drow = cur->data[r];
        uint16_t mrow = cur->mask[r];
        for (c = 0; c < CURSOR_DIM; c++) {
            int      px = ox + c;
            uint16_t bit;
            uint8_t *p;
            if (px < 0 || px >= W || py < 0 || py >= H) { continue; }
            p = base + (uint32_t)py * pitch + (uint32_t)px;
            g_cur_save[r * CURSOR_DIM + c] = *p;          /* save under          */
            bit = (uint16_t)(0x8000u >> c);               /* MSB = col 0         */
            if (mrow & bit) {                             /* opaque sprite pixel */
                *p = (uint8_t)((drow & bit) ? CURSOR_CIDX_INK
                                            : CURSOR_CIDX_PAPER);
            }
        }
    }
    g_cur_ox = ox; g_cur_oy = oy; g_cur_shown = 1;
    cursor_present_rect(bi, off, ox, oy, CURSOR_DIM, CURSOR_DIM);
}

/* cursor_hide -- restore the saved indices at the LAST shown position (g_cur_ox/
 * oy) + present that rect, erasing the sprite (revealing the desktop). No-op if
 * not shown. Uses the stored origin so hide is exact regardless of where the
 * tracked cursor has since advanced (movement = hide() then show(new)). */
static void cursor_hide(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    bitmap_t *off   = &ctx->off;
    int       W     = (int)off->width;
    int       H     = (int)off->height;
    uint8_t  *base  = (uint8_t *)off->base;
    uint32_t  pitch = off->pitch;
    int       ox    = g_cur_ox;
    int       oy    = g_cur_oy;
    int       r, c;

    if (!g_cur_shown) { return; }

    for (r = 0; r < CURSOR_DIM; r++) {
        int py = oy + r;
        for (c = 0; c < CURSOR_DIM; c++) {
            int px = ox + c;
            if (px < 0 || px >= W || py < 0 || py >= H) { continue; }
            base[(uint32_t)py * pitch + (uint32_t)px] =
                g_cur_save[r * CURSOR_DIM + c];           /* restore under       */
        }
    }
    g_cur_shown = 0;
    cursor_present_rect(bi, off, ox, oy, CURSOR_DIM, CURSOR_DIM);
}
#endif /* FLAIR_LIVE_INTERACTIVE */
#endif /* BOOT_FLAIR_LIVE */

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
    b.ext_mem_kb = bi->ext_mem_kb;   /* extended RAM probed by stage2 (DEC-03) */

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

    /* Echo the probed extended-memory size (ADR-0004 DEC-03 / FO-G) so the boot
     * oracle can confirm stage2's INT 15h probe populated ext_mem_kb with a sane
     * value (e.g. ~130048 KiB for a 128 MiB QEMU/Bochs guest). This is
     * diagnostic only; the gating decision is the panic-below-min below, which
     * happens AFTER the console is up so PC LOAD LETTER can render on screen. */
    serial_puts("EXTMEM kb=");
    serial_putu(b.ext_mem_kb);
    serial_putc('\n');

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

    /* ------------------------------------------------------------------ *
     * FLAIR HEAP RAM-SUFFICIENCY GATE (beads initech-k8o5.5; ADR-0004
     * DEC-03 / FO-G). The FLAIR Toolbox heap is a FIXED extended-memory
     * window [FLAIR_HEAP_BASE, FLAIR_HEAP_BASE+FLAIR_HEAP_SIZE) (spec/
     * memory_map.h). stage2 probed installed RAM above 1 MiB into
     * boot_info_t.ext_mem_kb; if the machine does not have enough extended
     * RAM to back the whole window, the OS must REFUSE TO RUN (fail loud,
     * Rule 2) rather than scribble GUI state into RAM that is not there.
     *
     * The probe GATES boot; it NEVER alters the memory map (Rule 11 -- the
     * layout is identical every boot; the self-host fixpoint K2==K3 is
     * unaffected). The decision is the SAME pure function the host oracle
     * tests (flair_heap_ram_ok, memory_map.h), so artifact and oracle agree
     * by construction (Law 2). Real emulators (QEMU/Bochs default >= 128 MiB)
     * pass; the panic only fires on a genuinely under-provisioned machine.
     *
     * Reuse the fail-loud panic contract (panic.c): the grep-able "PANIC ..."
     * serial line + a diagnostic dump + the on-screen "PC LOAD LETTER" line
     * (rendered through the live console, bound to panic via panic_set_console
     * above), then a permanent cli;hlt. We do not have a CPU int_frame_t here
     * (this is a boot-policy panic, not a CPU exception), so we render the
     * PC LOAD LETTER line directly and dump the relevant values to serial. */
    if (!flair_heap_ram_ok(b.ext_mem_kb)) {
        /* Grep-able marker first (the oracle keys on "PANIC"). */
        serial_puts("PANIC flair-heap: insufficient extended RAM\n");
        serial_puts("  ext_mem_kb=");
        serial_putu(b.ext_mem_kb);
        serial_puts(" required_kb=");
        serial_putu((uint32_t)FLAIR_HEAP_REQUIRED_EXT_KB);
        serial_putc('\n');
        serial_puts("  FLAIR_HEAP=[0x");
        serial_puthex32(FLAIR_HEAP_BASE);
        serial_puts(",0x");
        serial_puthex32(FLAIR_HEAP_BASE + FLAIR_HEAP_SIZE);
        serial_puts(")\n");
        serial_puts("HALTED\n");

        /* The one visible line -- PC LOAD LETTER canon (Rule 2 / Law 4).
         * g_panic_con is bound iff the console came up; render through it. */
        if (g_int21_con) {
            console_puts(g_int21_con,
                "\nPC LOAD LETTER  (not enough memory for FLAIR)\n");
        }

        /* Terminal: never proceed into FLAIR/heap work on a machine that
         * cannot back the heap. IF handling is irrelevant; cli;hlt forever. */
        for (;;) {
            __asm__ __volatile__("cli; hlt");
        }
    }
    serial_puts("FLAIR-HEAP-OK\n");

#ifdef BOOT_FLAIR_SHELL
    /* THE LIVE FLAIR DESKTOP (beads initech-re30.3, LANE 1; THE milestone): build
     * the Office Space chimera desktop into an indexed-8 offscreen from the FLAIR
     * heap, render it (EXACTLY as the host oracle test_shell.c does), and PRESENT
     * it to the live LFB. This is a DEDICATED demo kernel: it does NOT touch the
     * normal boot (guarded by BOOT_FLAIR_SHELL), and after painting it emits the
     * grep-able "FLAIR-DESKTOP" marker (the screendump oracle's paint-complete
     * signal) then HALTS forever -- a STABLE screen for the screendump. It does
     * NOT fall through to FAT-mount / the REPL. (b is the boot_info_t snapshot
     * populated at the top of kernel_main.) */
    flair_desktop_run(&b, (flair_live_ctx_t *)0);
    serial_puts("FLAIR-DESKTOP\n");
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
#endif

#ifdef BOOT_FLAIR_LIVE
    /* THE FLAIR LIVE COOPERATIVE WaitNextEvent PUMP -- FO-7/8 (beads initech-5l5z;
     * ADR-0006 E-D4 / FO-7; landing-sequence Step 6). This is the CAPSTONE: it
     * makes the booted desktop INTERACTIVE -- the live, draggable arrangement that
     * is Law 4's literal goal -- by replacing the FO-4/5/6 bounded raw-drain wake
     * loop with the cooperative WaitNextEvent pump driving the ALREADY-BUILT,
     * host-mutation-proven Managers (BC-1 single spine: kmain is a thin source/sink
     * adapter; NO event/geometry logic is re-implemented here).
     *
     * This is the THIRD FLAIR demo kernel (alongside BOOT_FLAIR_SHELL), shipped
     * behind -DBOOT_FLAIR_LIVE so the DEFAULT boot stays the static render-once-
     * and-HLT frame (ADR-0006 FO-GAP-A; Rule 11).
     *
     * Sequence (ADR-0006 E-D4):
     *   1. Render + PRESENT the SAME FLAIR scene the BOOT_FLAIR_SHELL kernel does
     *      (flair_desktop_run, CAPTURING the scene handles into `ctx`; keep the
     *      initial present FIRST -- BC-4. On Bochs the 640x480 guard fires here and
     *      halts before the pump, so test-flair-live-bochs is unchanged.)
     *   2. flair_event_init(&g_flair_kbd_ring): the SPSC ring + cursor + tick base.
     *   3. Install the tick (FO-4) + kbd (FO-5) + mouse IRQ12 (FO-6) hooks, then
     *      sysinit_enable_irqs() + pic_unmask_irq12(): all three ISR producers live.
     *   4. Install the HLT yield (flair_event_set_yield) and run the pump:
     *      WaitNextEvent -> FindWindow part-code -> dispatch via the EXISTING verbs
     *      (inDrag -> DragWindow + desktop_paint_damage + present; inGoAway ->
     *      HideWindow + repaint; inMenuBar -> flair_live_do_menu, FO-8b). The pump emits
     *      one cooked FLAIR-EVT per delivered EventRecord (FO-5/6 marker
     *      reconciliation) and FLAIR-DRAG on a completed drag.
     *
     * DETERMINISTIC BOUNDED TERMINATION (Rule 11): the pump runs until a tick BUDGET
     * elapses (bounded by COUNT, never wall-clock), then emits FLAIR-LIVE-OK +
     * cli;hlt so the harness can screendump + assert. FLAIR-TICK is still announced
     * (test-flair-live: >=2 distinct) and FLAIR-LIVE-OK still terminates the loop.
     * Cooperative, not preemptive (BC-7): the pump holds the CPU + yields via HLT. */
    flair_live_ctx_t ctx;
#ifdef FLAIR_LIVE_TENANTS
    /* The FLAIR App Contract resident-app (process) list + the two reference
     * tenants HELLO/NOTES (ADR-0013; Wave-4 Step 4). Declared here so BOTH the
     * launch site (below FLAIR-DESKTOP) and the pump (below FLAIR-LIVE-READY)
     * reach them; all behind FLAIR_LIVE_TENANTS so the non-tenant arms compile
     * byte-identically. */
    FlairProcessList ten_plist;
    FlairApp        *ten_hello = (FlairApp *)0;
    FlairApp        *ten_notes = (FlairApp *)0;
#endif
    flair_desktop_run(&b, &ctx);
    serial_puts("FLAIR-DESKTOP\n");

#ifdef FLAIR_LIVE_TENANTS
    /* ===========================================================================
     * THE FLAIR APP CONTRACT, BOOTED (ADR-0013; Wave-4 Step 4; beads initech-4e35).
     * flair_desktop_run rendered + presented the canonical chimera scene with
     * show_modal==0 (above). Now: hide the two canon frame doc windows so the
     * desktop shows just the two menu bars + teal, launch HELLO + NOTES as
     * co-resident tenants on the SAME live Window Manager / offscreen, recomposite,
     * and present. The pump (below) routes every cooked event through the SOLE
     * Layer-5 spine flair_app_dispatch (ADR-0006 E-D2 single spine); NO routing
     * logic is re-implemented in kmain.
     * ===========================================================================*/

    /* (1) Hide BOTH canon frame doc windows. shell_build_scene drew the canonical
     * chrome (reused byte-for-byte; only show_modal differs); HideWindow leaves
     * them in the z-order but invisible, so FindWindow skips them -- no unowned-
     * content-click panic on the now bare desktop. */
    HideWindow(ctx.wm, &ctx.windows[0].rec);
    HideWindow(ctx.wm, &ctx.windows[1].rec);

    /* (2) Launch HELLO then NOTES (NOTES last => foreground, partially occluding
     * HELLO -- the spec/flair_tenants_demo.h overlap that makes the drop-updateEvt
     * mutant bite). FlairProcess_launch carves each tenant from the master FLAIR
     * heap and runs its open() into ctx.wm / ctx.off (ADR-0013 Sec 3.2). */
    FlairProcessList_init(&ten_plist);
    {
        rgn_rect_t hb, nb;   /* QuickDraw field order: top,left,bottom,right */
        hb.top = (int16_t)FLAIR_TEN_HELLO_T; hb.left   = (int16_t)FLAIR_TEN_HELLO_L;
        hb.bottom = (int16_t)FLAIR_TEN_HELLO_B; hb.right = (int16_t)FLAIR_TEN_HELLO_R;
        nb.top = (int16_t)FLAIR_TEN_NOTES_T; nb.left   = (int16_t)FLAIR_TEN_NOTES_L;
        nb.bottom = (int16_t)FLAIR_TEN_NOTES_B; nb.right = (int16_t)FLAIR_TEN_NOTES_R;
        ten_hello = FlairProcess_launch(&ten_plist, ctx.wm, &ctx.off, ctx.master,
                                        &hello_procs, FLAIR_TEN_HELLO_NAME, hb,
                                        (uint32_t)FLAIR_TEN_BUDGET);
        ten_notes = FlairProcess_launch(&ten_plist, ctx.wm, &ctx.off, ctx.master,
                                        &notes_procs, FLAIR_TEN_NOTES_NAME, nb,
                                        (uint32_t)FLAIR_TEN_BUDGET);
    }
    if (ten_hello == (FlairApp *)0 || ten_notes == (FlairApp *)0) {
        /* Fail-loud (Rule 2): a tenant launch that could not carve its partition
         * must not silently run a half-built desktop. */
        serial_puts("PANIC flair-tenants: FlairProcess_launch returned NULL "
                    "(budget/heap exhausted)\nHALTED\n");
        for (;;) { __asm__ __volatile__("cli; hlt"); }
    }

    /* (3) Each foreground tenant shows ITS menu set in the System-7 band (the
     * MultiFinder menu swap). Reuse the scene's two CANONICAL bars (no new chrome):
     * NOTES (the initial foreground) keeps the canon System-7 bar shell_render drew
     * at the top; HELLO swaps the Photoshop-exact bar in when it is raised. */
    ten_hello->menubar = &ctx.scene->bar_photoshop;
    ten_notes->menubar = &ctx.scene->bar_sys;

    /* (4) Recomposite the offscreen: teal desktop + the two tenants' window chrome
     * (back-to-front) + the two menu bars (shell_render; modal_up==0 => no dialog).
     * The compositor owns chrome; the tenant CONTENT is (re)drawn next. */
    shell_render(ctx.scene, &ctx.off);

    /* (5) Repaint each tenant's content in z-order through the updateEvt spine.
     * WindowMgr_invalidate clips the seed to each window's VISIBLE region, so the
     * background HELLO does NOT overdraw the front NOTES in the overlap; then
     * flair_route_updates delivers the updateEvt to each owning tenant front-to-
     * back. Result: NOTES full content (gray), HELLO exposed sliver (white), overlap
     * = NOTES_FILL -- the PRE app-switch state the O-5 grader expects. */
    if (ten_hello->windows != (WindowPtr)0)
        WindowMgr_invalidate(ctx.wm, ten_hello->windows,
                             region_get_bbox(ten_hello->windows->contRgn));
    if (ten_notes->windows != (WindowPtr)0)
        WindowMgr_invalidate(ctx.wm, ten_notes->windows,
                             region_get_bbox(ten_notes->windows->contRgn));
    flair_route_updates(&ten_plist, ctx.wm);

    /* (6) BC-4: present the co-resident tenant desktop BEFORE arming the pump. */
    flair_desktop_present(&b, &ctx.off);
    serial_puts("FLAIR-TENANTS-READY hello+notes co-resident; NOTES foreground\n");
#endif

    /* --- FO-5: initialise the FLAIR raw ring BEFORE installing any hook ------- *
     * flair_event_init resets the ring (head=tail=0), cursor state, and the
     * event.c tick counter to 0. IRQs are still masked here (before sti); no
     * producer can be running yet. Ref: event.h flair_event_init contract. */
    flair_event_init(&g_flair_kbd_ring);

    /* --- FO-4: install the PIT tick hook BEFORE sti (ADR-0006 E-D3a) --------- *
     * pit_set_tick_hook defaults NULL; only THIS kernel installs it, so every
     * other pit.o-linking kernel is byte-identical (Rule 11). The hook is
     * flair_tick_advance (os/flair/event.c:98): a single volatile g_tick++ --
     * the ADR-0004 D-4 ISR-enqueue-only minimum (no Toolbox, no alloc, no I/O).
     *
     * The FLAIR_LIVE_MUTATE_NO_HOOK Rule-6 mutant (defined ONLY by the mutant
     * image) SKIPS the install, so the tick base never advances -> FLAIR-TICK
     * never appears -> test-flair-live goes RED. NEVER define in a real build. */
#ifndef FLAIR_LIVE_MUTATE_NO_HOOK
    pit_set_tick_hook(flair_tick_advance);
#endif

    /* --- FO-5: install the kbd scancode hook BEFORE sti (ADR-0006 E-D3c) ------ *
     * kbd_set_scancode_hook defaults NULL; only THIS kernel installs the hook, so
     * every other kbd.o-linking kernel is byte-identical (Rule 11). The hook is
     * flair_live_kbd_post (above in this TU): posts FLAIR_RAW_KEYBOARD to
     * g_flair_kbd_ring -- the ADR-0004 D-4 ISR-enqueue-only minimum. The DOS
     * g_kbd ASCII ring (kbd.c) is kept 100% intact (additive; ADR-0006 BC-2).
     *
     * The FLAIR_LIVE_MUTATE_NO_KBD_HOOK Rule-6 mutant SKIPS this install, so
     * no FLAIR-KEY ever appears -> the FO-5 gate goes RED. NEVER define in a
     * real build. */
#ifndef FLAIR_LIVE_MUTATE_NO_KBD_HOOK
    kbd_set_scancode_hook(flair_live_kbd_post);
#endif

    /* --- FO-6: bring up the PS/2 mouse IRQ12 path BEFORE sti (ADR-0006 E-D3b) - *
     * THE minefield lane (CLAUDE.md "real mode -> protected is a minefield"; the
     * dual-8259A cascade EOI). Order (all BEFORE sti, with IRQ12 still masked):
     *   (a) install the IRQ12 IDT gate: vector 0x34 (slave 8259A IR4 = 0x30+4)
     *       -> irq12_entry (isr.asm). A 0x8E INTERRUPT gate, same as IRQ0/IRQ1.
     *   (b) mouse_init(): the 8042 aux bring-up (enable aux, set config bit1 /
     *       clear bit5, 0xF4 enable-reporting). Polls the 8042 with BOUNDED
     *       spins (Rule 11); the 0xF4 ACK is drained here by polling, NOT as a
     *       surprise IRQ12 (IRQ12 is still masked + IF=0).
     *   (c) install the mouse completed-packet hook (flair_live_mouse_post):
     *       posts FLAIR_RAW_MOUSE to the SAME g_flair_kbd_ring. Guarded by the
     *       FLAIR_LIVE_MUTATE_NO_MOUSE_HOOK Rule-6 mutant (skip the install ->
     *       no FLAIR-MOUSE -> test-flair-mouse RED). NEVER define in a real build.
     * pic_unmask_irq12() (the cascade + IRQ12 unmask) is issued AFTER
     * sysinit_enable_irqs() below, so every gate is installed before the line is
     * live. The hourglass busy CURS is canon (NOT a wristwatch; ADR-0006 BC-8). */
    idt_install_irq(0x34u, (void *)irq12_entry);   /* PS/2 mouse IRQ12 -> 0x34 */
    mouse_init();
#ifndef FLAIR_LIVE_MUTATE_NO_MOUSE_HOOK
    mouse_set_event_hook(flair_live_mouse_post);
#endif

    serial_puts("FLAIR-HOOK-SET\n");

    /* Bring up the PIT (100 Hz), the IRQ0/IRQ1 gates, unmask IRQ0+IRQ1, and sti
     * -- the SAME proven path the shell uses (sysinit.c). pit_init resets the
     * pit.o tick counter to 0 but does NOT touch g_tick_hook; flair_tick_advance
     * maintains event.c's own g_tick (read via flair_tick_count), which starts
     * at 0 after flair_event_init above. After this, IRQ0 fires ~every 10 ms and
     * IRQ1 fires on every keystroke from the 8042 output buffer. */
    sysinit_enable_irqs(serial_puts);

    /* --- FO-6: unmask the PS/2 mouse IRQ12 AFTER sti (ADR-0006 E-D3d) --------- *
     * sysinit_enable_irqs() above installed the IRQ0/IRQ1 gates, unmasked IRQ0+
     * IRQ1, and issued sti. The IRQ12 gate (0x34) + 8042 aux bring-up are already
     * done (before sti). Now clear BOTH the master IMR bit2 (the IRQ2 cascade)
     * AND the slave IMR bit4 (IRQ12) so the slave-8259A mouse line can reach the
     * CPU -- the cascade requirement (CLAUDE.md minefield). After this the mouse
     * fires IRQ12 on every PS/2 packet byte. Ref: ADR-0006 E-D3d/BC-3; pic.h. */
    pic_unmask_irq12();

    /* --- FO-7: install the cooperative HLT yield, then announce READY ---------- *
     * flair_event_set_yield installs the HLT sleep WaitNextEvent calls while the
     * ring is empty (ADR-0006 E-D4 step 1; cooperative, BC-7). FLAIR-LIVE-READY is
     * the deterministic inject trigger: the harness waits for it (the guest tells
     * us the pump is armed), THEN injects the locked mouse/key trace (ADR-0006
     * E-D6). The ring buffers any IRQ that lands before the first WaitNextEvent. */
    flair_event_set_yield(flair_live_yield);
    serial_puts("FLAIR-LIVE-READY\n");

    /* --- FO-7/8: THE WaitNextEvent PUMP --------------------------------------- *
     * The cooperative loop that makes the desktop INTERACTIVE. Each iteration:
     *   - WaitNextEvent cooks the raw ring (kbd/mouse) into one EventRecord,
     *     yielding via HLT while the ring is empty (up to WNE_SLICE ticks);
     *   - announce FLAIR-TICK for the first few advances (test-flair-live: the
     *     PIT time base still advances under the pump -- >=2 distinct required);
     *   - emit a cooked FLAIR-EVT per delivered event (FO-5/6 reconciliation);
     *   - on mouseDown, FindWindow -> part-code -> dispatch via the EXISTING verbs
     *     (inDrag -> flair_live_do_drag; inGoAway -> flair_live_do_close;
     *     menu-bar band -> flair_live_do_menu, the FO-8b live pull-down).
     *
     * BOUNDED TERMINATION (Rule 11): run until FLAIR_LIVE_TICK_BUDGET ticks elapse
     * (bounded by the 100 Hz PIT COUNT, never wall-clock), then emit FLAIR-LIVE-OK
     * + cli;hlt so the harness can screendump a STABLE post-drag frame. The budget
     * (~2.5 s) is a generous margin over the inject latency and the screendump, and
     * well within every gate timeout (test-flair-live 8000 ms; the drag gate). */
#ifdef FLAIR_LIVE_TENANTS
    /* --- THE FLAIR APP CONTRACT PUMP (ADR-0013; Wave-4 Step 4) ----------------- *
     * Same cooperative WaitNextEvent time base as the FO-7/8 chrome pump, but every
     * cooked event goes through the SOLE Layer-5 routing spine flair_app_dispatch
     * (ADR-0006 E-D2; ADR-0013 BC-2) -- NO routing logic re-implemented here. The
     * EXISTING chrome branch (inDrag/inGoAway/menu band) is kept VERBATIM for the
     * chrome part-codes the dispatcher leaves to the shell. On a foreground SWITCH
     * (click-to-activate raised a background tenant), route the newly-exposed
     * content's updateEvt (so the raised tenant repaints its overlap -- the drop-
     * updateEvt mutant leaves it stale), swap the foreground tenant's menubar into
     * the System-7 band (the MultiFinder menu swap), present, and emit
     * FLAIR-DISPATCH app=<name>. Bounded (gate) vs unbounded (FLAIR_LIVE_INTERACTIVE)
     * exactly like the chrome pump. */
    {
        enum { FLAIR_TEN_TICK_BUDGET   = 250 }; /* ~2.5 s @100 Hz: inject+dump margin */
        enum { FLAIR_TEN_TICK_ANNOUNCE = 4   }; /* announce the first 4 advances only */
        enum { FLAIR_TEN_WNE_SLICE     = 3   }; /* WaitNextEvent sleepTicks per call   */
        /* Menu-bar fg/bg for the foreground-tenant menu swap. The offscreen is the
         * indexed-8 surface, so DrawMenuBar's packed value is the low-byte palette
         * index (the test_menu.c / shell.c 8bpp convention: idx0 black ink, idx3
         * menubar gray -- SHELL_MENU_INK_IDX / SHELL_MENU_BG_IDX). */
        enum { FLAIR_TEN_MENU_FG_IDX = 0u, FLAIR_TEN_MENU_BG_IDX = 3u };
        uint32_t start = flair_tick_count();
        uint32_t last  = start;
        uint32_t seen  = 0u;

        /* A whole-bitmap GrafPort over the offscreen for the foreground-menubar
         * swap (DrawMenuBar paints rows [0, FLAIR_MENUBAR_H) == the top System-7
         * band). Mirrors flair_live_do_menu's port set-up. */
        GrafPort ten_barport;
        {
            rgn_rect_t whole = { 0, 0, (int16_t)ctx.off.height, (int16_t)ctx.off.width };
            ten_barport.portBits.bm     = ctx.off;
            ten_barport.portBits.bounds = whole;
            ten_barport.portRect        = whole;
            ten_barport.visRgn          = (region_t *)0;
            ten_barport.clipRgn         = (region_t *)0;
            ten_barport.pnLoc.v = 0; ten_barport.pnLoc.h = 0;
            ten_barport.pnSize.v = 1; ten_barport.pnSize.h = 1;
            ten_barport.pnVis = 0;
            ten_barport.grafProcs = (QDProcs *)0;
        }

#ifdef FLAIR_LIVE_INTERACTIVE
        int cur_show_h = (int)(FLAIR_SCREEN_W / 2);
        int cur_show_v = (int)(FLAIR_SCREEN_H / 2);
        cursor_show(&ctx, &b, cur_show_h, cur_show_v);
        for (;;) {
#else
        while ((flair_tick_count() - start) < (uint32_t)FLAIR_TEN_TICK_BUDGET) {
#endif
            EventRecord ev;
            int got = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev,
                                    (uint32_t)FLAIR_TEN_WNE_SLICE);

            uint32_t now = flair_tick_count();
            if (now != last) {
                last = now;
                seen++;
                if (seen <= (uint32_t)FLAIR_TEN_TICK_ANNOUNCE) {
                    serial_puts("FLAIR-TICK n=");
                    serial_putu(now);
                    serial_putc('\n');
                }
            }

#ifdef FLAIR_LIVE_INTERACTIVE
            {
                int cur_moved   = ((int)ev.where.h != cur_show_h ||
                                   (int)ev.where.v != cur_show_v);
                int do_dispatch = (got && ev.what == (uint16_t)mouseDown);
                if (cur_moved || do_dispatch) {
                    cursor_hide(&ctx, &b);
                }
                if (!got) {
                    if (cur_moved) {
                        cur_show_h = (int)ev.where.h;
                        cur_show_v = (int)ev.where.v;
                        cursor_show(&ctx, &b, cur_show_h, cur_show_v);
                    }
                    continue;   /* nullEvent: track the cursor, keep pumping */
                }
            }
#else
            if (!got) {
                continue;   /* timeout / nullEvent: keep ticking to the budget */
            }
#endif

            /* Cooked-event marker (FO-5/6 reconciliation). */
            flair_live_emit_evt(&ev);

            /* THE SOLE ROUTING CALL (ADR-0006 E-D2 / ADR-0013 BC-2 single spine).
             * inContent click-to-activate (raise group + activate pair + deliver)
             * lives entirely inside flair_app_dispatch; chrome part-codes are left
             * to the shell branch below (the dispatcher returns early for them). */
            FlairApp *ten_prev_fg = ten_plist.head;
            flair_app_dispatch(&ten_plist, ctx.wm, &ev);

            /* The EXISTING chrome branch, VERBATIM, for the chrome part-codes the
             * dispatcher does not own (inDrag/inGoAway/the menu-bar band). */
            if (ev.what == (uint16_t)mouseDown) {
                WindowPtr w = (WindowPtr)0;
                flair_part_code_t pc = FindWindow(ctx.wm, ev.where, &w);
                if (pc == inDrag && w != (WindowPtr)0) {
                    flair_live_do_drag(&ctx, &b, w, ev.where);
                } else if (pc == inGoAway && w != (WindowPtr)0) {
                    flair_live_do_close(&ctx, &b, w);
                } else if (ev.where.v >= 0 &&
                           ev.where.v < (int16_t)FLAIR_MENUBAR_H) {
                    flair_live_do_menu(&ctx, &b, &ctx.scene->bar_sys, ev.where);
                }
            }

            /* POST-ACTIVATION: a click-to-activate switched the foreground tenant.
             * Repaint the raised tenant's newly-exposed content (the updateEvt
             * route), swap its menubar into the System-7 band, present, announce.
             * NOTE: flair_route_updates MUST run AFTER it is seeded and is NOT
             * preceded by desktop_paint_damage on the SAME updateRgn -- desktop.c
             * desktop_paint_damage validates (clears) every updateRgn at its tail
             * (desktop.c:233), so it runs FIRST (to service any raise exposure of
             * the desktop/chrome) and the content seed+route comes after it. */
            if (ten_plist.head != (FlairApp *)0 && ten_plist.head != ten_prev_fg) {
                desktop_paint_damage(ctx.wm, &ctx.off, ctx.comp);
                if (ten_plist.head->windows != (WindowPtr)0) {
                    WindowMgr_invalidate(ctx.wm, ten_plist.head->windows,
                            region_get_bbox(ten_plist.head->windows->contRgn));
                }
#ifndef FLAIR_LIVE_MUTATE_DROP_UPDATE
                flair_route_updates(&ten_plist, ctx.wm);
#else
                /* MUTANT FLAIR_LIVE_MUTATE_DROP_UPDATE (Rule 6; the O-5 tenants
                 * emu-mutant image ONLY): SKIP the updateEvt route after the switch.
                 * The raised tenant is never handed its updateEvt, so it never
                 * repaints the newly-exposed overlap -- the exposed region keeps the
                 * OLD foreground's content colour -> the booted O-5 gate's TIER-A
                 * overlap probe stays NOTES_FILL (RED). The switch + menubar swap
                 * still happen (FLAIR-DISPATCH still fires). NEVER in a real build. */
#endif
#ifndef FLAIR_LIVE_MUTATE_NO_MENUBAR_SWAP
                DrawMenuBar(&ten_barport, ten_plist.head->menubar,
                            (uint32_t)FLAIR_TEN_MENU_FG_IDX,
                            (uint32_t)FLAIR_TEN_MENU_BG_IDX, (const region_t *)0);
#else
                /* MUTANT FLAIR_LIVE_MUTATE_NO_MENUBAR_SWAP (Rule 6; the O-5 tenants
                 * emu-mutant image ONLY): SKIP the foreground-tenant menubar swap.
                 * The System-7 band keeps the OLD foreground's menu titles, so the
                 * booted O-5 gate's MENU-BAND differential finds the title strip
                 * UNCHANGED pre-vs-post (0 diffs) -> RED. The raise + activate still
                 * happen (TIER-A/TIER-B stay GREEN). NEVER in a real build. */
                (void)ten_barport;  /* referenced (else -Werror=unused-but-set-variable) */
#endif
                flair_desktop_present(&b, &ctx.off);
                serial_puts("FLAIR-DISPATCH app=");
                serial_puts(ten_plist.head->name ? ten_plist.head->name : "?");
                serial_putc('\n');
            }

#ifdef FLAIR_LIVE_INTERACTIVE
            /* Re-show the cursor on the freshly repainted desktop, on top. */
            cur_show_h = (int)ev.where.h;
            cur_show_v = (int)ev.where.v;
            cursor_show(&ctx, &b, cur_show_h, cur_show_v);
#endif
        }
    }
#else
    {
        enum { FLAIR_LIVE_TICK_BUDGET   = 250 }; /* ~2.5 s @100 Hz: inject+dump margin */
        enum { FLAIR_LIVE_TICK_ANNOUNCE = 4   }; /* announce the first 4 advances only */
        enum { FLAIR_LIVE_WNE_SLICE     = 3   }; /* WaitNextEvent sleepTicks per call   */
        uint32_t start = flair_tick_count();
        uint32_t last  = start;
        uint32_t seen  = 0u;
#ifdef FLAIR_LIVE_INTERACTIVE
        /* The currently-shown cursor hotspot. event.c inits the tracked cursor to
         * the screen centre (FLAIR_SCREEN_W/2, FLAIR_SCREEN_H/2), so SHOW the arrow
         * there once before the loop -- the cursor is visible before the first
         * move. Reading the constants (not the ring) is deterministic and consumes
         * no events. The pump reads the LIVE position each iteration from ev.where,
         * which WaitNextEvent stamps from g_cursor on EVERY return (event.c: the
         * nullEvent timeout path and cook_raw both stamp where), so no event.c
         * accessor is needed. */
        int cur_show_h = (int)(FLAIR_SCREEN_W / 2);
        int cur_show_v = (int)(FLAIR_SCREEN_H / 2);
        cursor_show(&ctx, &b, cur_show_h, cur_show_v);
#endif

        /* INTERACTIVE: run until power-off (the operator drags windows + uses menus
         * with a visible cursor). DEFAULT (gate): bounded by FLAIR_LIVE_TICK_BUDGET
         * ticks then FLAIR-LIVE-OK + cli;hlt (Rule 11, deterministic screendump). */
#ifdef FLAIR_LIVE_INTERACTIVE
        for (;;) {
#else
        while ((flair_tick_count() - start) < (uint32_t)FLAIR_LIVE_TICK_BUDGET) {
#endif
            EventRecord ev;
            int got = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev,
                                    (uint32_t)FLAIR_LIVE_WNE_SLICE);

            /* FO-4 tick announce (still the cooperative time base under the pump). */
            uint32_t now = flair_tick_count();
            if (now != last) {
                last = now;
                seen++;
                if (seen <= (uint32_t)FLAIR_LIVE_TICK_ANNOUNCE) {
                    serial_puts("FLAIR-TICK n=");
                    serial_putu(now);
                    serial_putc('\n');
                }
            }

#ifdef FLAIR_LIVE_INTERACTIVE
            /* SOFTWARE CURSOR (interactive build only). The LIVE cursor is ev.where
             * (WaitNextEvent stamps it from g_cursor on every return). HIDE the
             * sprite at the TOP of any reposition or dispatch so the save-under
             * holds the REAL desktop before a handler repaints + presents, then
             * SHOW it again at the bottom so it floats on top (the robust rule). A
             * drag advances the cursor past ev.where; the next iteration's
             * moved-test repositions it within one WNE slice. */
            {
                int cur_moved   = ((int)ev.where.h != cur_show_h ||
                                   (int)ev.where.v != cur_show_v);
                int do_dispatch = (got && ev.what == (uint16_t)mouseDown);
                if (cur_moved || do_dispatch) {
                    cursor_hide(&ctx, &b);
                }
                if (!got) {
                    if (cur_moved) {
                        cur_show_h = (int)ev.where.h;
                        cur_show_v = (int)ev.where.v;
                        cursor_show(&ctx, &b, cur_show_h, cur_show_v);
                    }
                    continue;   /* nullEvent: track the cursor, keep pumping */
                }
                flair_live_emit_evt(&ev);
                if (ev.what == (uint16_t)mouseDown) {
                    WindowPtr w = (WindowPtr)0;
                    flair_part_code_t pc = FindWindow(ctx.wm, ev.where, &w);
                    if (pc == inDrag && w != (WindowPtr)0) {
                        flair_live_do_drag(&ctx, &b, w, ev.where);
                    } else if (pc == inGoAway && w != (WindowPtr)0) {
                        flair_live_do_close(&ctx, &b, w);
                    } else if (ev.where.v >= 0 &&
                               ev.where.v < (int16_t)FLAIR_MENUBAR_H) {
                        flair_live_do_menu(&ctx, &b, &ctx.scene->bar_sys, ev.where);
                    }
                }
                /* Re-show on the freshly repainted desktop, on top. */
                cur_show_h = (int)ev.where.h;
                cur_show_v = (int)ev.where.v;
                cursor_show(&ctx, &b, cur_show_h, cur_show_v);
            }
#else
            if (!got) {
                continue;   /* timeout / nullEvent: keep ticking to the budget */
            }

            /* Cooked-event marker (FO-5/6 marker reconciliation). */
            flair_live_emit_evt(&ev);

            if (ev.what == (uint16_t)mouseDown) {
                WindowPtr w = (WindowPtr)0;
                flair_part_code_t pc = FindWindow(ctx.wm, ev.where, &w);
                if (pc == inDrag && w != (WindowPtr)0) {
                    flair_live_do_drag(&ctx, &b, w, ev.where);
                } else if (pc == inGoAway && w != (WindowPtr)0) {
                    flair_live_do_close(&ctx, &b, w);
                } else if (ev.where.v >= 0 &&
                           ev.where.v < (int16_t)FLAIR_MENUBAR_H) {
                    /* inMenuBar (ADR-0006 FO-8): the click is in the TOP System-7
                     * menu-bar band [0,FLAIR_MENUBAR_H). FindWindow returns inDesk
                     * for it (no document window covers the bar; window.c FindWindow
                     * has no inMenuBar branch), so the y<MENUBAR_H band test IS the
                     * menu-bar hit. DROP the System-7 bar's pull-down LIVE -- the
                     * FO-8b deliverable (Law 4 "working menus"). */
                    flair_live_do_menu(&ctx, &b, &ctx.scene->bar_sys, ev.where);
                }
            }
#endif
        }
    }
#endif /* FLAIR_LIVE_TENANTS (the App Contract pump) vs the FO-7/8 chrome pump */
    serial_puts("FLAIR-LIVE-OK\n");
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
#endif

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

#ifdef BOOT_SPURIOUS
    /* SELF-TEST BUILD ONLY (bcg.6; make test-spurious). Fire a stray, unhandled
     * software interrupt AFTER the banner to prove the spurious/unhandled-vector
     * path RESUMES (clean iret) instead of wedging the machine forever. Vector
     * 0x70 is not a CPU exception, not an IRQ (the PIC is remapped to 0x28/0x30),
     * and not an INT 21h trap gate -- it routes to isr_spurious (sentinel 0xFF).
     * The handler must emit "SPURIOUS vec=FF ... resuming" and RETURN; we then
     * print SPURIOUS-RESUMED, proving execution continued past the stray int and
     * the machine was NOT wedged. The NORMAL image never defines this macro, so
     * test-boot stays unchanged. */
    serial_puts("SPURIOUS-ARMED\n");
    __asm__ __volatile__("int $0x70");      /* generic unhandled -> isr_spurious (0xFF) */
    serial_puts("SPURIOUS-RESUMED\n");
    /* The two 8259A spurious-IRQ vectors (bcg.7): IRQ7 (0x2F, master, no EOI)
     * and IRQ15 (0x37, slave, master-only EOI). Both must resume. (A real
     * spurious IRQ is non-deterministic to provoke; a software int exercises
     * the dedicated stub + EOI-discipline + resume path. The EOI is a harmless
     * no-op here since nothing is in service.) */
    __asm__ __volatile__("int $0x2F");
    serial_puts("SPURIOUS-IRQ7-RESUMED\n");
    __asm__ __volatile__("int $0x37");
    serial_puts("SPURIOUS-IRQ15-RESUMED\n");
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
    crit_blockdev_t crit_fatdev;   /* INT 24h critical-error wrapper (beads mvg) */
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

        /* Wrap the FAT backend in the INT 24h critical-error layer (beads
         * initech-mvg): once armed, a HARD sector-I/O failure raises the real
         * INT 24h critical-error handler (MSG-DOS-0001) and honors Abort/Retry/
         * Fail, instead of silently returning an error up the syscall. The
         * INT 25h/26h absolute-disk seam below stays on the INNER &fatdev (its
         * own read-only/bounds contract is unchanged).
         *
         * The hook is NOT armed yet -- the wrapper stays TRANSPARENT through the
         * boot-time mount probe so a boot WITHOUT a data disk still fails the
         * mount gracefully and continues (real DOS raises the critical-error
         * prompt for PROGRAM disk I/O, not for boot-time drive probing). The
         * hook is armed only AFTER a successful mount (below), so it covers the
         * shell/program file I/O that runs through this volume. */
        crit_blockdev_init(&crit_fatdev, &fatdev);

        rc = fat12_mount(&vol, &crit_fatdev.dev, sector_buf);
        if (rc == FAT12_OK) {
            serial_puts("FAT-MOUNT-OK\n");
            mounted = 1;

            /* Mount succeeded -> ARM the INT 24h critical-error hook now, so the
             * shell/program file I/O through this volume raises MSG-DOS-0001 on a
             * hard sector failure (beads initech-mvg). Boot-time probing above
             * ran transparent so a missing disk still degraded gracefully. */
            crit_blockdev_set_hook(int21_run_critical_error);

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
             * (beads initech-saw). The loader SHARES the file backend's already-
             * loaded whole-FAT buffer (fileio_fat_fat_buffer, bound just above)
             * instead of keeping a second 6 KiB copy in kernel .bss -- this frees
             * the duplicate that drove the recurring kernel-window pressure (beads
             * y206/headroom). A windowed FAT16 volume returns NULL -> the loader
             * stays unbound (fail loud; EXEC-from-FAT16 unsupported this milestone). */
            {
                uint32_t load_fat_len = 0;
                const uint8_t *load_fat =
                    (const uint8_t *)fileio_fat_fat_buffer(&load_fat_len);
                loader_bind_fat_volume(&vol, load_fat, load_fat_len);
            }
            int21_set_exec_backend(loader_exec_by_name);
            serial_puts("LOADER-FAT-BIND-OK\n");

            /* Bind the INT 25h/26h ABSOLUTE-DISK seam (ADR-0003 DEC-15, beads
             * initech-4mq7) from THIS mounted volume's blockdev. ONLY on the
             * mounted==1 path (never a stale dev on the mount-failure path).
             * total_sectors = the BPB total-logical-sectors (the 16-bit field,
             * or the 32-bit field when the 16-bit is 0 -- DOS BPB rule), so the
             * bounds check (DX>=total / DX+CX>total) self-adjusts to the media.
             * The seam reaches the disk through the adapter thunks above; int21
             * never sees the blockdev/FAT types (Law 3). */
            g_absdisk_dev = &fatdev;
            {
                uint32_t total = vol.bpb.total_sectors_16 != 0u
                                   ? (uint32_t)vol.bpb.total_sectors_16
                                   : vol.bpb.total_sectors_32;
                int21_absdisk_backend_t absdisk;
                absdisk.read          = int21_absdisk_read;
                absdisk.write         = int21_absdisk_write;
                absdisk.total_sectors = total;
                int21_set_blockdev(&absdisk);
            }
            serial_puts("ABSDISK-BIND-OK\n");
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

    /* Wire ANSI.SYS into the CON output path (beads initech-p96i): bind the
     * console-ops vtable (ctx = the live console) + the ENABLE gate
     * (sysinit_ansi_enabled, set by CONFIG.SYS DEVICE=ANSI.SYS, applied in the
     * SYSINIT Phase-2 above). From here con_putc feeds the ANSI FSM whenever
     * ANSI.SYS is loaded; escape sequences move the cursor / set colour / erase,
     * and escape-free output renders identically (ansi_put_char fans to
     * console+serial exactly like int21_con_sink, so serial markers survive). */
    {
        int21_ansi_console_t ansi_ops;
        ansi_ops.put_char      = ansi_put_char;
        ansi_ops.set_cursor    = ansi_set_cursor;
        ansi_ops.cursor_rel    = ansi_cursor_rel;
        ansi_ops.erase_display = ansi_erase_display;
        ansi_ops.erase_line    = ansi_erase_line;
        ansi_ops.set_attr      = ansi_set_attr;
        ansi_ops.get_cursor    = ansi_get_cursor;
        ansi_ops.ctx           = g_int21_con;
        int21_set_ansi_console(&ansi_ops);
        int21_set_ansi_gate(sysinit_ansi_enabled);
    }
    serial_puts("ANSI-WIRED\n");

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
         * (beads initech-509.2 fix). The window has since been raised to 0x40000
         * (beads initech-5pe + o0td + re30.2 map shifts), but reusing the resident FAT is still the right
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
        int21_cwd_snapshot_t cwd_snap = int21_cwd_save();   /* save kernel CWD (mzxa) */
        loader_status_t st = load_program_from_fat("GREET.COM", 0u /* root */,
                                                   (const char *)0, 0u,
                                                   0u /* env: inherit-empty */,
                                                   &saw_rc);
        int21_set_exit(int21_exit_hook);   /* restore kernel-context terminate */
        int21_set_psp(&g_kernel_psp);      /* restore kernel-context JFT */
        int21_cwd_restore(&cwd_snap);      /* restore kernel-context CWD */
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

#ifdef BOOT_MZEXEC
    /* InitechMZ (.EXE) RUNTIME-PROOF self-test (beads initech-0kiq / dtw.2-emu;
     * ADR-0003 DEC-08a). Only in the -DBOOT_MZEXEC image so the normal boot is
     * unchanged. Requires --disk2 carrying MZEXEC.EXE (the mzlink container of
     * mzexec_fixture.asm), which is NOT baked -- it lives on the FAT volume, so
     * this proves a FROM-FAT MZ load through the REAL kernel path.
     *
     * THE RUNTIME PROOF (the complement to the host oracle test_mzload): call
     * load_program_from_fat("MZEXEC.EXE", ...) -- the SAME proven saw core the
     * BOOT_EXEC block uses for GREET.COM, EXCEPT the content dispatch (mz_is_mz)
     * routes the 'MZ' image through load_program_mz_in_place ->
     * loader_prepare_mz: parse the container, move the load module down over the
     * header, APPLY THE FLAT-32 RELOCATION (add PROGRAM_IMAGE to the dword at the
     * reloc site), then the ONE shared loader_run_plan transfer (JMP to
     * PROGRAM_IMAGE). The fixture's `mov edx, msg` was assembled at org 0, so its
     * imm32 is msg's offset RELATIVE TO LOAD BASE 0 -- WRONG at the real load
     * address (0x40100) UNLESS relocated. So:
     *   WITH reloc:    EDX = 0x40100 + msg_off -> AH=09h prints "MZEXEC-OK".
     *   WITHOUT reloc: EDX = msg_off (~0x11)   -> low RAM, no marker.
     * The gate asserts "MZEXEC-OK" on serial AND MZEXEC-EXIT rc=9 -- present iff
     * the relocation resolved AT RUNTIME (Law 2). The mzlink --no-reloc container
     * variant (test-mzexec-mutant) drops the fixup; the marker is ABSENT and the
     * gate goes RED (Rule 6 -- the runtime sibling of LOADER_MUTATE_MZ_NO_RELOC).
     *
     * We restore the kernel-context exit hook + PSP + CWD after the child (the
     * loader rebinds all three during a run), exactly as the BOOT_EXEC saw block. */
    serial_puts("MZEXEC-BEGIN\n");
    {
        uint8_t mz_rc = 0;
        int21_cwd_snapshot_t cwd_snap = int21_cwd_save();   /* save kernel CWD (mzxa) */
        loader_status_t st = load_program_from_fat("MZEXEC.EXE", 0u /* root */,
                                                   (const char *)0, 0u,
                                                   0u /* env: inherit-empty */,
                                                   &mz_rc);
        int21_set_exit(int21_exit_hook);   /* restore kernel-context terminate */
        int21_set_psp(&g_kernel_psp);      /* restore kernel-context JFT */
        int21_cwd_restore(&cwd_snap);      /* restore kernel-context CWD */
        if (st == LOADER_OK) {
            serial_puts("MZEXEC-EXIT rc=");
            serial_putu((uint32_t)mz_rc);
            serial_putc('\n');
        } else {
            serial_puts("MZEXEC-FAIL st=");
            serial_putu((uint32_t)st);
            serial_putc('\n');
        }
    }
    serial_puts("MZEXEC-END\n");
#endif

#ifdef BOOT_MZFOREIGN
    /* InitechMZ FAIL-LOUD FOREIGN-MZ self-test (beads initech-0kiq / dtw.2-emu;
     * ADR-0003 DEC-08a.5). Only in the -DBOOT_MZFOREIGN image. Requires --disk2
     * carrying FOREIGN.EXE: a valid MZ container whose e_res[0] tag is 0 (an
     * UNTAGGED 16-bit DOS .EXE, mzlink --foreign). The flat 32-bit CPU cannot
     * decode 16-bit code, so relocating + jumping to it would silently misexecute.
     * The kernel MUST refuse: load_program_from_fat -> mz_is_mz -> load_program_mz
     * _in_place -> loader_prepare_mz returns LOADER_ERR_FOREIGN_MZ -> the call site
     * calls loader_panic_foreign_mz, which emits "PANIC foreign-mz" to serial and
     * halts forever (cli;hlt). The emu gate asserts that marker AND that the
     * program's own marker NEVER appears (it never ran), AND no triple-fault.
     *
     * loader_panic_foreign_mz is NORETURN, so MZFOREIGN-AFTER must NOT appear on
     * serial -- if it does, the kernel did NOT panic on a foreign MZ (the honesty
     * gate failed). The harness sees the kernel halt INSIDE the EXEC. */
    serial_puts("MZFOREIGN-BEGIN\n");
    {
        uint8_t frc = 0;
        int21_cwd_snapshot_t cwd_snap = int21_cwd_save();
        loader_status_t st = load_program_from_fat("FOREIGN.EXE", 0u /* root */,
                                                   (const char *)0, 0u,
                                                   0u /* env: inherit-empty */,
                                                   &frc);
        /* Reached ONLY if the loader did NOT panic (it must -- DEC-08a.5). Restore
         * the kernel context and report the (wrong) outcome fail-loud (Rule 2). */
        int21_set_exit(int21_exit_hook);
        int21_set_psp(&g_kernel_psp);
        int21_cwd_restore(&cwd_snap);
        serial_puts("MZFOREIGN-AFTER st=");   /* the gate asserts this is ABSENT */
        serial_putu((uint32_t)st);
        serial_putc('\n');
    }
    serial_puts("MZFOREIGN-END\n");
#endif

#ifdef BOOT_MEMTEST
    /* MEMORY ARENA self-test (beads initech-509.6; make test-mcb-emu). Only in
     * the -DBOOT_MEMTEST image so the normal boot is unchanged. Drives the REAL
     * `int 0x21` trap path for AH=48h ALLOC / 4Ah SETBLOCK / 49h FREE over the
     * kernel-bound MCB arena ([PROGRAM_BASE, PROGRAM_ALLOC_END), sysinit_early),
     * from KERNEL context (the kernel PSP owns what it allocs). Proves the seam
     * end to end on real hardware emulation: a returned DOS segment is usable
     * memory (write a sentinel, read it back), SETBLOCK grows it, FREE +
     * re-ALLOC of the same size returns the SAME segment. The harness asserts
     * MEM-A=<seg> / MEM-WROTE / MEM-GROW-OK / MEM-FREED / MEM-OK + triple_fault=0.
     * The no-segbase mutant (test-mcb-emu-mutant) makes the read-back sentinel
     * mismatch -> MEM-BAD -> RED, proving the gate bites. */
    serial_puts("MEM-BEGIN\n");
    {
        /* AH=48h ALLOCATE 0x40 paragraphs. BX = paragraphs; AX = segment, CF=0. */
        uint32_t ax_a = 0x4800u;   /* AH=48h */
        uint32_t bx_a = 0x40u;     /* BX = 64 paragraphs (1 KiB) */
        uint32_t carry_a = 0;
        __asm__ __volatile__(
            "int $0x21\n\t"
            "sbb %2, %2\n\t"
            : "+a"(ax_a), "+b"(bx_a), "=r"(carry_a)
            : : "cc", "memory");
        uint16_t seg_a = (uint16_t)(ax_a & 0xFFFFu);
        if (carry_a != 0u) {
            serial_puts("MEM-BAD alloc1 ax="); serial_putu(ax_a & 0xFFFFu);
            serial_putc('\n');
        } else {
            serial_puts("MEM-A="); serial_putu(seg_a); serial_putc('\n');

            /* The segment is a paragraph address: linear = seg << 4. Write a
             * sentinel byte and read it back to prove the block is real memory. */
            volatile uint8_t *blk = (volatile uint8_t *)((uintptr_t)seg_a << 4);
            blk[0] = 0xA5u;
            blk[1] = 0x5Au;
            if (blk[0] == 0xA5u && blk[1] == 0x5Au) {
                serial_puts("MEM-WROTE\n");
            } else {
                serial_puts("MEM-BAD readback\n");
            }

            /* AH=4Ah SETBLOCK grow to 0x80 paragraphs. BX = segment, CX = new
             * size; CF=0 on success. */
            uint32_t ax_g = 0x4A00u;
            uint32_t bx_g = (uint32_t)seg_a;
            uint32_t cx_g = 0x80u;
            uint32_t carry_g = 0;
            __asm__ __volatile__(
                "int $0x21\n\t"
                "sbb %3, %3\n\t"
                : "+a"(ax_g), "+b"(bx_g), "+c"(cx_g), "=r"(carry_g)
                : : "cc", "memory");
            serial_puts(carry_g ? "MEM-BAD grow\n" : "MEM-GROW-OK\n");

            /* AH=49h FREE the block. BX = segment; CF=0 on success. */
            uint32_t ax_f = 0x4900u;
            uint32_t bx_f = (uint32_t)seg_a;
            uint32_t carry_f = 0;
            __asm__ __volatile__(
                "int $0x21\n\t"
                "sbb %2, %2\n\t"
                : "+a"(ax_f), "+b"(bx_f), "=r"(carry_f)
                : : "cc", "memory");
            serial_puts(carry_f ? "MEM-BAD free\n" : "MEM-FREED\n");

            /* AH=48h re-ALLOCATE the SAME 0x40 paragraphs -- coalesce restored the
             * tail, so the SAME segment must come back. */
            uint32_t ax_b = 0x4800u;
            uint32_t bx_b = 0x40u;
            uint32_t carry_b = 0;
            __asm__ __volatile__(
                "int $0x21\n\t"
                "sbb %2, %2\n\t"
                : "+a"(ax_b), "+b"(bx_b), "=r"(carry_b)
                : : "cc", "memory");
            uint16_t seg_b = (uint16_t)(ax_b & 0xFFFFu);
            if (carry_b == 0u && seg_b == seg_a) {
                serial_puts("MEM-OK\n");
            } else {
                serial_puts("MEM-BAD realloc seg="); serial_putu(seg_b);
                serial_putc('\n');
            }
        }
    }
    serial_puts("MEM-END\n");
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

#ifdef BOOT_ABSDISK
    /* INT 25h/26h ABSOLUTE-DISK asm-stub round-trip self-test (beads initech-8403;
     * make test-absdisk-emu). Only in the -DBOOT_ABSDISK image so the normal boot
     * is unchanged. Requires a WRITABLE FAT12 data disk on --disk2 so the FAT-MOUNT
     * block above bound the absolute-disk seam (ABSDISK-BIND-OK).
     *
     * THE COVERAGE GAP THIS CLOSES (Law 2): the host oracle (test_absdisk.c, beads
     * initech-4mq7) drives int25_dispatch / int26_dispatch DIRECTLY as C calls; the
     * asm  int25_entry/int26_entry -> dispatch -> IRETD  return path (isr.asm) is
     * invoked by NO host/emu gate. The baked ABSDISK program issues a REAL
     * `int $0x26` (WRITE a deterministic pattern to the SAFE scratch LBA 2879 ==
     * total_sectors-1, FREE on the blank scratch disk -- never boot/FAT/root) then
     * a REAL `int $0x25` (READ it back) through the live IDT trap gates 0x25/0x26,
     * byte-compares, and emits ABS-W26=OK / ABS-R25=OK / ABS-RT=OK. The harness
     * asserts all three + triple_fault=0 -- proving the WHOLE asm round-trip
     * (entry stub -> dispatch -> seam -> IRETD with CF) works end-to-end. Mirrors
     * BOOT_WRITE (also a pre-sti ATA round-trip) and BOOT_VECT (the int24_entry emu
     * keystone). The NORMAL image never defines BOOT_ABSDISK. */
    serial_puts("ABSDISK-OUTPUT-BEGIN\n");
    run_baked("ABSDISK", g_absdisk_prog_image, g_absdisk_prog_image_len);
    serial_puts("ABSDISK-OUTPUT-END\n");
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
            int21_cwd_snapshot_t cwd_snap = int21_cwd_save();  /* save kernel CWD (mzxa) */
            loader_status_t st = load_program_from_fat("EXITH.COM", 0u /* root */,
                                                       (const char *)0, 0u,
                                                       0u /* env: inherit-empty */,
                                                       &rc);
            int21_set_exit(int21_exit_hook);  /* restore kernel-context terminate */
            int21_set_psp(&g_kernel_psp);     /* restore kernel-context JFT */
            int21_cwd_restore(&cwd_snap);     /* restore kernel-context CWD */

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
        int21_cwd_snapshot_t cwd_snap = int21_cwd_save();   /* save kernel CWD (mzxa) */
        loader_status_t st = load_program_from_fat("SYSI.COM", 0u /* root */,
                                                   (const char *)0, 0u,
                                                   0u /* env: inherit-empty */,
                                                   &rc);
        int21_set_exit(int21_exit_hook);  /* restore kernel-context terminate */
        int21_set_psp(&g_kernel_psp);     /* restore kernel-context JFT */
        int21_cwd_restore(&cwd_snap);     /* restore kernel-context CWD */
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
