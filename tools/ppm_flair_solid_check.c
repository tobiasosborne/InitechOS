/*
 * ppm_flair_solid_check.c -- the FLAIR live-desktop SOLIDITY oracle's
 * screendump grader (HOST, C-only). Epic initech-av7s; beads initech-gofc
 * (leg A/B: the WM update contract -- tenant content survives expose/drag),
 * initech-rqz5 (leg C: activation repaints chrome), initech-r8r7
 * (leg G: the live-pump DragWindow policy keeps the title band reachable),
 * initech-haaq (leg H: title mouseDown raises before dragging), and
 * initech-t1rv (leg E: band 2 drops the active tenant's menu), and
 * initech-b3hl/-j0vt (leg D: menu cancel restores the whole frame exactly).
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
 *       ACTIVE Platinum chrome across its FULL width -- the exact 12-row
 *       light-first, strictly alternating white / CIDX_PLAT_STRIPE_DARK field
 *       at left/mid/right
 *       columns, including the previously-occluded left segment; (ii) HELLO's
 *       title band must be flat inactive CIDX_PLAT_FACE (no stripes/bevel);
 *       (iii)
 *       no stale vertical black run (HELLO's old right-edge frame) may cross
 *       NOTES's title band at x = HELLO right edge - 1.
 *
 *   D <pre.ppm> <post_cancel.ppm> -- MENU CANCEL RESTORE. PRE is a no-input
 *       boot captured after FLAIR-LIVE-OK. POST crosses Photoshop File -> Edit
 *       while held, releases in the bar (sel=0), and is captured after the
 *       same marker. The decoded P6 pixel buffers must be byte-identical over
 *       the whole frame (TOL=0): neither a panel nor a shell-render content
 *       wipe may survive the DQ2 restore cycle.
 *
 *   E <post_menu2.ppm> -- BAND-2 ACTIVE-TENANT MENU. With HELLO foreground,
 *       press Photoshop File in the second menu band. The DROP-time held panel
 *       must cover the formerly-teal rows immediately below band 2 with canon
 *       BTNFACE body gray plus a black top frame. The serial half of the gate
 *       independently requires menuID 256 and the later item-2 selection,
 *       distinguishing the active Photoshop bar from bar_sys (menuID 128).
 *
 *   G <post_clamp.ppm>  -- DRAG CLAMP. After activating NOTES, the locked
 *       trace grabs its title at (450,130) and releases at (5,5). The proposed
 *       struct (-185,-5) must be clamped to (-185,40): the complete title band
 *       begins immediately below the two menu bars and at least the classic
 *       four-pixel horizontal reachable strip remains on-screen.
 *
 *   H <post_raise_drag.ppm> -- RAISE ON TITLE CLICK. With NO prior content
 *       activation click, press NOTES's visible background title segment and
 *       drag it to struct (200,180). NOTES must land with content intact,
 *       active title stripes across its full width, and in front of HELLO at
 *       their old overlap.
 *
 * Usage: ppm_flair_solid_check <A|B|C|E|G|H> <dump.ppm>
 *        ppm_flair_solid_check D <pre.ppm> <post.ppm>
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
#include "menu.h"                 /* -Ios/flair: shared panel geometry constants */

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

/* ---- shared Platinum title-band geometry. DEC-10 Sec 4 +
 * sys8/window-chrome.md Sec 2.1/2.2: T is the top frame row and the exact
 * 12-row stripe field is [T+4,T+16), white first and dark last. Use the new
 * B1 names, not the retained System-7 compatibility aliases. */
#define STRIPE_Y0(top)  ((top) + FLAIR_CHROME_TITLE_STRIPE_TOP_OFF)
#define STRIPE_Y1(top)  (STRIPE_Y0(top) + FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS)

/* Exact Platinum stripe relation at a clear title column. DEC-10 Sec 4 +
 * sys8/window-chrome.md Sec 2.2: all 12 rows are present, white first, dark
 * last, and every adjacent pair differs. */
