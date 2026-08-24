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
 * grades the GEOMETRY, TOPOLOGY, Z-ORDER and title-band row relations of the composed
 * Office Space chimera desktop as PRESENTED to the live LFB (build/flair_desktop
 * .img, kmain -DBOOT_FLAIR_SHELL) and captured as a QEMU 32bpp screendump (a P6
 * PPM). It is NEVER the color VALUE oracle -- promoting it to one would rebirth
 * HER-01/HER-02 (ADR-0010 BC-2). The color VALUE authority is the SEPARATE,
 * INDEPENDENT oracle test-color-canon (harness/proptest/test_color_canon.c),
 * which grades flair_canon_rgb(idx) against the System-7 / Win-3.1 / sys8
 * DECOMP goldens, NOT by construction.
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
 * (ink-density ordering, strict stripe alternation, frame-vs-neighbor,
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
 *   (b) TWO MENU BARS -- band [0,20) is the system bar, band [20,40) is the
 *       Photoshop bar. Both bands paint the sampled Platinum white/E7/B3/black
 *       profile at a far-right non-title x; both carry TITLE INK
 *       (idx 0) glyphs. THE CHIMERA TELL: the System-7 band has Apple-menu-slot
 *       ink at x in [0,20) well above APPLE_INK_MIN (the hand-authored
 *       apple-with-bite glyph, spec/assets/apple_glyph.h -- initech-yx4v; NOT a
 *       filled square, but still far denser than the Photoshop band's incidental
 *       glyph bleed) while the Photoshop band does NOT (no Apple slot at all);
 *       AND the Photoshop band carries title ink far to the right (x in
 *       [240,260)) where the System-7 band -- a short "File Edit View Special"
 *       -- has none. A ONE-bar render leaves the Photoshop band as bare desktop
 *       (no fill, no ink) -> RED. test_shell.c assertion (2).
 *
 *   (c) WINDOW CHROME -- the front window has the exact Platinum 22-row title
 *       profile: black, white, two face rows, 12 strict light-first alternating
 *       stripe rows (white / CIDX_PLAT_STRIPE_DARK), four face rows, shadow,
 *       black. The right body edge carries the measured 4-px raised bar between
 *       its two black lines, and the body starts below row 21. DEC-10 Sec 4;
 *       sys8/window-chrome.md Sec 2.1, 2.2, and 4. test_shell.c assertion (3).
 *
 *   (d) MODAL FILE COPY -- the MOVEABLE TITLED modal (movableDBoxProc; beads
 *       initech-zvo6): the same exact Platinum 22-row title profile as a document
 *       window, then a PLAIN 1-px frame (idx0) around the whole box -- NOT
 *       the old dBoxProc 7-px solid border; the "Saving tables to disk..."
 *       text ink band (a non-white pixel inside the text rect); the progress
 *       bar with a NON-ZERO canon fill (idx5 navy; beads initech-a90f, was a
 *       zero-fill bug); AND it OCCLUDES the windows (z-order): a probe point
 *       on the modal's frame that ALSO lies over document window 0 reads the
 *       modal's BLACK frame, and a modal-interior probe over window 0 reads
 *       modal WHITE -- NOT window content. test_shell.c assertions (4) + (5).
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
 * beyond color_canon.h; test-chrome locks FLAIR_MENUBAR_H == 20 vs the JSON.)
 * DEC-10 Sec 4 audit: sys8/menus.md Sec 1.1 verifies 20 px is era-stable; the
 * two deliberately different chimera bars and Apple-density tell remain. */
#define MENUBAR_H   20

/* Front document window 1 (lower-right; fully visible, clear of the modal):
 * T120 L300 B360 R560 (test_shell.c W1_*).
 *
 * DEC-10 Sec 4 + sys8/window-chrome.md Sec 2.1/2.2/4: Platinum has a 22-row
 * title band, its 12 stripe rows start at T+4, and a 4-px raised body bar sits
 * between the outer and inner black lines. These values stay hardcoded here to
 * preserve this grader's independent, freestanding posture. */
#define W1_T 120
#define W1_L 300
#define W1_B 360
#define W1_R 560
#define FRAME       1
#define TITLEBAR_H             22
#define TITLE_STRIPE_TOP_OFF    4
#define TITLE_STRIPE_ROWS      12
#define BODY_BAR_ROWS           4

