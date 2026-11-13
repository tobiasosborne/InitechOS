/* Extracted production launch; reference: legacy-final kmain.c. */
#include "desktop_internal.h"

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
void flair_live_tenant_menu_result(flair_live_ctx_t *ctx,
                                          const boot_info_t *bi,
                                          const MenuBar *bar, uint32_t sel,
                                          const EventRecord *ev)
{
    if (tbx_menu_choice(bar, sel, ev, "mouse"))
        flair_live_tenant_heard(ctx, bi);
}

int flair_live_tenant_key(flair_live_ctx_t *ctx, const boot_info_t *bi,
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

char g_finder_vol_label[12];

uint8_t g_finder_db_buf[FINDER_DB_MAX_BYTES];

uint8_t g_finder_dirbuf[FINDER_FS_MAX_SPC * BLOCKDEV_SECTOR_SIZE];

uint8_t g_finder_clusbuf[FINDER_FS_MAX_SPC * BLOCKDEV_SECTOR_SIZE];
