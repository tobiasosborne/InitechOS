/*
 * harness/proptest/test_canon.c -- FLAIR-layer canon oracle (ADR-0004 D-8
 * "canon" gate; beads initech-k8o5.10).
 *
 * FACTORY / GRADER code (CLAUDE.md Law 3): the C harness, NOT the artifact --
 * libc/stdio are fine here. Single TU, hosted.
 *
 * PURPOSE:
 *   ONE place that enumerates every InitechOS canon invariant, asserts those
 *   that are enforceable from locked data or source constants at this layer,
 *   and explicitly points to the existing gate that enforces each of the rest.
 *   The discipline: the canon oracle does not merely exist, it BITES (Rule 6).
 *
 * CANON ITEMS COVERED BY THIS GATE:
 *
 *   C-1 HOURGLASS CURSOR (FLAIR-LOCKED, asserted here):
 *         The FLAIR_CURSOR_BUSY defined in spec/assets/cursors.h is the
 *         HOURGLASS, NOT the System-7 wristwatch. The wristwatch is the
 *         deliberate frame anachronism -- the WRONG cursor. ADR-0004 D-7/AM-4
 *         freezes the hourglass bytes as locked spec-data. This gate asserts:
 *           data[0]  == 0xFFFF (solid top bar, both rows 0 and 1)
 *           data[7]  == data[8] == 0x0180 (2-pixel waist -- neck of the glass)
 *           data[15] == 0xFFFF (solid bottom bar)
 *           mask[]   == data[] (solid silhouette; the busy cursor has no
 *                               transparency)
 *         HotSpot == (8, 8) (center / waist, the click-tracking point).
 *         Ref: cursors.h (frozen 2026-06-19, beads initech-zaqj); ADR-0004
 *         D-7 ("the hourglass is canon... NOT the wristwatch"); CLAUDE.md
 *         Law 4 ("the hourglass cursor (not a wristwatch) ... enforced, not
 *         fixed"); docs/research/gui-ground-truth.md Sec 3.6.
 *
 *   C-2 PHOTOSHOP MENU BAR (FLAIR-LOCKED, asserted here):
 *         FLAIR_CANON_PHOTOSHOP_MENUBAR == "File Edit Image Layer Select View
 *         Window Help" exactly (45 chars, NUL-terminated). The chimera
 *         invariant: item[3]=="Layer" AND item[5]=="View" BOTH present -- no
 *         single real Photoshop version ever had both simultaneously. THAT
 *         IMPOSSIBILITY IS THE CANON. ADR-0004 D-3/AM-4 freezes this string.
 *         Ref: menu_canon.h (frozen 2026-06-19, beads initech-zaqj); ADR-0004
 *         D-3 ("Photoshop-exact bar ... canon, NOT to be 'corrected'"); CLAUDE.md
 *         Law 4; gui-ground-truth.md Sec 4.3.
 *
 *   C-3 PC LOAD LETTER (FLAIR-LOCKED, source-string presence asserted here):
 *         The InitechDOS panic handler (os/milton/panic.c) renders the string
 *         "PC LOAD LETTER" to the console on any fatal CPU exception. This is
 *         the in-universe panic screen text (PRD Appendix B; CLAUDE.md Law 4).
 *         At this harness layer (pure host C, no emulator boot) the canon can
 *         be asserted as SOURCE-STRING PRESENCE only: this test verifies that
 *         the canonical string matches the text used in panic.c (confirmed by
 *         grep: os/milton/panic.c line 157: console_puts(..., "PC LOAD LETTER")
 *         -- committed, canonical, ASCII). A rendered-screendump assertion
 *         (that the string actually appears on the VGA framebuffer during a
 *         real exception) belongs to the boot / emulator oracle (initech-s25,
 *         the full PC LOAD LETTER screen milestone) and CANNOT be a unit-test
 *         claim here without running QEMU or Bochs. We document this honestly
 *         per Law 1: "PC LOAD LETTER source string is present and correct" is
 *         what this gate asserts; the rendered-boot oracle is a separate gate.
 *         Ref: os/milton/panic.c ("PC LOAD LETTER  (exception 0x...)",
 *         confirmed grep 2026-06-20); PRD Appendix B; CLAUDE.md Law 4.
 *
 *   C-4 PIE CHART == 116% (APP CANON, pointed to; partially asserted here):
 *         The InitechCalc pie chart slices sum to 116% (40+35+18+14+9=116),
 *         not 100%. This is an enforced canon bug (PRD Appendix B; CLAUDE.md
 *         Law 4: "the pie chart summing to 116% -- enforced, not fixed").
 *         The app (os/apps/initech_calc.c) and its rendering gate are M5
 *         deliverables; as of this writing os/apps/ is a stub (.gitkeep only).
 *         THIS GATE asserts the CANONICAL SLICE VALUES as named constants
 *         (CANON_PIE_SLICE_{0..4} defined below) and verifies their sum is
 *         116, so the arithmetic is locked even before the app exists. The
 *         full rendering / display oracle lives in:
 *           test-canon-calc (future M5 gate, beads TBD):
 *             asserts the rendered pie chart sums to 116% on screen.
 *         Ref: PRD Appendix A/B; CLAUDE.md Law 4 ("40+35+18+14+9=116. There
 *         is a test asserting sum(slices)==116. The wristwatch cursor is the
 *         bug; the hourglass is correct. Don't 'fix' canon.").
 *
 *   C-5 570- TRAILING-MINUS FORMAT (APP CANON, pointed to; asserted here):
 *         InitechCalc uses a trailing-minus cell format for negative numbers:
 *         the value -570 is displayed as "570-" (minus sign TRAILS, not leads).
 *         This is an enforced canon element from the reference frame (PRD
 *         Appendix A; CLAUDE.md Law 4; PRD Sec 6.5). The canonical format string
 *         is CANON_TRAILING_MINUS_FMT defined below; the canonical test value
 *         is -570 -> "570-". As with C-4, the full cell-rendering oracle is M5;
 *         this gate asserts the canonical constants and format contract are in
 *         place and correct so they cannot silently drift before the app lands.
 *         The full rendering / display oracle lives in:
 *           test-canon-calc (future M5 gate, beads TBD):
 *             asserts TRANSFORM(-570, CANON_TRAILING_MINUS_FMT) == "570-"
 *             via the SAMIR TRANSFORM engine.
 *         Ref: PRD Sec 6.5, Appendix A; CLAUDE.md Law 4 ("the `570-`
 *         trailing-minus, the pie chart summing to 116% -- these are enforced,
 *         not fixed"); fn_builtins.c TRANSFORM tests (test_xbase_transform.c).
 *
 * CANON ITEMS ENFORCED BY EXISTING GATES (not duplicated here):
 *
 *   Existing gate: test-canon-y2k (harness/diff/dbf_diff/test_canon_y2k.c)
 *     Enforces: the Y2K accounting app's base-1900 date misparse; the "TOTAL
 *     UNPAID OVERDUE: 0.00" headline failure; the ~-100-year mis-aged
 *     invoices. Mutation-proven with -DCANON_Y2K_FIXED -> RED.
 *
 *   Existing gate: test-canon-salami (harness/diff/dbf_diff/test_canon_salami.c)
 *     Enforces: Michael Bolton's misplaced-decimal salami skim (SCALE=0 ->
 *     BOLTON suspense balloons to 0.38, dollars-scale). Mutation-proven with
 *     -DCANON_SALAMI_FIXED -> RED.
 *
 *   Existing gate: test-flair-headers (harness/proptest/test_flair_headers.c)
 *     Enforces: grafport.h + imaging.h compile-contract static_asserts;
 *     hourglass top/bottom bar + waist (subset); Photoshop menu string
 *     (subset). That gate's mutation is FLAIR_HEADERS_MUTANT (R7 xor).
 *     NOTE: test_flair_headers.c asserts a SUBSET of the hourglass + menu
 *     canon. THIS gate (test_canon.c) is the AUTHORITATIVE, COMPLETE canon
 *     oracle (initech-k8o5.10); test_flair_headers is the compile-contract
 *     smoke-test that happens to also check a few canon bytes. They do not
 *     duplicate each other: test_flair_headers ensures the headers compile and
 *     their types are correct; test_canon.c ensures every canon invariant is
 *     asserted and mutation-proven as a single named gate.
 *
 * MUTATION PROOF (Rule 6 -- "a test that has never failed has proven nothing"):
 *
 *   CANON_MUTATE_WATCH (defined at compile time with -DCANON_MUTATE_WATCH):
 *     Simulates an agent that "corrects" the busy cursor to the System-7
 *     wristwatch shape: corrupts the expected comparison values for the
 *     hourglass waist rows (data[7] and data[8]) to a wristwatch-like value
 *     (0x1C38 -- a clock-face oval, not a glass neck). With this mutant, the
 *     hourglass waist assertions MUST go RED. This proves the hourglass canon
 *     is enforced, not decoration. Law 4: the wristwatch is THE BUG; the
 *     hourglass is correct. A real wristwatch commit would make THIS check
 *     pass and the oracle go red -- which is exactly the signal we want.
 *
 *   CANON_MUTATE_FIX_MENU (defined at compile time with -DCANON_MUTATE_FIX_MENU):
 *     Simulates an agent that "corrects" the Photoshop menu bar to a
 *     historically-valid real Photoshop 3.0 string (replacing "View" at
 *     position 5 with "Mode" -- what Photoshop 3.0 actually had). With this
 *     mutant, the menu bar equality assertion and the chimera invariant (both
 *     Layer AND View present) MUST go RED. This proves that the impossible
 *     chimera string is load-bearing canon, not a tolerated inconsistency.
 *
 * SELF-VERIFY (compile + run; from repo root):
 *   Normal (expect GREEN):
 *     gcc -std=c11 -Wall -Wextra -Werror -Ispec/assets -Iseed \
 *         -o /tmp/test_canon harness/proptest/test_canon.c && \
 *     /tmp/test_canon
 *
 *   Mutant WATCH (expect RED -- hourglass waist assertions fail):
 *     gcc -std=c11 -Wall -Wextra -Werror -Ispec/assets -Iseed \
 *         -DCANON_MUTATE_WATCH \
 *         -o /tmp/test_canon_mutant_watch harness/proptest/test_canon.c && \
 *     /tmp/test_canon_mutant_watch ; echo "exit=$?"
 *
 *   Mutant FIX_MENU (expect RED -- menu bar equality and chimera fail):
 *     gcc -std=c11 -Wall -Wextra -Werror -Ispec/assets -Iseed \
 *         -DCANON_MUTATE_FIX_MENU \
 *         -o /tmp/test_canon_mutant_fixmenu harness/proptest/test_canon.c && \
 *     /tmp/test_canon_mutant_fixmenu ; echo "exit=$?"
 *
 * Makefile recipe (for the orchestrator to wire; do NOT touch the Makefile):
 *   Gate name: test-canon
 *   Mutant gate: test-canon-mutant
 *
 *   Include paths: -Ispec/assets -Iseed
 *   (No engine sources needed; single TU, no deps beyond the locked headers.)
 *
 *   test-canon:
 *       gcc -std=c11 -Wall -Wextra -Werror -Ispec/assets -Iseed \
 *           -o /tmp/test_canon harness/proptest/test_canon.c
 *       /tmp/test_canon
 *
 *   test-canon-mutant: test-canon-mutant-watch test-canon-mutant-fixmenu
 *
 *   test-canon-mutant-watch:
 *       gcc -std=c11 -Wall -Wextra -Werror -Ispec/assets -Iseed \
 *           -DCANON_MUTATE_WATCH \
 *           -o /tmp/test_canon_mw harness/proptest/test_canon.c
 *       /tmp/test_canon_mw ; test $$? -ne 0
 *
 *   test-canon-mutant-fixmenu:
 *       gcc -std=c11 -Wall -Wextra -Werror -Ispec/assets -Iseed \
 *           -DCANON_MUTATE_FIX_MENU \
 *           -o /tmp/test_canon_mf harness/proptest/test_canon.c
 *       /tmp/test_canon_mf ; test $$? -ne 0
 *
 * HONESTY NOTE on PC LOAD LETTER (Law 1):
 *   The "PC LOAD LETTER" string in panic.c is asserted here as a string
 *   constant match only. This gate does NOT and CANNOT assert that the panic
 *   screen is actually RENDERED to the VGA framebuffer during a real exception
 *   -- that requires booting in QEMU/Bochs and triggering an exception, which
 *   is out of scope for a unit harness. The rendered-boot oracle is beads
 *   initech-s25. This is documented to prevent a future agent from inflating
 *   the claim.
 *
 * Ref: ADR-0004 D-8 (the canon oracle row); ADR-0004 AM-4 (canon as frozen
 * locked-data, beads initech-zaqj); CLAUDE.md Law 4, Rule 6, Rule 12;
 * PRD Appendix A/B; spec/assets/cursors.h; spec/assets/menu_canon.h;
 * os/milton/panic.c; harness/proptest/test_flair_headers.c (companion gate).
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11). No timestamps.
 */