static int probe_platinum_stripe_col(const Img *im, int x, int top,
                                     const char *leg)
{
    int mismatches = 0;
    int first_y = 0, first_r = 0, first_g = 0, first_b = 0;
    int first_er = 0, first_eg = 0, first_eb = 0;
    int lr, lg, lb, dr, dg, db;
    canon(CIDX_WHITE, &lr, &lg, &lb);
    canon(CIDX_PLAT_STRIPE_DARK, &dr, &dg, &db);

    for (int y = STRIPE_Y0(top); y < STRIPE_Y1(top); y++) {
        int r, g, b;
        int rel = y - STRIPE_Y0(top);
        int er = (rel & 1) ? dr : lr;
        int eg = (rel & 1) ? dg : lg;
        int eb = (rel & 1) ? db : lb;
        px(im, x, y, &r, &g, &b);
        if (!near3(r, g, b, er, eg, eb)) {
            if (mismatches == 0) {
                first_y = y;
                first_r = r; first_g = g; first_b = b;
                first_er = er; first_eg = eg; first_eb = eb;
            }
            mismatches++;
        }
    }
    if (mismatches) {
        fprintf(stderr,
                "FAIL %s: Platinum title x=%d has %d/12 wrong stripe rows; "
                "first T+%d sampled #%02X%02X%02X expected #%02X%02X%02X -- "
                "light-first strict relation broken\n",
                leg, x, mismatches, first_y - top,
                first_r, first_g, first_b, first_er, first_eg, first_eb);
        return 1;
    }
    return 0;
}

/* Shared leg-B/H content probes: offsets are relative to the independently
 * computed post-drag NOTES structure rect. They avoid the top-left active
 * accent and the centre mouseDown marker. */
static int probe_notes_content(const Img *im, int left, int top,
                               const char *what)
{
    static const int dx[4] = { 40, 80, 220, 260 };
    static const int dy[4] = { 50, 80, 150, 190 };
    int bad = 0;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            bad |= probe_is(im, left + dx[i], top + dy[j],
                            FLAIR_TEN_NOTES_FILL, what);
    return bad;
}

/* Shared active-title probe: left/middle/right columns avoid Platinum close
 * (L+4), right-hand zoom/collapse (R-32/R-16), and the title-text gap. The
 * left column is in the segment HELLO occluded at boot. DEC-10 Sec 4 +
 * sys8/window-chrome.md Sec 2.2/3.1. */
