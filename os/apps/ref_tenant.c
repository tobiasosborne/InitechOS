/*
 * os/apps/ref_tenant.c -- HELLO + NOTES, the two reference co-resident FLAIR
 * tenants (THE ARTIFACT; C, Law 3; host- AND freestanding-buildable).
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013) + amendment bead
 *        initech-fka6 (Wave-4 Step 1). These two tenants are the live demo scene
 *        behind -DFLAIR_LIVE_TENANTS: kmain launches them through FlairProcess_launch
 *        and routes every event through flair_app_dispatch / flair_route_updates
 *        (the SOLE routing spine), and the O-5 test-flair-appswitch grader probes the
 *        composited result at the points spec/flair_tenants_demo.h fixes.
 *
 * THE DESIGN (ADR-0013 Sec 3.1 tenant ABI + the demo contract):
 *
 *   - Each tenant is a const FlairAppProcs vtable (hello_procs / notes_procs) over a
 *     PER-INSTANCE private state struct (tenant_priv_t) that open() carves from
 *     self->arena (flair_alloc) and stashes in self->userData -- the FULL-WIDTH
 *     pointer slot added by bead fka6. event(self, ev) recovers it via self->userData,
 *     so the two tenants SHARE one event handler and differ only by their static demo
 *     descriptor (tenant_cfg_t: bounds + content FILL canon index).
 *
 *   - open(): allocate the private state + a WindowRecord + four region bundles
 *     (strucRgn / contRgn / updateRgn + a clip scratch, each rows[]/x_pool attached)
 *     from self->arena; compute the structure bounds for THIS tenant from
 *     spec/flair_tenants_demo.h and the content rect by the documentProc chrome inset
 *     (the shell.c convention: 1px frame + 19px title bar); NewWindow(lp->wm, ...); set
 *     w->refCon = (int32_t)(uintptr_t)self (the Sec 3.1 binding / demux rule); stash
 *     lp->surface / lp->wm / content in the private state; do the INITIAL content FILL
 *     on lp->surface over the content rect, clipped to the content region.
 *
 *   - event(): on updateEvt repaint the content (FILL + accent-if-active + marker-if-
 *     toggled) clipped to (contRgn INTERSECT updateRgn) -- THIS is the exposure-survival
 *     path: when HELLO is raised and re-exposes the overlap NOTES was covering,
 *     flair_route_updates hands HELLO an updateEvt and HELLO repaints the overlap in its
 *     FILL; the drop-updateEvt mutant skips it and the overlap keeps NOTES' colour (RED).
 *     On activateEvt set active from FLAIR_EVT_MOD_ACTIVE_FLAG and redraw the accent
 *     block (present when active, FILL when not) clipped to content. On mouseDown (the
 *     dispatcher delivers only inContent clicks to the owner) toggle a content marker --
 *     proof the click reached the RIGHT tenant. Every draw is clipped to the content
 *     region: a tenant never overdraws chrome (the Window Manager owns the title bar).
 *
 *   - idle / close: NULL (no caret blink; the shell disposes windows + resets the arena
 *     on teardown regardless, ADR-0013 Sec 3.4).
 *
 * COLOR (mechanism/policy C-8). The blitter takes a packed destination VALUE, not a
 * canon index. These tenants name NO color (no 0xRRGGBB literal): they carry the demo
 * header's canon INDICES (FLAIR_TEN_*_FILL / FLAIR_TEN_ACTIVE_ACCENT) and resolve each
 * to a depth-correct pixel through the ONE FLAIR policy seam flair_look_pixel_depth
 * (the same seam desktop_px / chrome_px use) -- index -> FLAIR PART -> flair_canon_rgb,
 * device-quantized for the surface depth (8bpp index byte / else packed 0x00RRGGBB).
 * The three demo indices map 1:1 onto the PARTs that resolve to the SAME canon index
 * (flair_look.h enum): CIDX_WHITE->CONTENT, CIDX_CONTROL->BTNFACE, CIDX_ACCENT->
 * CAPTION_NAVY. flair_look.c (with the device CLUT) is the ONLY sanctioned index->color
 * site (flair_look.h), so the tenants route through it rather than calling
 * flair_canon_rgb directly.
 *
 * FREESTANDING (Law 3): <stdint.h> + the locked spec headers + the FLAIR module headers
 * only; no host libc, no malloc (all storage is carved from self->arena). Dual-compiles
 * under the kernel flags and hosted (the surface.c / window.c pattern).
 *
 * Ref: ADR-0013 Sec 3.1 (tenant ABI + binding rule), Sec 3.3 (event routing /
 *      activation), amendment bead initech-fka6; spec/flair_tenants_demo.h (the demo
 *      layout + canon indices + probe points); os/flair/process.h (FlairAppProcs,
 *      FlairApp.userData, FlairLaunchParams.surface/wm); os/flair/window.h (NewWindow,
 *      contRgn/updateRgn); os/flair/blitter.h (blitter_fill_rect_clipped -- packed
 *      value + region clip); os/flair/flair_look.h (flair_look_pixel_depth, the C-8
 *      seam); spec/chrome_metrics.h (FLAIR_CHROME_FRAME / FLAIR_CHROME_TITLEBAR_H);
 *      spec/region_algebra.h (region_op INTERSECT). CLAUDE.md Law 2, Law 3, Rule 2,
 *      Rule 11, Rule 12.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stdint.h>

#include "ref_tenant.h"           /* hello_procs / notes_procs (-Ios/apps)         */
#include "process.h"              /* FlairApp, FlairAppProcs, FlairLaunchParams     */
#include "window.h"               /* NewWindow, WindowMgr, WindowPtr (-Ios/flair)  */
#include "window_record.h"        /* WindowRecord, documentKind/documentProc        */
#include "region_algebra.h"       /* region_t, rgn_rect_t, region_op, INTERSECT      */
#include "blitter.h"              /* blitter_fill_rect_clipped (-Ios/flair)        */
#include "surface.h"              /* bitmap_t (-Ios/flair)                          */
#include "heap.h"                 /* flair_alloc, FLAIR_CLASS_* (-Ios/flair)       */
#include "flair_look.h"           /* flair_look_pixel_depth, FLAIR_PART_* (the C-8 seam) */
#include "event_model.h"          /* EventRecord, updateEvt/activateEvt/mouseDown   */
#include "chrome_metrics.h"       /* FLAIR_CHROME_FRAME / FLAIR_CHROME_TITLEBAR_H   */
#include "flair_tenants_demo.h"   /* the SHARED demo layout + canon indices (-Ispec) */

