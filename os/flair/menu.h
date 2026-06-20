/*
 * os/flair/menu.h -- the FLAIR Menu Manager (THE ARTIFACT, FLAIR Layer 3).
 *
 * beads: initech-n3e ("FLAIR Menu Manager: pull-down menus + the menu bar,
 *        Photoshop-exact"). The Menu Manager (ADR-0004 D-3) owns the menu bar
 *        and the pull-down menus: the verbatim Inside Macintosh MenuInfo record,
 *        the proportional-text bar layout, the click-to-drop pull-down tracking,
 *        and MenuSelect / MenuKey returning the (menuID<<16 | item) packing.
 *
 * Ref:   ADR-0004 D-3 ("Menu Manager -- MenuInfo (menu ID, title, items with
 *          mark/style/cmd-char); the menu bar including the Photoshop-exact bar
 *          for InitechPaint ('File Edit Image Layer Select View Window Help' --
 *          canon, NOT to be 'corrected'). MenuSelect returns (menuID<<16|item).")
 *        ADR-0004 D-1 (5-layer stack; Layer 3 Managers draw THROUGH a GrafPort
 *          clipped by an ATKINSON region; never touch the framebuffer directly).
 *        ADR-0004 D-7 ("proportional NFNT text measurement -- Chicago ...; text
 *          width = sum of per-glyph advances; no fixed-pitch assumption").
 *        spec/assets/menu_canon.h (the FROZEN canon menu-bar string + items --
 *          USED here, NOT re-authored; CLAUDE.md Law 4 / AM-4).
 *        spec/chrome_metrics.h (FLAIR_CHROME_MENUBAR_H = 20; GetMBarHeight).
 *        os/flair/text.h (text_measure / text_draw -- proportional Chicago).
 *        os/flair/blitter.h (blitter_fill_rect_clipped -- region-clipped fill).
 *        os/flair/surface.h (bitmap_t; the ONE pixel writer).
 *        os/flair/atkinson/region.h, spec/region_algebra.h (region_t clip).
 *        spec/grafport.h (GrafPort, flair_point_t).
 *        Inside Macintosh Vol I "Menu Manager" -- the MenuInfo record, the
 *          enableFlags / mark / cmdChar item attributes, NewMenuBar/DrawMenuBar,
 *          MenuSelect/MenuKey verbatim names and the (menuID<<16|item) result.
 *        CLAUDE.md Law 2 (oracle is truth), Law 3 (freestanding artifact +
 *        dual-compile), Law 4 (look like the frame -- the canon bar), Rule 2
 *        (fail loud), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * ARTIFACT code: freestanding C (ADR-0002). No libc. No malloc -- ALL storage is
 * CALLER-supplied (the FLAIR heap backs the MenuInfo/MenuBar arrays; this module
 * allocates nothing). Compiles BOTH under kernel flags (gcc -m32 -ffreestanding
 * -nostdlib -std=c11 -Wall -Wextra -Werror) AND hosted for the property suite
 * (harness/proptest/test_menu.c), the region.c / blitter.c / window.c pattern.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_MENU_H
#define INITECH_OS_FLAIR_MENU_H

#include <stdint.h>
#include <stddef.h>

#include "surface.h"            /* bitmap_t + the ONE pixel writer            */
#include "text.h"               /* text_measure / text_draw (proportional)    */
#include "blitter.h"            /* blitter_fill_rect_clipped (region-clipped) */
#include "region_algebra.h"     /* region_t, rgn_rect_t (-Ispec)              */
#include "grafport.h"           /* GrafPort, flair_point_t (-Ispec)           */

/* ===========================================================================
 * 1. MENU-BAR LAYOUT CONSTANTS  (Law 1 -- grounded in chrome_metrics + IM)
 * ---------------------------------------------------------------------------
 * The menu bar is FLAIR_CHROME_MENUBAR_H (20) px tall (GetMBarHeight, Roman
 * script; chrome_metrics.h). Inside Macintosh draws each title with a small
 * left/right pad and an Apple-menu glyph slot at the far left. The padding here
 * is the standard System-7 menu-title side padding; it is the SAME value the
 * oracle re-derives independently (test_menu.c) so the layout is machine-checked,
 * not eyeballed.
 * ===========================================================================*/

/* The menu bar height. Aliased from chrome_metrics.h so there is one source of
 * truth (the locked spec); a divergence would fail the chrome consistency
 * tooth. We do NOT redefine the number -- we re-export the locked macro. */
#include "chrome_metrics.h"     /* FLAIR_CHROME_MENUBAR_H = 20 (-Ispec)       */
#define FLAIR_MENUBAR_H          FLAIR_CHROME_MENUBAR_H

