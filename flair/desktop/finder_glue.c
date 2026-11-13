/* Extracted production finder_glue; reference: legacy-final kmain.c. */
#include "desktop_internal.h"

void finder_desk_sync_ctx(void)
{
    if (g_finder_shell != (finder_shell_t *)0) {
        finder_shell_sync_ctx(g_finder_shell);
        return;
    }
    g_finder_ctx.selection_count =
        (g_finder_desk != (finder_desk_t *)0)
            ? finder_desk_selection_count(g_finder_desk) : 0u;
}

void flair_live_refresh_finder_bar(MenuBar *bar)
{
    if (bar != finder_menu_bar()) return;
    finder_desk_sync_ctx();
    finder_menu_refresh_enables(&g_finder_ctx);
}

int flair_live_finder_is_foreground(void)
{
    return g_ten_plist != (FlairProcessList *)0 &&
           g_ten_finder != (FlairApp *)0 &&
           g_ten_plist->head == g_ten_finder;
}

void finder_surface_desktop(finder_surface_t *s)
{
    s->fd = g_finder_desk;
    s->click = &g_finder_click;
    s->slot = -1;
}

int finder_surface_window(finder_surface_t *s, int slot)
{
    finder_desk_t *fd;
    if (g_finder_shell == (finder_shell_t *)0 ||
        slot < 0 || slot >= g_finder_shell->window_cap ||
        !g_finder_shell->windows[slot].open)
        return 0;
    fd = finder_win_view(g_finder_shell, slot);
    if (fd == (finder_desk_t *)0) return 0;
    s->fd = fd;
    s->click = &g_finder_shell->windows[slot].click;
    s->slot = slot;
    return 1;
}

void finder_surface_invalidate_all(const finder_surface_t *s,
                                          flair_live_ctx_t *ctx)
{
    if (s->fd == (finder_desk_t *)0) return;
    if (s->slot < 0) {
        for (int i = 0; i < (int)s->fd->n; i++)
            WindowMgr_invalidate_desktop(ctx->wm,
                                         finder_desk_cell_rect(s->fd, i));
        return;
    }
    finder_win_invalidate_all(g_finder_shell, s->slot);
}

void finder_surface_invalidate(const finder_surface_t *s,
                                      flair_live_ctx_t *ctx,
                                      rgn_rect_t old_rect, rgn_rect_t new_rect)
{
    if (s->slot < 0) {
        finder_desk_invalidate(ctx->wm, old_rect, new_rect);
        return;
    }
    finder_win_invalidate(g_finder_shell, s->slot, old_rect, new_rect);
}

void finder_desk_repaint(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    if (g_ten_plist != (FlairProcessList *)0)
        (void)FlairProcess_sync_active(g_ten_plist, ctx->wm);
    if (g_finder_shell != (finder_shell_t *)0 && g_finder_shell->trash_icon_dirty) {
        int t = finder_desk_find_kind(g_finder_desk, FINDER_ICON_TRASH);
        g_finder_shell->trash_icon_dirty = 0u;
        if (t >= 0)
            WindowMgr_invalidate_desktop(ctx->wm,
                                         finder_desk_cell_rect(g_finder_desk, t));
        serial_puts(g_finder_desk->trash_full ? "DESK-TRASH-ICON full\n"
                                              : "DESK-TRASH-ICON empty\n");
    }
    flair_live_repaint_present(ctx, bi);
}

void finder_report_open_fail(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                     finder_win_status_t st)
{
    if (st == FINDER_WIN_ERR_FULL) {
        serial_puts("FINDER-WIN-FULL\n");
    } else {
        serial_puts("FINDER-WIN-OPEN-FAIL rc=");
        serial_puti((int32_t)st);
        serial_putc('\n');
    }
    (void)finder_notice(ctx,bi,st,"");
}

void finder_report_dropped(int slot)
{
    uint16_t dropped;
    if (g_finder_shell == (finder_shell_t *)0 || slot < 0) return;
    dropped = g_finder_shell->windows[slot].dropped;
    if (dropped == 0u) return;
    serial_puts("FINDER-WIN-ICONS-FULL win=");
    serial_puti((int32_t)slot);
    serial_puts(" dropped=");
    serial_putu((uint32_t)dropped);
    serial_putc('\n');
}