/* ---------------------------------------------------------------------------
 * Per-region arena bundle (the engine never mallocs; the CALLER supplies
 * rows[]/x_pool -- the test_interact / test_process region-backing idiom). A
 * rectangular document window + a content-INTERSECT-updateRgn clip stay well
 * under these caps; a cap overflow FAILS LOUD in the engine (Rule 2).
 * ------------------------------------------------------------------------- */
enum { TEN_ROWS = 32, TEN_POOL = 128 };

typedef struct ten_rgn {
    region_t  r;
    rgn_row_t rows[TEN_ROWS];
    int16_t   pool[TEN_POOL];
} ten_rgn_t;

/* ---------------------------------------------------------------------------
 * tenant_cfg_t -- the static per-tenant demo descriptor. All values come from
 * spec/flair_tenants_demo.h so the tenants match that locked layout EXACTLY.
 * ------------------------------------------------------------------------- */
typedef struct tenant_cfg {
    uint8_t  fill_idx;       /* content FILL canon index (FLAIR_TEN_*_FILL)         */
    uint8_t  accent_idx;     /* active-accent canon index (FLAIR_TEN_ACTIVE_ACCENT) */
    int16_t  bnd_l, bnd_t, bnd_r, bnd_b;  /* structure bounds (demo l,t,r,b)        */
    uint8_t  accent_graded;  /* 1 -> accent block at the GRADED demo probe (HELLO)  */
    int16_t  accent_x, accent_y;          /* graded accent top-left (HELLO only)     */
} tenant_cfg_t;

