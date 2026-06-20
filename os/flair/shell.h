/*
 * os/flair/shell.h -- the FLAIR DESKTOP SHELL (THE ARTIFACT, FLAIR Layer 5).
 *
 * beads: initech-k8o5.12 (M4 Manager set: Window/Menu/Control/Dialog + two
 *        stacked menu bars + modal FILE COPY layer + desktop shell) and
 *        initech-859 (Desktop shell: reproduce the frame chrome -- the M4 gate).
 *
 * WHAT THIS IS (ADR-0004 D-1, Layer 5 "DESKTOP SHELL"): the COMPOSITOR that
 * composes EVERYTHING the FLAIR Managers build into ONE rendered InitechOS
 * desktop -- the M4 capstone that REPRODUCES THE OFFICE SPACE FRAME. It owns NO
 * pixel path of its own (ADR-0004 D-2 / C-2: one surface module, no second pixel
 * path): every pixel is written through the existing Managers --
 *   - the seafoam desktop background + window chrome via desktop_paint_all
 *     (os/flair/desktop.c -> blitter_fill_rect_clipped + flair_draw_document_window
 *     -> the ONE surface module),
 *   - the TWO STACKED MENU BARS via DrawMenuBar (os/flair/menu.c), and
 *   - the modal FILE COPY DIALOG via DrawDialog (os/flair/dialog.c).
 *
 * THE COMPOSED FRAME (Law 4 -- "look like the frame"; the Office Space
 * "Saving tables to disk..." still, PRD Sec 1 / Appendix A):
 *
 *   - The SEAFOAM desktop background (INITECH_DESKTOP_BG_RGB, OD-4).
 *   - The "two stacked menu bars" chimera (gui-ground-truth.md Sec 4;
 *     chimera_element_map.json elements 7/8/9): the TOP System-7 menu bar
 *     (Apple glyph + File/Edit/View/Special) at rows [0,20), AND the
 *     Photoshop-EXACT bar (the FROZEN canon string menu_canon.h:
 *     "File Edit Image Layer Select View Window Help" -- element 9, the canon
 *     chimera tell, AM-4) stacked directly below at rows [20,40). Both are real
 *     Mac System-7-style bars (Chicago 12, 20 px each; GetMBarHeight).
 *   - One or two System-7 documentProc WINDOWS (z-ordered, titled, with the
 *     pinstripe title bar + 1 px frame + 16 px scrollbar) placed per the frame.
 *   - The modal FILE COPY DIALOG on top (the comedic centerpiece): the dBoxProc
 *     7-px border box, the byte-exact FLAIR_CANON_FILECOPY_MSG, and a progress
 *     bar -- centered on the 640x480 desktop and ON TOP of (occluding) the
 *     windows behind it (correct z-order / clip).
 *
 * THE Z-ORDER (back to front -- the painter's algorithm; ADR-0004 D-5):
 *   1. seafoam desktop      (bottom)
 *   2. the document windows (z-ordered by the Window Manager)
 *   3. the two menu bars    (the menu bar is always above document windows)
 *   4. the FILE COPY modal  (top -- a modal is drawn LAST, over everything)
 * shell_render paints in exactly this order so the modal occludes what is
 * behind it and the menu bars sit above the windows.
 *
 * STORAGE DISCIPLINE (Law 3, freestanding; ADR-0004 DEC-03 FLAIR heap): this
 * module mallocs NOTHING. The caller supplies ALL storage via a shell_scene_t
 * (the WindowMgr + its arena-backed regions, the WindowRecords + their regions,
 * the MenuBar/MenuInfo/MenuItem arrays, the DialogRecord + its items + progress
 * ControlRecord + its regions, and one compositor scratch region) -- exactly the
 * caller-supplied-storage pattern the rest of FLAIR uses (window.h, dialog.h,
 * menu.h). shell_build_scene wires those pieces together from caller-supplied
 * arena storage; shell_render composites them onto a caller-supplied bitmap.
 *
 * DUAL-COMPILE (the desktop.c / window.c pattern; Law 3): shell.c compiles BOTH
 * freestanding for the kernel (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall
 * -Wextra -Werror) AND hosted for the M4 gate (harness/proptest/test_shell.c).
 * It uses only <stdint.h> + the FLAIR/spec headers; no host malloc; no libc.
 * Fail-loud (Rule 2) on a NULL scene / NULL bitmap / unattached region.
 *
 * Ref: ADR-0004 D-1 (5-layer stack incl. Layer 5 DESKTOP SHELL), D-2 (one
 *      surface module; no second pixel path), D-3 (the Managers; the Photoshop
 *      bar), D-5 (z-order / clip), D-8 (the oracle vector). AM-4 (canon frozen).
 *      PRD Sec 1 / Sec 6.3 / Sec 6.5 / Appendix A (the frame, the FILE COPY box).
 *      docs/research/gui-ground-truth.md Sec 4 (the chimera element map).
 *      spec/chimera_element_map.json (the 11-element map; elements 7/8/9).
 *      spec/assets/menu_canon.h (FLAIR_CANON_PHOTOSHOP_* -- the FROZEN canon).
 *      os/flair/desktop.h (desktop_paint_all), os/flair/menu.h (DrawMenuBar),
 *      os/flair/dialog.h (FileCopyDialog, DrawDialog, FLAIR_CANON_FILECOPY_MSG),
 *      os/flair/window.h (WindowMgr, NewWindow), os/flair/surface.h (bitmap_t),
 *      spec/region_algebra.h (region_t). CLAUDE.md Law 2/3/4, Rule 2/11/12.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_SHELL_H
#define INITECH_OS_FLAIR_SHELL_H

#include <stdint.h>

#include "surface.h"            /* bitmap_t (the ONE pixel-buffer descriptor)  */
#include "region_algebra.h"     /* region_t, rgn_rect_t (-Ispec)               */
#include "window.h"             /* WindowMgr, WindowRecord, WindowPtr          */
#include "menu.h"               /* MenuBar, MenuInfo, MenuItem, DrawMenuBar    */
#include "dialog.h"             /* DialogRecord, DialogItem, FileCopyDialog    */
#include "control.h"            /* ControlRecord (progress bar)                */

