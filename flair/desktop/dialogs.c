/* Extracted production dialogs; reference: legacy-final kmain.c. */
#include "desktop_internal.h"

rgn_rect_t fta_rect(int l, int t, int r, int b)
{
    rgn_rect_t x;
    x.left = (int16_t)l; x.top = (int16_t)t; x.right = (int16_t)r; x.bottom = (int16_t)b;
    return x;
}

void fta_rgn(finder_win_rgn_t *s)
{
    s->r.rows = s->rows;
    s->r.cap_rows = FINDER_WIN_RGN_ROWS;
    s->r.x_pool = s->pool;
    s->r.x_pool_cap = FINDER_WIN_RGN_POOL;
    region_set_empty(&s->r);
}

DialogRecord g_al_dr;

DialogItem g_al_it[3 + FINDER_ALERT_LINES + 1];

ControlRecord g_al_btn[3];

finder_win_rgn_t g_al_rs, g_al_rc, g_al_ru;

void flair_live_alert_open_icon(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                       const char *const *text, int n,
                                       const char *const *btn, int nb, int draw,
                                       unsigned icon)
{
    static const int16_t bx[3][2] = { { 435, 494 }, { 363, 422 }, { 211, 295 } };
    int k, ni = (icon != FTA_NO_ICON) ? 1 : 0;
    for (k = 0; k < nb; k++)
        control_init(&g_al_btn[k], pushButton, fta_rect(bx[k][0], 158, bx[k][1], 178),
                     0, 0, 1, 1, btn[k]);
    for (k = 0; k < nb + n; k++) {
        g_al_it[k].type = (k < nb) ? ctrlItem : statText;
        g_al_it[k].ctrl = (k < nb) ? &g_al_btn[k] : (ControlRecord *)0;
        g_al_it[k].rect = (k < nb) ? g_al_btn[k].contrlRect
                             : fta_rect(211, 97 + 16 * (k - nb),
                                        211 + FINDER_ALERT_TEXT_W,
                                        97 + 16 * (k - nb + 1));
        g_al_it[k].text = (k < nb) ? (const char *)0 : text[k - nb];
        g_al_it[k].enabled = (uint8_t)((k < nb) ? 1 : 0);
        g_al_it[k]._pad[0] = g_al_it[k]._pad[1] = g_al_it[k]._pad[2] = 0u;
    }
    if (ni) {
        g_al_it[k].type = iconItem;
        g_al_it[k].ctrl = (ControlRecord *)0;
        g_al_it[k].text = (const char *)0;
        g_al_it[k].rect = fta_rect(156, 100,
                                      156 + 32, 100 + 32);
        g_al_it[k].enabled = 0u;
        g_al_it[k]._pad[0] = (uint8_t)icon;
        g_al_it[k]._pad[1] = g_al_it[k]._pad[2] = 0u;
    }
    fta_rgn(&g_al_rs); fta_rgn(&g_al_rc); fta_rgn(&g_al_ru);
    (void)NewDialog(&g_al_dr, fta_rect(133, 87, 507, 191), "", g_al_it,
                    (uint16_t)(nb + n + ni), 1u ,
                    (uint16_t)(nb > 1 ? 2 : 1) ,
                    (WindowMgr *)0, &g_al_rs.r, &g_al_rc.r, &g_al_ru.r);
    g_al_dr.window.port.portBits.bm = ctx->off;
    g_al_dr.window.port.visRgn = (region_t *)0;
    g_al_dr.window.port.clipRgn = (region_t *)0;
    g_al_dr.window.port.portRect = region_get_bbox(g_al_dr.window.strucRgn);
    if (draw
        ) {
        DrawDialog(&g_al_dr);
        flair_desktop_present(bi, &ctx->off);
    }
}

void flair_live_alert_open(flair_live_ctx_t *ctx, const boot_info_t *bi,
                           const char *const *text, int n,
                           const char *const *btn, int nb, int draw)
{
    flair_live_alert_open_icon(ctx, bi, text, n, btn, nb, draw, FTA_NO_ICON);
}

