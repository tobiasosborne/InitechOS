/*
 * ppm_flair_appswitch_check.c -- the O-5 EMU app-switch oracle's screendump
 * grader (HOST, C-only). beads ADR-0013 (FLAIR App Contract); Wave-4 gate O-5.
 *
 * It takes the PRE-switch and POST-switch screendumps of the booted
 * -DFLAIR_LIVE_TENANTS desktop (two co-resident reference tenants: HELLO,
 * launched first, and NOTES, launched LAST so it is foreground and PARTIALLY
 * OCCLUDES HELLO).  The locked FLAIR_APPSWITCH_SPEC trace clicks HELLO's visible
 * sliver (the background tenant).  That single click must:
 *   - raise the HELLO group to the front (co-residency + group-raise),
 *   - repaint the newly-exposed overlap region via the updateEvt route, and
 *   - fire the activate/deactivate pair so HELLO becomes the active tenant and
 *     NOTES deactivates.
 * The PRE dump is grabbed before the click; the POST dump after the
 * FLAIR-DISPATCH app=HELLO marker.  This grader judges the PRE->POST delta.
 *
 * THREE DIFFERENTIALS, each independently catching a distinct app-switch mutant:
 *
 *   TIER-A -- CO-RESIDENCY + GROUP-RAISE + updateEvt REPAINT (structural).
 *     The OVERLAP probe (FLAIR_TEN_PROBE_OVERLAP_X/Y, inside HELLO content but
 *     under NOTES pre-switch) reads NOTES_FILL in the PRE dump (NOTES on top) and
 *     HELLO_FILL in the POST dump (HELLO raised to front AND its newly-exposed
 *     overlap content repainted).  This ONE differential proves all three: the
 *     two tenants co-exist, the clicked group rose to front, and the exposed
 *     region was repainted by the updateEvt route.  The drop-updateEvt / no-raise
 *     / ignore-refCon mutants all leave NOTES_FILL here -> RED.  (ADR-0006 E-D5
 *     Tier-A damage-law kind: a STRUCTURAL differential, not a single-scene read.)
 *
 *   TIER-B -- ACTIVATION reached the tenant (independent VALUE).
 *     The HELLO ACTIVE-ACCENT probe (a FLAIR_TEN_ACCENT_SIZE block at
 *     FLAIR_TEN_HELLO_ACCENT_X/Y, inside HELLO content, NOT under NOTES) reads
 *     HELLO_FILL in the PRE dump (HELLO inactive: content fill only) and
 *     FLAIR_TEN_ACTIVE_ACCENT in the POST dump (HELLO became active: the tenant
 *     painted its accent block in response to activateEvt active=1).  The
 *     skip-activate-pair mutant leaves it FILL -> RED.  NOTE (per the demo
 *     contract): the activation observable is the TENANT CONTENT ACCENT, NOT a
 *     title-bar hilite -- the renderer has no active/inactive CHROME distinction,
 *     so we do NOT probe chrome hilite.  (ADR-0006 E-D5 Tier-B independent golden
 *     kind: graded against the canon VALUE the demo header names, by recomputation
 *     from the independent canon, never by-construction.)
 *
 *   MENU-BAND swap -- the foreground app's menubar swapped in (PRE-vs-POST
 *     DIFFERENTIAL).  When HELLO becomes foreground, its menubar replaces NOTES'
 *     in the System-7 menu-bar band.  We grade this as a DIFFERENTIAL of the
 *     menu-bar title strip: the band's title region must DIFFER pre-vs-post.  The
 *     menubar-no-swap mutant leaves the band byte-identical -> 0 differing pixels
 *     -> RED.  We keep it a differential and NEVER a palette read of one scene
 *     (a static band cannot pass).  See the BAND_* probe note + the deviation
 *     callout in the trailing comment re: exact glyph columns.
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

/* Per-channel tolerance: capture-noise only (the XRGB8888 -> P6 dump is exact;
 * canon entries differ by far more than 2/channel), mirroring the drag grader. */
#define TOL 2

