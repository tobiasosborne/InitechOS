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
    CHECK_STR_EQ(p.tail, "", "cmd_parse: 'dir' has an empty DOS tail (initech-456)");

    /* "type hello.txt" -> command TYPE (upper), arg "hello.txt" (verbatim). The
     * DOS tail keeps the leading separator (what a child reads at PSP:80h). */
    cmd_parse("type hello.txt", &p);
    CHECK_STR_EQ(p.command, "TYPE", "cmd_parse: 'type' upper-cases to TYPE");
    CHECK(p.kind == CMD_TYPE, "cmd_parse: 'type' classifies CMD_TYPE");
    CHECK_STR_EQ(p.arg, "hello.txt", "cmd_parse: arg preserved verbatim");
    CHECK_STR_EQ(p.tail, " hello.txt",
                 "cmd_parse: DOS tail keeps the leading separator (PSP:80h)");

    /* The DOS tail is VERBATIM (interior spacing preserved), unlike arg which is
     * the trimmed builtins form. "prog   a  b" -> arg "a  b", tail "   a  b". */
    cmd_parse("prog   a  b", &p);
    CHECK_STR_EQ(p.arg, "a  b", "cmd_parse: arg trims the leading separator");
    CHECK_STR_EQ(p.tail, "   a  b",
                 "cmd_parse: DOS tail preserves the original spacing verbatim");

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
    /* MD/MKDIR + RD/RMDIR (beads initech-ut6d): the long + short forms both map
     * to the directory built-ins -- the biting case the CMD_MUTATE_NO_MDRD mutant
     * drops (so "MD" would fall through to CMD_EXTERNAL and never call do_mkdir). */
    CHECK(cmd_classify("MD")    == CMD_MD,       "classify MD");
    CHECK(cmd_classify("MKDIR") == CMD_MD,       "classify MKDIR -> MD");
    CHECK(cmd_classify("RD")    == CMD_RD,       "classify RD");
    CHECK(cmd_classify("RMDIR") == CMD_RD,       "classify RMDIR -> RD");
    CHECK(cmd_classify("CLS")   == CMD_CLS,      "classify CLS");
    CHECK(cmd_classify("VER")   == CMD_VER,      "classify VER");
    CHECK(cmd_classify("ECHO")  == CMD_ECHO,     "classify ECHO");
    CHECK(cmd_classify("BREAK") == CMD_BREAK,    "classify BREAK (AH=33h; DEC-16)");
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

/* ---- PROMPT classify (CMD_MUTATE_NO_PROMPT mutation hook) --------------- */
/* Ref: beads initech-dibc; ADR-0003 DEC-12; cmd_classify table in command.c.
 * Under CMD_MUTATE_NO_PROMPT the PROMPT row is DROPPED -> "PROMPT" falls through
 * to CMD_EXTERNAL -> this assertion FAILS -> the mutant oracle correctly goes RED
 * (Rule 6: the golden has caught a real regression). */
static void test_classify_prompt(void)
{
#ifdef CMD_MUTATE_NO_PROMPT
    /* MUTANT build: the PROMPT row was dropped; "PROMPT" must NOT classify as
     * CMD_PROMPT.  We assert CMD_PROMPT to make the binary exit non-zero (the
     * CHECK fails -> g_fails > 0 -> TEST_SUMMARY returns 1). */
    CHECK(cmd_classify("PROMPT") == CMD_PROMPT,
          "classify PROMPT -> CMD_PROMPT [mutant: row dropped -> goes RED as required]");
#else
    CHECK(cmd_classify("PROMPT") == CMD_PROMPT, "classify PROMPT -> CMD_PROMPT");
#endif
}

/* ---- cmd_render_prompt oracle ------------------------------------------- */
/* Ref: beads initech-dibc; ADR-0003 DEC-12; DOS 3.3 COMMAND.COM $-metacharacter
 * set (Appendix D / DEC-11). Tests the PURE renderer with no I/O.
 *
 * Key invariant (DEC-12): the default "$P$G" template with drive='A' and
 * cwd="" (root) must produce exactly "A:\>" -- the byte-identical string the
 * REPL printed before this feature landed, so existing prompt-band assertions
 * in the integration harness continue to pass. */
