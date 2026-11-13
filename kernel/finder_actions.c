/* Ref: ADR-0014 DEC-14-8/-10 lane 2; PRD 6.3/6.5.
 * Cold, user-initiated Finder operations; state and hot polling stay resident.
 * Moved without changing the file-operation or dialog behaviour. */
#include "finder_actions.h"
#include "fat12.h"
#include "tbxgate.h"
#include "toolbox_gate.h"
#include "event.h"
#include "blockdev.h"
#include "desktop_db.h"
#define FINDER_ZOOM_STEPS 8
int finder_fat_mkdir(void *fs_user, const char *name83,
                            uint16_t parent_dir_start)
{
    int rc;

    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MKDIR;

#if defined(KMAIN_MUT_MKDIR_CUT)
    {
        uint16_t c, v = 1u;
        (void)name83;
        (void)parent_dir_start;
        for (c = 2u; c < (uint16_t)(g_finder_vol->total_clusters + 2u); c++)
            if (fat12_next_cluster(g_finder_vol, g_finder_fat,
                                   g_finder_fat_len, c, &v) == FAT12_OK &&
                v == 0u) break;
        if (v != 0u ||
            fat12_set_entry(g_finder_fat, g_finder_fat_len, c,
                            FAT12_EOC_VALUE) != FAT12_OK ||
            fat12_flush_fats(g_finder_vol, g_finder_fat,
                             g_finder_fat_len) != FAT12_OK)
            return (int)FINDER_WIN_ERR_MKDIR;
        return (int)FINDER_WIN_OK;
    }
#endif
    rc = fat12_mkdir(g_finder_vol, g_finder_fat, g_finder_fat_len,
                     name83, parent_dir_start,
                     g_finder_dirbuf, g_finder_clusbuf);
    if (rc == FAT12_OK)             return (int)FINDER_WIN_OK;
    if (rc == FAT12_ERR_DIR_FULL)   return (int)FINDER_WIN_ERR_DIRFULL;
    if (rc == FAT12_ERR_NO_SPACE)   return (int)FINDER_WIN_ERR_DISKFULL;
    return (int)FINDER_WIN_ERR_MKDIR;
}

/* Internal to COLD: this utility is never a resident entry or callback. */
static int finder_fat_map(int rc)
{
    if (rc == FAT12_OK)            return (int)FINDER_WIN_OK;
    if (rc == FAT12_ERR_SAME_DIR)  return (int)FINDER_WIN_ERR_SAMEDIR;
    if (rc == FAT12_ERR_CYCLE)     return (int)FINDER_WIN_ERR_CYCLE;
    if (rc == FAT12_ERR_EXISTS)    return (int)FINDER_WIN_ERR_EXISTS;
    if (rc == FAT12_ERR_NOT_EMPTY) return (int)FINDER_WIN_ERR_NOTEMPTY;
    if (rc == FAT12_ERR_NO_SPACE)  return (int)FINDER_WIN_ERR_DISKFULL;
    return (int)FINDER_WIN_ERR_MOVE;
}

int finder_fat_move(void *fs_user, const char *src_name83,
                           uint16_t src_dir, const char *dst_name83,
                           uint16_t dst_dir)
{
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_move_dirent(g_finder_vol, g_finder_fat,
                                            g_finder_fat_len, src_name83,
                                            src_dir, dst_name83, dst_dir,
                                            g_finder_dirbuf,
                                            g_finder_clusbuf));
}

int finder_fat_trash_name(void *fs_user, const char *name83,
                                 uint16_t dir, char *out)
{
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_trash_suffix_name(g_finder_vol, g_finder_fat,
                                                  g_finder_fat_len, dir,
                                                  g_finder_dirbuf, name83,
                                                  out));
}

int finder_trash_reserve(void)
{
    desktop_trash_state_t state = DESKTOP_TRASH_OK;
    int rc;

    if (!finder_fat_geometry_ok()) return FAT12_ERR_NULL;
    rc = desktop_trash_ensure(g_finder_vol, g_finder_fat, g_finder_fat_len,
                              g_finder_dirbuf, g_finder_clusbuf, &state);
    if (rc == FAT12_OK) {
        if (state == DESKTOP_TRASH_CREATE) serial_puts("TRASH-CREATE\n");
    } else {
        serial_puts("TRASH-RESERVE-FAIL rc=");
        serial_puti((int32_t)rc);
        serial_putc('\n');
    }
    return rc;
}

int finder_fat_unlink(void *fs_user, const char *name83, uint16_t dir)
{
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_unlink(g_finder_vol, g_finder_fat,
                                       g_finder_fat_len, name83, dir,
                                       g_finder_dirbuf));
}