uint16_t flair_live_alert_run(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    uint16_t hit = 0u;
    uint32_t t0 = flair_tick_count();
    int cancel = 0;
    for (;;) {
        EventRecord ev;
        int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev, 3u);
        flair_live_cursor_track(&ev);
        if (g) {
            flair_live_emit_evt(&ev);
        }
        if (DialogModalDispatch(&g_al_dr, &ev, (dialog_modal_block_fn)0,
                                (void *)0) == FLAIR_MODAL_CAPTURE &&
            DialogHandleEvent(&g_al_dr, &ev, (dialog_filter_fn)0, &hit))
            break;
        if (g_al_dr.trackingItem || ev.what == mouseUp)
            flair_desktop_present(bi, &ctx->off);
        if (flair_live_track_cut(t0, &cancel)) { hit = g_al_dr.cancelItem; break; }
    }
    DisposeDialog(&g_al_dr, (WindowMgr *)0);
    WindowMgr_invalidate_desktop(ctx->wm, fta_rect(133, 87, 507, 191));
    finder_desk_repaint(ctx, bi);
    return hit;
}

void flair_live_tenant_alert(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    const char *text[TBX_ALERT_LINES], *btn[3];
    uint32_t n, nb, hit;
    if (!tbx_take_alert(text, &n, btn, &nb)) return;
    flair_live_alert_open(ctx, bi, text, (int)n, btn, (int)nb, 1);
    for (uint32_t i = 0; i < n; i++) {
        serial_puts("TENANT-ALERT-LINE ");
        serial_puts(text[i]);
        serial_putc('\n');
    }
    serial_puts("TENANT-ALERT up lines=");
    serial_putu(n);
    serial_puts(" buttons=");
    serial_putu(nb);
    serial_putc('\n');
    hit = flair_live_alert_run(ctx, bi);
    serial_puts("TENANT-ALERT-HIT item=");
    serial_putu(hit);
    serial_putc('\n');
    tbx_alert_answer(hit);
    flair_live_tenant_service(ctx, bi);
}

void flair_live_tenant_scrollbar(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    if (!tbx_take_bar_dirty()) return;
    desktop_paint_damage(ctx->wm, &ctx->off, ctx->comp);
    flair_desktop_present(bi, &ctx->off);
}

void flair_live_tenant_track(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    flair_point_t last;
    EventRecord ev;
    uint32_t t0, moves = 0u;
    int cancel = 0;
    if (!tbx_take_track(&last)) return;
    t0 = flair_tick_count();
    ev.where = last;
    for (;;) {
        int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev, 1u);
        flair_live_cursor_track(&ev);
        if (g) flair_live_emit_evt(&ev);
        if (g && ev.what == (uint16_t)mouseUp) break;
        if (flair_live_track_cut(t0, &cancel)) break;
        if (ev.where.h != last.h || ev.where.v != last.v) {
            last = ev.where;
            moves++;
            tbx_track_push(0, last);
            if (tbx_take_drew()) flair_desktop_present(bi, &ctx->off);
        }
    }
    if (ev.what != (uint16_t)mouseUp) ev.where = last;
    serial_puts("TENANT-TRACK up h=");
    serial_puti((int32_t)ev.where.h);
    serial_puts(" v=");
    serial_puti((int32_t)ev.where.v);
    serial_puts(" moves=");
    serial_putu(moves);
    serial_putc('\n');
    tbx_track_push(1, ev.where);
    flair_live_tenant_service(ctx, bi);
}

const sf_geom_t g_sf_geom[2] = {
    { 348, 193, 68, 176, 13, 7, 250, 38, 66, 0, 110, 141 },
    { 356, 250, 73, 181, 18, 8, 258, 38, 66, 93, 136, 167 },
};

FlairStandardFile g_sf;

DialogRecord g_sf_dr;

DialogItem g_sf_it[3];

ControlRecord g_sf_btn[5];

finder_win_rgn_t g_sf_rs, g_sf_rc, g_sf_ru;

int sf_upper(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }

int sf_ieq(const char *a, const char *b)
{
    while (*a != '\0' && sf_upper(*a) == sf_upper(*b)) { a++; b++; }
    return sf_upper(*a) == sf_upper(*b);
}

