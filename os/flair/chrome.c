/*
 * os/flair/chrome.c -- Platinum window chrome mechanism.
 *
 * The renderer owns geometry only. Every color role crosses the C-8 policy
 * seam as a FLAIR_PART_* key; this file contains no palette table and no era
 * selection. Geometry is the sampled Mac OS 8.1 base-era contract in
 * ../system7-decomp/specs/sys8/window-chrome.md Sec 1-6 and scrollbars.md
 * Sec 1-5, selected by ADR-0004-AMENDMENT-DEC-10 Sec 4 / OQ-7.
 */

#include <stdint.h>

#include "chrome.h"
#include "surface.h"
#include "chrome_metrics.h"
#include "region_algebra.h"
#include "flair_look.h"
#include "text.h"
#include "window_record.h"  /* FLAIR_WINDOW_TITLE_MAX: kernel-held title seam */
#include "rdoc.h"           /* rDocProc geometry (bead initech-tdnl.17)        */

/* True when a port-local pixel is in visRgn INTERSECT clipRgn. */
static int clip_in(const GrafPort *port, int x, int y)
{
    if (x < 0 || y < 0) {
        return 0;
    }
    if (port->visRgn != 0 &&
        !region_contains_point(port->visRgn, (int16_t)x, (int16_t)y)) {
        return 0;
    }
    if (port->clipRgn != 0 &&
        !region_contains_point(port->clipRgn, (int16_t)x, (int16_t)y)) {
        return 0;
    }
    return 1;
}

/* Fill the clipped half-open span [x,x+w) through the explicit D-9 data seam. */
static void chrome_cfill(GrafPort *port, const flair_skin_t *skin,
                         int x, int y, int w, int part)
{
    uint32_t px;
    int run_start = -1;

    if (w <= 0) {
        return;
    }
    px = flair_look_pixel_for_skin(port, skin, part);
    for (int i = 0; i <= w; i++) {
        int cx = x + i;
        int in = (i < w) ? clip_in(port, cx, y) : 0;
        if (in && run_start < 0) {
            run_start = cx;
        } else if (!in && run_start >= 0) {
            surface_fill_span(&port->portBits.bm,
                              (uint32_t)run_start, (uint32_t)y,
                              (uint32_t)(cx - run_start), px);
            run_start = -1;
        }
    }
}

/* The title run, CLIPPED like every other chrome primitive (bead
 * initech-tdnl.14). text_draw -> surface_blit has no region clip, so a WDEF
 * repaint of a PARTLY COVERED window's title bar used to stamp the whole
 * Chicago cell run -- ink AND background -- over the window in front of it.
 * Found on the real guest: a disk tenant's exit exposed part of NOTES' title
 * bar, and NOTES' "NOTES" run overdrew the APPS window's frame + scrollbar
 * (the test-flair-app-launch RESTORE leg's byte-compare caught it). Since bead
 * initech-tdnl.68 it is the Font Manager's ONE clipped run, text_draw_run,
 * under the port's visRgn INTERSECT clipRgn: the same pixels as text_draw
 * wherever they are visible (the opaque advance-wide box in bg, the strike's
 * ink in fg), nothing where they are not. */
__attribute__((unused)) /* the TITLE_BLANK/NO_TITLE mutants compile out the call */
static void chrome_ctext(GrafPort *port, int x, int y, const char *s,
                         uint32_t fg, uint32_t bg)
{
    text_clip_t k = { port->visRgn, port->clipRgn, 0, 0, 0, 0 };
    (void)text_draw_run(&port->portBits.bm, x, y, s, FONT_CHICAGO, 0u,
                        fg, bg, &k);
}

static void chrome_crect(GrafPort *port, const flair_skin_t *skin,
                         int x0, int y0, int x1, int y1, int part)
{
    for (int y = y0; y < y1; y++) {
        chrome_cfill(port, skin, x0, y, x1 - x0, part);
    }
}

static void chrome_cframe(GrafPort *port, const flair_skin_t *skin,
                          int x0, int y0, int x1, int y1, int part)
{
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    chrome_cfill(port, skin, x0, y0, x1 - x0, part);
    chrome_cfill(port, skin, x0, y1 - 1, x1 - x0, part);
    for (int y = y0; y < y1; y++) {
        chrome_cfill(port, skin, x0, y, 1, part);
        chrome_cfill(port, skin, x1 - 1, y, 1, part);
    }
}

/* Every composer below carries a parameter named `skin`. These wrappers keep
 * the geometry call sites naming only palette PARTs while making the pointer
 * an explicit per-call datum beside the GrafPort. */
#define cfill(port, x, y, w, part) \
    chrome_cfill((port), skin, (x), (y), (w), (part))
#define crect(port, x0, y0, x1, y1, part) \
    chrome_crect((port), skin, (x0), (y0), (x1), (y1), (part))
#define cframe(port, x0, y0, x1, y1, part) \
    chrome_cframe((port), skin, (x0), (y0), (x1), (y1), (part))

/* Rect tables (size round 6, WL-0162): each entry is one crect whose edges
 * are offsets from the box's left/top or (CR_*R/B) right/bottom edge, painted
 * in table order; CR_ACT / CR_INA restrict it to the active / inactive
 * frame, and part CR_DYN takes the caller's dynamic PART. */
typedef struct { int8_t x0, y0, x1, y1; uint8_t f, part; } chrome_rect_t;
#define CR_X0R 1u
#define CR_X1R 2u
#define CR_Y0B 4u
#define CR_Y1B 8u
#define CR_ACT 16u
#define CR_INA 32u
#define CR_DYN 0xFFu
static void chrome_rects(GrafPort *port, const flair_skin_t *skin,
                         int l, int t, int r, int b,
                         const chrome_rect_t *tab, int n, int active, int dyn)
{
    for (int i = 0; i < n; i++) {
        unsigned f = tab[i].f;
        if (((f & CR_ACT) && !active) || ((f & CR_INA) && active)) continue;
        crect(port, ((f & CR_X0R) ? r : l) + tab[i].x0,
              ((f & CR_Y0B) ? b : t) + tab[i].y0,
              ((f & CR_X1R) ? r : l) + tab[i].x1,
              ((f & CR_Y1B) ? b : t) + tab[i].y1,
              tab[i].part == CR_DYN ? dyn : (int)tab[i].part);
    }
}
#define CRN(tab) ((int)(sizeof (tab) / sizeof (tab)[0]))

enum {
    PLAT_WIDGET_CLOSE = 0,
    PLAT_WIDGET_ZOOM,
    PLAT_WIDGET_COLLAPSE
};

/* Sampled Platinum widget ink is #3F3F3F (idx63), not frame black.
 * Ref: sys8/window-chrome.md Sec 3.2-3.3 (SAMPLED domain). */
#if defined(CHROME_FID_MUT_DARK_RING_BLACK)
#define PLAT_DARK_RING_PART FLAIR_PART_FRAME
#else
#define PLAT_DARK_RING_PART FLAIR_PART_PLAT_DARK_RING
#endif

/* Platinum 12x12 widget plus the one-pixel right/bottom highlight.
 * Ref: window-chrome.md Sec 3.1-3.3. The seven ramp roles are the sampled
 * diagonal sequence documented at Sec 3.2 and reuse existing policy parts.
 *
 * `pressed` (bead initech-tdnl.60, TrackGoAway/TrackBox feedback) draws the
 * box held down. Platinum pressed widget art is a documented GOLDEN GAP
 * (sys8/window-chrome.md Sec 8: "No golden shows a close/zoom/collapse box
 * under the mouse button"), so this is AUTHORED after the one local pressed
 * golden, System 7's idle->pressed delta (chrome/close-zoom-box.md "PRESSED
 * state": the outline goes solid black, the face goes to the darker hilite
 * tone, the box does not move): the dark ring becomes frame black and the
 * face ramp shifts two rungs darker. [golden-resolves: a mouse-down capture
 * of each Platinum widget.] */
