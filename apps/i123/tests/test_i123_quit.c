/*
 * harness/proptest/test_i123_quit.c -- the host oracle of Initech 123's EXIT
 * GUARD (factory C; bead initech-fvnj, audit L008 "Quit discards an edited
 * worksheet without an unsaved-work warning").
 *
 * WHAT IS UNDER TEST. The real 123.EXE application source, os/i123/app/
 * face_a.c, compiled on the host over the real worksheet core, with the
 * Toolbox Gate (os/apps/tbx/tbx.h) replaced by a RECORDING MOCK: the cell
 * text the app draws lands in a 76 x 24 character screen, EXIT / ALERT calls
 * are recorded, and the file verbs are served from a host directory with
 * injectable failures. Each LAUNCH is a fork()ed child, so every launch starts
 * from the pristine image a real launch loads (zeroed BSS, initial .data) and
 * a relaunch reads what the previous launch saved to the same directory.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2 -- never from face_a.c):
 *   - the bead's DONE MEANS: committed edits and a pending entry are unsaved
 *     work; every exit route (File > Quit / Ctrl-Q, the close box, /Quit Yes)
 *     passes one guard; clean quits at once; Cancel keeps the sheet AND the
 *     pending entry; Save runs the /File Save path and quits only on success;
 *     a failed or cancelled Save keeps the sheet dirty and open; Don't Save
 *     quits; only a successful Save or Retrieve makes the sheet clean;
 *   - the Lotus 1-2-3 R2.2 Reference Manual p. 9-1 (PDF page 217 of the local
 *     scan): /Quit offers No / Yes; with unsaved changes a second No / Yes
 *     follows; ../lotus123-decomp/goldens/lotus-123-2.2-installed-ref/
 *     extracted/installed/123.RI: 0x300 "No", 0x303 "Do not end 1-2-3
 *     session; return to READY mode", 0x332 "Yes", 0x336 "End 1-2-3 session
 *     (Remember to save your worksheet first)", 0x370 "WORKSHEET CHANGES NOT
 *     SAVED!  End 1-2-3 anyway?" -- restated here by hand;
 *   - the alert ABI: spec/toolbox_gate.h Sec 11 (item 1 default, 2 cancel,
 *     3 other; CR-separated lines);
 *   - the save prompt: 123.RI 0x2f92 "Enter name of file to save: " (as the
 *     tdnl.91 file gates already grade it).
 * The saved bytes are judged INDEPENDENTLY by the Makefile with
 * ../lotus123-decomp/tools/wk1_ref.py (never by this codec).
 *
 * usage: test_i123_quit <empty-scratch-dir>
 * Mutants (Rule 6; -D on the app build, test-i123-quit-mutant):
 *   I123_MUT_NEVER_DIRTY     a commit never marks the sheet dirty  -> Q2
 *   I123_MUT_CANCEL_QUITS    Cancel in the alert quits anyway       -> Q3
 *   I123_MUT_PENDING_IGNORED a pending entry is not unsaved work    -> Q5
 *   I123_MUT_SAVE_FAIL_QUITS a failed Save from the alert quits     -> Q8
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "cpanels_record.h"

#include "../app/face_a.c"   /* the application under test */

#include "fake_tbx/recording.h"

/* ---- the traces --------------------------------------------------------- */
/* Q1 CLEAN: a never-edited sheet quits at once by every route, never asks. */
static void t_clean_menu(void)
{
    menu_quit();
    CHECK(m_exit_calls == 1 && m_exit_rc == 0, "Q1", "clean File > Quit must EXIT(0) at once");
    CHECK(m_alerts == 0, "Q1", "clean File > Quit must not ask");
}
static void t_clean_close(void)
{
    close_box();
    CHECK(m_exit_calls == 1 && m_exit_rc == 0, "Q1", "clean close box must EXIT(0) at once");
    CHECK(m_alerts == 0, "Q1", "clean close box must not ask");
}
static void t_clean_slash(void)
{
    tks("/q");
    CHECK(m_exit_calls == 0, "Q1", "/Quit alone must not end the session (R2.2 p. 9-1: No / Yes first)");
    CHECK(line_has(1, "No  Yes"), "Q1", "/Quit must show the No / Yes menu (123.RI 0x300/0x332)");
    CHECK(line_has(2, "Do not end 1-2-3 session; return to READY mode"), "Q1", "No's description (123.RI 0x303)");
    tk('n');
    CHECK(m_exit_calls == 0 && line_has(0, "READY"), "Q1", "/Quit No returns to READY");
    tks("/qy");
    CHECK(m_exit_calls == 1 && m_alerts == 0, "Q1", "clean /Quit Yes must EXIT at once, never ask");
}