static int probe_notes_active_title(const Img *im, int left, int top, int right,
                                    const char *leg)
{
    int xs[3] = { left + 40, left + 190, right - 40 };
    int bad = 0;
    for (int i = 0; i < 3; i++)
        bad |= probe_platinum_stripe_col(im, xs[i], top, leg);
    return bad;
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
#define SOLID_B_DRAG_DH  (-60)
#define SOLID_B_DRAG_DV  60
#define SOLID_B_NOTES_L  (FLAIR_TEN_NOTES_L + SOLID_B_DRAG_DH)
#define SOLID_B_NOTES_T  (FLAIR_TEN_NOTES_T + SOLID_B_DRAG_DV)
static int leg_B(const Img *im)
{
    /* Content interior at the NEW position, clear of the accent block (top-
     * left + ~16px) and the centre marker block (content centre +-8). */
    int bad = probe_notes_content(im, SOLID_B_NOTES_L, SOLID_B_NOTES_T,
                                  "leg B drag-preserved NOTES content");
    if (!bad)
        printf("solid B PASS: dragged NOTES content reads NOTES_FILL at the new rect\n");
    return bad;
}

/* ================= leg C: activation chrome ============================ */
static int leg_C(const Img *im)
{
    int bad = 0;

    /* (i) NOTES title band ACTIVE across full width: exact Platinum stripe
     * relation at left (previously occluded), mid, right columns. DEC-10 Sec 4
     * + sys8/window-chrome.md Sec 2.2/3.1. */
    bad |= probe_notes_active_title(im, FLAIR_TEN_NOTES_L,
                                    FLAIR_TEN_NOTES_T, FLAIR_TEN_NOTES_R,
                                    "leg C");

    /* (ii) HELLO title interior is flat inactive CIDX_PLAT_FACE for all 20
     * rows between its CIDX_PLAT_INACTIVE_FRAME lines. There are no stripes or
     * bevel. DEC-10 Sec 4 + sys8/window-chrome.md Sec 6. Columns avoid the
     * centered title-ink run. */
    {
        static const int xs[3] = { FLAIR_TEN_HELLO_L + 40,        /* 100 */
                                   FLAIR_TEN_HELLO_L + 80,        /* 140 */
                                   FLAIR_TEN_HELLO_R - 40 };      /* 320 */
        for (int i = 0; i < 3; i++) {
            for (int y = FLAIR_TEN_HELLO_T + 1;
                 y < FLAIR_TEN_HELLO_T + FLAIR_CHROME_TITLEBAR_H - 1; y++)
                bad |= probe_is(im, xs[i], y, CIDX_PLAT_FACE,
                                "leg C HELLO flat inactive Platinum title fill");
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
        printf("solid C PASS: activation chrome correct (NOTES exact Platinum "
               "stripes full-width, HELLO flat idx231, no stale band)\n");
    return bad;
}

/* ================= leg D: menu cancel restores byte-exact ============= */
static int leg_D(const Img *pre, const Img *post)
{
    size_t n;

    if (pre->w != post->w || pre->h != post->h) {
        fprintf(stderr,
                "FAIL leg D: PRE is %dx%d but POST is %dx%d -- frames cannot "
                "be byte-identical\n",
                pre->w, pre->h, post->w, post->h);
        return 1;
    }

    n = (size_t)pre->w * (size_t)pre->h * 3u;
    if (memcmp(pre->buf, post->buf, n) != 0) {
        size_t off;
        for (off = 0u; off < n && pre->buf[off] == post->buf[off]; off++) { }
        {
            size_t pix = off / 3u;
            int x = (int)(pix % (size_t)pre->w);
            int y = (int)(pix / (size_t)pre->w);
            const unsigned char *a = pre->buf + pix * 3u;
            const unsigned char *b = post->buf + pix * 3u;
            fprintf(stderr,
                    "FAIL leg D: PRE != POST at first differing pixel (%d,%d): "
                    "PRE #%02X%02X%02X POST #%02X%02X%02X -- menu cancel did "
                    "not restore the whole tenant frame byte-exact (TOL=0; "
                    "panel persistence or shell-render content wipe)\n",
                    x, y, a[0], a[1], a[2], b[0], b[1], b[2]);
        }
        return 1;
    }

    printf("solid D PASS: menu cross-title cancel restored the whole frame "
           "byte-identical (TOL=0)\n");
    return 0;
}

/* ================= leg E: active tenant's band-2 menu ================= */
/* Independent screen placement for the locked leg-E drop. The Menu Manager
 * assumes its bar begins at local y=0, so its Platinum panel shares local
 * baseline y=FLAIR_MENUBAR_H-1. Band 2 begins one shared menubar height down;
 * therefore screen panel top is y=39. The x probes lie in the
 * overlap of both possible File panels: Photoshop begins at x=0 (no Apple
 * slot), while the incorrect bar_sys begins at x=FLAIR_MENU_APPLE_W == 20.
 * At the first item's glyph rows 0 and 1, Chicago cells are blank, so these
 * points read panel BG for either bar. Thus pixels prove DROP+OFFSET, while the
 * Makefile's menu=256 tooth alone proves ROUTING (Law 2; initech-t1rv).
 * Shared chrome/menu constants are used wherever they exist (Rule 11). */
#define SOLID_E_BAND2_TOP       FLAIR_CHROME_MENUBAR_H
#define SOLID_E_PANEL_TOP       (SOLID_E_BAND2_TOP + FLAIR_MENUBAR_H - 1)
#define SOLID_E_FIRST_ROW_TOP   (SOLID_E_PANEL_TOP + FLAIR_MENU_PANEL_INSET)
#define SOLID_E_COMMON_X0       (FLAIR_MENU_APPLE_W + \
                                  FLAIR_MENU_PANEL_INSET + 3)
#define SOLID_E_COMMON_X1       (2 * FLAIR_MENU_APPLE_W + \
                                  FLAIR_MENU_TITLE_PAD)

static int leg_E(const Img *im)
{
    int bad = 0;

    /* The top frame shares band-2 baseline y=39. This x is inside both the clean
     * Photoshop panel and the bar_sys mutant's shifted panel. */
    bad |= probe_is(im, SOLID_E_COMMON_X0, SOLID_E_PANEL_TOP,
                    CIDX_BLACK, "leg E band-2 menu black top frame");

    /* Two independent sampled-E7 body probes at first-item top. Before
     * the drop these screen points are teal; after either possible File panel
     * drops they are gray. Glyph rows 0/1 are blank in the Chicago fixture. */
    bad |= probe_is(im, SOLID_E_COMMON_X0, SOLID_E_FIRST_ROW_TOP,
                    CIDX_PLAT_FACE, "leg E band-2 menu sampled E7 body row 0");
    bad |= probe_is(im, SOLID_E_COMMON_X1, SOLID_E_FIRST_ROW_TOP + 1,
                    CIDX_PLAT_FACE, "leg E band-2 menu sampled E7 body row 1");

    if (!bad)
        printf("solid E PASS: band-2 File panel shares baseline y=39 "
               "(black frame + sampled E7 body)\n");
    return bad;
}

/* ================= leg G: live-pump drag clamp ======================== */
/* Independent expected geometry for the LOCKED leg-G trace:
 * title grab (450,130) -> release (5,5) gives delta (-445,-125);
 * NOTES struct (260,120) would become (-185,-5). The two locked
 * FLAIR_CHROME_MENUBAR_H bands clamp top to 40. Horizontal reachability follows
 * the period sample's four-pixel DragWindow boundsRect inset; the proposed left
 * already leaves 115 px visible, so it remains -185. These are trace/spec
 * computations, never observations of FLAIR's render (Law 2; initech-r8r7). */
#define SOLID_G_GRAB_X          450
#define SOLID_G_GRAB_Y          130
#define SOLID_G_RELEASE_X       5
#define SOLID_G_RELEASE_Y       5
#define SOLID_G_DRAG_DH         (SOLID_G_RELEASE_X - SOLID_G_GRAB_X)
#define SOLID_G_DRAG_DV         (SOLID_G_RELEASE_Y - SOLID_G_GRAB_Y)
#define SOLID_G_NOTES_W         (FLAIR_TEN_NOTES_R - FLAIR_TEN_NOTES_L)
#define SOLID_G_PROPOSED_L      (FLAIR_TEN_NOTES_L + SOLID_G_DRAG_DH)
#define SOLID_G_PROPOSED_T      (FLAIR_TEN_NOTES_T + SOLID_G_DRAG_DV)
#define SOLID_G_NOTES_L         SOLID_G_PROPOSED_L
#define SOLID_G_NOTES_T         (2 * FLAIR_CHROME_MENUBAR_H)
#define SOLID_G_NOTES_R         (SOLID_G_NOTES_L + SOLID_G_NOTES_W)
#define SOLID_G_IM_DRAG_MARGIN  4

static int leg_G(const Img *im)
{
    int bad = 0;
    int visible_l = SOLID_G_NOTES_L < 0 ? 0 : SOLID_G_NOTES_L;
    int visible_r = SOLID_G_NOTES_R > im->w ? im->w : SOLID_G_NOTES_R;
    static const int xs[2] = { SOLID_G_IM_DRAG_MARGIN,
                               SOLID_G_NOTES_R - 40 };

    if (SOLID_G_PROPOSED_T >= SOLID_G_NOTES_T) {
        fprintf(stderr,
                "FAIL leg G: trace is not a vertical clamp case "
                "(proposed top=%d clamp top=%d)\n",
                SOLID_G_PROPOSED_T, SOLID_G_NOTES_T);
        bad = 1;
    }
    if (SOLID_G_NOTES_T < 2 * FLAIR_CHROME_MENUBAR_H ||
        SOLID_G_NOTES_T + FLAIR_CHROME_TITLEBAR_H > im->h) {
        fprintf(stderr,
                "FAIL leg G: expected title band [%d,%d) is outside the "
                "reachable vertical desktop [%d,%d)\n",
                SOLID_G_NOTES_T,
                SOLID_G_NOTES_T + FLAIR_CHROME_TITLEBAR_H,
                2 * FLAIR_CHROME_MENUBAR_H, im->h);
        bad = 1;
    }
    if (visible_r - visible_l < SOLID_G_IM_DRAG_MARGIN) {
        fprintf(stderr,
                "FAIL leg G: expected horizontal title reach is only %d px "
                "(minimum %d px)\n",
                visible_r - visible_l, SOLID_G_IM_DRAG_MARGIN);
        bad = 1;
    }

    /* The exact Platinum stripe relation at independently-computed y=40 proves that the
     * title, rather than content exposed below an overlay-hidden title, begins
     * immediately below menu band 2. Both columns avoid title text/gadgets.
     * DEC-10 Sec 4 + sys8/window-chrome.md Sec 2.1/2.2. */
    for (int i = 0; i < 2; i++)
        bad |= probe_platinum_stripe_col(im, xs[i], SOLID_G_NOTES_T, "leg G");

    /* Both frame lines are visible at their locked rows, proving the complete
     * FLAIR_CHROME_TITLEBAR_H band (not only an interior stripe sample) lies
     * below the half-open menu bands [0,2*FLAIR_CHROME_MENUBAR_H). */
    bad |= probe_is(im, SOLID_G_IM_DRAG_MARGIN, SOLID_G_NOTES_T,
                    CIDX_BLACK, "leg G reachable NOTES title top frame");
    bad |= probe_is(im, SOLID_G_IM_DRAG_MARGIN,
                    SOLID_G_NOTES_T + FLAIR_CHROME_TITLEBAR_H - 1,
                    CIDX_BLACK, "leg G reachable NOTES title bottom frame");

    /* Lock the visible right edge as a screendump counterpart to the exact
     * serial coordinate tooth: right-1 is frame, right is shadow, right+1 desktop.
     * At y=top+3 HELLO has not begun yet (HELLO top comes from the spec header). */
    bad |= probe_is(im, SOLID_G_NOTES_R - 1, SOLID_G_NOTES_T + 3,
                    CIDX_BLACK, "leg G visible NOTES right frame");
    bad |= probe_is(im, SOLID_G_NOTES_R, SOLID_G_NOTES_T + 3,
                    CIDX_BLACK, "leg G visible NOTES right shadow");
    bad |= probe_is(im, SOLID_G_NOTES_R + 1, SOLID_G_NOTES_T + 3,
                    CIDX_DESKTOP, "leg G pixel beyond NOTES shadow");

    if (!bad)
        printf("solid G PASS: NOTES title band reachable at struct (-185,40) "
               "with 115 px on-screen\n");
    return bad;
}

/* ================= leg H: raise on title click ======================== */
/* Independent expected geometry for the LOCKED leg-H trace. The cursor starts
 * at (320,240), moves directly by (+100,-100),(+30,-10) to the visible NOTES
 * title point (450,130), then drags (-60,+60) and releases at (390,190).
 * Therefore NOTES must move from (260,120) to (200,180), exactly the same final
 * structure origin as leg B but without leg B's prior content-activation click.
 * These values are trace/spec arithmetic, never renderer observations. */
#define SOLID_H_START_X          320
#define SOLID_H_START_Y          240
#define SOLID_H_GRAB_X           (SOLID_H_START_X + 100 + 30)
#define SOLID_H_GRAB_Y           (SOLID_H_START_Y - 100 - 10)
#define SOLID_H_RELEASE_X        (SOLID_H_GRAB_X - 60)
#define SOLID_H_RELEASE_Y        (SOLID_H_GRAB_Y + 60)
#define SOLID_H_DRAG_DH          (SOLID_H_RELEASE_X - SOLID_H_GRAB_X)
#define SOLID_H_DRAG_DV          (SOLID_H_RELEASE_Y - SOLID_H_GRAB_Y)
#define SOLID_H_NOTES_L          (FLAIR_TEN_NOTES_L + SOLID_H_DRAG_DH)
#define SOLID_H_NOTES_T          (FLAIR_TEN_NOTES_T + SOLID_H_DRAG_DV)
#define SOLID_H_NOTES_R          (SOLID_H_NOTES_L + \
                                  (FLAIR_TEN_NOTES_R - FLAIR_TEN_NOTES_L))
