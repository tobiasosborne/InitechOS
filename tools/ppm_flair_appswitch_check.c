/*
 * ppm_flair_appswitch_check.c -- the O-5 EMU app-switch oracle's screendump
 * grader (HOST, C-only). beads ADR-0013 (FLAIR App Contract); Wave-4 gate O-5.
 *
 * It takes the PRE-switch and POST-switch screendumps of the booted
 * -DFLAIR_LIVE_TENANTS desktop (two co-resident reference tenants: NOTES,
 * launched first, and HELLO, launched LAST so HELLO is the boot FOREGROUND and
 * PARTIALLY OCCLUDES NOTES -- bead initech-4w15: HELLO is the Photoshop-menu app,
 * so with HELLO foreground the resting scene is the distinct chimera: band 1
 * System-7 + band 2 Photoshop).  The locked FLAIR_APPSWITCH_SPEC trace clicks
 * NOTES's visible sliver (the background tenant).  That single click must:
 *   - raise the NOTES group to the front (co-residency + group-raise),
 *   - repaint the newly-exposed overlap region via the updateEvt route, and
 *   - fire the activate/deactivate pair so NOTES becomes the active tenant and
 *     HELLO deactivates.
 * The PRE dump is grabbed before the click; the POST dump after the
 * FLAIR-DISPATCH app=NOTES marker.  This grader judges the PRE->POST delta.
 *
 * SIX DIFFERENTIALS, each independently catching a distinct regression:
 *
 *   DISTINCT-CHIMERA -- the BOOT (resting) scene shows two DIFFERENT stacked bars
 *     (INTRA-scene structural; beads initech-4w15, Law 4).  The iconic Office
 *     Space image is band 1 = System-7 ("[Apple] File Edit View Special") over
 *     band 2 = Photoshop ("File Edit Image Layer Select View Window Help").  On
 *     the PRE (resting) scene alone, band 1's title strip and band 2's title strip
 *     must DIFFER.  This is the leg that catches the two-identical-bars regression
 *     (a boot that draws the same menu into both bands -- exactly the Law-4 bug an
 *     earlier fix attempt introduced by drawing the boot-foreground's System-7
 *     menu into band 2 when NOTES was the boot foreground).
 *
 *   POST-DISTINCT -- after switch-to-NOTES the two stacked bars still DIFFER
 *     (POST-only intra-scene structural; bead initech-7tjp, Law 4). NOTES owns a
 *     no-Apple SimpleText-flavored bar, so band 2 must not duplicate the shell's
 *     System-7 bar or its Apple slot. KMAIN_MUT_NOTES_BAR_SYS restores the
 *     original shared bar_sys assignment, making both POST title strips pixel-
 *     identical -> RED.
 *
 *   TIER-A -- CO-RESIDENCY + GROUP-RAISE + updateEvt REPAINT (structural).
 *     The OVERLAP probe (FLAIR_TEN_PROBE_OVERLAP_X/Y, inside NOTES content but
 *     under HELLO pre-switch) reads HELLO_FILL in the PRE dump (HELLO on top) and
 *     NOTES_FILL in the POST dump (NOTES raised to front AND its newly-exposed
 *     overlap content repainted).  This ONE differential proves all three: the
 *     two tenants co-exist, the clicked group rose to front, and the exposed
 *     region was repainted by the updateEvt route.  The drop-updateEvt / no-raise
 *     / ignore-refCon mutants all leave HELLO_FILL here -> RED.  (ADR-0006 E-D5
 *     Tier-A damage-law kind: a STRUCTURAL differential, not a single-scene read.)
 *
 *   TIER-B -- ACTIVATION reached the tenant (independent VALUE).
 *     The NOTES ACTIVE-ACCENT probe (a FLAIR_TEN_ACCENT_SIZE block at NOTES's
 *     content top-left, FLAIR_TEN_NOTES_ACCENT_X/Y) reads HELLO_FILL in the PRE
 *     dump (that block is UNDER the foreground HELLO) and FLAIR_TEN_ACTIVE_ACCENT
 *     in the POST dump (NOTES raised + became active: the tenant painted its accent
 *     block in response to activateEvt active=1).  The skip-activate-pair mutant
 *     leaves it NOTES_FILL -> RED. NOTE (per the demo contract): this activation
 *     observable remains the TENANT CONTENT ACCENT, independently of the now-real
 *     Platinum active/inactive title delta. The latter is graded by
 *     ppm_flair_solid_check leg C. DEC-10 Sec 4 + sys8/window-chrome.md Sec 6.
 *     (ADR-0006 E-D5 Tier-B independent golden kind: graded against the
 *     canon VALUE the demo header names, by recomputation from the independent
 *     canon, never by-construction.)
 *
 *   BAR1-STATIC -- the TOP System-7 shell bar survives the switch (PRE-vs-POST
 *     DIFFERENTIAL; beads initech-4w15).  bar_sys (rows [0,FLAIR_MENUBAR_H)) is
 *     SHELL-OWNED: shell_render draws it ONCE when the scene is built and the
 *     live app-switch path must NEVER repaint it.  We grade this as a
 *     DIFFERENTIAL of the bar-1 title-strip interior: it must be BYTE-IDENTICAL
 *     pre-vs-post (0 differing pixels beyond capture tolerance).  The Apple slot +
 *     System-7 titles must survive the switch.  (See the trailing DEVIATIONS note
 *     re: the row-0 mistarget bug is caught by MENU-BAND in this demo.)
 *
 *   MENU-BAND swap -- the foreground app's menubar swapped in (PRE-vs-POST
 *     DIFFERENTIAL).  When NOTES becomes foreground, its own menu replaces
 *     HELLO's Photoshop menu in the SECOND (Photoshop-chimera) menu-bar band (rows
 *     [FLAIR_MENUBAR_H, 2*FLAIR_MENUBAR_H) -- shell.h SHELL_MENUBAR2_TOP), NOT the
 *     top bar (see BAR1-STATIC above).  We grade this as a DIFFERENTIAL of the
 *     bar-2 menu-bar title strip: the band's title region must DIFFER pre-vs-post
 *     (Photoshop -> NOTES).  The menubar-no-swap mutant AND the initech-4w15
 *     row-0-mistarget bug both leave band 2 byte-identical -> 0 differing pixels
 *     -> RED (the row-0 bug draws the swap into band 1 instead of band 2, so band 2
 *     never changes).  We keep it a differential and NEVER a palette read of one
 *     scene (a static band cannot pass).  See the BAND_* probe note + the
 *     deviation callout in the trailing comment re: exact glyph columns.
 *
 * INDEPENDENT GOLDEN (Law 2; ADR-0006 E-D5; ADR-0010 / HER-02 / HER-11): every
 * expected color is flair_canon_rgb(CIDX_*) from spec/assets/color_canon.h (the
 * canon the INDEPENDENT test-color-canon vouches for) -- NEVER the renderer's
 * flair_palette_rgb, NEVER preview.webp.  The grader is told WHERE to look
 * (geometry from spec/flair_tenants_demo.h) and the canon VALUE to expect; it
 * never reads the renderer's palette, so it cannot agree by construction.  The
 * load-bearing assertions are PRE->POST differentials that flip under the
 * app-switch mutants.
 *
 * Usage: ppm_flair_appswitch_check <pre.ppm> <post.ppm>
 * Exit 0 = PASS; non-zero = a named FAIL (the assertion + sampled-vs-expected RGB).
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11): fixed probe coords from the
 * shared demo layout contract (spec/flair_tenants_demo.h).
 */
