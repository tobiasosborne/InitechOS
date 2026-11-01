/* Ref: ADR-0014 DEC-14-6/-8/-10; PRD 6.3/6.5. Finder operation bodies
 * run in section COLD. State, event polls, disk drivers and alert machinery
 * remain resident. All cross-boundary symbols are generated imports/stubs. */
#ifndef INITECH_FINDER_ACTIONS_H
#define INITECH_FINDER_ACTIONS_H
#include "boot_info.h"
#include "shell.h"
#include "finder_ops.h"
#include "dialog.h"
#include "about_box.h"
#include "fat12.h"
#include "event.h"
#define FINDER_FS_MAX_SPC 8u
#define FTA_NO_ICON 0xFFu
typedef struct flair_live_ctx {
    WindowMgr          *wm;         /* &scene->wm (z-order + damage)             */
    shell_window_store *windows;    /* the document-window store (FindWindow)    */
    int                 n_windows;  /* document window count                     */
    region_t           *comp;       /* compositor visible-region scratch (D-5)   */
    bitmap_t            off;         /* the indexed-8 offscreen (descriptor copy) */
    shell_scene_t      *scene;      /* the whole scene (heap-backed)             */
#ifdef FLAIR_LIVE_TENANTS
    flair_heap_t       *master;     /* the function-static FLAIR heap, exposed so
                                     * the FLAIR_LIVE_TENANTS launch site can carve
                                     * tenant partitions from it (ADR-0013 Sec 3.6).
                                     * Behind the guard so the struct layout stays
                                     * byte-identical for non-tenant builds.       */
    FlairProcessList   *plist;      /* the resident-app list, armed by the launch
                                     * site AFTER the tenants exist (NULL until
                                     * then) so the drag/close dispatches can run
                                     * the DQ2 content phase (flair_route_updates)
                                     * instead of destroying tenant-owed damage
                                     * (initech-gofc). Behind the guard: same
                                     * layout argument as `master`.               */
#endif
} flair_live_ctx_t;
#ifdef FLAIR_LIVE_TENANTS
typedef struct finder_surface {
    finder_desk_t        *fd;      /* the icon model this gesture works on     */
    finder_click_track_t *click;   /* that surface's double-click synthesiser  */
    int                   slot;    /* window slot, or -1 for THE DESKTOP       */
} finder_surface_t;
extern finder_shell_t       *g_finder_shell;
extern finder_desk_t        *g_finder_desk;
extern uint8_t               g_finder_db_buf[FINDER_DB_MAX_BYTES];
extern int                   g_finder_db_dirty;
extern const fat12_volume_t *g_finder_vol;
extern void                 *g_finder_fat;
extern uint32_t              g_finder_fat_len;
extern void                 *g_finder_secbuf;
extern uint8_t g_finder_dirbuf[FINDER_FS_MAX_SPC * BLOCKDEV_SECTOR_SIZE];
extern uint8_t g_finder_clusbuf[FINDER_FS_MAX_SPC * BLOCKDEV_SECTOR_SIZE];
extern finder_win_status_t g_finder_io_error;
extern DialogRecord     g_fc_dr;
extern DialogItem       g_fc_it[2];
extern ControlRecord    g_fc_bar;
extern finder_win_rgn_t g_fc_rs, g_fc_rc, g_fc_ru;
extern flair_raw_ring_t g_flair_kbd_ring;
void flair_live_emit_evt(const EventRecord *);
void flair_live_cursor_track(const EventRecord *);
extern char g_finder_notice_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
extern char g_finder_trash_confirm_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
extern char g_finder_delete_now_confirm_lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
int finder_fat_mkdir(void *fs_user, const char *name83,
                            uint16_t parent_dir_start);
int finder_fat_move(void *fs_user, const char *src_name83,
                           uint16_t src_dir, const char *dst_name83,
                           uint16_t dst_dir);
int finder_fat_trash_name(void *fs_user, const char *name83,
                                 uint16_t dir, char *out);
int finder_trash_reserve(void);
int finder_fat_unlink(void *fs_user, const char *name83, uint16_t dir);
int finder_fat_rmdir(void *fs_user, const char *name83, uint16_t dir);
int finder_fat_create(void *fs_user, const char *name83, uint16_t dir,
                             uint32_t *out_slot);
int finder_fat_dir_ident(void *fs_user, uint16_t dir, uint16_t parent,
                                const char *name83);
int finder_fat_read_partial(void *fs_user, const char *name83,
                                   uint16_t dir, uint32_t offset,
                                   uint32_t len, uint8_t *out,
                                   uint32_t *out_read);
int finder_fat_write_partial(void *fs_user, uint16_t dir, uint32_t slot,
                                    uint32_t offset, const uint8_t *data,
                                    uint32_t len, uint32_t *out_written);
void finder_zoom_back(flair_live_ctx_t *ctx, const boot_info_t *bi,
                             rgn_rect_t from, rgn_rect_t home);
void finder_delete_now(flair_live_ctx_t *ctx, const boot_info_t *bi,
                              const finder_surface_t *s, int idx);
int finder_surface_drop(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               const finder_surface_t *s, int idx,
                               int16_t dh, int16_t dv, flair_point_t where,
                               finder_tgt_t *hl, rgn_rect_t out_rect);
uint16_t finder_notice(flair_live_ctx_t *ctx, const boot_info_t *bi,
                              finder_win_status_t st, const char *name);
int finder_trash_confirm(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                uint16_t items);
int finder_delete_now_confirm(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                     const char *name);
void finder_empty_trash(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               uint16_t items);
void serial_putc(char c);
void serial_puti(int32_t v);
int finder_fat_geometry_ok(void);
void finder_desk_persist(void);
void finder_desk_repaint(flair_live_ctx_t *ctx, const boot_info_t *bi);
void finder_report_dropped(int slot);
void flair_live_outline_draw(bitmap_t *surface, rgn_rect_t rect);
void flair_live_outline_restore(bitmap_t *surface);
void flair_desktop_present(const boot_info_t *bi, const bitmap_t *off);
void flair_live_tenant_service(flair_live_ctx_t *ctx, const boot_info_t *bi);
void fta_rgn(finder_win_rgn_t *s);
void finder_surface_desktop(finder_surface_t *s);
void finder_surface_invalidate_all(const finder_surface_t *s,
                                          flair_live_ctx_t *ctx);
void finder_report_view(int slot);
void finder_power(flair_live_ctx_t *ctx, const boot_info_t *bi,
                         finder_cmd_id id, int16_t noted);
void finder_exit_dos(flair_live_ctx_t *ctx, const boot_info_t *bi,
                            int16_t noted);
void flair_live_launch_app(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                  const char *name83, uint16_t dir_start);
void flair_live_alert_open_icon(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                       const char *const *text, int n,
                                       const char *const *btn, int nb, int draw,
                                       unsigned icon);
void finder_filecopy_run(flair_live_ctx_t *, const boot_info_t *, const char *, const char *, const char *);
void finder_outcome_run(flair_live_ctx_t *, const boot_info_t *, const finder_cmd_outcome_t *);
#endif
#endif
