/*
 * os/flair/cursor.c -- 16x16 classic Mac arrow with save-under + ShieldCursor.
 *
 * beads: initech-tdnl.1 (GUI remediation R0.1).
 * Ref: Inside Macintosh: Imaging With QuickDraw, cursor ('CURS') resource:
 *      sixteen 16-bit data rows, sixteen 16-bit mask rows, MSB-left, followed
 *      by a hotspot. The strike below is CLEAN-ROOM hand-authored per
 *      CLAUDE.md's glyph rule; it is not extracted or copied from a binary.
 * Ref: PRD Sec 6.3; docs/plans/GUI-remediation-plan.md R0.1; surface.h
 *      (8/24/32bpp final-surface format and the sole pixel read/write seam).
 *
 * CURS compositing: mask=0 preserves the destination; mask=1,data=1 writes
 * black; mask=1,data=0 writes white. The mask is a one-pixel white dilation of
 * the black arrow body. Hotspot = arrow tip at (x=1,y=1).
 *
 * Artifact C per ADR-0002: freestanding, no allocation/libc, deterministic,
 * ASCII-only. The two mutation knobs are host-oracle builds only (Rule 6).
 */
#include "cursor.h"
#include "surface.h"
#include "cursors.h"   /* spec/assets LOCKED FLAIR_CURSOR_ARROW v2 (Rule-8
                        * amendment beads initech-tdnl.25): the strike + mask +
                        * hotspot are the locked spec-data, consumed VERBATIM
                        * (Law 1). test_cursor.c pins the exact bytes. */

enum { CURSOR_DIM = 16, CURSOR_SAVE_PIXELS = 16 * 16 };

#define CURSOR_BLACK_RGB  0x00000000u
#define CURSOR_WHITE_RGB  0x00FFFFFFu
#define CURSOR_BLACK_IDX  0u
#define CURSOR_WHITE_IDX  1u

#define cursor_data (FLAIR_CURSOR_ARROW.data)
#define cursor_mask (FLAIR_CURSOR_ARROW.mask)

#if defined(CURSOR_MUT_HOTSPOT)
/* Rule-6 knob: +3,+3 off the locked hotspot. */
#define CURSOR_HOT_X ((int)FLAIR_CURSOR_ARROW.hot_col + 3)
#define CURSOR_HOT_Y ((int)FLAIR_CURSOR_ARROW.hot_row + 3)
#else
#define CURSOR_HOT_X ((int)FLAIR_CURSOR_ARROW.hot_col)
#define CURSOR_HOT_Y ((int)FLAIR_CURSOR_ARROW.hot_row)
#endif

typedef struct flair_cursor_state {
    bitmap_t surface;
    uint32_t save[CURSOR_SAVE_PIXELS];
    uint32_t shield_depth;
    int x;
    int y;
    int origin_x;
    int origin_y;
    int bound;
    int visible; /* logical visibility request */
    int drawn;   /* save-under currently owns pixels on the bound surface */
} flair_cursor_state_t;

static flair_cursor_state_t g_cursor;

static uint32_t cursor_color(int white)
{
    if (g_cursor.surface.bpp == 8u) {
        return white ? CURSOR_WHITE_IDX : CURSOR_BLACK_IDX;
    }
    return white ? CURSOR_WHITE_RGB : CURSOR_BLACK_RGB;
}

/* Restore exactly the clipped visible sub-rectangle saved by cursor_draw. */
static void cursor_erase(void)
{
    int row;
    int col;

    if (!g_cursor.bound || !g_cursor.drawn) return;
    for (row = 0; row < CURSOR_DIM; row++) {
        int py = g_cursor.origin_y + row;
        if (py < 0 || py >= (int)g_cursor.surface.height) continue;
        for (col = 0; col < CURSOR_DIM; col++) {
            int px = g_cursor.origin_x + col;
            uint32_t off;
            if (px < 0 || px >= (int)g_cursor.surface.width) continue;
            off = (uint32_t)py * g_cursor.surface.pitch +
                  (uint32_t)px * g_cursor.surface.bytes_per_pixel;
            surface_put_pixel(&g_cursor.surface, off,
                              g_cursor.save[row * CURSOR_DIM + col]);
        }
    }
    g_cursor.drawn = 0;
}

