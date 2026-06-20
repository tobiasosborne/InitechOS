/*
 * os/milton/test_console.c -- host blit-math oracle for the LFB text console.
 *
 * FACTORY host test (CLAUDE.md Law 3: factory is C, libc OK on the host).
 * Reuses the seed test_assert.h idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) so
 * the binary exits NON-ZERO on any failed check (Law 2 -- never false-green).
 *
 * This is the RED->GREEN gate for os/milton/console.c::console_draw_glyph (the
 * load-bearing pixel math) and the cursor/wrap/scroll logic. The SAME
 * console.c compiles freestanding in the kernel and hosted here; the host
 * allocates a fake framebuffer (640x480) + a fake boot_info pointing at it, and
 * a fake font with hand-defined synthetic glyphs (solid / empty / single-bit).
 *
 * Ref (Law 1): docs/research/boot-to-text-ground-truth.md Sec 1.2 (glyph
 *   format: MSB=0x80=leftmost pixel, set bit = ink); Sec 5 Risk 2 (bpp 32/24
 *   packing; pixel addr = lfb + y*pitch + x*(bpp/8)); os/milton/console.h.
 *
 * Asserted properties:
 *   - solid glyph at (col,row) inks EXACTLY that 8x16 cell to fg, neighbors
 *     stay bg (boundary correctness: no overdraw -- the classic blit bug);
 *   - empty glyph leaves the cell all-bg;
 *   - a single-bit row (0x80) inks ONLY the leftmost pixel (MSB-left
 *     correctness -- the classic flipped-bit bug);
 *   - putc advances the cursor; '\n' -> next row, col 0; wrap at col 80;
 *     scroll at row 25 (content moves up one cell row, last row cleared);
 *   - 24bpp packing inks the right B,G,R triples.
 *
 * ASCII-clean (Rule 12). No timestamps (Rule 11). The fake framebuffer is
 * heap-allocated (host only); console.c itself never allocates.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "test_assert.h"   /* seed/, on -Iseed         */
#include "console.h"       /* os/milton/, on -Ios/milton */

TEST_HARNESS();

#define FB_W 640
#define FB_H 480

/* boot_info addresses are uint32_t (the 32-bit artifact's physical addresses).
 * This host harness may be 64-bit, where a heap/global pointer does NOT fit in
 * uint32_t. Allocate the fake framebuffer/font in the low 4 GiB (MAP_32BIT) so
 * (uint32_t)(uintptr_t)p round-trips losslessly -- mirroring the artifact's
 * world where every LFB/font address is < 4 GiB. Falls back to a low fixed-hint
 * mmap if MAP_32BIT is unavailable (Rule 2 -- abort the test, never false-green
 * on a truncated pointer). */