int finder_fat_rmdir(void *fs_user, const char *name83, uint16_t dir)
{
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_rmdir(g_finder_vol, g_finder_fat,
                                      g_finder_fat_len, name83, dir,
                                      g_finder_dirbuf));
}

int finder_fat_create(void *fs_user, const char *name83, uint16_t dir,
                             uint32_t *out_slot)
{
    dir_entry_t e;
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_create(g_finder_vol, g_finder_fat,
                                       g_finder_fat_len, name83,
                                       (uint8_t)DIR_ATTR_ARCHIVE, dir,
                                       g_finder_dirbuf, g_finder_clusbuf, &e,
                                       out_slot));
}

int finder_fat_dir_ident(void *fs_user, uint16_t dir, uint16_t parent,
                                const char *name83)
{
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_STALE;
    return fat12_dir_identity(g_finder_vol, g_finder_fat, g_finder_fat_len,
                              dir, parent, name83, g_finder_dirbuf) == FAT12_OK
         ? (int)FINDER_WIN_OK : (int)FINDER_WIN_ERR_STALE;
}

int finder_fat_read_partial(void *fs_user, const char *name83,
                                   uint16_t dir, uint32_t offset,
                                   uint32_t len, uint8_t *out,
                                   uint32_t *out_read)
{
    dir_entry_t e;
    uint32_t slot = 0u;
    int rc;

    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    rc = fat12_find_slot_in(g_finder_vol, g_finder_fat, g_finder_fat_len, dir,
                            g_finder_dirbuf, name83, &e, &slot);
    if (rc != FAT12_OK) return finder_fat_map(rc);
    return finder_fat_map(fat12_read_partial(g_finder_vol, g_finder_fat,
                                             g_finder_fat_len, &e, offset,
                                             len, out, g_finder_clusbuf,
                                             out_read));
}

int finder_fat_write_partial(void *fs_user, uint16_t dir, uint32_t slot,
                                    uint32_t offset, const uint8_t *data,
                                    uint32_t len, uint32_t *out_written)
{
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_write_partial(g_finder_vol, g_finder_fat,
                                              g_finder_fat_len, dir, slot,
                                              offset, data, len,
                                              g_finder_dirbuf,
                                              g_finder_clusbuf, out_written));
}

static int16_t finder_lerp16(int16_t a, int16_t b, int k)
{
    return (int16_t)((int)a + (((int)b - (int)a) * k) / FINDER_ZOOM_STEPS);
}

void finder_zoom_back(flair_live_ctx_t *ctx, const boot_info_t *bi,
                             rgn_rect_t from, rgn_rect_t home)
{
    for (int k = 1; k <= FINDER_ZOOM_STEPS; k++) {
        rgn_rect_t r;
        uint32_t t0;

        r.left   = finder_lerp16(from.left, home.left, k);
        r.top    = finder_lerp16(from.top, home.top, k);
        r.right  = finder_lerp16(from.right, home.right, k);
        r.bottom = finder_lerp16(from.bottom, home.bottom, k);
        flair_live_outline_draw(&ctx->off, r);
        flair_desktop_present(bi, &ctx->off);
        t0 = flair_tick_count();
        while (flair_tick_count() == t0) { __asm__ __volatile__("pause"); }
        flair_live_outline_restore(&ctx->off);
    }
    flair_desktop_present(bi, &ctx->off);
}

void finder_delete_now(flair_live_ctx_t *ctx, const boot_info_t *bi,
                              const finder_surface_t *s, int idx)
{
    char name[FINDER_DESK_NAME_MAX];
    uint16_t purged = 0u;
    uint8_t dirty = 0u;
    finder_win_status_t st;
    int k;

    for (k = 0; k + 1 < FINDER_DESK_NAME_MAX && s->fd->icons[idx].name[k]; k++)
        name[k] = s->fd->icons[idx].name[k];
    name[k] = '\0';
    if (!finder_delete_now_confirm(ctx, bi, name)) {
        serial_puts("FINDER-DELETE-CANCEL name=");
        serial_puts(name);
        serial_putc('\n');
        return;
    }
    st = finder_shell_delete_item(g_finder_shell, s->slot, idx, &purged, &dirty);
    finder_desk_repaint(ctx, bi);
    if (st != FINDER_WIN_OK) {
        serial_puts("FINDER-DELETE-FAIL rc=");
        serial_puti((int32_t)st);
        serial_puts(" name=");
        serial_puts(name);
        serial_putc('\n');
        if (st == FINDER_WIN_ERR_LOCKED) (void)finder_trash_confirm(ctx, bi, 0u);
        else (void)finder_notice(ctx, bi, st, name);
    } else {
        serial_puts("FINDER-DELETE-NOW name=");
        serial_puts(name);
        serial_puts(" purged=");
        serial_putu((uint32_t)purged);
        serial_putc('\n');
    }
    if (purged != 0u) (void)finder_trash_reserve();
    (void)finder_shell_recount_trash(g_finder_shell);
    if (dirty) { g_finder_db_dirty = 1; finder_desk_persist(); }
}

