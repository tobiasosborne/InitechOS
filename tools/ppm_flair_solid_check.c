/*
 * ppm_flair_solid_check.c -- the FLAIR live-desktop SOLIDITY oracle's
 * screendump grader (HOST, C-only). Epic initech-av7s; beads initech-gofc
 * (leg A/B: the WM update contract -- tenant content survives expose/drag),
 * initech-rqz5 (leg C: activation repaints chrome).
 *
 * The 2026-07-21 first-person drive battery (WL shard TBD) proved the live
 * tenants desktop degrades monotonically: the compositor repaints CHROME only
 * and destroys tenant-owed content damage (desktop.c:269-273 unconditional
 * validate; flair_route_updates only invoked on a foreground switch), and
 * activation seeds no chrome invalidate (window.c:226-235, 243-246). Every
 * leg below is RED against that baseline (proven on captured frames before
 * the fixes landed -- Rule 1) and graded against the INDEPENDENT color canon
 * (Law 2; ADR-0010: flair_canon_rgb from spec/assets/color_canon.h, vouched
 * by test-color-canon -- NEVER the renderer's palette).
 *
 * Legs (each one boot + one dump; traces locked in spec/flair_solid_traces.mk):
 *
 *   A <post_close.ppm>  -- CLOSE-EXPOSE CONTENT. After the locked close trace
 *       (click HELLO's go-away box), the previously-occluded NOTES overlap
 *       content must read NOTES_FILL (the owner repainted via the updateEvt
 *       route), NOT the WDEF blank CONTENT white and NOT stale HELLO_FILL.
 *       Probe grid avoids NOTES's accent block + centre marker.
 *
 *   B <post_drag.ppm>   -- DRAG PRESERVES CONTENT. After the locked
 *       switch-then-drag trace (activate NOTES, drag it by its title bar to
 *       struct (200,180)), the moved window's content interior must read
 *       NOTES_FILL at the NEW position -- the drag repaint must include the
 *       owner's content, not just WDEF chrome.
 *
 *   C <post_switch.ppm> -- ACTIVATION CHROME. After the locked O-5 switch
 *       trace (activate background NOTES), (i) NOTES's title band must show
 *       ACTIVE chrome across its FULL width -- pinstripe alternation (both
 *       PIN_LIGHT and PIN_DARK present in the stripe rows) at left/mid/right
 *       columns, including the previously-occluded left segment; (ii) HELLO's
 *       title band must be flat inactive CONTENT white (no pinstripe); (iii)
 *       no stale vertical black run (HELLO's old right-edge frame) may cross
 *       NOTES's title band at x = HELLO right edge - 1.
 *
 * Usage: ppm_flair_solid_check <A|B|C> <dump.ppm>
 * Exit 0 = PASS; non-zero = a named FAIL (assertion + sampled-vs-expected).
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11): probe geometry derives from
 * spec/flair_tenants_demo.h + spec/chrome_metrics.h, never grader-local magic
 * where a shared constant exists.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flair_tenants_demo.h"   /* -Ispec: tenant rects + fills + canon      */
#include "chrome_metrics.h"       /* -Ispec: FLAIR_CHROME_* title geometry     */

#define TOL 2   /* per-channel capture tolerance, mirroring the drag grader */

typedef struct { int w, h; unsigned char *buf; } Img;

static int read_ppm(const char *path, Img *im)
{
    FILE *f = fopen(path, "rb");
    char magic[3] = {0};
    int maxv = 0;
    if (!f) { fprintf(stderr, "solid_check: cannot open %s\n", path); return 1; }
    if (fscanf(f, "%2s %d %d %d", magic, &im->w, &im->h, &maxv) != 4 ||
        strcmp(magic, "P6") != 0 || maxv != 255 ||
        im->w <= 0 || im->h <= 0 || im->w > 4096 || im->h > 4096) {
        fprintf(stderr, "solid_check: %s is not a sane P6 ppm\n", path);
        fclose(f); return 1;
    }
    (void)fgetc(f);   /* the single whitespace after maxval */
    im->buf = (unsigned char *)malloc((size_t)im->w * (size_t)im->h * 3u);
    if (!im->buf) { fclose(f); return 1; }
    if (fread(im->buf, 3u, (size_t)im->w * (size_t)im->h, f)
        != (size_t)im->w * (size_t)im->h) {
        fprintf(stderr, "solid_check: %s truncated\n", path);
        fclose(f); free(im->buf); return 1;
    }
    fclose(f);
    return 0;
}

static void px(const Img *im, int x, int y, int *r, int *g, int *b)
{
    const unsigned char *p = im->buf + 3u * ((size_t)y * (size_t)im->w + (size_t)x);
    *r = p[0]; *g = p[1]; *b = p[2];
}