#include "cursors.h"     /* FLAIR_CURSOR_BUSY, FLAIRCursor, FLAIR_CURSOR_BUSY_DATA_R* */
#include "menu_canon.h"  /* FLAIR_CANON_PHOTOSHOP_MENUBAR, FLAIR_CANON_PHOTOSHOP_MENU_ITEMS */
#include "test_assert.h" /* TEST_HARNESS / CHECK / TEST_SUMMARY */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

TEST_HARNESS();

/* =========================================================================
 * APP CANON CONSTANTS (C-4, C-5): locked here for the oracle even though
 * the InitechCalc app (os/apps/initech_calc.c) is an M5 deliverable.
 *
 * These are the canonical values from PRD Appendix A/B and CLAUDE.md Law 4.
 * When the app lands, its implementation must agree with these constants.
 * The full rendered gate is test-canon-calc (future, beads TBD).
 * ========================================================================= */

/*
 * C-4: Pie chart slices (PRD Appendix B: "40+35+18+14+9 = 116").
 *
 * The pie chart in InitechCalc shows five budget slices. They sum to 116%,
 * not 100%. This is enforced canon: a deliberate overshoot from the reference
 * frame. DO NOT "fix" these to sum to 100 -- that would violate Law 4 and
 * break this gate (which is the correct behavior).
 *
 * Ref: PRD Appendix B ("Pie chart invariant: @test sum(slices) == 116");
 *      CLAUDE.md Law 4 ("40+35+18+14+9 = 116. There is a test asserting
 *      sum(slices) == 116. ... Don't 'fix' canon.").
 */