int sf_walk_cb(const finder_dirent_t *d, void *user)
{
    sf_walk_t *w = (sf_walk_t *)user;
    if ((d->attribute & 0x0Au) != 0u) return 0;
    if (d->name83[0] == '.') return 0;
    if (finder_win_is_service(w->at, d->name83)) return 0;
    if (w->want != (const char *)0) {
        if ((d->attribute & 0x10u) != 0u && sf_ieq(d->name83, w->want)) {
            w->dir = d->start_cluster;
            return 1;
        }
        return 0;
    }
    if (w->n >= w->max) return 2;
    {
        FlairSFEntry *e = &w->out[w->n++];
        int i = 0;
        for (; d->name83[i] != '\0' && i < FLAIR_SF_NAME_MAX - 1; i++)
            e->name[i] = d->name83[i];
        e->name[i] = '\0';
        e->is_dir = (uint8_t)((d->attribute & 0x10u) != 0u);
        e->size = d->size;
    }
    return 0;
}

int sf_provider(const char *path, FlairSFEntry *out, int16_t max, void *user)
{
    sf_walk_t w;
    char comp[FLAIR_SF_NAME_MAX];
    const char *p = path;
    int rc;
    (void)user;
    w.dir = 0u;
    while (*p == '/') p++;
    while (*p != '\0') {
        int n = 0;
        while (*p != '\0' && *p != '/' && n < FLAIR_SF_NAME_MAX - 1) comp[n++] = *p++;
        comp[n] = '\0';
        while (*p == '/') p++;
        w.want = comp; w.at = w.dir; w.n = 0; w.max = 0; w.out = out;
        rc = finder_fat_enumerate((void *)0, w.at, sf_walk_cb, &w);
        if (rc != 1) return -1;
    }
    w.want = (const char *)0; w.at = w.dir; w.n = 0; w.max = max; w.out = out;
    rc = finder_fat_enumerate((void *)0, w.at, sf_walk_cb, &w);
    if (rc < 0) return -1;
    return rc == 2 ? (int)max + 1 : (int)w.n;
}

void sf_fill(flair_live_ctx_t *ctx, int l, int t, int r, int b, int part)
{
    blitter_fill_rect_clipped(&ctx->off, fta_rect(l, t, r, b),
                              flair_look_pixel_depth(ctx->off.bpp, part),
                              (const region_t *)0);
}

void sf_frame(flair_live_ctx_t *ctx, int l, int t, int r, int b)
{
    sf_fill(ctx, l, t, r, t + 1, (int)FLAIR_PART_FRAME);
    sf_fill(ctx, l, b - 1, r, b, (int)FLAIR_PART_FRAME);
    sf_fill(ctx, l, t, l + 1, b, (int)FLAIR_PART_FRAME);
    sf_fill(ctx, r - 1, t, r, b, (int)FLAIR_PART_FRAME);
}

void sf_text(flair_live_ctx_t *ctx, int x, int y, const char *s,
                    int fg, int bg)
{
    (void)text_draw_run(&ctx->off, x, y, s, FONT_CHICAGO, 0u,
                        flair_look_pixel_depth(ctx->off.bpp, fg),
                        flair_look_pixel_depth(ctx->off.bpp, bg),
                        (const text_clip_t *)0);
}

void sf_dos_path(const char *name, char *out)
{
    int k = 0;
    const char *p = g_sf.path;
    while (*p == '/') p++;
    for (; *p != '\0' && k < (int)TBX_SF_REPLY_BYTES - 14; p++)
        out[k++] = (*p == '/') ? '\\' : *p;
    if (k > 0) out[k++] = '\\';
    for (int i = 0; name[i] != '\0' && k < (int)TBX_SF_REPLY_BYTES - 1; i++)
        out[k++] = (char)sf_upper(name[i]);
    out[k] = '\0';
}

const char *sf_vol(void)
{
    return g_finder_vol_label[0] ? g_finder_vol_label : "INITECH";
}

