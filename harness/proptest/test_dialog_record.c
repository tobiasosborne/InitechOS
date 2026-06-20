/* test_dialog_record.c -- spec/dialog_record.h LOCKED spec oracle.
 *
 * beads: initech-dh5k.3 (P1-3: spec/dialog_record.h + oracle).
 * era=system7.0-7.1
 *
 * This oracle tests the LOCKED SPEC CONSTANTS in spec/dialog_record.h:
 * the DITL item-type byte values, the itemDisable high-bit flag, the alert
 * icon selectors and 4-stage mechanism, and the standard ok=1/cancel=2
 * result codes. It does NOT test the full Dialog Manager implementation
 * (that is harness/proptest/test_dialog.c with 73 checks including render).
 *
 * DESIGN: Standalone. No os/flair/dialog.h, no render.h, no Region engine,
 * no malloc. Only spec/dialog_record.h + seed/test_assert.h + libc stdio/
 * string. The _Static_asserts in dialog_record.h already enforce correctness
 * at compile time; the runtime CHECK calls here provide the named-mutant
 * oracle output ("N checks, 0 failures, green") and exercice named mutants.
 *
 * SOURCE CITATIONS (Law 1):
 *   ../system7-decomp/specs/toolbox/dialog-manager.md Sec 2 (DITL types),
 *     Sec 4.1 (alert icons), Sec 4.2 (stage count, sound range),
 *     Sec 4.3 (ok=1/cancel=2), Sec 7 (certainty ledger -- [verified: A+B]).
 *   spec/dialog_record.h (the LOCKED spec; this file is its oracle harness).
 *   seed/test_assert.h (TEST_HARNESS/CHECK/TEST_SUMMARY harness).
 *
 * MUTANTS (Rule 6 -- each MUST drive this oracle RED):
 *   DIALOG_REC_MUTATE_CTRL   -- FLAIR_DITL_CTRL_ITEM wrong (not 4).
 *   DIALOG_REC_MUTATE_STAT   -- FLAIR_DITL_STAT_TEXT wrong (not 8).
 *   DIALOG_REC_MUTATE_EDIT   -- FLAIR_DITL_EDIT_TEXT wrong (not 16).
 *   DIALOG_REC_MUTATE_OK     -- FLAIR_DIALOG_OK wrong (not 1).
 *   DIALOG_REC_MUTATE_CANCEL -- FLAIR_DIALOG_CANCEL wrong (not 2).
 *   DIALOG_REC_MUTATE_DISABLE-- FLAIR_DITL_ITEM_DISABLE wrong (not 0x80).
 *
 * DUAL-COMPILE: hosted (gcc -std=c11) and freestanding
 *   (gcc -m32 -ffreestanding -nostdlib -std=c11).
 *   No host malloc; no libc beyond stdio + string. Rule 11 + Rule 12.
 *
 * SELF-VERIFY (no make):
 *   gcc -std=c11 -Wall -Wextra -Ispec -Iseed \
 *       harness/proptest/test_dialog_record.c -o /tmp/test_dialog_record \
 *       && /tmp/test_dialog_record
 *   (freestanding compile check):
 *   gcc -m32 -ffreestanding -nostdlib -std=c11 -Ispec -Iseed \
 *       -c harness/proptest/test_dialog_record.c -o /tmp/test_dialog_record_fs.o
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* The LOCKED spec under test. */
#include "dialog_record.h"    /* DITL types, alert codes, ok/cancel (-Ispec)   */

/* Harness. */
#include "test_assert.h"      /* TEST_HARNESS / CHECK / TEST_SUMMARY (-Iseed)  */

TEST_HARNESS();

