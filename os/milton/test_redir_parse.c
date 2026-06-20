/* test_redir_parse.c -- host unit oracle for cmd_redir_parse, the PURE COMMAND.COM
 * OUTPUT-redirection parser (beads initech-hsct). Factory test: libc OK, reuses
 * seed/test_assert.h.
 *
 * Ref: PRD Sec 6.1 (COMMAND.COM personality); MS-DOS 3.3 Tech Ref Ch.6 (the shell
 *      strips `> file` / `>> file` and DUP2's the file onto handle 1). CLAUDE.md
 *      Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6 (mutation-prove),
 *      Rule 12 (ASCII).
 *
 * Compiles HOSTED against the REAL artifact command.c WITHOUT defining
 * COMMAND_KERNEL_REPL, so only the pure logic (incl. cmd_redir_parse) is linked
 * -- the inline-asm REPL + redirect driver are compiled out (Law 3 seam).
 *
 * MUTATION (Rule 6), driven by make:
 *   -DCMD_MUTATE_REDIR_NO_APPEND   : `>>` is treated as a single `>` (truncate),
 *                                    so the append flag never sets -> the
 *                                    `>>`-append assertions go RED.
 *   -DCMD_MUTATE_REDIR_KEEP_TARGET : the target token is left IN clean_out, so
 *                                    the cleaned-command assertions go RED.
 */

#include <stdint.h>
#include <string.h>

#include "command.h"
#include "test_assert.h"

TEST_HARNESS();

/* Convenience wrapper: run the parser with generous buffers and surface the
 * three outputs.  Returns the has_redirect flag. */
static int run_redir(const char *line, char *clean, char *target, int *append)
{
    return cmd_redir_parse(line, clean, 128u, target, 128u, append);
}

/* --- no-redirect passthrough -------------------------------------------- */
static void test_no_redirect(void)
{
    char clean[128];
    char target[128];
    int  append = 9;   /* poisoned; must be reset to 0 */

    int has = run_redir("echo hello", clean, target, &append);
    CHECK(has == 0, "no '>' -> has_redirect == 0");
    CHECK_STR_EQ(clean, "echo hello", "no redirect: clean == line verbatim");
    CHECK_STR_EQ(target, "", "no redirect: target empty");
    CHECK(append == 0, "no redirect: append cleared to 0");

    /* A line that is just a plain command with args, no '>'. */
    has = run_redir("dir /w a:\\sub", clean, target, &append);
    CHECK(has == 0, "no redirect (dir /w): has_redirect == 0");
    CHECK_STR_EQ(clean, "dir /w a:\\sub", "no redirect: clean preserves args");

    /* Empty line. */
    has = run_redir("", clean, target, &append);
    CHECK(has == 0, "empty line: has_redirect == 0");
    CHECK_STR_EQ(clean, "", "empty line: clean empty");
}

/* --- create/truncate `>` ------------------------------------------------- */
static void test_truncate(void)
{
    char clean[128];
    char target[128];
    int  append = 9;

    /* Spaced form: "echo HELLO > OUT.TXT" (the canonical case). */
    int has = run_redir("echo HELLO > OUT.TXT", clean, target, &append);
    CHECK(has == 1, "'>' (spaced): has_redirect == 1");
    CHECK(append == 0, "'>' (spaced): append == 0 (truncate)");
    CHECK_STR_EQ(target, "OUT.TXT", "'>' (spaced): target == OUT.TXT");
    CHECK_STR_EQ(clean, "echo HELLO", "'>' (spaced): clean == 'echo HELLO'");

    /* No-space form: "echo HELLO>OUT.TXT". */
    has = run_redir("echo HELLO>OUT.TXT", clean, target, &append);
    CHECK(has == 1, "'>' (no-space): has_redirect == 1");
    CHECK(append == 0, "'>' (no-space): append == 0");
    CHECK_STR_EQ(target, "OUT.TXT", "'>' (no-space): target == OUT.TXT");
    CHECK_STR_EQ(clean, "echo HELLO", "'>' (no-space): clean == 'echo HELLO'");

    /* Operator hugging the command but spaced target: "echo HELLO> OUT.TXT". */
    has = run_redir("echo HELLO> OUT.TXT", clean, target, &append);
    CHECK(has == 1, "'>' (cmd-hug): has_redirect == 1");
    CHECK_STR_EQ(target, "OUT.TXT", "'>' (cmd-hug): target == OUT.TXT");
    CHECK_STR_EQ(clean, "echo HELLO", "'>' (cmd-hug): clean == 'echo HELLO'");

    /* Just a bare command + redirect: "dir > log". */
    has = run_redir("dir > log", clean, target, &append);
    CHECK(has == 1, "'>' (dir): has_redirect == 1");
    CHECK_STR_EQ(target, "log", "'>' (dir): target == log");
    CHECK_STR_EQ(clean, "dir", "'>' (dir): clean == 'dir'");
}