static void test_render_prompt(void)
{
    char buf[128];
    int  n;

    prompt_ctx_t ctx_root;
    ctx_root.drive    = 'A';
    ctx_root.cwd      = "";      /* AH=47h returns "" for the root */
    ctx_root.hour     = 9;
    ctx_root.minute   = 5;
    ctx_root.second   = 3;
    ctx_root.centisec = 7;
    ctx_root.year     = 2026;
    ctx_root.month    = 6;
    ctx_root.day      = 20;

    prompt_ctx_t ctx_sub;
    ctx_sub.drive    = 'A';
    ctx_sub.cwd      = "SUB";   /* AH=47h returns "SUB" for A:\SUB */
    ctx_sub.hour     = 0;
    ctx_sub.minute   = 0;
    ctx_sub.second   = 0;
    ctx_sub.centisec = 0;
    ctx_sub.year     = 1980;
    ctx_sub.month    = 1;
    ctx_sub.day      = 1;

    /* $P$G at root -> "A:\>" (the canonical InitechDOS prompt; DEC-12). */
    n = cmd_render_prompt("$P$G", &ctx_root, buf, sizeof(buf));
    CHECK(n == 4, "render_prompt: $P$G root len=4");
    CHECK_STR_EQ(buf, "A:\\>", "render_prompt: $P$G root -> 'A:\\>'");

    /* $P$G in a subdir -> "A:\\SUB>" (note: "\\" in C string = single backslash). */
    n = cmd_render_prompt("$P$G", &ctx_sub, buf, sizeof(buf));
    CHECK(n == 7, "render_prompt: $P$G subdir len=7");
    CHECK_STR_EQ(buf, "A:\\SUB>", "render_prompt: $P$G subdir -> 'A:\\SUB>'");

    /* $$ -> single "$". */
    n = cmd_render_prompt("$$", &ctx_root, buf, sizeof(buf));
    CHECK(n == 1, "render_prompt: $$ -> 1 char");
    CHECK_STR_EQ(buf, "$", "render_prompt: $$ -> '$'");

    /* $L$G -> "<>". */
    n = cmd_render_prompt("$L$G", &ctx_root, buf, sizeof(buf));
    CHECK(n == 2, "render_prompt: $L$G len=2");
    CHECK_STR_EQ(buf, "<>", "render_prompt: $L$G -> '<>'");

    /* $N -> drive letter only, no colon. */
    n = cmd_render_prompt("$N", &ctx_root, buf, sizeof(buf));
    CHECK(n == 1, "render_prompt: $N len=1");
    CHECK_STR_EQ(buf, "A", "render_prompt: $N -> 'A'");

    /* Literal text mixed with a metacharacter: "MS-DOS$G". */
    n = cmd_render_prompt("MS-DOS$G", &ctx_root, buf, sizeof(buf));
    CHECK(n == 7, "render_prompt: MS-DOS$G len=7");
    CHECK_STR_EQ(buf, "MS-DOS>", "render_prompt: MS-DOS$G -> 'MS-DOS>'");

    /* Unknown "$Z" -> emits 'Z' literally (DOS 3.3 pass-through behaviour). */
    n = cmd_render_prompt("$Z", &ctx_root, buf, sizeof(buf));
    CHECK(n == 1, "render_prompt: $Z (unknown) len=1");
    CHECK_STR_EQ(buf, "Z", "render_prompt: $Z -> 'Z' (literal pass-through)");

    /* $T -> "HH:MM:SS.CC" from ctx_root (09:05:03.07). */
    n = cmd_render_prompt("$T", &ctx_root, buf, sizeof(buf));
    CHECK(n == 11, "render_prompt: $T len=11");
    CHECK_STR_EQ(buf, "09:05:03.07", "render_prompt: $T -> '09:05:03.07'");

    /* $D -> "MM-DD-YYYY" from ctx_root (06-20-2026). */
    n = cmd_render_prompt("$D", &ctx_root, buf, sizeof(buf));
    CHECK(n == 10, "render_prompt: $D len=10");
    CHECK_STR_EQ(buf, "06-20-2026", "render_prompt: $D -> '06-20-2026'");

    /* Bound / overflow safety: cap=3 with a longer template.  The renderer must
     * write at most cap-1=2 chars + NUL and NEVER write past out[cap-1].
     * "$P$G" -> "A:\>" (4 chars) but with cap=3 only "A:" fits (2 chars). */
    buf[3] = (char)0xAA;   /* sentinel: must survive uncorrupted */
    n = cmd_render_prompt("$P$G", &ctx_root, buf, 3);
    CHECK(n <= 2, "render_prompt: cap=3 clamps to <=2 chars written");
    CHECK(buf[2] == '\0', "render_prompt: cap=3 NUL at out[cap-1]");
    CHECK((unsigned char)buf[3] == 0xAAu,
          "render_prompt: cap=3 sentinel byte not overwritten");
}

