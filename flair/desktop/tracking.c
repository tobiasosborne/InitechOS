/* Extracted production tracking; reference: legacy-final kmain.c. */
#include "desktop_internal.h"

int flair_live_window_index(const flair_live_ctx_t *ctx, WindowPtr w)
{
    int i;
    for (i = 0; i < ctx->n_windows; i++) {
        if (&ctx->windows[i].rec == w) {
            return i;
        }
    }
    return WindowMgr_window_index(ctx->wm, w);
}

void flair_live_content_phase(flair_live_ctx_t *ctx)
{
    if (ctx->plist != (FlairProcessList *)0) {
        flair_route_updates(ctx->plist, ctx->wm);
        return;
    }
    desktop_validate_all(ctx->wm);
}

__attribute__((noinline)) void flair_live_repaint(flair_live_ctx_t *ctx)
{
    desktop_paint_damage(ctx->wm, &ctx->off, ctx->comp);
    flair_live_content_phase(ctx);
}

__attribute__((noinline)) void flair_live_repaint_present(flair_live_ctx_t *ctx,
                                                                 const boot_info_t *bi)
{
    flair_live_repaint(ctx);
    flair_desktop_present(bi, &ctx->off);
}

MenuBar *flair_live_tenant_bar(flair_live_ctx_t *ctx,
                                      FlairProcessList *list)
{
    MenuBar *bar = finder_menu_bar();
    (void)ctx;
    if (list != (FlairProcessList *)0 && list->head != (FlairApp *)0 &&
        list->head->menubar != (MenuBar *)0)
        bar = list->head->menubar;
    flair_live_refresh_finder_bar(bar);
    return bar;
}

void flair_live_finish_tenant_switch(flair_live_ctx_t *ctx,
                                            const boot_info_t *bi,
                                            FlairProcessList *list,
                                            FlairApp *prev_fg,
                                            GrafPort *barport)
{
    if (list->head == (FlairApp *)0 || list->head == prev_fg) return;
    desktop_paint_damage(ctx->wm, &ctx->off, ctx->comp);
    flair_route_updates(list, ctx->wm);
    DrawMenuBar(barport, shell_front_menubar(ctx->scene,
                flair_live_tenant_bar(ctx, list)), -1,
                (const region_t *)0);
    flair_desktop_present(bi, &ctx->off);
    serial_puts("FLAIR-DISPATCH app=");
    serial_puts(list->head->name ? list->head->name : "?");
    serial_putc('\n');
}

void flair_live_clamp_drag(const flair_live_ctx_t *ctx, WindowPtr w,
                                  int16_t *dh, int16_t *dv)
{
    rgn_rect_t s = region_get_bbox(w->strucRgn);
    rgn_rect_t frame = ctx->wm->desktop_frame;
    int32_t width = (int32_t)s.right - (int32_t)s.left;
    int32_t left = (int32_t)s.left + (int32_t)*dh;
    int32_t top = (int32_t)s.top + (int32_t)*dv;
    int32_t min_left = (int32_t)frame.left + 4 - width;
    int32_t max_left = (int32_t)frame.right - 4;
    int32_t min_top = (int32_t)SHELL_MENUBAR_H;
    int32_t max_top = (int32_t)frame.bottom - (int32_t)FLAIR_CHROME_TITLEBAR_H;
    if (min_top < (int32_t)frame.top) min_top = (int32_t)frame.top;
    if (left < min_left) left = min_left;
    if (left > max_left) left = max_left;
    if (top < min_top) top = min_top;
    if (top > max_top) top = max_top;
    *dh = (int16_t)(left - (int32_t)s.left);
    *dv = (int16_t)(top - (int32_t)s.top);
}

flair_live_outline_t g_flair_live_outline;

int flair_live_rect_same(rgn_rect_t a, rgn_rect_t b)
{
    return a.top == b.top && a.left == b.left &&
           a.bottom == b.bottom && a.right == b.right;
}