/* Standard horizontal padding (pixels) on EACH side of a menu title in the bar
 * (Inside Macintosh Vol I "Menu Manager" -- titles are drawn with a side gap so
 * adjacent titles do not touch; the System-7 value is a small even pad). The
 * hit/draw width of a title is text_measure(title) + 2*FLAIR_MENU_TITLE_PAD. */
#define FLAIR_MENU_TITLE_PAD     7

/* The Apple-menu glyph slot at the far left of the bar. The bar begins with a
 * fixed-width slot for the Apple logo (the classic System-7 apple menu); the
 * first real title (File) starts at FLAIR_MENU_APPLE_W. The slot is a square the
 * height of the bar so the apple glyph centers in it. */
#define FLAIR_MENU_APPLE_W       FLAIR_MENUBAR_H

/* Vertical inset of the title baseline cell inside the 20px bar so Chicago text
 * (CHICAGO_CELL_H = 16) is vertically centered: (20 - 16) / 2 = 2. */
#define FLAIR_MENU_TITLE_VPAD    ((FLAIR_MENUBAR_H - 16) / 2)

/* ===========================================================================
 * 2. PULL-DOWN PANEL LAYOUT CONSTANTS
 * ---------------------------------------------------------------------------
 * A dropped menu is a rectangular panel below its title. Each item is a row
 * FLAIR_MENU_ITEM_H px tall; the item text is inset FLAIR_MENU_ITEM_LPAD on the
 * left (room for the mark/check column) and the panel is at least as wide as the
 * widest item + the left + right pad. A divider row is FLAIR_MENU_DIV_H tall and
 * never selectable.
 * ===========================================================================*/
#define FLAIR_MENU_ITEM_H        16   /* per-item row height (Chicago cell)    */
#define FLAIR_MENU_ITEM_LPAD     16   /* left inset (mark/check column)         */
#define FLAIR_MENU_ITEM_RPAD     12   /* right inset (cmd-key column gap)       */
#define FLAIR_MENU_DIV_H          8   /* divider row height (a separating line) */
#define FLAIR_MENU_PANEL_FRAME    1   /* 1px panel frame (Mac drop-shadow box)  */

/* ===========================================================================
 * 3. MENU ITEM + MENU + MENU BAR RECORDS  (verbatim Inside Macintosh)
 * ---------------------------------------------------------------------------
 * MenuItem carries the IM "menu item" attributes: text, a MARK character (a
 * check or diamond drawn in the mark column; 0 == no mark), a command-key
 * equivalent (cmdChar; 0 == none), a style byte (FLAIR_STYLE_* bits, shared
 * with grafport.h txFace), an enabled flag, and an is_divider flag. A divider
 * or a disabled item is NEVER selectable (IM: a dimmed item / a "(-" divider
 * cannot be chosen).
 * ===========================================================================*/

typedef struct MenuItem {
    const char *text;       /* item text (NUL-terminated, ASCII; Rule 12)      */
    char        mark;       /* mark char in the mark column (0 == no mark)      */
    char        cmdChar;    /* command-key equivalent (0 == none); MenuKey key  */
    uint8_t     style;      /* FLAIR_STYLE_* bits (bold/italic/...) for drawing */
    uint8_t     enabled;    /* 1 == selectable; 0 == dimmed, NOT selectable     */
    uint8_t     is_divider; /* 1 == a separator row, NEVER selectable           */
} MenuItem;

/*
 * MenuInfo -- one pull-down menu (Inside Macintosh "MenuInfo"): a menuID, a
 * title (the word in the bar), and an ordered array of items. The items[] array
 * is CALLER-supplied storage (no malloc); n_items is its live length. menuWidth
 * caches the pull-down panel width once laid out (0 == not yet computed).
 */
typedef struct MenuInfo {
    int16_t          menuID;     /* the menu id (>=1; used in the result word)  */
    const char      *title;      /* the bar title (e.g. "File"); ASCII          */
    const MenuItem  *items;      /* caller-supplied item array (no malloc)      */
    uint16_t         n_items;    /* number of live items in items[]             */
    int16_t          menuWidth;  /* cached pull-down panel width (0 == unset)   */
} MenuInfo;

/*
 * MenuBar -- the ordered list of menus shown across the top (Inside Macintosh
 * "menu bar"). Caller-supplied MenuInfo array (no malloc). has_apple selects the
 * Apple-menu glyph slot at the far left (the canon InitechPaint bar carries it).
 */
typedef struct MenuBar {
    MenuInfo  *menus;       /* caller-supplied array of menus                   */
    uint16_t   n_menus;     /* number of live menus                            */
    uint8_t    has_apple;   /* 1 == draw the Apple-menu glyph slot at the left  */
} MenuBar;