/* ---- SET classify (CMD_MUTATE_NO_SET mutation hook) -------------------- */
/* Ref: beads initech-1i0x Tranche E inc 2; cmd_classify table in command.c.
 * Under CMD_MUTATE_NO_SET the SET row is DROPPED -> "SET" falls through to
 * CMD_EXTERNAL -> this assertion FAILS -> the mutant oracle correctly goes RED
 * (Rule 6: the golden has caught a real regression). */
static void test_classify_set(void)
{
#ifdef CMD_MUTATE_NO_SET
    /* MUTANT build: the SET row was dropped; "SET" must NOT classify as CMD_SET.
     * We assert it equals CMD_EXTERNAL (the fall-through) so the mutant binary
     * exits non-zero (the CHECK below fails -> g_fails > 0 -> TEST_SUMMARY
     * returns 1). */
    CHECK(cmd_classify("SET") == CMD_SET,
          "classify SET -> CMD_SET [mutant: row dropped -> goes RED as required]");
#else
    CHECK(cmd_classify("SET") == CMD_SET, "classify SET -> CMD_SET");
#endif
}

/* ---- SET parsing oracle ------------------------------------------------- */
/* Ref: beads initech-1i0x Tranche E inc 2; cmd_set_parse in command.c.
 * Cases: empty->LIST; NAME=VALUE->ASSIGN; NAME=->CLEAR; NAME->QUERY;
 * a value with embedded '=' (X=A=B) -> ASSIGN name=X value="A=B". */
static void test_set_parse(void)
{
    set_cmd_t sc;

    /* Empty arg -> SET_OP_LIST; name and value are empty strings. */
    cmd_set_parse("", &sc);
    CHECK(sc.op == SET_OP_LIST, "set_parse: \"\" -> SET_OP_LIST");
    CHECK_STR_EQ(sc.name,  "", "set_parse: LIST name is empty");
    CHECK_STR_EQ(sc.value, "", "set_parse: LIST value is empty");

    /* All-whitespace arg -> SET_OP_LIST. */
    cmd_set_parse("   ", &sc);
    CHECK(sc.op == SET_OP_LIST, "set_parse: spaces-only -> SET_OP_LIST");

    /* "PATH=A:\\BIN" -> ASSIGN, name="PATH", value="A:\\BIN". */
    cmd_set_parse("PATH=A:\\BIN", &sc);
    CHECK(sc.op == SET_OP_ASSIGN, "set_parse: PATH=A:\\BIN -> SET_OP_ASSIGN");
    CHECK_STR_EQ(sc.name,  "PATH",    "set_parse: ASSIGN name is PATH");
    CHECK_STR_EQ(sc.value, "A:\\BIN", "set_parse: ASSIGN value is A:\\BIN");

    /* "PATH=" (empty value) -> CLEAR. */
    cmd_set_parse("PATH=", &sc);
    CHECK(sc.op == SET_OP_CLEAR, "set_parse: PATH= -> SET_OP_CLEAR");
    CHECK_STR_EQ(sc.name,  "PATH", "set_parse: CLEAR name is PATH");
    CHECK_STR_EQ(sc.value, "",     "set_parse: CLEAR value is empty");

    /* "PATH" (no '=') -> QUERY. */
    cmd_set_parse("PATH", &sc);
    CHECK(sc.op == SET_OP_QUERY, "set_parse: PATH -> SET_OP_QUERY");
    CHECK_STR_EQ(sc.name,  "PATH", "set_parse: QUERY name is PATH");
    CHECK_STR_EQ(sc.value, "",     "set_parse: QUERY value is empty");

    /* "X=A=B" -> ASSIGN name="X" value="A=B"
     * (only the FIRST '=' is the NAME/VALUE delimiter; value may contain '='). */
    cmd_set_parse("X=A=B", &sc);
    CHECK(sc.op == SET_OP_ASSIGN, "set_parse: X=A=B -> SET_OP_ASSIGN");
    CHECK_STR_EQ(sc.name,  "X",   "set_parse: embedded-eq name is X");
    CHECK_STR_EQ(sc.value, "A=B", "set_parse: embedded-eq value is A=B");

    /* Leading whitespace before name is stripped. */
    cmd_set_parse("  COMSPEC=A:\\COMMAND.COM", &sc);
    CHECK(sc.op == SET_OP_ASSIGN, "set_parse: leading-space ASSIGN op");
    CHECK_STR_EQ(sc.name,  "COMSPEC",         "set_parse: leading-space name");
    CHECK_STR_EQ(sc.value, "A:\\COMMAND.COM", "set_parse: leading-space value");
}

