/* Ref: ADR-0014 DEC-14-8/-10; finder_ops.h refusal ladder and origins.
 * User-initiated move, Replace, Put Away and Trash rename bodies, section COLD.
 * Host suites include this same source; production reaches generated stubs. */
#include "finder_ops_private.h"
static void fo_copy83(char *dst, const char *src)
{
    uint32_t i = 0u;
    if (src != (const char *)0) {
        while (i < (uint32_t)(FINDER_DESK_NAME_MAX - 1) && src[i] != '\0') {
            dst[i] = src[i];
            i++;
        }
    }
    while (i < (uint32_t)FINDER_DESK_NAME_MAX) dst[i++] = '\0';
}

static void fo_zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (uint32_t i = 0u; i < n; i++) b[i] = 0u;
}

static void fo_follow(finder_shell_t *sh,const finder_desk_icon_t *ic,
                       uint16_t parent,const char *name)
{
    int slot;
    if(ic->kind!=FINDER_ICON_FOLDER) return;
    slot=finder_win_find(sh,ic->dir_start);
    if(slot<0 || sh->windows[slot].is_root) return;
    sh->windows[slot].parent=parent;
    fo_copy83(sh->windows[slot].name83,name);
    SetWTitle(sh->wm,&sh->windows[slot].rec,name);
}

static int fo_slot_ok(const finder_shell_t *sh, int slot)
{
    return sh != (const finder_shell_t *)0 &&
           slot >= 0 && slot < sh->window_cap && sh->windows[slot].open;
}

static char fo_cold_upper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

static int fo_cold_ieq(const char *a, const char *b)
{
    if (a == (const char *)0 || b == (const char *)0) return 0;
    while (*a != '\0' && fo_cold_upper(*a) == fo_cold_upper(*b)) { a++; b++; }
    return fo_cold_upper(*a) == fo_cold_upper(*b);
}

static void fo_cold_to_view(const finder_window_t *fw, int16_t h, int16_t v,
                       int16_t *vh, int16_t *vv)
{
    rgn_rect_t c = region_get_bbox(fw->rec.contRgn);
    *vh = (int16_t)(fw->view.bounds.left + (h - c.left));
    *vv = (int16_t)(fw->view.bounds.top  + (v - c.top));
}
const char *finder_ops_reason(finder_win_status_t st)
{
    if (st == FINDER_WIN_ERR_SAMEDIR) return "samedir";
    if (st == FINDER_WIN_ERR_CYCLE)   return "cycle";
    if (st == FINDER_WIN_ERR_EXISTS)  return "exists";
    if (st == FINDER_WIN_ERR_SERVICE) return "service";
    if (st == FINDER_WIN_ERR_LOCKED)  return "locked";
    if (st == FINDER_WIN_ERR_STALE)   return "stale";
    if (st == FINDER_WIN_ERR_NOTRASH) return "notrash";
    return "err";
}

static finder_win_status_t fo_refuse(finder_move_result_t *out,
                                     finder_win_status_t st)
{
    out->status = st;
    return st;
}

/* The icon can predate a lock. Query the binding at the destructive boundary.
 * Ref: Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf pp. 88, 112
 * (unlock before discarding); audit L118. No local Finder message reference. */




/* Remove icon `idx` from a window's model, keeping every other icon exactly
 * where it is (a period Finder window does not re-flow when an item leaves). */
/* The window's per-entry FAT metadata (finder_window_t.meta: size, date,
 * time, attribute -- the list view's columns and sort keys) is a PARALLEL
 * array, same indices as the icons: it shifts with them, or every survivor
 * past `idx` would be drawn and sorted with its neighbour's size and date
 * (review of bead initech-vnl4: Put Away now removes in place, where it used
 * to re-enumerate). Each icon carries its own `selected`, so the selection
 * shifts with it.
 * MUTANT FINDER_OPS_MUT_REMOVE_NO_META (Rule 6; test-finder-ops O16): the icons
 * shift, the metadata does not. NEVER in a real build. */
static void fo_remove_icon(finder_window_t *w, int idx)
{
    for (int i = idx; i + 1 < (int)w->view.n; i++) {
        w->view.icons[i] = w->view.icons[i + 1];
#if !defined(FINDER_OPS_MUT_REMOVE_NO_META)
        if (w->meta != (finder_win_meta_t *)0) w->meta[i] = w->meta[i + 1];
#endif
    }
    w->view.n = (uint16_t)(w->view.n - 1u);
    finder_click_reset(&w->click);       /* stored indices shifted             */
}

