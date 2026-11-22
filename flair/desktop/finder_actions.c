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
#include "filecopy_cancel.h"
#include "cursor.h"
#include "volume_db.h"
#include "fileio_fat.h"
#include "finder_storage.h"
#define FINDER_ZOOM_STEPS 8
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
            if (dirty) finder_desk_repaint(ctx, bi);
            finder_zoom_back(ctx, bi, out_rect,
                             finder_desk_cell_rect(s->fd, idx));
            finder_delete_now(ctx, bi, s, idx);
            return 1;
        } else {
            r.status = status = rrc == FAT12_ERR_ACCESS ?
                FINDER_WIN_ERR_WRITEPROTECT : FINDER_WIN_ERR_STORAGE;
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
    int recovery=st==FINDER_WIN_ERR_STORAGE && name && name==desktop_db_recovery();
    int n=recovery?desktop_db_recovery_alert(text):finder_error_alert_text(st,name,g_finder_notice_lines);
    if(!n) return 2u;
    if(!recovery) for(int i=0;i<n;i++) text[i]=g_finder_notice_lines[i];
    flair_live_alert_open(ctx,bi,text,n,
                          st==FINDER_WIN_ERR_EXISTS?replace:labels,
                          st==FINDER_WIN_ERR_EXISTS?2:1,1);
    serial_puts("FINDER-ALERT reason=");
    serial_puts(st==FINDER_WIN_ERR_FULL?"windows":
                st==FINDER_WIN_ERR_WRITEPROTECT?"write-protect":
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
    else if (status != FINDER_WIN_OK)
        (void)finder_notice(ctx, bi, status, DESKTOP_TRASH_NAME);
}

void finder_filecopy_run(flair_live_ctx_t *ctx, const boot_info_t *bi,
                         const char *cap, const char *src, const char *dst)
{
    uint32_t size = 0u, t0, dt = 0u, v = 0u, shown = 101u, limit;
    int32_t rate, rc, done = 0;
    int mid = 0;
    static DialogItem live_items[3];
    static ControlRecord live_controls[2];
    int was_busy=flair_cursor_is_busy();
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


    flair_cursor_set_busy(1);
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
        flair_cursor_set_busy(was_busy);
        serial_puts("TENANT-FILECOPY-REFUSED rc=");
        serial_puti(rc);
        serial_putc('\n');
        tbx_filecopy_answer(rc);
        flair_live_tenant_service(ctx, bi);
        return;
    }
    fta_rgn(&g_fc_rs); fta_rgn(&g_fc_rc); fta_rgn(&g_fc_ru);
    (void)FileCopyCancelableDialog(&g_fc_dr,live_items,live_controls,
                                  &g_fc_rs.r,&g_fc_rc.r,&g_fc_ru.r);
    live_items[0].text = cap;
    live_controls[0].contrlValue = 0;
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
        /* GM3: Escape or Command-period cancels the same staged transfer
         * used by the live frame, before this tick can commit its last byte. */
        if (got && flair_filecopy_cancel(ev.what,ev.message,ev.modifiers)) {
            done = TBX_ERR_BUSY;
            break;
        }
        uint16_t item=0;
        if(DialogHandleEvent(&g_fc_dr,&ev,0,&item) && item==g_fc_dr.cancelItem) {
            done=TBX_ERR_BUSY;
            break;
        }
        if(got) flair_desktop_present(bi,&ctx->off);
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
            live_controls[0].contrlValue = (int16_t)v;
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
        if (v >= 100u && (uint32_t)done >= size) {
            done = tbx_fc_commit();
            break;
        }
        if (dt > limit) { serial_puts("FILECOPY-STALL\n"); done = TBX_ERR_BUSY; break; }
    }
    tbx_fc_close();
    flair_cursor_set_busy(was_busy);
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