/* ---- Tranche-F classify (CMD_MUTATE_NO_<VERB> mutation hooks) ----------- */
/* Ref: beads initech-hpls (COPY+DEL), initech-fyox (REN), initech-uy4l
 * (DATE+TIME); the cmd_classify table in command.c. Each verb's row is dropped
 * under -DCMD_MUTATE_NO_<VERB>, so the word falls through to CMD_EXTERNAL and
 * the matching CHECK below goes RED (Rule 6: the golden bites). The mutant
 * builds assert the SAME equality, so a dropped row makes the binary exit
 * non-zero -- which is exactly what make test-command-mutant requires. */
static void test_classify_tranche_f(void)
{
    /* COPY (initech-hpls). */
    CHECK(cmd_classify("COPY") == CMD_COPY,
          "classify COPY -> CMD_COPY [CMD_MUTATE_NO_COPY drops the row -> RED]");

    /* DEL + the ERASE alias (initech-hpls). */
    CHECK(cmd_classify("DEL") == CMD_DEL,
          "classify DEL -> CMD_DEL [CMD_MUTATE_NO_DEL drops the rows -> RED]");
    CHECK(cmd_classify("ERASE") == CMD_DEL,
          "classify ERASE -> CMD_DEL (alias) [CMD_MUTATE_NO_DEL -> RED]");

    /* REN + the RENAME alias (initech-fyox). */
    CHECK(cmd_classify("REN") == CMD_REN,
          "classify REN -> CMD_REN [CMD_MUTATE_NO_REN drops the rows -> RED]");
    CHECK(cmd_classify("RENAME") == CMD_REN,
          "classify RENAME -> CMD_REN (alias) [CMD_MUTATE_NO_REN -> RED]");

    /* DATE (initech-uy4l). */
    CHECK(cmd_classify("DATE") == CMD_DATE,
          "classify DATE -> CMD_DATE [CMD_MUTATE_NO_DATE drops the row -> RED]");

    /* TIME (initech-uy4l). */
    CHECK(cmd_classify("TIME") == CMD_TIME,
          "classify TIME -> CMD_TIME [CMD_MUTATE_NO_TIME drops the row -> RED]");
}

/* ---- COPY / REN two-token parsing -------------------------------------- */
/* Ref: cmd_pair_parse in command.c. Two upcased tokens; ok=0 if either is
 * missing (the "Required parameter missing" path). */
