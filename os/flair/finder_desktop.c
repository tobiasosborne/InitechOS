/*
 * os/flair/finder_desktop.c -- the R3 Finder DESKTOP MANAGER (THE ARTIFACT).
 *
 * beads: initech-tdnl.9 (GUI remediation R3.2 "DesktopMgr").
 * Ref:   os/flair/finder_desktop.h (the contract, the five rules, the serial
 *          marker list, and every stated deviation from the design document);
 *        docs/design/GUI-remediation-R3-finder-design.md F1.1/F1.2/F1.3,
 *          F2.2/F2.3, F2-4/F2-5;
 *        os/flair/finder_icon.c (the per-pixel clipped strike blit this file
 *          reuses verbatim and imitates for the label glyphs);
 *        os/flair/desktop.c :: desktop_px (the PART -> pixel idiom);
 *        os/flair/flair_look.h (the ONE policy seam -- C-8).
 *
 * Artifact C per ADR-0002: freestanding, no allocation outside the caller's
 * arenas, no libc, deterministic, ASCII-only. The three mutation knobs are
 * host-oracle builds only (Rule 6) and are never compiled into a real build.
 */
#include "finder_desktop.h"

#include "finder_icon.h"        /* finder_icon_draw (clipped, mask-honouring)  */
#include "desk_icons.h"         /* the LOCKED 32x32 desktop strikes            */
#include "finder_icons.h"       /* the LOCKED 32x32 disk-window strikes (R3.3) */
#include "flair_look.h"         /* flair_look_pixel_depth -- the ONE seam      */
#include "blitter.h"            /* blitter_fill_rect_clipped (label band)      */
#include "geneva9.h"            /* geneva9_glyph / geneva9_advance_w           */

/* WHY NOT os/flair/text.h: text_measure/text_draw are static inline and branch
 * on a RUNTIME font selector, so including text.h drags the Chicago strike into
 * this object as well as Geneva -- ~1.4 KiB of dead rodata in a kernel whose
 * disk window is measured in sectors. text_draw is unusable here anyway (it
 * routes through surface_blit, which has no region clip and paints a background
 * cell per glyph). fd_label_width below is text_measure's Geneva arm, arithmetic
 * for arithmetic: the sum of geneva9_advance_w over the string. */

/* The header's FINDER_LABEL_ROWS must track the real strike height; a silent
 * drift would put the label band and the glyphs on different rows. */
#if FINDER_LABEL_ROWS != GENEVA9_CELL_H
#error "FINDER_LABEL_ROWS must equal GENEVA9_CELL_H (spec/assets/geneva9.h)"
#endif

/* ---------------------------------------------------------------------------
 * Fail-loud, the desktop.c / window.c idiom: abort hosted, deterministic hang
 * in-kernel. Kept local so this TU dual-compiles with only FLAIR/spec headers.
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort -- hosted only */
#  define FDESK_PANIC(msg)  abort()
#else
#  define FDESK_PANIC(msg)  do { for (;;) { } } while (0)
#endif

/* ===========================================================================
 * Small helpers (no libc, Law 3).
 * ===========================================================================*/

static int16_t fd_min16(int16_t a, int16_t b) { return a < b ? a : b; }
static int16_t fd_max16(int16_t a, int16_t b) { return a > b ? a : b; }

static int fd_abs(int v) { return v < 0 ? -v : v; }

/* text_measure(FONT_GENEVA9, s) recomputed locally -- see the include note. */
static int fd_label_width(const char *s)
{
    int w = 0;
    if (s == (const char *)0) return 0;
    while (*s != '\0') {
        w += (int)geneva9_advance_w((int)(unsigned char)*s);
        s++;
    }
    return w;
}

static void fd_copy_name(char *dst, const char *src)
{
    uint32_t i = 0u;
    if (src != (const char *)0) {
        while (i < (uint32_t)(FINDER_DESK_NAME_MAX - 1) && src[i] != '\0') {
            dst[i] = src[i];
            i++;
        }
    }
    while (i < (uint32_t)FINDER_DESK_NAME_MAX) {
        dst[i] = '\0';
        i++;
    }
}

static int fd_valid(const finder_desk_t *fd, int idx)
{
    return fd != (const finder_desk_t *)0 &&
           fd->icons != (finder_desk_icon_t *)0 &&
           idx >= 0 && idx < (int)fd->n;
}

