/*
 * spec/flair_skins.h -- the FLAIR era/heritage skin registry: a DATA-ONLY VIEW
 *                       over the ONE color canon (LOCKED spec-data, Rule 8).
 *
 * beads: initech-3knt (DEC-10 Platinum base-era registry slice).
 * Ref:   ADR-0004-AMENDMENT-DEC-09 Sec 3.3 ARB-4 + Sec 3.7 D-9/D-9b -- the
 *        flair_skin_t registry VIEW spec. ADR-0006 Sec 4.6 -- the consuming
 *        oracles (test-skin-teal, test-skin-era-frozen, check-win95isms).
 *        ADR-0004-AMENDMENT-DEC-10 D-10.1/D-10.2 -- Platinum is the BASE;
 *        System 7 remains a retained, locked heritage row.
 *
 * WHAT THIS IS (and is NOT). This header is the era/heritage AXIS of FLAIR
 * decoration: a const registry of named color SLOTS, each tagged with an
 * (era_id, heritage_id) pair, that lets the chimera carry a System-8 Platinum
 * BASE skin, a retained locked System-7 heritage skin, and a Win-3.1 peer skin
 * WITHOUT forking the single color authority. It is a VIEW, not a fourth color
 * table: every slot's RGB comes BY INCLUSION from spec/assets/color_canon.h
 * (the INITECH_CANON_*_RGB macros and flair_canon_rgb's own gray-ramp formula).
 * There is ZERO second copy of any canon color value here. Every row remains
 * explicitly era-tagged; DEC-10 changes the default policy, not that discipline.
 *
 * THE TYPE FORBIDS AN ENGINE FORK (ADR-0004-AMENDMENT-DEC-09 D-9). flair_skin_t
 * carries ONLY data: color slots (idx + canon RGB), pattern-byte/metric scalars,
 * and the two era/heritage enum tags. It carries NO function pointers and NO
 * draw code. The proposed ElementDrawTable vtable is KILLED by construction:
 * the mechanism reads skin-> fields as a PARAMETER and NEVER branches on era,
 * so a data record cannot host a second decoration engine.
 *
 * BY-INCLUSION DISCIPLINE (the load-bearing invariant). Each color slot's
 * .rgb field is initialized from a color_canon.h macro -- e.g.
 *   caption_navy = { CIDX_ACCENT, INITECH_CANON_ACCENT_RGB }
 * The ONE win31 accent with no CIDX (btnshadow #808080) is NOT hand-typed
 * either: it is derived from the canon's OWN gray-ramp formula
 * (flair_canon_rgb(idx>=9) == (idx<<16)|(idx<<8)|idx; color_canon.h lines
 * 121-133) applied to its gray-ramp index 0x80, via FLAIR_CANON_GRAY_RGB below.
 * If you find yourself typing a 0xRRGGBB for a slot that HAS a canon macro,
 * STOP -- use the macro (CLAUDE.md Law 2; the by-inclusion grep gate bites).
 *
 * ACCRETION = APPEND, NEVER MUTATE (ADR-0004-AMENDMENT-DEC-09 D-9 P5). The
 * ERA_SYS7_0_1 and ERA_WIN31 rows are LOCKED pre-existing rows. DEC-10 appends
 * ERA_SYS8_PLATINUM as the BASE without mutating either retained row. test-skin-
 * era-frozen computes a digest-of-fields over those two locked rows and asserts
 * it equals a committed constant -- mutating a locked-row field goes RED.
 *
 * FAIL-LOUD RESOLVER (Rule 2; the region.c RGN_FAIL_LOUD idiom). flair_skin_-
 * resolve(era, heritage) is TOTAL: an unknown (era, heritage) pair NEVER returns
 * a wrong-but-plausible row. Hosted it CHECK-fails (abort, the oracle observes a
 * non-zero exit); freestanding it raises #UD (__builtin_trap), which the IDT
 * routes to the panic path (serial register dump + the PC LOAD LETTER banner).
 * The explicit default policy is (FLAIR_DEFAULT_ERA,
 * FLAIR_DEFAULT_HERITAGE) = (ERA_SYS8_PLATINUM, HERITAGE_QUICKDRAW).
 *
 * Freestanding-safe (Law 3): <stdint.h> + color_canon.h only; no libc, no malloc.
 * Dual-compiles under kernel flags and hosted. ASCII-clean (Rule 12).
 * Deterministic / no timestamps (Rule 11).
 */
