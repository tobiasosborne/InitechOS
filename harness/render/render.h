/*
 * harness/render/render.h -- the FLAIR host-render skeleton (FACTORY, host-only).
 *
 * beads: initech-k8o5.7 (host render skeleton) -- the crown-jewel step that lets
 *        the artifact chrome-drawing code (os/flair/chrome.c) run on a host
 *        offscreen bitmap and emit a deterministic PPM, so test-chrome can make
 *        structural assertions against chrome_metrics v1 (ADR-0004 D-8/FO-2).
 *
 * LAW 3 SEPARATION: this is FACTORY code. It HOSTS the artifact draw routines on
 * an offscreen bitmap and dumps a PPM. It may use libc (malloc/stdio). It is NOT
 * compiled into the kernel. The artifact draw code it invokes (chrome.c) is
 * freestanding and touches pixels ONLY through the surface module + GrafPort.
 *
 * FO-C / AM-1 (the load-bearing parameterization): the render context is built
 * from CALLER-SUPPLIED LFB geometry -- a `render_boot_info_t` that mirrors the
 * kernel's boot_info_t (os/milton/boot_info.h: lfb_addr/pitch/bpp/width/height).
 * There is NO hardcoded aperture, width, height, pitch, or bpp anywhere in this
 * skeleton: every pixel-buffer dimension flows from the caller's geometry exactly
 * as the kernel's surface state flows from the runtime boot_info_t the VBE/VGA
 * path reports. The offscreen buffer is allocated to `pitch * height` bytes; the
 * bitmap_t fields are copied 1:1 from the supplied geometry (lfb_addr is replaced
 * by the host offscreen base -- the ONLY field that cannot be a physical address
 * on the host, since the host has no LFB; everything else is the caller's).
 *
 * Ref: ADR-0004 D-1/D-2 (all drawing through a GrafPort clipped by
 *      visRgn INTERSECT clipRgn; one surface module), AM-1/FO-C (host model
 *      parameterized by runtime boot_info geometry, never a hardcoded aperture),
 *      D-8 (the oracle vector: test-chrome is a hard structural gate).
 *      spec/grafport.h (GrafPort), spec/imaging.h (FLAIR_BitMap), os/flair/
 *      surface.h (bitmap_t -- the ONE pixel buffer), os/flair/heap.h (the FLAIR
 *      arena), spec/region_algebra.h (region_t for visRgn/clipRgn).
 *      CLAUDE.md Law 2 (oracle is truth), Rule 11 (deterministic: fixed palette,
 *      no timestamps, byte-identical PPM across runs), Rule 12 (ASCII-clean).
 */
#ifndef INITECH_HARNESS_RENDER_H
#define INITECH_HARNESS_RENDER_H

#include <stdint.h>
#include <stddef.h>

#include "grafport.h"          /* GrafPort, FLAIR_BitMap (-Ispec)             */
#include "region_algebra.h"    /* region_t (-Ispec)                          */
#include "heap.h"              /* flair_heap_t (-Ios/flair)                  */

/* ---------------------------------------------------------------------------
 * render_boot_info_t -- a FAKE boot_info, the caller-supplied LFB geometry.
 *
 * Mirrors os/milton/boot_info.h boot_info_t's LFB fields field-for-field (AM-1).
 * The host has no real LFB, so `lfb_addr` is ignored on construction (the
 * skeleton allocates the offscreen and uses that base); every OTHER field --
 * pitch, bpp, width, height -- drives the offscreen exactly as the kernel's
 * surface state is driven by the runtime boot_info_t. Passing this struct (not
 * hardcoded constants) is the FO-C proof: the geometry is a runtime parameter.
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t lfb_addr;    /* (ignored on host -- the offscreen base is used)   */
    uint32_t lfb_pitch;   /* bytes per scanline (>= width * bpp/8); 0 => tight */
    uint32_t lfb_bpp;     /* 8 or 32 (the two modes ADR-0004 D-8/k8o5.7 name)  */
    uint32_t lfb_width;   /* horizontal resolution in pixels                   */
    uint32_t lfb_height;  /* vertical resolution in pixels                     */
} render_boot_info_t;

/* ---------------------------------------------------------------------------
 * render_ctx_t -- the host render context.
 *
 * Owns: a heap-window malloc'd buffer + a flair_heap_t over it (so offscreen
 * bitmaps and region storage come from the SAME FLAIR arena the kernel uses --
 * the dual-compile/caller-supplied-storage pattern), an offscreen FLAIR_BitMap,
 * a GrafPort SetPort'd over it, and the visRgn/clipRgn storage for that port.
 * ------------------------------------------------------------------------- */