/* "The next free row-major grid cell" (finder_ops.h): the LOWEST cell whose
 * sprite origin no icon of `dw` occupies. Found by bead initech-tdnl.39's put
 * back: the cell used to be indexed by the icon COUNT, which lands ON an icon
 * whenever an earlier item left the window -- a period window does not
 * re-flow (fo_remove_icon) -- so README.TXT put back from the Trash into a
 * root window still showing APPS in cell 1 was drawn on top of APPS. At most
 * n+1 cells are tried, so the walk always ends. */
static void fo_free_cell(finder_window_t *dw, int16_t *x, int16_t *y)
{
    rgn_rect_t doc = finder_win_doc_rect(dw);
    for (int c = 0;; c++) {
        int i = 0;
#if defined(FINDER_OPS_MUT_COUNT_CELL)
        /* MUTANT (Rule 6): the pre-fix count-indexed cell. */
        c = (int)dw->view.n;
#endif
        finder_win_grid_origin(doc, c, x, y);
        while (i < (int)dw->view.n &&
               (dw->view.icons[i].x != *x || dw->view.icons[i].y != *y))
            i++;
#if defined(FINDER_OPS_MUT_COUNT_CELL)
        i = (int)dw->view.n;
#endif
        if (i == (int)dw->view.n) return;
    }
}

static void fo_origin_add(finder_shell_t *sh, finder_move_result_t *out)
{
#if defined(FINDER_OPS_MUT_NO_ORIGIN)
    /* MUTANT (Rule 6): staging records NO kind=5 origin, so Put Away could
     * never return the item and the hand-authored origin set goes RED. */
    (void)sh; (void)out;
#else
    finder_origin_rec_t *o;
    if ((uint32_t)sh->n_origins >= FINDER_DB_MAX_ORIGINS) {
        out->origin_lost = 1u;           /* staged; the kernel says so (Rule 2)*/
        return;
    }
    o = &sh->origins[sh->n_origins];
    o->dir_start = out->from_dir;
    o->flags     = out->renamed ? (uint8_t)FINDER_ORIGIN_FLAG_RENAMED : 0u;
    fo_copy83(o->name83, out->as);
#ifdef FINDER_MUT_ORIGINAL_LOST
    o->original83[0]='\0';
#else
    fo_copy83(o->original83,out->name);
#endif
    sh->n_origins = (uint16_t)(sh->n_origins + 1u);
    out->db_dirty = 1u;
#endif
}

/* An item left \TRASH by an ordinary move: its origin record is stale. */
static void fo_origin_drop(finder_shell_t *sh, const char *name,
                           finder_move_result_t *out)
{
    for (uint16_t i = 0u; i < sh->n_origins; i++) {
        if (!fo_cold_ieq(sh->origins[i].name83, name)) continue;
        for (uint16_t k = i; (uint16_t)(k + 1u) < sh->n_origins; k++)
            sh->origins[k] = sh->origins[k + 1u];
        sh->n_origins = (uint16_t)(sh->n_origins - 1u);
        out->db_dirty = 1u;
        return;
    }
}

