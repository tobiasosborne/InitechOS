/* test_batch_exec.c -- host unit oracle for the .BAT EXECUTION-logic helpers
 *                      in batch.c (IF condition eval, FOR set parse/tokenize/
 *                      substitution).  The I/O-bound driver (run_batch, file
 *                      read, dispatch) lives in command.c behind
 *                      COMMAND_KERNEL_REPL and is exercised by the emulator
 *                      gate; THESE pure decision helpers are host-tested here.
 *
 * beads: initech-xw1 (batch interpreter -- REPL/AUTOEXEC integration; this is
 *        the host oracle for the pure execution-logic half added to batch.c).
 * Ref:   MS-DOS 3.3 Technical Reference, Chapter 3 "Batch Files" (IF [NOT]
 *        ERRORLEVEL n / EXIST file / str1==str2; FOR %%v IN (set) DO cmd);
 *        Ralf Brown's Interrupt List; Peter Norton "DOS Programmer's Reference".
 *        CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6
 *        (mutation-proven), Rule 12 (ASCII).
 *
 * Compiles HOSTED by #including batch.c directly (the same TU trick as
 * test_batch.c / test_env.c -- test_batch_exec.c is the ONLY source file passed
 * to gcc; batch.c is NOT compiled separately).  The batch_eval_if / batch_for_*
 * helpers are pure + I/O-free (the EXIST probe is a caller-supplied callback,
 * stubbed here); no kernel defines required.
 *
 * MUTATION (Rule 6) -- driven by make test-batch-exec-mutant:
 *   -DBATCH_MUTATE_IF_IGNORE_NOT : batch_eval_if ignores the NOT prefix; the
 *                                   IF-NOT checks go RED (the oracle bites).
 *   -DBATCH_MUTATE_FOR_NO_TOKENS : batch_for_next_token reports no tokens; the
 *                                   FOR-tokenize checks go RED.
 *
 * When compiled with either mutant flag this binary MUST exit non-zero.  The
 * clean build MUST exit 0 and print "<n> checks, 0 failures".
 */

#include <stdint.h>
#include <string.h>   /* strcmp -- libc OK in host test (Law 3) */
#include <stdio.h>

#include "batch.h"
#include "test_assert.h"

/* Pull in the real artifact source (same TU trick as test_batch.c).  The
 * mutation flags propagate from the gcc command line when a mutant is built. */
#include "batch.c"

TEST_HARNESS();

/* ---- stub EXIST prober --------------------------------------------------- */

/* A minimal file-existence table for IF EXIST testing.  "A:\CONFIG.SYS" and
 * "AUTOEXEC.BAT" exist; everything else does not.  Case-sensitive match is
 * fine -- the REPL upcases names before probing, and so do our test inputs. */
static int stub_exist(const char *spec, void *ctx)
{
    (void)ctx;
    if (strcmp(spec, "A:\\CONFIG.SYS") == 0) {
        return 1;
    }
    if (strcmp(spec, "AUTOEXEC.BAT") == 0) {
        return 1;
    }
    return 0;
}

/* ---- test_if_errorlevel -------------------------------------------------- */

