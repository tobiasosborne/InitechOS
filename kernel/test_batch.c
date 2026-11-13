/* test_batch.c -- host unit oracle for the .BAT parser/expander (batch.c).
 *
 * beads: initech-xw1 (batch interpreter -- LOGIC half; REPL/AUTOEXEC
 *        integration is a separate later bead).
 * Ref:   MS-DOS 3.3 Technical Reference, Chapter 3 "Batch Files";
 *        Ralf Brown's Interrupt List (parameter expansion, label matching).
 *        CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6
 *        (mutation-proven), Rule 12 (ASCII).
 *
 * Compiles HOSTED by #including batch.c directly (the same TU trick as
 * test_env.c / test_mz.c -- test_batch.c is the ONLY source file passed to
 * gcc; batch.c is NOT compiled separately).  All batch_* functions are pure
 * + I/O-free; no kernel defines required.
 *
 * MUTATION (Rule 6) -- driven by make test-batch-mutant:
 *   -DBATCH_MUTATE_NO_PCT_PCT    : %% does NOT collapse to %; the %% test
 *                                   goes RED (the oracle bites).
 *   -DBATCH_MUTATE_ECHO_OFF_MISS : ECHO OFF is misclassified as BL_ECHO_TEXT;
 *                                   the ECHO OFF classify test goes RED.
 *   -DBATCH_MUTATE_GOTO_NOLABEL  : bat_goto_target always copies 0 chars;
 *                                   the GOTO target test goes RED.
 *
 * When compiled with any mutant flag this binary MUST exit non-zero.
 * The clean build MUST exit 0 and print "<n> checks, 0 failures".
 */

#include <stdint.h>
#include <string.h>   /* strcmp, strncmp -- libc OK in host test (Law 3) */
#include <stdio.h>

#include "batch.h"
#include "test_assert.h"

/* Pull in the real artifact source (same TU trick as test_env.c). No KERNEL
 * define needed; batch.c is pure and I/O-free.  The mutation flags propagate
 * automatically from the gcc command line when the mutant build is requested. */
#include "batch.c"

TEST_HARNESS();

/* ---- stub environment callback ----------------------------------------- */

/* A minimal environment for testing %VAR% expansion.  Two variables are
 * defined ("HOME" -> "A:\\" and "COMSPEC" -> "A:\\COMMAND.COM"); all other
 * lookups return NULL (undefined). */
static const char *stub_env(const char *name, void *ctx)
{
    (void)ctx;  /* unused */
    if (strcmp(name, "HOME") == 0) {
        return "A:\\";
    }
    if (strcmp(name, "COMSPEC") == 0) {
        return "A:\\COMMAND.COM";
    }
    return 0;   /* undefined */
}

/* ---- test_expand -------------------------------------------------------- */