int finder_surface_drop(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               const finder_surface_t *s, int idx,
                               int16_t dh, int16_t dv, flair_point_t where,
                               finder_tgt_t *hl, rgn_rect_t out_rect)
{
    finder_tgt_t t, none;
    finder_move_result_t r;
    int dirty;
    finder_win_status_t status;

    t = finder_ops_resolve(g_finder_shell, s->slot, idx, where.h, where.v);
    none.kind = (uint8_t)FINDER_TGT_NONE; none.slot = -1; none.idx = -1;
    none.dir = 0u;
    dirty = finder_ops_track_hilite(g_finder_shell, hl, &none);
    if (t.kind == (uint8_t)FINDER_TGT_REPOSITION) {
        if (dirty) finder_desk_repaint(ctx, bi);
        return 0;
    }

    if (t.kind == (uint8_t)FINDER_TGT_NONE) {
        if (dirty) finder_desk_repaint(ctx, bi);
        finder_zoom_back(ctx, bi, out_rect,
                         finder_desk_cell_rect(s->fd, idx));
        serial_puts("FINDER-WIN-DRAG-REVERT win=");
        serial_puti((int32_t)s->slot);
        serial_puts(" name=");
        serial_puts(s->fd->icons[idx].name);
        serial_putc('\n');
        return 1;
    }

    status=finder_ops_drop(g_finder_shell, s->slot, idx, &t,
                        (int16_t)(s->fd->icons[idx].x + dh),
                        (int16_t)(s->fd->icons[idx].y + dv), &r);
    /* bead initech-vnl4 (audit 2026-10-23 desktop L001): no \TRASH. Make it
     * if a cluster is free and stage as usual; on a FULL volume offer Delete
     * Now -- the only Finder route that frees space there. */
    if (status == FINDER_WIN_ERR_NOTRASH) {
        int rrc = finder_trash_reserve();
        if (rrc == FAT12_OK) {
            status=finder_ops_drop(g_finder_shell, s->slot, idx, &t,
                                (int16_t)(s->fd->icons[idx].x + dh),
                                (int16_t)(s->fd->icons[idx].y + dv), &r);
        } else if (rrc == FAT12_ERR_NO_SPACE || rrc == FAT12_ERR_DIR_FULL) {
#if !defined(KMAIN_MUT_NO_DELETE_NOW)
            if (dirty) finder_desk_repaint(ctx, bi);
            finder_zoom_back(ctx, bi, out_rect,
                             finder_desk_cell_rect(s->fd, idx));
            finder_delete_now(ctx, bi, s, idx);
            return 1;
#else
            /* MUTANT KMAIN_MUT_NO_DELETE_NOW (Rule 6; test-flair-trash-
             * edges): the pre-fix dead end -- "could not be moved" and
             * nothing else. NEVER in a real build. */
            (void)finder_delete_now;
            r.status = status = FINDER_WIN_ERR_MOVE;
#endif
        } else {
            r.status = status = FINDER_WIN_ERR_STORAGE;  /* not "no room" */
        }
    }
    if(status==FINDER_WIN_ERR_EXISTS && finder_notice(ctx,bi,status,r.name)==1u)
        status=finder_ops_replace(g_finder_shell,s->slot,idx,&t,
                                  (int16_t)(s->fd->icons[idx].x+dh),
                                  (int16_t)(s->fd->icons[idx].y+dv),&r);
    if(status!=FINDER_WIN_OK) {
        if (dirty) finder_desk_repaint(ctx, bi);
        finder_zoom_back(ctx, bi, out_rect,
                         finder_desk_cell_rect(s->fd, idx));
        serial_puts("FINDER-MOVE-REFUSED reason=");
        serial_puts(finder_ops_reason(r.status));
        serial_puts(" name=");
        serial_puts(r.name);
        serial_putc('\n');
        if(r.db_dirty) { g_finder_db_dirty=1;finder_desk_persist(); }
        if (r.status == FINDER_WIN_ERR_LOCKED)
            (void)finder_trash_confirm(ctx, bi, 0u);
        else if(r.status!=FINDER_WIN_ERR_EXISTS)
            (void)finder_notice(ctx,bi,r.status,r.name);
        return 1;
    }

    finder_desk_repaint(ctx, bi);
    if (r.op == (uint8_t)FINDER_OP_TRASH) {
        if (r.renamed) {
            serial_puts("TRASH-RENAME from=");
            serial_puts(r.name);
            serial_puts(" to=");
            serial_puts(r.as);
            serial_putc('\n');
        }
        serial_puts("FINDER-TRASH name=");
        serial_puts(r.as);
        serial_puts(" origin=");
        serial_putu((uint32_t)r.from_dir);
        serial_putc('\n');
        if (r.origin_lost) {
            serial_puts("FINDER-TRASH-ORIGIN-FULL name=");
            serial_puts(r.as);
            serial_putc('\n');
        }
    } else {
        serial_puts("FINDER-MOVE name=");
        serial_puts(r.name);
        serial_puts(" from=");
        serial_putu((uint32_t)r.from_dir);
        serial_puts(" to=");
        serial_putu((uint32_t)r.to_dir);
        serial_putc('\n');
    }
    if (r.dst_slot >= 0) finder_report_dropped((int)r.dst_slot);
    if (r.db_dirty) {
        g_finder_db_dirty = 1;
        finder_desk_persist();
    }
    return 1;
}

