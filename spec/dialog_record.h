/*
 * spec/dialog_record.h -- FLAIR Dialog Manager: the LOCKED contract.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.5 -- "the Dialog Manager").
 * era=system7.0-7.1 (operator decision 2026-06-20; System 8 Platinum accretes
 * later as an additive layer; see docs/plans/FLAIR-implementation-plan.md Sec 1).
 * beads: initech-dh5k.3 (P1-3: spec/dialog_record.h + oracle).
 *
 * This header locks the DialogRecord layout invariants, the DITL item-type byte
 * catalog, the alert-type selectors and 4-stage escalation mechanism, the
 * ok=1/cancel=2 standard-button result codes, and the ModalDialog cooperative-
 * loop contract -- all verbatim from Inside Macintosh, System 7.0/7.1 era.
 *
 * SOURCE CITATIONS (Law 1 -- local corpus is authority):
 *
 *   ../system7-decomp/specs/toolbox/dialog-manager.md (BASE; primary authority):
 *     Sec 1: DialogRecord verbatim Pascal layout (window@0, items, textH,
 *       editField, editOpen, aDefItem); windowKind=dialogKind(2) inside window.
 *     Sec 2: DITL item-type byte catalog (userItem=0, helpItem=1, ctrlItem=4,
 *       btnCtrl=0/chkCtrl=1/radCtrl=2/resCtrl=3 sub-types, statText=8,
 *       editText=16, iconItem=32, picItem=64, itemDisable=128 high bit).
 *     Sec 3: ModalDialog cooperative-loop contract (filterProc/itemHit;
 *       modal/modeless; Return->default, Escape/Cmd-. -> Cancel convention).
 *     Sec 4: Alert mechanism (Alert/StopAlert/NoteAlert/CautionAlert returns
 *       1-based itemHit; stopIcon=0/noteIcon=1/cautionIcon=2 ICON resource IDs;
 *       4-stage StageList escalation, sound number 0..3; GetAlertStage).
 *     Sec 4.3: Standard items ok=1/cancel=2 (1-based DITL item index conventions).
 *     Sec 6: FLAIR mapping + deliberate deviations (DialogPtr->WindowPtr chain;
 *       FILE COPY canon "Saving tables to disk..."; test-dialog 73 checks).
 *     Sec 7: Certainty ledger -- all constants [verified: A+B].
 *   [verified: A+B = im-toolbox-records-verbatim.md + Toolbox-442 verbatim;
 *    dialog-manager.md Sec 7 contradiction check: none found]
 *
 *   spec/window_record.h (WindowRecord, dBoxProc=1, dialogKind=2, WDEF variants;
 *     do NOT redefine those constants here -- include this file instead).
 *   spec/chrome_metrics.h (FLAIR_CHROME_DIALOG_BORDER=7 -- dBoxBorderSize EQU 7;
 *     not redefined here; this file relies on chrome_metrics.h for that constant).
 *   os/flair/dialog.h (FLAIR implementation; READ-ONLY reference for deviations;
 *     FLAIR_CANON_FILECOPY_MSG "Saving tables to disk...", Law 4 canon).
 *
 *   Inside Macintosh Vol I Ch 13 (IM-I) + MTE Ch 6 "Dialog Manager" (cross-
 *     check; the system7-decomp corpus is the local authority, not re-read here).
 *
 * CANON WARNING (Law 4 -- PRD Sec 6.5 / Appendix A/B):
 *   The FILE COPY dialog message string ("Saving tables to disk...") is LOCKED
 *   in os/flair/dialog.h FLAIR_CANON_FILECOPY_MSG. This spec carries no
 *   duplicate definition -- the implementation header is the single source of
 *   truth for that string. The oracle (harness/proptest/test_dialog_record.c)
 *   checks the spec constants only; the full Law-4 byte-exact assertion is in
 *   harness/proptest/test_dialog.c (DIALOG_MUTATE_FILECOPY_MSG).
 *
 * WHAT THIS HEADER LOCKS:
 *   1. DITL item-type byte values (all [verified: A+B]).
 *   2. itemDisable high-bit value (0x80).
 *   3. Alert icon resource selectors stopIcon/noteIcon/cautionIcon.
 *   4. Alert stage count (4 stages) and sound selector range (0..3).
 *   5. Standard result codes ok=1, cancel=2.
 *   6. windowKind == dialogKind(2) for dialog windows (cross-check to
 *      spec/window_record.h; the _Static_assert here verifies consistency).
 *   7. The 1-based item numbering convention (item 0 = no item).
 *   _Static_asserts enforce every numeric constant.
 *
 * FLAIR DEVIATIONS (all justified; verbatim IM field NAMES preserved):
 *   The DialogRecord in os/flair/dialog.h is the FLAIR canonical implementation.
 *   This spec header does NOT redefine it (that would create a duplicate type).
 *   Instead, this header provides:
 *   (a) The DITL item-type byte constants the FLAIR DialogItem.type enum must
 *       match (verified by the oracle against flair_dialog_item_type_t).
 *   (b) Alert-related constants (icon selectors, stage count, sound range).
 *   (c) Standard result codes ok/cancel.
 *   (d) _Static_assert gates on the DITL values, itemDisable, and ok/cancel.
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11)
 * AND hosted (cc -std=c11). Only <stdint.h> + <stddef.h> + local spec headers.
 * No host malloc; no libc beyond stdint/stddef. Rule 11 (reproducible).
 * ASCII-clean (Rule 12). Changing this file is a deliberate, Rule 8 act.
 */
