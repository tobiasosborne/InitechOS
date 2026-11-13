/*
 * FLAIR desktop compositor. Source for single-bar policy: operator ruling
 * 2026-11-13, CLAUDE.md "Hazards that are still true", GOLDEN-MASTER item 3.
 * Compose desktop/windows, the front program's menu bar, then FILE COPY.
 * All drawing goes through the Toolbox managers. Storage is caller-owned;
 * every region must have its rows and x_pool attached before construction.
 */
#ifndef INITECH_OS_FLAIR_SHELL_H
#define INITECH_OS_FLAIR_SHELL_H

#include <stdint.h>
#include "surface.h"
#include "region_algebra.h"
#include "window.h"
#include "menu.h"
#include "dialog.h"
#include "control.h"

#define SHELL_SCREEN_W       640
#define SHELL_SCREEN_H       480
#define SHELL_MENUBAR_TOP       0
#define SHELL_MENUBAR_H       FLAIR_MENUBAR_H
#define SHELL_MAX_WINDOWS      2

typedef struct shell_window_store {
    WindowRecord rec;
    region_t *strucRgn;
    region_t *contRgn;
    region_t *updateRgn;
} shell_window_store;

typedef struct shell_scene {
    WindowMgr wm;
    region_t *desktop_update;
    region_t *scratch_a;
    region_t *scratch_b;
    region_t *scratch_c;
    region_t *comp_scratch;
    shell_window_store *windows;
    int n_windows;

    /* The default is the Finder's bar. The display view reserves the Initech
     * glyph and clock even if an app omitted has_apple. Do not change app
     * storage: menu delivery still compares the original owner's identity. */
    const MenuBar *desktop_bar;
    MenuBar bar;

    DialogRecord *dlg;
    int modal_up;
    /* Always-on-top occlusion: rows [0,20), plus the modal if shown. */
    region_t *overlay_rgn;
    uint8_t _built;
} shell_scene_t;

/* Install zero to SHELL_MAX_WINDOWS windows, back to front, and the standalone
 * FILE COPY dialog. desktop_frame is the full screen. desktop_bar is the
 * Finder's menu resource. An optional attached overlay protects menu/modal
 * pixels during damage repaint. NULL required storage fails loudly. */
void shell_build_scene(shell_scene_t *s,
                       rgn_rect_t desktop_frame,
                       shell_window_store *windows, int n_windows,
                       const rgn_rect_t *win_bounds,
                       const char * const *win_titles,
                       region_t *desktop_update,
                       region_t *scratch_a, region_t *scratch_b,
                       region_t *scratch_c, region_t *comp_scratch,
                       const MenuBar *desktop_bar,
                       DialogRecord *dlg, DialogItem *dlg_items,
                       ControlRecord *dlg_progress,
                       region_t *dlg_struc, region_t *dlg_cont,
                       region_t *dlg_update, region_t *overlay_rgn,
                       int show_modal);

void shell_sync_overlay(shell_scene_t *s);

/* Return the shell-owned display view of foreground, or the Finder on NULL.
 * Titles/items are shared with the owner; glyph and clock belong to the shell.
 * Use the original owner pointer when delivering a menu selection. */
const MenuBar *shell_front_menubar(shell_scene_t *s, const MenuBar *foreground);

/* Paint desktop/windows, one menu bar at rows [0,20), then the modal.
 * NULL foreground uses desktop_bar. The scene must have been built. */
void shell_render(shell_scene_t *s, const bitmap_t *dst);
void shell_render_with_menubar(shell_scene_t *s, const bitmap_t *dst,
                               const MenuBar *foreground);

/* A bounded view beginning at y_top; shares pixels, pitch and depth with dst.
 * Used by the menu tracker to translate its port, not to stack menu bars. */
void make_offset_view(bitmap_t *view, const bitmap_t *dst, uint32_t y_top);

#endif /* INITECH_OS_FLAIR_SHELL_H */
