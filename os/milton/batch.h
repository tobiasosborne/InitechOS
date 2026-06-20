/* batch.h -- InitechDOS COMMAND.COM .BAT interpreter: pure batch-language
 *            parser and parameter/variable expander.
 *
 * beads: initech-xw1 (batch interpreter -- LOGIC half; REPL/AUTOEXEC
 *        integration into command.c is a separate later bead).
 * Ref:   DOS 3.3 COMMAND.COM batch-file processing (Ralf Brown's Interrupt
 *        List; Peter Norton "DOS Programmer's Reference" 3rd ed.; Microsoft
 *        MS-DOS 3.3 Technical Reference, Chapter 3 "Batch Files"):
 *          - Parameter expansion: %0..%9 from argv, %VAR% from environment,
 *            %% -> literal '%'.  Out-of-range %n -> empty string (DOS 3.3).
 *          - Batch directives: REM, ECHO ON|OFF|<text>, @, GOTO :label,
 *            :label, IF [NOT] ERRORLEVEL n / IF [NOT] EXIST file /
 *            IF [NOT] "%1"=="x", FOR %%v IN (set) DO cmd, SHIFT, CALL,
 *            PAUSE.
 *          - ECHO with no argument: prints "ECHO is on" or "ECHO is off"
 *            (not exposed here; the REPL handles that display).
 *        CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Law 3
 *        (artifact = C), Rule 2 (fail loud), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 * No libc: hand-rolled helpers (same discipline as env.c / command.c).
 *
 * HOST-TESTABILITY SEAM: all batch_* functions are PURE (no asm, no I/O),
 * so the SAME TU compiles HOSTED for os/milton/test_batch.c.  The test file
 * #includes batch.c directly (the same TU trick as test_env.c / test_mz.c).
 *
 * DECOUPLING: this module does NOT depend on env.c.  The env callback is a
 * caller-supplied function pointer (batch_env_lookup_fn) so the orchestrator
 * can wire g_master_env's env_get as the callback during REPL integration
 * without this module knowing about env_store_t.
 *
 * MUTATION hooks (CLAUDE.md Rule 6 -- driven by make test-batch-mutant):
 *   BATCH_MUTATE_NO_PCT_PCT    -- batch_expand does NOT collapse %% -> '%';
 *                                 the %% test goes RED.
 *   BATCH_MUTATE_ECHO_OFF_MISS -- batch_classify misidentifies "ECHO OFF"
 *                                 as BL_ECHO_TEXT; the ECHO OFF test goes RED.
 *   BATCH_MUTATE_GOTO_NOLABEL  -- batch_goto_target always returns 0 chars
 *                                 extracted; the GOTO target test goes RED.
 */
#ifndef INITECH_BATCH_H
#define INITECH_BATCH_H

#include <stdint.h>

/* Maximum line length a .BAT file may contain.  Mirror CMD_LINE_MAX (128)
 * from command.h -- .BAT lines are fed through the same DOS line-input path.
 * We define our own constant here to keep batch.h independent of command.h. */
#define BATCH_LINE_MAX  128

/* Maximum length of a GOTO target label or a batch variable name (%%v FOR
 * loop variable; %VAR% environment name).  Conservative; labels in real DOS
 * batch files are rarely longer than 8 characters. */
#define BATCH_LABEL_MAX  64

/* ---- Environment callback ------------------------------------------------
 * The caller supplies a function of this type so batch_expand can look up
 * %VARNAME% without depending on env.c.  The callback receives a NUL-
 * terminated variable name (already stripped of the surrounding '%') and
 * returns a pointer to the value string on success, or NULL if the variable
 * is not defined.  The returned pointer must remain valid for the duration of
 * the batch_expand call (the batch module does not cache it). */
typedef const char *(*batch_env_lookup_fn)(const char *name, void *ctx);

/* ---- Parameter / variable expansion ------------------------------------- */

/* Expand one .BAT line's parameter and variable references into `out`.
 *
 * Substitution rules (DOS 3.3 faithful):
 *   %0         -> argv[0]  (the batch file name, caller-supplied)
 *   %1..%9     -> argv[1]..argv[9]  (batch parameters)
 *   %n out-of-range (n >= argc) -> empty string  (DOS 3.3 behavior)
 *   %VARNAME%  -> value from `env` callback; if NULL (undefined) -> the
 *                 literal text "%VARNAME%" is emitted  (DOS 3.3 behavior:
 *                 undefined variables are left unexpanded in the output).
 *   %%         -> '%'  (escape for a literal percent sign)
 *   Lone '%' at end of string / %<non-digit-non-name> -> '%' passed through.
 *
 * `argv` is an array of `argc` pointers (argv[0] = batch name, argv[1..9]
 * are the positional parameters).  Any of argv[i] may be NULL, treated as
 * empty string.
 *
 * `env` may be NULL (no environment lookup); in that case all %VAR% refs
 * are left as the literal text "%VAR%".  `envctx` is forwarded to `env`.
 *
 * Writes at most `cap`-1 characters to `out` and always NUL-terminates
 * (Rule 2: never overflow).  Returns the number of characters written (not
 * counting the NUL).  Returns -1 if the expanded result would not fit in
 * `cap` bytes (the partial output is indeterminate; `out[0]` is set to NUL
 * for safety).
 *
 * Deterministic (Rule 11).  Pure: no asm, no I/O. */
