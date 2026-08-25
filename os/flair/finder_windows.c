/*
 * os/flair/finder_windows.c -- the R3 Finder DISK WINDOWS (THE ARTIFACT).
 *
 * beads: initech-tdnl.10 (GUI remediation R3.3 "disk windows").
 * Ref:   os/flair/finder_windows.h -- the contract, the layering rule, the
 *          binding seam, the LOCKED grid + name-ladder conventions, the serial
 *          marker list (including the FINDER-OPEN-VOLUME re-key) and every
 *          stated deviation from the design document;
 *        docs/design/GUI-remediation-R3-finder-design.md F1.1/F1.2/F1.3,
 *          F2-3/F2-4, F3.1/F3.2/F3.3, F5.2;
 *        os/apps/ref_tenant.c (the arena-carved WindowRecord + region bundles,
 *          and the contRgn INTERSECT updateRgn repaint this file mirrors);
 *        os/flair/finder_desktop.c (the trackers + the ONE icon painter).
 *
 * Artifact C per ADR-0002: freestanding, no allocation outside the tenant's
 * arenas, no libc, deterministic, ASCII-only.
 *
 * MUTANT KNOBS (Rule 6; -D on this TU only, from the Makefile, never a real
 * build). Each perturbs the IMPLEMENTATION, never the oracle's hand-authored
 * expectations:
 *   FINDER_WIN_MUT_SINGLETON_DUP    -- the spatial-singleton lookup always
 *                                      misses, so reopening a folder builds a
 *                                      SECOND window for it (design F5.2's
 *                                      named SPATIAL_DUP mutant).
 *   FINDER_WIN_MUT_VOLLABEL_SHOWN   -- the volume-label skip is compiled out,
 *                                      so the volume name leaks into the
 *                                      listing as a document icon (F1.1).
 *   FINDER_WIN_MUT_CLEANUP_UNSORTED -- Clean Up assigns grid cells in REVERSE
 *                                      record order, so the snap is no longer
 *                                      row-major (F3.3).
 */
#include "finder_windows.h"

#include "flair_look.h"         /* flair_look_pixel_depth -- the ONE seam      */
#include "blitter.h"            /* blitter_fill_rect_clipped (the content fill)*/
#include "heap.h"               /* flair_alloc / FLAIR_CLASS_HANDLE            */
#include "event_model.h"        /* EventRecord, updateEvt / keyDown            */
#include "window_record.h"      /* documentKind                                */

/* NO PANIC MACRO HERE, DELIBERATELY. Every failure this file can meet is a
 * CONDITION, not a programming error: the window table is full, the binding
 * refused, a directory would not enumerate, the name ladder is exhausted. Each
 * one returns a named finder_win_status_t and the caller (os/milton/kmain.c)
 * says so on serial. The one genuine invariant violation -- the tenant's
 * records arena being too small for the shell -- is reported by returning 1
 * from open(), which FlairProcess_launch turns into a launch failure and kmain
 * turns into a loud halt (Rule 2, at the layer that owns the console).
 * ------------------------------------------------------------------------- */

/* ===========================================================================
 * Small libc-free helpers (Law 3).
 * ===========================================================================*/

static void fw_zero(void *dst, uint32_t n)
{
    uint8_t *b = (uint8_t *)dst;
    for (uint32_t i = 0u; i < n; i++) b[i] = 0u;
}

static char fw_upper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

static uint32_t fw_len(const char *s)
{
    uint32_t n = 0u;
    if (s == (const char *)0) return 0u;
    while (s[n] != '\0') n++;
    return n;
}

/* Case-insensitive 8.3 compare (FAT names are case-insensitive). */
static int fw_ieq(const char *a, const char *b)
{
    if (a == (const char *)0 || b == (const char *)0) return 0;
    while (*a != '\0' && fw_upper(*a) == fw_upper(*b)) { a++; b++; }
    return fw_upper(*a) == fw_upper(*b);
}

static void fw_copy83(char *dst, const char *src)
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

static int fw_slot_ok(const finder_shell_t *sh, int slot)
{
    return sh != (const finder_shell_t *)0 &&
           slot >= 0 && slot < FINDER_WIN_MAX && sh->windows[slot].open;
}

/* Attach one region bundle to its caller-supplied backing (the ref_tenant
 * rgn_attach idiom; the engine never mallocs). */
static void fw_rgn_attach(finder_win_rgn_t *s)
{
    s->r.rows       = s->rows;
    s->r.cap_rows   = FINDER_WIN_RGN_ROWS;
    s->r.x_pool     = s->pool;
    s->r.x_pool_cap = FINDER_WIN_RGN_POOL;
    region_set_empty(&s->r);
}

/* ===========================================================================
 * 9. PURE HELPERS
 * ===========================================================================*/

finder_icon_kind_t finder_win_kind_of(const char *name83, uint8_t attribute)
{
    uint32_t n;

    if ((attribute & (uint8_t)FINDER_ATTR_DIRECTORY) != 0u)
        return FINDER_ICON_FOLDER;

    /* ".EXE" suffix, case-insensitive (design F1.1: the V1 app heuristic; the
     * extension is COSMETIC -- DEC-08a.4 dispatches on content). */
    n = fw_len(name83);
    if (n >= 4u &&
        name83[n - 4] == '.' &&
        fw_upper(name83[n - 3]) == 'E' &&
        fw_upper(name83[n - 2]) == 'X' &&
        fw_upper(name83[n - 1]) == 'E')
        return FINDER_ICON_APP;

    return FINDER_ICON_FILE;
}