#ifndef INITECH_SPEC_DIALOG_RECORD_H
#define INITECH_SPEC_DIALOG_RECORD_H

#include <stdint.h>
#include <stddef.h>

#include "window_record.h"   /* dialogKind=2, dBoxProc=1 (already locked)      */
#include "chrome_metrics.h"  /* FLAIR_CHROME_DIALOG_BORDER=7 (already locked)  */

/* ===========================================================================
 * 1. DITL ITEM-TYPE BYTE CATALOG  (verbatim Inside Macintosh values)
 *    era=system7.0-7.1
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/dialog-manager.md Sec 2
 *      [verified: A+B; im-toolbox-records-verbatim.md + Toolbox-442]:
 *
 * The item-type byte = a base type OR'd with itemDisable(0x80).
 * ctrlItem(4) is a base that is ADDED to a 0..3 control sub-type.
 *
 * Sub-type worked examples (always add to ctrlItem=4):
 *   push button  = ctrlItem + btnCtrl = 4 + 0 = 4
 *   check box    = ctrlItem + chkCtrl = 4 + 1 = 5
 *   radio button = ctrlItem + radCtrl = 4 + 2 = 6
 *   CNTL-resource= ctrlItem + resCtrl = 4 + 3 = 7
 *
 * NAME-COLLISION NOTE (documented; not a conflict):
 *   helpItem=1 and btnCtrl=0 look similar but are NEVER ambiguous:
 *   helpItem is a standalone base type; btnCtrl is only ever added to ctrlItem.
 *   [verified: dialog-manager.md Sec 2 disambiguation note]
 *
 * 1-BASED ITEM NUMBERING (load-bearing):
 *   aDefItem, ok/cancel, and ModalDialog itemHit are ALL 1-based DITL indices.
 *   Item 0 means "no item."
 *   [verified: A+B; dialog-manager.md Sec 2 + Sec 4.3]
 * ===========================================================================*/

/* Base types (standalone; never combined with sub-types). */
#define FLAIR_DITL_USER_ITEM      0u  /* user-drawn; app supplies draw proc    */
                                      /* [verified: A+B dialog-manager.md Sec 2] */
#define FLAIR_DITL_HELP_ITEM      1u  /* Help Manager balloon (System 7; no widget) */
                                      /* [verified: A+B dialog-manager.md Sec 2] */
#define FLAIR_DITL_CTRL_ITEM      4u  /* control base; ADD btnCtrl/chkCtrl/... */
                                      /* [verified: A+B dialog-manager.md Sec 2] */
#define FLAIR_DITL_STAT_TEXT      8u  /* static (non-editable) text label      */
                                      /* [verified: A+B dialog-manager.md Sec 2] */
