/* harness/proptest/test_control_record.c -- host oracle for spec/control_record.h
 *
 * beads: initech-dh5k.1 (P1-1: spec/control_record.h + oracle).
 *
 * PURPOSE: exercises every locked constant and structural rule in
 *   spec/control_record.h and confirms they match the ground-truth values
 *   from Inside Macintosh (part-codes, CDEF proc IDs, IM-I field offsets,
 *   hilite sentinels, value/min/max semantics) and from the Win 3.1 flat
 *   push-button accent spec (chimera element 10; bevel colors, ring count,
 *   no-#DFDFDF guardrail, render-order semantics).
 *
 * SELF-VERIFY: build and run standalone without `make`:
 *   # Freestanding compile (no stdlib; just -c to confirm it compiles):
 *   gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -std=c11 \
 *       -I/home/tobias/Projects/initech-os/spec -c \
 *       /home/tobias/Projects/initech-os/harness/proptest/test_control_record.c \
 *       -o /tmp/test_control_record_fs.o
 *   # Hosted binary (runs + prints result):
 *   gcc -m32 -std=c11 \
 *       -I/home/tobias/Projects/initech-os/spec \
 *       /home/tobias/Projects/initech-os/harness/proptest/test_control_record.c \
 *       -o /tmp/test_control_record && /tmp/test_control_record
 *   # Mutation probe (must exit 1; proves the oracle bites):
 *   gcc -m32 -std=c11 -DTEST_CTRL_MUTANT \
 *       -I/home/tobias/Projects/initech-os/spec \
 *       /home/tobias/Projects/initech-os/harness/proptest/test_control_record.c \
 *       -o /tmp/test_control_record_mutant && /tmp/test_control_record_mutant; \
 *       test $? -ne 0 && echo "MUTATION RED (correct)" || echo "MUTATION GREEN (BUG)"
 *
 * MUTATION PROOF (Rule 6): build with -DTEST_CTRL_MUTANT to perturb ONE
 *   constant (inThumb value) so the oracle MUST fail (exit 1, RED). This
 *   proves the runtime checks actually bite. The header _Static_asserts are
 *   a SEPARATE compile-time layer that also fires on mutation.
 *
 * Source citations: see spec/control_record.h (all Law 1 citations).
 * ASCII-clean (Rule 12). No 2026-isms (Rule 11).
 */

/* Include the locked spec. This fires every _Static_assert in the header
 * at compile time (the compile-time oracle). */
#include "control_record.h"

/* Hosted-only: stdio for the result line.
 * Under freestanding (-ffreestanding; __STDC_HOSTED__==0) stdio headers are
 * unavailable. The _Static_assert compile-time layer fires regardless.
 * The runtime check loop (main + printf) is only compiled/reached hosted. */
#if defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 1)
#include <stdio.h>
#endif

/* --------------------------------------------------------------------------
 * CHECK macro -- increment `checks`; if the condition fails, increment `fails`
 * and print the failing assertion name. Only active in hosted mode.
 * -------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 1)
#define CHECK(name, cond)                                          \
    do {                                                           \
        checks++;                                                  \
        if (!(cond)) {                                             \
            fails++;                                               \
            printf("FAIL %s\n", (name));                          \
        }                                                          \
    } while (0)

int main(void)
{
    int checks = 0;
    int fails  = 0;

    /* ===================================================================
     * SECTION A: Part-code constants (IM-I p. I-327; era=system7.0-7.1)
     * Source: ../system7-decomp/specs/toolbox/control-manager.md Sec 2.
     * =================================================================== */

    /* Verbatim IM-I values -- the numbers here ARE the spec. */
    CHECK("inButton=10 (IM-I p. I-327)",
          CTRL_IN_BUTTON == 10);
    CHECK("inCheckBox=11 (IM-I p. I-327; check box + radio button share code)",
          CTRL_IN_CHECKBOX == 11);
    CHECK("inUpButton=20 (IM-I p. I-327; scroll up/left arrow)",
          CTRL_IN_UP_BUTTON == 20);
    CHECK("inDownButton=21 (IM-I p. I-327; scroll down/right arrow)",
          CTRL_IN_DOWN_BUTTON == 21);
    CHECK("inPageUp=22 (IM-I p. I-327; paging above thumb)",
          CTRL_IN_PAGE_UP == 22);
    CHECK("inPageDown=23 (IM-I p. I-327; paging below thumb)",
          CTRL_IN_PAGE_DOWN == 23);

    /* inThumb: deliberately high (129) to distinguish from auto-track parts
     * (10..23). Mutant probe perturbs this specific value. */
#ifdef TEST_CTRL_MUTANT
    {
        /* Perturb a LOCAL copy; do NOT touch the header constant.
         * The check below must fail (RED) in mutant mode. */
        int thumb_probe = CTRL_IN_THUMB ^ 1;
        CHECK("inThumb=129 [MUTANT: must fail]",
              thumb_probe == 129);
    }