#if !defined(CHROME_FID_MUT_BOX_GEOM)
static void draw_platinum_widget(GrafPort *port, const flair_skin_t *skin,
                                 int bx, int by, int kind, int pressed)
{
    const int box = FLAIR_CHROME_WIDGET_BOX;
    const int interior = FLAIR_CHROME_WIDGET_INTERIOR;
#if defined(CHROME_MUT_PRESSED_IDLE)
    /* MUTANT (Rule 6; test-chrome-mutant, initech-tdnl.60): no press
     * feedback -- the held box looks idle. NEVER in a real build. */
    pressed = 0;
#endif
#if !defined(CHROME_FID_MUT_RAMP)
    static const int ramp_part[FLAIR_CHROME_WIDGET_RAMP_RUNGS] = {
        FLAIR_PART_PLAT_FRAME_SHADOW,
        FLAIR_PART_PLAT_WELL,
        FLAIR_PART_PLAT_TILE_SHADOW,
        FLAIR_PART_PLAT_FRAME_FACE,
        FLAIR_PART_PLAT_FACE,
        FLAIR_PART_PLAT_TROUGH,
        FLAIR_PART_CONTENT
    };
#endif

    /* The fixed 13x13 cell as a rect table (size round 6, WL-0162), offsets
     * from (bx,by): E7-less DA face, A5 outer highlight, the 1-px ring
     * (black when pressed), the DA interior, its A5 shadow and the white
     * top-left pixel. The redundant DA re-fills of the interior's first
     * row/column (same PART over the same pixels) are not repeated. */
#define WX(x0, y0, x1, y1, p) { (int8_t)(x0), (int8_t)(y0), (int8_t)(x1), (int8_t)(y1), 0u, (uint8_t)(p) }
    static const chrome_rect_t cell[] = {
        WX(0, 0, box, box, FLAIR_PART_PLAT_FRAME_FACE),
        WX(0, 0, box, 1, FLAIR_PART_PLAT_WIDGET_EDGE),
        WX(0, 0, 1, box, FLAIR_PART_PLAT_WIDGET_EDGE),
        WX(1, 1, box, 2, CR_DYN), WX(1, box - 1, box, box, CR_DYN),
        WX(1, 1, 2, box, CR_DYN), WX(box - 1, 1, box, box, CR_DYN),
        WX(2, 2, 2 + interior, 2 + interior, FLAIR_PART_PLAT_FRAME_FACE),
        /* Interior shadow: the bottom row (dy=8) runs dx=1..8 and the right
         * column (dx=8) runs dy=1..8. The two off-diagonal corners (8,0) and
         * (0,8) stay #DADADA -- they belong to the top-row/left-column
         * highlight (window-chrome.md Sec 3.2 transcription, y=76 x=79 'd'
         * and y=84 x=71 'd'; bead initech-tdnl.52, the LOOK pass 6 2/169
         * residual). */
#if defined(CHROME_FID_MUT_WIDGET_CORNERS_EDGE)
        /* Rule-6 restoration of the pre-tdnl.52 full-length shadow strokes. */
        WX(2, 1 + interior, 2 + interior, 2 + interior, FLAIR_PART_PLAT_WIDGET_EDGE),
        WX(1 + interior, 2, 2 + interior, 2 + interior, FLAIR_PART_PLAT_WIDGET_EDGE),
#else
        WX(3, 1 + interior, 2 + interior, 2 + interior, FLAIR_PART_PLAT_WIDGET_EDGE),
        WX(1 + interior, 3, 2 + interior, 2 + interior, FLAIR_PART_PLAT_WIDGET_EDGE),
#endif
        WX(2, 2, 3, 3, FLAIR_PART_CONTENT)
    };
    chrome_rects(port, skin, bx, by, 0, 0, cell, CRN(cell), 0,
                 pressed ? FLAIR_PART_FRAME : PLAT_DARK_RING_PART);

    for (int dy = 1; dy <= FLAIR_CHROME_WIDGET_RAMP_FACE; dy++) {
        for (int dx = 1; dx <= FLAIR_CHROME_WIDGET_RAMP_FACE; dx++) {
#if defined(CHROME_FID_MUT_RAMP)
            int part = FLAIR_PART_PLAT_FRAME_FACE;
#else
            int rung = (dx + dy - 2) /
                       FLAIR_CHROME_WIDGET_RAMP_PX_PER_STEP;
            if (pressed) rung = (rung >= 2) ? rung - 2 : 0;
            int part = ramp_part[rung];
#endif
            cfill(port, bx + 2 + dx, by + 2 + dy, 1, part);
        }
    }

    if (kind == PLAT_WIDGET_ZOOM) {
#if defined(CHROME_FID_MUT_ZOOM_CENTERED)
        cframe(port, bx + 4, by + 4, bx + 7, by + 7,
               PLAT_DARK_RING_PART);
#else
        for (int d = 0; d < FLAIR_CHROME_ZOOM_GLYPH_EDGE; d++) {
            cfill(port, bx + 2 + 5, by + 2 + d, 1,
                  PLAT_DARK_RING_PART);
            cfill(port, bx + 2 + d, by + 2 + 5, 1,
                  PLAT_DARK_RING_PART);
        }
#endif
    } else if (kind == PLAT_WIDGET_COLLAPSE) {
        cfill(port, bx + 2,
              by + 2 + FLAIR_CHROME_COLLAPSE_GLYPH_ROW_0,
              interior, PLAT_DARK_RING_PART);
        cfill(port, bx + 2,
              by + 2 + FLAIR_CHROME_COLLAPSE_GLYPH_ROW_1,
              interior, PLAT_DARK_RING_PART);
    }

    for (int y = by + 1; y <= by + box; y++) {
        cfill(port, bx + box, y, 1, FLAIR_PART_CONTENT);
    }
    cfill(port, bx + 1, by + box, box, FLAIR_PART_CONTENT);
#if defined(CHROME_FID_MUT_CBOX_CORNER)
    /* Rule-6 restoration of the old sz-4 corner gap (bead initech-hber). */
    cfill(port, bx + box - 1, by + box - 1, 1,
          FLAIR_PART_PLAT_WIDGET_EDGE);
#endif
}
#endif

typedef struct title_layout {
    char text[FLAIR_WINDOW_TITLE_MAX];
    int x;
    int width;
    int gap_left;
    int gap_right;
} title_layout_t;

static void chicago_ink_bounds(const char *text, int width,
                               int *ink_left, int *ink_right)
{
    int lo = width;
    int hi = -1;
    int pen = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        int ch = (int)(unsigned char)text[i];
        int aw = chicago_advance(ch);
        for (int row = 0; row < CHICAGO_CELL_H; row++) {
            unsigned int bits = chicago_cell_bits(ch, row);
            for (int col = 0; col < aw; col++) {
                if ((bits & (0x8000u >> col)) != 0u) {
                    int x = pen + col;
                    if (x < lo) lo = x;
                    if (x > hi) hi = x;
                }
            }
        }
        pen += aw;
    }
    if (hi < lo) {
        lo = 0;
        hi = width - 1;
    }
    *ink_left = lo;
    *ink_right = hi + 1;
}

/* Fit a Chicago title into the measured stripe run while keeping it centered
 * on the WHOLE title bar (window-chrome.md Sec 2.3: "centred in the bar").
 * The widget-side limits come from the measured stripe extents between the
 * close and zoom clusters (Sec 2.2 / Sec 3.1).  Overlong titles are truncated
 * at a whole-character boundary and END IN THE ELLIPSIS, the single Chicago
 * glyph 0xC9 (FLAIR_CHROME_TITLE_TRUNC_MARKER; NFNT 5478 lb 2 aw 14), the way
 * TruncString(smTruncEnd) ends a System 7 / Mac OS 8 title (bead
 * initech-tdnl.68; it was one period, the R0.2 simplification, before the
 * strike had the glyph). Widths are the proportional Chicago 12 advances
 * (initech-tdnl.33). No font condensation. */