#ifndef INITECH_SPEC_FLAIR_SKINS_H
#define INITECH_SPEC_FLAIR_SKINS_H

#include <stdint.h>

#include "color_canon.h"   /* THE by-inclusion source: CIDX_*, INITECH_CANON_*_RGB,
                            * flair_canon_rgb (the gray-ramp formula reproduced
                            * by FLAIR_CANON_GRAY_RGB below). */

/* ---------------------------------------------------------------------------
 * FLAIR_CANON_GRAY_RGB(v) -- the canon's OWN gray-ramp formula as a compile-time
 * constant, so a gray slot with no CIDX (win31 btnshadow #808080) is STILL
 * by-inclusion (derived from its index, never a hand-typed RGB literal). This
 * mirrors flair_canon_rgb's idx>=9 branch VERBATIM (color_canon.h:128-132):
 *   return (v << 16) | (v << 8) | v;
 * It is the formula, not a new value: FLAIR_CANON_GRAY_RGB(0x80) == 0x808080,
 * which is exactly flair_canon_rgb(0x80). (A _Static_assert below proves it.)
 * ------------------------------------------------------------------------- */
#define FLAIR_CANON_GRAY_RGB(v) \
    ( ((uint32_t)(v) << 16) | ((uint32_t)(v) << 8) | (uint32_t)(v) )

/* CIDX-equivalent index for the win31 btnshadow gray-ramp shade. NOT a CIDX_*
 * (the canon table is idx 0..8); this is a gray-ramp index (color_canon.h:113,
 * "idx >= 9 -> deterministic gray ramp"). Named so the slot carries an honest
 * index, exactly as the other slots carry a CIDX_*. */
#define FLAIR_GRAY_IDX_808080  0x80u

/* ---------------------------------------------------------------------------
 * era_id -- the heritage ERA axis. Platinum is the BASE per DEC-10 D-10.1;
 * System 7 remains a locked heritage row per D-10.2. The enum integers stay
 * stable (Rule 11 / BC-10.5); explicit default policy is declared below.
 * ------------------------------------------------------------------------- */
typedef enum {
    ERA_SYS7_0_1      = 0,   /* retained System 7.0-7.1 locked heritage row      */
    ERA_WIN31         = 1,   /* Windows 3.1 / GDI peer skin                      */
    ERA_SYS8_PLATINUM = 2,   /* Mac OS 8.0/8.1 Platinum BASE; minted from 8.1    */
    ERA__COUNT               /* sentinel: number of declared eras                */
} era_id;

/* ---------------------------------------------------------------------------
 * heritage_id -- the toolbox heritage axis. The default skin is QuickDraw.
 * ------------------------------------------------------------------------- */
typedef enum {
    HERITAGE_QUICKDRAW = 0,  /* System-7 QuickDraw (DEFAULT)                     */
    HERITAGE_GDI       = 1,  /* Windows 3.1 GDI                                  */
    HERITAGE__COUNT          /* sentinel: number of declared heritages           */
} heritage_id;

/* ---------------------------------------------------------------------------
 * flair_skin_slot_t -- ONE named color slot of a skin: the palette index AND
 * its canon 0x00RRGGBB, the latter ALWAYS supplied by inclusion from
 * color_canon.h (a CIDX_* macro pair, or the gray-ramp formula). Carrying both
 * makes a slot gradeable WITHOUT routing back through the render path (the
 * oracle reads slot.rgb; the canon supplies the expected value independently).
 * PURE DATA: no pointer, no callback. (ADR-0004-AMENDMENT-DEC-09 ARB-4.)
 * ------------------------------------------------------------------------- */
typedef struct {
    uint16_t idx;   /* canon index: a CIDX_* (0..8) or a gray-ramp index (>=9). */
    uint32_t rgb;   /* 0x00RRGGBB, BY INCLUSION from color_canon.h. NO literal.  */
} flair_skin_slot_t;

/* ---------------------------------------------------------------------------
 * flair_skin_t -- the DATA-ONLY era/heritage skin record (the registry VIEW
 * row). Named color slots + scalar pattern/metric fields + the two era/heritage
 * tags. NO function pointers, NO draw code (the TYPE prevents the engine fork,
 * D-9). The mechanism consumes skin-> fields as a PARAMETER; it NEVER branches
 * on era_id/heritage_id and NEVER names a literal RGB.
 * The `struct flair_skin` tag exists only so artifact headers can forward-
 * declare this data row without instantiating the header-only registry; it
 * changes no field, layout, digest input, or locked value.
 *
 * The color slots cover the decoration roles the chimera needs across both
 * skins; a slot a given era does not distinguish is filled from the canon role
 * that era DOES use (e.g. the System-7 row's btnshadow/btnhilight fall back to
 * the canon pinstripe/bevel shades -- still by-inclusion, never a literal).
 * ------------------------------------------------------------------------- */
