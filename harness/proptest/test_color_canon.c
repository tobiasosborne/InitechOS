/*
 * test_color_canon.c -- THE single FLAIR color VALUE oracle (test-color-canon).
 *
 * beads: initech-mwpw (epic initech-qipc step 3). ADR-0010 CD-2 (the one value
 *   oracle, 4 legs A/B/C/D + named VALUE mutants). era=system7.0-7.1 / win31.
 *
 * PURPOSE: grade the GENERATED color canon (spec/assets/color_canon.h --
 *   flair_canon_rgb(idx) + color_canon[9][3] + INITECH_CANON_*_RGB) against
 *   INDEPENDENT decomp goldens (System-7 wctb binary, Win-3.1 WIN.INI text,
 *   System-7 pinstripe.md rendered rows).  This is the anti-heresy oracle: it
 *   exists because the OLD FLAIR color grade was BY CONSTRUCTION (HER-02 -- it
 *   computed the "expected" RGB from the same function the artifact renders
 *   from, so a wrong palette passed every gate).  CLAUDE.md Law 2:
 *     "An oracle that computes its expected values from the same source the
 *      artifact renders from is not an oracle."
 *
 *   THE VALUE-UNDER-TEST is color_canon.h (generated from color_canon.json).
 *   THE EXPECTED VALUES come from goldens that are INDEPENDENT of
 *   color_canon.json:
 *     LEG A  expected = bytes parsed from the wctb binary resource
 *     LEG B  expected = decimals parsed from the win31 WIN.INI cross-check text
 *     LEG C  expected = hex parsed from pinstripe.md rendered-golden rows
 *     LEG D  authored -- NO external golden exists; gated by a locked authored
 *            constant stated literally here + octant bound + same-hue lock +
 *            VALUE mutants.  NEVER claimed decomp-sourced (P4 honesty).
 *   No leg recomputes its expected value from flair_canon_rgb / color_canon.json
 *   and compares it to itself (that would be by-construction; the whole point of
 *   this oracle is to extinguish that).
 *
 * SOURCE CITATIONS (Law 1):
 *   LEG A: $(SYSTEM7_DECOMP)/goldens/resources/wctb_0_System_753.bin (112 bytes,
 *     MD5 dede55082e92a3c6bce408335cf77614 == wctb_0_System_colorHD.bin).
 *     Format: ../system7-decomp/specs/resources/wctb-mctb-format.md Sec 2.1/2.5.
 *     Header 8B (wCSeed:u32, wCReserved:u16, ctSize:u16=12 => 13 entries),
 *     then 13 ColorSpec {value:u16, r:u16, g:u16, b:u16} big-endian; 8-bit
 *     channel = HIGH byte of each 16-bit word.  part0=#FFFFFF, part1=#000000,
 *     part2=#000000 (verified against the hexdump).
 *   LEG B: $(WIN31_DECOMP)/refs/win31-chrome/default-colors-cross-check.txt --
 *     "Keyname = R G B" decimal lines (3 independent documented WIN.INI sources).
 *     Hilight/ActiveTitle = 0 0 128 (#000080); ButtonFace = 192 192 192
 *     (#C0C0C0).  DEPTH-TRAP (ADR-0010 CD-4, R5): the documented indexed-8 value
 *     is the PRIMARY assertion at TOL=0; the 16-color VGA artifact (#0000AA) is
 *     corroboration-only and MUST FAIL -- no ~42-level blue tolerance.
 *   LEG C: $(SYSTEM7_DECOMP)/specs/chrome/pinstripe.md rendered-golden rows
 *     "166 #F3F3F3  light" / "168 #969696  dark" (era 7.5.3, Basilisk II,
 *     Quadra 650 ROM).  These are the RENDERED dither shades, NOT wctb
 *     part7/part8 (which are the #FFFFFF/#000000 WDEF endpoints, a distinct lane).
 *   LEG D: AUTHORED (no upstream decomp golden).  idx2 teal #8DDCDC +
 *     bevel_light #8DDCDC + bevel_shadow #4E9BA3 -- Initech-identity injections
 *     (operator WL-0053).  color_canon.json grading_contract.authored_exception.
 *   color_canon.h (the LOCKED generated header under test); color_canon.json
 *     (the locked source it is generated from; consulted ONLY for which idx maps
 *     to which golden slot -- the crosswalk -- never for an expected RGB value).
 *
 * DUAL-COMPILE (mirrors test_clut.c):
 *   # Freestanding compile check (header compiles -ffreestanding):
 *   gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -std=c11 \
 *       -Ispec/assets -c harness/proptest/test_color_canon.c -o /tmp/tcc_fs.o
 *   # Hosted binary (runs + prints result):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *       -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson \
 *       -DSYSTEM7_DECOMP='"../system7-decomp"' -DWIN31_DECOMP='"../win31-decomp"' \
 *       harness/proptest/test_color_canon.c -o build/test_color_canon \
 *       && build/test_color_canon
 *
 * MUTATION PROOF (Rule 6): build with ONE of the -D mutant hooks below; each
 *   perturbs the VALUE the oracle reads for one idx (simulating a drifted
 *   header), so the INDEPENDENT golden catches it.  The relevant leg goes RED;
 *   the others stay GREEN.  Restore by dropping the -D.
 *     -DCANON_MUTATE_TEAL   idx2 -> #6FA08E (seafoam relapse)  => LEG D RED
 *     -DCANON_MUTATE_NAVY   idx5 -> #0000AA (16-color VGA)     => LEG B RED
 *     -DCANON_MUTATE_WHITE  idx1 -> #FEFEFE                    => LEG A RED
 *     -DCANON_MUTATE_PIN    idx7 -> #FFFFFF (wctb endpoint,    => LEG C RED
 *                                  NOT the rendered shade)
 *
 * LOUD-SKIP: each leg resolves its golden by macro priority and LOUD-SKIPS that
 *   leg (prints the exact missing path) if absent -- NEVER silent-passes -- and
 *   the summary prints "N rows NOT graded (goldens absent)".  LEG D is authored
 *   (no external golden) so it always runs.  (test_clut.c idiom, ADR-0010 CD-8.)
 *
 * ASCII-clean (Rule 12). Deterministic / no timestamps (Rule 11).
 */