uint16_t finder_notice(flair_live_ctx_t *ctx, const boot_info_t *bi,
                              finder_win_status_t st, const char *name)
{

    static const char *const labels[]={"OK"};
    static const char *const replace[]={"Replace","Cancel"};
    const char *text[FINDER_ALERT_LINES];
    int n=finder_error_alert_text(st,name,g_finder_notice_lines);
    if(!n) return 2u;
    for(int i=0;i<n;i++) text[i]=g_finder_notice_lines[i];
    flair_live_alert_open(ctx,bi,text,n,
                          st==FINDER_WIN_ERR_EXISTS?replace:labels,
                          st==FINDER_WIN_ERR_EXISTS?2:1,1);
    serial_puts("FINDER-ALERT reason=");
    serial_puts(st==FINDER_WIN_ERR_FULL?"windows":
                st==FINDER_WIN_ERR_MEMORY?"memory":
                st==FINDER_WIN_ERR_DISKFULL?"disk-full":
                st==FINDER_WIN_ERR_LAUNCH?"launch":
                st==FINDER_WIN_ERR_CRASH?"crash":
                st==FINDER_WIN_ERR_EXISTS?"replace":
                st==FINDER_WIN_ERR_NOTRASH?"notrash":"move");
    serial_putc('\n');
    for(int i=0;i<n;i++) {serial_puts("FINDER-ALERT-LINE "); serial_puts(text[i]);serial_putc('\n');}
    return flair_live_alert_run(ctx,bi);
}

int finder_trash_confirm(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                uint16_t items)
{
    /* A zero item count selects the locked-file notice, with only OK.
     * Behaviour: Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf
     * pp. 83, 88, 112. Geometry: system7-decomp s8_alert_modal.png, above.
     * Wording authored, no local reference: the corpus has no locked-file
     * alert capture/string. L118. This notice never authorizes a deletion. */
    static const char *const locked_text[3] = {
        "The item is locked.", "Unlock it before", "discarding it."
    };
    static const char *const labels[2] = { "OK", "Cancel" };

    const char *text[FINDER_ALERT_LINES];
    int n, k, draw = 1;
    int locked = items == 0u;

    n = locked ? 3 : finder_trash_alert_text(items, g_finder_shell->trash_bytes, g_finder_trash_confirm_lines);
    for (k = 0; k < n; k++) {
        if (locked) {
            int j = 0;
            do {
                g_finder_trash_confirm_lines[k][j] = locked_text[k][j];
            } while (locked_text[k][j++]);
        }
        /* DialogItem.text lives in resident state while the modal loop runs.
         * Keep even the locked notice's text in its resident buffer. */
        text[k] = g_finder_trash_confirm_lines[k];
    }
#ifdef KMAIN_MUT_LOCKED_NOTICE
    /* Keep the marker and protected disk: only the independent pixel oracle
     * may certify that the notice was actually visible (Rule 6 / Law 2). */
    draw = !locked;
#endif
    /* tdnl.87: the Empty Trash confirm is the captured CAUTION alert; the
     * locked-file notice has no capture, so it keeps no icon. */
    flair_live_alert_open_icon(ctx, bi, text, n, labels, locked ? 1 : 2, draw,
                               locked ? FTA_NO_ICON : DIALOG_ICON_ID_CAUTION);
    if (locked) {
        serial_puts("FINDER-LOCKED-ALERT\n");
    } else {
        serial_puts("FINDER-TRASH-ALERT n=");
        serial_putu((uint32_t)items);
        serial_puts(" k=");
        serial_putu((g_finder_shell->trash_bytes + 1023u) / 1024u);
        serial_putc('\n');
    }
    return flair_live_alert_run(ctx, bi) == 1u;
}