#define FLAIR_DITL_EDIT_TEXT     16u  /* editable text field (drives textH/editField) */
                                      /* [verified: A+B dialog-manager.md Sec 2] */
#define FLAIR_DITL_ICON_ITEM     32u  /* icon ('ICON' resource)                */
                                      /* [verified: A+B dialog-manager.md Sec 2] */
#define FLAIR_DITL_PIC_ITEM      64u  /* QuickDraw picture ('PICT' resource)   */
                                      /* [verified: A+B dialog-manager.md Sec 2] */

/* itemDisable high-bit flag: OR'd with any base type to disable the item.
 * A disabled item is DRAWN and occupies its rect, but ModalDialog will NEVER
 * return it (it is filtered out of hit-testing before reporting itemHit).
 * [verified: A+B dialog-manager.md Sec 2 "item not returned by ModalDialog"] */
#define FLAIR_DITL_ITEM_DISABLE 0x80u /* 128 decimal; high bit of item-type byte */

/* Control sub-types: ADDED to FLAIR_DITL_CTRL_ITEM to form the full type byte.
 * [verified: A+B dialog-manager.md Sec 2 (btnCtrl=0 chkCtrl=1 radCtrl=2 resCtrl=3)] */
#define FLAIR_DITL_BTN_CTRL       0u  /* + ctrlItem(4) => 4: standard push button */
#define FLAIR_DITL_CHK_CTRL       1u  /* + ctrlItem(4) => 5: standard check box   */
#define FLAIR_DITL_RAD_CTRL       2u  /* + ctrlItem(4) => 6: standard radio button */
#define FLAIR_DITL_RES_CTRL       3u  /* + ctrlItem(4) => 7: CNTL-resource control */

/* Verbatim Inside Macintosh names (aliases for cross-reference ease). */
#define userItem     FLAIR_DITL_USER_ITEM
#define helpItem     FLAIR_DITL_HELP_ITEM
#define ctrlItem     FLAIR_DITL_CTRL_ITEM
#define statText     FLAIR_DITL_STAT_TEXT
#define editText     FLAIR_DITL_EDIT_TEXT
#define iconItem     FLAIR_DITL_ICON_ITEM
#define picItem      FLAIR_DITL_PIC_ITEM
#define itemDisable  FLAIR_DITL_ITEM_DISABLE
#define btnCtrl      FLAIR_DITL_BTN_CTRL
#define chkCtrl      FLAIR_DITL_CHK_CTRL
#define radCtrl      FLAIR_DITL_RAD_CTRL
#define resCtrl      FLAIR_DITL_RES_CTRL

/* Composed control-item bytes (the actual byte values for each control kind). */
#define FLAIR_DITL_PUSH_BUTTON    (FLAIR_DITL_CTRL_ITEM + FLAIR_DITL_BTN_CTRL)  /* 4 */
#define FLAIR_DITL_CHECK_BOX      (FLAIR_DITL_CTRL_ITEM + FLAIR_DITL_CHK_CTRL)  /* 5 */
#define FLAIR_DITL_RADIO_BUTTON   (FLAIR_DITL_CTRL_ITEM + FLAIR_DITL_RAD_CTRL)  /* 6 */
#define FLAIR_DITL_CNTL_RESOURCE  (FLAIR_DITL_CTRL_ITEM + FLAIR_DITL_RES_CTRL)  /* 7 */

/* ===========================================================================
 * 2. ALERT TYPE SELECTORS -- standard alert icon ICON resource IDs
 *    era=system7.0-7.1
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/dialog-manager.md Sec 4.1
 *      [verified: A+B; im-toolbox-records-verbatim.md (stopIcon=0 noteIcon=1
 *       cautionIcon=2) + IM-I p. I-409]:
 *
 * StopAlert/NoteAlert/CautionAlert draw the standard icon in the alert's
 * top-left corner. The values are indices/resource IDs into the System-file
 * ICON resources. Alert() (no icon) uses none of these.
 *
 * The RENDERED icon bitmaps (stop sign / note / caution triangle) are
 * [golden-resolves] owned by ../system7-decomp/specs/desktop/alerts.md;
 * this spec carries ONLY the selector constants.
 * ===========================================================================*/