static int title_layout_make(title_layout_t *out, const char *title,
                             int left, int right,
                             int safe_left, int safe_right)
{
    int n = 0;
    int shown;
    int source_truncated;

    if (out == 0 || title == 0 || title[0] == '\0') {
        return 0;
    }
    while (title[n] != '\0' && n < (int)FLAIR_WINDOW_TITLE_MAX - 1) {
        n++;
    }
    source_truncated = title[n] != '\0';
    shown = n;
    while (shown > 0) {
        int ink_left;
        int ink_right;
        for (int i = 0; i < shown; i++) {
            out->text[i] = title[i];
        }
        if (source_truncated || shown < n) {
#if defined(CHROME_MUT_TRUNC_PERIOD)
            /* Rule-6 restoration of the retired R0.2 marker (test-flair-fonts
             * W3 must go RED): one period instead of the 0xC9 ellipsis. */
            out->text[shown - 1] = '.';
#else
            out->text[shown - 1] = FLAIR_CHROME_TITLE_TRUNC_MARKER;
#endif
        }
        out->text[shown] = '\0';
        int width = text_measure(FONT_CHICAGO, out->text);
        int x = left + ((right - left) - width) / 2;
        chicago_ink_bounds(out->text, width, &ink_left, &ink_right);
        int gap_left = x + ink_left - FLAIR_CHROME_TITLE_GAP_PAD_LEFT;
        int gap_right = x + ink_right + FLAIR_CHROME_TITLE_GAP_PAD_RIGHT;
        if (gap_left >= safe_left && gap_right <= safe_right) {
            out->x = x;
            out->width = width;
            out->gap_left = gap_left;
            out->gap_right = gap_right;
            break;
        }
        shown--;
    }
    if (shown == 0) {
        return 0;
    }
#if defined(CHROME_FID_MUT_CENTER_OFF)
    /* Rule-6 R0.2 mutant: displace the entire centered title treatment +7px. */
    out->x += 7;
    out->gap_left += 7;
    out->gap_right += 7;
#endif
    return 1;
}

/* Shared Platinum title-band renderer. The exact sampled row profile is:
 * frame, highlight, two face, twelve alternating stripes, four face, shadow,
 * frame. Ref: window-chrome.md Sec 2.1-2.3. */
static int draw_titlebar_band(GrafPort *port, const flair_skin_t *skin,
                              int left, int top, int right,
                              const char *title, int hilited,
                              int title_safe_left, int title_safe_right)
{
    int active = hilited;
    int shared_line = top + FLAIR_CHROME_TITLEBAR_H - 1;
    int w = right - left;

#if defined(CHROME_FID_MUT_NO_INACTIVE)
    active = 1;
#endif
#if defined(CHROME_MUTATE_TITLEBAR_H)
    shared_line++;
#endif

    if (active) {
#if defined(CHROME_FID_MUT_NO_BEVEL)
        for (int y = top + 1; y < shared_line; y++) {
            int part = ((y - top) & 1) ? FLAIR_PART_CONTENT
                                       : FLAIR_PART_PLAT_STRIPE_DARK;
            cfill(port, left + 1, y, w - 2, part);
        }
#else
        cfill(port, left + 1, top + 1, w - 2, FLAIR_PART_CONTENT);
        crect(port, left + 1, top + 2, right - 1, top + 4,
              FLAIR_PART_PLAT_FRAME_FACE);
        crect(port, left + 1, top + FLAIR_CHROME_TITLE_STRIPE_TOP_OFF,
              right - 1,
              top + FLAIR_CHROME_TITLE_STRIPE_TOP_OFF +
                    FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS,
              FLAIR_PART_PLAT_FRAME_FACE);
        for (int row = 0; row < FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS; row++) {
#if defined(CHROME_FID_MUT_PHASE)
            int light = (row & 1) != 0;
#else
            int light = (row & 1) == 0;
#endif
            int shift = row & 1;
#if defined(CHROME_FID_MUT_STRIPE_UNSHIFTED)
            shift = 0;
#endif
            cfill(port, title_safe_left + shift,
                  top + FLAIR_CHROME_TITLE_STRIPE_TOP_OFF + row,
                  title_safe_right - title_safe_left,
                  light ? FLAIR_PART_CONTENT : FLAIR_PART_PLAT_STRIPE_DARK);
        }
        crect(port, left + 1, top + 16, right - 1, top + 20,
              FLAIR_PART_PLAT_FRAME_FACE);
        cfill(port, left + 1, top + 20, w - 2,
              FLAIR_PART_PLAT_FRAME_SHADOW);
#if defined(CHROME_MUTATE_TITLEBAR_H)
        cfill(port, left + 1, top + 21, w - 2,
              FLAIR_PART_PLAT_FRAME_FACE);
#endif
#endif
    } else {
        crect(port, left + 1, top + 1, right - 1, shared_line,
              FLAIR_PART_PLAT_FACE);
    }

#if defined(CHROME_FID_MUT_INACTIVE_BLACK_FRAME)
    cfill(port, left + 1, shared_line, w - 2, FLAIR_PART_FRAME);
#else
    cfill(port, left + 1, shared_line, w - 2,
          active ? FLAIR_PART_FRAME : FLAIR_PART_PLAT_INACTIVE_FRAME);
#endif

    {
        title_layout_t layout;
        if (title_layout_make(&layout, title, left, right,
                              title_safe_left, title_safe_right)) {
            int ty = top + FLAIR_CHROME_TITLE_TEXT_TOP_OFF;
            int ink_part;

            if (active) {
                int gap_w = layout.gap_right - layout.gap_left;
                for (int row = 0;
                     row < FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS; row++) {
                    int shift = (row & 1)
                                    ? FLAIR_CHROME_TITLE_DARK_GAP_SHIFT : 0;
                    cfill(port, layout.gap_left + shift,
                          top + FLAIR_CHROME_TITLE_STRIPE_TOP_OFF + row,
                          gap_w, FLAIR_PART_PLAT_FRAME_FACE);
                }
            }

#if defined(CHROME_FID_MUT_INACTIVE_BRIGHT_TITLE)
            ink_part = FLAIR_PART_TEXT;
#else
            ink_part = active ? FLAIR_PART_TEXT
                              : FLAIR_PART_PLAT_INACTIVE_TEXT;
#endif
#if defined(CHROME_MUT_TITLE_UNCLIPPED)
            /* MUTANT CHROME_MUT_TITLE_UNCLIPPED (Rule 6; test-flair-routing-
             * mutant leg T): the pre-tdnl.14 title run -- text_draw has no
             * region clip, so a partly covered window's repaint stamps its
             * whole Chicago cell run over the window in front. NEVER in a real
             * build. */
            text_draw(&port->portBits.bm, layout.x, ty, layout.text,
                      FONT_CHICAGO,
                      flair_look_pixel_for_skin(port, skin, ink_part),
                      flair_look_pixel_for_skin(
                          port, skin,
                          active ? FLAIR_PART_PLAT_FRAME_FACE
                                 : FLAIR_PART_PLAT_FACE));
#elif !defined(CHROME_FID_MUT_TITLE_BLANK) && !defined(CHROME_FID_MUT_NO_TITLE)
            chrome_ctext(port, layout.x, ty, layout.text,
                         flair_look_pixel_for_skin(port, skin, ink_part),
                         flair_look_pixel_for_skin(
                             port, skin,
                             active ? FLAIR_PART_PLAT_FRAME_FACE
                                    : FLAIR_PART_PLAT_FACE));
#else
            (void)ty;
            (void)ink_part;
#endif
        }
    }
    return shared_line;
}

/* Axis-generic rect (size round 6, WL-0162): along [a0,a1) x cross [c0,c1).
 * V: along = y, cross = x; H: along = x, cross = y. One colour per call, so
 * the decomposition into spans never changes a pixel. */