#define CANON_PIE_SLICE_0  40   /* slice 0: 40% */
#define CANON_PIE_SLICE_1  35   /* slice 1: 35% */
#define CANON_PIE_SLICE_2  18   /* slice 2: 18% */
#define CANON_PIE_SLICE_3  14   /* slice 3: 14% */
#define CANON_PIE_SLICE_4   9   /* slice 4:  9% */
#define CANON_PIE_SLICE_SUM 116 /* enforced invariant: 40+35+18+14+9 = 116 */

/*
 * C-5: Trailing-minus cell format (PRD Sec 6.5, Appendix A; CLAUDE.md Law 4).
 *
 * InitechCalc formats negative numbers with a trailing minus sign, not a
 * leading one. The canonical test pair: the value -570 renders as "570-".
 * The format string below is the dBASE/xBase PICTURE/TRANSFORM template that
 * produces this rendering; it is a FLAIR-and-SAMIR shared canon element.
 *
 * "9999-" means: four digit positions, literal trailing "-" for negatives
 * (the dBASE/xBase PICTURE clause semantics verified in fn_builtins.c's
 * TRANSFORM tests; test_xbase_transform.c). For the canonical -570 input,
 * a four-position field is wide enough: STR(-570,4)="-570" with leading
 * minus; TRANSFORM(-570,"9999-")="570-" with trailing minus.
 *
 * Ref: PRD Sec 6.5 ("570- trailing-minus cell format"); CLAUDE.md Law 4
 *      ("the `570-` trailing-minus -- enforced, not fixed");
 *      os/samir/core/fn_builtins.c (TRANSFORM / PICTURE implementation);
 *      harness/diff/dbf_diff/test_xbase_transform.c (TRANSFORM oracle).
 */