/* Q2 DIRTY ASKS: committed entries make every route ask, with the words. */
static void t_dirty_menu_asks(void)
{
    type_row();
    menu_quit();
    CHECK(m_exit_calls == 0, "Q2", "dirty File > Quit exited without asking -- the L008 data loss");
    CHECK(asked_properly(), "Q2", "dirty File > Quit must ask: WORKSHEET CHANGES NOT SAVED! / Save, Cancel, Don't Save");
}
static void t_dirty_close_asks(void)
{
    tks("Hello\r");
    close_box();
    CHECK(m_exit_calls == 0, "Q2", "dirty close box exited without asking");
    CHECK(asked_properly(), "Q2", "dirty close box must ask the same alert");
}
static void t_dirty_slash_asks(void)
{
    tks("Hello\r");
    tks("/qy");
    CHECK(m_exit_calls == 0, "Q2", "dirty /Quit Yes exited without asking");
    CHECK(asked_properly(), "Q2", "dirty /Quit Yes must ask the same alert");
    answer(2);
    CHECK(m_exit_calls == 0 && line_has(0, "READY"), "Q3", "/Quit Yes then Cancel must leave READY (R2.2 p. 9-1 second No)");
}

/* Q3 CANCEL keeps the sheet, stays dirty (asks again). */
static void t_dirty_cancel(void)
{
    char c[64];
    type_row();
    menu_quit();
    answer(2);
    CHECK(m_exit_calls == 0, "Q3", "Cancel must not quit");
    cell(0, 0, c, sizeof c);
    CHECK(strcmp(c, "'Hello") == 0, "Q3", "Cancel must keep A1 'Hello in the model");
    cell(3, 0, c, sizeof c);
    CHECK(strcmp(c, "-7") == 0, "Q3", "Cancel must keep D1 -7 in the model");
    CHECK(line_has(4, "Hello") && line_has(4, "-7"), "Q3", "Cancel must leave row 1 on screen");
    menu_quit();
    CHECK(m_exit_calls == 0 && m_alerts == 2, "Q3", "after Cancel the sheet is still dirty: Quit asks again");
}

/* Q4 DON'T SAVE quits, writes nothing (R2.2 p. 9-1 second Yes). */
static void t_dirty_dont_save(void)
{
    type_row();
    menu_quit();
    answer(3);
    CHECK(m_exit_calls == 1 && m_exit_rc == 0, "Q4", "Don't Save must EXIT(0)");
}

/* Q5 A PENDING ENTRY is unsaved work; Cancel keeps the typed input. */
static void t_pending_cancel(void)
{
    char c[64];
    tks("Hel");
    menu_quit();
    CHECK(m_exit_calls == 0, "Q5", "a pending (uncommitted) entry was discarded by Quit without asking");
    CHECK(asked_properly(), "Q5", "a pending entry must raise the alert");
    answer(2);
    CHECK(m_exit_calls == 0, "Q5", "Cancel must not quit");
    CHECK(line_has(1, "Hel_"), "Q5", "Cancel must keep the pending input on the edit line");
    CHECK(line_has(0, "LABEL"), "Q5", "Cancel must keep LABEL mode");
    tks("p\r");
    cell(0, 0, c, sizeof c);
    CHECK(strcmp(c, "'Help") == 0, "Q5", "the kept entry continues and commits: A1 'Help");
}
static void t_pending_close(void)
{
    tks("12");
    close_box();
    CHECK(m_exit_calls == 0 && m_alerts == 1, "Q5", "a pending VALUE must make the close box ask");
    answer(2);
    CHECK(line_has(1, "12_") && line_has(0, "VALUE"), "Q5", "Cancel keeps the pending VALUE");
}