#define FLAIR_ALERT_STOP_ICON      0   /* stopIcon=0:    error/cannot-proceed  */
                                       /* [verified: A+B dialog-manager.md Sec 4.1] */
#define FLAIR_ALERT_NOTE_ICON      1   /* noteIcon=1:    informational note    */
                                       /* [verified: A+B dialog-manager.md Sec 4.1] */
#define FLAIR_ALERT_CAUTION_ICON   2   /* cautionIcon=2: warning/proceed-with-care */
                                       /* [verified: A+B dialog-manager.md Sec 4.1] */

/* Verbatim IM names. */
#define stopIcon     FLAIR_ALERT_STOP_ICON
#define noteIcon     FLAIR_ALERT_NOTE_ICON
#define cautionIcon  FLAIR_ALERT_CAUTION_ICON

/* ===========================================================================
 * 3. ALERT STAGE MECHANISM  (4-stage StageList escalation)
 *    era=system7.0-7.1
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/dialog-manager.md Sec 4.2
 *      [documented: single src -- Toolbox-442 + WebSearch IM:Tb; Sec 7 ledger]:
 *
 * An ALRT resource carries a StageList of 4 stages (1..4). Each stage applies
 * to the Nth consecutive invocation of the same alert ID. The stage encodes:
 *   - the default (bold) item index
 *   - whether the alert box is drawn visibly
 *   - which of the 4 sound selectors (0..3) to play (0 = silent)
 *
 * The Dialog Manager auto-escalates: repeated alerts climb 1->2->3->4 (stick
 * at 4). GetAlertStage() queries the current stage. The per-stage BIT layout
 * (boldItm/visible/soundNum packed per stage) is the resource format, owned by
 * ../system7-decomp/specs/resources/wind-menu-dlog-ditl.md.
 * ===========================================================================*/

#define FLAIR_ALERT_STAGE_COUNT    4   /* 4 stages per ALRT StageList          */
                                       /* [documented: dialog-manager.md Sec 4.2] */
#define FLAIR_ALERT_SOUND_SILENT   0   /* sound number 0 = silent              */
                                       /* [documented: dialog-manager.md Sec 4.2] */
#define FLAIR_ALERT_SOUND_MIN      1   /* sound numbers 1..3 select sound1..3  */
#define FLAIR_ALERT_SOUND_MAX      3   /* [documented: dialog-manager.md Sec 4.2] */

/* ===========================================================================
 * 4. STANDARD RESULT CODES  (ok=1, cancel=2)
 *    era=system7.0-7.1
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/dialog-manager.md Sec 4.3
 *      [verified: A+B; im-toolbox-records-verbatim.md (ok=1 cancel=2) +
 *       Toolbox-442 (ok=1 first button, cancel=2 second button)]:
 *
 * These are CONVENTIONS the standard ALRT templates follow -- the OK button
 * is DITL item 1, Cancel is DITL item 2. They are NOT baked into the record.
 * aDefItem (the default-button field) is set to 1 by the standard templates.
 * ModalDialog returns these item numbers via its itemHit parameter.
 *
 * 1-based: item 0 means "no item."
 * ===========================================================================*/

#define FLAIR_DIALOG_OK            1   /* ok=1: the OK/default button is item 1 */
                                       /* [verified: A+B dialog-manager.md Sec 4.3] */
#define FLAIR_DIALOG_CANCEL        2   /* cancel=2: the Cancel button is item 2 */
                                       /* [verified: A+B dialog-manager.md Sec 4.3] */

/* Verbatim IM names (easier cross-reference to Inside Macintosh code samples). */
#define ok      FLAIR_DIALOG_OK
#define cancel  FLAIR_DIALOG_CANCEL