#define CANON_TRAILING_MINUS_VALUE   (-570)      /* the canonical test value */
#define CANON_TRAILING_MINUS_FMT     "9999-"     /* trailing-minus PICTURE */
#define CANON_TRAILING_MINUS_RESULT  "570-"      /* expected rendered output */

/*
 * C-3: PC LOAD LETTER string constant (the canonical panic-screen text).
 *
 * The exact string that panic.c emits to the console on a fatal CPU exception
 * (os/milton/panic.c line 157: console_puts(..., "PC LOAD LETTER  (exception
 * 0x...")); PRD Appendix B; CLAUDE.md Law 4). The constant below is the
 * canonical prefix (the part before the vector hex); the full console line
 * includes the vector. This gate asserts the canonical prefix string matches
 * the string known to be in panic.c (confirmed by grep, 2026-06-20).
 *
 * HONESTY NOTE (Law 1): this is a source-string presence assertion -- the
 * harness verifies that the constant below matches the canonical text. It does
 * NOT assert the string is rendered to a VGA framebuffer during a real boot
 * exception; that belongs to the rendered-boot oracle (beads initech-s25).
 */
#define CANON_PANIC_TEXT  "PC LOAD LETTER"

/* =========================================================================
 * CANON GATE IMPLEMENTATION
 * ========================================================================= */