void sf_dim_button(flair_live_ctx_t *ctx, ControlRecord *c)
{
    rgn_rect_t r = c->contrlRect;
    DrawControl(&g_sf_dr.window.port, c);
    sf_text(ctx, r.left + (80 - text_measure(FONT_CHICAGO, c->contrlTitle)) / 2,
            r.top + 2, c->contrlTitle, (int)FLAIR_PART_MENU_DISABLED_INK,
            (int)FLAIR_PART_PLAT_FACE);
}

void sf_draw_static(flair_live_ctx_t *ctx, const sf_ui_t *u)
{
    const sf_geom_t *g = u->g;
    DrawDialog(&g_sf_dr);
    sf_fill(ctx, u->l + 4, u->t + 4, u->l + g->w - 4, u->t + g->h - 4,
            (int)FLAIR_PART_CONTENT);
    for (int k = 0; k < 3; k++) {
        if (k == 0) DrawControlDefaultRing(&g_sf_dr.window.port, &g_sf_btn[k]);
        DrawControl(&g_sf_dr.window.port, &g_sf_btn[k]);
    }
    sf_dim_button(ctx, &g_sf_btn[3]);
    if (u->put) sf_dim_button(ctx, &g_sf_btn[4]);
    if (!u->put)
        for (int x = u->l + g->btn_l; x < u->l + g->btn_l + 80; x += 2)
            sf_fill(ctx, x, u->t + 97, x + 1, u->t + 98, (int)FLAIR_PART_PLAT_INACTIVE_FRAME);
    if (u->put)
        sf_text(ctx, u->l + 23, u->t + 194, u->prompt,
                (int)FLAIR_PART_TEXT, (int)FLAIR_PART_CONTENT);
}

void sf_draw_dynamic(flair_live_ctx_t *ctx, const sf_ui_t *u)
{
    const sf_geom_t *g = u->g;
    int lx = u->l + g->list_l, ly = u->t + 35;
    int lb = ly + g->rows * 18 + 2, bx = lx + 218 - 16;
    const char *fold = (const char *)0;
    for (const char *q = g_sf.path; *q != '\0'; q++)
        if (*q == '/' && q[1] != '\0') fold = q + 1;
    if (fold == (const char *)0) fold = sf_vol();
    sf_fill(ctx, u->l + g->pop_l, u->t + 12, u->l + g->pop_r, u->t + 31,
            (int)FLAIR_PART_PLAT_FACE);
    sf_frame(ctx, u->l + g->pop_l, u->t + 12, u->l + g->pop_r, u->t + 31);
    sf_text(ctx, u->l + g->pop_l + 6, u->t + 12 + 2, fold,
            (int)FLAIR_PART_TEXT, (int)FLAIR_PART_PLAT_FACE);
    for (int k = 0; k < 4; k++)
        sf_fill(ctx, u->l + g->pop_r - 15 + k, u->t + 12 + 8 + k,
                u->l + g->pop_r - 8 - k, u->t + 12 + 9 + k, (int)FLAIR_PART_FRAME);
    sf_fill(ctx, lx, ly, lx + 218, lb, (int)FLAIR_PART_CONTENT);
    sf_frame(ctx, lx, ly, lx + 218, lb);
    sf_fill(ctx, bx, ly + 1, bx + 1, lb - 1, (int)FLAIR_PART_FRAME);
    sf_fill(ctx, bx + 1, ly + 1, lx + 218 - 1, lb - 1, (int)FLAIR_PART_PLAT_TROUGH);
    for (int r = 0; r < g->rows && u->top + r < g_sf.count; r++) {
        int i = u->top + r, y = ly + 1 + r * 18;
        int hil = (i == u->sel);
        if (hil) sf_fill(ctx, lx + 1, y, bx, y + 18, (int)FLAIR_PART_FRAME);
        sf_text(ctx, lx + 22, y + 1, g_sf.entries[i].name,
                hil ? (int)FLAIR_PART_CONTENT : (int)FLAIR_PART_TEXT,
                hil ? (int)FLAIR_PART_FRAME : (int)FLAIR_PART_CONTENT);
    }
    sf_text(ctx, u->l + g->btn_l, u->t + 12 + 2, sf_vol(),
            (int)FLAIR_PART_TEXT, (int)FLAIR_PART_CONTENT);
    if (u->put) {
        int nl = u->l + 18, nt = u->t + 212;
        int w = text_measure(FONT_CHICAGO, u->name);
        sf_fill(ctx, nl, nt, u->l + 220, u->t + 234,
                (int)FLAIR_PART_CONTENT);
        sf_frame(ctx, nl, nt, u->l + 220, u->t + 234);
        sf_text(ctx, nl + 3, nt + 3, u->name, (int)FLAIR_PART_TEXT,
                (int)FLAIR_PART_CONTENT);
        sf_fill(ctx, nl + 3 + w, nt + 3, nl + 4 + w, nt + 19, (int)FLAIR_PART_FRAME);
    }
}