static void sb_rect(GrafPort *port, const flair_skin_t *skin, int axis,
                    int a0, int a1, int c0, int c1, int part)
{
    if (axis == WSCROLL_V) crect(port, c0, a0, c1, a1, part);
    else                   crect(port, a0, c0, a1, c1, part);
}

#if !defined(CHROME_FID_MUT_SCROLL_FLAT)
/* The 4-step arrow glyph: up (V) / left (H) when `grow`, else down / right;
 * step i spans 2+2i (or 8-2i) cross pixels centred on `cc`. */
static void sb_tri(GrafPort *port, const flair_skin_t *skin, int axis,
                   int a, int cc, int grow, int part)
{
    for (int i = 0; i < 4; i++) {
        int width = grow ? 2 + 2 * i : 8 - 2 * i;
        sb_rect(port, skin, axis, a + i, a + i + 1,
                cc - width / 2, cc + width / 2, part);
    }
}

/* Raised 14x14 Platinum arrow tile. Off-diagonal corners retain the face.
 * Ref: ../system7-decomp/specs/sys8/scrollbars.md Sec 2.2 (SAMPLED). */
static void draw_scroll_tile(GrafPort *port, const flair_skin_t *skin,
                             int x0, int y0, int x1, int y1)
{
    /* Rect table (size round 6, WL-0162), box (x0,y0)-(x1,y1): face, white
     * top row + left column, CD bottom row + right column, the two
     * off-diagonal corners back to the face. */
#define TX(a, b, c, d, f, p) { (int8_t)(a), (int8_t)(b), (int8_t)(c), (int8_t)(d), (uint8_t)(f), (uint8_t)(p) }
    static const chrome_rect_t tile[] = {
        TX(0, 0, 0, 0, CR_X1R | CR_Y1B, FLAIR_PART_PLAT_FACE),
#if !defined(SB_MUT_TILE_FLAT)
        TX(0, 0, -1, 1, CR_X1R, FLAIR_PART_CONTENT),
        TX(0, 0, 1, 0, CR_Y1B, FLAIR_PART_CONTENT),
        TX(1, -1, 0, 0, CR_Y0B | CR_X1R | CR_Y1B, FLAIR_PART_PLAT_TILE_SHADOW),
        TX(-1, 0, 0, 0, CR_X0R | CR_X1R | CR_Y1B, FLAIR_PART_PLAT_TILE_SHADOW),
#if !defined(SB_LOOK_MUT_CORNERS_BEVELLED)
        TX(-1, 0, 0, 1, CR_X0R | CR_X1R, FLAIR_PART_PLAT_FACE),
        TX(0, -1, 1, 0, CR_Y0B | CR_Y1B, FLAIR_PART_PLAT_FACE),
#endif
#endif
    };
    chrome_rects(port, skin, x0, y0, x1, y1, tile, CRN(tile), 0, 0);
}

/* Enabled page well: 96/A5/C0x10/CD/DA across the bar, plus a two-pixel
 * 96/A5 inset at the near along-axis end.  The far end is deliberately left
 * unstroked. Ref: scrollbars.md Sec 2.3 (SAMPLED). */
static void sb_well(GrafPort *port, const flair_skin_t *skin, int axis,
                    int a0, int a1, int c0, int c1)
{
    if (c1 <= c0 || a1 <= a0) {
        return;
    }
#if defined(SB_MUT_FLAT_WELL)
    sb_rect(port, skin, axis, a0, a1, c0, c1, FLAIR_PART_PLAT_WELL);
#else
    sb_rect(port, skin, axis, a0, a1, c0, c0 + 1, FLAIR_PART_PLAT_STRIPE_DARK);
    sb_rect(port, skin, axis, a0, a1, c0 + 1, c0 + 2, FLAIR_PART_PLAT_WIDGET_EDGE);
    sb_rect(port, skin, axis, a0, a1, c0 + 2,
            c0 + 2 + FLAIR_CHROME_SCROLL_WELL_FILL_ROWS, FLAIR_PART_PLAT_WELL);
    sb_rect(port, skin, axis, a0, a1, c1 - 2, c1 - 1, FLAIR_PART_PLAT_TILE_SHADOW);
    sb_rect(port, skin, axis, a0, a1, c1 - 1, c1, FLAIR_PART_PLAT_FRAME_FACE);
    sb_rect(port, skin, axis, a0, a0 + 1, c0, c1, FLAIR_PART_PLAT_STRIPE_DARK);
    if (a0 + 1 < a1) {
        sb_rect(port, skin, axis, a0 + 1, a0 + 2, c0, c1,
                FLAIR_PART_PLAT_WIDGET_EDGE);
    }
#endif
}
#endif

/* ---------------------------------------------------------------------------
 * The window gutters' THREE states (bead initech-tdnl.35; scrollbars.md Sec 5):
 *   HOLLOW   inactive window: gray outer lines, flat F3 interior, nothing else
 *            (Sec 4).
 *   DISABLED active window whose content FITS (no WindowScroll record, or its
 *            max == 0): black outer lines, flat F3 interior, #777777 arrow-box
 *            separators, #A5A5A5 arrows, NO thumb (Sec 3 -- "Both bars of the
 *            'Initech81' window ... are disabled: the folder contents fit the
 *            view").
 *   ENABLED  active window with something to scroll: raised arrow tiles,
 *            recessed wells, black separators and the 15-px accent thumb at
 *            the value's position (Sec 2; position math winscroll.h).
 * A held arrow draws PRESSED: scrollbars.md Sec 6 has no golden for it, so it
 * is AUTHORED after the System 7 idle->pressed delta (close-zoom-box.md
 * "PRESSED state": the face goes to the darker tone, the box does not move):
 * the tile's bevel inverts (dark top/left, light bottom/right) and its face
 * takes the widget-edge gray; the glyph stays black. [golden-resolves: a
 * mouse-down capture of a Platinum scroll arrow.]
 * ------------------------------------------------------------------------- */
typedef struct sb_draw {
    int enabled;   /* 1 == ENABLED (Sec 2), 0 == DISABLED (Sec 3)          */
    int thumb;     /* thumb leading edge along the axis (ENABLED only)      */
    int hilite;    /* WSCROLL_PART_UP / _DOWN when that arrow is held       */
} sb_draw_t;

static void sb_state_of(sb_draw_t *st, rgn_rect_t frame, int axis,
                        const WindowScroll *ws)
{
    const WindowScrollAxis *a = (ws != (const WindowScroll *)0)
                                    ? &ws->axis[axis] : (const WindowScrollAxis *)0;
    rgn_rect_t bar = wscroll_bar_rect(frame, axis);
#if defined(CHROME_MUT_SCROLL_ALWAYS_ENABLED)
    /* MUTANT (Rule 6; test-chrome-fidelity SCROLL-DISABLED leg): the pre-
     * tdnl.35 composition -- every active gutter draws the ENABLED anatomy
     * even when nothing can scroll. NEVER in a real build. */
    st->enabled = 1;
#else
    st->enabled = wscroll_enabled(a);
#endif
    st->thumb = (a != (const WindowScrollAxis *)0 && a->drag_pos >= 0)
                    ? a->drag_pos : wscroll_thumb_pos(bar, axis, a);
    st->hilite = (a != (const WindowScrollAxis *)0) ? a->hilite : 0;
}

#if !defined(CHROME_FID_MUT_SCROLL_FLAT)
/* Axis-generic plot: along/cross -> (x,y). V: along = y, cross = x. */
static void sb_plot(GrafPort *port, const flair_skin_t *skin, int axis,
                    int along, int cross, int part)
{
    if (axis == WSCROLL_V) cfill(port, cross, along, 1, part);
    else                   cfill(port, along, cross, 1, part);
}