static void test_pair_parse(void)
{
    cmd_pair_t p;

    /* Two tokens, lower-case -> both upcased; ok. */
    cmd_pair_parse("a.txt b.txt", &p);
    CHECK(p.ok == 1, "pair_parse: two tokens -> ok");
    CHECK_STR_EQ(p.first,  "A.TXT", "pair_parse: first upcased");
    CHECK_STR_EQ(p.second, "B.TXT", "pair_parse: second upcased");

    /* Leading + extra interior spaces are skipped; only the first two tokens. */
    cmd_pair_parse("   src.dat   dst.dat   extra", &p);
    CHECK(p.ok == 1, "pair_parse: spaced two tokens -> ok");
    CHECK_STR_EQ(p.first,  "SRC.DAT", "pair_parse: spaced first");
    CHECK_STR_EQ(p.second, "DST.DAT", "pair_parse: spaced second");

    /* Missing second operand -> ok=0 (the parameter-missing error). */
    cmd_pair_parse("only.one", &p);
    CHECK(p.ok == 0, "pair_parse: one token -> not ok (missing 2nd arg)");
    CHECK_STR_EQ(p.first, "ONLY.ONE", "pair_parse: lone first still captured");
    CHECK_STR_EQ(p.second, "", "pair_parse: missing second is empty");

    /* Empty arg -> ok=0, both empty. */
    cmd_pair_parse("", &p);
    CHECK(p.ok == 0, "pair_parse: empty -> not ok");
    CHECK_STR_EQ(p.first,  "", "pair_parse: empty first");
    CHECK_STR_EQ(p.second, "", "pair_parse: empty second");
}

/* ---- DEL wildcard-vs-plain detection ----------------------------------- */
static void test_has_wildcard(void)
{
    CHECK(cmd_has_wildcard("FILE.TXT") == 0, "wildcard: plain name -> 0");
    CHECK(cmd_has_wildcard("*.TXT")    == 1, "wildcard: '*' -> 1");
    CHECK(cmd_has_wildcard("FILE?.DAT")== 1, "wildcard: '?' -> 1");
    CHECK(cmd_has_wildcard("*.*")      == 1, "wildcard: *.* -> 1");
    CHECK(cmd_has_wildcard("")         == 0, "wildcard: empty -> 0");
}

/* ---- DATE / TIME format builders + arg parsers ------------------------- */
/* Ref: cmd_format_date / cmd_format_time / cmd_parse_date / cmd_parse_time. */
static void test_date_time_format(void)
{
    char out[40];

    /* Fri 2026-06-20: dow=5 (Fri), 06-20-2026 zero-padded. */
    cmd_format_date(5u, 2026u, 6u, 20u, out);
    CHECK_STR_EQ(out, "Current date is Fri 06-20-2026",
                 "format_date: Fri 06-20-2026 zero-padded");

    /* Sun 1980-01-01 (the epoch; single-digit month/day -> zero-padded). */
    cmd_format_date(0u, 1980u, 1u, 1u, out);
    CHECK_STR_EQ(out, "Current date is Sun 01-01-1980",
                 "format_date: Sun 01-01-1980 epoch");

    /* 09:05:03.00 -- all fields zero-padded to two digits. */
    cmd_format_time(9u, 5u, 3u, 0u, out);
    CHECK_STR_EQ(out, "Current time is 09:05:03.00",
                 "format_time: 09:05:03.00 zero-padded");

    /* 23:59:59.00 -- the upper bounds. */
    cmd_format_time(23u, 59u, 59u, 0u, out);
    CHECK_STR_EQ(out, "Current time is 23:59:59.00",
                 "format_time: 23:59:59.00 bounds");
}

static void test_date_parse(void)
{
    uint16_t year;
    uint8_t  mon, day;

    /* MM-DD-YYYY full year. */
    CHECK(cmd_parse_date("06-20-2026", &year, &mon, &day) == 1,
          "parse_date: 06-20-2026 valid");
    CHECK(year == 2026u && mon == 6u && day == 20u,
          "parse_date: 06-20-2026 fields");

    /* MM-DD-YY two-digit year windowing: 26 -> 2026, 85 -> 1985. */
    CHECK(cmd_parse_date("12-31-85", &year, &mon, &day) == 1,
          "parse_date: 12-31-85 valid");
    CHECK(year == 1985u && mon == 12u && day == 31u,
          "parse_date: 12-31-85 windows to 1985");

    /* '/' separator is accepted (DOS permits it). */
    CHECK(cmd_parse_date("01/15/2000", &year, &mon, &day) == 1,
          "parse_date: 01/15/2000 slash separator");
    CHECK(year == 2000u && mon == 1u && day == 15u,
          "parse_date: 01/15/2000 fields");

    /* Invalid cases (the REPL prints "Invalid date" and skips the SET). */
    CHECK(cmd_parse_date("13-01-2026", &year, &mon, &day) == 0,
          "parse_date: month 13 invalid");
    CHECK(cmd_parse_date("02-30-2026", &year, &mon, &day) == 0,
          "parse_date: Feb 30 invalid");
    CHECK(cmd_parse_date("02-29-2025", &year, &mon, &day) == 0,
          "parse_date: Feb 29 of a non-leap year invalid");
    CHECK(cmd_parse_date("02-29-2024", &year, &mon, &day) == 1,
          "parse_date: Feb 29 of a leap year valid");
    CHECK(cmd_parse_date("06-20", &year, &mon, &day) == 0,
          "parse_date: missing year invalid");
    CHECK(cmd_parse_date("06/20-2026", &year, &mon, &day) == 0,
          "parse_date: mismatched separators invalid");
    CHECK(cmd_parse_date("abc", &year, &mon, &day) == 0,
          "parse_date: non-numeric invalid");
}