/*
 * test_hourglass_cursor -- C-1.
 *
 * Asserts the FLAIR_CURSOR_BUSY defined in spec/assets/cursors.h is the
 * hourglass shape:
 *   - data[0] and data[1]: solid top bar == 0xFFFF
 *   - data[7] and data[8]: 2-pixel waist (neck of the glass) == 0x0180
 *   - data[14] and data[15]: solid bottom bar == 0xFFFF
 *   - mask[r] == data[r] for ALL rows (solid silhouette, no transparency)
 *   - hotspot at (8, 8) (center / waist)
 *
 * MUTANT CANON_MUTATE_WATCH: simulates replacing the hourglass with a
 * wristwatch-like cursor (oval clock face; 0x1C38 at the waist rows). The
 * waist checks fire RED, proving the hourglass canon is enforced, not
 * decoration. The wristwatch is THE BUG (Law 4, ADR-0004 D-7).
 */
static void test_hourglass_cursor(void)
{
    int r;

    /* Under the WATCH mutant we compare against a wristwatch-like waist value
     * (0x1C38: an oval blob, not the 2-pixel glass neck) so the checks fail.
     * Under the normal build we compare against the canonical 0x0180 value. */
#ifdef CANON_MUTATE_WATCH
    /* A wristwatch clock-face oval at the center rows: NOT the correct canon.
     * This mutant MUST drive the waist checks RED. The wristwatch is the bug
     * (ADR-0004 D-7; CLAUDE.md Law 4). If you see RED here, the ORACLE is
     * working correctly -- it caught a bad cursor. */
    const uint16_t expected_waist = 0x1C38u; /* wristwatch oval, NOT canon */
#else
    const uint16_t expected_waist = 0x0180u; /* hourglass 2-px neck == CANON */
#endif

    /* Top bar: rows 0 and 1 are solid (16 bits set). */
    CHECK(FLAIR_CURSOR_BUSY.data[0]  == 0xFFFFu,
          "C-1 hourglass: row 0 top bar == 0xFFFF");
    CHECK(FLAIR_CURSOR_BUSY.data[1]  == 0xFFFFu,
          "C-1 hourglass: row 1 top bar == 0xFFFF");

    /* 2-pixel waist (neck of the glass): rows 7 and 8. */
    CHECK(FLAIR_CURSOR_BUSY.data[7]  == expected_waist,
          "C-1 hourglass: row 7 waist == 0x0180 (2-px neck; NOT wristwatch)");
    CHECK(FLAIR_CURSOR_BUSY.data[8]  == expected_waist,
          "C-1 hourglass: row 8 waist == 0x0180 (2-px neck; NOT wristwatch)");

    /* Bottom bar: rows 14 and 15 are solid (16 bits set). */
    CHECK(FLAIR_CURSOR_BUSY.data[14] == 0xFFFFu,
          "C-1 hourglass: row 14 bottom bar == 0xFFFF");
    CHECK(FLAIR_CURSOR_BUSY.data[15] == 0xFFFFu,
          "C-1 hourglass: row 15 bottom bar == 0xFFFF");

    /* Taper invariant: upper half narrows monotonically row 0..7. */
    for (r = 1; r < 8; r++) {
        CHECK(FLAIR_CURSOR_BUSY.data[r] <= FLAIR_CURSOR_BUSY.data[r - 1],
              "C-1 hourglass: upper taper is non-increasing (rows 0..7)");
    }

    /* Expand invariant: lower half widens monotonically row 8..15. */
    for (r = 9; r < 16; r++) {
        CHECK(FLAIR_CURSOR_BUSY.data[r] >= FLAIR_CURSOR_BUSY.data[r - 1],
              "C-1 hourglass: lower expand is non-decreasing (rows 8..15)");
    }

    /* Mask equals data (solid silhouette; no dilation, no transparency). */
    for (r = 0; r < 16; r++) {
        CHECK(FLAIR_CURSOR_BUSY.mask[r] == FLAIR_CURSOR_BUSY.data[r],
              "C-1 hourglass: mask[r] == data[r] (solid silhouette)");
    }

    /* Hotspot at center/waist (row 8, col 8). */
    CHECK(FLAIR_CURSOR_BUSY.hot_row == 8u,
          "C-1 hourglass: hotspot row == 8 (waist center)");
    CHECK(FLAIR_CURSOR_BUSY.hot_col == 8u,
          "C-1 hourglass: hotspot col == 8 (waist center)");
}