int finder_win_skip_entry(uint8_t attribute)
{
#if defined(FINDER_WIN_MUT_VOLLABEL_SHOWN)
    /* MUTANT (design F1.1): the volume-label skip is compiled out, so the
     * volume's name leaks into the listing as an extra document icon and the
     * hand-authored expected listing goes RED. NEVER in a real build. */
    (void)attribute;
    return 0;
#else
    /* The FAT layer deliberately passes volume-label entries through
     * (fat12.h :: fat12_read_root_dir); skipping them is the FINDER's job. A
     * VFAT long-name slot (attr == 0x0F) is already filtered by that layer, and
     * it would test TRUE here anyway -- both are correct outcomes. */
    return ((attribute & (uint8_t)FINDER_ATTR_VOLLABEL) != 0u) ? 1 : 0;
#endif
}

rgn_rect_t finder_win_default_frame(int slot)
{
    rgn_rect_t r;
    int i = (slot < 0) ? 0 : (slot % FINDER_WIN_MAX);

    r.left   = (int16_t)(FINDER_WIN_DEFAULT_L + i * FINDER_WIN_CASCADE_DX);
    r.top    = (int16_t)(FINDER_WIN_DEFAULT_T + i * FINDER_WIN_CASCADE_DY);
    r.right  = (int16_t)(r.left + FINDER_WIN_DEFAULT_W);
    r.bottom = (int16_t)(r.top + FINDER_WIN_DEFAULT_H);
    return r;
}

int finder_win_grid_cols(rgn_rect_t c)
{
    int usable = (int)c.right - (int)c.left - FINDER_GRID_INSET_X;
    int cols;
    if (usable < FINDER_GRID_PITCH_X) return 1;   /* always at least one column */
    cols = usable / FINDER_GRID_PITCH_X;
    return (cols < 1) ? 1 : cols;
}

void finder_win_grid_origin(rgn_rect_t c, int i, int16_t *out_x, int16_t *out_y)
{
    int cols = finder_win_grid_cols(c);
    int col, row;

    if (i < 0) i = 0;
    col = i % cols;
    row = i / cols;

    if (out_x != (int16_t *)0)
        *out_x = (int16_t)((int)c.left + FINDER_GRID_INSET_X +
                           col * FINDER_GRID_PITCH_X);
    if (out_y != (int16_t *)0)
        *out_y = (int16_t)((int)c.top + FINDER_GRID_INSET_Y +
                           row * FINDER_GRID_PITCH_Y);
}

/* ---------------------------------------------------------------------------
 * The New Folder name ladder (finder_windows.h Sec 4 -- the LOCKED convention).
 *
 * `taken` is a 100-bit set: bit 0 == the un-suffixed FINDER_NEWFOLDER_BASE is
 * in use, bit k (2 <= k <= 99) == FINDER_NEWFOLDER_STEM + two digits of k is in
 * use. Lowest free wins; the ladder starts at the un-suffixed name and then
 * counts from 02, so a directory with no NEWFOL* entries always gets
 * "NEWFOLD" -- the same name, every time, on every machine (Rule 11).
 * ------------------------------------------------------------------------- */
typedef struct fw_taken {
    uint8_t bits[FINDER_NEWFOLDER_MAX_N + 1];   /* index 0 and 2..99 are live  */
} fw_taken_t;

static void fw_taken_note(fw_taken_t *t, const char *name)
{
    uint32_t stem = fw_len(FINDER_NEWFOLDER_STEM);
    uint32_t i;
    int n;

    if (name == (const char *)0 || name[0] == '\0') return;

    if (fw_ieq(name, FINDER_NEWFOLDER_BASE)) { t->bits[0] = 1u; return; }

    for (i = 0u; i < stem; i++)
        if (fw_upper(name[i]) != fw_upper(FINDER_NEWFOLDER_STEM[i])) return;
    if (name[stem] < '0' || name[stem] > '9') return;
    if (name[stem + 1u] < '0' || name[stem + 1u] > '9') return;
    if (name[stem + 2u] != '\0') return;         /* an extension is a MISS      */

    n = (name[stem] - '0') * 10 + (name[stem + 1u] - '0');
    if (n >= 2 && n <= FINDER_NEWFOLDER_MAX_N) t->bits[n] = 1u;
}

static finder_win_status_t fw_taken_next(const fw_taken_t *t, char *out)
{
    uint32_t stem = fw_len(FINDER_NEWFOLDER_STEM);
    int n;

    if (out == (char *)0) return FINDER_WIN_ERR_NULL;

    if (!t->bits[0]) { fw_copy83(out, FINDER_NEWFOLDER_BASE); return FINDER_WIN_OK; }

    for (n = 2; n <= FINDER_NEWFOLDER_MAX_N; n++) {
        if (t->bits[n]) continue;
        fw_copy83(out, FINDER_NEWFOLDER_STEM);
        out[stem]      = (char)('0' + (n / 10));
        out[stem + 1u] = (char)('0' + (n % 10));
        out[stem + 2u] = '\0';
        return FINDER_WIN_OK;
    }
    return FINDER_WIN_ERR_NAMES;                 /* fail loud (Rule 2)          */
}

