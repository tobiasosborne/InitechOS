/*
 * os/flair/menu.c -- the FLAIR Menu Manager implementation (THE ARTIFACT).
 *
 * beads: initech-n3e ("FLAIR Menu Manager: pull-down menus + the menu bar,
 *        Photoshop-exact"). FLAIR Layer 3 Manager (ADR-0004 D-3).
 *
 * Ref:   ADR-0004 D-3 (MenuInfo + the Photoshop-exact bar + MenuSelect ->
 *          (menuID<<16|item)); D-1/D-2 (draw THROUGH a GrafPort clipped by an
 *          ATKINSON region; one surface module, no second pixel path); D-7
 *          (proportional text -- text_measure = sum of per-glyph advances).
 *        spec/assets/menu_canon.h (the FROZEN canon string; USED, NOT re-authored).
 *        spec/chrome_metrics.h (FLAIR_CHROME_MENUBAR_H = 20).
 *        os/flair/text.h (text_measure / text_draw), os/flair/blitter.h
 *          (blitter_fill_rect_clipped), os/flair/surface.h (the ONE writer).
 *        Inside Macintosh Vol I "Menu Manager" (MenuInfo, MenuSelect, MenuKey,
 *          DrawMenuBar, HiliteMenu; the (menuID<<16|item) result; 1-based items;
 *          a disabled/divider item is never selectable; Command-key match).
 *        CLAUDE.md Law 2/3/4, Rule 2 (fail loud / bounds), Rule 11 (deterministic),
 *        Rule 12 (ASCII-clean).
 *
 * ARTIFACT: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11) AND hosted
 * (for harness/proptest/test_menu.c). No libc, no malloc -- all storage is
 * caller-supplied (the MenuBar/MenuInfo arrays). Only <stdint.h>/<stddef.h> +
 * the FLAIR headers.
 *
 * NAMED MUTANTS (Rule 6; mutation-proven by test_menu.c):
 *   MENU_MUTATE_FIXED_WIDTH    -- lay titles/items out with a FIXED width instead
 *                                 of the proportional text_measure width. Wrong
 *                                 x-positions => the bar-layout oracle goes RED.
 *   MENU_MUTATE_SELECT_DISABLED-- allow selecting a DISABLED item / divider in
 *                                 the tracking loop. => the selectability oracle
 *                                 goes RED.
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 */

#include "menu.h"

/* The fixed-width fallback used ONLY by the MENU_MUTATE_FIXED_WIDTH mutant.
 * Chosen distinct from any proportional title width so the mutant misplaces the
 * titles measurably (the oracle catches a wrong x). */
#define FLAIR_MENU_MUTANT_FIXED_W   48

/* --------------------------------------------------------------------------
 * Internal: the proportional width of one title slot (PAD + text + PAD).
 *
 * Under MENU_MUTATE_FIXED_WIDTH this returns a constant, ignoring the title text
 * length -- the named layout mutant (Rule 6).
 * -------------------------------------------------------------------------- */
static int title_slot_w(const char *title)
{
#if defined(MENU_MUTATE_FIXED_WIDTH) && MENU_MUTATE_FIXED_WIDTH
    (void)title;
    return FLAIR_MENU_MUTANT_FIXED_W;
#else
    int tw = text_measure(FONT_CHICAGO, title);
    return tw + 2 * FLAIR_MENU_TITLE_PAD;
#endif
}

/* --------------------------------------------------------------------------
 * Bar layout queries (proportional cumulative; ADR-0004 D-7).
 * -------------------------------------------------------------------------- */
int MenuBar_title_x(const MenuBar *bar, int i)
{
    if (!bar || i < 0 || i >= (int)bar->n_menus)
        return -1;
    int x = bar->has_apple ? (int)FLAIR_MENU_APPLE_W : 0;
    for (int k = 0; k < i; k++)
        x += title_slot_w(bar->menus[k].title);
    return x;
}

int MenuBar_title_w(const MenuBar *bar, int i)
{
    if (!bar || i < 0 || i >= (int)bar->n_menus)
        return 0;
    return title_slot_w(bar->menus[i].title);
}