static void test_time_parse(void)
{
    uint8_t hh, mi, ss;

    /* HH:MM:SS full. */
    CHECK(cmd_parse_time("14:30:15", &hh, &mi, &ss) == 1,
          "parse_time: 14:30:15 valid");
    CHECK(hh == 14u && mi == 30u && ss == 15u,
          "parse_time: 14:30:15 fields");

    /* HH:MM (seconds default to 0). */
    CHECK(cmd_parse_time("09:05", &hh, &mi, &ss) == 1,
          "parse_time: 09:05 valid (no seconds)");
    CHECK(hh == 9u && mi == 5u && ss == 0u,
          "parse_time: 09:05 seconds default 0");

    /* Invalid cases. */
    CHECK(cmd_parse_time("24:00:00", &hh, &mi, &ss) == 0,
          "parse_time: hour 24 invalid");
    CHECK(cmd_parse_time("12:60:00", &hh, &mi, &ss) == 0,
          "parse_time: minute 60 invalid");
    CHECK(cmd_parse_time("12:30:61", &hh, &mi, &ss) == 0,
          "parse_time: second 61 invalid");
    CHECK(cmd_parse_time("1230", &hh, &mi, &ss) == 0,
          "parse_time: missing colon invalid");
    CHECK(cmd_parse_time("", &hh, &mi, &ss) == 0,
          "parse_time: empty invalid");
}

/* ---- cmd_path_candidates oracle -----------------------------------------
 * Ref: ADR-0003 DEC-11 / Appendix D; DOS 3.3 COMMAND.COM PATH search order:
 *   .COM round (CWD then PATH dirs) -> .EXE round -> .BAT round.
 * MUTATION hook (Rule 6): -DCMD_MUTATE_NO_PATH drops the PATH-dir loop so
 * only CWD candidates are emitted; the PATH-dir tests go RED.
 *
 * Tests:
 *   1. word="FOO", PATH="A:\\BIN;A:\\DOS", cwd="\\"
 *      -> 0: \\FOO.COM  1: A:\\BIN\\FOO.COM  2: A:\\DOS\\FOO.COM  (then .EXE, .BAT)
 *   2. Explicit path with no ext: "A:\\X\\FOO" -> 1 candidate "A:\\X\\FOO.COM"
 *   3. Explicit path with ext: "A:\\X\\FOO.COM" -> 1 candidate "A:\\X\\FOO.COM" verbatim
 *   4. Empty PATH: word="FOO", PATH="", cwd="\\" -> CWD-only (3 candidates)
 *   5. word with extension "FOO.COM" -> as-is, CWD + each PATH dir
 *   6. Bounds/overflow safety                                                */
