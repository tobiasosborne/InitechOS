/*
 * os/flair/finder_ops.c -- the R3.4a Finder FILE OPERATIONS (THE ARTIFACT).
 *
 * beads: initech-34dh (reslice 1/3 of initech-tdnl.11).
 * Ref:   os/flair/finder_ops.h -- the contract: what the user sees and the
 *          period references, the drop-target table, the refusal ladder and
 *          its order, Trash staging + kind=5 origins, the serial lines the
 *          kernel prints, and the mutant knobs;
 *        docs/design/GUI-remediation-R3-finder-design.md F1.4/F1.5/F2.2/F3.1.
 *
 * Artifact C per ADR-0002: freestanding, no allocation, no libc, no serial
 * (os/flair files never call serial_puts -- the kernel prints the outcome),
 * deterministic, ASCII-only. Dual-compiles for harness/proptest/
 * test_finder_ops.c.
 */
#include "finder_ops.h"

static char fo_upper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

/* Case-insensitive 8.3 compare (FAT names are case-insensitive). */
static int fo_ieq(const char *a, const char *b)
{
    if (a == (const char *)0 || b == (const char *)0) return 0;
    while (*a != '\0' && fo_upper(*a) == fo_upper(*b)) { a++; b++; }
    return fo_upper(*a) == fo_upper(*b);
}

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

static int fo_slot_ok(const finder_shell_t *sh, int slot)
{
    return sh != (const finder_shell_t *)0 &&
           slot >= 0 && slot < FINDER_WIN_MAX && sh->windows[slot].open;
}

/* The icon model a target's slot names (the desktop for -1), re-based onto
 * the window's current content rect (finder_win_view, bead initech-tdnl.34). */
static finder_desk_t *fo_surface(finder_shell_t *sh, int slot)
{
    if (slot < 0) return &sh->desk;
    return finder_win_view(sh, slot);
}

/* A global point re-based onto window `fw`'s icon model: the offset INTO the
 * window's content rect, added to the model's own origin. Every caller syncs
 * the model first (finder_win_view, bead initech-tdnl.34), so the model's
 * bounds ARE the content rect and this is the identity; it stays written
 * relative so drop targeting cannot drift if that ever stops being true. */
static void fo_to_view(const finder_window_t *fw, int16_t h, int16_t v,
                       int16_t *vh, int16_t *vv)
{
    rgn_rect_t c = region_get_bbox(fw->rec.contRgn);
    *vh = (int16_t)(fw->view.bounds.left + (h - c.left));
    *vv = (int16_t)(fw->view.bounds.top  + (v - c.top));
}

/* ===========================================================================
 * TARGET RESOLUTION
 * ===========================================================================*/

finder_tgt_t finder_ops_resolve(finder_shell_t *sh, int src_slot,
                                int src_idx, int16_t h, int16_t v)
{
    finder_tgt_t t;
    flair_point_t pt;
    WindowPtr w = (WindowPtr)0;
    flair_part_code_t part;

    t.kind = (uint8_t)FINDER_TGT_NONE;
    t.slot = -1;
    t.idx  = -1;
    t.dir  = 0u;
    if (sh == (finder_shell_t *)0 || sh->wm == (WindowMgr *)0) return t;

    pt.h = h;
    pt.v = v;
    part = FindWindow(sh->wm, pt, &w);

    if (part == inContent) {
        const finder_window_t *fw;
        int16_t vh, vv;
        int s = finder_win_slot_of(sh, w);
        int j;

        if (s < 0) return t;            /* another tenant's window: no target */
        (void)finder_win_view(sh, s);   /* re-base onto the live content rect */
        fw = &sh->windows[s];
        fo_to_view(fw, h, v, &vh, &vv);
        j = finder_desk_hit(&fw->view, vh, vv);
        if (j >= 0 && !(s == src_slot && j == src_idx) &&
            fw->view.icons[j].kind == (uint8_t)FINDER_ICON_FOLDER) {
            t.kind = (uint8_t)FINDER_TGT_FOLDER;
            t.slot = (int8_t)s;
            t.idx  = (int16_t)j;
            t.dir  = fw->view.icons[j].dir_start;
        } else if (s == src_slot) {
            t.kind = (uint8_t)FINDER_TGT_REPOSITION;
            t.slot = (int8_t)s;
        } else {
            t.kind = (uint8_t)FINDER_TGT_WINDOW;
            t.slot = (int8_t)s;
            t.dir  = fw->dir_start;
        }
        return t;
    }

    if (part == inDesk) {
        int j = finder_desk_hit(&sh->desk, h, v);
        if (j < 0) return t;
        if (sh->desk.icons[j].kind == (uint8_t)FINDER_ICON_TRASH) {
            t.kind = (uint8_t)FINDER_TGT_TRASH;
            t.idx  = (int16_t)j;
        } else if (sh->desk.icons[j].kind == (uint8_t)FINDER_ICON_VOLUME) {
            t.kind = (uint8_t)FINDER_TGT_VOLUME;
            t.idx  = (int16_t)j;
            t.dir  = 0u;                 /* the fixed root                     */
        }
    }
    return t;
}

