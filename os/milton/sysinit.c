/* sysinit.c -- InitechDOS SYSINIT phases (beads initech-509.2).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding kernel code. NAMED, ordered
 * bring-up phases extracted from kmain.c kernel_main(). A RELOCATION, not a
 * re-ordering -- see sysinit.h for the ordering minefield contract. The CONFIG.SYS
 * apply phase (sysinit_apply_config) is the new behavior; the rest is relocation
 * of the exact original sequence.
 *
 * Ref (Law 1): kmain.c kernel_main (the original ordered init); ADR-0003 DEC-06
 *   (FILES= governs the SFT capacity) + Appendix D.2; spec/dos_config_sys_baseline.txt
 *   (the LOCKED baseline, embedded below); os/milton/config_sys.h; os/milton/sft.h.
 *   Rule 2 (fail loud), Rule 11 (deterministic), Rule 12 (ASCII).
 */

#include "sysinit.h"
#include "pic.h"
#include "idt.h"
#include "pit.h"
#include "kbd.h"
#include "sft.h"
#include "config_sys.h"
#include "fat12.h"
#include "int21.h"        /* int21_set_mcb_arena (the AH=48/49/4A seam; 509.6) */
#include "devices.h"      /* devices_init / devices_head (the char-device chain; 6zd9) */
#include "memory_map.h"   /* PROGRAM_BASE / PROGRAM_ALLOC_END (the arena region) */

/* ANSI.SYS install flag (beads initech-6zd9 records it; beads initech-p96i, the
 * ANSI escape interpreter, consumes it). A CONFIG.SYS `DEVICE=ANSI.SYS` line sets
 * this so p96i can gate its CSI handling on "the operator loaded ANSI.SYS"
 * (period-authentic: without ANSI.SYS, DOS prints the escape bytes literally).
 * This bead ONLY records the flag -- it does NOT implement any ANSI behavior. */
static int g_ansi_enabled = 0;

int sysinit_ansi_enabled(void) { return g_ansi_enabled; }

/* The serial sink for the PRN/AUX device callbacks (beads initech-6zd9). Captured
 * from the sysinit_early `serial` fn so a WRITE to a chain-routed PRN/AUX handle
 * reaches the COM1 boot log -- the only output sink sysinit can reach without
 * pulling console.c (off-limits) into the phase. A real LPT1/COM1 port driver is
 * a flagged follow-up; this makes PRN/AUX OBSERVABLE (Rule 2: not a silent drop)
 * rather than a fault. NULL-safe (no serial bound -> the write is discarded). */
static sysinit_serial_fn g_dev_serial = 0;

/* PRN/AUX write adapter (dev_write_fn): emit each byte to the captured serial
 * sink. Returns the byte count (a synchronous sink never short-writes). The
 * single-char buffer keeps the freestanding, malloc-free discipline (Law 3). */
static int sysinit_prn_aux_write(const uint8_t *buf, int len, void *ctx)
{
    (void)ctx;
    if (g_dev_serial == 0 || buf == 0) {
        return len;   /* no sink -> discard (still "consumed", never a fault) */
    }
    for (int i = 0; i < len; i++) {
        char s[2];
        s[0] = (char)buf[i];
        s[1] = '\0';
        g_dev_serial(s);
    }
    return len;
}

/* The interrupt/IRQ stub entrypoints (isr.asm) -- the SAME externs kmain.c uses.
 * Naming them here keeps the gate-install order self-contained in the phase. */
extern void int21_entry(void);
extern void int20_entry(void);
extern void int22_entry(void);   /* DOS termination handler   (beads initech-509.8) */
extern void int23_entry(void);   /* DOS control-break handler (beads initech-509.8) */
extern void int24_entry(void);   /* DOS critical-error handler(beads initech-509.8) */
extern void int25_entry(void);   /* INT 25h absolute disk READ  (DEC-15, initech-4mq7) */
extern void int26_entry(void);   /* INT 26h absolute disk WRITE (DEC-15, initech-4mq7) */
extern void irq0_entry(void);
extern void irq1_entry(void);