/* Q6 SAVE from the alert runs /File Save; success quits; relaunch has it. */
static void t_save_ok(void)
{
    type_row();
    menu_quit();
    answer(1);
    CHECK(m_exit_calls == 0, "Q6", "Save must not quit before the save happened");
    CHECK(line_has(1, "Enter name of file to save: "), "Q6", "Save must open the /File Save prompt (123.RI 0x2f92)");
    tks("q\r");
    CHECK(file_exists("Q.WK1"), "Q6", "Save must write Q.WK1");
    CHECK(m_exit_calls == 1 && m_exit_rc == 0, "Q6", "a successful Save from the alert must then EXIT(0)");
}
static void t_relaunch_q(void)
{
    char c[64];
    tks("/frq\r");
    cell(0, 0, c, sizeof c);
    CHECK(strcmp(c, "'Hello") == 0, "Q6", "relaunch + Retrieve Q: A1 'Hello");
    cell(2, 0, c, sizeof c);
    CHECK(strcmp(c, "1.5") == 0, "Q6", "relaunch + Retrieve Q: C1 1.5");
    menu_quit();
    CHECK(m_exit_calls == 1 && m_alerts == 0, "Q9", "a just-retrieved sheet is clean: Quit must not ask");
}

/* Q7 PENDING + SAVE: the pending entry is committed and saved. */
static void t_pending_save(void)
{
    tks("Pend");
    menu_quit();
    answer(1);
    tks("pe\r");
    CHECK(file_exists("PE.WK1") && m_exit_calls == 1, "Q7", "Save with a pending entry must commit it, save and quit");
}
static void t_relaunch_pe(void)
{
    char c[64];
    tks("/frpe\r");
    cell(0, 0, c, sizeof c);
    CHECK(strcmp(c, "'Pend") == 0, "Q7", "the pending entry reached the saved file: A1 'Pend");
}

/* Q8 A FAILED or CANCELLED SAVE keeps the sheet open and dirty. */
static void t_save_create_fails(void)
{
    char c[64];
    type_row();
    menu_quit();
    answer(1);
    m_fail_create = 1;
    tks("z\r");
    CHECK(m_exit_calls == 0, "Q8", "a failed Save (CREATE refused) must not quit");
    CHECK(line_has(1, "File I/O error"), "Q8", "a failed Save must say so");
    cell(0, 0, c, sizeof c);
    CHECK(strcmp(c, "'Hello") == 0, "Q8", "a failed Save keeps the sheet");
    m_fail_create = 0;
    tk(27);
    menu_quit();
    CHECK(m_exit_calls == 0 && m_alerts == 2, "Q8", "after a failed Save the sheet is still dirty: Quit asks again");
}
static void t_save_write_fails(void)
{
    type_row();
    menu_quit();
    answer(1);
    m_fail_write = 1;
    tks("w\r");
    CHECK(m_exit_calls == 0, "Q8", "a failed Save (short WRITE) must not quit");
    CHECK(!file_exists("W.WK1"), "Q8", "a failed new Save leaves no partial file");
    m_fail_write = 0;
    tk(27);
    menu_quit();
    CHECK(m_exit_calls == 0 && m_alerts == 2, "Q8", "after a failed write the sheet is still dirty");
}
static void t_save_escaped(void)
{
    type_row();
    menu_quit();
    answer(1);
    tk(27);
    CHECK(m_exit_calls == 0, "Q8", "Esc at the save prompt cancels the quit");
    CHECK(line_has(0, "READY"), "Q8", "Esc at the save prompt returns to READY");
    menu_quit();
    CHECK(m_exit_calls == 0 && m_alerts == 2, "Q8", "a cancelled Save keeps the sheet dirty");
}
static void t_replace_cancel(void)
{
    tks("/frx\r");            /* X.WK1 from t_save_then_quit */
    tks("Changed\r");
    menu_quit();
    answer(1);
    CHECK(line_has(1, "x.wk1"), "Q8", "the save prompt offers the current file name");
    tk('\r');
    CHECK(line_has(1, "Cancel  Replace  Backup"), "Q8", "an existing file asks Cancel / Replace / Backup (R2.2 p. 5-22)");
    tk('c');
    CHECK(m_exit_calls == 0, "Q8", "Replace > Cancel must not quit");
    menu_quit();
    CHECK(m_exit_calls == 0 && m_alerts == 2, "Q8", "Replace > Cancel keeps the sheet dirty");
}

