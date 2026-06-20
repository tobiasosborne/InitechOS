/* command.h -- InitechDOS COMMAND.COM-alike interactive shell (the A:\> REPL).
 *
 * beads: initech-7pc ("COMMAND.COM-alike shell: DIR, TYPE, CD, run program").
 *        M2 capstone (PRD Sec 6.1). RE-SCOPE (ADR-0003 DEC-11): the $P$G prompt
 *        + external-command resolution; the COMMAND.COM designation is retained
 *        verbatim. Batch (.BAT), PATH/COMSPEC resolution, and subdirectory
 *        traversal are DEFERRED (the last to initech-ti8).
 * Ref:   ADR-0003 DEC-11 (COMMAND.COM), DEC-12 (the $P$G prompt + version 3.30),
 *        Appendix D; DOS 3.3 COMMAND.COM behaviour (case-insensitive command +
 *        filename matching; the internal-command set DIR/TYPE/CD/CLS/VER/ECHO/
 *        EXIT; external .COM resolution); spec/dos_messages.json (the controlled
 *        diagnostics -- MSG-DOS-0002 "Bad command or file name", MSG-DOS-0003
 *        "File not found"); spec/find_data.h (the 43-byte DTA find-record DIR
 *        reads). CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Law 3
 *        (artifact = C), Law 4 (look/feel like period DOS), Rule 2 (fail loud),
 *        Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 *
 * HOST-TESTABILITY SEAM (Law 3, mirroring int21.c's sink/conin seams): the PURE
 * shell logic -- the command tokenizer + upcaser, the built-in dispatch
 * classifier, the ".COM appender", and the DIR-record line formatter -- lives in
 * command.c behind plain functions with NO inline asm and NO I/O, so the SAME TU
 * compiles HOSTED for os/milton/test_command.c. The REPL itself (command_repl)
 * and its thin `int $0x21` wrappers (CON I/O, FINDFIRST/NEXT, OPEN/READ/CLOSE,
 * EXEC) are kernel-only and compiled out of the host build via
 * COMMAND_KERNEL_REPL so test_command.c never sees the inline asm.
 */
#ifndef INITECH_COMMAND_H
#define INITECH_COMMAND_H

#include <stdint.h>

/* The maximum line the shell reads via AH=0Ah and the largest command/arg token
 * it parses. DOS COMMAND.COM uses a 128-byte command line; we mirror that order
 * of magnitude. The buffered-input block reserves CMD_LINE_MAX bytes for chars
 * plus the 2-byte AH=0Ah header (max + count). */
#define CMD_LINE_MAX   128
#define CMD_TOKEN_MAX   64   /* a single command word or filename arg (8.3+path) */

/* The recognized internal (built-in) commands, plus the "external" and "empty"
 * classifications. The REPL dispatches on this; the host oracle asserts the
 * classifier maps each command word to the right one (case-insensitively). */
typedef enum cmd_kind {
    CMD_EMPTY = 0,   /* the line had no command word (blank / all spaces)        */
    CMD_DIR,         /* DIR        -- list the root directory                    */
    CMD_TYPE,        /* TYPE <file>-- print a file's contents                    */
    CMD_CD,          /* CD/CHDIR   -- print/change the current directory         */
    CMD_MD,          /* MD/MKDIR   -- create a subdirectory (AH=39h)             */
    CMD_RD,          /* RD/RMDIR   -- remove an empty subdirectory (AH=3Ah)      */
    CMD_CLS,         /* CLS        -- clear the screen                           */
    CMD_VER,         /* VER        -- print the InitechDOS version line          */
    CMD_ECHO,        /* ECHO <text>-- print the text                            */
    CMD_BREAK,       /* BREAK [ON|OFF] -- report/set CTRL-BREAK state (AH=33h)   */
    CMD_EXIT,        /* EXIT       -- leave the REPL                             */
    CMD_SET,         /* SET [NAME[=VALUE]] -- list/assign/clear/query env vars   */
    CMD_COPY,        /* COPY <src> <dst> -- copy a single file (3Dh/3Ch/3Fh/40h) */
    CMD_DEL,         /* DEL/ERASE <name> -- delete file(s) (41h; wildcard 4E/4F) */
    CMD_REN,         /* REN/RENAME <old> <new> -- rename in place (56h)          */
    CMD_DATE,        /* DATE [MM-DD-YY[YY]] -- show/set the system date (2Ah/2Bh) */
    CMD_TIME,        /* TIME [HH:MM[:SS]] -- show/set the system time (2Ch/2Dh)   */
    CMD_EXTERNAL     /* not a built-in -- try to EXEC <word>.COM                 */
} cmd_kind_t;

/* A parsed command line: the command word (upper-cased) + the raw argument tail
 * (everything after the command word and its separating spaces, NOT upper-cased
 * -- ECHO must preserve case; TYPE/EXEC upcase the filename themselves) + the
 * DOS command tail (the verbatim remainder INCLUDING the leading separator, what
 * real DOS copies to the child PSP:80h for an EXEC'd program -- initech-456). */