/* The VALUE-UNDER-TEST: the generated color canon. */
#include "color_canon.h"

/* ---------------------------------------------------------------------------
 * MUTANT HOOKS (Rule 6).  canon_val(idx) is the VALUE-UNDER-TEST the oracle
 * reads.  In the default build it is exactly flair_canon_rgb(idx) (the real
 * generated canon).  A mutant hook perturbs ONE idx to simulate a header that
 * drifted off the locked spec, so the INDEPENDENT golden must catch it.  We do
 * NOT edit color_canon.h; the perturbation lives only in this wrapper.
 * ------------------------------------------------------------------------- */
static unsigned long canon_val(unsigned idx)
{
    unsigned long v = (unsigned long)flair_canon_rgb((unsigned char)idx);
#ifdef CANON_MUTATE_TEAL
    if (idx == CIDX_DESKTOP)   v = 0x6FA08EuL; /* seafoam relapse */
#endif
#ifdef CANON_MUTATE_NAVY
    if (idx == CIDX_ACCENT)    v = 0x0000AAuL; /* 16-color VGA navy artifact */
#endif
#ifdef CANON_MUTATE_WHITE
    if (idx == CIDX_WHITE)     v = 0xFEFEFEuL; /* off-white drift */
#endif
#ifdef CANON_MUTATE_PIN
    if (idx == CIDX_PIN_LIGHT) v = 0xFFFFFFuL; /* wctb part7 endpoint, not the
                                                  rendered #F3F3F3 dither shade */
#endif
    return v;
}

/* bevel_light / bevel_shadow are derived rows (not in the 0..8 index table).
 * They are exposed as compile-time macros; expose them through the same mutant
 * wrapper discipline so a LEG-D teal mutant also drives the bevel checks. */
static unsigned long canon_bevel_light(void)
{
    unsigned long v = INITECH_CANON_BEVEL_LIGHT_RGB;
#ifdef CANON_MUTATE_TEAL
    v = 0x6FA08EuL; /* bevel_light tracks idx2 teal; relapse together */
#endif
    return v;
}
static unsigned long canon_bevel_shadow(void)
{
    return (unsigned long)INITECH_CANON_BEVEL_SHADOW_RGB;
}

