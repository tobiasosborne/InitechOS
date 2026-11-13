#include "finder_actions.h"
#include "power.h"
#include "power_notice.h"
#include "color_canon.h"
void flair_live_tenant_heard(flair_live_ctx_t *, const boot_info_t *);
#include "tbxgate.h"
extern int g_desktop_exit;
extern void serial_puts(const char *);
extern void serial_puthex32(uint32_t);
static void finder_shutdown_life_check(void) {}
static inline void dos_puts(const char *dollar_terminated)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x0900u), "d"((uint32_t)(uintptr_t)dollar_terminated)
        : "cc", "memory");
}
static rgn_rect_t fta_rect(int l, int t, int r, int b)
{
    rgn_rect_t x;
    x.left = (int16_t)l; x.top = (int16_t)t; x.right = (int16_t)r; x.bottom = (int16_t)b;
    return x;
}
static const uint8_t FSD_ICON[128] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x7F, 0xFF, 0xFF,
    0x80, 0x7F, 0xFF, 0xFF, 0x80, 0x7F, 0xFF, 0xFF,
    0x80, 0x7F, 0xFF, 0xFF, 0x80, 0x7F, 0xC0, 0xFF,
    0x88, 0x7F, 0x00, 0x3F, 0x88, 0x7E, 0x00, 0x1F,
    0x88, 0x7C, 0x00, 0x0F, 0x80, 0x78, 0x00, 0x07,
    0x80, 0x78, 0x00, 0x07, 0x80, 0x70, 0x00, 0x03,
    0x80, 0x71, 0xDD, 0xC3, 0x80, 0x70, 0x00, 0x03,
    0x80, 0x70, 0x00, 0x03, 0x80, 0x71, 0xDD, 0x43,
    0x80, 0x70, 0x00, 0x03, 0x80, 0x70, 0x00, 0x03,
    0x80, 0x71, 0xD7, 0x03, 0x80, 0x70, 0x00, 0x03,
    0x87, 0xF0, 0x00, 0x03, 0x81, 0xF1, 0xEE, 0xC3,
    0x81, 0xF0, 0x00, 0x07, 0x81, 0xF0, 0x00, 0x07,
    0x81, 0xF0, 0x00, 0x0F, 0x81, 0xE0, 0x00, 0x1F,
    0x8F, 0x80, 0x00, 0x7F, 0x81, 0xFF, 0xFF, 0xFF,
    0x81, 0xFF, 0xFF, 0xFF, 0x81, 0xFF, 0xFF, 0xFF,
    0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static  int finder_shutdown_screen(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    static const char *const text[3] = {
        "It is now safe to switch off your computer.",
        "If you wish to continue using your computer,",
        "press Restart."
    };
    static DialogRecord dr;
    static DialogItem it[4];
    static ControlRecord restart_c;
    static finder_win_rgn_t rs, rc, ru;
    const bitmap_t *off = &ctx->off;
    uint16_t hit = 0u;
    int k;
    for (uint32_t y = 0u; y < off->height; y++)
        surface_fill_span(off, 0u, y, off->width, (uint32_t)CIDX_BLACK);
    control_init(&restart_c, pushButton,
                 fta_rect((404 + 64), (154 + 69), (463 + 64), (174 + 69)),
                 0, 0, 1, 1, "Restart");
    for (k = 0; k < 4; k++) {
        it[k].type = (k == 0) ? ctrlItem : statText;
        it[k].ctrl = (k == 0) ? &restart_c : (ControlRecord *)0;
        it[k].rect = (k == 0) ? restart_c.contrlRect
                                 : fta_rect((114 + 64), (94 - 12 + 69) + 16 * (k - 1),
                                            (463 + 64), (94 - 12 + 69) + 16 * k);
        it[k].text = (k == 0) ? (const char *)0 : text[k - 1];
        it[k].enabled = (uint8_t)((k == 0) ? 1 : 0);
        it[k]._pad[0] = it[k]._pad[1] = it[k]._pad[2] = 0u;
    }
    fta_rgn(&rs); fta_rgn(&rc); fta_rgn(&ru);
    (void)NewDialog(&dr, fta_rect((32 + 64), (64 + 69), (482 + 64), (190 + 69)), "", it, 4u,
                    1u , 0u,
                    (WindowMgr *)0, &rs.r, &rc.r, &ru.r);
    dr.window.port.portBits.bm = ctx->off;
    dr.window.port.visRgn = (region_t *)0;
    dr.window.port.clipRgn = (region_t *)0;
    dr.window.port.portRect = region_get_bbox(dr.window.strucRgn);
    DrawDialog(&dr);
    for (int r = 0; r < 32; r++)
        for (int c = 0; c < 32; c++)
            surface_put_pixel(off,
                              (uint32_t)((81 + 69) + r) * off->pitch +
                              (uint32_t)((60 + 64) + c) * off->bytes_per_pixel,
                              (FSD_ICON[r * 4 + (c >> 3)] & (0x80u >> (c & 7)))
                                  ? (uint32_t)CIDX_BLACK : (uint32_t)CIDX_WHITE);
    flair_desktop_present(bi, &ctx->off);
    serial_puts("FLAIR-SHUTDOWN-SAFE\n");
    for (;;) {
        EventRecord ev;
        int g = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev, 3u);
        flair_live_cursor_track(&ev);
        if (g) {
            flair_live_emit_evt(&ev);
            if (DialogHandleEvent(&dr, &ev, (dialog_filter_fn)0, &hit) &&
                hit == 1u)
                return 1;
        }
        finder_shutdown_life_check();
    }
}
static void finder_restart_now(void)
{
    uint32_t st;
    serial_puts("FLAIR-RESTART\n");
    st = kernel_reset();
    serial_puts("PANIC kernel_reset: the 8042 pulse did not reset the machine st=");
    serial_puthex32(st);
    serial_puts("\nHALTED\n");
    kernel_halt();
}
static  int finder_power_ask_apps(flair_live_ctx_t *ctx, const boot_info_t *bi)
{
    const char *still;
    int r;
    r = tbx_ask_quit();
    if (r == 0) return 1;
    if (r > 0) flair_live_tenant_heard(ctx, bi);
    still = tbx_resident_name();
    if (still == (const char *)0) return 1;
    serial_puts("FINDER-POWER-CANCEL app=");
    serial_puts(still);
    serial_puts(r > 0 ? " declined\n" : " cannot-ask\n");
    {
        static const char *const ok[] = { "OK" };
        const char *lines[3];
        unsigned n = finder_power_notice(r > 0, still, lines);
        if (n) {
            flair_live_alert_open(ctx, bi, lines, (int)n, ok, 1, 1);
            serial_puts("FINDER-POWER-NOTICE\n");
            for (unsigned i = 0; i < n; i++) {
                serial_puts("FINDER-ALERT-LINE ");
                serial_puts(lines[i]); serial_putc('\n');
            }
            (void)flair_live_alert_run(ctx, bi);
        }
    }
    return 0;
}
void finder_power(flair_live_ctx_t *ctx, const boot_info_t *bi,
                         finder_cmd_id id, int16_t noted)
{
    serial_puts(id == FCMD_RESTART ? "FINDER-POWER name=RESTART views="
                                   : "FINDER-POWER name=SHUTDOWN views=");
    serial_puti((int32_t)noted);
    serial_putc('\n');
    if (!finder_power_ask_apps(ctx, bi))
        return;
    g_finder_db_dirty = 1;
    finder_desk_persist();
    if (id == FCMD_SHUTDOWN && !finder_shutdown_screen(ctx, bi))
        return;
    finder_restart_now();
}
void finder_exit_dos(flair_live_ctx_t *ctx, const boot_info_t *bi,
                            int16_t noted)
{
    static const char *const labels[2] = { "Exit", "Cancel" };
    static char lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
    const char *text[FINDER_ALERT_LINES];
    int n = finder_exit_dos_alert_text(lines), k;
    for (k = 0; k < n; k++) text[k] = lines[k];
    flair_live_alert_open(ctx, bi, text, n, labels, 2, 1);
    serial_puts("FINDER-EXIT-DOS-ALERT\n");
    if (flair_live_alert_run(ctx, bi) != 1u) {
        serial_puts("FINDER-EXIT-DOS-CANCEL\n");
        return;
    }
    serial_puts("FINDER-POWER name=EXITDOS views=");
    serial_puti((int32_t)noted);
    serial_putc('\n');
    if (!finder_power_ask_apps(ctx, bi))
        return;
    g_finder_db_dirty = 1;
    finder_desk_persist();
    g_desktop_exit=1;
}