/* Q9 ONLY Save / Retrieve make the sheet clean; editing again dirties it. */
static void t_save_then_quit(void)
{
    tks("Hello\r");
    tks("/fsx\r");
    CHECK(file_exists("X.WK1"), "Q9", "/File Save x");
    menu_quit();
    CHECK(m_exit_calls == 1 && m_alerts == 0, "Q9", "a just-saved sheet is clean: Quit must not ask");
}
static void t_save_edit_quit(void)
{
    tks("Hello\r");
    tks("/fsy\r");
    tks("More\r");
    menu_quit();
    CHECK(m_exit_calls == 0 && m_alerts == 1, "Q9", "an edit after Save makes the sheet dirty again");
}
static void t_replace_ok(void)
{
    tks("/frx\r");
    tks("Again\r");
    menu_quit();
    answer(1);
    tk('\r');
    tk('r');
    CHECK(m_exit_calls == 1 && m_exit_rc == 0, "Q6", "Save + Replace from the alert must then EXIT(0)");
}
static void t_relaunch_again(void)
{
    char c[64];
    tks("/frx\r");
    cell(0, 0, c, sizeof c);
    CHECK(strcmp(c, "'Again") == 0, "Q6", "the Replace from the alert reached the disk: A1 'Again");
}

/* Q10 FAIL SAFE: an alert the Toolbox refuses never becomes a silent quit. */
static void t_alert_refused(void)
{
    tks("Hel");
    m_alert_rc = TBX_ERR_BUSY;
    menu_quit();
    CHECK(m_exit_calls == 0, "Q10", "a refused alert must not quit a dirty sheet");
    CHECK(line_has(1, "Hel_"), "Q10", "a refused alert keeps the pending entry");
}

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s <scratch-dir>\n", argv[0]); return 2; }
    m_dir = argv[1];
    g_ctr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (g_ctr == MAP_FAILED) { perror("mmap"); return 2; }
    g_ctr[0] = g_ctr[1] = 0;

    launch("clean_menu", t_clean_menu);
    launch("clean_close", t_clean_close);
    launch("clean_slash", t_clean_slash);
    launch("dirty_menu_asks", t_dirty_menu_asks);
    launch("dirty_close_asks", t_dirty_close_asks);
    launch("dirty_slash_asks", t_dirty_slash_asks);
    launch("dirty_cancel", t_dirty_cancel);
    launch("dirty_dont_save", t_dirty_dont_save);
    launch("pending_cancel", t_pending_cancel);
    launch("pending_close", t_pending_close);
    launch("save_ok", t_save_ok);
    launch("relaunch_q", t_relaunch_q);
    launch("pending_save", t_pending_save);
    launch("relaunch_pe", t_relaunch_pe);
    launch("save_create_fails", t_save_create_fails);
    launch("save_write_fails", t_save_write_fails);
    launch("save_escaped", t_save_escaped);
    launch("save_then_quit", t_save_then_quit);
    launch("save_edit_quit", t_save_edit_quit);
    launch("replace_cancel", t_replace_cancel);
    launch("alert_refused", t_alert_refused);
    /* Last: overwrite X.WK1 through the alert's Save + Replace, then reread. */
    launch("replace_ok", t_replace_ok);
    launch("relaunch_again", t_relaunch_again);

    printf("test_i123_quit: %d checks, %d failures\n", g_ctr[0], g_ctr[1]);
    return g_ctr[1] == 0 ? 0 : 1;
}