typedef struct flair_skin {
    /* --- era/heritage tags (the registry KEY). --- */
    uint16_t era;        /* era_id      */
    uint16_t heritage;   /* heritage_id */

    /* --- named color slots (each idx + canon RGB, by-inclusion). --- */
    flair_skin_slot_t desktop;      /* desktop background                       */
    flair_skin_slot_t caption_navy; /* active caption / accent                  */
    flair_skin_slot_t caption_ink;  /* caption text ink                         */
    flair_skin_slot_t menubar;      /* menu-bar background                      */
    flair_skin_slot_t btnface;      /* control / button face                    */
    flair_skin_slot_t btnshadow;    /* control 3-D shadow edge                  */
    flair_skin_slot_t btnhilight;   /* control 3-D highlight edge               */
    flair_skin_slot_t frame_ink;    /* window frame / border ink                */
    flair_skin_slot_t content;      /* window content body                      */
    flair_skin_slot_t bevel_light;  /* title/groove bevel highlight             */
    flair_skin_slot_t bevel_shadow; /* title/groove bevel shadow                */

    /* --- scalar pattern/metric fields (pure data; no color). --- */
    uint8_t  pinstripe_pattern;  /* title-bar racing-stripe dither byte         */
    uint8_t  bevel_width;        /* 3-D bevel ring width, px                    */
} flair_skin_t;

/* ---------------------------------------------------------------------------
 * The LOCKED registry. DEC-10 D-10.1 appends the ERA_SYS8_PLATINUM/QUICKDRAW
 * BASE after the retained locked SYS7 heritage row (D-10.2) and WIN31 peer row.
 * EVERY .rgb is by-inclusion from color_canon.h -- there is no hand-typed canon
 * value below. The era= tags stay explicit on every row.
 *
 * Win-3.1 peer accents (ADR-0004-AMENDMENT-DEC-09 D-9b): caption navy #000080
 * (== CIDX_ACCENT), btnface #C0C0C0 (== CIDX_CONTROL), btnshadow #808080
 * (gray-ramp 0x80), btnhilight #FFFFFF (== CIDX_WHITE), menubar #FFFFFF
 * (== CIDX_WHITE). The flat-2-D Win-3.1 target carries NO Win95 3D-light
 * inner-bevel import (check-win95isms enforces the forbidden-token list; the
 * tokens themselves live ONLY in that scanner, ARB-8 / Law 3).
 * ------------------------------------------------------------------------- */