typedef struct cmd_line {
    char     command[CMD_TOKEN_MAX];  /* the upper-cased command word ("" if none) */
    char     arg[CMD_LINE_MAX];       /* the argument tail, verbatim (may be "")    */
    char     tail[CMD_LINE_MAX];      /* DOS PSP:80h tail: leading sep + rest, raw  */
    cmd_kind_t kind;                  /* the classified built-in / external / empty */
} cmd_line_t;

/* ---- PURE, host-testable shell logic (NO asm, NO I/O) ---------------------- */

/* Upper-case one ASCII byte (a-z -> A-Z); other bytes pass through. DOS is
 * case-insensitive for commands + filenames; the keyboard injects lowercase. */
char cmd_upcase_char(char c);

/* Upper-case an ASCIIZ string in place (used for the first filename arg before
 * an OPEN / FINDFIRST / EXEC; DOS upcases 8.3 names). */
void cmd_upcase_str(char *s);

/* Parse `line` (an ASCIIZ command line, already stripped of the CR) into `out`:
 * skip leading spaces/tabs, copy the command word (up to the first space) into
 * out->command UPPER-CASED, then skip the spaces after it and copy the rest
 * verbatim into out->arg. Classifies into out->kind. A line with no command
 * word yields CMD_EMPTY with command[0]=='\0'. Deterministic; never overflows
 * (the inputs are bounded by CMD_LINE_MAX and tokens clamp to CMD_TOKEN_MAX). */
void cmd_parse(const char *line, cmd_line_t *out);

/* Classify an already-upper-cased command word into its cmd_kind_t. An empty
 * word is CMD_EMPTY; a recognized built-in is its enum; anything else is
 * CMD_EXTERNAL. Exposed for the oracle (cmd_parse calls it internally). */
cmd_kind_t cmd_classify(const char *upper_command);

/* Append ".COM" to a program name that carries no extension (no '.'), writing
 * the result into out[CMD_TOKEN_MAX]. A name that already contains a '.' is
 * copied unchanged (DOS only defaults the extension when none was typed). The
 * name is assumed already upper-cased. Returns 1 on success, 0 if the result
 * would not fit (Rule 2: never truncate a program name into a wrong file). */
int cmd_append_com(const char *name, char *out);

/* Trim trailing spaces from the FIRST whitespace-delimited token of `arg` into
 * out[CMD_TOKEN_MAX] (TYPE/EXEC take a single filename; extra params ignored
 * this milestone). Returns the token length; out is ASCIIZ. */
int cmd_first_token(const char *arg, char *out);

/* ---- SET built-in parsing (PURE, host-testable; beads initech-1i0x Tranche E
 * increment 2). Ref: DOS 3.3 COMMAND.COM SET behaviour (DEC-11 / Appendix D):
 *   SET              -> list every NAME=VALUE in the master environment.
 *   SET NAME=VALUE   -> UPSERT the variable (NAME is upcased by env_set).
 *   SET NAME=        -> remove the variable (empty value = CLEAR).
 *   SET NAME         -> show the current value, or an error if undefined. */

typedef enum set_op {
    SET_OP_LIST,     /* no arg / all-whitespace: print all NAME=VALUE lines     */
    SET_OP_ASSIGN,   /* NAME=VALUE (value may be empty -> CLEAR; see set_cmd_t) */
    SET_OP_CLEAR,    /* NAME= (trailing '=' with no value -> unset the var)     */
    SET_OP_QUERY     /* NAME (no '=' at all, name nonempty -> show current val) */
} set_op_t;

/* Parsed SET argument. `name` is the variable name (NOT yet upcased; env_set
 * handles upcasing). `value` is verbatim (may contain '=' or spaces after the
 * first '='; only the first '=' is the delimiter). For SET_OP_LIST both fields
 * are empty strings. For SET_OP_QUERY `value` is empty. For SET_OP_CLEAR
 * `value` is also empty (the REPL distinguishes CLEAR vs ASSIGN by the op). */
typedef struct set_cmd {
    set_op_t op;
    char     name[CMD_TOKEN_MAX];
    char     value[CMD_LINE_MAX];
} set_cmd_t;

/* Parse the argument tail of a SET command line into `out`. `arg` is the raw
 * argument string after the "SET " command word (i.e. cmd_line_t.arg). PURE:
 * no asm, no I/O. The function is deterministic and never overflows (inputs are
 * bounded by CMD_LINE_MAX; name is clamped to CMD_TOKEN_MAX-1). */
void cmd_set_parse(const char *arg, set_cmd_t *out);

/* ---- Tranche-F file/clock built-in parsing (PURE; host-testable) ----------
 * Ref: DOS 3.3 COMMAND.COM (DEC-11 / Appendix D); spec/int21h_register.json
 *   (3Dh OPEN, 3Ch CREAT, 3Fh READ, 40h WRITE, 3Eh CLOSE, 41h UNLINK,
 *    56h RENAME, 4Eh/4Fh FINDFIRST/NEXT, 2Ah/2Bh DATE, 2Ch/2Dh TIME).
 *   - COPY <src> <dst> : two filename tokens (initech-hpls).
 *   - REN/RENAME <old> <new> : two filename tokens (initech-fyox).
 *   - DEL/ERASE <name> : one filename token, wildcard-aware (initech-hpls).
 *   - DATE / TIME : the get-format builders + the optional set-arg parsers
 *     (initech-uy4l). */