void finder_open_volume(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    int slot = -1;
    int singleton = 0;
    finder_win_status_t st;
    if (g_finder_shell == (finder_shell_t *)0) return;
    st = finder_win_open(g_finder_shell, 0u, 0u , "", 1u,
                         &slot, &singleton);
    if (st != FINDER_WIN_OK) { finder_report_open_fail(ctx,bi,st); return; }
    finder_desk_repaint(ctx, bi);
    serial_puts("FINDER-OPEN-VOLUME win=");
    serial_puti((int32_t)slot);
    serial_puts(" n=");
    serial_putu((uint32_t)g_finder_shell->windows[slot].view.n);
    serial_putc('\n');
    finder_report_dropped(slot);
}

void finder_open_folder(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               uint16_t parent, uint16_t dir_start,
                               const char *name83)
{
    int slot = -1;
    int singleton = 0;
    finder_win_status_t st;
    if (g_finder_shell == (finder_shell_t *)0) return;
    st = finder_win_open(g_finder_shell, parent, dir_start, name83, 0u,
                         &slot, &singleton);
    if (st != FINDER_WIN_OK) { finder_report_open_fail(ctx,bi,st); return; }
    finder_desk_repaint(ctx, bi);
    serial_puts("FINDER-OPEN-FOLDER name=");
    serial_puts(name83);
    serial_puts(" win=");
    serial_puti((int32_t)slot);
    serial_puts(" singleton=");
    serial_puti((int32_t)singleton);
    serial_putc('\n');
    if (!singleton) finder_report_dropped(slot);
    serial_puts("FINDER-VIEW-RESTORE win="); serial_puti(slot);
    serial_puts(" bits="); serial_putu(g_finder_shell->windows[slot].view_bits); serial_putc('\n');
}

void finder_open_trash(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    int slot = -1;
    int singleton = 0;
    finder_win_status_t st;
    if (g_finder_shell == (finder_shell_t *)0) return;
    st = finder_win_open_trash(g_finder_shell, &slot, &singleton);
    if (st == FINDER_WIN_ERR_NOTRASH) {
        int rrc = finder_trash_reserve();
        if (rrc == FAT12_OK)
            st = finder_win_open_trash(g_finder_shell, &slot, &singleton);
        else if (rrc != FAT12_ERR_NO_SPACE && rrc != FAT12_ERR_DIR_FULL)
            st = FINDER_WIN_ERR_STORAGE;
    }
    if (st != FINDER_WIN_OK) { finder_report_open_fail(ctx,bi,st); return; }
    finder_desk_repaint(ctx, bi);
    serial_puts("FINDER-OPEN-TRASH win=");
    serial_puti((int32_t)slot);
    serial_puts(" n=");
    serial_putu((uint32_t)g_finder_shell->windows[slot].view.n);
    serial_puts(" singleton=");
    serial_puti((int32_t)singleton);
    serial_putc('\n');
    if (!singleton) finder_report_dropped(slot);
}

void finder_surface_open(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                const finder_surface_t *s, int idx)
{
    const finder_desk_icon_t *ic = &s->fd->icons[idx];
    if (s->slot < 0) {
        if (ic->kind == (uint8_t)FINDER_ICON_TRASH) {
            finder_open_trash(ctx, bi);
            return;
        }
        finder_open_volume(ctx, bi);
        return;
    }
    if (ic->kind == (uint8_t)FINDER_ICON_FOLDER) {
        finder_open_folder(ctx, bi, g_finder_shell->windows[s->slot].dir_start,
                           ic->dir_start, ic->name);
        return;
    }
    if (ic->kind == (uint8_t)FINDER_ICON_APP) {
        flair_live_launch_app(ctx, bi, ic->name,
                              g_finder_shell->windows[s->slot].dir_start);
        return;
    }
    serial_puts("FINDER-OPEN-ITEM NYI name=");
    serial_puts(ic->name);
    serial_putc('\n');
}

