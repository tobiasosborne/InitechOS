/*
 * ppm_flair_check.c -- the FLAIR LIVE DESKTOP screendump STRUCTURE oracle
 *                      (factory, C-only).
 *
 * beads: initech-re30.3 (LANE 2 -- the screendump oracle); re-keyed under
 *        initech-7x9k / epic initech-qipc step 4b (ADR-0010 CD-5, the single
 *        ppm_flair_check re-key that hard-revokes HER-02). Ref: CLAUDE.md Law 2
 *        ("the oracle is the truth"; "an oracle that computes its expected
 *        values from the same source the artifact renders from is NOT an
 *        oracle"), Law 4 ("it must look like the frame"), Rule 2 (fail
 *        fast/loud), Rule 6 (the gate must BITE -- mutation-proven by
 *        test-flair-desktop-mutant), Rule 11 (deterministic), Rule 12
 *        (ASCII-clean), Law 3 (factory is C). PRD Sec 1/6.3/6.5/Appendix A.
 *
 * WHAT THIS IS (and is NOT): this is the STRUCTURE / screendump oracle ONLY. It
 * grades the GEOMETRY, TOPOLOGY, Z-ORDER and PERIOD-2 ALTERNATION of the composed
 * Office Space chimera desktop as PRESENTED to the live LFB (build/flair_desktop
 * .img, kmain -DBOOT_FLAIR_SHELL) and captured as a QEMU 32bpp screendump (a P6
 * PPM). It is NEVER the color VALUE oracle -- promoting it to one would rebirth
 * HER-01/HER-02 (ADR-0010 BC-2). The color VALUE authority is the SEPARATE,
 * INDEPENDENT oracle test-color-canon (harness/proptest/test_color_canon.c),
 * which grades flair_canon_rgb(idx) against the System-7 / Win-3.1 DECOMP
 * goldens (wctb binary / win31 WIN.INI text / pinstripe.md), NOT by construction.
 *
 * THE HER-02 RE-KEY (ADR-0010 CD-5, what changed and WHY it is now honest):
 * the predecessor of this file computed its "expected" RGB from flair_palette_rgb
 * (spec/assets/palette.h) -- the SAME function kmain's present path renders from.
 * A wrong palette value flowed IDENTICALLY into both the rendered pixel and the
 * "expected" value, so the +/-2 diff could never bite on color: it agreed BY
 * CONSTRUCTION. That was heresy HER-02, the canonical anti-oracle the FLAIR
 * re-ratification was convened against (REVOCATION-RECORD-2026-06-21; ADR-0010
 * Sec 6.2). The fix: the expected colors are now re-keyed onto flair_canon_rgb
 * (spec/assets/color_canon.h), whose VALUES are INDEPENDENTLY decomp-graded by
 * test-color-canon (ADR-0010 CD-2) -- a source DISTINCT from the render. This
 * file no longer owns or vouches for the color values; it owns the STRUCTURE.
 * (flair_palette_rgb is itself now a thin alias to flair_canon_rgb, so the two
 * are byte-identical -- but the JUSTIFICATION has moved: the values are trusted
 * because test-color-canon grades them against the decomp master, not because
 * this oracle reads the same switch the renderer reads.)
 *
 * The structural probes here grade RELATIONS that are INVARIANT to the exact RGB
 * (ink-density ordering, period-2 alternation index relation, frame-vs-neighbor,
 * z-order occlusion); the few absolute-color ANCHORs that remain (e.g. "this bare
 * corner reads the desktop color") are checked against the decomp-graded canon,
 * so a wrong scene still reads wrong and the gate BITES. The +/-2 tolerance is
 * kept TIGHT for capture noise (1-LSB rounding) ONLY -- NOT as a value fudge:
 * the QEMU XRGB8888 -> P6 dump is EXACT, and the canon entries differ by far more
 * than 2 per channel. See the HER-02 demonstration at the foot of this file
 * (-DPPM_FLAIR_HER02_DEMO) which proves the structure/value boundary mechanically.
 *
 * FOUR structural assertions, all must hold (exit 0 iff all pass; non-zero + a
 * fail-loud message naming the assertion + sampled-vs-expected RGB otherwise):
 *
 *   (a) TEAL DESKTOP -- bare-desktop sample points (corners/edges clear of the
 *       windows + the modal + the two menu bars) read INITECH_CANON_DESKTOP_RGB
 *       (flair_canon_rgb index 2 = Initech teal #8DDCDC). test_shell.c assert (1).
 *
 *   (b) TWO MENU BARS -- band [0,20) is the System-7 bar, band [20,40) is the
 *       Photoshop bar. Both bands paint the menubar fill (idx 3) at a far-
 *       right non-title x AND a black (idx 0) baseline; both carry TITLE INK
 *       (idx 0) glyphs. THE CHIMERA TELL: the System-7 band has a dense Apple
 *       slot at x in [0,20) (a filled ink square, >= APPLE_INK_MIN px) while the
 *       Photoshop band does NOT (no Apple slot); AND the Photoshop band carries
 *       title ink far to the right (x in [240,260)) where the System-7 band -- a
 *       short "File Edit View Special" -- has none. A ONE-bar render leaves the
 *       Photoshop band as bare desktop (no fill, no ink) -> RED. test_shell.c
 *       assertion (2).
 *
 *   (c) WINDOW CHROME -- the front window's pinstripe title bar ALTERNATES with
 *       period 2 between idx 7 (#F3F3F3) and idx 8 (#969696) at adjacent y; the
 *       1 px right frame column is painted (non-desktop) and the pixel just
 *       outside it is bare teal (frame is exactly 1 px); the body just below
 *       the title bar is window white (idx 1). test_shell.c assertion (3).
 *
 *   (d) MODAL FILE COPY -- the centered dBoxProc box's thick black (idx 0) border
 *       on all four sides, white (idx 1) interior just inside the 7 px border, the
 *       "Saving tables to disk..." text ink band (a non-white pixel inside the
 *       text rect), AND it OCCLUDES the windows (z-order): a probe point inside
 *       the modal border that ALSO lies over document window 0 reads the modal's
 *       BLACK border, and a modal-interior probe over window 0 reads modal WHITE
 *       -- NOT window content. test_shell.c assertions (4) + (5).
 *
 * The expected colors are the flair_canon_rgb(idx) values (color_canon.h,
 * -Ispec/assets), the now-INDEPENDENTLY-graded canon (NOT the render source); the
 * probe coordinates mirror test_shell.c's scene geometry (W0/W1 bounds, the
 * {140,200,500,280} centered modal, the two 20px bar bands). Tightly calibrated
 * against the live render.
 *
 * Usage: ppm_flair_check <screendump.ppm>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"   /* flair_canon_rgb + INITECH_CANON_*_RGB
                            * (-Ispec/assets). THE independently decomp-graded
                            * canon (graded by test-color-canon, ADR-0010 CD-2);
                            * this file grades STRUCTURE only, never the values. */