int finder_delete_now_confirm(flair_live_ctx_t *ctx, const boot_info_t *bi,
                                     const char *name)
{
    static const char *const labels[2] = { "OK", "Cancel" };

    const char *text[FINDER_ALERT_LINES];
    int n = finder_delete_now_alert_text(name, g_finder_delete_now_confirm_lines);

    for (int k = 0; k < n; k++) text[k] = g_finder_delete_now_confirm_lines[k];
    flair_live_alert_open_icon(ctx, bi, text, n, labels, 2, 1,
                               DIALOG_ICON_ID_CAUTION);
    for (int k = 0; k < n; k++) {
        serial_puts("FINDER-ALERT-LINE ");
        serial_puts(text[k]);
        serial_putc('\n');
    }
    serial_puts("FINDER-DELETE-ALERT name=");
    serial_puts(name);
    serial_putc('\n');
    return flair_live_alert_run(ctx, bi) == 1u;
}

void finder_empty_trash(flair_live_ctx_t *ctx, const boot_info_t *bi,
                               uint16_t items)
{
    uint16_t purged = 0u, refused = 0u;
    uint8_t  dirty = 0u;

    if (!finder_trash_confirm(ctx, bi, items)) {
        serial_puts("FINDER-TRASH-EMPTY-CANCEL\n");
        return;
    }
    finder_win_status_t status = finder_shell_empty_trash(g_finder_shell, &purged, &refused, &dirty);
    finder_desk_repaint(ctx, bi);
    serial_puts("FINDER-TRASH-EMPTIED purged=");
    serial_putu((uint32_t)purged);
    serial_puts(" refused=");
    serial_putu((uint32_t)refused);
    serial_putc('\n');
    /* bead initech-uios (audit L002): every view of a removed folder was
     * disposed by the purge (finder_windows.h Sec 12) -- say which. */
    for (int s = 0; s < g_finder_shell->window_cap; s++) {
        if ((g_finder_shell->purge_closed & (1u << s)) == 0u) continue;
        serial_puts("FINDER-TRASH-VIEW-CLOSED win=");
        serial_puti((int32_t)s);
        serial_putc('\n');
    }
    if (dirty) {
        g_finder_db_dirty = 1;
        finder_desk_persist();
    }
    if (status == FINDER_WIN_ERR_LOCKED)
        (void)finder_trash_confirm(ctx, bi, 0u);
}

