/* Extracted production scene; reference: legacy-final kmain.c. */
#include "desktop_internal.h"
#include "boot_screen.h"

unsigned g_vga_dac_shift;

uint8_t g_volume_sector[512];

int g_desktop_exit;

void vga_set_dac(uint8_t i,uint8_t r,uint8_t g,uint8_t b)
{ outb(0x3c8,i);outb(0x3c9,r>>g_vga_dac_shift);outb(0x3c9,g>>g_vga_dac_shift);outb(0x3c9,b>>g_vga_dac_shift); }

void flair_desktop_oom(const char *what)
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

region_t *flair_desktop_alloc_region(flair_heap_t *h, const char *what)
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

void flair_boot_screen_row(void *ctx, int y, const uint8_t *row)
{
    const bitmap_t *bm = (const bitmap_t *)ctx;
    uint32_t off = (uint32_t)y * bm->pitch;
    for (int x = 0; x < BOOT_SCREEN_W; x++, off += bm->bytes_per_pixel)
        surface_put_pixel(bm, off, bm->bpp == 8u ? (uint32_t)row[x]
                                   : (flair_palette_rgb(row[x]) & 0x00FFFFFFu));
}

void flair_dac_load(void)
{
    for (uint32_t idx = 0; idx < sizeof(flair_dac)/sizeof(flair_dac[0]); idx++) {
        uint32_t rgb = flair_palette_rgb((uint8_t)idx);
        vga_set_dac((uint8_t)idx, (uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb);
    }
}

void flair_boot_screen(const boot_info_t *bi)
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

void flair_boot_screen_hold(void)
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

void flair_desktop_run(const boot_info_t *bi, flair_live_ctx_t *ctx_out)
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
    shell_window_store *wins = 0;
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
                      wins, 0, 0, 0,
                      desk, sa, sb, sc, comp,
                      finder_menu_bar(),
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
        ctx_out->n_windows = 0;
        ctx_out->comp = comp;
        ctx_out->off = off;
        ctx_out->scene = scene;
        ctx_out->master = &heap;
        ctx_out->plist = (FlairProcessList *)0;
    }
}

flair_raw_ring_t g_flair_kbd_ring;

kbd_state_t g_flair_consumed_keys;
