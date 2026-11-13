/* Desktop composition. Single-bar policy: operator 2026-11-13;
 * CLAUDE.md, Hazards that are still true; GOLDEN-MASTER item 3. */
#include <stdint.h>

#include "shell.h"
#include "desktop.h"            /* desktop_paint_all (-Ios/flair)              */
#include "menu.h"               /* DrawMenuBar, MenuBar, MenuInfo (-Ios/flair) */
#include "dialog.h"             /* FileCopyDialog, DrawDialog (-Ios/flair)     */
#include "window.h"             /* WindowMgr_init, NewWindow (-Ios/flair)      */
#include "surface.h"            /* bitmap_t (-Ios/flair)                       */
#include "region_algebra.h"     /* region_t, rgn_rect_t (-Ispec)              */

/* ---------------------------------------------------------------------------
 * Fail-loud (dual: abort hosted / deterministic hang in-kernel), mirroring the
 * desktop.c / window.c convention so shell.c dual-compiles with only the FLAIR/
 * spec headers (no panic.h dependency).
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort -- hosted only */
#  define SHELL_PANIC(msg)  abort()
#else
#  define SHELL_PANIC(msg)  do { (void)(msg); __builtin_trap(); } while (0)
#endif

/* ---------------------------------------------------------------------------
 * Build a GrafPort over `dst` (whole-bitmap port; the menu bar draws at the top
 * FLAIR_MENUBAR_H rows). visRgn/clipRgn are NULL == no additional clip (the bar
 * spans the full width; the surface module enforces the bitmap bounds).
 * ------------------------------------------------------------------------- */
static void make_bar_port(GrafPort *port, const bitmap_t *dst)
{
    rgn_rect_t whole;
    whole.top    = 0;
    whole.left   = 0;
    whole.bottom = (int16_t)dst->height;
    whole.right  = (int16_t)dst->width;

    port->portBits.bm     = *dst;
    port->portBits.bounds = whole;
    port->portRect        = whole;
    port->visRgn          = (region_t *)0;
    port->clipRgn         = (region_t *)0;
    port->pnLoc.v  = 0;
    port->pnLoc.h  = 0;
    port->pnSize.v = 1;
    port->pnSize.h = 1;
    port->pnVis    = 0;
    port->grafProcs = (QDProcs *)0;
}

/* Build a bounded sub-bitmap view sharing the destination's pixels. */
void make_offset_view(bitmap_t *view, const bitmap_t *dst, uint32_t y_top)
{
    if (view == 0 || dst == 0 || y_top >= dst->height)
        SHELL_PANIC("make_offset_view: invalid row (mqsn)");
    *view = *dst;
    view->base   = dst->base + (uint32_t)y_top * dst->pitch;
    view->height = dst->height - y_top;
}

/* ===========================================================================
 * shell_build_scene -- wire the WindowMgr + menus + dialog from arena storage.
 * ===========================================================================*/