/* Save the whole clipped 16x16 rectangle, then apply the CURS mask. Saving
 * transparent cells too makes erase a simple exact rectangle restoration. */
static void cursor_draw(void)
{
    int row;
    int col;
    int ox;
    int oy;

    if (!g_cursor.bound || !g_cursor.visible ||
        g_cursor.shield_depth != 0u || g_cursor.drawn) {
        return;
    }

    ox = g_cursor.x - CURSOR_HOT_X;
    oy = g_cursor.y - CURSOR_HOT_Y;
    for (row = 0; row < CURSOR_DIM; row++) {
        int py = oy + row;
        uint16_t data_row = cursor_data[row];
        uint16_t mask_row = cursor_mask[row];
        if (py < 0 || py >= (int)g_cursor.surface.height) continue;
        for (col = 0; col < CURSOR_DIM; col++) {
            int px = ox + col;
            uint16_t bit;
            uint32_t off;
            if (px < 0 || px >= (int)g_cursor.surface.width) continue;
            off = (uint32_t)py * g_cursor.surface.pitch +
                  (uint32_t)px * g_cursor.surface.bytes_per_pixel;
            g_cursor.save[row * CURSOR_DIM + col] =
                surface_get_pixel(&g_cursor.surface, off);
            bit = (uint16_t)(0x8000u >> col);
            if (mask_row & bit) {
                surface_put_pixel(&g_cursor.surface, off,
                                  cursor_color(!(data_row & bit)));
            }
        }
    }
    g_cursor.origin_x = ox;
    g_cursor.origin_y = oy;
    g_cursor.drawn = 1;
}

void flair_cursor_init(void *fb, uint32_t pitch,
                       uint32_t w, uint32_t h, uint32_t bpp)
{
    uint32_t bytes = bpp / 8u;

    g_cursor.surface.base = (volatile uint8_t *)fb;
    g_cursor.surface.pitch = pitch;
    g_cursor.surface.bpp = bpp;
    g_cursor.surface.bytes_per_pixel = bytes;
    g_cursor.surface.width = w;
    g_cursor.surface.height = h;
    g_cursor.shield_depth = 0u;
    g_cursor.x = 0;
    g_cursor.y = 0;
    g_cursor.origin_x = 0;
    g_cursor.origin_y = 0;
    g_cursor.visible = 0;
    g_cursor.drawn = 0;
    g_cursor.bound = (fb != (void *)0 && w != 0u && h != 0u &&
                      (bpp == 8u || bpp == 24u || bpp == 32u) &&
                      pitch >= w * bytes) ? 1 : 0;
}

void flair_cursor_show(int x, int y)
{
    if (g_cursor.visible) {
        flair_cursor_move(x, y);
        return;
    }
    g_cursor.x = x;
    g_cursor.y = y;
    g_cursor.visible = 1;
    cursor_draw();
}

void flair_cursor_hide(void)
{
    cursor_erase();
    g_cursor.visible = 0;
}

void flair_cursor_move(int x, int y)
{
    if (x == g_cursor.x && y == g_cursor.y) return;
    if (!g_cursor.visible) {
        g_cursor.x = x;
        g_cursor.y = y;
        return;
    }

#if !defined(CURSOR_MUT_NO_ERASE)
    cursor_erase();
#else
    /* Rule-6 host mutant: abandon the old save-under, leaving a visible trail. */
    g_cursor.drawn = 0;
#endif
    g_cursor.x = x;
    g_cursor.y = y;
    cursor_draw();
}

void flair_cursor_shield(void)
{
    if (g_cursor.shield_depth == 0u) cursor_erase();
    if (g_cursor.shield_depth != 0xFFFFFFFFu) g_cursor.shield_depth++;
}

void flair_cursor_unshield(void)
{
    if (g_cursor.shield_depth == 0u) return;
    g_cursor.shield_depth--;
    if (g_cursor.shield_depth == 0u) cursor_draw();
}
