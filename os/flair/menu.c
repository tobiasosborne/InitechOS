/*
 * os/flair/menu.c -- the FLAIR Menu Manager implementation (THE ARTIFACT).
 *
 * beads: initech-n3e ("FLAIR Menu Manager: pull-down menus + the menu bar,
 *        Photoshop-exact"). FLAIR Layer 3 Manager (ADR-0004 D-3).
 *        initech-yx4v ("Apple menu slot rendered as a solid black filled
 *        square, not an apple glyph") -- fixed by spec/assets/apple_glyph.h.
 *
 * ERA AXIS: Mac OS 8 Platinum (DEC-10) is the FLAIR BASE. The drawing path uses
 * the sampled Platinum menu anatomy from sys8/menus.md; the canon strings and
 * Menu Manager behavior remain unchanged (bead initech-sjvq).
 *
 * Ref:   ADR-0004 D-3 (MenuInfo + the Photoshop-exact bar + MenuSelect ->
 *          (menuID<<16|item)); D-1/D-2 (draw THROUGH a GrafPort clipped by an
 *          ATKINSON region; one surface module, no second pixel path); D-7
 *          (proportional text -- text_measure = sum of per-glyph advances).
 *        spec/assets/menu_canon.h (the FROZEN canon string; USED, NOT re-authored).
 *        spec/assets/apple_glyph.h (the hand-authored Apple-menu glyph strike;
 *          initech-yx4v; back-checked against the system7-decomp captures).
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
 *   MENU_MUT_NO_REHIT          -- freeze the tracked menu index at the click
 *                                 (do NOT re-hit the bar per tracked point) --
 *                                 the ORIGINAL initech-rl4v bug: cross-menu
 *                                 drag is structurally impossible. => the
 *                                 PROPERTY 3c cross-menu-drag oracle goes RED.
 *   MENU_MUT_APPLE_SQUARE      -- paint the Apple slot as a SOLID FILLED
 *                                 RECTANGLE instead of the apple_glyph.h masked
 *                                 blit -- the ORIGINAL initech-yx4v bug. =>
 *                                 the APPLE GLYPH property oracle goes RED
 *                                 (ink ratio, bite notch, leaf-above-body).
 *   MENU_MUT_BAR_FLAT          -- erase the sampled 3-D profile + round corners.
 *   MENU_MUT_NO_PANEL_BEVEL    -- omit the panel's white/B3 inner bevel.
 *   MENU_MUT_SEP_PLAIN         -- collapse the etched separator to one plain row.
 *   MENU_MUT_DISABLED_NORMAL_INK -- render disabled items as normal black ink.
 *   MENU_MUT_TITLE_NO_HILITE   -- ignore the explicit pulled-title state.
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 */

#include "menu.h"
#include "apple_glyph.h"        /* APPLE_GLYPH_ROWS -- hand-authored strike
                                 * (-Ispec/assets); initech-yx4v            */
#include "flair_look.h"         /* PART->pixel policy seam (DEC-09 C-8)          */

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
static int menu_panel_w(const MenuInfo *m)
{
    int widest = FLAIR_MENU_ITEM_LPAD + FLAIR_MENU_ITEM_RPAD;
    for (int k = 0; k < (int)m->n_items; k++) {
        if (m->items[k].is_divider)
            continue;
#if defined(MENU_MUTATE_FIXED_WIDTH) && MENU_MUTATE_FIXED_WIDTH
        int w = FLAIR_MENU_MUTANT_FIXED_W;
#else
        int w = text_measure(FONT_CHICAGO, m->items[k].text);
#endif
        int needed = FLAIR_MENU_ITEM_LPAD + w + FLAIR_MENU_ITEM_RPAD;
        if (m->items[k].cmdChar != 0) {
            needed = FLAIR_MENU_ITEM_LPAD + w + FLAIR_MENU_CMD_GAP +
                     FLAIR_MENU_CMD_CHARS * CHICAGO_CELL_W +
                     FLAIR_MENU_CMD_RPAD;
        }
        if (needed > widest)
            widest = needed;
    }
    return widest;
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

    int width  = menu_panel_w(m);

    int height = FLAIR_MENU_PANEL_INSET;
    for (int k = 0; k < (int)m->n_items; k++)
        height += item_row_h(&m->items[k]);

    r.left   = (int16_t)left;
    r.top    = (int16_t)(FLAIR_MENUBAR_H - FLAIR_MENU_PANEL_FRAME);
    r.right  = (int16_t)(left + width);
    r.bottom = (int16_t)(r.top + height);
    return r;
}