void flair_live_outline_pixel(flair_live_outline_t *outline,
                                     bitmap_t *surface,
                                     int x, int y, uint32_t ink,
                                     int restore, uint32_t *index)
{
    uint32_t off;
    if (x < 0 || y < 0 || x >= (int)surface->width ||
        y >= (int)surface->height) return;
    if (*index >= (2u * ((uint32_t)FLAIR_SCREEN_W + (uint32_t)FLAIR_SCREEN_H) + 4u)) {
        serial_puts("PANIC flair-outline: save-under capacity exceeded\nHALTED\n");
        for (;;) { __asm__ __volatile__("cli; hlt"); }
    }
    off = (uint32_t)y * surface->pitch +
          (uint32_t)x * surface->bytes_per_pixel;
    if (restore) {
        surface_put_pixel(surface, off, outline->saved[*index]);
    } else {
        outline->saved[*index] = (uint8_t)surface_get_pixel(surface, off);
        surface_put_pixel(surface, off, ink);
    }
    (*index)++;
}

void flair_live_outline_walk(flair_live_outline_t *outline,
                                    bitmap_t *surface, uint32_t ink,
                                    int restore)
{
    rgn_rect_t r = outline->rect;
    uint32_t index = 0u;
    int x;
    int y;
    for (x = r.left; x < r.right; x++)
        flair_live_outline_pixel(outline, surface, x, r.top, ink,
                                 restore, &index);
    if (r.bottom - r.top > 1) {
        for (x = r.left; x < r.right; x++)
            flair_live_outline_pixel(outline, surface, x, r.bottom - 1, ink,
                                     restore, &index);
    }
    for (y = r.top + 1; y < r.bottom - 1; y++)
        flair_live_outline_pixel(outline, surface, r.left, y, ink,
                                 restore, &index);
    if (r.right - r.left > 1) {
        for (y = r.top + 1; y < r.bottom - 1; y++)
            flair_live_outline_pixel(outline, surface, r.right - 1, y, ink,
                                     restore, &index);
    }
    if (restore && index != outline->saved_count) {
        serial_puts("PANIC flair-outline: save-under traversal drift\nHALTED\n");
        for (;;) { __asm__ __volatile__("cli; hlt"); }
    }
    if (!restore) outline->saved_count = index;
}

void flair_live_outline_draw(bitmap_t *surface, rgn_rect_t rect)
{
    uint32_t gray;
    if (g_flair_live_outline.drawn) {
        serial_puts("PANIC flair-outline: nested draw\nHALTED\n");
        for (;;) { __asm__ __volatile__("cli; hlt"); }
    }
    if (surface->bpp != 8u || surface->bytes_per_pixel != 1u) {
        serial_puts("PANIC flair-outline: feedback surface is not indexed-8\nHALTED\n");
        for (;;) { __asm__ __volatile__("cli; hlt"); }
    }
    g_flair_live_outline.rect = rect;
    gray = flair_look_pixel_depth(surface->bpp,
                                  FLAIR_PART_PLAT_INACTIVE_FRAME);
    flair_live_outline_walk(&g_flair_live_outline, surface, gray, 0);
    g_flair_live_outline.drawn = 1;
}

void flair_live_outline_restore(bitmap_t *surface)
{
    if (!g_flair_live_outline.drawn) return;
    flair_live_outline_walk(&g_flair_live_outline, surface, 0u, 1);
    g_flair_live_outline.drawn = 0;
    g_flair_live_outline.saved_count = 0u;
}

void flair_live_drag_delta(const flair_live_ctx_t *ctx, WindowPtr w,
                                  flair_point_t where0, flair_point_t where,
                                  int16_t *dh, int16_t *dv)
{
    *dh = (int16_t)(where.h - where0.h);
    *dv = (int16_t)(where.v - where0.v);
    flair_live_clamp_drag(ctx, w, dh, dv);
}

int flair_live_track_cut(uint32_t t0, int *cancel)
{
    (void)t0;
    *cancel = 0;
    return 0;
}

