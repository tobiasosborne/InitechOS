/*
 * spec/control_record.h -- FLAIR Control Manager: LOCKED spec-data.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.4 -- "the Control Manager").
 * beads: initech-dh5k.1 (P1-1: control_record.h + oracle).
 *
 * This header locks:
 *   (A) The verbatim Inside Macintosh ControlRecord field layout (IM-I Vol I
 *       Ch 10 p. I-318; MTE Ch 5 p. 5-51); the part-code constants
 *       (IM-I p. I-327; MTE Table 5-3); the CDEF proc IDs (IM-I p. I-322);
 *       and the value/min/max/hilite/vis semantics.
 *   (B) The Win 3.1 flat push-button accent constants (chimera element 10;
 *       Win31-ACCENT sub-section) -- bevel colors, render order, and the
 *       hard NO-#DFDFDF rule -- from:
 *       ../win31-decomp/specs/chrome/button-bevel.md
 *       ../win31-decomp/specs/user/button-control.md
 *
 * ERA AXIS (operator, 2026-06-20): System 7.0/7.1 is the BASE built now.
 * A System 8 Platinum layer accretes later as additive era-deltas WITHOUT
 * rewriting the base. Every constant below carries an era= tag so the future
 * layer can override additively. The Win 3.1 accent is tagged era=win31
 * (a confined chimera element, not the base layer).
 *
 * SOURCE CITATIONS (Law 1 -- cite local or cached source; do NOT guess):
 *
 *   Inside Macintosh Vol I Ch 10 (IM-I) -- the Control Manager:
 *     "The ControlRecord Data Structure" (IM-I p. I-318): nextControl,
 *       contrlOwner, contrlRect, contrlVis, contrlHilite, contrlValue,
 *       contrlMin, contrlMax, contrlDefProc, contrlData, contrlAction,
 *       contrlRfCon, contrlTitle -- field order and types verbatim.
 *     "FindControl / TestControl part-codes" (IM-I p. I-327, Table):
 *       inButton=10, inCheckBox=11, inUpButton=20, inDownButton=21,
 *       inPageUp=22, inPageDown=23, inThumb=129. Source: two independent
 *       local caches (MacTech ControlsFORTRAN + dev.os9.ca ControlMgrRef).
 *     "Standard CDEF definition IDs" (IM-I p. I-322): defID = resID*16 +
 *       variant: pushButProc=0, checkBoxProc=1, radioButProc=2,
 *       scrollBarProc=16, useWFont=8.
 *     Verbatim field TYPES / sizes: Handle/Ptr/ProcPtr=4B, Rect=8B,
 *       INTEGER=2B, Byte=1B, LONGINT=4B, Str255=256B. The record is PACKED
 *       (Pascal PACKED RECORD; no inter-field padding). Total=296 bytes.
 *     Verified against: ../system7-decomp/specs/toolbox/control-manager.md
 *       (local verbatim cache; refs/im-toolbox/im-toolbox-records-verbatim.md;
 *       MacTech Vol.03.04; dev.os9.ca Toolbox-317; dev.os9.ca ControlMgrRef).
 *
 *   ERA DELTA (Law 3): System 7.5+/Appearance Manager renames part-codes as
 *     kControlButtonPart, kControlIndicatorPart, etc. (same numeric values).
 *     These renames are NOT in the System 7.0/7.1 target; they are recorded
 *     only as a delta. Applies: kControlNoPart=0 (post-7.1 spelling).
 *     Source: ../system7-decomp/specs/toolbox/control-manager.md Sec 2.
 *
 *   Win 3.1 flat push-button accent (chimera element 10; era=win31):
 *     ../win31-decomp/specs/chrome/button-bevel.md -- pixel render order,
 *       color roles, bevel structure, NO-#DFDFDF ruling.
 *     ../win31-decomp/specs/user/button-control.md -- BS_PUSHBUTTON flat
 *       render, COLOR_* role index-to-documented-RGB table.
 *     Verified: real Win 3.1 DOSBox-X VGA screendump (w31_run_dialog.png
 *       pixel scan: Cancel button x=398..467, y=133..155). Color roles:
 *       COLOR_WINDOWFRAME=#000000, COLOR_BTNHIGHLIGHT=#FFFFFF,
 *       COLOR_BTNSHADOW=#808080, COLOR_BTNFACE=#C0C0C0.
 *     Hard guardrail: #DFDFDF = COLOR_3DLIGHT (Win 95 slot 22). That slot
 *       does NOT exist in Win 3.1 (table only goes to index 20). Any spec
 *       that cites #DFDFDF for Win 3.1 must be rejected (M-B WARNING 1 +
 *       Law 3). Source: refs/w31-user-gdi/getsyscolor-indices.txt.
 *
 *   spec/control.h (os/flair/control.h) -- FLAIR's live implementation of
 *     the ControlRecord. Read for cross-check (READ-ONLY from this lane).
 *     Documented FLAIR deviations: contrlDefProc replaced by flair_ctrl_type_t
 *     enum dispatch; contrlData/contrlAction omitted (no handle table);
 *     contrlTitle stored as fixed char[] not Str255; contrlOwner omitted.
 *     Part-codes and proc-IDs kept EXACTLY and asserted at build time.
 *     Source: ../system7-decomp/specs/toolbox/control-manager.md Sec "FLAIR
 *     mapping and deliberate deviations"; os/flair/control.h (verified read).
 *
 * SCOPE: this header is SPEC-DATA, not the live artifact. It locks the
 *   Outside-world interface contract so test_control_record.c can assert
 *   it independently of the living implementation in os/flair/control.h.
 *   Both consumers (the oracle and the impl) must agree on these values.
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib
 *   -fno-stack-protector -std=c11 -c) AND hosted (gcc -m32 -std=c11 -c).
 *   Only <stdint.h> + <stddef.h>. No libc beyond stdint/stddef.
 *   ASCII-clean (Rule 12). Changing this file is a deliberate Rule 8 act.
 */