void finder_filecopy_run(flair_live_ctx_t *ctx, const boot_info_t *bi,
                         const char *cap, const char *src, const char *dst)
{
    uint32_t size = 0u, t0, dt = 0u, v = 0u, shown = 101u, limit;
    int32_t rate, rc, done = 0;
    int mid = 0;
#ifdef FILECOPY_RECORD_STEP
    /* Record images ONLY (make record-flair, FLAIRTENANTS_RECORDXLONGDBL_IMG):
     * the bar advances a fixed FILECOPY_RECORD_STEP ticks per injected input
     * event (a mouse move), never by the clock -- per-event clip frames are
     * grabbed on a wall-clock settle and a clock-driven bar raced them
     * (record-flair-repro file_copy, 2026-10-17). The shipped kernel copies
     * by the tick at the MICHAEL.CFG rate. */
    uint32_t fc_steps = 0u;
    int16_t fc_h = 0, fc_v = 0;
    int fc_have = 0;
#endif


    rate = tbx_fc_rate();
    serial_puts("TENANT-FILECOPY s=\"");
    serial_puts(cap);
    serial_puts("\" src=");
    serial_puts(src);
    serial_puts(" dst=");
    serial_puts(dst);
    serial_puts(" rate=");
    serial_puti(rate);
    serial_putc('\n');
    rc = rate < 0 ? rate : tbx_fc_open(&size);
    if (rc < 0) {
        serial_puts("TENANT-FILECOPY-REFUSED rc=");
        serial_puti(rc);
        serial_putc('\n');
        tbx_filecopy_answer(rc);
        flair_live_tenant_service(ctx, bi);
        return;
    }
    fta_rgn(&g_fc_rs); fta_rgn(&g_fc_rc); fta_rgn(&g_fc_ru);
    (void)FileCopyDialog(&g_fc_dr, g_fc_it, &g_fc_bar, &g_fc_rs.r, &g_fc_rc.r, &g_fc_ru.r);
    g_fc_it[0].text = cap;                     /* the tenant's statText line */
    g_fc_bar.contrlValue = 0;
    g_fc_dr.window.port.portBits.bm = ctx->off;
    g_fc_dr.window.port.visRgn  = (region_t *)0;
    g_fc_dr.window.port.clipRgn = (region_t *)0;
    g_fc_dr.window.port.portRect = region_get_bbox(g_fc_dr.window.strucRgn);
    serial_puts("TENANT-FILECOPY up bytes=");
    serial_putu(size);
    serial_putc('\n');
    limit = 2u * (10000u + (uint32_t)rate - 1u) / (uint32_t)rate + 100u;
    t0 = flair_tick_count();
    for (;;) {
        EventRecord ev;
        int got = WaitNextEvent(&g_flair_kbd_ring, everyEvent, &ev, 1u);
        if (got)
            flair_live_emit_evt(&ev);          /* absorbed: the box is modal */
        flair_live_cursor_track(&ev);
        dt = flair_tick_count() - t0;
#ifdef FILECOPY_RECORD_STEP
        if (!fc_have) { fc_have = 1; fc_h = ev.where.h; fc_v = ev.where.v; }
        else if (got || ev.where.h != fc_h || ev.where.v != fc_v) {
            fc_steps++; fc_h = ev.where.h; fc_v = ev.where.v;
        }
        dt = fc_steps * (uint32_t)FILECOPY_RECORD_STEP;
#endif
        v = (uint32_t)rate * dt / 100u;
        if (v > 100u) v = 100u;
        /* size * v / 100 without a 64-bit divide (freestanding, no libgcc). */
        done = tbx_fc_step((size / 100u) * v + (size % 100u) * v / 100u);
        if (done < 0) break;
        if (v != shown) {
            shown = v;
            g_fc_bar.contrlValue = (int16_t)v;
            DrawDialog(&g_fc_dr);
            flair_desktop_present(bi, &ctx->off);
        }
        if (!mid && v >= 50u) {
            mid = 1;
            serial_puts("TENANT-FILECOPY-MID v=");
            serial_putu(v);
            serial_puts(" ticks=");
            serial_putu(dt);
            serial_putc('\n');
        }
        if (v >= 100u && (uint32_t)done >= size) break;
        if (dt > limit) { serial_puts("FILECOPY-STALL\n"); done = TBX_ERR_BUSY; break; }
    }
    tbx_fc_close();
    serial_puts("TENANT-FILECOPY-DONE rc=");
    serial_puti(done);
    serial_puts(" ticks=");
    serial_putu(dt);
    serial_putc('\n');
    {
        rgn_rect_t box = region_get_bbox(g_fc_dr.window.strucRgn);
        DisposeDialog(&g_fc_dr, (WindowMgr *)0);
        WindowMgr_invalidate_desktop(ctx->wm, box);
    }
    finder_desk_repaint(ctx, bi);
    tbx_filecopy_answer(done);
    flair_live_tenant_service(ctx, bi);
}

