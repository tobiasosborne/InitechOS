#ifndef FLAIR_DESKTOP_INTERNAL_H
#define FLAIR_DESKTOP_INTERNAL_H
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
#include "environment.h"
extern void serial_puts(const char *);
extern void serial_putu(uint32_t);
extern void serial_puthex32(uint32_t);
extern void irq12_entry(void);
extern void tbx_gate_entry(void);

typedef struct flair_live_outline {
    rgn_rect_t rect;
    uint8_t saved[(2u * ((uint32_t)FLAIR_SCREEN_W + (uint32_t)FLAIR_SCREEN_H) + 4u)];
    uint32_t saved_count;
    int drawn;
} flair_live_outline_t;
typedef struct finder_fat_enum {
    finder_enum_cb cb;
    void *user;
} finder_fat_enum_t;
typedef struct sf_geom {
    int16_t w, h;
    int16_t pop_l, pop_r;
    int16_t list_l, rows;
    int16_t btn_l;
    int16_t eject_y, desk_y, new_y, cancel_y, def_y;
} sf_geom_t;
typedef struct sf_walk {
    const char *want;
    uint16_t dir;
    FlairSFEntry *out;
    int16_t n, max;
    uint16_t at;
} sf_walk_t;
typedef struct sf_ui {
    int put, sel, top, l, t;
    const sf_geom_t *g;
    char name[TBX_SF_NAME_MAX];
    const char *prompt;
} sf_ui_t;
extern unsigned g_vga_dac_shift;
extern uint8_t g_volume_sector[512];
extern int g_desktop_exit;
void vga_set_dac(uint8_t i,uint8_t r,uint8_t g,uint8_t b);
extern const char *const fd_sys_titles[4];
void flair_desktop_oom(const char *what);
region_t *flair_desktop_alloc_region(flair_heap_t *h, const char *what);
void flair_boot_screen_row(void *ctx, int y, const uint8_t *row);
void flair_dac_load(void);
void flair_boot_screen(const boot_info_t *bi);
void flair_boot_screen_hold(void);
void flair_desktop_present(const boot_info_t *bi, const bitmap_t *off);
void flair_desktop_run(const boot_info_t *bi, flair_live_ctx_t *ctx_out);
extern flair_raw_ring_t g_flair_kbd_ring;
extern kbd_state_t g_flair_consumed_keys;
void flair_consumed_key(uint8_t sc);
extern volatile uint32_t g_flair_mouse_activity;
extern int g_flair_cursor_visible;
extern int g_flair_cursor_x;
extern int g_flair_cursor_y;
__attribute__((unused))
void flair_live_kbd_post(uint8_t sc);
__attribute__((unused))
void flair_live_mouse_post(int dx, int dy, uint8_t buttons);
void flair_live_yield(void);
void flair_live_emit_evt(const EventRecord *ev);
void flair_live_cursor_track(const EventRecord *ev);
int flair_live_window_index(const flair_live_ctx_t *ctx, WindowPtr w);
void flair_live_content_phase(flair_live_ctx_t *ctx);
__attribute__((noinline)) void flair_live_repaint(flair_live_ctx_t *ctx);
__attribute__((noinline)) void flair_live_repaint_present(flair_live_ctx_t *ctx,
                                                                 const boot_info_t *bi);
void flair_live_refresh_finder_bar(MenuBar *bar);
MenuBar *flair_live_tenant_bar(flair_live_ctx_t *ctx,
                                      FlairProcessList *list);
void flair_live_finish_tenant_switch(flair_live_ctx_t *ctx,
                                            const boot_info_t *bi,
                                            FlairProcessList *list,
                                            FlairApp *prev_fg,
                                            GrafPort *barport);
void flair_live_clamp_drag(const flair_live_ctx_t *ctx, WindowPtr w,
                                  int16_t *dh, int16_t *dv);
extern flair_live_outline_t g_flair_live_outline;
int flair_live_rect_same(rgn_rect_t a, rgn_rect_t b);
void flair_live_outline_pixel(flair_live_outline_t *outline,
                                     bitmap_t *surface,
                                     int x, int y, uint32_t ink,
                                     int restore, uint32_t *index);
void flair_live_outline_walk(flair_live_outline_t *outline,
                                    bitmap_t *surface, uint32_t ink,
                                    int restore);
void flair_live_outline_draw(bitmap_t *surface, rgn_rect_t rect);
void flair_live_outline_restore(bitmap_t *surface);
void flair_live_drag_delta(const flair_live_ctx_t *ctx, WindowPtr w,
                                  flair_point_t where0, flair_point_t where,
                                  int16_t *dh, int16_t *dv);