rgn_rect_t MenuInfo_panel_footprint_rect(const MenuBar *bar, int mi)
{
    rgn_rect_t r = MenuInfo_panel_rect(bar, mi);
    if (r.right > r.left && r.bottom > r.top) {
        r.right = (int16_t)(r.right + FLAIR_MENU_DROP_SHADOW);
        r.bottom = (int16_t)(r.bottom + FLAIR_MENU_DROP_SHADOW);
    }
    return r;
}

/* --------------------------------------------------------------------------
 * Internal: the screen y of the TOP of item index `it`'s row in the panel.
 * (panel top + two-row top anatomy + sum of earlier item row heights)
 * -------------------------------------------------------------------------- */
static int item_row_top(const MenuInfo *m, int it)
{
    int y = FLAIR_MENUBAR_H - FLAIR_MENU_PANEL_FRAME +
            FLAIR_MENU_PANEL_INSET;
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
    int row_top = panel.top + FLAIR_MENU_PANEL_INSET;
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
 * Pulled-title geometry. The 10px accent block pad is measured from the idle
 * INK run, not from the retained 7px layout slot; opening a menu never shifts
 * the titles after it. Ref: sys8/menus.md Sec 1.4; bead initech-sjvq.
 * -------------------------------------------------------------------------- */
rgn_rect_t MenuBar_hilite_rect(const MenuBar *bar, int mi)
{
    rgn_rect_t r = { 0, 0, 0, 0 };
    if (!bar || mi < 0 || mi >= (int)bar->n_menus)
        return r;
    int left = MenuBar_title_x(bar, mi);
    int tw = text_measure(FONT_CHICAGO, bar->menus[mi].title);
    if (left < 0 || tw <= 0)
        return r;
    r.top    = 0;
    r.left   = (int16_t)(left + FLAIR_MENU_TITLE_PAD -
                         FLAIR_MENU_TITLE_HILITE_PAD);
    r.bottom = (int16_t)(FLAIR_MENUBAR_H - 1);
    r.right  = (int16_t)(left + FLAIR_MENU_TITLE_PAD + tw +
                         FLAIR_MENU_TITLE_HILITE_PAD);
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
 * (the cursor sequence after the click; the LAST point is the release).
 *
 * Ref: Inside Macintosh Vol I "Menu Manager" MenuSelect -- while the button is
 * down, the title CURRENTLY under the cursor is the one dropped: dragging out
 * of the clicked title into a different one CLOSES the old pull-down and OPENS
 * the new one; releasing inside a panel selects that menu's item (docs/
 * research/gui-ground-truth.md Sec 3.2). So the tracked menu index is re-hit
 * from the bar EVERY tracked point whose y is still within the bar band
 * [0, FLAIR_MENUBAR_H) -- a hit on a DIFFERENT title switches the tracked
 * menu; a point below the bar (in a panel) tracks the item within whichever
 * menu is CURRENTLY open (initech-rl4v: menu.c used to capture mi ONCE at the
 * click and never re-hit, making cross-menu drag structurally impossible).
 * The hilited item follows the cursor (MenuInfo_item_at) against the
 * currently-tracked menu. On release, return the selection IFF the released
 * item (in the FINAL tracked menu) is selectable, else 0.
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

    int hi = -1;

    /* Track the cursor: each point may switch the open menu (if it is in the
     * bar band and over a DIFFERENT title) and always updates the hilited
     * item against whichever menu is currently tracked. */
    for (int p = 0; p < n_pts; p++) {
#if defined(MENU_MUT_NO_REHIT) && MENU_MUT_NO_REHIT
        /* NAMED MUTANT (Rule 6): freeze mi at the click -- the ORIGINAL
         * initech-rl4v bug. Never re-hit the bar, so a cross-menu drag can
         * never switch the tracked menu. */
#else
        if (pts[p].v >= 0 && pts[p].v < (int)FLAIR_MENUBAR_H) {
            int nb = MenuBar_hit(bar, (int)pts[p].h);
            if (nb >= 0)
                mi = nb;                /* a different title -> switch open menu */
        }
#endif
        hi = MenuInfo_item_at(bar, mi, (int)pts[p].h, (int)pts[p].v);
    }

    if (out_hi)
        *out_hi = hi;                   /* the FINAL tracked menu's item        */

    /* The selection is the item under the RELEASE point (the last pts entry),
     * evaluated against the FINAL tracked menu (mi, after any re-hits above).
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

    return MenuResult(bar->menus[mi].menuID, (uint16_t)(rel + 1)); /* 1-based  */
}

