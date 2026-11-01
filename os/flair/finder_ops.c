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
 * ADR-0014 lane 2: this resident model keeps drag resolution, highlighting,
 * boot-time origin loading and the enumeration callback. Operation bodies
 * live in finder_ops_cold.c; resident callers resolve to generated stubs.
 */
#include "finder_ops_private.h"

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

int fo_entry_cb(const finder_dirent_t *e, void *u)
{
    fo_entry_probe_t *p = (fo_entry_probe_t *)u;
    if (!fo_ieq(e->name83, p->name)) return 0;
    p->meta.attribute = e->attribute;
    p->meta.size = e->size;
    p->meta.date = e->date;
    p->meta.time = e->time;
    p->cluster=e->start_cluster;
    p->found = 1;
    return 1;
}
#ifndef FINDER_OVL_RESIDENT
#include "finder_ops_cold.c"
#endif