int MenuBar_hit(const MenuBar *bar, int x)
{
    if (!bar)
        return -1;
    int cur = bar->has_apple ? (int)FLAIR_MENU_APPLE_W : 0;
    /* x in the Apple slot (or left of the first title) is not a title hit. */
    if (x < cur)
        return -1;
    for (int k = 0; k < (int)bar->n_menus; k++) {
        int w = title_slot_w(bar->menus[k].title);
        if (x >= cur && x < cur + w)
            return k;
        cur += w;
    }
    return -1;
}

/* --------------------------------------------------------------------------
 * Per-item row height: a divider is short and unselectable; others are full.
 * -------------------------------------------------------------------------- */
static int item_row_h(const MenuItem *it)
{
    return it->is_divider ? FLAIR_MENU_DIV_H : FLAIR_MENU_ITEM_H;
}

/* --------------------------------------------------------------------------
 * The widest item text in a menu (proportional), for the panel width.
 * Under MENU_MUTATE_FIXED_WIDTH the panel uses a fixed width too (so the
 * item-row layout shifts), keeping the mutant self-consistent.
 * -------------------------------------------------------------------------- */
static int menu_body_text_w(const MenuInfo *m)
{
#if defined(MENU_MUTATE_FIXED_WIDTH) && MENU_MUTATE_FIXED_WIDTH
    (void)m;
    return FLAIR_MENU_MUTANT_FIXED_W;
#else
    int widest = 0;
    for (int k = 0; k < (int)m->n_items; k++) {
        if (m->items[k].is_divider)
            continue;
        int w = text_measure(FONT_CHICAGO, m->items[k].text);
        if (w > widest)
            widest = w;
    }
    return widest;
#endif
}

/* --------------------------------------------------------------------------
 * Pull-down panel rect for menu index mi: directly below its title.
 * -------------------------------------------------------------------------- */
rgn_rect_t MenuInfo_panel_rect(const MenuBar *bar, int mi)
{
    rgn_rect_t r = { 0, 0, 0, 0 };
    if (!bar || mi < 0 || mi >= (int)bar->n_menus)
        return r;
    const MenuInfo *m = &bar->menus[mi];

    int left = MenuBar_title_x(bar, mi);
    if (left < 0)
        return r;

    int body_w = menu_body_text_w(m);
    int width  = FLAIR_MENU_ITEM_LPAD + body_w + FLAIR_MENU_ITEM_RPAD
                 + 2 * FLAIR_MENU_PANEL_FRAME;

    int height = 2 * FLAIR_MENU_PANEL_FRAME;
    for (int k = 0; k < (int)m->n_items; k++)
        height += item_row_h(&m->items[k]);

    r.left   = (int16_t)left;
    r.top    = (int16_t)FLAIR_MENUBAR_H;             /* just below the bar     */
    r.right  = (int16_t)(left + width);
    r.bottom = (int16_t)(FLAIR_MENUBAR_H + height);
    return r;
}

/* --------------------------------------------------------------------------
 * Internal: the screen y of the TOP of item index `it`'s row in the panel.
 * (panel top + frame + sum of earlier item row heights)
 * -------------------------------------------------------------------------- */
static int item_row_top(const MenuInfo *m, int it)
{
    int y = FLAIR_MENUBAR_H + FLAIR_MENU_PANEL_FRAME;
    for (int k = 0; k < it; k++)
        y += item_row_h(&m->items[k]);
    return y;
}

/* --------------------------------------------------------------------------
 * Which item row contains screen point (x,y)? -1 if outside the panel or in a
 * divider row (a divider is not a selectable target; geometry returns -1 for it
 * so the hilite never lands on a divider).
 * -------------------------------------------------------------------------- */
int MenuInfo_item_at(const MenuBar *bar, int mi, int x, int y)
{
    if (!bar || mi < 0 || mi >= (int)bar->n_menus)
        return -1;
    rgn_rect_t panel = MenuInfo_panel_rect(bar, mi);
    if (panel.right <= panel.left || panel.bottom <= panel.top)
        return -1;
    if (x < panel.left || x >= panel.right || y < panel.top || y >= panel.bottom)
        return -1;

    const MenuInfo *m = &bar->menus[mi];
    int row_top = FLAIR_MENUBAR_H + FLAIR_MENU_PANEL_FRAME;
    for (int k = 0; k < (int)m->n_items; k++) {
        int h = item_row_h(&m->items[k]);
        if (y >= row_top && y < row_top + h) {
            if (m->items[k].is_divider)
                return -1;        /* divider row is never a target */
            return k;
        }
        row_top += h;
    }
    return -1;
}