finder_win_status_t finder_win_next_folder_name(
        const char (*existing)[FINDER_DESK_NAME_MAX], int n, char *out)
{
    fw_taken_t t;
    fw_zero(&t, (uint32_t)sizeof t);
    if (existing != (const char (*)[FINDER_DESK_NAME_MAX])0)
        for (int i = 0; i < n; i++) fw_taken_note(&t, existing[i]);
    return fw_taken_next(&t, out);
}

/* ===========================================================================
 * 10. WINDOW OPERATIONS
 * ===========================================================================*/

int finder_win_find(const finder_shell_t *sh, uint16_t dir_start)
{
#if defined(FINDER_WIN_MUT_SINGLETON_DUP)
    /* MUTANT (design F5.2 "SPATIAL_DUP"): the lookup always misses, so opening
     * an already-open folder builds a SECOND window for the same directory and
     * the singleton property goes RED. NEVER in a real build. */
    (void)sh; (void)dir_start;
    return -1;
#else
    if (sh == (const finder_shell_t *)0) return -1;
    for (int i = 0; i < FINDER_WIN_MAX; i++)
        if (sh->windows[i].open && sh->windows[i].dir_start == dir_start)
            return i;
    return -1;
#endif
}

int finder_win_slot_of(const finder_shell_t *sh, const WindowRecord *w)
{
    if (sh == (const finder_shell_t *)0 || w == (const WindowRecord *)0)
        return -1;
    for (int i = 0; i < FINDER_WIN_MAX; i++)
        if (sh->windows[i].open && &sh->windows[i].rec == w) return i;
    return -1;
}

int finder_win_front_slot(const finder_shell_t *sh)
{
    if (sh == (const finder_shell_t *)0 || sh->wm == (WindowMgr *)0) return -1;
    /* The z-order runs front-to-back, so the FIRST owned match is the front. */
    for (WindowPtr w = sh->wm->front; w != (WindowPtr)0; w = w->nextWindow) {
        int slot;
        if (!w->visible) continue;
        slot = finder_win_slot_of(sh, w);
        if (slot >= 0) return slot;
    }
    return -1;
}

/* Re-point the tenant's window-group head at the frontmost owned window (or
 * NULL when none remain). process.h uses app->windows for the activateEvt
 * message and for FlairProcess_launch's SelectWindow; the ROUTING demux is
 * refCon-based, so this is bookkeeping, not dispatch -- but a stale head after
 * a close is exactly the 8fhu class of bug, so it is maintained explicitly. */
static void fw_refresh_group_head(finder_shell_t *sh)
{
    int slot;
    if (sh == (finder_shell_t *)0 || sh->app == (FlairApp *)0) return;
    slot = finder_win_front_slot(sh);
    sh->app->windows = (slot >= 0) ? &sh->windows[slot].rec : (WindowPtr)0;
}

/* ---- enumeration -> icon records ---------------------------------------- */

typedef struct fw_enum_ctx {
    finder_window_t *w;
    uint16_t         dropped;
} fw_enum_ctx_t;

/* THE CALLBACK. Everything it needs is copied out HERE, during the call: the
 * formatted name (finder_desk_add copies it), the kind (derived from the
 * attribute byte and the name, both read now), and the entry's own first
 * cluster. Nothing from `e` is retained (finder_windows.h banner). */
static int fw_enum_cb(const finder_dirent_t *e, void *user)
{
    fw_enum_ctx_t *ec = (fw_enum_ctx_t *)user;
    finder_icon_kind_t kind;
    int idx;

    if (e == (const finder_dirent_t *)0 || ec == (fw_enum_ctx_t *)0) return 0;
    if (finder_win_skip_entry(e->attribute)) return 0;

    if (ec->w->view.n >= (uint16_t)FINDER_WIN_ICONS_MAX) {
        /* The cap bit. Keep counting so the caller can report HOW MANY entries
         * the window is not showing -- a silent truncation is the bug (Rule 2). */
        ec->dropped = (uint16_t)(ec->dropped + 1u);
        return 0;
    }

    kind = finder_win_kind_of(e->name83, e->attribute);
    idx  = finder_desk_add(&ec->w->view, kind, e->name83, 0, 0, 1u);
    if (idx < 0) { ec->dropped = (uint16_t)(ec->dropped + 1u); return 0; }

    /* A FOLDER's own cluster is the key its window (and its kind=4 view record)
     * live under; everything else keeps 0 (design F3.3). */
    if (kind == FINDER_ICON_FOLDER)
        finder_desk_set_cluster(&ec->w->view, idx, e->start_cluster);
    return 0;
}