int finder_ops_tgt_hilites(const finder_tgt_t *t)
{
    if (t == (const finder_tgt_t *)0 || t->idx < 0) return 0;
    return t->kind == (uint8_t)FINDER_TGT_FOLDER ||
           t->kind == (uint8_t)FINDER_TGT_VOLUME ||
           t->kind == (uint8_t)FINDER_TGT_TRASH;
}

static void fo_set_hilite(finder_shell_t *sh, const finder_tgt_t *t,
                          uint8_t on)
{
    finder_desk_t *fd = fo_surface(sh, (int)t->slot);
    rgn_rect_t cell;

    if (fd == (finder_desk_t *)0 || t->idx < 0 || t->idx >= (int16_t)fd->n)
        return;
    fd->icons[t->idx].hilite = on;
    cell = finder_desk_cell_rect(fd, (int)t->idx);
    if (t->slot < 0) finder_desk_invalidate(sh->wm, cell, cell);
    else             finder_win_invalidate(sh, (int)t->slot, cell, cell);
}

int finder_ops_track_hilite(finder_shell_t *sh, finder_tgt_t *cur,
                            const finder_tgt_t *next)
{
    int ch = finder_ops_tgt_hilites(cur);
    int nh = finder_ops_tgt_hilites(next);

    if (sh == (finder_shell_t *)0 || cur == (finder_tgt_t *)0 ||
        next == (const finder_tgt_t *)0)
        return 0;
    if (ch == nh && (!ch || (cur->slot == next->slot && cur->idx == next->idx))) {
        *cur = *next;
        return 0;
    }
    if (ch) fo_set_hilite(sh, cur, 0u);
    if (nh) fo_set_hilite(sh, next, 1u);
    *cur = *next;
    return 1;
}

/* ===========================================================================
 * THE DROP
 * ===========================================================================*/

const char *finder_ops_reason(finder_win_status_t st)
{
    if (st == FINDER_WIN_ERR_SAMEDIR) return "samedir";
    if (st == FINDER_WIN_ERR_CYCLE)   return "cycle";
    if (st == FINDER_WIN_ERR_EXISTS)  return "exists";
    return "err";
}

static finder_win_status_t fo_refuse(finder_move_result_t *out,
                                     finder_win_status_t st)
{
    out->status = st;
    return st;
}

/* Remove icon `idx` from a window's model, keeping every other icon exactly
 * where it is (a period Finder window does not re-flow when an item leaves). */
static void fo_remove_icon(finder_window_t *w, int idx)
{
    for (int i = idx; i + 1 < (int)w->view.n; i++)
        w->view.icons[i] = w->view.icons[i + 1];
    w->view.n = (uint16_t)(w->view.n - 1u);
    finder_click_reset(&w->click);       /* stored indices shifted             */
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
    sh->n_origins = (uint16_t)(sh->n_origins + 1u);
    out->db_dirty = 1u;
#endif
}