/* --------------------------------------------------------------------------
 * Selectability: enabled AND not a divider (Inside Macintosh -- a dimmed item
 * or a separator cannot be chosen). The decisive predicate; the
 * MENU_MUTATE_SELECT_DISABLED mutant subverts it.
 * -------------------------------------------------------------------------- */
int MenuInfo_item_selectable(const MenuBar *bar, int mi, int it)
{
    if (!bar || mi < 0 || mi >= (int)bar->n_menus)
        return 0;
    const MenuInfo *m = &bar->menus[mi];
    if (it < 0 || it >= (int)m->n_items)
        return 0;
#if defined(MENU_MUTATE_SELECT_DISABLED) && MENU_MUTATE_SELECT_DISABLED
    /* NAMED MUTANT: pretend every existing item is selectable, even a disabled
     * item or a divider. The selectability oracle MUST catch this (Rule 6). */
    return 1;
#else
    if (m->items[it].is_divider)
        return 0;
    return m->items[it].enabled ? 1 : 0;
#endif
}

/* --------------------------------------------------------------------------
 * HiliteMenu -- the bar title slot rect to invert (mi < 0 == nothing hilited).
 * -------------------------------------------------------------------------- */
rgn_rect_t HiliteMenu(const MenuBar *bar, int mi)
{
    rgn_rect_t r = { 0, 0, 0, 0 };
    if (!bar || mi < 0 || mi >= (int)bar->n_menus)
        return r;
    int left = MenuBar_title_x(bar, mi);
    int w    = MenuBar_title_w(bar, mi);
    if (left < 0 || w <= 0)
        return r;
    r.top    = 0;
    r.left   = (int16_t)left;
    r.bottom = (int16_t)FLAIR_MENUBAR_H;
    r.right  = (int16_t)(left + w);
    return r;
}

/* --------------------------------------------------------------------------
 * ASCII case-fold for the Command-key match (IM: Command-key letters match
 * case-insensitively).
 * -------------------------------------------------------------------------- */
static char cmd_fold(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - 'a' + 'A');
    return c;
}

/* --------------------------------------------------------------------------
 * MenuKey -- map a command-key char to the first enabled, non-divider item.
 * -------------------------------------------------------------------------- */
uint32_t MenuKey(const MenuBar *bar, char ch)
{
    if (!bar || ch == 0)
        return 0;
    char want = cmd_fold(ch);
    for (int mi = 0; mi < (int)bar->n_menus; mi++) {
        const MenuInfo *m = &bar->menus[mi];
        for (int it = 0; it < (int)m->n_items; it++) {
            const MenuItem *item = &m->items[it];
            if (item->cmdChar == 0)
                continue;
            if (cmd_fold(item->cmdChar) != want)
                continue;
            if (!MenuInfo_item_selectable(bar, mi, it))
                continue;     /* a dimmed item's cmd-key does nothing (IM)     */
            return MenuResult(m->menuID, (uint16_t)(it + 1)); /* 1-based item  */
        }
    }
    return 0;
}

/* --------------------------------------------------------------------------
 * flair_menu_track -- the deterministic pull-down tracking primitive.
 *
 * Hit the bar at startPt; if no title, return 0. Otherwise track through pts[]
 * (the cursor sequence after the click; the LAST point is the release). The
 * hilited item follows the cursor (MenuInfo_item_at). On release, return the
 * selection IFF the released item is selectable, else 0.
 * -------------------------------------------------------------------------- */
