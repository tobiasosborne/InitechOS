/*
 * test_clut.c -- round-trip oracle for spec/assets/clut.h
 *
 * beads: initech-dh5k.5 (P1-5: spec/assets/clut.json + clut.h + oracle).
 * era=system7.0-7.1
 *
 * PURPOSE: exercises every structural invariant of the LOCKED System-7 ROM
 *   CLUT (spec/assets/clut.h flair_clut8[256][3]) and the rgb_to_index
 *   contract, including an entry-by-entry diff against the ROM binary golden
 *   when it is present.
 *
 * SOURCE CITATIONS (Law 1):
 *   ../system7-decomp/specs/resources/clut-pltt-format.md Sec 2 (byte layout),
 *     Sec 4.2 (sentinels: idx0=white, idx255=black, idx161=#339900),
 *     Sec 4.3 (cube structure + ramp values).
 *   ../system7-decomp/goldens/resources/clut_8_rom.bin (ground-truth binary,
 *     gitignored; 2056 bytes; extracted from Quadra 650 ROM F1ACAD13 offset
 *     0x072F90, ctSeed=8, ctFlags=0x8000, ctSize=255).
 *   refs/ImagingWithQuickDraw.pdf (cached) -- entry-161 "medium green" example.
 *   spec/assets/clut.h (the LOCKED spec under test).
 *
 * DUAL-COMPILE:
 *   # Freestanding compile check (confirms header compiles -ffreestanding):
 *   gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -std=c11 \
 *       -I/home/tobias/Projects/initech-os/spec/assets \
 *       -c /home/tobias/Projects/initech-os/harness/proptest/test_clut.c \
 *       -o /tmp/test_clut_fs.o
 *   # Hosted binary (runs + prints result):
 *   gcc -std=c11 -Wall \
 *       -I/home/tobias/Projects/initech-os/spec/assets \
 *       /home/tobias/Projects/initech-os/harness/proptest/test_clut.c \
 *       -o /tmp/test_clut && /tmp/test_clut
 *   # Hosted with explicit golden path:
 *   gcc -std=c11 -Wall \
 *       -I/home/tobias/Projects/initech-os/spec/assets \
 *       -DCLUT_ROM_GOLDEN_PATH=\"/path/to/clut_8_rom.bin\" \
 *       /home/tobias/Projects/initech-os/harness/proptest/test_clut.c \
 *       -o /tmp/test_clut && /tmp/test_clut
 *   # Mutation probe (must exit 1; proves oracle bites):
 *   gcc -std=c11 -Wall -DTEST_CLUT_MUTANT \
 *       -I/home/tobias/Projects/initech-os/spec/assets \
 *       /home/tobias/Projects/initech-os/harness/proptest/test_clut.c \
 *       -o /tmp/test_clut_mutant && /tmp/test_clut_mutant; \
 *       test $? -ne 0 && echo "MUTATION RED (correct)" || echo "MUTATION GREEN (BUG)"
 *
 * MUTATION PROOF (Rule 6): build with -DTEST_CLUT_MUTANT to flip ONE bit in
 *   the idx161 check. This MUST drive the oracle RED (exit 1). The
 *   _Static_assert in clut.h is a separate compile-time layer.
 *
 * GOLDEN DIFF: if ../system7-decomp/goldens/resources/clut_8_rom.bin is
 *   present, every entry is diffed against the ROM bytes. If absent, the test
 *   LOUD-SKIPS (prints a clear message) and continues with the structural
 *   checks -- NEVER silent-passes. Override path via:
 *     -DCLUT_ROM_GOLDEN_PATH="<absolute_path>"
 *   or the SYSTEM7_DECOMP macro:
 *     -DSYSTEM7_DECOMP="<path_to_system7-decomp>"
 *   (Makefile target test-clut passes SYSTEM7_DECOMP ?= ../system7-decomp.)
 *
 * OUTPUT: "test-clut: N checks, 0 failures, green"
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 */

/* Pull in the LOCKED CLUT spec. All _Static_asserts fire at compile time. */
#include "clut.h"

