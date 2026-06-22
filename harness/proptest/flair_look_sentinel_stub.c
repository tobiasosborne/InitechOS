/*
 * flair_look_sentinel_stub.c -- the SENTINEL policy stub for
 *                               test-flair-mechanism-colorblind (THE ORACLE STUB).
 *
 * beads: initech-6bq2 (C-8 MECHANISM/POLICY enforcement); epic initech-qipc.
 * Ref:   ADR-0004-AMENDMENT-DEC-09 Sec 3.10 (DEC-09-D4 #2); CLAUDE.md Law 2,
 *        Rule 6, Rule 12.
 *
 * This stub REPLACES the real flair_look resolver (os/flair/flair_look.c) at
 * link time for the colorblind oracle.  It makes the ONE policy seam
 * (flair_look_pixel / flair_look_pixel_depth) return a SENTINEL for EVERY PART:
 *   32bpp -> magenta 0x00FF00FF  (the sentinel color)
 *   8bpp  -> a sentinel palette index (FLAIR_LOOK_SENTINEL_IDX)
 *
 * If chrome/control/dialog name color ONLY through the seam (C-8), then EVERY
 * decoration pixel they draw is the sentinel.  Any non-sentinel, non-background
 * pixel in the rendered scene means a mechanism module hard-coded a color ->
 * the colorblind oracle goes RED.  This is the BEHAVIORAL tooth the source
 * grep cannot give (it catches an obfuscated or computed literal too).
 *
 * Host-only factory code (the oracle's stub policy); not compiled into the OS.
 * ASCII-clean (Rule 12).
 */

#include <stdint.h>

#include "flair_look.h"        /* the seam signatures we override (-Ios/flair)  */
#include "surface.h"           /* surface_pack_rgb (-Ios/flair)                 */

/* The 32bpp sentinel color (magenta) and the 8bpp sentinel index. */
#define FLAIR_LOOK_SENTINEL_RGB  0x00FF00FFu
#define FLAIR_LOOK_SENTINEL_IDX  200u

uint32_t flair_look_pixel_depth(uint32_t bpp, int part)
{
    (void)part;                            /* EVERY PART -> the sentinel        */
    if (bpp == 8u) {
        return FLAIR_LOOK_SENTINEL_IDX;
    }
    return surface_pack_rgb(bpp, 0, 0, 0) | FLAIR_LOOK_SENTINEL_RGB;
}

uint32_t flair_look_pixel(const GrafPort *port, int part)
{
    uint32_t bpp = (port != 0) ? port->portBits.bm.bpp : 32u;
    return flair_look_pixel_depth(bpp, part);
}