/* An item left \TRASH by an ordinary move: its origin record is stale. */
static void fo_origin_drop(finder_shell_t *sh, const char *name,
                           finder_move_result_t *out)
{
    for (uint16_t i = 0u; i < sh->n_origins; i++) {
        if (!fo_ieq(sh->origins[i].name83, name)) continue;
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
        if (td < 0) return fo_refuse(out, FINDER_WIN_ERR_MOVE); /* no \TRASH  */
        dst = (uint16_t)td;
    } else {
        dst = t->dir;
    }
    is_trash = (td >= 0 && dst == (uint16_t)td);
    out->op     = (uint8_t)(is_trash ? FINDER_OP_TRASH : FINDER_OP_MOVE);
    out->to_dir = dst;

    /* THE LADDER, in order (finder_ops.h). */
    if (dst == out->from_dir) return fo_refuse(out, FINDER_WIN_ERR_SAMEDIR);
#if !defined(FINDER_OPS_MUT_NO_CYCLE)
    if (ic.kind == (uint8_t)FINDER_ICON_FOLDER && dst == ic.dir_start)
        return fo_refuse(out, FINDER_WIN_ERR_CYCLE);
#endif

    if (is_trash) {
        char as[FINDER_DESK_NAME_MAX];
        if (sh->fs.trash_name == (int (*)(void *, const char *, uint16_t,
                                          char *))0)
            return fo_refuse(out, FINDER_WIN_ERR_MOVE);
        fo_zero(as, (uint32_t)sizeof as);
        if (sh->fs.trash_name(sh->fs.user, ic.name, dst, as) !=
            (int)FINDER_WIN_OK || as[0] == '\0')
            return fo_refuse(out, FINDER_WIN_ERR_MOVE);
        fo_copy83(out->as, as);
        out->renamed = fo_ieq(out->as, ic.name) ? 0u : 1u;
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
            rc == (int)FINDER_WIN_ERR_EXISTS)
            return fo_refuse(out, (finder_win_status_t)rc);
        return fo_refuse(out, FINDER_WIN_ERR_MOVE);
    }

    /* COMMITTED on the volume -- now make every model tell the same truth. */
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
            fo_to_view(dw, gx, gy, &x, &y);
            finder_desk_clamp(&dw->view, &x, &y);
        } else {
            /* Dropped on a folder ICON (or the volume / Trash) whose window
             * happens to be open: the next free row-major grid cell. */
            /* The grid is in DOCUMENT coordinates (finder_windows.h Sec
             * 10c): the cell may lie below the visible rows of a scrolled
             * window, which the scroll bar then reaches -- so no clamp. */
            finder_win_grid_origin(finder_win_doc_rect(dw), (int)dw->view.n,
                                   &x, &y);
        }
        nidx = finder_desk_add(&dw->view, (finder_icon_kind_t)ic.kind,
                               out->as, x, y, 1u);
        if (nidx < 0) {
            dw->dropped = (uint16_t)(dw->dropped + 1u);   /* the cap bit      */
        } else if (ic.kind == (uint8_t)FINDER_ICON_FOLDER) {
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

    out->status = FINDER_WIN_OK;
    return FINDER_WIN_OK;
}

uint16_t finder_shell_load_origins(finder_shell_t *sh, const uint8_t *buf,
                                   uint32_t len)
{
    int n;
    if (sh == (finder_shell_t *)0) return 0u;
    sh->n_origins = 0u;
    n = finder_desk_db_get_origins(buf, len, sh->origins,
                                   (uint16_t)FINDER_DB_MAX_ORIGINS);
    if (n <= 0) return 0u;
    sh->n_origins = (uint16_t)n;
    return (uint16_t)n;
}