/* Canon RGB for a palette index -- the INDEPENDENT expected value (Law 2). */
static void canon(unsigned char idx, int *r, int *g, int *b)
{
    unsigned int v = (unsigned int)(flair_canon_rgb(idx) & 0x00FFFFFFu);
    *r = (int)((v >> 16) & 0xFFu);
    *g = (int)((v >> 8) & 0xFFu);
    *b = (int)(v & 0xFFu);
}

static int near3(int r, int g, int b, int er, int eg, int eb)
{
    int dr = r - er, dg = g - eg, db = b - eb;
    if (dr < 0) dr = -dr;
    if (dg < 0) dg = -dg;
    if (db < 0) db = -db;
    return dr <= TOL && dg <= TOL && db <= TOL;
}

static int probe_is(const Img *im, int x, int y, unsigned char cidx,
                    const char *what)
{
    int r, g, b, er, eg, eb;
    if (x < 0 || y < 0 || x >= im->w || y >= im->h) {
        fprintf(stderr, "FAIL %s: probe (%d,%d) outside %dx%d dump\n",
                what, x, y, im->w, im->h);
        return 1;
    }
    px(im, x, y, &r, &g, &b);
    canon(cidx, &er, &eg, &eb);
    if (!near3(r, g, b, er, eg, eb)) {
        fprintf(stderr,
                "FAIL %s: (%d,%d) sampled #%02X%02X%02X expected canon idx %u "
                "#%02X%02X%02X\n", what, x, y, r, g, b, (unsigned)cidx,
                er, eg, eb);
        return 1;
    }
    return 0;
}

/* ---- shared title-band geometry (chrome.c documentProc layout):
 * title_top = top+1, stripe rows [top+2, top+2+FLAIR_CHROME_TITLE_STRIPE_ROWS).
 * We scan the stripe interior one row in from each end. */
#define STRIPE_Y0(top)  ((top) + 3)
#define STRIPE_Y1(top)  ((top) + 2 + FLAIR_CHROME_TITLE_STRIPE_ROWS - 2)

/* Column stack scan: does [y0,y1] at x contain a pixel near canon(a) AND one
 * near canon(b)? (pinstripe present).  Or all near canon(a)? (flat). */
static void scan_col(const Img *im, int x, int y0, int y1,
                     unsigned char a, unsigned char b,
                     int *saw_a, int *saw_b, int *saw_other)
{
    int ar, ag, ab_, br, bg, bb;
    canon(a, &ar, &ag, &ab_);
    canon(b, &br, &bg, &bb);
    *saw_a = *saw_b = *saw_other = 0;
    for (int y = y0; y <= y1; y++) {
        int r, g, bl;
        px(im, x, y, &r, &g, &bl);
        if (near3(r, g, bl, ar, ag, ab_))      (*saw_a)++;
        else if (near3(r, g, bl, br, bg, bb))  (*saw_b)++;
        else                                    (*saw_other)++;
    }
}

/* ================= leg A: close-expose content ========================= */
static int leg_A(const Img *im)
{
    /* The exposed overlap: x [FLAIR_TEN_OVERLAP_L, FLAIR_TEN_OVERLAP_R),
     * content rows only (below NOTES's title band). Probe a grid clear of
     * NOTES's top-left accent block (content top-left + ~16px). */
    static const int xs[4] = { 290, 310, 330, 350 };
    static const int ys[4] = { 160, 190, 220, 250 };
    int bad = 0;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            bad |= probe_is(im, xs[i], ys[j], FLAIR_TEN_NOTES_FILL,
                            "leg A close-expose NOTES content");
    if (!bad)
        printf("solid A PASS: exposed overlap reads NOTES_FILL (owner repainted)\n");
    return bad;
}

/* ================= leg B: drag preserves content ======================= */
/* The locked leg-B trace drags NOTES by its title bar with delta (-60,+60):
 * struct (260,120)->(200,180). Mirror of the trace in
 * spec/flair_solid_traces.mk -- keep in sync. */
#define SOLID_B_NOTES_L  (FLAIR_TEN_NOTES_L - 60)
#define SOLID_B_NOTES_T  (FLAIR_TEN_NOTES_T + 60)
static int leg_B(const Img *im)
{
    /* Content interior at the NEW position, clear of the accent block (top-
     * left + ~16px) and the centre marker block (content centre +-8). */
    int cl = SOLID_B_NOTES_L, ct = SOLID_B_NOTES_T;
    static const int dx[4] = { 40, 80, 220, 260 };
    static const int dy[4] = { 50, 80, 150, 190 };
    int bad = 0;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            bad |= probe_is(im, cl + dx[i], ct + dy[j], FLAIR_TEN_NOTES_FILL,
                            "leg B drag-preserved NOTES content");
    if (!bad)
        printf("solid B PASS: dragged NOTES content reads NOTES_FILL at the new rect\n");
    return bad;
}