/* ===========================================================================
 * 5. DIALOGRECORD LAYOUT INVARIANTS
 *    era=system7.0-7.1
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/dialog-manager.md Sec 1 + Sec 6
 *      [verified: A+B; im-toolbox-records-verbatim.md + Toolbox-442 assembly-
 *       language summary (dWindow@0 size 156, items@156, teHandle@160,
 *       editField@164, editOpen@166, aDefItem@168)]:
 *
 * Key invariants this spec locks (checked by _Static_asserts below):
 *
 *  A. DialogPtr IS a WindowPtr IS a GrafPtr (window at offset 0).
 *     [verified: A+B -- "LOAD-BEARING invariant" in dialog-manager.md Sec 1]
 *     => FLAIR DialogRecord.window must be the FIRST field at offset 0.
 *     => _Static_assert: offsetof(DialogRecord, window) == 0.
 *
 *  B. windowKind inside the embedded WindowRecord is set to dialogKind(2).
 *     [verified: A+B; window-manager.md Sec 1.1 + dialog-manager.md Sec 1]
 *     => Checked at runtime in the oracle (field value, not a static assert).
 *
 *  C. The default-button item number (aDefItem / defaultItem) is 1-based.
 *     0 means no default item.
 *
 *  D. FLAIR deviation: items are a caller-supplied DialogItem[] struct array,
 *     NOT a packed Handle to a DITL resource (no resource fork in current
 *     release). The IM field NAME (items) is preserved. The enabled byte in
 *     DialogItem mirrors itemDisable semantics: enabled=0 <=> item is disabled
 *     (drawn but ModalDialog will NOT return it).
 *     [verified: dialog-manager.md Sec 6 deviations table]
 *
 * NOTE: The 68K offsets (dWindow@0/156, items@156, ...) are documented and
 * cited above for audit trail; they are NOT load-bearing for FLAIR because
 * FLAIR re-types the fields in C (dialog-manager.md Sec 1 says "not load-
 * bearing for FLAIR (which re-types the fields, Sec 6)").
 * ===========================================================================*/

/* ===========================================================================
 * 6. MODALDIALOG COOPERATIVE-LOOP CONTRACT
 *    era=system7.0-7.1
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/dialog-manager.md Sec 3
 *      [verified: A+B; Toolbox-442 signature + dialog-manager.md Sec 3]:
 *
 * ModalDialog(filterProc, VAR itemHit): repeatedly fetches events, optionally
 * passes them through the caller's filterProc, and returns the 1-based item
 * number of the first ENABLED item the user activated.
 *
 * FLAIR cooperative form (ADR-0004 D-6):
 *   - Drains flair_raw_ring_t via WaitNextEvent (task context, non-preemptive).
 *   - Takes a dialog_filter_fn (== filterProc).
 *   - Returns via *itemHit (1-based). 0 = no item (ring empty, sleepTicks=0).
 *   - sleepTicks=0 returns immediately on null event (for deterministic testing).
 *
 * Standard keyboard shortcuts (IM-I / MTE Ch 6):
 *   - Return or Enter activates aDefItem (the default button).
 *   - Escape or Cmd-. (period) activates the cancel item (convention; filterProc).
 *   [documented: dialog-manager.md Sec 3 + certainty ledger Sec 7 "[inferred]"]
 *
 * This contract is enforced by harness/proptest/test_dialog.c Property 2.
 * ===========================================================================*/

/* The item number meaning "no item hit / no default item / no cancel item." */
#define FLAIR_DIALOG_NO_ITEM       0   /* 0 = no item (0-based sentinel for 1-based items) */

/* ===========================================================================
 * 7. _Static_assert ORACLE GATES
 *    (mirror style: spec/window_record.h + spec/event_model.h)
 * ---------------------------------------------------------------------------
 * All numeric constants are locked below. Each assert documents:
 *   - the value being checked,
 *   - the IM source,
 *   - the dialog-manager.md section.
 * Any change to a value above MUST update the matching assert and carry a
 * Rule 8 worklog note + deliberate beads-issue justification.
 * ===========================================================================*/

/* --- DITL item-type byte values ------------------------------------------- */

_Static_assert(FLAIR_DITL_USER_ITEM == 0u,
    "userItem must be 0 "
    "[verified: A+B dialog-manager.md Sec 2; IM-I p. I-403]");

_Static_assert(FLAIR_DITL_HELP_ITEM == 1u,
    "helpItem must be 1 (System 7 balloon-help; standalone type) "
    "[verified: A+B dialog-manager.md Sec 2; Toolbox-442]");