int flair_live_track_cut(uint32_t t0, int *cancel);
void flair_live_do_drag(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               WindowPtr w, flair_point_t where0);
int flair_live_box_hit(const flair_live_ctx_t *ctx, WindowPtr w,
                              flair_part_code_t pc, flair_point_t pt);
int flair_live_track_box(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                WindowPtr w, flair_part_code_t pc);
void flair_live_scroll_report(const flair_live_ctx_t *ctx, WindowPtr w,
                                     int axis, int part,
                                     const WindowScrollAxis *a);
void flair_live_scroll_repaint(flair_live_ctx_t *ctx,
                                      const boot_info_t *bi, WindowPtr w,
                                      int axis, int content);
void flair_live_track_scroll(flair_live_ctx_t *ctx,
                                    const boot_info_t *bi, WindowPtr w,
                                    flair_point_t where0);
void flair_live_tenant_heard(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_do_close(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                WindowPtr w, GrafPort *tenant_barport);
void flair_live_do_zoom(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               WindowPtr w);
void flair_live_do_collapse(flair_live_ctx_t *ctx,
                                   const boot_info_t *bi, WindowPtr w);
void flair_live_do_grow(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               WindowPtr w, flair_point_t where0);
void flair_live_init_menu_port(GrafPort *port, const bitmap_t *view);
void flair_live_erase_menu_panel(flair_live_ctx_t *ctx,
                                        const MenuBar *bar, int mi,
                                        uint32_t y_top);
void flair_live_apple_refresh(void);
uint32_t flair_live_do_menu_at(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                      MenuBar *bar, flair_point_t where0,
                                      uint32_t y_top);
__attribute__((unused))
void flair_live_do_menu(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               MenuBar *bar, flair_point_t where0);
extern finder_shell_t *g_finder_shell;
extern finder_desk_t *g_finder_desk;
extern finder_click_track_t g_finder_click;
extern int g_finder_db_dirty;
extern FinderCtx g_finder_ctx;
extern FlairProcessList *g_ten_plist;
extern FlairApp *g_ten_finder;
extern GrafPort *g_ten_barport;
extern finder_win_status_t g_finder_io_error;
uint16_t finder_notice(flair_live_ctx_t *ctx, const boot_info_t *bi,
                              finder_win_status_t st, const char *name);
void flair_live_tenant_alert(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_tenant_scrollbar(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_tenant_sf(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_tenant_track(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_tenant_filecopy(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_tenant_service(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_launch_app(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                  const char *name83, uint16_t dir_start);
void flair_live_tenant_heard(flair_live_ctx_t *ctx, const boot_info_t *bi);
__attribute__((unused))
void flair_live_tenant_menu_result(flair_live_ctx_t *ctx,
                                          const boot_info_t *bi,
                                          const MenuBar *bar, uint32_t sel,
                                          const EventRecord *ev);
int flair_live_tenant_key(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                 const EventRecord *ev);
extern const fat12_volume_t *g_finder_vol;
extern void *g_finder_fat;
extern uint32_t g_finder_fat_len;
extern void *g_finder_secbuf;
extern char g_finder_vol_label[12];
extern uint8_t g_finder_db_buf[FINDER_DB_MAX_BYTES];
extern uint8_t g_finder_dirbuf[FINDER_FS_MAX_SPC * BLOCKDEV_SECTOR_SIZE];
extern uint8_t g_finder_clusbuf[FINDER_FS_MAX_SPC * BLOCKDEV_SECTOR_SIZE];
int finder_abs(int v);
void __attribute__((unused)) finder_trace_serial(void *user, const char *line);
int finder_fat_dirent_cb(const dir_entry_t *e, void *user);
int finder_fat_geometry_ok(void);
int finder_fat_enumerate(void *fs_user, uint16_t dir_start,
                                finder_enum_cb cb, void *cb_user);
int finder_fat_trash_dir(void *fs_user);
extern const finder_fs_t g_finder_fs_binding __attribute__((unused));
void __attribute__((unused)) finder_desk_load_positions(void);
void flair_tenant_mount_volume(void);
void finder_desk_sync_ctx(void);
void flair_live_refresh_finder_bar(MenuBar *bar);
int flair_live_finder_is_foreground(void);
int finder_trash_confirm(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                uint16_t items);
int finder_delete_now_confirm(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                     const char *name);
void finder_surface_desktop(finder_surface_t *s);
int finder_surface_window(finder_surface_t *s, int slot);
void finder_surface_invalidate_all(const finder_surface_t *s,
                                          flair_live_ctx_t *ctx);
void finder_surface_invalidate(const finder_surface_t *s,
                                      flair_live_ctx_t *ctx,
                                      rgn_rect_t old_rect, rgn_rect_t new_rect);
void finder_desk_repaint(flair_live_ctx_t *ctx, const boot_info_t *bi);
void finder_report_open_fail(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                     finder_win_status_t st);
void finder_report_dropped(int slot);
void finder_open_volume(flair_live_ctx_t *ctx, const boot_info_t *bi);
void finder_open_folder(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               uint16_t parent, uint16_t dir_start,
                               const char *name83);
void finder_open_trash(flair_live_ctx_t *ctx, const boot_info_t *bi);
void finder_surface_open(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                const finder_surface_t *s, int idx);
void flair_live_do_surface(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                  const EventRecord *ev,
                                  const finder_surface_t *s);
void flair_live_do_desk(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               const EventRecord *ev);
void finder_report_view(int slot);
void flair_live_do_finder_content(flair_live_ctx_t *ctx,
                                         const boot_info_t *bi,
                                         const EventRecord *ev, int slot);
void flair_live_do_finder_close(flair_live_ctx_t *ctx,
                                       const boot_info_t *bi, int slot);
rgn_rect_t fta_rect(int l, int t, int r, int b);
void fta_rgn(finder_win_rgn_t *s);
extern DialogRecord g_al_dr;
extern DialogItem g_al_it[3 + FINDER_ALERT_LINES + 1];
extern ControlRecord g_al_btn[3];
extern finder_win_rgn_t g_al_rs, g_al_rc, g_al_ru;
void flair_live_alert_open_icon(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                       const char *const *text, int n,
                                       const char *const *btn, int nb, int draw,
                                       unsigned icon);
void flair_live_alert_open(flair_live_ctx_t *ctx, const boot_info_t *bi,
                           const char *const *text, int n,
                           const char *const *btn, int nb, int draw);
uint16_t flair_live_alert_run(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_tenant_alert(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_tenant_scrollbar(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_tenant_track(flair_live_ctx_t *ctx, const boot_info_t *bi);
extern const sf_geom_t g_sf_geom[2];
extern FlairStandardFile g_sf;
extern DialogRecord g_sf_dr;
extern DialogItem g_sf_it[3];
extern ControlRecord g_sf_btn[5];
extern finder_win_rgn_t g_sf_rs, g_sf_rc, g_sf_ru;
int sf_upper(int c);
int sf_ieq(const char *a, const char *b);
int sf_walk_cb(const finder_dirent_t *d, void *user);
int sf_provider(const char *path, FlairSFEntry *out, int16_t max, void *user);
void sf_fill(flair_live_ctx_t *ctx, int l, int t, int r, int b, int part);
void sf_frame(flair_live_ctx_t *ctx, int l, int t, int r, int b);
void sf_text(flair_live_ctx_t *ctx, int x, int y, const char *s,
                    int fg, int bg);
void sf_dos_path(const char *name, char *out);
const char *sf_vol(void);
void sf_dim_button(flair_live_ctx_t *ctx, ControlRecord *c);
void sf_draw_static(flair_live_ctx_t *ctx, const sf_ui_t *u);
void sf_draw_dynamic(flair_live_ctx_t *ctx, const sf_ui_t *u);
void sf_select(sf_ui_t *u, int i);
void sf_reset_view(sf_ui_t *u);
void sf_report(const char *what, const sf_ui_t *u);
int sf_name_ok(const char *n);
void sf_button(ControlRecord *c, const sf_ui_t *u, int y, const char *t);
int sf_run(flair_live_ctx_t *ctx, const boot_info_t *bi, sf_ui_t *u,
                  char *out);
void flair_live_tenant_sf(flair_live_ctx_t *ctx, const boot_info_t *bi);
extern DialogRecord g_fc_dr;
extern DialogItem g_fc_it[2];
extern ControlRecord g_fc_bar;
extern finder_win_rgn_t g_fc_rs, g_fc_rc, g_fc_ru;
void flair_live_tenant_filecopy(flair_live_ctx_t *ctx, const boot_info_t *bi);
int flair_live_apple_installed(void *user, const char *dir83,
                                      const char *file83);
void flair_live_apple_refresh(void);
int flair_live_apple_result(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                   uint32_t sel);
void finder_report_outcome(flair_live_ctx_t *ctx, const boot_info_t *bi);
void flair_live_sync_foreground(flair_live_ctx_t *ctx,
                                       const boot_info_t *bi,
                                       const EventRecord *ev);
void flair_live_desk_activate(flair_live_ctx_t *ctx,
                                     const boot_info_t *bi,
                                     const EventRecord *ev);
int flair_live_finder_key(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                 const EventRecord *ev);
__attribute__((unused))
void flair_live_finder_menu_result(flair_live_ctx_t *ctx,
                                          const boot_info_t *bi,
                                          const MenuBar *bar, uint32_t sel);
void kernel_graphical_environment(void);
extern char g_finder_notice_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
extern char g_finder_trash_confirm_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
extern char g_finder_delete_now_confirm_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
#endif