/* Snap icon `i` to its row-major grid cell. Returns 1 when it moved. */
static int fw_snap(finder_window_t *w, int i, int cell)
{
    int16_t gx = 0, gy = 0;
    finder_win_grid_origin(w->view.bounds, cell, &gx, &gy);
    if (w->view.icons[i].x == gx && w->view.icons[i].y == gy) return 0;
    w->view.icons[i].x = gx;
    w->view.icons[i].y = gy;
    return 1;
}

finder_win_status_t finder_win_populate(finder_shell_t *sh, int slot)
{
    fw_enum_ctx_t ec;
    finder_window_t *w;
    int rc;

    if (!fw_slot_ok(sh, slot) || !sh->have_fs ||
        sh->fs.enumerate == (int (*)(void *, uint16_t, finder_enum_cb, void *))0)
        return FINDER_WIN_ERR_NULL;

    w = &sh->windows[slot];
    w->view.n  = 0u;             /* rebuild from scratch; the array is reused  */
    w->dropped = 0u;

    ec.w = w;
    ec.dropped = 0u;
    rc = sh->fs.enumerate(sh->fs.user, w->dir_start, fw_enum_cb, &ec);
    if (rc < 0) return FINDER_WIN_ERR_ENUM;

    w->dropped = ec.dropped;

    /* A freshly populated window is already CLEAN: row-major from the top-left
     * (design F3.3). Clean Up later restores exactly this arrangement. */
    for (int i = 0; i < (int)w->view.n; i++) (void)fw_snap(w, i, i);
    return FINDER_WIN_OK;
}

finder_win_status_t finder_win_open(finder_shell_t *sh, uint16_t dir_start,
                                    const char *name83, uint8_t is_root,
                                    int *out_slot, int *out_singleton)
{
    finder_window_t *w;
    rgn_rect_t frame, content;
    const finder_view_rec_t *vr;
    finder_win_status_t st;
    int slot;

    if (out_singleton != (int *)0) *out_singleton = 0;
    if (out_slot != (int *)0) *out_slot = -1;
    if (sh == (finder_shell_t *)0 || sh->wm == (WindowMgr *)0)
        return FINDER_WIN_ERR_NULL;

    /* SPATIAL SINGLETON (design F3.3): an already-open folder is BROUGHT
     * FORWARD, never duplicated, and is NOT re-enumerated -- the user's
     * arrangement inside it is the whole point of a spatial Finder. */
    slot = finder_win_find(sh, dir_start);
    if (slot >= 0) {
        SelectWindow(sh->wm, &sh->windows[slot].rec);
        fw_refresh_group_head(sh);
        if (out_slot != (int *)0) *out_slot = slot;
        if (out_singleton != (int *)0) *out_singleton = 1;
        return FINDER_WIN_OK;
    }

    for (slot = 0; slot < FINDER_WIN_MAX; slot++)
        if (!sh->windows[slot].open) break;
    if (slot >= FINDER_WIN_MAX) return FINDER_WIN_ERR_FULL;  /* fail loud       */

    w = &sh->windows[slot];

    /* The frame: the saved kind=4 origin when there is one, else the cascaded
     * default. The SIZE is always the default -- the locked 24-byte record has
     * no field for it (finder_desktop.h Sec 9, the stated deviation). */
    frame = finder_win_default_frame(slot);
    vr = finder_shell_find_view(sh, dir_start);
    if (vr != (const finder_view_rec_t *)0) {
        int16_t dx = (int16_t)((int)vr->x - (int)frame.left);
        int16_t dy = (int16_t)((int)vr->y - (int)frame.top);
        frame.left   = (int16_t)(frame.left + dx);
        frame.right  = (int16_t)(frame.right + dx);
        frame.top    = (int16_t)(frame.top + dy);
        frame.bottom = (int16_t)(frame.bottom + dy);
        w->view_bits = vr->view_bits;
    } else {
        w->view_bits = 0u;
    }
    content = CalcDocContentRect(frame);

    /* The WindowRecord is REUSED storage (carved once at launch), so zero it and
     * re-attach the regions before every build -- a stale region pointer or a
     * stale hilite byte from the previous tenant of this slot would be exactly
     * the kind of quietly-wrong state Rule 2 exists to prevent. */
    fw_zero(&w->rec, (uint32_t)sizeof w->rec);
    fw_rgn_attach(&w->rs);
    fw_rgn_attach(&w->rc);
    fw_rgn_attach(&w->ru);
    fw_rgn_attach(&w->rk);
    w->rec.strucRgn   = &w->rs.r;
    w->rec.contRgn    = &w->rc.r;
    w->rec.updateRgn  = &w->ru.r;
    w->rec.nextWindow = (WindowRecord *)0;

    w->open      = 1u;
    w->is_root   = is_root ? 1u : 0u;
    w->dir_start = dir_start;
    fw_copy83(w->name83, is_root ? "" : name83);
    finder_click_reset(&w->click);
    finder_desk_init(&w->view, w->icons, (uint16_t)FINDER_WIN_ICONS_MAX, content);

    NewDocumentWindow(sh->wm, &w->rec, frame, (int16_t)documentKind,
                      1 /* goAway -- design F3.3 "documentProc, goAway" */);
    SetWTitle(sh->wm, &w->rec, is_root ? FINDER_WIN_ROOT_TITLE : w->name83);

    /* The binding / demux rule (ADR-0013 Sec 3.1): FindWindow -> refCon ->
     * FlairApp*. This is what makes the window FINDER-OWNED, which is in turn
     * what makes its go-away box a DisposeWindow and not a tenant terminate
     * (design F2-3). */
    if (sh->app != (FlairApp *)0)
        w->rec.refCon = (int32_t)(uintptr_t)sh->app;
    fw_refresh_group_head(sh);

    st = finder_win_populate(sh, slot);
    if (st != FINDER_WIN_OK) {
        /* Enumeration failed: tear the window straight back down rather than
         * leaving an empty one claiming to show a directory (Rule 2). */
        DisposeWindow(sh->wm, &w->rec);
        w->open = 0u;
        fw_refresh_group_head(sh);
        return st;
    }

    if (out_slot != (int *)0) *out_slot = slot;
    return FINDER_WIN_OK;
}

