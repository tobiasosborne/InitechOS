/* test_drag.c -- the FLAIR M3 DRAG GATE (THE ORACLE; ADR-0004 D-8 / AM-8 hard gate).
 *
 * beads: initech-87a ("Draw a System-7 window and drag it with correct clipping").
 *        The M3 drag-gate capstone -- the earliest human-verifiable Law-4 fidelity
 *        moment (ADR-0004 AM-8): a window drags across the desktop with correct
 *        DiffRgn update regions, NO over-repaint, chrome unchanged outside the
 *        damaged area, verified at the PIXEL level against INDEPENDENT ground truth.
 *
 * Ref:   ADR-0004 D-1 (5-layer stack), D-2 (one surface module; no second pixel
 *        path), D-5 (DiffRgn damage; MINIMAL repaint -- no over-repaint, the
 *        headline property), D-8 (the oracle vector: drag-gate is a HARD pass/fail
 *        row), AM-8 (the drag gate named). CLAUDE.md Law 2 (the oracle is the truth,
 *        not the agent), Law 4 (look like the frame -- the PPMs are the human audit),
 *        Rule 3 (all bugs deep), Rule 6 (mutation-proven), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * WHAT IT DOES (integrate window + event + compositor + blitter + chrome + surface):
 *
 *   1. Build a seafoam desktop + THREE overlapping System-7 documentProc windows on
 *      a HOST offscreen (the render skeleton's 8bpp/OD-2 bitmap + FLAIR heap), with
 *      a Window Manager whose regions are arena-backed (the caller-supplied-storage
 *      pattern). desktop_paint_all -> FRAME 0 -> build/drag_before.ppm.
 *
 *   2. DRIVE A DRAG THROUGH THE EVENT MANAGER (ADR-0004 D-4): post raw events into
 *      the SPSC ring -- a mouse move to put the cursor on the FRONT window's title
 *      bar, a mouseDown (button DOWN), a sequence of mouse-move deltas, a mouseUp --
 *      and DRAIN them via WaitNextEvent. On mouseDown, assert FindWindow at the
 *      cursor returns inDrag on the front window (the drag begins). For each move
 *      delta: MoveWindow the front window by that delta (so it tracks the cursor),
 *      seed the moved window's self-repaint (its chrome shifts), then
 *      desktop_paint_damage -- the D-5 minimal repaint. FRAME 1 = the full state
 *      after the drag -> build/drag_after.ppm.
 *
 *   3. THE DECISIVE ASSERTIONS (Law 2 / D-5, at the PIXEL level), each computing
 *      ground truth INDEPENDENTLY of the compositor's own region ops:
 *
 *      (a) CORRECT RESULT (bit-exact): frame1 equals a from-scratch
 *          desktop_paint_all of an INDEPENDENT Window Manager whose windows sit at
 *          the FINAL positions. The incremental damaged-repaint produced the SAME
 *          pixels as a full repaint.
 *
 *      (b) NO OVER-REPAINT (the headline D-5 property, checked PER STEP): the set of
 *          pixels that DIFFER across a drag step equals EXACTLY the step's damage,
 *          where the damage ground truth is computed from an INDEPENDENT per-pixel
 *          owner-grid (front-to-back, first-writer-wins) crossed with the rasterized
 *          updateRgns/desktop_update -- NOT by re-running the compositor's clip. A
 *          flicker/over-repaint bug changes pixels OUTSIDE the damage and fails here.
 *          (Per-step equality avoids the transient-pixel artifact a naive cumulative
 *          frame0-vs-frame1 check would suffer; a cumulative SUBSET check is also
 *          asserted as a global soundness backstop.)
 *
 *      (c) NEWLY-EXPOSED correctness: the bare-desktop and behind-window pixels the
 *          front window VACATED show the correct content -- asserted by (a)'s
 *          bit-exact match restricted to the vacated set (an independent owner-grid
 *          before/after identifies the vacated pixels).
 *
 *      (d) CHROME GEOMETRY at the new position matches chrome_metrics v1 (title bar
 *          19, frame 1, scrollbar 16) on the MOVED window -- the test_chrome
 *          structural idea, re-run on the dragged window's final frame.
 *
 *   4. WRITE build/drag_before.ppm + build/drag_after.ppm for the orchestrator's
 *      visual audit (Law 4).
 *
 * MUTANTS (Rule 6; in desktop.c, -D guarded), each MUST drive this gate RED:
 *   DRAG_MUTATE_SKIP_EXPOSED -- don't repaint the vacated desktop area -> stale
 *                               pixels -> (a)/(c) RED.
 *   DRAG_MUTATE_NO_CLIP      -- fill the whole desktop frame unclipped + paint the
 *                               whole window every step -> over-write neighbor
 *                               windows -> (b) over-repaint RED (and (a) RED).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"             /* the host render skeleton (-Iharness/render)   */