static void *alloc_low(size_t n)
{
    void *p = MAP_FAILED;
#ifdef MAP_32BIT
    p = mmap(NULL, n, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#endif
    if (p == MAP_FAILED) {
        return NULL;
    }
    if ((uintptr_t)p > 0xFFFFFFFFu) {
        munmap(p, n);
        return NULL;
    }
    return p;
}

/* Synthetic glyph codes in the fake font. */
#define GLYPH_SOLID  'S'   /* sixteen 0xFF rows  -> full 8x16 ink */
#define GLYPH_EMPTY  'E'   /* sixteen 0x00 rows  -> all bg        */
#define GLYPH_LEFT   'L'   /* sixteen 0x80 rows  -> leftmost px   */

/* Build a 4096-byte font with the three synthetic glyphs; everything else 0. */
static void build_font(uint8_t font[CONSOLE_FONT_BYTES])
{
    memset(font, 0, CONSOLE_FONT_BYTES);
    for (int r = 0; r < CONSOLE_CELL_H; r++) {
        font[(unsigned)GLYPH_SOLID * CONSOLE_CELL_H + r] = 0xFF;
        font[(unsigned)GLYPH_EMPTY * CONSOLE_CELL_H + r] = 0x00;
        font[(unsigned)GLYPH_LEFT  * CONSOLE_CELL_H + r] = 0x80;
    }
}

/* Make a boot_info pointing at a host buffer. */
static boot_info_t make_bi(void *fb, uint32_t bpp, void *font)
{
    boot_info_t bi;
    bi.lfb_addr   = (uint32_t)(uintptr_t)fb;
    bi.lfb_bpp    = bpp;
    bi.lfb_pitch  = (uint32_t)FB_W * (bpp / 8u);
    bi.lfb_width  = FB_W;
    bi.lfb_height = FB_H;
    bi.font_addr  = (uint32_t)(uintptr_t)font;
    return bi;
}

/* Read a pixel as canonical 0x00RRGGBB from a fake framebuffer of given bpp. */
static uint32_t fb_get(const uint8_t *fb, uint32_t pitch, uint32_t bpp,
                       uint32_t x, uint32_t y)
{
    uint32_t bpp_bytes = bpp / 8u;
    uint32_t off = y * pitch + x * bpp_bytes;
    if (bpp == 8) {
        return fb[off];          /* mode 0x13: one byte = a palette index */
    }
    if (bpp == 32) {
        uint32_t d;
        memcpy(&d, fb + off, 4);
        return d & 0x00FFFFFFu; /* drop the X byte */
    }
    /* 24bpp: bytes are B,G,R. */
    return ((uint32_t)fb[off + 2] << 16) |
           ((uint32_t)fb[off + 1] << 8)  |
           ((uint32_t)fb[off + 0]);
}

/* Count fg pixels inside cell (col,row); also check no fg leaked into the
 * 1-pixel border immediately around the cell. */
static void check_solid_cell(console_t *con, const uint8_t *fb, uint32_t col,
                             uint32_t row, uint32_t fg, uint32_t bg)
{
    uint32_t x0 = col * CONSOLE_CELL_W;
    uint32_t y0 = row * CONSOLE_CELL_H;
    int inside_fg = 0, inside_bad = 0;

    for (uint32_t dy = 0; dy < CONSOLE_CELL_H; dy++) {
        for (uint32_t dx = 0; dx < CONSOLE_CELL_W; dx++) {
            uint32_t px = fb_get(fb, con->pitch, con->bpp, x0 + dx, y0 + dy);
            if (px == fg) inside_fg++;
            else inside_bad++;
        }
    }
    CHECK(inside_fg == CONSOLE_CELL_W * CONSOLE_CELL_H,
          "solid glyph inks exactly 8x16 = 128 fg pixels in its cell");
    CHECK(inside_bad == 0, "solid glyph cell has no non-fg pixels");

    /* Neighbor cells must remain bg (no overdraw -- the classic blit bug). The
     * pixel just left of the cell, just right, just above, just below. */
    if (x0 >= 1) {
        CHECK(fb_get(fb, con->pitch, con->bpp, x0 - 1, y0) == bg,
              "no overdraw to the left of the cell");
    }
    CHECK(fb_get(fb, con->pitch, con->bpp, x0 + CONSOLE_CELL_W, y0) == bg,
          "no overdraw to the right of the cell");
    if (y0 >= 1) {
        CHECK(fb_get(fb, con->pitch, con->bpp, x0, y0 - 1) == bg,
              "no overdraw above the cell");
    }
    CHECK(fb_get(fb, con->pitch, con->bpp, x0, y0 + CONSOLE_CELL_H) == bg,
          "no overdraw below the cell");
}

int main(void)
{
    uint8_t *font = alloc_low(CONSOLE_FONT_BYTES);
    CHECK(font != NULL, "alloc fake font in low 4 GiB (MAP_32BIT)");
    if (!font) return TEST_SUMMARY("test_console");
    build_font(font);

    /* ===================== 32bpp path ===================== */
    {
        size_t fb_bytes = (size_t)FB_W * FB_H * 4u;
        uint8_t *fb = alloc_low(fb_bytes);
        CHECK(fb != NULL, "alloc 32bpp fake framebuffer in low 4 GiB");
        if (!fb) return TEST_SUMMARY("test_console");

        boot_info_t bi = make_bi(fb, 32, font);
        console_t con;
        int rc = console_init(&con, &bi);
        CHECK(rc == CONSOLE_OK, "console_init OK on a valid 32bpp boot_info");

        uint32_t fg = con.fg, bg = con.bg;

        /* After init the whole framebuffer is bg. */
        CHECK(fb_get(fb, con.pitch, con.bpp, 0, 0) == bg,
              "init clears (0,0) to bg");
        CHECK(fb_get(fb, con.pitch, con.bpp, FB_W - 1, FB_H - 1) == bg,
              "init clears the last pixel to bg");

        /* --- solid glyph boundary correctness at an interior cell --- */
        console_draw_glyph(&con, 3, 2, GLYPH_SOLID, fg, bg);
        check_solid_cell(&con, fb, 3, 2, fg, bg);

        /* --- empty glyph leaves the cell all bg --- */
        console_draw_glyph(&con, 5, 4, GLYPH_EMPTY, fg, bg);
        {
            int allbg = 1;
            for (uint32_t dy = 0; dy < CONSOLE_CELL_H; dy++)
                for (uint32_t dx = 0; dx < CONSOLE_CELL_W; dx++)
                    if (fb_get(fb, con.pitch, con.bpp, 5*CONSOLE_CELL_W+dx,
                               4*CONSOLE_CELL_H+dy) != bg) allbg = 0;
            CHECK(allbg, "empty glyph leaves its cell all bg");
        }

        /* --- single-bit (0x80) row inks ONLY the leftmost pixel (MSB-left) --- */
        console_draw_glyph(&con, 7, 6, GLYPH_LEFT, fg, bg);
        {
            uint32_t x0 = 7 * CONSOLE_CELL_W, y0 = 6 * CONSOLE_CELL_H;
            CHECK(fb_get(fb, con.pitch, con.bpp, x0 + 0, y0) == fg,
                  "0x80 row inks the LEFTMOST pixel (col 0)");
            int rest_bg = 1;
            for (uint32_t dx = 1; dx < CONSOLE_CELL_W; dx++)
                if (fb_get(fb, con.pitch, con.bpp, x0 + dx, y0) != bg) rest_bg = 0;
            CHECK(rest_bg, "0x80 row leaves columns 1..7 bg (MSB-left correctness)");
        }

        /* --- cursor: putc advances col; '\n' -> next row col 0 --- */
        con.cur_col = 0; con.cur_row = 0;
        console_putc(&con, (char)GLYPH_SOLID);
        CHECK(con.cur_col == 1 && con.cur_row == 0, "putc advances cursor col by 1");
        console_putc(&con, '\n');
        CHECK(con.cur_col == 0 && con.cur_row == 1, "'\\n' -> col 0, next row");

        /* --- wrap at col 80 --- */
        con.cur_col = CONSOLE_COLS - 1; con.cur_row = 3;
        console_putc(&con, (char)GLYPH_SOLID);
        CHECK(con.cur_col == 0 && con.cur_row == 4,
              "writing the last column wraps to col 0 of the next row");

        /* --- scroll at the last row: a sentinel cell moves up by one row --- */
        /* Put a solid sentinel at row 24, col 0; then a '\n' on row 24 must
         * scroll, leaving the sentinel content now at row 23. */
        con.cur_col = 0; con.cur_row = CONSOLE_ROWS - 1;     /* row 24 */
        console_putc(&con, (char)GLYPH_SOLID);               /* draw at (0,24) */
        CHECK(con.cur_col == 1 && con.cur_row == CONSOLE_ROWS - 1,
              "putc on last row advances col without scrolling yet");
        /* Verify the sentinel is at row 24 before scroll. */
        CHECK(fb_get(fb, con.pitch, con.bpp, 0, (CONSOLE_ROWS-1)*CONSOLE_CELL_H) == fg,
              "sentinel inked at row 24 cell origin before scroll");
        con.cur_col = 0;
        console_putc(&con, '\n');   /* on last row -> scroll, stay on row 24 */
        CHECK(con.cur_row == CONSOLE_ROWS - 1 && con.cur_col == 0,
              "'\\n' on last row scrolls and stays on the last row");
        /* The sentinel that was at row 24 is now at row 23. */
        CHECK(fb_get(fb, con.pitch, con.bpp, 0, (CONSOLE_ROWS-2)*CONSOLE_CELL_H) == fg,
              "after scroll, the row-24 sentinel content moved up to row 23");
        /* And the last row is now cleared to bg at that pixel. */
        CHECK(fb_get(fb, con.pitch, con.bpp, 0, (CONSOLE_ROWS-1)*CONSOLE_CELL_H) == bg,
              "after scroll, the last text row is cleared to bg");

        munmap(fb, fb_bytes);
    }

    /* ===================== 24bpp path ===================== */
    {
        size_t fb_bytes = (size_t)FB_W * FB_H * 3u;
        uint8_t *fb = alloc_low(fb_bytes);
        CHECK(fb != NULL, "alloc 24bpp fake framebuffer in low 4 GiB");
        if (!fb) return TEST_SUMMARY("test_console");

        boot_info_t bi = make_bi(fb, 24, font);
        console_t con;
        int rc = console_init(&con, &bi);
        CHECK(rc == CONSOLE_OK, "console_init OK on a valid 24bpp boot_info");
        CHECK(con.bytes_per_pixel == 3, "24bpp -> 3 bytes per pixel");

        uint32_t fg = con.fg, bg = con.bg;

        /* Solid glyph inks the right BGR triples in 24bpp. */
        console_draw_glyph(&con, 2, 1, GLYPH_SOLID, fg, bg);
        {
            uint32_t x0 = 2 * CONSOLE_CELL_W, y0 = 1 * CONSOLE_CELL_H;
            uint32_t off = y0 * con.pitch + x0 * 3u;
            /* fg canonical is 0x00RRGGBB; stored bytes must be B,G,R. */
            CHECK(fb[off + 0] == (uint8_t)(fg & 0xFF),       "24bpp B byte == fg blue");
            CHECK(fb[off + 1] == (uint8_t)((fg >> 8) & 0xFF), "24bpp G byte == fg green");
            CHECK(fb[off + 2] == (uint8_t)((fg >> 16) & 0xFF),"24bpp R byte == fg red");
            CHECK(fb_get(fb, con.pitch, con.bpp, x0, y0) == fg,
                  "24bpp solid pixel reads back as fg");
        }
        /* MSB-left in 24bpp too. */
        console_draw_glyph(&con, 4, 3, GLYPH_LEFT, fg, bg);
        {
            uint32_t x0 = 4 * CONSOLE_CELL_W, y0 = 3 * CONSOLE_CELL_H;
            CHECK(fb_get(fb, con.pitch, con.bpp, x0, y0) == fg,
                  "24bpp 0x80 row inks leftmost pixel");
            CHECK(fb_get(fb, con.pitch, con.bpp, x0 + 1, y0) == bg,
                  "24bpp 0x80 row leaves col 1 bg");
        }

        munmap(fb, fb_bytes);
    }

    /* ============ 8bpp mode-0x13 fallback path (320x200) ============ */
    /* The standard-VGA fallback (initech-6pj): a 320x200x256 LINEAR framebuffer
     * where each byte is a palette index. Verifies bpp=8 acceptance, the
     * resolution-derived 40x12 cell grid (vs 80x25 for the 640x480 LFB), index
     * rendering, MSB-left correctness, and scroll -- all on the real fallback
     * geometry so a wrong cols/rows would overrun this exact-size buffer. */
    {
        const uint32_t W8 = 320, H8 = 200;
        size_t fb_bytes = (size_t)W8 * H8 * 1u;
        uint8_t *fb = alloc_low(fb_bytes);
        CHECK(fb != NULL, "alloc 8bpp 320x200 fake framebuffer in low 4 GiB");
        if (!fb) return TEST_SUMMARY("test_console");

        boot_info_t bi;
        bi.lfb_addr   = (uint32_t)(uintptr_t)fb;
        bi.lfb_bpp    = 8;
        bi.lfb_pitch  = W8;            /* 1 byte/pixel */
        bi.lfb_width  = W8;
        bi.lfb_height = H8;
        bi.font_addr  = (uint32_t)(uintptr_t)font;

        console_t con;
        int rc = console_init(&con, &bi);
        CHECK(rc == CONSOLE_OK, "console_init OK on a valid 8bpp boot_info");
        CHECK(con.bytes_per_pixel == 1, "8bpp -> 1 byte per pixel");
        CHECK(con.cols == 40, "320px wide -> 40 cell columns (320/8)");
        CHECK(con.rows == 12, "200px tall -> 12 cell rows (200/16)");
        CHECK(con.fg == CONSOLE_FG_IDX, "8bpp fg bound to CONSOLE_FG_IDX (palette index)");
        CHECK(con.bg == CONSOLE_BG_IDX, "8bpp bg bound to CONSOLE_BG_IDX (palette index)");

        uint32_t fg = con.fg, bg = con.bg;

        /* init clears to the bg index everywhere. */
        CHECK(fb_get(fb, con.pitch, con.bpp, 0, 0) == bg,
              "8bpp init clears (0,0) to bg index");
        CHECK(fb_get(fb, con.pitch, con.bpp, W8 - 1, H8 - 1) == bg,
              "8bpp init clears the last pixel to bg index");

        /* solid glyph at an interior cell inks fg index, no overdraw. */
        console_draw_glyph(&con, 3, 2, GLYPH_SOLID, fg, bg);
        check_solid_cell(&con, fb, 3, 2, fg, bg);

        /* MSB-left: 0x80 row inks only the leftmost pixel. */
        console_draw_glyph(&con, 5, 4, GLYPH_LEFT, fg, bg);
        {
            uint32_t x0 = 5 * CONSOLE_CELL_W, y0 = 4 * CONSOLE_CELL_H;
            CHECK(fb_get(fb, con.pitch, con.bpp, x0, y0) == fg,
                  "8bpp 0x80 row inks leftmost pixel (index)");
            CHECK(fb_get(fb, con.pitch, con.bpp, x0 + 1, y0) == bg,
                  "8bpp 0x80 row leaves col 1 bg (MSB-left)");
        }

        /* scroll: sentinel on the last row moves up one cell row. */
        con.cur_col = 0; con.cur_row = con.rows - 1;
        console_putc(&con, (char)GLYPH_SOLID);
        CHECK(fb_get(fb, con.pitch, con.bpp, 0, (con.rows-1)*CONSOLE_CELL_H) == fg,
              "8bpp sentinel inked at last row before scroll");
        con.cur_col = 0;
        console_putc(&con, '\n');
        CHECK(con.cur_row == con.rows - 1 && con.cur_col == 0,
              "8bpp '\\n' on last row scrolls and stays on the last row");
        CHECK(fb_get(fb, con.pitch, con.bpp, 0, (con.rows-2)*CONSOLE_CELL_H) == fg,
              "8bpp after scroll, last-row sentinel moved up one row");
        CHECK(fb_get(fb, con.pitch, con.bpp, 0, (con.rows-1)*CONSOLE_CELL_H) == bg,
              "8bpp after scroll, last text row cleared to bg index");

        munmap(fb, fb_bytes);
    }

    /* ===================== fail-loud init guards ===================== */
    {
        /* A valid-looking bi whose error fields we mutate; lfb_addr/font_addr
         * point at the low-memory font buffer (only the guard fields matter --
         * each bad case returns before any framebuffer write). */
        boot_info_t bi = make_bi(font, 32, font);
        console_t con;

        CHECK(console_init(NULL, &bi) == CONSOLE_ERR_NULL, "NULL console -> ERR_NULL");
        CHECK(console_init(&con, NULL) == CONSOLE_ERR_NULL, "NULL boot_info -> ERR_NULL");

        boot_info_t bad = bi; bad.lfb_addr = 0;
        CHECK(console_init(&con, &bad) == CONSOLE_ERR_ADDR, "lfb_addr 0 -> ERR_ADDR");

        bad = bi; bad.lfb_bpp = 16;
        CHECK(console_init(&con, &bad) == CONSOLE_ERR_BPP, "bpp 16 -> ERR_BPP");

        bad = bi; bad.font_addr = 0;
        CHECK(console_init(&con, &bad) == CONSOLE_ERR_FONT, "font_addr 0 -> ERR_FONT");

        /* bcg.11: degenerate geometry must be rejected, not silently accepted.
         * (Each returns before any framebuffer write, so fb=font is safe here.) */
        bad = bi; bad.lfb_width = 0;
        CHECK(console_init(&con, &bad) == CONSOLE_ERR_GEOMETRY, "width 0 -> ERR_GEOMETRY");
        bad = bi; bad.lfb_height = 0;
        CHECK(console_init(&con, &bad) == CONSOLE_ERR_GEOMETRY, "height 0 -> ERR_GEOMETRY");
        bad = bi; bad.lfb_pitch = 0;
        CHECK(console_init(&con, &bad) == CONSOLE_ERR_GEOMETRY, "pitch 0 -> ERR_GEOMETRY");
        bad = bi; bad.lfb_pitch = bi.lfb_width * (bi.lfb_bpp / 8u) - 1u;
        CHECK(console_init(&con, &bad) == CONSOLE_ERR_GEOMETRY,
              "pitch < width*bpp -> ERR_GEOMETRY");
    }

    /* ===================== padded-pitch path (bcg.11) ===================== *
     * Bochs/86Box modes can give a pitch LARGER than width*bpp (scanline
     * padding). The blit must use `pitch`, not width*bpp, for the row stride.
     * Build a 32bpp fb with 64 padding bytes per scanline; assert a glyph lands
     * on the correct scanline and the padding gap stays untouched. */
    {
        uint32_t bpp = 32u, bpx = bpp / 8u;
        uint32_t pad = 64u;
        uint32_t pitch = (uint32_t)FB_W * bpx + pad;
        size_t fb_bytes = (size_t)pitch * FB_H;
        uint8_t *fb = alloc_low(fb_bytes);
        CHECK(fb != NULL, "alloc padded-pitch framebuffer");
        if (fb) {
            boot_info_t bi;
            bi.lfb_addr   = (uint32_t)(uintptr_t)fb;
            bi.lfb_bpp    = bpp;
            bi.lfb_pitch  = pitch;
            bi.lfb_width  = FB_W;
            bi.lfb_height = FB_H;
            bi.font_addr  = (uint32_t)(uintptr_t)font;

            console_t con;
            int rc = console_init(&con, &bi);
            CHECK(rc == CONSOLE_OK, "console_init OK on a padded-pitch boot_info");
            CHECK(con.pitch == pitch, "console honors the padded pitch (not width*bpp)");

            uint32_t fg = con.fg, bg = con.bg;
            /* fb_get inside check_solid_cell uses con.pitch -- a width*bpp stride
             * in console.c would put the glyph on the wrong scanline and fail. */
            console_draw_glyph(&con, 3, 2, GLYPH_SOLID, fg, bg);
            check_solid_cell(&con, fb, 3, 2, fg, bg);

            /* The padding past `width` on a glyph scanline must be untouched
             * (still zero): a stride bug would scribble bg/ink into it. */
            {
                uint32_t row = 2u * CONSOLE_CELL_H;          /* a glyph scanline */
                size_t   pad_off = (size_t)row * pitch + (size_t)FB_W * bpx;
                int      pad_clean = 1;
                for (uint32_t i = 0; i < pad; i++)
                    if (fb[pad_off + i] != 0) pad_clean = 0;
                CHECK(pad_clean, "scanline padding past width is left untouched");
            }
            munmap(fb, fb_bytes);
        }
    }

    /* ============ ANSI.SYS console extensions (beads initech-p96i) ============
     * console_set_attr / console_set_cursor / console_get_cursor /
     * console_erase_line / console_erase_display, and the LOAD-BEARING claim:
     * the cur_fg/cur_bg DEFAULT path is pixel-identical to drawing with fg/bg
     * (CLAUDE.md Law 4 -- the no-ANSI rendering must not change). 32bpp path. */
    {
        size_t fb_bytes = (size_t)FB_W * FB_H * 4u;
        uint8_t *fb = alloc_low(fb_bytes);
        CHECK(fb != NULL, "alloc 32bpp fb for ANSI console-extension tests");
        if (fb) {
            boot_info_t bi = make_bi(fb, 32, font);
            console_t con;
            int rc = console_init(&con, &bi);
            CHECK(rc == CONSOLE_OK, "console_init OK (ANSI-ext block)");

            uint32_t fg = con.fg, bg = con.bg;

            /* --- default-attribute pixel identity --- */
            CHECK(con.cur_fg == con.fg && con.cur_bg == con.bg,
                  "init: cur_fg/cur_bg default EQUAL to fg/bg (Law 4 pixel-identity)");
            /* A glyph drawn via console_putc (which uses cur_fg/cur_bg) is the
             * SAME pixels as one drawn with fg/bg explicitly. */
            con.cur_row = 1; con.cur_col = 1;
            console_putc(&con, (char)GLYPH_SOLID);
            check_solid_cell(&con, fb, 1, 1, fg, bg);

            /* --- console_set_cursor clamps to the grid --- */
            console_set_cursor(&con, 4, 9);
            CHECK(con.cur_row == 4 && con.cur_col == 9, "set_cursor lands at (4,9)");
            console_set_cursor(&con, 9999, 9999);
            CHECK(con.cur_row == con.rows - 1 && con.cur_col == con.cols - 1,
                  "set_cursor clamps an out-of-range target to the last cell");

            /* --- console_get_cursor reads it back --- */
            {
                uint32_t r = 99, c = 99;
                console_set_cursor(&con, 7, 3);
                console_get_cursor(&con, &r, &c);
                CHECK(r == 7 && c == 3, "get_cursor reads back the set position");
            }

            /* --- console_set_attr: default 0x07 maps back to fg/bg (seafoam) --- */
            console_set_attr(&con, 0x47u);   /* some non-default attr first */
            console_set_attr(&con, 0x07u);   /* ANSI default / ESC[0m */
            CHECK(con.cur_fg == con.fg && con.cur_bg == con.bg,
                  "set_attr(0x07) restores the console default ink/paper (seafoam-safe)");

            /* --- console_set_attr: bright red (CGA attr 0x0C) gives a red ink --- */
            console_set_attr(&con, 0x0Cu);   /* fg nibble 0x0C = bright red */
            {
                uint32_t want = console_pack_rgb(con.bpp, 0xFF, 0x55, 0x55); /* CGA 12 */
                CHECK(con.cur_fg == want,
                      "set_attr(0x0C) sets cur_fg to bright-red (CGA palette 12)");
            }
            /* A glyph now draws in bright red, not the default ink. */
            console_set_cursor(&con, 6, 6);
            console_putc(&con, (char)GLYPH_SOLID);
            {
                uint32_t red = console_pack_rgb(con.bpp, 0xFF, 0x55, 0x55);
                CHECK(fb_get(fb, con.pitch, con.bpp, 6*CONSOLE_CELL_W, 6*CONSOLE_CELL_H) == red,
                      "a glyph drawn after set_attr(0x0C) is inked bright red");
            }
            console_set_attr(&con, 0x07u);   /* back to default for the erase tests */

            /* --- console_erase_line mode 2: whole current line -> cur_bg --- */
            console_set_cursor(&con, 2, 0);
            for (uint32_t c = 0; c < 10; c++) console_putc(&con, (char)GLYPH_SOLID);
            console_set_cursor(&con, 2, 0);
            console_erase_line(&con, 2);
            {
                int all_bg = 1;
                for (uint32_t c = 0; c < 10; c++)
                    if (fb_get(fb, con.pitch, con.bpp, c*CONSOLE_CELL_W, 2*CONSOLE_CELL_H) != bg)
                        all_bg = 0;
                CHECK(all_bg, "erase_line(2) clears the whole current row to bg");
            }

            /* --- console_erase_line mode 0: cursor..end-of-line --- */
            console_set_cursor(&con, 3, 0);
            for (uint32_t c = 0; c < 6; c++) console_putc(&con, (char)GLYPH_SOLID);
            console_set_cursor(&con, 3, 3);   /* erase from col 3 to end */
            console_erase_line(&con, 0);
            CHECK(fb_get(fb, con.pitch, con.bpp, 2*CONSOLE_CELL_W, 3*CONSOLE_CELL_H) == fg,
                  "erase_line(0) leaves cells BEFORE the cursor (col 2) inked");
            CHECK(fb_get(fb, con.pitch, con.bpp, 4*CONSOLE_CELL_W, 3*CONSOLE_CELL_H) == bg,
                  "erase_line(0) clears cells FROM the cursor (col 4) to end");

            /* --- console_erase_display mode 2: whole screen -> cur_bg --- */
            console_set_cursor(&con, 5, 5);
            console_putc(&con, (char)GLYPH_SOLID);
            console_erase_display(&con, 2);
            CHECK(fb_get(fb, con.pitch, con.bpp, 5*CONSOLE_CELL_W, 5*CONSOLE_CELL_H) == bg,
                  "erase_display(2) clears the whole screen to bg");

            /* --- erase fills with the CURRENT paper, not the default --- *
             * Set a bright-blue background (CGA attr bg nibble 1 -> 0x10) and
             * erase: the cleared cells must be bright-blue, proving erase honours
             * cur_bg (the ANSI "erase with current attribute" contract). */
            console_set_attr(&con, 0x10u);    /* bg nibble 1 = blue; fg nibble 0 */
            console_set_cursor(&con, 8, 0);
            console_erase_line(&con, 2);
            {
                uint32_t blue = console_pack_rgb(con.bpp, 0x00, 0x00, 0xAA); /* CGA 1 */
                CHECK(fb_get(fb, con.pitch, con.bpp, 0, 8*CONSOLE_CELL_H) == blue,
                      "erase fills with the CURRENT paper (cur_bg), not the default bg");
            }

            munmap(fb, fb_bytes);
        }
    }

    munmap(font, CONSOLE_FONT_BYTES);
    return TEST_SUMMARY("test_console");
}
