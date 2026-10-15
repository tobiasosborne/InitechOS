/*
 * test_chrome_fidelity.c -- Platinum chrome fidelity oracle.
 *
 * This grades the real os/flair/chrome.c render against the independent
 * sys8 golden encoded in spec/chrome_fidelity_golden.h. Expected values never
 * come from chrome_metrics.h (Law 2 / HER-02). The live renderer is Platinum;
 * every leg is green and its named implementation mutants must run red.
 *
 * Ground truth: ../system7-decomp/specs/sys8/window-chrome.md Sec 1-6,
 * ../system7-decomp/specs/sys8/scrollbars.md Sec 1-5, and
 * ../system7-decomp/specs/sys8/platinum-palette.md Sec 1-2.
 * Re-key authority: ADR-0004-AMENDMENT-DEC-10 Sec 4 / OQ-7.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#include "render.h"
#include "chrome.h"
#include "window.h"        /* CalcDocContentRect: the owner's content seam (tdnl.51) */
#include "flair_look.h"
#include "chrome_fidelity_golden.h"
#include "chicago12.h"     /* glyph ART only + CHICAGO_CAP_TOP; metrics come from the golden */
#include "test_assert.h"

TEST_HARNESS();

enum {
    WIN_LEFT = 40,
    WIN_TOP = 30,
    WIN_RIGHT = 360,
    WIN_BOTTOM = 300,
    TRUNC_LEFT = 400,
    TRUNC_TOP = 30,
    TRUNC_RIGHT = 550,
    TRUNC_BOTTOM = 220
};

#define TEST_TITLE FG_TITLE_FIXED_TEXT

static rgn_rect_t win_frame(void)
{
    rgn_rect_t r;
    r.top = WIN_TOP;
    r.left = WIN_LEFT;
    r.bottom = WIN_BOTTOM;
    r.right = WIN_RIGHT;
    return r;
}

/* THE ACTIVE SCENE SCROLLS (bead initech-tdnl.35). Before tdnl.35 the active
 * gutters drew the ENABLED anatomy with no thumb (a product-state substitute
 * for the missing scroll model). Now the Window Manager carries one, so the
 * enabled-anatomy legs (WELL/TILE/SEPARATOR, BODYBAR's DA seam) grade a window
 * that HAS something to scroll -- unchanged probes -- and its thumb is graded
 * at the value's hand-computed position (leg SCROLL-THUMB). The window with
 * nothing to scroll is the separate DISABLED scene (leg SCROLL-DISABLED,
 * scrollbars.md Sec 3). The values are picked so the thumbs avoid every
 * existing well probe (y=150 / x=150 / x=mid / both near insets):
 *   V bar x [339,355) y [51,280): track_lo = 51+16 = 67,
 *     span = (280-16-15) - 67 = 182; value 133 of max 182 -> pos 67+133 = 200
 *   H bar x [45,340) y [279,295): track_lo = 45+16 = 61,
 *     span = (340-16-15) - 61 = 248; value 189 of max 248 -> pos 61+189 = 250
 * (winscroll.h geometry; the arithmetic is restated here by hand.) */
enum { SB_V_VALUE = 133, SB_V_MAX = 182, SB_V_POS = 200,
       SB_H_VALUE = 189, SB_H_MAX = 248, SB_H_POS = 250 };

static void draw_active(GrafPort *port)
{
    WindowScroll ws;
    memset(&ws, 0, sizeof ws);
    ws.axis[WSCROLL_V].value = SB_V_VALUE;
    ws.axis[WSCROLL_V].max = SB_V_MAX;
    ws.axis[WSCROLL_V].drag_pos = -1;
    ws.axis[WSCROLL_H].value = SB_H_VALUE;
    ws.axis[WSCROLL_H].max = SB_H_MAX;
    ws.axis[WSCROLL_H].drag_pos = -1;
    flair_draw_document_window_scroll(port, flair_look_default_skin(),
                                      win_frame(), TEST_TITLE, 1,
                                      FLAIR_WINDOW_WIDGET_ALL, &ws);
}

/* Nothing to scroll: no scroll record at all (scrollbars.md Sec 3). */
static void draw_disabled(GrafPort *port)
{
    flair_draw_document_window(port, flair_look_default_skin(),
                               win_frame(), TEST_TITLE, 1,
                               FLAIR_WINDOW_WIDGET_ALL);
}

/* The DOWN arrow of the vertical bar held (authored PRESSED art). */
static void draw_pressed(GrafPort *port)
{
    WindowScroll ws;
    memset(&ws, 0, sizeof ws);
    ws.axis[WSCROLL_V].value = SB_V_VALUE;
    ws.axis[WSCROLL_V].max = SB_V_MAX;
    ws.axis[WSCROLL_V].drag_pos = -1;
    ws.axis[WSCROLL_V].hilite = WSCROLL_PART_DOWN;
    ws.axis[WSCROLL_H].drag_pos = -1;
    flair_draw_document_window_scroll(port, flair_look_default_skin(),
                                      win_frame(), TEST_TITLE, 1,
                                      FLAIR_WINDOW_WIDGET_ALL, &ws);
}

static void draw_inactive(GrafPort *port)
{
    flair_draw_document_window(port, flair_look_default_skin(),
                               win_frame(), TEST_TITLE, 0,
                               FLAIR_WINDOW_WIDGET_ALL);
}

static rgn_rect_t trunc_frame(void)
{
    rgn_rect_t r;
    r.top = TRUNC_TOP;
    r.left = TRUNC_LEFT;
    r.bottom = TRUNC_BOTTOM;
    r.right = TRUNC_RIGHT;
    return r;
}

static void draw_truncated(GrafPort *port)
{
    flair_draw_document_window(port, flair_look_default_skin(),
                               trunc_frame(), FG_TITLE_TRUNC_SOURCE, 1,
                               FLAIR_WINDOW_WIDGET_ALL);
}

static void draw_close_collapse_only(GrafPort *port)
{
    flair_draw_document_window(port, flair_look_default_skin(),
                               win_frame(), TEST_TITLE, 1,
                               FLAIR_WINDOW_WIDGET_CLOSE |
                               FLAIR_WINDOW_WIDGET_COLLAPSE);
}

static uint32_t px(const render_ctx_t *ctx, int x, int y)
{
    return render_pixel_index(ctx, (uint32_t)x, (uint32_t)y);
}

static int count_idx(const render_ctx_t *ctx, int x0, int y0, int x1, int y1,
                     uint32_t want)
{
    int n = 0;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (px(ctx, x, y) == want) {
                n++;
            }
        }
    }
    return n;
}

static uint32_t title_class_idx(char c)
{
    switch (c) {
    case 'K': return FG_FRAME_IDX;
    case 'H': return FG_WHITE_IDX;
    case 'F': return FG_TITLE_FRAME_FACE_IDX;
    case 'L': return FG_TITLE_STRIPE_LIGHT_IDX;
    case 'D': return FG_TITLE_STRIPE_DARK_IDX;
    case 'S': return FG_TITLE_FRAME_SHADOW_IDX;
    default:  return 0xFFFFFFFFu;
    }
}

/* Golden Chicago 12 metrics for byte c (NFNT 5478, chicago12_nfnt_golden.h);
 * the oracle's title strings are printable ASCII. */
static int fg_aw(int c)
{
    return nfnt5478_aw(c);
}

static int fg_lb(int c)
{
    return nfnt5478_lb(c);
}

static int fg_run_w(const char *text)
{
    int w = 0;
    for (; *text; text++) w += fg_aw((unsigned char)*text);
    return w;
}

/* Ink at run column x, cell row `row`: the glyph whose GOLDEN advance covers
 * x, its locked ART row placed at the glyph's GOLDEN left bearing. */
static int golden_run_ink(const char *text, int x, int row)
{
    int pen = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        int c = (unsigned char)text[i];
        int aw = fg_aw(c);
        if (x >= pen && x < pen + aw) {
            int col = x - pen - fg_lb(c);
            if (col < 0 || col > 15) return 0;
            return (chicago12_rows[chicago12_index(c)][row] &
                    (0x8000u >> col)) != 0u;
        }
        pen += aw;
    }
    return 0;
}

static void golden_chicago_ink_bounds(const char *text,
                                      int *ink_left, int *ink_right)
{
    int width = fg_run_w(text);
    int lo = width;
    int hi = -1;
    for (int x = 0; x < width; x++) {
        for (int row = 0; row < CHICAGO_CELL_H; row++) {
            if (golden_run_ink(text, x, row)) {
                if (x < lo) lo = x;
                if (x > hi) hi = x;
            }
        }
    }
    if (hi < lo) {
        lo = 0;
        hi = width - 1;
    }
    *ink_left = lo;
    *ink_right = hi + 1;
}