#include <stdio.h>
#include <stdlib.h>

#include "flair_tenants_demo.h"  /* -Ispec: the shared demo layout (WHERE to
                                  * probe) + canon indices; it pulls in
                                  * assets/color_canon.h (flair_canon_rgb). */
#include "chrome_metrics.h"      /* -Ispec: FLAIR_CHROME_MENUBAR_H -- the SAME
                                  * locked geometry os/flair/shell.h derives
                                  * SHELL_MENUBAR1_TOP/SHELL_MENUBAR2_TOP from,
                                  * so a fix that mistargets a different row
                                  * range is caught, not "matched" by a new
                                  * grader-local magic number (initech-4w15). */

/* Per-channel tolerance: capture-noise only (the XRGB8888 -> P6 dump is exact;
 * canon entries differ by far more than 2/channel), mirroring the drag grader. */
#define TOL 2

/* ---- the TWO stacked menu-bar TITLE STRIP probes (the bar1-static leg +
 * the bar2 band-swap leg; beads initech-4w15). ----
 * The demo is 640x480 with TWO FLAIR_CHROME_MENUBAR_H-tall bars stacked (shell.h
 * SHELL_MENUBAR1_TOP=0 / SHELL_MENUBAR2_TOP=FLAIR_MENUBAR_H): bar 1 (the TOP,
 * SHELL-OWNED, STATIC System-7 bar: Apple slot + File/Edit/View/Special) at
 * y[0,FLAIR_CHROME_MENUBAR_H), bar 2 (the Photoshop-chimera band that swaps to
 * the foreground tenant's own menu) at y[FLAIR_CHROME_MENUBAR_H,
 * 2*FLAIR_CHROME_MENUBAR_H); content begins at y>=40.  For EACH bar we scan its
 * title-strip INTERIOR (avoid the 1px bar edges/baseline) to the right of the
 * Apple menu (titles begin at x=20, cf. ppm_flair_menu_check's MenuBar_title_x)
 * and count pixels that DIFFER pre-vs-post.  Both bands are strictly y<40, well
 * clear of the content probes (y>=88), so a content change cannot leak into
 * either leg; only a menu-bar repaint can move them.
 *
 * DEVIATION (documented; see trailing note): the demo header does not name
 * menu-bar TITLE-COLUMN geometry (X0/X1 stay grader-local), though the Y ranges
 * are now grounded in the SAME spec/chrome_metrics.h constant the shell uses.
 * We assert the coarse DIFFERENTIAL (title strip differs / is identical)
 * rather than exact glyph columns, which would be brittle against the
 * hand-made proof pair; Step 4/5 may tighten to specific columns once the real
 * booted image exists.  BAND_MIN_DIFFS is set above capture noise but far
 * below a real glyph-strip swap. */