void finder_outcome_run(flair_live_ctx_t *ctx, const boot_info_t *bi,
                        const finder_cmd_outcome_t *o)
{
    switch (o->id) {
    case FCMD_NEW_FOLDER:
        if (o->status == FINDER_WIN_OK) {
            finder_win_invalidate_all(g_finder_shell, o->slot);
            finder_desk_repaint(ctx, bi);
            serial_puts("FINDER-NEW-FOLDER name=");
            serial_puts(o->name83);
            serial_puts(" parent=");
            serial_putu((uint32_t)o->parent);
            serial_putc('\n');
            finder_report_dropped(o->slot);
        } else if (o->status == FINDER_WIN_ERR_STALE) {
            /* bead initech-uios (audit L002): the front view's folder no
             * longer exists as that folder -- nothing was written. */
            serial_puts("FINDER-NEW-FOLDER-STALE parent=");
            serial_putu((uint32_t)o->parent);
            serial_putc('\n');
        } else if (o->status == FINDER_WIN_ERR_DIRFULL) {
            /* The fixed root cannot grow (design F3.1). The period ALERT is the
             * tdnl.11 slice; until then this is loud on serial and NEVER
             * silent (Rule 2). */
            serial_puts("FINDER-DIR-FULL parent=");
            serial_putu((uint32_t)o->parent);
            serial_putc('\n');
        } else {
            serial_puts("FINDER-NEW-FOLDER-FAIL rc=");
            serial_puti((int32_t)o->status);
            serial_putc('\n');
        }
        if(o->status!=FINDER_WIN_OK)
            (void)finder_notice(ctx,bi,o->status==FINDER_WIN_ERR_DIRFULL?
                               FINDER_WIN_ERR_DISKFULL:o->status,o->name83);
        return;

    case FCMD_CLEANUP:
        if (o->status != FINDER_WIN_OK) return;
        if (o->moved != 0) {
            finder_win_invalidate_all(g_finder_shell, o->slot);
            finder_desk_repaint(ctx, bi);
        }
        serial_puts("FINDER-CLEANUP win=");
        serial_puti((int32_t)o->slot);
        serial_puts(" moved=");
        serial_puti((int32_t)o->moved);
        serial_putc('\n');
        return;

    case FCMD_CLOSE_WINDOW:
        if (o->status != FINDER_WIN_OK) return;
        finder_desk_repaint(ctx, bi);
        serial_puts("FINDER-CLOSE-WINDOW win=");
        serial_puti((int32_t)o->slot);
        serial_putc('\n');
        g_finder_db_dirty = 1;
        finder_desk_persist();
        return;

    case FCMD_SELECT_ALL: {
        /* Bead initech-tdnl.36 (audit F03): every icon of the front disk
         * window, or of the desktop, is now selected -- repaint its cells. */
        finder_surface_t sf;
        if (o->slot >= 0) {
            finder_win_invalidate_all(g_finder_shell, o->slot);
        } else {
            finder_surface_desktop(&sf);
            finder_surface_invalidate_all(&sf, ctx);
        }
        finder_desk_repaint(ctx, bi);
        serial_puts("FINDER-SELECT-ALL win=");
        serial_puti((int32_t)o->slot);
        serial_puts(" n=");
        serial_puti((int32_t)o->moved);
        serial_putc('\n');
        return;
    }

    case FCMD_ARRANGE_BY_NAME:
        /* Bead initech-tdnl.67 (audit G10). */
        if (o->status != FINDER_WIN_OK) return;
        if (o->moved != 0) {
            finder_win_invalidate_all(g_finder_shell, o->slot);
            finder_desk_repaint(ctx, bi);
        }
        serial_puts("FINDER-ARRANGE win=");
        serial_puti((int32_t)o->slot);
        serial_puts(" moved=");
        serial_puti((int32_t)o->moved);
        serial_putc('\n');
        return;

    case FCMD_VIEW_ICONS:
    case FCMD_VIEW_SMALL:
    case FCMD_VIEW_NAME:
    case FCMD_VIEW_SIZE:
    case FCMD_VIEW_KIND:
    case FCMD_VIEW_DATE:
        if (o->status != FINDER_WIN_OK) return;
        finder_win_invalidate_all(g_finder_shell, o->slot);
        finder_desk_repaint(ctx, bi);
        finder_report_view(o->slot);
        return;

    case FCMD_EMPTY_TRASH:
        /* bead initech-6k12: the shell counted; confirm, then purge. */
        if (o->status != FINDER_WIN_OK) return;
        finder_empty_trash(ctx, bi, (uint16_t)o->moved);
        return;

    case FCMD_PUT_AWAY:
        finder_desk_repaint(ctx,bi);
        if(o->moved) { g_finder_db_dirty=1;finder_desk_persist(); }
        serial_puts("FINDER-PUT-AWAY n=");serial_puti(o->moved);serial_putc('\n');
        if(o->status!=FINDER_WIN_OK) (void)finder_notice(ctx,bi,o->status,"");
        return;

    case FCMD_RESTART:
    case FCMD_SHUTDOWN:
        /* bead initech-tdnl.78: put everything away, then reset / shut down. */
        finder_power(ctx, bi, o->id, o->moved);
        return;

    case FCMD_EXIT_DOS:
        /* bead initech-3p1a: confirm, put everything away, hand the console
         * to COMMAND.COM. Returns only on Cancel / an application's cancel. */
        finder_exit_dos(ctx, bi, o->moved);
        return;

    case FCMD_OPEN: {
        /* bead initech-fbo4 (audit 2026-10-23 desktop L004): the outcome may
         * NAME an application to launch. Copy it out first -- the launch verb
         * services the tenant, and the shell's `last` is not ours after that. */
        uint16_t launch = o->launch;
        uint16_t ldir   = o->launch_dir;
        char     l83[FINDER_DESK_NAME_MAX];
        for (int i = 0; i < FINDER_DESK_NAME_MAX; i++) l83[i] = o->launch83[i];
        l83[FINDER_DESK_NAME_MAX - 1] = '\0';
        serial_puts("FINDER-OPEN-COUNT n=");serial_puti(o->moved);serial_putc('\n');
        if (o->status != FINDER_WIN_OK) {
            finder_desk_repaint(ctx,bi);
            (void)finder_notice(ctx,bi,o->status,o->name83);
            return;
        }
        finder_desk_repaint(ctx, bi);
        if (o->moved > 0) {
            serial_puts("FINDER-OPEN-FOLDER name=");
            serial_puts(o->name83);
            serial_puts(" win=");
            serial_puti((int32_t)o->slot);
            serial_puts(" singleton=");
            serial_puti((int32_t)o->singleton);
            serial_putc('\n');
        }
        if (launch) {
            /* THE ADAPTER: File > Open / Cmd-O reach the SAME verb the
             * double-click (finder_surface_open) and the Apple menu use --
             * no second launch implementation (design F4.3). */
            serial_puts("FINDER-OPEN-APP name=");
            serial_puts(l83);
            serial_puts(" dir=");
            serial_puti((int32_t)ldir);
            serial_puts(" n=");
            serial_puti((int32_t)launch);
            serial_putc('\n');
#ifndef KMAIN_MUT_OPEN_NO_LAUNCH
            /* MUTANT KMAIN_MUT_OPEN_NO_LAUNCH (Rule 6): the command runs and
             * is reported, the verb is never called -- Open "works" on serial
             * and launches nothing (test-flair-finder-open-mutant). */
            flair_live_launch_app(ctx, bi, l83, ldir);
#endif
        }
        return;
    }

    default:
        return;
    }
}