void flair_live_do_surface(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                  const EventRecord *ev,
                                  const finder_surface_t *s)
{
    finder_desk_t *fd = s->fd;
    EventRecord up;
    flair_point_t where0;
    flair_point_t where1;
    uint32_t t0;
    int cancel = 0;
    rgn_rect_t outline_now;
    int outline_live = 0;
    int moved = 0;
    int idx;
    int16_t dh = 0, dv = 0;
    finder_tgt_t hl;
    if (fd == (finder_desk_t *)0) return;
    hl.kind = (uint8_t)FINDER_TGT_NONE; hl.slot = -1; hl.idx = -1; hl.dir = 0u;
    where0 = ev->where;
    where1 = where0;
    idx = s->slot >= 0 ? finder_win_content_hit(&g_finder_shell->windows[s->slot],where0.h,where0.v)
                          : finder_desk_hit(fd, where0.h, where0.v);
    t0 = flair_tick_count();
    outline_now.top = 0; outline_now.left = 0;
    outline_now.bottom = 0; outline_now.right = 0;
    for (;;) {
        int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &up, 3u);
        rgn_rect_t next;
        flair_live_cursor_track(&up);
        if (g) flair_live_emit_evt(&up);
        where1 = up.where;
        dh = (int16_t)(where1.h - where0.h);
        dv = (int16_t)(where1.v - where0.v);
        if (!moved &&
            (finder_abs((int)dh) > FINDER_DRAG_SLOP ||
             finder_abs((int)dv) > FINDER_DRAG_SLOP)) {
            moved = 1;
        }
        if (moved && idx >= 0 && s->slot >= 0 &&
            g_finder_shell != (finder_shell_t *)0) {
            finder_tgt_t nt = finder_ops_resolve(g_finder_shell, s->slot, idx,
                                                 where1.h, where1.v);
            if (finder_ops_track_hilite(g_finder_shell, &hl, &nt)) {
                if (outline_live) {
                    flair_live_outline_restore(&ctx->off);
                    outline_live = 0;
                }
                finder_desk_repaint(ctx, bi);
                if (finder_ops_tgt_hilites(&hl)) {
                    const finder_desk_t *tfd = (hl.slot < 0)
                        ? &g_finder_shell->desk
                        : finder_win_view(g_finder_shell, (int)hl.slot);
                    serial_puts("FINDER-DROP-HILITE name=");
                    serial_puts(tfd->icons[(int)hl.idx].name);
                    serial_putc('\n');
                }
            }
        }
        if (moved) {
            next = (idx >= 0)
                 ? finder_desk_drag_outline(fd, idx, dh, dv)
                 : finder_band_rect(where0.h, where0.v, where1.h, where1.v);
            if (idx >= 0 && s->slot >= 0) {
                next = finder_desk_cell_rect(fd, idx);
                next.left = (int16_t)(next.left + dh);
                next.right = (int16_t)(next.right + dh);
                next.top = (int16_t)(next.top + dv);
                next.bottom = (int16_t)(next.bottom + dv);
            }
            if (!outline_live || !flair_live_rect_same(next, outline_now)) {
                if (outline_live) flair_live_outline_restore(&ctx->off);
                flair_live_outline_draw(&ctx->off, next);
                outline_now = next;
                outline_live = 1;
                flair_desktop_present(bi, &ctx->off);
            }
        }
        if (g && up.what == (uint16_t)mouseUp) break;
        if (flair_live_track_cut(t0, &cancel)) break;
    }
    if (outline_live) {
        flair_live_outline_restore(&ctx->off);
        flair_desktop_present(bi, &ctx->off);
    }
    if (cancel) {
        if (g_finder_shell != (finder_shell_t *)0) {
            finder_tgt_t none;
            none.kind = (uint8_t)FINDER_TGT_NONE; none.slot = -1;
            none.idx = -1; none.dir = 0u;
            if (finder_ops_track_hilite(g_finder_shell, &hl, &none))
                finder_desk_repaint(ctx, bi);
        }
        return;
    }
    if (!moved && idx < 0) {
        uint16_t before = finder_desk_selection_count(fd);
        (void)finder_click_classify(s->click, -1, where0.h, where0.v, ev->when);
        finder_desk_deselect_all(fd);
        if (before != 0u) {
            finder_surface_invalidate_all(s, ctx);
            finder_desk_repaint(ctx, bi);
        }
        if (s->slot < 0) {
            serial_puts("FINDER-ICON-DESELECT-ALL\n");
        } else {
            serial_puts("FINDER-WIN-DESELECT-ALL win=");
            serial_puti((int32_t)s->slot);
            serial_putc('\n');
        }
    } else if (!moved) {
        finder_click_kind_t k =
            finder_click_classify(s->click, (int16_t)idx,
                                  where0.h, where0.v, ev->when);
        if (k == FINDER_CLICK_DOUBLE) {
            finder_surface_open(ctx, bi, s, idx);
        } else {
            if ((ev->modifiers & (uint16_t)FLAIR_EVT_MOD_SHIFT_KEY) != 0u) {
                finder_desk_select_extend(fd, idx);
            } else {
                finder_desk_select_only(fd, idx);
            }
            finder_surface_invalidate_all(s, ctx);
            finder_desk_repaint(ctx, bi);
            if (s->slot < 0) {
                serial_puts("FINDER-ICON-SELECT name=");
                serial_puts(fd->icons[idx].name);
                serial_puts(" count=");
                serial_putu((uint32_t)finder_desk_selection_count(fd));
                serial_putc('\n');
            } else {
                serial_puts("FINDER-WIN-SELECT win=");
                serial_puti((int32_t)s->slot);
                serial_puts(" name=");
                serial_puts(fd->icons[idx].name);
                serial_puts(" count=");
                serial_putu((uint32_t)finder_desk_selection_count(fd));
                serial_putc('\n');
            }
        }
    } else if (idx < 0) {
        uint16_t n = finder_desk_marquee_select(
            fd, finder_band_rect(where0.h, where0.v, where1.h, where1.v));
        finder_surface_invalidate_all(s, ctx);
        finder_desk_repaint(ctx, bi);
        if (s->slot < 0) {
            serial_puts("FINDER-MARQUEE n=");
            serial_putu((uint32_t)n);
            serial_putc('\n');
        } else {
            serial_puts("FINDER-WIN-MARQUEE win=");
            serial_puti((int32_t)s->slot);
            serial_puts(" n=");
            serial_putu((uint32_t)n);
            serial_putc('\n');
        }
    } else if (s->slot >= 0 && g_finder_shell != (finder_shell_t *)0 &&
               finder_surface_drop(ctx, bi, s, idx, dh, dv, where1, &hl,
                                   outline_now)) {
    } else {
        rgn_rect_t old_cell, new_cell;
        finder_drop_t drop = finder_desk_drag_commit(fd, idx, dh, dv,
                                                     &old_cell, &new_cell);
        if (drop == FINDER_DROP_MOVED) {
            finder_surface_invalidate(s, ctx, old_cell, new_cell);
            finder_desk_repaint(ctx, bi);
            if (s->slot < 0) {
                serial_puts("FINDER-ICON-DRAG name=");
                serial_puts(fd->icons[idx].name);
                serial_puts(" x=");
                serial_puti((int32_t)fd->icons[idx].x);
                serial_puts(" y=");
                serial_puti((int32_t)fd->icons[idx].y);
                serial_putc('\n');
                g_finder_db_dirty = 1;
                finder_desk_persist();
            } else {
                serial_puts("FINDER-WIN-DRAG win=");
                serial_puti((int32_t)s->slot);
                serial_puts(" name=");
                serial_puts(fd->icons[idx].name);
                serial_puts(" x=");
                serial_puti((int32_t)fd->icons[idx].x);
                serial_puts(" y=");
                serial_puti((int32_t)fd->icons[idx].y);
                serial_putc('\n');
            }
        } else if (drop == FINDER_DROP_REVERT) {
            serial_puts("FINDER-ICON-DRAG-REVERT\n");
        }
    }
    finder_desk_sync_ctx();
}

