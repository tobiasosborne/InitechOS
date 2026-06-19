/*
 * harness/render/render.c -- the FLAIR host-render skeleton (FACTORY, host-only).
 *
 * beads: initech-k8o5.7. See render.h for the full contract + Law-3 separation.
 *
 * This is FACTORY code (libc allowed). It builds a render context from
 * caller-supplied LFB geometry (a fake boot_info -- AM-1/FO-C: NO hardcoded
 * aperture), backs an offscreen bitmap_t + region storage from a FLAIR arena,
 * opens a GrafPort over the offscreen, runs an artifact draw program against it,
 * and dumps a deterministic P6 PPM (Rule 11). It supports 8bpp (indexed-8, the
 * canonical OD-2 depth) and 32bpp (direct XRGB8888) -- the two modes k8o5.7 names.
 *
 * Ref: ADR-0004 D-1/D-2/D-8, AM-1/FO-C; spec/grafport.h, spec/imaging.h,
 *      os/flair/surface.h, os/flair/heap.h, spec/region_algebra.h,
 *      spec/assets/palette.h. CLAUDE.md Law 2, Law 3, Rule 11, Rule 12.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "render.h"
#include "palette.h"           /* INITECH_*_RGB sampled colors (-Ispec/assets) */

/* RENDER_DESKTOP_INDEX (the blank-canvas desktop gray, NOT 0/black) is declared
 * in render.h so the oracle can distinguish painted chrome from the bare desktop.
 * It matches render_palette_rgb index 2 and chrome.c CIDX_DESKTOP. */

/* ---------------------------------------------------------------------------
 * The deterministic indexed-8 render palette.
 *
 * 8bpp offscreens store palette indices; the PPM and the RGB accessor map them
 * through this fixed ramp. It is byte-stable (Rule 11). Named indices:
 *
 *   0  black            (frame lines / ink)
 *   1  window white     (body fill; spec/assets/palette.h WINDOW_WHITE)
 *   2  desktop gray     (DESKTOP_BG)
 *   3  menubar gray     (MENUBAR_BG)
 *   4  titlebar ink     (TITLEBAR_INK; dark frame/ink)
 *   5  accent blue      (ACCENT_BLUE)
 *   6  light gray       (scrollbar track / control face)
 *   7  pinstripe LIGHT  (wTitleBarLight, WDEF index 7 -- used verbatim as the
 *                        indexed-8 light pinstripe shade; the RGB is golden-
 *                        resolves so we choose a plausible light gray here, but
 *                        the INDEX 7 is exact and is what the oracle checks)
 *   8  pinstripe DARK   (wTitleBarDark,  WDEF index 8 -- the dark pinstripe)
 *
 * The pinstripe shade INDICES (7/8) are the WDEF constants; their exact 8-bpp
 * RGBs are golden-resolves (gui-ground-truth.md Sec 3.3), so the RGB values here
 * are plausible grays chosen to read correctly to a human -- the oracle asserts
 * INDEX alternation (period 2), not the exact RGB. Higher indices fall back to a
 * gray ramp so the palette is total for all 256 indices.
 * ------------------------------------------------------------------------- */
uint32_t render_palette_rgb(uint8_t index)
{
    switch (index) {
    case 0: return 0x000000u;                        /* black                  */
    case 1: return INITECH_WINDOW_WHITE_RGB;         /* body white             */
    case 2: return INITECH_DESKTOP_BG_RGB;           /* desktop gray           */
    case 3: return INITECH_MENUBAR_BG_RGB;           /* menubar gray           */
    case 4: return INITECH_TITLEBAR_INK_RGB;         /* title ink / dark frame */
    case 5: return INITECH_ACCENT_BLUE_RGB;          /* accent blue            */
    case 6: return 0xBFBFBFu;                         /* light control gray     */
    case 7: return INITECH_TITLEBAR_PINSTRIPE_RGB;   /* pinstripe LIGHT (idx 7)*/
    case 8: return 0x8A8A93u;                         /* pinstripe DARK  (idx 8)*/
    default:
        /* Total ramp for any other index: a deterministic gray scaled by index.*/
        {
            uint32_t v = (uint32_t)index;
            return (v << 16) | (v << 8) | v;
        }
    }
}

/* ---- geometry helpers ---------------------------------------------------- */

static uint32_t tight_pitch(uint32_t width, uint32_t bpp)
{
    return width * (bpp / 8u);
}

