/* Independent scroll-bar crops, initech-tdnl.96/.97/.101.
 * Transcribed from system7-decomp/goldens/captures/s8_doc_window_inactive.png.
 * Half-open crops: V thumb (608,311,622,326), H thumb (330,401,345,415),
 * arrows up/down (608,296/386,622,310/400), left/right
 * (315/593,401,329/607,415). Ref: specs/sys8/scrollbars.md 2.2, 2.4.
 * H/P/S/R = sampled Lavender DADAFF/B3B3FF/8787DA/5454B3;
 * f = F3F3F3. W/F/D/K = FFFFFF/E7E7E7/CDCDCD/000000.
 * The host gate verifies these literal matrices against the original PNG.
 * They are never read by the artifact. No renderer constants are included.
 */
#ifndef INITECH_SCROLLBAR_LOOK_GOLDEN_H
#define INITECH_SCROLLBAR_LOOK_GOLDEN_H
#include <stdint.h>
static const char SL_thumb_v[] =
    "fHHHHHHHHHHHHP"
    "HPPPPPPPPPPPPS"
    "HPPPPPPPPPPPPS"
    "HPPfHHHHHHPPPS"
    "HPPPRRRRRRRPPS"
    "HPPfHHHHHHPPPS"
    "HPPPRRRRRRRPPS"
    "HPPfHHHHHHPPPS"
    "HPPPRRRRRRRPPS"
    "HPPfHHHHHHPPPS"
    "HPPPRRRRRRRPPS"
    "HPPPPPPPPPPPPS"
    "HPPPPPPPPPPPPS"
    "HPPPPPPPPPPPPS"
    "PSSSSSSSSSSSSS";
static const char SL_thumb_h[] =
    "fHHHHHHHHHHHHHP"
    "HPPPPPPPPPPPPPS"
    "HPPPPPPPPPPPPPS"
    "HPPfPfPfPfPPPPS"
    "HPPHRHRHRHRPPPS"
    "HPPHRHRHRHRPPPS"
    "HPPHRHRHRHRPPPS"
    "HPPHRHRHRHRPPPS"
    "HPPHRHRHRHRPPPS"
    "HPPHRHRHRHRPPPS"
    "HPPPRPRPRPRPPPS"
    "HPPPPPPPPPPPPPS"
    "HPPPPPPPPPPPPPS"
    "PSSSSSSSSSSSSSS";
static const char SL_up[] =
    "WWWWWWWWWWWWWF"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFKKFFFFFD"
    "WFFFFKKKKFFFFD"
    "WFFFKKKKKKFFFD"
    "WFFKKKKKKKKFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "FDDDDDDDDDDDDD";
static const char SL_down[] =
    "WWWWWWWWWWWWWF"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFKKKKKKKKFFD"
    "WFFFKKKKKKFFFD"
    "WFFFFKKKKFFFFD"
    "WFFFFFKKFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "FDDDDDDDDDDDDD";
static const char SL_left[] =
    "WWWWWWWWWWWWWF"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFKFFFFD"
    "WFFFFFFKKFFFFD"
    "WFFFFFKKKFFFFD"
    "WFFFFKKKKFFFFD"
    "WFFFFKKKKFFFFD"
    "WFFFFFKKKFFFFD"
    "WFFFFFFKKFFFFD"
    "WFFFFFFFKFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "FDDDDDDDDDDDDD";
static const char SL_right[] =
    "WWWWWWWWWWWWWF"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFKFFFFFFFD"
    "WFFFFKKFFFFFFD"
    "WFFFFKKKFFFFFD"
    "WFFFFKKKKFFFFD"
    "WFFFFKKKKFFFFD"
    "WFFFFKKKFFFFFD"
    "WFFFFKKFFFFFFD"
    "WFFFFKFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "WFFFFFFFFFFFFD"
    "FDDDDDDDDDDDDD";
typedef struct scroll_look_crop {
    const char *name, *pixels;
    int x, y, w, h;
} scroll_look_crop;
static const scroll_look_crop SL_crops[] = {
    {"thumb-v", SL_thumb_v, 608,311,14,15},
    {"thumb-h", SL_thumb_h, 330,401,15,14},
    {"up", SL_up, 608,296,14,14}, {"down", SL_down, 608,386,14,14},
    {"left", SL_left, 315,401,14,14}, {"right", SL_right, 593,401,14,14}
};
static inline uint32_t sl_source_rgb(char c)
{
    switch (c) {
    case 'H': return 0xDADAFF; case 'P': return 0xB3B3FF;
    case 'S': return 0x8787DA; case 'R': return 0x5454B3;
    case 'f': return 0xF3F3F3; case 'W': return 0xFFFFFF;
    case 'F': return 0xE7E7E7; case 'D': return 0xCDCDCD;
    case 'K': return 0; default: return 0xFFFFFFFFu;
    }
}
/* Palette-preserving authored substitution, independent expected values.
 * 8-bit: white / canon teal / existing dark gray. Direct colour: white /
 * canon teal / ratified dark teal. S and R share the dark tone. The geometry
 * and source shade populations remain independently graded (49/102/26/28/5).
 * Ref: assets/color_canon.json entries + derived_rows; scrollbars.md 2.4.
 */
static inline uint32_t sl_mapped_rgb(char c, int bpp)
{
    switch (c) {
    case 'H': return 0xFFFFFF;
    case 'P': return 0x8DDCDC;
    case 'S': case 'R': return bpp == 8 ? 0x3F3F3F : 0x4E9BA3;
    default: return sl_source_rgb(c);
    }
}
#endif