/* Hosted runtime: stdio + string + file I/O for the golden diff.
 * Under freestanding (-ffreestanding), __STDC_HOSTED__==0 and there is NO
 * runtime entry point -- the -c compile check above confirms the header
 * compiles; runtime checks only run in hosted mode. */
#if defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 1)
#include <stdio.h>
#include <string.h>

/* ----------------------------------------------------------------------- */
/* CHECK macro: count + optional fail print (hosted only).                 */
/* ----------------------------------------------------------------------- */
static int g_checks = 0;
static int g_fails  = 0;

#define CHECK(name, cond)                          \
    do {                                           \
        g_checks++;                                \
        if (!(cond)) {                             \
            g_fails++;                             \
            printf("  FAIL %s\n", (name));         \
        }                                          \
    } while (0)

/* ----------------------------------------------------------------------- */
/* Golden path resolution                                                   */
/* Priority: CLUT_ROM_GOLDEN_PATH > SYSTEM7_DECOMP/goldens/... > default   */
/* ----------------------------------------------------------------------- */
#ifdef CLUT_ROM_GOLDEN_PATH
#  define GOLDEN_BIN_PATH CLUT_ROM_GOLDEN_PATH
#elif defined(SYSTEM7_DECOMP)
#  define GOLDEN_BIN_PATH \
       SYSTEM7_DECOMP "/goldens/resources/clut_8_rom.bin"
#else
#  define GOLDEN_BIN_PATH \
       "../system7-decomp/goldens/resources/clut_8_rom.bin"
#endif

/* Big-endian read helpers for parsing the 2056-byte ROM binary. */
static unsigned short be16_buf(const unsigned char *p)
{
    return (unsigned short)(((unsigned short)p[0] << 8) | p[1]);
}
static unsigned int be32_buf(const unsigned char *p)
{
    return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) |
           ((unsigned int)p[2] <<  8) |  (unsigned int)p[3];
}