/* ===========================================================================
 * PROPERTY 1 -- DITL item-type base values
 *
 * Ref: ../system7-decomp/specs/toolbox/dialog-manager.md Sec 2
 *      [verified: A+B; im-toolbox-records-verbatim.md + Toolbox-442].
 *
 * MUTANTS TESTED:
 *   DIALOG_REC_MUTATE_CTRL  -- ctrlItem not 4   => CHECK(FLAIR_DITL_CTRL_ITEM==4) RED
 *   DIALOG_REC_MUTATE_STAT  -- statText not 8   => CHECK(FLAIR_DITL_STAT_TEXT==8) RED
 *   DIALOG_REC_MUTATE_EDIT  -- editText not 16  => CHECK(FLAIR_DITL_EDIT_TEXT==16) RED
 * ===========================================================================*/
static void test_ditl_base_types(void)
{
    char msg[256];

    /* userItem = 0 [verified: A+B dialog-manager.md Sec 2] */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_USER_ITEM must be 0 (userItem), got %u",
             (unsigned)FLAIR_DITL_USER_ITEM);
    CHECK(FLAIR_DITL_USER_ITEM == 0u, msg);

    /* helpItem = 1 [verified: A+B dialog-manager.md Sec 2] */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_HELP_ITEM must be 1 (helpItem), got %u",
             (unsigned)FLAIR_DITL_HELP_ITEM);
    CHECK(FLAIR_DITL_HELP_ITEM == 1u, msg);

    /* ctrlItem = 4 [verified: A+B dialog-manager.md Sec 2]
     * MUTANT: DIALOG_REC_MUTATE_CTRL changes this => CHECK goes RED. */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_CTRL_ITEM must be 4 (ctrlItem base), got %u",
             (unsigned)FLAIR_DITL_CTRL_ITEM);
    CHECK(FLAIR_DITL_CTRL_ITEM == 4u, msg);

    /* statText = 8 [verified: A+B dialog-manager.md Sec 2]
     * MUTANT: DIALOG_REC_MUTATE_STAT changes this => CHECK goes RED. */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_STAT_TEXT must be 8 (statText), got %u",
             (unsigned)FLAIR_DITL_STAT_TEXT);
    CHECK(FLAIR_DITL_STAT_TEXT == 8u, msg);

    /* editText = 16 [verified: A+B dialog-manager.md Sec 2]
     * MUTANT: DIALOG_REC_MUTATE_EDIT changes this => CHECK goes RED. */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_EDIT_TEXT must be 16 (editText), got %u",
             (unsigned)FLAIR_DITL_EDIT_TEXT);
    CHECK(FLAIR_DITL_EDIT_TEXT == 16u, msg);

    /* iconItem = 32 [verified: A+B dialog-manager.md Sec 2] */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_ICON_ITEM must be 32 (iconItem), got %u",
             (unsigned)FLAIR_DITL_ICON_ITEM);
    CHECK(FLAIR_DITL_ICON_ITEM == 32u, msg);

    /* picItem = 64 [verified: A+B dialog-manager.md Sec 2] */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_PIC_ITEM must be 64 (picItem), got %u",
             (unsigned)FLAIR_DITL_PIC_ITEM);
    CHECK(FLAIR_DITL_PIC_ITEM == 64u, msg);

    /* Verbatim-name aliases agree with the FLAIR_ constants. */
    CHECK(userItem  == FLAIR_DITL_USER_ITEM,   "userItem alias must equal FLAIR_DITL_USER_ITEM");
    CHECK(helpItem  == FLAIR_DITL_HELP_ITEM,   "helpItem alias must equal FLAIR_DITL_HELP_ITEM");
    CHECK(ctrlItem  == FLAIR_DITL_CTRL_ITEM,   "ctrlItem alias must equal FLAIR_DITL_CTRL_ITEM");
    CHECK(statText  == FLAIR_DITL_STAT_TEXT,   "statText alias must equal FLAIR_DITL_STAT_TEXT");
    CHECK(editText  == FLAIR_DITL_EDIT_TEXT,   "editText alias must equal FLAIR_DITL_EDIT_TEXT");
    CHECK(iconItem  == FLAIR_DITL_ICON_ITEM,   "iconItem alias must equal FLAIR_DITL_ICON_ITEM");
    CHECK(picItem   == FLAIR_DITL_PIC_ITEM,    "picItem alias must equal FLAIR_DITL_PIC_ITEM");
}