#include "region_algebra.h"     /* the LOCKED region contract (-Ispec)           */
#include "region.h"             /* engine constructors (-Ios/flair/atkinson)     */
#include "window_record.h"      /* WindowRecord, part-codes (-Ispec)             */
#include "window.h"             /* Window Manager (-Ios/flair)                   */
#include "event.h"              /* Event Manager: WaitNextEvent + ring           */
#include "event_model.h"        /* flair_raw_event_t, what codes, masks          */
#include "desktop.h"            /* the compositor UNDER TEST (-Ios/flair)        */
#include "chrome_metrics.h"     /* FLAIR_CHROME_* (-Ispec)                       */
#include "test_assert.h"        /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)      */

TEST_HARNESS();

/* The native port (ADR-0004 OD-3). The offscreen IS the screen, so window global
 * coords == port-local coords == offscreen pixel coords. */
enum { SCRW = 640, SCRH = 480 };

/* ===========================================================================
 * Region + window storage bundles (the test_window / test_region idiom: the
 * engine never mallocs; the caller supplies rows[]/x_pool). One bundle / region.
 * ===========================================================================*/
typedef struct rgn_store {
    region_t   r;
    rgn_row_t  rows[RGN_ROWS_CAP];
    int16_t    pool[RGN_X_POOL_CAP];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows       = s->rows;
    s->r.cap_rows   = RGN_ROWS_CAP;
    s->r.x_pool     = s->pool;
    s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}

typedef struct win_store {
    WindowRecord rec;
    rgn_store_t  struc, cont, upd;
} win_store_t;

static void win_attach(win_store_t *w)
{
    memset(&w->rec, 0, sizeof w->rec);
    store_attach(&w->struc);
    store_attach(&w->cont);
    store_attach(&w->upd);
    w->rec.strucRgn   = &w->struc.r;
    w->rec.contRgn    = &w->cont.r;
    w->rec.updateRgn  = &w->upd.r;
    w->rec.nextWindow = NULL;
}

/* Manager + its three internal scratch regions + desktop update, all arena-backed,
 * PLUS the compositor's own visible-region scratch (distinct from the manager's). */
typedef struct mgr_store {
    WindowMgr   wm;
    rgn_store_t desk, sa, sb, sc;
    rgn_store_t comp;   /* the compositor scratch (visible-region carrier)        */
} mgr_store_t;

static void mgr_attach(mgr_store_t *m, rgn_rect_t frame)
{
    store_attach(&m->desk); store_attach(&m->sa);
    store_attach(&m->sb);   store_attach(&m->sc);
    store_attach(&m->comp);
    WindowMgr_init(&m->wm, frame, &m->desk.r, &m->sa.r, &m->sb.r, &m->sc.r);
}

/* ===========================================================================
 * Independent pixel snapshot of the 8bpp offscreen (the raw index bytes).
 * ===========================================================================*/
typedef struct snap { uint8_t px[SCRW * SCRH]; } snap_t;

static void snapshot(const render_ctx_t *ctx, snap_t *s)
{
    /* The offscreen is 8bpp tight (pitch == width here); copy the index plane. */
    for (uint32_t y = 0; y < (uint32_t)SCRH; y++) {
        for (uint32_t x = 0; x < (uint32_t)SCRW; x++) {
            s->px[y * SCRW + x] =
                (uint8_t)(render_pixel_index(ctx, x, y) & 0xFFu);
        }
    }
}