uint32_t flair_menu_track(const MenuBar *bar,
                          flair_point_t startPt,
                          const flair_point_t *pts, int n_pts,
                          int *out_hi)
{
    if (out_hi)
        *out_hi = -1;
    if (!bar)
        return 0;

    /* startPt is screen coords (v,h). The bar is rows [0, FLAIR_MENUBAR_H). */
    if (startPt.v < 0 || startPt.v >= (int)FLAIR_MENUBAR_H)
        return 0;
    int mi = MenuBar_hit(bar, (int)startPt.h);
    if (mi < 0)
        return 0;                       /* clicked outside any title          */

    const MenuInfo *m = &bar->menus[mi];
    int hi = -1;

    /* Track the cursor: each point updates the hilited item under it. */
    for (int p = 0; p < n_pts; p++) {
        hi = MenuInfo_item_at(bar, mi, (int)pts[p].h, (int)pts[p].v);
    }

    if (out_hi)
        *out_hi = hi;

    /* The selection is the item under the RELEASE point (the last pts entry).
     * If there were no tracking points, the release is the click itself (still
     * on the title, not on any item) -> nothing chosen. */
    if (n_pts <= 0)
        return 0;

    int rel = MenuInfo_item_at(bar, mi, (int)pts[n_pts - 1].h,
                               (int)pts[n_pts - 1].v);
    if (rel < 0)
        return 0;                       /* released outside any item row       */
    if (!MenuInfo_item_selectable(bar, mi, rel))
        return 0;                       /* disabled / divider: not selectable  */

    (void)m;
    return MenuResult(bar->menus[mi].menuID, (uint16_t)(rel + 1)); /* 1-based  */
}

uint32_t MenuSelect(const MenuBar *bar, flair_point_t startPt,
                    const flair_point_t *pts, int n_pts)
{
    return flair_menu_track(bar, startPt, pts, n_pts, NULL);
}

/* --------------------------------------------------------------------------
 * DrawMenuBar -- paint the 20px bar across the top of the port's bitmap.
 *
 * Fills the bar background (clipped), draws the Apple glyph slot (a filled cell
 * with a contrasting square so the canon apple slot is visibly present), then
 * each title via text_draw(FONT_CHICAGO) at its proportional x. Bounds/clip are
 * delegated to blitter_fill_rect_clipped and text_draw->surface_blit (Rule 2).
 * -------------------------------------------------------------------------- */
void DrawMenuBar(GrafPort *port, const MenuBar *bar,
                 uint32_t fg, uint32_t bg, const region_t *clip)
{
    if (!port || !bar)
        return;
    const bitmap_t *bm = &port->portBits.bm;

    /* Bar background: full width, FLAIR_MENUBAR_H tall. */
    rgn_rect_t barrect;
    barrect.top = 0; barrect.left = 0;
    barrect.bottom = (int16_t)FLAIR_MENUBAR_H;
    barrect.right = (int16_t)bm->width;
    blitter_fill_rect_clipped(bm, barrect, bg, clip);

    /* A 1px baseline under the bar (the classic Mac menu-bar bottom line). */
    rgn_rect_t baseline;
    baseline.top = (int16_t)(FLAIR_MENUBAR_H - 1);
    baseline.left = 0;
    baseline.bottom = (int16_t)FLAIR_MENUBAR_H;
    baseline.right = (int16_t)bm->width;
    blitter_fill_rect_clipped(bm, baseline, fg, clip);

    /* Apple-menu glyph slot at the far left (filled square, ink-on-bg). */
    if (bar->has_apple) {
        rgn_rect_t apple;
        apple.top = (int16_t)(FLAIR_MENU_TITLE_VPAD);
        apple.left = (int16_t)(FLAIR_MENU_TITLE_VPAD);
        apple.bottom = (int16_t)(FLAIR_MENUBAR_H - FLAIR_MENU_TITLE_VPAD - 1);
        apple.right = (int16_t)(FLAIR_MENU_APPLE_W - FLAIR_MENU_TITLE_VPAD);
        blitter_fill_rect_clipped(bm, apple, fg, clip);
    }

    /* Each title at its proportional x (text_draw clips internally). */
    for (int k = 0; k < (int)bar->n_menus; k++) {
        int x = MenuBar_title_x(bar, k);
        if (x < 0)
            continue;
        text_draw(bm, x + FLAIR_MENU_TITLE_PAD, FLAIR_MENU_TITLE_VPAD,
                  bar->menus[k].title, FONT_CHICAGO, fg, bg);
    }
}