#define BAND1_Y0         (0 + 4)                              /* bar-1 interior top    */
#define BAND1_Y1         (0 + FLAIR_CHROME_MENUBAR_H - 4)      /* bar-1 interior bottom */
#define BAND2_Y0         (FLAIR_CHROME_MENUBAR_H + 4)          /* bar-2 interior top    */
#define BAND2_Y1         (FLAIR_CHROME_MENUBAR_H + FLAIR_CHROME_MENUBAR_H - 4) /* bar-2 bottom */
#define BAND_X0          24    /* right of the Apple menu (titles start at x=20) */
#define BAND_X1          200   /* across the foreground app's menu-title strip   */
#define BAND_MIN_DIFFS   8     /* above capture noise, below a real title swap   */

/* ---- the NOTES active-accent block CENTRE (sampled inside the 12x12 block). --
 * bead initech-4w15: the O-5 switch now activates NOTES (HELLO is the boot
 * foreground), so TIER-B probes NOTES's accent (its content top-left, exposed
 * only when NOTES is raised + active), not HELLO's. */
#define ACC_CX (FLAIR_TEN_NOTES_ACCENT_X + FLAIR_TEN_ACCENT_SIZE / 2)
#define ACC_CY (FLAIR_TEN_NOTES_ACCENT_Y + FLAIR_TEN_ACCENT_SIZE / 2)

/* ---- PPM P6 reader (the ppm_flair_drag_check / ppm_flair_menu_check idiom). -- */
typedef struct {
    unsigned char *buf;
    long w, h;
} Img;

static int read_uint(FILE *f, long *out)
{
    int c;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); continue; }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    long v = 0;
    while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = fgetc(f); }
    *out = v;
    return 0;
}

/* Load a >=640x480 P6 PPM into im. Returns 0 on success, 2 on any error
 * (mirrors the drag grader's usage-error exit code). */
static int read_ppm(const char *path, Img *im)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "ppm_flair_appswitch_check: cannot open %s\n", path); return 2; }
    int c0 = fgetc(f), c1 = fgetc(f);
    if (c0 != 'P' || c1 != '6') {
        fprintf(stderr, "ppm_flair_appswitch_check: %s is not a P6 PPM\n", path);
        fclose(f); return 2;
    }
    long maxv;
    if (read_uint(f, &im->w) || read_uint(f, &im->h) || read_uint(f, &maxv)) {
        fprintf(stderr, "ppm_flair_appswitch_check: bad PPM header in %s\n", path);
        fclose(f); return 2;
    }
    if (im->w < 640 || im->h < 480) {
        fprintf(stderr, "ppm_flair_appswitch_check: %s is %ldx%ld < 640x480\n",
                path, im->w, im->h);
        fclose(f); return 2;
    }
    size_t n = (size_t)im->w * (size_t)im->h * 3;
    im->buf = (unsigned char *)malloc(n);
    if (!im->buf) { fprintf(stderr, "ppm_flair_appswitch_check: OOM\n"); fclose(f); return 2; }
    if (fread(im->buf, 1, n, f) != n) {
        fprintf(stderr, "ppm_flair_appswitch_check: short read on %s\n", path);
        free(im->buf); im->buf = NULL; fclose(f); return 2;
    }
    fclose(f);
    return 0;
}

static const unsigned char *px(const Img *im, int x, int y)
{
    return im->buf + ((long)y * im->w + x) * 3;
}

static unsigned int IDX(int i)
{
    return (unsigned int)(flair_canon_rgb((unsigned char)i) & 0x00FFFFFFu);
}