static void test_if_errorlevel(void)
{
    const char *cmd;
    int r;

    /* ERRORLEVEL is a >= test (DOS semantics). errorlevel=1, IF ERRORLEVEL 1. */
    cmd = 0;
    r = batch_eval_if("ERRORLEVEL 1 GOTO :ERR", 1, stub_exist, 0, &cmd);
    CHECK(r == 1, "IF ERRORLEVEL 1 with rc=1: true");
    CHECK(cmd != 0 && strcmp(cmd, "GOTO :ERR") == 0,
          "IF ERRORLEVEL 1: out_cmd == 'GOTO :ERR'");

    /* errorlevel=0, IF ERRORLEVEL 1 -> 0 >= 1 is FALSE. */
    cmd = (const char *)1;
    r = batch_eval_if("ERRORLEVEL 1 GOTO :ERR", 0, stub_exist, 0, &cmd);
    CHECK(r == 0, "IF ERRORLEVEL 1 with rc=0: false (0 >= 1 is false)");
    CHECK(cmd == 0, "IF ERRORLEVEL 1 false: out_cmd == NULL");

    /* errorlevel=5, IF ERRORLEVEL 3 -> 5 >= 3 TRUE (the >= semantics). */
    r = batch_eval_if("ERRORLEVEL 3 ECHO high", 5, stub_exist, 0, &cmd);
    CHECK(r == 1, "IF ERRORLEVEL 3 with rc=5: true (>= test)");
    CHECK(cmd != 0 && strcmp(cmd, "ECHO high") == 0,
          "IF ERRORLEVEL 3: out_cmd == 'ECHO high'");

    /* errorlevel=0, IF ERRORLEVEL 0 -> 0 >= 0 TRUE (the boundary). */
    r = batch_eval_if("ERRORLEVEL 0 ECHO zero", 0, stub_exist, 0, &cmd);
    CHECK(r == 1, "IF ERRORLEVEL 0 with rc=0: true (0 >= 0)");

    /* IF NOT ERRORLEVEL 1 with rc=0 -> NOT(false) = TRUE.
     * RED under BATCH_MUTATE_IF_IGNORE_NOT (the NOT is dropped -> false). */
    r = batch_eval_if("NOT ERRORLEVEL 1 ECHO ok", 0, stub_exist, 0, &cmd);
    CHECK(r == 1,
          "IF NOT ERRORLEVEL 1 with rc=0: true (RED under IF_IGNORE_NOT)");
    CHECK(cmd != 0 && strcmp(cmd, "ECHO ok") == 0,
          "IF NOT ERRORLEVEL 1: out_cmd == 'ECHO ok'");

    /* IF NOT ERRORLEVEL 1 with rc=2 -> NOT(true) = FALSE.
     * RED under BATCH_MUTATE_IF_IGNORE_NOT (the NOT is dropped -> true). */
    r = batch_eval_if("NOT ERRORLEVEL 1 ECHO ok", 2, stub_exist, 0, &cmd);
    CHECK(r == 0,
          "IF NOT ERRORLEVEL 1 with rc=2: false (RED under IF_IGNORE_NOT)");

    /* Malformed: ERRORLEVEL with no number -> false, no command. */
    cmd = (const char *)1;
    r = batch_eval_if("ERRORLEVEL GOTO :X", 9, stub_exist, 0, &cmd);
    CHECK(r == 0, "IF ERRORLEVEL (no number): false (malformed)");
    CHECK(cmd == 0, "IF ERRORLEVEL (no number): out_cmd == NULL");
}

/* ---- test_if_exist ------------------------------------------------------- */

static void test_if_exist(void)
{
    const char *cmd;
    int r;

    /* IF EXIST <present file> -> true. */
    r = batch_eval_if("EXIST A:\\CONFIG.SYS GOTO :HAVE", 0, stub_exist, 0, &cmd);
    CHECK(r == 1, "IF EXIST A:\\CONFIG.SYS: true (present)");
    CHECK(cmd != 0 && strcmp(cmd, "GOTO :HAVE") == 0,
          "IF EXIST: out_cmd == 'GOTO :HAVE'");

    /* IF EXIST <absent file> -> false. */
    cmd = (const char *)1;
    r = batch_eval_if("EXIST A:\\NOPE.SYS ECHO x", 0, stub_exist, 0, &cmd);
    CHECK(r == 0, "IF EXIST A:\\NOPE.SYS: false (absent)");
    CHECK(cmd == 0, "IF EXIST absent: out_cmd == NULL");

    /* IF NOT EXIST <absent> -> NOT(false) = TRUE.
     * RED under BATCH_MUTATE_IF_IGNORE_NOT. */
    r = batch_eval_if("NOT EXIST A:\\NOPE.SYS ECHO mk", 0, stub_exist, 0, &cmd);
    CHECK(r == 1,
          "IF NOT EXIST absent: true (RED under IF_IGNORE_NOT)");
    CHECK(cmd != 0 && strcmp(cmd, "ECHO mk") == 0,
          "IF NOT EXIST: out_cmd == 'ECHO mk'");

    /* IF NOT EXIST <present> -> NOT(true) = FALSE.
     * RED under BATCH_MUTATE_IF_IGNORE_NOT. */
    r = batch_eval_if("NOT EXIST A:\\CONFIG.SYS ECHO mk", 0, stub_exist, 0,
                      &cmd);
    CHECK(r == 0,
          "IF NOT EXIST present: false (RED under IF_IGNORE_NOT)");

    /* NULL exist_fn -> EXIST evaluates as "does not exist" (false). */
    r = batch_eval_if("EXIST A:\\CONFIG.SYS ECHO y", 0, 0, 0, &cmd);
    CHECK(r == 0, "IF EXIST with NULL prober: false (fail safe)");
}

/* ---- test_if_strcmp ------------------------------------------------------ */

