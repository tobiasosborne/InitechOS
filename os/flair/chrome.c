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

enum {
    PLAT_WIDGET_CLOSE = 0,
    PLAT_WIDGET_ZOOM,
    PLAT_WIDGET_COLLAPSE
};

/* Platinum 12x12 widget plus the one-pixel right/bottom highlight.
 * Ref: window-chrome.md Sec 3.1-3.3. The seven ramp roles are the sampled
 * diagonal sequence documented at Sec 3.2 and reuse existing policy parts. */
#if !defined(CHROME_FID_MUT_BOX_GEOM)
static void draw_platinum_widget(GrafPort *port, const flair_skin_t *skin,
                                 int bx, int by, int kind)
{
    const int box = FLAIR_CHROME_WIDGET_BOX;
    const int interior = FLAIR_CHROME_WIDGET_INTERIOR;
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

    crect(port, bx, by, bx + box, by + box, FLAIR_PART_PLAT_FRAME_FACE);
    cfill(port, bx, by, box, FLAIR_PART_PLAT_WIDGET_EDGE);
    for (int y = by; y < by + box; y++) {
        cfill(port, bx, y, 1, FLAIR_PART_PLAT_WIDGET_EDGE);
    }
    cframe(port, bx + 1, by + 1, bx + box, by + box,
           FLAIR_PART_PLAT_DARK_RING);

    crect(port, bx + 2, by + 2, bx + 2 + interior, by + 2 + interior,
          FLAIR_PART_PLAT_FRAME_FACE);
    cfill(port, bx + 2, by + 2, interior, FLAIR_PART_PLAT_FRAME_FACE);
    for (int y = by + 2; y < by + 2 + interior; y++) {
        cfill(port, bx + 2, y, 1, FLAIR_PART_PLAT_FRAME_FACE);
    }
    cfill(port, bx + 2, by + 2 + interior - 1, interior,
          FLAIR_PART_PLAT_WIDGET_EDGE);
    for (int y = by + 2; y < by + 2 + interior; y++) {
        cfill(port, bx + 2 + interior - 1, y, 1,
              FLAIR_PART_PLAT_WIDGET_EDGE);
    }
    cfill(port, bx + 2, by + 2, 1, FLAIR_PART_CONTENT);

    for (int dy = 1; dy <= FLAIR_CHROME_WIDGET_RAMP_FACE; dy++) {
        for (int dx = 1; dx <= FLAIR_CHROME_WIDGET_RAMP_FACE; dx++) {
#if defined(CHROME_FID_MUT_RAMP)
            int part = FLAIR_PART_PLAT_FRAME_FACE;
#else
            int rung = (dx + dy - 2) /
                       FLAIR_CHROME_WIDGET_RAMP_PX_PER_STEP;
            int part = ramp_part[rung];
#endif
            cfill(port, bx + 2 + dx, by + 2 + dy, 1, part);
        }
    }

    if (kind == PLAT_WIDGET_ZOOM) {
        for (int d = 0; d < FLAIR_CHROME_ZOOM_GLYPH_EDGE; d++) {
            cfill(port, bx + 2 + 5, by + 2 + d, 1,
                  FLAIR_PART_PLAT_DARK_RING);
            cfill(port, bx + 2 + d, by + 2 + 5, 1,
                  FLAIR_PART_PLAT_DARK_RING);
        }
    } else if (kind == PLAT_WIDGET_COLLAPSE) {
        cfill(port, bx + 2,
              by + 2 + FLAIR_CHROME_COLLAPSE_GLYPH_ROW_0,
              interior, FLAIR_PART_PLAT_DARK_RING);
        cfill(port, bx + 2,
              by + 2 + FLAIR_CHROME_COLLAPSE_GLYPH_ROW_1,
              interior, FLAIR_PART_PLAT_DARK_RING);
    }

    for (int y = by + 1; y <= by + box; y++) {
        cfill(port, bx + box, y, 1, FLAIR_PART_CONTENT);
    }
    cfill(port, bx + 1, by + box, box, FLAIR_PART_CONTENT);
}
#endif

/* Shared Platinum title-band renderer. The exact sampled row profile is:
 * frame, highlight, two face, twelve alternating stripes, four face, shadow,
 * frame. Ref: window-chrome.md Sec 2.1-2.3. */