/* Back document window 0 (upper-left; overlapped by the centered modal):
 * T80 L60 B300 R360 (test_shell.c W0_*) -- used for the z-order occlusion probe. */
#define W0_T 80
#define W0_L 60
#define W0_B 300
#define W0_R 360

/* The centered FILE COPY modal: {dl,dt,dr,db} = {140,200,500,280} (centered on
 * 640x480). MOVEABLE TITLED chrome (movableDBoxProc; beads initech-zvo6): the
 * Platinum title profile over the top TITLEBAR_H (22) px + a PLAIN 1-px frame
 * (FRAME) -- NOT the old dBoxProc 7-px solid border. Progress bar canon value
 * FLAIR_CANON_FILECOPY_PROGRESS (68; ~65-70% per bug-hunt #25, initech-a90f). */
#define DL 140
#define DT 200
#define DR 500
#define DB 280
#define FILECOPY_CANON_PROGRESS  68

/* The Apple-slot density tell: the System-7 band's x in [0,20) carries the
 * hand-authored apple-with-bite glyph (spec/assets/apple_glyph.h -- initech-
 * yx4v; NOT the old solid-square bug); the Photoshop band has no Apple slot at
 * all. Calibrated against the live render (build/desktop_scene.ppm):
 * System-7 band0 x[0,20) inks == 94 px (the glyph's own ink count, apple_
 * glyph.h APPLE_GLYPH_ROWS -- test_menu.c's PROPERTY 5 asserts the render
 * reproduces this exactly); Photoshop band1 x[0,20) inks ~39 (incidental title-
 * glyph bleed only, no Apple slot). The threshold sits at the midpoint with a
 * margin of ~25-30 px on each side -- narrower than the old 150-vs-(240,39)
 * split because a real glyph is sparser than a filled square, but the ordering
 * relation (sys >> ps) still holds with room to spare. */
#define APPLE_INK_MIN   65

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

/* Exact Platinum title-band cross-section at a clear (non-widget/non-text)
 * column. This is the DEC-10 Sec 4 replacement for the removed System-7
 * 19-row/15-stripe tell. Values and row roles: sys8/window-chrome.md Sec 2.1;
 * strict white-first/dark-last alternation: Sec 2.2. The expected indices are
 * independent color-canon roles, never derived from chrome_metrics.h. */