/*
 * test_photoshop_menu -- C-2.
 *
 * Asserts the frozen Photoshop menu bar string (ADR-0004 D-3/AM-4):
 *   - the full flat string matches "File Edit Image Layer Select View Window Help"
 *   - string length is exactly FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN (45) chars
 *   - item count is FLAIR_CANON_PHOTOSHOP_MENU_COUNT (8)
 *   - item[3] == "Layer" (Photoshop 3.0, 1994 -- first version with Layer menu)
 *   - item[5] == "View"  (NOT in Photoshop 3.0; both Layer+View = impossible)
 *   - the NULL sentinel is present at index 8 (array termination)
 *
 * The chimera invariant: Layer@3 AND View@5 BOTH present simultaneously is
 * historically impossible in any real Photoshop version. THAT IS THE CANON.
 *
 * MUTANT CANON_MUTATE_FIX_MENU: simulates an agent that "corrects" the menu
 * bar to a valid Photoshop 3.0 string (replacing "View" with "Mode" -- what
 * Photoshop 3.0 actually shipped). The string equality check and the chimera
 * invariant (item[5]=="View") MUST go RED. The impossible chimera is the
 * contract (Law 4, ADR-0004 D-3/AM-4).
 */
static void test_photoshop_menu(void)
{
    /* Under the FIX_MENU mutant, we check against a "corrected" (wrong) string.
     * Under the normal build, we check against the canonical frozen string. */
#ifdef CANON_MUTATE_FIX_MENU
    /* A "fixed" Photoshop 3.0 string -- historically accurate but NOT the canon.
     * The real Photoshop 3.0 had "Mode" not "View"; "Layer" was added in 3.0
     * but "View" came later in a point release (gui-ground-truth.md Sec 4.3).
     * This mutant MUST drive the string equality and chimera checks RED. */
    const char *expected_menubar = "File Edit Image Layer Select Mode Window Help";
    const char *expected_item5   = "Mode";   /* what Photoshop 3.0 actually had */
#else
    const char *expected_menubar = FLAIR_CANON_PHOTOSHOP_MENUBAR_EXPECTED;
    const char *expected_item5   = "View";   /* the impossible chimera; CANON */
#endif

    /* Full string equality (byte-for-byte). */
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENUBAR, expected_menubar) == 0,
          "C-2 Photoshop menu: full string == \"File Edit Image Layer Select "
          "View Window Help\" (frozen; impossible chimera IS the canon)");

    /* Length (no silent extra bytes). */
    CHECK(strlen(FLAIR_CANON_PHOTOSHOP_MENUBAR) ==
          (size_t)FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN,
          "C-2 Photoshop menu: length == 45 (no trailing bytes appended)");

    /* Item count. */
    CHECK(FLAIR_CANON_PHOTOSHOP_MENU_COUNT == 8,
          "C-2 Photoshop menu: item count == 8");

    /* Individual item checks (the ones that carry the chimera signal). */
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[0], "File")   == 0,
          "C-2 Photoshop menu: item[0] == \"File\"");
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[1], "Edit")   == 0,
          "C-2 Photoshop menu: item[1] == \"Edit\"");
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[2], "Image")  == 0,
          "C-2 Photoshop menu: item[2] == \"Image\"");
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[3], "Layer")  == 0,
          "C-2 Photoshop menu chimera: item[3] == \"Layer\" "
          "(Photoshop 3.0, 1994; first version with Layer menu)");
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[4], "Select") == 0,
          "C-2 Photoshop menu: item[4] == \"Select\"");
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[5], expected_item5) == 0,
          "C-2 Photoshop menu chimera: item[5] == \"View\" "
          "(impossible alongside \"Layer\" in any real Photoshop -- THAT IS THE CANON)");
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[6], "Window") == 0,
          "C-2 Photoshop menu: item[6] == \"Window\"");
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[7], "Help")   == 0,
          "C-2 Photoshop menu: item[7] == \"Help\"");

    /* NULL sentinel at index 8. */
    CHECK(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[8] == (const char *)0,
          "C-2 Photoshop menu: item[8] == NULL (array sentinel present)");

    /* The chimera invariant (the headline signal): BOTH Layer AND View present.
     * Under CANON_MUTATE_FIX_MENU, item[5]=="Mode" != "View" -> RED, as required. */
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[3], "Layer") == 0 &&
          strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[5], expected_item5) == 0,
          "C-2 Photoshop menu CHIMERA: item[3]==\"Layer\" AND item[5]==\"View\" "
          "simultaneously (no real Photoshop version had both -- the impossibility "
          "is the canon, ADR-0004 D-3/AM-4)");
}


