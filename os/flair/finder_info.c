/*
 * os/flair/finder_info.c -- the R3.4c Finder GET INFO window (THE ARTIFACT).
 *
 * beads: initech-p6st. The contract, the field rules, the measured geometry
 * and the mutant knobs are in os/flair/finder_info.h (read it first).
 *
 * FREESTANDING (Law 3): no libc; the decimal and date formatting below are
 * hand-rolled, as finder_cmd.c's are.
 */
#include "finder_info.h"
#include "finder_icon.h"        /* finder_icon_draw (the 32x32 strike)        */
#include "blitter.h"            /* blitter_fill_rect_clipped                  */
#include "flair_look.h"         /* flair_look_pixel_depth (C-8 policy seam)   */
#include "text.h"               /* text_draw_run / text_measure_face (bold)   */
#include "chrome_metrics.h"     /* FLAIR_CHROME_FRAME / BODY_BAR / TITLEBAR_H  */

/* ---------------------------------------------------------------------------
 * Libc-free helpers.
 * ------------------------------------------------------------------------- */
static int fi_utoa(char *dst, uint32_t v)
{
    char b[10]; int n = 0, k = 0;
    do { b[n++] = (char)('0' + v % 10u); v /= 10u; } while (v != 0u);
    while (n > 0) dst[k++] = b[--n];
    dst[k] = '\0';
    return k;
}

static int fi_cat(char *dst, int at, const char *s)
{
    while (*s != '\0') dst[at++] = *s++;
    dst[at] = '\0';
    return at;
}

static uint32_t fi_len(const char *s)
{
    uint32_t n = 0u;
    while (s[n] != '\0') n++;
    return n;
}

static void fi_rgn_attach(finder_win_rgn_t *s)
{
    s->r.rows = s->rows; s->r.cap_rows = FINDER_WIN_RGN_ROWS;
    s->r.x_pool = s->pool; s->r.x_pool_cap = FINDER_WIN_RGN_POOL;
    region_set_empty(&s->r);
}

/* ---------------------------------------------------------------------------
 * THE TEXTS (pure; graded by test-finder-dupinfo legs I2/I3)
 * ------------------------------------------------------------------------- */