#ifndef INITECH_SPEC_CONTROL_RECORD_H
#define INITECH_SPEC_CONTROL_RECORD_H

#include <stdint.h>
#include <stddef.h>

/* ===========================================================================
 * 1. PART-CODE CONSTANTS  (verbatim IM-I p. I-327; MTE Table 5-3)
 * ---------------------------------------------------------------------------
 * Returned by FindControl / TestControl; also stored in contrlHilite while a
 * part is pressed (0 = normal, 255 = inactive, part-code = that part pressed).
 *
 * era=system7.0-7.1 (two independent cached sources confirm; Law 1 verified).
 * Source: ../system7-decomp/specs/toolbox/control-manager.md Sec 2.
 *
 * IMPORTANT: 0 = NOT in any active control (FindControl returns 0 + NULL ctrl).
 *   A point in an INACTIVE control (contrlHilite==255) also returns 0.
 *   inThumb=129 is deliberately high to distinguish thumb-drag (live drag,
 *   no actionProc) from auto-tracking arrow/page parts (10..23).
 * ===========================================================================*/

/* era=system7.0-7.1 -- IM-I p. I-327 / MTE Table 5-3 verbatim values      */
#define CTRL_IN_BUTTON      10   /* in a push button (push-button body)       */
#define CTRL_IN_CHECKBOX    11   /* in a check box OR radio button (one code) */
#define CTRL_IN_UP_BUTTON   20   /* scroll-bar up (or left) arrow             */
#define CTRL_IN_DOWN_BUTTON 21   /* scroll-bar down (or right) arrow          */
#define CTRL_IN_PAGE_UP     22   /* scroll-bar paging region above thumb      */
#define CTRL_IN_PAGE_DOWN   23   /* scroll-bar paging region below thumb      */
#define CTRL_IN_THUMB       129  /* scroll-bar thumb (draggable indicator)    */