/* Independent TITLE bitmap oracle.  Geometry comes only from the sampled
 * golden constants above; glyph pixels are composed directly from the locked
 * Chicago strike bytes (MSB is the leftmost pixel).  This path deliberately
 * calls neither text_draw/text_measure nor any chrome.c layout helper.
 * Ref: window-chrome.md Sec 2.2/2.3 and PRD Sec 6.4. */
static uint32_t expected_title_idx(int x, int y, rgn_rect_t frame,
                                   const char *shown, int tx, int active)
{
    int row = y - frame.top;
    uint32_t base;
    int text_row = row - (FG_TITLE_INK_TOP_OFF - CHICAGO_CAP_TOP);
    int text_x = x - tx;
    int text_w = fg_run_w(shown);
    int ink_left;
    int ink_right;
    golden_chicago_ink_bounds(shown, &ink_left, &ink_right);

    if (active) {
        base = title_class_idx(FG_TITLE_BAND_PROFILE[row]);
        if (row >= FG_TITLE_STRIPE_TOP_OFF &&
            row < FG_TITLE_STRIPE_TOP_OFF + FG_TITLE_STRIPE_ROWS) {
            int stripe_row = row - FG_TITLE_STRIPE_TOP_OFF;
            int shift = (stripe_row & 1) ? FG_TITLE_STRIPE_DARK_SHIFT : 0;
            int field_l = frame.left + FG_TITLE_STRIPE_LEFT_OFF + shift;
            int field_r = frame.right - FG_TITLE_STRIPE_RIGHT_OFF + shift;
            int gap_l = tx + ink_left - FG_TITLE_GAP_PAD_LEFT + shift;
            int gap_r = tx + ink_right + FG_TITLE_GAP_PAD_RIGHT + shift;
            if (x < field_l || x >= field_r) {
                base = FG_TITLE_FRAME_FACE_IDX;
            } else if (x >= gap_l && x < gap_r) {
                base = FG_TITLE_KNOCKOUT_IDX;
            }
        }
    } else {
        base = (row == 0 || row == FG_TITLE_BAND_ROWS - 1)
                   ? FG_INACTIVE_FRAME_IDX : FG_INACTIVE_TITLE_FILL_IDX;
    }

    if (text_row >= 0 && text_row < CHICAGO_CELL_H && text_x >= 0 &&
        text_x < text_w) {
        if (golden_run_ink(shown, text_x, text_row)) {
            return active ? FG_TITLE_INK_IDX : FG_INACTIVE_TEXT_IDX;
        }
        return active ? FG_TITLE_KNOCKOUT_IDX : FG_INACTIVE_TITLE_FILL_IDX;
    }
    return base;
}

static int title_bitmap_matches(const render_ctx_t *ctx, rgn_rect_t frame,
                                const char *shown, int tx, int active)
{
    int x0 = frame.left + FG_TITLE_STRIPE_LEFT_OFF - 1;
    int x1 = frame.right - FG_TITLE_STRIPE_RIGHT_OFF + 2;
    for (int y = frame.top; y < frame.top + FG_TITLE_BAND_ROWS; y++) {
        for (int x = x0; x < x1; x++) {
            if (px(ctx, x, y) != expected_title_idx(x, y, frame,
                                                    shown, tx, active)) {
                return 0;
            }
        }
    }
    return 1;
}

static int widget_shell_ok(const render_ctx_t *ctx, int bx, int by)
{
    int ok = 1;
    for (int k = 0; k < FG_BOX_RENDER_SIZE; k++) {
        ok = ok && px(ctx, bx + k, by) == (uint32_t)FG_BOX_EDGE_IDX;
        ok = ok && px(ctx, bx, by + k) == (uint32_t)FG_BOX_EDGE_IDX;
    }
    for (int k = 1; k < FG_BOX_RENDER_SIZE; k++) {
        ok = ok && px(ctx, bx + k, by + 1) == (uint32_t)FG_BOX_RING_IDX;
        ok = ok && px(ctx, bx + 1, by + k) == (uint32_t)FG_BOX_RING_IDX;
        ok = ok && px(ctx, bx + k, by + FG_BOX_RENDER_SIZE - 1) ==
                       (uint32_t)FG_BOX_RING_IDX;
        ok = ok && px(ctx, bx + FG_BOX_RENDER_SIZE - 1, by + k) ==
                       (uint32_t)FG_BOX_RING_IDX;
        ok = ok && px(ctx, bx + FG_BOX_RENDER_SIZE, by + k) ==
                       (uint32_t)FG_BOX_OUTER_HIGHLIGHT_IDX;
        ok = ok && px(ctx, bx + k, by + FG_BOX_RENDER_SIZE) ==
                       (uint32_t)FG_BOX_OUTER_HIGHLIGHT_IDX;
    }
    return ok;
}

static int ramp_ok_at(const render_ctx_t *ctx, int bx, int by)
{
    int ok = FG_BOX_RAMP_SAMPLED[0] == 179 &&
             FG_BOX_RAMP_SAMPLED[1] == 192 &&
             FG_BOX_RAMP_SAMPLED[2] == 205 &&
             FG_BOX_RAMP_SAMPLED[3] == 218 &&
             FG_BOX_RAMP_SAMPLED[4] == 231 &&
             FG_BOX_RAMP_SAMPLED[5] == 243 &&
             FG_BOX_RAMP_SAMPLED[6] == 255;
    for (int dy = 1; dy <= FG_BOX_RAMP_FACE_SIZE; dy++) {
        for (int dx = 1; dx <= FG_BOX_RAMP_FACE_SIZE; dx++) {
            int rung = (dx + dy - 2) / FG_BOX_RAMP_PX_PER_STEP;
            ok = ok && rung >= 0 && rung < FG_BOX_RAMP_RUNGS;
            if (rung >= 0 && rung < FG_BOX_RAMP_RUNGS) {
                ok = ok && px(ctx, bx + 2 + dx, by + 2 + dy) ==
                               (uint32_t)FG_BOX_RAMP_IDX[rung];
            }
        }
    }
    return ok;
}


/* ===========================================================================
 * CAPTURE LEGS (beads initech-tdnl.51 / initech-tdnl.52; audit F16).
 *
 * These legs decode the two INDEPENDENT Mac OS 8.1 captures at test time
 * (../system7-decomp/goldens/captures/s8_doc_window_active.png and
 * s8_doc_window_inactive.png, libpng, the test-scrollbar-look pattern) and
 * grade the rendered window against the capture's own pixels, aligned on the
 * title bar (frame-relative offsets). No expected value comes from
 * chrome_metrics.h, from chrome_fidelity_golden.h or from the renderer (Law 2
 * / HER-02): the capture IS the golden.
 *
 * The scene is the LIVE composition, not the chrome alone: the window is
 * drawn, then its OWNER fills the content rect the Window Manager hands it
 * (CalcDocContentRect -- what the Finder's finder_win_paint and every tenant
 * do) with the white body. The F16 body-frame defect was exactly that owner
 * fill erasing the left raised bar (contRgn began at L+1), which a chrome-only
 * render cannot see.
 *
 * Capture frames (window-chrome.md Sec 1 / Sec 6, inclusive):
 *   active   "Initech81"  L=65 T=70 R=480 B=315 (s8_doc_window_active)
 *   inactive "Initech81"  L=13 T=29 R=325 B=198 (s8_doc_window_inactive)
 * Cross-sections (outside -> in):
 *   left   row T+80 (active) / T+91 (inactive), 7 px: line, 4-px bar, line,
 *          the content-inset column
 *   right  same row, 21 px: line, bar, inner line, the 14-px disabled /
 *          hollow gutter interior, the gutter's left line
 *   bottom column L+55 (active) / L+200 (inactive), 21 px, likewise
 *   top    same column, the 22-row title band
 * Widgets: the 13x13 footprints at L+4 / R-32 / R-16, T+4 (Sec 3.1), all
 * 169 pixels each, against the active capture (the inactive window has none).
 * Hazard (Sec 2.3): the capture cursor sits on the active title TEXT only;
 * every sampled column/footprint here is clear of it.
 * ===========================================================================*/