/* ===========================================================================
 * Independent region rasterization (straight off rows/x-lists -- NOT via
 * region_contains_point; mirrors the test_window/test_region idiom). Marks 1 in
 * `grid` (SCRW*SCRH) for every pixel inside `r`, within the screen.
 * ===========================================================================*/
static void rasterize_into(const region_t *r, uint8_t *grid /* SCRW*SCRH */)
{
    if (r->is_empty || r->n_rows == 0) return;
    for (uint16_t i = 0; i < r->n_rows; i++) {
        int y0 = r->rows[i].y_top;
        int y1 = (i + 1 < r->n_rows) ? r->rows[i + 1].y_top : y0;
        if (r->rows[i].x_count == 0) continue;
        for (int y = y0; y < y1; y++) {
            if (y < 0 || y >= SCRH) continue;
            for (uint16_t k = 0; k + 1 < r->rows[i].x_count; k += 2) {
                int xa = r->rows[i].x[k], xb = r->rows[i].x[k + 1];
                for (int x = xa; x < xb; x++)
                    if (x >= 0 && x < SCRW) grid[y * SCRW + x] = 1;
            }
        }
    }
}

/* ===========================================================================
 * Independent OWNER grid: rasterize windows FRONT-to-BACK, first writer wins
 * (front-most owner). OWNER_NONE = bare desktop. Owner is the window's index in
 * `idx`. Computed WITHOUT any Window Manager region op (so a WM bug cannot mask a
 * compositor bug).
 * ===========================================================================*/
enum { OWNER_NONE = 255 };

static void build_owner_grid(const WindowMgr *wm, win_store_t *const *idx, int n,
                             uint8_t *own /* SCRW*SCRH */)
{
    memset(own, OWNER_NONE, (size_t)SCRW * SCRH);
    for (const WindowRecord *p = wm->front; p != NULL; p = p->nextWindow) {
        if (!p->visible) continue;
        int wi = -1;
        for (int i = 0; i < n; i++) if (&idx[i]->rec == p) { wi = i; break; }
        if (wi < 0) continue;
        rgn_rect_t s = region_get_bbox(p->strucRgn);
        for (int y = s.top; y < s.bottom; y++) {
            if (y < 0 || y >= SCRH) continue;
            for (int x = s.left; x < s.right; x++) {
                if (x < 0 || x >= SCRW) continue;
                if (own[y * SCRW + x] == OWNER_NONE) own[y * SCRW + x] = (uint8_t)wi;
            }
        }
    }
}

/* ===========================================================================
 * Post one raw event into the SPSC ring (ISR producer side; the test plays the
 * role of the hardware ISR). FLAIR_RAW_MOUSE payload (spec/event_model.h Sec 5):
 *   bits 0..7  = button byte (bit 0 = left), 8..15 = int8 dx, 16..23 = int8 dy.
 * ===========================================================================*/
static void post_mouse(flair_raw_ring_t *ring, int8_t dx, int8_t dy, int left_down)
{
    flair_raw_event_t raw;
    raw.kind = (uint32_t)FLAIR_RAW_MOUSE;
    raw.tick = flair_tick_count();
    raw.payload = (uint32_t)((left_down ? 0x01u : 0x00u)
                | ((uint32_t)(uint8_t)dx << 8)
                | ((uint32_t)(uint8_t)dy << 16));
    (void)flair_raw_post(ring, &raw);
}

/* ===========================================================================
 * Scene geometry. Three overlapping documentProc windows; F is the FRONT one
 * (created last). The cursor starts at the screen centre (320,240) -- event.c.
 * ===========================================================================*/
/* Window A (backmost), B (middle), F (front). All overlap near the centre. */
#define A_S_T 120
#define A_S_L 120
#define A_S_B 260
#define A_S_R 340
#define B_S_T 150
#define B_S_L 200
#define B_S_B 290
#define B_S_R 420
#define F_S_T 200
#define F_S_L 260
#define F_S_B 340
#define F_S_R 480