/* Verbatim IM-I names as aliases (for cross-reference searches against IM).
 * These match the names in os/flair/control.h (verified read, CLAUDE Law 1). */
#define inButton     CTRL_IN_BUTTON
#define inCheckBox   CTRL_IN_CHECKBOX
#define inUpButton   CTRL_IN_UP_BUTTON
#define inDownButton CTRL_IN_DOWN_BUTTON
#define inPageUp     CTRL_IN_PAGE_UP
#define inPageDown   CTRL_IN_PAGE_DOWN
#define inThumb      CTRL_IN_THUMB

/* Hilite sentinels (IM-I p. I-318 / control-manager.md Sec "Drawing and
 * hilite"): era=system7.0-7.1 */
#define CTRL_HILITE_NONE     0   /* no part highlighted (normal state)        */
#define CTRL_HILITE_INACTIVE 255 /* control is inactive / dimmed; not hit-testable */

/* ===========================================================================
 * 2. CDEF PROC IDs  (verbatim IM-I p. I-322; MTE Ch 5)
 * ---------------------------------------------------------------------------
 * defID = CDEF_resID * 16 + variation_code  (IM-I p. I-322 formula).
 * The button family (push/check/radio) is ONE CDEF resource (resID=0);
 * the scroll bar is a SEPARATE CDEF (resID=1), giving scrollBarProc=16.
 *
 * era=system7.0-7.1.
 * Source: ../system7-decomp/specs/toolbox/control-manager.md Sec 3.
 * Two cached sources: MacTech Vol.03.04 + dev.os9.ca Toolbox-317.
 *
 * ERA DELTA: popupMenuProc=1008 (resID=63, variant=0) exists in System 7
 *   but is NOT in the Office Space frame and NOT implemented in FLAIR M4.
 *   Constant recorded only for the catalog; not locked as an assert here.
 * ===========================================================================*/

/* era=system7.0-7.1 -- IM-I p. I-322 / MTE Ch 5 CDEF proc IDs verbatim    */
#define CDEF_PUSH_BUT_PROC   0   /* pushButProc:  resID=0, variant=0          */
#define CDEF_CHECK_BOX_PROC  1   /* checkBoxProc: resID=0, variant=1          */
#define CDEF_RADIO_BUT_PROC  2   /* radioButProc: resID=0, variant=2          */
#define CDEF_SCROLL_BAR_PROC 16  /* scrollBarProc: resID=1, variant=0 (1*16)  */
#define CDEF_USE_WFONT       8   /* useWFont: variation BIT OR'd in (+8)      */

/* Verbatim IM-I names as aliases. */
#define pushButProc   CDEF_PUSH_BUT_PROC
#define checkBoxProc  CDEF_CHECK_BOX_PROC
#define radioButProc  CDEF_RADIO_BUT_PROC
#define scrollBarProc CDEF_SCROLL_BAR_PROC
#define useWFont      CDEF_USE_WFONT

/* ===========================================================================
 * 3. ControlRecord FIELD OFFSETS (verbatim IM-I p. I-318 PACKED layout)
 * ---------------------------------------------------------------------------
 * The IM-I ControlRecord is a Pascal PACKED RECORD; offsets below are the
 * classic 68K in-memory layout (no inter-field padding). Types: Handle/
 * Ptr/ProcPtr=4B, Rect=8B, INTEGER=2B, Byte=1B, LONGINT=4B, Str255=256B.
 * Total in-memory: 296 bytes. [inferred arithmetic from verified field order
 * + types; field order and types [verified: A+B] per control-manager.md.]
 *
 * era=system7.0-7.1.
 *
 * FLAIR DEVIATIONS from IM-I (documented in os/flair/control.h and
 *   ../system7-decomp/specs/toolbox/control-manager.md Sec "FLAIR mapping"):
 *   - nextControl/contrlOwner: omitted (single-threaded cooperative; no
 *     per-window linked-list handle model).
 *   - contrlDefProc: replaced by flair_ctrl_type_t dispatch enum.
 *   - contrlData: omitted (no heap handle table, ADR-0004 DEC-03).
 *   - contrlAction: omitted (TrackControl takes action fn as parameter).
 *   - contrlTitle: stored as fixed char[64] not Str255 (freestanding C).
 *   - contrlVis/contrlHilite: stored as int16_t not Byte (alignment neutral
 *     in FLAIR's C struct; semantics identical: 0=hidden, 255=active).
 *   The LOAD-BEARING fields (contrlRect, contrlValue, contrlMin, contrlMax,
 *   contrlHilite, contrlVis, contrlRfCon) are kept verbatim by name and
 *   semantics. Part-codes and proc-IDs are kept exactly and asserted.
 *
 * These are the CANONICAL IM offsets for cross-reference, NOT the FLAIR
 * struct offsets (FLAIR deviates as noted above).
 * ===========================================================================*/