/* ================= leg C: activation chrome ============================ */
static int leg_C(const Img *im)
{
    int bad = 0;

    /* (i) NOTES title band ACTIVE across full width: pinstripe alternation at
     * left (previously occluded), mid, right columns. Columns avoid the
     * close box (left+9..left+20) and zoom box (right-20..right-9). */
    {
        static const int xs[3] = { FLAIR_TEN_NOTES_L + 40,        /* 300 */
                                   FLAIR_TEN_NOTES_L + 190,       /* 450 */
                                   FLAIR_TEN_NOTES_R - 40 };      /* 520 */
        for (int i = 0; i < 3; i++) {
            int sl, sd, so;
            scan_col(im, xs[i], STRIPE_Y0(FLAIR_TEN_NOTES_T),
                     STRIPE_Y1(FLAIR_TEN_NOTES_T),
                     CIDX_PIN_LIGHT, CIDX_PIN_DARK, &sl, &sd, &so);
            if (sl == 0 || sd == 0) {
                fprintf(stderr,
                        "FAIL leg C: NOTES title x=%d stripes not ACTIVE "
                        "(pin_light rows=%d pin_dark rows=%d other=%d) -- "
                        "newly-active window kept inactive chrome\n",
                        xs[i], sl, sd, so);
                bad = 1;
            }
        }
    }

    /* (ii) HELLO title band flat INACTIVE white (no pinstripe). */
    {
        static const int xs[3] = { FLAIR_TEN_HELLO_L + 40,        /* 100 */
                                   FLAIR_TEN_HELLO_L + 140,       /* 200 */
                                   FLAIR_TEN_HELLO_R - 40 };      /* 320 */
        for (int i = 0; i < 3; i++) {
            int sw, sd, so;
            scan_col(im, xs[i], STRIPE_Y0(FLAIR_TEN_HELLO_T),
                     STRIPE_Y1(FLAIR_TEN_HELLO_T),
                     CIDX_WHITE, CIDX_PIN_DARK, &sw, &sd, &so);
            if (sd != 0 || so != 0) {
                fprintf(stderr,
                        "FAIL leg C: HELLO title x=%d not flat inactive white "
                        "(white=%d pin_dark=%d other=%d)\n",
                        xs[i], sw, sd, so);
                bad = 1;
            }
        }
    }

    /* (iii) no stale HELLO right-edge black run crossing NOTES's title band.
     * HELLO's right frame column is FLAIR_TEN_HELLO_R-1; post-raise those
     * pixels inside NOTES's title band belong to NOTES's chrome. A run of
     * >= 10 FRAME-black rows there is the stale-expose bug. */
    {
        int x = FLAIR_TEN_HELLO_R - 1;
        int er, eg, eb, run = 0, maxrun = 0;
        canon(CIDX_BLACK, &er, &eg, &eb);
        for (int y = FLAIR_TEN_NOTES_T + 1;
             y < FLAIR_TEN_NOTES_T + FLAIR_CHROME_TITLEBAR_H; y++) {
            int r, g, b;
            px(im, x, y, &r, &g, &b);
            if (near3(r, g, b, er, eg, eb)) { run++; if (run > maxrun) maxrun = run; }
            else run = 0;
        }
        if (maxrun >= 10) {
            fprintf(stderr,
                    "FAIL leg C: stale black run (%d rows) at x=%d inside "
                    "NOTES title band -- exposed title band not repainted\n",
                    maxrun, x);
            bad = 1;
        }
    }

    if (!bad)
        printf("solid C PASS: activation chrome correct (NOTES active full-width, "
               "HELLO flat, no stale band)\n");
    return bad;
}

int main(int argc, char **argv)
{
    Img im;
    int rc;
    if (argc != 3 || strlen(argv[1]) != 1) {
        fprintf(stderr, "usage: ppm_flair_solid_check <A|B|C> <dump.ppm>\n");
        return 2;
    }
    if (read_ppm(argv[2], &im)) return 2;
    switch (argv[1][0]) {
    case 'A': rc = leg_A(&im); break;
    case 'B': rc = leg_B(&im); break;
    case 'C': rc = leg_C(&im); break;
    default:
        fprintf(stderr, "solid_check: unknown leg '%s'\n", argv[1]);
        rc = 2;
    }
    free(im.buf);
    return rc;
}