uint32_t MenuSelect(const MenuBar *bar, flair_point_t startPt,
                    const flair_point_t *pts, int n_pts)
{
    return flair_menu_track(bar, startPt, pts, n_pts, NULL);
}

#if !defined(MENU_MUT_BAR_FLAT) || !MENU_MUT_BAR_FLAT
/* One-pixel corner transcription from sys8/menus.md Sec 1.2. Entries not in a
 * row's prefix retain the sampled vertical bar profile. The right corner is the
 * exact mirror. Values are PART names, never palette indices (DEC-09 C-8). */
static const uint8_t menu_corner_n[9] = { 8u, 8u, 5u, 4u, 3u, 2u, 2u, 2u, 1u };
static const uint8_t menu_corner_part[9][8] = {
    { FLAIR_PART_FRAME, FLAIR_PART_FRAME, FLAIR_PART_FRAME, FLAIR_PART_FRAME,
      FLAIR_PART_FRAME, FLAIR_PART_PLAT_INACTIVE_FRAME, FLAIR_PART_PLAT_WELL,
      FLAIR_PART_MENU_BAR_FACE },
    { FLAIR_PART_FRAME, FLAIR_PART_FRAME, FLAIR_PART_FRAME,
      FLAIR_PART_PLAT_INACTIVE_FRAME, FLAIR_PART_PLAT_WELL,
      FLAIR_PART_MENU_BAR_HL, FLAIR_PART_MENU_BAR_HL, FLAIR_PART_MENU_BAR_HL },
    { FLAIR_PART_FRAME, FLAIR_PART_FRAME, FLAIR_PART_PLAT_INACTIVE_FRAME,
      FLAIR_PART_MENU_BAR_FACE, FLAIR_PART_MENU_BAR_HL },
    { FLAIR_PART_FRAME, FLAIR_PART_PLAT_INACTIVE_FRAME,
      FLAIR_PART_MENU_BAR_FACE, FLAIR_PART_MENU_BAR_HL },
    { FLAIR_PART_FRAME, FLAIR_PART_PLAT_WELL, FLAIR_PART_MENU_BAR_HL },
    { FLAIR_PART_PLAT_INACTIVE_FRAME, FLAIR_PART_MENU_BAR_HL },
    { FLAIR_PART_PLAT_WELL, FLAIR_PART_MENU_BAR_HL },
    { FLAIR_PART_MENU_BAR_FACE, FLAIR_PART_MENU_BAR_HL },
    { FLAIR_PART_MENU_BAR_HL }
};

