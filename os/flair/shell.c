/*
 * os/flair/shell.c -- the FLAIR DESKTOP SHELL (THE ARTIFACT, FLAIR Layer 5).
 *
 * beads: initech-k8o5.12 (M4 Manager set + two stacked menu bars + modal FILE
 *        COPY layer + desktop shell) and initech-859 (Desktop shell: reproduce
 *        the frame chrome -- the M4 gate). See shell.h for the full contract.
 *
 * Ref:   ADR-0004 D-1 (5-layer stack incl. Layer 5 DESKTOP SHELL), D-2 (one
 *        surface module; NO second pixel path -- the shell composes EXISTING
 *        Managers and never touches base[] directly), D-3 (the Managers; the
 *        Photoshop bar canon), D-5 (z-order: back to front). PRD Sec 1 / 6.3 /
 *        6.5 / Appendix A (the frame). spec/assets/menu_canon.h (FROZEN canon).
 *        CLAUDE.md Law 2/3/4, Rule 2 (fail loud), Rule 11 (deterministic),
 *        Rule 12 (ASCII-clean -- no non-ASCII incl. comments; 'Sec' not the
 *        section sign).
 *
 * THE Z-ORDER (shell_render paints back to front; ADR-0004 D-5):
 *   1. seafoam desktop + window chrome  (desktop_paint_all)
 *   2. the TWO STACKED MENU BARS        (DrawMenuBar; above the windows)
 *   3. the modal FILE COPY DIALOG       (DrawDialog; LAST -- occludes the rest)
 *
 * THE SECOND-BAR OFFSET-BITMAP TRICK (ADR-0004 D-2 -- still the ONE surface
 * module): DrawMenuBar (os/flair/menu.c) hardcodes the bar at rows [0, 20) of
 * the port's bitmap. To stack the SECOND (Photoshop) bar at rows [20, 40) of the
 * real offscreen WITHOUT a second pixel path, the shell builds a sub-bitmap VIEW
 * whose `base` points at the start of row FLAIR_MENUBAR_H of the same offscreen
 * (base + MENUBAR_H * pitch) and whose height is reduced by FLAIR_MENUBAR_H.
 * DrawMenuBar then paints rows [0, 20) of the VIEW == rows [20, 40) of the real
 * offscreen, all through surface_fill_span / surface_blit -- the SAME ONE writer.
 * This is the period-authentic offset-bitmap technique (a GrafPort over a
 * sub-rectangle of the screen), not a new pixel path.
 *
 * NAMED MUTATION SWITCHES (Rule 6): test_shell.c compiles this file with named
 * mutants to prove the M4 gate bites. The default build defines none.
 *
 *   SHELL_MUTATE_ONE_MENUBAR -- shell_render draws ONLY the first (System-7)
 *     menu bar and SKIPS the second (Photoshop) bar. The "two stacked menu bars"
 *     chimera (the canon tell) collapses to one -> the Photoshop-bar band reads
 *     as bare desktop and the byte-exact canon-string assertion fails: the M4
 *     STRUCTURAL gate (both bars present) goes RED. The "forgot the chimera" bug.
 *
 *   SHELL_MUTATE_NO_MODAL -- shell_render SKIPS the modal FILE COPY layer even
 *     when the scene's modal is up. The comedic centerpiece vanishes -> the
 *     "Saving tables to disk..." text + the centered dBoxProc box are absent and
 *     the windows behind show through: the M4 gate (modal present + occluding)
 *     goes RED. The "forgot the modal" bug.
 *
 *   SHELL_MUTATE_MODAL_BEHIND -- shell_render draws the modal FIRST (before the
 *     windows + bars) instead of LAST, so the document windows + menu bars paint
 *     OVER it. The modal is occluded instead of occluding -> the z-order
 *     assertion (the modal is ON TOP) goes RED. The "wrong z-order" bug.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stdint.h>

#include "shell.h"
#include "desktop.h"            /* desktop_paint_all (-Ios/flair)              */
#include "menu.h"               /* DrawMenuBar, MenuBar, MenuInfo (-Ios/flair) */
#include "dialog.h"             /* FileCopyDialog, DrawDialog (-Ios/flair)     */
#include "window.h"             /* WindowMgr_init, NewWindow (-Ios/flair)      */
#include "surface.h"            /* bitmap_t (-Ios/flair)                       */
#include "region_algebra.h"     /* region_t, rgn_rect_t (-Ispec)              */
#include "menu_canon.h"         /* FLAIR_CANON_PHOTOSHOP_* (-Ispec/assets)     */