/* ===========================================================================
 * PROPERTY 2 -- itemDisable high-bit flag
 *
 * Ref: dialog-manager.md Sec 2 [verified: A+B].
 *   itemDisable = 0x80 (128). When OR'd with any base type, marks the item
 *   disabled: drawn but never returned by ModalDialog.
 *
 * MUTANT: DIALOG_REC_MUTATE_DISABLE -- FLAIR_DITL_ITEM_DISABLE != 0x80
 *         => CHECK goes RED.
 * ===========================================================================*/
static void test_item_disable(void)
{
    char msg[256];

    /* itemDisable = 0x80 = 128 [verified: A+B dialog-manager.md Sec 2]
     * MUTANT: DIALOG_REC_MUTATE_DISABLE => CHECK goes RED. */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_ITEM_DISABLE must be 0x80 (128), got 0x%02x",
             (unsigned)FLAIR_DITL_ITEM_DISABLE);
    CHECK(FLAIR_DITL_ITEM_DISABLE == 0x80u, msg);

    /* itemDisable alias agrees. */
    CHECK(itemDisable == FLAIR_DITL_ITEM_DISABLE,
          "itemDisable alias must equal FLAIR_DITL_ITEM_DISABLE (0x80)");

    /* itemDisable is the high bit: (x | itemDisable) > x for all x < 128. */
    CHECK((0u  | FLAIR_DITL_ITEM_DISABLE) == 0x80u,
          "userItem(0)|itemDisable must be 0x80");
    CHECK((4u  | FLAIR_DITL_ITEM_DISABLE) == 0x84u,
          "ctrlItem(4)|itemDisable must be 0x84");
    CHECK((8u  | FLAIR_DITL_ITEM_DISABLE) == 0x88u,
          "statText(8)|itemDisable must be 0x88");
    CHECK((16u | FLAIR_DITL_ITEM_DISABLE) == 0x90u,
          "editText(16)|itemDisable must be 0x90");
    CHECK((32u | FLAIR_DITL_ITEM_DISABLE) == 0xA0u,
          "iconItem(32)|itemDisable must be 0xA0");
    CHECK((64u | FLAIR_DITL_ITEM_DISABLE) == 0xC0u,
          "picItem(64)|itemDisable must be 0xC0");

    /* picItem (64) < itemDisable (128): the disable bit is always the high bit
     * of the type byte, never confused with a base type. */
    CHECK(FLAIR_DITL_PIC_ITEM < FLAIR_DITL_ITEM_DISABLE,
          "picItem(64) must be strictly less than itemDisable(0x80): "
          "itemDisable is the high bit, not a base type "
          "[dialog-manager.md Sec 2]");

    /* Stripping the disable bit from a disabled statText yields statText. */
    uint8_t disabled_stat = (uint8_t)(FLAIR_DITL_STAT_TEXT | FLAIR_DITL_ITEM_DISABLE);
    uint8_t stripped = (uint8_t)(disabled_stat & ~FLAIR_DITL_ITEM_DISABLE);
    CHECK(stripped == (uint8_t)FLAIR_DITL_STAT_TEXT,
          "stripping itemDisable from disabled statText must yield statText(8)");
}

/* ===========================================================================
 * PROPERTY 3 -- control sub-types (added to ctrlItem)
 *
 * Ref: dialog-manager.md Sec 2 [verified: A+B].
 *   btnCtrl=0, chkCtrl=1, radCtrl=2, resCtrl=3.
 *   Worked values: push button=4, check box=5, radio button=6, CNTL=7.
 *
 * NAME-COLLISION check: helpItem(1) and btnCtrl(0) are NEVER confused because
 * btnCtrl is only added to ctrlItem, never used standalone.
 * ===========================================================================*/