static void draw_menu_bar_corners(GrafPort *port, const region_t *clip)
{
    const bitmap_t *bm = &port->portBits.bm;
    for (int y = 0; y < 9; y++) {
        for (int x = 0; x < (int)menu_corner_n[y]; x++) {
            uint32_t pxv = flair_look_pixel(port, menu_corner_part[y][x]);
            rgn_rect_t lpx = { (int16_t)y, (int16_t)x,
                               (int16_t)(y + 1), (int16_t)(x + 1) };
            blitter_fill_rect_clipped(bm, lpx, pxv, clip);
            if ((uint32_t)x < bm->width) {
                int rx = (int)bm->width - 1 - x;
                rgn_rect_t rpx = { (int16_t)y, (int16_t)rx,
                                   (int16_t)(y + 1), (int16_t)(rx + 1) };
                blitter_fill_rect_clipped(bm, rpx, pxv, clip);
            }
        }
    }
}
#endif

/* --------------------------------------------------------------------------
 * DrawMenuBar -- sampled Platinum 20px profile + explicit pulled-title state.
 * Ref: sys8/menus.md Sec 1.1-1.4; DEC-10 Sec 6 OQ-2/OQ-3; initech-sjvq.
 * -------------------------------------------------------------------------- */
void DrawMenuBar(GrafPort *port, const MenuBar *bar,
                 int hilited_menu, const region_t *clip)
{
    if (!port || !bar)
        return;
    const bitmap_t *bm = &port->portBits.bm;
    uint32_t ink = flair_look_pixel(port, FLAIR_PART_FRAME);
    uint32_t text_ink = flair_look_pixel(port, FLAIR_PART_TEXT);
    uint32_t white = flair_look_pixel(port, FLAIR_PART_CONTENT);
    uint32_t bar_hl = flair_look_pixel(port, FLAIR_PART_MENU_BAR_HL);
    uint32_t bar_face = flair_look_pixel(port, FLAIR_PART_MENU_BAR_FACE);
    uint32_t bar_shadow = flair_look_pixel(port, FLAIR_PART_MENU_BAR_SHADOW);

#if defined(MENU_MUT_BAR_FLAT) && MENU_MUT_BAR_FLAT
    /* NAMED MUTANT: the old flat face, with neither sampled profile nor round. */
    (void)bar_hl;
    (void)bar_shadow;
    rgn_rect_t flat = { 0, 0, (int16_t)(FLAIR_MENUBAR_H - 1),
                        (int16_t)bm->width };
    blitter_fill_rect_clipped(bm, flat, bar_face, clip);
#else
    rgn_rect_t top = { 0, 0, 1, (int16_t)bm->width };
    rgn_rect_t face = { 1, 0, (int16_t)(FLAIR_MENUBAR_H - 2),
                        (int16_t)bm->width };
    rgn_rect_t shadow = { (int16_t)(FLAIR_MENUBAR_H - 2), 0,
                          (int16_t)(FLAIR_MENUBAR_H - 1),
                          (int16_t)bm->width };
    blitter_fill_rect_clipped(bm, top, bar_hl, clip);
    blitter_fill_rect_clipped(bm, face, bar_face, clip);
    blitter_fill_rect_clipped(bm, shadow, bar_shadow, clip);
#endif

    rgn_rect_t baseline = { (int16_t)(FLAIR_MENUBAR_H - 1), 0,
                            (int16_t)FLAIR_MENUBAR_H,
                            (int16_t)bm->width };
    blitter_fill_rect_clipped(bm, baseline, ink, clip);

#if !defined(MENU_MUT_BAR_FLAT) || !MENU_MUT_BAR_FLAT
    draw_menu_bar_corners(port, clip);
#endif

    /* Apple stays the locked monochrome authored strike pending D3-3 row 32. */
    if (bar->has_apple) {
        rgn_rect_t apple;
        apple.top = (int16_t)FLAIR_MENU_TITLE_VPAD;
        apple.left = (int16_t)FLAIR_MENU_TITLE_VPAD;
        apple.bottom = (int16_t)(FLAIR_MENUBAR_H - FLAIR_MENU_TITLE_VPAD - 1);
        apple.right = (int16_t)(FLAIR_MENU_APPLE_W - FLAIR_MENU_TITLE_VPAD);

#if defined(MENU_MUT_APPLE_SQUARE) && MENU_MUT_APPLE_SQUARE
        blitter_fill_rect_clipped(bm, apple, ink, clip);
#else
        for (int gy = 0; gy < APPLE_GLYPH_H; gy++) {
            uint16_t bits = APPLE_GLYPH_ROWS[gy];
            for (int gx = 0; gx < APPLE_GLYPH_W; gx++) {
                if (bits & (uint16_t)(0x8000u >> gx)) {
                    rgn_rect_t px = { (int16_t)(apple.top + gy),
                                      (int16_t)(apple.left + gx),
                                      (int16_t)(apple.top + gy + 1),
                                      (int16_t)(apple.left + gx + 1) };
                    blitter_fill_rect_clipped(bm, px, ink, clip);
                }
            }
        }
#endif
    }

#if defined(MENU_MUT_TITLE_NO_HILITE) && MENU_MUT_TITLE_NO_HILITE
    hilited_menu = -1;
#endif
    if (hilited_menu >= 0 && hilited_menu < (int)bar->n_menus) {
        rgn_rect_t hr = MenuBar_hilite_rect(bar, hilited_menu);
        uint32_t htop = flair_look_pixel(port, FLAIR_PART_MENU_TITLE_HILITE_HL);
        uint32_t hface = flair_look_pixel(port, FLAIR_PART_MENU_TITLE_HILITE_FACE);
        uint32_t hshadow = flair_look_pixel(port,
                                            FLAIR_PART_MENU_TITLE_HILITE_SHADOW);
        rgn_rect_t r0 = { 0, hr.left, 1, hr.right };
        rgn_rect_t rf = { 1, hr.left, (int16_t)(FLAIR_MENUBAR_H - 2), hr.right };
        rgn_rect_t rs = { (int16_t)(FLAIR_MENUBAR_H - 2), hr.left,
                          (int16_t)(FLAIR_MENUBAR_H - 1), hr.right };
        blitter_fill_rect_clipped(bm, r0, htop, clip);
        blitter_fill_rect_clipped(bm, rf, hface, clip);
        blitter_fill_rect_clipped(bm, rs, hshadow, clip);
    }

    for (int k = 0; k < (int)bar->n_menus; k++) {
        int x = MenuBar_title_x(bar, k);
        if (x < 0)
            continue;
        uint32_t title_fg = (k == hilited_menu) ? white : text_ink;
        uint32_t title_bg = (k == hilited_menu)
                            ? flair_look_pixel(port,
                                  FLAIR_PART_MENU_TITLE_HILITE_FACE)
                            : bar_face;
        text_draw(bm, x + FLAIR_MENU_TITLE_PAD, FLAIR_MENU_TITLE_VPAD,
                  bar->menus[k].title, FONT_CHICAGO, title_fg, title_bg);
    }
}

