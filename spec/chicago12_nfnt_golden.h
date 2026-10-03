/*
 * spec/chicago12_nfnt_golden.h -- the INDEPENDENT Chicago 12 metrics golden
 * for the factory's pixel oracles (bead initech-tdnl.33).
 *
 * The REAL System 7.0.1 Chicago 12 owTable (NFNT 5478) for 0x20..0x7E, typed
 * into the factory so oracles that cannot read the gitignored Apple resource at
 * run time (the emulator screendump graders, the chrome fidelity oracle) still
 * place glyphs with the REAL advances and bearings -- never with the artifact's
 * spec/assets/chicago12.h tables (Law 2 / ADR-0010 HER-02).
 *
 * PROVENANCE: transcribed mechanically from
 * ../system7-decomp/goldens/resources/NFNT_5478.bin bytes 2612 + 2*c (high
 * byte lb, low byte aw; specs/fonts/font-manager.md Sec 1.1), identical to
 * specs/fonts/chicago.md "Advance-width table". test-chicago-metrics leg G
 * re-reads the binary and proves every entry here equal to it, so this copy
 * cannot drift silently.
 *
 * FACTORY data (harness/tools only; the artifact never includes it). ASCII.
 */
#ifndef INITECH_SPEC_CHICAGO12_NFNT_GOLDEN_H
#define INITECH_SPEC_CHICAGO12_NFNT_GOLDEN_H

#define NFNT5478_FIRST 0x20
#define NFNT5478_LAST  0x7E
static const unsigned char __attribute__((unused)) NFNT5478_AW[NFNT5478_LAST - NFNT5478_FIRST + 1] = {
    4, 6, 7, 10, 7, 11, 10, 3, 5, 5, 7, 7, 4, 7, 4, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4, 6, 8, 6, 8,
    11, 8, 8, 8, 8, 7, 7, 8, 8, 6, 7, 9, 7, 12, 9, 8,
    8, 8, 8, 7, 6, 8, 8, 12, 8, 8, 8, 5, 7, 5, 8, 8,
    6, 8, 8, 7, 8, 8, 6, 8, 8, 4, 6, 8, 4, 12, 8, 8,
    8, 8, 6, 7, 6, 8, 8, 12, 8, 8, 8, 5, 5, 5, 8
};
static const unsigned char __attribute__((unused)) NFNT5478_LB[NFNT5478_LAST - NFNT5478_FIRST + 1] = {
    4, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1
};

/* Golden advance / bearing of printable byte c (callers pass 0x20..0x7E). */
static inline int nfnt5478_aw(int c) { return NFNT5478_AW[c - NFNT5478_FIRST]; }
static inline int nfnt5478_lb(int c) { return NFNT5478_LB[c - NFNT5478_FIRST]; }

#endif /* INITECH_SPEC_CHICAGO12_NFNT_GOLDEN_H */