finder_win_status_t finder_ops_drop(finder_shell_t *sh, int src_slot,
                                    int src_idx, const finder_tgt_t *t,
                                    int16_t gx, int16_t gy,
                                    finder_move_result_t *out)
{
    finder_window_t *w;
    finder_desk_icon_t ic;
    fo_entry_probe_t probe;
    int td = -1;
    int is_trash;
    uint16_t dst;
    int rc;
    int dslot;

    if (out == (finder_move_result_t *)0) return FINDER_WIN_ERR_NULL;
    fo_zero(out, (uint32_t)sizeof *out);
    out->dst_slot = -1;
    out->status   = FINDER_WIN_ERR_MOVE;

    if (!fo_slot_ok(sh, src_slot) || t == (const finder_tgt_t *)0 ||
        src_idx < 0 || src_idx >= (int)sh->windows[src_slot].view.n)
        return fo_refuse(out, FINDER_WIN_ERR_MOVE);

    (void)finder_win_view(sh, src_slot);     /* re-based (tdnl.34)            */
    w  = &sh->windows[src_slot];
    ic = w->view.icons[src_idx];             /* a COPY: the array may shift   */
    fo_copy83(out->name, ic.name);
    fo_copy83(out->as, ic.name);
    out->from_dir = w->dir_start;

#if !defined(FINDER_OPS_MUT_NO_SERVICE_GUARD)
    /* The Finder's own \TRASH / \DESKTOP.DB never move, by IDENTITY, even if
     * a path reaches them (they are not listed -- finder_win_is_service): the
     * audit's H03 move of \TRASH stranded the staged items and broke the
     * Trash. Checked first, before the backend is touched. */
    if (finder_win_is_service(out->from_dir, ic.name))
        return fo_refuse(out, FINDER_WIN_ERR_SERVICE);
#else
    /* MUTANT (Rule 6; test-finder-ops-mutant): the identity guard removed. */
#endif
    if (t->kind != (uint8_t)FINDER_TGT_WINDOW &&
        t->kind != (uint8_t)FINDER_TGT_FOLDER &&
        t->kind != (uint8_t)FINDER_TGT_VOLUME &&
        t->kind != (uint8_t)FINDER_TGT_TRASH)
        return fo_refuse(out, FINDER_WIN_ERR_MOVE);
    if (!sh->have_fs ||
        sh->fs.move == (int (*)(void *, const char *, uint16_t,
                                const char *, uint16_t))0)
        return fo_refuse(out, FINDER_WIN_ERR_MOVE);

    if (sh->fs.trash_dir != (int (*)(void *))0) td = sh->fs.trash_dir(sh->fs.user);
    if (t->kind == (uint8_t)FINDER_TGT_TRASH) {
        out->op = (uint8_t)FINDER_OP_TRASH;
        /* No \TRASH (bead initech-vnl4, audit 2026-10-23 desktop L001): said
         * as itself, so the kernel can try to make it and -- on a full disk --
         * offer Delete Now instead of a dead-end "could not be moved". */
        if (td < 0) return fo_refuse(out, FINDER_WIN_ERR_NOTRASH);
        dst = (uint16_t)td;
    } else {
        dst = t->dir;
    }
    is_trash = (td >= 0 && dst == (uint16_t)td);
    out->op     = (uint8_t)(is_trash ? FINDER_OP_TRASH : FINDER_OP_MOVE);
    out->to_dir = dst;

#if !defined(FINDER_OPS_MUT_NO_SERVICE_GUARD)
    /* ... nor may a move CREATE one (a user's own TRASH folder dropped into
     * the root would become the staging directory's twin). */
    if (finder_win_is_service(dst, ic.name))
        return fo_refuse(out, FINDER_WIN_ERR_SERVICE);
#endif
    /* THE LADDER, in order (finder_ops.h). */
    if (dst == out->from_dir) return fo_refuse(out, FINDER_WIN_ERR_SAMEDIR);
    /* Bead initech-uios (audit L002): the windows the gesture runs through
     * must still BE their folders -- a drag out of, or into, the view of a
     * purged / recycled directory writes nothing (finder_windows.h Sec 10d). */
    {
        finder_win_status_t live = finder_win_check_live(sh, src_slot);
        if (live == FINDER_WIN_OK && t->slot >= 0 && (int)t->slot != src_slot)
            live = finder_win_check_live(sh, (int)t->slot);
        if (live != FINDER_WIN_OK) return fo_refuse(out, live);
    }
#if !defined(FINDER_OPS_MUT_NO_CYCLE)
    if (ic.kind == (uint8_t)FINDER_ICON_FOLDER && dst == ic.dir_start)
        return fo_refuse(out, FINDER_WIN_ERR_CYCLE);
#endif

    /* Design F3.1: a move preserves the live directory entry's metadata.
     * Copy it while the callback is valid, before storage or icon compaction
     * changes the source. The cached source view may predate a write/lock.
     * Ref: finder_windows.h callback lifetime + meta contract; initech-iebt. */
    probe.name = ic.name; probe.found = 0;
    if (sh->fs.enumerate == (int (*)(void *, uint16_t, finder_enum_cb, void *))0 ||
        sh->fs.enumerate(sh->fs.user, out->from_dir, fo_entry_cb, &probe) < 0 ||
        !probe.found)
        return fo_refuse(out, FINDER_WIN_ERR_ENUM);

    if (is_trash) {
        char as[FINDER_DESK_NAME_MAX];
#ifndef FINDER_OPS_MUT_LOCKED_STAGE
        if ((probe.meta.attribute & FINDER_ATTR_READONLY) != 0u)
            return fo_refuse(out, FINDER_WIN_ERR_LOCKED);
#endif
        if (sh->fs.trash_name == (int (*)(void *, const char *, uint16_t,
                                          char *))0)
            return fo_refuse(out, FINDER_WIN_ERR_MOVE);
        fo_zero(as, (uint32_t)sizeof as);
        if (sh->fs.trash_name(sh->fs.user, ic.name, dst, as) !=
            (int)FINDER_WIN_OK || as[0] == '\0')
            return fo_refuse(out, FINDER_WIN_ERR_MOVE);
        fo_copy83(out->as, as);
        out->renamed = fo_cold_ieq(out->as, ic.name) ? 0u : 1u;
    }

    /* tdnl.61: reserve the destination view BEFORE committing storage. */
    dslot=finder_win_find(sh,dst);
    if(dslot>=0 && dslot!=src_slot) {
        uint16_t n=sh->windows[dslot].view.n;
        finder_win_status_t reserve=n==65535u?FINDER_WIN_ERR_MEMORY:
            finder_win_reserve_icons(sh,dslot,(uint16_t)(n+1u));
        if(reserve!=FINDER_WIN_OK) return fo_refuse(out,reserve);
    }
#if defined(FINDER_OPS_MUT_TRASH_NO_STAGE)
    /* MUTANT (design F1.4's named TRASH_NO_STAGE): the Trash path DELETES the
     * item outright instead of transplanting it, so the mtools differential
     * shows it absent from \TRASH (and its chain freed) -- RED. */
    if (is_trash) {
        rc = (sh->fs.unlink != (int (*)(void *, const char *, uint16_t))0)
           ? sh->fs.unlink(sh->fs.user, ic.name, out->from_dir)
           : (int)FINDER_WIN_ERR_MOVE;
    } else
#endif
    rc = sh->fs.move(sh->fs.user, ic.name, out->from_dir,
                     out->renamed ? out->as : (const char *)0, dst);
    if (rc != (int)FINDER_WIN_OK) {
        if (rc == (int)FINDER_WIN_ERR_SAMEDIR || rc == (int)FINDER_WIN_ERR_CYCLE ||
            rc == (int)FINDER_WIN_ERR_EXISTS || rc == (int)FINDER_WIN_ERR_DISKFULL)
            return fo_refuse(out, (finder_win_status_t)rc);
        return fo_refuse(out, FINDER_WIN_ERR_MOVE);
    }

    /* COMMITTED on the volume -- now make every model tell the same truth. */
    fo_follow(sh,&ic,dst,out->as);
    {
        rgn_rect_t old_cell = finder_desk_cell_rect(&w->view, src_idx);
        finder_win_invalidate(sh, src_slot, old_cell, old_cell);
        fo_remove_icon(w, src_idx);
    }

    dslot = finder_win_find(sh, dst);
    if (dslot >= 0 && dslot != src_slot) {
        finder_window_t *dw = &sh->windows[dslot];
        int16_t x = 0, y = 0;
        int nidx;

        (void)finder_win_sync_geometry(dw);  /* land on the CURRENT content  */
        if (t->kind == (uint8_t)FINDER_TGT_WINDOW && (int)t->slot == dslot) {
            /* Dropped on the window body: it lands WHERE IT WAS DROPPED,
             * measured relative to that window's content and held inside it
             * by the same clamp a same-window drop uses. */
            fo_cold_to_view(dw, gx, gy, &x, &y);
            finder_desk_clamp(&dw->view, &x, &y);
        } else {
            /* Dropped on a folder ICON (or the volume / Trash) whose window
             * happens to be open: the next free row-major grid cell. */
            /* The grid is in DOCUMENT coordinates (finder_windows.h Sec
             * 10c): the cell may lie below the visible rows of a scrolled
             * window, which the scroll bar then reaches -- so no clamp. */
            fo_free_cell(dw, &x, &y);
        }
        nidx = finder_desk_add(&dw->view, (finder_icon_kind_t)ic.kind,
                               out->as, x, y, 1u);
        if (nidx < 0) {
            dw->dropped = (uint16_t)(dw->dropped + 1u);   /* the cap bit      */
        } else {
#ifndef FINDER_OPS_MUT_DROP_NO_META
            dw->meta[nidx] = probe.meta;
#endif
            if (ic.kind == (uint8_t)FINDER_ICON_FOLDER)
                finder_desk_set_cluster(&dw->view, nidx, ic.dir_start);
        }
        finder_win_invalidate_all(sh, dslot);
        out->dst_slot = (int16_t)dslot;
    }

    if (is_trash) {
        fo_origin_add(sh, out);
    } else if (td >= 0 && out->from_dir == (uint16_t)td) {
        fo_origin_drop(sh, ic.name, out);
    }
    /* \TRASH may have gained or lost an item: the FULL icon and the Empty
     * Trash predicate follow (bead initech-6k12). */
    (void)finder_shell_recount_trash(sh);

    out->status = FINDER_WIN_OK;
    return FINDER_WIN_OK;
}