/* The 15-px accent thumb, `a0` = leading edge along the axis, `c0` = first
 * interior cross pixel (14 across). An exact transcription of the sampled
 * 15x14 matrix (scrollbars.md Sec 2.4, s8_doc_window_inactive x=329..345
 * y=400..415), axis-generic: control.c draw_ctrl_scroll_thumb is the same
 * matrix transposed for its vertical control. Parts resolve through the C-8
 * seam (FLAIR_PART_SB_THUMB_*), never a color literal. */
static void draw_sb_thumb(GrafPort *port, const flair_skin_t *skin, int axis,
                          int a0, int c0)
{
    const int len = FLAIR_CHROME_SCROLL_THUMB_MIN;   /* 15 along  */
    const int wid = FLAIR_CHROME_SCROLL_INTERIOR;    /* 14 across */
    for (int a = 0; a < len; a++) {
        for (int c = 0; c < wid; c++) {
            int part = FLAIR_PART_SB_THUMB_FACE;
            if (c == wid - 1 || a == len - 1)
                part = (c == 0 && a == len - 1) ? FLAIR_PART_SB_THUMB_FACE
                                                : FLAIR_PART_SB_THUMB_SHADOW;
            if (c == 0 && a < len - 1) part = FLAIR_PART_SB_THUMB_HL;
            if (a == 0 && c < wid - 1) part = FLAIR_PART_SB_THUMB_HL;
            if (a == 0 && c == 0) part = FLAIR_PART_PLAT_TROUGH;
            if (a == len - 1 && c == 0) part = FLAIR_PART_SB_THUMB_FACE;
            if (a == 0 && c == wid - 1) part = FLAIR_PART_SB_THUMB_FACE;
#if defined(SB_LOOK_MUT_RAMP_FLAT)
            if (part != FLAIR_PART_PLAT_TROUGH) part = FLAIR_PART_SB_THUMB_FACE;
#endif
            sb_plot(port, skin, axis, a0 + a, c0 + c, part);
        }
    }
    /* Four grip lines at along +4/+6/+8/+10, cross rows 4..10, each with a
     * highlight companion one along-pixel earlier (cross rows 4..9) capped by
     * an F3 pixel at cross row 3. */
    for (int g = 0; g < 4; g++) {
        int ac = a0 + 3 + 2 * g;
        sb_plot(port, skin, axis, ac, c0 + 3, FLAIR_PART_PLAT_TROUGH);
        for (int c = 4; c <= 9; c++)
            sb_plot(port, skin, axis, ac, c0 + c, FLAIR_PART_SB_THUMB_HL);
#if !defined(SB_LOOK_MUT_GRIPS_MISSING)
        for (int c = 4; c <= 10; c++)
            sb_plot(port, skin, axis, ac + 1, c0 + c, FLAIR_PART_SB_THUMB_GRIP);
#endif
    }
}

/* The authored PRESSED arrow tile (banner above). */
static void draw_scroll_tile_pressed(GrafPort *port, const flair_skin_t *skin,
                                     int x0, int y0, int x1, int y1)
{
    static const chrome_rect_t held[] = {
        TX(0, 0, 0, 0, CR_X1R | CR_Y1B, FLAIR_PART_PLAT_WIDGET_EDGE),
        TX(0, 0, -1, 1, CR_X1R, FLAIR_PART_PLAT_STRIPE_DARK),
        TX(0, 0, 1, 0, CR_Y1B, FLAIR_PART_PLAT_STRIPE_DARK),
        TX(1, -1, 0, 0, CR_Y0B | CR_X1R | CR_Y1B, FLAIR_PART_PLAT_WELL),
        TX(-1, 1, 0, 0, CR_X0R | CR_X1R | CR_Y1B, FLAIR_PART_PLAT_WELL)
    };
    chrome_rects(port, skin, x0, y0, x1, y1, held, CRN(held), 0, 0);
}

static void draw_arrow_tile(GrafPort *port, const flair_skin_t *skin,
                            int x0, int y0, int x1, int y1, int pressed)
{
#if defined(CHROME_MUT_SCROLL_PRESSED_IDLE)
    /* MUTANT (Rule 6; test-chrome-fidelity SCROLL-PRESSED leg): a held arrow
     * looks idle. NEVER in a real build. */
    pressed = 0;
#endif
    if (pressed) draw_scroll_tile_pressed(port, skin, x0, y0, x1, y1);
    else         draw_scroll_tile(port, skin, x0, y0, x1, y1);
}
#endif

/* One gutter bar, axis-generic (size round 6, WL-0162): along [a0,a1),
 * cross [c0,c1] INCLUSIVE (the V bar's right line / the H bar's bottom line).
 * V: a = top..bottom, c = left..right; H: a = left..right, c = top..bottom.
 * The arrow tiles keep their x/y bevel (light top+left) on both axes. */
static void draw_scrollbar(GrafPort *port, const flair_skin_t *skin, int axis,
                           int a0, int a1, int c0, int c1,
                           int active, const sb_draw_t *st)
{
#if defined(CHROME_FID_MUT_SCROLL_FLAT)
    (void)active; (void)st;
    sb_rect(port, skin, axis, a0, a1, c0, c1 + 1, FLAIR_PART_BTNFACE);
    if (axis == WSCROLL_V) cframe(port, c0, a0, c1 + 1, a1, FLAIR_PART_FRAME);
    else                   cframe(port, a0, c0, a1, c1 + 1, FLAIR_PART_FRAME);
    return;
#else
    int frame_part = active ? FLAIR_PART_FRAME
                            : FLAIR_PART_PLAT_INACTIVE_FRAME;
    /* All interior rows, including the old well's final row when an
     * enabled bar becomes disabled (tdnl.82). */
    sb_rect(port, skin, axis, a0 + 1, a1 - 1, c0 + 1, c1, FLAIR_PART_PLAT_TROUGH);
    sb_rect(port, skin, axis, a0, a1, c0, c0 + 1, frame_part);
    sb_rect(port, skin, axis, a0, a1, c1, c1 + 1, frame_part);
    sb_rect(port, skin, axis, a0, a0 + 1, c0, c1 + 1, frame_part);
    sb_rect(port, skin, axis, a1 - 1, a1, c0, c1 + 1, frame_part);

    if (active && a1 - a0 >= 2 * FLAIR_CHROME_SCROLL_ARROW_TILE) {
        int s0 = a0 + FLAIR_CHROME_SCROLL_ARROW_TILE - 1;
        int s1 = a1 - FLAIR_CHROME_SCROLL_ARROW_TILE;
        int cc = c0 + FLAIR_CHROME_SCROLLBAR_W / 2;
        if (!st->enabled) {
            /* DISABLED (Sec 3): partition kept by #777777 separators, the
             * arrows #A5A5A5, every bevel and the well dropped, no thumb
             * (h-bar separators x=85/445 in the golden). */
            sb_rect(port, skin, axis, s0, s0 + 1, c0 + 1, c1,
                    FLAIR_PART_PLAT_INACTIVE_FRAME);
            sb_rect(port, skin, axis, s1, s1 + 1, c0 + 1, c1,
                    FLAIR_PART_PLAT_INACTIVE_FRAME);
            sb_tri(port, skin, axis, a0 + 6, cc, 1, FLAIR_PART_PLAT_WIDGET_EDGE);
            sb_tri(port, skin, axis, a1 - 10, cc, 0, FLAIR_PART_PLAT_WIDGET_EDGE);
            return;
        }
        int sep_part = FLAIR_PART_FRAME;
#if defined(SB_MUT_SEP_GRAY)
        sep_part = FLAIR_PART_PLAT_INACTIVE_FRAME;
#endif
        int t = st->thumb;
        sb_well(port, skin, axis, s0 + 1, t - 1, c0 + 1, c1);
        sb_well(port, skin, axis, t + FLAIR_CHROME_SCROLL_THUMB_MIN + 1, s1,
                c0 + 1, c1);
        draw_sb_thumb(port, skin, axis, t, c0 + 1);
        if (axis == WSCROLL_V) {
            draw_arrow_tile(port, skin, c0 + 1, a0 + 1, c1, s0,
                            st->hilite == WSCROLL_PART_UP);
            draw_arrow_tile(port, skin, c0 + 1, s1 + 1, c1, a1 - 1,
                            st->hilite == WSCROLL_PART_DOWN);
        } else {
            draw_arrow_tile(port, skin, a0 + 1, c0 + 1, s0, c1,
                            st->hilite == WSCROLL_PART_UP);
            draw_arrow_tile(port, skin, s1 + 1, c0 + 1, a1 - 1, c1,
                            st->hilite == WSCROLL_PART_DOWN);
        }
        sb_rect(port, skin, axis, s0, s0 + 1, c0 + 1, c1, sep_part);
        sb_rect(port, skin, axis, s1, s1 + 1, c0 + 1, c1, sep_part);
        sb_rect(port, skin, axis, t - 1, t, c0 + 1, c1, sep_part);
        sb_rect(port, skin, axis, t + FLAIR_CHROME_SCROLL_THUMB_MIN,
                t + FLAIR_CHROME_SCROLL_THUMB_MIN + 1, c0 + 1, c1, sep_part);
        sb_tri(port, skin, axis, a0 + 6, cc, 1, FLAIR_PART_FRAME);
        sb_tri(port, skin, axis, a1 - 10, cc, 0, FLAIR_PART_FRAME);
    }
#endif
}
#define draw_vertical_scrollbar(p, k, l, t, r, b, act, st) \
    draw_scrollbar((p), (k), WSCROLL_V, (t), (b), (l), (r), (act), (st))