#ifndef CHROME_FID_CAPTURE_DIR
#define CHROME_FID_CAPTURE_DIR "../system7-decomp/goldens/captures"
#endif

typedef struct capture_img {
    unsigned char *rgb;
    unsigned w, h;
} capture_img_t;

static int capture_load(capture_img_t *c, const char *path)
{
    png_image im;
    memset(c, 0, sizeof *c);
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, path)) {
        fprintf(stderr, "FAIL missing/unreadable independent capture: %s\n", path);
        return 0;
    }
    im.format = PNG_FORMAT_RGB;
    c->rgb = malloc(PNG_IMAGE_SIZE(im));
    if (c->rgb == NULL || !png_image_finish_read(&im, NULL, c->rgb, 0, NULL)) {
        fprintf(stderr, "FAIL independent capture decode: %s\n", path);
        free(c->rgb);
        c->rgb = NULL;
        png_image_free(&im);
        return 0;
    }
    c->w = im.width;
    c->h = im.height;
    png_image_free(&im);
    if (c->w != 640u || c->h != 480u) {
        fprintf(stderr, "FAIL independent capture is not 640x480: %s\n", path);
        free(c->rgb);
        c->rgb = NULL;
        return 0;
    }
    return 1;
}

static uint32_t cap_rgb(const capture_img_t *c, int x, int y)
{
    const unsigned char *q = c->rgb + 3u * ((unsigned)y * c->w + (unsigned)x);
    return ((uint32_t)q[0] << 16) | ((uint32_t)q[1] << 8) | (uint32_t)q[2];
}

static uint32_t our_rgb(const render_ctx_t *ctx, int x, int y)
{
    return render_pixel_rgb(ctx, (uint32_t)x, (uint32_t)y) & 0x00FFFFFFu;
}

/* The owner's content fill (finder_win_paint / ref_tenant): the white body
 * over exactly the rect the Window Manager derives. */
static void owner_fill_content(GrafPort *port)
{
    rgn_rect_t c = CalcDocContentRect(win_frame());
    uint32_t white = flair_look_pixel_for_skin(port, flair_look_default_skin(),
                                               FLAIR_PART_CONTENT);
    for (int y = c.top; y < c.bottom; y++) {
        if (c.right > c.left) {
            surface_fill_span(&port->portBits.bm, (uint32_t)c.left,
                              (uint32_t)y, (uint32_t)(c.right - c.left), white);
        }
    }
}

static void draw_capture_active(GrafPort *port)
{
    /* No scroll record: the bars are DISABLED, as on "Initech81" (Sec 3). */
    flair_draw_document_window(port, flair_look_default_skin(), win_frame(),
                               TEST_TITLE, 1, FLAIR_WINDOW_WIDGET_ALL);
    owner_fill_content(port);
}

static void draw_capture_inactive(GrafPort *port)
{
    flair_draw_document_window(port, flair_look_default_skin(), win_frame(),
                               TEST_TITLE, 0, FLAIR_WINDOW_WIDGET_ALL);
    owner_fill_content(port);
}

typedef struct cap_frame { int l, t, r, b; } cap_frame_t;

static size_t cap_hex(char *buf, size_t at, size_t cap, uint32_t v)
{
    unsigned r = (unsigned)((v >> 16) & 0xFFu);
    unsigned g = (unsigned)((v >> 8) & 0xFFu);
    unsigned b = (unsigned)(v & 0xFFu);
    int n = (r == g && g == b)
                ? snprintf(buf + at, cap - at, "%02X ", r)
                : snprintf(buf + at, cap - at, "#%06X ", (unsigned)v);
    return (n > 0 && at + (size_t)n < cap) ? at + (size_t)n : at;
}

/* One cross-section: n samples from the frame-relative anchor inward.
 * which: 0 left (+x), 1 right (-x), 2 bottom (-y), 3 top (+y).
 * Prints both sequences (the measured table). */
static int cap_section(const char *leg, const char *edge,
                       const render_ctx_t *ours, cap_frame_t of,
                       const capture_img_t *cap, cap_frame_t cf,
                       int which, int off, int n)
{
    int ok = 1;
    char a[512], b[512];
    size_t ai = 0, bi = 0;
    a[0] = b[0] = '\0';
    for (int k = 0; k < n; k++) {
        int ox, oy, cx, cy;
        switch (which) {
        case 0:  ox = of.l + k;   oy = of.t + off; cx = cf.l + k;   cy = cf.t + off; break;
        case 1:  ox = of.r - k;   oy = of.t + off; cx = cf.r - k;   cy = cf.t + off; break;
        case 2:  ox = of.l + off; oy = of.b - k;   cx = cf.l + off; cy = cf.b - k;   break;
        default: ox = of.l + off; oy = of.t + k;   cx = cf.l + off; cy = cf.t + k;   break;
        }
        uint32_t o = our_rgb(ours, ox, oy);
        uint32_t c = cap_rgb(cap, cx, cy);
        if (o != c) ok = 0;
        ai = cap_hex(a, ai, sizeof a, c);
        bi = cap_hex(b, bi, sizeof b, o);
    }
    printf("  %s %-6s capture: %s\n", leg, edge, a);
    printf("  %s %-6s ours   : %s%s\n", leg, edge, b, ok ? "(match)" : "(DIFF)");
    return ok;
}

static int cap_widget(const char *name, const render_ctx_t *ours, int ox, int oy,
                      const capture_img_t *cap, int cx, int cy)
{
    int bad = 0;
    for (int dy = 0; dy <= FG_BOX_RENDER_SIZE; dy++) {
        for (int dx = 0; dx <= FG_BOX_RENDER_SIZE; dx++) {
            uint32_t o = our_rgb(ours, ox + dx, oy + dy);
            uint32_t c = cap_rgb(cap, cx + dx, cy + dy);
            if (o != c) {
                if (bad < 8) {
                    printf("  CAPTURE-WIDGETS %s footprint (dx%d,dy%d): capture %06X ours %06X\n",
                           name, dx, dy, (unsigned)c, (unsigned)o);
                }
                bad++;
            }
        }
    }
    printf("  CAPTURE-WIDGETS %-8s ours (%d,%d) vs capture (%d,%d): %d/169 differ\n",
           name, ox, oy, cx, cy, bad);
    return bad == 0;
}

