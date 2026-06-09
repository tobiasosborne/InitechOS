/* test_command.c -- host unit oracle for the PURE COMMAND.COM shell logic
 * (beads initech-7pc). Factory test: libc OK, reuses seed/test_assert.h.
 *
 * Ref: ADR-0003 DEC-11/DEC-12; DOS 3.3 COMMAND.COM (case-insensitive command +
 *      filename matching; the DIR/TYPE/CD/CLS/VER/ECHO/EXIT built-ins; external
 *      .COM resolution); spec/find_data.h (the DTA find-record fields the DIR
 *      formatter reads). CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN),
 *      Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * Compiles HOSTED against the REAL artifact command.c WITHOUT defining
 * COMMAND_KERNEL_REPL, so the inline-asm REPL is compiled out and only the pure
 * logic (the tokenizer/upcaser, the dispatch classifier, the ".COM appender",
 * and the DIR-record line formatter) is linked -- exactly the host-testability
 * seam command.h documents (mirrors how int21.c keeps its dispatch host-testable
 * behind the sink/conin seams).
 *
 * MUTATION (Rule 6), driven by make:
 *   -DCMD_MUTATE_NO_UPCASE     : cmd_parse stops upper-casing the command word
 *                                -> the classifier no longer recognizes a
 *                                lowercase "dir" as DIR; the parse test goes RED.
 *   -DCMD_MUTATE_COM_ALWAYS    : cmd_append_com appends ".COM" even when the name
 *                                already has an extension -> the appender test
 *                                (asserting GREET.COM stays GREET.COM) goes RED.
 *   -DCMD_MUTATE_BADCMD_BUILTIN: cmd_classify treats an unknown word as a
 *                                built-in (CMD_DIR) instead of CMD_EXTERNAL ->
 *                                the "badcmd is external" test goes RED.
 */

#include <stdint.h>
#include <string.h>

#include "command.h"
#include "dos_structs.h"   /* DIR_ATTR_DIRECTORY -- the formatter's attr input */
#include "test_assert.h"

TEST_HARNESS();

/* --- the tokenizer + upcaser + classifier ------------------------------- */
static void test_parse_basic(void)
{
    cmd_line_t p;

    /* Lowercase "dir" -> upper-cased command DIR, classified CMD_DIR, no arg. */
    cmd_parse("dir", &p);
    CHECK_STR_EQ(p.command, "DIR", "cmd_parse: 'dir' upper-cases to DIR");
    CHECK(p.kind == CMD_DIR, "cmd_parse: 'dir' classifies CMD_DIR");
    CHECK_STR_EQ(p.arg, "", "cmd_parse: 'dir' has no arg");

    /* "type hello.txt" -> command TYPE (upper), arg "hello.txt" (verbatim). */
    cmd_parse("type hello.txt", &p);
    CHECK_STR_EQ(p.command, "TYPE", "cmd_parse: 'type' upper-cases to TYPE");
    CHECK(p.kind == CMD_TYPE, "cmd_parse: 'type' classifies CMD_TYPE");
    CHECK_STR_EQ(p.arg, "hello.txt", "cmd_parse: arg preserved verbatim");

    /* Leading + interior spaces are trimmed around the command word. */
    cmd_parse("   dir   ", &p);
    CHECK_STR_EQ(p.command, "DIR", "cmd_parse: leading/trailing spaces trimmed");
    CHECK(p.kind == CMD_DIR, "cmd_parse: trimmed 'dir' still CMD_DIR");

    /* Empty / whitespace-only line -> CMD_EMPTY, command "". */
    cmd_parse("", &p);
    CHECK(p.kind == CMD_EMPTY, "cmd_parse: empty line -> CMD_EMPTY");
    CHECK_STR_EQ(p.command, "", "cmd_parse: empty line command is \"\"");
    cmd_parse("     ", &p);
    CHECK(p.kind == CMD_EMPTY, "cmd_parse: spaces-only -> CMD_EMPTY");

    /* ECHO preserves the case of its argument tail. */
    cmd_parse("echo Hello World", &p);
    CHECK_STR_EQ(p.command, "ECHO", "cmd_parse: echo -> ECHO");
    CHECK_STR_EQ(p.arg, "Hello World", "cmd_parse: ECHO arg keeps mixed case");
}