#if defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 1)
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ---------------------------------------------------------------------------
 * Tally.  We distinguish GRADED (golden present + diffed), SKIPPED (golden
 * absent -> loud-skip), and FAILED (a real mismatch) so a skip-everything run
 * can never be mistaken for a pass (ADR-0010 CD-8.4).
 * ------------------------------------------------------------------------- */
static int g_graded  = 0;  /* rows actually diffed against a golden / authored datum */
static int g_fails   = 0;  /* real mismatches */
static int g_skipped = 0;  /* rows NOT graded because a golden was absent */

#define GRADE(name, expect, got)                                            \
    do {                                                                    \
        g_graded++;                                                         \
        if ((unsigned long)(expect) != (unsigned long)(got)) {             \
            g_fails++;                                                       \
            printf("  FAIL %s: expected #%06lX got #%06lX\n",              \
                   (name), (unsigned long)(expect), (unsigned long)(got)); \
        } else {                                                            \
            printf("  PASS %s: #%06lX (golden) == #%06lX (canon)\n",       \
                   (name), (unsigned long)(expect), (unsigned long)(got)); \
        }                                                                   \
    } while (0)

/* A boolean structural assertion (octant bound, same-hue lock, guardrails). */
#define ASSERT_TRUE(name, cond)                                             \
    do {                                                                    \
        g_graded++;                                                         \
        if (!(cond)) { g_fails++; printf("  FAIL %s\n", (name)); }          \
        else         {            printf("  PASS %s\n", (name)); }          \
    } while (0)

/* ---------------------------------------------------------------------------
 * Golden path resolution (test_clut.c idiom): per-leg macro priority
 *   FLAIR_<LEG>_GOLDEN_PATH > $(SYSTEM7_DECOMP|WIN31_DECOMP)/... > default.
 * The decomp macros are QUOTED string literals (we fopen the path string).
 * ------------------------------------------------------------------------- */
#ifdef FLAIR_WCTB_GOLDEN_PATH
#  define LEG_A_PATH FLAIR_WCTB_GOLDEN_PATH
#elif defined(SYSTEM7_DECOMP)
#  define LEG_A_PATH SYSTEM7_DECOMP "/goldens/resources/wctb_0_System_753.bin"
#else
#  define LEG_A_PATH "../system7-decomp/goldens/resources/wctb_0_System_753.bin"
#endif

#ifdef FLAIR_WIN31_GOLDEN_PATH
#  define LEG_B_PATH FLAIR_WIN31_GOLDEN_PATH
#elif defined(WIN31_DECOMP)
#  define LEG_B_PATH WIN31_DECOMP "/refs/win31-chrome/default-colors-cross-check.txt"
#else
#  define LEG_B_PATH "../win31-decomp/refs/win31-chrome/default-colors-cross-check.txt"
#endif

#ifdef FLAIR_PINSTRIPE_GOLDEN_PATH
#  define LEG_C_PATH FLAIR_PINSTRIPE_GOLDEN_PATH
#elif defined(SYSTEM7_DECOMP)
#  define LEG_C_PATH SYSTEM7_DECOMP "/specs/chrome/pinstripe.md"
#else
#  define LEG_C_PATH "../system7-decomp/specs/chrome/pinstripe.md"
#endif

static void loud_skip(const char *leg, const char *path, const char *override)
{
    g_skipped++;
    printf("  LOUD-SKIP %s -- golden absent: %s\n", leg, path);
    printf("    (gitignored; pass -D%s=... or set the decomp macro in the Makefile)\n",
           override);
}

/* big-endian u16 from a byte buffer (wctb is 68k byte order). */
static unsigned be16(const unsigned char *p)
{
    return ((unsigned)p[0] << 8) | (unsigned)p[1];
}

/* ===========================================================================
 * LEG A -- wctb binary resource (System-7), TOL=0.
 * Expected RGB is PARSED FROM THE BINARY GOLDEN, not from color_canon.*.
 * Cross-walk (color_canon.json wctb_part field):
 *   idx0 CIDX_BLACK     <- wctb part1 (wFrameColor)   #000000
 *   idx1 CIDX_WHITE     <- wctb part0 (wContentColor) #FFFFFF
 *   idx3 CIDX_MENUBAR   <- wctb part0 (bar = content) #FFFFFF
 *   idx4 CIDX_TITLE_INK <- wctb part2 (wTextColor)    #000000
 * =========================================================================== */