static int is_rgb(const Img *im, int x, int y, unsigned int rgb)
{
    const unsigned char *p = px(im, x, y);
    int r = (int)((rgb >> 16) & 0xFFu);
    int g = (int)((rgb >> 8) & 0xFFu);
    int b = (int)(rgb & 0xFFu);
    return abs((int)p[0] - r) <= TOL &&
           abs((int)p[1] - g) <= TOL &&
           abs((int)p[2] - b) <= TOL;
}

static int g_fail = 0;

/* Assert pixel (x,y) of `scene` (im) is canon index `idx`. Records a fail +
 * prints sampled-vs-expected RGB on mismatch (the drag-grader idiom). */
static void assert_idx(const Img *im, const char *scene, int x, int y,
                       int idx, const char *what)
{
    if (!is_rgb(im, x, y, IDX(idx))) {
        const unsigned char *p = px(im, x, y);
        unsigned int e = IDX(idx);
        fprintf(stderr,
                "ppm_flair_appswitch_check: FAIL %s [%s]\n"
                "    at (%d,%d): sampled RGB(%d,%d,%d)=#%02X%02X%02X, "
                "expected canon idx %d = #%06X (tol +/-%d)\n",
                what, scene, x, y, p[0], p[1], p[2], p[0], p[1], p[2], idx, e, TOL);
        g_fail = 1;
    }
}

/* Count pixels in the menu-bar title strip x[BAND_X0,BAND_X1) y[y0,y1) that
 * differ (beyond TOL) pre-vs-post. A pure DIFFERENTIAL: no canon VALUE is read
 * here, only the PRE<->POST delta. Parametrized by y0/y1 so the SAME routine
 * grades both bar-1 (expect 0 diffs -- static) and bar-2 (expect >=
 * BAND_MIN_DIFFS -- swapped) (initech-4w15). */
static int band_diff_count(const Img *pre, const Img *post, int y0, int y1)
{
    int diffs = 0;
    for (int y = y0; y < y1; y++) {
        for (int x = BAND_X0; x < BAND_X1; x++) {
            const unsigned char *a = px(pre, x, y);
            const unsigned char *b = px(post, x, y);
            if (abs((int)a[0] - (int)b[0]) > TOL ||
                abs((int)a[1] - (int)b[1]) > TOL ||
                abs((int)a[2] - (int)b[2]) > TOL) {
                diffs++;
            }
        }
    }
    return diffs;
}

/* Count pixels that differ (beyond TOL) between TWO vertically-offset title
 * strips of the SAME image `im`: strip A at rows [y0a, y0a+h) vs strip B at rows
 * [y0b, y0b+h), aligned row-by-row, over x[BAND_X0, BAND_X1). Used by the
 * DISTINCT-CHIMERA leg to assert the boot scene's band 1 (System-7) and band 2
 * (Photoshop) are visibly DIFFERENT bars (Law 4; initech-4w15). A pure intra-
 * scene structural differential -- no canon VALUE is read. */