/* ----------------------------------------------------------------------- */
/* main                                                                     */
/* ----------------------------------------------------------------------- */
int main(void)
{
    unsigned int i;

    printf("test-clut: starting (era=system7.0-7.1)\n");

    /* =================================================================== */
    /* BLOCK 1: count via sizeof -- _Static_assert in clut.h is the        */
    /* compile-time layer; this is the runtime double-check.                */
    /* =================================================================== */
    {
        unsigned int sz = (unsigned int)sizeof(flair_clut8);
        g_checks++;
        if (sz != 768) {
            g_fails++;
            printf("  FAIL sizeof(flair_clut8) expected 768 got %u\n", sz);
        }
    }

    /* =================================================================== */
    /* BLOCK 2: sentinel entries                                            */
    /* Ref: clut-pltt-format.md Sec 4.2 [verified: ROM bytes]              */
    /* =================================================================== */

    /* idx 0 = white (#FFFFFF) */
    CHECK("idx0 WHITE #FFFFFF R",   flair_clut8[0][0] == 0xFF);
    CHECK("idx0 WHITE #FFFFFF G",   flair_clut8[0][1] == 0xFF);
    CHECK("idx0 WHITE #FFFFFF B",   flair_clut8[0][2] == 0xFF);

    /* idx 255 = black (#000000) */
    CHECK("idx255 BLACK #000000 R", flair_clut8[255][0] == 0x00);
    CHECK("idx255 BLACK #000000 G", flair_clut8[255][1] == 0x00);
    CHECK("idx255 BLACK #000000 B", flair_clut8[255][2] == 0x00);

    /* idx 161 = #339900 (IM "medium green"; ImagingWithQuickDraw.pdf)
     * Mutation probe perturbs the expected R value (0x33 -> 0x34). */
#ifdef TEST_CLUT_MUTANT
    {
        /* Perturb a local copy; do NOT touch the table constant.
         * In mutant mode the check below must fail (RED). */
        unsigned char probe_r = 0x34; /* wrong R; correct is 0x33 */
        CHECK("idx161 #339900 R [MUTANT: must FAIL]",
              flair_clut8[161][0] == probe_r);
    }
#else
    CHECK("idx161 #339900 R (IM medium green)", flair_clut8[161][0] == 0x33);
#endif
    CHECK("idx161 #339900 G",  flair_clut8[161][1] == 0x99);
    CHECK("idx161 #339900 B",  flair_clut8[161][2] == 0x00);

    /* =================================================================== */
    /* BLOCK 3: cube structure (indices 0-214)                              */
    /* All components from {0,51,102,153,204,255}, step=51=0x33.           */
    /* Ref: clut-pltt-format.md Sec 4.3 [verified: ROM bytes]              */
    /* =================================================================== */
    {
        static const unsigned char cv[6] = {0, 51, 102, 153, 204, 255};
        int cube_ok = 1;
        for (i = 0; i < 215; i++) {
            unsigned char r = flair_clut8[i][0];
            unsigned char g = flair_clut8[i][1];
            unsigned char b = flair_clut8[i][2];
            int ro = 0, go = 0, bo = 0, j;
            for (j = 0; j < 6; j++) {
                if (r == cv[j]) ro = 1;
                if (g == cv[j]) go = 1;
                if (b == cv[j]) bo = 1;
            }
            if (!ro || !go || !bo) {
                if (cube_ok) {
                    /* Report first violation only; don't flood output */
                    printf("  FAIL cube[%u] #%02X%02X%02X has non-cube component\n",
                           i, r, g, b);
                }
                cube_ok = 0;
            }
            g_checks++;
            if (!ro || !go || !bo) g_fails++;
        }
        if (cube_ok)
            printf("  cube structure [0..214]: all 215 entries OK\n");
    }

    /* =================================================================== */
    /* BLOCK 4: ramp structure (indices 215-254)                            */
    /* Ramp values: {238,221,187,170,136,119,85,68,34,17}.                 */
    /* Ref: clut-pltt-format.md Sec 4.3 [verified: ROM bytes]              */
    /* =================================================================== */
    {
        static const unsigned char rv[10] = {
            238, 221, 187, 170, 136, 119, 85, 68, 34, 17
        };
        /* R ramp [215..224]: R in rv, G=0, B=0 */
        for (i = 215; i <= 224; i++) {
            unsigned char r = flair_clut8[i][0];
            unsigned char g = flair_clut8[i][1];
            unsigned char b = flair_clut8[i][2];
            int ok = 0, j;
            for (j = 0; j < 10; j++) if (r == rv[j]) { ok = 1; break; }
            g_checks++;
            if (!ok || g != 0 || b != 0) {
                g_fails++;
                printf("  FAIL R-ramp idx%u #%02X%02X%02X\n", i, r, g, b);
            }
        }
        /* G ramp [225..234]: G in rv, R=0, B=0 */
        for (i = 225; i <= 234; i++) {
            unsigned char r = flair_clut8[i][0];
            unsigned char g = flair_clut8[i][1];
            unsigned char b = flair_clut8[i][2];
            int ok = 0, j;
            for (j = 0; j < 10; j++) if (g == rv[j]) { ok = 1; break; }
            g_checks++;
            if (!ok || r != 0 || b != 0) {
                g_fails++;
                printf("  FAIL G-ramp idx%u #%02X%02X%02X\n", i, r, g, b);
            }
        }
        /* B ramp [235..244]: B in rv, R=0, G=0 */
        for (i = 235; i <= 244; i++) {
            unsigned char r = flair_clut8[i][0];
            unsigned char g = flair_clut8[i][1];
            unsigned char b = flair_clut8[i][2];
            int ok = 0, j;
            for (j = 0; j < 10; j++) if (b == rv[j]) { ok = 1; break; }
            g_checks++;
            if (!ok || r != 0 || g != 0) {
                g_fails++;
                printf("  FAIL B-ramp idx%u #%02X%02X%02X\n", i, r, g, b);
            }
        }
        /* Gray ramp [245..254]: R=G=B in rv */
        for (i = 245; i <= 254; i++) {
            unsigned char r = flair_clut8[i][0];
            unsigned char g = flair_clut8[i][1];
            unsigned char b = flair_clut8[i][2];
            int ok = 0, j;
            for (j = 0; j < 10; j++) if (r == rv[j]) { ok = 1; break; }
            g_checks++;
            if (!ok || r != g || r != b) {
                g_fails++;
                printf("  FAIL Gray-ramp idx%u #%02X%02X%02X\n", i, r, g, b);
            }
        }
        printf("  ramp structure [215..254]: 40 entries checked\n");
    }

    /* =================================================================== */
    /* BLOCK 5: flair_clut8_rgb_to_index -- determinism + known mappings   */
    /* Ref: spec/assets/clut.h contract comment                             */
    /* =================================================================== */

    /* Known exact mappings (cube vertices: dist=0; unique) */
    {
        struct { unsigned char r, g, b, want; } km[] = {
            /* Sentinels (verified from ROM bytes) */
            {0xFF, 0xFF, 0xFF,   0},   /* white */
            {0x00, 0x00, 0x00, 255},   /* black */
            /* IM medium green (ImagingWithQuickDraw.pdf + ROM bytes) */
            {0x33, 0x99, 0x00, 161},
            /* Cube gray #666666 (ROM bytes idx 129) */
            {0x66, 0x66, 0x66, 129},
            /* idx 1 = #FFFFCC (ROM bytes) */
            {0xFF, 0xFF, 0xCC,   1},
            /* idx 214 = #000033 (ROM bytes; last cube before black) */
            {0x00, 0x00, 0x33, 214},
            /* Ramp entries (ROM bytes) */
            {0xEE, 0x00, 0x00, 215},   /* R-ramp first */
            {0x00, 0xEE, 0x00, 225},   /* G-ramp first */
            {0x00, 0x00, 0xEE, 235},   /* B-ramp first */
            {0xEE, 0xEE, 0xEE, 245},   /* Gray-ramp first */
        };
        unsigned int n = (unsigned int)(sizeof(km) / sizeof(km[0]));
        for (i = 0; i < n; i++) {
            unsigned char got = flair_clut8_rgb_to_index(km[i].r, km[i].g, km[i].b);
            g_checks++;
            if (got != km[i].want) {
                g_fails++;
                printf("  FAIL rgb_to_index(0x%02X,0x%02X,0x%02X) want %u got %u\n",
                       km[i].r, km[i].g, km[i].b, km[i].want, got);
            }
        }
    }

    /* Determinism: same call twice, same result */
    {
        unsigned char a = flair_clut8_rgb_to_index(0x33, 0x99, 0x00);
        unsigned char b2 = flair_clut8_rgb_to_index(0x33, 0x99, 0x00);
        CHECK("rgb_to_index determinism (same call twice)",  a == b2);
    }

    /* Round-trip: for every CLUT index i,
     * flair_clut8_rgb_to_index(flair_clut8[i][0..2]) == i.
     * Each entry is a unique RGB (no duplicates in the system CLUT).
     * Ref: clut-pltt-format.md Sec 4.3 (cube vertices are unique;
     *      ramp entries are unique within and across their sections). */
    {
        int rt_ok = 1;
        for (i = 0; i < 256; i++) {
            unsigned char r  = flair_clut8[i][0];
            unsigned char g  = flair_clut8[i][1];
            unsigned char b2 = flair_clut8[i][2];
            unsigned char got = flair_clut8_rgb_to_index(r, g, b2);
            g_checks++;
            if (got != (unsigned char)i) {
                g_fails++;
                if (rt_ok)
                    printf("  FAIL round-trip idx%u: got %u\n", i, got);
                rt_ok = 0;
            }
        }
        if (rt_ok)
            printf("  round-trip [all 256 entries]: OK\n");
    }

    /* =================================================================== */
    /* BLOCK 6: golden diff vs clut_8_rom.bin                              */
    /* LOUD-SKIP if absent; NEVER silent-pass.                              */
    /* Ref: clut-pltt-format.md Sec 2 (byte layout: header 8B +            */
    /*      256 * ColorSpec{value:u16, r:u16, g:u16, b:u16}).              */
    /* =================================================================== */
    {
        FILE *f = fopen(GOLDEN_BIN_PATH, "rb");
        if (!f) {
            printf("test-clut: LOUD-SKIP golden diff -- %s absent\n",
                   GOLDEN_BIN_PATH);
            printf("  (gitignored ground-truth; pass path via "
                   "-DCLUT_ROM_GOLDEN_PATH=... or "
                   "-DSYSTEM7_DECOMP=... or set SYSTEM7_DECOMP in Makefile)\n");
        } else {
            unsigned char rom[2056];
            size_t nread = fread(rom, 1, 2056, f);
            fclose(f);
            if (nread != 2056) {
                g_checks++;
                g_fails++;
                printf("  FAIL golden read: got %u bytes (expected 2056)\n",
                       (unsigned int)nread);
            } else {
                /* Validate ColorTable header */
                unsigned int ct_seed  = be32_buf(rom + 0);
                unsigned short ct_flags = be16_buf(rom + 4);
                unsigned short ct_size  = be16_buf(rom + 6);

                g_checks++;
                if (ct_seed != 8) {
                    g_fails++;
                    printf("  FAIL golden ctSeed expected 8 got %u\n", ct_seed);
                }
                g_checks++;
                if (ct_flags != 0x8000) {
                    g_fails++;
                    printf("  FAIL golden ctFlags expected 0x8000 got 0x%04X\n",
                           ct_flags);
                }
                g_checks++;
                if (ct_size != 255) {
                    g_fails++;
                    printf("  FAIL golden ctSize expected 255 got %u\n", ct_size);
                }

                /* Entry-by-entry diff: high byte of each 16-bit channel */
                {
                    int diff_ok = 1;
                    for (i = 0; i < 256; i++) {
                        unsigned int off = 8 + i * 8;
                        /* value field: must be 0 for device CLUT */
                        unsigned short vf = be16_buf(rom + off);
                        /* high byte = 8-bit component */
                        unsigned char rom_r = rom[off + 2]; /* bytes 2-3: r16 */
                        unsigned char rom_g = rom[off + 4]; /* bytes 4-5: g16 */
                        unsigned char rom_b = rom[off + 6]; /* bytes 6-7: b16 */

                        g_checks++;
                        if (vf != 0) {
                            g_fails++;
                            printf("  FAIL golden idx%u value_field=%u (expected 0)\n",
                                   i, vf);
                        }
                        g_checks++;
                        if (rom_r != flair_clut8[i][0] ||
                            rom_g != flair_clut8[i][1] ||
                            rom_b != flair_clut8[i][2])
                        {
                            g_fails++;
                            if (diff_ok)
                                printf("  FAIL golden diff idx%u: ROM=#%02X%02X%02X "
                                       "table=#%02X%02X%02X\n",
                                       i, rom_r, rom_g, rom_b,
                                       flair_clut8[i][0], flair_clut8[i][1],
                                       flair_clut8[i][2]);
                            diff_ok = 0;
                        }
                    }
                    if (diff_ok)
                        printf("  golden diff: 256 entries match ROM binary -- OK\n");
                }
            }
        }
    }

    /* =================================================================== */
    /* Final result                                                          */
    /* =================================================================== */
    printf("test-clut: %d checks, %d failures, %s\n",
           g_checks, g_fails, g_fails == 0 ? "green" : "RED");
    return g_fails == 0 ? 0 : 1;
}

#else  /* freestanding: no main; _Static_asserts in clut.h are the oracle */
/*
 * In freestanding mode the _Static_assert in clut.h fires at compile time.
 * The -c compile check (see DUAL-COMPILE comment above) is the oracle for
 * the freestanding target. No entry point or runtime needed.
 */
void flair_clut_freestanding_noop(void); /* suppress empty-translation-unit warning */
#endif /* __STDC_HOSTED__ */