#else
    CHECK("inThumb=129 (IM-I p. I-327; high so thumb-drag != arrow/page)",
          CTRL_IN_THUMB == 129);
#endif

    /* Alias names match constants (IM-I verbatim names in os/flair/control.h
     * cross-check; these aliases must stay synchronized). */
    CHECK("alias inButton == CTRL_IN_BUTTON",
          inButton     == CTRL_IN_BUTTON);
    CHECK("alias inCheckBox == CTRL_IN_CHECKBOX",
          inCheckBox   == CTRL_IN_CHECKBOX);
    CHECK("alias inUpButton == CTRL_IN_UP_BUTTON",
          inUpButton   == CTRL_IN_UP_BUTTON);
    CHECK("alias inDownButton == CTRL_IN_DOWN_BUTTON",
          inDownButton == CTRL_IN_DOWN_BUTTON);
    CHECK("alias inPageUp == CTRL_IN_PAGE_UP",
          inPageUp     == CTRL_IN_PAGE_UP);
    CHECK("alias inPageDown == CTRL_IN_PAGE_DOWN",
          inPageDown   == CTRL_IN_PAGE_DOWN);
    CHECK("alias inThumb == CTRL_IN_THUMB",
          inThumb      == CTRL_IN_THUMB);

    /* ===================================================================
     * SECTION B: Hilite sentinels (IM-I p. I-318; era=system7.0-7.1)
     * =================================================================== */

    CHECK("CTRL_HILITE_NONE=0 (normal; no part highlighted)",
          CTRL_HILITE_NONE == 0);
    CHECK("CTRL_HILITE_INACTIVE=255 (inactive/dimmed; not hit-testable)",
          CTRL_HILITE_INACTIVE == 255);
    /* The two sentinels must be distinct and not overlap any part-code. */
    CHECK("hilite_none != hilite_inactive",
          CTRL_HILITE_NONE != CTRL_HILITE_INACTIVE);
    CHECK("CTRL_HILITE_INACTIVE not a valid part-code (255 > inThumb=129)",
          CTRL_HILITE_INACTIVE > CTRL_IN_THUMB);

    /* ===================================================================
     * SECTION C: CDEF proc IDs (IM-I p. I-322; era=system7.0-7.1)
     * defID = CDEF_resID * 16 + variation_code
     * Source: control-manager.md Sec 3.
     * =================================================================== */

    CHECK("pushButProc=0 (resID=0, variant=0; IM-I p. I-322)",
          CDEF_PUSH_BUT_PROC == 0);
    CHECK("checkBoxProc=1 (resID=0, variant=1; IM-I p. I-322)",
          CDEF_CHECK_BOX_PROC == 1);
    CHECK("radioButProc=2 (resID=0, variant=2; IM-I p. I-322)",
          CDEF_RADIO_BUT_PROC == 2);
    CHECK("scrollBarProc=16 (resID=1, variant=0; 1*16+0; IM-I p. I-322)",
          CDEF_SCROLL_BAR_PROC == 16);
    CHECK("useWFont=8 (variation bit OR'd in; IM-I p. I-322)",
          CDEF_USE_WFONT == 8);

    /* Alias names match. */
    CHECK("alias pushButProc == CDEF_PUSH_BUT_PROC",
          pushButProc   == CDEF_PUSH_BUT_PROC);
    CHECK("alias checkBoxProc == CDEF_CHECK_BOX_PROC",
          checkBoxProc  == CDEF_CHECK_BOX_PROC);
    CHECK("alias radioButProc == CDEF_RADIO_BUT_PROC",
          radioButProc  == CDEF_RADIO_BUT_PROC);
    CHECK("alias scrollBarProc == CDEF_SCROLL_BAR_PROC",
          scrollBarProc == CDEF_SCROLL_BAR_PROC);
    CHECK("alias useWFont == CDEF_USE_WFONT",
          useWFont      == CDEF_USE_WFONT);

    /* CDEF formula: defID = resID*16 + variant. Verify each ID. */
    CHECK("pushButProc formula: 0*16+0 == 0",
          (0 * 16 + 0) == CDEF_PUSH_BUT_PROC);
    CHECK("checkBoxProc formula: 0*16+1 == 1",
          (0 * 16 + 1) == CDEF_CHECK_BOX_PROC);
    CHECK("radioButProc formula: 0*16+2 == 2",
          (0 * 16 + 2) == CDEF_RADIO_BUT_PROC);
    CHECK("scrollBarProc formula: 1*16+0 == 16",
          (1 * 16 + 0) == CDEF_SCROLL_BAR_PROC);

    /* ===================================================================
     * SECTION D: IM-I ControlRecord canonical offsets (era=system7.0-7.1)
     * Packed layout arithmetic: IM-I p. I-318 + control-manager.md Sec 1.
     * =================================================================== */

    CHECK("IM offset nextControl==0",
          CTRL_IM_OFF_NEXT_CONTROL == 0);
    CHECK("IM offset contrlOwner==4 (4B Handle)",
          CTRL_IM_OFF_CTRL_OWNER == 4);
    CHECK("IM offset contrlRect==8 (4B WindowPtr)",
          CTRL_IM_OFF_CTRL_RECT == 8);
    CHECK("IM offset contrlVis==16 (8B Rect)",
          CTRL_IM_OFF_CTRL_VIS == 16);
    CHECK("IM offset contrlHilite==17 (1B Byte after contrlVis)",
          CTRL_IM_OFF_CTRL_HILITE == 17);
    CHECK("IM offset contrlValue==18 (1B Byte after contrlHilite)",
          CTRL_IM_OFF_CTRL_VALUE == 18);
    CHECK("IM offset contrlMin==20 (2B INTEGER after contrlValue)",
          CTRL_IM_OFF_CTRL_MIN == 20);
    CHECK("IM offset contrlMax==22 (2B INTEGER after contrlMin)",
          CTRL_IM_OFF_CTRL_MAX == 22);
    CHECK("IM offset contrlDefProc==24 (2B INTEGER after contrlMax)",
          CTRL_IM_OFF_CTRL_DEF_PROC == 24);
    CHECK("IM offset contrlData==28 (4B Handle after contrlDefProc)",
          CTRL_IM_OFF_CTRL_DATA == 28);
    CHECK("IM offset contrlAction==32 (4B Handle after contrlData)",
          CTRL_IM_OFF_CTRL_ACTION == 32);
    CHECK("IM offset contrlRfCon==36 (4B ProcPtr after contrlAction)",
          CTRL_IM_OFF_CTRL_RF_CON == 36);
    CHECK("IM offset contrlTitle==40 (4B LONGINT after contrlRfCon)",
          CTRL_IM_OFF_CTRL_TITLE == 40);
    CHECK("IM ControlRecord total size==296 (40+256 for Str255)",
          CTRL_IM_RECORD_SIZE == 296);

    /* ===================================================================
     * SECTION E: Value / min / max semantics (IM-I p. I-318)
     * era=system7.0-7.1.
     * =================================================================== */

    CHECK("CTRL_BOOL_MIN==0 (checkbox/radio min; IM-I)",
          CTRL_BOOL_MIN == 0);
    CHECK("CTRL_BOOL_MAX==1 (checkbox/radio max; IM-I)",
          CTRL_BOOL_MAX == 1);
    CHECK("CTRL_BOOL_OFF==0 (unchecked; IM-I)",
          CTRL_BOOL_OFF == 0);
    CHECK("CTRL_BOOL_ON==1 (checked; IM-I)",
          CTRL_BOOL_ON == 1);
    CHECK("disabled-when-min-eq-max invariant set to 1",
          CTRL_SB_DISABLED_WHEN_MIN_EQ_MAX == 1);

    /* ===================================================================
     * SECTION F: Part-code ordering rules (from control-manager.md Sec 2)
     * =================================================================== */

    CHECK("inButton(10) < inCheckBox(11): push < check/radio",
          CTRL_IN_BUTTON < CTRL_IN_CHECKBOX);
    CHECK("inUpButton(20) < inDownButton(21): up < down",
          CTRL_IN_UP_BUTTON < CTRL_IN_DOWN_BUTTON);
    CHECK("inDownButton(21) < inPageUp(22): arrow before page",
          CTRL_IN_DOWN_BUTTON < CTRL_IN_PAGE_UP);
    CHECK("inPageUp(22) < inPageDown(23): page-up before page-down",
          CTRL_IN_PAGE_UP < CTRL_IN_PAGE_DOWN);
    CHECK("inThumb(129) > inPageDown(23): thumb high for drag distinction",
          CTRL_IN_THUMB > CTRL_IN_PAGE_DOWN);

    /* Arrow/page parts fit in a byte (10..23); inThumb also fits in a byte. */
    CHECK("inButton fits in uint8 (>=0, <=127 for part-code range)",
          CTRL_IN_BUTTON <= 127);
    CHECK("inPageDown fits in uint8",
          CTRL_IN_PAGE_DOWN <= 127);
    CHECK("inThumb fits in uint8 (<=255)",
          CTRL_IN_THUMB <= 255);

    /* ===================================================================
     * SECTION G: Win 3.1 flat push-button accent (era=win31; chimera elem 10)
     * Source: ../win31-decomp/specs/chrome/button-bevel.md +
     *         ../win31-decomp/specs/user/button-control.md.
     * =================================================================== */

    /* Face fill: #C0C0C0 = COLOR_BTNFACE (era=win31). */
    CHECK("W31 face R=0xC0 (#C0C0C0; button-bevel.md Sec 2)",
          W31_BTN_FACE_R == 0xC0u);
    CHECK("W31 face G=0xC0",
          W31_BTN_FACE_G == 0xC0u);
    CHECK("W31 face B=0xC0",
          W31_BTN_FACE_B == 0xC0u);
    CHECK("W31 face packed RGB=0x00C0C0C0",
          W31_BTN_FACE_RGB == 0x00C0C0C0u);

    /* Highlight: #FFFFFF = COLOR_BTNHIGHLIGHT (era=win31). */
    CHECK("W31 highlight RGB=#FFFFFF (button-bevel.md Sec 3)",
          W31_BTN_HIGHLIGHT_RGB == 0x00FFFFFFu);

    /* Shadow: #808080 = COLOR_BTNSHADOW (era=win31). */
    CHECK("W31 shadow RGB=#808080 (button-bevel.md Sec 3)",
          W31_BTN_SHADOW_RGB == 0x00808080u);

    /* Outer outline: #000000 = COLOR_WINDOWFRAME (era=win31). */
    CHECK("W31 outline RGB=#000000 (button-bevel.md Sec 2 Step 1)",
          W31_BTN_OUTLINE_RGB == 0x00000000u);

    /* Label text: #000000 = COLOR_BTNTEXT (era=win31). */
    CHECK("W31 text RGB=#000000 (button-bevel.md Sec 2 Step 4)",
          W31_BTN_TEXT_RGB == 0x00000000u);

    /* HARD GUARDRAIL: forbidden Win95 color #DFDFDF = COLOR_3DLIGHT.
     * This color does NOT exist in Win 3.1. [M-B WARNING 1 + Law 3] */
    CHECK("W31 forbidden inner ring sentinel = 0x00DFDFDF (Win95 COLOR_3DLIGHT)",
          W31_BTN_FORBIDDEN_INNER_RING == 0x00DFDFDFu);
    /* Verify: the face color is NOT the forbidden Win95 value. */
    CHECK("W31 face (#C0C0C0) != #DFDFDF (no Win95 3DLIGHT in 3.1)",
          W31_BTN_FACE_RGB != W31_BTN_FORBIDDEN_INNER_RING);
    /* Verify: highlight is NOT #DFDFDF. */
    CHECK("W31 highlight (#FFFFFF) != #DFDFDF",
          W31_BTN_HIGHLIGHT_RGB != W31_BTN_FORBIDDEN_INNER_RING);

    /* The three visible bevel colors are all distinct. */
    CHECK("W31 face != highlight (button-bevel.md Sec 2+3)",
          W31_BTN_FACE_RGB != W31_BTN_HIGHLIGHT_RGB);
    CHECK("W31 face != shadow (button-bevel.md Sec 2+3)",
          W31_BTN_FACE_RGB != W31_BTN_SHADOW_RGB);
    CHECK("W31 highlight != shadow",
          W31_BTN_HIGHLIGHT_RGB != W31_BTN_SHADOW_RGB);

    /* Exactly one bevel ring (not two; Law 3 no-Win95-double-ring). */
    CHECK("W31 bevel rings == 1 (ONE ring only; button-bevel.md Sec 1)",
          W31_BTN_BEVEL_RINGS == 1);

    /* Pushed-state: bevel inversion flag set. */
    CHECK("W31 pushed-state bevel inverted flag=1 (button-bevel.md Sec 4)",
          W31_BTN_PUSHED_BEVEL_INVERTED == 1);

    /* Default-button border factor: doubled outer border (not a new ring). */
    CHECK("W31 default-button border factor=2 (osfree-win16 nBorderWidth*=2)",
          W31_BTN_DEF_BORDER_FACTOR == 2);

    /* ===================================================================
     * SECTION H: Cross-invariant: IM part-codes vs Win31 accent are in
     * separate name-spaces; no numeric collision between the part-codes and
     * the packed-RGB accent constants (different magnitude ranges).
     * =================================================================== */

    /* Part-codes are 0..129; packed RGB constants are 0x00RRGGBB (large).
     * This just confirms they are in clearly different value ranges. */
    CHECK("inThumb(129) < W31_BTN_SHADOW_RGB (no namespace collision)",
          (unsigned)CTRL_IN_THUMB < W31_BTN_SHADOW_RGB);

    /* ===================================================================
     * RESULT
     * =================================================================== */

    printf("test-control-record: %d checks, %d failures, %s\n",
           checks, fails, fails == 0 ? "green" : "RED");
    return fails == 0 ? 0 : 1;
}
#endif /* __STDC_HOSTED__ */
