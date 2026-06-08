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
#include "test_prog.h"   /* the baked flat test program bytes */
#include "ata.h"         /* ATA PIO backend (FAT-mount over the emulator) */
#include "fat12.h"       /* FAT12 mount + root-dir enumerate (proto-DIR)  */

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
    while ((inb(COM1_LSR) & LSR_THRE) == 0) {
        /* spin until the holding register is empty */
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

    if (bi->lfb_bpp == 32) {
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
    if (!(b.lfb_bpp == 24 || b.lfb_bpp == 32)) {
        serial_puts("BI-BAD bpp=");   serial_putu(b.lfb_bpp);    serial_putc('\n');
        ok = 0;
    }
    if (b.lfb_addr == 0) {
        serial_puts("BI-BAD addr=0\n");
        ok = 0;
    }
    if (b.lfb_width != 640) {
        serial_puts("BI-BAD width="); serial_putu(b.lfb_width);  serial_putc('\n');
        ok = 0;
    }
    if (b.lfb_height != 480) {
        serial_puts("BI-BAD height=");serial_putu(b.lfb_height); serial_putc('\n');
        ok = 0;
    }
    if (ok) {
        serial_puts("BI-OK\n");
    }

    /* Interrupt foundation (beads initech-a5a). Order matters (ground-truth
     * Sec 8 Risk 1): remap+MASK the 8259 PIC away from CPU exception vectors
     * FIRST, then install the IDT (256 gates; exceptions 0-31 as 0x8E interrupt
     * gates -> fail-loud panic). We keep IF=0 all milestone (no sti); software
     * interrupts still dispatch through the IDT with IF masked. */
    pic_remap_and_mask();
    serial_puts("PIC\n");
    idt_init();
    serial_puts("IDT\n");

    /* INT 21h syscall spine (beads initech-509.5). Install the literal `int
     * 0x21` dispatcher as a 32-bit TRAP gate (0x8F, DPL=0, CODE_SEL) at the now-
     * free vector 0x21 (PIC is at 0x28/0x30). Bind the CON sink + terminate hook
     * BEFORE any int 0x21 fires. The console sink is bound later (once the
     * console is up); until then the sink fans to serial only. */
    int21_set_sink(int21_con_sink);
    int21_set_exit(int21_exit_hook);

    /* System File Table + kernel-context PSP (beads initech-509.3). sft_init
     * lays the predefined device entries (slots 0-3: CON-in, CON-out, AUX, PRN);
     * the kernel PSP carries the matching standard JFT (jft[0..4]) so kernel-
     * context INT 21h (AH=40h WRITE etc.) has valid standard handles BEFORE any
     * program loads. The loader rebinds to the program's PSP on load and the
     * kernel restores g_kernel_psp on return (below). Ref: DEC-06; sft.h. */
    sft_init();
    {
        psp_params_t kp;
        kp.alloc_end_linear  = 0u;  /* kernel context: no program ceiling */
        kp.env_linear        = 0u;
        kp.parent_psp_linear = 0u;
        kp.cmd_tail          = (const char *)0;
        kp.cmd_tail_len      = 0u;
        (void)psp_build(&g_kernel_psp, &kp);
    }
    int21_set_psp(&g_kernel_psp);

    idt_install_trap(0x21u, (void *)int21_entry);
    /* INT 20h legacy-terminate trap gate (beads initech-509.5). Vector 0x20 is
     * free (PIC master at 0x28, DEC-04a); a loaded program can terminate via
     * `int 0x20` as well as INT 21h AH=4Ch (ground-truth Sec 2.1 / Sec 4.4). */
    idt_install_trap(0x20u, (void *)int20_entry);
    serial_puts("INT21\n");

    /* LIVE self-test: install a trap gate on an UNUSED vector (0x80 -- not an
     * IRQ vector after the 0x28/0x30 remap), fire `int 0x80`, and confirm
     * control returns. Proves gate-install + dispatch + iret + resume on the
     * real boot (the host oracle proves the byte-level encode; this proves the
     * CPU actually honors it). The handler emits IDT-SELFTEST-OK; IDT-RESUMED
     * after the int proves the iret resumed us. */
    idt_install_trap(0x80u, (void *)isr_selftest);
    __asm__ __volatile__("int $0x80");
    serial_puts("IDT-RESUMED\n");

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

    /* RUN A PROGRAM (beads initech-509.5). The milestone: lay out a PSP + the
     * baked flat test program at PROGRAM_BASE/PROGRAM_IMAGE, JMP to it, let it
     * print via INT 21h AH=09h, and regain control when it issues AH=4Ch -- the
     * return-to-loader mechanism (ground-truth Sec 4). The program's own output
     * ("Hello from InitechOS program.") fans to the LFB console + serial through
     * the SAME int21 CON sink the banner used. We frame it with PROGRAM-BEGIN /
     * PROGRAM-EXIT markers so the boot oracle can assert load->run->return.
     *
     * After load_program returns, the loader unbound its exit hook; re-bind the
     * kernel's cli;hlt hook so any later stray 4Ch from kernel context halts
     * cleanly (ground-truth Sec 4.5 / Risk 3). */
    serial_puts("PROGRAM-BEGIN\n");
    {
        uint8_t rc = 0;
        loader_status_t st = load_program(g_test_prog_image,
                                          g_test_prog_image_len,
                                          (const char *)0, 0u, &rc);
        int21_set_exit(int21_exit_hook);  /* restore kernel-context terminate */
        int21_set_psp(&g_kernel_psp);     /* restore kernel-context JFT (509.3) */
        if (st == LOADER_OK) {
            serial_puts("PROGRAM-EXIT rc=");
            serial_putu((uint32_t)rc);
            serial_putc('\n');
        } else {
            /* Fail loud (Rule 2): the program was rejected before it ran. */
            serial_puts("PROGRAM-LOAD-FAIL st=");
            serial_putu((uint32_t)st);
            serial_putc('\n');
        }
    }

    /* MOUNT A REAL FILESYSTEM (beads initech-saw). Attach the FAT12 data disk
     * on the IDE primary SLAVE, read sector 0 via the ATA PIO backend (its
     * FIRST live run on the emulator -- beads initech-adf), mount it, and (on
     * success) render a proto-DIR of the root directory to console + serial.
     *
     * Fail loud (Rule 2): on any failure -- no drive (floating bus), bad boot
     * signature, ATA error -- emit "FAT-MOUNT-FAIL rc=NN" and CONTINUE to the
     * halt loop. The ATA floating-bus guard guarantees mount returns an error
     * (never hangs) when no data disk is attached, so a boot WITHOUT --disk2
     * still reaches this point and halts cleanly (test-boot/test-program stay
     * green with no second disk). Ref: fs-mount-sft-ground-truth.md Sec 5.1. */
    serial_puts("FAT-MOUNT-BEGIN\n");
    {
        static uint8_t sector_buf[BLOCKDEV_SECTOR_SIZE]; /* caller scratch (BSS) */
        ata_ctx_t      ata_slave;
        blockdev_t     fatdev;
        fat12_volume_t vol;
        int rc;

        /* Fan the proto-DIR to the live console too (the SAME console the
         * banner used, already bound via g_int21_con). NULL is fine -- dir_putc
         * then fans to serial only. */
        g_dir_con = g_int21_con;

        ata_ctx_init_primary_slave(&ata_slave);
        ata_blockdev_init(&fatdev, &ata_slave);

        rc = fat12_mount(&vol, &fatdev, sector_buf);
        if (rc == FAT12_OK) {
            serial_puts("FAT-MOUNT-OK\n");

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
        } else {
            /* Fail loud + continue (do NOT hang). A missing disk yields
             * ATA_ERR_NO_DRIVE propagated as FAT12_ERR_READ. */
            serial_puts("FAT-MOUNT-FAIL rc=");
            serial_puti((int32_t)rc);
            serial_putc('\n');
        }
    }
    serial_puts("FAT-MOUNT-END\n");

    /* Stay live (hlt-loop) so the QMP screendump captures the framebuffer. Do
     * NOT isa-debug-exit (that kills the guest before capture). The kstart.asm
     * hang guard also catches an unexpected return. */
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