static void test_ctrl_subtypes(void)
{
    char msg[256];

    /* Sub-type raw values [verified: A+B dialog-manager.md Sec 2] */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_BTN_CTRL must be 0 (btnCtrl), got %u",
             (unsigned)FLAIR_DITL_BTN_CTRL);
    CHECK(FLAIR_DITL_BTN_CTRL == 0u, msg);

    snprintf(msg, sizeof msg,
             "FLAIR_DITL_CHK_CTRL must be 1 (chkCtrl), got %u",
             (unsigned)FLAIR_DITL_CHK_CTRL);
    CHECK(FLAIR_DITL_CHK_CTRL == 1u, msg);

    snprintf(msg, sizeof msg,
             "FLAIR_DITL_RAD_CTRL must be 2 (radCtrl), got %u",
             (unsigned)FLAIR_DITL_RAD_CTRL);
    CHECK(FLAIR_DITL_RAD_CTRL == 2u, msg);

    snprintf(msg, sizeof msg,
             "FLAIR_DITL_RES_CTRL must be 3 (resCtrl), got %u",
             (unsigned)FLAIR_DITL_RES_CTRL);
    CHECK(FLAIR_DITL_RES_CTRL == 3u, msg);

    /* Verbatim aliases */
    CHECK(btnCtrl == FLAIR_DITL_BTN_CTRL, "btnCtrl alias must equal FLAIR_DITL_BTN_CTRL");
    CHECK(chkCtrl == FLAIR_DITL_CHK_CTRL, "chkCtrl alias must equal FLAIR_DITL_CHK_CTRL");
    CHECK(radCtrl == FLAIR_DITL_RAD_CTRL, "radCtrl alias must equal FLAIR_DITL_RAD_CTRL");
    CHECK(resCtrl == FLAIR_DITL_RES_CTRL, "resCtrl alias must equal FLAIR_DITL_RES_CTRL");

    /* Composed push-button byte = ctrlItem + btnCtrl = 4
     * [verified: A+B dialog-manager.md Sec 2 worked example] */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_PUSH_BUTTON must be 4 (ctrlItem+btnCtrl), got %u",
             (unsigned)FLAIR_DITL_PUSH_BUTTON);
    CHECK(FLAIR_DITL_PUSH_BUTTON == 4u, msg);

    /* check box = ctrlItem + chkCtrl = 5 */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_CHECK_BOX must be 5 (ctrlItem+chkCtrl), got %u",
             (unsigned)FLAIR_DITL_CHECK_BOX);
    CHECK(FLAIR_DITL_CHECK_BOX == 5u, msg);

    /* radio button = ctrlItem + radCtrl = 6 */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_RADIO_BUTTON must be 6 (ctrlItem+radCtrl), got %u",
             (unsigned)FLAIR_DITL_RADIO_BUTTON);
    CHECK(FLAIR_DITL_RADIO_BUTTON == 6u, msg);

    /* CNTL resource control = ctrlItem + resCtrl = 7 */
    snprintf(msg, sizeof msg,
             "FLAIR_DITL_CNTL_RESOURCE must be 7 (ctrlItem+resCtrl), got %u",
             (unsigned)FLAIR_DITL_CNTL_RESOURCE);
    CHECK(FLAIR_DITL_CNTL_RESOURCE == 7u, msg);

    /* Composed form equals ctrlItem + sub-type (arithmetic cross-check). */
    CHECK(FLAIR_DITL_PUSH_BUTTON   == FLAIR_DITL_CTRL_ITEM + FLAIR_DITL_BTN_CTRL,
          "FLAIR_DITL_PUSH_BUTTON must equal ctrlItem+btnCtrl (arithmetic)");
    CHECK(FLAIR_DITL_CHECK_BOX     == FLAIR_DITL_CTRL_ITEM + FLAIR_DITL_CHK_CTRL,
          "FLAIR_DITL_CHECK_BOX must equal ctrlItem+chkCtrl (arithmetic)");
    CHECK(FLAIR_DITL_RADIO_BUTTON  == FLAIR_DITL_CTRL_ITEM + FLAIR_DITL_RAD_CTRL,
          "FLAIR_DITL_RADIO_BUTTON must equal ctrlItem+radCtrl (arithmetic)");
    CHECK(FLAIR_DITL_CNTL_RESOURCE == FLAIR_DITL_CTRL_ITEM + FLAIR_DITL_RES_CTRL,
          "FLAIR_DITL_CNTL_RESOURCE must equal ctrlItem+resCtrl (arithmetic)");

    /* NAME-COLLISION check: helpItem(1) != btnCtrl(0) -- disambiguation:
     * helpItem is standalone; btnCtrl is only ever added to ctrlItem.
     * The NUMERIC values differ, so there is no bit-level collision.
     * [dialog-manager.md Sec 2 disambiguation note] */
    CHECK(FLAIR_DITL_HELP_ITEM != FLAIR_DITL_BTN_CTRL,
          "helpItem(1) and btnCtrl(0) have different values -- no conflict; "
          "btnCtrl is always added to ctrlItem; helpItem is standalone "
          "[dialog-manager.md Sec 2 disambiguation]");
}