static const char FI_DAY[7][4]   = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char FI_MONTH[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/* Sakamoto's day-of-week (0 == Sunday), valid for the whole FAT range. */
static int fi_weekday(int y, int m, int d)
{
    static const int t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (m < 3) y -= 1;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

void finder_info_date_text(char *out, uint16_t date, uint16_t time)
{
    int year  = 1980 + (int)(date >> 9);
    int month = (int)((date >> 5) & 15u);
    int day   = (int)(date & 31u);
    int hour  = (int)(time >> 11);
    int min   = (int)((time >> 5) & 63u);
    int h12   = hour % 12;
    int n = 0;
    char num[12];

    if (out == (char *)0) return;
    out[0] = '\0';
    if (h12 == 0) h12 = 12;
#if defined(FINDER_INFO_MUT_DATE_SYNTH)
    /* MUTANT (Rule 6; the F3.5 honesty rule): a zero stamp is PROMOTED to
     * the FAT epoch "Jan 1, 1980" -- a date the volume does not store.
     * NEVER in a real build. */
    if (month == 0) month = 1;
    if (day == 0) day = 1;
#endif
    if (month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        n = fi_cat(out, n, FI_DAY[fi_weekday(year, month, day)]);
        n = fi_cat(out, n, ", ");
        n = fi_cat(out, n, FI_MONTH[month - 1]);
        n = fi_cat(out, n, " ");
        fi_utoa(num, (uint32_t)day);   n = fi_cat(out, n, num);
        n = fi_cat(out, n, ", ");
    } else {
        /* RAW: month/day/year as decoded, the DOS DIR spelling of the same
         * packed word; never a calendar date the volume does not hold. */
        fi_utoa(num, (uint32_t)month); n = fi_cat(out, n, num);
        n = fi_cat(out, n, "/");
        fi_utoa(num, (uint32_t)day);   n = fi_cat(out, n, num);
        n = fi_cat(out, n, "/");
    }
    fi_utoa(num, (uint32_t)year);      n = fi_cat(out, n, num);
    n = fi_cat(out, n, ", ");
    fi_utoa(num, (uint32_t)h12);       n = fi_cat(out, n, num);
    n = fi_cat(out, n, ":");
    out[n++] = (char)('0' + min / 10);
    out[n++] = (char)('0' + min % 10);
    out[n] = '\0';
    n = fi_cat(out, n, (hour >= 12) ? " pm" : " am");
    (void)n;
}

void finder_info_size_text(char *out, uint32_t on_disk, uint32_t bytes,
                           int is_folder, uint16_t items)
{
    int n = 0;
    char num[12];
    if (out == (char *)0) return;
    out[0] = '\0';
    if (on_disk == 0u) {
        n = fi_cat(out, n, "zero K");
    } else {
        fi_utoa(num, (on_disk + 1023u) / 1024u);
        n = fi_cat(out, n, num);
        n = fi_cat(out, n, "K");
    }
    n = fi_cat(out, n, " on disk (");
    fi_utoa(num, bytes); n = fi_cat(out, n, num);
    n = fi_cat(out, n, " bytes)");
    if (is_folder) {
        n = fi_cat(out, n, ", for ");
        fi_utoa(num, (uint32_t)items); n = fi_cat(out, n, num);
        n = fi_cat(out, n, (items == 1u) ? " item" : " items");
    }
}

/* ---------------------------------------------------------------------------
 * THE SNAPSHOT
 * ------------------------------------------------------------------------- */
typedef struct fi_sum {
    finder_shell_t *sh;
    uint16_t dir;
    uint16_t items;
    uint32_t bytes, on_disk;
} fi_sum_t;

static int fi_sum_cb(const finder_dirent_t *e, void *u)
{
    fi_sum_t *s = (fi_sum_t *)u;
    uint32_t od = 0u;
    if (finder_win_skip_entry(e->attribute)) return 0;
    s->items++;
    if ((e->attribute & FINDER_ATTR_DIRECTORY) != 0u) return 0;
    s->bytes += e->size;
    if (s->sh->fs.on_disk != 0 &&
        s->sh->fs.on_disk(s->sh->fs.user, e->name83, s->dir, &od) == (int)FINDER_WIN_OK)
        s->on_disk += od;
    return 0;
}

static void fi_snapshot(finder_shell_t *sh, int slot, int idx)
{
    finder_info_t *fi = &sh->info;
    const finder_window_t *w = &sh->windows[slot];
    const finder_desk_icon_t *ic = &w->view.icons[idx];
    int n = 0, v;

    fi->kind = ic->kind;
    fi->is_folder = (uint8_t)(ic->kind == (uint8_t)FINDER_ICON_FOLDER ? 1 : 0);
    n = fi_cat(fi->name, 0, ic->name);
    n = fi_cat(fi->title, 0, ic->name);
    (void)fi_cat(fi->title, n, " Info");
    fi->size = w->meta[idx].size;
    fi->date = w->meta[idx].date;
    fi->time = w->meta[idx].time;
    fi->items = 0u;
    fi->on_disk = 0u;

    if (fi->is_folder) {
        fi_sum_t s;
        s.sh = sh; s.dir = ic->dir_start; s.items = 0u; s.bytes = 0u; s.on_disk = 0u;
        if (sh->have_fs && sh->fs.enumerate != 0)
            (void)sh->fs.enumerate(sh->fs.user, ic->dir_start, fi_sum_cb, &s);
        fi->items = s.items;
        fi->size = s.bytes;
        fi->on_disk = s.on_disk;
    } else {
#if defined(FINDER_INFO_MUT_SIZE_FROM_FIELD)
        /* MUTANT (Rule 6; F3.5 "from the chain walk"): "on disk" computed
         * from the SIZE FIELD, so a chain longer than its size (the kernel
         * DUP_SAME_CHAIN alias, a sparse-tail write) reads wrong. NEVER in a
         * real build. */
        fi->on_disk = ((fi->size + 511u) / 512u) * 512u;
#else
        if (sh->have_fs && sh->fs.on_disk != 0)
            (void)sh->fs.on_disk(sh->fs.user, ic->name, w->dir_start, &fi->on_disk);
#endif
    }

    /* Where: "<VOLUME>:" + the window's chain. The volume name is the desktop
     * VOLUME icon's label (the kernel reads it off the disk at mount, R3.2). */
    v = finder_desk_find_kind(&sh->desk, FINDER_ICON_VOLUME);
    n = fi_cat(fi->where, 0, v >= 0 ? sh->desk.icons[v].name : "");
    n = fi_cat(fi->where, n, ":");
    if (fi_len(finder_win_path(sh, slot)) + (uint32_t)n < sizeof fi->where)
        (void)fi_cat(fi->where, n, finder_win_path(sh, slot));
}

/* ---------------------------------------------------------------------------
 * THE WINDOW
 * ------------------------------------------------------------------------- */
int finder_info_is(const finder_shell_t *sh, const WindowRecord *w)
{
    return (sh != (const finder_shell_t *)0 && sh->info.open &&
            w == &sh->info.rec) ? 1 : 0;
}

void finder_info_close(finder_shell_t *sh)
{
    if (sh == (finder_shell_t *)0 || !sh->info.open) return;
    DisposeWindow(sh->wm, &sh->info.rec);
    sh->info.open = 0u;
}

void finder_info_open(finder_shell_t *sh, int slot, int idx)
{
    finder_info_t *fi;
    rgn_rect_t frame, content;

    if (sh == (finder_shell_t *)0 || sh->wm == (WindowMgr *)0) return;
    if (slot < 0 || slot >= FINDER_WIN_MAX || !sh->windows[slot].open) return;
    if (idx < 0 || idx >= (int)sh->windows[slot].view.n) return;
    fi = &sh->info;

#if !defined(FINDER_INFO_MUT_TWO_INSTANCES)
    /* SINGLE INSTANCE: reopening REPLACES (the window is torn down and
     * rebuilt over the new item, so the z-order and the snapshot are fresh). */
    if (fi->open) finder_info_close(sh);
#else
    /* MUTANT (Rule 6): a second Get Info builds a SECOND window over the
     * same record -- the Window Manager now lists the record twice (the
     * test counts the Finder-owned windows). NEVER in a real build. */
#endif

    fi_snapshot(sh, slot, idx);

    frame.left   = FINDER_INFO_LEFT;
    frame.top    = FINDER_INFO_TOP;
    frame.right  = FINDER_INFO_LEFT + FINDER_INFO_W;
    frame.bottom = FINDER_INFO_TOP +
                   (fi->is_folder ? FINDER_INFO_H_FOLDER : FINDER_INFO_H_FILE);
    /* The content: inside the body rails (L+6 / R-6 / B-6) and below the
     * title band (T+22) -- the no-bars form of CalcDocContentRect. */
    content.left   = (int16_t)(frame.left + FLAIR_CHROME_FRAME +
                               FLAIR_CHROME_BODY_BAR + FLAIR_CHROME_FRAME);
    content.top    = (int16_t)(frame.top + FLAIR_CHROME_TITLEBAR_H);
    content.right  = (int16_t)(frame.right - FLAIR_CHROME_FRAME -
                               FLAIR_CHROME_BODY_BAR - FLAIR_CHROME_FRAME);
    content.bottom = (int16_t)(frame.bottom - FLAIR_CHROME_FRAME -
                               FLAIR_CHROME_BODY_BAR - FLAIR_CHROME_FRAME);

    {
        uint8_t *p = (uint8_t *)&fi->rec;
        for (uint32_t i = 0u; i < (uint32_t)sizeof fi->rec; i++) p[i] = 0u;
    }
    fi_rgn_attach(&fi->rs); fi_rgn_attach(&fi->rc);
    fi_rgn_attach(&fi->ru); fi_rgn_attach(&fi->rk);
    fi->rec.strucRgn  = &fi->rs.r;
    fi->rec.contRgn   = &fi->rc.r;
    fi->rec.updateRgn = &fi->ru.r;
    fi->rec.nextWindow = (WindowRecord *)0;

    NewWindow(sh->wm, &fi->rec, frame, content, (int16_t)documentKind,
              (int16_t)noGrowDocProc, 1u);
    SetWTitle(sh->wm, &fi->rec, fi->title);
    if (sh->app != (FlairApp *)0)
        fi->rec.refCon = (int32_t)(uintptr_t)sh->app;   /* Finder-owned */
    fi->open = 1u;
}

/* ---------------------------------------------------------------------------
 * THE PAINT
 * ------------------------------------------------------------------------- */
static void fi_fill(const bitmap_t *dst, const region_t *clip, int x, int y,
                    int w, int h, int part)
{
    rgn_rect_t r;
    r.left = (int16_t)x; r.top = (int16_t)y;
    r.right = (int16_t)(x + w); r.bottom = (int16_t)(y + h);
    blitter_fill_rect_clipped(dst, r, flair_look_pixel_depth(dst->bpp, part), clip);
}

static void fi_frame(const bitmap_t *dst, const region_t *clip, int l, int t,
                     int r, int b, int part)
{
    fi_fill(dst, clip, l, t, r - l, 1, part);
    fi_fill(dst, clip, l, b - 1, r - l, 1, part);
    fi_fill(dst, clip, l, t, 1, b - t, part);
    fi_fill(dst, clip, r - 1, t, 1, b - t, part);
}

/* A bold Geneva 9 label right-aligned so its colon ends at `right_x`
 * (exclusive), cap top at `cap_y` (the strike's caps start on row 1). */
static void fi_label(const bitmap_t *dst, const region_t *clip, int right_x,
                     int cap_y, const char *s, uint32_t fg, uint32_t bg)
{
    text_clip_t k;
    int w = text_measure_face(FONT_GENEVA9, TEXT_FACE_BOLD, s);
    int x = right_x - w;
#if defined(FINDER_INFO_MUT_LABEL_SHIFT)
    x++;   /* MUTANT (Rule 6): the column 1 px off the capture. NEVER real. */
#endif
    k.rgn_a = clip; k.rgn_b = (const region_t *)0;
    k.left = 0; k.top = 0; k.right = 0; k.bottom = 0;
    (void)text_draw_run(dst, x, cap_y - 1, s, FONT_GENEVA9, TEXT_FACE_BOLD,
                        fg, bg, &k);
}

static void fi_value(const bitmap_t *dst, const region_t *clip, int x,
                     int cap_y, const char *s, uint32_t ink)
{
    finder_desk_text(dst, x, cap_y - 1, s, ink, clip);
}

void finder_info_paint(finder_shell_t *sh, const bitmap_t *dst,
                       const region_t *clip)
{
    finder_info_t *fi;
    rgn_rect_t c;
    int ox, oy;
    uint32_t ink, face, white, dim;
    char txt[FINDER_INFO_SIZE_MAX];

    if (sh == (finder_shell_t *)0 || !sh->info.open) return;
    if (dst == (const bitmap_t *)0 || dst->base == (volatile uint8_t *)0) return;
    fi = &sh->info;
    c  = region_get_bbox(fi->rec.contRgn);
    ox = c.left; oy = c.top;

    ink   = flair_look_pixel_depth(dst->bpp, (int)FLAIR_PART_FRAME);
    face  = flair_look_pixel_depth(dst->bpp, (int)FLAIR_PART_PLAT_FACE);
    white = flair_look_pixel_depth(dst->bpp, (int)FLAIR_PART_CONTENT);
    dim   = flair_look_pixel_depth(dst->bpp, (int)FLAIR_PART_MENU_DISABLED_INK);

    /* The dialog face (E7), then the item's icon and name. */
    blitter_fill_rect_clipped(dst, c, face, clip);
    {
        const FLAIRDeskIcon *strike = (const FLAIRDeskIcon *)finder_desk_strike(fi->kind);
        if (strike != (const FLAIRDeskIcon *)0)
            finder_icon_draw(dst, (int16_t)(ox + FINDER_INFO_ICON_X),
                             (int16_t)(oy + FINDER_INFO_ICON_Y), strike, clip);
    }
    fi_value(dst, clip, ox + FINDER_INFO_NAME_X, oy + FINDER_INFO_NAME_CAP,
             fi->name, ink);

    /* The labelled rows. */
    fi_label(dst, clip, ox + FINDER_INFO_LABEL_R, oy + FINDER_INFO_ROW_KIND,  "Kind:", ink, face);
    fi_label(dst, clip, ox + FINDER_INFO_LABEL_R, oy + FINDER_INFO_ROW_SIZE,  "Size:", ink, face);
    fi_label(dst, clip, ox + FINDER_INFO_LABEL_R, oy + FINDER_INFO_ROW_WHERE, "Where:", ink, face);
    fi_label(dst, clip, ox + FINDER_INFO_LABEL_R, oy + FINDER_INFO_ROW_CREAT, "Created:", ink, face);
    fi_label(dst, clip, ox + FINDER_INFO_LABEL_R, oy + FINDER_INFO_ROW_MOD,   "Modified:", ink, face);

    fi_value(dst, clip, ox + FINDER_INFO_VALUE_X, oy + FINDER_INFO_ROW_KIND,
             finder_win_kind_text(fi->kind), ink);
    finder_info_size_text(txt, fi->on_disk, fi->size, (int)fi->is_folder, fi->items);
    fi_value(dst, clip, ox + FINDER_INFO_VALUE_X, oy + FINDER_INFO_ROW_SIZE, txt, ink);
    fi_value(dst, clip, ox + FINDER_INFO_VALUE_X, oy + FINDER_INFO_ROW_WHERE, fi->where, ink);
    finder_info_date_text(txt, fi->date, fi->time);
    fi_value(dst, clip, ox + FINDER_INFO_VALUE_X, oy + FINDER_INFO_ROW_CREAT, txt, ink);
    fi_value(dst, clip, ox + FINDER_INFO_VALUE_X, oy + FINDER_INFO_ROW_MOD,   txt, ink);

    if (!fi->is_folder) {
        fi_label(dst, clip, ox + FINDER_INFO_LABEL_R, oy + FINDER_INFO_ROW_VERS, "Version:", ink, face);
        fi_value(dst, clip, ox + FINDER_INFO_VALUE_X, oy + FINDER_INFO_ROW_VERS, "n/a", ink);
        fi_label(dst, clip, ox + FINDER_INFO_LABEL_R, oy + FINDER_INFO_ROW_CMT_F, "Comments:", ink, face);
        fi_fill(dst, clip, ox + FINDER_INFO_BOX_L, oy + FINDER_INFO_BOX_T_F,
                FINDER_INFO_BOX_R - FINDER_INFO_BOX_L,
                FINDER_INFO_BOX_B_F - FINDER_INFO_BOX_T_F, (int)FLAIR_PART_CONTENT);
        fi_frame(dst, clip, ox + FINDER_INFO_BOX_L, oy + FINDER_INFO_BOX_T_F,
                 ox + FINDER_INFO_BOX_R, oy + FINDER_INFO_BOX_B_F, (int)FLAIR_PART_FRAME);
        /* Locked / Stationery Pad: DISABLED (F3.5 "grayed") -- the box ring
         * and the text in the dimmed ink, the box interior white. */
        fi_fill(dst, clip, ox + FINDER_INFO_LOCK_X, oy + FINDER_INFO_CHK_Y,
                FINDER_INFO_CHK_DIM, FINDER_INFO_CHK_DIM, (int)FLAIR_PART_CONTENT);
        fi_frame(dst, clip, ox + FINDER_INFO_LOCK_X, oy + FINDER_INFO_CHK_Y,
                 ox + FINDER_INFO_LOCK_X + FINDER_INFO_CHK_DIM,
                 oy + FINDER_INFO_CHK_Y + FINDER_INFO_CHK_DIM, (int)FLAIR_PART_MENU_DISABLED_INK);
        fi_fill(dst, clip, ox + FINDER_INFO_STAT_X, oy + FINDER_INFO_CHK_Y,
                FINDER_INFO_CHK_DIM, FINDER_INFO_CHK_DIM, (int)FLAIR_PART_CONTENT);
        fi_frame(dst, clip, ox + FINDER_INFO_STAT_X, oy + FINDER_INFO_CHK_Y,
                 ox + FINDER_INFO_STAT_X + FINDER_INFO_CHK_DIM,
                 oy + FINDER_INFO_CHK_Y + FINDER_INFO_CHK_DIM, (int)FLAIR_PART_MENU_DISABLED_INK);
        {
            text_clip_t k;
            k.rgn_a = clip; k.rgn_b = (const region_t *)0;
            k.left = 0; k.top = 0; k.right = 0; k.bottom = 0;
            (void)text_draw_run(dst, ox + FINDER_INFO_LOCK_TX, oy + FINDER_INFO_CHK_Y + 1,
                                "Locked", FONT_GENEVA9, TEXT_FACE_BOLD, dim, face, &k);
            (void)text_draw_run(dst, ox + FINDER_INFO_STAT_TX, oy + FINDER_INFO_CHK_Y + 1,
                                "Stationery Pad", FONT_GENEVA9, TEXT_FACE_BOLD, dim, face, &k);
        }
    } else {
        fi_label(dst, clip, ox + FINDER_INFO_LABEL_R, oy + FINDER_INFO_ROW_CMT_D, "Comments:", ink, face);
        fi_fill(dst, clip, ox + FINDER_INFO_BOX_L, oy + FINDER_INFO_BOX_T_D,
                FINDER_INFO_BOX_R - FINDER_INFO_BOX_L,
                FINDER_INFO_BOX_B_D - FINDER_INFO_BOX_T_D, (int)FLAIR_PART_CONTENT);
        fi_frame(dst, clip, ox + FINDER_INFO_BOX_L, oy + FINDER_INFO_BOX_T_D,
                 ox + FINDER_INFO_BOX_R, oy + FINDER_INFO_BOX_B_D, (int)FLAIR_PART_FRAME);
    }
    (void)white;
}