/* The LOCKED strike for a record kind. desk_icons.h / finder_icons.h declare
 * the strikes as file-static const data, so each including TU owns its own copy
 * -- never compare these pointers across translation units.
 *
 * WHY THE MAP LIVES HERE AND COVERS BOTH SURFACES (bead initech-tdnl.10):
 * finder_desk_paint is the ONE icon painter -- a disk window's content is drawn
 * by calling it with the WINDOW's finder_desk_t (whose bounds are the content
 * rect) instead of the desktop's. That reuse is the whole reason the R3.2
 * trackers work unchanged inside a window (design F2-5: the trackers are pure
 * functions of numbers, and a surface is just a different `bounds`). One
 * painter therefore needs one kind->strike map, and this is it. Unknown kinds
 * return NULL and finder_desk_paint SKIPS them -- fail-safe, never a garbage
 * blit (Rule 2). */
static const FLAIRDeskIcon *fd_strike(uint8_t kind)
{
    if (kind == (uint8_t)FINDER_ICON_VOLUME) return &FLAIR_DESK_ICON_VOLUME;
    if (kind == (uint8_t)FINDER_ICON_TRASH)  return &FLAIR_DESK_ICON_TRASH;
    if (kind == (uint8_t)FINDER_ICON_FOLDER) return &FLAIR_FINDER_ICON_FOLDER;
    if (kind == (uint8_t)FINDER_ICON_FILE)   return &FLAIR_FINDER_ICON_DOC;
    if (kind == (uint8_t)FINDER_ICON_APP)    return &FLAIR_FINDER_ICON_APP;
    return (const FLAIRDeskIcon *)0;
}

/* ===========================================================================
 * 3. LIFECYCLE + RECORD BUILDING
 * ===========================================================================*/

void finder_desk_init(finder_desk_t *fd, finder_desk_icon_t *storage,
                      uint16_t cap, rgn_rect_t usable)
{
    if (fd == (finder_desk_t *)0 || storage == (finder_desk_icon_t *)0 ||
        cap == 0u) {
        FDESK_PANIC("finder_desk_init: NULL/zero storage");
        return;
    }
    fd->icons  = storage;
    fd->n      = 0u;
    fd->cap    = cap;
    fd->bounds = usable;
}