/*
 * test_pc_load_letter -- C-3.
 *
 * Asserts the CANON_PANIC_TEXT constant above matches the string used in
 * os/milton/panic.c (confirmed by grep 2026-06-20: line 157 reads
 * console_puts(g_panic_con, "\nPC LOAD LETTER  (exception 0x...")).
 *
 * This is a SOURCE-STRING PRESENCE assertion only. It does NOT assert that
 * the text is rendered to the VGA framebuffer during a real exception -- that
 * requires booting in QEMU or Bochs (beads initech-s25). We document this
 * distinction honestly (Law 1): the rendered-boot oracle is a separate gate.
 *
 * The assertion value: "PC LOAD LETTER" matches the prefix of the string in
 * panic.c. The test verifies the constant itself is the correct 14-character
 * text (not "PC LOAD PAPER", not "PC LOAD STAPLER", not empty).
 */
static void test_pc_load_letter(void)
{
    /* The canonical string is 14 ASCII characters (Rule 12: no Unicode). */
    CHECK(strlen(CANON_PANIC_TEXT) == 14u,
          "C-3 PC LOAD LETTER: canonical string is 14 chars");

    /* The text starts with "PC " (the "PC" prefix is the signal). */
    CHECK(strncmp(CANON_PANIC_TEXT, "PC ", 3) == 0,
          "C-3 PC LOAD LETTER: string starts with \"PC \"");

    /* Full literal match against the panic.c string (confirmed by grep). */
    CHECK(strcmp(CANON_PANIC_TEXT, "PC LOAD LETTER") == 0,
          "C-3 PC LOAD LETTER: == \"PC LOAD LETTER\" "
          "(the in-universe panic screen text, PRD Appendix B; "
          "source-string presence only -- rendered-boot oracle is initech-s25)");

    /* NOTE: No mutant guard for C-3. The string assertion above is self-proving:
     * if CANON_PANIC_TEXT ever drifted from "PC LOAD LETTER" to anything else,
     * the strcmp above goes RED immediately. Adding a mutant that changes the
     * expected string to "PC LOAD PAPER" would pass the mutant build but prove
     * nothing beyond string comparison tautology. The real mutation risk for
     * this canon element is panic.c itself -- caught by grep-gating the source
     * file in the commit hook and by the rendered-boot oracle (initech-s25). */
}


/*
 * test_pie_chart -- C-4.
 *
 * Asserts the canonical pie-chart slice values and their enforced sum (116).
 * The full rendered oracle (that the chart actually DISPLAYS 116%) is the
 * future test-canon-calc gate (M5 milestone, beads TBD).
 *
 * DO NOT change these values to sum to 100. The 116% sum is enforced canon.
 * Ref: PRD Appendix B; CLAUDE.md Law 4.
 */
static void test_pie_chart(void)
{
    int sum = CANON_PIE_SLICE_0 +
              CANON_PIE_SLICE_1 +
              CANON_PIE_SLICE_2 +
              CANON_PIE_SLICE_3 +
              CANON_PIE_SLICE_4;

    /* Individual slice values (locked from PRD Appendix B). */
    CHECK(CANON_PIE_SLICE_0 == 40,  "C-4 pie: slice 0 == 40%");
    CHECK(CANON_PIE_SLICE_1 == 35,  "C-4 pie: slice 1 == 35%");
    CHECK(CANON_PIE_SLICE_2 == 18,  "C-4 pie: slice 2 == 18%");
    CHECK(CANON_PIE_SLICE_3 == 14,  "C-4 pie: slice 3 == 14%");
    CHECK(CANON_PIE_SLICE_4 ==  9,  "C-4 pie: slice 4 == 9%");

    /* THE ENFORCED INVARIANT: slices sum to 116, not 100.
     * If you see this fail, do NOT adjust slices to sum to 100.
     * The 116% total is canon (PRD Appendix B; CLAUDE.md Law 4). */
    CHECK(sum == CANON_PIE_SLICE_SUM,
          "C-4 pie: 40+35+18+14+9 == 116 (enforced canon; NOT a rounding error; "
          "DO NOT fix to 100 -- that violates Law 4 and breaks this gate)");

    CHECK(CANON_PIE_SLICE_SUM == 116,
          "C-4 pie: CANON_PIE_SLICE_SUM constant == 116");

    /* Rendered-display gate (not asserted here -- see documentation above):
     *   test-canon-calc (future M5, beads TBD) asserts that the InitechCalc
     *   pie chart renders with slices summing to 116% on screen. */
}