static int draw_titlebar_band(GrafPort *port, const flair_skin_t *skin,
                              int left, int top, int right,
                              const char *title, int hilited)
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
        for (int row = 0; row < FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS; row++) {
#if defined(CHROME_FID_MUT_PHASE)
            int light = (row & 1) != 0;
#else
            int light = (row & 1) == 0;
#endif
            cfill(port, left + 1,
                  top + FLAIR_CHROME_TITLE_STRIPE_TOP_OFF + row,
                  w - 2,
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

#if defined(CHROME_FID_MUT_NO_TITLE)
    (void)title;
#else
    if (title != 0 && title[0] != '\0') {
        int tw = text_measure(FONT_CHICAGO, title);
        int tx = left + (w - tw) / 2;
        int ty = top + FLAIR_CHROME_WIDGET_TOP_OFF;
        int ink_part;

#if defined(CHROME_FID_MUT_INACTIVE_BRIGHT_TITLE)
        ink_part = FLAIR_PART_TEXT;
#else
        ink_part = active ? FLAIR_PART_TEXT : FLAIR_PART_PLAT_INACTIVE_TEXT;
#endif
        text_draw(&port->portBits.bm, tx, ty, title, FONT_CHICAGO,
                  flair_look_pixel_for_skin(port, skin, ink_part),
                  flair_look_pixel_for_skin(port, skin,
                                            active
                                            ? FLAIR_PART_PLAT_FRAME_FACE
                                            : FLAIR_PART_PLAT_FACE));
    }
#endif
    return shared_line;
}

#if !defined(CHROME_FID_MUT_SCROLL_FLAT)
static void draw_up_triangle(GrafPort *port, const flair_skin_t *skin,
                             int cx, int top, int part)
{
    for (int row = 0; row < 4; row++) {
        int width = 2 + 2 * row;
        cfill(port, cx - width / 2, top + row, width, part);
    }
}

static void draw_down_triangle(GrafPort *port, const flair_skin_t *skin,
                               int cx, int top, int part)
{
    for (int row = 0; row < 4; row++) {
        int width = 8 - 2 * row;
        cfill(port, cx - width / 2, top + row, width, part);
    }
}

static void draw_left_triangle(GrafPort *port, const flair_skin_t *skin,
                               int left, int cy, int part)
{
    for (int col = 0; col < 4; col++) {
        int height = 2 + 2 * col;
        for (int y = cy - height / 2; y < cy + height / 2; y++) {
            cfill(port, left + col, y, 1, part);
        }
    }
}

static void draw_right_triangle(GrafPort *port, const flair_skin_t *skin,
                                int left, int cy, int part)
{
    for (int col = 0; col < 4; col++) {
        int height = 8 - 2 * col;
        for (int y = cy - height / 2; y < cy + height / 2; y++) {
            cfill(port, left + col, y, 1, part);
        }
    }
}
#endif

/* Disabled active bars use a flat trough, dim arrows/separators, and no thumb.
 * Inactive bars are hollow: trough plus inactive frame only.
 * Ref: scrollbars.md Sec 1, Sec 3, and Sec 4. */
static void draw_vertical_scrollbar(GrafPort *port, const flair_skin_t *skin,
                                    int left, int top, int right, int bottom,
                                    int active)
{
#if defined(CHROME_FID_MUT_SCROLL_FLAT)
    (void)active;
    crect(port, left, top, right + 1, bottom, FLAIR_PART_BTNFACE);
    cframe(port, left, top, right + 1, bottom, FLAIR_PART_FRAME);
    return;
#else
    int frame_part = active ? FLAIR_PART_FRAME
                            : FLAIR_PART_PLAT_INACTIVE_FRAME;
    int interior_right = active ? right - 1 : right;
    crect(port, left + 1, top + 1, interior_right, bottom - 1,
          FLAIR_PART_PLAT_TROUGH);
    for (int y = top; y < bottom; y++) {
        cfill(port, left, y, 1, frame_part);
        cfill(port, right, y, 1, frame_part);
    }
    cfill(port, left, top, right - left + 1, frame_part);
    cfill(port, left, bottom - 1, right - left + 1, frame_part);

    if (active && bottom - top >= 2 * FLAIR_CHROME_SCROLL_ARROW_TILE) {
        int top_sep = top + FLAIR_CHROME_SCROLL_ARROW_TILE - 1;
        int bottom_sep = bottom - FLAIR_CHROME_SCROLL_ARROW_TILE;
        int cx = (left + right) / 2;
        cfill(port, left + 1, top_sep, right - left - 1,
              FLAIR_PART_PLAT_INACTIVE_FRAME);
        cfill(port, left + 1, bottom_sep, right - left - 1,
              FLAIR_PART_PLAT_INACTIVE_FRAME);
        draw_up_triangle(port, skin, cx, top + 5,
                         FLAIR_PART_PLAT_WIDGET_EDGE);
        draw_down_triangle(port, skin, cx, bottom - 9,
                           FLAIR_PART_PLAT_WIDGET_EDGE);
    }
#endif
}

static void draw_horizontal_scrollbar(GrafPort *port, const flair_skin_t *skin,
                                      int left, int top, int right, int bottom,
                                      int active)
{
#if defined(CHROME_FID_MUT_SCROLL_FLAT)
    (void)active;
    crect(port, left, top, right, bottom + 1, FLAIR_PART_BTNFACE);
    cframe(port, left, top, right, bottom + 1, FLAIR_PART_FRAME);
    return;
#else
    int frame_part = active ? FLAIR_PART_FRAME
                            : FLAIR_PART_PLAT_INACTIVE_FRAME;
    int interior_bottom = active ? bottom - 1 : bottom;
    crect(port, left + 1, top + 1, right - 1, interior_bottom,
          FLAIR_PART_PLAT_TROUGH);
    cfill(port, left, top, right - left, frame_part);
    cfill(port, left, bottom, right - left, frame_part);
    for (int y = top; y <= bottom; y++) {
        cfill(port, left, y, 1, frame_part);
        cfill(port, right - 1, y, 1, frame_part);
    }

    if (active && right - left >= 2 * FLAIR_CHROME_SCROLL_ARROW_TILE) {
        int left_sep = left + FLAIR_CHROME_SCROLL_ARROW_TILE - 1;
        int right_sep = right - FLAIR_CHROME_SCROLL_ARROW_TILE;
        int cy = (top + bottom) / 2;
        for (int y = top + 1; y < bottom; y++) {
            cfill(port, left_sep, y, 1, FLAIR_PART_PLAT_INACTIVE_FRAME);
            cfill(port, right_sep, y, 1, FLAIR_PART_PLAT_INACTIVE_FRAME);
        }
        draw_left_triangle(port, skin, left + 5, cy,
                           FLAIR_PART_PLAT_WIDGET_EDGE);
        draw_right_triangle(port, skin, right - 9, cy,
                            FLAIR_PART_PLAT_WIDGET_EDGE);
    }
#endif
}

/* Active 18x18 grow cell and exact three-line grip transcription.
 * Ref: window-chrome.md Sec 5. Inactive is the flat Sec 6 face. */
static void draw_grow_box(GrafPort *port, const flair_skin_t *skin,
                          int gx, int gy, int active)
{
    const int cell = FLAIR_CHROME_GROW;

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
                  1, FLAIR_PART_PLAT_STRIPE_DARK);
        }
        cfill(port, gx + start_x - 5, gy + start_row + 6,
              1, FLAIR_PART_PLAT_WELL);
        cfill(port, gx + start_x - 4, gy + start_row + 6,
              1, FLAIR_PART_PLAT_STRIPE_DARK);
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
    int bi = bottom - 1;

    for (int y = shared_line + 1; y < bi; y++) {
        if (active) {
            cfill(port, left + 1, y, 1, FLAIR_PART_CONTENT);
            cfill(port, left + 2, y, 2, FLAIR_PART_PLAT_FRAME_FACE);
            cfill(port, left + 4, y, 1, FLAIR_PART_PLAT_FRAME_SHADOW);
            cfill(port, right - 5, y, 1, FLAIR_PART_CONTENT);
            cfill(port, right - 4, y, 2, FLAIR_PART_PLAT_FRAME_FACE);
            cfill(port, right - 2, y, 1, FLAIR_PART_PLAT_FRAME_SHADOW);
        } else {
            cfill(port, left + 1, y, FLAIR_CHROME_BODY_BAR,
                  FLAIR_PART_PLAT_FACE);
            cfill(port, right - 5, y, FLAIR_CHROME_BODY_BAR,
                  FLAIR_PART_PLAT_FACE);
        }
    }

    if (active) {
        cfill(port, left + 1, bi - 4, right - left - 2,
              FLAIR_PART_CONTENT);
        cfill(port, left + 1, bi - 3, right - left - 2,
              FLAIR_PART_PLAT_FRAME_FACE);
        cfill(port, left + 1, bi - 2, right - left - 2,
              FLAIR_PART_PLAT_FRAME_FACE);
        cfill(port, left + 1, bi - 1, right - left - 2,
              FLAIR_PART_PLAT_FRAME_SHADOW);
    } else {
        crect(port, left + 1, bi - 4, right - 1, bi,
              FLAIR_PART_PLAT_FACE);
    }

    for (int y = shared_line; y <= bi - 4; y++) {
        cfill(port, left + 5, y, 1, frame_part);
        cfill(port, right - 6, y, 1, frame_part);
    }
    cfill(port, left + 5, bi - 5, right - left - 10, frame_part);

    cfill(port, left + 6, shared_line + 1, right - left - 12,
          FLAIR_PART_CONTENT);
    for (int y = shared_line + 1; y <= bi - 6; y++) {
        cfill(port, left + 6, y, 1, FLAIR_PART_CONTENT);
        cfill(port, right - 7, y, 1, FLAIR_PART_PLAT_WELL);
    }
    cfill(port, left + 6, bi - 6, right - left - 12,
          FLAIR_PART_PLAT_WELL);
