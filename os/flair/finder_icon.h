/*
 * os/flair/finder_icon.h -- the Finder desktop-icon blitter (THE ARTIFACT).
 *
 * beads: initech-tdnl.9 (R3 "STAPLER-DESK": desktop icon assets + their blit).
 *
 * WHAT THIS IS: the one routine that puts a 32x32 FLAIRDeskIcon strike
 * (spec/assets/desk_icons.h) on a surface, plus the mask hit-test the DesktopMgr
 * lane needs for click and drag targeting. It is deliberately tiny: strike in,
 * pixels out, no state, no allocation.
 *
 * THREE RULES IT OBEYS
 *   1. TRANSPARENCY IS THE MASK. A strike pixel whose mask bit is 0 is not
 *      written at all -- the desktop (or whatever is already there) shows
 *      through. This is the cursors.h/CURS rule the CursorMgr already follows.
 *   2. NO COLOR IS NAMED HERE (ADR-0004-AMENDMENT-DEC-09 Sec 3.1, constraint
 *      C-8). A tone is a SEMANTIC role -- FLAIR_PART_ICON_INK / _FACE / _SHADE
 *      -- resolved to a destination pixel ONLY through flair_look_pixel_depth,
 *      the ONE policy seam. This TU owns zero 0xRRGGBB literal and zero palette
 *      index, exactly like os/flair/desktop.c's desktop_px.
 *   3. ALL DRAWING IS CLIPPED. A destination pixel is written IFF it is inside
 *      the caller's effective clip region (visRgn INTERSECT clipRgn), matching
 *      os/flair/blitter.h. NULL clip means "no additional clip".
 *
 * WHY PER-PIXEL AND NOT blitter_blit_clipped: blitter_blit_clipped is a srcCopy
 * of an opaque block -- it has no transparency and no tone->PART resolution, so
 * feeding it a strike would paint the mask holes. The per-pixel walk is the
 * os/flair/cursor.c precedent (a 16x16 CURS composite done pixel-by-pixel
 * through surface_put_pixel); at 1024 pixels per icon the cost is irrelevant and
 * the correctness is obvious.
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F1.1/F1.2 (icon records,
 *      32x32 sprites, "selection colors resolve through FLAIR_PART_* roles,
 *      never literals"); spec/assets/desk_icons.h (the LOCKED strikes + the
 *      tone decode); os/flair/flair_look.h (the policy seam);
 *      os/flair/surface.h (the ONE pixel writer); spec/region_algebra.h
 *      (region_contains_point -- clip membership).
 *      CLAUDE.md Law 2 (mechanical oracle), Law 3 (freestanding + dual-compile),
 *      Rule 2 (bounds-checked, never write past the buffer), Rule 6
 *      (mutation-proven), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * DUAL-COMPILE (the cursor.c / blitter.c pattern): finder_icon.c compiles BOTH
 * freestanding for the kernel and hosted for the property suite
 * (harness/proptest/test_desk_icons.c). No libc, no malloc; all storage is
 * caller-supplied.
 */
#ifndef INITECH_OS_FLAIR_FINDER_ICON_H
#define INITECH_OS_FLAIR_FINDER_ICON_H

#include <stdint.h>

#include "surface.h"            /* bitmap_t + the ONE pixel writer            */
#include "region_algebra.h"     /* region_t + region_contains_point (-Ispec)  */
#include "desk_icons.h"         /* FLAIRDeskIcon strikes (-Ispec/assets)      */

/* --------------------------------------------------------------------------
 * finder_icon_draw -- draw the 32x32 strike `icon` with its top-left at (x,y).
 *
 *   dst   -- destination surface (8bpp indexed or direct color).
 *   x, y  -- destination top-left; may be negative or off the right/bottom edge
 *            (the routine draws the visible part and nothing else).
 *   icon  -- the LOCKED strike (spec/assets/desk_icons.h). NULL is a no-op.
 *   clip  -- effective visRgn INTERSECT clipRgn; NULL => no additional clip.
 *
 * A destination pixel (x+c, y+r) is written IFF
 *   (a) the strike's mask bit for (r,c) is set (the pixel is opaque), AND
 *   (b) the pixel lies on the destination bitmap, AND
 *   (c) clip == NULL or region_contains_point(clip, x+c, y+r).
 * Its value is flair_look_pixel_depth(dst->bpp, PART) for the tone's PART.
 * Transparent strike pixels are never written -- the destination survives.
 * -------------------------------------------------------------------------- */
void finder_icon_draw(const bitmap_t *dst, int16_t x, int16_t y,
                      const FLAIRDeskIcon *icon, const region_t *clip);

/* --------------------------------------------------------------------------
 * finder_icon_hit -- does the point (x,y) land on an OPAQUE pixel of `icon`
 * drawn with its top-left at (icon_x, icon_y)?
 *
 * Returns 1 for a hit, 0 otherwise (including a NULL icon and any point outside
 * the 32x32 cell). The test is the strike's MASK, not its bounding box: a click
 * in the transparent corner of the cell is not a hit on the icon, which is the
 * period behaviour the Finder's click/drag targeting depends on (design F1.2,
 * "point -> nearest icon whose sprite ... rect contains it").
 *
 * Pure function of the strike and the coordinates -- no surface, no state.
 * -------------------------------------------------------------------------- */
int finder_icon_hit(int16_t x, int16_t y,
                    const FLAIRDeskIcon *icon, int16_t icon_x, int16_t icon_y);

#endif /* INITECH_OS_FLAIR_FINDER_ICON_H */