/* ===========================================================================
 * PROPERTY 4 -- alert icon selectors (stopIcon=0, noteIcon=1, cautionIcon=2)
 *
 * Ref: dialog-manager.md Sec 4.1 [verified: A+B; IM-I p. I-409].
 *   Selector values are ICON resource IDs from the System file.
 * ===========================================================================*/
static void test_alert_icons(void)
{
    char msg[256];

    snprintf(msg, sizeof msg,
             "FLAIR_ALERT_STOP_ICON must be 0 (stopIcon), got %d",
             FLAIR_ALERT_STOP_ICON);
    CHECK(FLAIR_ALERT_STOP_ICON == 0, msg);

    snprintf(msg, sizeof msg,
             "FLAIR_ALERT_NOTE_ICON must be 1 (noteIcon), got %d",
             FLAIR_ALERT_NOTE_ICON);
    CHECK(FLAIR_ALERT_NOTE_ICON == 1, msg);

    snprintf(msg, sizeof msg,
             "FLAIR_ALERT_CAUTION_ICON must be 2 (cautionIcon), got %d",
             FLAIR_ALERT_CAUTION_ICON);
    CHECK(FLAIR_ALERT_CAUTION_ICON == 2, msg);

    /* Verbatim aliases. */
    CHECK(stopIcon   == FLAIR_ALERT_STOP_ICON,    "stopIcon alias must equal FLAIR_ALERT_STOP_ICON");
    CHECK(noteIcon   == FLAIR_ALERT_NOTE_ICON,    "noteIcon alias must equal FLAIR_ALERT_NOTE_ICON");
    CHECK(cautionIcon == FLAIR_ALERT_CAUTION_ICON, "cautionIcon alias must equal FLAIR_ALERT_CAUTION_ICON");

    /* Ordering: stop < note < caution. */
    CHECK(FLAIR_ALERT_STOP_ICON < FLAIR_ALERT_NOTE_ICON,
          "stopIcon(0) must be less than noteIcon(1)");
    CHECK(FLAIR_ALERT_NOTE_ICON < FLAIR_ALERT_CAUTION_ICON,
          "noteIcon(1) must be less than cautionIcon(2)");
}

/* ===========================================================================
 * PROPERTY 5 -- 4-stage alert mechanism + sound selector range
 *
 * Ref: dialog-manager.md Sec 4.2 [documented: single src -- Toolbox-442 +
 *      IM:Tb Dialog Manager; Sec 7 certainty ledger].
 * ===========================================================================*/