/* tdnl.57: Replace uses the existing staging path. The displaced item is
 * recoverable in Trash; if the incoming move fails it is put back. */
finder_win_status_t finder_ops_replace(finder_shell_t *sh,int slot,int idx,
                                      const finder_tgt_t *t,int16_t x,int16_t y,
                                      finder_move_result_t *out)
{
    finder_move_result_t backup;
    finder_desk_icon_t displaced;
    fo_entry_probe_t probe;
    finder_win_status_t st;
    char name[FINDER_DESK_NAME_MAX];
    int td, rc, dw, tw;
#ifdef FINDER_MUT_REPLACE_CANCEL
    (void)sh;(void)slot;(void)idx;(void)t;(void)x;(void)y;
    return fo_refuse(out,FINDER_WIN_ERR_EXISTS);
#endif
    if(!fo_slot_ok(sh,slot) || !t || !sh->fs.move || !sh->fs.trash_name ||
        !sh->fs.trash_dir || !sh->fs.enumerate) return fo_refuse(out,FINDER_WIN_ERR_NULL);
    if(idx<0 || idx>=sh->windows[slot].view.n) return fo_refuse(out,FINDER_WIN_ERR_NULL);
    if(sh->n_origins>=FINDER_DB_MAX_ORIGINS) return fo_refuse(out,FINDER_WIN_ERR_MEMORY);
    fo_copy83(name,sh->windows[slot].view.icons[idx].name);
    probe.name=name;probe.found=0;
    if(sh->fs.enumerate(sh->fs.user,t->dir,fo_entry_cb,&probe)<0 || !probe.found)
        return fo_refuse(out,FINDER_WIN_ERR_MOVE);
    if(probe.meta.attribute & FINDER_ATTR_READONLY) return fo_refuse(out,FINDER_WIN_ERR_LOCKED);
    fo_zero(&displaced,sizeof displaced);
    displaced.kind=(probe.meta.attribute & FINDER_ATTR_DIRECTORY)?FINDER_ICON_FOLDER:FINDER_ICON_FILE;
    displaced.dir_start=probe.cluster;
    td=sh->fs.trash_dir(sh->fs.user);
    if(td<0 || (uint16_t)td==t->dir) return fo_refuse(out,FINDER_WIN_ERR_MOVE);
    fo_zero(&backup,sizeof backup);
    backup.from_dir=t->dir;backup.to_dir=(uint16_t)td;
    fo_copy83(backup.name,name);
    rc=sh->fs.trash_name(sh->fs.user,name,(uint16_t)td,backup.as);
    if(rc!=FINDER_WIN_OK) return fo_refuse(out,(finder_win_status_t)rc);
    backup.renamed=(uint8_t)!fo_cold_ieq(backup.name,backup.as);
    tw=finder_win_find(sh,(uint16_t)td);dw=finder_win_find(sh,t->dir);
    if(tw>=0) {
        st=finder_win_reserve_icons(sh,tw,(uint16_t)(sh->windows[tw].view.n+1u));
        if(st!=FINDER_WIN_OK) return fo_refuse(out,st);
    }
    rc=sh->fs.move(sh->fs.user,name,t->dir,backup.as,(uint16_t)td);
    if(rc!=FINDER_WIN_OK) return fo_refuse(out,(finder_win_status_t)rc);
    fo_follow(sh,&displaced,(uint16_t)td,backup.as);
    if(dw>=0) (void)finder_win_populate(sh,dw);
    st=finder_ops_drop(sh,slot,idx,t,x,y,out);
    if(st!=FINDER_WIN_OK) {
        if(sh->fs.move(sh->fs.user,backup.as,(uint16_t)td,name,t->dir)!=FINDER_WIN_OK) {
            fo_origin_add(sh,&backup);
            out->db_dirty=1u;
        } else fo_follow(sh,&displaced,t->dir,name);
    } else {
        fo_origin_add(sh,&backup);
        out->db_dirty=1u;
    }
    if(dw>=0) { (void)finder_win_populate(sh,dw);finder_win_invalidate_all(sh,dw); }
    if(tw>=0) { (void)finder_win_populate(sh,tw);finder_win_invalidate_all(sh,tw); }
    (void)finder_shell_recount_trash(sh);
    return st;
}