static void leg_a_wctb(void)
{
    printf("LEG A -- wctb binary (System-7), TOL=0  [golden: %s]\n", LEG_A_PATH);
    FILE *f = fopen(LEG_A_PATH, "rb");
    if (!f) { loud_skip("LEG A (wctb)", LEG_A_PATH, "FLAIR_WCTB_GOLDEN_PATH"); return; }

    unsigned char buf[256];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    if (n < 8) {
        g_graded++; g_fails++;
        printf("  FAIL LEG A: short read (%u bytes)\n", (unsigned)n);
        return;
    }

    /* Header: wCSeed:u32 @0, wCReserved:u16 @4, ctSize:u16 @6 (= nEntries-1). */
    unsigned ct_size  = be16(buf + 6);
    unsigned n_entries = ct_size + 1;
    g_graded++;
    if (ct_size != 0x000C) {  /* 12 => 13 entries; ground truth from the doc/hexdump */
        g_fails++;
        printf("  FAIL LEG A header ctSize: expected 0x000C (13 entries) got 0x%04X\n",
               ct_size);
    } else {
        printf("  PASS LEG A header ctSize=0x000C => %u entries\n", n_entries);
    }
    if ((size_t)(8 + n_entries * 8) > n) {
        g_graded++; g_fails++;
        printf("  FAIL LEG A: file too short for %u entries\n", n_entries);
        return;
    }

    /* Parse the ColorSpec table into a part-code -> RGB8 map (high byte of each
     * 16-bit channel).  value field is the part code (NOT the array index). */
    unsigned long part_rgb[16];
    int           part_seen[16];
    unsigned i;
    for (i = 0; i < 16; i++) part_seen[i] = 0;
    for (i = 0; i < n_entries; i++) {
        const unsigned char *e = buf + 8 + i * 8;
        unsigned value = be16(e + 0);
        unsigned char r = e[2];   /* bytes 2-3 = red16 BE; high byte = e[2]   */
        unsigned char g = e[4];   /* bytes 4-5 = green16; high byte = e[4]    */
        unsigned char b = e[6];   /* bytes 6-7 = blue16; high byte = e[6]     */
        if (value < 16) {
            part_rgb[value]  = ((unsigned long)r << 16) |
                               ((unsigned long)g <<  8) | (unsigned long)b;
            part_seen[value] = 1;
        }
    }

    /* Confirm the parser actually recovered the anchor parts (Law 1 honesty). */
    if (!part_seen[0] || !part_seen[1] || !part_seen[2]) {
        g_graded++; g_fails++;
        printf("  FAIL LEG A: wctb part0/1/2 not all present in golden\n");
        return;
    }
    printf("  parsed wctb parts: part0=#%06lX part1=#%06lX part2=#%06lX\n",
           part_rgb[0], part_rgb[1], part_rgb[2]);

    /* GRADE the canon idx values against the PARSED wctb part RGBs (TOL=0). */
    GRADE("idx0 CIDX_BLACK vs wctb part1 (wFrameColor)",   part_rgb[1], canon_val(CIDX_BLACK));
    GRADE("idx1 CIDX_WHITE vs wctb part0 (wContentColor)", part_rgb[0], canon_val(CIDX_WHITE));
    GRADE("idx3 CIDX_MENUBAR vs wctb part0 (content white)", part_rgb[0], canon_val(CIDX_MENUBAR));
    GRADE("idx4 CIDX_TITLE_INK vs wctb part2 (wTextColor)", part_rgb[2], canon_val(CIDX_TITLE_INK));
}

/* ===========================================================================
 * LEG B -- win31 WIN.INI cross-check text, TOL=0 (DEPTH-TRAP guardrail).
 * Expected RGB is PARSED FROM THE TEXT GOLDEN ("Keyname = R G B" lines).
 *   idx5 CIDX_ACCENT  <- Hilight / ActiveTitle = 0 0 128  (#000080)
 *   idx6 CIDX_CONTROL <- ButtonFace = 192 192 192         (#C0C0C0)
 * DEPTH-TRAP (CD-4, R5): #000080 graded at TOL=0; a #0000AA header value MUST
 * FAIL (proven by CANON_MUTATE_NAVY).  No ~42-level blue tolerance.
 * =========================================================================== */