finder_win_status_t finder_win_close(finder_shell_t *sh, int slot)
{
    finder_window_t *w;

    if (!fw_slot_ok(sh, slot)) return FINDER_WIN_ERR_NULL;
    w = &sh->windows[slot];

    /* View state first -- the window's frame is still live to read (F3.3
     * "Closing a folder window disposes the WindowRecord ... and saves view
     * state"). */
    (void)finder_shell_save_view(sh, slot);

    /* Design F2-3: a FINDER-owned window closes with DisposeWindow ONLY. The
     * D1-5 terminate path is for a GUEST tenant's last window; the Finder is
     * always resident and must survive its own window closing. */
    DisposeWindow(sh->wm, &w->rec);
    w->open   = 0u;
    w->view.n = 0u;
    fw_refresh_group_head(sh);
    return FINDER_WIN_OK;
}

finder_win_status_t finder_win_new_folder(finder_shell_t *sh, int slot,
                                          char *out_name83,
                                          uint16_t *out_parent)
{
    finder_window_t *w;
    fw_taken_t t;
    char name[FINDER_DESK_NAME_MAX];
    finder_win_status_t st;
    int rc;

    if (!fw_slot_ok(sh, slot) || !sh->have_fs ||
        sh->fs.mkdir == (int (*)(void *, const char *, uint16_t))0)
        return FINDER_WIN_ERR_NULL;

    w = &sh->windows[slot];
    if (out_parent != (uint16_t *)0) *out_parent = w->dir_start;

    /* The taken-set comes from the window's CURRENT listing, which is exactly
     * the directory's contents (populate ran on open and after every mkdir).
     * Reading it here rather than re-enumerating keeps New Folder a single
     * directory write, and the backend's own duplicate check
     * (fat12_mkdir -> FAT12_ERR_EXISTS) is the belt to this braces. */
    fw_zero(&t, (uint32_t)sizeof t);
    for (int i = 0; i < (int)w->view.n; i++) fw_taken_note(&t, w->view.icons[i].name);

    st = fw_taken_next(&t, name);
    if (st != FINDER_WIN_OK) return st;

    rc = sh->fs.mkdir(sh->fs.user, name, w->dir_start);
    if (rc != (int)FINDER_WIN_OK)
        return (rc == (int)FINDER_WIN_ERR_DIRFULL) ? FINDER_WIN_ERR_DIRFULL
                                                   : FINDER_WIN_ERR_MKDIR;

    if (out_name83 != (char *)0) fw_copy83(out_name83, name);

    /* Show it. The re-populate is a full re-enumeration, so the new folder
     * lands wherever the directory order puts it -- which is the truth of the
     * volume, not a guess (design F3-3). */
    return finder_win_populate(sh, slot);
}

int finder_win_cleanup(finder_shell_t *sh, int slot)
{
    finder_window_t *w;
    int moved = 0;
    int n;

    if (!fw_slot_ok(sh, slot)) return 0;
    w = &sh->windows[slot];
    n = (int)w->view.n;

#if defined(FINDER_WIN_MUT_CLEANUP_UNSORTED)
    /* MUTANT (design F3.3 "plain row-major"): cells are handed out in REVERSE
     * record order, so the snapped positions are a mirror of the correct ones
     * and the hand-authored expected coordinates go RED. NEVER in a real build. */
    for (int i = 0; i < n; i++) moved += fw_snap(w, i, n - 1 - i);
#else
    /* Row-major from the top-left, in RECORD order, selected-items-first
     * deliberately NOT applied (design F3.3: "selected-items-first is NOT
     * period -- plain row-major"). */
    for (int i = 0; i < n; i++) moved += fw_snap(w, i, i);
#endif
    return moved;
}

/* ===========================================================================
 * DAMAGE + PAINT  (the ordinary owned-window update path -- NOT the underlay)
 * ===========================================================================*/

void finder_win_invalidate_all(finder_shell_t *sh, int slot)
{
    if (!fw_slot_ok(sh, slot) || sh->wm == (WindowMgr *)0) return;
    WindowMgr_invalidate(sh->wm, &sh->windows[slot].rec,
                         region_get_bbox(sh->windows[slot].rec.contRgn));
}