void sf_select(sf_ui_t *u, int i)
{
    if (g_sf.count <= 0) { u->sel = -1; return; }
    if (i < 0) i = 0;
    if (i >= g_sf.count) i = g_sf.count - 1;
    u->sel = i;
    FlairSF_select(&g_sf, (int16_t)i);
    if (i < u->top) u->top = i;
    if (i >= u->top + u->g->rows) u->top = i - u->g->rows + 1;
}

void sf_reset_view(sf_ui_t *u)
{
    u->top = 0;
    u->sel = -1;
    if (!u->put) sf_select(u, 0);
}

void sf_report(const char *what, const sf_ui_t *u)
{
    serial_puts("TENANT-SF ");
    serial_puts(what);
    serial_puts(" dir=");
    serial_puts(g_sf.path);
    serial_puts(" n=");
    serial_puti((int32_t)g_sf.count);
    if (u->put) { serial_puts(" name="); serial_puts(u->name); }
    serial_putc('\n');
}

int sf_name_ok(const char *n)
{
    int i = 0, base = 0, ext = -1;
    if (n[0] == '\0' || n[0] == '.') return 0;
    for (; n[i] != '\0'; i++) {
        if (n[i] == '.') { if (ext >= 0) return 0; ext = 0; continue; }
        if (ext >= 0) ext++; else base++;
    }
    return base >= 1 && base <= 8 && ext <= 3;
}

void sf_button(ControlRecord *c, const sf_ui_t *u, int y, const char *t)
{
    control_init(c, pushButton,
                 fta_rect(u->l + u->g->btn_l, u->t + y,
                          u->l + u->g->btn_l + 80, u->t + y + 20),
                 0, 0, 1, 1, t);
}