static void test_if_strcmp(void)
{
    const char *cmd;
    int r;

    /* The classic IF "%1"=="" empty-argument guard.  After batch_expand a bare
     * %1 expands to "" so the line reads IF ""=="".  Equal -> true. */
    r = batch_eval_if("\"\"==\"\" GOTO :USAGE", 0, stub_exist, 0, &cmd);
    CHECK(r == 1, "IF \"\"==\"\": true (empty-arg guard)");
    CHECK(cmd != 0 && strcmp(cmd, "GOTO :USAGE") == 0,
          "IF \"\"==\"\": out_cmd == 'GOTO :USAGE'");

    /* Non-empty argument: IF "install"=="" -> not equal -> false. */
    cmd = (const char *)1;
    r = batch_eval_if("\"install\"==\"\" GOTO :USAGE", 0, stub_exist, 0, &cmd);
    CHECK(r == 0, "IF \"install\"==\"\": false (arg present)");
    CHECK(cmd == 0, "IF \"install\"==\"\": out_cmd == NULL");

    /* Equal non-empty operands -> true. */
    r = batch_eval_if("\"YES\"==\"YES\" ECHO match", 0, stub_exist, 0, &cmd);
    CHECK(r == 1, "IF \"YES\"==\"YES\": true");
    CHECK(cmd != 0 && strcmp(cmd, "ECHO match") == 0,
          "IF strcmp equal: out_cmd == 'ECHO match'");

    /* Case-sensitive (DOS string IF is case-sensitive): "YES" != "yes". */
    r = batch_eval_if("\"YES\"==\"yes\" ECHO m", 0, stub_exist, 0, &cmd);
    CHECK(r == 0, "IF \"YES\"==\"yes\": false (case-sensitive)");

    /* IF NOT "a"=="b" -> NOT(false) = true (operands differ).
     * RED under BATCH_MUTATE_IF_IGNORE_NOT. */
    r = batch_eval_if("NOT \"a\"==\"b\" ECHO ne", 0, stub_exist, 0, &cmd);
    CHECK(r == 1, "IF NOT \"a\"==\"b\": true (RED under IF_IGNORE_NOT)");
    CHECK(cmd != 0 && strcmp(cmd, "ECHO ne") == 0,
          "IF NOT strcmp: out_cmd == 'ECHO ne'");

    /* Unquoted operands also compare (DOS takes them literally). */
    r = batch_eval_if("FOO==FOO ECHO u", 0, stub_exist, 0, &cmd);
    CHECK(r == 1, "IF FOO==FOO (unquoted): true");

    /* Malformed: no "==" and not ERRORLEVEL/EXIST -> false, no command. */
    cmd = (const char *)1;
    r = batch_eval_if("garbage here", 0, stub_exist, 0, &cmd);
    CHECK(r == 0, "IF garbage (no ==): false (malformed)");
    CHECK(cmd == 0, "IF garbage: out_cmd == NULL");
}

/* ---- test_for_parse ------------------------------------------------------ */

static void test_for_parse(void)
{
    char var[BATCH_LABEL_MAX];
    char set[BATCH_LINE_MAX];
    char cmd[BATCH_LINE_MAX];
    int  r;

    r = batch_for_parse("%%F IN (A.TXT B.TXT) DO TYPE %%F", var, set, cmd);
    CHECK(r == 1, "FOR parse: ok");
    CHECK(strcmp(var, "%%F") == 0, "FOR parse: var == '%%F' (with %% prefix)");
    CHECK(strcmp(set, "A.TXT B.TXT") == 0, "FOR parse: set == 'A.TXT B.TXT'");
    CHECK(strcmp(cmd, "TYPE %%F") == 0, "FOR parse: cmd == 'TYPE %%F'");

    /* Lowercase keywords (DOS is case-insensitive). */
    r = batch_for_parse("%%i in (a b c) do echo %%i", var, set, cmd);
    CHECK(r == 1, "FOR parse (lowercase): ok");
    CHECK(strcmp(var, "%%i") == 0, "FOR parse lc: var == '%%i'");
    CHECK(strcmp(set, "a b c") == 0, "FOR parse lc: set == 'a b c'");
    CHECK(strcmp(cmd, "echo %%i") == 0, "FOR parse lc: cmd == 'echo %%i'");

    /* Malformed: missing DO. */
    r = batch_for_parse("%%F IN (a b)", var, set, cmd);
    CHECK(r == 0, "FOR parse (no DO): fails");
    CHECK(var[0] == '\0', "FOR parse (no DO): var cleared");

    /* Malformed: missing IN. */
    r = batch_for_parse("%%F (a b) DO echo", var, set, cmd);
    CHECK(r == 0, "FOR parse (no IN): fails");

    /* Malformed: unterminated set. */
    r = batch_for_parse("%%F IN (a b DO echo", var, set, cmd);
    CHECK(r == 0, "FOR parse (unterminated set): fails");
}

/* ---- test_for_tokenize --------------------------------------------------- */