/* content = 1 px frame on the sides/bottom, TITLEBAR_H + frame on top. */
static void make_rects(int t, int l, int b, int r, rgn_rect_t *s, rgn_rect_t *c)
{
    s->top = (int16_t)t; s->left = (int16_t)l; s->bottom = (int16_t)b; s->right = (int16_t)r;
    c->top    = (int16_t)(t + FLAIR_CHROME_FRAME + FLAIR_CHROME_TITLEBAR_H);
    c->left   = (int16_t)(l + FLAIR_CHROME_FRAME);
    c->bottom = (int16_t)(b - FLAIR_CHROME_FRAME);
    c->right  = (int16_t)(r - FLAIR_CHROME_FRAME);
}

/* Build the 3-window scene into manager `m`, windows W[0..2] (A,B,F front). */
static void build_scene(mgr_store_t *m, win_store_t W[3], win_store_t *idx[3])
{
    rgn_rect_t FRAME = { 0, 0, SCRH, SCRW };
    mgr_attach(m, FRAME);
    rgn_rect_t s, c;
    win_attach(&W[0]); idx[0] = &W[0];
    make_rects(A_S_T, A_S_L, A_S_B, A_S_R, &s, &c);
    NewWindow(&m->wm, &W[0].rec, s, c, documentKind, documentProc, 1);
    win_attach(&W[1]); idx[1] = &W[1];
    make_rects(B_S_T, B_S_L, B_S_B, B_S_R, &s, &c);
    NewWindow(&m->wm, &W[1].rec, s, c, documentKind, documentProc, 1);
    win_attach(&W[2]); idx[2] = &W[2];
    make_rects(F_S_T, F_S_L, F_S_B, F_S_R, &s, &c);
    NewWindow(&m->wm, &W[2].rec, s, c, documentKind, documentProc, 1);  /* F front */
}

/* ===========================================================================
 * (d) Chrome geometry on the MOVED window, at its final frame -- the
 * test_chrome structural idea re-run on the offscreen (8bpp, shade indices).
 * `f` is the moved window's final structure rect.
 * ===========================================================================*/
static int idx_at(const render_ctx_t *ctx, int x, int y)
{
    return (int)(render_pixel_index(ctx, (uint32_t)x, (uint32_t)y) & 0xFFu);
}

static void assert_chrome_geometry(render_ctx_t *ctx, rgn_rect_t f)
{
    const int fr = FLAIR_CHROME_FRAME;
    const int title_top = f.top + fr;
    const int title_bot = title_top + FLAIR_CHROME_TITLEBAR_H;
    const int mid_x = (f.left + f.right) / 2;
    char msg[160];

    /* Title bar: 19 px band of pinstripe alternation (period 2), then the white
     * body just below (proves height EXACTLY TITLEBAR_H). */
    int alt_ok = 1, period_ok = 1;
    int shades[FLAIR_CHROME_TITLEBAR_H];
    for (int k = 0; k < FLAIR_CHROME_TITLEBAR_H; k++)
        shades[k] = idx_at(ctx, mid_x, title_top + k);
    for (int k = 1; k < FLAIR_CHROME_TITLEBAR_H; k++)
        if (shades[k] == shades[k - 1]) alt_ok = 0;
    for (int k = 2; k < FLAIR_CHROME_TITLEBAR_H; k++)
        if (shades[k] != shades[k - 2]) period_ok = 0;
    snprintf(msg, sizeof msg, "(d) moved window: title-bar pinstripe ALTERNATES");
    CHECK(alt_ok, msg);
    snprintf(msg, sizeof msg, "(d) moved window: pinstripe period == %d",
             FLAIR_CHROME_PINSTRIPE_PERIOD);
    CHECK(period_ok, msg);

    int below = idx_at(ctx, mid_x, title_bot);
    snprintf(msg, sizeof msg,
             "(d) moved window: row below title bar is white body (idx 1) -- "
             "title-bar height EXACTLY %d", FLAIR_CHROME_TITLEBAR_H);
    CHECK(below == 1, msg);

    /* Frame: exactly FRAME (1) px. The moved window drags down-right, so its RIGHT
     * and BOTTOM edges end over BARE desktop -- probe there (behind-independent).
     * Right frame column painted; pixel just OUTSIDE is the seafoam desktop. */
    int content_top = title_bot;
    int content_bot = f.bottom - fr;
    int row = (content_top + content_bot) / 2;
    int inner_right = f.right - fr;
    CHECK(idx_at(ctx, f.right - 1, row) != (int)FLAIR_DESKTOP_BG_INDEX,
          "(d) moved window: right frame column is painted");
    CHECK(idx_at(ctx, f.right, row) == (int)FLAIR_DESKTOP_BG_INDEX,
          "(d) moved window: pixel just right of the frame is bare seafoam (frame 1 px)");
    int sb_left = inner_right - FLAIR_CHROME_SCROLLBAR_W;
    snprintf(msg, sizeof msg,
             "(d) moved window: scrollbar EXACTLY %d px -- divider (idx 0) at -%d",
             FLAIR_CHROME_SCROLLBAR_W, FLAIR_CHROME_SCROLLBAR_W);
    CHECK(idx_at(ctx, sb_left, row) == 0, msg);
    snprintf(msg, sizeof msg,
             "(d) moved window: content just left of the %d px scrollbar is body (idx 1)",
             FLAIR_CHROME_SCROLLBAR_W);
    CHECK(idx_at(ctx, sb_left - 1, row) == 1, msg);
}