_Static_assert(FLAIR_DITL_CTRL_ITEM == 4u,
    "ctrlItem base must be 4 "
    "[verified: A+B dialog-manager.md Sec 2; IM-I p. I-403]");

_Static_assert(FLAIR_DITL_STAT_TEXT == 8u,
    "statText must be 8 "
    "[verified: A+B dialog-manager.md Sec 2; IM-I p. I-403]");

_Static_assert(FLAIR_DITL_EDIT_TEXT == 16u,
    "editText must be 16 "
    "[verified: A+B dialog-manager.md Sec 2; IM-I p. I-403]");

_Static_assert(FLAIR_DITL_ICON_ITEM == 32u,
    "iconItem must be 32 "
    "[verified: A+B dialog-manager.md Sec 2; IM-I p. I-403]");

_Static_assert(FLAIR_DITL_PIC_ITEM == 64u,
    "picItem must be 64 "
    "[verified: A+B dialog-manager.md Sec 2; IM-I p. I-403]");

/* --- itemDisable high-bit flag -------------------------------------------- */

_Static_assert(FLAIR_DITL_ITEM_DISABLE == 0x80u,
    "itemDisable must be 0x80 (128; high bit of item-type byte) "
    "[verified: A+B dialog-manager.md Sec 2; Toolbox-442]");

/* --- Control sub-types (added to ctrlItem=4) ------------------------------- */

_Static_assert(FLAIR_DITL_BTN_CTRL == 0u,
    "btnCtrl sub-type must be 0 (push button; ctrlItem+0=4) "
    "[verified: A+B dialog-manager.md Sec 2; Toolbox-442]");

_Static_assert(FLAIR_DITL_CHK_CTRL == 1u,
    "chkCtrl sub-type must be 1 (check box; ctrlItem+1=5) "
    "[verified: A+B dialog-manager.md Sec 2; Toolbox-442]");

_Static_assert(FLAIR_DITL_RAD_CTRL == 2u,
    "radCtrl sub-type must be 2 (radio button; ctrlItem+2=6) "
    "[verified: A+B dialog-manager.md Sec 2; Toolbox-442]");

_Static_assert(FLAIR_DITL_RES_CTRL == 3u,
    "resCtrl sub-type must be 3 (CNTL resource; ctrlItem+3=7) "
    "[verified: A+B dialog-manager.md Sec 2; Toolbox-442]");

/* --- Composed push-button byte = ctrlItem+btnCtrl = 4 -------------------- */

_Static_assert(FLAIR_DITL_PUSH_BUTTON == 4u,
    "push button DITL type must be ctrlItem+btnCtrl = 4 "
    "[verified: A+B dialog-manager.md Sec 2 worked example]");

_Static_assert(FLAIR_DITL_CHECK_BOX == 5u,
    "check box DITL type must be ctrlItem+chkCtrl = 5 "
    "[verified: A+B dialog-manager.md Sec 2 worked example]");

_Static_assert(FLAIR_DITL_RADIO_BUTTON == 6u,
    "radio button DITL type must be ctrlItem+radCtrl = 6 "
    "[verified: A+B dialog-manager.md Sec 2 worked example]");

_Static_assert(FLAIR_DITL_CNTL_RESOURCE == 7u,
    "CNTL-resource control DITL type must be ctrlItem+resCtrl = 7 "
    "[verified: A+B dialog-manager.md Sec 2 worked example]");

/* --- Alert icon selectors ------------------------------------------------- */

_Static_assert(FLAIR_ALERT_STOP_ICON == 0,
    "stopIcon must be 0 (ICON resource ID for stop/error icon) "
    "[verified: A+B dialog-manager.md Sec 4.1; IM-I p. I-409]");

_Static_assert(FLAIR_ALERT_NOTE_ICON == 1,
    "noteIcon must be 1 (ICON resource ID for note/info icon) "
    "[verified: A+B dialog-manager.md Sec 4.1; IM-I p. I-409]");

_Static_assert(FLAIR_ALERT_CAUTION_ICON == 2,
    "cautionIcon must be 2 (ICON resource ID for caution/warning icon) "
    "[verified: A+B dialog-manager.md Sec 4.1; IM-I p. I-409]");