void finder_desk_persist(void)
{
    uint32_t len;
    int rc;

    if (!g_finder_db_dirty || g_finder_desk == (finder_desk_t *)0) return;
    g_finder_db_dirty = 0;
    if (g_finder_vol == (const fat12_volume_t *)0) {
        serial_puts("DESKTOP-DB-WRITE-FAIL rc=0 no-volume\n");
        return;
    }
    /* Desktop icon records THEN the kind=4 per-folder view records (design
     * F1.3 / F3.3). With no window ever opened the view set is empty and the
     * image is byte-identical to R3.2's -- which is exactly why the locked
     * 56-byte on-disk assertion of test-flair-desktop-icons leg 7 still holds. */
    /* ... THEN the kind=5 trash-origin records (bead initech-34dh, design
     * F1.5). With nothing ever trashed the set is empty and the bytes are
     * exactly the R3.3 image. */
    len = finder_desk_db_encode_full(g_finder_desk,
                                     (g_finder_shell != (finder_shell_t *)0)
                                         ? g_finder_shell->views
                                         : (const finder_view_rec_t *)0,
                                     (g_finder_shell != (finder_shell_t *)0)
                                         ? g_finder_shell->n_views : 0u,
                                     (g_finder_shell != (finder_shell_t *)0)
                                         ? g_finder_shell->origins
                                         : (const finder_origin_rec_t *)0,
                                     (g_finder_shell != (finder_shell_t *)0)
                                         ? g_finder_shell->n_origins : 0u,
                                     g_finder_db_buf,
                                     (uint32_t)sizeof g_finder_db_buf);
    if (len == 0u) {
        /* The buffer is sized from the SAME fixed cap the record array is, so a
         * zero here is a programming error, not a disk condition (Rule 2). */
        serial_puts("PANIC finder-desktop: DESKTOP.DB encode overflow\nHALTED\n");
        for (;;) { __asm__ __volatile__("cli; hlt"); }
    }
    rc = desktop_db_write(g_finder_vol, g_finder_fat, g_finder_fat_len,
                          g_finder_secbuf, g_finder_clusbuf,
                          g_finder_db_buf, len);
    if (rc == FAT12_OK) {
        serial_puts("DESKTOP-DB-SAVE n=");
        serial_putu((uint32_t)((len - FINDER_DB_HEADER_SIZE) /
                               FINDER_DB_RECORD_SIZE));
        serial_putc('\n');
    } else {
        g_finder_io_error=rc==FAT12_ERR_NO_SPACE || rc==FAT12_ERR_DIR_FULL?
                            FINDER_WIN_ERR_DISKFULL:FINDER_WIN_ERR_STORAGE;
        serial_puts("DESKTOP-DB-WRITE-FAIL rc=");
        serial_puti((int32_t)rc);
        serial_putc('\n');
    }
}
