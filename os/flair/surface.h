/* os/flair/surface.h -- the ONE pixel-write module for InitechOS (FLAIR Layer 1).
 *
 * beads: initech-k8o5. Ref: ADR-0004 D-2 ("exactly one surface module; the single
 *        pixel path; no second pixel writer anywhere in the OS"); ADR-0004 D-2 OD-2
 *        (indexed-8 offscreen: 1 byte/pixel = palette index); ADR-0004 FO-1/AM-2
 *        (fb-agree gate must be GREEN and MUTATION-PROVEN before any Manager code).
 * Ref:   PRD Sec 5 (hardware contract: VBE 2.0 LFB, 8/24/32bpp, pitch >= width*bpp/8);
 *        docs/research/boot-to-text-ground-truth.md Sec 5 Risk 2 (branch on bpp;
 *        pixel addr = base + y*pitch + x*(bpp/8); 32bpp XRGB8888 little-endian
 *        B,G,R,X; 24bpp B,G,R bytes; 8bpp = VGA palette index).
 *        CLAUDE.md Law 1 (ground truth before code), Law 2 (oracle is truth),
 *        Law 3 (freestanding artifact; dual-compile for host oracle),
 *        Rule 2 (fail loud -- no write past the buffer; bounds-checked),
 *        Rule 11 (reproducible), Rule 12 (ASCII-clean source).
 *
 * ARTIFACT code: freestanding, <stdint.h> only, no libc, no malloc. Compiles BOTH
 * under kernel flags (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra
 * -Werror) AND hosted for the fb-agree oracle (harness/proptest/test_fbagree.c).
 *
 * ONE-SURFACE INVARIANT (ADR-0004 D-2): every pixel write in InitechOS goes
 * through surface_put_pixel. console.c is a CLIENT of this module; no second
 * pixel path exists. The fb-agree oracle enforces this at build time: it renders
 * identical content via both the console API and the surface primitives directly
 * and asserts byte-identical framebuffer state.
 *
 * Pixel format (canonical 0x00RRGGBB packed value throughout this module):
 *   32bpp -- XRGB8888 dword, little-endian; stored bytes B,G,R,X.
 *   24bpp -- low 24 bits 0xRRGGBB; stored bytes B,G,R (no padding per pixel).
 *   8bpp  -- low 8 bits = VGA palette index (mode 0x13 fallback).
 * This is the same representation used by console_pack_rgb / console_color_t.
 */
#ifndef INITECH_OS_FLAIR_SURFACE_H
#define INITECH_OS_FLAIR_SURFACE_H

#include <stdint.h>

/* --------------------------------------------------------------------------
 * bitmap_t -- describes a drawable surface (ADR-0004 D-2).
 *
 * The destination is EITHER the LFB (from boot_info) OR an indexed-8
 * offscreen buffer (OD-2: 1 byte/pixel = palette index, bpp==8).
 * Caller supplies all storage; this module never allocates.
 * -------------------------------------------------------------------------- */
typedef struct {
    volatile uint8_t *base;      /* framebuffer base (LFB or offscreen buf)   */
    uint32_t          pitch;     /* bytes per scanline (>= width * bpp/8)      */
    uint32_t          bpp;       /* 8, 24, or 32 (validated by caller)         */
    uint32_t          bytes_per_pixel; /* bpp / 8: 1, 3, or 4                 */
    uint32_t          width;     /* width in pixels                            */
    uint32_t          height;    /* height in pixels                           */
} bitmap_t;

/* --------------------------------------------------------------------------
 * surface_pack_rgb -- pack (r,g,b) as canonical 0x00RRGGBB.
 *
 * The bpp argument is ignored (the canonical representation is always
 * 0x00RRGGBB; the blit branches on bpp when writing). Matches
 * console_pack_rgb semantics exactly (ADR-0004 D-2).
 * -------------------------------------------------------------------------- */
static inline uint32_t surface_pack_rgb(uint32_t bpp,
                                        uint8_t r, uint8_t g, uint8_t b)
{
    (void)bpp;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

/* --------------------------------------------------------------------------
 * surface_put_pixel -- THE single low-level pixel writer (ADR-0004 D-2).
 *
 * Write packed value `px` (canonical 0x00RRGGBB) at byte offset `off`
 * into bm->base, branching on bm->bpp. The caller must supply a valid
 * offset (already bounds-checked against pitch * height). No write past
 * the buffer (Rule 2 -- overdraw invariant).
 *
 *   8bpp:  one byte  = low 8 bits of px (palette index).
 *   32bpp: one dword = px as XRGB8888 little-endian (B,G,R,X bytes).
 *   24bpp: three bytes B,G,R (low 24 bits of 0x00RRGGBB).
 * -------------------------------------------------------------------------- */
static inline void surface_put_pixel(const bitmap_t *bm,
                                     uint32_t off, uint32_t px)
{
    if (bm->bpp == 8) {
        bm->base[off] = (uint8_t)(px & 0xFFu);
    } else if (bm->bpp == 32) {
        *(volatile uint32_t *)(bm->base + off) = px;
    } else {
        /* 24bpp: B,G,R at increasing addresses (PRD Sec 5 Risk 2). */
        bm->base[off + 0u] = (uint8_t)(px & 0xFFu);          /* B */
        bm->base[off + 1u] = (uint8_t)((px >> 8) & 0xFFu);   /* G */
        bm->base[off + 2u] = (uint8_t)((px >> 16) & 0xFFu);  /* R */
    }
}

/* --------------------------------------------------------------------------
 * surface_get_pixel -- read one pixel at byte offset `off` as 0x00RRGGBB.
 *
 * Used by the fb-agree oracle and scroll (copy pixel runs). Symmetric
 * inverse of surface_put_pixel.
 * -------------------------------------------------------------------------- */
static inline uint32_t surface_get_pixel(const bitmap_t *bm, uint32_t off)
{
    if (bm->bpp == 8) {
        return (uint32_t)bm->base[off];
    } else if (bm->bpp == 32) {
        return *(volatile uint32_t *)(bm->base + off);
    } else {
        return ((uint32_t)bm->base[off + 2u] << 16) |
               ((uint32_t)bm->base[off + 1u] << 8)  |
               ((uint32_t)bm->base[off + 0u]);
    }
}

/* surface_fill_span -- fill a horizontal run of `len` pixels starting at
 * (x,y) with packed color `px`. Bounds-checked: clips to the bitmap width
 * so no write ever reaches past the end of the scanline (Rule 2). */
void surface_fill_span(const bitmap_t *bm,
                       uint32_t x, uint32_t y,
                       uint32_t len, uint32_t px);

/* surface_blit -- blit an 8-pixel-wide glyph (font row bitmap) into `bm`.
 *
 * `glyph` points to CELL_H rows of 1-byte-per-row data (MSB = leftmost
 * pixel, set bit = ink). The cell is at pixel origin (x0, y0). Ink pixels
 * receive `fg`; background pixels receive `bg`. Bounds-checked: cells whose
 * pixel extent exceeds the bitmap are silently clipped (Rule 2).
 *
 * This is the load-bearing primitive lifted from console_draw_glyph. The
 * fb-agree oracle compares console_draw_glyph's output to an independent
 * call to this function and asserts byte-identical results (ADR-0004 AM-2).
 */
void surface_blit(const bitmap_t *bm,
                  uint32_t x0, uint32_t y0,
                  const uint8_t *glyph,
                  uint32_t cell_w, uint32_t cell_h,
                  uint32_t fg, uint32_t bg);

#endif /* INITECH_OS_FLAIR_SURFACE_H */