#define draw_horizontal_scrollbar(p, k, l, t, r, b, act, st) \
    draw_scrollbar((p), (k), WSCROLL_H, (l), (r), (t), (b), (act), (st))

/* Active 18x18 grow cell and exact three-line grip transcription.
 * Ref: window-chrome.md Sec 5. Inactive is the flat Sec 6 face. */
static void draw_grow_box(GrafPort *port, const flair_skin_t *skin,
                          int gx, int gy, int active)
{
    const int cell = FLAIR_CHROME_GROW;
    int trail_part = FLAIR_PART_PLAT_STRIPE_DARK;

    /* h6y1 Rule-6 perturbations. Default art remains the independent
     * Mac OS 8.1 cell (sys8/window-chrome.md Sec 5/6), not System-7 art. */
#if defined(CHROME_MUT_GROW_OMIT)
    return;
#endif
#if defined(CHROME_MUT_GROW_INACTIVE_ACTIVE)
    active = 1;
#endif
#if defined(CHROME_MUT_GROW_FIGURE_SWAP)
    trail_part = FLAIR_PART_PLAT_FACE;
#endif

    crect(port, gx, gy, gx + cell, gy + cell,
          active ? FLAIR_PART_PLAT_FRAME_FACE : FLAIR_PART_PLAT_FACE);
    if (!active) {
        return;
    }

    cfill(port, gx, gy, cell - 2, FLAIR_PART_CONTENT);
    for (int y = gy; y < gy + cell - 2; y++) {
        cfill(port, gx, y, 1, FLAIR_PART_CONTENT);
    }

    for (int line = 0; line < FLAIR_CHROME_GROW_GRIP_LINES; line++) {
        int start_row = 3 + 2 * line;
        int start_x = 8 + 2 * line;
        cfill(port, gx + start_x, gy + start_row, 2, FLAIR_PART_CONTENT);
        for (int step = 1; step <= 5; step++) {
            cfill(port, gx + start_x - step, gy + start_row + step,
                  1, FLAIR_PART_CONTENT);
            cfill(port, gx + start_x - step + 2, gy + start_row + step,
                  1, trail_part);
        }
        cfill(port, gx + start_x - 5, gy + start_row + 6,
              1, FLAIR_PART_PLAT_WELL);
        cfill(port, gx + start_x - 4, gy + start_row + 6,
              1, trail_part);
    }
}

/* Four-pixel raised rail between outer and inner lines, plus content inset.
 * Ref: window-chrome.md Sec 4. */
static void draw_body_structure(GrafPort *port, const flair_skin_t *skin,
                                int left, int shared_line, int right, int bottom,
                                int active, int frame_part)
{
#if defined(CHROME_FID_MUT_BODYBAR)
    (void)port;
    (void)skin;
    (void)left;
    (void)shared_line;
    (void)right;
    (void)bottom;
    (void)active;
    (void)frame_part;
#else
    /* Box: l = left, t = shared_line, r = right, b = bottom - 1. */
#define BX(x0, y0, x1, y1, f, p) { (int8_t)(x0), (int8_t)(y0), (int8_t)(x1), (int8_t)(y1), (uint8_t)(f), (uint8_t)(p) }
    static const chrome_rect_t body[] = {
        /* the rails, rows t+1 .. b-1 */
        BX(1, 1, 2, 0, CR_ACT | CR_Y1B, FLAIR_PART_CONTENT),
        BX(2, 1, 4, 0, CR_ACT | CR_Y1B, FLAIR_PART_PLAT_FRAME_FACE),
        BX(4, 1, 5, 0, CR_ACT | CR_Y1B, FLAIR_PART_PLAT_FRAME_SHADOW),
        BX(1, 1, 1 + FLAIR_CHROME_BODY_BAR, 0, CR_INA | CR_Y1B, FLAIR_PART_PLAT_FACE),
        BX(-5, 1, -4, 0, CR_ACT | CR_X0R | CR_X1R | CR_Y1B, FLAIR_PART_CONTENT),
        BX(-4, 1, -2, 0, CR_ACT | CR_X0R | CR_X1R | CR_Y1B, FLAIR_PART_PLAT_FRAME_FACE),
        BX(-2, 1, -1, 0, CR_ACT | CR_X0R | CR_X1R | CR_Y1B, FLAIR_PART_PLAT_FRAME_SHADOW),
        BX(-5, 1, -5 + FLAIR_CHROME_BODY_BAR, 0, CR_INA | CR_X0R | CR_X1R | CR_Y1B, FLAIR_PART_PLAT_FACE),
        /* the bottom band, rows b-4 .. b-1 */
        BX(1, -4, -1, -3, CR_ACT | CR_X1R | CR_Y0B | CR_Y1B, FLAIR_PART_CONTENT),
        BX(1, -3, -1, -1, CR_ACT | CR_X1R | CR_Y0B | CR_Y1B, FLAIR_PART_PLAT_FRAME_FACE),
        BX(1, -1, -1, 0, CR_ACT | CR_X1R | CR_Y0B | CR_Y1B, FLAIR_PART_PLAT_FRAME_SHADOW),
        BX(1, -4, -1, 0, CR_INA | CR_X1R | CR_Y0B | CR_Y1B, FLAIR_PART_PLAT_FACE),
        /* the inner frame lines and the content inset */
        BX(5, 0, 6, -3, CR_Y1B, CR_DYN),
        BX(-6, 0, -5, -3, CR_X0R | CR_X1R | CR_Y1B, CR_DYN),
        BX(5, -5, -5, -4, CR_X1R | CR_Y0B | CR_Y1B, CR_DYN),
        BX(6, 1, -6, 2, CR_X1R, FLAIR_PART_CONTENT),
        BX(6, 1, 7, -5, CR_Y1B, FLAIR_PART_CONTENT),
        BX(-7, 1, -6, -5, CR_X0R | CR_X1R | CR_Y1B, FLAIR_PART_PLAT_WELL),
        BX(6, -6, -6, -5, CR_X1R | CR_Y0B | CR_Y1B, FLAIR_PART_PLAT_WELL)
    };
    chrome_rects(port, skin, left, shared_line, right, bottom - 1,
                 body, CRN(body), active, frame_part);
#endif
}

