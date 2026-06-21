/*
 * palette.h -- GENERATED from spec/assets/palette.json by tools/palette_extract.c.
 * DO NOT EDIT BY HAND. Regenerate: make spec/assets/palette.h
 *
 * InitechOS desktop palette v0. Ref: PRD Sec 10 (palette extracted
 * from the reference frame), Sec 6.4 (assets), Sec 12 (frame is
 * REFERENCE ONLY -- these are MEASURED samples, not embedded image).
 * Source fixture: spec/assets/preview.webp. Each color was 3x3-sampled at the (x,y)
 * recorded in palette.json (auditable; test-assets re-checks it).
 */
#ifndef INITECH_PALETTE_H
#define INITECH_PALETTE_H

/* sampled @ (1240,180) */
#define INITECH_DESKTOP_BG_FRAME_V0_R 0x73
#define INITECH_DESKTOP_BG_FRAME_V0_G 0x69
#define INITECH_DESKTOP_BG_FRAME_V0_B 0x6C
#define INITECH_DESKTOP_BG_FRAME_V0_RGB 0x73696Cu

/* sampled @ (1000,82) */
#define INITECH_MENUBAR_BG_R 0x67
#define INITECH_MENUBAR_BG_G 0x69
#define INITECH_MENUBAR_BG_B 0x6C
#define INITECH_MENUBAR_BG_RGB 0x67696Cu

/* sampled @ (560,380) */
#define INITECH_WINDOW_WHITE_R 0x7F
#define INITECH_WINDOW_WHITE_G 0x7F
#define INITECH_WINDOW_WHITE_B 0x86
#define INITECH_WINDOW_WHITE_RGB 0x7F7F86u

/* sampled @ (620,314) */
#define INITECH_TITLEBAR_PINSTRIPE_R 0x6B
#define INITECH_TITLEBAR_PINSTRIPE_G 0x6B
#define INITECH_TITLEBAR_PINSTRIPE_B 0x74
#define INITECH_TITLEBAR_PINSTRIPE_RGB 0x6B6B74u

/* sampled @ (744,105) */
#define INITECH_TITLEBAR_INK_R 0x52
#define INITECH_TITLEBAR_INK_G 0x5A
#define INITECH_TITLEBAR_INK_B 0x63
#define INITECH_TITLEBAR_INK_RGB 0x525A63u

/* sampled @ (520,372) */
#define INITECH_TEXT_BLACK_R 0x48
#define INITECH_TEXT_BLACK_G 0x3D
#define INITECH_TEXT_BLACK_B 0x4A
#define INITECH_TEXT_BLACK_RGB 0x483D4Au

/* sampled @ (700,401) */
#define INITECH_ACCENT_BLUE_R 0x1E
#define INITECH_ACCENT_BLUE_G 0x2F
#define INITECH_ACCENT_BLUE_B 0x87
#define INITECH_ACCENT_BLUE_RGB 0x1E2F87u

/* canonical (OS canon -- ADR/oracle, not frame-sampled) */
#define INITECH_DESKTOP_BG_R 0x6F
#define INITECH_DESKTOP_BG_G 0xA0
#define INITECH_DESKTOP_BG_B 0x8E
#define INITECH_DESKTOP_BG_RGB 0x6FA08Eu

/* ---------------------------------------------------------------------------
 * flair_palette_rgb -- the deterministic FLAIR indexed-8 palette: a palette
 * INDEX (0..255) -> a canonical 0x00RRGGBB value.
 *
 * This is THE single point of truth for "what RGB does FLAIR index N mean", so
 * the LIVE desktop converter (os/milton/kmain.c BOOT_FLAIR_SHELL: present the
 * 8bpp offscreen onto a 24/32bpp LFB) and the host screendump oracle agree by
 * CONSTRUCTION (Law 2). It is a byte-for-byte port of the FACTORY render
 * skeleton's render_palette_rgb (harness/render/render.c:56-75) -- SAME cases,
 * SAME constants, SAME total gray ramp for any other index. The two share the
 * same INITECH_*_RGB samples here, so the rendered-in-indices desktop reads back
 * the SAME pixels whether it is run on the host (render_palette_rgb) or composed
 * live and presented to a direct-color LFB (this function).
 *
 * Indices (verbatim render.c comments; chrome.c CIDX_* / desktop.h
 * FLAIR_DESKTOP_BG_INDEX use these too):
 *   0 black ink   1 window white   2 desktop seafoam   3 menubar gray
 *   4 titlebar ink   5 accent blue   6 light control gray
 *   7 pinstripe LIGHT (WDEF idx 7)   8 pinstripe DARK (WDEF idx 8)
 *   default: a deterministic gray scaled by the index (total over all 256).
 *
 * The 0xBFBFBFu (idx 6) and 0x8A8A93u (idx 8) literals are golden-resolves
 * (gui-ground-truth.md Sec 3.3) carried verbatim from render.c; the oracle keys
 * on the INDEX, not the exact RGB, for the pinstripe shades.
 *
 * Pure, freestanding-safe (no libc, <stdint.h> only): callable from the kernel
 * AND the host. Ref: harness/render/render.c:56-75 (render_palette_rgb, the
 * correspondence the screendump oracle reuses); CLAUDE.md Law 2 (oracle is
 * truth), Rule 11 (deterministic), Rule 12 (ASCII-clean). beads initech-re30.3
 * (toward the bmih palette-share consolidation). */
#include <stdint.h>
static inline uint32_t flair_palette_rgb(uint8_t index)
{
    switch (index) {
    case 0: return 0x000000u;                        /* black                  */
    case 1: return INITECH_WINDOW_WHITE_RGB;         /* body white             */
    case 2: return INITECH_DESKTOP_BG_RGB;           /* desktop seafoam        */
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

#endif /* INITECH_PALETTE_H */