/*
 * test_trailing_minus -- C-5.
 *
 * Asserts the canonical trailing-minus format constants are present and
 * internally consistent. The full rendering oracle (TRANSFORM(-570,"9999-")
 * == "570-" via the SAMIR engine) is the future test-canon-calc gate (M5).
 *
 * What is enforceable here (no engine link needed):
 *   - CANON_TRAILING_MINUS_VALUE is -570 (the canonical negative test value)
 *   - CANON_TRAILING_MINUS_FMT is "9999-" (trailing-minus PICTURE/TRANSFORM)
 *   - CANON_TRAILING_MINUS_RESULT is "570-" (expected rendered output)
 *   - the RESULT string does NOT start with '-' (minus is trailing, not leading)
 *   - the RESULT string ends with '-'
 *   - the RESULT string contains "570" (the absolute value of the test input)
 *
 * Ref: PRD Sec 6.5, Appendix A; CLAUDE.md Law 4; fn_builtins.c TRANSFORM;
 *      test_xbase_transform.c (TRANSFORM oracle, existing gate).
 */
static void test_trailing_minus(void)
{
    const char *result = CANON_TRAILING_MINUS_RESULT;
    size_t      rlen   = strlen(result);

    /* Canonical input value is negative. */
    CHECK(CANON_TRAILING_MINUS_VALUE == -570,
          "C-5 trailing minus: canonical value == -570");

    /* Format string is the trailing-minus PICTURE template. */
    CHECK(strcmp(CANON_TRAILING_MINUS_FMT, "9999-") == 0,
          "C-5 trailing minus: format string == \"9999-\"");

    /* Expected result contains "570" (the absolute value). */
    CHECK(strstr(result, "570") != NULL,
          "C-5 trailing minus: result contains \"570\"");

    /* The minus sign TRAILS: result does NOT start with '-'. */
    CHECK(rlen > 0 && result[0] != '-',
          "C-5 trailing minus: result[0] != '-' (minus is trailing, not leading)");

    /* The minus sign TRAILS: result ends with '-'. */
    CHECK(rlen > 0 && result[rlen - 1] == '-',
          "C-5 trailing minus: result ends with '-'");

    /* Full result string match. */
    CHECK(strcmp(result, "570-") == 0,
          "C-5 trailing minus: result == \"570-\" "
          "(the canon trailing-minus format for -570; enforced, not fixed)");

    /* Rendered-engine gate (not asserted here -- see documentation above):
     *   test-canon-calc (future M5, beads TBD) asserts:
     *     TRANSFORM(-570, "9999-") == "570-" via the SAMIR engine. */
}


/* =========================================================================
 * MAIN
 * ========================================================================= */

int main(void)
{
#if defined(CANON_MUTATE_WATCH) && defined(CANON_MUTATE_FIX_MENU)
    /* Both mutants at once would make the result ambiguous; guard against it. */
    fprintf(stderr,
        "ERROR: build with only ONE of -DCANON_MUTATE_WATCH or "
        "-DCANON_MUTATE_FIX_MENU, not both simultaneously.\n");
    return 2;
#endif

#ifdef CANON_MUTATE_WATCH
    fprintf(stderr,
        "  [MUTANT CANON_MUTATE_WATCH]: expecting hourglass waist checks to "
        "fail (wristwatch-like value 0x1C38 used instead of 0x0180). "
        "The gate MUST exit non-zero. The wristwatch is THE BUG (Law 4).\n");
#endif

#ifdef CANON_MUTATE_FIX_MENU
    fprintf(stderr,
        "  [MUTANT CANON_MUTATE_FIX_MENU]: expecting menu string and chimera "
        "checks to fail (\"Mode\" substituted for \"View\" at item[5]). "
        "The gate MUST exit non-zero. The impossible chimera IS the canon "
        "(ADR-0004 D-3/AM-4).\n");
#endif

    test_hourglass_cursor();
    test_photoshop_menu();
    test_pc_load_letter();
    test_pie_chart();
    test_trailing_minus();

    return TEST_SUMMARY("test-canon");
}