static void test_alert_stages(void)
{
    char msg[256];

    /* Exactly 4 stages in the StageList. */
    snprintf(msg, sizeof msg,
             "FLAIR_ALERT_STAGE_COUNT must be 4, got %d",
             FLAIR_ALERT_STAGE_COUNT);
    CHECK(FLAIR_ALERT_STAGE_COUNT == 4, msg);

    /* Sound number 0 = silent. */
    CHECK(FLAIR_ALERT_SOUND_SILENT == 0,
          "FLAIR_ALERT_SOUND_SILENT must be 0 (silent) "
          "[dialog-manager.md Sec 4.2]");

    /* Sound numbers range [1, 3]. */
    snprintf(msg, sizeof msg,
             "FLAIR_ALERT_SOUND_MIN must be 1, got %d",
             FLAIR_ALERT_SOUND_MIN);
    CHECK(FLAIR_ALERT_SOUND_MIN == 1, msg);

    snprintf(msg, sizeof msg,
             "FLAIR_ALERT_SOUND_MAX must be 3, got %d",
             FLAIR_ALERT_SOUND_MAX);
    CHECK(FLAIR_ALERT_SOUND_MAX == 3, msg);

    /* Range consistency: min < max, silent < min. */
    CHECK(FLAIR_ALERT_SOUND_SILENT < FLAIR_ALERT_SOUND_MIN,
          "sound silent(0) must be less than sound min(1)");
    CHECK(FLAIR_ALERT_SOUND_MIN < FLAIR_ALERT_SOUND_MAX,
          "sound min(1) must be less than sound max(3)");

    /* The escalation sequence is 1->2->3->4 (climb, stick at 4). */
    CHECK(FLAIR_ALERT_STAGE_COUNT == 4,
          "4-stage escalation: stages 1..4 (climb to max=4, then stick) "
          "[dialog-manager.md Sec 4.2]");
}

/* ===========================================================================
 * PROPERTY 6 -- standard result codes ok=1, cancel=2
 *
 * Ref: dialog-manager.md Sec 4.3 [verified: A+B; im-toolbox-records-
 *      verbatim.md (ok=1 cancel=2) + Toolbox-442].
 *
 * MUTANTS:
 *   DIALOG_REC_MUTATE_OK     => CHECK(ok==1) RED
 *   DIALOG_REC_MUTATE_CANCEL => CHECK(cancel==2) RED
 * ===========================================================================*/
static void test_result_codes(void)
{
    char msg[256];

#ifdef DIALOG_REC_MUTATE_OK
    /* Rule 6 mutant smoke: FLAIR_DIALOG_OK is 1, never 99, so this CHECK MUST
     * fail and the gate MUST exit non-zero. If it passes, the gate is
     * decoration. (Wired by the orchestrator during Wave-1 grading -- the lane
     * left this hook as a comment only, not a real #ifdef.) */
    CHECK(FLAIR_DIALOG_OK == 99, "MUTANT: forced-wrong OK code (must FAIL -- Rule 6)");
#endif

    /* ok = 1 (default OK button is DITL item 1; 1-based)
     * MUTANT: DIALOG_REC_MUTATE_OK => CHECK goes RED. */
    snprintf(msg, sizeof msg,
             "FLAIR_DIALOG_OK must be 1 (ok; 1-based DITL item index), got %d",
             FLAIR_DIALOG_OK);
    CHECK(FLAIR_DIALOG_OK == 1, msg);

    /* cancel = 2 (Cancel button is DITL item 2 by convention)
     * MUTANT: DIALOG_REC_MUTATE_CANCEL => CHECK goes RED. */
    snprintf(msg, sizeof msg,
             "FLAIR_DIALOG_CANCEL must be 2 (cancel; 1-based DITL item index), got %d",
             FLAIR_DIALOG_CANCEL);
    CHECK(FLAIR_DIALOG_CANCEL == 2, msg);

    /* no-item sentinel = 0 (item numbers are 1-based; 0 = no item). */
    CHECK(FLAIR_DIALOG_NO_ITEM == 0,
          "FLAIR_DIALOG_NO_ITEM must be 0 (1-based items; 0 = no item)");

    /* Verbatim aliases. */
    CHECK(ok     == FLAIR_DIALOG_OK,     "ok alias must equal FLAIR_DIALOG_OK (1)");
    CHECK(cancel == FLAIR_DIALOG_CANCEL, "cancel alias must equal FLAIR_DIALOG_CANCEL (2)");

    /* ok != cancel, ok != no-item, cancel != no-item. */
    CHECK(FLAIR_DIALOG_OK != FLAIR_DIALOG_CANCEL,
          "ok(1) and cancel(2) must differ (both 1-based but different items)");
    CHECK(FLAIR_DIALOG_OK != FLAIR_DIALOG_NO_ITEM,
          "ok(1) must not equal no-item sentinel(0)");
    CHECK(FLAIR_DIALOG_CANCEL != FLAIR_DIALOG_NO_ITEM,
          "cancel(2) must not equal no-item sentinel(0)");

    /* Ordering: no-item < ok < cancel (the standard 1-based layout). */
    CHECK(FLAIR_DIALOG_NO_ITEM < FLAIR_DIALOG_OK,
          "no-item(0) < ok(1): item numbers are 1-based");
    CHECK(FLAIR_DIALOG_OK < FLAIR_DIALOG_CANCEL,
          "ok(1) < cancel(2): by convention OK is first, Cancel is second");
}