static const tenant_cfg_t HELLO_CFG = {
    (uint8_t)FLAIR_TEN_HELLO_FILL, (uint8_t)FLAIR_TEN_ACTIVE_ACCENT,
    FLAIR_TEN_HELLO_L, FLAIR_TEN_HELLO_T, FLAIR_TEN_HELLO_R, FLAIR_TEN_HELLO_B,
    1, FLAIR_TEN_HELLO_ACCENT_X, FLAIR_TEN_HELLO_ACCENT_Y
};
static const tenant_cfg_t NOTES_CFG = {
    (uint8_t)FLAIR_TEN_NOTES_FILL, (uint8_t)FLAIR_TEN_ACTIVE_ACCENT,
    FLAIR_TEN_NOTES_L, FLAIR_TEN_NOTES_T, FLAIR_TEN_NOTES_R, FLAIR_TEN_NOTES_B,
    0, 0, 0   /* NOTES has no graded accent probe -> accent at content top-left      */
};

/* ---------------------------------------------------------------------------
 * tenant_priv_t -- the per-INSTANCE private state (carved from self->arena in
 * open(); pointer stored in self->userData; recovered in event() as
 * self->userData). The prescribed shape (surface/wm/win/content/active/toggled)
 * plus the static descriptor + the clip scratch the updateEvt repaint needs.
 * ------------------------------------------------------------------------- */
typedef struct tenant_priv {
    const bitmap_t      *surface;   /* offscreen the tenant draws into (lp->surface) */
    WindowMgr           *wm;        /* the shell Window Manager (lp->wm)             */
    WindowPtr            win;       /* this tenant's window (refCon-bound to self)   */
    rgn_rect_t           content;   /* content rect, global coords                   */
    int                  active;    /* foreground/active (activateEvt active-flag)    */
    int                  toggled;   /* mouseDown content-marker toggle               */
    const tenant_cfg_t  *cfg;       /* this tenant's static demo descriptor          */
    region_t            *clip;      /* scratch: contRgn INTERSECT updateRgn          */
} tenant_priv_t;

/* ---------------------------------------------------------------------------
 * Small freestanding helpers (no libc -- the heap.c / process.c discipline).
 * ------------------------------------------------------------------------- */
static void zero_bytes(void *dst, uint32_t n)
{
    uint8_t *b = (uint8_t *)dst;
    for (uint32_t i = 0u; i < n; i++) b[i] = 0u;
}

static void rgn_attach(ten_rgn_t *s)
{
    s->r.rows       = s->rows;
    s->r.cap_rows   = TEN_ROWS;
    s->r.x_pool     = s->pool;
    s->r.x_pool_cap = TEN_POOL;
    region_set_empty(&s->r);   /* sets is_empty/n_rows/bbox/x_pool_used (no memset) */
}

/* mk_rect -- rgn_rect_t in QuickDraw field order {top,left,bottom,right}. */
static rgn_rect_t mk_rect(int16_t left, int16_t top, int16_t right, int16_t bottom)
{
    rgn_rect_t r;
    r.top = top; r.left = left; r.bottom = bottom; r.right = right;
    return r;
}

/* ---------------------------------------------------------------------------
 * demo_px -- resolve a demo canon INDEX to the destination pixel through the
 * ONE FLAIR policy seam (flair_look_pixel_depth), depth-correct, naming NO
 * color (C-8). The demo's three canon indices map 1:1 onto FLAIR PARTs that
 * resolve to the SAME canon index (flair_look.h enum):
 *     CIDX_WHITE   == FLAIR_TEN_*_FILL (HELLO)  -> FLAIR_PART_CONTENT
 *     CIDX_CONTROL == FLAIR_TEN_*_FILL (NOTES)  -> FLAIR_PART_BTNFACE
 *     CIDX_ACCENT  == FLAIR_TEN_ACTIVE_ACCENT   -> FLAIR_PART_CAPTION_NAVY
 * ------------------------------------------------------------------------- */