/* Tight per-channel tolerance -- for CAPTURE NOISE, NOT a value fudge (ADR-0010
 * CD-5). The QEMU XRGB8888 -> P6 dump is EXACT and the canon entries differ by
 * far more than 2 per channel, so +/-2 only absorbs any 1-LSB rounding while the
 * checker still discriminates a wrong scene. The canon VALUES are trusted because
 * test-color-canon grades them against the INDEPENDENT decomp goldens -- this
 * oracle is NEVER the value authority (it may not be promoted to one; BC-2). */
#define TOL 2

/* ---- The composed-scene geometry (mirrors test_shell.c). ----------------- */
enum { SCRW = 640, SCRH = 480 };

/* Menu bars: each FLAIR_MENUBAR_H = 20 px. Band 0 = System-7 [0,20); band 1 =
 * Photoshop [20,40). (Hardcoded as 20 to keep the tool freestanding-header-free
 * beyond color_canon.h; test-chrome locks FLAIR_MENUBAR_H == 20 vs the JSON.) */
#define MENUBAR_H   20

/* Front document window 1 (lower-right; fully visible, clear of the modal):
 * T120 L300 B360 R560 (test_shell.c W1_*). 1 px frame, 19 px title bar. */
#define W1_T 120
#define W1_L 300
#define W1_B 360
#define W1_R 560
#define FRAME       1
#define TITLEBAR_H  19

/* Back document window 0 (upper-left; overlapped by the centered modal):
 * T80 L60 B300 R360 (test_shell.c W0_*) -- used for the z-order occlusion probe. */
#define W0_T 80
#define W0_L 60
#define W0_B 300
#define W0_R 360

/* The centered FILE COPY modal: {dl,dt,dr,db} = {140,200,500,280} (centered on
 * 640x480), dBoxProc border = 7 px. */
#define DL 140
#define DT 200
#define DR 500
#define DB 280
#define DBORDER     7

/* The Apple-slot density tell: the System-7 band's x in [0,20) is a filled ink
 * square (the Apple menu slot); the Photoshop band has no such slot. Calibrated:
 * System-7 band0 x[0,20) inks ~240 px; Photoshop band1 x[0,20) inks ~39 (glyphs
 * only). Threshold splits them with a wide margin. */
#define APPLE_INK_MIN   150

/* Title-ink presence threshold per probed column block (16x20 cell-ish). The
 * Photoshop bar's title ink must reach the far-right column block [240,260)
 * where the short System-7 string never paints (the two-bar tell). */
#define TITLE_INK_MIN   8