/* Canonical IM-I PACKED ControlRecord byte offsets (era=system7.0-7.1).
 * Source: ../system7-decomp/specs/toolbox/control-manager.md Table. */
#define CTRL_IM_OFF_NEXT_CONTROL   0   /* nextControl:  ControlHandle (4B)    */
#define CTRL_IM_OFF_CTRL_OWNER     4   /* contrlOwner:  WindowPtr     (4B)    */
#define CTRL_IM_OFF_CTRL_RECT      8   /* contrlRect:   Rect          (8B)    */
#define CTRL_IM_OFF_CTRL_VIS      16   /* contrlVis:    Byte          (1B)    */
#define CTRL_IM_OFF_CTRL_HILITE   17   /* contrlHilite: Byte          (1B)    */
#define CTRL_IM_OFF_CTRL_VALUE    18   /* contrlValue:  INTEGER       (2B)    */
#define CTRL_IM_OFF_CTRL_MIN      20   /* contrlMin:    INTEGER       (2B)    */
#define CTRL_IM_OFF_CTRL_MAX      22   /* contrlMax:    INTEGER       (2B)    */
#define CTRL_IM_OFF_CTRL_DEF_PROC 24   /* contrlDefProc:Handle        (4B)    */
#define CTRL_IM_OFF_CTRL_DATA     28   /* contrlData:   Handle        (4B)    */
#define CTRL_IM_OFF_CTRL_ACTION   32   /* contrlAction: ProcPtr       (4B)    */
#define CTRL_IM_OFF_CTRL_RF_CON   36   /* contrlRfCon:  LONGINT       (4B)    */
#define CTRL_IM_OFF_CTRL_TITLE    40   /* contrlTitle:  Str255       (256B)   */

/* Total IM-I PACKED ControlRecord size (era=system7.0-7.1). Arithmetic:
 * 40 (through contrlRfCon end) + 256 (Str255) = 296 bytes.             */
#define CTRL_IM_RECORD_SIZE       296

/* ===========================================================================
 * 4. VALUE / MIN / MAX SEMANTICS  (IM-I p. I-318; control-manager.md Sec 1)
 * ---------------------------------------------------------------------------
 * era=system7.0-7.1.
 * ===========================================================================*/

/* Check box / radio button: contrlValue is 0 (off) or 1 (on).
 * contrlMin=0, contrlMax=1. (IM-I p. I-318 / control-manager.md Sec 1)    */
#define CTRL_BOOL_MIN     0    /* era=system7.0-7.1: min for checkbox/radio  */
#define CTRL_BOOL_MAX     1    /* era=system7.0-7.1: max for checkbox/radio  */
#define CTRL_BOOL_OFF     0    /* era=system7.0-7.1: unchecked / not active  */
#define CTRL_BOOL_ON      1    /* era=system7.0-7.1: checked / active        */

/* Scroll bar: min==max means disabled appearance (empty / full document).
 * (IM-I p. I-318 / control-manager.md Sec 1)                              */