void flair_draw_window_scrollbar(GrafPort *port, const flair_skin_t *skin,
                                 rgn_rect_t frame, int axis,
                                 const WindowScroll *ws)
{
    sb_draw_t st;
    int ri = frame.right - 1;
    int bi = frame.bottom - 1;
    int grow_x = ri - FLAIR_CHROME_GROW_RIGHT_OFF;
    int grow_y = bi - FLAIR_CHROME_GROW_BOTTOM_OFF;
    if (port == 0 || skin == (const flair_skin_t *)0) return;
    sb_state_of(&st, frame, axis, ws);
    /* The SAME arguments flair_draw_document_window_scroll passes. */
    if (axis == WSCROLL_V)
        draw_vertical_scrollbar(port, skin, grow_x - FLAIR_CHROME_FRAME,
                                frame.top + FLAIR_CHROME_TITLEBAR_H - 1,
                                ri - 5, grow_y, 1, &st);
    else
        draw_horizontal_scrollbar(port, skin, frame.left + 5,
                                  grow_y - FLAIR_CHROME_FRAME,
                                  grow_x, bi - 5, 1, &st);
}

void flair_draw_window_widget(GrafPort *port, const flair_skin_t *skin,
                              rgn_rect_t frame, int part, int pressed)
{
#if !defined(CHROME_FID_MUT_BOX_GEOM)
    int by = frame.top + FLAIR_CHROME_WIDGET_TOP_OFF;
    int ri = frame.right - 1;
    if (port == 0 || skin == (const flair_skin_t *)0) return;
    if (part == (int)inGoAway)
        draw_platinum_widget(port, skin, frame.left + FLAIR_CHROME_CLOSE_LEFT_OFF,
                             by, PLAT_WIDGET_CLOSE, pressed);
    else if (part == (int)inZoomIn || part == (int)inZoomOut)
        draw_platinum_widget(port, skin, ri - FLAIR_CHROME_ZOOM_RIGHT_OFF,
                             by, PLAT_WIDGET_ZOOM, pressed);
    else if (part == (int)inCollapse)
        draw_platinum_widget(port, skin, ri - FLAIR_CHROME_COLLAPSE_RIGHT_OFF,
                             by, PLAT_WIDGET_COLLAPSE, pressed);
#else
    (void)port; (void)skin; (void)frame; (void)part; (void)pressed;
#endif
}

void flair_draw_document_window(GrafPort *port, const flair_skin_t *skin,
                                rgn_rect_t frame,
                                const char *title, int hilited,
                                uint8_t widget_flags)
{
    flair_draw_document_window_scroll(port, skin, frame, title, hilited,
                                      widget_flags, (const WindowScroll *)0);
}

void flair_draw_document_window_scroll(GrafPort *port,
                                       const flair_skin_t *skin,
                                       rgn_rect_t frame,
                                       const char *title, int hilited,
                                       uint8_t widget_flags,
                                       const WindowScroll *ws)
{
    sb_draw_t sbv, sbh;
    int left;
    int top;
    int right;
    int bottom;
    int w;
    int h;
    int active;
    int shared_line;
    int frame_part;
    int content_left;
    int content_top;
    int content_right;
    int content_bottom;
    int ri;
    int bi;
    int grow_x;
    int grow_y;
    int sb_left;
    int collapsed;

    if (port == 0 || skin == (const flair_skin_t *)0) {
        return;
    }
    left = frame.left;
    top = frame.top;
    right = frame.right;
    bottom = frame.bottom;
    w = right - left;
    h = bottom - top;
    collapsed = h == FLAIR_CHROME_TITLEBAR_H;
    if (w < 2 * FLAIR_CHROME_GROW + 2 ||
        (!collapsed &&
         h < FLAIR_CHROME_TITLEBAR_H + FLAIR_CHROME_GROW + 2)) {
        return;
    }

    active = hilited;
#if defined(CHROME_FID_MUT_NO_INACTIVE)
    active = 1;
#endif
    shared_line = draw_titlebar_band(
        port, skin, left, top, right, title, active,
        left + FLAIR_CHROME_TITLE_RUN_LEFT_OFF,
        right - FLAIR_CHROME_TITLE_RUN_RIGHT_OFF);
    frame_part = active ? FLAIR_PART_FRAME
                        : FLAIR_PART_PLAT_INACTIVE_FRAME;
#if defined(CHROME_FID_MUT_INACTIVE_BLACK_FRAME)
    if (!active) {
        frame_part = FLAIR_PART_FRAME;
    }
#endif

    if (!collapsed) {
#if defined(CHROME_FID_MUT_BODYBAR)
        content_left = left + 1;
        content_right = right - 1;
        content_bottom = bottom - 1;
#else
        content_left = left + 6;
        content_right = right - 6;
        content_bottom = bottom - 6;
#endif
        content_top = shared_line + 1;
        crect(port, content_left, content_top, content_right, content_bottom,
              FLAIR_PART_CONTENT);

        draw_body_structure(port, skin, left, shared_line, right, bottom,
                            active, frame_part);

#if defined(FLAIR_COLORBLIND_MUTANT)
    /* Named behavioral mutant: bypass the policy seam with a computed value.
     * Default builds do not compile this oracle-only perturbation. */
    {
        uint32_t orange = ((uint32_t)0xFFu << 16) |
                          ((uint32_t)0x88u << 8);
        surface_fill_span(&port->portBits.bm,
                          (uint32_t)content_left, (uint32_t)content_top,
                          (uint32_t)(content_right - content_left), orange);
    }
#endif

        ri = right - 1;
        bi = bottom - 1;
        grow_x = ri - FLAIR_CHROME_GROW_RIGHT_OFF;
        grow_y = bi - FLAIR_CHROME_GROW_BOTTOM_OFF;
        sb_left = grow_x - FLAIR_CHROME_FRAME;
#if defined(CHROME_MUTATE_SCROLLBAR_W)
        sb_left++;
#endif
        {
            int sb_top = shared_line;
#if defined(CHROME_FID_MUT_NOTCH_DOUBLE)
            sb_top++;
#endif
            sb_state_of(&sbv, frame, WSCROLL_V, ws);
            draw_vertical_scrollbar(port, skin, sb_left, sb_top,
                                    ri - 5, grow_y, active, &sbv);
        }
        sb_state_of(&sbh, frame, WSCROLL_H, ws);
        draw_horizontal_scrollbar(port, skin, left + 5,
                                  grow_y - FLAIR_CHROME_FRAME,
                                  grow_x, bi - 5, active, &sbh);
        if ((widget_flags & FLAIR_WINDOW_WIDGET_GROW) != 0u
#if defined(CHROME_MUT_GROW_NO_REGION)
            || 1
#endif
        )
            draw_grow_box(port, skin, grow_x, grow_y, active);
    }

    ri = right - 1;
    bi = bottom - 1;

    if (active
#if defined(CHROME_FID_MUT_KEEP_GADGETS)
        || !active
#endif
       ) {
        int by = top + FLAIR_CHROME_WIDGET_TOP_OFF;
#if defined(CHROME_FID_MUT_BOX_GEOM)
        int old_box = FLAIR_CHROME_SYS7_WBOX_DELTA;
        if ((widget_flags & FLAIR_WINDOW_WIDGET_CLOSE) != 0u)
            cframe(port, left + 4, by, left + 4 + old_box, by + old_box,
                   FLAIR_PART_FRAME);
        if ((widget_flags & FLAIR_WINDOW_WIDGET_ZOOM) != 0u)
            cframe(port, ri - 20, by, ri - 20 + old_box, by + old_box,
                   FLAIR_PART_FRAME);
#else
        if ((widget_flags & FLAIR_WINDOW_WIDGET_CLOSE) != 0u)
            draw_platinum_widget(port, skin,
                                 left + FLAIR_CHROME_CLOSE_LEFT_OFF,
                                 by, PLAT_WIDGET_CLOSE, 0);
        if ((widget_flags & FLAIR_WINDOW_WIDGET_ZOOM) != 0u)
            draw_platinum_widget(port, skin,
                                 ri - FLAIR_CHROME_ZOOM_RIGHT_OFF,
                                 by, PLAT_WIDGET_ZOOM, 0);
#if !defined(CHROME_FID_MUT_COLLAPSE)
        if ((widget_flags & FLAIR_WINDOW_WIDGET_COLLAPSE) != 0u)
            draw_platinum_widget(port, skin,
                                 ri - FLAIR_CHROME_COLLAPSE_RIGHT_OFF,
                                 by, PLAT_WIDGET_COLLAPSE, 0);
#endif
#endif
    }

#if !defined(CHROME_MUTATE_NO_FRAME)
    cfill(port, left, top, w, frame_part);
    cfill(port, left, bi, w, frame_part);
    for (int y = top; y < bottom; y++) {
        cfill(port, left, y, 1, frame_part);
        cfill(port, ri, y, 1, frame_part);
    }
#endif

#if !defined(CHROME_FID_MUT_NO_SHADOW)
    {
#if defined(CHROME_FID_MUT_NOTCH)
        int notch = 1;
#else
        int notch = FLAIR_CHROME_SHADOW_NOTCH;
#endif
        for (int y = top + notch; y <= bottom; y++) {
            cfill(port, right, y, 1, frame_part);
        }
        cfill(port, left + notch, bottom, right - left - notch + 1,
              frame_part);
    }
#endif
}