/* ===========================================================================
 * 4. RESULT WORD HELPERS  (ADR-0004 D-3: result == menuID<<16 | item)
 * ---------------------------------------------------------------------------
 * MenuSelect / MenuKey pack the chosen (menuID, item) into a uint32_t exactly
 * as Inside Macintosh: the high 16 bits are the menuID, the low 16 bits are the
 * 1-BASED item number (IM menu items are 1-based: item 1 is the first item). A
 * result of 0 means "nothing chosen" (high word 0 is never a valid menuID).
 * ===========================================================================*/

/* Pack (menuID, item-1-based) into the IM result word. */
static inline uint32_t MenuResult(int16_t menuID, uint16_t item1based)
{
    return ((uint32_t)(uint16_t)menuID << 16) | (uint32_t)item1based;
}
/* Extract the menuID (high word) from an IM result word. */
static inline int16_t MenuResultID(uint32_t r)   { return (int16_t)(r >> 16); }
/* Extract the 1-based item number (low word) from an IM result word. */
static inline uint16_t MenuResultItem(uint32_t r){ return (uint16_t)(r & 0xFFFFu); }

/* ===========================================================================
 * 5. BAR LAYOUT QUERIES  (proportional text; D-7)
 * ---------------------------------------------------------------------------
 * The bar lays each title out at a running x: starting at FLAIR_MENU_APPLE_W if
 * has_apple (else 0), each title occupies [x, x + PAD + text_measure + PAD) and
 * the next title starts where this one ends. The hit/draw width of a title is
 * text_measure(FONT_CHICAGO, title) + 2*FLAIR_MENU_TITLE_PAD.
 * ===========================================================================*/

/*
 * MenuBar_title_x -- the LEFT x of menu index i's title slot in the bar.
 * Cumulative: sum of all earlier title slot widths (+ the Apple slot if any).
 * Returns -1 if i is out of range.
 */
int MenuBar_title_x(const MenuBar *bar, int i);

/*
 * MenuBar_title_w -- the WIDTH of menu index i's title slot (PAD + text + PAD).
 * Returns 0 if i is out of range.
 */
int MenuBar_title_w(const MenuBar *bar, int i);

/*
 * MenuBar_hit -- which menu title is at bar x-coordinate `x` (y is implicitly
 * within the bar). Returns the menu INDEX (0-based) or -1 if x is in the Apple
 * slot or past the last title. The hit test uses the half-open slot
 * [title_x, title_x + title_w).
 */
int MenuBar_hit(const MenuBar *bar, int x);

/* ===========================================================================
 * 6. PULL-DOWN PANEL LAYOUT
 * ---------------------------------------------------------------------------
 * MenuInfo_panel_rect computes the dropped panel rectangle for menu index `mi`
 * in `bar`: it drops directly below that title (left edge aligned with the title
 * slot left), is as wide as the widest item (+ left + right pad), and as tall as
 * the sum of the item rows (FLAIR_MENU_ITEM_H per normal item, FLAIR_MENU_DIV_H
 * per divider) plus the 1px frame. The panel TOP is at FLAIR_MENUBAR_H (just
 * below the bar).
 * ===========================================================================*/

/* Compute the pull-down panel rect for menu index `mi`. Returns an empty rect
 * (all zero) if mi is out of range. */
rgn_rect_t MenuInfo_panel_rect(const MenuBar *bar, int mi);

/*
 * MenuInfo_item_at -- which item row in the dropped panel of menu `mi` is at
 * panel-relative cursor point (x,y) in SCREEN coordinates. Returns the 0-based
 * ITEM INDEX whose row band contains y (and x within the panel), or -1 if the
 * point is outside the panel or in a divider row. Note: a DISABLED item still
 * returns its index here (geometry only); selectability is enforced by the
 * tracking loop (a disabled/divider item is never the selection).
 */
int MenuInfo_item_at(const MenuBar *bar, int mi, int x, int y);

/* MenuInfo_item_selectable -- 1 iff item index `it` of menu `mi` may be chosen
 * (enabled AND not a divider). The decisive selectability predicate. */
int MenuInfo_item_selectable(const MenuBar *bar, int mi, int it);

/* ===========================================================================
 * 7. DRAWING  (through a GrafPort, clipped; ADR-0004 D-1/D-2)
 * ---------------------------------------------------------------------------
 * DrawMenuBar paints the whole 20px bar across the top of the port's bitmap via
 * text_draw (Chicago) + blitter_fill_rect_clipped; the Apple slot is drawn as a
 * filled glyph cell. flair_draw_menu_panel paints a dropped panel and HILITEs
 * the item under the cursor (an inverted band). Both clip to the port's effective
 * region (visRgn INTERSECT clipRgn); a NULL clip means the whole bitmap.
 * ===========================================================================*/