int sf_run(flair_live_ctx_t *ctx, const boot_info_t *bi, sf_ui_t *u,
                  char *out)
{
    const sf_geom_t *g = u->g;
    uint32_t t0 = flair_tick_count(), last_click = 0u;
    int last_row = -1, cancel = 0, good = 0, k;
    u->l = (FLAIR_SCREEN_W - g->w) / 2;
    u->t = (FLAIR_SCREEN_H - g->h) / 2;
    sf_button(&g_sf_btn[0], u, g->def_y, u->put ? "Save" : "Open");
    sf_button(&g_sf_btn[1], u, g->cancel_y, "Cancel");
    sf_button(&g_sf_btn[2], u, g->desk_y, "Desktop");
    sf_button(&g_sf_btn[3], u, g->eject_y, "Eject");
    sf_button(&g_sf_btn[4], u, g->new_y, "New");
    for (k = 0; k < 3; k++) {
        g_sf_it[k].type = ctrlItem;
        g_sf_it[k].ctrl = &g_sf_btn[k];
        g_sf_it[k].rect = g_sf_btn[k].contrlRect;
        g_sf_it[k].text = (const char *)0;
        g_sf_it[k].enabled = 1u;
        g_sf_it[k]._pad[0] = g_sf_it[k]._pad[1] = g_sf_it[k]._pad[2] = 0u;
    }
    fta_rgn(&g_sf_rs); fta_rgn(&g_sf_rc); fta_rgn(&g_sf_ru);
    (void)NewDialog(&g_sf_dr, fta_rect(u->l, u->t, u->l + g->w, u->t + g->h), "",
                    g_sf_it, 3u, 1u, 2u, (WindowMgr *)0,
                    &g_sf_rs.r, &g_sf_rc.r, &g_sf_ru.r);
    g_sf_dr.window.port.portBits.bm = ctx->off;
    g_sf_dr.window.port.visRgn = (region_t *)0;
    g_sf_dr.window.port.clipRgn = (region_t *)0;
    g_sf_dr.window.port.portRect = region_get_bbox(g_sf_dr.window.strucRgn);
    sf_draw_static(ctx, u);
    sf_draw_dynamic(ctx, u);
    flair_desktop_present(bi, &ctx->off);
    sf_report(u->put ? "put up" : "get up", u);
    for (;;) {
        EventRecord ev;
        uint16_t hit = 0u;
        int act = 0;
        int g2 = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev, 3u);
        flair_live_cursor_track(&ev);
        if (flair_live_track_cut(t0, &cancel)) { act = 2; }
        else if (!g2) continue;
        else {
            flair_live_emit_evt(&ev);
            if (DialogModalDispatch(&g_sf_dr, &ev, (dialog_modal_block_fn)0,
                                    (void *)0) != FLAIR_MODAL_CAPTURE)
                continue;
            if (ev.what == (uint16_t)keyDown || ev.what == (uint16_t)autoKey) {
                uint8_t a = (uint8_t)(ev.message & 0xFFu);
                uint8_t vk = (uint8_t)((ev.message >> 8) & 0xFFu);
                int n = 0;
                while (u->name[n] != '\0') n++;
                if (vk == 0x48u && a == 0u) { sf_select(u, u->sel - 1); }
                else if (vk == 0x50u && a == 0u) { sf_select(u, u->sel + 1); }
                else if (u->put && a == 0x08u) { if (n > 0) u->name[n - 1] = '\0'; }
                else if (u->put && a > 0x20u && a < 0x7Fu &&
                         n < (int)TBX_SF_NAME_MAX - 1) {
                    u->name[n] = (char)sf_upper(a);
                    u->name[n + 1] = '\0';
                    u->sel = -1;
                } else if (DialogHandleEvent(&g_sf_dr, &ev, (dialog_filter_fn)0, &hit)) {
                    act = (int)hit;
                }
            } else if (ev.what == (uint16_t)mouseDown) {
                int lx = u->l + g->list_l, ly = u->t + 35;
                if (ev.where.h > lx && ev.where.h < lx + 218 - 16 &&
                    ev.where.v > ly && ev.where.v < ly + 1 + g->rows * 18) {
                    int row = u->top + (ev.where.v - ly - 1) / 18;
                    if (row < g_sf.count) {
                        if (row == last_row &&
                            ev.when - last_click <= (uint32_t)flair_double_click_ticks)
                            act = 1;
                        sf_select(u, row);
                        last_row = row;
                        last_click = ev.when;
                    }
                } else if (ev.where.h >= u->l + g->pop_l && ev.where.h < u->l + g->pop_r &&
                           ev.where.v >= u->t + 12 && ev.where.v < u->t + 31) {
                    if (FlairSF_up(&g_sf) == FLAIR_SF_OK) sf_reset_view(u);
                } else if (DialogHandleEvent(&g_sf_dr, &ev, (dialog_filter_fn)0, &hit)) {
                    act = (int)hit;
                }
            } else if (ev.what == (uint16_t)mouseUp && g_sf_dr.trackingItem) {
                if (DialogHandleEvent(&g_sf_dr, &ev, (dialog_filter_fn)0, &hit))
                    act = (int)hit;
            }
            if (g_sf_dr.trackingItem || ev.what == (uint16_t)mouseUp)
                flair_desktop_present(bi, &ctx->off);
        }
        if (act == 2) break;
        if (act == 3) {
            while (g_sf.path[1] != '\0' && FlairSF_up(&g_sf) == FLAIR_SF_OK) { }
            sf_reset_view(u);
        } else if (act == 1) {
            if (u->sel >= 0 && g_sf.entries[u->sel].is_dir) {
                if (FlairSF_navigate(&g_sf, (int16_t)u->sel) == FLAIR_SF_OK)
                    sf_reset_view(u);
            } else if (!u->put && u->sel >= 0) {
                sf_dos_path(g_sf.entries[u->sel].name, out);
                good = 1;
                break;
            } else if (u->put && sf_name_ok(u->name)) {
                int exists = 0;
                for (k = 0; k < g_sf.count; k++)
                    if (sf_ieq(g_sf.entries[k].name, u->name)) exists = 1 + g_sf.entries[k].is_dir;
                if (exists == 2) continue;
                if (exists == 1) {
                    static char line[FINDER_ALERT_LINE_MAX];
                    static const char *const rl[2] = { "Replace", "Cancel" };
                    const char *txt[1];
                    int i = 0;
                    const char *pre = "Replace existing \"";
                    for (; pre[i] != '\0'; i++) line[i] = pre[i];
                    for (k = 0; u->name[k] != '\0'; k++) line[i++] = u->name[k];
                    line[i++] = '"'; line[i++] = '?'; line[i] = '\0';
                    txt[0] = line;
                    flair_live_alert_open(ctx, bi, txt, 1, rl, 2, 1);
                    serial_puts("TENANT-SF replace ");
                    serial_puts(u->name);
                    serial_putc('\n');
                    k = (int)flair_live_alert_run(ctx, bi);
                    sf_draw_static(ctx, u);
                    sf_draw_dynamic(ctx, u);
                    flair_desktop_present(bi, &ctx->off);
                    if (k != 1) continue;
                }
                sf_dos_path(u->name, out);
                good = 1;
                break;
            } else if (u->put
                       ) {
                static const char *const nl[2] = {
                    "That is not a valid DOS file name.",
                    "Use up to 8 letters, a dot, then 3 more." };
                static const char *const ok[1] = { "OK" };
                flair_live_alert_open(ctx, bi, nl, 2, ok, 1, 1);
                serial_puts("TENANT-SF badname ");
                serial_puts(u->name);
                serial_putc('\n');
                (void)flair_live_alert_run(ctx, bi);
                sf_draw_static(ctx, u);
                sf_draw_dynamic(ctx, u);
                flair_desktop_present(bi, &ctx->off);
                continue;
            }
        }
        sf_draw_dynamic(ctx, u);
        flair_desktop_present(bi, &ctx->off);
    }
    DisposeDialog(&g_sf_dr, (WindowMgr *)0);
    WindowMgr_invalidate_desktop(ctx->wm, fta_rect(u->l, u->t, u->l + g->w, u->t + g->h));
    finder_desk_repaint(ctx, bi);
    return good;
}