void shell_build_scene(shell_scene_t *s,
                       rgn_rect_t          desktop_frame,
                       shell_window_store *windows, int n_windows,
                       const rgn_rect_t   *win_bounds,
                       const char * const *win_titles,
                       region_t  *desktop_update,
                       region_t  *scratch_a, region_t *scratch_b,
                       region_t  *scratch_c, region_t *comp_scratch,
                       const MenuBar *desktop_bar,
                       DialogRecord  *dlg,
                       DialogItem    *dlg_items,
                       ControlRecord *dlg_progress,
                       region_t  *dlg_struc, region_t *dlg_cont,
                       region_t  *dlg_update,
                       region_t  *overlay_rgn,
                       int show_modal)
{
    if (s == (shell_scene_t *)0) {
        SHELL_PANIC("shell_build_scene: NULL scene");
        return;
    }
    if (n_windows > 0 && (windows == (shell_window_store *)0 ||
        win_bounds == (const rgn_rect_t *)0 || win_titles == (const char * const *)0)) {
        SHELL_PANIC("shell_build_scene: NULL window storage");
        return;
    }
    if (n_windows < 0 || n_windows > SHELL_MAX_WINDOWS) {
        SHELL_PANIC("shell_build_scene: n_windows out of range");
        return;
    }
    if (desktop_update == (region_t *)0 || scratch_a == (region_t *)0 ||
        scratch_b == (region_t *)0 || scratch_c == (region_t *)0 ||
        comp_scratch == (region_t *)0) {
        SHELL_PANIC("shell_build_scene: NULL manager region");
        return;
    }
    if (comp_scratch->rows == (rgn_row_t *)0 ||
        comp_scratch->x_pool == (int16_t *)0) {
        SHELL_PANIC("shell_build_scene: compositor scratch unattached");
        return;
    }
    if (desktop_bar == (const MenuBar *)0) {
        SHELL_PANIC("shell_build_scene: NULL desktop menu");
        return;
    }
    if (dlg == (DialogRecord *)0 || dlg_items == (DialogItem *)0 ||
        dlg_progress == (ControlRecord *)0 ||
        dlg_struc == (region_t *)0 || dlg_cont == (region_t *)0 ||
        dlg_update == (region_t *)0) {
        SHELL_PANIC("shell_build_scene: NULL dialog storage");
        return;
    }

    /* --- The Window Manager: bound to the full-screen desktop + its regions. */
    s->desktop_update = desktop_update;
    s->scratch_a      = scratch_a;
    s->scratch_b      = scratch_b;
    s->scratch_c      = scratch_c;
    s->comp_scratch   = comp_scratch;
    WindowMgr_init(&s->wm, desktop_frame, desktop_update,
                   scratch_a, scratch_b, scratch_c);

    /* --- The document windows: installed BACK-to-FRONT (the LAST is frontmost;
     * NewWindow inserts at the head). Each window's regions were attached by the
     * caller; we point the WindowRecord fields and install via NewWindow. */
    s->windows   = windows;
    s->n_windows = n_windows;
    for (int i = 0; i < n_windows; i++) {
        shell_window_store *w = &windows[i];
        if (w->strucRgn == (region_t *)0 || w->contRgn == (region_t *)0 ||
            w->updateRgn == (region_t *)0) {
            SHELL_PANIC("shell_build_scene: window region NULL");
            return;
        }
        if (w->strucRgn->rows == (rgn_row_t *)0 ||
            w->contRgn->rows == (rgn_row_t *)0 ||
            w->updateRgn->rows == (rgn_row_t *)0) {
            SHELL_PANIC("shell_build_scene: window region unattached");
            return;
        }
        w->rec.strucRgn  = w->strucRgn;
        w->rec.contRgn   = w->contRgn;
        w->rec.updateRgn = w->updateRgn;
        w->rec.nextWindow = (WindowRecord *)0;

        /* CalcDoc owns both content geometry and documentProc selection. */
        rgn_rect_t b = win_bounds[i];
        NewDocumentWindow(&s->wm, &w->rec, b,
                          (int16_t)documentKind, 1 /* goAway */);

        /* The ONE kernel-held title seam (D2-3 / future FLAIR_SETWTITLE). */
        SetWTitle(&s->wm, &w->rec, win_titles[i]);
    }

    s->desktop_bar = desktop_bar;
    (void)shell_front_menubar(s, desktop_bar);

    /* --- The modal FILE COPY dialog (standalone -- NOT in the WindowMgr z-order;
     * it is drawn LAST, over everything). FileCopyDialog stamps the canon string
     * + the progress bar (dialog.h FLAIR_CANON_FILECOPY_MSG; contract). */
    s->dlg = dlg;
    (void)FileCopyDialog(dlg, dlg_items, dlg_progress,
                         dlg_struc, dlg_cont, dlg_update);
    s->modal_up = show_modal ? 1 : 0;

    /* --- The always-on-top OVERLAY occluder (beads initech-pipa; ADR-0005 region
     * spine). The menu bar + (iff the modal is up) the modal FILE COPY box
     * are painted OUTSIDE the WindowMgr z-order (by shell_render) and so are NOT
     * damage-tracked participants. Build their union into the caller's region and
     * install it as wm.overlay_rgn: window.c fronts_union then treats these pixels
     * as occluding every window, so the live compositor never lets a dragged
     * window overpaint them (defect b) NOR seafoam-erases their vacated area
     * (defect a) -- BOTH close in the ONE shared occlusion primitive.
     *
     * The modal rect is the dialog's ACTUAL strucRgn bbox (what DrawDialog fills;
     * contract -- the overlay must match what is drawn), which FileCopyDialog set
     * above. The bar spans rows [0, SHELL_MENUBAR_H) full width. NULL overlay ==
     * disabled (the fold is a no-op; the M4 host gate). */
    s->overlay_rgn = overlay_rgn;
    shell_sync_overlay(s);

    s->_built = 1u;
}

/* Rebuild the always-on-top occluder from the scene's current geometry. The
 * modal is a standalone shell layer (not a WindowMgr participant), so moving it
 * must update this union before old-position exposure is distributed. */