/* Parse "Keyname = R G B" -> 0x00RRGGBB; returns 1 if found, 0 if not. */
static int win31_lookup(const char *path, const char *key, unsigned long *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;  /* -1 distinguishes "file absent" from "key not found" */
    char line[512];
    size_t klen = strlen(key);
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        /* match a line whose first token is exactly <key> followed by ws/'=' */
        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, key, klen) != 0) continue;
        const char *after = p + klen;
        if (*after != ' ' && *after != '\t' && *after != '=') continue;
        /* find '=' then read three decimal ints */
        const char *eq = strchr(p, '=');
        if (!eq) continue;
        int r = -1, g = -1, b = -1;
        if (sscanf(eq + 1, " %d %d %d", &r, &g, &b) == 3 &&
            r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
            *out = ((unsigned long)r << 16) | ((unsigned long)g << 8) | (unsigned long)b;
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}

static void leg_b_win31(void)
{
    printf("LEG B -- win31 WIN.INI text (depth-trap, TOL=0)  [golden: %s]\n", LEG_B_PATH);

    unsigned long navy = 0, face = 0;
    int rn = win31_lookup(LEG_B_PATH, "Hilight", &navy);
    if (rn == -1) { loud_skip("LEG B (win31)", LEG_B_PATH, "FLAIR_WIN31_GOLDEN_PATH"); return; }
    /* fall back to ActiveTitle if "Hilight" key not present (same value) */
    if (rn == 0) rn = win31_lookup(LEG_B_PATH, "ActiveTitle", &navy);
    int rf = win31_lookup(LEG_B_PATH, "ButtonFace", &face);

    if (rn != 1 || rf != 1) {
        g_graded++; g_fails++;
        printf("  FAIL LEG B: could not parse Hilight/ActiveTitle (%d) or ButtonFace (%d) "
               "from the win31 text golden\n", rn, rf);
        return;
    }
    printf("  parsed win31: Hilight/ActiveTitle=#%06lX  ButtonFace=#%06lX\n", navy, face);

    /* DEPTH-TRAP guardrail (CD-4): the golden MUST be the documented indexed-8
     * #000080, not the 16-color #0000AA artifact.  Assert the golden we parsed
     * is the indexed-8 datum before grading against it (Law 1: prove the source). */
    ASSERT_TRUE("LEG B depth-trap: win31 golden Hilight is indexed-8 #000080 (not #0000AA)",
                navy == 0x000080uL);

    GRADE("idx5 CIDX_ACCENT vs win31 Hilight/ActiveTitle (indexed-8 navy)", navy, canon_val(CIDX_ACCENT));
    GRADE("idx6 CIDX_CONTROL vs win31 ButtonFace",                          face, canon_val(CIDX_CONTROL));

    /* Law-3 guardrail asserts (CD-4): neither the Win95 COLOR_3DLIGHT #DFDFDF
     * nor the Win desktop teal #008080 may ever be conflated with the canon.
     * These grade the canon (idx2 teal / idx6 control), not the golden. */
    ASSERT_TRUE("guardrail: canon idx6 CONTROL is NOT #DFDFDF (Win95 3DLIGHT import)",
                canon_val(CIDX_CONTROL) != 0xDFDFDFuL);
    ASSERT_TRUE("guardrail: canon idx2 DESKTOP teal is NOT #008080 (Win COLOR_BACKGROUND)",
                canon_val(CIDX_DESKTOP) != 0x008080uL);
}

/* ===========================================================================
 * LEG C -- pinstripe.md rendered-golden rows (System-7), TOL=0.
 * Expected RGB is PARSED FROM THE MARKDOWN GOLDEN rows of the form
 *   "<rownum> #HEXHEX  light|dark".  These are the RENDERED dither shades.
 *   idx7 CIDX_PIN_LIGHT <- the #F3F3F3 'light' row
 *   idx8 CIDX_PIN_DARK  <- the #969696 'dark'  row
 * NOTE: these are NOT wctb part7/part8 (#FFFFFF/#000000 WDEF endpoints); the
 * CANON_MUTATE_PIN mutant (idx7 -> #FFFFFF) proves that distinction bites.
 * =========================================================================== */