void flair_live_do_drag(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               WindowPtr w, flair_point_t where0)
{
    EventRecord up;
    flair_point_t where1 = where0;
    uint32_t t0 = flair_tick_count();
    int cancel = 0;
    rgn_rect_t before, after;
    rgn_rect_t outline_base, outline_now;
    int16_t dh, dv;
    int wid;
    before = region_get_bbox(w->strucRgn);
    outline_base = WindowFrameRect(w);
    outline_now = outline_base;
    flair_live_outline_draw(&ctx->off, outline_now);
    flair_desktop_present(bi, &ctx->off);
    for (;;) {
        int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &up, 3u);
        flair_live_cursor_track(&up);
        if (g) {
            flair_live_emit_evt(&up);
        }
        where1 = up.where;
        flair_live_drag_delta(ctx, w, where0, where1, &dh, &dv);
        {
            rgn_rect_t next = outline_base;
            next.left = (int16_t)(next.left + dh);
            next.right = (int16_t)(next.right + dh);
            next.top = (int16_t)(next.top + dv);
            next.bottom = (int16_t)(next.bottom + dv);
            if (!flair_live_rect_same(next, outline_now)) {
                flair_live_outline_restore(&ctx->off);
                flair_live_outline_draw(&ctx->off, next);
                outline_now = next;
                flair_desktop_present(bi, &ctx->off);
            }
        }
        if (g && up.what == (uint16_t)mouseUp) break;
        if (flair_live_track_cut(t0, &cancel)) break;
    }
    flair_live_outline_restore(&ctx->off);
    flair_live_drag_delta(ctx, w, where0, where1, &dh, &dv);
    if (cancel) { dh = 0; dv = 0; }
    if (dh != 0 || dv != 0) {
        DragWindow(ctx->wm, w, dh, dv);
        WindowMgr_invalidate(ctx->wm, w, region_get_bbox(w->strucRgn));
    }
    flair_live_repaint_present(ctx, bi);
    after = region_get_bbox(w->strucRgn);
    wid = flair_live_window_index(ctx, w);
    serial_puts("FLAIR-DRAG win ");
    serial_puti((int32_t)wid);
    serial_puts(" (");
    serial_puti((int32_t)before.left);
    serial_putc(',');
    serial_puti((int32_t)before.top);
    serial_puts(")->(");
    serial_puti((int32_t)after.left);
    serial_putc(',');
    serial_puti((int32_t)after.top);
    serial_puts(")\n");
}

int flair_live_box_hit(const flair_live_ctx_t *ctx, WindowPtr w,
                              flair_part_code_t pc, flair_point_t pt)
{
    WindowPtr hw = (WindowPtr)0;
    flair_part_code_t at = FindWindow(ctx->wm, pt, &hw);
    if (at == inZoomOut) at = inZoomIn;
    if (pc == inZoomOut) pc = inZoomIn;
    return hw == w && at == pc;
}

int flair_live_track_box(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                WindowPtr w, flair_part_code_t pc)
{
    GrafPort port;
    rgn_rect_t whole = { 0, 0, (int16_t)ctx->off.height,
                         (int16_t)ctx->off.width };
    rgn_rect_t frame = WindowFrameRect(w);
    EventRecord up;
    uint32_t t0 = flair_tick_count();
    int cancel = 0, inside = 1, drawn = 0;
    ComputeVisible(ctx->wm, w, ctx->comp);
    port.portBits.bm = ctx->off;
    port.portBits.bounds = whole;
    port.portRect = whole;
    port.visRgn = ctx->comp;
    port.clipRgn = (region_t *)0;
    port.pnLoc.v = 0; port.pnLoc.h = 0;
    port.pnSize.v = 1; port.pnSize.h = 1;
    port.pnVis = 0;
    port.grafProcs = (QDProcs *)0;
    for (;;) {
        int g;
        if (inside != drawn && w->windowDefProcVariant != rDocProc) {
            flair_draw_window_widget(&port, flair_look_default_skin(), frame,
                                     (int)pc, inside);
            flair_desktop_present(bi, &ctx->off);
            drawn = inside;
        }
        g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &up, 3u);
        flair_live_cursor_track(&up);
        if (g) flair_live_emit_evt(&up);
        inside = flair_live_box_hit(ctx, w, pc, up.where);
        if (g && up.what == (uint16_t)mouseUp) break;
        if (flair_live_track_cut(t0, &cancel)) break;
    }
    if (drawn) {
        flair_draw_window_widget(&port, flair_look_default_skin(), frame,
                                 (int)pc, 0);
        flair_desktop_present(bi, &ctx->off);
    }
    region_set_empty(ctx->comp);
    if (cancel) inside = 0;
    serial_puts("FLAIR-TRACKBOX win ");
    serial_puti((int32_t)flair_live_window_index(ctx, w));
    serial_puts(" part ");
    serial_puti((int32_t)pc);
    serial_puts(inside ? " in=1\n" : " in=0\n");
    return inside;
}