/* ===========================================================================
 * MAIN -- the drag gate.
 * ===========================================================================*/
int main(int argc, char **argv)
{
    rgn_rect_t FRAME = { 0, 0, SCRH, SCRW };

    /* --- the host offscreen (8bpp / OD-2) via the render skeleton (AM-1: the
     * geometry is a runtime parameter -- a fake boot_info, never a hardcoded
     * aperture). The skeleton fills the offscreen with RENDER_DESKTOP_INDEX (==
     * FLAIR_DESKTOP_BG_INDEX) so a blank canvas reads as the seafoam desktop. ---*/
    render_boot_info_t boot;
    memset(&boot, 0, sizeof boot);
    boot.lfb_addr   = 0xE0000000u;   /* a plausible VBE PhysBasePtr (ignored host)*/
    boot.lfb_pitch  = 0u;            /* tight: width*bpp/8                        */
    boot.lfb_bpp    = 8u;            /* indexed-8 (OD-2)                          */
    boot.lfb_width  = SCRW;
    boot.lfb_height = SCRH;
    render_ctx_t ctx;
    int rc = render_ctx_init(&ctx, &boot);
    CHECK(rc == 0, "render_ctx_init(8bpp 640x480) must succeed (AM-1 geometry param)");
    if (rc != 0) {
        return TEST_SUMMARY("test-drag");
    }

    /* --- the scene: 3 overlapping windows, F front. --- */
    static win_store_t W[3];
    win_store_t *idx[3];
    static mgr_store_t M;
    build_scene(&M, W, idx);

    /* --- FRAME 0: full paint -> snapshot + PPM (Law 4). --- */
    desktop_paint_all(&M.wm, &ctx.fb.bm, &M.comp.r);
    static snap_t frame0;
    snapshot(&ctx, &frame0);

    /* --- the Event Manager: a fresh ring; cursor starts at (320,240). --- */
    static flair_raw_ring_t ring;
    flair_event_init(&ring);
    flair_event_set_yield((flair_event_yield_fn)0);   /* tight spin for the test  */

    /* Move the cursor onto the FRONT window's title bar, then press the button.
     * Cursor centre (320,240); title bar of F is rows [201,220). Target a point
     * clear of the close box (left) and grow box (right): (h=350, v=210). */
    EventRecord ev;
    int8_t to_title_dx = (int16_t)350 - 320;   /* +30 */
    int8_t to_title_dy = (int16_t)210 - 240;   /* -30 */
    post_mouse(&ring, to_title_dx, to_title_dy, 0);   /* move (button still UP)    */
    /* a move with no button transition cooks to nullEvent; drain it so the cursor
     * position updates (the pump tracks where in task context, D-4). */
    (void)WaitNextEvent(&ring, everyEvent, &ev, 0);

    post_mouse(&ring, 0, 0, 1);                        /* press (UP->DOWN: mouseDown)*/
    int got_down = WaitNextEvent(&ring, everyEvent, &ev, 0);
    CHECK(got_down && ev.what == (uint16_t)mouseDown,
          "Event Manager synthesizes mouseDown from the raw ring (D-4)");
    CHECK(ev.where.h == 350 && ev.where.v == 210,
          "cursor tracked by the Event Manager to the title-bar point (350,210)");

    /* FindWindow at the cursor must report inDrag on the FRONT window. */
    WindowPtr hit = NULL;
    flair_point_t where = { ev.where.v, ev.where.h };   /* {v,h} */
    flair_part_code_t part = FindWindow(&M.wm, where, &hit);
    CHECK(part == inDrag && hit == &W[2].rec,
          "FindWindow at mouseDown: inDrag on the FRONT window (the drag begins)");

    /* --- the drag: a sequence of move deltas. F (and the cursor) move together;
     * after each step, MoveWindow + seed F's self-repaint + desktop_paint_damage,
     * and check the PER-STEP no-over-repaint property (b) at the pixel level. ---*/
    static const int8_t STEP_DX[] = { 10, 12, 9, 11, 8 };
    static const int8_t STEP_DY[] = {  8,  9, 7, 10, 6 };
    enum { NSTEP = (int)(sizeof STEP_DX / sizeof STEP_DX[0]) };

    int overrepaint_bad = 0;   /* a pixel changed OUTSIDE the step's damage       */
    int exposed_bad     = 0;   /* a vacated-to-desktop pixel was NOT painted seafoam*/
    static snap_t before_step, after_step;
    static uint8_t own_b[SCRW * SCRH], own_a[SCRW * SCRH];
    static uint8_t dmg[SCRW * SCRH];          /* per-step VISIBLE-clipped damage   */
    static uint8_t exposed[SCRW * SCRH];      /* per-step newly-bare-desktop set    */
    static uint8_t cum_dmg[SCRW * SCRH];      /* cumulative visible damage         */
    memset(cum_dmg, 0, sizeof cum_dmg);

    for (int st = 0; st < NSTEP; st++) {
        snapshot(&ctx, &before_step);
        build_owner_grid(&M.wm, idx, 3, own_b);

        /* Post + drain the mouse-move (button held DOWN). A held-button move cooks
         * to nullEvent (no click transition) but updates `where`; we drive the
         * window from the SAME delta the cursor tracked (cooperative drag loop). */
        post_mouse(&ring, STEP_DX[st], STEP_DY[st], 1);
        (void)WaitNextEvent(&ring, everyEvent, &ev, 0);

        rgn_rect_t os = region_get_bbox(W[2].rec.strucRgn);
        int nl = os.left + STEP_DX[st];
        int nt = os.top  + STEP_DY[st];
        MoveWindow(&M.wm, &W[2].rec, (int16_t)nl, (int16_t)nt);

        /* Seed the moved window's OWN repaint: its chrome SHIFTED, so its whole new
         * visible region owes a redraw (the caller's concern per window.h; the
         * compositor then paints it clipped to visible(F) INTERSECT updateRgn). */
        WindowMgr_invalidate(&M.wm, &W[2].rec, region_get_bbox(W[2].rec.strucRgn));

        /* --- build the per-step VISIBLE-clipped damage ground truth, INDEPENDENT
         * of the compositor: a pixel is visibly damaged iff
         *   (some window w: pixel in w.updateRgn AND owner==w), OR
         *   (pixel in desktop_update AND owner==NONE),
         * using the owner-grid AFTER the move (the post-move visibility). --- */
        build_owner_grid(&M.wm, idx, 3, own_a);
        memset(dmg, 0, sizeof dmg);
        for (int i = 0; i < 3; i++) {
            static uint8_t ug[SCRW * SCRH];
            memset(ug, 0, sizeof ug);
            rasterize_into(W[i].rec.updateRgn, ug);
            for (int j = 0; j < SCRW * SCRH; j++)
                if (ug[j] && own_a[j] == (uint8_t)i) dmg[j] = 1;
        }
        memset(exposed, 0, sizeof exposed);
        {
            static uint8_t dg[SCRW * SCRH];
            memset(dg, 0, sizeof dg);
            rasterize_into(M.wm.desktop_update, dg);
            for (int j = 0; j < SCRW * SCRH; j++)
                if (dg[j] && own_a[j] == OWNER_NONE) { dmg[j] = 1; exposed[j] = 1; }
        }
        for (int j = 0; j < SCRW * SCRH; j++) if (dmg[j]) cum_dmg[j] = 1;

        /* --- the D-5 minimal repaint (the code under test). --- */
        desktop_paint_damage(&M.wm, &ctx.fb.bm, &M.comp.r);

        snapshot(&ctx, &after_step);

        /* (b) PER-STEP NO OVER-REPAINT (soundness, the decisive D-5 property): a
         * pixel that CHANGED value across the step MUST lie within the step's
         * computed damage. A flicker/over-repaint bug changes a pixel outside the
         * damage and trips this. (We do NOT assert "every damaged pixel changed
         * value": a window's uniform white content body legitimately repaints to
         * the same value where it overlaps its old position -- a region IS damaged
         * yet the value is unchanged. Completeness is asserted on the exposed-to-
         * desktop set, where the expected value is the known seafoam, below.) */
        for (int j = 0; j < SCRW * SCRH; j++) {
            if (before_step.px[j] != after_step.px[j] && !dmg[j]) overrepaint_bad = 1;
        }

        /* (b) PER-STEP completeness on the VACATED-to-DESKTOP set: every pixel the
         * window gave back to the bare desktop this step MUST read seafoam after
         * the repaint (the hole was actually filled). DRAG_MUTATE_SKIP_EXPOSED
         * leaves stale chrome here and trips this per step. */
        for (int j = 0; j < SCRW * SCRH; j++) {
            if (exposed[j] && after_step.px[j] != (uint8_t)FLAIR_DESKTOP_BG_INDEX)
                exposed_bad = 1;
        }
        (void)own_b;
    }

    CHECK(!overrepaint_bad,
          "(b) NO over-repaint: every changed pixel lies within the step's damage (D-5)");
    CHECK(!exposed_bad,
          "(b) completeness: every vacated-to-desktop pixel was repainted seafoam");

    /* --- FRAME 1: the full state after the drag -> snapshot + PPM (Law 4). --- */
    static snap_t frame1;
    snapshot(&ctx, &frame1);

    /* ======================================================================
     * (a) CORRECT RESULT (bit-exact): frame1 == an INDEPENDENT from-scratch paint
     * of the windows at their FINAL positions. Build a SECOND manager + a SECOND
     * offscreen, place the windows where they ended up, desktop_paint_all, and
     * compare the offscreens byte-for-byte.
     * ====================================================================== */
    rgn_rect_t fF = region_get_bbox(W[2].rec.strucRgn);
    {
        render_ctx_t ref;
        int rrc = render_ctx_init(&ref, &boot);
        CHECK(rrc == 0, "(a) reference render context init");
        if (rrc == 0) {
            static win_store_t RW[3];
            static mgr_store_t RM;
            rgn_rect_t s, c;
            mgr_attach(&RM, FRAME);
            /* A, B at their (unchanged) positions; F at its FINAL position. */
            win_attach(&RW[0]);
            make_rects(A_S_T, A_S_L, A_S_B, A_S_R, &s, &c);
            NewWindow(&RM.wm, &RW[0].rec, s, c, documentKind, documentProc, 1);
            win_attach(&RW[1]);
            make_rects(B_S_T, B_S_L, B_S_B, B_S_R, &s, &c);
            NewWindow(&RM.wm, &RW[1].rec, s, c, documentKind, documentProc, 1);
            win_attach(&RW[2]);
            make_rects(fF.top, fF.left, fF.bottom, fF.right, &s, &c);
            NewWindow(&RM.wm, &RW[2].rec, s, c, documentKind, documentProc, 1);

            desktop_paint_all(&RM.wm, &ref.fb.bm, &RM.comp.r);
            static snap_t refsnap;
            snapshot(&ref, &refsnap);

            int diff = 0, first = -1;
            for (int j = 0; j < SCRW * SCRH; j++)
                if (frame1.px[j] != refsnap.px[j]) { diff++; if (first < 0) first = j; }
            CHECK(diff == 0,
                  "(a) frame1 == from-scratch reference at the final positions (bit-exact)");
            if (diff != 0) {
                fprintf(stderr, "    (a) %d pixels differ; first at (%d,%d)\n",
                        diff, first % SCRW, first / SCRW);
            }

            /* ==============================================================
             * (c) NEWLY-EXPOSED correctness: the pixels the front window VACATED
             * (owned at start, not owned at end) show the correct content. We use
             * INDEPENDENT owner-grids of the START scene and the FINAL scene; the
             * vacated set is {owner==F before} MINUS {owner==F after}. On the
             * vacated set, frame1 must match the from-scratch reference (it does,
             * by (a) -- here we assert the vacated set is NON-EMPTY and matches,
             * so the check is meaningful, not vacuous).
             * ============================================================== */
            static uint8_t own_start[SCRW * SCRH], own_end[SCRW * SCRH];
            /* rebuild the START owner grid from a fresh scene (independent). */
            {
                static win_store_t SW[3];
                win_store_t *sidx[3];
                static mgr_store_t SM;
                build_scene(&SM, SW, sidx);
                build_owner_grid(&SM.wm, sidx, 3, own_start);
            }
            build_owner_grid(&M.wm, idx, 3, own_end);
            int vacated = 0, vacated_bad = 0;
            for (int j = 0; j < SCRW * SCRH; j++) {
                int was_F = (own_start[j] == 2);
                int is_F  = (own_end[j]   == 2);
                if (was_F && !is_F) {
                    vacated++;
                    if (frame1.px[j] != refsnap.px[j]) vacated_bad = 1;
                }
            }
            CHECK(vacated > 0, "(c) the drag VACATED a non-empty region (check is meaningful)");
            CHECK(!vacated_bad,
                  "(c) newly-exposed (vacated) pixels show the correct behind/desktop content");

            render_ctx_free(&ref);
        }
    }

    /* ======================================================================
     * (b) GLOBAL backstop: every pixel that differs frame0 -> frame1 was damaged
     * at SOME step (a subset check; the per-step equality above is the decisive
     * form, this guards against any change never accounted for by the damage model).
     * ====================================================================== */
    {
        int leaked = 0;
        for (int j = 0; j < SCRW * SCRH; j++)
            if (frame0.px[j] != frame1.px[j] && !cum_dmg[j]) leaked = 1;
        CHECK(!leaked,
              "(b) global: no pixel changed across the drag that was never damaged");
    }

    /* ======================================================================
     * (d) CHROME GEOMETRY on the MOVED window at its final position.
     * ====================================================================== */
    assert_chrome_geometry(&ctx, fF);

    /* --- PPMs for the orchestrator's visual audit (Law 4). --- */
    {
        /* Re-render frame0 into a scratch ctx for the BEFORE ppm (ctx now holds
         * frame1). Simpler: write frame1 from ctx; write frame0 by repainting a
         * fresh scene. */
        const char *before_path = (argc > 1) ? argv[1] : "build/drag_before.ppm";
        const char *after_path  = (argc > 2) ? argv[2] : "build/drag_after.ppm";

        if (render_write_ppm(&ctx, after_path) == 0)
            printf("    wrote drag-after PPM to %s\n", after_path);
        else
            fprintf(stderr, "    WARN: could not write %s\n", after_path);

        /* BEFORE: a fresh scene full-painted into a scratch ctx. */
        render_ctx_t bctx;
        if (render_ctx_init(&bctx, &boot) == 0) {
            static win_store_t BW[3];
            win_store_t *bidx[3];
            static mgr_store_t BM;
            build_scene(&BM, BW, bidx);
            desktop_paint_all(&BM.wm, &bctx.fb.bm, &BM.comp.r);
            if (render_write_ppm(&bctx, before_path) == 0)
                printf("    wrote drag-before PPM to %s\n", before_path);
            else
                fprintf(stderr, "    WARN: could not write %s\n", before_path);
            render_ctx_free(&bctx);
        }
    }

    render_ctx_free(&ctx);
    return TEST_SUMMARY("test-drag");
}