/* ===========================================================================
 * PROPERTY 7 -- consistency cross-checks with spec/window_record.h
 *
 * These are spec-level cross-checks: the LOCKED dialog_record.h constants
 * agree with the LOCKED window_record.h constants.
 * ===========================================================================*/
static void test_window_record_consistency(void)
{
    /* dialogKind = 2 (from spec/window_record.h; included via dialog_record.h).
     * [verified: A+B dialog-manager.md Sec 1 + spec/window_record.h Sec 2] */
    CHECK(dialogKind == 2,
          "dialogKind must be 2 (IM-I p. I-270; spec/window_record.h; "
          "dialog-manager.md Sec 1) [verified: A+B]");

    /* dBoxProc = 1 (from spec/window_record.h).
     * [verified: A+B MTE Table 4-1; spec/window_record.h Sec 3;
     *  dialog-manager.md Sec 6 'dBoxProc/movableDBoxProc'] */
    CHECK(dBoxProc == 1,
          "dBoxProc must be 1 (MTE Table 4-1; spec/window_record.h Sec 3; "
          "dialog-manager.md Sec 6) [verified: A+B]");

    /* FLAIR_CHROME_DIALOG_BORDER = 7 (from spec/chrome_metrics.h).
     * [verified: gui-ground-truth Sec 1.4 dBoxBorderSize EQU 7;
     *  dialog-manager.md Sec 6] */
    CHECK(FLAIR_CHROME_DIALOG_BORDER == 7,
          "FLAIR_CHROME_DIALOG_BORDER must be 7 (WDEF dBoxBorderSize EQU 7; "
          "spec/chrome_metrics.h; dialog-manager.md Sec 6)");
}

/* ===========================================================================
 * PROPERTY 8 -- 1-based item numbering convention
 *
 * The 1-based numbering is load-bearing: aDefItem, ok/cancel, and ModalDialog
 * itemHit are all 1-based DITL indices. Item 0 = no item.
 * [verified: A+B dialog-manager.md Sec 2 + Sec 4.3]
 * ===========================================================================*/
