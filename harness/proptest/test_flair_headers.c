/* test_flair_headers.c -- compile-contract oracle for the FLAIR locked-spec
 * headers (spec/grafport.h, spec/imaging.h) + the frozen canon assets
 * (spec/assets/cursors.h, spec/assets/menu_canon.h).
 *
 * beads: initech-k8o5.3 (GrafPort/imaging spec) + initech-zaqj (canon assets).
 * Ref:   CLAUDE.md Law 2 (oracle is truth), Law 4 (canon is enforced -- the
 *        hourglass cursor is canon, the wristwatch is THE BUG), Rule 6
 *        (mutation-prove), Rule 12 (ASCII-clean). ADR-0004 D-1/D-2 (GrafPort
 *        imaging), D-3/D-7/AM-4 (canon as frozen locked-data).
 *
 * WHY THIS GATE: until a Manager consumer exists, NOTHING in the build includes
 * grafport.h / imaging.h, so their 47 _Static_asserts (the compile-time
 * contract) never fire and a breaking edit would go unnoticed. This gate FORCES
 * those headers to compile (firing every static_assert) and asserts the frozen
 * canon (the hourglass cursor bytes + the Photoshop menu string) at runtime.
 *
 * MUTATION PROOF (Rule 6): built a second time with -DFLAIR_HEADERS_MUTANT, the
 * runtime canon check is perturbed so the comparison MUST fail (RED) -- proving
 * the canon checks bite. The full mutation-proven canon oracle (pie==116,
 * 570- trailing minus, PC LOAD LETTER, etc.) is the separate bead initech-k8o5.10.
 *
 * Single TU, hosted (libc) -- the static_asserts are platform-independent.
 */
#include "grafport.h"
#include "imaging.h"
#include "cursors.h"
#include "menu_canon.h"
#include "event_model.h"     /* beads k8o5.4 -- EventRecord + what codes (D-4) */
#include "window_record.h"   /* beads k8o5.4 -- WindowRecord + FindWindow part-codes */
#include "ssim_params.h"     /* beads k8o5.4 -- SSIM guide params (guide, NOT a gate) */

#include <string.h>
#include <stdio.h>

int main(void)
{
    int checks = 0, fails = 0;

    /* Instantiate every locked type. Merely compiling this TU fires all of the
     * grafport.h + imaging.h _Static_asserts (sizes, offsets, transfer-mode
     * values, field ordering) -- the compile-time half of the contract. */
    GrafPort      port; (void)port;
    FLAIR_BitMap  bm;   (void)bm;
    Pattern       pat;  (void)pat;
    RGBColor      rgb;  (void)rgb;
    flair_point_t pt;   (void)pt;
    EventRecord   ev;   (void)ev;   /* k8o5.4: fires event_model.h static_asserts */
    WindowRecord  win;  (void)win;  /* k8o5.4: fires window_record.h static_asserts */

    /* event/window model constants (verbatim Inside Macintosh values) */
    checks++; if ((int)nullEvent != 0)  { fails++; printf("FAIL nullEvent!=0\n"); }
    checks++; if ((int)mouseDown != 1)  { fails++; printf("FAIL mouseDown!=1\n"); }
    checks++; if ((int)inContent != 3)  { fails++; printf("FAIL inContent!=3\n"); }
    checks++; if ((int)inGoAway != 6)   { fails++; printf("FAIL inGoAway!=6\n"); }

    /* The transfer-mode contract is reachable as constants. */
    checks++; if ((int)srcCopy != 0)     { fails++; printf("FAIL srcCopy!=0\n"); }
    checks++; if ((int)notPatBic != 15)  { fails++; printf("FAIL notPatBic!=15\n"); }
    checks++; if (sizeof(Pattern) != 8u) { fails++; printf("FAIL sizeof(Pattern)!=8\n"); }

    /* --- CANON: the HOURGLASS busy cursor (hand-authored canon, NOT the real
     * System-7 wristwatch -- the wristwatch is THE BUG, Law 4). Solid top/bottom
     * bars (0xFFFF), a 2-pixel waist (0x0180) at rows 7,8. --- */
    {
        uint16_t r0  = FLAIR_CURSOR_BUSY.data[0];
        uint16_t r7  = FLAIR_CURSOR_BUSY.data[7];
        uint16_t r8  = FLAIR_CURSOR_BUSY.data[8];
        uint16_t r15 = FLAIR_CURSOR_BUSY.data[15];
#ifdef FLAIR_HEADERS_MUTANT
        /* Perturb the compared value so the canon assert below MUST fire (RED).
         * We mutate the LOCAL copy, never the locked header data. */
        r7 ^= 0x0001u;
#endif
        checks++; if (r0  != 0xFFFFu) { fails++; printf("FAIL hourglass row0 (top bar)\n"); }
        checks++; if (r7  != 0x0180u) { fails++; printf("FAIL hourglass row7 (waist)\n"); }
        checks++; if (r8  != 0x0180u) { fails++; printf("FAIL hourglass row8 (waist)\n"); }
        checks++; if (r15 != 0xFFFFu) { fails++; printf("FAIL hourglass row15 (bottom bar)\n"); }
        /* mask is the solid silhouette (mask == data for the busy cursor). */
        checks++; if (FLAIR_CURSOR_BUSY.mask[0] != FLAIR_CURSOR_BUSY.data[0]) {
            fails++; printf("FAIL hourglass mask!=data\n");
        }
    }

    /* --- CANON: the frozen Photoshop menu bar (ADR-0004 D-3/AM-4). The exact
     * string is locked; "Layer" (Photoshop 3.0, 1994) and "View" never coexisted
     * in any real Photoshop version -- that impossibility IS the canon. --- */
    checks++;
    if (strcmp(FLAIR_CANON_PHOTOSHOP_MENUBAR,
               "File Edit Image Layer Select View Window Help") != 0) {
        fails++; printf("FAIL Photoshop menu string\n");
    }
    checks++;
    if (strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[3], "Layer") != 0 ||
        strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[5], "View")  != 0) {
        fails++; printf("FAIL Photoshop chimera invariant (Layer@3, View@5)\n");
    }

    printf("test_flair_headers: %d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