/* Tokenize a set and collect the tokens, returning the count.  Helper for the
 * tokenize checks below. */
static int collect_tokens(const char *set, char tokens[][BATCH_LINE_MAX],
                          int maxn)
{
    int cursor = 0;
    int n = 0;
    char tok[BATCH_LINE_MAX];
    while (n < maxn && batch_for_next_token(set, &cursor, tok, BATCH_LINE_MAX)) {
        strcpy(tokens[n], tok);
        n++;
    }
    return n;
}

static void test_for_tokenize(void)
{
    char tokens[8][BATCH_LINE_MAX];
    int  n;

    /* Space-separated.  RED under BATCH_MUTATE_FOR_NO_TOKENS (n == 0). */
    n = collect_tokens("A.TXT B.TXT C.TXT", tokens, 8);
    CHECK(n == 3, "FOR tokenize (spaces): 3 tokens (RED under FOR_NO_TOKENS)");
    CHECK(n >= 1 && strcmp(tokens[0], "A.TXT") == 0,
          "FOR tokenize: tokens[0] == 'A.TXT'");
    CHECK(n >= 3 && strcmp(tokens[2], "C.TXT") == 0,
          "FOR tokenize: tokens[2] == 'C.TXT'");

    /* Comma-separated (DOS FOR set comma separators). */
    n = collect_tokens("a,b,c", tokens, 8);
    CHECK(n == 3, "FOR tokenize (commas): 3 tokens (RED under FOR_NO_TOKENS)");
    CHECK(n >= 2 && strcmp(tokens[1], "b") == 0,
          "FOR tokenize (commas): tokens[1] == 'b'");

    /* Mixed + extra whitespace and trailing comma. */
    n = collect_tokens("  one ,two,  three  ", tokens, 8);
    CHECK(n == 3, "FOR tokenize (mixed ws/comma): 3 tokens");
    CHECK(n >= 1 && strcmp(tokens[0], "one") == 0,
          "FOR tokenize (mixed): tokens[0] == 'one'");
    CHECK(n >= 3 && strcmp(tokens[2], "three") == 0,
          "FOR tokenize (mixed): tokens[2] == 'three'");

    /* Single token. */
    n = collect_tokens("solo", tokens, 8);
    CHECK(n == 1, "FOR tokenize (single): 1 token");

    /* Empty set -> zero tokens (also the FOR_NO_TOKENS mutant result, but this
     * case is genuinely zero, so it stays GREEN under the mutant -- the
     * RED-biting checks above are the load-bearing ones). */
    n = collect_tokens("", tokens, 8);
    CHECK(n == 0, "FOR tokenize (empty set): 0 tokens");

    n = collect_tokens("   ", tokens, 8);
    CHECK(n == 0, "FOR tokenize (all whitespace): 0 tokens");
}

/* ---- test_for_subst ------------------------------------------------------ */

static void test_for_subst(void)
{
    char out[BATCH_LINE_MAX];
    int  n;

    /* Single occurrence. */
    n = batch_for_subst("TYPE %%F", "%%F", "README.TXT", out,
                        (int)sizeof(out));
    CHECK(n > 0, "FOR subst single: returns > 0");
    CHECK(strcmp(out, "TYPE README.TXT") == 0,
          "FOR subst single: 'TYPE README.TXT'");

    /* Multiple occurrences of the variable. */
    n = batch_for_subst("COPY %%F %%F.BAK", "%%F", "DATA", out,
                        (int)sizeof(out));
    CHECK(n > 0, "FOR subst multi: returns > 0");
    CHECK(strcmp(out, "COPY DATA DATA.BAK") == 0,
          "FOR subst multi: 'COPY DATA DATA.BAK'");

    /* No occurrence -> template copied verbatim. */
    n = batch_for_subst("ECHO hello", "%%F", "X", out, (int)sizeof(out));
    CHECK(n > 0, "FOR subst none: returns > 0");
    CHECK(strcmp(out, "ECHO hello") == 0, "FOR subst none: verbatim");

    /* Overflow guard (Rule 2): cap too small. */
    char tiny[4];
    n = batch_for_subst("ECHO %%F", "%%F", "LONGTOKEN", tiny, (int)sizeof(tiny));
    CHECK(n == -1, "FOR subst overflow: returns -1");
    CHECK(tiny[0] == '\0', "FOR subst overflow: out[0] == NUL (safe)");
}

/* ---- main ---------------------------------------------------------------- */

int main(void)
{
    test_if_errorlevel();
    test_if_exist();
    test_if_strcmp();
    test_for_parse();
    test_for_tokenize();
    test_for_subst();
    return TEST_SUMMARY("test_batch_exec");
}