/* ===========================================================================
 * 1. SHELL LAYOUT CONSTANTS  (Law 1 -- grounded in chrome_metrics + the frame)
 * ---------------------------------------------------------------------------
 * The native desktop is 640x480 indexed-8 (ADR-0004 OD-2 / OD-3). The two menu
 * bars are each FLAIR_MENUBAR_H (20 px; GetMBarHeight) tall and stacked: the
 * System-7 bar occupies rows [0,20) and the Photoshop bar rows [20,40). The
 * windows are placed in the working area below the second bar.
 * ===========================================================================*/
#define SHELL_SCREEN_W            640
#define SHELL_SCREEN_H            480

/* The two stacked menu bars (the chimera; gui-ground-truth.md Sec 4). The top
 * (System-7) bar's TOP is row 0; the Photoshop bar's TOP is FLAIR_MENUBAR_H. */
#define SHELL_MENUBAR1_TOP        0                     /* System-7 bar top     */
#define SHELL_MENUBAR2_TOP        FLAIR_MENUBAR_H       /* Photoshop bar top    */
#define SHELL_MENUBARS_H          (2 * FLAIR_MENUBAR_H) /* both bars: 40 px     */

/* The maximum number of document windows the shell composes (frame shows 1-2). */
#define SHELL_MAX_WINDOWS         2

/* ===========================================================================
 * 2. SHELL SCENE -- the caller-supplied storage bundle (no malloc; Law 3)
 * ---------------------------------------------------------------------------
 * The caller allocates this whole struct (e.g. one static / one arena block)
 * and hands it to shell_build_scene, which wires the WindowMgr, the two menu
 * bars, and the FILE COPY dialog into it. shell_render then composites it.
 *
 * EVERY region carried here MUST have its rows[]/x_pool pools ALREADY ATTACHED
 * by the caller (the region engine never mallocs; spec/region_algebra.h). The
 * shell does NOT attach them -- it asserts they are attached (Rule 2).
 * ===========================================================================*/

/* One document window's storage: a WindowRecord + its three regions. The caller
 * attaches the region pools and points the WindowRecord's region fields at them
 * (the window.c caller-supplied-storage idiom; see test_drag.c win_store_t). */
typedef struct shell_window_store {
    WindowRecord  rec;          /* the WindowRecord (embeds its GrafPort)        */
    region_t     *strucRgn;     /* attached by the caller (rows[]/x_pool set)     */
    region_t     *contRgn;      /* attached by the caller                         */
    region_t     *updateRgn;    /* attached by the caller                         */
} shell_window_store;

/*
 * shell_scene_t -- everything the shell composes, all caller-supplied.
 *
 * The Window Manager (wm) is bound by shell_build_scene to the caller's desktop
 * frame + desktop-update region + three scratch regions (window.h WindowMgr_init).
 * The two menu bars (bar_sys, bar_photoshop) are built over the caller's
 * MenuInfo/MenuItem arrays. The FILE COPY dialog (dlg) is built by FileCopyDialog
 * over the caller's DialogItem array + progress ControlRecord + three regions.
 * The compositor's per-window visible-region scratch (comp_scratch) is DISTINCT
 * from the manager's three internal scratch regions (desktop.h requirement).
 */