void flair_live_do_desk(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               const EventRecord *ev)
{
    finder_surface_t s;
    finder_surface_desktop(&s);
    flair_live_do_surface(ctx, bi, ev, &s);
}

void finder_report_view(int slot)
{
    unsigned bits=g_finder_shell->windows[slot].view_bits;
    serial_puts("FINDER-VIEW win="); serial_puti(slot);
    if((bits & FINDER_VIEW_MASK)==0) { serial_puts(" mode=icons\n"); return; }
    if((bits & FINDER_VIEW_MASK)==2) serial_puts(" mode=small");
    else serial_puts(" mode=list");
    serial_puts(" sort=");
    serial_puts((bits&FINDER_VIEW_MASK)==5?"size":(bits&FINDER_VIEW_MASK)==9?"kind":(bits&FINDER_VIEW_MASK)==13?"date":"name");
    serial_puts(" bits=");serial_putu(bits);serial_putc('\n');
}

void flair_live_do_finder_content(flair_live_ctx_t *ctx,
                                         const boot_info_t *bi,
                                         const EventRecord *ev, int slot)
{
    finder_surface_t s;
    if (!finder_surface_window(&s, slot)) return;
    if (finder_win_header_click(&g_finder_shell->windows[slot], ev->where.h, ev->where.v)) {
        finder_win_invalidate_all(g_finder_shell, slot);
        finder_desk_repaint(ctx, bi);
        finder_report_view(slot);
        return;
    }
    flair_live_do_surface(ctx, bi, ev, &s);
}