void shell_sync_overlay(shell_scene_t *s)
{
    region_t *overlay_rgn;

    if (s == (shell_scene_t *)0) {
        SHELL_PANIC("shell_sync_overlay: NULL scene");
        return;
    }
    overlay_rgn = s->overlay_rgn;
    if (overlay_rgn != (region_t *)0) {
        if (overlay_rgn->rows == (rgn_row_t *)0 ||
            overlay_rgn->x_pool == (int16_t *)0) {
            SHELL_PANIC("shell_build_scene: overlay region unattached");
            return;
        }
        rgn_rect_t ov[2];
        uint16_t nov = 0;
        /* The single menu bar: rows [0, SHELL_MENUBAR_H) x full width. */
        ov[nov].top    = (int16_t)SHELL_MENUBAR_TOP;
        ov[nov].left   = 0;
        ov[nov].bottom = (int16_t)SHELL_MENUBAR_H;
        ov[nov].right  = (int16_t)SHELL_SCREEN_W;
        nov++;
        /* The modal FILE COPY box (its dBoxProc bounds fill opaquely), iff up. */
        if (s->modal_up) {
            if (s->dlg == (DialogRecord *)0 ||
                s->dlg->window.strucRgn == (region_t *)0) {
                SHELL_PANIC("shell_sync_overlay: modal has no strucRgn");
                return;
            }
            ov[nov] = s->dlg->window.strucRgn->bbox;
            nov++;
        }
        region_from_rects(overlay_rgn, ov, nov);
        s->wm.overlay_rgn = overlay_rgn;
    } else {
        s->wm.overlay_rgn = (region_t *)0;
    }
}

/* ===========================================================================
 * shell_render -- composite the whole scene onto `dst`, back to front (D-5).
 * ===========================================================================*/
const MenuBar *shell_front_menubar(shell_scene_t *s, const MenuBar *foreground)
{
    const MenuBar *owner = foreground ? foreground : s->desktop_bar;
    s->bar = *owner;
    s->bar.has_apple = 1;
    MenuClock_attach(&s->bar);
    return &s->bar;
}

void shell_render(shell_scene_t *s, const bitmap_t *dst)
{
    shell_render_with_menubar(s, dst, (const MenuBar *)0);
}

void shell_render_with_menubar(shell_scene_t *s, const bitmap_t *dst,
                               const MenuBar *foreground)
{
    if (dst != 0 && dst->height < SHELL_MENUBAR_H)
        SHELL_PANIC("shell_render: menu bar must fit (mqsn)");
    if (s == (shell_scene_t *)0 || !s->_built) {
        SHELL_PANIC("shell_render: NULL/unbuilt scene");
        return;
    }
    if (dst == (const bitmap_t *)0 || dst->base == (volatile uint8_t *)0 ||
        dst->width == 0u || dst->height == 0u) {
        SHELL_PANIC("shell_render: bad dst");
        return;
    }
    if (s->comp_scratch == (region_t *)0 ||
        s->comp_scratch->rows == (rgn_row_t *)0 ||
        s->comp_scratch->x_pool == (int16_t *)0) {
        SHELL_PANIC("shell_render: compositor scratch unattached");
        return;
    }


    /* 1. The seafoam desktop background + every visible window's chrome, painted
     * back to front by the compositor (desktop.c). This is the bottom layer. */
    desktop_paint_all(&s->wm, dst, s->comp_scratch);

    /* The front program owns the titles in the single top menu bar. */
    {
        GrafPort port;
        make_bar_port(&port, dst);
        DrawMenuBar(&port, shell_front_menubar(s, foreground), -1,
                    (const region_t *)0);
    }

    /* 3. The modal FILE COPY DIALOG on top -- drawn LAST so it occludes the
     * windows + bars behind it (correct z-order; ADR-0004 D-5). The dialog is a
     * standalone window (not in the WindowMgr z-order); its port is pointed at
     * `dst` with NO clip so its dBoxProc box paints over whatever is behind it. */
    if (s->modal_up && s->dlg != (DialogRecord *)0) {
        s->dlg->window.port.portBits.bm = *dst;
        s->dlg->window.port.visRgn  = (region_t *)0;
        s->dlg->window.port.clipRgn = (region_t *)0;
        /* portRect == the dialog's bounds (its structure bbox); DrawDialog draws
         * the dBoxProc border + items relative to portRect. */
        s->dlg->window.port.portRect = s->dlg->window.strucRgn->bbox;
        DrawDialog(s->dlg);
    }
}