static uint32_t demo_px(const bitmap_t *bm, uint8_t cidx)
{
    int part;
    switch (cidx) {
        case CIDX_WHITE:   part = (int)FLAIR_PART_CONTENT;      break;
        case CIDX_CONTROL: part = (int)FLAIR_PART_BTNFACE;      break;
        case CIDX_ACCENT:  part = (int)FLAIR_PART_CAPTION_NAVY; break;
        default:           part = (int)FLAIR_PART_CONTENT;      break; /* unreachable in the demo */
    }
    return flair_look_pixel_depth(bm->bpp, part);
}

/* draw_block -- fill `r` with the surface value for canon index `cidx`, clipped
 * to `clip`. Fail-soft on a NULL surface (the stub tenants draw nothing). */
static void draw_block(const tenant_priv_t *p, rgn_rect_t r, uint8_t cidx,
                       const region_t *clip)
{
    if (p->surface == (const bitmap_t *)0) return;
    blitter_fill_rect_clipped(p->surface, r, demo_px(p->surface, cidx), clip);
}

/* accent_rect -- the active-accent block (FLAIR_TEN_ACCENT_SIZE square). HELLO
 * uses the GRADED demo probe location (FLAIR_TEN_HELLO_ACCENT_*); NOTES (not
 * graded) places it at its content top-left. */
static rgn_rect_t accent_rect(const tenant_priv_t *p)
{
    int16_t left = p->cfg->accent_graded ? p->cfg->accent_x : p->content.left;
    int16_t top  = p->cfg->accent_graded ? p->cfg->accent_y : p->content.top;
    return mk_rect(left, top,
                   (int16_t)(left + FLAIR_TEN_ACCENT_SIZE),
                   (int16_t)(top  + FLAIR_TEN_ACCENT_SIZE));
}

/* marker_rect -- the mouseDown click marker (FLAIR_TEN_ACCENT_SIZE square),
 * centered in the content so it never collides with the accent block (content
 * top-left) or the overlap probe. Not graded; proves click routing reached
 * the right tenant. */
static rgn_rect_t marker_rect(const tenant_priv_t *p)
{
    int16_t cx = (int16_t)(p->content.left +
                           (p->content.right - p->content.left) / 2);
    int16_t cy = (int16_t)(p->content.top +
                           (p->content.bottom - p->content.top) / 2);
    int16_t left = (int16_t)(cx - FLAIR_TEN_ACCENT_SIZE / 2);
    int16_t top  = (int16_t)(cy - FLAIR_TEN_ACCENT_SIZE / 2);
    return mk_rect(left, top,
                   (int16_t)(left + FLAIR_TEN_ACCENT_SIZE),
                   (int16_t)(top  + FLAIR_TEN_ACCENT_SIZE));
}

/* paint_content -- full content reconstruction, clipped to `clip`: the FILL
 * background, then the accent block (only when active), then the marker (only
 * when toggled), in painter's order. Used for the initial paint (open()) and
 * the updateEvt exposure repaint -- so an exposure never loses the accent/marker
 * state (Rule 3: a partial repaint that drops state is a latent bug). */
static void paint_content(const tenant_priv_t *p, const region_t *clip)
{
    draw_block(p, p->content, p->cfg->fill_idx, clip);
    if (p->active)
        draw_block(p, accent_rect(p), p->cfg->accent_idx, clip);
    if (p->toggled)
        draw_block(p, marker_rect(p), p->cfg->accent_idx, clip);
}

/* ---------------------------------------------------------------------------
 * open_common -- build the tenant's world from self->arena (ADR-0013 Sec 3.2 d).
 * Returns 0 on success, 1 on launch fail (the shell reclaims + reports, Rule 2).
 * ------------------------------------------------------------------------- */