/* The LOCKED CONFIG.SYS baseline, embedded byte-for-byte from
 * spec/dos_config_sys_baseline.txt (Rule 8 -- this is the fallback when no
 * CONFIG.SYS is present on the volume). Kept in sync with the spec file; the host
 * oracle (test_config_sys.c) parses the spec FILE directly so a drift between this
 * literal and the spec is caught there as a parse-result mismatch is NOT -- so the
 * literal is the deliberate embedded copy of the contract. ASCII, '\n' line ends. */
static const char SYSINIT_CONFIG_BASELINE[] =
    "FILES=20\n"
    "BUFFERS=20\n"
    "LASTDRIVE=Z\n"
    "DEVICE=ANSI.SYS\n"
    "DEVICE=INITNET.SYS\n"
    "INSTALL=SHARE.EXE\n"
    "SHELL=COMMAND.COM /P /E:512\n";

/* Case-insensitive ASCII compare of a NUL-terminated parsed DEVICE= name against
 * a literal (e.g. "ANSI.SYS"). Freestanding (Law 3), bounded by the NUL on both
 * sides. Returns 1 iff equal ignoring case. */
static int sysinit_name_eq_ci(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 32);
        if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 32);
        if (ca != cb) return 0;
        a++; b++;
    }
    return (*a == '\0' && *b == '\0') ? 1 : 0;
}