/* tdnl.57: persisted origin, including the spelling before a Trash collision.
 * Legacy renamed records lacking kind=6 refuse rather than guess. */
finder_win_status_t finder_ops_put_away(finder_shell_t *sh,int slot,int idx)
{
    finder_window_t *w;
    finder_origin_rec_t origin;
    finder_win_status_t st;
    finder_move_result_t result;
    int found=-1, dest, rc;
    if(!fo_slot_ok(sh,slot) || !sh->fs.move) return FINDER_WIN_ERR_NULL;
    w=&sh->windows[slot];
    if(idx<0 || idx>=w->view.n || !w->is_trash) return FINDER_WIN_ERR_MOVE;
    for(int i=0;i<sh->n_origins;i++)
        if(fo_cold_ieq(sh->origins[i].name83,w->view.icons[idx].name)) {found=i;break;}
    if(found<0) return FINDER_WIN_ERR_MOVE;
    origin=sh->origins[found];
    if(!origin.original83[0]) return FINDER_WIN_ERR_MOVE;
    st=finder_win_check_live(sh,slot);
    if(st!=FINDER_WIN_OK) return st;
    dest=finder_win_find(sh,origin.dir_start);
    if(dest>=0) {
        st=finder_win_reserve_icons(sh,dest,(uint16_t)(sh->windows[dest].view.n+1u));
        if(st!=FINDER_WIN_OK) return st;
    }
    rc=sh->fs.move(sh->fs.user,origin.name83,w->dir_start,
                   origin.original83,origin.dir_start);
    if(rc!=FINDER_WIN_OK) return (finder_win_status_t)rc;
    fo_follow(sh,&w->view.icons[idx],origin.dir_start,origin.original83);
    fo_zero(&result,sizeof result);
    fo_origin_drop(sh,origin.name83,&result);
#if defined(FINDER_MUT_PUTAWAY_REPOPULATE)
    /* MUTANT (Rule 6; test-flair-trash-edges): the pre-fix re-enumeration --
     * a fresh listing has NO selection, so a multiple Put Away stops after
     * the first item (audit L006). NEVER in a real build. */
    (void)finder_win_populate(sh,slot);
#else
    /* bead initech-vnl4 (audit docs/audits/2026-10-23-desktop REPORT.md L006):
     * the item LEAVES the Trash window -- remove just its icon, the way a
     * drag out does (fo_remove_icon: a period window does not re-flow). A
     * re-enumeration here cleared every OTHER selected icon, so Put Away of
     * a multiple selection restored one item and silently stopped. "Put Away
     * returns any selected documents, folders, or applications ... in the
     * Trash window back to the folders and disks they belong to"
     * (Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf printed p. 117).
     * fw_do_put_away walks the selection from the END, so removing index idx
     * shifts no icon it has yet to visit. */
    fo_remove_icon(w,idx);
#endif
    finder_win_invalidate_all(sh,slot);
    if(dest>=0) { (void)finder_win_populate(sh,dest);finder_win_invalidate_all(sh,dest); }
    (void)finder_shell_recount_trash(sh);
    return FINDER_WIN_OK;
}