#define CTRL_SB_DISABLED_WHEN_MIN_EQ_MAX 1 /* era=system7.0-7.1 invariant  */

/* ===========================================================================
 * 5. WIN 3.1 FLAT PUSH-BUTTON ACCENT (chimera element 10; era=win31)
 * ---------------------------------------------------------------------------
 * SCOPE: confined Win 3.1 ACCENT sub-section for FLAIR chimera element 10.
 *   Source of truth: ../win31-decomp/specs/chrome/button-bevel.md (the pixel
 *   render order + exact color roles, verified against w31_run_dialog.png) and
 *   ../win31-decomp/specs/user/button-control.md (BS_PUSHBUTTON flat render,
 *   COLOR_* index table). This is NOT the System 7 base; it is a win31 accent.
 *
 * RENDER ORDER (normal, unpushed BS_PUSHBUTTON; era=win31):
 *   Step 1: 1px black outer outline (COLOR_WINDOWFRAME = #000000).
 *   Step 2: Face fill with COLOR_BTNFACE = #C0C0C0 (inset by the outline).
 *   Step 3: Single 1px bevel: top+left = COLOR_BTNHIGHLIGHT = #FFFFFF (white);
 *           bottom+right = COLOR_BTNSHADOW = #808080 (dark gray). EXACTLY ONE
 *           bevel ring -- NO second inner ring.
 *   Step 4: Label text centered in COLOR_BTNTEXT = #000000, bkmode TRANSPARENT.
 *   Step 5: Focus dotted rect (DrawFocusRect) if button has focus (inset).
 *
 * Source: button-bevel.md Sec 2 + Sec 3 (pixel offset table); verified via
 *   real Win 3.1 DOSBox-X VGA screendump pixel scan (Cancel button crop).
 *
 * HARD GUARDRAIL -- NO #DFDFDF (era=win31; Law 3):
 *   #DFDFDF = COLOR_3DLIGHT, Win 95 GetSysColor index 22. This color index
 *   does NOT exist in Win 3.1 (the Win 3.1 GetSysColor table ends at index 20).
 *   Any FLAIR code that writes #DFDFDF on the flat Win31 button is a Win 95
 *   ism and MUST be rejected. Source: button-bevel.md Sec 1 + Sec 8 era table;
 *   button-control.md Sec 7 era table; refs/w31-user-gdi/getsyscolor-indices.txt
 *   (Win 3.1 table goes to index 20 only; no COLOR_3DLIGHT slot exists).
 *   [M-B WARNING 1 + Law 3]
 * ===========================================================================*/

/* era=win31 -- flat BS_PUSHBUTTON face color (COLOR_BTNFACE).
 * Source: button-bevel.md Sec 2; button-control.md Sec 5.
 * 16-color VGA renders as #C3C7CB (DOSBox palette shift) but the documented
 * value (and what FLAIR shall use in indexed-8) is #C0C0C0. */
#define W31_BTN_FACE_R  0xC0u   /* era=win31; COLOR_BTNFACE red   component  */
#define W31_BTN_FACE_G  0xC0u   /* era=win31; COLOR_BTNFACE green component  */
#define W31_BTN_FACE_B  0xC0u   /* era=win31; COLOR_BTNFACE blue  component  */
#define W31_BTN_FACE_RGB  0x00C0C0C0u  /* era=win31; 0x00RRGGBB packed RGB  */

/* era=win31 -- bevel highlight: top+left edge = white (COLOR_BTNHIGHLIGHT).
 * Source: button-bevel.md Sec 3. Pure white; no secondary inner ring. */
#define W31_BTN_HIGHLIGHT_RGB  0x00FFFFFFu  /* era=win31; #FFFFFF             */

/* era=win31 -- bevel shadow: bottom+right edge = gray (COLOR_BTNSHADOW).
 * Source: button-bevel.md Sec 3. */
#define W31_BTN_SHADOW_RGB     0x00808080u  /* era=win31; #808080             */