void flair_live_scroll_report(const flair_live_ctx_t *ctx, WindowPtr w,
                                     int axis, int part,
                                     const WindowScrollAxis *a)
{
    serial_puts("FLAIR-SCROLL win ");
    serial_puti((int32_t)flair_live_window_index(ctx, w));
    serial_puts(axis == WSCROLL_V ? " v part=" : " h part=");
    serial_puti((int32_t)part);
    serial_puts(" value=");
    serial_puti((int32_t)a->value);
    serial_puts(" max=");
    serial_puti((int32_t)a->max);
    serial_putc('\n');
}

void flair_live_scroll_repaint(flair_live_ctx_t *ctx,
                                      const boot_info_t *bi, WindowPtr w,
                                      int axis, int content)
{
    rgn_rect_t f = WindowFrameRect(w);
    WindowMgr_invalidate(ctx->wm, w, wscroll_bar_rect(f, axis));
    if (content)
        WindowMgr_invalidate(ctx->wm, w, region_get_bbox(w->contRgn));
    flair_live_repaint_present(ctx, bi);
}

void flair_live_track_scroll(flair_live_ctx_t *ctx,
                                    const boot_info_t *bi, WindowPtr w,
                                    flair_point_t where0)
{
    int axis = -1, ax = -1;
    int part = WindowScrollFindPart(ctx->wm, w, where0, &axis);
    WindowScroll *ws = WindowScrollOf(ctx->wm, w);
    WindowScrollAxis *a;
    rgn_rect_t bar;
    EventRecord up;
    uint32_t t0 = flair_tick_count();
    int cancel = 0;
    if (axis < 0) return;
    if (part == WSCROLL_PART_NONE || ws == (WindowScroll *)0) {
        serial_puts("FLAIR-SCROLL-IGNORED win ");
        serial_puti((int32_t)flair_live_window_index(ctx, w));
        serial_puts(axis == WSCROLL_V ? " v\n" : " h\n");
        return;
    }
    a = &ws->axis[axis];
    bar = wscroll_bar_rect(WindowFrameRect(w), axis);
    if (part == WSCROLL_PART_THUMB) {
        int lo = wscroll_track_lo(bar, axis);
        int hi = lo + wscroll_track_span(bar, axis);
        int pos0 = wscroll_thumb_pos(bar, axis, a);
        int p0 = (axis == WSCROLL_V) ? where0.v : where0.h;
        a->drag_pos = (int16_t)pos0;
        for (;;) {
            int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &up, 3u);
            int p, pos;
            flair_live_cursor_track(&up);
            if (g) flair_live_emit_evt(&up);
            p = (axis == WSCROLL_V) ? up.where.v : up.where.h;
            pos = pos0 + (p - p0);
            if (pos < lo) pos = lo;
            if (pos > hi) pos = hi;
            if (pos != a->drag_pos) {
                a->drag_pos = (int16_t)pos;
                flair_live_scroll_repaint(ctx, bi, w, axis, 0);
            }
            if (g && up.what == (uint16_t)mouseUp) break;
            if (flair_live_track_cut(t0, &cancel)) break;
        }
        {
            int16_t v = cancel ? a->value
                               : wscroll_value_from_thumb(bar, axis, a,
                                                          a->drag_pos);
            int changed;
            a->drag_pos = -1;
            changed = wscroll_set_value(a, v);
            flair_live_scroll_repaint(ctx, bi, w, axis, changed);
            flair_live_scroll_report(ctx, w, axis, part, a);
        }
        return;
    }
    {
        int is_arrow = (part == WSCROLL_PART_UP || part == WSCROLL_PART_DOWN);
        uint32_t next = t0 + (uint32_t)WSCROLL_REPEAT_DELAY;
        int inside = 1;
        a->hilite = (int16_t)(is_arrow ? part : 0);
        {
            int changed = wscroll_step(a, part);
            flair_live_scroll_repaint(ctx, bi, w, axis, changed);
            if (changed) flair_live_scroll_report(ctx, w, axis, part, a);
        }
        for (;;) {
            int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &up, 1u);
            int now_inside;
            uint32_t now;
            flair_live_cursor_track(&up);
            if (g) flair_live_emit_evt(&up);
            if (g && up.what == (uint16_t)mouseUp) break;
            if (flair_live_track_cut(t0, &cancel)) break;
            now_inside = WindowScrollFindPart(ctx->wm, w, up.where, &ax) == part &&
                         ax == axis;
            if (is_arrow && now_inside != inside) {
                a->hilite = (int16_t)(now_inside ? part : 0);
                flair_live_scroll_repaint(ctx, bi, w, axis, 0);
            }
            inside = now_inside;
            now = flair_tick_count();
            if (inside && (int32_t)(now - next) >= 0) {
                if (wscroll_step(a, part)) {
                    flair_live_scroll_repaint(ctx, bi, w, axis, 1);
                    flair_live_scroll_report(ctx, w, axis, part, a);
                }
                next = now + (uint32_t)WSCROLL_REPEAT_RATE;
            }
        }
        if (a->hilite != 0) {
            a->hilite = 0;
            flair_live_scroll_repaint(ctx, bi, w, axis, 0);
        }
    }
}