void finder_win_invalidate(finder_shell_t *sh, int slot,
                           rgn_rect_t old_rect, rgn_rect_t new_rect)
{
    WindowPtr wp;
    if (!fw_slot_ok(sh, slot) || sh->wm == (WindowMgr *)0) return;
    wp = &sh->windows[slot].rec;
    WindowMgr_invalidate(sh->wm, wp, old_rect);
    if (new_rect.left != old_rect.left || new_rect.top != old_rect.top ||
        new_rect.right != old_rect.right || new_rect.bottom != old_rect.bottom)
        WindowMgr_invalidate(sh->wm, wp, new_rect);
}

void finder_win_paint(const finder_window_t *w, const bitmap_t *dst,
                      const region_t *clip)
{
    rgn_rect_t content;

    if (w == (const finder_window_t *)0 || !w->open) return;
    if (dst == (const bitmap_t *)0 || dst->base == (volatile uint8_t *)0) return;

    /* The content body. C-8: the tone is a semantic ROLE resolved at the ONE
     * policy seam; this TU owns no color literal. FLAIR_PART_CONTENT is the
     * window/content body the whole Toolbox already paints windows with. */
    content = region_get_bbox(w->rec.contRgn);
    blitter_fill_rect_clipped(dst, content,
                              flair_look_pixel_depth(dst->bpp,
                                                     (int)FLAIR_PART_CONTENT),
                              clip);

    /* The icons, through the ONE painter (finder_desktop.c). The window's view
     * is an ordinary finder_desk_t whose bounds are this content rect, so the
     * painter needs no window concept at all. */
    finder_desk_paint(&w->view, dst, clip);
}

/* ===========================================================================
 * 11. THE kind=4 VIEW RECORDS
 * ===========================================================================*/

const finder_view_rec_t *finder_shell_find_view(const finder_shell_t *sh,
                                                uint16_t dir_start)
{
    if (sh == (const finder_shell_t *)0) return (const finder_view_rec_t *)0;
    for (uint16_t i = 0u; i < sh->n_views; i++)
        if (sh->views[i].dir_start == dir_start) return &sh->views[i];
    return (const finder_view_rec_t *)0;
}

int finder_shell_note_view(finder_shell_t *sh, uint16_t dir_start,
                           const char *name83, uint16_t x, uint16_t y,
                           uint16_t view_bits)
{
    finder_view_rec_t *rec = (finder_view_rec_t *)0;
    const char *nm = (name83 != (const char *)0) ? name83 : "";
    int is_new = 0;

    if (sh == (finder_shell_t *)0) return 0;

    for (uint16_t i = 0u; i < sh->n_views; i++)
        if (sh->views[i].dir_start == dir_start) { rec = &sh->views[i]; break; }

    if (rec == (finder_view_rec_t *)0) {
        /* The view set is fixed-capacity like everything else here. A full set
         * simply does not remember the new folder's position -- the window
         * still opens, at its cascaded default. Losing a REMEMBERED POSITION is
         * a cache miss, not an invariant violation (design F1.3: icon positions
         * are "disposable caches of user preference"), so this is a quiet 0
         * rather than a panic. */
        if ((uint32_t)sh->n_views >= FINDER_DB_MAX_VIEWS) return 0;
        rec = &sh->views[sh->n_views];
        sh->n_views = (uint16_t)(sh->n_views + 1u);
        is_new = 1;
    } else if (rec->x == x && rec->y == y && rec->view_bits == view_bits &&
               fw_ieq(rec->name83, nm)) {
        return 0;                          /* already exactly this              */
    }

    rec->dir_start = dir_start;
    fw_copy83(rec->name83, nm);
    rec->x         = x;
    rec->y         = y;
    rec->view_bits = view_bits;
    (void)is_new;
    return 1;
}

uint16_t finder_shell_load_views(finder_shell_t *sh, const uint8_t *buf,
                                 uint32_t len)
{
    finder_view_rec_t rec;
    uint32_t n_rec;
    uint16_t took = 0u;

    if (sh == (finder_shell_t *)0) return 0u;
    sh->n_views = 0u;
    if (buf == (const uint8_t *)0 || len < FINDER_DB_HEADER_SIZE) return 0u;

    /* A corrupt image contributes NO view records and every window then opens
     * at its cascaded default -- design F1.3's regenerate-default posture, and
     * the reason this returns a count rather than a status: the caller's loud
     * report about the DB itself already happened at mount. The DECODE goes
     * through finder_desk_db_find_view so there is exactly ONE place that knows
     * the record's field offsets, and its validation gate short-circuits the
     * whole walk on the first call if the image is bad. */
    n_rec = (uint32_t)buf[6] | ((uint32_t)buf[7] << 8);
    for (uint32_t i = 0u; i < n_rec; i++) {
        uint32_t off = FINDER_DB_HEADER_SIZE + i * FINDER_DB_RECORD_SIZE;
        uint16_t key;

        if (off + FINDER_DB_RECORD_SIZE > len) break;
        if (buf[off] != (uint8_t)FINDER_DB_KIND_VIEW) continue;
        key = (uint16_t)((uint16_t)buf[off + 2] | ((uint16_t)buf[off + 3] << 8));

        if (finder_desk_db_find_view(buf, len, key, &rec) != 1) return took;
        if (finder_shell_find_view(sh, key) != (const finder_view_rec_t *)0)
            continue;                       /* first record for a key wins      */
        if ((uint32_t)sh->n_views >= FINDER_DB_MAX_VIEWS) break;
        sh->views[sh->n_views] = rec;
        sh->n_views = (uint16_t)(sh->n_views + 1u);
        took = (uint16_t)(took + 1u);
    }
    return took;
}