int finder_desk_add(finder_desk_t *fd, finder_icon_kind_t kind,
                    const char *name, int16_t x, int16_t y, uint8_t draggable)
{
    finder_desk_icon_t *ic;
    int idx;

    if (fd == (finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0) {
        return -1;
    }
    if (fd->n >= fd->cap) {
        return -1;              /* FULL -- the caller fails loud (Rule 2)      */
    }
    idx = (int)fd->n;
    ic  = &fd->icons[idx];
    ic->kind      = (uint8_t)kind;
    ic->selected  = 0u;
    ic->draggable = draggable ? 1u : 0u;
    ic->reserved  = 0u;
    ic->x         = x;
    ic->y         = y;
    ic->dir_start = 0u;     /* R3.3: set by finder_desk_set_cluster if needed  */
    fd_copy_name(ic->name, name);
    fd->n = (uint16_t)(fd->n + 1u);
    return idx;
}

int finder_desk_seed_defaults(finder_desk_t *fd)
{
    int16_t sx, vy, ty;

    if (fd == (finder_desk_t *)0) return -1;

    sx = (int16_t)(fd->bounds.right - FINDER_DESK_MARGIN_R - FINDER_ICON_DIM);
    vy = (int16_t)(fd->bounds.top + FINDER_DESK_MARGIN_T);
    ty = (int16_t)(fd->bounds.bottom - FINDER_DESK_TRASH_BOTTOM);

    /* Record order IS z order (design F1.2), and it is also the DB record
     * order: volume first, Trash second. The Trash is NOT draggable in V1
     * (design F1.1: period Trash is a fixed-position target). */
    if (finder_desk_add(fd, FINDER_ICON_VOLUME, "", sx, vy, 1u) < 0) return -1;
    if (finder_desk_add(fd, FINDER_ICON_TRASH, "Trash", sx, ty, 0u) < 0) return -1;
    return 0;
}

void finder_desk_set_name(finder_desk_t *fd, int idx, const char *name)
{
    if (!fd_valid(fd, idx)) return;
    fd_copy_name(fd->icons[idx].name, name);
}

void finder_desk_set_cluster(finder_desk_t *fd, int idx, uint16_t cluster)
{
    if (!fd_valid(fd, idx)) return;
    fd->icons[idx].dir_start = cluster;
}

int finder_desk_find_kind(const finder_desk_t *fd, finder_icon_kind_t kind)
{
    if (fd == (const finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0)
        return -1;
    for (int i = 0; i < (int)fd->n; i++) {
        if (fd->icons[i].kind == (uint8_t)kind) return i;
    }
    return -1;
}

/* ===========================================================================
 * 4. GEOMETRY
 * ===========================================================================*/

static rgn_rect_t fd_empty_rect(void)
{
    rgn_rect_t r;
    r.top = 0; r.left = 0; r.bottom = 0; r.right = 0;
    return r;
}

rgn_rect_t finder_desk_sprite_rect(const finder_desk_t *fd, int idx)
{
    rgn_rect_t r;
    if (!fd_valid(fd, idx)) return fd_empty_rect();
    r.left   = fd->icons[idx].x;
    r.top    = fd->icons[idx].y;
    r.right  = (int16_t)(r.left + FINDER_ICON_DIM);
    r.bottom = (int16_t)(r.top + FINDER_ICON_DIM);
    return r;
}

rgn_rect_t finder_desk_label_rect(const finder_desk_t *fd, int idx)
{
    rgn_rect_t r;
    int w;
    int cx;
    int left;

    if (!fd_valid(fd, idx)) return fd_empty_rect();

    w  = fd_label_width(fd->icons[idx].name) + 2 * FINDER_LABEL_PAD_X;
    cx = (int)fd->icons[idx].x + FINDER_ICON_DIM / 2;
    /* Centered under the sprite; integer division matches text_center_in. */
    left = cx - w / 2;

    /* Clamp into the usable desktop so a long label never runs off the edge. */
    if (left + w > (int)fd->bounds.right) left = (int)fd->bounds.right - w;
    if (left < (int)fd->bounds.left)      left = (int)fd->bounds.left;

    r.left   = (int16_t)left;
    r.right  = (int16_t)(left + w);
    r.top    = (int16_t)(fd->icons[idx].y + FINDER_ICON_DIM + FINDER_LABEL_GAP);
    r.bottom = (int16_t)(r.top + FINDER_LABEL_H);
    return r;
}

rgn_rect_t finder_desk_cell_rect(const finder_desk_t *fd, int idx)
{
    rgn_rect_t s, l, u;
    if (!fd_valid(fd, idx)) return fd_empty_rect();
    s = finder_desk_sprite_rect(fd, idx);
    l = finder_desk_label_rect(fd, idx);
    u.left   = fd_min16(s.left, l.left);
    u.top    = fd_min16(s.top, l.top);
    u.right  = fd_max16(s.right, l.right);
    u.bottom = fd_max16(s.bottom, l.bottom);
    return u;
}

int finder_rect_contains(rgn_rect_t r, int16_t h, int16_t v)
{
    return h >= r.left && h < r.right && v >= r.top && v < r.bottom;
}

int finder_rect_intersects(rgn_rect_t a, rgn_rect_t b)
{
    if (a.right <= a.left || a.bottom <= a.top) return 0;
    if (b.right <= b.left || b.bottom <= b.top) return 0;
    return a.left < b.right && b.left < a.right &&
           a.top < b.bottom && b.top < a.bottom;
}

int finder_desk_hit(const finder_desk_t *fd, int16_t h, int16_t v)
{
    if (fd == (const finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0)
        return -1;
    /* TOPMOST WINS: later record == drawn later == on top.
     * The target is the sprite rect UNION the label rect -- NOT their bounding
     * box (design F1.2: "point -> icon whose sprite UNION label rect contains
     * it"). The bbox would swallow the two dead corners beside the centered
     * label, so a click on bare desktop just under an icon's shoulder would
     * select it. finder_desk_cell_rect (the bbox) is for DAMAGE only. */
    for (int i = (int)fd->n - 1; i >= 0; i--) {
        if (finder_rect_contains(finder_desk_sprite_rect(fd, i), h, v) ||
            finder_rect_contains(finder_desk_label_rect(fd, i), h, v))
            return i;
    }
    return -1;
}

/* ===========================================================================
 * 5. SELECTION
 * ===========================================================================*/

void finder_desk_deselect_all(finder_desk_t *fd)
{
    if (fd == (finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0) return;
    for (int i = 0; i < (int)fd->n; i++) fd->icons[i].selected = 0u;
}

void finder_desk_select_only(finder_desk_t *fd, int idx)
{
    if (fd == (finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0) return;
    for (int i = 0; i < (int)fd->n; i++)
        fd->icons[i].selected = (i == idx) ? 1u : 0u;
}

void finder_desk_select_extend(finder_desk_t *fd, int idx)
{
    if (!fd_valid(fd, idx)) return;
    fd->icons[idx].selected = fd->icons[idx].selected ? 0u : 1u;
}

uint16_t finder_desk_selection_count(const finder_desk_t *fd)
{
    uint16_t n = 0u;
    if (fd == (const finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0)
        return 0u;
    for (int i = 0; i < (int)fd->n; i++) if (fd->icons[i].selected) n++;
    return n;
}

/* ===========================================================================
 * 6. THE RUBBER BAND
 * ===========================================================================*/

rgn_rect_t finder_band_rect(int16_t h0, int16_t v0, int16_t h1, int16_t v1)
{
    rgn_rect_t r;
    r.left = fd_min16(h0, h1);
    r.top  = fd_min16(v0, v1);
#if defined(FINDER_DESK_MUT_MARQUEE_OFFBYONE)
    /* MUTANT (Rule 6): drop the half-open +1, so the band excludes its own
     * right/bottom edge pixel. An icon whose left/top edge is exactly under the
     * release point is then MISSED and the hand-authored selected sets go RED.
     * NEVER in a real build. */
    r.right  = fd_max16(h0, h1);
    r.bottom = fd_max16(v0, v1);
#else
    r.right  = (int16_t)(fd_max16(h0, h1) + 1);
    r.bottom = (int16_t)(fd_max16(v0, v1) + 1);
#endif
    return r;
}

uint16_t finder_desk_marquee_select(finder_desk_t *fd, rgn_rect_t band)
{
    uint16_t n = 0u;
    if (fd == (finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0)
        return 0u;
    for (int i = 0; i < (int)fd->n; i++) {
        /* Same sprite-UNION-label target the hit test uses, for the same
         * reason: a band that only crosses the dead corner beside a centered
         * label must not select the icon. */
        int in = finder_rect_intersects(finder_desk_sprite_rect(fd, i), band) ||
                 finder_rect_intersects(finder_desk_label_rect(fd, i), band);
        fd->icons[i].selected = in ? 1u : 0u;
        if (in) n++;
    }
    return n;
}

/* ===========================================================================
 * 7. DOUBLE-CLICK SYNTHESIS
 * ===========================================================================*/

void finder_click_reset(finder_click_track_t *t)
{
    if (t == (finder_click_track_t *)0) return;
    t->last_when   = 0u;
    t->last_h      = 0;
    t->last_v      = 0;
    t->last_target = -1;
    t->armed       = 0u;
    t->reserved    = 0u;
}

finder_click_kind_t finder_click_classify(finder_click_track_t *t,
                                          int16_t target,
                                          int16_t h, int16_t v, uint32_t when)
{
    int is_double = 0;

    if (t == (finder_click_track_t *)0) return FINDER_CLICK_SINGLE;

#if defined(FINDER_DESK_MUT_DBLTICK_OFF)
    /* MUTANT (Rule 6): the interval test never succeeds, so a double-click is
     * delivered as two singles -- the hand-authored event stream goes RED (and
     * live, a volume never opens). NEVER in a real build. */
    (void)h; (void)v; (void)fd_abs;
#else
    if (t->armed && target >= 0 && target == t->last_target &&
        when >= t->last_when &&
        (when - t->last_when) <= (uint32_t)FINDER_DBLCLICK_TICKS &&
        fd_abs((int)h - (int)t->last_h) <= FINDER_DBLCLICK_SLOP &&
        fd_abs((int)v - (int)t->last_v) <= FINDER_DBLCLICK_SLOP) {
        is_double = 1;
    }
#endif

    if (is_double) {
        /* A double DISARMS: click 3 of a triple starts a fresh pair. */
        t->armed       = 0u;
        t->last_target = -1;
        return FINDER_CLICK_DOUBLE;
    }

    t->last_when   = when;
    t->last_h      = h;
    t->last_v      = v;
    t->last_target = target;
    t->armed       = 1u;
    return FINDER_CLICK_SINGLE;
}

/* ===========================================================================
 * 8. ICON DRAG
 * ===========================================================================*/

/* Clamp the proposed sprite origin so the WHOLE cell (sprite UNION label) stays
 * inside fd->bounds. The label is horizontally centered and already clamped by
 * finder_desk_label_rect, so the sprite clamp only has to keep the sprite and
 * the label BAND vertically on the desktop. */
static void fd_clamp_origin(const finder_desk_t *fd, int16_t *x, int16_t *y)
{
    int nx = (int)*x;
    int ny = (int)*y;
    int max_x = (int)fd->bounds.right - FINDER_ICON_DIM;
    int max_y = (int)fd->bounds.bottom - FINDER_CELL_H;

    if (nx > max_x) nx = max_x;
    if (nx < (int)fd->bounds.left) nx = (int)fd->bounds.left;
    if (ny > max_y) ny = max_y;
    if (ny < (int)fd->bounds.top) ny = (int)fd->bounds.top;

    *x = (int16_t)nx;
    *y = (int16_t)ny;
}

rgn_rect_t finder_desk_drag_outline(const finder_desk_t *fd, int idx,
                                    int16_t dh, int16_t dv)
{
    rgn_rect_t cell;
    int16_t nx, ny;
    int16_t adh, adv;

    if (!fd_valid(fd, idx)) return fd_empty_rect();

    nx = (int16_t)(fd->icons[idx].x + dh);
    ny = (int16_t)(fd->icons[idx].y + dv);
    fd_clamp_origin(fd, &nx, &ny);
    adh = (int16_t)(nx - fd->icons[idx].x);
    adv = (int16_t)(ny - fd->icons[idx].y);

    cell = finder_desk_cell_rect(fd, idx);
    cell.left   = (int16_t)(cell.left + adh);
    cell.right  = (int16_t)(cell.right + adh);
    cell.top    = (int16_t)(cell.top + adv);
    cell.bottom = (int16_t)(cell.bottom + adv);
    return cell;
}

finder_drop_t finder_desk_drag_commit(finder_desk_t *fd, int idx,
                                      int16_t dh, int16_t dv,
                                      rgn_rect_t *out_old, rgn_rect_t *out_new)
{
    rgn_rect_t old_cell;
    int16_t nx, ny;

    if (!fd_valid(fd, idx)) return FINDER_DROP_NOOP;

    old_cell = finder_desk_cell_rect(fd, idx);
    if (out_old != (rgn_rect_t *)0) *out_old = old_cell;
    if (out_new != (rgn_rect_t *)0) *out_new = old_cell;

    if (!fd->icons[idx].draggable) {
        /* Fixed-position icon (the Trash, design F1.1): the drop is refused and
         * the save-under outline is simply erased. */
        return FINDER_DROP_REVERT;
    }
    if (dh == 0 && dv == 0) return FINDER_DROP_NOOP;

    nx = (int16_t)(fd->icons[idx].x + dh);
    ny = (int16_t)(fd->icons[idx].y + dv);
    fd_clamp_origin(fd, &nx, &ny);
    if (nx == fd->icons[idx].x && ny == fd->icons[idx].y) return FINDER_DROP_NOOP;

    fd->icons[idx].x = nx;
    fd->icons[idx].y = ny;
    if (out_new != (rgn_rect_t *)0) *out_new = finder_desk_cell_rect(fd, idx);
    return FINDER_DROP_MOVED;
}

/* ===========================================================================
 * 9. THE DESKTOP.DB RECORD CODEC (LOCKED layout -- see the header)
 * ===========================================================================*/

static void fd_put_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static uint16_t fd_get_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

finder_db_status_t finder_desk_db_validate(const uint8_t *buf, uint32_t len)
{
    uint16_t n;

    if (buf == (const uint8_t *)0 || len < FINDER_DB_HEADER_SIZE)
        return FINDER_DB_ERR_LEN;
    if (buf[0] != (uint8_t)'I' || buf[1] != (uint8_t)'D' ||
        buf[2] != (uint8_t)'B' || buf[3] != (uint8_t)'1')
        return FINDER_DB_ERR_MAGIC;
    if (fd_get_le16(buf + 4) != (uint16_t)FINDER_DB_VERSION)
        return FINDER_DB_ERR_VER;
    n = fd_get_le16(buf + 6);
    if ((uint32_t)n > FINDER_DB_MAX_RECORDS)
        return FINDER_DB_ERR_COUNT;
    if (FINDER_DB_HEADER_SIZE + (uint32_t)n * FINDER_DB_RECORD_SIZE != len)
        return FINDER_DB_ERR_LEN;
    return FINDER_DB_OK;
}

/* One 24-byte record, positionally assembled. `name` may be NULL (the field is
 * then all zero). Every byte of the record is written -- there is no padding
 * ambiguity and no reliance on the caller's buffer being pre-zeroed. */
static void fd_put_record(uint8_t *r, uint8_t kind, uint16_t dir_start,
                          const char *name, uint16_t x, uint16_t y,
                          uint16_t view_bits)
{
    uint32_t k;

    for (k = 0u; k < FINDER_DB_RECORD_SIZE; k++) r[k] = 0u;
    r[0] = kind;                        /* +0  kind                            */
    r[1] = 0u;                          /* +1  flags                           */
    fd_put_le16(r + 2, dir_start);      /* +2  dir_start (0 == root)           */
    if (name != (const char *)0) {
        for (k = 0u; k < 13u; k++) {    /* +4  name83[13]                      */
            char c = name[k];
            r[4 + k] = (uint8_t)c;
            if (c == '\0') break;       /* the rest is already zero            */
        }
    }
    fd_put_le16(r + 17, x);             /* +17 grid_x (pixels; see the header) */
    fd_put_le16(r + 19, y);             /* +19 grid_y                          */
    fd_put_le16(r + 21, view_bits);     /* +21 view_bits                       */
    r[23] = 0u;                         /* +23 pad                             */
}

uint32_t finder_desk_db_encode_all(const finder_desk_t *fd,
                                   const finder_view_rec_t *views,
                                   uint16_t n_views,
                                   uint8_t *buf, uint32_t cap)
{
    uint32_t total;
    uint32_t need;
    uint32_t off;

    if (fd == (const finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0 ||
        buf == (uint8_t *)0)
        return 0u;
    if (n_views != 0u && views == (const finder_view_rec_t *)0)
        return 0u;

    total = (uint32_t)fd->n + (uint32_t)n_views;
    if (total > FINDER_DB_MAX_RECORDS) return 0u;   /* caller fails loud       */

    need = FINDER_DB_HEADER_SIZE + total * FINDER_DB_RECORD_SIZE;
    if (cap < need) return 0u;

    buf[0] = (uint8_t)'I';
    buf[1] = (uint8_t)'D';
    buf[2] = (uint8_t)'B';
    buf[3] = (uint8_t)'1';
    fd_put_le16(buf + 4, (uint16_t)FINDER_DB_VERSION);
    fd_put_le16(buf + 6, (uint16_t)total);

    /* Icons first, in record order (which IS z order), then the view records --
     * the ONE deterministic ordering (Rule 11). */
    off = FINDER_DB_HEADER_SIZE;
    for (int i = 0; i < (int)fd->n; i++) {
        const finder_desk_icon_t *ic = &fd->icons[i];
        int16_t px = ic->x;
        int16_t py = ic->y;

        /* Origins are clamped >= 0 so the unsigned position fields are exact. */
        if (px < 0) px = 0;
        if (py < 0) py = 0;

        fd_put_record(buf + off, ic->kind, 0u, ic->name,
                      (uint16_t)px, (uint16_t)py, 0u);
        off += FINDER_DB_RECORD_SIZE;
    }
    for (uint16_t v = 0u; v < n_views; v++) {
        fd_put_record(buf + off, (uint8_t)FINDER_DB_KIND_VIEW,
                      views[v].dir_start, views[v].name83,
                      views[v].x, views[v].y, views[v].view_bits);
        off += FINDER_DB_RECORD_SIZE;
    }
    return need;
}

uint32_t finder_desk_db_encode(const finder_desk_t *fd, uint8_t *buf,
                               uint32_t cap)
{
    /* The R3.2 entry point: desktop icons only. Byte-identical to what it wrote
     * before the view records existed -- the hand-authored 56-byte golden in
     * harness/proptest/test_finder_desktop.c still holds it to that. */
    return finder_desk_db_encode_all(fd, (const finder_view_rec_t *)0, 0u,
                                     buf, cap);
}

int finder_desk_db_find_view(const uint8_t *buf, uint32_t len,
                             uint16_t dir_start, finder_view_rec_t *out)
{
    finder_db_status_t st;
    uint16_t n;
    uint32_t off;

    st = finder_desk_db_validate(buf, len);
    if (st != FINDER_DB_OK) return (int)st;      /* negative == corrupt image  */
    if (out == (finder_view_rec_t *)0) return 0;

    n   = fd_get_le16(buf + 6);
    off = FINDER_DB_HEADER_SIZE;
    for (uint16_t i = 0u; i < n; i++) {
        const uint8_t *r = buf + off;
        if (r[0] == (uint8_t)FINDER_DB_KIND_VIEW &&
            fd_get_le16(r + 2) == dir_start) {
            uint32_t k;
            out->dir_start = dir_start;
            for (k = 0u; k < 13u; k++) out->name83[k] = (char)r[4 + k];
            out->name83[13] = '\0';   /* FINDER_DESK_NAME_MAX-1; always closed */
            out->x         = fd_get_le16(r + 17);
            out->y         = fd_get_le16(r + 19);
            out->view_bits = fd_get_le16(r + 21);
            return 1;
        }
        off += FINDER_DB_RECORD_SIZE;
    }
    return 0;
}

finder_db_status_t finder_desk_db_apply(finder_desk_t *fd, const uint8_t *buf,
                                        uint32_t len)
{
    finder_db_status_t st;
    uint16_t n;
    uint32_t off;

    if (fd == (finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0)
        return FINDER_DB_ERR_LEN;

    st = finder_desk_db_validate(buf, len);
    if (st != FINDER_DB_OK) return st;

    n   = fd_get_le16(buf + 6);
    off = FINDER_DB_HEADER_SIZE;
    for (uint16_t i = 0u; i < n; i++) {
        const uint8_t *r = buf + off;
        int target = finder_desk_find_kind(fd, (finder_icon_kind_t)r[0]);
        if (target >= 0) {
            int16_t nx = (int16_t)fd_get_le16(r + 17);
            int16_t ny = (int16_t)fd_get_le16(r + 19);
            fd_clamp_origin(fd, &nx, &ny);
            fd->icons[target].x = nx;
            fd->icons[target].y = ny;
        }
        /* Unknown kinds (item-pos / folder-view / trash-origin) are simply not
         * ours this slice -- ignoring them is correct, not an error. */
        off += FINDER_DB_RECORD_SIZE;
    }
    return FINDER_DB_OK;
}

/* ===========================================================================
 * 10. PAINTING + THE UNDERLAY SEAM
 * ===========================================================================*/

/* One clipped Geneva glyph run. text.h's text_draw goes through surface_blit,
 * which has no region clip and paints a background cell for every glyph, so it
 * cannot be used at the underlay seam (rule 4 of the header). This is the
 * finder_icon.c per-pixel walk applied to the geneva9 strike. */
static void fd_label_text(const bitmap_t *dst, int x, int y, const char *s,
                          uint32_t ink, const region_t *clip)
{
    while (*s != '\0') {
        int c = (int)(unsigned char)*s;
        const unsigned char *glyph = geneva9_glyph(c);
        int adv = (int)geneva9_advance_w(c);

        for (int r = 0; r < GENEVA9_CELL_H; r++) {
            int py = y + r;
            unsigned int row;
            if (py < 0 || py >= (int)dst->height) continue;
            row = (unsigned int)glyph[r];
            for (int cc = 0; cc < 8; cc++) {
                int px;
                uint32_t off;
                if ((row & (0x80u >> cc)) == 0u) continue;
                px = x + cc;
                if (px < 0 || px >= (int)dst->width) continue;
                if (clip != (const region_t *)0 &&
                    !region_contains_point(clip, (int16_t)px, (int16_t)py))
                    continue;
                off = (uint32_t)py * dst->pitch +
                      (uint32_t)px * dst->bytes_per_pixel;
                surface_put_pixel(dst, off, ink);
            }
        }
        x += adv;
        s++;
    }
}

void finder_desk_paint(const finder_desk_t *fd, const bitmap_t *dst,
                       const region_t *clip)
{
    uint32_t ink_px;
    uint32_t face_px;
    rgn_rect_t clip_bb;
    int have_bb = 0;

    if (fd == (const finder_desk_t *)0 || fd->icons == (finder_desk_icon_t *)0)
        return;
    if (dst == (const bitmap_t *)0 || dst->base == (volatile uint8_t *)0 ||
        dst->width == 0u || dst->height == 0u)
        return;

    /* C-8: resolve the two tones ONCE at this drawing-context boundary (the
     * desktop.c desktop_px idiom). This TU owns no color literal. */
    ink_px  = flair_look_pixel_depth(dst->bpp, (int)FLAIR_PART_ICON_INK);
    face_px = flair_look_pixel_depth(dst->bpp, (int)FLAIR_PART_ICON_FACE);

    if (clip != (const region_t *)0) {
        if (region_is_empty(clip)) return;
        clip_bb = region_get_bbox(clip);
        have_bb = 1;
    }

    for (int i = 0; i < (int)fd->n; i++) {
        const finder_desk_icon_t *ic = &fd->icons[i];
        const FLAIRDeskIcon *strike = fd_strike(ic->kind);
        rgn_rect_t cell = finder_desk_cell_rect(fd, i);
        rgn_rect_t lab;
        uint32_t band_px;
        uint32_t text_px;

        if (have_bb && !finder_rect_intersects(cell, clip_bb)) continue;
        if (strike == (const FLAIRDeskIcon *)0) continue;

        /* The sprite (mask-honouring + clipped; os/flair/finder_icon.c). */
        finder_icon_draw(dst, ic->x, ic->y, strike, clip);

        /* The label band. Selection INVERTS it (design F1.1: "selected icons
         * draw with the label inverted -- black label band, white text"); the
         * two tones are the SAME semantic roles, swapped, so no new PART and no
         * color literal is introduced. */
        band_px = ic->selected ? ink_px  : face_px;
        text_px = ic->selected ? face_px : ink_px;

        lab = finder_desk_label_rect(fd, i);
        blitter_fill_rect_clipped(dst, lab, band_px, clip);
        fd_label_text(dst, (int)lab.left + FINDER_LABEL_PAD_X,
                      (int)lab.top + 1, ic->name, text_px, clip);
    }
}

/* The WindowMgr underlay trampoline: adapts the manager's void* user cookie to
 * the typed painter. Installed by finder_desk_install_underlay. */
static void fd_underlay_cb(void *user, const bitmap_t *dst, const region_t *clip)
{
    finder_desk_paint((const finder_desk_t *)user, dst, clip);
}

void finder_desk_install_underlay(finder_desk_t *fd, WindowMgr *wm)
{
    if (wm == (WindowMgr *)0 || fd == (finder_desk_t *)0) return;
#if defined(FINDER_DESK_MUT_NO_UNDERLAY)
    /* MUTANT (Rule 6): never install the hook. desktop.c then fills bare
     * desktop and NOTHING draws the icons, so the composed-desktop render leg
     * (icons present inside the desktop-owned clip) goes RED. This is the
     * "second compositor / forgot the seam" bug. NEVER in a real build. */
    (void)fd;
    (void)fd_underlay_cb;
#else
    wm->desktop_underlay      = fd_underlay_cb;
    wm->desktop_underlay_user = (void *)fd;
#endif
}

void finder_desk_invalidate(WindowMgr *wm, rgn_rect_t old_rect,
                            rgn_rect_t new_rect)
{
    if (wm == (WindowMgr *)0) return;
    WindowMgr_invalidate_desktop(wm, old_rect);
    /* Accumulating a second rect IS the union, and it does NOT damage the
     * (possibly large) bounding rectangle that spans both -- which a single
     * bbox-union call would have over-repainted (ADR-0004 D-5). */
    if (new_rect.left != old_rect.left || new_rect.top != old_rect.top ||
        new_rect.right != old_rect.right || new_rect.bottom != old_rect.bottom) {
        WindowMgr_invalidate_desktop(wm, new_rect);
    }
}

/* ===========================================================================
 * 11. THE DESKTOP HALF OF THE SHELL TENANT
 * ---------------------------------------------------------------------------
 * R3.2's finder_desk_procs / finder_desk_of moved to os/flair/finder_windows.c
 * with bead initech-tdnl.10 (see the header's Sec 11). What remains is the
 * model build the tenant's open() calls -- the SAME two calls, in the SAME
 * order, so the seeded icons and therefore the boot frame are unchanged.
 * ===========================================================================*/

int finder_desk_tenant_build(finder_desk_t *fd, finder_desk_icon_t *storage,
                             uint16_t cap, const FlairLaunchParams *lp)
{
    if (fd == (finder_desk_t *)0 || storage == (finder_desk_icon_t *)0 ||
        lp == (const FlairLaunchParams *)0 || cap == 0u)
        return -1;

    finder_desk_init(fd, storage, cap, lp->bounds);
    return finder_desk_seed_defaults(fd);
}