static int intra_band_diff(const Img *im, int y0a, int y0b, int h)
{
    int diffs = 0;
    for (int k = 0; k < h; k++) {
        for (int x = BAND_X0; x < BAND_X1; x++) {
            const unsigned char *a = px(im, x, y0a + k);
            const unsigned char *b = px(im, x, y0b + k);
            if (abs((int)a[0] - (int)b[0]) > TOL ||
                abs((int)a[1] - (int)b[1]) > TOL ||
                abs((int)a[2] - (int)b[2]) > TOL) {
                diffs++;
            }
        }
    }
    return diffs;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <pre.ppm> <post.ppm>\n", argv[0]);
        return 2;
    }

    Img pre = {0}, post = {0};
    if (read_ppm(argv[1], &pre)) return 2;
    if (read_ppm(argv[2], &post)) { free(pre.buf); return 2; }
    if (pre.w != post.w || pre.h != post.h) {
        fprintf(stderr, "ppm_flair_appswitch_check: PRE %ldx%ld != POST %ldx%ld\n",
                pre.w, pre.h, post.w, post.h);
        free(pre.buf); free(post.buf); return 2;
    }

    printf("ppm_flair_appswitch_check: grading the app-switch PRE->POST delta "
           "(click NOTES sliver @(%d,%d) -> raise+activate NOTES over HELLO)\n",
           FLAIR_TEN_NOTES_CLICK_X, FLAIR_TEN_NOTES_CLICK_Y);

    /* ---- TIER-A: co-residency + group-raise + updateEvt repaint ------------- */
    /* bead initech-4w15: HELLO is the boot foreground, so the overlap flips
     * HELLO_FILL (pre) -> NOTES_FILL (post) as NOTES is raised over HELLO. */
    /* PRE: HELLO is on top -> the overlap probe reads HELLO_FILL. */
    assert_idx(&pre, "PRE", FLAIR_TEN_PROBE_OVERLAP_X, FLAIR_TEN_PROBE_OVERLAP_Y,
               FLAIR_TEN_HELLO_FILL,
               "TIER-A: PRE overlap probe is HELLO_FILL (HELLO on top, NOTES covered)");
    /* POST: NOTES raised + the exposed overlap repainted -> reads NOTES_FILL. */
    assert_idx(&post, "POST", FLAIR_TEN_PROBE_OVERLAP_X, FLAIR_TEN_PROBE_OVERLAP_Y,
               FLAIR_TEN_NOTES_FILL,
               "TIER-A: POST overlap probe is NOTES_FILL (NOTES raised + updateEvt-repainted)");
    if (!g_fail) {
        printf("    TIER-A: overlap HELLO_FILL->NOTES_FILL -- co-residency + "
               "group-raise + updateEvt repaint of the exposed region\n");
    }

    /* ---- TIER-B: activation reached the tenant (content accent VALUE) ------- */
    /* bead initech-4w15: TIER-B probes NOTES's accent (its content top-left).
     * PRE that block is UNDER HELLO -> reads HELLO_FILL; POST NOTES is raised +
     * active -> the tenant painted FLAIR_TEN_ACTIVE_ACCENT there. */
    /* PRE: NOTES accent spot covered by HELLO -> reads HELLO_FILL (no accent). */
    assert_idx(&pre, "PRE", ACC_CX, ACC_CY, FLAIR_TEN_HELLO_FILL,
               "TIER-B: PRE NOTES accent block is HELLO_FILL (NOTES inactive + covered by HELLO)");
    /* POST: NOTES active -> the tenant painted FLAIR_TEN_ACTIVE_ACCENT. Sample
     * three interior points so a lucky single pixel cannot pass the block. */
    assert_idx(&post, "POST", ACC_CX, ACC_CY, FLAIR_TEN_ACTIVE_ACCENT,
               "TIER-B: POST NOTES accent block (centre) is the active accent");
    assert_idx(&post, "POST", FLAIR_TEN_NOTES_ACCENT_X + 2,
               FLAIR_TEN_NOTES_ACCENT_Y + 2, FLAIR_TEN_ACTIVE_ACCENT,
               "TIER-B: POST NOTES accent block (top-left interior) is the active accent");
    assert_idx(&post, "POST", FLAIR_TEN_NOTES_ACCENT_X + FLAIR_TEN_ACCENT_SIZE - 3,
               FLAIR_TEN_NOTES_ACCENT_Y + FLAIR_TEN_ACCENT_SIZE - 3,
               FLAIR_TEN_ACTIVE_ACCENT,
               "TIER-B: POST NOTES accent block (bottom-right interior) is the active accent");
    if (!g_fail) {
        printf("    TIER-B: NOTES accent FILL->ACTIVE_ACCENT -- the activate/"
               "deactivate pair fired and reached the tenant (active=1)\n");
    }

    /* ---- DISTINCT-CHIMERA: the BOOT scene shows two DIFFERENT stacked bars --- *
     * Ref: bead initech-4w15; Law 4 (the Office Space chimera is the RESTING look:
     * band 1 = System-7 "[Apple] File Edit View Special", band 2 = Photoshop
     * "File Edit Image Layer Select View Window Help"). This is the leg that would
     * catch the two-identical-bars regression (a boot that draws the same menu into
     * both bands). Graded on the PRE (resting) scene ONLY: band 1's title strip and
     * band 2's title strip must DIFFER by >= BAND_MIN_DIFFS px. (Not a canon read;
     * a pure intra-scene structural differential -- ADR-0010.) With HELLO the boot
     * foreground, band 2 == HELLO's Photoshop bar != band 1's System-7 bar. */
    {
        int h = BAND1_Y1 - BAND1_Y0;   /* == BAND2_Y1 - BAND2_Y0 (equal strips) */
        int diffs = intra_band_diff(&pre, BAND1_Y0, BAND2_Y0, h);
        if (diffs < BAND_MIN_DIFFS) {
            fprintf(stderr,
                    "ppm_flair_appswitch_check: FAIL DISTINCT-CHIMERA -- at BOOT the "
                    "two stacked menu bars are IDENTICAL (band 1 y[%d,%d) vs band 2 "
                    "y[%d,%d), x[%d,%d): only %d differing px < %d): the resting scene "
                    "is NOT the distinct System-7 + Photoshop chimera (Law-4 "
                    "regression; initech-4w15)\n",
                    BAND1_Y0, BAND1_Y1, BAND2_Y0, BAND2_Y1, BAND_X0, BAND_X1,
                    diffs, BAND_MIN_DIFFS);
            g_fail = 1;
        } else {
            printf("    DISTINCT-CHIMERA: at boot band 1 (System-7) and band 2 "
                   "(Photoshop) DIFFER (%d px) -- the resting scene is the distinct "
                   "Office Space chimera (Law 4; initech-4w15)\n", diffs);
        }
    }

    /* ---- POST-DISTINCT: NOTES leaves the stacked bars visibly DIFFERENT ----- *
     * Ref: bead initech-7tjp; PRD Sec 1.1 / Sec 3 / Sec 6.3 and Law 4. After
     * NOTES becomes foreground, band 2 must be NOTES's own no-Apple SimpleText-
     * flavored bar, not the shell-owned System-7 bar_sys already present in band
     * 1. Grade the POST scene alone with the same structural differential as
     * DISTINCT-CHIMERA: the aligned title strips must differ by at least
     * BAND_MIN_DIFFS pixels. The KMAIN_MUT_NOTES_BAR_SYS mutant restores the
     * original shared-object assignment, making both POST strips pixel-identical
     * and proving this leg RED (Rule 6). */
    {
        int h = BAND1_Y1 - BAND1_Y0;   /* == BAND2_Y1 - BAND2_Y0 (equal strips) */
        int diffs = intra_band_diff(&post, BAND1_Y0, BAND2_Y0, h);
        if (diffs < BAND_MIN_DIFFS) {
            fprintf(stderr,
                    "ppm_flair_appswitch_check: FAIL POST-DISTINCT -- after "
                    "switch-to-NOTES the two stacked menu bars are IDENTICAL "
                    "(band 1 y[%d,%d) vs band 2 y[%d,%d), x[%d,%d): only %d "
                    "differing px < %d): NOTES reused the shell-owned System-7 "
                    "bar_sys / Apple slot instead of its distinct no-Apple bar "
                    "(Law-4 regression; initech-7tjp)\n",
                    BAND1_Y0, BAND1_Y1, BAND2_Y0, BAND2_Y1, BAND_X0, BAND_X1,
                    diffs, BAND_MIN_DIFFS);
            g_fail = 1;
        } else {
            printf("    POST-DISTINCT: after switch-to-NOTES band 1 (System-7) "
                   "and band 2 (NOTES, no Apple slot) DIFFER (%d px) -- NOTES "
                   "owns a distinct menu bar (Law 4; initech-7tjp)\n", diffs);
        }
    }

    /* ---- BAR1-STATIC: the TOP System-7 shell bar survives the switch -------- *
     * Ref: bead initech-4w15; shell.h SHELL_MENUBAR1_TOP static-bar invariant;
     * Law 4 (the Office Space two-bar chimera must survive an app-switch: the
     * Apple slot + System-7 titles never vanish). bar_sys is SHELL-OWNED and
     * drawn ONCE by shell_render; the live app-switch path must NEVER repaint
     * it. Assert the bar-1 title-strip interior is BYTE-IDENTICAL pre-vs-post
     * (0 differing pixels beyond capture tolerance). This is the tier the
     * initech-4w15 bug fails: a whole-bitmap GrafPort landed the foreground-
     * tenant's Photoshop-style menu on row 0, clobbering bar_sys. */
    {
        int diffs = band_diff_count(&pre, &post, BAND1_Y0, BAND1_Y1);
        if (diffs != 0) {
            fprintf(stderr,
                    "ppm_flair_appswitch_check: FAIL BAR1-STATIC -- the TOP "
                    "System-7 shell bar x[%d,%d) y[%d,%d) CHANGED pre-vs-post "
                    "(%d differing px, expected 0): the live app-switch clobbered "
                    "the shell-owned static bar-1 band (initech-4w15) -- the Apple "
                    "slot / System-7 titles did NOT survive the switch\n",
                    BAND_X0, BAND_X1, BAND1_Y0, BAND1_Y1, diffs);
            g_fail = 1;
        } else {
            printf("    BAR1-STATIC: the top System-7 bar x[%d,%d) y[%d,%d) is "
                   "BYTE-IDENTICAL pre-vs-post -- the Apple slot + System-7 "
                   "titles survive the app-switch (bar-1 is shell-owned + "
                   "static; initech-4w15)\n",
                   BAND_X0, BAND_X1, BAND1_Y0, BAND1_Y1);
        }
    }

    /* ---- MENU-BAND swap: the foreground menubar title strip differs --------- *
     * The SECOND (Photoshop-chimera) bar, NOT the top bar (see BAR1-STATIC). */
    {
        int diffs = band_diff_count(&pre, &post, BAND2_Y0, BAND2_Y1);
        if (diffs < BAND_MIN_DIFFS) {
            fprintf(stderr,
                    "ppm_flair_appswitch_check: FAIL MENU-BAND -- the foreground "
                    "menubar title strip x[%d,%d) y[%d,%d) (bar 2) is UNCHANGED "
                    "pre-vs-post (%d differing px < %d): the menubar did NOT swap "
                    "to NOTES (Photoshop -> NOTES)\n",
                    BAND_X0, BAND_X1, BAND2_Y0, BAND2_Y1, diffs, BAND_MIN_DIFFS);
            g_fail = 1;
        } else {
            printf("    MENU-BAND: the menu-bar title strip x[%d,%d) y[%d,%d) "
                   "(bar 2) DIFFERS pre-vs-post (%d px) -- NOTES's menubar "
                   "swapped in (Photoshop -> NOTES)\n",
                   BAND_X0, BAND_X1, BAND2_Y0, BAND2_Y1, diffs);
        }
    }

    /* ---- TIER-C: the RAISED window's own structure frame survives the switch
     * (beads initech-jmc5 / initech-qi8v; the oracle-gap fix). ROOT CAUSE (pinned
     * by a live re-test): kmain hides the two canon frame doc windows BEFORE
     * launching the tenants, seeding wm->desktop_update with their footprint
     * (fd_win_bounds) via distribute_exposure. The tenants' init recomposite
     * (shell_render -> desktop_paint_all) paints the whole desktop correctly from
     * scratch, but -- pre-fix -- desktop_paint_all never cleared wm->desktop_update,
     * so that stale footprint survived into the pump. The FIRST desktop_paint_damage
     * after the switch teal-stomps whatever window chrome geometrically falls inside
     * it, WITHOUT repainting it (the window loop only repaints windows whose
     * updateRgn is non-empty). The RAISED window (NOTES here: SelectWindow's 0->1
     * transition) is NOT invalidated by window.c reaffirm_active -- only the 1->0
     * DEACTIVATE transition is (initech-v6t2). So on THIS integrated scene the
     * ERASED window is NOTES (raised), not HELLO (deactivated -> v6t2 repaints it):
     * empirically HELLO's frame stays CIDX_BLACK in both builds, while NOTES's
     * right + bottom structure frame reads CIDX_DESKTOP teal on the mutant and
     * CIDX_BLACK on the fixed build. We probe NOTES's right edge (x=NOTES_R-1) and
     * bottom edge (y=NOTES_B-1) -- both inside fd_win_bounds -- POST-switch only (a
     * structural invariant: the frame was never supposed to move). MUTATION-PROOF
     * MOVED HOST-SIDE (2026-07-31 Wave A): the DESKTOP_MUTATE_NO_PAINTALL_CLEAR
     * emu image used to flip this RED, but the DQ3 activation seed (initech-rqz5)
     * now repaints the raised window's full chrome in the same dispatch, healing
     * the stomp in the 2-tenant scene -- the knob is proven by test_drag.c leg (e)
     * (test-drag-mutant) instead, and this tier remains a live invariant check.
     * This is the leg that catches the erasure jmc5/qi8v exist for -- TIER-A/B and
     * MENU-BAND probe CONTENT + the menu band, never the raised window's own frame,
     * which is exactly the pixel range the stale desktop_update corrupts. Fix:
     * desktop.c desktop_paint_all resets wm->desktop_update at its tail. Coords are
     * grader-local (like BAND_/ACC_), kept >=10px off the corners for +/-1px chrome
     * drift, and clear of the HELLO/NOTES overlap. Under Platinum an ACTIVE
     * structure outline is still black; inactive outlines are idx119. Thus this
     * post-switch raised-NOTES probe is re-attributed, not value-coincident:
     * DEC-10 Sec 4 + sys8/window-chrome.md Sec 6. Grade vs the INDEPENDENT canon
     * (Law 2; CIDX_BLACK is the active structure-frame role). ------------------ */
    assert_idx(&post, "POST", FLAIR_TEN_NOTES_R - 1, 200, CIDX_BLACK,
               "TIER-C: POST NOTES right frame edge (upper) is CIDX_BLACK, not "
               "desktop teal (initech-jmc5/-qi8v stale desktop_update erasure)");
    assert_idx(&post, "POST", FLAIR_TEN_NOTES_R - 1, 300, CIDX_BLACK,
               "TIER-C: POST NOTES right frame edge (lower) is CIDX_BLACK, not "
               "desktop teal (initech-jmc5/-qi8v stale desktop_update erasure)");
    assert_idx(&post, "POST", 400, FLAIR_TEN_NOTES_B - 1, CIDX_BLACK,
               "TIER-C: POST NOTES bottom frame edge (left) is CIDX_BLACK, not "
               "desktop teal (initech-jmc5/-qi8v stale desktop_update erasure)");
    assert_idx(&post, "POST", 500, FLAIR_TEN_NOTES_B - 1, CIDX_BLACK,
               "TIER-C: POST NOTES bottom frame edge (right) is CIDX_BLACK, not "
               "desktop teal (initech-jmc5/-qi8v stale desktop_update erasure)");
    if (!g_fail) {
        printf("    TIER-C: the raised window's (NOTES) own structure frame (right "
               "+ bottom edges, inside fd_win_bounds) is CIDX_BLACK post-switch, "
               "not desktop teal -- no stale desktop_update erasure "
               "(initech-jmc5/-qi8v)\n");
    }

    free(pre.buf);
    free(post.buf);

    if (g_fail) {
        fprintf(stderr, "ppm_flair_appswitch_check: FAIL -- the click did not "
                "raise+activate the background tenant + swap its menubar without "
                "clobbering or duplicating the static top bar (the FLAIR App "
                "Contract app-switch is not actually wired correctly)\n");
        return 1;
    }
    printf("ppm_flair_appswitch_check: PASS -- the boot scene is the DISTINCT "
           "chimera (band1 System-7 != band2 Photoshop); clicking NOTES's sliver "
           "raised the NOTES group (overlap HELLO_FILL->NOTES_FILL + updateEvt "
           "repaint), activated it (accent FILL->ACTIVE_ACCENT), left the static "
           "top System-7 bar untouched (BAR1-STATIC), and swapped band 2 to NOTES's "
           "distinct no-Apple menu (Photoshop -> NOTES; POST-DISTINCT); the booted "
           "desktop honours the O-5 app-switch contract (ADR-0013) with distinct "
           "stacked bars both before and after the switch and no initech-4w15 / "
           "initech-7tjp regression\n");
    return 0;
}