int batch_expand(const char *line,
                 const char *const argv[], int argc,
                 batch_env_lookup_fn env, void *envctx,
                 char *out, int cap);

/* ---- Line classification ------------------------------------------------ */

/* Every .BAT line falls into exactly one of these kinds.  The batch REPL
 * uses this to dispatch execution; the PARSE/classification is all that
 * lives in this module -- execution semantics (IF evaluation, FOR iteration,
 * label scanning) belong in the REPL integration (the later command.c bead).
 *
 * Ref: MS-DOS 3.3 Technical Reference, Chapter 3: "Batch Commands". */
typedef enum {
    BL_BLANK,       /* empty or all-whitespace line (skip)                     */
    BL_REM,         /* REM <comment>  (skip; no echo even when echo is on)     */
    BL_LABEL,       /* :labelname  (a GOTO target; skip execution)             */
    BL_ECHO_ON,     /* ECHO ON      (turn echo on)                             */
    BL_ECHO_OFF,    /* ECHO OFF     (turn echo off)                            */
    BL_ECHO_TEXT,   /* ECHO <text>  (print text; also "ECHO" with no arg       */
                    /*              -> caller checks echo_text[0] for empty)   */
    BL_GOTO,        /* GOTO :label  (jump to label)                            */
    BL_IF,          /* IF [NOT] ...  (conditional; REPL evaluates condition)   */
    BL_FOR,         /* FOR %%v IN (...) DO cmd  (loop; REPL expands set)       */
    BL_SHIFT,       /* SHIFT  (shift positional params left by one)            */
    BL_CALL,        /* CALL <batchfile>  (nested batch invocation)             */
    BL_PAUSE,       /* PAUSE  (display "Strike a key..." and wait)             */
    BL_COMMAND      /* anything else: an external or internal DOS command      */
} batch_line_kind_t;

/* Output of batch_classify: the kind plus extracted sub-fields.
 *
 * Fields populated per kind:
 *   BL_LABEL    -> label_name (the name after ':', NUL-terminated, no ':')
 *   BL_ECHO_ON/OFF/TEXT -> echo_text (for BL_ECHO_TEXT: the text to print;
 *                          for BL_ECHO_ON/OFF: empty string "")
 *   BL_GOTO     -> goto_target (the label name to jump to, no ':')
 *   BL_COMMAND  -> (the caller uses the full unexpanded `line` after stripping
 *                   the leading '@' if at_suppressed is set)
 *   All other kinds: fields are empty strings / 0.
 *
 * `at_suppressed` is set to 1 if the line began with '@' (suppress echoing
 * this line regardless of ECHO state).  The '@' is always stripped before
 * the directive is parsed.
 *
 * PURE: no asm, no I/O.  Deterministic (Rule 11). */
typedef struct {
    batch_line_kind_t kind;
    int               at_suppressed;          /* 1 if leading '@' was present */
    char              label_name[BATCH_LABEL_MAX];  /* BL_LABEL: the name    */
    char              echo_text[BATCH_LINE_MAX];    /* BL_ECHO_TEXT: the text */
    char              goto_target[BATCH_LABEL_MAX]; /* BL_GOTO: target label  */
} batch_parsed_t;

/* Classify and partially parse one .BAT line (BEFORE parameter expansion;
 * classification operates on the raw line as written in the file).
 *
 * `line`  -- ASCIIZ .BAT source line, CR/LF already stripped.
 * `out`   -- receives the classification result (must not be NULL).
 *
 * The function never overflows any output field (BATCH_LINE_MAX /
 * BATCH_LABEL_MAX guards are applied; excess chars are silently truncated
 * within the field bounds).  Returns the kind (same as out->kind) for
 * convenience.
 *
 * Case-insensitive for the directive keyword (ECHO == echo == Echo; DOS
 * does not distinguish).  The '@' prefix is recognized and stripped first.
 *
 * PURE: no asm, no I/O. */
batch_line_kind_t batch_classify(const char *line, batch_parsed_t *out);

/* ---- GOTO / label helper ------------------------------------------------ */

/* Return 1 if `line` is a label definition (:name) whose name matches
 * `target` (case-insensitive, DOS 3.3 behaviour: GOTO is case-insensitive).
 * Return 0 otherwise.  Skips leading whitespace before the ':'.
 *
 * Used by the REPL's label-scan loop: step through the .BAT file line by
 * line calling batch_label_matches(line, target) until a match is found.
 *
 * PURE: no asm, no I/O.  Deterministic. */
int batch_label_matches(const char *line, const char *target);

#endif /* INITECH_BATCH_H */