static int open_common(FlairApp *self, const FlairLaunchParams *lp,
                       const tenant_cfg_t *cfg)
{
    if (self == (FlairApp *)0 || lp == (const FlairLaunchParams *)0 ||
        cfg == (const tenant_cfg_t *)0 || lp->wm == (WindowMgr *)0)
        return 1;

    /* the private per-instance state -- DATA arena (GENERAL class). This is the
     * only thing open() carves from self->arena; the WindowRecord + region pools
     * move to self->records_arena per ADR-0013 Amendment AC-2 (bead initech-ubd0). */
    tenant_priv_t *p = (tenant_priv_t *)flair_alloc(&self->arena, FLAIR_CLASS_GENERAL,
                                                    (uint32_t)sizeof *p);
    if (p == (tenant_priv_t *)0) return 1;
    zero_bytes(p, (uint32_t)sizeof *p);

    /* the WindowRecord (HANDLE) + four region bundles (REGION) -- RECORDS arena
     * (ADR-0013 Amendment AC-2): the shell reads ONLY these during teardown, so app
     * DEATH survives a scribbled DATA arena (BC-6). They carve from self->records_-
     * arena, NEVER self->arena. */
    WindowRecord *rec = (WindowRecord *)flair_alloc(&self->records_arena, FLAIR_CLASS_HANDLE,
                                                    (uint32_t)sizeof *rec);
    ten_rgn_t *rs = (ten_rgn_t *)flair_alloc(&self->records_arena, FLAIR_CLASS_REGION,
                                             (uint32_t)sizeof *rs);
    ten_rgn_t *rc = (ten_rgn_t *)flair_alloc(&self->records_arena, FLAIR_CLASS_REGION,
                                             (uint32_t)sizeof *rc);
    ten_rgn_t *ru = (ten_rgn_t *)flair_alloc(&self->records_arena, FLAIR_CLASS_REGION,
                                             (uint32_t)sizeof *ru);
    ten_rgn_t *rk = (ten_rgn_t *)flair_alloc(&self->records_arena, FLAIR_CLASS_REGION,
                                             (uint32_t)sizeof *rk);
    if (rec == (WindowRecord *)0 || rs == (ten_rgn_t *)0 || rc == (ten_rgn_t *)0 ||
        ru == (ten_rgn_t *)0 || rk == (ten_rgn_t *)0)
        return 1;   /* budget too small -> launch fail (Sec 3.2 d / BC-5)            */

    zero_bytes(rec, (uint32_t)sizeof *rec);
    rgn_attach(rs); rgn_attach(rc); rgn_attach(ru); rgn_attach(rk);
    rec->strucRgn   = &rs->r;
    rec->contRgn    = &rc->r;
    rec->updateRgn  = &ru->r;
    rec->nextWindow = (WindowRecord *)0;

    /* structure bounds (demo l,t,r,b) + content via the documentProc chrome inset
     * (the shell.c convention: 1px frame on sides/bottom, frame + title bar on top). */
    rgn_rect_t bounds = mk_rect(cfg->bnd_l, cfg->bnd_t, cfg->bnd_r, cfg->bnd_b);
    rgn_rect_t content = mk_rect(
        (int16_t)(bounds.left + FLAIR_CHROME_FRAME),
        (int16_t)(bounds.top  + FLAIR_CHROME_FRAME + FLAIR_CHROME_TITLEBAR_H),
        (int16_t)(bounds.right  - FLAIR_CHROME_FRAME),
        (int16_t)(bounds.bottom - FLAIR_CHROME_FRAME));

    p->cfg     = cfg;
    p->surface = lp->surface;
    p->wm      = lp->wm;
    p->content = content;
    p->clip    = &rk->r;
    p->active  = 0;
    p->toggled = 0;

    NewWindow(lp->wm, rec, bounds, content,
              (int16_t)documentKind, (int16_t)documentProc, 1 /* goAway */);

    /* the binding / demux rule (ADR-0013 Sec 3.1): FindWindow -> refCon -> FlairApp*. */
    rec->refCon = (int32_t)(uintptr_t)self;
    self->windows  = rec;
    self->userData = p;            /* per-instance state (FlairApp.userData, fka6)    */
    p->win         = rec;

    /* INITIAL content fill, clipped to the content region (active=0/toggled=0 here,
     * so paint_content draws just the FILL background). */
    paint_content(p, rec->contRgn);
    return 0;
}