static const flair_skin_t flair_skin_registry[] = {
    /* ===== ERA_SYS7_0_1 / HERITAGE_QUICKDRAW -- the BASE skin (LOCKED). ===== */
    {
        ERA_SYS7_0_1, HERITAGE_QUICKDRAW,
        /* desktop      */ { CIDX_DESKTOP,    INITECH_CANON_DESKTOP_RGB      },
        /* caption_navy */ { CIDX_ACCENT,     INITECH_CANON_ACCENT_RGB       },
        /* caption_ink  */ { CIDX_TITLE_INK,  INITECH_CANON_TITLE_INK_RGB    },
        /* menubar      */ { CIDX_MENUBAR,    INITECH_CANON_MENUBAR_RGB      },
        /* btnface      */ { CIDX_CONTROL,    INITECH_CANON_CONTROL_RGB      },
        /* btnshadow    */ { CIDX_PIN_DARK,   INITECH_CANON_PIN_DARK_RGB     },
        /* btnhilight   */ { CIDX_PIN_LIGHT,  INITECH_CANON_PIN_LIGHT_RGB    },
        /* frame_ink    */ { CIDX_BLACK,      INITECH_CANON_BLACK_RGB        },
        /* content      */ { CIDX_WHITE,      INITECH_CANON_WHITE_RGB        },
        /* bevel_light  */ { CIDX_DESKTOP,    INITECH_CANON_BEVEL_LIGHT_RGB  },
        /* bevel_shadow */ { CIDX_DESKTOP,    INITECH_CANON_BEVEL_SHADOW_RGB },
        /* pinstripe    */ 0x55u,
        /* bevel_width  */ 1u
    },

    /* ===== ERA_WIN31 / HERITAGE_GDI -- the PEER skin (LOCKED). ============== */
    {
        ERA_WIN31, HERITAGE_GDI,
        /* desktop      */ { CIDX_DESKTOP,    INITECH_CANON_DESKTOP_RGB      },
        /* caption_navy */ { CIDX_ACCENT,     INITECH_CANON_ACCENT_RGB       },
        /* caption_ink  */ { CIDX_WHITE,      INITECH_CANON_WHITE_RGB        },
        /* menubar      */ { CIDX_WHITE,      INITECH_CANON_WHITE_RGB        },
        /* btnface      */ { CIDX_CONTROL,    INITECH_CANON_CONTROL_RGB      },
        /* btnshadow    */ { FLAIR_GRAY_IDX_808080,
                             FLAIR_CANON_GRAY_RGB(FLAIR_GRAY_IDX_808080)     },
        /* btnhilight   */ { CIDX_WHITE,      INITECH_CANON_WHITE_RGB        },
        /* frame_ink    */ { CIDX_BLACK,      INITECH_CANON_BLACK_RGB        },
        /* content      */ { CIDX_WHITE,      INITECH_CANON_WHITE_RGB        },
        /* bevel_light  */ { CIDX_WHITE,      INITECH_CANON_WHITE_RGB        },
        /* bevel_shadow */ { FLAIR_GRAY_IDX_808080,
                             FLAIR_CANON_GRAY_RGB(FLAIR_GRAY_IDX_808080)     },
        /* pinstripe    */ 0x00u,
        /* bevel_width  */ 2u
    }
    ,

    /* ===== ERA_SYS8_PLATINUM / HERITAGE_QUICKDRAW -- the BASE skin. ========= */
    {
        ERA_SYS8_PLATINUM, HERITAGE_QUICKDRAW,
        /* desktop: Initech teal identity.
         * Ref: DEC-10 BC-10.6 / Sec 3.3.1. */
        { CIDX_DESKTOP, INITECH_CANON_DESKTOP_RGB },
        /* caption_navy: navy accent survives verbatim.
         * Ref: DEC-10 Sec 3.3.1. */
        { CIDX_ACCENT, INITECH_CANON_ACCENT_RGB },
        /* caption_ink: active title ink #000000.
         * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 2.3. */
        { CIDX_TITLE_INK, INITECH_CANON_TITLE_INK_RGB },
        /* menubar: sampled #E7E7E7 face (nominal #DDDDDD is provenance).
         * Ref: ../system7-decomp/specs/sys8/platinum-palette.md Sec 2;
         * ../system7-decomp/specs/sys8/menus.md Sec 1.1. */
        { CIDX_PLAT_FACE, FLAIR_CANON_GRAY_RGB(CIDX_PLAT_FACE) },
        /* btnface: sampled #E7E7E7 dialog/control/push-button face.
         * Ref: ../system7-decomp/specs/sys8/platinum-palette.md Sec 2. */
        { CIDX_PLAT_FACE, FLAIR_CANON_GRAY_RGB(CIDX_PLAT_FACE) },
        /* btnshadow: sampled #A5A5A5 widget edge/interior shadow; nominal
         * #888888 is provenance only.
         * Ref: ../system7-decomp/specs/sys8/platinum-palette.md Sec 2. */
        { CIDX_PLAT_WIDGET_EDGE, FLAIR_CANON_GRAY_RGB(CIDX_PLAT_WIDGET_EDGE) },
        /* btnhilight: every Platinum 3-D highlight is #FFFFFF.
         * Ref: ../system7-decomp/specs/sys8/platinum-palette.md Sec 2. */
        { CIDX_WHITE, INITECH_CANON_WHITE_RGB },
        /* frame_ink: active frame lines are #000000.
         * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 1. */
        { CIDX_BLACK, INITECH_CANON_BLACK_RGB },
        /* content: Finder content field is white.
         * Ref: ../system7-decomp/specs/sys8/platinum-palette.md Sec 2. */
        { CIDX_WHITE, INITECH_CANON_WHITE_RGB },
        /* bevel_light: lavender accent becomes same-hue Initech teal, using
         * the same canon slot as SYS7. Ref: DEC-10 Sec 6 OQ-2 (operator,
         * 2026-08-16). */
        { CIDX_DESKTOP, INITECH_CANON_BEVEL_LIGHT_RGB },
        /* bevel_shadow: same OQ-2 substitution and SYS7 canon slot.
         * Ref: DEC-10 Sec 6 OQ-2 (operator, 2026-08-16). */
        { CIDX_DESKTOP, INITECH_CANON_BEVEL_SHADOW_RGB },
        /* pinstripe_pattern: 12 row-alternating racing stripes, LIGHT first at
         * stripe row 0; exact structure is graded by chrome oracles, not this
         * scalar. Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 2.2. */
        0x55u,
        /* bevel_width: 4-px raised body frame (SYS7 uses 1 px).
         * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 4. */
        4u
    }
};