void flair_live_do_close(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                WindowPtr w, GrafPort *tenant_barport)
{
    int wid = flair_live_window_index(ctx, w);
    const char *terminated_name = (const char *)0;
    int terminated = 0;
    if (tbx_close_ask(w)) {
        serial_puts("FLAIR-CLOSE-ASK win ");
        serial_puti((int32_t)wid);
        serial_putc('\n');
        flair_live_tenant_heard(ctx, bi);
        return;
    }
    if (ctx->plist != (FlairProcessList *)0 &&
        ctx->master != (flair_heap_t *)0) {
        terminated = FlairProcess_close_window(ctx->plist, ctx->wm, ctx->master,
                                                w, &terminated_name);
        (void)tbx_reap();
    } else {
        HideWindow(ctx->wm, w);
    }
    flair_live_repaint(ctx);
    if (tenant_barport != (GrafPort *)0)
        DrawMenuBar(tenant_barport,
                    shell_front_menubar(ctx->scene,
                        flair_live_tenant_bar(ctx, ctx->plist)), -1,
                    (const region_t *)0);
    flair_desktop_present(bi, &ctx->off);
    if (terminated) {
        serial_puts("FLAIR-TENANT-EXIT name=");
        serial_puts(terminated_name != (const char *)0 ? terminated_name : "?");
        serial_putc('\n');
    }
    serial_puts("FLAIR-CLOSE win ");
    serial_puti((int32_t)wid);
    serial_putc('\n');
}

void flair_live_do_zoom(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               WindowPtr w)
{
    int wid = flair_live_window_index(ctx, w);
    int zoomed = ZoomWindow(ctx->wm, w);
    flair_live_repaint_present(ctx, bi);
    serial_puts("FLAIR-ZOOM win ");
    serial_puti((int32_t)wid);
    serial_puts(zoomed ? " in\n" : " out\n");
}

void flair_live_do_collapse(flair_live_ctx_t *ctx,
                                   const boot_info_t *bi, WindowPtr w)
{
    int wid = flair_live_window_index(ctx, w);
    int collapsed = CollapseWindow(ctx->wm, w);
    flair_live_repaint_present(ctx, bi);
    serial_puts("FLAIR-COLLAPSE win ");
    serial_puti((int32_t)wid);
    serial_putc(' ');
    serial_puti((int32_t)collapsed);
    serial_putc('\n');
}