/* Scan pinstripe.md for the first row tagged <tag> ("light"/"dark") that
 * carries a #RRGGBB hex; returns 1 if found, 0 if not, -1 if file absent. */
static int pinstripe_lookup(const char *path, const char *tag, unsigned long *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    char line[1024];
    int found = 0;
    size_t taglen = strlen(tag);
    while (fgets(line, sizeof(line), f)) {
        /* A golden stripe row looks like: "166 #F3F3F3  light".  Require BOTH a
         * leading row number, a #hex, and the trailing tag word, to avoid the
         * prose/table lines that also mention these hexes. */
        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (!isdigit((unsigned char)*p)) continue;          /* must start with a row number */
        const char *hash = strchr(p, '#');
        if (!hash) continue;
        unsigned r, g, b;
        if (sscanf(hash, "#%2x%2x%2x", &r, &g, &b) != 3) continue;
        /* the tag word must appear after the hex on this row */
        const char *tagpos = strstr(hash, tag);
        if (!tagpos || (size_t)(tagpos - hash) < 7) continue; /* after "#RRGGBB" */
        /* reject "light" matching inside other words: require ws boundary before */
        if (tagpos > line && !isspace((unsigned char)tagpos[-1])) continue;
        if (tagpos[taglen] != '\0' && !isspace((unsigned char)tagpos[taglen]) &&
            tagpos[taglen] != '(' ) continue;
        *out = ((unsigned long)r << 16) | ((unsigned long)g << 8) | (unsigned long)b;
        found = 1;
        break;
    }
    fclose(f);
    return found;
}

static void leg_c_pinstripe(void)
{
    printf("LEG C -- pinstripe.md rendered rows (System-7), TOL=0  [golden: %s]\n", LEG_C_PATH);

    unsigned long light = 0, dark = 0;
    int rl = pinstripe_lookup(LEG_C_PATH, "light", &light);
    if (rl == -1) { loud_skip("LEG C (pinstripe)", LEG_C_PATH, "FLAIR_PINSTRIPE_GOLDEN_PATH"); return; }
    int rd = pinstripe_lookup(LEG_C_PATH, "dark", &dark);

    if (rl != 1 || rd != 1) {
        g_graded++; g_fails++;
        printf("  FAIL LEG C: could not parse light (%d) / dark (%d) rendered rows "
               "from pinstripe.md\n", rl, rd);
        return;
    }
    printf("  parsed pinstripe.md: light row=#%06lX  dark row=#%06lX\n", light, dark);

    GRADE("idx7 CIDX_PIN_LIGHT vs pinstripe.md rendered LIGHT row", light, canon_val(CIDX_PIN_LIGHT));
    GRADE("idx8 CIDX_PIN_DARK  vs pinstripe.md rendered DARK row",  dark,  canon_val(CIDX_PIN_DARK));
}

/* ===========================================================================
 * LEG D -- AUTHORED (Initech identity; NO external decomp golden).
 * idx2 teal #8DDCDC + bevel_light #8DDCDC + bevel_shadow #4E9BA3 are operator
 * WL-0053 injections (VIC-20 cyan).  No upstream golden exists, so this leg is
 * HONESTLY authored-not-decomp-sourced (P4).  It is gated by:
 *   (a) locked-constant equality -- the canon header value == the
 *       operator-ratified authored datum stated LITERALLY below;
 *   (b) a green-cyan-octant luminance/hue bound (teal = high-G, high-B, lower-R);
 *   (c) bevel_shadow same-hue-as-bevel_light (derived-shadow lock);
 *   (d) the seafoam-relapse VALUE mutant (CANON_MUTATE_TEAL -> #6FA08E RED).
 * The superseded System-7 lavender/gray baseline is recorded as a comment only,
 * NEVER as the expected value (deviation-audit, CD-8.5).
 * =========================================================================== */