int render_ctx_init(render_ctx_t *ctx, const render_boot_info_t *boot)
{
    if (ctx == NULL || boot == NULL) {
        return 1;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->boot = *boot;

    /* The two modes k8o5.7 names: indexed-8 (OD-2) and direct 32bpp. */
    if (boot->lfb_bpp != 8u && boot->lfb_bpp != 32u) {
        return 2;
    }
    if (boot->lfb_width == 0u || boot->lfb_height == 0u) {
        return 3;
    }

    /* Pitch flows from the caller (AM-1). 0 means "tight" (width * bpp/8). The
     * caller may pass a wider pitch to model an LFB whose scanline is padded --
     * the skeleton honors it exactly, never assuming a hardcoded aperture. */
    uint32_t pitch = boot->lfb_pitch ? boot->lfb_pitch
                                     : tight_pitch(boot->lfb_width, boot->lfb_bpp);
    if (pitch < tight_pitch(boot->lfb_width, boot->lfb_bpp)) {
        return 4; /* a pitch narrower than the row is impossible (fail loud) */
    }

    /* The offscreen pixel buffer: pitch * height bytes. Initialized to the
     * DESKTOP background (a deterministic blank canvas, Rule 11) -- NOT zero,
     * because index 0 is black ink (frame/box borders); a black-on-black canvas
     * would make a drawn frame indistinguishable from the bare desktop. An
     * authentic System-7 desktop is gray/seafoam, never black (gui-ground-truth
     * Sec; spec/assets/palette.json desktop_bg). 8bpp: fill the desktop index;
     * 32bpp: fill the desktop RGB. */
    size_t fb_bytes = (size_t)pitch * (size_t)boot->lfb_height;
    uint8_t *fb_base = (uint8_t *)malloc(fb_bytes);
    if (fb_base == NULL) {
        return 5;
    }
    if (boot->lfb_bpp == 8u) {
        memset(fb_base, RENDER_DESKTOP_INDEX, fb_bytes);
    } else {
        /* 32bpp: write the desktop RGB into every pixel slot. */
        uint32_t bg = render_palette_rgb(RENDER_DESKTOP_INDEX) & 0x00FFFFFFu;
        for (size_t i = 0; i + 4u <= fb_bytes; i += 4u) {
            fb_base[i + 0] = (uint8_t)(bg & 0xFFu);          /* B */
            fb_base[i + 1] = (uint8_t)((bg >> 8) & 0xFFu);   /* G */
            fb_base[i + 2] = (uint8_t)((bg >> 16) & 0xFFu);  /* R */
            fb_base[i + 3] = 0u;                              /* X */
        }
    }

    /* A FLAIR arena for region storage (and any offscreen bitmaps a richer draw
     * program might allocate). Sized generously for the chrome regions. This is
     * the SAME flair_heap_t the kernel binds to its fixed window; here we back it
     * with a malloc'd buffer (caller-supplied-storage pattern, heap.h). */
    ctx->arena_size = 1u << 20; /* 1 MiB host arena */
    ctx->arena = (uint8_t *)malloc(ctx->arena_size);
    if (ctx->arena == NULL) {
        free(fb_base);
        return 6;
    }
    flair_heap_init(&ctx->heap, ctx->arena, ctx->arena_size);

    /* Wrap the offscreen as a FLAIR_BitMap. Every field is from the caller's
     * geometry except `base` (the host offscreen, since the host has no LFB). */
    ctx->fb.bm.base            = (volatile uint8_t *)fb_base;
    ctx->fb.bm.pitch           = pitch;
    ctx->fb.bm.bpp             = boot->lfb_bpp;
    ctx->fb.bm.bytes_per_pixel = boot->lfb_bpp / 8u;
    ctx->fb.bm.width           = boot->lfb_width;
    ctx->fb.bm.height          = boot->lfb_height;
    ctx->fb.bounds.top    = 0;
    ctx->fb.bounds.left   = 0;
    ctx->fb.bounds.bottom = (int16_t)boot->lfb_height;
    ctx->fb.bounds.right  = (int16_t)boot->lfb_width;

    /* Region storage for visRgn / clipRgn from the FLAIR arena (no per-row
     * malloc; the engine never allocates). Each region needs rows[] + x_pool. */
    rgn_row_t *vis_rows = (rgn_row_t *)flair_alloc(
        &ctx->heap, FLAIR_CLASS_REGION, sizeof(rgn_row_t) * RGN_ROWS_CAP);
    int16_t *vis_xpool = (int16_t *)flair_alloc(
        &ctx->heap, FLAIR_CLASS_REGION, sizeof(int16_t) * RGN_X_POOL_CAP);
    rgn_row_t *clip_rows = (rgn_row_t *)flair_alloc(
        &ctx->heap, FLAIR_CLASS_REGION, sizeof(rgn_row_t) * RGN_ROWS_CAP);
    int16_t *clip_xpool = (int16_t *)flair_alloc(
        &ctx->heap, FLAIR_CLASS_REGION, sizeof(int16_t) * RGN_X_POOL_CAP);
    if (vis_rows == NULL || vis_xpool == NULL ||
        clip_rows == NULL || clip_xpool == NULL) {
        free(fb_base);
        free(ctx->arena);
        memset(ctx, 0, sizeof(*ctx));
        return 7;
    }

    ctx->visRgn.rows = vis_rows;
    ctx->visRgn.cap_rows = RGN_ROWS_CAP;
    ctx->visRgn.x_pool = vis_xpool;
    ctx->visRgn.x_pool_cap = RGN_X_POOL_CAP;

    ctx->clipRgn.rows = clip_rows;
    ctx->clipRgn.cap_rows = RGN_ROWS_CAP;
    ctx->clipRgn.x_pool = clip_xpool;
    ctx->clipRgn.x_pool_cap = RGN_X_POOL_CAP;

    /* visRgn = the whole port; clipRgn = the whole port (no extra app clip).
     * The draw program clips through visRgn INTERSECT clipRgn (D-1/D-2). */
    rgn_rect_t whole = ctx->fb.bounds;
    region_set_rect(&ctx->visRgn, whole);
    region_set_rect(&ctx->clipRgn, whole);

    /* Open the GrafPort over the offscreen (SetPort target). portRect = whole
     * bitmap; the surface module enforces the clip at the pixel level. */
    ctx->port.portBits = ctx->fb;
    ctx->port.portRect = whole;
    ctx->port.visRgn   = &ctx->visRgn;
    ctx->port.clipRgn  = &ctx->clipRgn;
    ctx->port.pnLoc.v  = 0;
    ctx->port.pnLoc.h  = 0;
    ctx->port.pnSize.v = 1;
    ctx->port.pnSize.h = 1;
    ctx->port.pnVis    = 0;

    return 0;
}

void render_ctx_free(render_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->fb.bm.base != NULL) {
        free((void *)ctx->fb.bm.base);
    }
    if (ctx->arena != NULL) {
        free(ctx->arena);
    }
    memset(ctx, 0, sizeof(*ctx));
}