void flair_live_do_grow(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               WindowPtr w, flair_point_t where0)
{
    EventRecord up;
    flair_point_t where1 = where0;
    rgn_rect_t before = WindowFrameRect(w);
    rgn_rect_t outline_now = before;
    int16_t old_w = (int16_t)(before.right - before.left);
    int16_t old_h = (int16_t)(before.bottom - before.top);
    int16_t width = old_w;
    int16_t height = old_h;
    uint32_t t0 = flair_tick_count();
    int cancel = 0;
    int wid = flair_live_window_index(ctx, w);
    flair_live_outline_draw(&ctx->off, outline_now);
    flair_desktop_present(bi, &ctx->off);
    for (;;) {
        int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &up, 3u);
        flair_live_cursor_track(&up);
        if (g) flair_live_emit_evt(&up);
        where1 = up.where;
        width = (int16_t)(old_w + where1.h - where0.h);
        height = (int16_t)(old_h + where1.v - where0.v);
        ConstrainWindowSize(ctx->wm, w, &width, &height);
        {
            rgn_rect_t next = before;
            next.right = (int16_t)(next.left + width);
            next.bottom = (int16_t)(next.top + height);
            if (!flair_live_rect_same(next, outline_now)) {
                flair_live_outline_restore(&ctx->off);
                flair_live_outline_draw(&ctx->off, next);
                outline_now = next;
                flair_desktop_present(bi, &ctx->off);
            }
        }
        if (g && up.what == (uint16_t)mouseUp) break;
        if (flair_live_track_cut(t0, &cancel)) break;
    }
    flair_live_outline_restore(&ctx->off);
    if (cancel) where1 = where0;
    width = (int16_t)(old_w + where1.h - where0.h);
    height = (int16_t)(old_h + where1.v - where0.v);
    ConstrainWindowSize(ctx->wm, w, &width, &height);
    if (width != old_w || height != old_h)
        SizeWindow(ctx->wm, w, width, height);
    flair_live_repaint_present(ctx, bi);
    serial_puts("FLAIR-GROW win ");
    serial_puti((int32_t)wid);
    serial_puts(" (");
    serial_puti((int32_t)old_w);
    serial_putc(',');
    serial_puti((int32_t)old_h);
    serial_puts(")->(");
    serial_puti((int32_t)width);
    serial_putc(',');
    serial_puti((int32_t)height);
    serial_puts(")\n");
}

void flair_live_init_menu_port(GrafPort *port, const bitmap_t *view)
{
    rgn_rect_t whole = { 0, 0, (int16_t)view->height,
                         (int16_t)view->width };
    port->portBits.bm = *view;
    port->portBits.bounds = whole;
    port->portRect = whole;
    port->visRgn = (region_t *)0;
    port->clipRgn = (region_t *)0;
    port->pnLoc.v = 0; port->pnLoc.h = 0;
    port->pnSize.v = 1; port->pnSize.h = 1;
    port->pnVis = 0;
    port->grafProcs = (QDProcs *)0;
}

void flair_live_erase_menu_panel(flair_live_ctx_t *ctx,
                                        const MenuBar *bar, int mi,
                                        uint32_t y_top)
{
    rgn_rect_t panel = MenuInfo_panel_footprint_rect(bar, mi);
    panel.top = (int16_t)(panel.top + (int16_t)y_top);
    panel.bottom = (int16_t)(panel.bottom + (int16_t)y_top);
    WindowMgr_invalidate_desktop(ctx->wm, panel);
    flair_live_repaint(ctx);
    {
        bitmap_t owner_view;
        GrafPort owner_port;
        make_offset_view(&owner_view, &ctx->off, y_top);
        flair_live_init_menu_port(&owner_port, &owner_view);
        DrawMenuBar(&owner_port, shell_front_menubar(ctx->scene, bar), -1,
                    (const region_t *)0);
    }
}