/* era=win31 -- outer outline: COLOR_WINDOWFRAME = black.
 * Source: button-bevel.md Sec 2 Step 1 / Sec 3 pixel table. */
#define W31_BTN_OUTLINE_RGB    0x00000000u  /* era=win31; #000000             */

/* era=win31 -- label text color: COLOR_BTNTEXT = black.
 * Source: button-bevel.md Sec 2 Step 4; button-control.md Sec 5.1 Step 4. */
#define W31_BTN_TEXT_RGB       0x00000000u  /* era=win31; #000000             */

/* era=win31 -- FORBIDDEN color: #DFDFDF = COLOR_3DLIGHT (Win 95 ONLY).
 * Presence of this value in a Win31 button render is a Win95-ism and a
 * hard Law-3 violation. Defined here as a named sentinel so an assert can
 * check: (actual_color != W31_BTN_FORBIDDEN_INNER_RING).
 * Source: button-bevel.md Sec 1 "If you see #DFDFDF in a Win 3.1 spec claim,
 *   reject it -- Win95 artifact."; Sec 8 era table; [M-B WARNING 1]. */
#define W31_BTN_FORBIDDEN_INNER_RING 0x00DFDFDFu  /* NOT a Win31 color!     */

/* era=win31 -- bevel ring count: EXACTLY 1 (one outer bevel ring only).
 * A count of 2 implies the Win95 double-bevel, which is forbidden here.
 * Source: button-bevel.md Sec 1 "ONE ring only"; Sec 3 "NO second inner
 *   bevel ring." */
#define W31_BTN_BEVEL_RINGS    1   /* era=win31: exactly one bevel ring       */

/* era=win31 -- pushed-state inversion: bevel reverses (top/left <- shadow;
 * bottom/right <- highlight). Label shifts by (1,1). [golden-resolves: exact
 * pushed-state px layout -- button-bevel.md Sec 4]. */
#define W31_BTN_PUSHED_BEVEL_INVERTED 1  /* era=win31: bevel inverts on push */

/* era=win31 -- BS_DEFPUSHBUTTON gets a DOUBLED outer black border (not a
 * second bevel ring -- the border is thickened, not a new color ring).
 * Source: button-bevel.md Sec 5; button-control.md Sec 5.1 Step 1. */
#define W31_BTN_DEF_BORDER_FACTOR 2  /* era=win31: default border doubled    */

/* ===========================================================================
 * 6. COMPILE-TIME ASSERTIONS (_Static_assert oracle)
 * ---------------------------------------------------------------------------
 * Mirror the window_record.h / event_model.h style: every locked numeric
 * constant gets a _Static_assert so a silent edit to the value fires at
 * compile time. References cite the local source per Law 1.
 * ===========================================================================*/

/* --- Part-code constants (IM-I p. I-327; control-manager.md Sec 2) ---    */
_Static_assert(CTRL_IN_BUTTON      == 10,
    "inButton=10 (IM-I p. I-327; control-manager.md Sec 2; era=system7.0-7.1)");
_Static_assert(CTRL_IN_CHECKBOX    == 11,
    "inCheckBox=11 (IM-I p. I-327; one code for check box AND radio button)");
_Static_assert(CTRL_IN_UP_BUTTON   == 20,
    "inUpButton=20 (IM-I p. I-327; scroll-bar up/left arrow)");
_Static_assert(CTRL_IN_DOWN_BUTTON == 21,
    "inDownButton=21 (IM-I p. I-327; scroll-bar down/right arrow)");
_Static_assert(CTRL_IN_PAGE_UP     == 22,
    "inPageUp=22 (IM-I p. I-327; paging region above thumb)");
_Static_assert(CTRL_IN_PAGE_DOWN   == 23,
    "inPageDown=23 (IM-I p. I-327; paging region below thumb)");
_Static_assert(CTRL_IN_THUMB       == 129,
    "inThumb=129 (IM-I p. I-327; deliberately high: thumb-drag vs arrow/page)");