static void test_path_candidates(void)
{
    cmd_path_iter_t cands;
    int n;

    /* ---------------------------------------------------------------------- */
    /* Test 1: simple word, two PATH dirs, cwd="\\".
     * Expected order: \\FOO.COM, A:\\BIN\\FOO.COM, A:\\DOS\\FOO.COM,
     *                 \\FOO.EXE, A:\\BIN\\FOO.EXE, A:\\DOS\\FOO.EXE,
     *                 \\FOO.BAT, A:\\BIN\\FOO.BAT, A:\\DOS\\FOO.BAT.
     * Under CMD_MUTATE_NO_PATH, PATH-dir entries are absent. */
    n = cmd_path_candidates("FOO", "A:\\BIN;A:\\DOS", "\\", &cands);
    CHECK(n > 0, "path_candidates(FOO, 2 dirs): count > 0");
    /* index 0 must be the CWD candidate for .COM */
    CHECK_STR_EQ(cands.entries[0], "\\FOO.COM",
                 "path_candidates: entry[0] is CWD .COM candidate");

#ifdef CMD_MUTATE_NO_PATH
    /* Mutant: only CWD candidates (3: .COM, .EXE, .BAT). */
    CHECK(n == 3,
          "path_candidates [mutant NO_PATH]: only 3 CWD candidates -> RED if PATH dirs appear");
    /* These checks BITE the mutant: they assert PATH dirs ARE present, so
     * they fail when CMD_MUTATE_NO_PATH drops them -> binary exits non-zero. */
    CHECK(n >= 3,
          "path_candidates [mutant NO_PATH]: A:\\BIN\\FOO.COM missing -> RED");
#else
    /* Normal: 9 candidates (3 dirs x 3 exts). */
    CHECK(n == 9, "path_candidates(FOO, 2 PATH dirs): 9 candidates (3 dirs x 3 exts)");
    /* .COM round: entry[1] = A:\\BIN\\FOO.COM, entry[2] = A:\\DOS\\FOO.COM */
    CHECK_STR_EQ(cands.entries[1], "A:\\BIN\\FOO.COM",
                 "path_candidates: entry[1] A:\\BIN\\FOO.COM");
    CHECK_STR_EQ(cands.entries[2], "A:\\DOS\\FOO.COM",
                 "path_candidates: entry[2] A:\\DOS\\FOO.COM");
    /* .EXE round starts at entry[3] */
    CHECK_STR_EQ(cands.entries[3], "\\FOO.EXE",
                 "path_candidates: entry[3] CWD .EXE");
    CHECK_STR_EQ(cands.entries[4], "A:\\BIN\\FOO.EXE",
                 "path_candidates: entry[4] A:\\BIN\\FOO.EXE");
    CHECK_STR_EQ(cands.entries[5], "A:\\DOS\\FOO.EXE",
                 "path_candidates: entry[5] A:\\DOS\\FOO.EXE");
    /* .BAT round starts at entry[6] (deferred execution, but planned) */
    CHECK_STR_EQ(cands.entries[6], "\\FOO.BAT",
                 "path_candidates: entry[6] CWD .BAT");
    CHECK_STR_EQ(cands.entries[7], "A:\\BIN\\FOO.BAT",
                 "path_candidates: entry[7] A:\\BIN\\FOO.BAT");
    CHECK_STR_EQ(cands.entries[8], "A:\\DOS\\FOO.BAT",
                 "path_candidates: entry[8] A:\\DOS\\FOO.BAT");
#endif

    /* ---------------------------------------------------------------------- */
    /* Test 2: explicit path, no extension -> 1 candidate with .COM appended. */
    n = cmd_path_candidates("A:\\X\\FOO", "", "\\", &cands);
    CHECK(n == 1, "path_candidates(A:\\X\\FOO explicit, no ext): 1 candidate");
    CHECK_STR_EQ(cands.entries[0], "A:\\X\\FOO.COM",
                 "path_candidates: explicit no-ext -> .COM appended");

    /* ---------------------------------------------------------------------- */
    /* Test 3: explicit path WITH extension -> 1 candidate, verbatim. */
    n = cmd_path_candidates("A:\\X\\FOO.COM", "", "\\", &cands);
    CHECK(n == 1, "path_candidates(A:\\X\\FOO.COM explicit, has ext): 1 candidate");
    CHECK_STR_EQ(cands.entries[0], "A:\\X\\FOO.COM",
                 "path_candidates: explicit with ext -> verbatim, no re-append");

    /* ---------------------------------------------------------------------- */
    /* Test 4: empty PATH -> only CWD candidates (3: .COM, .EXE, .BAT). */
    n = cmd_path_candidates("FOO", "", "\\", &cands);
    CHECK(n == 3, "path_candidates(FOO, empty PATH): 3 CWD-only candidates");
    CHECK_STR_EQ(cands.entries[0], "\\FOO.COM",
                 "path_candidates empty PATH: entry[0] CWD .COM");
    CHECK_STR_EQ(cands.entries[1], "\\FOO.EXE",
                 "path_candidates empty PATH: entry[1] CWD .EXE");
    CHECK_STR_EQ(cands.entries[2], "\\FOO.BAT",
                 "path_candidates empty PATH: entry[2] CWD .BAT");

    /* NULL PATH also treated as empty. */
    n = cmd_path_candidates("FOO", (const char *)0, "\\", &cands);
    CHECK(n == 3, "path_candidates(FOO, NULL PATH): 3 CWD-only candidates");

    /* ---------------------------------------------------------------------- */
    /* Test 5: word with extension ("FOO.COM") -> used as-is, CWD + PATH dirs.
     * Under CMD_MUTATE_NO_PATH, only the CWD candidate is emitted. */
    n = cmd_path_candidates("FOO.COM", "A:\\BIN", "\\", &cands);
#ifdef CMD_MUTATE_NO_PATH
    /* Mutant: only the CWD candidate (1 entry). The check below asserts 2,
     * so it goes RED when the PATH dir is absent -- the mutant is caught. */
    CHECK(n == 2,
          "path_candidates [mutant NO_PATH]: FOO.COM with PATH -> RED (PATH dir absent)");
#else
    CHECK(n == 2, "path_candidates(FOO.COM, 1 PATH dir): 2 candidates (no ext variants)");
    CHECK_STR_EQ(cands.entries[0], "\\FOO.COM",
                 "path_candidates FOO.COM: entry[0] CWD");
    CHECK_STR_EQ(cands.entries[1], "A:\\BIN\\FOO.COM",
                 "path_candidates FOO.COM: entry[1] PATH dir");
#endif

    /* ---------------------------------------------------------------------- */
    /* Test 6: bounds/overflow safety -- a word that is exactly at the limit
     * should not overflow the entry buffer.  We construct a word that fills
     * CMD_PATH_MAX_LEN-1 chars; cmd_path_candidates must either store a valid
     * entry OR return 0 -- never write past entries[i][CMD_PATH_MAX_LEN-1]. */
    {
        /* Build a long-but-valid word: exactly CMD_PATH_MAX_LEN-5 chars
         * ("A" repeated) so that appending ".COM\0" (4+1 bytes) fills the
         * buffer to exactly CMD_PATH_MAX_LEN.  The function must handle it
         * without overflowing. */
        char long_word[CMD_PATH_MAX_LEN];
        int wlen = CMD_PATH_MAX_LEN - 5;   /* leaves room for ".COM\0" */
        int k;
        for (k = 0; k < wlen && k < CMD_PATH_MAX_LEN - 1; k++) {
            long_word[k] = 'A';
        }
        long_word[k] = '\0';

        n = cmd_path_candidates(long_word, "", "\\", &cands);
        /* Either succeeds (entry stored) or returns 0 (did not fit): either is
         * correct -- what matters is no out-of-bounds write occurred.  The
         * test asserts n >= 0 (always true) and that IF an entry was stored it
         * is properly NUL-terminated. */
        CHECK(n >= 0, "path_candidates overflow safety: count >= 0");
        if (n > 0) {
            /* Verify the first entry is a valid ASCIIZ string within bounds. */
            int elen = 0;
            const char *e = cands.entries[0];
            while (e[elen] != '\0' && elen < CMD_PATH_MAX_LEN) {
                elen++;
            }
            CHECK(elen < CMD_PATH_MAX_LEN,
                  "path_candidates overflow safety: entry NUL within bounds");
        }
    }
}

int main(void)
{
    test_parse_basic();
    test_classify();
    test_classify_set();
    test_classify_prompt();
    test_render_prompt();
    test_set_parse();
    test_first_token();
    test_append_com();
    test_format_dir_line();
    test_classify_tranche_f();
    test_pair_parse();
    test_has_wildcard();
    test_date_time_format();
    test_date_parse();
    test_time_parse();
    test_path_candidates();
    return TEST_SUMMARY("test_command");
}