/* ---- the foreground-tenant MENU-BAR TITLE STRIP probe (the band-swap leg). ----
 * The demo is 640x480 with TWO 20px bars (flair_tenants_demo.h header rationale),
 * so the System-7 menu bar occupies y[0,20) and content begins at y>=40.  We scan
 * the menu-bar title strip -- the band INTERIOR (avoid the 1px bar edges) to the
 * right of the Apple menu (titles begin at x=20, cf. ppm_flair_menu_check's
 * MenuBar_title_x) -- and count pixels that DIFFER pre-vs-post.  The band is
 * strictly y<20, well clear of the content probes (y>=88), so a content change
 * cannot leak into this leg; only the menubar title swap can move it.
 *
 * DEVIATION (documented; see trailing note): the demo header does not yet name
 * menu-bar geometry, so these BAND_* coords are grader-local.  We assert the
 * coarse DIFFERENTIAL (title strip differs) rather than exact glyph columns,
 * which would be brittle against the hand-made proof pair; Step 4/5 may tighten
 * to specific columns once the real booted image exists.  BAND_MIN_DIFFS is set
 * above capture noise but far below a real glyph-strip swap. */
#define BAND_Y0          4     /* top    of the menu-bar interior strip          */
#define BAND_Y1          16    /* bottom of the menu-bar interior strip (y<20)   */
#define BAND_X0          24    /* right of the Apple menu (titles start at x=20) */
#define BAND_X1          200   /* across the foreground app's menu-title strip   */
#define BAND_MIN_DIFFS   8     /* above capture noise, below a real title swap   */

/* ---- the HELLO active-accent block CENTRE (sampled inside the 12x12 block). -- */
#define ACC_CX (FLAIR_TEN_HELLO_ACCENT_X + FLAIR_TEN_ACCENT_SIZE / 2)
#define ACC_CY (FLAIR_TEN_HELLO_ACCENT_Y + FLAIR_TEN_ACCENT_SIZE / 2)

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

/* Count pixels in the menu-bar title strip that differ (beyond TOL) pre-vs-post.
 * A pure DIFFERENTIAL: no canon VALUE is read here, only the PRE<->POST delta. */