/* --- Hilite sentinels (IM-I p. I-318; control-manager.md Sec "Drawing") --*/
_Static_assert(CTRL_HILITE_NONE     == 0,
    "hilite 0 = normal (no part highlighted; IM-I p. I-318)");
_Static_assert(CTRL_HILITE_INACTIVE == 255,
    "hilite 255 = inactive/dimmed (IM-I p. I-318; era=system7.0-7.1)");

/* --- CDEF proc IDs (IM-I p. I-322; defID=resID*16+variant) ---           */
_Static_assert(CDEF_PUSH_BUT_PROC   == 0,
    "pushButProc=0: resID=0, variant=0 (IM-I p. I-322; era=system7.0-7.1)");
_Static_assert(CDEF_CHECK_BOX_PROC  == 1,
    "checkBoxProc=1: resID=0, variant=1 (IM-I p. I-322; era=system7.0-7.1)");
_Static_assert(CDEF_RADIO_BUT_PROC  == 2,
    "radioButProc=2: resID=0, variant=2 (IM-I p. I-322; era=system7.0-7.1)");
_Static_assert(CDEF_SCROLL_BAR_PROC == 16,
    "scrollBarProc=16: resID=1, variant=0 (1*16+0; IM-I p. I-322)");
_Static_assert(CDEF_USE_WFONT       == 8,
    "useWFont=8: variation bit OR'd in (IM-I p. I-322; era=system7.0-7.1)");

/* --- CDEF formula sanity: resID*16+variant produces the proc IDs ---      */
_Static_assert(0 * 16 + 0 == CDEF_PUSH_BUT_PROC,
    "pushButProc formula: 0*16+0=0 (IM-I p. I-322 defID formula)");
_Static_assert(0 * 16 + 1 == CDEF_CHECK_BOX_PROC,
    "checkBoxProc formula: 0*16+1=1");
_Static_assert(0 * 16 + 2 == CDEF_RADIO_BUT_PROC,
    "radioButProc formula: 0*16+2=2");
_Static_assert(1 * 16 + 0 == CDEF_SCROLL_BAR_PROC,
    "scrollBarProc formula: 1*16+0=16");

/* --- IM-I ControlRecord canonical offsets (packed layout arithmetic) ---  */
_Static_assert(CTRL_IM_OFF_NEXT_CONTROL == 0,
    "nextControl at offset 0 (IM-I p. I-318 PACKED record)");
_Static_assert(CTRL_IM_OFF_CTRL_OWNER   == 4,
    "contrlOwner at offset 4 (4B Handle after nextControl)");
_Static_assert(CTRL_IM_OFF_CTRL_RECT    == 8,
    "contrlRect at offset 8 (4B WindowPtr after owner)");
_Static_assert(CTRL_IM_OFF_CTRL_VIS     == 16,
    "contrlVis at offset 16 (8B Rect after contrlRect)");
_Static_assert(CTRL_IM_OFF_CTRL_HILITE  == 17,
    "contrlHilite at offset 17 (1B Byte after contrlVis)");
_Static_assert(CTRL_IM_OFF_CTRL_VALUE   == 18,
    "contrlValue at offset 18 (1B Byte after contrlHilite)");
_Static_assert(CTRL_IM_OFF_CTRL_MIN     == 20,
    "contrlMin at offset 20 (2B INTEGER after contrlValue)");
_Static_assert(CTRL_IM_OFF_CTRL_MAX     == 22,
    "contrlMax at offset 22 (2B INTEGER after contrlMin)");
_Static_assert(CTRL_IM_OFF_CTRL_DEF_PROC == 24,
    "contrlDefProc at offset 24 (2B INTEGER after contrlMax)");
_Static_assert(CTRL_IM_OFF_CTRL_DATA    == 28,
    "contrlData at offset 28 (4B Handle after contrlDefProc)");