/* ---------------------------------------------------------------------------
 * Fail-loud (dual: abort hosted / deterministic hang in-kernel), mirroring the
 * desktop.c / window.c convention so shell.c dual-compiles with only the FLAIR/
 * spec headers (no panic.h dependency).
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort -- hosted only */
#  define SHELL_PANIC(msg)  abort()
#else
#  define SHELL_PANIC(msg)  do { for (;;) { } } while (0)
#endif

/* ---------------------------------------------------------------------------
 * Menu-bar colors as indexed-8 palette indices (the test_menu.c convention: for
 * the 8bpp pass DrawMenuBar's fg/bg are the low-byte palette indices). idx 0 ==
 * black ink (the title text + baseline + the Apple glyph square), idx 3 ==
 * the menubar gray fill (render_palette_rgb / chrome.c CIDX_MENUBAR). On a 32bpp
 * destination the surface module writes the low bits as the packed color; the
 * shell uses the canonical packed values so both depths read consistently.
 *
 * For 8bpp these are the bare palette indices; for >8bpp the surface module
 * packs the low 24 bits, so we OR in the canonical menubar-gray RGB for the bg.
 * ------------------------------------------------------------------------- */
#define SHELL_MENU_INK_IDX     0u   /* black title text / baseline / apple slot */
#define SHELL_MENU_BG_IDX      3u   /* menubar gray (chrome.c CIDX_MENUBAR)     */
#define SHELL_MENUBAR_BG_RGB   0x67696Cu /* INITECH_MENUBAR_BG_RGB (palette.h)  */

/* Resolve the menu-bar fg/bg for this destination depth (one conversion site,
 * mirroring desktop.c desktop_px). 8bpp: the palette indices directly. >8bpp:
 * black ink (0) and the packed menubar-gray RGB. */
static uint32_t shell_menu_fg(const bitmap_t *dst)
{
    if (dst->bpp == 8u) {
        return SHELL_MENU_INK_IDX;
    }
    return 0x000000u;   /* black ink (packed) */
}
static uint32_t shell_menu_bg(const bitmap_t *dst)
{
    if (dst->bpp == 8u) {
        return SHELL_MENU_BG_IDX;
    }
    return SHELL_MENUBAR_BG_RGB;
}

/* ---------------------------------------------------------------------------
 * Build a GrafPort over `dst` (whole-bitmap port; the menu bar draws at the top
 * FLAIR_MENUBAR_H rows). visRgn/clipRgn are NULL == no additional clip (the bar
 * spans the full width; the surface module enforces the bitmap bounds). This is
 * the port the FIRST (System-7) bar draws into.
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

/* ---------------------------------------------------------------------------
 * Build an OFFSET sub-bitmap VIEW of `dst` starting at row `y_top` (the second-
 * bar trick; ADR-0004 D-2 -- still the ONE surface module). The view shares the
 * SAME pitch/bpp/width as `dst`; its base points at base + y_top*pitch and its
 * height is dst->height - y_top. DrawMenuBar into a port over this view paints
 * rows [0, MENUBAR_H) of the view == rows [y_top, y_top+MENUBAR_H) of `dst`.
 * ------------------------------------------------------------------------- */
/* (marked unused so the SHELL_MUTATE_ONE_MENUBAR build -- which omits the second
 * bar and thus this helper's only caller -- still compiles clean under -Werror.) */
