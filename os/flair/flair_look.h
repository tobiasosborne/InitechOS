/*
 * os/flair/flair_look.h -- the FLAIR policy seam: the PART->pixel resolver
 *                          (THE ARTIFACT; the ONE color seam below the C-8 cut).
 *
 * beads: initech-6bq2 (C-8 MECHANISM/POLICY enforcement); epic initech-qipc.
 *
 * THE C-8 CUT-LINE (ADR-0004-AMENDMENT-DEC-09 Sec 3.1 / Sec 5.1, constraint
 * C-8): no mechanism module names a color.  The imaging MECHANISM names a
 * palette PART (a wctb-keyed semantic role) and converts PART -> destination
 * pixel ONLY through flair_look_pixel(port, PART) -- the single policy seam.
 *
 * ARB-3 (Sec 3.3): flair_look_pixel is a RESOLVER ON TOP of flair_canon_rgb,
 * NOT a second color table.  It maps a PART to a canon INDEX via a static
 * const PART->idx map (PURE DATA), then calls flair_canon_rgb(idx) -- the ONE
 * locked color authority (spec/assets/color_canon.h) -- and device-quantizes
 * for the port depth.  It owns ZERO 0xRRGGBB literal and ZERO index->RGB
 * switch.  This resolver TU and the device CLUT (spec/assets/clut.h) are the
 * ONLY two sites in the OS permitted to turn an index into a color.
 *
 * DEPTH QUANTIZE (mirrors chrome.c chrome_px's existing logic):
 *   8bpp  -> the palette index byte (surface writes the low byte).
 *   else  -> surface_pack_rgb(bpp,0,0,0) | flair_canon_rgb(idx)  (0x00RRGGBB).
 *
 * Two entry points share ONE resolution core:
 *   flair_look_pixel(port, PART)        -- the GrafPort-keyed seam (decoration).
 *   flair_look_pixel_depth(bpp, PART)   -- the bitmap-only seam (the desktop
 *                                          compositor fills a bitmap_t with no
 *                                          GrafPort; same one resolution core).
 *
 * Freestanding-safe (Law 3): <stdint.h> + the locked spec headers only; no
 * libc, no malloc.  Dual-compiles under kernel flags and hosted.
 * ASCII-clean (Rule 12).  Deterministic (Rule 11).
 *
 * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.1 (C-8), Sec 3.3 (ARB-3), Sec 5.1;
 *      spec/assets/color_canon.h (flair_canon_rgb + CIDX_*);
 *      spec/grafport.h (GrafPort; the policy seam surface);
 *      os/flair/chrome.c (chrome_px -- the depth logic mirrored here).
 */
#ifndef INITECH_OS_FLAIR_LOOK_H
#define INITECH_OS_FLAIR_LOOK_H

#include <stdint.h>

#include "grafport.h"          /* GrafPort (the policy seam surface)            */

/* ---------------------------------------------------------------------------
 * FLAIR_PART -- the wctb-keyed PART namespace (ADR-0004-AMENDMENT-DEC-09
 * Sec 3.1 / ARB-3).  Each PART is a SEMANTIC role the decoration names; the
 * resolver maps it to a canon index.  Keying PART on the wctb namespace makes
 * the resolver KEY identical to the golden KEY (the value oracle diffs
 * key-for-key; ADR-0010).
 *
 * The enum VALUES are an internal contiguous index into the static PART->idx
 * map below; they are NOT palette indices and NOT colors (Rule 11 stable).
 * ------------------------------------------------------------------------- */
typedef enum {
    FLAIR_PART_CONTENT = 0,   /* window/content body white   -> CIDX_WHITE   (1) */
    FLAIR_PART_FRAME,         /* frame / border ink (black)  -> CIDX_BLACK   (0) */
    FLAIR_PART_TEXT,          /* title / text ink            -> CIDX_TITLE_INK(4) */
    FLAIR_PART_DESKTOP,       /* desktop background (teal)   -> CIDX_DESKTOP (2) */
    FLAIR_PART_MENUBAR,       /* menu bar background         -> CIDX_MENUBAR (3) */
    FLAIR_PART_CAPTION_NAVY,  /* caption / accent navy       -> CIDX_ACCENT  (5) */
    FLAIR_PART_BTNFACE,       /* control / button face gray  -> CIDX_CONTROL (6) */
    FLAIR_PART_PIN_LIGHT,     /* pinstripe light shade       -> CIDX_PIN_LIGHT(7) */
    FLAIR_PART_PIN_DARK,      /* pinstripe dark shade        -> CIDX_PIN_DARK (8) */
    FLAIR_PART_BEVEL_LIGHT,   /* title bevel light (teal)    -> bevel_light row  */
    FLAIR_PART_BEVEL_SHADOW,  /* title bevel shadow          -> bevel_shadow row */
    FLAIR_PART__COUNT         /* sentinel: number of PARTs                       */
} FLAIR_PART;

/* ---------------------------------------------------------------------------
 * flair_look_pixel_depth(bpp, PART) -- resolve a PART to the destination pixel
 * value for depth `bpp`.  The bitmap-only seam (the desktop compositor has a
 * bitmap_t, not a GrafPort).  This is the resolution CORE; flair_look_pixel
 * (the GrafPort seam) delegates here.
 *
 * 8bpp  -> the palette index byte.
 * else  -> surface_pack_rgb(bpp,0,0,0) | flair_canon_rgb(idx)  (0x00RRGGBB).
 * ------------------------------------------------------------------------- */
uint32_t flair_look_pixel_depth(uint32_t bpp, int part);

/* ---------------------------------------------------------------------------
 * flair_look_pixel(port, PART) -- the GrafPort-keyed policy seam (ARB-3).
 * Reads the destination depth from port->portBits.bm.bpp and delegates to
 * flair_look_pixel_depth.  This is the function decoration (chrome/control/
 * dialog) calls instead of naming any color.
 * ------------------------------------------------------------------------- */
uint32_t flair_look_pixel(const GrafPort *port, int part);

#endif /* INITECH_OS_FLAIR_LOOK_H */