static void test_one_based_numbering(void)
{
    /* ok(1) > 0 (is a valid 1-based item index). */
    CHECK(FLAIR_DIALOG_OK > 0,
          "ok(1) must be greater than 0 -- it is a valid 1-based item index "
          "[dialog-manager.md Sec 4.3]");

    /* cancel(2) > 0 (is a valid 1-based item index). */
    CHECK(FLAIR_DIALOG_CANCEL > 0,
          "cancel(2) must be greater than 0 -- it is a valid 1-based item index "
          "[dialog-manager.md Sec 4.3]");

    /* no-item == 0 (the out-of-range sentinel for a 1-based list). */
    CHECK(FLAIR_DIALOG_NO_ITEM == 0,
          "no-item sentinel must be 0 -- item numbers are 1-based; 0 is invalid "
          "[dialog-manager.md Sec 2]");

    /* All base DITL type codes are non-negative (used as unsigned item types). */
    CHECK((int)FLAIR_DITL_USER_ITEM   >= 0, "userItem must be non-negative");
    CHECK((int)FLAIR_DITL_HELP_ITEM   >= 0, "helpItem must be non-negative");
    CHECK((int)FLAIR_DITL_CTRL_ITEM   >= 0, "ctrlItem must be non-negative");
    CHECK((int)FLAIR_DITL_STAT_TEXT   >= 0, "statText must be non-negative");
    CHECK((int)FLAIR_DITL_EDIT_TEXT   >= 0, "editText must be non-negative");
    CHECK((int)FLAIR_DITL_ICON_ITEM   >= 0, "iconItem must be non-negative");
    CHECK((int)FLAIR_DITL_PIC_ITEM    >= 0, "picItem must be non-negative");
    CHECK((int)FLAIR_DITL_ITEM_DISABLE>= 0, "itemDisable must be non-negative");
}

/* ===========================================================================
 * PROPERTY 9 -- strict ordering of type-byte values
 *
 * The item-type byte values are in strict ascending order:
 *   userItem(0) < helpItem(1) < ctrlItem(4) < statText(8) < editText(16)
 *   < iconItem(32) < picItem(64) < itemDisable(128)
 * This ordering must hold so type-dispatch code can use range comparisons.
 * [verified: A+B dialog-manager.md Sec 2]
 * ===========================================================================*/
static void test_type_ordering(void)
{
    CHECK(FLAIR_DITL_USER_ITEM   < FLAIR_DITL_HELP_ITEM,
          "userItem(0) < helpItem(1) [dialog-manager.md Sec 2]");
    CHECK(FLAIR_DITL_HELP_ITEM   < FLAIR_DITL_CTRL_ITEM,
          "helpItem(1) < ctrlItem(4) [dialog-manager.md Sec 2]");
    CHECK(FLAIR_DITL_CTRL_ITEM   < FLAIR_DITL_STAT_TEXT,
          "ctrlItem(4) < statText(8) [dialog-manager.md Sec 2]");
    CHECK(FLAIR_DITL_STAT_TEXT   < FLAIR_DITL_EDIT_TEXT,
          "statText(8) < editText(16) [dialog-manager.md Sec 2]");
    CHECK(FLAIR_DITL_EDIT_TEXT   < FLAIR_DITL_ICON_ITEM,
          "editText(16) < iconItem(32) [dialog-manager.md Sec 2]");
    CHECK(FLAIR_DITL_ICON_ITEM   < FLAIR_DITL_PIC_ITEM,
          "iconItem(32) < picItem(64) [dialog-manager.md Sec 2]");
    CHECK(FLAIR_DITL_PIC_ITEM    < FLAIR_DITL_ITEM_DISABLE,
          "picItem(64) < itemDisable(128) -- disable bit is strictly above all types "
          "[dialog-manager.md Sec 2]");
}

/* ===========================================================================
 * main -- run all properties; print check count; exit non-zero on failure.
 * ===========================================================================*/
int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    printf("test-dialog-record: spec/dialog_record.h LOCKED spec oracle "
           "(era=system7.0-7.1; beads initech-dh5k.3)\n");
    printf("Ref: ../system7-decomp/specs/toolbox/dialog-manager.md "
           "[verified: A+B]\n");

    test_ditl_base_types();
    test_item_disable();
    test_ctrl_subtypes();
    test_alert_icons();
    test_alert_stages();
    test_result_codes();
    test_window_record_consistency();
    test_one_based_numbering();
    test_type_ordering();

    return TEST_SUMMARY("test-dialog-record");
}