#define SOLID_H_NOTES_B          (SOLID_H_NOTES_T + \
                                  (FLAIR_TEN_NOTES_B - FLAIR_TEN_NOTES_T))
#define SOLID_H_CONTENT_L        (SOLID_H_NOTES_L + FLAIR_CHROME_FRAME)
#define SOLID_H_CONTENT_T        (SOLID_H_NOTES_T + FLAIR_CHROME_FRAME + \
                                  FLAIR_CHROME_TITLEBAR_H)
#define SOLID_H_CONTENT_R        (SOLID_H_NOTES_R - FLAIR_CHROME_FRAME)
#define SOLID_H_CONTENT_B        (SOLID_H_NOTES_B - FLAIR_CHROME_FRAME)

static int leg_H(const Img *im)
{
    int bad = 0;

    /* The leg-H and leg-B traces intentionally have the same (-60,+60) drag.
     * Keep their independent arithmetic tied before reusing the B/H content
     * probe helper at the common expected origin (200,180). */
    if (SOLID_H_GRAB_X != 450 || SOLID_H_GRAB_Y != 130 ||
        SOLID_H_NOTES_L != SOLID_B_NOTES_L ||
        SOLID_H_NOTES_T != SOLID_B_NOTES_T) {
        fprintf(stderr,
                "FAIL leg H: locked trace arithmetic drifted "
                "(grab=%d,%d H=%d,%d B=%d,%d)\n",
                SOLID_H_GRAB_X, SOLID_H_GRAB_Y,
                SOLID_H_NOTES_L, SOLID_H_NOTES_T,
                SOLID_B_NOTES_L, SOLID_B_NOTES_T);
        bad = 1;
    }

    /* (a) The moved tenant still owns and repaints its content at (200,180),
     * using the exact leg-B probe offsets that avoid accent/marker blocks. */
    bad |= probe_notes_content(im, SOLID_H_NOTES_L, SOLID_H_NOTES_T,
                               "leg H raised-drag NOTES content");

    /* (b1) NOTES is ACTIVE across the FULL moved title width. The left sample
     * is the relative title segment that was occluded by HELLO before the raise. */
    bad |= probe_notes_active_title(im, SOLID_H_NOTES_L, SOLID_H_NOTES_T,
                                    SOLID_H_NOTES_R, "leg H");

    /* (b2) NOTES is IN FRONT at the old HELLO/NOTES overlap. Intersect the
     * locked old overlap [260,360)x[120,260) with moved NOTES content
     * [201,499)x[200,399) => [260,360)x[200,260), then probe its interior
     * quartiles. Every pixel must be NOTES_FILL, never HELLO_FILL. */
    {
        int l = FLAIR_TEN_OVERLAP_L > SOLID_H_CONTENT_L
                    ? FLAIR_TEN_OVERLAP_L : SOLID_H_CONTENT_L;
        int t = FLAIR_TEN_OVERLAP_T > SOLID_H_CONTENT_T
                    ? FLAIR_TEN_OVERLAP_T : SOLID_H_CONTENT_T;
        int r = FLAIR_TEN_OVERLAP_R < SOLID_H_CONTENT_R
                    ? FLAIR_TEN_OVERLAP_R : SOLID_H_CONTENT_R;
        int b = FLAIR_TEN_OVERLAP_B < SOLID_H_CONTENT_B
                    ? FLAIR_TEN_OVERLAP_B : SOLID_H_CONTENT_B;
        if (l >= r || t >= b) {
            fprintf(stderr,
                    "FAIL leg H: old overlap has no moved NOTES content "
                    "intersection ([%d,%d)x[%d,%d))\n", l, r, t, b);
            bad = 1;
        } else {
            for (int i = 1; i <= 3; i++) {
                int x = l + i * (r - l) / 4;
                for (int j = 1; j <= 3; j++) {
                    int y = t + j * (b - t) / 4;
                    bad |= probe_is(im, x, y, FLAIR_TEN_NOTES_FILL,
                                    "leg H NOTES in front at old overlap");
                }
            }
        }
    }

    if (!bad)
        printf("solid H PASS: title click raised active NOTES, then dragged it "
               "to struct (200,180) in front of HELLO\n");
    return bad;
}