void HiliteMenu(GrafPort *port, const MenuBar *bar, int mi,
                const region_t *clip)
{
    DrawMenuBar(port, bar, mi, clip);
}

/* --------------------------------------------------------------------------
 * flair_draw_menu_panel -- sampled black/bevel/drop frame + item anatomy.
 * Tracking keeps the unresolved classic invert (D3.b row 47) unchanged.
 * -------------------------------------------------------------------------- */
void flair_draw_menu_panel(GrafPort *port, const MenuBar *bar, int mi,
                           int hilite_item, const region_t *clip)
{
    if (!port || !bar || mi < 0 || mi >= (int)bar->n_menus)
        return;
    const bitmap_t *bm = &port->portBits.bm;
    rgn_rect_t panel = MenuInfo_panel_rect(bar, mi);
    if (panel.right <= panel.left || panel.bottom <= panel.top)
        return;

    uint32_t frame = flair_look_pixel(port, FLAIR_PART_FRAME);
    uint32_t text_ink = flair_look_pixel(port, FLAIR_PART_TEXT);
    uint32_t face = flair_look_pixel(port, FLAIR_PART_MENU_PANEL_FACE);
    uint32_t bevel_hl = flair_look_pixel(port, FLAIR_PART_MENU_PANEL_HL);
    uint32_t bevel_shadow = flair_look_pixel(port, FLAIR_PART_MENU_PANEL_SHADOW);
    uint32_t drop = flair_look_pixel(port, FLAIR_PART_MENU_DROP_SHADOW);
    uint32_t disabled = flair_look_pixel(port, FLAIR_PART_MENU_DISABLED_INK);
#if defined(MENU_MUT_NO_PANEL_BEVEL) && MENU_MUT_NO_PANEL_BEVEL
    (void)bevel_shadow;
#endif

    /* The menu shadow is sampled #3F3F3F, deliberately distinct from the black
     * window shadow. It extends exactly one pixel right and bottom. */
    rgn_rect_t drop_r = { (int16_t)(panel.top + FLAIR_MENU_DROP_SHADOW),
                          panel.right,
                          (int16_t)(panel.bottom + FLAIR_MENU_DROP_SHADOW),
                          (int16_t)(panel.right + FLAIR_MENU_DROP_SHADOW) };
    rgn_rect_t drop_b = { panel.bottom,
                          (int16_t)(panel.left + FLAIR_MENU_DROP_SHADOW),
                          (int16_t)(panel.bottom + FLAIR_MENU_DROP_SHADOW),
                          (int16_t)(panel.right + FLAIR_MENU_DROP_SHADOW) };
    blitter_fill_rect_clipped(bm, drop_r, drop, clip);
    blitter_fill_rect_clipped(bm, drop_b, drop, clip);
    blitter_fill_rect_clipped(bm, panel, face, clip);

    const MenuInfo *m = &bar->menus[mi];
    int draw_hi = (hilite_item >= 0 &&
                   MenuInfo_item_selectable(bar, mi, hilite_item))
                  ? hilite_item : -1;
    int text_x = panel.left + FLAIR_MENU_ITEM_LPAD;

    for (int k = 0; k < (int)m->n_items; k++) {
        const MenuItem *it = &m->items[k];
        int row_top = item_row_top(m, k);
        int h = item_row_h(it);

        if (it->is_divider) {
#if defined(MENU_MUT_SEP_PLAIN) && MENU_MUT_SEP_PLAIN
            rgn_rect_t plain = { (int16_t)(row_top + h / 2),
                                 (int16_t)(panel.left + FLAIR_MENU_PANEL_INSET),
                                 (int16_t)(row_top + h / 2 + 1),
                                 (int16_t)(panel.right - FLAIR_MENU_PANEL_INSET) };
            blitter_fill_rect_clipped(bm, plain, disabled, clip);
#else
            rgn_rect_t dark = { (int16_t)(row_top + 1),
                                (int16_t)(panel.left + FLAIR_MENU_PANEL_INSET),
                                (int16_t)(row_top + 2),
                                (int16_t)(panel.right - FLAIR_MENU_PANEL_INSET) };
            rgn_rect_t light = { (int16_t)(row_top + 2),
                                 (int16_t)(panel.left + FLAIR_MENU_PANEL_INSET),
                                 (int16_t)(row_top + 3),
                                 (int16_t)(panel.right - FLAIR_MENU_PANEL_INSET) };
            blitter_fill_rect_clipped(bm, dark, disabled, clip);
            blitter_fill_rect_clipped(bm, light, bevel_hl, clip);
#endif
            continue;
        }

        uint32_t row_fg = text_ink;
        uint32_t row_bg = face;
        bitmap_t row_bm = *bm;
        if ((uint32_t)(row_top + h) < row_bm.height)
            row_bm.height = (uint32_t)(row_top + h);
#if defined(MENU_MUT_DISABLED_NORMAL_INK) && MENU_MUT_DISABLED_NORMAL_INK
        (void)disabled;
#else
        if (!it->enabled)
            row_fg = disabled;
#endif
        if (k == draw_hi) {
            rgn_rect_t band = { (int16_t)row_top,
                                (int16_t)(panel.left + FLAIR_MENU_PANEL_INSET),
                                (int16_t)(row_top + h),
                                (int16_t)(panel.right - FLAIR_MENU_PANEL_INSET) };
            blitter_fill_rect_clipped(bm, band, frame, clip);
            row_fg = face;
            row_bg = frame;
        }

        if (it->mark) {
            char marks[2];
            marks[0] = it->mark;
            marks[1] = 0;
            text_draw(&row_bm, panel.left + FLAIR_MENU_PANEL_INSET + 2,
                      row_top + 2, marks, FONT_CHICAGO, row_fg, row_bg);
        }
        text_draw(&row_bm, text_x, row_top + 2, it->text,
                  FONT_CHICAGO, row_fg, row_bg);

        if (it->cmdChar) {
            /* The source strike has no cloverleaf. Render the period-plausible
             * caret-letter form required by initech-sjvq; do not author a glyph
             * in this lane. The two-cell run is right-aligned like x=199..216 in
             * the sampled 198px File panel (sys8/menus.md Sec 2.2/2.3). */
            char cmd[3];
            cmd[0] = '^';
            cmd[1] = cmd_fold(it->cmdChar);
            cmd[2] = 0;
            int cmd_x = panel.right - FLAIR_MENU_CMD_RPAD -
                        text_measure(FONT_CHICAGO, cmd);
            text_draw(&row_bm, cmd_x, row_top + 2, cmd,
                      FONT_CHICAGO, row_fg, row_bg);
        }
    }

    /* Last: sampled inner bevel and outer black frame cover the last item's
     * final two rows, exactly as the captured panel anatomy does. */
#if !defined(MENU_MUT_NO_PANEL_BEVEL) || !MENU_MUT_NO_PANEL_BEVEL
    rgn_rect_t hi_top = { (int16_t)(panel.top + 1),
                          (int16_t)(panel.left + 1),
                          (int16_t)(panel.top + 2),
                          (int16_t)(panel.right - 1) };
    rgn_rect_t hi_left = { (int16_t)(panel.top + 1),
                           (int16_t)(panel.left + 1),
                           (int16_t)(panel.bottom - 1),
                           (int16_t)(panel.left + 2) };
    rgn_rect_t sh_bottom = { (int16_t)(panel.bottom - 2),
                             (int16_t)(panel.left + 2),
                             (int16_t)(panel.bottom - 1),
                             (int16_t)(panel.right - 1) };
    rgn_rect_t sh_right = { (int16_t)(panel.top + 2),
                            (int16_t)(panel.right - 2),
                            (int16_t)(panel.bottom - 1),
                            (int16_t)(panel.right - 1) };
    blitter_fill_rect_clipped(bm, hi_top, bevel_hl, clip);
    blitter_fill_rect_clipped(bm, hi_left, bevel_hl, clip);
    blitter_fill_rect_clipped(bm, sh_bottom, bevel_shadow, clip);
    blitter_fill_rect_clipped(bm, sh_right, bevel_shadow, clip);
#endif

    rgn_rect_t fr_top = { panel.top, panel.left,
                          (int16_t)(panel.top + 1), panel.right };
    rgn_rect_t fr_bot = { (int16_t)(panel.bottom - 1), panel.left,
                          panel.bottom, panel.right };
    rgn_rect_t fr_lft = { panel.top, panel.left, panel.bottom,
                          (int16_t)(panel.left + 1) };
    rgn_rect_t fr_rgt = { panel.top, (int16_t)(panel.right - 1),
                          panel.bottom, panel.right };
    blitter_fill_rect_clipped(bm, fr_top, frame, clip);
    blitter_fill_rect_clipped(bm, fr_bot, frame, clip);
    blitter_fill_rect_clipped(bm, fr_lft, frame, clip);
    blitter_fill_rect_clipped(bm, fr_rgt, frame, clip);
}