static void leg_d_authored(void)
{
    printf("LEG D -- authored Initech teal (NO external golden; WL-0053)\n");

    /* (a) The operator-ratified AUTHORED datum, stated literally here as the
     * locked constant.  This is NOT decomp-sourced; it is the WL-0053 identity
     * injection.  Superseded baseline (deviation-audit, NEVER the expected
     * value): System-7 desktop had no teal; the dialog bevel was lavender
     * #CCCCFF/#DADAFF (wctb part9/11) -- replaced by WL-0053. */
    const unsigned long AUTHORED_TEAL         = 0x8DDCDCuL; /* idx2 + bevel_light */
    const unsigned long AUTHORED_BEVEL_SHADOW = 0x4E9BA3uL; /* darkened teal      */

    GRADE("idx2 CIDX_DESKTOP vs AUTHORED teal datum (WL-0053, locked-constant)",
          AUTHORED_TEAL, canon_val(CIDX_DESKTOP));
    GRADE("bevel_light vs AUTHORED teal datum (== idx2)",
          AUTHORED_TEAL, canon_bevel_light());
    GRADE("bevel_shadow vs AUTHORED darkened-teal datum (locked-constant)",
          AUTHORED_BEVEL_SHADOW, canon_bevel_shadow());

    /* (b) green-cyan octant bound on the canon teal: teal is high-G, high-B,
     * with R strictly below both, and reasonably bright.  A seafoam relapse
     * (#6FA08E: R=111,G=160,B=142) violates B>R margin / B>=G and the bound. */
    {
        unsigned long t = canon_val(CIDX_DESKTOP);
        unsigned tr = (unsigned)((t >> 16) & 0xFF);
        unsigned tg = (unsigned)((t >>  8) & 0xFF);
        unsigned tb = (unsigned)( t        & 0xFF);
        ASSERT_TRUE("idx2 teal octant: G high (>=180)",      tg >= 180);
        ASSERT_TRUE("idx2 teal octant: B high (>=180)",      tb >= 180);
        ASSERT_TRUE("idx2 teal octant: R below G and B",     tr < tg && tr < tb);
        ASSERT_TRUE("idx2 teal octant: B within 24 of G (cyan, not green)",
                    (tg > tb ? tg - tb : tb - tg) <= 24);
        ASSERT_TRUE("idx2 teal octant: R darker than G by >=48 (not muddy seafoam)",
                    tg >= tr + 48);
    }

    /* (c) derived-shadow lock: bevel_shadow is the SAME HUE as bevel_light
     * (a darkened teal), i.e. its channel ordering is B>=R and G>=R and it is
     * darker than the light.  This catches a bevel_shadow that drifts off the
     * teal hue (e.g. a lavender relapse where R climbs above G). */
    {
        unsigned long lo = canon_bevel_shadow();
        unsigned long hi = canon_bevel_light();
        unsigned sr = (unsigned)((lo >> 16) & 0xFF);
        unsigned sg = (unsigned)((lo >>  8) & 0xFF);
        unsigned sb = (unsigned)( lo        & 0xFF);
        unsigned lr = (unsigned)((hi >> 16) & 0xFF);
        ASSERT_TRUE("bevel_shadow same-hue: R below G and B (teal family)",
                    sr < sg && sr < sb);
        ASSERT_TRUE("bevel_shadow same-hue: G/B near-equal (cyan)",
                    (sg > sb ? sg - sb : sb - sg) <= 24);
        ASSERT_TRUE("bevel_shadow derived: darker than bevel_light",
                    sr < lr);
    }
}

/* =========================================================================== */
int main(void)
{
    printf("test-color-canon: starting (FLAIR color VALUE oracle; ADR-0010 CD-2)\n");
    printf("  value-under-test: spec/assets/color_canon.h (flair_canon_rgb / color_canon[])\n");
    printf("  expected values:  INDEPENDENT decomp goldens (A wctb / B win31 / C pinstripe)\n");
    printf("                    + LEG D authored locked-constant (no external golden)\n\n");

    leg_a_wctb();        printf("\n");
    leg_b_win31();       printf("\n");
    leg_c_pinstripe();   printf("\n");
    leg_d_authored();    printf("\n");

    printf("test-color-canon: %d graded, %d failures, %d rows NOT graded (goldens absent), %s\n",
           g_graded, g_fails, g_skipped, g_fails == 0 ? "green" : "RED");
    return g_fails == 0 ? 0 : 1; /* 0 on green, 1 on red */
}

#else  /* freestanding: no main; the #include of color_canon.h is the compile oracle */
void flair_color_canon_freestanding_noop(void); /* suppress empty-TU warning */
#endif /* __STDC_HOSTED__ */