/* --- Alert stage mechanism ------------------------------------------------ */

_Static_assert(FLAIR_ALERT_STAGE_COUNT == 4,
    "StageList must have exactly 4 stages (1..4) "
    "[documented: dialog-manager.md Sec 4.2; Toolbox-442 GetAlertStage]");

_Static_assert(FLAIR_ALERT_SOUND_SILENT == 0,
    "alert sound number 0 must be silent "
    "[documented: dialog-manager.md Sec 4.2]");

_Static_assert(FLAIR_ALERT_SOUND_MIN == 1,
    "alert sound numbers start at 1 "
    "[documented: dialog-manager.md Sec 4.2]");

_Static_assert(FLAIR_ALERT_SOUND_MAX == 3,
    "alert sound numbers end at 3 (sound1/sound2/sound3) "
    "[documented: dialog-manager.md Sec 4.2]");

/* --- Standard result codes ok=1, cancel=2 --------------------------------- */

_Static_assert(FLAIR_DIALOG_OK == 1,
    "ok must be 1 (default OK button is DITL item 1; 1-based) "
    "[verified: A+B dialog-manager.md Sec 4.3; Toolbox-442]");

_Static_assert(FLAIR_DIALOG_CANCEL == 2,
    "cancel must be 2 (Cancel button is DITL item 2 by convention; 1-based) "
    "[verified: A+B dialog-manager.md Sec 4.3; Toolbox-442]");

_Static_assert(FLAIR_DIALOG_NO_ITEM == 0,
    "no-item sentinel must be 0 (item numbers are 1-based; 0 = no item) "
    "[verified: A+B dialog-manager.md Sec 2 + Sec 4.3]");

/* --- itemDisable semantics cross-check: disabled byte has high bit set ----- */

_Static_assert((FLAIR_DITL_STAT_TEXT | FLAIR_DITL_ITEM_DISABLE) == (8u | 0x80u),
    "disabled statText = statText | itemDisable = 0x88 "
    "[verified: dialog-manager.md Sec 2; disabled statText is drawn but not returned]");

_Static_assert((FLAIR_DITL_ICON_ITEM | FLAIR_DITL_ITEM_DISABLE) == (32u | 0x80u),
    "disabled iconItem = iconItem | itemDisable = 0xA0 "
    "[verified: dialog-manager.md Sec 2; icons are typically disabled]");

/* --- windowKind cross-check (consistency with spec/window_record.h) ------- */

_Static_assert(dialogKind == 2,
    "dialogKind must be 2 (IM-I p. I-270; dialog-manager.md Sec 1; "
    "spec/window_record.h FLAIR_WINDOW_KIND_DIALOG) "
    "[verified: A+B dialog-manager.md Sec 1]");

/* --- dBoxProc border cross-check (consistency with spec/chrome_metrics.h) - */

_Static_assert(FLAIR_CHROME_DIALOG_BORDER == 7,
    "dBoxProc border must be 7 px (WDEF dBoxBorderSize EQU 7; "
    "spec/chrome_metrics.h; dialog-manager.md Sec 6) "
    "[verified: gui-ground-truth Sec 1.4 dBoxBorderSize EQU 7]");

/* --- dBoxProc variant cross-check (consistency with spec/window_record.h) - */

_Static_assert(dBoxProc == 1,
    "dBoxProc variant must be 1 (MTE Table 4-1; spec/window_record.h; "
    "dialog-manager.md Sec 6 'dBoxProc/movableDBoxProc window variant') "
    "[verified: A+B; window_record.h + dialog-manager.md Sec 6]");

/* --- Numeric ordering sanity (itemDisable is the high bit, above all types) */

_Static_assert(FLAIR_DITL_PIC_ITEM < FLAIR_DITL_ITEM_DISABLE,
    "itemDisable (0x80) must be strictly greater than picItem (64) "
    "-- itemDisable is the high bit, not a base type "
    "[verified: A+B dialog-manager.md Sec 2]");

#endif /* INITECH_SPEC_DIALOG_RECORD_H */