/* --- append `>>` --------------------------------------------------------- */
static void test_append(void)
{
    char clean[128];
    char target[128];
    int  append = 0;

    /* Spaced form: "dir >> log". */
    int has = run_redir("dir >> log", clean, target, &append);
    CHECK(has == 1, "'>>' (spaced): has_redirect == 1");
    CHECK(append == 1, "'>>' (spaced): append == 1 (THE append assertion)");
    CHECK_STR_EQ(target, "log", "'>>' (spaced): target == log");
    CHECK_STR_EQ(clean, "dir", "'>>' (spaced): clean == 'dir'");

    /* No-space form: "echo HI>>log.txt". */
    has = run_redir("echo HI>>log.txt", clean, target, &append);
    CHECK(has == 1, "'>>' (no-space): has_redirect == 1");
    CHECK(append == 1, "'>>' (no-space): append == 1");
    CHECK_STR_EQ(target, "log.txt", "'>>' (no-space): target == log.txt");
    CHECK_STR_EQ(clean, "echo HI", "'>>' (no-space): clean == 'echo HI'");

    /* Mixed spacing: "type a.txt >>b.log". */
    has = run_redir("type a.txt >>b.log", clean, target, &append);
    CHECK(has == 1, "'>>' (mixed): has_redirect == 1");
    CHECK(append == 1, "'>>' (mixed): append == 1");
    CHECK_STR_EQ(target, "b.log", "'>>' (mixed): target == b.log");
    CHECK_STR_EQ(clean, "type a.txt", "'>>' (mixed): clean preserves the arg");
}

/* --- last-operator-wins + interior args preserved ------------------------ */
static void test_last_wins(void)
{
    char clean[128];
    char target[128];
    int  append = 0;

    /* Two operators: the LAST wins (DOS: a later stdout redirect supersedes). */
    int has = run_redir("echo X > a.txt > b.txt", clean, target, &append);
    CHECK(has == 1, "two '>': has_redirect == 1");
    CHECK_STR_EQ(target, "b.txt", "two '>': last target wins (b.txt)");
    CHECK(append == 0, "two '>': append == 0");
    /* The earlier "> a.txt" is left in clean as ordinary text (we only strip the
     * winning operator+target); the command still dispatches with the first
     * redirect text as args, which is acceptable for this simple shell. The key
     * assertion is the WINNING target. */

    /* `>` then `>>`: the last (>>) wins, append set. */
    has = run_redir("dir > a >> b", clean, target, &append);
    CHECK(has == 1, "'>' then '>>': has_redirect == 1");
    CHECK_STR_EQ(target, "b", "'>' then '>>': last target wins (b)");
    CHECK(append == 1, "'>' then '>>': append == 1 (last op is append)");

    /* Interior args of the command are preserved on the clean side. */
    has = run_redir("type a.txt b.txt > out", clean, target, &append);
    CHECK(has == 1, "multi-arg cmd: has_redirect == 1");
    CHECK_STR_EQ(target, "out", "multi-arg cmd: target == out");
    CHECK_STR_EQ(clean, "type a.txt b.txt",
                 "multi-arg cmd: clean keeps BOTH args");
}

