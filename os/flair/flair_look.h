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
 * ARB-3 (Sec 3.3): flair_look is a RESOLVER ON TOP of flair_canon_rgb, NOT a
 * second color table. Its base map is PART->canon INDEX pure data. The D-9
 * chrome entry additionally reads same-valued named slots from the selected
 * flair_skin_t registry row; the remaining composition roles keep their named
 * palette indices. This TU owns ZERO hand-typed color value and ZERO era
 * switch. It and the device CLUT are the only OS sites permitted to resolve an
 * index or policy slot into a destination pixel.
 *
 * DEPTH QUANTIZE (mirrors chrome.c chrome_px's existing logic):
 *   8bpp  -> the palette index byte (surface writes the low byte).
 *   else  -> surface_pack_rgb(bpp,0,0,0) | flair_canon_rgb(idx)  (0x00RRGGBB).
 *
 * The default entry points remain for bitmap-only and non-chrome consumers.
 * Chrome additionally threads a const flair_skin_t * into
 * flair_look_pixel_for_skin: D-9 era selection is DATA at the policy seam,
 * never a switch(era) in mechanism code.
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

/* Opaque here so including the policy API does not instantiate the header-only
 * registry in every mechanism TU. The complete data-only row lives in
 * spec/flair_skins.h and is consumed by flair_look.c. */
typedef struct flair_skin flair_skin_t;

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
    FLAIR_PART_HILITE_FRAME,  /* inactive title frame (gray) -> CIDX_HILITE_FRAME
                               * (idx>=9 ramp, #777777; beads initech-hv7u)      */
    FLAIR_PART_HILITE_TEXT,   /* inactive title ink (dimmed) -> CIDX_HILITE_TEXT
                               * (idx>=9 ramp, #A5A5A5; beads initech-hv7u)      */
    /* Platinum rows append only: existing PART ordinals stay frozen.
     * Ref: ../system7-decomp/specs/sys8/platinum-palette.md Sec 2. */
    FLAIR_PART_PLAT_STRIPE_DARK,    /* title dark stripe                         */
    FLAIR_PART_PLAT_FRAME_FACE,     /* frame-bar face / title gap / grow fill    */
    FLAIR_PART_PLAT_FACE,           /* inactive + dialog/menu face               */
    FLAIR_PART_PLAT_FRAME_SHADOW,   /* active frame-bar shadow                    */
    FLAIR_PART_PLAT_WIDGET_EDGE,    /* widget edge and interior shadow            */
    FLAIR_PART_PLAT_DARK_RING,      /* widget ring and glyphs                     */
    FLAIR_PART_PLAT_INACTIVE_FRAME, /* inactive frames and shadow                 */
    FLAIR_PART_PLAT_INACTIVE_TEXT,  /* inactive title ink                         */
    FLAIR_PART_PLAT_TROUGH,         /* disabled/hollow scrollbar trough           */
    FLAIR_PART_PLAT_WELL,           /* enabled scrollbar page well                */
    FLAIR_PART_PLAT_TILE_SHADOW,    /* enabled arrow-tile shadow                  */
    /* Platinum Menu Manager rows append only.  These names keep menu.c on the
     * mechanism side of the DEC-09 C-8 cut: it names semantic menu roles, never
     * palette indices or RGB values.  Ref: sys8/menus.md Sec 1.1-1.4, Sec 2.1
     * and Sec 2.3; bead initech-sjvq. */
    FLAIR_PART_MENU_BAR_HL,          /* bar row 0                                  */
    FLAIR_PART_MENU_BAR_FACE,        /* bar rows 1..17                             */
    FLAIR_PART_MENU_BAR_SHADOW,      /* bar row 18                                 */
    FLAIR_PART_MENU_PANEL_FACE,      /* pull-down body                             */
    FLAIR_PART_MENU_PANEL_HL,        /* inner top/left bevel                       */
    FLAIR_PART_MENU_PANEL_SHADOW,    /* inner bottom/right bevel                   */
    FLAIR_PART_MENU_DROP_SHADOW,     /* distinct dark-gray (+1,+1) shadow          */
    FLAIR_PART_MENU_DISABLED_INK,    /* dimmed item + command-key ink              */
    FLAIR_PART_MENU_TITLE_HILITE_HL, /* authored teal top row                      */
    FLAIR_PART_MENU_TITLE_HILITE_FACE,/* authored teal face rows                  */
    FLAIR_PART_MENU_TITLE_HILITE_SHADOW,/* authored teal bottom row               */
    FLAIR_PART__COUNT         /* sentinel: number of PARTs                       */
} FLAIR_PART;

/* Platinum composition policy:
 * - Chrome white/highlights reuse FLAIR_PART_CONTENT (window-chrome.md Sec 2.1,
 *   Sec 3.2, Sec 4, Sec 5; scrollbars.md Sec 2.2).
 * - The widget 7-rung diagonal ramp reuses FRAME_SHADOW, WELL, TILE_SHADOW,
 *   FRAME_FACE, FACE, TROUGH, then CONTENT-white in that order
 *   (window-chrome.md Sec 3.2); no extra PART is required.
 * - The real Platinum Lavender thumb is the OQ-2 authored substitution and
 *   reuses FLAIR_PART_BEVEL_LIGHT / FLAIR_PART_BEVEL_SHADOW as the two-tone
 *   Initech-teal pair (ADR-0004-AMENDMENT-DEC-10 Sec 3.2 / P4;
 *   scrollbars.md Sec 2.4 records the source accent).
 * - The pulled menu title applies that same ratified two-tone teal pair to the
 *   clut-208 accent structure.  No third accent row is authored or accreted
 *   (DEC-10 Sec 6 OQ-2/OQ-3; sys8/menus.md Sec 1.4; bead initech-sjvq).
 */

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

/* Resolve the default policy row once at a drawing-context boundary. The named
 * mutant deliberately selects retained SYS7 so the independent Platinum
 * fidelity oracle proves this pointer reaches pixels (Rule 6). */
const flair_skin_t *flair_look_default_skin(void);

/* Explicit D-9 era-data seam used by chrome. Only roles carried by the row at
 * the same default value are read from skin; the remaining composition roles
 * stay named palette indices (C-8). */
uint32_t flair_look_pixel_for_skin(const GrafPort *port,
                                   const flair_skin_t *skin, int part);

/* ---------------------------------------------------------------------------
 * flair_look_pixel(port, PART) -- the GrafPort-keyed policy seam (ARB-3).
 * Reads the destination depth from port->portBits.bm.bpp and delegates to
 * flair_look_pixel_depth.  This is the function decoration (chrome/control/
 * dialog) calls instead of naming any color.
 * ------------------------------------------------------------------------- */
uint32_t flair_look_pixel(const GrafPort *port, int part);

#endif /* INITECH_OS_FLAIR_LOOK_H */