typedef struct {
    render_boot_info_t  boot;        /* the caller-supplied geometry (AM-1)    */

    uint8_t            *arena;       /* malloc'd FLAIR heap window (libc)       */
    uint32_t            arena_size;  /* bytes in `arena`                       */
    flair_heap_t        heap;        /* FLAIR arena over `arena`               */

    GrafPort            port;        /* the current port (SetPort'd over fb)   */
    FLAIR_BitMap        fb;          /* the offscreen destination bitmap       */

    /* visRgn / clipRgn backing storage (arena-allocated; the Window Manager
     * would own visRgn and the app clipRgn -- here the skeleton supplies a
     * full-port visRgn and a NULL-equivalent full clipRgn for the draw program
     * to clip against the whole offscreen). */
    region_t            visRgn;
    region_t            clipRgn;
} render_ctx_t;

/* A draw program: a function the skeleton runs against the current port.
 * The artifact draw code (chrome.c) exposes one of these. */
typedef void (*render_draw_fn)(GrafPort *port);

/* The blank-canvas DESKTOP background palette index the skeleton fills the
 * offscreen with (NOT 0 -- index 0 is black ink; see render_ctx_init). The
 * oracle uses it to tell painted chrome from the bare desktop. */
#define RENDER_DESKTOP_INDEX 2u

/* ---------------------------------------------------------------------------
 * render_ctx_init -- build a render context from caller-supplied LFB geometry.
 *
 * Allocates an offscreen of `boot->lfb_pitch * boot->lfb_height` bytes (pitch
 * defaults to a tight `width * bpp/8` if the caller passes 0), wraps it as a
 * FLAIR_BitMap, opens a GrafPort over it (portRect = whole bitmap, visRgn = the
 * whole port, clipRgn = the whole port), and binds a FLAIR heap over a separate
 * arena window. The offscreen is zero-filled (deterministic blank, Rule 11).
 *
 * Returns 0 on success, non-zero on a bad geometry or allocation failure
 * (fail-loud, Rule 2 -- the caller/oracle treats non-zero as a hard error).
 * The bpp MUST be 8 or 32 (the two modes named by k8o5.7); other values FAIL.
 * ------------------------------------------------------------------------- */
int render_ctx_init(render_ctx_t *ctx, const render_boot_info_t *boot);

/* ---------------------------------------------------------------------------
 * render_ctx_free -- release the offscreen + arena. Safe on a zeroed ctx.
 * ------------------------------------------------------------------------- */
void render_ctx_free(render_ctx_t *ctx);

/* ---------------------------------------------------------------------------
 * render_run -- SetPort over the offscreen and run the draw program.
 *
 * Equivalent to the kernel's SetPort(myPort); draw(); discipline: it makes
 * ctx->port the current FLAIR port, invokes `draw(&ctx->port)`, then leaves the
 * offscreen holding the rendered pixels. The draw program clips through
 * visRgn INTERSECT clipRgn (D-1/D-2).
 * ------------------------------------------------------------------------- */
void render_run(render_ctx_t *ctx, render_draw_fn draw);

/* ---------------------------------------------------------------------------
 * render_pixel_rgb -- read pixel (x,y) from the offscreen as 0x00RRGGBB.
 *
 * For 32bpp this is the stored packed value. For 8bpp the palette index is
 * mapped through the deterministic render palette (render_palette_rgb). This is
 * the accessor test-chrome makes its structural assertions through, so the
 * oracle reasons in RGB regardless of the offscreen depth.
 * ------------------------------------------------------------------------- */
uint32_t render_pixel_rgb(const render_ctx_t *ctx, uint32_t x, uint32_t y);

/* ---------------------------------------------------------------------------
 * render_pixel_index -- read the raw stored value at (x,y).
 *
 * 8bpp: the palette index (0..255). 32bpp: the packed 0x00RRGGBB. Used by the
 * oracle when it must reason about the indexed-8 shade index directly (e.g. the
 * pinstripe alternation uses wctb shade indices 7/8, which are golden-resolves
 * as RGBs but exact as indices). On 32bpp this equals render_pixel_rgb.
 * ------------------------------------------------------------------------- */
uint32_t render_pixel_index(const render_ctx_t *ctx, uint32_t x, uint32_t y);

/* ---------------------------------------------------------------------------
 * render_palette_rgb -- the deterministic indexed-8 palette: index -> 0x00RRGGBB.
 *
 * A fixed, byte-stable ramp (Rule 11) so the PPM is identical across runs. A few
 * indices are pinned to named FLAIR colors (the spec/assets/palette.h samples and
 * the WDEF pinstripe shade indices 7/8) so the rendered window reads correctly to
 * a human and the oracle's index<->rgb mapping is stable. Pure function.
 * ------------------------------------------------------------------------- */
uint32_t render_palette_rgb(uint8_t index);

/* ---------------------------------------------------------------------------
 * render_write_ppm -- dump the offscreen to a deterministic P6 (binary) PPM.
 *
 * 8bpp indices are mapped through render_palette_rgb; 32bpp is written directly.
 * The PPM header is "P6\n<w> <h>\n255\n" -- NO timestamp, NO comment line, so the
 * output is byte-identical across runs (Rule 11). Returns 0 on success.
 * ------------------------------------------------------------------------- */
int render_write_ppm(const render_ctx_t *ctx, const char *path);

#endif /* INITECH_HARNESS_RENDER_H */
