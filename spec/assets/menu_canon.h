/*
 * menu_canon.h -- InitechOS FLAIR canon menu-bar strings v1 (LOCKED spec-data).
 *
 * beads: initech-zaqj (canon assets: hourglass CURS bytes + Photoshop menu
 *        string, ADR-0004 AM-4 / FO-E).
 *
 * Ref: CLAUDE.md Law 4 ("the hourglass cursor (not a wristwatch) ... are
 *      enforced, not fixed"), Law 1 (ground truth before code), Rule 8
 *      (specs are locked data), Rule 11 (byte-stable), Rule 12 (ASCII-clean).
 *      ADR-0004 D-3 ("the menu bar including the Photoshop-exact bar for
 *      InitechPaint ('File Edit Image Layer Select View Window Help') -- canon,
 *      NOT to be 'corrected'").
 *      ADR-0004 AM-4 ("D-3's Photoshop menu bar (File Edit Image Layer Select
 *      View Window Help) ... must be FROZEN locked-data (named spec/assets/
 *      files or named locked constants), not prose an agent may silently
 *      'correct'").
 *      spec/chimera_element_map.json element 9 ("frozen_string_owner:
 *      beads initech-zaqj (ADR-0004 D-3 / AM-4 / Sec 5.2 FO-E)").
 *      docs/research/gui-ground-truth.md Sec 4.3 ("the exact string
 *      'File Edit Image Layer Select View Window Help' is FROZEN locked-data
 *      ... Do NOT 'correct' it to match any real Photoshop version").
 *      PRD Sec 1, Appendix A (the canon frame / the Office Space prop).
 *
 * =========================================================================
 * CANON WARNING -- READ BEFORE ANY EDIT
 * =========================================================================
 *
 * THE PHOTOSHOP MENU-BAR STRING IS THE SPEC, NOT A BUG.
 *
 * The InitechPaint menu bar reads:
 *   "File Edit Image Layer Select View Window Help"
 *
 * This is the deliberate chimera inconsistency from the Office Space prop:
 * the menu is Mac-located (System 7 style, Chicago 12 in the Mac menu bar)
 * but carries a Photoshop-style item set. It is HISTORICALLY IMPOSSIBLE as
 * a single real Photoshop version:
 *   - Photoshop 3.0 for Mac (Sept 1994) was the FIRST version with a "Layer"
 *     menu, but it had "Mode" where the frame shows "View".
 *   - "View" (as a top-level menu) arrived in a later Photoshop point release.
 *   - No single real Photoshop version ever had BOTH "Layer" and "View" in
 *     this exact position simultaneously.
 *
 * The impossible combination IS the canon. InitechOS reproduces the prop's
 * string verbatim. Do NOT "fix" it to match any real Photoshop version.
 * Do NOT add, remove, or reorder items. Do NOT change capitalization.
 *
 * The canon oracle (ADR-0004 D-8 "canon" gate, beads initech-k8o5.10) asserts
 * FLAIR_CANON_PHOTOSHOP_MENUBAR and FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[]
 * against their known values. Any change to these constants will cause the
 * canon oracle to go RED -- which is the correct behavior.
 *
 * Changing these constants is a DELIBERATE, issue-tracked Rule 8 act.
 * Never silently edit to "correct" the menu bar.
 *
 * Ref: spec/chimera_element_map.json element 9; ADR-0004 D-3 / AM-4;
 *      gui-ground-truth.md Sec 4.3; CLAUDE.md Law 4; PRD Appendix A.
 *
 * =========================================================================
 *
 * PROVENANCE (Law 1 honesty): the string below is sourced directly from the
 * canon (the Office Space film prop / the frame still in spec/assets/
 * preview.webp, as analyzed in docs/research/gui-ground-truth.md Sec 4.3
 * and ratified in ADR-0004 D-3). It was NOT derived from any real Adobe
 * Photoshop installation or boot. The canon analysis confirms it is the
 * correct string. Locked 2026-06-19, beads initech-zaqj.
 *
 * Rule 12: ALL strings below are 7-bit ASCII. No smart quotes. No non-ASCII.
 * Rule 11: No timestamps, no host paths baked in.
 */

#ifndef INITECH_MENU_CANON_H
#define INITECH_MENU_CANON_H

/* =========================================================================
 * FLAIR_CANON_PHOTOSHOP_MENUBAR
 *
 * The complete InitechPaint menu bar as a single space-delimited string.
 * This is the FROZEN canon string (ADR-0004 D-3 / AM-4; beads initech-zaqj).
 *
 * Used for: display in the Mac System-7-style menu bar for InitechPaint;
 * asserted byte-for-byte by the canon oracle (D-8, initech-k8o5.10).
 *
 * DO NOT MODIFY. DO NOT "CORRECT" TO A REAL PHOTOSHOP VERSION.
 */