int finder_shell_save_view(finder_shell_t *sh, int slot)
{
    finder_window_t *w;
    rgn_rect_t frame;

    if (!fw_slot_ok(sh, slot)) return 0;
    w = &sh->windows[slot];
    frame = WindowFrameRect(&w->rec);
    if (frame.left < 0) frame.left = 0;
    if (frame.top < 0) frame.top = 0;
    return finder_shell_note_view(sh, w->dir_start, w->name83,
                                  (uint16_t)frame.left, (uint16_t)frame.top,
                                  w->view_bits);
}

/* ===========================================================================
 * THE COMMAND EXECUTION HOOK  (finder_cmd.h FinderCtx.exec)
 * ===========================================================================*/

static void fw_outcome(finder_shell_t *sh, finder_cmd_id id,
                       finder_win_status_t st, int slot)
{
    fw_zero(&sh->last, (uint32_t)sizeof sh->last);
    sh->last.valid  = 1u;
    sh->last.id     = id;
    sh->last.status = st;
    sh->last.slot   = (int16_t)slot;
}

static void fw_exec(void *shell, finder_cmd_id id)
{
    finder_shell_t *sh = (finder_shell_t *)shell;
    int slot;

    if (sh == (finder_shell_t *)0) return;
    slot = finder_win_front_slot(sh);

    switch (id) {
    case FCMD_NEW_FOLDER: {
        char     name[FINDER_DESK_NAME_MAX];
        uint16_t parent = 0u;
        finder_win_status_t st;

        if (slot < 0) { fw_outcome(sh, id, FINDER_WIN_ERR_NULL, -1); return; }
        st = finder_win_new_folder(sh, slot, name, &parent);
        fw_outcome(sh, id, st, slot);
        sh->last.parent = parent;
        if (st == FINDER_WIN_OK) fw_copy83(sh->last.name83, name);
        return;
    }
    case FCMD_CLEANUP: {
        int moved;
        if (slot < 0) { fw_outcome(sh, id, FINDER_WIN_ERR_NULL, -1); return; }
        moved = finder_win_cleanup(sh, slot);
        fw_outcome(sh, id, FINDER_WIN_OK, slot);
        sh->last.moved = (int16_t)moved;
        return;
    }
    case FCMD_CLOSE_WINDOW: {
        finder_win_status_t st;
        if (slot < 0) { fw_outcome(sh, id, FINDER_WIN_ERR_NULL, -1); return; }
        st = finder_win_close(sh, slot);
        fw_outcome(sh, id, st, slot);
        return;
    }
    case FCMD_OPEN: {
        /* Open the SELECTED folder of the front window (design F4.4's
         * HAS_SELECTION predicate has already passed, or dispatch would have
         * reported FINDER-CMD-DISABLED). The first selected FOLDER wins; a
         * selected document has no opener until the tdnl.14 launch slice, so it
         * reports a refusal rather than pretending. */
        int chosen = -1;
        if (slot < 0) { fw_outcome(sh, id, FINDER_WIN_ERR_NULL, -1); return; }
        for (int i = 0; i < (int)sh->windows[slot].view.n; i++) {
            const finder_desk_icon_t *ic = &sh->windows[slot].view.icons[i];
            if (ic->selected && ic->kind == (uint8_t)FINDER_ICON_FOLDER) {
                chosen = i;
                break;
            }
        }
        if (chosen < 0) { fw_outcome(sh, id, FINDER_WIN_ERR_NULL, slot); return; }
        {
            const finder_desk_icon_t *ic = &sh->windows[slot].view.icons[chosen];
            int nslot = -1, singleton = 0;
            finder_win_status_t st = finder_win_open(sh, ic->dir_start, ic->name,
                                                     0u, &nslot, &singleton);
            fw_outcome(sh, id, st, nslot);
            sh->last.singleton = (uint8_t)(singleton ? 1 : 0);
            fw_copy83(sh->last.name83, ic->name);
        }
        return;
    }
    default:
        /* Every other row is another slice's (tdnl.11 file ops, tdnl.12 menu
         * modes, tdnl.14 launch). Leaving `last` invalid makes kmain fall back
         * to the table's own FINDER-NYI stub report -- honest, and impossible
         * to confuse with a command that ran (Rule 2). */
        return;
    }
}

const finder_cmd_outcome_t *finder_shell_take_outcome(finder_shell_t *sh)
{
    if (sh == (finder_shell_t *)0 || !sh->last.valid)
        return (const finder_cmd_outcome_t *)0;
    sh->last.valid = 0u;
    return &sh->last;
}

void finder_shell_sync_ctx(finder_shell_t *sh)
{
    int slot;
    if (sh == (finder_shell_t *)0 || sh->ctx == (FinderCtx *)0) return;

    slot = finder_win_front_slot(sh);
    sh->ctx->front_is_diskwin = (uint8_t)((slot >= 0) ? 1 : 0);
    sh->ctx->selection_count  =
        (slot >= 0) ? finder_desk_selection_count(&sh->windows[slot].view)
                    : finder_desk_selection_count(&sh->desk);
}