/* ---- freestanding numeric helper for the summary line ------------------- */
static void sysinit_put_uint(sysinit_serial_fn serial, uint32_t v)
{
    char buf[10];
    int n = 0;
    char out[12];
    int k = 0;

    if (v == 0) {
        const char z[2] = { '0', '\0' };
        serial(z);
        return;
    }
    while (v > 0 && n < (int)sizeof(buf)) {
        buf[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n > 0 && k < (int)sizeof(out) - 1) {
        out[k++] = buf[--n];
    }
    out[k] = '\0';
    serial(out);
}

/* ======================================================================== *
 * PHASE 1 -- interrupt + syscall foundation (kmain.c original 431-467).
 * ======================================================================== */
void sysinit_early(int21_sink_fn sink, int21_exit_fn exit_hook,
                   psp_t *kernel_psp, sysinit_serial_fn serial)
{
    /* Order matters (the minefield, sysinit.h): remap+MASK the 8259 FIRST, then
     * install the IDT, THEN bind the int21 glue, THEN the kernel PSP, THEN the
     * trap gates. IDENTICAL to the original kernel_main sequence. */
    pic_remap_and_mask();
    serial("PIC\n");
    idt_init();
    serial("IDT\n");

    /* Bind the CON sink + terminate hook BEFORE any `int 0x21` fires. */
    int21_set_sink(sink);
    int21_set_exit(exit_hook);

    /* System File Table + kernel-context PSP: sft_init lays the device entries
     * (slots 0-3); the kernel PSP carries the standard JFT so kernel-context INT
     * 21h has valid handles before any program loads. */
    sft_init();
    {
        psp_params_t kp;
        kp.alloc_end_linear  = 0u;
        kp.env_linear        = 0u;
        kp.parent_psp_linear = 0u;
        kp.cmd_tail          = (const char *)0;
        kp.cmd_tail_len      = 0u;
        (void)psp_build(kernel_psp, &kp);
    }
    int21_set_psp(kernel_psp);

    /* Character-device chain (beads initech-6zd9; ADR-0003 DEC-09). Build the
     * resident chain (devices_init lays NUL/CON/AUX/PRN/CLOCK$) and wire it into
     * the INT 21h OPEN-by-name + READ/WRITE routing (int21_set_devices). The chain
     * MUST exist before the shell so a program can OPEN "CON"/"NUL"/"PRN"/"AUX"/
     * "CLOCK$" by name. int21.c self-wires the CON + CLOCK$ legs to its OWN sink /
     * clock seams (so device CON output is the SAME byte stream as handle-1 output
     * -- the CON path is preserved exactly); the bundle here supplies only the
     * PRN + AUX write sinks (-> the COM1 boot log, the only sink reachable here).
     * The clock seam itself is bound later in kmain (int21_set_clock); CLOCK$ READ
     * works once it is. */
    g_dev_serial = serial;
    devices_init();
    {
        devices_io_t dio;
        for (uint32_t i = 0u; i < (uint32_t)sizeof(dio); i++) {
            ((uint8_t *)&dio)[i] = 0u;
        }
        dio.prn_write = sysinit_prn_aux_write;
        dio.aux_write = sysinit_prn_aux_write;
        int21_set_devices(devices_head(), &dio);
    }

    /* Bind a DISJOINT kernel-context default MCB heap arena (beads initech-1q4u;
     * ADR-0009 DEC-04). The arena is REBOUND per program load by the loader
     * (int21_mcb_bind_program) to a window computed disjoint from THAT program's
     * image; until a program loads, kernel/shell-context AH=48h needs an arena
     * that does NOT overlay the program window. We use the SAME disjoint formula
     * the loader uses for the smallest (zero-length) image: base = roundup_para(
     * PROGRAM_IMAGE + PROGRAM_BSS_RESERVE), ceiling = PROGRAM_ARENA_CEIL (==
     * ENV_BLOCK). This is BELOW the env+stack and ABOVE the (kernel-context: empty)
     * image region, so a kernel-context 48h ALLOC gets real, non-overlapping RAM.
     *
     * THIS REPLACES the latent-corruption bind the bug introduced: the OLD code
     * bound [PROGRAM_BASE, PROGRAM_ALLOC_END) with base == PROGRAM_BASE -- the
     * EXACT window a loaded program's PSP (0x38000) / image (0x38100+) / env
     * (0x67000) / stack (top 0x77FFC) occupy, so a 48h ALLOC overlaid the running
     * program (ADR-0009 Sec 1). The base is now a COMPUTED disjoint value, never
     * PROGRAM_BASE. See spec/memory_map.h's ARENA DISJOINTNESS INVARIANT. */
    {
        uint32_t arena_base = (PROGRAM_IMAGE + PROGRAM_BSS_RESERVE + 0xFu) & ~0xFu;
        uint32_t arena_ceil = PROGRAM_ARENA_CEIL;   /* == ENV_BLOCK (exclusive) */
        /* Lay ONE terminal FREE block (mcb_init via int21_set_mcb_arena) over the
         * disjoint window -- NOT int21_mcb_reset (which would stamp the bound
         * kernel PSP as owner and make a direct kernel-context 48h fail). The old
         * SYSINIT bind likewise left the window FREE; we keep that exact behavior,
         * only over the disjoint window. A loaded program REBINDS + reowns its own
         * disjoint window via int21_mcb_bind_program. */
        uint32_t total_paras = (arena_ceil - arena_base) / 16u;
        int21_set_mcb_arena((void *)(uintptr_t)arena_base, total_paras, arena_base);
    }

    idt_install_trap(0x21u, (void *)int21_entry);
    idt_install_trap(0x20u, (void *)int20_entry);
    /* DOS termination / control-break / critical-error handlers (DEC-10, beads
     * initech-509.8). Vectors 0x22-0x24 are free (PIC remapped to 0x28/0x30,
     * DEC-04a), so they are real 0x8F TRAP gates exactly like INT 21h. The gates
     * stay installed so the loader's PSP vector save (PSP 0Ah-15h; a SEPARATE
     * step) can read their current offsets. */
    idt_install_trap(0x22u, (void *)int22_entry);
    idt_install_trap(0x23u, (void *)int23_entry);
    idt_install_trap(0x24u, (void *)int24_entry);
    /* INT 25h/26h ABSOLUTE DISK READ/WRITE (ADR-0003 DEC-15, beads initech-4mq7).
     * Vectors 0x25/0x26 are free in the DEC-04a 0x22-0x27 band (PIC remapped to
     * 0x28/0x30), so they are real 0x8F TRAP gates exactly like INT 21h/24h. The
     * disk SEAM is bound separately in kmain.c after a successful FAT12 mount; an
     * unbound seam makes the handlers fail loud (CF=1), never fault. */
    idt_install_trap(0x25u, (void *)int25_entry);
    idt_install_trap(0x26u, (void *)int26_entry);
    serial("INT21\n");
}

/* ======================================================================== *
 * PHASE 2 -- CONFIG.SYS apply (the NEW behavior + the hook point).
 * ======================================================================== */
uint8_t sysinit_apply_config(const fat12_volume_t *vol, void *sector_buf,
                             void *fat_buf, uint32_t fat_buf_len,
                             sysinit_serial_fn serial)
{
    dos_config_t cfg;
    const char *src = SYSINIT_CONFIG_BASELINE;
    uint32_t    src_len = (uint32_t)(sizeof(SYSINIT_CONFIG_BASELINE) - 1u);
    int         from_disk = 0;
    int         truncated = 0;

    /* Read CONFIG.SYS from the mounted volume if present; otherwise embed the
     * LOCKED baseline. `fat_buf` caches the FAT for the cluster walk; the file
     * bytes land in a small static scratch sized for a realistic CONFIG.SYS (no
     * malloc, Law 3). A file LARGER than the scratch is not discarded: we honor
     * its first sizeof(cfg_buf) bytes (the directives we apply -- FILES=, SHELL=
     * -- sit in the early lines) and flag the truncation in the SYSINIT summary
     * (bcg.9; operator choice 2026-06-13). */
    static char cfg_buf[1024];
    if (vol != 0 && sector_buf != 0 && fat_buf != 0) {
        dir_entry_t de;
        int rc = fat12_find(vol, sector_buf, "CONFIG.SYS", &de);
        if (rc == FAT12_OK) {
            /* Cache the FAT, then read the whole (small) file. */
            int frc = fat12_read_fat(vol, fat_buf, fat_buf_len);
            if (frc == FAT12_OK) {
                uint32_t got = 0;
                /* sector_buf doubles as the per-cluster scratch (>= sectors_per_
                 * cluster*512; 512 for a 1.44 MB FAT12). The earlier fat12_find
                 * use is complete, so reuse is safe (sequential, single-threaded). */
                int rrc = fat12_read_file(vol, fat_buf, fat_buf_len, &de,
                                          cfg_buf, (uint32_t)sizeof(cfg_buf),
                                          sector_buf, &got);
                if (rrc == FAT12_OK && got > 0u) {
                    src = cfg_buf;
                    src_len = got;
                    from_disk = 1;
                } else if (rrc == FAT12_ERR_BUFFER) {
                    /* CONFIG.SYS exceeds cfg_buf: honor the first sizeof(cfg_buf)
                     * bytes via a positioned read instead of silently discarding
                     * the whole file (bcg.9). fat12_read_file fails loud on a
                     * short buffer; fat12_read_partial clamps to the prefix. Trim
                     * to the last complete line so a cut final line is not mis-
                     * parsed. Truncation is surfaced in the summary (loud). */
                    uint32_t got2 = 0;
                    int prc = fat12_read_partial(vol, fat_buf, fat_buf_len, &de,
                                                 0u, (uint32_t)sizeof(cfg_buf),
                                                 cfg_buf, sector_buf, &got2);
                    if (prc == FAT12_OK && got2 > 0u) {
                        while (got2 > 0u && cfg_buf[got2 - 1u] != '\n') {
                            got2--;          /* drop the partial final line */
                        }
                        if (got2 > 0u) {
                            src = cfg_buf;
                            src_len = got2;
                            from_disk = 1;
                            truncated = 1;
                        }
                    }
                }
            }
        }
    }

    (void)config_sys_parse(src, src_len, &cfg);

    /* APPLY the CTRL-BREAK directive (beads initech-er3h; ADR-0003 Amendment
     * DEC-16 Sec 3.3 / C-4): BREAK=ON|OFF flows into the kernel g_break_flag via
     * the public seam. Absent BREAK= keeps the boot default ON (the baseline has
     * no BREAK= line, so the default stands). The flag is the SAME source of
     * truth AH=33h GET/SET and the BREAK built-in read/write. */
    int21_set_break_flag(cfg.break_present ? cfg.break_on : 1u);

    /* APPLY the one directive with teeth: FILES=N -> the runtime SFT cap. If
     * FILES= was absent (or the file was unparseable), fall back to the baseline
     * FILES=20 so the system always has a coherent cap. */
    uint16_t files = cfg.files_present ? cfg.files : 20u;
    uint8_t  applied_limit = sft_set_files_limit(
        (files > 255u) ? 255u : (uint8_t)files);

    /* ---- the SYSINIT summary line the in-emulator oracle asserts ---- */
    serial("SYSINIT: source=");
    if (from_disk) {
        serial(truncated ? "CONFIG.SYS(truncated@1024)" : "CONFIG.SYS");
    } else {
        serial("baseline");
    }
    serial(" FILES=");
    sysinit_put_uint(serial, (uint32_t)files);
    serial(" cap=");
    sysinit_put_uint(serial, (uint32_t)applied_limit);
    serial(" SHELL=");
    serial(cfg.shell_present ? cfg.shell : "(none)");
    serial("\n");

    /* DEVICE / INSTALL / BUFFERS / LASTDRIVE: parsed + recorded. The resident
     * character-device chain (NUL/CON/AUX/PRN/CLOCK$) is now installed at
     * sysinit_early (beads initech-6zd9) -- it is intrinsic, not a CONFIG.SYS
     * DEVICE= load. A CONFIG.SYS DEVICE= line names an INSTALLABLE driver:
     *   - DEVICE=ANSI.SYS sets g_ansi_enabled so beads initech-p96i (the ANSI
     *     escape interpreter) can gate its CSI handling on it -- this bead ONLY
     *     records the flag, it implements no ANSI behavior; logged "ansi-enabled".
     *   - DEVICE=INITNET.SYS stays accepted(deferred): the INT 2Fh redirector is
     *     amendment-gated and OUT of scope here.
     * Anything else stays accepted(deferred). Emitting a line per DEVICE= keeps
     * the operator log honest (Rule 2: directives were seen, not silently dropped). */
    for (uint8_t d = 0; d < cfg.device_count; d++) {
        serial("SYSINIT: DEVICE=");
        serial(cfg.devices[d]);
        if (sysinit_name_eq_ci(cfg.devices[d], "ANSI.SYS")) {
            g_ansi_enabled = 1;          /* recorded for beads initech-p96i (ANSI) */
            serial(" ansi-enabled\n");
        } else {
            serial(" accepted(deferred)\n");
        }
    }
    for (uint8_t n = 0; n < cfg.install_count; n++) {
        serial("SYSINIT: INSTALL=");
        serial(cfg.install[n]);
        serial(" accepted(deferred)\n");
    }
    if (cfg.buffers_present) {
        serial("SYSINIT: BUFFERS=");
        sysinit_put_uint(serial, (uint32_t)cfg.buffers);
        serial(" accepted(deferred)\n");
    }
    if (cfg.lastdrive_present) {
        char ld[2];
        ld[0] = cfg.lastdrive;
        ld[1] = '\0';
        serial("SYSINIT: LASTDRIVE=");
        serial(ld);
        serial(" accepted(deferred)\n");
    }

    /* BREAK= state APPLIED (beads initech-er3h; DEC-16). Unlike DEVICE/INSTALL/
     * BUFFERS/LASTDRIVE this one has teeth -- it set g_break_flag above. Report
     * the effective state (the parsed value, or the boot default ON if absent)
     * so the boot log shows it was honored (Rule 2 -- not silently dropped). */
    serial("SYSINIT: BREAK=");
    serial((cfg.break_present ? cfg.break_on : 1u) ? "ON" : "OFF");
    serial(cfg.break_present ? " applied\n" : " (default)\n");

    return applied_limit;
}

/* ======================================================================== *
 * PHASE 3 -- enable hardware interrupts (kmain.c original 844-850, the tail).
 * ======================================================================== */
void sysinit_enable_irqs(sysinit_serial_fn serial)
{
    /* Order matters (the minefield): program the devices + install the gates
     * BEFORE unmasking, and unmask BEFORE sti. IDENTICAL to the original. */
    pit_init();
    kbd_init();
    idt_install_irq(0x28u, (void *)irq0_entry);   /* PIT  IRQ0 -> vector 0x28 */
    idt_install_irq(0x29u, (void *)irq1_entry);   /* kbd  IRQ1 -> vector 0x29 */
    pic_unmask_irq0_irq1();
    __asm__ __volatile__("sti");
    serial("IRQ-LIVE\n");
}