int main(int argc, char **argv)
{
    render_boot_info_t boot;
    render_ctx_t active;
    render_ctx_t inactive;
    render_ctx_t trunc;
    render_ctx_t flagged;
    memset(&boot, 0, sizeof boot);
    boot.lfb_bpp = 8u;
    boot.lfb_width = 640u;
    boot.lfb_height = 480u;

    int rc = render_ctx_init(&active, &boot);
    CHECK(rc == 0, "active 8bpp render context must initialize");
    if (rc != 0) {
        return TEST_SUMMARY("test-chrome-fidelity");
    }
    render_run(&active, draw_active);

    const int ri = WIN_RIGHT - 1;   /* inclusive structure right */
    const int bi = WIN_BOTTOM - 1;  /* inclusive structure bottom */
    const int pin_x = WIN_LEFT + FG_TITLE_STRIPE_LEFT_OFF + 2;
    const int mid_x = (WIN_LEFT + WIN_RIGHT) / 2;

    /* TITLE BAND: exact 22-row class profile, including 2/4 face asymmetry.
     * Ref: window-chrome.md Sec 2.1 and Sec 2.2.
     * Existing _PHASE and _BVL mutants bite this exact profile. */
    int band_ok = strlen(FG_TITLE_BAND_PROFILE) == FG_TITLE_BAND_ROWS;
    for (int row = 0; row < FG_TITLE_BAND_ROWS; row++) {
        band_ok = band_ok &&
            px(&active, pin_x, WIN_TOP + row) ==
                title_class_idx(FG_TITLE_BAND_PROFILE[row]);
    }
    CHECK(band_ok,
          "leg BAND: title must match 22-row KHFF+(LDx6)+FFFF+SK Platinum profile; "
          "12 stripes start light/end dark and face rows are 2 above/4 below "
          "(window-chrome.md Sec 2.1/2.2)");

    /* STRIPE EXTENTS: equal-length dark rows are shifted one pixel right,
     * including both field endpoints. Ref: window-chrome.md Sec 2.2. */
    const int stripe_l = WIN_LEFT + FG_TITLE_STRIPE_LEFT_OFF;
    const int stripe_r = WIN_RIGHT - FG_TITLE_STRIPE_RIGHT_OFF;
    int stripe_extent_ok =
        px(&active, stripe_l - 1, WIN_TOP + FG_TITLE_STRIPE_TOP_OFF) ==
            FG_TITLE_FRAME_FACE_IDX &&
        px(&active, stripe_l, WIN_TOP + FG_TITLE_STRIPE_TOP_OFF) ==
            FG_TITLE_STRIPE_LIGHT_IDX &&
        px(&active, stripe_r - 1, WIN_TOP + FG_TITLE_STRIPE_TOP_OFF) ==
            FG_TITLE_STRIPE_LIGHT_IDX &&
        px(&active, stripe_r, WIN_TOP + FG_TITLE_STRIPE_TOP_OFF) ==
            FG_TITLE_FRAME_FACE_IDX &&
        px(&active, stripe_l, WIN_TOP + FG_TITLE_STRIPE_TOP_OFF + 1) ==
            FG_TITLE_FRAME_FACE_IDX &&
        px(&active, stripe_l + FG_TITLE_STRIPE_DARK_SHIFT,
           WIN_TOP + FG_TITLE_STRIPE_TOP_OFF + 1) ==
            FG_TITLE_STRIPE_DARK_IDX &&
        px(&active, stripe_r, WIN_TOP + FG_TITLE_STRIPE_TOP_OFF + 1) ==
            FG_TITLE_STRIPE_DARK_IDX &&
        px(&active, stripe_r + 1, WIN_TOP + FG_TITLE_STRIPE_TOP_OFF + 1) ==
            FG_TITLE_FRAME_FACE_IDX;
    CHECK(stripe_extent_ok,
          "leg STRIPE-OFFSET: dark stripe rows must shift +1 at both equal-length "
          "field endpoints (window-chrome.md Sec 2.2)");

    /* TITLE: exact expected bitmap from the independent Chicago strike path.
     * The proportional run "TITLE" is T6+I6+T6+L7+E7 = 32 px (NFNT 5478,
     * golden table), centered at x=40+(320-32)/2 = 184, run x=184..215;
     * visible ink is x=184..214 (T lb 0; E lb 1 + 5 ink columns at pen 209),
     * so the sampled 6/5 gap is x=178..219 on light rows and +1 on dark rows.
     * (Was the fixed 5-cell run x=180..219; bead initech-tdnl.33.)
     * TITLE_BLANK and CENTER_OFF both RED.
     * Ref: window-chrome.md Sec 2.2/2.3. */
    const int title_tx = WIN_LEFT +
        ((WIN_RIGHT - WIN_LEFT) - fg_run_w(TEST_TITLE)) / 2;
    int title_ink = count_idx(&active, mid_x - 40, WIN_TOP + 4,
                              mid_x + 40, WIN_TOP + 21,
                              FG_TITLE_INK_IDX);
    int title_gap = count_idx(&active, mid_x - 40, WIN_TOP + 4,
                              mid_x + 40, WIN_TOP + 21,
                              FG_TITLE_KNOCKOUT_IDX);
    int title_ok = title_tx == 184 && title_ink >= 8 && title_gap >= 24 &&
                   title_bitmap_matches(&active, win_frame(), TEST_TITLE,
                                        title_tx, 1);
    CHECK(title_ok,
          "leg TITLE: Chicago 12 title run must occupy centered x=184..215 "
          "with black ink and the sampled idx218 6/5 gap (+1 on dark rows) "
          "(window-chrome.md Sec 2.2/2.3)");

    /* SHADOW: (+1,+1) black L with two-pixel near-corner notch.
     * Ref: window-chrome.md Sec 1. _NO_SHADOW and new _NOTCH bite here. */
    int shadow_ok =
        px(&active, WIN_RIGHT, WIN_TOP + FG_SHADOW_NOTCH - 1) ==
            (uint32_t)RENDER_DESKTOP_INDEX &&
        px(&active, WIN_RIGHT, WIN_TOP + FG_SHADOW_NOTCH) ==
            (uint32_t)FG_SHADOW_INK_IDX &&
        px(&active, WIN_RIGHT, (WIN_TOP + WIN_BOTTOM) / 2) ==
            (uint32_t)FG_SHADOW_INK_IDX &&
        px(&active, WIN_LEFT + FG_SHADOW_NOTCH - 1, WIN_BOTTOM) ==
            (uint32_t)RENDER_DESKTOP_INDEX &&
        px(&active, WIN_LEFT + FG_SHADOW_NOTCH, WIN_BOTTOM) ==
            (uint32_t)FG_SHADOW_INK_IDX;
    CHECK(shadow_ok,
          "leg SHADOW/NOTCH: black (+1,+1) shadow must start at top+2/left+2; "
          "System-7 starts one pixel too near and a no-shadow render misses the L "
          "(window-chrome.md Sec 1)");

    /* BODY: outer black, four-pixel raised bar, inner black, content inset.
     * Ref: window-chrome.md Sec 4. New _BODYBAR bites this inversion of the
     * retained FG_SYS7_BODY_NO_GROOVE tell. */
    static const uint8_t left_body[] = { 0, 1, 218, 218, 179, 0, 1 };
    int body_ok = 1;
    const int body_y = 150;
    for (int k = 0; k < (int)(sizeof left_body / sizeof left_body[0]); k++) {
        body_ok = body_ok && px(&active, WIN_LEFT + k, body_y) == left_body[k];
    }
    /* At the right side the 16px gutter replaces the content-inset column;
     * its sampled DA far highlight meets the same inner black body line. */
    body_ok = body_ok && px(&active, ri - 6, body_y) ==
                              FG_SB_ENABLED_WELL_HILITE1_IDX;
    body_ok = body_ok && px(&active, ri - 5, body_y) == FG_BODY_INNER_LINE_IDX;
    for (int k = 0; k < FG_BODY_BAR_ROWS; k++) {
        body_ok = body_ok && px(&active, ri - 4 + k, body_y) == FG_BODY_BAR_IDX[k];
    }
    body_ok = body_ok && px(&active, ri, body_y) == FG_FRAME_IDX;
    body_ok = body_ok && px(&active, mid_x, bi - 5) == FG_BODY_INNER_LINE_IDX;
    for (int k = 0; k < FG_BODY_BAR_ROWS; k++) {
        body_ok = body_ok && px(&active, mid_x, bi - 4 + k) == FG_BODY_BAR_IDX[k];
    }
    body_ok = body_ok && px(&active, mid_x, bi) == FG_FRAME_IDX;
    body_ok = body_ok && px(&active, mid_x, bi - 6) ==
                              FG_SB_ENABLED_WELL_HILITE1_IDX;
    CHECK(body_ok,
          "leg BODYBAR: body must have the four-pixel raised white/218/218/179 bar "
          "between outer and inner black lines plus the one-pixel content inset "
          "(or the enabled gutter's DA far highlight at the right seam); "
          "a System-7 single-line body is wrong (window-chrome.md Sec 4)");

    /* WIDGETS: measured inclusive-R offsets. Ref: window-chrome.md Sec 3.1. */
    const int box_y = WIN_TOP + FG_BOX_TOP_OFF;
    const int close_x = WIN_LEFT + FG_CLOSE_BOX_LEFT_OFF;
    const int zoom_x = ri - FG_ZOOM_BOX_RIGHT_OFF;
    const int collapse_x = ri - FG_COLLAPSE_BOX_RIGHT_OFF;
    int widget_ok = widget_shell_ok(&active, close_x, box_y) &&
                    widget_shell_ok(&active, zoom_x, box_y) &&
                    widget_shell_ok(&active, collapse_x, box_y) &&
                    collapse_x > zoom_x;
    CHECK(widget_ok,
          "leg WIDGETS: active chrome must carry three 12x12+highlight widgets at "
          "L+4, R-32, R-16 with collapse rightmost (window-chrome.md Sec 3.1)");

    int bevel_ring_ok = FG_BOX_RING_IDX == 63 &&
        px(&active, close_x + FG_BOX_RENDER_SIZE - 1,
           box_y + FG_BOX_RENDER_SIZE - 1) == FG_BOX_RING_IDX &&
        px(&active, zoom_x + FG_BOX_RENDER_SIZE - 1,
           box_y + FG_BOX_RENDER_SIZE - 1) == FG_BOX_RING_IDX &&
        px(&active, collapse_x + FG_BOX_RENDER_SIZE - 1,
           box_y + FG_BOX_RENDER_SIZE - 1) == FG_BOX_RING_IDX &&
        FG_BOX_RING_IDX != FG_FRAME_IDX;
    CHECK(bevel_ring_ok,
          "leg WIDGET-RING: every dark ring closes at its bottom-right corner "
          "in sampled idx63, distinct from black (window-chrome.md Sec 3.2)");

    int widget_ramp_ok = ramp_ok_at(&active, close_x, box_y);
    CHECK(widget_ramp_ok,
          "leg RAMP: close interior must be the seven-rung two-pixels-per-step "
          "diagonal ramp, dark upper-left to white lower-right "
          "(window-chrome.md Sec 3.2)");

    int collapse_ok = 1;
    for (int dx = 0; dx < FG_BOX_INTERIOR_SIZE; dx++) {
        collapse_ok = collapse_ok &&
            px(&active, collapse_x + 2 + dx,
               box_y + 2 + FG_COLLAPSE_GLYPH_ROW_0) == FG_BOX_RING_IDX;
        collapse_ok = collapse_ok &&
            px(&active, collapse_x + 2 + dx,
               box_y + 2 + FG_COLLAPSE_GLYPH_ROW_1) == FG_BOX_RING_IDX;
    }
    CHECK(collapse_ok,
          "leg COLLAPSE: the rightmost widget must exist and carry dark full-interior "
          "rows dy=3 and dy=5 (window-chrome.md Sec 3.3)");

    /* D3.a row 16: flags, not a drawer-wide assumption, select widgets. The
     * control-panel combination keeps close+collapse in their sampled slots
     * while the zoom slot remains plain title band. */
    int widget_flags_ok = 0;
    int flagged_init = render_ctx_init(&flagged, &boot);
    CHECK(flagged_init == 0, "flagged-widget 8bpp render context must initialize");
    if (flagged_init == 0) {
        render_run(&flagged, draw_close_collapse_only);
        int zoom_ink = count_idx(&flagged, zoom_x, box_y,
                                 zoom_x + FG_BOX_FOOTPRINT,
                                 box_y + FG_BOX_FOOTPRINT, FG_BOX_RING_IDX) +
                        count_idx(&flagged, zoom_x, box_y,
                                  zoom_x + FG_BOX_FOOTPRINT,
                                  box_y + FG_BOX_FOOTPRINT, FG_BOX_EDGE_IDX);
        int close_ink = count_idx(&flagged, close_x, box_y,
                                  close_x + FG_BOX_FOOTPRINT,
                                  box_y + FG_BOX_FOOTPRINT, FG_BOX_RING_IDX);
        int collapse_ink = count_idx(&flagged, collapse_x, box_y,
                                     collapse_x + FG_BOX_FOOTPRINT,
                                     box_y + FG_BOX_FOOTPRINT, FG_BOX_RING_IDX);
        widget_flags_ok = zoom_ink == 0 && close_ink > 0 && collapse_ink > 0;
        render_ctx_free(&flagged);
    }
    CHECK(widget_flags_ok,
          "leg WIDGET-FLAGS: close+collapse/no-zoom keeps collapse at R-16 and leaves zoom slot plain");

    int zoom_glyph_ok = 1;
    for (int d = 0; d < FG_ZOOM_GLYPH_EDGE; d++) {
        zoom_glyph_ok = zoom_glyph_ok &&
            px(&active, zoom_x + 2 + 5, box_y + 2 + d) == FG_BOX_RING_IDX;
        zoom_glyph_ok = zoom_glyph_ok &&
            px(&active, zoom_x + 2 + d, box_y + 2 + 5) == FG_BOX_RING_IDX;
    }
    int close_glyph_px = count_idx(&active, close_x + 3, box_y + 3,
                                    close_x + 10, box_y + 10,
                                    FG_BOX_RING_IDX);
    int glyph_ok = zoom_glyph_ok && close_glyph_px == 0;
    CHECK(glyph_ok,
          "leg GLYPHS: close has no glyph; zoom has only the six-pixel right and "
          "bottom edges in dark-ring ink (window-chrome.md Sec 3.3)");

    /* ACTIVE skeleton state choice: ENABLED well/tile anatomy, black arrows
     * and separators, but no thumb until Window Manager receives the R1.5
     * scroll-position model.  Each visible element is sampled from
     * scrollbars.md Sec 2; the explicit thumbless composition is recorded in
     * chrome_fidelity_golden.h. */
    const int sb_left = ri - 20;
    const int sb_right = ri - 5;
    const int sb_top = WIN_TOP + FG_SB_TOP_OFF;
    const int sb_mid_x = (sb_left + sb_right) / 2;
    const int sb_sep_y = sb_top + FG_SB_ARROW_TILE - 1;
    const int sb_bottom_sep_y = (bi - 19) - FG_SB_ARROW_TILE;
    const int sb_well_top = sb_sep_y + 1;
    const int sb_track_y = 150;
    const int grow_x = ri - 19;
    const int grow_y = bi - 19;
    const int hsb_left = WIN_LEFT + 5;
    const int hsb_top = grow_y - 1;
    const int hsb_right = grow_x;
    const int hsb_bottom = bi - 5;
    const int hsb_left_sep = hsb_left + FG_SB_ARROW_TILE - 1;
    const int hsb_right_sep = hsb_right - FG_SB_ARROW_TILE;
    const int hsb_mid_y = hsb_top + FG_SB_BAND / 2;
    const int hsb_track_x = 150;
    int sb_arrow_px = count_idx(&active, sb_left + 1, sb_top + 1,
                                 sb_right, sb_sep_y,
                                 FG_SB_ENABLED_GLYPH_IDX);
    int sb_thumb_px = count_idx(&active, sb_left + 1, sb_well_top,
                                 sb_right, sb_bottom_sep_y,
                                 FG_SB_ENABLED_THUMB_TEAL_IDX);
    int sb_well_ok =
        px(&active, sb_left, sb_track_y) == FG_SB_ENABLED_FRAME_IDX &&
        px(&active, sb_mid_x, sb_top) == FG_SB_ENABLED_FRAME_IDX &&
        px(&active, sb_left + 1, sb_track_y) ==
            FG_SB_ENABLED_WELL_SHADOW0_IDX &&
        px(&active, sb_left + 2, sb_track_y) ==
            FG_SB_ENABLED_WELL_SHADOW1_IDX &&
        px(&active, sb_left + 3, sb_track_y) == FG_SB_ENABLED_WELL_IDX &&
        px(&active, sb_right - 2, sb_track_y) ==
            FG_SB_ENABLED_WELL_HILITE0_IDX &&
        px(&active, sb_right - 1, sb_track_y) ==
            FG_SB_ENABLED_WELL_HILITE1_IDX &&
        px(&active, sb_mid_x, sb_well_top) ==
            FG_SB_ENABLED_WELL_SHADOW0_IDX &&
        px(&active, sb_mid_x, sb_well_top + 1) ==
            FG_SB_ENABLED_WELL_SHADOW1_IDX &&
        px(&active, sb_mid_x, sb_bottom_sep_y - 1) ==
            FG_SB_ENABLED_WELL_IDX &&
        px(&active, hsb_track_x, hsb_top + 1) ==
            FG_SB_ENABLED_WELL_SHADOW0_IDX &&
        px(&active, hsb_track_x, hsb_top + 2) ==
            FG_SB_ENABLED_WELL_SHADOW1_IDX &&
        px(&active, hsb_track_x, hsb_top + 3) ==
            FG_SB_ENABLED_WELL_IDX &&
        px(&active, hsb_track_x, hsb_bottom - 2) ==
            FG_SB_ENABLED_WELL_HILITE0_IDX &&
        px(&active, hsb_track_x, hsb_bottom - 1) ==
            FG_SB_ENABLED_WELL_HILITE1_IDX &&
        px(&active, hsb_left_sep + 1, hsb_mid_y) ==
            FG_SB_ENABLED_WELL_SHADOW0_IDX &&
        px(&active, hsb_left_sep + 2, hsb_mid_y) ==
            FG_SB_ENABLED_WELL_SHADOW1_IDX &&
        px(&active, hsb_right_sep - 1, hsb_mid_y) ==
            FG_SB_ENABLED_WELL_IDX;
    CHECK(sb_well_ok,
          "leg SCROLL-WELL: active gutter page must have the sampled "
          "150/165/192x10/205/218 cross-section, two-pixel dark near inset, "
          "and unstroked far end (scrollbars.md Sec 2.3)");

    int sb_tile_ok =
        px(&active, sb_mid_x, sb_top + 1) == FG_SB_AFTER_TOP_IDX &&
        px(&active, sb_left + 1, sb_top + 8) ==
            FG_SB_ENABLED_TILE_HI_IDX &&
        px(&active, sb_mid_x, sb_top + 2) ==
            FG_SB_ENABLED_TILE_FACE_IDX &&
        px(&active, sb_right - 1, sb_top + 8) ==
            FG_SB_ENABLED_TILE_SHADOW_IDX &&
        px(&active, sb_mid_x, sb_sep_y - 1) ==
            FG_SB_ENABLED_TILE_SHADOW_IDX &&
        sb_arrow_px == 20 &&
        px(&active, hsb_left + 8, hsb_top + 1) ==
            FG_SB_ENABLED_TILE_HI_IDX &&
        px(&active, hsb_left + 1, hsb_mid_y) ==
            FG_SB_ENABLED_TILE_HI_IDX &&
        px(&active, hsb_left + 3, hsb_top + 3) ==
            FG_SB_ENABLED_TILE_FACE_IDX &&
        px(&active, hsb_left_sep - 1, hsb_top + 3) ==
            FG_SB_ENABLED_TILE_SHADOW_IDX &&
        px(&active, hsb_left + 8, hsb_bottom - 1) ==
            FG_SB_ENABLED_TILE_SHADOW_IDX &&
        count_idx(&active, hsb_left + 1, hsb_top + 1,
                  hsb_left_sep, hsb_bottom,
                  FG_SB_ENABLED_GLYPH_IDX) == 20;
    CHECK(sb_tile_ok,
          "leg SCROLL-TILE: active arrow boxes must be raised white/E7/CD "
          "tiles with solid black 8x4 triangles (scrollbars.md Sec 2.2)");

    int sb_separator_ok =
        px(&active, sb_mid_x, sb_sep_y) == FG_SB_ENABLED_FRAME_IDX &&
        px(&active, sb_mid_x, sb_bottom_sep_y) == FG_SB_ENABLED_FRAME_IDX &&
        px(&active, hsb_left_sep, hsb_mid_y) == FG_SB_ENABLED_FRAME_IDX &&
        px(&active, hsb_right_sep, hsb_mid_y) == FG_SB_ENABLED_FRAME_IDX;
    CHECK(sb_separator_ok,
          "leg SCROLL-SEPARATOR: both active arrow-box separators must be "
          "black, never inactive gray (scrollbars.md Sec 2.1)");

    /* RE-KEYED (tdnl.35): SCROLL-NO-THUMB asserted the absence of a thumb
     * because no scroll model existed. With one, the ENABLED bar carries its
     * 15-px thumb at the value's position (scrollbars.md Sec 2.4): accent
     * pixels ONLY inside [pos, pos+15) along the axis, black separators at
     * pos-1 and pos+15, the sampled face at (along 1, cross 5) and highlight
     * at (along 1, cross 0). The thumbless case moved to SCROLL-DISABLED. */
    int sb_vthumb_in = count_idx(&active, sb_left + 1, SB_V_POS, sb_right,
                                 SB_V_POS + FG_SB_THUMB_MIN,
                                 FG_SB_ENABLED_THUMB_TEAL_IDX);
    int sb_hthumb_in = count_idx(&active, SB_H_POS, hsb_top + 1,
                                 SB_H_POS + FG_SB_THUMB_MIN, hsb_bottom,
                                 FG_SB_ENABLED_THUMB_TEAL_IDX);
    int sb_thumb_ok = sb_thumb_px == sb_vthumb_in && sb_vthumb_in > 0 &&
        count_idx(&active, hsb_left_sep + 1, hsb_top + 1,
                  hsb_right_sep, hsb_bottom,
                  FG_SB_ENABLED_THUMB_TEAL_IDX) == sb_hthumb_in &&
        sb_hthumb_in > 0 &&
        px(&active, sb_mid_x, SB_V_POS - 1) == FG_SB_ENABLED_FRAME_IDX &&
        px(&active, sb_mid_x, SB_V_POS + FG_SB_THUMB_MIN) ==
            FG_SB_ENABLED_FRAME_IDX &&
        px(&active, sb_left + 1 + 5, SB_V_POS + 1) ==
            FG_SB_ENABLED_THUMB_TEAL_IDX &&
        px(&active, SB_H_POS - 1, hsb_mid_y) == FG_SB_ENABLED_FRAME_IDX &&
        px(&active, SB_H_POS + FG_SB_THUMB_MIN, hsb_mid_y) ==
            FG_SB_ENABLED_FRAME_IDX &&
        px(&active, SB_H_POS + 1, hsb_top + 1 + 5) ==
            FG_SB_ENABLED_THUMB_TEAL_IDX;
    CHECK(sb_thumb_ok,
          "leg SCROLL-THUMB: an enabled bar's 15-px thumb sits at the value's "
          "position (v 200, h 250) between black separators, nowhere else "
          "(scrollbars.md Sec 2.4; control-manager.md value<->thumb)");

    /* GROW BOX: 18x18 cell, face/highlight, three pitched grip lines.
     * Ref: window-chrome.md Sec 5. */
    int grow_ok = strlen(FG_GROW_PROFILE) ==
                  FG_GROW_PROFILE_ROWS * FG_GROW_PROFILE_COLS &&
                  FG_GROW_GRIP_LINES == 3 && FG_GROW_GRIP_PITCH == 4;
    for (int y = 0; y < FG_GROW_PROFILE_ROWS; y++) {
        for (int x = 0; x < FG_GROW_PROFILE_COLS; x++) {
            char klass = FG_GROW_PROFILE[y * FG_GROW_PROFILE_COLS + x];
            uint32_t want = klass == 'W' ? FG_GROW_GRIP_LEAD_IDX :
                            klass == 'g' ? FG_GROW_GRIP_TRAIL_IDX :
                            klass == 'c' ? FG_GROW_GRIP_TERMINATOR_IDX :
                                           FG_GROW_FILL_IDX;
            grow_ok = grow_ok && px(&active, grow_x + x, grow_y + y) == want;
        }
    }
    CHECK(grow_ok,
          "leg GROW: active 18x18 cell must have idx218 fill, white top/left, "
          "and three white+150 grip lines at pitch4 with idx192 terminators "
          "(window-chrome.md Sec 5)");

    rc = render_ctx_init(&inactive, &boot);
    CHECK(rc == 0, "inactive 8bpp render context must initialize");
    if (rc != 0) {
        render_ctx_free(&active);
        return TEST_SUMMARY("test-chrome-fidelity");
    }
    render_run(&inactive, draw_inactive);

    /* INACTIVE title: flat 231, no stripes/bevel. _NO_INACTIVE bites here.
     * Ref: window-chrome.md Sec 6. */
    int inactive_title_ok = 1;
    for (int y = WIN_TOP + 1; y < WIN_TOP + FG_TITLE_BAND_ROWS - 1; y++) {
        inactive_title_ok = inactive_title_ok &&
            px(&inactive, pin_x, y) == FG_INACTIVE_TITLE_FILL_IDX;
    }
    CHECK(inactive_title_ok,
          "leg INACTIVE-TITLE: background title is flat idx231 for 20 interior "
          "rows, with no stripes or bevel; System-7 white idx1 is wrong "
          "(window-chrome.md Sec 6)");

    /* Inactive every frame line and shadow are 119. _INACTIVE_BLACK_FRAME bites. */
    int inactive_frame_ok =
        px(&inactive, pin_x, WIN_TOP) == FG_INACTIVE_FRAME_IDX &&
        px(&inactive, WIN_LEFT, body_y) == FG_INACTIVE_FRAME_IDX &&
        px(&inactive, ri, body_y) == FG_INACTIVE_FRAME_IDX &&
        px(&inactive, mid_x, bi) == FG_INACTIVE_FRAME_IDX &&
        px(&inactive, WIN_RIGHT, body_y) == FG_INACTIVE_SHADOW_IDX;
    CHECK(inactive_frame_ok,
          "leg INACTIVE-FRAME: all structure lines and the drop shadow must be "
          "inactive-frame idx119, not active black (window-chrome.md Sec 1/Sec 6)");

    /* Inactive text is sampled idx135, not retained System-7 idx165.
     * _INACTIVE_BRIGHT_TITLE bites active black residue. */
    int inactive_dim = count_idx(&inactive, mid_x - 40, WIN_TOP + 4,
                                 mid_x + 40, WIN_TOP + 21,
                                 FG_INACTIVE_TEXT_IDX);
    int inactive_black = count_idx(&inactive, mid_x - 40, WIN_TOP + 4,
                                   mid_x + 40, WIN_TOP + 21,
                                   FG_TITLE_INK_IDX);
    int inactive_text_ok = inactive_dim >= 8 && inactive_black == 0 &&
                           title_bitmap_matches(&inactive, win_frame(),
                                                TEST_TITLE, title_tx, 0);
    CHECK(inactive_text_ok,
          "leg INACTIVE-TEXT: background title ink must be idx135 and never active "
          "black; retained System-7 dim ink idx165 is wrong "
          "(window-chrome.md Sec 6)");

    /* TRUNCATION: the 150px frame has a sampled safe title run [421,512).
     * With the proportional NFNT 5478 advances only "ABCDEFGH." (66 px) fits
     * once the 6/5 gap is included, centered at x=442 (derivation in
     * chrome_fidelity_golden.h; bead initech-tdnl.33).  This records the R0.2
     * simplification: whole-character truncation plus one period, no font
     * condensation. */
    rc = render_ctx_init(&trunc, &boot);
    CHECK(rc == 0, "truncation 8bpp render context must initialize");
    int trunc_ok = 0;
    if (rc == 0) {
        int trunc_tx = TRUNC_LEFT +
            ((TRUNC_RIGHT - TRUNC_LEFT) -
             fg_run_w(FG_TITLE_TRUNC_EXPECTED)) / 2;
        render_run(&trunc, draw_truncated);
        trunc_ok = trunc_tx == FG_TITLE_TRUNC_X &&
                   title_bitmap_matches(&trunc, trunc_frame(),
                                        FG_TITLE_TRUNC_EXPECTED, trunc_tx, 1);
    }
    CHECK(trunc_ok,
          "leg TITLE-TRUNC: overlong title must become centered 'ABCDEFG' + the "
          "0xC9 ellipsis inside the measured widget-safe run (initech-tdnl.68)");

    /* No inactive widgets. _KEEP_GADGETS bites this retained relation.
     * Ref: window-chrome.md Sec 6. */
    int inactive_widget_px = 0;
    const int widget_x[3] = { close_x, zoom_x, collapse_x };
    for (int k = 0; k < 3; k++) {
        inactive_widget_px += count_idx(&inactive, widget_x[k], box_y,
                                        widget_x[k] + FG_BOX_FOOTPRINT,
                                        box_y + FG_BOX_FOOTPRINT,
                                        FG_BOX_EDGE_IDX);
        inactive_widget_px += count_idx(&inactive, widget_x[k], box_y,
                                        widget_x[k] + FG_BOX_FOOTPRINT,
                                        box_y + FG_BOX_FOOTPRINT,
                                        FG_BOX_RING_IDX);
    }
    int inactive_gadgets_ok = inactive_widget_px == 0;
    CHECK(inactive_gadgets_ok,
          "leg INACTIVE-GADGETS: background title carries no close, zoom, or "
          "collapse widget (window-chrome.md Sec 6)");

    /* Inactive scrollbar is HOLLOW and grow is flat. _SCROLL_FLAT/_NO_INACTIVE
     * cover the old active residue. Ref: scrollbars.md Sec 4 and
     * window-chrome.md Sec 5/Sec 6. */
    int hollow_interior_ok = 1;
    for (int y = sb_top + 1; y < bi - 20; y++) {
        for (int x = sb_left + 1; x < sb_right; x++) {
            hollow_interior_ok = hollow_interior_ok &&
                px(&inactive, x, y) == FG_SB_HOLLOW_TROUGH_IDX;
        }
    }
    int inactive_grow_ok = 1;
    for (int y = 0; y < FG_GROW_CELL; y++) {
        for (int x = 0; x < FG_GROW_CELL; x++) {
            inactive_grow_ok = inactive_grow_ok &&
                px(&inactive, grow_x + x, grow_y + y) ==
                    FG_INACTIVE_GROW_FILL_IDX;
        }
    }
    int hollow_ok = px(&inactive, sb_left, sb_track_y) == FG_SB_HOLLOW_FRAME_IDX &&
                    px(&inactive, sb_mid_x, sb_track_y) == FG_SB_HOLLOW_TROUGH_IDX &&
                    hollow_interior_ok && inactive_grow_ok;
    CHECK(hollow_ok,
          "leg HOLLOW/GROW-INACTIVE: background bars are idx243 trough inside "
          "idx119 frame with nothing else, and grow cell is flat idx231 "
          "(scrollbars.md Sec 4; window-chrome.md Sec 5/Sec 6)");

    /* Keep the acceptance tail self-contained: one line per Platinum leg. */
    printf("Platinum chrome fidelity leg inventory:\n");
    /* SCROLL-DISABLED (bead initech-tdnl.35): an ACTIVE window with nothing
     * to scroll draws Sec 3's DISABLED bar -- black outer lines, flat F3
     * trough, #777777 arrow-box separators, #A5A5A5 8x4 arrows (20 px each),
     * and NO thumb, no well, no black glyph. Ref: scrollbars.md Sec 3
     * (s8_doc_window_active "Initech81", whose contents fit). */
    render_ctx_t disabled;
    int sb_disabled_ok = 0;
    if (render_ctx_init(&disabled, &boot) == 0) {
        render_run(&disabled, draw_disabled);
        sb_disabled_ok =
            px(&disabled, sb_left, sb_track_y) == FG_SB_ENABLED_FRAME_IDX &&
            px(&disabled, sb_mid_x, sb_track_y) == FG_SB_DISABLED_TROUGH_IDX &&
            px(&disabled, sb_left + 1, sb_track_y) == FG_SB_DISABLED_TROUGH_IDX &&
            px(&disabled, sb_right - 1, sb_track_y) == FG_SB_DISABLED_TROUGH_IDX &&
            px(&disabled, sb_mid_x, sb_sep_y) == FG_SB_DISABLED_SEPARATOR_IDX &&
            px(&disabled, sb_mid_x, sb_bottom_sep_y) ==
                FG_SB_DISABLED_SEPARATOR_IDX &&
            px(&disabled, hsb_left_sep, hsb_mid_y) ==
                FG_SB_DISABLED_SEPARATOR_IDX &&
            px(&disabled, hsb_right_sep, hsb_mid_y) ==
                FG_SB_DISABLED_SEPARATOR_IDX &&
            px(&disabled, hsb_track_x, hsb_mid_y) == FG_SB_DISABLED_TROUGH_IDX &&
            count_idx(&disabled, sb_left + 1, sb_top + 1, sb_right, sb_sep_y,
                      FG_SB_DISABLED_ARROW_IDX) == 20 &&
            count_idx(&disabled, sb_left + 1, sb_top + 1, sb_right, sb_sep_y,
                      FG_SB_ENABLED_GLYPH_IDX) == 0 &&
            count_idx(&disabled, hsb_left + 1, hsb_top + 1, hsb_left_sep,
                      hsb_bottom, FG_SB_DISABLED_ARROW_IDX) == 20 &&
            FG_SB_DISABLED_NO_THUMB &&
            count_idx(&disabled, sb_left + 1, sb_sep_y + 1, sb_right,
                      sb_bottom_sep_y, FG_SB_ENABLED_THUMB_TEAL_IDX) == 0 &&
            count_idx(&disabled, hsb_left_sep + 1, hsb_top + 1, hsb_right_sep,
                      hsb_bottom, FG_SB_ENABLED_THUMB_TEAL_IDX) == 0 &&
            count_idx(&disabled, sb_left + 1, sb_sep_y + 1, sb_right,
                      sb_bottom_sep_y, FG_SB_ENABLED_WELL_IDX) == 0;
        render_ctx_free(&disabled);
    }
    CHECK(sb_disabled_ok,
          "leg SCROLL-DISABLED: an active window with nothing to scroll has "
          "flat F3 bars, #777777 separators, #A5A5A5 arrows and NO thumb "
          "(scrollbars.md Sec 3)");

    /* SCROLL-PRESSED (tdnl.35): a held down arrow is drawn PRESSED -- the
     * authored art (chrome.c banner; scrollbars.md Sec 6 golden gap): the
     * tile face is no longer the idle E7 face and the glyph stays black
     * (20 px). The up arrow stays idle. */
    render_ctx_t pressed;
    int sb_pressed_ok = 0;
    if (render_ctx_init(&pressed, &boot) == 0) {
        render_run(&pressed, draw_pressed);
        sb_pressed_ok =
            px(&pressed, sb_mid_x, sb_bottom_sep_y + 2) !=
                FG_SB_ENABLED_TILE_FACE_IDX &&
            px(&pressed, sb_mid_x, sb_bottom_sep_y + 2) ==
                FG_SB_DISABLED_ARROW_IDX &&
            count_idx(&pressed, sb_left + 1, sb_bottom_sep_y + 1, sb_right,
                      (bi - 19) - 1, FG_SB_ENABLED_GLYPH_IDX) == 20 &&
            px(&pressed, sb_mid_x, sb_top + 2) == FG_SB_ENABLED_TILE_FACE_IDX;
        render_ctx_free(&pressed);
    }
    CHECK(sb_pressed_ok,
          "leg SCROLL-PRESSED: a held arrow draws its pressed tile, the other "
          "arrow stays idle (authored; scrollbars.md Sec 6 gap)");


    /* CAPTURE LEGS (tdnl.51 / tdnl.52) -- see the banner above main. */
    int cap_body_ok = 0, cap_body_inactive_ok = 0, cap_widgets_ok = 0;
    {
        char pa[1024], pi[1024];
        capture_img_t ca, ci;
        render_ctx_t sa, si;
        const cap_frame_t of = { WIN_LEFT, WIN_TOP, WIN_RIGHT - 1, WIN_BOTTOM - 1 };
        const cap_frame_t cfa = { 65, 70, 480, 315 };
        const cap_frame_t cfi = { 13, 29, 325, 198 };
        snprintf(pa, sizeof pa, "%s", argc > 1 ? argv[1]
                 : CHROME_FID_CAPTURE_DIR "/s8_doc_window_active.png");
        snprintf(pi, sizeof pi, "%s", argc > 2 ? argv[2]
                 : CHROME_FID_CAPTURE_DIR "/s8_doc_window_inactive.png");
        int have_a = capture_load(&ca, pa);
        int have_i = capture_load(&ci, pi);
        CHECK(have_a && have_i,
              "leg CAPTURE-LOAD: both independent s8_doc_window captures must decode "
              "(640x480) -- a missing golden is RED, never skipped");
        if (have_a && have_i && render_ctx_init(&sa, &boot) == 0) {
            render_run(&sa, draw_capture_active);
            int ok = 1;
            ok &= cap_section("CAPTURE-BODY", "left", &sa, of, &ca, cfa, 0, 80, 7);
            ok &= cap_section("CAPTURE-BODY", "right", &sa, of, &ca, cfa, 1, 80, 21);
            ok &= cap_section("CAPTURE-BODY", "bottom", &sa, of, &ca, cfa, 2, 55, 21);
            ok &= cap_section("CAPTURE-BODY", "top", &sa, of, &ca, cfa, 3, 55, 22);
            cap_body_ok = ok;
            int wok = 1;
            wok &= cap_widget("close", &sa, of.l + 4, of.t + 4, &ca, cfa.l + 4, cfa.t + 4);
            wok &= cap_widget("zoom", &sa, of.r - 32, of.t + 4, &ca, cfa.r - 32, cfa.t + 4);
            wok &= cap_widget("collapse", &sa, of.r - 16, of.t + 4, &ca, cfa.r - 16, cfa.t + 4);
            cap_widgets_ok = wok;
            render_ctx_free(&sa);
        }
        if (have_a && have_i && render_ctx_init(&si, &boot) == 0) {
            render_run(&si, draw_capture_inactive);
            int ok = 1;
            ok &= cap_section("CAPTURE-BODY-INACTIVE", "left", &si, of, &ci, cfi, 0, 91, 7);
            ok &= cap_section("CAPTURE-BODY-INACTIVE", "right", &si, of, &ci, cfi, 1, 91, 21);
            ok &= cap_section("CAPTURE-BODY-INACTIVE", "bottom", &si, of, &ci, cfi, 2, 200, 21);
            ok &= cap_section("CAPTURE-BODY-INACTIVE", "top", &si, of, &ci, cfi, 3, 200, 22);
            cap_body_inactive_ok = ok;
            render_ctx_free(&si);
        }
        free(ca.rgb);
        free(ci.rgb);
    }
    CHECK(cap_body_ok,
          "leg CAPTURE-BODY: the ACTIVE window, after its owner fills the content "
          "rect, must reproduce s8_doc_window_active's body frame on all four edges "
          "(black, white, DA, DA, B3, black, then the inset) and its title band, "
          "pixel for pixel (window-chrome.md Sec 2.1/4; bead tdnl.51)");
    CHECK(cap_body_inactive_ok,
          "leg CAPTURE-BODY-INACTIVE: the INACTIVE window, after its owner fills the "
          "content rect, must reproduce s8_doc_window_inactive's flat E7 frame "
          "between 77 lines on all four edges (window-chrome.md Sec 6; tdnl.51)");
    CHECK(cap_widgets_ok,
          "leg CAPTURE-WIDGETS: close/zoom/collapse 13x13 footprints must equal "
          "s8_doc_window_active's, all 169 pixels each, including the #DADADA "
          "off-diagonal interior corners (window-chrome.md Sec 3.2; tdnl.52)");

#define LEG_STATUS(name, ok, why) \
    printf("  %-24s %s -- %s\n", name, (ok) ? "PASS" : "RED", why)
    LEG_STATUS("BAND", band_ok, "22-row profile / 12 light-first stripes / 2+4 face");
    LEG_STATUS("STRIPE-OFFSET", stripe_extent_ok, "dark rows shift +1 at both endpoints");
    LEG_STATUS("TITLE", title_ok, "black centered ink over idx218 gap");
    LEG_STATUS("TITLE-TRUNC", trunc_ok, "whole cells plus the 0xC9 ellipsis");
    LEG_STATUS("SHADOW-NOTCH", shadow_ok, "(+1,+1) black L with 2px notch");
    LEG_STATUS("BODYBAR", body_ok, "four-pixel raised body bar plus inset");
    LEG_STATUS("WIDGETS", widget_ok, "three 12+1 widgets at Platinum offsets");
    LEG_STATUS("WIDGET-RING", bevel_ring_ok, "closed idx63 ring, never black");
    LEG_STATUS("RAMP", widget_ramp_ok, "seven-rung diagonal widget ramp");
    LEG_STATUS("COLLAPSE", collapse_ok, "rightmost collapse widget and two rows");
    LEG_STATUS("WIDGET-FLAGS", widget_flags_ok, "close+collapse selected; zoom absent");
    LEG_STATUS("GLYPHS", glyph_ok, "close none; zoom right+bottom edges");
    LEG_STATUS("SCROLL-WELL", sb_well_ok, "five-value section + 2px near inset");
    LEG_STATUS("SCROLL-TILE", sb_tile_ok, "white/E7/CD raised arrow tiles");
    LEG_STATUS("SCROLL-SEPARATOR", sb_separator_ok, "active separators black");
    LEG_STATUS("SCROLL-THUMB", sb_thumb_ok, "15px thumb at the value's position");
    LEG_STATUS("SCROLL-DISABLED", sb_disabled_ok, "nothing to scroll: Sec 3, no thumb");
    LEG_STATUS("SCROLL-PRESSED", sb_pressed_ok, "held arrow pressed, other idle");
    LEG_STATUS("GROW", grow_ok, "18x18, three pitched grip lines");
    LEG_STATUS("INACTIVE-TITLE", inactive_title_ok, "flat idx231, not Sys7 white");
    LEG_STATUS("INACTIVE-FRAME", inactive_frame_ok, "all lines/shadow idx119");
    LEG_STATUS("INACTIVE-TEXT", inactive_text_ok, "idx135, not Sys7 idx165");
    LEG_STATUS("INACTIVE-GADGETS", inactive_gadgets_ok, "all three widgets absent");
    LEG_STATUS("HOLLOW/GROW-INACTIVE", hollow_ok, "hollow bar and flat idx231 grow");
    LEG_STATUS("CAPTURE-BODY", cap_body_ok, "live body frame == active capture");
    LEG_STATUS("CAPTURE-BODY-INACTIVE", cap_body_inactive_ok, "live body frame == inactive capture");
    LEG_STATUS("CAPTURE-WIDGETS", cap_widgets_ok, "widget faces == active capture, 169/169");
#undef LEG_STATUS

    if (rc == 0) render_ctx_free(&trunc);
    render_ctx_free(&inactive);
    render_ctx_free(&active);
    return TEST_SUMMARY("test-chrome-fidelity");
}
