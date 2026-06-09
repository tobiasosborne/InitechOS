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
    CMD_CLS,         /* CLS        -- clear the screen                           */
    CMD_VER,         /* VER        -- print the InitechDOS version line          */
    CMD_ECHO,        /* ECHO <text>-- print the text                            */
    CMD_EXIT,        /* EXIT       -- leave the REPL                             */
    CMD_EXTERNAL     /* not a built-in -- try to EXEC <word>.COM                 */
} cmd_kind_t;

/* A parsed command line: the command word (upper-cased) + the raw argument tail
 * (everything after the command word and its separating spaces, NOT upper-cased
 * -- ECHO must preserve case; TYPE/EXEC upcase the filename themselves). */
typedef struct cmd_line {
    char     command[CMD_TOKEN_MAX];  /* the upper-cased command word ("" if none) */
    char     arg[CMD_LINE_MAX];       /* the argument tail, verbatim (may be "")    */
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