void flair_live_do_finder_close(flair_live_ctx_t *ctx,
                                       const boot_info_t *bi, int slot)
{
    if (g_finder_shell == (finder_shell_t *)0) return;
    if (finder_win_close(g_finder_shell, slot) != FINDER_WIN_OK) return;
    finder_desk_repaint(ctx, bi);
    serial_puts("FINDER-CLOSE-WINDOW win=");
    serial_puti((int32_t)slot);
    serial_putc('\n');
    g_finder_db_dirty = 1;
    finder_desk_persist();
}

int flair_live_apple_installed(void *user, const char *dir83,
                                      const char *file83)
{
    dir_entry_t d, f;
    uint32_t slot;
    (void)user;
    if (g_finder_vol == (const fat12_volume_t *)0) return 0;
    if (fat12_find(g_finder_vol, g_finder_secbuf, dir83, &d) != FAT12_OK ||
        (d.attribute & 0x10u) == 0u)
        return 0;
    return fat12_find_slot_in(g_finder_vol, g_finder_fat, g_finder_fat_len,
                              d.start_cluster, g_finder_secbuf, file83, &f,
                              &slot) == FAT12_OK;
}

void flair_live_apple_refresh(void)
{
    const MenuInfo *m = apple_menu_build(flair_live_apple_installed, (void *)0);
    MenuBar_set_apple_menu(m);
    serial_puts("APPLE-MENU n=");
    serial_putu((uint32_t)m->n_items);
    serial_putc('\n');
}

int flair_live_apple_result(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                   uint32_t sel)
{
    const apple_da_t *da = (const apple_da_t *)0;
    dir_entry_t d;
    if (sel == 0u || MenuResultID(sel) != (int16_t)FLAIR_MENU_ID_APPLE) return 0;
    switch (apple_menu_action(sel, &da)) {
    case APPLE_ACT_ABOUT:
        flair_about_box(ctx, bi, "InitechDOS  Version 3.30");
        break;
    case APPLE_ACT_LAUNCH:
        serial_puts("APPLE-LAUNCH name=");
        serial_puts(da->file83);
        serial_puts(" dir=");
        serial_puts(da->dir83);
        serial_putc('\n');
        if (g_finder_vol != (const fat12_volume_t *)0 &&
            fat12_find(g_finder_vol, g_finder_secbuf, da->dir83, &d) == FAT12_OK)
            flair_live_launch_app(ctx, bi, da->file83, d.start_cluster);
        else
            serial_puts("APPLE-LAUNCH-FAIL\n");
        break;
    default:
        break;
    }
    return 1;
}

