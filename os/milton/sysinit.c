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
#include "memory_map.h"   /* PROGRAM_BASE / PROGRAM_ALLOC_END (the arena region) */

/* The interrupt/IRQ stub entrypoints (isr.asm) -- the SAME externs kmain.c uses.
 * Naming them here keeps the gate-install order self-contained in the phase. */
extern void int21_entry(void);
extern void int20_entry(void);
extern void int22_entry(void);   /* DOS termination handler   (beads initech-509.8) */
extern void int23_entry(void);   /* DOS control-break handler (beads initech-509.8) */
extern void int24_entry(void);   /* DOS critical-error handler(beads initech-509.8) */
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

    /* Bind the MCB memory arena behind AH=48h/49h/4Ah (beads initech-509.6) over
     * the LOCKED program-memory region [PROGRAM_BASE, PROGRAM_ALLOC_END)
     * (spec/memory_map.h). The arena's flat base IS PROGRAM_BASE, so the DOS
     * segment a program sees == (PROGRAM_BASE >> 4) + data_para -- exactly the
     * paragraph addressing real DOS uses. total_paras = the whole 256 KiB window
     * / 16. The loader re-initializes this per program load (int21_mcb_reset) so
     * each program owns its whole window (the authentic single-big-block). NO
     * locked-spec change: the region is already PROGRAM_BASE..PROGRAM_ALLOC_END. */
    int21_set_mcb_arena((void *)(uintptr_t)PROGRAM_BASE,
                        (PROGRAM_ALLOC_END - PROGRAM_BASE) / 16u,
                        PROGRAM_BASE);

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

    /* DEVICE / INSTALL / BUFFERS / LASTDRIVE: parsed + recorded; honored later
     * (DEVICE drivers land in 509.7). Emit "accepted(deferred)" so the oracle
     * sees they were seen and NOT silently dropped (Rule 2). */
    for (uint8_t d = 0; d < cfg.device_count; d++) {
        serial("SYSINIT: DEVICE=");
        serial(cfg.devices[d]);
        serial(" accepted(deferred)\n");
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