void flair_live_tenant_sf(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    static sf_ui_t u;
    static char out[TBX_SF_REPLY_BYTES];
    const char *prompt, *name;
    int put, good = 0, k;
    Cell cs;
    rgn_rect_t rv;
    if (!tbx_take_sf(&put, &prompt, &name)) return;
    u.put = put;
    u.prompt = prompt;
    u.g = &g_sf_geom[put ? 1 : 0];
    for (k = 0; name[k] != '\0' && k < (int)TBX_SF_NAME_MAX - 1; k++) u.name[k] = (char)sf_upper(name[k]);
    u.name[k] = '\0';
    out[0] = '\0';
    cs.h = 218; cs.v = 18;
    rv = fta_rect(0, 0, 218, 18 * u.g->rows);
    if (FlairSF_open(&g_sf, sf_provider, (void *)0, cs, rv) != FLAIR_SF_OK) {
        serial_puts("TENANT-SF fail open\n");
    } else {
        sf_reset_view(&u);
        good = sf_run(ctx, bi, &u, out);
    }
    serial_puts("TENANT-SF-REPLY good=");
    serial_putu((uint32_t)good);
    if (good) { serial_puts(" path="); serial_puts(out); }
    serial_putc('\n');
    tbx_sf_answer(good, out);
    flair_live_tenant_service(ctx, bi);
}

DialogRecord g_fc_dr;

DialogItem g_fc_it[2];

ControlRecord g_fc_bar;

finder_win_rgn_t g_fc_rs, g_fc_rc, g_fc_ru;

void flair_live_tenant_filecopy(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    const char *cap, *src, *dst;
    if (tbx_take_filecopy(&cap, &src, &dst))
        finder_filecopy_run(ctx, bi, cap, src, dst);
}