uint32_t flair_live_do_menu_at(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                      MenuBar *bar, flair_point_t where0,
                                      uint32_t y_top)
{
    bar = (MenuBar *)shell_front_menubar(ctx->scene, bar);
    if (bar->has_apple && where0.h < (int16_t)FLAIR_MENU_APPLE_W)
        flair_live_apple_refresh();
    int mi = MenuBar_hit(bar, (int)where0.h);
    int16_t menuID = MenuBar_menu_id(bar, mi);
    flair_point_t menu_where0 = where0;
    menu_where0.v = (int16_t)(menu_where0.v - (int16_t)y_top);
    MenuTrackState st;
    MenuTrack_begin(bar, menu_where0, &st);
    mi = st.mi;
    if (!MenuBar_menu(bar, mi)) {
        serial_puts("FLAIR-MENU menu=0 item=0 (sel=0x00000000)\n");
        return 0u;
    }
    bitmap_t panel_view;
    make_offset_view(&panel_view, &ctx->off, y_top);
    GrafPort port;
    flair_live_init_menu_port(&port, &panel_view);
    HiliteMenu(&port, bar, mi, (const region_t *)0);
    flair_draw_menu_panel(&port, bar, mi, -1, (const region_t *)0);
    flair_desktop_present(bi, &ctx->off);
    serial_puts("FLAIR-MENU-DROP menu=");
    serial_puti((int32_t)menuID);
    serial_putc('\n');
    int last_mi = mi;
    int last_hi = -1;
    uint32_t t0 = flair_tick_count();
    int cancel = 0;
    for (;;) {
        EventRecord mev;
        int got = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &mev, 3u);
        flair_live_cursor_track(&mev);
        if (got) {
            flair_live_emit_evt(&mev);
        }
        flair_point_t menu_where = mev.where;
        menu_where.v = (int16_t)(menu_where.v - (int16_t)y_top);
        MenuTrack_step(bar, &st, menu_where);
        mi = st.mi;
        int hi = MenuInfo_item_at(bar, mi, (int)menu_where.h,
                                  (int)menu_where.v);
        if (mi != last_mi || hi != last_hi) {
            if (mi != last_mi) {
                flair_live_erase_menu_panel(ctx, bar, last_mi, y_top);
            }
            HiliteMenu(&port, bar, mi, (const region_t *)0);
            flair_draw_menu_panel(&port, bar, mi, hi,
                                  (const region_t *)0);
            flair_desktop_present(bi, &ctx->off);
            if (mi != last_mi) {
                serial_puts("FLAIR-MENU-XDROP menu=");
                serial_puti((int32_t)MenuBar_menu_id(bar, mi));
                serial_putc('\n');
            }
            last_mi = mi;
            last_hi = hi;
        }
        if (got && mev.what == (uint16_t)mouseUp) {
            break;
        }
        if (flair_live_track_cut(t0, &cancel)) {
            break;
        }
    }
    uint32_t sel = cancel ? 0u : MenuTrack_result(bar, &st);
    int sel_item = (int)MenuResultItem(sel);
    flair_live_erase_menu_panel(ctx, bar, last_mi, y_top);
    flair_desktop_present(bi, &ctx->off);
    serial_puts("FLAIR-MENU menu=");
    serial_puti((int32_t)menuID);
    serial_puts(" item=");
    serial_putu((uint32_t)sel_item);
    serial_puts(" (sel=0x");
    serial_puthex32(sel);
    serial_puts(")\n");
    return sel;
}

__attribute__((unused))
void flair_live_do_menu(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               MenuBar *bar, flair_point_t where0)
{
    (void)flair_live_do_menu_at(ctx, bi, bar, where0, 0u);
}

finder_shell_t *g_finder_shell;

finder_desk_t *g_finder_desk;

finder_click_track_t g_finder_click;

int g_finder_db_dirty;

FinderCtx g_finder_ctx;

FlairProcessList *g_ten_plist;

FlairApp *g_ten_finder;

GrafPort *g_ten_barport;

finder_win_status_t g_finder_io_error;