static int platinum_title_profile(int x, int top, const char *leg)
{
    static const int expected[TITLEBAR_H] = {
        CIDX_BLACK,
        CIDX_WHITE,
        CIDX_PLAT_FRAME_FACE, CIDX_PLAT_FRAME_FACE,
        CIDX_WHITE, CIDX_PLAT_STRIPE_DARK,
        CIDX_WHITE, CIDX_PLAT_STRIPE_DARK,
        CIDX_WHITE, CIDX_PLAT_STRIPE_DARK,
        CIDX_WHITE, CIDX_PLAT_STRIPE_DARK,
        CIDX_WHITE, CIDX_PLAT_STRIPE_DARK,
        CIDX_WHITE, CIDX_PLAT_STRIPE_DARK,
        CIDX_PLAT_FRAME_FACE, CIDX_PLAT_FRAME_FACE,
        CIDX_PLAT_FRAME_FACE, CIDX_PLAT_FRAME_FACE,
        CIDX_PLAT_FRAME_SHADOW,
        CIDX_BLACK
    };
    int bad = 0;

    for (int dy = 0; dy < TITLEBAR_H; dy++) {
        if (!is_rgb(x, top + dy, IDX(expected[dy]))) {
            const unsigned char *p = at(x, top + dy);
            fprintf(stderr,
                    "ppm_flair_check: FAIL %s Platinum title profile row T+%d "
                    "at (%d,%d): sampled #%02X%02X%02X, expected idx %d #%06X\n",
                    leg, dy, x, top + dy, p[0], p[1], p[2], expected[dy],
                    IDX(expected[dy]));
            bad = 1;
        }
    }
    if (bad) g_fail = 1;
    return bad;
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
 *     (strict Platinum stripe alternation, Apple-slot ink-density ORDERING, the
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
static int her02_classify_stripe(int x, int y)
{
    /* Value-FREE classification: which Platinum stripe role is this row? -1 if
     * neither. sys8/window-chrome.md Sec 2.2 (DEC-10 Sec 4): light is canon
     * white and dark is CIDX_PLAT_STRIPE_DARK. */
    if (is_rgb(x, y, IDX(CIDX_WHITE))) return 0;
    if (is_rgb(x, y, IDX(CIDX_PLAT_STRIPE_DARK))) return 1;
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

    /* (1) STRUCTURE STAYS GREEN: the Platinum stripe RELATION -- a pure relation
     * over pixel classifications, never reading the desktop color. DEC-10 Sec 4
     * + sys8/window-chrome.md Sec 2.2 replace the System-7 phase-lock signature:
     * exactly 12 rows alternate strictly, light first and dark last, with NO
     * adjacent-equal pair. */
    {
        const int stripe_top = W1_T + TITLE_STRIPE_TOP_OFF;
        const int stripe_bot = stripe_top + TITLE_STRIPE_ROWS;
        /* Scan a CLEAR stripe column (right of the close box, LEFT of the
         * centered title) so the title knockout/glyphs do not break the relation
         * (the title sits at the bar center now; beads initech-lxg9). */
        const int pin_x = W1_L + 24;
        int relation_ok = 1;
        for (int y = stripe_top; y < stripe_bot; y++) {
            int s = her02_classify_stripe(pin_x, y);
            int want = (y - stripe_top) & 1;
            if (s != want) relation_ok = 0;
        }
        if (!relation_ok) {
            fprintf(stderr, "ppm_flair_check[HER02-DEMO]: UNEXPECTED -- Platinum "
                    "12-row light-first/no-doubled stripe RELATION not green on "
                    "the real desktop\n");
            demo_fail = 1;
        } else {
            printf("    [blind] Platinum stripe relation (12 rows, light-first, "
                   "strict alternation): GREEN under the seafoam value "
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
     * (the modal frame ink occludes the window behind it), value-free.
     * x=140 is the modal's plain 1px LEFT frame column (was x=143, inside the
     * OLD 7px dBoxProc border band, before initech-zvo6's moveable-titled
     * chrome). */
    {
        int border_black = is_rgb(140, 240, IDX(0));   /* modal frame on top */
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
           "desktop=#%06X menu-face=#%06X white=#%06X ink=#%06X "
           "Platinum stripe L=#%06X D=#%06X\n",
           w, h, TOL, IDX(CIDX_DESKTOP), IDX(CIDX_PLAT_FACE),
           IDX(CIDX_WHITE), IDX(CIDX_BLACK), IDX(CIDX_WHITE),
           IDX(CIDX_PLAT_STRIPE_DARK));

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
    /* Both bands: sampled Platinum profile. Ref: sys8/menus.md Sec 1.1;
     * strictly-stronger re-key under bead initech-sjvq row 30. */
    assert_idx(600, 0, CIDX_WHITE, "(b) bar 1 row 0 is sampled white");
    assert_idx(600, 5, CIDX_PLAT_FACE, "(b) bar 1 face rows are sampled E7");
    assert_idx(600, MENUBAR_H - 2, CIDX_PLAT_FRAME_SHADOW,
               "(b) bar 1 row 18 is sampled B3 shadow");
    assert_idx(300, MENUBAR_H - 1,  0, "(b) bar 1 baseline (y=19) is black ink");
    assert_idx(600, MENUBAR_H, CIDX_WHITE, "(b) Photoshop bar row 0 is sampled white");
    assert_idx(600, MENUBAR_H + 5, CIDX_PLAT_FACE,
               "(b) Photoshop bar face rows are sampled E7");
    assert_idx(600, 2 * MENUBAR_H - 2, CIDX_PLAT_FRAME_SHADOW,
               "(b) Photoshop bar row 18 is sampled B3 shadow");
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
        /* DEC-10 Sec 4 + sys8/window-chrome.md Sec 2.1/2.2: T is the top
         * frame row; stripes occupy T+4..T+15 and the content begins after the
         * bottom frame row T+21. */
        const int stripe_top = W1_T + TITLE_STRIPE_TOP_OFF;
        const int stripe_bot = stripe_top + TITLE_STRIPE_ROWS;
        const int content_top = W1_T + TITLEBAR_H;
        const int mid_x = (W1_L + W1_R) / 2;          /* 430 = the centered title  */
        /* The profile is scanned at a clear column -- right of the close box and LEFT
         * of the centered title ("untitled-2"), so the title knockout/glyphs do not
         * interrupt the stripe run (beads initech-lxg9). */
        const int pin_x = W1_L + 24;                  /* 324 */
        const int body_row = (content_top + (W1_B - FRAME)) / 2;
        const int inner_line = W1_R - FRAME - BODY_BAR_ROWS - FRAME;
        /* x=530 is right of the modal (x<500), left of the vertical gutter
         * (x>=539), and inside the Platinum content area. The former x=540
         * probe became the active scrollbar well after the 4-px body bar moved
         * the gutter inward. Measured first in build/flair_desktop.ppm. */
        const int body_x = W1_R - 30;

        if (!platinum_title_profile(pin_x, W1_T, "(c) front-window")) {
            printf("    (c) Platinum title profile at x=%d: 22 rows, 12 strict "
                   "white/#%06X stripes, light-first/dark-last\n",
                   pin_x, IDX(CIDX_PLAT_STRIPE_DARK));
        }

        /* The centered title knocks both stripe parities out to frame-face 218.
         * Probe rows T+4/T+5 before the glyph band begins. DEC-10 Sec 4;
         * sys8/window-chrome.md Sec 2.3. */
        assert_idx(mid_x, stripe_top, CIDX_PLAT_FRAME_FACE,
                   "(c) active title light-row gap fill is Platinum frame face");
        assert_idx(mid_x, stripe_top + 1, CIDX_PLAT_FRAME_FACE,
                   "(c) active title dark-row gap fill is Platinum frame face");

        /* TITLE (beads initech-lxg9): the window name is drawn CENTERED in Chicago
         * over a knocked-out light gap. Live-screendump tripwire: assert FIGURE ink
         * (black, idx 4 = CIDX_TITLE_INK) is present in the centered title region --
         * a blank title bar is a regression. (The full title geometry/knockout is
         * graded host-side by test-chrome-fidelity.) */
        {
            int title_ink = 0;
            /* Scan the 12-row stripe field where the centered title is drawn over
             * the frame-face gap. sys8/window-chrome.md Sec 2.3. */
            for (int y = stripe_top; y < stripe_bot; y++) {
                for (int x = mid_x - 24; x < mid_x + 24; x++) {
                    if (is_rgb(x, y, IDX(CIDX_BLACK))) title_ink++;
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
            printf("    (c) active title ink (black) in centered region: %d px\n",
                   title_ink);
        }

        /* Row T+22 is content, proving the title profile stops after T+21. */
        assert_idx(mid_x, content_top, CIDX_WHITE,
                   "(c) front window first content row below the title band is white body (title height exact)");
        assert_idx(body_x, body_row, CIDX_WHITE,
                   "(c) front window body fill is window white");

        /* Platinum right body edge, inside -> outside: inner black, white,
         * face, face, shadow, outer black. The active 16px gutter replaces the
         * adjacent content-inset pixel: its five-value cross-section ends in
         * sampled #DADADA at the inner separator. DEC-10 Sec 4;
         * sys8/window-chrome.md Sec 4; sys8/scrollbars.md Sec 2.3.
         *
         * CalcDoc now includes the exact notched shadow L in strucRgn while the
         * drawer receives WindowFrameRect, so the live clip owns x=W1_R and the
         * next column remains desktop. Ref: sys8/window-chrome.md Sec 1;
         * bead initech-9d0e. */
        assert_idx(inner_line - 1, body_row, CIDX_PLAT_FRAME_FACE,
                   "(c) enabled scrollbar well meets separator with sampled DA highlight");
        assert_idx(inner_line, body_row, CIDX_BLACK,
                   "(c) right body inner line is black");
        assert_idx(inner_line + 1, body_row, CIDX_WHITE,
                   "(c) right raised body bar starts with white");
        assert_idx(inner_line + 2, body_row, CIDX_PLAT_FRAME_FACE,
                   "(c) right raised body bar face row 1");
        assert_idx(inner_line + 3, body_row, CIDX_PLAT_FRAME_FACE,
                   "(c) right raised body bar face row 2");
        assert_idx(inner_line + 4, body_row, CIDX_PLAT_FRAME_SHADOW,
                   "(c) right raised body bar ends with frame shadow");
        assert_idx(W1_R - 1, body_row, CIDX_BLACK,
                   "(c) right body outer line is black");
        assert_idx(W1_R, body_row, CIDX_BLACK,
                   "(c) live drop shadow occupies the column just right of the frame");
        assert_idx(W1_R + 1, body_row, CIDX_DESKTOP,
                   "(c) pixel just right of the one-pixel shadow is bare teal");
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

        /* DEC-10 Sec 4 requires movableDBoxProc to use the same Platinum band.
         * x=DL+5 is clear of its centred title. Measured in the supplied dump
         * before encoding; sys8/window-chrome.md Sec 2.1/2.2. */
        if (!platinum_title_profile(DL + 5, DT, "(d) FILE COPY modal")) {
            printf("    (d) modal Platinum title profile: 22 rows, 12 strict "
                   "white/#%06X stripes, light-first/dark-last\n",
                   IDX(CIDX_PLAT_STRIPE_DARK));
        }
        assert_idx((DL + DR) / 2, DT + TITLE_STRIPE_TOP_OFF,
                   CIDX_PLAT_FRAME_FACE,
                   "(d) modal active-title gap fill is Platinum frame face");

        /* The frame is PLAIN 1-px, NOT the old 7px dBoxProc border: content
         * just inside the left/right edges, at a row below the 22px title
         * band, is white; the outermost columns are still black. */
        assert_idx(DL + FRAME, cy, 1,
                   "(d) modal content just inside the LEFT frame is white "
                   "(1px frame, not 7px border; initech-zvo6)");
        assert_idx(DR - FRAME - 1, cy, 1,
                   "(d) modal content just inside the RIGHT frame is white (initech-zvo6)");
        assert_idx(DL, cy, 0, "(d) modal outer LEFT frame column is black");
        assert_idx(DR - 1, cy, 0, "(d) modal outer RIGHT frame column is black");

        /* The "Saving tables to disk..." static text is RENDERED as ink: the text
         * rect (left=154, top=225 -- below the 22px title band, was top=212
         * under the old 7px-border layout) contains a non-white pixel. */
        {
            int tx0 = 154, ty0 = 225;
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

        /* The progress bar (left=154, top=249, right=486, bottom=269 -- re-based
         * below the title band, was top=236/bottom=256): left border black, a
         * NON-ZERO solid-blue fill (idx5, CTRL_ACCENT navy) at the canon value
         * FILECOPY_CANON_PROGRESS (68), and white for the unfilled remainder.
         * Beads initech-a90f -- the old bug rendered value=0 (zero fill). */
        assert_idx(154, 259, 0, "(d) progress bar left border is painted (black)");
        assert_idx(160, 259, 5,
                   "(d) progress bar FILL (x=160,y=259) is idx5 CTRL_ACCENT navy "
                   "-- a non-zero fill (initech-a90f)");
        assert_idx(400, 259, 1,
                   "(d) progress bar unfilled remainder (x=400,y=259) is white");

        /* Z-ORDER: the modal occludes the windows. Probe (140,240) lies BOTH
         * ON the modal's plain 1-px LEFT frame column AND inside window 0
         * (x in [60,360), y in [80,300)). Correct z-order -> modal BLACK frame. */
        int in_modal_frame = (140 >= DL && 140 < DL + FRAME && 240 >= DT && 240 < DB);
        int in_window0      = (140 >= W0_L && 140 < W0_R && 240 >= W0_T && 240 < W0_B);
        if (!(in_modal_frame && in_window0)) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (d) z-order probe (140,240) is not over "
                    "BOTH the modal frame and window 0 (test would be vacuous)\n");
            g_fail = 1;
        }
        assert_idx(140, 240, 0,
                   "(d) modal OCCLUDES the window behind it -- modal black frame on top (z-order)");
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