/*
 * DEVIATIONS / RISKS (Law 2 honesty):
 *  - DISTINCT-CHIMERA / MENU-BAND / BAR1-STATIC probe X columns are grader-local
 *    geometry (the demo header names content rects, not menu-bar columns); the Y
 *    ranges ARE grounded in spec/chrome_metrics.h FLAIR_CHROME_MENUBAR_H, the SAME
 *    constant shell.h derives SHELL_MENUBAR1_TOP/SHELL_MENUBAR2_TOP from. All are
 *    COARSE differentials (strip-differs / strip-identical, not exact glyph
 *    columns).  Step 4/5 SHOULD tighten to specific title-glyph columns once the
 *    real booted tenants image exists (and may promote BAND_X0/X1 into
 *    flair_tenants_demo.h).  All stay DIFFERENTIALs (never a one-scene palette
 *    read) under ADR-0010.
 *  - MUTATION COVERAGE OF THE MENU-BAR LEGS (bead initech-4w15, honest):
 *      * MENU-BAND bites the no-menubar-swap mutant AND the initech-4w15 root cause
 *        (the row-0 mistarget draws the swap into band 1, so band 2 never changes
 *        -> 0 diffs -> RED).  This is the load-bearing proof the fix is real.
 *      * DISTINCT-CHIMERA bites the two-identical-bars Law-4 regression (a boot that
 *        collapses band 2 onto band 1's System-7 menu).
 *      * POST-DISTINCT bites KMAIN_MUT_NOTES_BAR_SYS (bead initech-7tjp): the
 *        original shared-object assignment makes post-switch band 2 identical to
 *        band 1, including the duplicate Apple slot.
 *      * BAR1-STATIC guards "the switch never repaints band 1". NOTES now carries
 *        a menu distinct from band 1, so the initech-4w15 row-0 mistarget would
 *        change band 1 as well as leave band 2 stale; BAR1-STATIC and MENU-BAND
 *        would both go RED. There is no dedicated row-0 mutant image; the named
 *        NOTES_BAR_SYS mutant proves POST-DISTINCT independently.
 *  - The MENU-BAND leg cannot distinguish "swapped to NOTES's menu" from "swapped
 *    to anything"; the TIER-A/TIER-B legs (canon VALUEs) carry the identity proof,
 *    so the band leg is intentionally only the swap-happened differential.
 *    BAR1-STATIC / DISTINCT-CHIMERA are intentionally structural differentials
 *    (no-change / bars-differ), not canon reads.
 */