#endif
}

void flair_draw_document_window(GrafPort *port, const flair_skin_t *skin,
                                rgn_rect_t frame,
                                const char *title, int hilited)
{
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

    if (port == 0 || skin == (const flair_skin_t *)0) {
        return;
    }
    left = frame.left;
    top = frame.top;
    right = frame.right;
    bottom = frame.bottom;
    w = right - left;
    h = bottom - top;
    if (w < 2 * FLAIR_CHROME_GROW + 2 ||
        h < FLAIR_CHROME_TITLEBAR_H + FLAIR_CHROME_GROW + 2) {
        return;
    }

    active = hilited;
#if defined(CHROME_FID_MUT_NO_INACTIVE)
    active = 1;
#endif
    shared_line = draw_titlebar_band(port, skin, left, top, right, title, active);
    frame_part = active ? FLAIR_PART_FRAME
                        : FLAIR_PART_PLAT_INACTIVE_FRAME;
#if defined(CHROME_FID_MUT_INACTIVE_BLACK_FRAME)
    if (!active) {
        frame_part = FLAIR_PART_FRAME;
    }
#endif

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
    grow_x = ri - 19;
    grow_y = bi - 19;
    sb_left = ri - 20;
#if defined(CHROME_MUTATE_SCROLLBAR_W)
    sb_left++;
#endif
    draw_vertical_scrollbar(port, skin, sb_left, shared_line + 1,
                            ri - 5, grow_y, active);
    draw_horizontal_scrollbar(port, skin, left + 5, bi - 20,
                              grow_x, bi - 5, active);
    draw_grow_box(port, skin, grow_x, grow_y, active);

    if (active
#if defined(CHROME_FID_MUT_KEEP_GADGETS)
        || !active
#endif
       ) {
        int by = top + FLAIR_CHROME_WIDGET_TOP_OFF;
#if defined(CHROME_FID_MUT_BOX_GEOM)
        int old_box = FLAIR_CHROME_SYS7_WBOX_DELTA;
        cframe(port, left + 4, by, left + 4 + old_box, by + old_box,
               FLAIR_PART_FRAME);
        cframe(port, ri - 20, by, ri - 20 + old_box, by + old_box,
               FLAIR_PART_FRAME);
#else
        draw_platinum_widget(port, skin,
                             left + FLAIR_CHROME_CLOSE_LEFT_OFF,
                             by, PLAT_WIDGET_CLOSE);
        draw_platinum_widget(port, skin,
                             ri - FLAIR_CHROME_ZOOM_RIGHT_OFF,
                             by, PLAT_WIDGET_ZOOM);
#if !defined(CHROME_FID_MUT_COLLAPSE)
        draw_platinum_widget(port, skin,
                             ri - FLAIR_CHROME_COLLAPSE_RIGHT_OFF,
                             by, PLAT_WIDGET_COLLAPSE);
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

    (void)draw_titlebar_band(port, skin, left, top, right, title, 1);
    cframe(port, left, top, right, bottom, FLAIR_PART_FRAME);
}
