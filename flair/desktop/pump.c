/* Production desktop extracted from legacy-final kmain.c, sections 996-5744
 * and the interactive App Contract pump. Bodies retain the reference ordering.
 * Boot personalities, mutants, reference tenants and overlay machinery omitted. */
#include "stdint.h"
#include "io.h"
#include "boot_info.h"
#include "memory_map.h"
#include "console.h"
#include "idt.h"
#include "pic.h"
#include "int21.h"
#include "psp.h"
#include "sft.h"
#include "loader.h"
#include "fat12.h"
#include "fileio_fat.h"
#include "kbd.h"
#include "mouse.h"
#include "mouse_pack.h"
#include "pit.h"
#include "rtc.h"
#include "panic.h"
#include "heap.h"
#include "surface.h"
#include "cursor.h"
#include "region_algebra.h"
#include "region.h"
#include "shell.h"
#include "shell_menus.h"
#include "desktop.h"
#include "flair_look.h"
#include "chrome.h"
#include "menu_canon.h"
#include "palette.h"
#include "flair_dac.h"
#include "event.h"
#include "desktop_db.h"
#include "process.h"
#include "power_notice.h"
#include "finder_desktop.h"
#include "finder_cmd.h"
#include "finder_windows.h"
#include "finder_ops.h"
#include "finder_menu.h"
#include "tbxgate.h"
#include "apple_menu.h"
#include "toolbox_gate.h"
#include "stdfile.h"
#include "scrap.h"
#include "finder_actions.h"
#include "boot_screen.h"
#include "environment.h"
extern void serial_puts(const char *);
extern void serial_putu(uint32_t);
extern void serial_puthex32(uint32_t);
extern void irq12_entry(void);
extern void tbx_gate_entry(void);
static unsigned g_vga_dac_shift;
static uint8_t g_volume_sector[512];
int g_desktop_exit;
static void vga_set_dac(uint8_t i,uint8_t r,uint8_t g,uint8_t b)
{ outb(0x3c8,i);outb(0x3c9,r>>g_vga_dac_shift);outb(0x3c9,g>>g_vga_dac_shift);outb(0x3c9,b>>g_vga_dac_shift); }
static const rgn_rect_t fd_win_bounds[2] = {
    { 80, 60, 300, 360 },
    { 120, 300, 360, 560 }
};
static const char *const fd_win_titles[2] = {
    "untitled-1",
    "Saving tables to disk"
};
static const char *const fd_sys_titles[4] = {
    "File", "Edit", "View", "Special"
};
static void flair_desktop_oom(const char *what)
{
    serial_puts("PANIC flair-desktop: flair_alloc returned NULL for ");
    serial_puts(what);
    serial_putc('\n');
    panic_show_reason(PANIC_REASON_HEAP, what);
    serial_puts("HALTED\n");
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}
static region_t *flair_desktop_alloc_region(flair_heap_t *h, const char *what)
{
    region_t *r = (region_t *)flair_alloc(h, FLAIR_CLASS_HANDLE,
                                              (uint32_t)sizeof(region_t));
    rgn_row_t *rows = (rgn_row_t *)flair_alloc(h, FLAIR_CLASS_REGION,
                          (uint32_t)(sizeof(rgn_row_t) * RGN_ROWS_CAP));
    int16_t *pool = (int16_t *)flair_alloc(h, FLAIR_CLASS_REGION,
                          (uint32_t)(sizeof(int16_t) * RGN_X_POOL_CAP));
    if (!r || !rows || !pool) {
        flair_desktop_oom(what);
    }
    r->rows = rows;
    r->cap_rows = RGN_ROWS_CAP;
    r->x_pool = pool;
    r->x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(r);
    return r;
}
static void flair_boot_screen_row(void *ctx, int y, const uint8_t *row)
{
    const bitmap_t *bm = (const bitmap_t *)ctx;
    uint32_t off = (uint32_t)y * bm->pitch;
    for (int x = 0; x < BOOT_SCREEN_W; x++, off += bm->bytes_per_pixel)
        surface_put_pixel(bm, off, bm->bpp == 8u ? (uint32_t)row[x]
                                   : (flair_palette_rgb(row[x]) & 0x00FFFFFFu));
}
static void flair_dac_load(void)
{
    for (uint32_t idx = 0; idx < sizeof(flair_dac)/sizeof(flair_dac[0]); idx++) {
        uint32_t rgb = flair_palette_rgb((uint8_t)idx);
        vga_set_dac((uint8_t)idx, (uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb);
    }
}
static void flair_boot_screen(const boot_info_t *bi)
{
    bitmap_t lbm;
    if (bi->lfb_width != (uint32_t)BOOT_SCREEN_W ||
        bi->lfb_height != (uint32_t)BOOT_SCREEN_H) {
        return;
    }
    if (bi->lfb_bpp == 8u)
        flair_dac_load();
    lbm.base = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
    lbm.pitch = bi->lfb_pitch;
    lbm.bpp = bi->lfb_bpp;
    lbm.bytes_per_pixel = bi->lfb_bpp / 8u;
    lbm.width = bi->lfb_width;
    lbm.height = bi->lfb_height;
    boot_screen_render(flair_boot_screen_row, &lbm);
    serial_puts("FLAIR-BOOT-SCREEN\n");
}
static void flair_boot_screen_hold(void)
{
    uint8_t p61 = inb(0x61);
    uint16_t div = pit_divisor_for_hz(PIT_TICK_HZ);
    outb(0x61, (uint8_t)((p61 & 0xFCu) | 0x01u));
    for (uint32_t t = 0; t < (uint32_t)200; t++) {
        outb(0x43, 0xB0u);
        outb(0x42, (uint8_t)(div & 0xFFu));
        outb(0x42, (uint8_t)(div >> 8));
        while ((inb(0x61) & 0x20u) == 0u) {
        }
    }
    outb(0x61, p61);
}
void flair_desktop_present(const boot_info_t *bi, const bitmap_t *off)
{
    uint32_t W = off->width;
    uint32_t H = off->height;
    flair_cursor_shield();
    if (bi->lfb_bpp == 8u) {
        flair_dac_load();
        volatile uint8_t *lfb = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
        for (uint32_t y = 0; y < H; y++) {
            const uint8_t *srow = (const uint8_t *)off->base + (uint32_t)y * off->pitch;
            volatile uint8_t *drow = lfb + (uint32_t)y * bi->lfb_pitch;
            for (uint32_t x = 0; x < W; x++) {
                drow[x] = srow[x];
            }
        }
        flair_cursor_unshield();
        return;
    }
    bitmap_t lbm;
    lbm.base = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
    lbm.pitch = bi->lfb_pitch;
    lbm.bpp = bi->lfb_bpp;
    lbm.bytes_per_pixel = bi->lfb_bpp / 8u;
    lbm.width = bi->lfb_width;
    lbm.height = bi->lfb_height;
    for (uint32_t y = 0; y < H; y++) {
        const uint8_t *srow = (const uint8_t *)off->base + (uint32_t)y * off->pitch;
        for (uint32_t x = 0; x < W; x++) {
            uint32_t rgb = flair_palette_rgb(srow[x]) & 0x00FFFFFFu;
            uint32_t boff = (uint32_t)y * lbm.pitch + x * lbm.bytes_per_pixel;
            surface_put_pixel(&lbm, boff, rgb);
        }
    }
    flair_cursor_unshield();
}
static void flair_desktop_run(const boot_info_t *bi, flair_live_ctx_t *ctx_out)
{
    enum { FD_W = 640, FD_H = 480 };
    if (bi->lfb_width < (uint32_t)FD_W || bi->lfb_height < (uint32_t)FD_H) {
        serial_puts("PANIC flair-desktop: LFB smaller than 640x480 w=");
        serial_putu(bi->lfb_width);
        serial_puts(" h=");
        serial_putu(bi->lfb_height);
        serial_putc('\n');
        serial_puts("HALTED\n");

        for (;;) {
            __asm__ __volatile__("cli; hlt");
        }
    }
    static flair_heap_t heap;
    flair_heap_init(&heap, (void *)(uintptr_t)FLAIR_HEAP_BASE, FLAIR_HEAP_SIZE);
    rgn_ws_slot_t *rgn_ws_a =
        (rgn_ws_slot_t *)flair_alloc(&heap, FLAIR_CLASS_REGION,
                                     (uint32_t)sizeof(rgn_ws_slot_t));
    rgn_ws_slot_t *rgn_ws_b =
        (rgn_ws_slot_t *)flair_alloc(&heap, FLAIR_CLASS_REGION,
                                     (uint32_t)sizeof(rgn_ws_slot_t));
    if (!rgn_ws_a || !rgn_ws_b) { flair_desktop_oom("region engine working set"); }
    region_engine_bind_ws(rgn_ws_a, rgn_ws_b);
    region_engine_reset();
    shell_scene_t *scene =
        (shell_scene_t *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                     (uint32_t)sizeof(shell_scene_t));
    if (!scene) { flair_desktop_oom("shell_scene_t"); }
    region_t *desk = flair_desktop_alloc_region(&heap, "wm.desktop_update");
    region_t *sa = flair_desktop_alloc_region(&heap, "wm.scratch_a");
    region_t *sb = flair_desktop_alloc_region(&heap, "wm.scratch_b");
    region_t *sc = flair_desktop_alloc_region(&heap, "wm.scratch_c");
    region_t *comp = flair_desktop_alloc_region(&heap, "wm.comp_scratch");
    shell_window_store *wins =
        (shell_window_store *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                          (uint32_t)(sizeof(shell_window_store) * 2));
    if (!wins) { flair_desktop_oom("shell_window_store[]"); }
    for (int i = 0; i < 2; i++) {
        wins[i].strucRgn = flair_desktop_alloc_region(&heap, "win.strucRgn");
        wins[i].contRgn = flair_desktop_alloc_region(&heap, "win.contRgn");
        wins[i].updateRgn = flair_desktop_alloc_region(&heap, "win.updateRgn");
    }
    MenuInfo *sys_menus =
        (MenuInfo *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                (uint32_t)(sizeof(MenuInfo) * 4));
    MenuItem *sys_items =
        (MenuItem *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                            (uint32_t)(sizeof(MenuItem) * 4 * 2));
    if (!sys_menus || !sys_items) { flair_desktop_oom("sys menus/items"); }
    for (int mi = 0; mi < 4; mi++) {
        MenuItem *it = &sys_items[mi * 2];
        it[0] = SHELL_SYS_MENU_ITEMS[0];
        it[1] = SHELL_SYS_MENU_ITEMS[1];
        sys_menus[mi].menuID = (int16_t)(128 + mi);
        sys_menus[mi].title = fd_sys_titles[mi];
        sys_menus[mi].items = it;
        sys_menus[mi].n_items = 2;
        sys_menus[mi].menuWidth = 0;
    }
    MenuInfo *ps_menus =
        (MenuInfo *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
            (uint32_t)(sizeof(MenuInfo) * FLAIR_CANON_PHOTOSHOP_MENU_COUNT));
    MenuItem *ps_items =
        (MenuItem *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
            (uint32_t)(sizeof(MenuItem) * FLAIR_CANON_PHOTOSHOP_MENU_COUNT * 2));
    if (!ps_menus || !ps_items) { flair_desktop_oom("photoshop menus/items"); }
    for (int mi = 0; mi < FLAIR_CANON_PHOTOSHOP_MENU_COUNT; mi++) {
        MenuItem *it = &ps_items[mi * 2];
        it[0]=(MenuItem){ "New",0,'N',0,1,0 };it[1]=(MenuItem){ "Open",0,'O',0,1,0 };
        ps_menus[mi].title = (const char *)0;
        ps_menus[mi].items = it;
        ps_menus[mi].n_items = 2;
        ps_menus[mi].menuWidth = 0;
    }
    DialogRecord *dlg =
        (DialogRecord *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                    (uint32_t)sizeof(DialogRecord));
    DialogItem *dlg_items =
        (DialogItem *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                  (uint32_t)(sizeof(DialogItem) * 2));
    ControlRecord *dlg_progress =
        (ControlRecord *)flair_alloc(&heap, FLAIR_CLASS_HANDLE,
                                     (uint32_t)sizeof(ControlRecord));
    if (!dlg || !dlg_items || !dlg_progress) { flair_desktop_oom("dialog storage"); }
    region_t *dlg_struc = flair_desktop_alloc_region(&heap, "dlg.strucRgn");
    region_t *dlg_cont = flair_desktop_alloc_region(&heap, "dlg.contRgn");
    region_t *dlg_upd = flair_desktop_alloc_region(&heap, "dlg.updateRgn");
    region_t *overlay = flair_desktop_alloc_region(&heap, "wm.overlay_rgn");
    uint8_t *off_px =
        (uint8_t *)flair_alloc(&heap, FLAIR_CLASS_BITMAP,
                               (uint32_t)((uint32_t)FD_W * (uint32_t)FD_H));
    if (!off_px) { flair_desktop_oom("offscreen8 (640x480)"); }
    bitmap_t off;
    off.base = (volatile uint8_t *)off_px;
    off.pitch = (uint32_t)FD_W;
    off.bpp = 8u;
    off.bytes_per_pixel = 1u;
    off.width = (uint32_t)FD_W;
    off.height = (uint32_t)FD_H;
    rgn_rect_t frame = { 0, 0, (int16_t)FD_H, (int16_t)FD_W };
    shell_build_scene(scene,
                      frame,
                      wins, 2, fd_win_bounds, fd_win_titles,
                      desk, sa, sb, sc, comp,
                      sys_menus, 4,
                      ps_menus,
                      dlg, dlg_items, dlg_progress,
                      dlg_struc, dlg_cont, dlg_upd,
                      overlay,
                      0 );
    flair_cursor_init((void *)(uintptr_t)bi->lfb_addr, bi->lfb_pitch,
                      bi->lfb_width, bi->lfb_height, bi->lfb_bpp);
    shell_render(scene, &off);
    desktop_validate_all(&scene->wm);
    flair_boot_screen_hold();
    flair_desktop_present(bi, &off);
    if (ctx_out) {
        ctx_out->wm = &scene->wm;
        ctx_out->windows = wins;
        ctx_out->n_windows = 2;
        ctx_out->comp = comp;
        ctx_out->off = off;
        ctx_out->scene = scene;
        ctx_out->master = &heap;
        ctx_out->plist = (FlairProcessList *)0;
    }
}
flair_raw_ring_t g_flair_kbd_ring;
static kbd_state_t g_flair_consumed_keys;
static void flair_consumed_key(uint8_t sc)
{
    (void)kbd_translate(&g_flair_consumed_keys, sc);
}
static volatile uint32_t g_flair_mouse_activity;
static int g_flair_cursor_visible;
static int g_flair_cursor_x;
static int g_flair_cursor_y;
__attribute__((unused))
static void flair_live_kbd_post(uint8_t sc)
{
    flair_raw_event_t raw;
    raw.kind = (uint32_t)FLAIR_RAW_KEYBOARD;
    raw.tick = flair_tick_count();
    raw.payload = (uint32_t)sc;
    (void)flair_raw_post(&g_flair_kbd_ring, &raw);
}
__attribute__((unused))
static void flair_live_mouse_post(int dx, int dy, uint8_t buttons)
{
    flair_raw_event_t raw;
    raw.kind = (uint32_t)FLAIR_RAW_MOUSE;
    raw.tick = flair_tick_count();
    raw.payload = mouse_pack_raw_payload(dx, dy, buttons);
    if (flair_raw_post(&g_flair_kbd_ring, &raw)) {
        g_flair_mouse_activity++;
    }
}
static void flair_live_yield(void)
{
    __asm__ __volatile__("hlt");
}
void flair_live_emit_evt(const EventRecord *ev)
{
    serial_puts("FLAIR-EVT what=");
    serial_putu((uint32_t)ev->what);
    serial_puts(" where=");
    serial_puti((int32_t)ev->where.h);
    serial_putc(',');
    serial_puti((int32_t)ev->where.v);
    serial_puts(" msg=");
    serial_puthex32(ev->message & 0xFFFFu);
    serial_putc('\n');
}
void flair_live_cursor_track(const EventRecord *ev)
{
    int x;
    int y;
    if (g_flair_mouse_activity == 0u) return;
    x = (int)ev->where.h;
    y = (int)ev->where.v;
    if (!g_flair_cursor_visible) {
        flair_cursor_show(x, y);
        g_flair_cursor_visible = 1;
    } else {
        if (x == g_flair_cursor_x && y == g_flair_cursor_y) return;
        flair_cursor_move(x, y);
    }
    g_flair_cursor_x = x;
    g_flair_cursor_y = y;
    serial_puts("FLAIR-CURSOR x=");
    serial_puti((int32_t)x);
    serial_puts(" y=");
    serial_puti((int32_t)y);
    serial_putc('\n');
}
static int flair_live_window_index(const flair_live_ctx_t *ctx, WindowPtr w)
{
    int i;
    for (i = 0; i < ctx->n_windows; i++) {
        if (&ctx->windows[i].rec == w) {
            return i;
        }
    }
    return WindowMgr_window_index(ctx->wm, w);
}
static void flair_live_content_phase(flair_live_ctx_t *ctx)
{
    if (ctx->plist != (FlairProcessList *)0) {
        flair_route_updates(ctx->plist, ctx->wm);
        return;
    }
    desktop_validate_all(ctx->wm);
}
static __attribute__((noinline)) void flair_live_repaint(flair_live_ctx_t *ctx)
{
    desktop_paint_damage(ctx->wm, &ctx->off, ctx->comp);
    flair_live_content_phase(ctx);
}
static __attribute__((noinline)) void flair_live_repaint_present(flair_live_ctx_t *ctx,
                                                                 const boot_info_t *bi)
{
    flair_live_repaint(ctx);
    flair_desktop_present(bi, &ctx->off);
}
static void flair_live_refresh_finder_bar(MenuBar *bar);
static MenuBar *flair_live_tenant_bar(flair_live_ctx_t *ctx,
                                      FlairProcessList *list)
{
    MenuBar *bar = &ctx->scene->bar_photoshop;
    if (list != (FlairProcessList *)0 && list->head != (FlairApp *)0 &&
        list->head->menubar != (MenuBar *)0)
        bar = list->head->menubar;
    flair_live_refresh_finder_bar(bar);
    return bar;
}
static void flair_live_finish_tenant_switch(flair_live_ctx_t *ctx,
                                            const boot_info_t *bi,
                                            FlairProcessList *list,
                                            FlairApp *prev_fg,
                                            GrafPort *barport)
{
    if (list->head == (FlairApp *)0 || list->head == prev_fg) return;
    desktop_paint_damage(ctx->wm, &ctx->off, ctx->comp);
    flair_route_updates(list, ctx->wm);
    DrawMenuBar(barport, flair_live_tenant_bar(ctx, list), -1,
                (const region_t *)0);
    flair_desktop_present(bi, &ctx->off);
    serial_puts("FLAIR-DISPATCH app=");
    serial_puts(list->head->name ? list->head->name : "?");
    serial_putc('\n');
}
static void flair_live_clamp_drag(const flair_live_ctx_t *ctx, WindowPtr w,
                                  int16_t *dh, int16_t *dv)
{
    rgn_rect_t s = region_get_bbox(w->strucRgn);
    rgn_rect_t frame = ctx->wm->desktop_frame;
    int32_t width = (int32_t)s.right - (int32_t)s.left;
    int32_t left = (int32_t)s.left + (int32_t)*dh;
    int32_t top = (int32_t)s.top + (int32_t)*dv;
    int32_t min_left = (int32_t)frame.left + 4 - width;
    int32_t max_left = (int32_t)frame.right - 4;
    int32_t min_top = (int32_t)SHELL_MENUBAR2_TOP + (int32_t)FLAIR_MENUBAR_H;
    int32_t max_top = (int32_t)frame.bottom - (int32_t)FLAIR_CHROME_TITLEBAR_H;
    if (min_top < (int32_t)frame.top) min_top = (int32_t)frame.top;
    if (left < min_left) left = min_left;
    if (left > max_left) left = max_left;
    if (top < min_top) top = min_top;
    if (top > max_top) top = max_top;
    *dh = (int16_t)(left - (int32_t)s.left);
    *dv = (int16_t)(top - (int32_t)s.top);
}
typedef struct flair_live_outline {
    rgn_rect_t rect;
    uint8_t saved[(2u * ((uint32_t)FLAIR_SCREEN_W + (uint32_t)FLAIR_SCREEN_H) + 4u)];
    uint32_t saved_count;
    int drawn;
} flair_live_outline_t;
static flair_live_outline_t g_flair_live_outline;
static int flair_live_rect_same(rgn_rect_t a, rgn_rect_t b)
{
    return a.top == b.top && a.left == b.left &&
           a.bottom == b.bottom && a.right == b.right;
}
static void flair_live_outline_pixel(flair_live_outline_t *outline,
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
static void flair_live_outline_walk(flair_live_outline_t *outline,
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
static void flair_live_drag_delta(const flair_live_ctx_t *ctx, WindowPtr w,
                                  flair_point_t where0, flair_point_t where,
                                  int16_t *dh, int16_t *dv)
{
    *dh = (int16_t)(where.h - where0.h);
    *dv = (int16_t)(where.v - where0.v);
    flair_live_clamp_drag(ctx, w, dh, dv);
}
static int flair_live_track_cut(uint32_t t0, int *cancel)
{
    (void)t0;
    *cancel = 0;
    return 0;
}
static void flair_live_do_drag(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static int flair_live_box_hit(const flair_live_ctx_t *ctx, WindowPtr w,
                              flair_part_code_t pc, flair_point_t pt)
{
    WindowPtr hw = (WindowPtr)0;
    flair_part_code_t at = FindWindow(ctx->wm, pt, &hw);
    if (at == inZoomOut) at = inZoomIn;
    if (pc == inZoomOut) pc = inZoomIn;
    return hw == w && at == pc;
}
static int flair_live_track_box(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static void flair_live_scroll_report(const flair_live_ctx_t *ctx, WindowPtr w,
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
static void flair_live_scroll_repaint(flair_live_ctx_t *ctx,
                                      const boot_info_t *bi, WindowPtr w,
                                      int axis, int content)
{
    rgn_rect_t f = WindowFrameRect(w);
    WindowMgr_invalidate(ctx->wm, w, wscroll_bar_rect(f, axis));
    if (content)
        WindowMgr_invalidate(ctx->wm, w, region_get_bbox(w->contRgn));
    flair_live_repaint_present(ctx, bi);
}
static void flair_live_track_scroll(flair_live_ctx_t *ctx,
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
void flair_live_tenant_heard(flair_live_ctx_t *ctx, const boot_info_t *bi);
static void flair_live_do_close(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
                    flair_live_tenant_bar(ctx, ctx->plist), -1,
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
static void flair_live_do_zoom(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               WindowPtr w)
{
    int wid = flair_live_window_index(ctx, w);
    int zoomed = ZoomWindow(ctx->wm, w);
    flair_live_repaint_present(ctx, bi);
    serial_puts("FLAIR-ZOOM win ");
    serial_puti((int32_t)wid);
    serial_puts(zoomed ? " in\n" : " out\n");
}
static void flair_live_do_collapse(flair_live_ctx_t *ctx,
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
static void flair_live_do_grow(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static void flair_live_init_menu_port(GrafPort *port, const bitmap_t *view)
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
static void flair_live_erase_menu_panel(flair_live_ctx_t *ctx,
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
        DrawMenuBar(&owner_port, bar, -1, (const region_t *)0);
    }
    if (y_top == 0u && panel.top < (int16_t)(2 * FLAIR_MENUBAR_H) &&
        panel.bottom > (int16_t)FLAIR_MENUBAR_H) {
        MenuBar *under_bar = &ctx->scene->bar_photoshop;
        if (ctx->plist != (FlairProcessList *)0 &&
            ctx->plist->head != (FlairApp *)0 &&
            ctx->plist->head->menubar != (MenuBar *)0) {
            under_bar = ctx->plist->head->menubar;
        }
        bitmap_t bar_view;
        GrafPort bar_port;
        rgn_rect_t clip;
        make_offset_view(&bar_view, &ctx->off,
                         (uint32_t)FLAIR_MENUBAR_H);
        flair_live_init_menu_port(&bar_port, &bar_view);
        clip.top = 0;
        clip.left = panel.left;
        clip.bottom = (int16_t)(panel.bottom - FLAIR_MENUBAR_H);
        if (clip.bottom > (int16_t)FLAIR_MENUBAR_H)
            clip.bottom = (int16_t)FLAIR_MENUBAR_H;
        clip.right = panel.right;
        region_set_rect(ctx->comp, clip);
        DrawMenuBar(&bar_port, under_bar, -1, ctx->comp);
        region_set_empty(ctx->comp);
    }
}
static void flair_live_apple_refresh(void);
static uint32_t flair_live_do_menu_at(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                      MenuBar *bar, flair_point_t where0,
                                      uint32_t y_top)
{
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
static void flair_live_do_menu(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               MenuBar *bar, flair_point_t where0)
{
    (void)flair_live_do_menu_at(ctx, bi, bar, where0, 0u);
}
finder_shell_t *g_finder_shell;
finder_desk_t *g_finder_desk;
static finder_click_track_t g_finder_click;
int g_finder_db_dirty;
static FinderCtx g_finder_ctx;
static FlairProcessList *g_ten_plist;
static FlairApp *g_ten_finder;
static GrafPort *g_ten_barport;
finder_win_status_t g_finder_io_error;
uint16_t finder_notice(flair_live_ctx_t *ctx, const boot_info_t *bi,
                              finder_win_status_t st, const char *name);
static __attribute__((cold,noinline)) void flair_live_tenant_alert(flair_live_ctx_t *ctx, const boot_info_t *bi);
static void flair_live_tenant_scrollbar(flair_live_ctx_t *ctx, const boot_info_t *bi);
static __attribute__((cold,noinline)) void flair_live_tenant_sf(flair_live_ctx_t *ctx, const boot_info_t *bi);
static void flair_live_tenant_track(flair_live_ctx_t *ctx, const boot_info_t *bi);
static __attribute__((cold,noinline)) void flair_live_tenant_filecopy(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_tenant_service(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    FlairApp *app = tbx_take_affirm();
    if (app != (FlairApp *)0 && g_ten_plist != (FlairProcessList *)0) {
        EventRecord ev0;
        FlairApp *prev = g_ten_plist->head;
        ev0.what = (uint16_t)nullEvent;
        ev0.message = 0u;
        ev0.when = 0u;
        ev0.where.h = 0;
        ev0.where.v = 0;
        ev0.modifiers = 0u;
        if (FlairProcess_activate(g_ten_plist, ctx->wm, &ev0, app))
            flair_live_finish_tenant_switch(ctx, bi, g_ten_plist, prev,
                                            g_ten_barport);
    }
    if (tbx_take_mbar_redraw() && g_ten_barport != (GrafPort *)0 &&
        g_ten_plist != (FlairProcessList *)0) {
        DrawMenuBar(g_ten_barport, flair_live_tenant_bar(ctx, g_ten_plist), -1,
                    (const region_t *)0);
        flair_desktop_present(bi, &ctx->off);
    }
    if (tbx_take_drew()) flair_desktop_present(bi, &ctx->off);
    flair_live_tenant_alert(ctx, bi);
    flair_live_tenant_scrollbar(ctx, bi);
    flair_live_tenant_sf(ctx, bi);
    flair_live_tenant_track(ctx, bi);
    flair_live_tenant_filecopy(ctx, bi);
    {
        int reaped=tbx_reap();
        if (!reaped) return;
        if(reaped<0) (void)finder_notice(ctx,bi,
            reaped==-2?FINDER_WIN_ERR_LAUNCH:FINDER_WIN_ERR_CRASH,"");
    }
    flair_live_repaint(ctx);
    if (g_ten_barport != (GrafPort *)0)
        DrawMenuBar(g_ten_barport, flair_live_tenant_bar(ctx, ctx->plist), -1,
                    (const region_t *)0);
    flair_desktop_present(bi, &ctx->off);
}
void flair_live_launch_app(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                  const char *name83, uint16_t dir_start)
{
    int rc=tbx_launch(name83, dir_start);
    flair_live_tenant_service(ctx, bi);
    if (rc<0) (void)finder_notice(ctx,bi,FINDER_WIN_ERR_LAUNCH,name83);
}
void flair_live_tenant_heard(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    flair_live_tenant_service(ctx, bi);
    flair_desktop_present(bi, &ctx->off);
}
__attribute__((unused))
static void flair_live_tenant_menu_result(flair_live_ctx_t *ctx,
                                          const boot_info_t *bi,
                                          const MenuBar *bar, uint32_t sel,
                                          const EventRecord *ev)
{
    if (tbx_menu_choice(bar, sel, ev, "mouse"))
        flair_live_tenant_heard(ctx, bi);
}
static int flair_live_tenant_key(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                 const EventRecord *ev)
{
    if (!tbx_menu_key(ev)) return 0;
    flair_live_tenant_heard(ctx, bi);
    return 1;
}
const fat12_volume_t *g_finder_vol;
void *g_finder_fat;
uint32_t g_finder_fat_len;
void *g_finder_secbuf;
static char g_finder_vol_label[12];
uint8_t g_finder_db_buf[FINDER_DB_MAX_BYTES];
uint8_t g_finder_dirbuf[FINDER_FS_MAX_SPC * BLOCKDEV_SECTOR_SIZE];
uint8_t g_finder_clusbuf[FINDER_FS_MAX_SPC * BLOCKDEV_SECTOR_SIZE];
static int finder_abs(int v) { return v < 0 ? -v : v; }
static void __attribute__((unused)) finder_trace_serial(void *user, const char *line)
{
    (void)user;
    if (line == (const char *)0) return;
    serial_puts(line);
    serial_putc('\n');
}
typedef struct finder_fat_enum {
    finder_enum_cb cb;
    void *user;
} finder_fat_enum_t;
static int finder_fat_dirent_cb(const dir_entry_t *e, void *user)
{
    finder_fat_enum_t *fe = (finder_fat_enum_t *)user;
    finder_dirent_t d;
    char name[FAT12_NAME83_MAX];
    if (e == (const dir_entry_t *)0 || fe == (finder_fat_enum_t *)0) return 0;
    if (fat12_format_83(e, name) != FAT12_OK) return 0;
    d.name83 = name;
    d.attribute = e->attribute;
    d.size = e->file_size;
    d.start_cluster = e->start_cluster;
    d.date = e->mdate; d.time = e->mtime;
    return fe->cb(&d, fe->user);
}
int finder_fat_geometry_ok(void)
{
    return g_finder_vol != (const fat12_volume_t *)0 &&
           g_finder_vol->bpb.sectors_per_cluster >= 1u &&
           (uint32_t)g_finder_vol->bpb.sectors_per_cluster <= FINDER_FS_MAX_SPC;
}
static int finder_fat_enumerate(void *fs_user, uint16_t dir_start,
                                finder_enum_cb cb, void *cb_user)
{
    fat12_dir_t dir;
    finder_fat_enum_t fe;
    int rc;
    (void)fs_user;
    if (!finder_fat_geometry_ok() || cb == (finder_enum_cb)0) return -1;
    dir.is_root = (dir_start == 0u) ? 1 : 0;
    dir.start_cluster = dir_start;
    fe.cb = cb;
    fe.user = cb_user;
    rc = fat12_read_dir(g_finder_vol, &dir, g_finder_dirbuf,
                        g_finder_fat, g_finder_fat_len,
                        finder_fat_dirent_cb, &fe);
    return (rc < 0) ? -1 : rc;
}
static int finder_fat_trash_dir(void *fs_user)
{
    dir_entry_t e;
    uint32_t slot = 0u;
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return -1;
    if (fat12_find_slot_in(g_finder_vol, g_finder_fat, g_finder_fat_len, 0u,
                           g_finder_dirbuf, DESKTOP_TRASH_NAME, &e, &slot) !=
        FAT12_OK)
        return -1;
    if ((e.attribute & DIR_ATTR_DIRECTORY) == 0u || e.start_cluster < 2u)
        return -1;
    return (int)e.start_cluster;
}
static const finder_fs_t g_finder_fs_binding __attribute__((unused)) = {
    finder_fat_enumerate,
    finder_fat_mkdir,
    finder_fat_move,
    finder_fat_trash_name,
    finder_fat_trash_dir,
    finder_fat_unlink,
    finder_fat_rmdir,
    finder_fat_create,
    finder_fat_read_partial,
    finder_fat_write_partial,
    finder_fat_dir_ident,
    (void *)0
};
static void __attribute__((unused)) finder_desk_load_positions(void)
{
    uint32_t len = 0u;
    int rc;
    if (g_finder_desk == (finder_desk_t *)0 ||
        g_finder_vol == (const fat12_volume_t *)0) return;
    rc = desktop_db_read(g_finder_vol, g_finder_fat, g_finder_fat_len,
                         g_finder_secbuf, g_finder_clusbuf,
                         g_finder_db_buf, (uint32_t)sizeof g_finder_db_buf,
                         &len);
    if (rc != FAT12_OK) {
        serial_puts("DESKTOP-DB-POS-SKIP rc=");
        serial_puti((int32_t)rc);
        serial_putc('\n');
        return;
    }
    if (finder_desk_db_apply(g_finder_desk, g_finder_db_buf, len) !=
        FINDER_DB_OK) {
        serial_puts("DESKTOP-DB-POS-REGEN\n");
        return;
    }
    if (g_finder_shell != (finder_shell_t *)0) {
        uint16_t views = finder_shell_load_views(g_finder_shell,
                                                 g_finder_db_buf, len);
        uint16_t origins;
        if (views != 0u) {
            serial_puts("DESKTOP-DB-VIEWS n=");
            serial_putu((uint32_t)views);
            serial_putc('\n');
        }
        origins = finder_shell_load_origins(g_finder_shell, g_finder_db_buf,
                                            len);
        if (origins != 0u) {
            serial_puts("DESKTOP-DB-ORIGINS n=");
            serial_putu((uint32_t)origins);
            serial_putc('\n');
        }
    }
}
static void flair_tenant_mount_volume(void)
{
    const fat12_volume_t *v=fileio_fat_current_volume();
    uint8_t *ften_secbuf=g_volume_sector;
    uint8_t ften_cluster[BLOCKDEV_SECTOR_SIZE];
    uint8_t ften_db_header[DESKTOP_DB_HEADER_SIZE];
    int mount_rc=v ? FAT12_OK : FAT12_ERR_PARTITION;
    serial_puts("FLAIR-FAT-MOUNT-BEGIN\n");
    if (mount_rc == FAT12_OK) {
        int bind_rc;
        serial_puts("FLAIR-FAT-MOUNT-OK\n");
        bind_rc = FAT12_OK;
        if (bind_rc == FAT12_OK) {
            uint32_t fat_len = 0u;
            uint8_t *fat = (uint8_t *)fileio_fat_fat_buffer(&fat_len);
            desktop_db_boot_state_t db_state = DESKTOP_DB_BOOT_OK;
            desktop_db_reason_t db_reason = DESKTOP_DB_REASON_NONE;
            desktop_trash_state_t trash_state = DESKTOP_TRASH_OK;
            int db_rc;
            int trash_rc;
            db_rc = desktop_db_bootstrap(v, fat, fat_len,
                                          ften_secbuf, ften_cluster,
                                          ften_db_header,
                                          sizeof(ften_db_header),
                                          &db_state, &db_reason);
            if (db_state == DESKTOP_DB_BOOT_CREATE) {
                serial_puts("DESKTOP-DB-CREATE\n");
            } else if (db_state == DESKTOP_DB_BOOT_REGEN) {
                serial_puts("DESKTOP-DB-REGEN reason=");
                serial_putu((uint32_t)db_reason);
                serial_putc('\n');
            } else if (db_rc == FAT12_OK) {
                serial_puts("DESKTOP-DB-OK\n");
            }
            if (db_rc != FAT12_OK) {
                g_finder_io_error=db_rc==FAT12_ERR_NO_SPACE || db_rc==FAT12_ERR_DIR_FULL?
                                    FINDER_WIN_ERR_DISKFULL:FINDER_WIN_ERR_STORAGE;
                if (db_state == DESKTOP_DB_BOOT_CREATE ||
                    db_state == DESKTOP_DB_BOOT_REGEN) {
                    serial_puts("DESKTOP-DB-WRITE-FAIL rc=");
                } else {
                    serial_puts("DESKTOP-DB-READ-FAIL rc=");
                }
                serial_puti((int32_t)db_rc);
                serial_putc('\n');
            }
            trash_rc = desktop_trash_ensure(v, fat, fat_len,
                                            ften_secbuf, ften_cluster,
                                            &trash_state);
            if (trash_rc == FAT12_OK) {
                serial_puts(trash_state == DESKTOP_TRASH_CREATE
                                ? "TRASH-CREATE\n" : "TRASH-OK\n");
            } else {
                serial_puts("TRASH-INIT-FAIL rc=");
                g_finder_io_error=trash_rc==FAT12_ERR_NO_SPACE || trash_rc==FAT12_ERR_DIR_FULL?
                                    FINDER_WIN_ERR_DISKFULL:FINDER_WIN_ERR_STORAGE;
                serial_puti((int32_t)trash_rc);
                serial_putc('\n');
            }
            loader_bind_fat_volume(v, fat, fat_len);
            g_finder_vol = v;
            g_finder_fat = fat;
            g_finder_fat_len = fat_len;
            g_finder_secbuf = ften_secbuf;
            flair_live_apple_refresh();
            if (desktop_db_volume_label(v, ften_secbuf,
                                        g_finder_vol_label,
                                        (uint32_t)sizeof g_finder_vol_label) !=
                FAT12_OK) {
                g_finder_vol_label[0] = '\0';
            }
        } else {
            serial_puts("FLAIR-FAT-BIND-FAIL rc=");
            serial_puti((int32_t)bind_rc);
            serial_putc('\n');
        }
    } else {
        serial_puts("FLAIR-FAT-MOUNT-FAIL rc=");
        serial_puti(mount_rc);
    }
}
static void finder_desk_sync_ctx(void)
{
    if (g_finder_shell != (finder_shell_t *)0) {
        finder_shell_sync_ctx(g_finder_shell);
        return;
    }
    g_finder_ctx.selection_count =
        (g_finder_desk != (finder_desk_t *)0)
            ? finder_desk_selection_count(g_finder_desk) : 0u;
}
static void flair_live_refresh_finder_bar(MenuBar *bar)
{
    if (bar != finder_menu_bar()) return;
    finder_desk_sync_ctx();
    finder_menu_refresh_enables(&g_finder_ctx);
}
static int flair_live_finder_is_foreground(void)
{
    return g_ten_plist != (FlairProcessList *)0 &&
           g_ten_finder != (FlairApp *)0 &&
           g_ten_plist->head == g_ten_finder;
}
int finder_trash_confirm(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                uint16_t items);
int finder_delete_now_confirm(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                     const char *name);
void finder_surface_desktop(finder_surface_t *s)
{
    s->fd = g_finder_desk;
    s->click = &g_finder_click;
    s->slot = -1;
}
static int finder_surface_window(finder_surface_t *s, int slot)
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
static void finder_surface_invalidate(const finder_surface_t *s,
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
static void finder_report_open_fail(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static void finder_open_volume(flair_live_ctx_t *ctx, const boot_info_t *bi)
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
static void finder_open_folder(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static void finder_open_trash(flair_live_ctx_t *ctx, const boot_info_t *bi)
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
static void finder_surface_open(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static void flair_live_do_surface(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static void flair_live_do_desk(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static void flair_live_do_finder_content(flair_live_ctx_t *ctx,
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
static void flair_live_do_finder_close(flair_live_ctx_t *ctx,
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
static rgn_rect_t fta_rect(int l, int t, int r, int b)
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
static DialogRecord g_al_dr;
static DialogItem g_al_it[3 + FINDER_ALERT_LINES + 1];
static ControlRecord g_al_btn[3];
static finder_win_rgn_t g_al_rs, g_al_rc, g_al_ru;
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
static __attribute__((cold,noinline)) void flair_live_tenant_alert(flair_live_ctx_t *ctx, const boot_info_t *bi)
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
static void flair_live_tenant_scrollbar(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    if (!tbx_take_bar_dirty()) return;
    desktop_paint_damage(ctx->wm, &ctx->off, ctx->comp);
    flair_desktop_present(bi, &ctx->off);
}
static void flair_live_tenant_track(flair_live_ctx_t *ctx, const boot_info_t *bi)
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
typedef struct sf_geom {
    int16_t w, h;
    int16_t pop_l, pop_r;
    int16_t list_l, rows;
    int16_t btn_l;
    int16_t eject_y, desk_y, new_y, cancel_y, def_y;
} sf_geom_t;
static const sf_geom_t g_sf_geom[2] = {
    { 348, 193, 68, 176, 13, 7, 250, 38, 66, 0, 110, 141 },
    { 356, 250, 73, 181, 18, 8, 258, 38, 66, 93, 136, 167 },
};
static FlairStandardFile g_sf;
static DialogRecord g_sf_dr;
static DialogItem g_sf_it[3];
static ControlRecord g_sf_btn[5];
static finder_win_rgn_t g_sf_rs, g_sf_rc, g_sf_ru;
typedef struct sf_walk {
    const char *want;
    uint16_t dir;
    FlairSFEntry *out;
    int16_t n, max;
    uint16_t at;
} sf_walk_t;
static int sf_upper(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }
static int sf_ieq(const char *a, const char *b)
{
    while (*a != '\0' && sf_upper(*a) == sf_upper(*b)) { a++; b++; }
    return sf_upper(*a) == sf_upper(*b);
}
static int sf_walk_cb(const finder_dirent_t *d, void *user)
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
static int sf_provider(const char *path, FlairSFEntry *out, int16_t max, void *user)
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
static void sf_fill(flair_live_ctx_t *ctx, int l, int t, int r, int b, int part)
{
    blitter_fill_rect_clipped(&ctx->off, fta_rect(l, t, r, b),
                              flair_look_pixel_depth(ctx->off.bpp, part),
                              (const region_t *)0);
}
static void sf_frame(flair_live_ctx_t *ctx, int l, int t, int r, int b)
{
    sf_fill(ctx, l, t, r, t + 1, (int)FLAIR_PART_FRAME);
    sf_fill(ctx, l, b - 1, r, b, (int)FLAIR_PART_FRAME);
    sf_fill(ctx, l, t, l + 1, b, (int)FLAIR_PART_FRAME);
    sf_fill(ctx, r - 1, t, r, b, (int)FLAIR_PART_FRAME);
}
static void sf_text(flair_live_ctx_t *ctx, int x, int y, const char *s,
                    int fg, int bg)
{
    (void)text_draw_run(&ctx->off, x, y, s, FONT_CHICAGO, 0u,
                        flair_look_pixel_depth(ctx->off.bpp, fg),
                        flair_look_pixel_depth(ctx->off.bpp, bg),
                        (const text_clip_t *)0);
}
typedef struct sf_ui {
    int put, sel, top, l, t;
    const sf_geom_t *g;
    char name[TBX_SF_NAME_MAX];
    const char *prompt;
} sf_ui_t;
static void sf_dos_path(const char *name, char *out)
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
static const char *sf_vol(void)
{
    return g_finder_vol_label[0] ? g_finder_vol_label : "INITECH";
}
static void sf_dim_button(flair_live_ctx_t *ctx, ControlRecord *c)
{
    rgn_rect_t r = c->contrlRect;
    DrawControl(&g_sf_dr.window.port, c);
    sf_text(ctx, r.left + (80 - text_measure(FONT_CHICAGO, c->contrlTitle)) / 2,
            r.top + 2, c->contrlTitle, (int)FLAIR_PART_MENU_DISABLED_INK,
            (int)FLAIR_PART_PLAT_FACE);
}
static void sf_draw_static(flair_live_ctx_t *ctx, const sf_ui_t *u)
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
static void sf_draw_dynamic(flair_live_ctx_t *ctx, const sf_ui_t *u)
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
static void sf_select(sf_ui_t *u, int i)
{
    if (g_sf.count <= 0) { u->sel = -1; return; }
    if (i < 0) i = 0;
    if (i >= g_sf.count) i = g_sf.count - 1;
    u->sel = i;
    FlairSF_select(&g_sf, (int16_t)i);
    if (i < u->top) u->top = i;
    if (i >= u->top + u->g->rows) u->top = i - u->g->rows + 1;
}
static void sf_reset_view(sf_ui_t *u)
{
    u->top = 0;
    u->sel = -1;
    if (!u->put) sf_select(u, 0);
}
static void sf_report(const char *what, const sf_ui_t *u)
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
static int sf_name_ok(const char *n)
{
    int i = 0, base = 0, ext = -1;
    if (n[0] == '\0' || n[0] == '.') return 0;
    for (; n[i] != '\0'; i++) {
        if (n[i] == '.') { if (ext >= 0) return 0; ext = 0; continue; }
        if (ext >= 0) ext++; else base++;
    }
    return base >= 1 && base <= 8 && ext <= 3;
}
static void sf_button(ControlRecord *c, const sf_ui_t *u, int y, const char *t)
{
    control_init(c, pushButton,
                 fta_rect(u->l + u->g->btn_l, u->t + y,
                          u->l + u->g->btn_l + 80, u->t + y + 20),
                 0, 0, 1, 1, t);
}
static int sf_run(flair_live_ctx_t *ctx, const boot_info_t *bi, sf_ui_t *u,
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
static __attribute__((cold,noinline)) void flair_live_tenant_sf(flair_live_ctx_t *ctx, const boot_info_t *bi)
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
static __attribute__((cold,noinline)) void flair_live_tenant_filecopy(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    const char *cap, *src, *dst;
    if (tbx_take_filecopy(&cap, &src, &dst))
        finder_filecopy_run(ctx, bi, cap, src, dst);
}
static int flair_live_apple_installed(void *user, const char *dir83,
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
static void flair_live_apple_refresh(void)
{
    const MenuInfo *m = apple_menu_build(flair_live_apple_installed, (void *)0);
    MenuBar_set_apple_menu(m);
    serial_puts("APPLE-MENU n=");
    serial_putu((uint32_t)m->n_items);
    serial_putc('\n');
}
static int flair_live_apple_result(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static void finder_report_outcome(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    const finder_cmd_outcome_t *o;
    if (g_finder_shell == (finder_shell_t *)0) return;
    o = finder_shell_take_outcome(g_finder_shell);
    if (o == (const finder_cmd_outcome_t *)0) return;
    finder_outcome_run(ctx, bi, o);
}
static void flair_live_sync_foreground(flair_live_ctx_t *ctx,
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
static void flair_live_desk_activate(flair_live_ctx_t *ctx,
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
static int flair_live_finder_key(flair_live_ctx_t *ctx, const boot_info_t *bi,
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
static void flair_live_finder_menu_result(flair_live_ctx_t *ctx,
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
void kernel_graphical_environment(void)
{
    boot_info_t b=*boot_info();
    g_desktop_exit=0;
    g_vga_dac_shift=b.lfb_dac_bits==8?0:2;
    flair_boot_screen(&b);
    flair_tenant_mount_volume();
    flair_live_ctx_t ctx;
    FlairProcessList ten_plist;
    FlairApp *ten_finder = (FlairApp *)0;
    flair_desktop_run(&b, &ctx);
    serial_puts("FLAIR-DESKTOP\n");
    HideWindow(ctx.wm, &ctx.windows[0].rec);
    HideWindow(ctx.wm, &ctx.windows[1].rec);
    FlairProcessList_init(&ten_plist);
    {
        rgn_rect_t db;
        db.top = (int16_t)(SHELL_MENUBAR2_TOP + FLAIR_MENUBAR_H);
        db.left = 0;
        db.bottom = (int16_t)FLAIR_SCREEN_H;
        db.right = (int16_t)FLAIR_SCREEN_W;
        ten_finder = FlairProcess_launch(&ten_plist, ctx.wm, &ctx.off, ctx.master,
                                         &finder_shell_procs, "FINDER", db,
                                         (uint32_t)(64u * 1024u),
                                         (uint32_t)(256u * 1024u));
        if (ten_finder == (FlairApp *)0) {
            serial_puts("PANIC flair-tenants: FINDER launch returned NULL "
                        "(budget/heap exhausted)\nHALTED\n");
            while (!g_desktop_exit) { __asm__ __volatile__("cli; hlt"); }
        }
        g_finder_shell = finder_shell_of(ten_finder);
        g_finder_desk = finder_shell_desk(g_finder_shell);
        if (g_finder_desk == (finder_desk_t *)0) {
            serial_puts("PANIC flair-tenants: FINDER has no desktop model\n"
                        "HALTED\n");
            while (!g_desktop_exit) { __asm__ __volatile__("cli; hlt"); }
        }
        finder_click_reset(&g_finder_click);
        {
            int vidx = finder_desk_find_kind(g_finder_desk, FINDER_ICON_VOLUME);
            if (vidx >= 0)
                finder_desk_set_name(g_finder_desk, vidx, g_finder_vol_label);
        }
        finder_desk_load_positions();
        finder_desk_install_underlay(g_finder_desk, ctx.wm);
        finder_shell_bind_fs(g_finder_shell, &g_finder_fs_binding);
        if (finder_shell_recount_trash(g_finder_shell)) {
            g_finder_shell->trash_icon_dirty = 0u;
            serial_puts("DESK-TRASH-ICON full\n");
        }
        g_finder_ctx.trace = finder_trace_serial;
        g_finder_ctx.trace_user = (void *)0;
        finder_shell_bind_ctx(g_finder_shell, &g_finder_ctx);
        finder_desk_sync_ctx();
        ten_finder->menubar = finder_menu_bar();
        g_ten_plist = &ten_plist;
        g_ten_finder = ten_finder;
        serial_puts("FINDER-DESKTOP-ICONS n=");
        serial_putu((uint32_t)g_finder_desk->n);
        serial_putc('\n');
    }
    ctx.plist = &ten_plist;
    {
        tbx_host_t th;
        th.list = &ten_plist;
        th.wm = ctx.wm;
        th.master = ctx.master;
        th.surface = &ctx.off;
        th.puts = serial_puts;
        th.cellfont = (const uint8_t *)(uintptr_t)b.font_addr;
        th.scrap = (FlairScrap *)flair_alloc(ctx.master, FLAIR_CLASS_GENERAL,
                                             (uint32_t)sizeof(FlairScrap));
        if (th.scrap == (FlairScrap *)0) flair_desktop_oom("desk scrap");
        FlairScrap_init(th.scrap);
        serial_puts("FLAIR-SCRAP-READY bytes=");
        serial_putu((uint32_t)sizeof(FlairScrap));
        serial_putc('\n');
        tbx_bind(&th);
        tbx_preferences_boot(1024u+b.ext_mem_kb);
        idt_set_gate((uint8_t)TBX_GATE_VECTOR, (void *)tbx_gate_entry,
                     (uint16_t)TBX_GATE_SELECTOR, (uint8_t)TBX_GATE_TYPE_ATTR);
        panic_set_fault_hook(tbx_fault_triage);
        serial_puts("TBX-GATE-READY vec=0x81\n");
    }
    shell_render_with_menubar(ctx.scene, &ctx.off, finder_menu_bar());
    desktop_validate_all(ctx.wm);
    flair_desktop_present(&b, &ctx.off);
    flair_event_init(&g_flair_kbd_ring);
    kbd_state_init(&g_flair_consumed_keys);
    flair_event_set_keyboard_observer(flair_consumed_key);
    g_flair_mouse_activity = 0u;
    g_flair_cursor_visible = 0;
    g_flair_cursor_x = (int)(FLAIR_SCREEN_W / 2);
    g_flair_cursor_y = (int)(FLAIR_SCREEN_H / 2);
    flair_cursor_show(g_flair_cursor_x, g_flair_cursor_y);
    g_flair_cursor_visible = 1;
    serial_puts("FLAIR-CURSOR x=");
    serial_puti((int32_t)g_flair_cursor_x);
    serial_puts(" y=");
    serial_puti((int32_t)g_flair_cursor_y);
    serial_putc('\n');
    __asm__ volatile("cli" : : : "memory");
    pit_set_tick_hook(flair_tick_advance);
    kbd_set_scancode_hook(flair_live_kbd_post);
    idt_install_irq(0x34u, (void *)irq12_entry);
    mouse_init();
    mouse_set_event_hook(flair_live_mouse_post);
    serial_puts("FLAIR-HOOK-SET\n");
    __asm__ volatile("sti");
    kbd_set_input_owner(KBD_OWNER_GUI);
    int21_conin_handoff();
    pic_unmask_irq12();
    flair_event_set_yield(flair_live_yield);
    serial_puts("FLAIR-LIVE-READY\n");
    {
        enum { FLAIR_TEN_TICK_ANNOUNCE = 4 };
        enum { FLAIR_TEN_WNE_SLICE = 3 };
        uint32_t start = flair_tick_count();
        uint32_t last = start;
        uint32_t seen = 0u;
        ((void)(start + (uint32_t)250));
        bitmap_t ten_bar2_view;
        GrafPort ten_barport;
        {
            make_offset_view(&ten_bar2_view, &ctx.off, (uint32_t)SHELL_MENUBAR2_TOP);
            rgn_rect_t whole = { 0, 0, (int16_t)ten_bar2_view.height,
                                 (int16_t)ten_bar2_view.width };
            ten_barport.portBits.bm = ten_bar2_view;
            ten_barport.portBits.bounds = whole;
            ten_barport.portRect = whole;
            ten_barport.visRgn = (region_t *)0;
            ten_barport.clipRgn = (region_t *)0;
            ten_barport.pnLoc.v = 0; ten_barport.pnLoc.h = 0;
            ten_barport.pnSize.v = 1; ten_barport.pnSize.h = 1;
            ten_barport.pnVis = 0;
            ten_barport.grafProcs = (QDProcs *)0;
        }
        g_ten_barport = &ten_barport;
        MenuClock_attach(&ctx.scene->bar_sys);
        uint32_t clock_last_poll = 0;
        int clock_polled = 0;
        while (!g_desktop_exit) {
            if(g_finder_io_error) {
                finder_win_status_t st=g_finder_io_error;
                g_finder_io_error=FINDER_WIN_OK;
                (void)finder_notice(&ctx,&b,st,"");
            }
            EventRecord ev;
            int got = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev,
                                    (uint32_t)FLAIR_TEN_WNE_SLICE);
            flair_live_cursor_track(&ev);
            tbx_preferences_tick();
            if(tbx_preferences_dirty()) {
                for(WindowPtr w=ctx.wm->front;w;w=w->nextWindow)
                    WindowMgr_invalidate(ctx.wm,w,region_get_bbox(w->contRgn));
                shell_render_with_menubar(ctx.scene,&ctx.off,
                                          flair_live_tenant_bar(&ctx,&ten_plist));
                flair_live_content_phase(&ctx);
                flair_desktop_present(&b,&ctx.off);
            }
            uint32_t now = flair_tick_count();
            if (!clock_polled || now - clock_last_poll >= 100u) {
                clock_polled = 1;
                clock_last_poll = now;
                int_frame_t time = { 0 };
                time.eax = 0x2C00u;
                int21_dispatch(&time);
                serial_puts("FLAIR-CLOCK-POLL\n");
                if (MenuClock_set((time.ecx >> 8) & 255u, time.ecx & 255u)) {
                    GrafPort clock_port = ten_barport;
                    clock_port.portBits.bm = ctx.off;
                    MenuClock_draw(&clock_port, (const region_t *)0);
                    flair_desktop_present(&b, &ctx.off);
                    serial_puts("FLAIR-CLOCK ");
                    serial_puts(MenuClock_text());
                    serial_putc('\n');
                }
            }
            if (now != last) {
                last = now;
                seen++;
                if (seen <= (uint32_t)FLAIR_TEN_TICK_ANNOUNCE) {
                    serial_puts("FLAIR-TICK n=");
                    serial_putu(now);
                    serial_putc('\n');
                }
            }
            if (!got) {
                continue;
            }
            flair_live_emit_evt(&ev);

            if (flair_live_finder_key(&ctx, &b, &ev)) {
                continue;
            }
            if (flair_live_tenant_key(&ctx, &b, &ev)) {
                continue;
            }
            WindowPtr chrome_w = (WindowPtr)0;
            flair_part_code_t chrome_pc = inDesk;
            int chrome_was_active = 0;
            if (ev.what == (uint16_t)mouseDown) {
                chrome_pc = FindWindow(ctx.wm, ev.where, &chrome_w);
                chrome_was_active = chrome_w != (WindowPtr)0 && chrome_w->hilited;
            }
            FlairApp *ten_prev_fg = ten_plist.head;
            flair_app_dispatch(&ten_plist, ctx.wm, &ev);
            flair_live_finish_tenant_switch(&ctx, &b, &ten_plist,
                                            ten_prev_fg, &ten_barport);
            if (ev.what == (uint16_t)mouseDown) {
                if (chrome_pc == inDrag && chrome_w != (WindowPtr)0) {
                    flair_live_do_drag(&ctx, &b, chrome_w, ev.where);
                } else if (chrome_pc == inGoAway && chrome_w != (WindowPtr)0) {
                    if (!flair_live_track_box(&ctx, &b, chrome_w, chrome_pc)) {
                    } else {
                    int fslot = (g_finder_shell != (finder_shell_t *)0)
                        ? finder_win_slot_of(g_finder_shell, chrome_w) : -1;
                    if (fslot >= 0)
                        flair_live_do_finder_close(&ctx, &b, fslot);
                    else
                        flair_live_do_close(&ctx, &b, chrome_w, &ten_barport);
                    }
                } else if ((chrome_pc == inZoomIn || chrome_pc == inZoomOut) &&
                           chrome_w != (WindowPtr)0) {
                    if (flair_live_track_box(&ctx, &b, chrome_w, chrome_pc))
                        flair_live_do_zoom(&ctx, &b, chrome_w);
                } else if (chrome_pc == inGrow && chrome_w != (WindowPtr)0) {
                    flair_live_do_grow(&ctx, &b, chrome_w, ev.where);
                } else if (chrome_pc == inCollapse &&
                           chrome_w != (WindowPtr)0) {
                    if (flair_live_track_box(&ctx, &b, chrome_w, chrome_pc))
                        flair_live_do_collapse(&ctx, &b, chrome_w);
                } else if (ev.where.v >= 0 &&
                           ev.where.v < (int16_t)FLAIR_MENUBAR_H) {
                    (void)flair_live_apple_result(&ctx, &b,
                        flair_live_do_menu_at(&ctx, &b, &ctx.scene->bar_sys,
                                              ev.where, 0u));
                } else if (ev.where.v >= (int16_t)SHELL_MENUBAR2_TOP &&
                           ev.where.v < (int16_t)(SHELL_MENUBAR2_TOP +
                                                  FLAIR_MENUBAR_H)) {
                    MenuBar *menu2_bar = flair_live_tenant_bar(&ctx, &ten_plist);
                    uint32_t menu2_sel =
                        flair_live_do_menu_at(&ctx, &b, menu2_bar, ev.where,
                                              (uint32_t)SHELL_MENUBAR2_TOP);
                    if (!flair_live_apple_result(&ctx, &b, menu2_sel)) {
                        flair_live_finder_menu_result(&ctx, &b, menu2_bar,
                                                      menu2_sel);
                        flair_live_tenant_menu_result(&ctx, &b, menu2_bar,
                                                      menu2_sel, &ev);
                    }
                }
                if (chrome_pc == inDesk &&
                    ev.where.v >= (int16_t)(SHELL_MENUBAR2_TOP +
                                            FLAIR_MENUBAR_H)) {
                    flair_live_desk_activate(&ctx, &b, &ev);
                    flair_live_do_desk(&ctx, &b, &ev);
                }
                if (chrome_pc == inContent && chrome_w != (WindowPtr)0 &&
                    WindowScrollBand(chrome_w, ev.where) >= 0) {
                    if (chrome_was_active)
                        flair_live_track_scroll(&ctx, &b, chrome_w, ev.where);
                } else if (chrome_pc == inContent && chrome_w != (WindowPtr)0 &&
                    g_finder_shell != (finder_shell_t *)0) {
                    int fslot = finder_win_slot_of(g_finder_shell, chrome_w);
                    if (fslot >= 0 && chrome_was_active)
                        flair_live_do_finder_content(&ctx, &b, &ev, fslot);
                }
                flair_live_sync_foreground(&ctx, &b, &ev);
            }
            flair_live_tenant_service(&ctx, &b);
        }
    }
    flair_cursor_hide();
    pit_set_tick_hook(0);
    kbd_set_scancode_hook(0);
    mouse_set_event_hook(0);
    panic_set_fault_hook(0);
    kbd_set_input_owner(KBD_OWNER_DOS);
    int21_conin_handoff();
    flair_event_set_yield(0);
    desktop_console_restore();
    serial_puts("FLAIR-EXIT-TO-DOS\n");
}
char g_finder_notice_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
char g_finder_trash_confirm_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
char g_finder_delete_now_confirm_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