static int band_diff_count(const Img *pre, const Img *post)
{
    int diffs = 0;
    for (int y = BAND_Y0; y < BAND_Y1; y++) {
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
           "(click HELLO sliver @(%d,%d) -> raise+activate HELLO over NOTES)\n",
           FLAIR_TEN_HELLO_CLICK_X, FLAIR_TEN_HELLO_CLICK_Y);

    /* ---- TIER-A: co-residency + group-raise + updateEvt repaint ------------- */
    /* PRE: NOTES is on top -> the overlap probe reads NOTES_FILL. */
    assert_idx(&pre, "PRE", FLAIR_TEN_PROBE_OVERLAP_X, FLAIR_TEN_PROBE_OVERLAP_Y,
               FLAIR_TEN_NOTES_FILL,
               "TIER-A: PRE overlap probe is NOTES_FILL (NOTES on top, HELLO covered)");
    /* POST: HELLO raised + the exposed overlap repainted -> reads HELLO_FILL. */
    assert_idx(&post, "POST", FLAIR_TEN_PROBE_OVERLAP_X, FLAIR_TEN_PROBE_OVERLAP_Y,
               FLAIR_TEN_HELLO_FILL,
               "TIER-A: POST overlap probe is HELLO_FILL (HELLO raised + updateEvt-repainted)");
    if (!g_fail) {
        printf("    TIER-A: overlap NOTES_FILL->HELLO_FILL -- co-residency + "
               "group-raise + updateEvt repaint of the exposed region\n");
    }

    /* ---- TIER-B: activation reached the tenant (content accent VALUE) ------- */
    /* PRE: HELLO inactive -> the accent block shows HELLO_FILL (no accent). */
    assert_idx(&pre, "PRE", ACC_CX, ACC_CY, FLAIR_TEN_HELLO_FILL,
               "TIER-B: PRE HELLO accent block is HELLO_FILL (HELLO inactive)");
    /* POST: HELLO active -> the tenant painted FLAIR_TEN_ACTIVE_ACCENT. Sample
     * three interior points so a lucky single pixel cannot pass the block. */
    assert_idx(&post, "POST", ACC_CX, ACC_CY, FLAIR_TEN_ACTIVE_ACCENT,
               "TIER-B: POST HELLO accent block (centre) is the active accent");
    assert_idx(&post, "POST", FLAIR_TEN_HELLO_ACCENT_X + 2,
               FLAIR_TEN_HELLO_ACCENT_Y + 2, FLAIR_TEN_ACTIVE_ACCENT,
               "TIER-B: POST HELLO accent block (top-left interior) is the active accent");
    assert_idx(&post, "POST", FLAIR_TEN_HELLO_ACCENT_X + FLAIR_TEN_ACCENT_SIZE - 3,
               FLAIR_TEN_HELLO_ACCENT_Y + FLAIR_TEN_ACCENT_SIZE - 3,
               FLAIR_TEN_ACTIVE_ACCENT,
               "TIER-B: POST HELLO accent block (bottom-right interior) is the active accent");
    if (!g_fail) {
        printf("    TIER-B: HELLO accent FILL->ACTIVE_ACCENT -- the activate/"
               "deactivate pair fired and reached the tenant (active=1)\n");
    }

    /* ---- MENU-BAND swap: the foreground menubar title strip differs --------- */
    {
        int diffs = band_diff_count(&pre, &post);
        if (diffs < BAND_MIN_DIFFS) {
            fprintf(stderr,
                    "ppm_flair_appswitch_check: FAIL MENU-BAND -- the foreground "
                    "menubar title strip x[%d,%d) y[%d,%d) is UNCHANGED pre-vs-post "
                    "(%d differing px < %d): the menubar did NOT swap to HELLO\n",
                    BAND_X0, BAND_X1, BAND_Y0, BAND_Y1, diffs, BAND_MIN_DIFFS);
            g_fail = 1;
        } else {
            printf("    MENU-BAND: the menu-bar title strip x[%d,%d) y[%d,%d) "
                   "DIFFERS pre-vs-post (%d px) -- HELLO's menubar swapped in\n",
                   BAND_X0, BAND_X1, BAND_Y0, BAND_Y1, diffs);
        }
    }

    free(pre.buf);
    free(post.buf);

    if (g_fail) {
        fprintf(stderr, "ppm_flair_appswitch_check: FAIL -- the click did not "
                "raise+activate the background tenant + swap its menubar (the "
                "FLAIR App Contract app-switch is not actually wired)\n");
        return 1;
    }
    printf("ppm_flair_appswitch_check: PASS -- clicking HELLO's sliver raised the "
           "HELLO group (overlap NOTES_FILL->HELLO_FILL + updateEvt repaint), "
           "activated it (accent FILL->ACTIVE_ACCENT), and swapped its menubar; "
           "the booted desktop honours the O-5 app-switch contract (ADR-0013)\n");
    return 0;
}

/*
 * DEVIATIONS / RISKS (Law 2 honesty):
 *  - MENU-BAND probe is grader-local geometry (the demo header names content
 *    rects, not menu-bar columns) and a COARSE differential (strip-differs, not
 *    exact glyph columns).  This is the deliberate brittleness trade-off for the
 *    hand-made proof pair; it bites the menubar-no-swap mutant (0 diffs) today.
 *    Step 4/5 SHOULD tighten to specific title-glyph columns once the real booted
 *    tenants image exists (and may promote BAND_* into flair_tenants_demo.h).  It
 *    stays a DIFFERENTIAL (never a one-scene palette read) under ADR-0010.
 *  - The band leg cannot distinguish "swapped to HELLO's menu" from "swapped to
 *    anything"; the TIER-A/TIER-B legs (canon VALUEs) carry the identity proof,
 *    so the band leg is intentionally only the swap-happened differential.
 */