static void test_classify(void)
{
    CHECK(cmd_classify("DIR")   == CMD_DIR,      "classify DIR");
    CHECK(cmd_classify("TYPE")  == CMD_TYPE,     "classify TYPE");
    CHECK(cmd_classify("CD")    == CMD_CD,       "classify CD");
    CHECK(cmd_classify("CHDIR") == CMD_CD,       "classify CHDIR -> CD");
    CHECK(cmd_classify("CLS")   == CMD_CLS,      "classify CLS");
    CHECK(cmd_classify("VER")   == CMD_VER,      "classify VER");
    CHECK(cmd_classify("ECHO")  == CMD_ECHO,     "classify ECHO");
    CHECK(cmd_classify("EXIT")  == CMD_EXIT,     "classify EXIT");
    CHECK(cmd_classify("")      == CMD_EMPTY,    "classify empty -> CMD_EMPTY");
    /* The biting case (Rule 6 mutant target): an unknown word is EXTERNAL, NOT
     * a built-in -- so the REPL tries to EXEC it and emits Bad command on miss. */
    CHECK(cmd_classify("BADCMD") == CMD_EXTERNAL, "classify BADCMD -> EXTERNAL");
    CHECK(cmd_classify("GREET")  == CMD_EXTERNAL, "classify GREET -> EXTERNAL");
}

static void test_first_token(void)
{
    char tok[CMD_TOKEN_MAX];
    int n;

    n = cmd_first_token("hello.txt", tok);
    CHECK(n == 9, "first_token: length of hello.txt");
    CHECK_STR_EQ(tok, "hello.txt", "first_token: hello.txt");

    n = cmd_first_token("  greet  extra args ", tok);
    CHECK_STR_EQ(tok, "greet", "first_token: leading spaces skipped, first word only");

    n = cmd_first_token("", tok);
    CHECK(n == 0, "first_token: empty -> 0");
    CHECK_STR_EQ(tok, "", "first_token: empty -> \"\"");
}

static void test_append_com(void)
{
    char out[CMD_TOKEN_MAX];

    /* No extension -> append .COM. */
    CHECK(cmd_append_com("GREET", out) == 1, "append_com GREET ok");
    CHECK_STR_EQ(out, "GREET.COM", "append_com: GREET -> GREET.COM");

    /* Already has an extension -> unchanged (the biting mutant case). */
    CHECK(cmd_append_com("GREET.COM", out) == 1, "append_com GREET.COM ok");
    CHECK_STR_EQ(out, "GREET.COM", "append_com: GREET.COM stays GREET.COM");

    /* A different extension is preserved (not coerced to .COM). */
    CHECK(cmd_append_com("FOO.EXE", out) == 1, "append_com FOO.EXE ok");
    CHECK_STR_EQ(out, "FOO.EXE", "append_com: FOO.EXE preserved");
}

static void test_format_dir_line(void)
{
    char out[64];
    int n;

    /* A regular file: name left-justified, size right-justified, '\n'. */
    n = cmd_format_dir_line("HELLO.TXT", 31u, 0x20u /* ARCHIVE */, out);
    CHECK(n > 0, "format_dir_line: nonzero length");
    CHECK(out[n - 1] == '\n', "format_dir_line: ends with newline");
    CHECK(strncmp(out, "HELLO.TXT", 9) == 0, "format_dir_line: name at start");
    CHECK(strstr(out, "31") != NULL, "format_dir_line: size 31 present");

    /* A directory entry shows <DIR>, not a size. */
    n = cmd_format_dir_line("SUBDIR", 0u, DIR_ATTR_DIRECTORY, out);
    CHECK(strstr(out, "<DIR>") != NULL, "format_dir_line: directory shows <DIR>");
    CHECK(strncmp(out, "SUBDIR", 6) == 0, "format_dir_line: dir name at start");
}

int main(void)
{
    test_parse_basic();
    test_classify();
    test_first_token();
    test_append_com();
    test_format_dir_line();
    return TEST_SUMMARY("test_command");
}