/* ---- PPM P6 reader (same invariant as ppm_text_check). ------------------- */
static int read_uint(FILE *f, long *out)
{
    int c;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(f);
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    long v = 0;
    while (c >= '0' && c <= '9') {
        v = v * 10 + (c - '0');
        c = fgetc(f);
    }
    *out = v;
    return 0;
}

static unsigned char *g_buf;
static long g_w, g_h;

static const unsigned char *at(int x, int y)
{
    return g_buf + ((long)y * g_w + x) * 3;
}

/* Match a pixel against an expected 0x00RRGGBB within TOL. */
static int is_rgb(int x, int y, unsigned int rgb)
{
    const unsigned char *p = at(x, y);
    int r = (int)((rgb >> 16) & 0xFFu);
    int g = (int)((rgb >> 8) & 0xFFu);
    int b = (int)(rgb & 0xFFu);
    return abs((int)p[0] - r) <= TOL &&
           abs((int)p[1] - g) <= TOL &&
           abs((int)p[2] - b) <= TOL;
}

/* idx -> expected RGB, re-keyed onto the INDEPENDENTLY decomp-graded canon
 * flair_canon_rgb (color_canon.h), NOT the render source flair_palette_rgb
 * (ADR-0010 CD-5; HER-02 hard-revoke). The canon VALUES are vouched for by
 * test-color-canon, not by this oracle. The normal gate path NEVER perturbs:
 * IDX() is exactly flair_canon_rgb(i). The HER-02 demonstration perturbs its OWN
 * separate expected (her02_expect below), it does NOT touch IDX(), so the four
 * structural assertions read the true canon. */
static unsigned int IDX(int i)
{
    return (unsigned int)(flair_canon_rgb((unsigned char)i) & 0x00FFFFFFu);
}

/* Count idx-0 (black ink) pixels in a rectangle [x0,x1) x [y0,y1). */
static long ink_in(int x0, int y0, int x1, int y1)
{
    long n = 0;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (is_rgb(x, y, IDX(0)))
                n++;
    return n;
}

static int g_fail = 0;

/* Assert that pixel (x,y) equals palette index `idx`; fail loud naming the
 * assertion + the sampled-vs-expected RGB. */
static void assert_idx(int x, int y, int idx, const char *what)
{
    if (!is_rgb(x, y, IDX(idx))) {
        const unsigned char *p = at(x, y);
        unsigned int e = IDX(idx);
        fprintf(stderr,
                "ppm_flair_check: FAIL %s\n"
                "    at (%d,%d): sampled RGB(%d,%d,%d)=#%02X%02X%02X, "
                "expected idx %d = RGB(%d,%d,%d)=#%06X (tol +/-%d)\n",
                what, x, y, p[0], p[1], p[2], p[0], p[1], p[2],
                idx, (e >> 16) & 0xFF, (e >> 8) & 0xFF, e & 0xFF, e, TOL);
        g_fail = 1;
    }
}

#ifdef PPM_FLAIR_HER02_DEMO
/* ===========================================================================
 * THE HER-02 DEMONSTRATION (ADR-0010 CD-5 / Sec 6.2; the bead asks for it).
 *
 * Built ONLY with -DPPM_FLAIR_HER02_DEMO. Run on the REAL teal screendump
 * (build/flair_desktop.ppm). It makes the STRUCTURE-not-VALUE boundary
 * mechanically visible -- it proves WHERE the color value authority now lives:
 *
 *   - ppm_flair_check is VALUE-BLIND on STRUCTURE. The pure-RELATION probes
 *     (period-2 pinstripe alternation, Apple-slot ink-density ORDERING, the
 *     two-bar far-right ink ORDERING, z-order occlusion) compare pixels to
 *     OTHER pixels, never to an absolute canon RGB. So a canon-VALUE mutation
 *     (teal #8DDCDC -> seafoam #6FA08E, exactly CANON_MUTATE_TEAL) is INVISIBLE
 *     to them: they STAY GREEN. A structural oracle MUST be value-blind on its
 *     relations -- that is what makes it structural, not by-construction.
 *
 *   - The same teal->seafoam VALUE mutation is CAUGHT by the INDEPENDENT value
 *     oracle test-color-canon (LEG D, the seafoam-relapse tripwire) -- because
 *     test-color-canon grades the canon VALUE against the decomp golden, a
 *     source distinct from the render. THAT is the value authority now.
 *
 *   - The CONTRAST that locates the authority: ppm_flair_check's only color
 *     SENSITIVITY is its absolute desktop ANCHOR (section (a)), which reads the
 *     canon idx2 value. Under the perturbed expected (seafoam) that anchor no
 *     longer matches the real teal pixel -- demonstrating the value-sensitive
 *     limb is the canon anchor, NOT the structural relations. ppm_flair_check
 *     borrows that one value from the canon; it does not grade it (BC-2).
 *
 * This is honest because it asserts a TRUE property of the real screendump (the
 * relations hold) under a real value perturbation (the demo's own seafoam
 * expected), and it does NOT fake a pass: if any structural relation actually
 * depended on the absolute desktop RGB, this demo would expose it by going RED.
 * It exits 0 on the expected outcome (relations blind + anchor sensitive),
 * non-zero if either claim is violated.
 * =========================================================================== */