#define FLAIR_SKIN_REGISTRY_COUNT \
    ( (uint16_t)(sizeof(flair_skin_registry) / sizeof(flair_skin_registry[0])) )

/* Explicit default policy (DEC-10 D-10.1 / D-10.6 Option B). The enum values
 * remain stable; the default is never inferred from numeric or row position. */
#define FLAIR_DEFAULT_ERA       ERA_SYS8_PLATINUM
#define FLAIR_DEFAULT_HERITAGE  HERITAGE_QUICKDRAW

/* By-inclusion proof (compile-time): the gray-ramp slot equals the canon's OWN
 * gray-ramp formula for 0x80; i.e. FLAIR_CANON_GRAY_RGB is the SAME formula as
 * flair_canon_rgb's idx>=9 branch (color_canon.h:128-132), not a new literal. */
_Static_assert(FLAIR_CANON_GRAY_RGB(FLAIR_GRAY_IDX_808080) == 0x808080u,
               "win31 btnshadow gray-ramp slot == flair_canon_rgb(0x80) by formula");
_Static_assert(FLAIR_DEFAULT_ERA == ERA_SYS8_PLATINUM,
               "DEC-10 D-10.1: Platinum is the default era");
_Static_assert(FLAIR_DEFAULT_HERITAGE == HERITAGE_QUICKDRAW,
               "DEC-10 D-10.1: QuickDraw is the default heritage");
_Static_assert(ERA_SYS7_0_1 == 0 && ERA_WIN31 == 1 && ERA_SYS8_PLATINUM == 2,
               "BC-10.5 / Rule 11: era enum integer values are stable");

/* ---------------------------------------------------------------------------
 * FLAIR_SKIN_FAIL_LOUD() -- the region.c idiom (Rule 2). Hosted: abort() (the
 * oracle observes a non-zero exit / CHECK-fail). Freestanding: __builtin_trap()
 * raises #UD, which the IDT routes to the panic path (serial register dump + the
 * PC LOAD LETTER banner) and halts. No panic.c pull-in, so the header is purely
 * host-testable. (Mirrors os/flair/atkinson/region.c:41-46.)
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#include <stdlib.h>            /* abort */
#define FLAIR_SKIN_FAIL_LOUD() abort()
#else
#define FLAIR_SKIN_FAIL_LOUD() __builtin_trap()
#endif

/* ---------------------------------------------------------------------------
 * flair_skin_resolve(era, heritage) -- TOTAL + FAIL-LOUD lookup of the registry
 * row for an (era, heritage) pair. An unknown pair FAILS LOUD; it NEVER returns
 * a wrong-but-plausible row (the region engine's normalize-or-panic posture).
 * The default is the explicit FLAIR_DEFAULT_ERA/FLAIR_DEFAULT_HERITAGE policy,
 * not enum zero or registry position (DEC-10 D-10.6 Option B).
 *
 * Returns a const pointer into the locked registry (no allocation, no copy).
 * Freestanding-safe (no libc on the success path).
 * ------------------------------------------------------------------------- */
static inline const flair_skin_t *flair_skin_resolve(int era, int heritage)
{
    uint16_t i;
    for (i = 0; i < FLAIR_SKIN_REGISTRY_COUNT; i++) {
        if ((int)flair_skin_registry[i].era == era &&
            (int)flair_skin_registry[i].heritage == heritage) {
            return &flair_skin_registry[i];
        }
    }
    /* Unknown (era, heritage): a TOTAL resolver fails loud rather than guess
     * (Rule 2). On metal this renders PC LOAD LETTER; hosted it aborts. */
    FLAIR_SKIN_FAIL_LOUD();
    return (const flair_skin_t *)0;  /* unreachable; keeps -Werror happy. */
}

static inline const flair_skin_t *flair_skin_default(void)
{ return flair_skin_resolve(FLAIR_DEFAULT_ERA, FLAIR_DEFAULT_HERITAGE); }

#endif /* INITECH_SPEC_FLAIR_SKINS_H */