static int hello_open(FlairApp *self, const FlairLaunchParams *lp)
{
    return open_common(self, lp, &HELLO_CFG);
}
static int notes_open(FlairApp *self, const FlairLaunchParams *lp)
{
    return open_common(self, lp, &NOTES_CFG);
}

/* ---------------------------------------------------------------------------
 * event_common -- the ONE shared event entry-point (ADR-0013 Sec 3.3). The
 * per-tenant config is recovered through self->userData, so HELLO and NOTES
 * share this handler. REQUIRED entry; returning IS the cooperative yield.
 * ------------------------------------------------------------------------- */
static void event_common(FlairApp *self, const EventRecord *ev)
{
    if (self == (FlairApp *)0 || ev == (const EventRecord *)0) return;
    tenant_priv_t *p = (tenant_priv_t *)self->userData;
    if (p == (tenant_priv_t *)0 || p->win == (WindowPtr)0) return;

    switch (ev->what) {
    case updateEvt:
        /* exposure-survival repaint: reconstruct the content clipped to
         * (contRgn INTERSECT updateRgn) -- the newly-exposed damage only. When
         * HELLO is raised over the overlap NOTES covered, this repaints it in
         * HELLO's FILL; the drop-updateEvt mutant leaves NOTES' colour (RED). */
        region_op(p->clip, p->win->contRgn, p->win->updateRgn, RGN_OP_INTERSECT);
        paint_content(p, p->clip);
        return;

    case activateEvt: {
        /* activation observable = the CONTENT accent (not title-bar hilite -- the
         * renderer draws no active/inactive chrome distinction; fka6 verifier fix). */
        rgn_rect_t a = accent_rect(p);
        p->active = ((ev->modifiers & FLAIR_EVT_MOD_ACTIVE_FLAG) != 0u) ? 1 : 0;
        draw_block(p, a, p->cfg->fill_idx, p->win->contRgn);    /* erase old accent  */
        if (p->active)
            draw_block(p, a, p->cfg->accent_idx, p->win->contRgn); /* present when active */
        return;
    }

    case mouseDown: {
        /* the dispatcher delivers only inContent clicks to the owner, so any
         * mouseDown that reaches here is a content click on THIS tenant: toggle a
         * visible content marker (proof the click routed to the right tenant). */
        rgn_rect_t m = marker_rect(p);
        p->toggled = p->toggled ? 0 : 1;
        draw_block(p, m, p->cfg->fill_idx, p->win->contRgn);    /* erase old marker  */
        if (p->toggled)
            draw_block(p, m, p->cfg->accent_idx, p->win->contRgn);
        return;
    }

    default:
        /* keyDown / nullEvent / ... -- nothing for the demo tenants. */
        return;
    }
}

/* ---------------------------------------------------------------------------
 * The two reference tenant vtables (idle / close = NULL; ADR-0013 Sec 3.1).
 * ------------------------------------------------------------------------- */
const FlairAppProcs hello_procs = {
    hello_open,     /* open  */
    event_common,   /* event (REQUIRED) */
    (void (*)(FlairApp *))0,                            /* idle  */
    (void (*)(FlairApp *))0                             /* close */
};
const FlairAppProcs notes_procs = {
    notes_open,     /* open  */
    event_common,   /* event (REQUIRED) */
    (void (*)(FlairApp *))0,                            /* idle  */
    (void (*)(FlairApp *))0                             /* close */
};