void render_run(render_ctx_t *ctx, render_draw_fn draw)
{
    if (ctx == NULL || draw == NULL) {
        return;
    }
    /* The kernel's SetPort(myPort); draw(); discipline -- the offscreen GrafPort
     * is the current port for the duration of the draw program. */
    draw(&ctx->port);
}

/* ---- pixel readback ------------------------------------------------------ */

static uint32_t raw_at(const render_ctx_t *ctx, uint32_t x, uint32_t y)
{
    const bitmap_t *bm = &ctx->fb.bm;
    if (x >= bm->width || y >= bm->height) {
        return 0u;
    }
    uint32_t off = y * bm->pitch + x * bm->bytes_per_pixel;
    return surface_get_pixel(bm, off);
}

uint32_t render_pixel_index(const render_ctx_t *ctx, uint32_t x, uint32_t y)
{
    if (ctx == NULL) {
        return 0u;
    }
    return raw_at(ctx, x, y);
}

uint32_t render_pixel_rgb(const render_ctx_t *ctx, uint32_t x, uint32_t y)
{
    if (ctx == NULL) {
        return 0u;
    }
    uint32_t v = raw_at(ctx, x, y);
    if (ctx->fb.bm.bpp == 8u) {
        return render_palette_rgb((uint8_t)(v & 0xFFu));
    }
    return v & 0x00FFFFFFu;
}

/* ---- deterministic P6 PPM ------------------------------------------------ */

int render_write_ppm(const render_ctx_t *ctx, const char *path)
{
    if (ctx == NULL || path == NULL || ctx->fb.bm.base == NULL) {
        return 1;
    }
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        return 2;
    }
    uint32_t w = ctx->fb.bm.width;
    uint32_t h = ctx->fb.bm.height;

    /* No timestamp, no comment -- byte-identical across runs (Rule 11). */
    if (fprintf(f, "P6\n%u %u\n255\n", w, h) < 0) {
        fclose(f);
        return 3;
    }
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t rgb = render_pixel_rgb(ctx, x, y);
            uint8_t px[3];
            px[0] = (uint8_t)((rgb >> 16) & 0xFFu); /* R */
            px[1] = (uint8_t)((rgb >> 8)  & 0xFFu); /* G */
            px[2] = (uint8_t)(rgb & 0xFFu);         /* B */
            if (fwrite(px, 1, 3, f) != 3) {
                fclose(f);
                return 4;
            }
        }
    }
    return (fclose(f) == 0) ? 0 : 5;
}