static int her02_classify_pin(int x, int y)
{
    /* Value-FREE classification: which pinstripe shade is this row? -1 if
     * neither. Reads the two canon shades (the REAL canon, not perturbed) only
     * to bucket the pixel; the assertion downstream is a pure index RELATION. */
    if (is_rgb(x, y, IDX(7))) return 7;
    if (is_rgb(x, y, IDX(8))) return 8;
    return -1;
}

static int her02_demo(void)
{
    int demo_fail = 0;
    /* The perturbed desktop expected: the demo's OWN value, NOT IDX(). This is
     * the canon-VALUE mutation (teal -> seafoam) that test-color-canon LEG D
     * catches; here it must be INVISIBLE to the structural relations. */
    const unsigned int her02_desktop_seafoam = 0x6FA08Eu;
    const unsigned int true_teal = IDX(CIDX_DESKTOP); /* real canon idx2 */

    printf("ppm_flair_check[HER02-DEMO]: structure-not-value boundary check\n");
    printf("    perturbing the EXPECTED desktop value teal #%06X -> seafoam "
           "#%06X (== CANON_MUTATE_TEAL)\n", true_teal, her02_desktop_seafoam);

    /* (1) STRUCTURE STAYS GREEN: the phase-locked pinstripe RELATION -- a pure
     * relation over pixel classifications, never reading the desktop color. The
     * System-7 racing stripe is phase-locked (doubled-LIGHT pairs at the band
     * edges; chrome.c + ../system7-decomp pinstripe.md), so the value-blind facts
     * are: every title row is shade 7 or 8, both shades appear, and >=1 adjacent
     * EQUAL-row pair exists (the phase lock). (A strict period-2 relation would be
     * WRONG now -- the real stripe has doubled rows.) */
    {
        const int title_top = W1_T + FRAME;
        /* Scan a CLEAR pinstripe column (right of the close box, LEFT of the
         * centered title) so the title knockout/glyphs do not break the relation
         * (the title sits at the bar center now; beads initech-lxg9). */
        const int pin_x = W1_L + 24;
        int shade_ok = 1, saw_light = 0, saw_dark = 0, doubled = 0, prev = -2;
        for (int k = 0; k < TITLEBAR_H; k++) {
            int s = her02_classify_pin(pin_x, title_top + k);
            if (s < 0) shade_ok = 0;
            if (s == 7) saw_light = 1;
            if (s == 8) saw_dark = 1;
            if (k > 0 && s == prev) doubled = 1;
            prev = s;
        }
        if (!(shade_ok && saw_light && saw_dark && doubled)) {
            fprintf(stderr, "ppm_flair_check[HER02-DEMO]: UNEXPECTED -- pinstripe "
                    "RELATION not green on the real desktop (shade=%d light=%d "
                    "dark=%d doubled=%d)\n",
                    shade_ok, saw_light, saw_dark, doubled);
            demo_fail = 1;
        } else {
            printf("    [blind] phase-locked pinstripe relation (two shades + "
                   "doubled-light pair): GREEN under the seafoam value "
                   "perturbation (relation, not RGB)\n");
        }
    }

    /* (2) STRUCTURE STAYS GREEN: the Apple-slot ink-density ORDERING relation. */
    {
        long sys_apple = ink_in(0, 2, 20, MENUBAR_H - 2);
        long ps_apple  = ink_in(0, MENUBAR_H + 2, 20, 2 * MENUBAR_H - 2);
        if (!(sys_apple >= APPLE_INK_MIN && ps_apple < APPLE_INK_MIN)) {
            fprintf(stderr, "ppm_flair_check[HER02-DEMO]: UNEXPECTED -- Apple-slot "
                    "density ORDERING not green (sys=%ld ps=%ld)\n",
                    sys_apple, ps_apple);
            demo_fail = 1;
        } else {
            printf("    [blind] Apple-slot density ordering (sys=%ld >> ps=%ld): "
                   "GREEN under the seafoam value perturbation\n",
                   sys_apple, ps_apple);
        }
    }

    /* (3) STRUCTURE STAYS GREEN: z-order occlusion is a pixel-vs-pixel relation
     * (the modal border ink occludes the window behind it), value-free. */
    {
        int border_black = is_rgb(143, 240, IDX(0));   /* modal border on top */
        int interior_wht = is_rgb(160, 240, IDX(1));   /* modal interior on top */
        if (!(border_black && interior_wht)) {
            fprintf(stderr, "ppm_flair_check[HER02-DEMO]: UNEXPECTED -- z-order "
                    "occlusion relation not green (border=%d interior=%d)\n",
                    border_black, interior_wht);
            demo_fail = 1;
        } else {
            printf("    [blind] z-order occlusion (modal over window 0): GREEN "
                   "under the seafoam value perturbation\n");
        }
    }

    /* (4) THE CONTRAST -- the ONE value sensitivity ppm_flair_check has is its
     * absolute desktop ANCHOR. Under the perturbed (seafoam) expected, the
     * anchor does NOT match the real teal pixel -- so the value sensitivity
     * lives in the CANON anchor (graded independently by test-color-canon),
     * NOT in the structural relations above. Confirm the real corner is teal
     * and the perturbed seafoam expected would mismatch it. */
    {
        int real_is_teal     = is_rgb(20, 460, true_teal);
        int perturbed_match  = is_rgb(20, 460, her02_desktop_seafoam);
        if (!real_is_teal) {
            fprintf(stderr, "ppm_flair_check[HER02-DEMO]: UNEXPECTED -- bare "
                    "corner is not the real teal #%06X\n", true_teal);
            demo_fail = 1;
        }
        if (perturbed_match) {
            fprintf(stderr, "ppm_flair_check[HER02-DEMO]: UNEXPECTED -- the "
                    "seafoam-perturbed expected MATCHED the real corner "
                    "(teal and seafoam are not distinguishable at +/-%d?)\n", TOL);
            demo_fail = 1;
        }
        if (real_is_teal && !perturbed_match) {
            printf("    [contrast] absolute desktop ANCHOR is VALUE-sensitive: "
                   "real corner reads teal #%06X, the seafoam-perturbed expected "
                   "#%06X does NOT match -- that sensitivity is the canon anchor, "
                   "graded by test-color-canon (NOT owned here)\n",
                   true_teal, her02_desktop_seafoam);
        }
    }

    if (demo_fail) {
        fprintf(stderr, "ppm_flair_check[HER02-DEMO]: FAIL -- the structure/value "
                "boundary did not hold as claimed\n");
        return 1;
    }
    printf("ppm_flair_check[HER02-DEMO]: PASS -- the structural RELATIONS are "
           "VALUE-BLIND (a teal->seafoam canon mutation is invisible to them, as "
           "it must be for a structure oracle); the value authority is the "
           "INDEPENDENT test-color-canon (LEG D catches the same mutation RED). "
           "HER-02 is fixed: structure here, value there.\n");
    return 0;
}
#endif /* PPM_FLAIR_HER02_DEMO */

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <screendump.ppm>\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "ppm_flair_check: cannot open %s\n", argv[1]);
        return 2;
    }
    char magic[3] = {0};
    if (fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "ppm_flair_check: not a P6 PPM (got '%s')\n", magic);
        fclose(f);
        return 2;
    }
    long w = 0, h = 0, maxv = 0;
    if (read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv)) {
        fprintf(stderr, "ppm_flair_check: malformed PPM header\n");
        fclose(f);
        return 2;
    }
    if (w != SCRW || h != SCRH || maxv != 255) {
        fprintf(stderr,
                "ppm_flair_check: unexpected dims/maxval w=%ld h=%ld max=%ld "
                "(expect %dx%d, 255)\n", w, h, maxv, SCRW, SCRH);
        fclose(f);
        return 2;
    }
    /* read_uint already consumed the single whitespace byte after maxval, so the
     * file position is exactly at the raster (same invariant as the sibling
     * checkers -- do NOT skip another byte). */
    long npix = w * h;
    g_buf = malloc((size_t)npix * 3);
    if (!g_buf) {
        fprintf(stderr, "ppm_flair_check: OOM\n");
        fclose(f);
        return 2;
    }
    size_t got = fread(g_buf, 3, (size_t)npix, f);
    fclose(f);
    if (got != (size_t)npix) {
        fprintf(stderr, "ppm_flair_check: short raster (%zu of %ld pixels)\n",
                got, npix);
        free(g_buf);
        return 2;
    }
    g_w = w;
    g_h = h;