typedef struct { flair_live_ctx_t *ctx; const boot_info_t *bi; } finder_viewer_host_t;
static uint16_t finder_clipboard_dialog(void *user,const char *const *text,
                                       const char *const *buttons,unsigned n)
{
    finder_viewer_host_t *host=user;
    for(int i=0;i<4;i++){serial_puts("FINDER-CLIPBOARD ");serial_puts(text[i]);serial_putc('\n');}
    flair_live_alert_open(host->ctx,host->bi,text,4,buttons,(int)n,1);
    g_al_dr.cancelItem=1; /* Escape closes even on a paged snapshot. */
    return flair_live_alert_run(host->ctx,host->bi);
}
void finder_outcome_run(flair_live_ctx_t *ctx, const boot_info_t *bi,
                        const finder_cmd_outcome_t *o)
{
    switch (o->id) {
    case FCMD_GET_INFO: {
        char lines[4][64];const char *text[4];const char *buttons[]={"OK"};
        finder_file_info_lines(&g_finder_shell->info,lines);
        for(int i=0;i<4;i++){text[i]=lines[i];serial_puts("FINDER-INFO ");serial_puts(lines[i]);serial_putc('\n');}
        flair_live_alert_open(ctx,bi,text,4,buttons,1,1);
        (void)flair_live_alert_run(ctx,bi);
        break;
    }
    case FCMD_SHOW_CLIPBOARD: {
        static finder_clipboard_t snapshot;
        finder_viewer_host_t host={ctx,bi};
        if(finder_clipboard_show(&snapshot,g_finder_shell->scrap,finder_clipboard_dialog,&host)<0){
            const char *error[]={"The Clipboard could not be read."};const char *ok[]={"OK"};
            flair_live_alert_open(ctx,bi,error,1,ok,1,1);(void)flair_live_alert_run(ctx,bi);break;
        }
        break;
    }
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
             * silent (contract). */
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
        finder_dir_id_t ldir   = o->launch_dir;
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
            /* MUTANT KMAIN_MUT_OPEN_NO_LAUNCH (contract): the command runs and
             * is reported, the verb is never called -- Open "works" on serial
             * and launches nothing (test-flair-finder-open-mutant). */
            flair_live_launch_app(ctx, bi, l83, ldir);
        }
        return;
    }

    default:
        return;
    }
}

/* A different volume's view change must not cause writes to an unchanged DB.
 * The comparison is against the existing disk bytes, never a repaint result. */
static uint8_t finder_db_previous[FINDER_DB_MAX_BYTES];
static int finder_db_equal(const uint8_t *a,const uint8_t *b,uint32_t n)
{
    for(uint32_t i=0;i<n;i++)if(a[i]!=b[i])return 0;
    return 1;
}
void finder_desk_persist(void)
{
    if(!g_finder_db_dirty || !g_finder_desk)return;
    g_finder_db_dirty=0;
    unsigned drives[FINDER_VOLUME_SLOTS]={g_finder_drive,0};
    unsigned count=g_finder_drive==0?1:FINDER_VOLUME_SLOTS;
    for(unsigned i=0;i<count;i++){
        unsigned drive=drives[i];
        if(i && !finder_volume_db_has_state(g_finder_shell,drive))continue;
        void *fat=NULL;uint32_t fat_len=0,len=0,previous_len=0;
        const fat12_volume_t *vol=fileio_fat_drive_volume(drive,&fat,&fat_len);
        if(!vol){
            if(i==0){g_finder_io_error=FINDER_WIN_ERR_STORAGE;g_finder_db_dirty=1;}
            continue; /* A is optional. Never substitute the system volume. */
        }
        if(g_finder_shell)len=finder_volume_db_encode(g_finder_shell,drive,
                                    g_finder_db_buf,sizeof g_finder_db_buf);
        else if(i==0)len=finder_desk_db_encode(g_finder_desk,g_finder_db_buf,
                                    sizeof g_finder_db_buf);
        else continue;
        if(!len){
            serial_puts("DESKTOP-DB-WRITE-FAIL invalid-state\n");
            g_finder_io_error=FINDER_WIN_ERR_STORAGE;g_finder_db_dirty=1;continue;
        }
        fat=finder_storage_read_cache(vol,fat);
        int rc=desktop_db_read(vol,fat,fat_len,g_finder_secbuf,g_finder_clusbuf,
                    finder_db_previous,sizeof finder_db_previous,&previous_len);
        if(rc==FAT12_OK && previous_len==len &&
           finder_db_equal(finder_db_previous,g_finder_db_buf,len)){
            serial_puts("DESKTOP-DB-SAVE n=");
            serial_putu((len-FINDER_DB_HEADER_SIZE)/FINDER_DB_RECORD_SIZE);
            serial_puts(" drive=");serial_putc((char)('A'+drive));
            serial_puts(" unchanged\n");continue;
        }
        if(rc==FAT12_OK || rc==FAT12_ERR_NOT_FOUND)
            rc=desktop_db_write(vol,fat,fat_len,g_finder_secbuf,g_finder_clusbuf,
                                  g_finder_db_buf,len);
        if(rc==FAT12_OK){
            serial_puts("DESKTOP-DB-SAVE n=");
            serial_putu((len-FINDER_DB_HEADER_SIZE)/FINDER_DB_RECORD_SIZE);
        }else{
            g_finder_db_dirty=1;
            g_finder_io_error=rc==FAT12_ERR_ACCESS ? FINDER_WIN_ERR_WRITEPROTECT :
                rc==FAT12_ERR_NO_SPACE || rc==FAT12_ERR_DIR_FULL ?
                FINDER_WIN_ERR_DISKFULL:FINDER_WIN_ERR_STORAGE;
            serial_puts("DESKTOP-DB-WRITE-FAIL rc=");serial_puti(rc);
            if(desktop_db_recovery()){
                serial_puts(" previous=");serial_puts(desktop_db_recovery());
            }
        }
        serial_puts(" drive=");serial_putc((char)('A'+drive));serial_putc('\n');
    }
}