#define FLAIR_CANON_PHOTOSHOP_MENUBAR \
    "File Edit Image Layer Select View Window Help"

/* Total length of the string above, not counting the NUL terminator.
 * Canon oracle uses this to assert no extra bytes were silently appended. */
#define FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN 45

/* =========================================================================
 * FLAIR_CANON_PHOTOSHOP_MENU_ITEMS
 *
 * The individual top-level menu titles as a NULL-terminated array of
 * string literals. This is the canonical decomposition for the Menu Manager
 * (ADR-0004 D-3 MenuInfo items); each entry becomes one MenuInfo record
 * in the Mac-style menu bar.
 *
 * Count: 8 items (FLAIR_CANON_PHOTOSHOP_MENU_COUNT).
 * The array is terminated by a NULL sentinel so iteration is unambiguous.
 *
 * DO NOT MODIFY. DO NOT ADD OR REMOVE ITEMS. DO NOT CHANGE CAPITALIZATION.
 */
#define FLAIR_CANON_PHOTOSHOP_MENU_COUNT 8

/* Individual item name macros (for static assertions and oracle checks): */
#define FLAIR_CANON_MENU_ITEM_0  "File"
#define FLAIR_CANON_MENU_ITEM_1  "Edit"
#define FLAIR_CANON_MENU_ITEM_2  "Image"
#define FLAIR_CANON_MENU_ITEM_3  "Layer"
#define FLAIR_CANON_MENU_ITEM_4  "Select"
#define FLAIR_CANON_MENU_ITEM_5  "View"
#define FLAIR_CANON_MENU_ITEM_6  "Window"
#define FLAIR_CANON_MENU_ITEM_7  "Help"

/* The array: extern declaration for use in non-static compilation units.
 * The definition is provided as a static inline below so the header is
 * self-contained and freestanding-legal (no separate .c needed). */
static const char * const FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[
    FLAIR_CANON_PHOTOSHOP_MENU_COUNT + 1 /* +1 for NULL sentinel */
] = {
    FLAIR_CANON_MENU_ITEM_0,  /* "File"   */
    FLAIR_CANON_MENU_ITEM_1,  /* "Edit"   */
    FLAIR_CANON_MENU_ITEM_2,  /* "Image"  */
    FLAIR_CANON_MENU_ITEM_3,  /* "Layer"  -- Photoshop 3.0 (1994), FIRST
                                *             version with Layer menu      */
    FLAIR_CANON_MENU_ITEM_4,  /* "Select" */
    FLAIR_CANON_MENU_ITEM_5,  /* "View"   -- arrived in a later Photoshop
                                *             point release, NOT in 3.0.
                                *             Both Layer+View simultaneously
                                *             = historically impossible.
                                *             THAT IS THE POINT. CANON.     */
    FLAIR_CANON_MENU_ITEM_6,  /* "Window" */
    FLAIR_CANON_MENU_ITEM_7,  /* "Help"   */
    (const char *)0           /* NULL sentinel -- end of array */
};

/* =========================================================================
 * Canon-assertion macros (for the canon oracle, beads initech-k8o5.10)
 * =========================================================================
 *
 * The oracle includes this header and calls FLAIR_MENU_CANON_ASSERT() to
 * verify:
 *   1. FLAIR_CANON_PHOTOSHOP_MENUBAR matches the expected literal.
 *   2. FLAIR_CANON_PHOTOSHOP_MENU_COUNT == 8.
 *   3. Each item in FLAIR_CANON_PHOTOSHOP_MENU_ITEMS matches the expected
 *      string (by strcmp, not pointer identity).
 *   4. The NULL sentinel is present at index 8.
 *
 * The expected flat string and item-by-item values are fixed above.
 * The oracle does NOT need additional macros here; it uses string
 * comparison against the frozen literals defined above.
 *
 * The key canon invariants (frozen, not to be changed without a Rule 8 act):
 *   - Flat string: "File Edit Image Layer Select View Window Help"
 *   - Item count: 8
 *   - Item 3 is "Layer"  (NOT "Mode", NOT "Filters", NOT anything else)
 *   - Item 5 is "View"   (NOT "Mode", NOT "Filter")
 *   - Item 3 "Layer" AND item 5 "View" BOTH present = the chimera signal
 */
#define FLAIR_CANON_PHOTOSHOP_MENUBAR_EXPECTED \
    "File Edit Image Layer Select View Window Help"

#define FLAIR_CANON_PHOTOSHOP_MENU_COUNT_EXPECTED 8

#endif /* INITECH_MENU_CANON_H */