#ifdef PPM_FLAIR_HER02_DEMO
    /* HER-02 demonstration build: run the structure-not-value boundary check on
     * the real teal screendump and exit with its verdict (the normal four
     * assertions are not the point of this build). */
    {
        int dr = her02_demo();
        free(g_buf);
        return dr;
    }
#endif

    printf("ppm_flair_check: %ldx%ld P6, tol +/-%d -- expected indices: "
           "desktop=#%06X menubar=#%06X white=#%06X ink=#%06X "
           "pinstripe L=#%06X D=#%06X\n",
           w, h, TOL, IDX(2), IDX(3), IDX(1), IDX(0), IDX(7), IDX(8));

    /* ======================================================================
     * (a) TEAL DESKTOP -- bare-desktop sample points, every one clear of the
     * two bars (y>=40), window 0 (L60-360,T80-300), window 1 (L300-560,T120-360)
     * and the modal (L140-500,T200-280). idx2 = Initech teal #8DDCDC, the canon
     * desktop background (graded by test-color-canon LEG D). test_shell.c
     * assertion (1). (These are absolute-color ANCHORs against the decomp-graded
     * canon idx2 -- NOT a value grade owned here; see the file header.)
     * ====================================================================== */
    assert_idx( 20, 460, 2, "(a) bare desktop bottom-left reads teal");
    assert_idx(600, 460, 2, "(a) bare desktop bottom-right reads teal");
    assert_idx( 20,  60, 2, "(a) bare desktop above-left reads teal");
    assert_idx(620,  60, 2, "(a) bare desktop top-right reads teal");
    assert_idx(620, 400, 2, "(a) bare desktop right edge reads teal");
    assert_idx( 40, 400, 2, "(a) bare desktop left edge reads teal");

    /* ======================================================================
     * (b) TWO MENU BARS + the chimera tell. test_shell.c assertion (2).
     * ====================================================================== */
    /* Both bands: a far-right non-title x reads the menubar-gray fill (idx 3),
     * and the band baseline (bottom row) reads black ink (idx 0). */
    assert_idx(600, 5,              3, "(b) System-7 bar fill (rows [0,20)) is menubar gray");
    assert_idx(300, MENUBAR_H - 1,  0, "(b) System-7 bar baseline (y=19) is black ink");
    assert_idx(600, MENUBAR_H + 5,  3, "(b) Photoshop bar fill (rows [20,40)) is menubar gray");
    assert_idx(300, 2 * MENUBAR_H - 1, 0, "(b) Photoshop bar baseline (y=39) is black ink");

    /* THE CHIMERA TELL #1: the System-7 band has a dense Apple slot at x[0,20)
     * (a filled ink square), the Photoshop band does NOT. */
    {
        long sys_apple = ink_in(0, 2, 20, MENUBAR_H - 2);
        long ps_apple  = ink_in(0, MENUBAR_H + 2, 20, 2 * MENUBAR_H - 2);
        if (sys_apple < APPLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) System-7 bar Apple slot missing -- "
                    "x[0,20) ink=%ld (need >=%d); the top bar is not the System-7 bar\n",
                    sys_apple, APPLE_INK_MIN);
            g_fail = 1;
        }
        if (ps_apple >= APPLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) Photoshop bar has an Apple slot -- "
                    "x[0,20) ink=%ld (must be <%d); the two bands are not DISTINCT bars\n",
                    ps_apple, APPLE_INK_MIN);
            g_fail = 1;
        }
        printf("    (b) Apple-slot tell: System-7 x[0,20) ink=%ld (>=%d req), "
               "Photoshop x[0,20) ink=%ld (<%d req)\n",
               sys_apple, APPLE_INK_MIN, ps_apple, APPLE_INK_MIN);
    }

    /* THE CHIMERA TELL #2 (the load-bearing two-bar assertion): the Photoshop
     * bar carries TITLE INK far to the right (x[240,260)) where the short
     * System-7 string ("File Edit View Special") never paints. A ONE-bar render
     * (SHELL_MUTATE_ONE_MENUBAR) leaves the Photoshop band as bare desktop -> no
     * fill, no baseline ink, no far-right title ink -> all three RED above + here. */
    {
        long ps_far = ink_in(240, MENUBAR_H + 2, 260, 2 * MENUBAR_H - 2);
        long sys_far = ink_in(240, 2, 260, MENUBAR_H - 2);
        if (ps_far < TITLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) Photoshop bar title ink missing at "
                    "the far-right (x[240,260) ink=%ld, need >=%d) -- the long "
                    "'File Edit Image Layer Select View Window Help' chimera bar "
                    "is absent (one-bar render?)\n",
                    ps_far, TITLE_INK_MIN);
            g_fail = 1;
        }
        printf("    (b) two-bar tell: Photoshop far-right x[240,260) title ink=%ld "
               "(>=%d req), System-7 far-right ink=%ld (short bar, expected ~0)\n",
               ps_far, TITLE_INK_MIN, sys_far);
        /* Also assert each band carries SOME title ink to the right of the slot
         * region (proves both bars are rendered, not just baselines). */
        long sys_title = ink_in(20, 2, 220, MENUBAR_H - 2);
        long ps_title  = ink_in(0, MENUBAR_H + 2, 220, 2 * MENUBAR_H - 2);
        if (sys_title < TITLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) System-7 bar has no title glyph ink "
                    "(x[20,220) ink=%ld, need >=%d)\n", sys_title, TITLE_INK_MIN);
            g_fail = 1;
        }
        if (ps_title < TITLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) Photoshop bar has no title glyph ink "
                    "(x[0,220) ink=%ld, need >=%d)\n", ps_title, TITLE_INK_MIN);
            g_fail = 1;
        }
    }

    /* ======================================================================
     * (c) WINDOW CHROME (front window 1). test_shell.c assertion (3).
     * ====================================================================== */
    {
        const int title_top = W1_T + FRAME;          /* 121 */
        const int title_bot = title_top + TITLEBAR_H; /* 140 */
        const int mid_x = (W1_L + W1_R) / 2;          /* 430 = the centered title  */
        /* The stripe is scanned at a clear column -- right of the close box and LEFT
         * of the centered title ("untitled-2"), so the title knockout/glyphs do not
         * interrupt the stripe run (beads initech-lxg9). */
        const int pin_x = W1_L + 24;                  /* 324 */
        const int body_row = (title_bot + (W1_B - FRAME)) / 2;

        /* The title bar must be a two-shade STRIPE (every row is shade 7 or 8, both
         * appear, with >=1 adjacent change). The specific System-7 racing-stripe
         * PHASE (the patAlign mod-8 doubled-LIGHT pairs) is graded against the
         * INDEPENDENT ../system7-decomp golden by test-chrome-fidelity (beads
         * initech-hmll), NOT here: a strict period-2 assertion would be WRONG (the
         * phase-locked stripe is not strict period-2) and accepted the free-running
         * L,D,L,D bug. This leg grades STRUCTURE; the fidelity oracle grades phase. */
        int shade_ok = 1, saw_light = 0, saw_dark = 0, striped = 0, prev = -1;
        for (int k = 0; k < TITLEBAR_H; k++) {
            int y = title_top + k;
            int s;
            if      (is_rgb(pin_x, y, IDX(7))) s = 7;
            else if (is_rgb(pin_x, y, IDX(8))) s = 8;
            else { s = -1; shade_ok = 0; }
            if (s == 7) saw_light = 1;
            if (s == 8) saw_dark = 1;
            if (k > 0 && s != prev) striped = 1;
            prev = s;
        }
        if (!shade_ok) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (c) front window title bar at x=%d is not "
                    "the pinstripe shade pair (idx 7 #%06X / idx 8 #%06X)\n",
                    pin_x, IDX(7), IDX(8));
            g_fail = 1;
        }
        if (!(saw_light && saw_dark && striped)) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (c) front window title bar at x=%d is not a "
                    "two-shade pinstripe (light=%d dark=%d striped=%d)\n",
                    pin_x, saw_light, saw_dark, striped);
            g_fail = 1;
        }
        printf("    (c) pinstripe at x=%d: %s (idx7 #%06X / idx8 #%06X)\n",
               pin_x, (saw_light && saw_dark && striped) ? "two-shade stripe" : "BAD",
               IDX(7), IDX(8));

        /* TITLE (beads initech-lxg9): the window name is drawn CENTERED in Chicago
         * over a knocked-out light gap. Live-screendump tripwire: assert FIGURE ink
         * (black, idx 4 = CIDX_TITLE_INK) is present in the centered title region --
         * a blank title bar is a regression. (The full title geometry/knockout is
         * graded host-side by test-chrome-fidelity.) */
        {
            int title_ink = 0;
            for (int y = title_top + 2; y < title_bot - 2; y++) {
                for (int x = mid_x - 24; x < mid_x + 24; x++) {
                    if (is_rgb(x, y, IDX(4))) title_ink++;
                }
            }
            if (title_ink < 8) {
                fprintf(stderr,
                        "ppm_flair_check: FAIL (c) front window title TEXT missing -- "
                        "centered title region [x %d..%d] has %d ink px (need >=8); "
                        "the title bar is blank\n",
                        mid_x - 24, mid_x + 24, title_ink);
                g_fail = 1;
            }
            printf("    (c) title ink (idx4 black) in centered region: %d px\n",
                   title_ink);
        }

        /* The row just below the title bar is the white body (idx 1) -> the title
         * bar is EXACTLY TITLEBAR_H tall. */
        assert_idx(mid_x, title_bot, 1,
                   "(c) front window row below title bar is white body (title height exact)");
        assert_idx(mid_x, body_row, 1,
                   "(c) front window body fill is window white");

        /* Frame: exactly 1 px. The right frame column is painted (non-desktop);
         * the pixel just outside is bare teal. (Value-free RELATION: the frame
         * pixel must DIFFER from the desktop color and the neighbor must MATCH
         * it -- a topology test, not an RGB grade.)
         *
         * LIVE-SHADOW NOTE (beads initech-54nw + follow-up initech-9d0e; Law 1
         * honesty). The documentProc 1px drop shadow at offset (1,1) IS emitted by
         * flair_draw_document_window and graded by test-chrome-fidelity (host,
         * UNCLIPPED port). It does NOT appear on the booted desktop here because
         * the live compositor clips each window's chrome to its strucRgn
         * (desktop.c:223) and strucRgn == the window bounds -- it does NOT yet
         * include the shadow band (real System 7 CalcDoc: struct = content + frame
         * + shadow band; window-frame.md Sec 1). The drawer's frame rect is itself
         * bbox(strucRgn) (desktop.c:117), so widening strucRgn to render the shadow
         * live also shifts the frame -- the fix must DECOUPLE the drawer rect from
         * the clip (the region-layer follow-up initech-7sd2). So x=W1_R is
         * genuinely bare teal on the booted desktop today; this assertion stays
         * TRUE to the current render (Law 2: do not assert a pixel the OS does not
         * paint). */
        if (is_rgb(W1_R - 1, body_row, IDX(2))) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (c) front window right frame column "
                    "(x=%d) reads bare teal -- the frame is not painted\n", W1_R - 1);
            g_fail = 1;
        }
        assert_idx(W1_R, body_row, 2,
                   "(c) pixel just right of the 1 px frame is bare teal "
                   "(the drop shadow is clipped by strucRgn live -- initech-9d0e)");
    }

    /* ======================================================================
     * (d) MODAL FILE COPY + z-order occlusion. test_shell.c assertions (4)+(5).
     * ====================================================================== */
    {
        /* Centered (sanity). */
        if ((DL + DR) / 2 != SCRW / 2 || (DT + DB) / 2 != SCRH / 2) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (d) modal bounds not centered on %dx%d\n",
                    SCRW, SCRH);
            g_fail = 1;
        }
        const int cy = (DT + DB) / 2;  /* 240 */
        const int cx = (DL + DR) / 2;  /* 320 */

        /* The 7-px dBoxProc border is black on all four sides. */
        assert_idx(DL + 3, cy, 0, "(d) modal left border is black (dBoxProc 7 px)");
        assert_idx(DR - 4, cy, 0, "(d) modal right border is black (dBoxProc 7 px)");
        assert_idx(cx, DT + 3, 0, "(d) modal top border is black (dBoxProc 7 px)");
        assert_idx(cx, DB - 4, 0, "(d) modal bottom border is black (dBoxProc 7 px)");

        /* First content row just inside the 7-px border is white (idx 1) -> the
         * border is EXACTLY 7 px. */
        assert_idx(cx, DT + DBORDER, 1,
                   "(d) modal interior just inside the 7 px border is white");

        /* The "Saving tables to disk..." static text is RENDERED as ink: the text
         * rect (left=154, top=212, 16 px tall) contains a non-white pixel. */
        {
            int tx0 = 154, ty0 = 212;
            int text_ink = 0;
            for (int x = tx0; x < tx0 + 300 && !text_ink; x++)
                for (int y = ty0; y < ty0 + 16; y++)
                    if (!is_rgb(x, y, IDX(1))) { text_ink = 1; break; }
            if (!text_ink) {
                fprintf(stderr,
                        "ppm_flair_check: FAIL (d) 'Saving tables to disk...' text "
                        "did NOT render (no ink in the text band) -- modal absent?\n");
                g_fail = 1;
            }
        }

        /* The progress bar (left=154, top=236, right=486, bottom=256): its left
         * border is black and its interior (value 0) is white. */
        assert_idx(154, 246, 0, "(d) progress bar left border is painted (black)");
        assert_idx(300, 246, 1, "(d) progress bar interior (value 0) is white");

        /* Z-ORDER: the modal occludes the windows. Probe (143,240) lies BOTH
         * inside the modal's 7-px LEFT border (x in [140,147)) AND inside window 0
         * (x in [60,360), y in [80,300)). Correct z-order -> modal BLACK border. */
        int in_modal_border = (143 >= DL && 143 < DL + DBORDER && 240 >= DT && 240 < DB);
        int in_window0      = (143 >= W0_L && 143 < W0_R && 240 >= W0_T && 240 < W0_B);
        if (!(in_modal_border && in_window0)) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (d) z-order probe (143,240) is not over "
                    "BOTH the modal border and window 0 (test would be vacuous)\n");
            g_fail = 1;
        }
        assert_idx(143, 240, 0,
                   "(d) modal OCCLUDES the window behind it -- modal black border on top (z-order)");
        assert_idx(160, 240, 1,
                   "(d) modal interior occludes the window behind it (modal white on top)");
    }

    free(g_buf);

    if (g_fail) {
        fprintf(stderr, "ppm_flair_check: FAIL -- the live FLAIR desktop screendump "
                        "is NOT the Office Space frame structure\n");
        return 1;
    }
    printf("ppm_flair_check: PASS -- teal desktop + TWO stacked menu bars + "
           "window chrome + centered FILE COPY modal occluding the windows\n");
    return 0;
}