/* A parsed two-token command tail (COPY <a> <b>, REN <a> <b>). `first`/`second`
 * are the two whitespace-delimited tokens, each clamped to CMD_TOKEN_MAX-1 and
 * UPPER-CASED (DOS upcases 8.3 names). `ok` is 1 iff BOTH tokens are present
 * (a missing second token => 0, the "Required parameter missing" case). */
typedef struct cmd_pair {
    char first[CMD_TOKEN_MAX];
    char second[CMD_TOKEN_MAX];
    int  ok;                 /* 1 iff both tokens parsed (else missing operand) */
} cmd_pair_t;

/* Parse the argument tail of a COPY / REN command into the two upcased tokens.
 * PURE: no asm, no I/O. `ok` is 0 if either token is empty (DOS needs both a
 * source and a destination). Deterministic; never overflows. */
void cmd_pair_parse(const char *arg, cmd_pair_t *out);

/* Return 1 if `name` contains a DOS wildcard ('*' or '?'), else 0. DEL/ERASE
 * dispatches a single UNLINK for a plain name and a FINDFIRST/NEXT delete loop
 * for a wildcard pattern. PURE. */
int cmd_has_wildcard(const char *name);

/* Build the DOS "Current date is Day MM-DD-YYYY" line into out (>= 32 bytes).
 * `dow` is the day-of-week (0=Sun..6=Sat) AH=2Ah returns in AL; `year` is the
 * full year, `mon`/`day` are 1-based. Returns the formatted length. PURE
 * formatting -- the day name + the zero-padded MM-DD-YYYY are period-DOS (Law 4). */
int cmd_format_date(uint8_t dow, uint16_t year, uint8_t mon, uint8_t day,
                    char *out);

/* Build the DOS "Current time is HH:MM:SS.cc" line into out (>= 32 bytes).
 * `hh`/`mi`/`ss` are 0-based; `cs` is centiseconds (the RTC reports 0). Returns
 * the formatted length. PURE. */
int cmd_format_time(uint8_t hh, uint8_t mi, uint8_t ss, uint8_t cs, char *out);

/* Parse a "MM-DD-YY[YY]" date argument into mon/day/year. Accepts '-' or '/' as
 * the DOS-permitted separators. A two-digit year is windowed DOS-style (80..99
 * -> 1980..1999, else 2000..2079). Returns 1 on a syntactically + range-valid
 * date, 0 otherwise (the REPL then prints "Invalid date" and skips the SET).
 * PURE: validation only, no I/O. */
int cmd_parse_date(const char *arg, uint16_t *year, uint8_t *mon, uint8_t *day);

/* Parse a "HH:MM[:SS]" time argument into hh/mi/ss. Seconds default to 0 when
 * omitted. Returns 1 on a syntactically + range-valid time, 0 otherwise (the
 * REPL then prints "Invalid time" and skips the SET). PURE. */
int cmd_parse_time(const char *arg, uint8_t *hh, uint8_t *mi, uint8_t *ss);

/* Format one DIR listing line from a find-record into out (DOS-like columns):
 * "NAME     EXT       <size>\n" -- the 8.3 name left-justified, the size right-
 * justified. `fname` is the ASCIIZ 8.3 name from the DTA find-record (offset
 * 0x15); `fsize` is the size (offset 0x11); `attr` is the attribute (0x0C). A
 * directory entry shows "<DIR>" in place of the size. out must hold >= 40 bytes;
 * returns the formatted length. Look/feel: period DOS DIR columns (Law 4). */
int cmd_format_dir_line(const char *fname, uint32_t fsize, uint8_t attr,
                        char *out);

/* ---- The REPL (kernel-only; compiled out of the host build) --------------- */
/* COMMAND_KERNEL_REPL is defined for the kernel command.o; the kmain BOOT_SHELL
 * object also needs the declaration, so BOOT_SHELL implies it for the header. */
#if defined(BOOT_SHELL) && !defined(COMMAND_KERNEL_REPL)
#define COMMAND_KERNEL_REPL
#endif

#ifdef COMMAND_KERNEL_REPL
/* Run the interactive COMMAND.COM REPL: print the banner is the caller's job;
 * this prints the $P$G prompt (A:\>), reads a line via INT 21h AH=0Ah, parses
 * and dispatches it, and loops until EXIT. All I/O is issued as real `int $0x21`
 * calls (dogfooding the OS API, the authentic COMMAND.COM design). Returns when
 * the user types EXIT; the caller (kmain BOOT_SHELL) then halts with a marker. */
void command_repl(void);
#endif

#endif /* INITECH_COMMAND_H */