/* movableDBoxProc shares the Platinum title-band mechanism. Its untitled body
 * remains the caller-owned dialog face; this entry point only draws the band
 * and the one-pixel structure frame. Ref: window-chrome.md Sec 2. */
void flair_draw_movable_dbox_chrome(GrafPort *port, const flair_skin_t *skin,
                                    rgn_rect_t frame,
                                    const char *title)
{
    int left;
    int top;
    int right;
    int bottom;
    int w;
    int h;

    if (port == 0 || skin == (const flair_skin_t *)0) {
        return;
    }
    left = frame.left;
    top = frame.top;
    right = frame.right;
    bottom = frame.bottom;
    w = right - left;
    h = bottom - top;
    if (w < 3 || h < FLAIR_CHROME_TITLEBAR_H + 2) {
        return;
    }

    (void)draw_titlebar_band(
        port, skin, left, top, right, title, 1,
        left + 1, right - 1);
    cframe(port, left, top, right, bottom, FLAIR_PART_FRAME);
}

/* ===========================================================================
 * flair_draw_rdoc_window -- the rDocProc (16) WDEF: the Calculator's rounded
 * window (bead initech-tdnl.17; geometry and its ground truth: rdoc.h).
 * Paints the outline + title band + the body's sides, NEVER the content (the
 * owner paints that through contRgn). Every span is clipped like the rest of
 * the chrome (chrome_cfill). Inks resolve through the C-8 seam: FRAME black,
 * CONTENT white.
 *
 * The outline's rounding is ALSO the window's structure (window.c builds the
 * same rdoc.h insets into strucRgn), so the drawer can never paint outside
 * it; the insets here pick the outline's own pixels.
 *
 * CHROME_MUT_RDOC_NO_CLOSE (Rule 6; test-flair-calc-mutant): the active band
 * loses its close-box ring (a solid black band, as a window without a go-away
 * box). NEVER in a real build.
 * =========================================================================*/
static int rdoc_inset(int y, int h)
{
    if (y < FLAIR_RDOC_CORNER_ROWS) return flair_rdoc_corner[y];
    if (h - 1 - y < FLAIR_RDOC_CORNER_ROWS) return flair_rdoc_corner[h - 1 - y];
    return 0;
}

void flair_draw_rdoc_window(GrafPort *port, const flair_skin_t *skin,
                            rgn_rect_t frame, const char *title, int hilited,
                            uint8_t widget_flags)
{
    int L = frame.left, T = frame.top;
    int W = (int)frame.right - L, H = (int)frame.bottom - T;
    int cl = FLAIR_RDOC_INSET_L, cr = W - FLAIR_RDOC_INSET_R;  /* content [cl,cr) */
    int ch_top = FLAIR_RDOC_TITLE_H, ch_bot = H - FLAIR_RDOC_INSET_B;

    if (port == (GrafPort *)0 || W < 2 * FLAIR_RDOC_TITLE_H || H <= ch_top + FLAIR_RDOC_INSET_B)
        return;

    for (int y = 0; y < H; y++) {
        int in = rdoc_inset(y, H);
        int a = in, b = W - in;                       /* outline [a, b) */
        if (y < ch_top) {
            if (hilited || y == 0 || y == ch_top - 1) {
                chrome_cfill(port, skin, L + a, T + y, b - a, FLAIR_PART_FRAME);
            } else {
                /* inactive band: the rounded 1-px outline, white inside */
                /* the outline is as wide as the step from the row above
                 * plus one (s8_calc_inactive rows 1..5: 3,2,2,1,2 px; then
                 * 1 px down the straight sides) */
                int ia = a + 1, ib = b - 1;
                int p = rdoc_inset(y - 1, H) + 1;
                if (p > ia) ia = p;
                if (W - p < ib) ib = W - p;
                chrome_cfill(port, skin, L + a, T + y, ia - a, FLAIR_PART_FRAME);
                chrome_cfill(port, skin, L + ib, T + y, b - ib, FLAIR_PART_FRAME);
                chrome_cfill(port, skin, L + ia, T + y, ib - ia, FLAIR_PART_CONTENT);
            }
        } else if (y >= ch_bot) {
            chrome_cfill(port, skin, L + a, T + y, b - a, FLAIR_PART_FRAME);
        } else {
            /* the body's sides + the content's inner corners (rdoc.h) */
            int k = y - ch_top, nl = 0, nr = 0, rows = ch_bot - ch_top;
            if (k < 2) { nl = flair_rdoc_notch_top_l[k]; nr = flair_rdoc_notch_top_r[k]; }
            else if (rows - k <= 3) {
                nl = flair_rdoc_notch_bot_l[3 - (rows - k)];
                nr = flair_rdoc_notch_bot_r[3 - (rows - k)];
            }
            chrome_cfill(port, skin, L + a, T + y, cl + nl - a, FLAIR_PART_FRAME);
            chrome_cfill(port, skin, L + cr - nr, T + y, b - (cr - nr), FLAIR_PART_FRAME);
        }
    }

#if !defined(CHROME_MUT_RDOC_NO_CLOSE)
    if (hilited && (widget_flags & FLAIR_WINDOW_WIDGET_CLOSE) != 0u) {
#else
    if (0 && widget_flags) {
#endif
        int x0 = L + FLAIR_RDOC_CLOSE_L, y0 = T + FLAIR_RDOC_CLOSE_T;
        chrome_cframe(port, skin, x0, y0, x0 + FLAIR_RDOC_CLOSE_W,
                      y0 + FLAIR_RDOC_CLOSE_W, FLAIR_PART_CONTENT);
    }

    if (title != (const char *)0 && title[0] != '\0') {
        int tw = 0;
        for (int i = 0; title[i]; i++) tw += chicago_advance((unsigned char)title[i]);
        int area = W - FLAIR_RDOC_TITLE_LEFT;
        int pen = L + FLAIR_RDOC_TITLE_LEFT + (area - tw) / 2;
        uint32_t ink = flair_look_pixel_for_skin(port, skin,
                         hilited ? FLAIR_PART_CONTENT : FLAIR_PART_FRAME);
        uint32_t bg = flair_look_pixel_for_skin(port, skin,
                         hilited ? FLAIR_PART_FRAME : FLAIR_PART_CONTENT);
        chrome_ctext(port, pen, T + FLAIR_RDOC_TITLE_CAP - CHICAGO_CAP_TOP,
                     title, ink, bg);
    }
}