/* --------------------------------------------------------------------------
 * flair_draw_menu_panel -- paint a dropped panel and hilite one item.
 *
 * Fills the panel (clipped), frames it (1px), draws each item's text, and -- if
 * hilite_item is a selectable row -- inverts (paints fg) that item's band with
 * the text redrawn in bg over it. A non-selectable hilite request draws no band.
 * -------------------------------------------------------------------------- */
void flair_draw_menu_panel(GrafPort *port, const MenuBar *bar, int mi,
                           int hilite_item,
                           uint32_t fg, uint32_t bg, const region_t *clip)
{
    if (!port || !bar || mi < 0 || mi >= (int)bar->n_menus)
        return;
    const bitmap_t *bm = &port->portBits.bm;
    rgn_rect_t panel = MenuInfo_panel_rect(bar, mi);
    if (panel.right <= panel.left || panel.bottom <= panel.top)
        return;

    /* Panel body (bg fill). */
    blitter_fill_rect_clipped(bm, panel, bg, clip);

    /* 1px frame: top, bottom, left, right lines in fg. */
    rgn_rect_t fr_top = { panel.top, panel.left,
                          (int16_t)(panel.top + FLAIR_MENU_PANEL_FRAME),
                          panel.right };
    rgn_rect_t fr_bot = { (int16_t)(panel.bottom - FLAIR_MENU_PANEL_FRAME),
                          panel.left, panel.bottom, panel.right };
    rgn_rect_t fr_lft = { panel.top, panel.left, panel.bottom,
                          (int16_t)(panel.left + FLAIR_MENU_PANEL_FRAME) };
    rgn_rect_t fr_rgt = { panel.top,
                          (int16_t)(panel.right - FLAIR_MENU_PANEL_FRAME),
                          panel.bottom, panel.right };
    blitter_fill_rect_clipped(bm, fr_top, fg, clip);
    blitter_fill_rect_clipped(bm, fr_bot, fg, clip);
    blitter_fill_rect_clipped(bm, fr_lft, fg, clip);
    blitter_fill_rect_clipped(bm, fr_rgt, fg, clip);

    const MenuInfo *m = &bar->menus[mi];
    int draw_hi = (hilite_item >= 0 &&
                   MenuInfo_item_selectable(bar, mi, hilite_item))
                  ? hilite_item : -1;

    int text_x = panel.left + FLAIR_MENU_PANEL_FRAME + FLAIR_MENU_ITEM_LPAD;
    for (int k = 0; k < (int)m->n_items; k++) {
        const MenuItem *it = &m->items[k];
        int row_top = item_row_top(m, k);
        int h = item_row_h(it);

        if (it->is_divider) {
            /* A divider is a single horizontal rule centered in its short row. */
            rgn_rect_t rule = { (int16_t)(row_top + h / 2),
                                (int16_t)(panel.left + FLAIR_MENU_PANEL_FRAME),
                                (int16_t)(row_top + h / 2 + 1),
                                (int16_t)(panel.right - FLAIR_MENU_PANEL_FRAME) };
            blitter_fill_rect_clipped(bm, rule, fg, clip);
            continue;
        }

        uint32_t row_fg = fg, row_bg = bg;
        if (k == draw_hi) {
            /* Hilite band: invert (fill fg, text in bg). */
            rgn_rect_t band = { (int16_t)row_top,
                                (int16_t)(panel.left + FLAIR_MENU_PANEL_FRAME),
                                (int16_t)(row_top + h),
                                (int16_t)(panel.right - FLAIR_MENU_PANEL_FRAME) };
            blitter_fill_rect_clipped(bm, band, fg, clip);
            row_fg = bg; row_bg = fg;
        }

        /* Item text (Chicago). The mark column is the LPAD gap; if marked, draw
         * the mark char just inside the frame. */
        if (it->mark) {
            char marks[2]; marks[0] = it->mark; marks[1] = 0;
            text_draw(bm, panel.left + FLAIR_MENU_PANEL_FRAME + 2,
                      row_top, marks, FONT_CHICAGO, row_fg, row_bg);
        }
        text_draw(bm, text_x, row_top, it->text, FONT_CHICAGO, row_fg, row_bg);
    }
}