static void test_expand(void)
{
    char out[256];
    int  n;

    /* --- %0..%9 positional parameters ----------------------------------- */

    const char *argv3[] = { "MYSCRIPT.BAT", "first", "second" };

    /* %0 -> argv[0] (the batch script name). */
    n = batch_expand("%0", argv3, 3, 0, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand %0: returns > 0");
    CHECK(strcmp(out, "MYSCRIPT.BAT") == 0, "expand %0: == MYSCRIPT.BAT");

    /* %1 -> argv[1]. */
    n = batch_expand("%1", argv3, 3, 0, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand %1: returns > 0");
    CHECK(strcmp(out, "first") == 0, "expand %1: == first");

    /* %2 -> argv[2]. */
    n = batch_expand("%2", argv3, 3, 0, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand %2: returns > 0");
    CHECK(strcmp(out, "second") == 0, "expand %2: == second");

    /* %3 with argc==3 -> out-of-range -> empty string (DOS 3.3 behavior). */
    n = batch_expand("%3", argv3, 3, 0, 0, out, (int)sizeof(out));
    CHECK(n == 0, "expand %3 out-of-range: returns 0");
    CHECK(strcmp(out, "") == 0, "expand %3 out-of-range: empty string");

    /* %9 with argc==3 -> also empty. */
    n = batch_expand("%9", argv3, 3, 0, 0, out, (int)sizeof(out));
    CHECK(n == 0, "expand %9 out-of-range: returns 0");
    CHECK(strcmp(out, "") == 0, "expand %9 out-of-range: empty string");

    /* Mixed line with %0 and %1 and literal text. */
    n = batch_expand("ECHO %0 %1", argv3, 3, 0, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand mixed: returns > 0");
    CHECK(strcmp(out, "ECHO MYSCRIPT.BAT first") == 0,
          "expand mixed: correct substitution");

    /* NULL argv entry at %1 -> empty (defensive: argv[1] == NULL). */
    const char *argv_null[] = { "SCRIPT.BAT", 0 };
    n = batch_expand("%1", argv_null, 2, 0, 0, out, (int)sizeof(out));
    CHECK(n == 0, "expand NULL argv[1]: returns 0 (empty)");
    CHECK(strcmp(out, "") == 0, "expand NULL argv[1]: empty string");

    /* --- %VAR% environment substitution --------------------------------- */

    /* Defined variable: HOME. */
    n = batch_expand("%HOME%", 0, 0, stub_env, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand %HOME%: returns > 0");
    CHECK(strcmp(out, "A:\\") == 0, "expand %HOME%: == A:\\");

    /* Defined variable: COMSPEC. */
    n = batch_expand("%COMSPEC%", 0, 0, stub_env, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand %COMSPEC%: returns > 0");
    CHECK(strcmp(out, "A:\\COMMAND.COM") == 0,
          "expand %COMSPEC%: == A:\\COMMAND.COM");

    /* Undefined variable: %MISSING% -> literal "%MISSING%" (DOS 3.3). */
    n = batch_expand("%MISSING%", 0, 0, stub_env, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand %MISSING%: returns > 0");
    CHECK(strcmp(out, "%MISSING%") == 0,
          "expand %MISSING%: unexpanded literal (DOS 3.3 behavior)");

    /* Variable expansion with surrounding text. */
    n = batch_expand("CD %HOME%", 0, 0, stub_env, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand 'CD %HOME%': returns > 0");
    CHECK(strcmp(out, "CD A:\\") == 0, "expand 'CD %HOME%': correct");

    /* NULL env callback + variable: leaves literal (same as undefined). */
    n = batch_expand("%HOME%", 0, 0, 0, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand %HOME% with NULL env: returns > 0");
    CHECK(strcmp(out, "%HOME%") == 0, "expand %HOME% with NULL env: literal");

    /* --- %% -> literal '%' (DOS percent escape) ------------------------- */

    /* Under BATCH_MUTATE_NO_PCT_PCT this outputs "%%" (two chars) and the
     * strcmp check below goes RED -- the oracle bites the mutant. */
    n = batch_expand("100%%", 0, 0, 0, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand 100%%: returns > 0");
    CHECK(strcmp(out, "100%") == 0,
          "expand 100%%: collapses to 100% (RED under BATCH_MUTATE_NO_PCT_PCT)");

    /* Multiple %% in one line. */
    n = batch_expand("%%PATH%%", 0, 0, 0, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand %%PATH%%: returns > 0");
    CHECK(strcmp(out, "%PATH%") == 0, "expand %%PATH%%: collapses both %%");

    /* Lone '%' at end of string -> passed through verbatim. */
    n = batch_expand("hello%", 0, 0, 0, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand lone trailing %%: returns > 0");
    CHECK(strcmp(out, "hello%") == 0, "expand lone trailing %: verbatim");

    /* --- Overflow guard (Rule 2) ---------------------------------------- */

    /* A cap of 1 means only room for the NUL terminator -- expansion fails. */
    char tiny[1];
    n = batch_expand("hello", 0, 0, 0, 0, tiny, 1);
    CHECK(n == -1, "expand overflow cap=1: returns -1");
    CHECK(tiny[0] == '\0', "expand overflow cap=1: out[0] is NUL (safe)");

    /* A cap just big enough for "hi\0" (3 bytes) -- boundary passes. */
    char exact[3];
    n = batch_expand("hi", 0, 0, 0, 0, exact, 3);
    CHECK(n == 2, "expand exact cap: returns 2");
    CHECK(strcmp(exact, "hi") == 0, "expand exact cap: correct");

    /* One byte too small -- fails. */
    char small[2];
    n = batch_expand("hi", 0, 0, 0, 0, small, 2);
    CHECK(n == -1, "expand one-byte-too-small: returns -1");

    /* --- NULL / empty inputs -------------------------------------------- */

    n = batch_expand("", 0, 0, 0, 0, out, (int)sizeof(out));
    CHECK(n == 0, "expand empty line: returns 0");
    CHECK(strcmp(out, "") == 0, "expand empty line: empty output");

    n = batch_expand(0, 0, 0, 0, 0, out, (int)sizeof(out));
    CHECK(n == 0, "expand NULL line: returns 0");
    CHECK(strcmp(out, "") == 0, "expand NULL line: empty output");
}

/* ---- test_classify ------------------------------------------------------- */

static void test_classify(void)
{
    batch_parsed_t p;
    batch_line_kind_t k;

    /* ---- Blank / whitespace ------------------------------------------- */

    k = batch_classify("", &p);
    CHECK(k == BL_BLANK, "classify empty: BL_BLANK");
    CHECK(p.at_suppressed == 0, "classify empty: not at-suppressed");

    k = batch_classify("   \t  ", &p);
    CHECK(k == BL_BLANK, "classify whitespace: BL_BLANK");

    /* ---- REM ----------------------------------------------------------  */

    k = batch_classify("REM this is a comment", &p);
    CHECK(k == BL_REM, "classify REM: BL_REM");
    CHECK(p.at_suppressed == 0, "classify REM: not at-suppressed");

    k = batch_classify("rem lowercase comment", &p);
    CHECK(k == BL_REM, "classify rem (lowercase): BL_REM");

    k = batch_classify("Rem Mixed", &p);
    CHECK(k == BL_REM, "classify Rem (mixed case): BL_REM");

    /* ---- ECHO ON ------------------------------------------------------- */

    k = batch_classify("ECHO ON", &p);
    CHECK(k == BL_ECHO_ON, "classify ECHO ON: BL_ECHO_ON");

    k = batch_classify("echo on", &p);
    CHECK(k == BL_ECHO_ON, "classify echo on (lowercase): BL_ECHO_ON");

    /* ---- ECHO OFF (RED under BATCH_MUTATE_ECHO_OFF_MISS) --------------- */

    k = batch_classify("ECHO OFF", &p);
    /* Under BATCH_MUTATE_ECHO_OFF_MISS: k == BL_ECHO_TEXT, so this CHECK
     * goes RED -- the oracle bites the mutant (Rule 6). */
    CHECK(k == BL_ECHO_OFF,
          "classify ECHO OFF: BL_ECHO_OFF (RED under BATCH_MUTATE_ECHO_OFF_MISS)");

    k = batch_classify("echo off", &p);
    CHECK(k == BL_ECHO_OFF, "classify echo off (lowercase): BL_ECHO_OFF "
          "(RED under BATCH_MUTATE_ECHO_OFF_MISS)");

    /* ---- ECHO <text> --------------------------------------------------- */

    k = batch_classify("ECHO hello world", &p);
    CHECK(k == BL_ECHO_TEXT, "classify ECHO <text>: BL_ECHO_TEXT");
    CHECK(strcmp(p.echo_text, "hello world") == 0,
          "classify ECHO <text>: echo_text == 'hello world'");

    k = batch_classify("ECHO", &p);
    CHECK(k == BL_ECHO_TEXT, "classify bare ECHO: BL_ECHO_TEXT");
    CHECK(strcmp(p.echo_text, "") == 0, "classify bare ECHO: echo_text empty");

    /* ---- @ suppression ------------------------------------------------- */

    k = batch_classify("@ECHO OFF", &p);
    /* ECHO OFF classification still applies even after stripping '@'. */
    CHECK(k == BL_ECHO_OFF,
          "classify @ECHO OFF: BL_ECHO_OFF (RED under BATCH_MUTATE_ECHO_OFF_MISS)");
    CHECK(p.at_suppressed == 1, "classify @ECHO OFF: at_suppressed == 1");

    k = batch_classify("@REM silent comment", &p);
    CHECK(k == BL_REM, "classify @REM: BL_REM");
    CHECK(p.at_suppressed == 1, "classify @REM: at_suppressed == 1");

    k = batch_classify("@DIR", &p);
    CHECK(k == BL_COMMAND, "classify @DIR: BL_COMMAND");
    CHECK(p.at_suppressed == 1, "classify @DIR: at_suppressed == 1");

    /* ---- :label definition --------------------------------------------- */

    k = batch_classify(":START", &p);
    CHECK(k == BL_LABEL, "classify :START: BL_LABEL");
    CHECK(strcmp(p.label_name, "START") == 0,
          "classify :START: label_name == START");

    k = batch_classify(":begin", &p);
    CHECK(k == BL_LABEL, "classify :begin: BL_LABEL");
    CHECK(strcmp(p.label_name, "begin") == 0,
          "classify :begin: label_name == begin");

    /* ---- GOTO :label (target extracted) -------------------------------- */

    k = batch_classify("GOTO :LOOP", &p);
    CHECK(k == BL_GOTO, "classify GOTO :LOOP: BL_GOTO");
    /* Under BATCH_MUTATE_GOTO_NOLABEL: goto_target is "" so this fails.
     * The oracle bites the mutant (Rule 6). */
    CHECK(strcmp(p.goto_target, "LOOP") == 0,
          "classify GOTO :LOOP: goto_target == LOOP "
          "(RED under BATCH_MUTATE_GOTO_NOLABEL)");

    k = batch_classify("GOTO END", &p);
    CHECK(k == BL_GOTO, "classify GOTO END: BL_GOTO");
    CHECK(strcmp(p.goto_target, "END") == 0,
          "classify GOTO END: goto_target == END "
          "(RED under BATCH_MUTATE_GOTO_NOLABEL)");

    k = batch_classify("goto :start", &p);
    CHECK(k == BL_GOTO, "classify goto :start (lowercase): BL_GOTO");
    CHECK(strcmp(p.goto_target, "start") == 0,
          "classify goto :start: goto_target == start "
          "(RED under BATCH_MUTATE_GOTO_NOLABEL)");

    /* ---- IF ------------------------------------------------------------- */

    k = batch_classify("IF EXIST A:\\CONFIG.SYS ECHO found", &p);
    CHECK(k == BL_IF, "classify IF EXIST ...: BL_IF");

    k = batch_classify("IF NOT ERRORLEVEL 1 GOTO :DONE", &p);
    CHECK(k == BL_IF, "classify IF NOT ERRORLEVEL: BL_IF");

    k = batch_classify("if \"%1\"==\"\" GOTO :USAGE", &p);
    CHECK(k == BL_IF, "classify if (lowercase): BL_IF");

    /* ---- FOR ----------------------------------------------------------- */

    k = batch_classify("FOR %%F IN (*.TXT) DO TYPE %%F", &p);
    CHECK(k == BL_FOR, "classify FOR: BL_FOR");

    k = batch_classify("for %%i in (a b c) DO echo %%i", &p);
    CHECK(k == BL_FOR, "classify for (lowercase): BL_FOR");

    /* ---- SHIFT --------------------------------------------------------- */

    k = batch_classify("SHIFT", &p);
    CHECK(k == BL_SHIFT, "classify SHIFT: BL_SHIFT");

    k = batch_classify("shift", &p);
    CHECK(k == BL_SHIFT, "classify shift (lowercase): BL_SHIFT");

    /* ---- CALL ---------------------------------------------------------- */

    k = batch_classify("CALL SETUP.BAT", &p);
    CHECK(k == BL_CALL, "classify CALL: BL_CALL");

    k = batch_classify("call install.bat arg1", &p);
    CHECK(k == BL_CALL, "classify call (lowercase): BL_CALL");

    /* ---- PAUSE --------------------------------------------------------- */

    k = batch_classify("PAUSE", &p);
    CHECK(k == BL_PAUSE, "classify PAUSE: BL_PAUSE");

    k = batch_classify("pause", &p);
    CHECK(k == BL_PAUSE, "classify pause (lowercase): BL_PAUSE");

    /* ---- Plain command (BL_COMMAND) ------------------------------------ */

    k = batch_classify("DIR A:\\", &p);
    CHECK(k == BL_COMMAND, "classify DIR: BL_COMMAND");

    k = batch_classify("COPY A:\\SRC.TXT B:\\DST.TXT", &p);
    CHECK(k == BL_COMMAND, "classify COPY: BL_COMMAND");

    k = batch_classify("TYPE README.TXT", &p);
    CHECK(k == BL_COMMAND, "classify TYPE: BL_COMMAND");

    k = batch_classify("MYAPP.COM ARG1 ARG2", &p);
    CHECK(k == BL_COMMAND, "classify external app: BL_COMMAND");

    /* A word that merely starts with a directive keyword (e.g. "REMARK") must
     * NOT match the keyword -- it is a BL_COMMAND. */
    k = batch_classify("REMARK not a REM", &p);
    CHECK(k == BL_COMMAND, "classify REMARK (not REM): BL_COMMAND");

    k = batch_classify("ECHO123 something", &p);
    CHECK(k == BL_COMMAND, "classify ECHO123 (not ECHO): BL_COMMAND");
}

/* ---- test_label_matches -------------------------------------------------- */

static void test_label_matches(void)
{
    /* Basic match (case-insensitive). */
    CHECK(batch_label_matches(":START", "START") == 1,
          "label_matches :START / START");
    CHECK(batch_label_matches(":start", "START") == 1,
          "label_matches :start / START (case-insensitive)");
    CHECK(batch_label_matches(":START", "start") == 1,
          "label_matches :START / start (case-insensitive)");
    CHECK(batch_label_matches(":Loop", "LOOP") == 1,
          "label_matches :Loop / LOOP (mixed case)");

    /* Non-label lines must not match. */
    CHECK(batch_label_matches("ECHO hello", "hello") == 0,
          "label_matches ECHO line: 0");
    CHECK(batch_label_matches("", "START") == 0,
          "label_matches empty line: 0");

    /* Wrong target. */
    CHECK(batch_label_matches(":START", "END") == 0,
          "label_matches :START / END: 0 (wrong target)");

    /* Whitespace before ':' is acceptable (batch files may indent labels). */
    CHECK(batch_label_matches("  :BEGIN", "BEGIN") == 1,
          "label_matches '  :BEGIN' with leading ws: 1");

    /* NULL guards. */
    CHECK(batch_label_matches(0, "START") == 0, "label_matches NULL line: 0");
    CHECK(batch_label_matches(":START", 0) == 0, "label_matches NULL target: 0");
}

/* ---- test_expand_combined ------------------------------------------------ */

/* Exercise batch_expand with both %n and %VAR% in the same line, plus %%. */
static void test_expand_combined(void)
{
    char out[256];
    int  n;

    const char *argv2[] = { "SETUP.BAT", "install" };

    /* Mixed %0, %VAR%, and %%. */
    n = batch_expand("@%0 %1 %HOME% 100%%", argv2, 2, stub_env, 0,
                     out, (int)sizeof(out));
    CHECK(n > 0, "expand combined: returns > 0");
    CHECK(strcmp(out, "@SETUP.BAT install A:\\ 100%") == 0,
          "expand combined: correct (RED for %% under BATCH_MUTATE_NO_PCT_PCT)");

    /* %2 is out of range with argc==2 -> empty. */
    n = batch_expand("arg2=[%2]", argv2, 2, stub_env, 0, out, (int)sizeof(out));
    CHECK(n > 0, "expand out-of-range in brackets: returns > 0");
    CHECK(strcmp(out, "arg2=[]") == 0,
          "expand out-of-range in brackets: empty substitution");
}

/* ---- main ---------------------------------------------------------------- */

int main(void)
{
    test_expand();
    test_classify();
    test_label_matches();
    test_expand_combined();
    return TEST_SUMMARY("test_batch");
}