_Static_assert(CTRL_IM_OFF_CTRL_ACTION  == 32,
    "contrlAction at offset 32 (4B Handle after contrlData)");
_Static_assert(CTRL_IM_OFF_CTRL_RF_CON  == 36,
    "contrlRfCon at offset 36 (4B ProcPtr after contrlAction)");
_Static_assert(CTRL_IM_OFF_CTRL_TITLE   == 40,
    "contrlTitle at offset 40 (4B LONGINT after contrlRfCon)");
_Static_assert(CTRL_IM_RECORD_SIZE      == 296,
    "IM ControlRecord total=296B: 40+256(Str255) (IM-I p. I-318 inferred)");

/* --- Part-code ordering invariants (from control-manager.md Sec 2) ---    */
_Static_assert(CTRL_IN_BUTTON < CTRL_IN_CHECKBOX,
    "inButton(10) < inCheckBox(11): push button part < check/radio part");
_Static_assert(CTRL_IN_UP_BUTTON < CTRL_IN_DOWN_BUTTON,
    "inUpButton(20) < inDownButton(21): up arrow < down arrow");
_Static_assert(CTRL_IN_DOWN_BUTTON < CTRL_IN_PAGE_UP,
    "inDownButton(21) < inPageUp(22): arrow parts before page parts");
_Static_assert(CTRL_IN_PAGE_UP < CTRL_IN_PAGE_DOWN,
    "inPageUp(22) < inPageDown(23): page-up before page-down");
_Static_assert(CTRL_IN_THUMB > CTRL_IN_PAGE_DOWN,
    "inThumb(129) > inPageDown(23): thumb high so drag-vs-autotrack distinct");

/* --- Win 3.1 accent: bevel ring count (era=win31; Law 3 guardrail) ---   */
_Static_assert(W31_BTN_BEVEL_RINGS == 1,
    "Win31 flat button: EXACTLY 1 bevel ring (era=win31; button-bevel.md Sec 1)");

/* --- Win 3.1 accent: forbidden Win95 color sentinel ---                  */
_Static_assert(W31_BTN_FORBIDDEN_INNER_RING == 0x00DFDFDFu,
    "#DFDFDF = Win95 COLOR_3DLIGHT: forbidden in Win31 (M-B WARNING 1 + Law 3)");

/* --- Win 3.1 accent: face != highlight != shadow (distinct colors) ---   */
_Static_assert(W31_BTN_FACE_RGB != W31_BTN_HIGHLIGHT_RGB,
    "Win31: face (#C0C0C0) != highlight (#FFFFFF) (button-bevel.md Sec 2+3)");
_Static_assert(W31_BTN_FACE_RGB != W31_BTN_SHADOW_RGB,
    "Win31: face (#C0C0C0) != shadow (#808080) (button-bevel.md Sec 2+3)");
_Static_assert(W31_BTN_HIGHLIGHT_RGB != W31_BTN_SHADOW_RGB,
    "Win31: highlight (#FFFFFF) != shadow (#808080) (button-bevel.md Sec 3)");

/* --- Win 3.1 accent: face is NOT the forbidden Win95 inner ring color --- */
_Static_assert(W31_BTN_FACE_RGB != W31_BTN_FORBIDDEN_INNER_RING,
    "Win31: face (#C0C0C0) != #DFDFDF (no Win95 3DLIGHT in 3.1 button)");
_Static_assert(W31_BTN_HIGHLIGHT_RGB != W31_BTN_FORBIDDEN_INNER_RING,
    "Win31: highlight (#FFFFFF) != #DFDFDF (distinct from Win95 3DLIGHT)");

/* --- Win 3.1 default-button border doubling factor ---                   */
_Static_assert(W31_BTN_DEF_BORDER_FACTOR == 2,
    "BS_DEFPUSHBUTTON border factor=2 (osfree-win16 nBorderWidth*=2; era=win31)");

#endif /* INITECH_SPEC_CONTROL_RECORD_H */