int main(int argc, char **argv)
{
    Img im;
    int rc;
    if (argc < 2 || strlen(argv[1]) != 1 ||
        (argv[1][0] == 'D' ? argc != 4 : argc != 3)) {
        fprintf(stderr,
                "usage: ppm_flair_solid_check <A|B|C|E|G|H> <dump.ppm>\n"
                "       ppm_flair_solid_check D <pre.ppm> <post.ppm>\n");
        return 2;
    }
    if (argv[1][0] == 'D') {
        Img post;
        if (read_ppm(argv[2], &im)) return 2;
        if (read_ppm(argv[3], &post)) {
            free(im.buf);
            return 2;
        }
        rc = leg_D(&im, &post);
        free(post.buf);
        free(im.buf);
        return rc;
    }
    if (read_ppm(argv[2], &im)) return 2;
    switch (argv[1][0]) {
    case 'A': rc = leg_A(&im); break;
    case 'B': rc = leg_B(&im); break;
    case 'C': rc = leg_C(&im); break;
    case 'E': rc = leg_E(&im); break;
    case 'G': rc = leg_G(&im); break;
    case 'H': rc = leg_H(&im); break;
    default:
        fprintf(stderr, "solid_check: unknown leg '%s'\n", argv[1]);
        rc = 2;
    }
    free(im.buf);
    return rc;
}