typedef struct shell_scene {
    /* --- The Window Manager + its regions (window.h; caller-supplied) -------- */
    WindowMgr     wm;
    region_t     *desktop_update;   /* desktop background damage region          */
    region_t     *scratch_a;        /* manager internal scratch (fronts-union)   */
    region_t     *scratch_b;        /* manager internal scratch                  */
    region_t     *scratch_c;        /* manager internal scratch                  */
    region_t     *comp_scratch;     /* compositor visible-region carrier (D-5)    */

    /* --- The document windows (1..n_windows; back at index 0) --------------- */
    shell_window_store *windows;    /* caller array of >= n_windows stores       */
    int           n_windows;        /* number of document windows (<= SHELL_MAX) */

    /* --- The two stacked menu bars (the chimera) --------------------------- */
    MenuBar       bar_sys;          /* TOP System-7 bar (Apple + File/Edit/...)  */
    MenuBar       bar_photoshop;    /* Photoshop-exact bar (the canon string)    */

    /* --- The modal FILE COPY dialog (the comedic centerpiece) -------------- */
    DialogRecord *dlg;              /* caller-supplied DialogRecord storage      */
    int           modal_up;         /* 1 == the FILE COPY modal is shown (on top)*/

    uint8_t       _built;           /* set by shell_build_scene (Rule 2 guard)   */
} shell_scene_t;

/* ===========================================================================
 * 3. shell_build_scene -- wire the WindowMgr + menus + dialog from arena storage
 * ---------------------------------------------------------------------------
 * Initializes the Window Manager to the full-screen desktop, installs the
 * caller's document windows (z-ordered, the LAST one frontmost), builds the two
 * stacked menu bars over the caller's MenuInfo/MenuItem arrays (the Photoshop
 * bar from the FROZEN menu_canon.h canon string), and builds the FILE COPY modal
 * dialog (FileCopyDialog) over the caller's DialogRecord/items/progress control.
 *
 * The caller MUST pre-attach EVERY region in the scene (rows[]/x_pool) AND point
 * each WindowRecord's strucRgn/contRgn/updateRgn at its store's regions, AND
 * provide:
 *   - the MenuInfo + MenuItem arrays for both bars (sys_menus/sys_items and the
 *     photoshop_menus -- the Photoshop titles come from menu_canon.h; the caller
 *     supplies the item lists so a dropped menu has rows),
 *   - the DialogItem array (>= 2) + the progress ControlRecord for FileCopyDialog,
 *   - the dialog's three regions (strucRgn/contRgn/updateRgn).
 *
 * `show_modal` selects whether the FILE COPY modal is part of the composed
 * scene (1 == the canon frame; 0 == bare desktop). On success the scene is fully
 * wired and shell_render may be called.
 *
 * Fail-loud (Rule 2) on a NULL scene, a NULL required pointer, an unattached
 * region, or n_windows out of [1, SHELL_MAX_WINDOWS].
 *
 * Parameters:
 *   s             -- the scene to wire (caller storage).
 *   desktop_frame -- the desktop bounds (typically {0,0,SCREEN_H,SCREEN_W}).
 *   windows       -- array of >= n_windows shell_window_store (regions attached).
 *   n_windows     -- number of document windows to install.
 *   win_bounds    -- array of n_windows structure rects (global coords).
 *   win_titles    -- array of n_windows titles (ASCII; may be "").
 *   sys_menus     -- MenuInfo array for the System-7 bar (caller-supplied items).
 *   n_sys_menus   -- number of System-7 menus.
 *   ps_menus      -- MenuInfo array for the Photoshop bar; titles are SET by the
 *                    shell from the frozen menu_canon.h canon (the caller need
 *                    only supply the item lists). MUST have at least
 *                    FLAIR_CANON_PHOTOSHOP_MENU_COUNT (8) entries.
 *   dlg           -- DialogRecord storage for the FILE COPY modal.
 *   dlg_items     -- DialogItem array (>= 2) for FileCopyDialog.
 *   dlg_progress  -- ControlRecord for the FILE COPY progress bar.
 *   dlg_struc/cont/update -- the dialog's three attached regions.
 *   show_modal    -- 1 to compose the FILE COPY modal on top.
 * ------------------------------------------------------------------------- */
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
                       int show_modal);

/* ===========================================================================
 * 4. shell_render -- composite the whole scene onto `dst`
 * ---------------------------------------------------------------------------
 * Paints, back to front (the painter's algorithm; ADR-0004 D-5 z-order):
 *   1. the seafoam desktop background + every visible window's chrome
 *      (desktop_paint_all),
 *   2. the TWO STACKED MENU BARS (DrawMenuBar; the System-7 bar at rows
 *      [0,20), the Photoshop canon bar at rows [20,40) -- above the windows),
 *   3. the modal FILE COPY DIALOG on top (DrawDialog) IFF the scene's modal is
 *      up -- drawn LAST so it occludes the windows + bars behind it (z-order).
 *
 * `dst` is the destination bitmap (the LFB or an offscreen). The scene must have
 * been wired by shell_build_scene. Fail-loud (Rule 2) on a NULL/unbuilt scene,
 * a NULL/zero dst, or an unattached compositor scratch.
 * ------------------------------------------------------------------------- */
void shell_render(shell_scene_t *s, const bitmap_t *dst);

#endif /* INITECH_OS_FLAIR_SHELL_H */
