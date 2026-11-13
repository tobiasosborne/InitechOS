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
 * THE FILE IS NOT A PROGRAM (initech-13ig). COMMAND.COM on the boot volume is
 * this overlay, linked at TRANSIENT_BASE -- not a flat .COM. An AH=4Bh EXEC
 * that reads it (C:\COMMAND.COM, or any copy under another name) must never
 * run its bytes at PROGRAM_IMAGE: os/milton/loader.c recognises the magic and
 * refuses with LOADER_ERR_BAD_FORMAT (DOS EXEC error 11, "Invalid format";
 * IBM DOS 3.30 TR p. 6-200), and a resident program's EXEC of COMMAND.COM by
 * name is routed to exec_secondary once the transient is resident, exactly as
 * the shell kernels route it to their resident interpreter (initech-n8q8).
 */
#ifndef INITECH_TRANSIENT_H
#define INITECH_TRANSIENT_H

#include <stdint.h>

#define TRANSIENT_MAGIC          "COMMAND3"   /* 8 bytes, no NUL on disk     */
#define TRANSIENT_MAGIC_LEN      8u

typedef struct transient_hdr {
    char     magic[8];       /* TRANSIENT_MAGIC                                */
    uint32_t entry;          /* void command_repl(int permanent)               */
    uint32_t exec_secondary; /* void command_exec_secondary(const char *tail,
                              *                             uint32_t len):
                              * the COMMAND /C secondary processor a resident
                              * program's EXEC of COMMAND.COM runs once the
                              * transient is resident (initech-13ig; MS-DOS 3.3
                              * Reference pp.46-47; ADR-0003 DEC-03/DEC-11).  */
    uint32_t load_end;       /* first byte past .data (the bytes on disk end)  */
    uint32_t bss_end;        /* first byte past .bss (zeroed by the kernel)    */
    uint32_t n_imports;      /* == transient_import_count (below)              */
    /* EVERY kernel symbol the transient's code imports, in the GENERATED
     * order (the Makefile's TRANSIENT_IMPORTS_LD rule: gcc-nm -u of the
     * transient's C objects, intersected with the kernel's defined symbols,
     * sorted -- no hand-kept list). The same list generates both this table
     * (build/transient_imports_dd.inc, included by transient_hdr.asm) and the
     * kernel's `transient_import_want[]` / `transient_import_count`
     * (build/transient_want.asm, linked into every kernel that carries a
     * transient), so a new import can never be forgotten on either side; the
     * kernel refuses a count mismatch and any address mismatch, and the
     * transient .bin rule asserts the on-disk count against that list.
     * Found wanting (initech-13ig review): five of ten were checked. */
    uint32_t imports[];
} transient_hdr_t;

/* The kernel side of the table (GENERATED, build/transient_want.asm). */
extern const uint32_t transient_import_want[];
extern const uint32_t transient_import_count;

#endif /* INITECH_TRANSIENT_H */