void finder_report_outcome(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    const finder_cmd_outcome_t *o;
    if (g_finder_shell == (finder_shell_t *)0) return;
    o = finder_shell_take_outcome(g_finder_shell);
    if (o == (const finder_cmd_outcome_t *)0) return;
    finder_outcome_run(ctx, bi, o);
}

void flair_live_sync_foreground(flair_live_ctx_t *ctx,
                                       const boot_info_t *bi,
                                       const EventRecord *ev)
{
    FlairApp *prev;
    FlairApp *want = (FlairApp *)0;
    WindowPtr w;
    if (g_ten_plist == (FlairProcessList *)0 || ctx->wm == (WindowMgr *)0)
        return;
    for (w = ctx->wm->front; w != (WindowPtr)0; w = w->nextWindow)
        if (w->visible) break;
    if (w != (WindowPtr)0 && g_finder_shell != (finder_shell_t *)0 &&
        finder_win_slot_of(g_finder_shell, w) >= 0)
        want = g_ten_finder;
    if (want != (FlairApp *)0 && want != g_ten_plist->head) {
        prev = g_ten_plist->head;
        if (FlairProcess_activate(g_ten_plist, ctx->wm, ev, want)) {
            flair_live_finish_tenant_switch(ctx, bi, g_ten_plist, prev,
                                            g_ten_barport);
            return;
        }
    }
    if (FlairProcess_sync_active(g_ten_plist, ctx->wm)) {
        flair_live_repaint_present(ctx, bi);
    }
}

void flair_live_desk_activate(flair_live_ctx_t *ctx,
                                     const boot_info_t *bi,
                                     const EventRecord *ev)
{
    FlairApp *prev;
    if (g_ten_plist == (FlairProcessList *)0 ||
        g_ten_finder == (FlairApp *)0 || ctx->wm == (WindowMgr *)0) return;
    if (g_ten_plist->head == g_ten_finder) return;
    prev = g_ten_plist->head;
    if (FlairProcess_activate(g_ten_plist, ctx->wm, ev, g_ten_finder))
        flair_live_finish_tenant_switch(ctx, bi, g_ten_plist, prev,
                                        g_ten_barport);
}

int flair_live_finder_key(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                 const EventRecord *ev)
{
    uint32_t sel;
    char ch;
    if (g_finder_shell == (finder_shell_t *)0) return 0;
    if (ev->what != (uint16_t)keyDown) return 0;
    if ((ev->modifiers & (uint16_t)(FLAIR_EVT_MOD_CONTROL_KEY |
                                    FLAIR_EVT_MOD_CMD_KEY)) == 0u) return 0;
    if (!flair_live_finder_is_foreground()) return 0;
    ch = (char)(ev->message & 0xFFu);
    flair_live_refresh_finder_bar(finder_menu_bar());
    sel = MenuKey(finder_menu_bar(), ch);
    if (sel == 0u) return 0;
    finder_shell_sync_ctx(g_finder_shell);
    finder_dispatch(&g_finder_ctx, sel, "key");
    finder_report_outcome(ctx, bi);
    finder_desk_sync_ctx();
    flair_live_sync_foreground(ctx, bi, ev);
    return 1;
}

__attribute__((unused))
void flair_live_finder_menu_result(flair_live_ctx_t *ctx,
                                          const boot_info_t *bi,
                                          const MenuBar *bar, uint32_t sel)
{
    if (g_finder_shell == (finder_shell_t *)0) return;
    if (bar != finder_menu_bar()) return;
    if (sel == 0u) return;
    finder_shell_sync_ctx(g_finder_shell);
    finder_dispatch(&g_finder_ctx, sel, "mouse");
    finder_report_outcome(ctx, bi);
    finder_desk_sync_ctx();
}