__attribute__((unused))
static void make_offset_view(bitmap_t *view, const bitmap_t *dst, uint32_t y_top)
{
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
                       MenuInfo  *sys_menus, int n_sys_menus,
                       MenuInfo  *ps_menus,
                       DialogRecord  *dlg,
                       DialogItem    *dlg_items,
                       ControlRecord *dlg_progress,
                       region_t  *dlg_struc, region_t *dlg_cont,
                       region_t  *dlg_update,
                       int show_modal)
{
    if (s == (shell_scene_t *)0) {
        SHELL_PANIC("shell_build_scene: NULL scene");
        return;
    }
    if (windows == (shell_window_store *)0 || win_bounds == (const rgn_rect_t *)0 ||
        win_titles == (const char * const *)0) {
        SHELL_PANIC("shell_build_scene: NULL window storage");
        return;
    }
    if (n_windows < 1 || n_windows > SHELL_MAX_WINDOWS) {
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
    if (sys_menus == (MenuInfo *)0 || ps_menus == (MenuInfo *)0) {
        SHELL_PANIC("shell_build_scene: NULL menu array");
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

        /* content = 1 px frame on sides/bottom, (frame + title bar) on top --
         * the documentProc chrome bands (the test_drag make_rects convention,
         * grounded in chrome_metrics.h). */
        rgn_rect_t b = win_bounds[i];
        rgn_rect_t c;
        c.top    = (int16_t)(b.top + FLAIR_CHROME_FRAME + FLAIR_CHROME_TITLEBAR_H);
        c.left   = (int16_t)(b.left + FLAIR_CHROME_FRAME);
        c.bottom = (int16_t)(b.bottom - FLAIR_CHROME_FRAME);
        c.right  = (int16_t)(b.right - FLAIR_CHROME_FRAME);

        NewWindow(&s->wm, &w->rec, b, c,
                  (int16_t)documentKind, (int16_t)documentProc, 1 /* goAway */);

        /* Copy the title into the WindowRecord (NewWindow leaves it ""). */
        const char *t = win_titles[i] ? win_titles[i] : "";
        int k = 0;
        while (t[k] != '\0' && k < (int)(FLAIR_WINDOW_TITLE_MAX - 1u)) {
            w->rec.titleHandle[k] = t[k];
            k++;
        }
        w->rec.titleHandle[k] = '\0';
    }

    /* --- The TOP System-7 menu bar (Apple glyph + the caller's menus). */
    s->bar_sys.menus     = sys_menus;
    s->bar_sys.n_menus   = (uint16_t)((n_sys_menus < 0) ? 0 : n_sys_menus);
    s->bar_sys.has_apple = 1;   /* the System-7 bar carries the Apple-menu slot */

    /* --- The Photoshop-EXACT bar: the titles are SET from the FROZEN canon
     * (menu_canon.h; AM-4 -- NOT re-authored). The caller supplies the MenuInfo
     * array (>= 8) so each menu has an item list; the shell stamps the canon
     * title into each MenuInfo.title (the chimera tell, element 9). */
    for (int mi = 0; mi < FLAIR_CANON_PHOTOSHOP_MENU_COUNT; mi++) {
        ps_menus[mi].title  = FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[mi];
        ps_menus[mi].menuID = (int16_t)(256 + mi); /* distinct from sys bar IDs  */
    }
    s->bar_photoshop.menus     = ps_menus;
    s->bar_photoshop.n_menus   = (uint16_t)FLAIR_CANON_PHOTOSHOP_MENU_COUNT;
    s->bar_photoshop.has_apple = 0;   /* the Photoshop bar has no Apple slot     */

    /* --- The modal FILE COPY dialog (standalone -- NOT in the WindowMgr z-order;
     * it is drawn LAST, over everything). FileCopyDialog stamps the canon string
     * + the progress bar (dialog.h FLAIR_CANON_FILECOPY_MSG; Law 4). */
    s->dlg = dlg;
    (void)FileCopyDialog(dlg, dlg_items, dlg_progress,
                         dlg_struc, dlg_cont, dlg_update);
    s->modal_up = show_modal ? 1 : 0;

    s->_built = 1u;
}

/* ===========================================================================
 * shell_render -- composite the whole scene onto `dst`, back to front (D-5).
 * ===========================================================================*/
void shell_render(shell_scene_t *s, const bitmap_t *dst)
{
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

#if defined(SHELL_MUTATE_MODAL_BEHIND)
    /* MUTANT: draw the modal FIRST (before the windows + bars) so they paint
     * OVER it -- the modal ends up OCCLUDED instead of occluding: the z-order
     * (modal ON TOP) assertion goes RED. */
    if (s->modal_up && s->dlg != (DialogRecord *)0) {
        s->dlg->window.port.portBits.bm = *dst;
        s->dlg->window.port.visRgn  = (region_t *)0;
        s->dlg->window.port.clipRgn = (region_t *)0;
        s->dlg->window.port.portRect = s->dlg->window.strucRgn->bbox;
        DrawDialog(s->dlg);
    }
#endif

    /* 1. The seafoam desktop background + every visible window's chrome, painted
     * back to front by the compositor (desktop.c). This is the bottom layer. */
    desktop_paint_all(&s->wm, dst, s->comp_scratch);

    /* 2. The TWO STACKED MENU BARS (the chimera), above the windows. Both via
     * DrawMenuBar (os/flair/menu.c) -- the ONE surface module. The System-7 bar
     * draws into a whole-bitmap port (rows [0, MENUBAR_H)); the Photoshop bar
     * draws into an OFFSET sub-bitmap view starting at row MENUBAR_H, so it lands
     * at rows [MENUBAR_H, 2*MENUBAR_H) of the real offscreen. */
    {
        uint32_t fg = shell_menu_fg(dst);
        uint32_t bg = shell_menu_bg(dst);

        /* Bar 1: the TOP System-7 bar at rows [0, MENUBAR_H). */
        GrafPort p1;
        make_bar_port(&p1, dst);
        DrawMenuBar(&p1, &s->bar_sys, fg, bg, (const region_t *)0);

#if !defined(SHELL_MUTATE_ONE_MENUBAR)
        /* Bar 2: the Photoshop-EXACT canon bar at rows [MENUBAR_H, 2*MENUBAR_H),
         * via an offset sub-bitmap view (the second-bar trick; D-2 ONE surface).
         * Skipped by the SHELL_MUTATE_ONE_MENUBAR mutant (the chimera collapses). */
        bitmap_t view2;
        make_offset_view(&view2, dst, (uint32_t)SHELL_MENUBAR2_TOP);
        GrafPort p2;
        make_bar_port(&p2, &view2);
        DrawMenuBar(&p2, &s->bar_photoshop, fg, bg, (const region_t *)0);
#endif
    }

#if !defined(SHELL_MUTATE_MODAL_BEHIND)
    /* 3. The modal FILE COPY DIALOG on top -- drawn LAST so it occludes the
     * windows + bars behind it (correct z-order; ADR-0004 D-5). The dialog is a
     * standalone window (not in the WindowMgr z-order); its port is pointed at
     * `dst` with NO clip so its dBoxProc box paints over whatever is behind it. */
#if !defined(SHELL_MUTATE_NO_MODAL)
    if (s->modal_up && s->dlg != (DialogRecord *)0) {
        s->dlg->window.port.portBits.bm = *dst;
        s->dlg->window.port.visRgn  = (region_t *)0;
        s->dlg->window.port.clipRgn = (region_t *)0;
        /* portRect == the dialog's bounds (its structure bbox); DrawDialog draws
         * the dBoxProc border + items relative to portRect. */
        s->dlg->window.port.portRect = s->dlg->window.strucRgn->bbox;
        DrawDialog(s->dlg);
    }
#else
    /* MUTANT: SKIP the modal layer -> the FILE COPY box + "Saving tables to
     * disk..." text are absent and the windows behind show through: RED. */
    (void)0;
#endif
#endif /* !SHELL_MUTATE_MODAL_BEHIND */
}