void finder_shell_bind_ctx(finder_shell_t *sh, FinderCtx *ctx)
{
    if (sh == (finder_shell_t *)0 || ctx == (FinderCtx *)0) return;
    sh->ctx     = ctx;
    ctx->exec   = fw_exec;
    ctx->shell  = (void *)sh;
    finder_shell_sync_ctx(sh);
}

void finder_shell_bind_fs(finder_shell_t *sh, const finder_fs_t *fs)
{
    if (sh == (finder_shell_t *)0 || fs == (const finder_fs_t *)0) return;
    sh->fs      = *fs;
    sh->have_fs = 1u;
}

/* ===========================================================================
 * 8. THE TENANT
 * ===========================================================================*/

static int finder_shell_open(FlairApp *self, const FlairLaunchParams *lp)
{
    finder_shell_t *sh;

    if (self == (FlairApp *)0 || lp == (const FlairLaunchParams *)0) return 1;

    /* ONE allocation, RECORDS arena (FLAIR_CLASS_HANDLE, ADR-0013 AC-2): the
     * icon arrays, the WindowRecords and every region pool live in the block
     * the shell reads during teardown, so tenant death survives a scribbled
     * DATA arena (BC-6). A budget too small to hold it is a LAUNCH FAILURE, not
     * a half-built Finder (Rule 2 / BC-5). */
    sh = (finder_shell_t *)flair_alloc(&self->records_arena, FLAIR_CLASS_HANDLE,
                                       (uint32_t)sizeof(finder_shell_t));
    if (sh == (finder_shell_t *)0) return 1;
    fw_zero(sh, (uint32_t)sizeof *sh);

    sh->wm      = lp->wm;
    sh->surface = lp->surface;
    sh->app     = self;

    /* The R3.2 desktop model -- the SAME two calls in the SAME order R3.2's
     * open() made, so the seeded icons (and therefore the boot frame) are
     * unchanged by the tenant having moved here. */
    if (finder_desk_tenant_build(&sh->desk, sh->desk_icons,
                                 (uint16_t)FINDER_DESK_MAX_ICONS, lp) != 0)
        return 1;
    for (int i = 0; i < FINDER_WIN_MAX; i++)
        finder_click_reset(&sh->windows[i].click);

    self->userData = (void *)sh;
    /* No window at launch: a disk window opens ONLY on a double-click. So
     * self->windows stays NULL, FlairProcess_launch skips its SelectWindow, and
     * the boot foreground + every locked band-2 gate are exactly as before. */
    return 0;
}

static void finder_shell_event(FlairApp *self, const EventRecord *ev)
{
    finder_shell_t *sh;

    if (self == (FlairApp *)0 || ev == (const EventRecord *)0) return;
    sh = (finder_shell_t *)self->userData;
    if (sh == (finder_shell_t *)0) return;

    switch (ev->what) {
    case updateEvt: {
        /* The exposure-survival repaint, the ref_tenant.c discipline verbatim:
         * reconstruct the content clipped to contRgn INTERSECT updateRgn -- the
         * newly-exposed damage only, never the whole window. */
        WindowRecord *w = (WindowRecord *)(uintptr_t)ev->message;
        int slot = finder_win_slot_of(sh, w);
        if (slot < 0) return;
        region_op(&sh->windows[slot].rk.r,
                  sh->windows[slot].rec.contRgn,
                  sh->windows[slot].rec.updateRgn, RGN_OP_INTERSECT);
        finder_win_paint(&sh->windows[slot], sh->surface,
                         &sh->windows[slot].rk.r);
        return;
    }

    case keyDown:
        /* Handled by the live pump's Cmd-chord arm (kmain), which owns the
         * modifier decode and the serial report. Nothing to do here: a second
         * handler would be a second dispatch path, which F4-4 forbids. */
        return;

    case mouseDown:
        /* DELIBERATELY IGNORED. A content click may start a marquee or an icon
         * drag, both of which must be tracked to mouseUp through WaitNextEvent
         * -- a live-pump concern that lives in kmain beside flair_live_do_drag,
         * exactly like every other tracked gesture. Acting here as well would
         * double-handle the click. */
        return;

    default:
        return;
    }
}

const FlairAppProcs finder_shell_procs = {
    finder_shell_open,
    finder_shell_event,
    (void (*)(FlairApp *))0,        /* idle  -- nothing to do                  */
    (void (*)(FlairApp *))0         /* close -- always resident (design F2-1)  */
};

finder_shell_t *finder_shell_of(FlairApp *app)
{
    if (app == (FlairApp *)0) return (finder_shell_t *)0;
    if (app->magic != (uint32_t)FLAIR_APP_MAGIC) return (finder_shell_t *)0;
    return (finder_shell_t *)app->userData;
}

finder_desk_t *finder_shell_desk(finder_shell_t *sh)
{
    if (sh == (finder_shell_t *)0) return (finder_desk_t *)0;
    return &sh->desk;
}
