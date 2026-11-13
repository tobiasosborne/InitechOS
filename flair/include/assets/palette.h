/*
 * palette.h -- GENERATED from spec/assets/palette.json by tools/palette_extract.c.
 * DO NOT EDIT BY HAND. Regenerate: make spec/assets/palette.h
 *
 * InitechOS desktop palette. Ref: PRD Sec 10 (palette extracted
 * from the reference frame), Sec 6.4 (assets), Sec 12 (frame is
 * REFERENCE ONLY -- these are MEASURED samples, not embedded image).
 * Source fixture: spec/assets/preview.webp. Each color was 3x3-sampled at the (x,y)
 * recorded in palette.json (auditable; test-assets re-checks it).
 *
 * REVOKED provenance (ADR-0004-AMENDMENT-DEC-09 / ADR-0010, HER-03):
 * the frame-sampled INITECH_*_FRAME_V0 constants below are the dim
 * Office-Space CRT MOCK-UP samples. They are PROVENANCE ONLY -- NOTHING
 * renders or grades against them. The ONE color authority is
 * flair_canon_rgb (color_canon.h, generated from the LOCKED
 * color_canon.json); flair_palette_rgb below is a THIN ALIAS to it
 * (idx2 = Initech teal #8DDCDC, NOT the revoked seafoam/preview gray).
 */
#ifndef INITECH_PALETTE_H
#define INITECH_PALETTE_H

#include "color_canon.h"  /* flair_canon_rgb -- THE color authority (ARB-1) */

/* sampled @ (1240,180) -- REVOKED provenance (HER-03); nothing renders it */
#define INITECH_DESKTOP_BG_FRAME_V0_R 0x73
#define INITECH_DESKTOP_BG_FRAME_V0_G 0x69
#define INITECH_DESKTOP_BG_FRAME_V0_B 0x6C
#define INITECH_DESKTOP_BG_FRAME_V0_RGB 0x73696Cu

/* sampled @ (1000,82) -- REVOKED provenance (HER-03); nothing renders it */
#define INITECH_MENUBAR_BG_FRAME_V0_R 0x67
#define INITECH_MENUBAR_BG_FRAME_V0_G 0x69
#define INITECH_MENUBAR_BG_FRAME_V0_B 0x6C
#define INITECH_MENUBAR_BG_FRAME_V0_RGB 0x67696Cu

/* sampled @ (560,380) -- REVOKED provenance (HER-03); nothing renders it */
#define INITECH_WINDOW_WHITE_FRAME_V0_R 0x7F
#define INITECH_WINDOW_WHITE_FRAME_V0_G 0x7F
#define INITECH_WINDOW_WHITE_FRAME_V0_B 0x86
#define INITECH_WINDOW_WHITE_FRAME_V0_RGB 0x7F7F86u

/* sampled @ (620,314) -- REVOKED provenance (HER-03); nothing renders it */
#define INITECH_TITLEBAR_PINSTRIPE_FRAME_V0_R 0x6B
#define INITECH_TITLEBAR_PINSTRIPE_FRAME_V0_G 0x6B
#define INITECH_TITLEBAR_PINSTRIPE_FRAME_V0_B 0x74
#define INITECH_TITLEBAR_PINSTRIPE_FRAME_V0_RGB 0x6B6B74u

/* sampled @ (744,105) -- REVOKED provenance (HER-03); nothing renders it */
#define INITECH_TITLEBAR_INK_FRAME_V0_R 0x52
#define INITECH_TITLEBAR_INK_FRAME_V0_G 0x5A
#define INITECH_TITLEBAR_INK_FRAME_V0_B 0x63
#define INITECH_TITLEBAR_INK_FRAME_V0_RGB 0x525A63u

/* sampled @ (520,372) -- REVOKED provenance (HER-03); nothing renders it */
#define INITECH_TEXT_BLACK_FRAME_V0_R 0x48
#define INITECH_TEXT_BLACK_FRAME_V0_G 0x3D
#define INITECH_TEXT_BLACK_FRAME_V0_B 0x4A
#define INITECH_TEXT_BLACK_FRAME_V0_RGB 0x483D4Au

/* sampled @ (700,401) -- REVOKED provenance (HER-03); nothing renders it */
#define INITECH_ACCENT_BLUE_FRAME_V0_R 0x1E
#define INITECH_ACCENT_BLUE_FRAME_V0_G 0x2F
#define INITECH_ACCENT_BLUE_FRAME_V0_B 0x87
#define INITECH_ACCENT_BLUE_FRAME_V0_RGB 0x1E2F87u

/* canonical (OS canon -- ADR/oracle, not frame-sampled; LOCKED) */
#define INITECH_DESKTOP_BG_R 0x8D
#define INITECH_DESKTOP_BG_G 0xDC
#define INITECH_DESKTOP_BG_B 0xDC
#define INITECH_DESKTOP_BG_RGB 0x8DDCDCu

/* ---------------------------------------------------------------------------
 * flair_palette_rgb -- THIN ALIAS to flair_canon_rgb (color_canon.h).
 *
 * A palette INDEX (0..255) -> a canonical 0x00RRGGBB value, resolved by the ONE
 * color authority flair_canon_rgb (idx2 = Initech teal #8DDCDC, crisp white,
 * navy accent, System-7 pinstripe; gray ramp for idx>=9). The LIVE desktop
 * converter (os/milton/kmain.c BOOT_FLAIR_SHELL) and the host screendump oracle
 * call this name, so both route through the SAME locked table and the rendered
 * desktop reads back identical pixels on host and metal (Law 2; fb-agree).
 *
 * Provenance: the old switch read the REVOKED preview/seafoam samples; it is
 * retired (ADR-0004-AMENDMENT-DEC-09 ARB-1, ADR-0010 HER-02/HER-03). Pure,
 * freestanding-safe (color_canon.h pulls <stdint.h> only): callable from the
 * kernel AND the host. Ref: spec/assets/color_canon.json (LOCKED, Rule 8);
 * CLAUDE.md Law 2, Rule 11 (deterministic), Rule 12 (ASCII-clean). */
static inline uint32_t flair_palette_rgb(uint8_t index)
{
    return flair_canon_rgb(index);
}

#endif /* INITECH_PALETTE_H */