/* DrawMenuBar -- render the menu bar into `port`'s bitmap (top FLAIR_MENUBAR_H
 * rows). Uses text_draw(FONT_CHICAGO). clip is the effective clip (NULL = none).
 * fg/bg are packed 0x00RRGGBB (surface.h). */
void DrawMenuBar(GrafPort *port, const MenuBar *bar,
                 uint32_t fg, uint32_t bg, const region_t *clip);

/*
 * flair_draw_menu_panel -- render the dropped panel of menu `mi`, hiliting item
 * `hilite_item` (0-based; -1 == none). Disabled/divider rows are never hilited
 * (a hilite request for a non-selectable row draws no hilite band). clip as
 * above. fg/bg are packed colors.
 */
void flair_draw_menu_panel(GrafPort *port, const MenuBar *bar, int mi,
                           int hilite_item,
                           uint32_t fg, uint32_t bg, const region_t *clip);

/* ===========================================================================
 * 8. TRACKING  (MenuSelect / MenuKey -- the verbatim Inside Macintosh API)
 * ---------------------------------------------------------------------------
 * MenuSelect is driven by a SUPPLIED POINT SEQUENCE (the event deltas the Event
 * Manager would feed in task context; ADR-0004 D-4). This keeps the Menu Manager
 * host-testable (deterministic, Rule 11) without baking in an event-pump
 * dependency: the caller hands MenuSelect the click point and a sequence of
 * cursor points ending in the release point. MenuSelect hit-tests the bar at the
 * click, drops that menu, tracks the cursor through the points hiliting the item
 * under it, and on the final (release) point returns the selection.
 * ===========================================================================*/

/*
 * flair_menu_track -- the core deterministic tracking primitive (host-testable).
 *
 *   bar       -- the menu bar.
 *   startPt   -- the click point (where the mouse went down, screen coords).
 *   pts       -- the cursor point sequence AFTER the click (screen coords); the
 *                LAST point in the array is the RELEASE point. May be NULL/0.
 *   n_pts     -- length of pts[].
 *   out_hi    -- (optional, may be NULL) receives the final hilited item index
 *                (0-based) the tracking ended on, or -1 if none (for the drawer).
 *
 * Returns the IM result word (menuID<<16 | item-1based), or 0 if:
 *   - startPt is not on a menu title (no menu dropped), OR
 *   - the release point is outside any item row, OR
 *   - the release point lands on a DISABLED item or a DIVIDER (not selectable).
 *
 * Deterministic: the same (bar, startPt, pts) yields the same result every run.
 */
uint32_t flair_menu_track(const MenuBar *bar,
                          flair_point_t startPt,
                          const flair_point_t *pts, int n_pts,
                          int *out_hi);

/*
 * MenuSelect -- Inside Macintosh "MenuSelect(startPt)": the classic entry point.
 * In FLAIR it takes the click point plus the tracked cursor sequence (see
 * flair_menu_track). Returns the IM result word, or 0 if nothing was chosen.
 */
uint32_t MenuSelect(const MenuBar *bar, flair_point_t startPt,
                    const flair_point_t *pts, int n_pts);

/*
 * MenuKey -- Inside Macintosh "MenuKey(ch)": map a command-key character to the
 * menu item with that cmdChar equivalent. Scans every menu's items for the FIRST
 * ENABLED, non-divider item whose cmdChar matches `ch` (case-insensitive for
 * letters, matching IM Command-key behavior). Returns the IM result word, or 0
 * if no enabled item has that command key.
 */
uint32_t MenuKey(const MenuBar *bar, char ch);

/* ===========================================================================
 * 9. HILITE  (Inside Macintosh "HiliteMenu")
 * ---------------------------------------------------------------------------
 * HiliteMenu inverts a menu TITLE in the bar (the classic "the chosen menu's
 * title stays inverted while its pull-down is down"). It records the hilited
 * menu index in the MenuBar-companion state the caller passes; FLAIR exposes it
 * as a pure geometry helper (the title slot rect to invert) so the drawer and
 * the oracle agree on what gets inverted.
 * ===========================================================================*/

/* HiliteMenu -- return the bar title slot rect for menu index `mi` (the rect the
 * drawer inverts to show the menu is active). Returns empty rect if out of
 * range or mi < 0 (mi < 0 == "no menu hilited"). */
rgn_rect_t HiliteMenu(const MenuBar *bar, int mi);

#endif /* INITECH_OS_FLAIR_MENU_H */