/* --- syntax / edge cases ------------------------------------------------- */
static void test_edges(void)
{
    char clean[128];
    char target[128];
    int  append = 0;

    /* Redirect with NO target: "echo hi >".  has_redirect==1 but target empty
     * (the driver treats this as a DOS "Required parameter missing"). */
    int has = run_redir("echo hi >", clean, target, &append);
    CHECK(has == 1, "no target: has_redirect == 1 (operator present)");
    CHECK_STR_EQ(target, "", "no target: target empty (driver aborts the line)");
    CHECK_STR_EQ(clean, "echo hi", "no target: clean keeps the command");

    /* Trailing whitespace after the operator only: "dir >   ". */
    has = run_redir("dir >   ", clean, target, &append);
    CHECK(has == 1, "ws-only target: has_redirect == 1");
    CHECK_STR_EQ(target, "", "ws-only target: target empty");
    CHECK_STR_EQ(clean, "dir", "ws-only target: clean == 'dir'");

    /* Leading operator (no command): "> file" -- degenerate but must not crash;
     * clean becomes empty, target captured. */
    has = run_redir("> file", clean, target, &append);
    CHECK(has == 1, "leading '>': has_redirect == 1");
    CHECK_STR_EQ(target, "file", "leading '>': target == file");
    CHECK_STR_EQ(clean, "", "leading '>': clean empty");

    /* NULL line must not crash and reports no redirect. */
    has = cmd_redir_parse(0, clean, 128u, target, 128u, &append);
    CHECK(has == 0, "NULL line: has_redirect == 0");
    CHECK_STR_EQ(clean, "", "NULL line: clean empty");
}

/* --- buffer-bound guards (Rule 2: never overflow) ------------------------ */
static void test_bounds(void)
{
    /* A long line + tiny output buffers: the parser must NUL-terminate within
     * the caps and never write past the end.  We pad the buffers with a sentinel
     * and assert the byte just past the cap is untouched. */
    char clean[8];
    char target[8];
    int  append = 0;

    /* 16-char-ish command + 16-char target into 8-byte buffers. */
    const char *line = "command_word > target_filename.txt";

    /* Surround buffers with sentinels by over-allocating and checking the cap-1
     * boundary. We pass cap=8 explicitly. */
    char clean_guard[16];
    char target_guard[16];
    memset(clean_guard, 0x7E, sizeof(clean_guard));
    memset(target_guard, 0x7E, sizeof(target_guard));

    int has = cmd_redir_parse(line, clean_guard, 8u, target_guard, 8u, &append);
    CHECK(has == 1, "bounds: has_redirect == 1");
    /* clean_guard[0..7] must be a valid ASCIIZ string (NUL within [0,8)). */
    CHECK(memchr(clean_guard, '\0', 8) != 0, "bounds: clean NUL within cap");
    CHECK(memchr(target_guard, '\0', 8) != 0, "bounds: target NUL within cap");
    /* The sentinel past the cap must be intact (no overflow). */
    CHECK((unsigned char)clean_guard[8] == 0x7E,
          "bounds: no write past clean_out cap");
    CHECK((unsigned char)target_guard[8] == 0x7E,
          "bounds: no write past target_out cap");

    /* Same for the no-redirect passthrough clamp. */
    memset(clean_guard, 0x7E, sizeof(clean_guard));
    has = cmd_redir_parse("a_very_long_command_with_no_redirect_at_all",
                          clean_guard, 8u, target, 8u, &append);
    CHECK(has == 0, "bounds: no-redirect long line -> has_redirect == 0");
    CHECK(memchr(clean_guard, '\0', 8) != 0,
          "bounds: no-redirect clean NUL within cap");
    CHECK((unsigned char)clean_guard[8] == 0x7E,
          "bounds: no-redirect no write past clean cap");

    /* Tiny target buffer truncation: target longer than cap. */
    char tgt2[4];
    memset(tgt2, 0x7E, sizeof(tgt2));
    has = cmd_redir_parse("x > abcdefgh", clean, 128u, tgt2, 4u, &append);
    CHECK(has == 1, "bounds: tiny target buf -> has_redirect == 1");
    CHECK(strncmp(tgt2, "abc", 3) == 0, "bounds: target truncated to 'abc'");
    CHECK(tgt2[3] == '\0', "bounds: truncated target NUL at cap-1");

    (void)clean;
}

int main(void)
{
    test_no_redirect();
    test_truncate();
    test_append();
    test_last_wins();
    test_edges();
    test_bounds();
    return TEST_SUMMARY("test_redir_parse");
}
