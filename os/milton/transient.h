/* transient.h -- the TRANSIENT portion of COMMAND.COM on the flagship image.
 *
 * bead:  initech-3p1a (Special > Exit to InitechDOS: leave FLAIR for the
 *        A:\> prompt and run the TPS Report Generator there).
 * Ref:   ADR-0003 DEC-03 ("the resident/transient division of the command
 *        interpreter ... the transient command interpreter is reloaded from
 *        disk ... retained ... as a fidelity feature"), DEC-11 (COMMAND.COM),
 *        Appendix C MSG-DOS-0014 ("Bad or missing Command Interpreter");
 *        spec/memory_map.h TRANSIENT_BASE/TRANSIENT_CEIL; CLAUDE.md Rule 2
 *        (fail loud), Rule 11 (reproducible), Rule 12 (ASCII).
 *
 * WHY A TRANSIENT. The flagship FLAIR kernel fills the stage2 load window
 * (KERNEL_SECTORS is at the ratified bounce ceiling, ~16 KiB free); the
 * COMMAND.COM interpreter (command.o + env.o + batch.o, ~33 KiB) cannot ride
 * in it. So the flagship image carries the interpreter as root COMMAND.COM
 * on the boot FAT, LINKED AGAINST THAT IMAGE'S OWN KERNEL (ld --just-symbols)
 * at TRANSIENT_BASE, and the kernel reads it in only when the user leaves the
 * desktop for the prompt -- the period's resident/transient split, literally.
 *
 * THE FILE (initech-e9sx): MS-DOS 3.3 User's Guide printed pp28/119
 * (PDF41/204): COMMAND.COM is visible to DIR, copied separately from SYS's
 * hidden IO.SYS/MSDOS.SYS. tools/mk_boot_system.sh copies it with archive-only
 * attributes into the boot root. DEC-07a mounts the primary-master partition;
 * FAT lookup/chain reading replaces the former raw system-area sector slot.
 *
 * THE HEADER (first bytes of the transient, os/milton/transient_hdr.asm). The
 * kernel trusts NOTHING it reads: the magic must match, the extents must lie
 * inside [TRANSIENT_BASE, TRANSIENT_CEIL), and every kernel address the
 * transient was linked against must equal the running kernel's own -- a
 * transient linked against a different kernel build is refused LOUDLY
 * (MSG-DOS-0014 on the console, PANIC on serial), never jumped into.
 *
 * THE CHECKSUM RULE (bead initech-tdnl.106; ADR-0003 Amendment DEC-19).
 * Since FLAIR became a command the transient is read at SYSINIT and stays
 * in memory while every program -- the desktop included -- runs. IBM DOS
 * 3.30 Technical Reference p1-5 (PDF22): "When a program terminates, a
 * checksum methodology determines if the program has caused the transient
 * portion to be overlaid. If the transient portion is overlaid, it is
 * reloaded." The span summed is the transient's CODE: the header, .text and
 * .rodata, [TRANSIENT_BASE, code_end) -- the bytes no run of the interpreter
 * changes. .data/.bss are its live state (the environment, the batch in
 * progress) and are NOT summed: summing them would call every SET a overlay.
 * The sum is the period's word checksum: the 16-bit sum of the span's
 * little-endian words (an odd last byte is a low byte). After a program
 * returns, a mismatch reloads the code span from COMMAND.COM on the boot
 * disk in place (kmain.c transient_verify); the live state is kept.
 * MUTANTS (Rule 6; host test-flair-cmd-host-mutant, never in a real build):
 *   TRANSIENT_MUT_NO_CHECK  the verdict is always "intact" (an overlay is
 *                           never noticed -- the prompt runs on garbage);
 *   TRANSIENT_MUT_SUM_DATA  the span runs to load_end (a SET is an overlay).
 */
#ifndef INITECH_TRANSIENT_H
#define INITECH_TRANSIENT_H

#include <stdint.h>

#define TRANSIENT_MAGIC          "COMMAND3"   /* 8 bytes, no NUL on disk     */
#define TRANSIENT_MAGIC_LEN      8u
#define TRANSIENT_N_IMPORTS      5u

typedef struct transient_hdr {
    char     magic[8];       /* TRANSIENT_MAGIC                                */
    uint32_t entry;          /* void command_repl(int permanent)               */
    uint32_t load_end;       /* first byte past .data (the bytes on disk end)  */
    uint32_t bss_end;        /* first byte past .bss (zeroed by the kernel)    */
    uint32_t code_end;       /* first byte past .rodata: the checksummed span  */
    uint32_t n_imports;      /* TRANSIENT_N_IMPORTS                            */
    /* The kernel addresses the transient was linked against, IN THIS ORDER:
     * fileio_fat_volume_label, int21_directory_exists, int21_get_psp,
     * int21_same_file, int21_set_break_recovery (nm -u of command.o). */
    uint32_t imports[TRANSIENT_N_IMPORTS];
} transient_hdr_t;

/* The bytes the checksum covers, measured from the transient's base: the
 * header + .text + .rodata. `h` is the kernel's OWN validated copy of the
 * header (the copy in the window may itself be what was overlaid). */
static inline uint32_t transient_sum_len(const transient_hdr_t *h,
                                         uint32_t base)
{
#ifndef TRANSIENT_MUT_SUM_DATA
    return h->code_end - base;
#else
    return h->load_end - base;
#endif
}

/* The word checksum: the 16-bit sum of the little-endian words of p[0..n). */
static inline uint16_t transient_checksum(const uint8_t *p, uint32_t n)
{
    uint32_t sum = 0u, i;

    for (i = 0u; i + 1u < n; i += 2u)
        sum += (uint32_t)p[i] | ((uint32_t)p[i + 1u] << 8);
    if (i < n)
        sum += (uint32_t)p[i];
    return (uint16_t)sum;
}

/* 1 when the code span no longer sums to what it summed at load. */
static inline int transient_overlaid(const uint8_t *p, uint32_t n,
                                     uint16_t sum_at_load)
{
#ifndef TRANSIENT_MUT_NO_CHECK
    return transient_checksum(p, n) != sum_at_load;
#else
    (void)p; (void)n; (void)sum_at_load;
    return 0;
#endif
}

#endif /* INITECH_TRANSIENT_H */
