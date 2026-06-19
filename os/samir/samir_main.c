/*
 * os/samir/samir_main.c -- SAMIR (InitechBase) dot-prompt REPL.
 *                          Step S5.8 (Phase 5 convergence finale; initech-7az.9).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib). Uses
 * ONLY <stdint.h> and the samir/ engine headers. No libc, no int 0x21 -- every
 * byte of I/O is through the PAL vtable carried by the interpreter. Memory comes
 * from the PAL arena (no malloc). ASCII-clean (Rule 12). Fail loud (Rule 2).
 * Reproducible (Rule 11): pure function of (typed lines, interp state, the
 * injected clock).
 *
 * dBASE III PLUS 1.1 ONLY (docs/plans/SAMIR-implementation-plan.md Sec 2.C).
 *
 * WHAT THIS IS (the S5.8 contract; plan Sec 8.2 -- samir_repl is THE dot prompt):
 *   samir_repl(pal, ip) is the resident interpreter's read/eval/print loop. It
 *   registers the four S5.4..S5.7 command modules (query / mutate / set / proc)
 *   into the interp's command-hook chain, then loops:
 *       1. emit the dot prompt ("." then a space, dBASE-style);
 *       2. read one cooked line via pal->conin_line (EOF -> stop cleanly);
 *       3. trim it; an empty line just re-prompts;
 *       4. QUIT / EXIT typed at the dot prompt -> stop cleanly;
 *       5. USE / CLOSE -- handled HERE (no command module owns the work-area
 *          open/close verbs; the spine owns SELECT, so SELECT falls through);
 *       6. anything else -> samir_do(ip, line) (the S5.3 spine + the hook chain);
 *       7. on a run-time error, render the dBASE catalog message for
 *          samir_last_error(ip) as "<code>  <message>" and CONTINUE the loop --
 *          a bad line never aborts the session (real dot-prompt behavior).
 *   On exit (EOF / QUIT / EXIT) it closes every open work area and returns
 *   INTERP_OK.
 *
 * WHY USE/CLOSE ARE HANDLED HERE (not in a command module):
 *   The four registered modules (query/mutate/set/proc) do NOT own a USE or CLOSE
 *   verb, and the spine (flow.c) owns SELECT but not USE. A real dot prompt must
 *   open tables, so the REPL parses "USE <file> [ALIAS a] [INDEX i1,i2,...]" and
 *   "CLOSE [DATABASES|ALL]" itself and drives the work-area env (workarea.h
 *   wa_set_open_rw / wa_select / wa_close / wa_close_all). USE opens read-WRITE
 *   (wa_set_open_rw, 7az.16 / 7az.19) so the S5.5 mutation verbs (REPLACE /
 *   APPEND / DELETE / PACK / ZAP) and dbf_flush work on the table directly after
 *   a plain `USE <file>` -- this is the dBASE default (a plain USE opens for
 *   editing). The old wa_adopt_table seam for writable tables is no longer needed
 *   at the dot prompt; it remains available for tests/tooling that need to inject
 *   an already-created table. Ref: workarea.h wa_set_open_rw (7az.16 / 7az.19).
 *
 * ERROR RENDERING (the 151-code catalog):
 *   The catalog lives as spec/samir/dbase_msg_codes.tsv -- a factory artifact the
 *   freestanding engine cannot read at runtime. So the 1..151 messages are
 *   TRANSCRIBED here as a static ASCII table (provenance + the empty-slot notes
 *   are in the table comment). repl_render_error(pal, code) emits
 *   "<code>  <message>\n". Codes outside 1..151 (the engine's negative interp_err
 *   ordinals, surfaced positive by samir_last_error as small magnitudes that do
 *   NOT collide with the catalog) render a generic "<code>  Internal error."
 *
 * STANDALONE main():
 *   A main() exists ONLY behind #ifdef SAMIR_MAIN_STANDALONE so the engine stays
 *   freestanding-linkable and the oracle can drive samir_repl directly. The
 *   standalone main() is a thin host/artifact entry point (it needs a concrete
 *   PAL, supplied by whoever links it -- pal_host on the factory, pal_milton on
 *   Milton at S8.2); it is NOT compiled into the oracle or the freestanding check.
 *
 * Mutation hooks (Rule 6 -- the oracle's mutant siblings):
 *   -DREPL_MUTATE_NO_MUTATE_MODULE : skip registering the mutate module, so a
 *       REPLACE typed in the session fails (#16 unrecognized) -> the oracle's
 *       "REPLACE changed the field" assertions go RED (both the convergence
 *       session and the plain-USE-REPLACE-persists leg, initech-7az.19).
 *   -DREPL_MUTATE_ERR_RENDER : render code+1 instead of code, so the catalog
 *       message text for a forced error no longer matches -> the oracle's
 *       error-text assertion goes RED.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.8 ("read line via PAL, parse,
 *     execute, render via the 151-code error catalog") + Sec 8.2 (samir_repl).
 *   - os/samir/include/samir/interp.h (samir_repl decl; samir_do; the hook chain;
 *     samir_last_error).
 *   - os/samir/include/samir/pal.h (conin_line / conout; the ONLY OS surface).
 *   - os/samir/include/samir/workarea.h (wa_set_open / wa_select / wa_close /
 *     wa_close_all -- the USE/CLOSE engine).
 *   - spec/samir/dbase_msg_codes.tsv (the 151-code catalog transcribed below).
 */

#include <stdint.h>

#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/value.h"
#include "samir/eval.h"
#include "samir/rt.h"
#include "samir/pal.h"

/* ---------------------------------------------------------------------------
 * The four S5.4..S5.7 command modules. Their register functions are defined in
 * their own translation units (cmd/query.c, cmd/mutate.c, cmd/set.c, cmd/proc.c)
 * but there is no shared commands.h yet, so they are extern-declared here with
 * matching signatures. FOLLOW-UP (below): consolidate into samir/commands.h.
 * ------------------------------------------------------------------------- */
extern int query_register(xb_interp *ip);
extern int mutate_register(xb_interp *ip);
extern int set_register(xb_interp *ip);
extern int proc_register(xb_interp *ip);

/*
 * proc_run(ip, line) is the proc module's program driver: it registers `line`
 * as the source (so a DO <name> in the same line finds a PROCEDURE defined in
 * it), runs the main body via samir_do, maps a top-level RETURN to a clean exit,
 * and FIRES an installed ON ERROR trap once on a runtime error. The REPL uses it
 * as the per-line executor (instead of bare samir_do) so DO/PROCEDURE and
 * ON ERROR work at the dot prompt; for a plain single-statement line it is
 * exactly samir_do of that line. Defined in cmd/proc.c (extern-declared here for
 * the same reason as the register fns -- no shared commands.h yet).
 */
extern int proc_run(xb_interp *ip, const char *prg);

/* ===================================================================== */
/* The 151-code dBASE III PLUS 1.1 error catalog.                          */
/*                                                                         */
/* TRANSCRIBED from spec/samir/dbase_msg_codes.tsv (mined from DBASE.MSG;  */
/* 151 codes, 1-based ordinal). Codes 32, 40, 69, 135, 151 have empty      */
/* messages in the source and are preserved empty here. The freestanding   */
/* engine cannot read the .tsv at runtime, so the strings live inline.     */
/* ASCII-clean (Rule 12). g_msg[0] is unused (ordinals are 1-based).       */
/* ===================================================================== */

#define REPL_MSG_COUNT 151

static const char *const g_msg[REPL_MSG_COUNT + 1] = {
    /*  0 */ "",
    /*  1 */ "File does not exist.",
    /*  2 */ "Unassigned file no.",
    /*  3 */ "File is already open.",
    /*  4 */ "End of file encountered.",
    /*  5 */ "Record is out of range.",
    /*  6 */ "Too many files are open.",
    /*  7 */ "File already exists.",
    /*  8 */ "Unbalanced parenthesis.",
    /*  9 */ "Data type mismatch.",
    /* 10 */ "Syntax error.",
    /* 11 */ "Invalid function argument.",
    /* 12 */ "Variable not found.",
    /* 13 */ "ALIAS not found.",
    /* 14 */ "No find.",
    /* 15 */ "Not a dBASE database.",
    /* 16 */ "*** Unrecognized command verb.",
    /* 17 */ "Cannot select requested database.",
    /* 18 */ "Line exceeds maximum of 254 characters.",
    /* 19 */ "Index file does not match database.",
    /* 20 */ "Record is not in index.",
    /* 21 */ "Out of memory variable memory.",
    /* 22 */ "Out of memory variable slots.",
    /* 23 */ "Index is too big (100 char maximum).",
    /* 24 */ "ALIAS name already in use.",
    /* 25 */ "Record is not inserted.",
    /* 26 */ "Database is not indexed.",
    /* 27 */ "Not a numeric expression.",
    /* 28 */ "Too many indices.",
    /* 29 */ "File is not accessible.",
    /* 30 */ "Position is off the screen.",
    /* 31 */ "Invalid function name.",
    /* 32 */ "",
    /* 33 */ "Structure invalid.",
    /* 34 */ "Operation with Memo field invalid.",
    /* 35 */ "Unterminated string.",
    /* 36 */ "Unrecognized phrase/keyword in command.",
    /* 37 */ "Not a Logical expression.",
    /* 38 */ "Beginning of file encountered.",
    /* 39 */ "Numeric overflow (data was lost).",
    /* 40 */ "",
    /* 41 */ ".DBT file cannot be opened.",
    /* 42 */ "CONTINUE without LOCATE.",
    /* 43 */ "Insufficient memory.",
    /* 44 */ "Cyclic relation.",
    /* 45 */ "Not a Character expression.",
    /* 46 */ "Illegal value.",
    /* 47 */ "No fields to process.",
    /* 48 */ "Field not found.",
    /* 49 */ "File has been deleted.",
    /* 50 */ "Report file invalid.",
    /* 51 */ "End of file or error on keyboard input.",
    /* 52 */ "No database is in USE.",
    /* 53 */ "There are no files of the type requested in this drive or catalog.",
    /* 54 */ "Label file invalid.",
    /* 55 */ "Memory Variable file is invalid.",
    /* 56 */ "Disk full when writing file:",
    /* 57 */ "CHR() : Out of range.",
    /* 58 */ "LOG() : Zero or negative.",
    /* 59 */ "SPACE() : Too large.",
    /* 60 */ "SPACE() : Negative.",
    /* 61 */ "SQRT() : Negative.",
    /* 62 */ "SUBSTR() : Start point out of range.",
    /* 63 */ "STR() : Out of range.",
    /* 64 */ "Internal error:",
    /* 65 */ "Unknown command code:",
    /* 66 */ "CMDSET():",
    /* 67 */ "EVAL work area overflow",
    /* 68 */ "Illegal opcode",
    /* 69 */ "",
    /* 70 */ "** WARNING ** Data will probably be lost.  Confirm? (Y/N)",
    /* 71 */ ", Database in Use:",
    /* 72 */ "could not be opened.",
    /* 73 */ "^^ Expected ON or OFF.",
    /* 74 */ "^--- Truncated.",
    /* 75 */ "^--- Out of range.",
    /* 76 */ "- : Concatenated string too large.",
    /* 77 */ "+ : Concatenated string too large.",
    /* 78 */ "^ or ** : Negative base, fractional exponent.",
    /* 79 */ "STORE : String too large.",
    /* 80 */ "***Execution error on",
    /* 81 */ "Invalid date.",
    /* 82 */ "** Not Found **",
    /* 83 */ "is not a dBASE command.",
    /* 84 */ "Unlink of old name incomplete, errno =",
    /* 85 */ "Error:",
    /* 86 */ "^--- Keyword not found.",
    /* 87 */ "NDX(): Invalid index number.",
    /* 88 */ "REPLICATE(): String too large.",
    /* 89 */ "Cannot erase a file which is open.",
    /* 90 */ "Operation with Logical field invalid.",
    /* 91 */ "File was not LOADed.",
    /* 92 */ "Unable to load COMMAND.COM.",
    /* 93 */ "No PARAMETER statement found.",
    /* 94 */ "Wrong number of parameters.",
    /* 95 */ "Valid only in programs.",
    /* 96 */ "Mismatched DO WHILE and ENDDO.",
    /* 97 */ "Bad read or illegal printer file.",
    /* 98 */ "Not a RunTime file.",
    /* 99 */ "Invalid DOS SET option.",
    /* 100 */ "Lock failed, but not because of previous lock.",
    /* 101 */ "Not suspended.",
    /* 102 */ "STUFF():  String too large.",
    /* 103 */ "DOs nested too deep.",
    /* 104 */ "Unknown function key.",
    /* 105 */ "Table is full.",
    /* 106 */ "Invalid index number.",
    /* 107 */ "Invalid operator.",
    /* 108 */ "File is in use by another.",
    /* 109 */ "Record is in use by another.",
    /* 110 */ "Exclusive open of file is required.",
    /* 111 */ "Cannot write to a read-only file.",
    /* 112 */ "Index expression is too big (220 char maximum).",
    /* 113 */ "Index interrupted.  Index will be deleted if not completed.",
    /* 114 */ "Index damaged.  REINDEX should be done before using data.",
    /* 115 */ "Invalid DIF File Header.",
    /* 116 */ "Invalid DIF vector - DBF field mismatch.",
    /* 117 */ "Invalid DIF type indicator.",
    /* 118 */ "Invalid DIF character.",
    /* 119 */ "Invalid SYLK file header.",
    /* 120 */ "Invalid SYLK file dimension bounds.",
    /* 121 */ "Invalid SYLK file format.",
    /* 122 */ "Data Catalog has not been established.",
    /* 123 */ "Invalid printer port.",
    /* 124 */ "Invalid printer redirection.",
    /* 125 */ "Printer not ready.",
    /* 126 */ "Printer is either not connected or turned off.",
    /* 127 */ "Not a valid VIEW file.",
    /* 128 */ "Unable to SKIP.",
    /* 129 */ "Unable to LOCK.",
    /* 130 */ "Record is not locked.",
    /* 131 */ "Database is encrypted.",
    /* 132 */ "Unauthorized login.",
    /* 133 */ "Unauthorized access level.",
    /* 134 */ "Not a valid QUERY file.",
    /* 135 */ "",
    /* 136 */ "Unsupported path given.",
    /* 137 */ "Maximum record length exceeded.",
    /* 138 */ "No fields were found to copy.",
    /* 139 */ "Cannot JOIN a file with itself.",
    /* 140 */ "Not a valid PFS file.",
    /* 141 */ "Fields list too complicated.",
    /* 142 */ "Relation record is in use by others.",
    /* 143 */ "Query not valid for this environment.",
    /* 144 */ "Unauthorized duplicate.",
    /* 145 */ "Error in configuration value.",
    /* 146 */ "Maximum path length exceeded.",
    /* 147 */ "Cannot append in column order.",
    /* 148 */ "Network server busy.",
    /* 149 */ "Master catalog is empty.",
    /* 150 */ "Help text not found.",
    /* 151 */ ""
};

/* ===================================================================== */
/* Small freestanding helpers (no libc).                                   */
/* ===================================================================== */

/* fold one byte to upper-case (ASCII letters only). */
static char repl_up1(char c)
{
    if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
    return c;
}

static int repl_isspace(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* write a NUL-terminated C string to the console. */
static void repl_puts(samir_pal_t *pal, const char *s)
{
    pal->conout(pal, s, rt_strlen(s));
}

/*
 * repl_uitoa: format a non-negative integer into `buf` (NUL-terminated),
 * returning the byte length. Freestanding (no libc snprintf).
 */
static uint32_t repl_uitoa(uint32_t v, char *buf)
{
    char tmp[12];
    uint32_t n = 0, i;
    if (v == 0u) { buf[0] = '0'; buf[1] = '\0'; return 1u; }
    while (v > 0u) { tmp[n++] = (char)('0' + (v % 10u)); v /= 10u; }
    for (i = 0; i < n; i++) buf[i] = tmp[n - 1u - i];
    buf[n] = '\0';
    return n;
}

/*
 * repl_render_error: emit "<code>  <message>" + newline for a dBASE catalog
 * ordinal (1..151). An out-of-range code (the engine's small interp_err
 * ordinals, or 0) renders a generic line so the loop never silently swallows a
 * failure (Rule 2). The -DREPL_MUTATE_ERR_RENDER mutant perturbs the looked-up
 * code by +1 so the message text no longer matches (the oracle bites).
 */
static void repl_render_error(samir_pal_t *pal, int code)
{
    char num[12];
    int lookup = code;

#ifdef REPL_MUTATE_ERR_RENDER
    lookup = code + 1;   /* MUTANT: wrong catalog code -> wrong message text */
#endif

    repl_uitoa((uint32_t)(code < 0 ? -code : code), num);
    repl_puts(pal, num);
    repl_puts(pal, "  ");
    if (lookup >= 1 && lookup <= REPL_MSG_COUNT && g_msg[lookup][0] != '\0')
        repl_puts(pal, g_msg[lookup]);
    else
        repl_puts(pal, "Internal error.");
    repl_puts(pal, "\n");
}

/* ===================================================================== */
/* Line trimming + verb splitting.                                         */
/* ===================================================================== */

/*
 * repl_trim: in place, strip leading + trailing whitespace from a NUL-terminated
 * line; returns a pointer to the first non-blank byte (still inside `line`).
 */
static char *repl_trim(char *line)
{
    char *s = line;
    uint32_t n;
    while (*s != '\0' && repl_isspace(*s)) s++;
    n = rt_strlen(s);
    while (n > 0u && repl_isspace(s[n - 1u])) { s[n - 1u] = '\0'; n--; }
    return s;
}

/*
 * repl_verb_eq: does the leading word of `line` equal the upper-cased `kw`
 * (case-insensitive)? The word ends at the first whitespace or NUL. `*rest`
 * (may be NULL) receives the trimmed remainder after the word.
 */
static int repl_verb_eq(const char *line, const char *kw, const char **rest)
{
    uint32_t i = 0;
    while (line[i] != '\0' && !repl_isspace(line[i])) {
        if (kw[i] == '\0') break;                       /* line word is longer */
        if (repl_up1(line[i]) != kw[i]) break;          /* mismatch */
        i++;
    }
    if (kw[i] != '\0') return 0;                        /* kw not consumed */
    if (line[i] != '\0' && !repl_isspace(line[i])) return 0; /* line word longer */
    if (rest) {
        while (line[i] != '\0' && repl_isspace(line[i])) i++;
        *rest = line + i;
    }
    return 1;
}

/* ===================================================================== */
/* USE / CLOSE -- the REPL-owned work-area verbs.                          */
/* ===================================================================== */

#define REPL_TOK_CAP   128   /* one parsed file/alias token */

/*
 * Copy the next blank/comma-delimited word from `*p` into `tok` (NUL-terminated,
 * capped). Advances `*p` past the word + any trailing separators. Returns the
 * word length (0 = nothing left).
 */
static uint32_t repl_word(const char **p, char *tok, uint32_t cap)
{
    const char *q = *p;
    uint32_t n = 0;
    while (*q != '\0' && (repl_isspace(*q) || *q == ',')) q++;
    while (*q != '\0' && !repl_isspace(*q) && *q != ',') {
        if (n < cap - 1u) tok[n++] = *q;
        q++;
    }
    tok[n] = '\0';
    *p = q;
    return n;
}

/*
 * repl_do_use: parse + execute "USE <file> [ALIAS <a>] [INDEX <i1>,<i2>,...]"
 * into the SELECTED work area. Returns INTERP_OK, or a negative interp_err with
 * *ec set to the dBASE catalog ordinal to render.
 *
 * Bare "USE" (no file) closes the selected area (dBASE: USE with no operand
 * closes the current database). USE re-uses the SELECTED area; the area is
 * closed first (dBASE USE replaces the open table in the current area).
 */
static int repl_do_use(xb_interp *ip, const char *args, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    char file[REPL_TOK_CAP];
    char alias[REPL_TOK_CAP];
    static char idxbuf[NDX_PER_AREA][REPL_TOK_CAP];
    wa_index_list il;
    const char *have_alias = (const char *)0;
    int area;
    int rc;
    const char *p = args;

    if (ec) *ec = 0;
    area = wa_selected(env);
    if (area < 1) { if (ec) *ec = 17; return -INTERP_ERR_SYNTAX; }

    /* bare USE -> close the current area. */
    if (repl_word(&p, file, sizeof file) == 0u) {
        wa_close(env, area);
        return INTERP_OK;
    }

    il.count = 0;
    for (;;) {
        char kw[REPL_TOK_CAP];
        const char *save = p;
        if (repl_word(&p, kw, sizeof kw) == 0u) break;
        if (repl_verb_eq(kw, "ALIAS", (const char **)0)) {
            if (repl_word(&p, alias, sizeof alias) == 0u) {
                if (ec) *ec = 10;            /* Syntax error */
                return -INTERP_ERR_SYNTAX;
            }
            have_alias = alias;
        } else if (repl_verb_eq(kw, "INDEX", (const char **)0)) {
            char one[REPL_TOK_CAP];
            while (repl_word(&p, one, sizeof one) != 0u) {
                if (il.count >= NDX_PER_AREA) {
                    if (ec) *ec = 28;        /* Too many indices */
                    return -INTERP_ERR_SYNTAX;
                }
                rt_memcpy(idxbuf[il.count], one, rt_strlen(one) + 1u);
                il.names[il.count] = idxbuf[il.count];
                il.count++;
            }
        } else {
            /* an unrecognized USE clause word -- fail loud (Rule 2). */
            (void)save;
            if (ec) *ec = 36;                /* Unrecognized phrase/keyword */
            return -INTERP_ERR_SYNTAX;
        }
    }

    /* dBASE USE replaces the table in the current area: close it first. */
    wa_close(env, area);
    /* Ref: PRD S5.8; workarea.h wa_set_open_rw (7az.16 / 7az.19).
     * Open read-WRITE: dBASE USE opens the table for editing by default, so
     * REPLACE/APPEND/DELETE persist without the dbf_create + wa_adopt_table seam.
     * wa_set_open (read-only) is kept for tests/tooling that must NOT write to a
     * corpus golden; the dot prompt always opens rw. */
    rc = wa_set_open_rw(env, area, file, have_alias,
                        il.count > 0 ? &il : (const wa_index_list *)0);
    if (rc != WA_OK) {
        /* file open / codec faults: a missing .dbf is #1; anything else #15
         * "Not a dBASE database." The engine returns negated wa/codec codes;
         * we map to the user-facing catalog ordinal. */
        if (ec) *ec = (rc == -WA_ERR_IO) ? 1 : 15;
        return -INTERP_ERR_EVAL;
    }
    wa_select(env, area);                     /* USE selects the area it opens */
    return INTERP_OK;
}

/*
 * repl_do_close: "CLOSE [DATABASES|ALL|...]" -> close every open area. dBASE has
 * finer CLOSE forms (INDEXES/FORMAT/...), but the REPL's table-centric subset
 * closes all databases for any CLOSE word (and bare CLOSE). Always INTERP_OK.
 */
static int repl_do_close(xb_interp *ip, const char *args)
{
    (void)args;
    wa_close_all(xb_interp_env(ip));
    return INTERP_OK;
}

/* ===================================================================== */
/* DO <file> -- run a .PRG program off disk from the dot prompt.           */
/*                                                                         */
/* dBASE III PLUS 1.1: `DO <name>` at the dot prompt runs <name>.PRG from   */
/* disk when <name> is NOT a PROCEDURE defined in the (already-registered)  */
/* program source (control-flow-and-procedures.md sec 7; "DO-name           */
/* precedence" -- open PROCEDURE vs disk <name>.prg). At the dot prompt no   */
/* multi-PROCEDURE program is registered (each typed line is its own         */
/* proc_run source), so a bare `DO Y2KACCT` resolves to Y2KACCT.PRG on the   */
/* data disk: the canonical "run my program" verb. This is the keystone for  */
/* initech-9a0f -- the Initech AR aging app (Y2KACCT.PRG, the enforced Y2K   */
/* bug, bead 586.1) RUNS inside InitechOS by typing `DO Y2KACCT`.            */
/*                                                                          */
/* ADDITIVE (does NOT touch cmd/proc.c's in-source DO): this is a REPL-level */
/* pre-resolution. Only a BARE `DO <name>` line (one word after DO, not      */
/* "DO WHILE"/"DO CASE" -- those are flow-spine and never reach the REPL as  */
/* the leading verb of a typed line; and not "DO <name> WITH ..." which is   */
/* an in-source procedure call) is offered to disk first. If the file is NOT */
/* on disk, we FALL THROUGH to the normal proc_run path (an in-source        */
/* PROCEDURE, else dBASE #16) -- so every existing REPL/host gate is          */
/* unchanged. ASCII-clean (Rule 12); fail loud on a too-large .prg (Rule 2). */
/* ===================================================================== */

#define REPL_PRG_CAP   8192u   /* max .PRG size we load (proc body cap mirror) */

/*
 * repl_load_prg: open `name` (then `name` + ".PRG") via the PAL, read the whole
 * file into a PAL-arena buffer, NUL-terminate it. On success returns the buffer
 * (arena-owned; the interp's xb_interp_free reset reclaims it) and sets *out_len;
 * returns NULL if the file does not exist on disk (so the caller falls through to
 * the in-source DO path) or on a read fault / over-cap (fail loud via *ec).
 *
 * `name` is the bare target token (already upper-cased by the caller is fine --
 * the FAT layer is case-insensitive). We try the name as typed, then with a
 * ".PRG" extension appended (the dBASE default program extension).
 */
static char *repl_load_prg(xb_interp *ip, const char *name, uint32_t *out_len,
                           int *ec)
{
    samir_pal_t *pal = xb_interp_pal(ip);
    char fn[REPL_TOK_CAP + 4];
    pal_fd fd;
    int32_t total;
    char *buf;
    uint32_t cap, used;
    int try2;

    if (out_len) *out_len = 0u;
    if (ec) *ec = 0;
    if (!pal || !pal->open || !pal->read || !pal->close) return (char *)0;

    /* Two open attempts: the bare name, then name + ".PRG". A negative fd on
     * BOTH means "no such program on disk" -> caller falls through (NULL, ec=0).
     * The DO target is opened READ-ONLY (a program is executed, never written). */
    fd = -1;
    for (try2 = 0; try2 < 2 && fd < 0; try2++) {
        uint32_t i = 0;
        while (name[i] != '\0' && i < REPL_TOK_CAP - 1u) { fn[i] = name[i]; i++; }
        if (try2 == 1) {
            /* append ".PRG" (only when the bare open missed). */
            fn[i++] = '.'; fn[i++] = 'P'; fn[i++] = 'R'; fn[i++] = 'G';
        }
        fn[i] = '\0';
        fd = pal->open(pal, fn, PAL_RD);
    }
    if (fd < 0) return (char *)0;     /* not on disk -> fall through (in-source DO) */

    /* Determine the size via the seek=END idiom, then rewind. A program over the
     * cap is a fail-loud (Rule 2) -- we never silently truncate the .prg, which
     * would run a partial program (the DO-read mutant exploits exactly this). */
    total = pal->seek ? pal->seek(pal, fd, 0, PAL_SEEK_END) : -1;
    if (total < 0 || (uint32_t)total >= REPL_PRG_CAP) {
        pal->close(pal, fd);
        if (ec) *ec = 18;             /* "Line exceeds maximum ..." (closest cat) */
        return (char *)0;
    }
    (void)pal->seek(pal, fd, 0, PAL_SEEK_SET);

    cap = (uint32_t)total + 1u;
    buf = (char *)(pal->alloc ? pal->alloc(pal, cap) : (void *)0);
    if (!buf) { pal->close(pal, fd); if (ec) *ec = 43; return (char *)0; } /* #43 mem */

#ifdef REPL_MUTATE_DO_TRUNC
    /* MUTANT (Rule 6 -- -DREPL_MUTATE_DO_TRUNC): read only the FIRST HALF of the
     * .prg, then claim success. The aging-report body (the DO WHILE record walk +
     * the TOTAL line) is cut off, so the in-emu report is wrong/incomplete -> the
     * test-samir-canon-y2k assertions (the buggy A1001 -36477 line + the 0.00
     * total) go RED for the RIGHT reason (a truncated DO-file read, not a crash).
     * NEVER define in a real build. */
    total = (int32_t)((uint32_t)total / 2u);
#endif

    /* Read the WHOLE file (loop until EOF -- the PAL read may be short). */
    used = 0u;
    while (used < (uint32_t)total) {
        int32_t n = pal->read(pal, fd, buf + used, (uint32_t)total - used);
        if (n < 0) { pal->close(pal, fd); if (ec) *ec = 1; return (char *)0; }
        if (n == 0) break;            /* EOF */
        used += (uint32_t)n;
    }
    pal->close(pal, fd);
    buf[used] = '\0';
    if (out_len) *out_len = used;
    return buf;
}

/*
 * repl_try_do_file: if `s` is a bare "DO <name>" (no WITH, single target word),
 * try to load <name>[.PRG] off disk and run it via proc_run. Returns:
 *   1  -- handled here (the .prg was found + run); *rc holds proc_run's result.
 *   0  -- NOT a disk-program DO (no file on disk, or the line was DO ... WITH /
 *         DO WHILE-shaped): the caller runs the normal proc_run(ip, s) path.
 * *ec receives the catalog ordinal on a fail-loud load error (over-cap / I/O).
 */
static int repl_try_do_file(xb_interp *ip, const char *s, int *rc, int *ec)
{
    const char *rest = (const char *)0;
    char name[REPL_TOK_CAP];
    const char *p;
    uint32_t prglen = 0u;
    char *prg;

    if (ec) *ec = 0;
    if (!repl_verb_eq(s, "DO", &rest) || !rest) return 0;

    /* Take the single target word. If anything non-blank follows it (e.g. WITH
     * <args>), this is an in-source procedure call -- leave it to proc_run. */
    p = rest;
    if (repl_word(&p, name, sizeof name) == 0u) return 0;     /* bare "DO" */
    while (*p != '\0' && repl_isspace(*p)) p++;
    if (*p != '\0') return 0;             /* DO <name> WITH ... -> in-source DO */

    prg = repl_load_prg(ip, name, &prglen, ec);
    if (!prg) {
        /* ec!=0 -> a real load fault (surface it); ec==0 -> not on disk (fall
         * through so an in-source PROCEDURE of that name still runs). */
        return (ec && *ec != 0) ? 1 : 0;
    }

    *rc = proc_run(ip, prg);
    return 1;
}

/* ===================================================================== */
/* Module registration.                                                    */
/* ===================================================================== */

/*
 * repl_register_modules: install the four S5.4..S5.7 command modules into the
 * interp's hook chain, in dispatch order query -> mutate -> set -> proc. Returns
 * INTERP_OK if all registered, or the first negative interp_err.
 *
 * The -DREPL_MUTATE_NO_MUTATE_MODULE mutant SKIPS the mutate module, so a typed
 * REPLACE is then unrecognized (#16) -> the oracle bites.
 */
static int repl_register_modules(xb_interp *ip)
{
    int rc;

    rc = query_register(ip);
    if (rc != INTERP_OK) return rc;

#ifndef REPL_MUTATE_NO_MUTATE_MODULE
    rc = mutate_register(ip);
    if (rc != INTERP_OK) return rc;
#endif

    rc = set_register(ip);
    if (rc != INTERP_OK) return rc;

    rc = proc_register(ip);
    if (rc != INTERP_OK) return rc;

    return INTERP_OK;
}

/* ===================================================================== */
/* The REPL loop.                                                          */
/* ===================================================================== */

#define REPL_LINE_CAP 256   /* dBASE line cap is 254; +slack for NUL */

/*
 * samir_repl: the dot-prompt read/eval/print loop. See the file header for the
 * full contract. Returns INTERP_OK on a clean exit (EOF / QUIT / EXIT), or a
 * negative interp_err if module registration failed before the loop began.
 */
int samir_repl(samir_pal_t *pal, xb_interp *ip)
{
    char line[REPL_LINE_CAP];
    int rc;

    if (!pal || !ip)
        return -INTERP_ERR_NOMEM;

    rc = repl_register_modules(ip);
    if (rc != INTERP_OK)
        return rc;

    for (;;) {
        const char *rest = (const char *)0;
        char *s;
        int32_t n;

        repl_puts(pal, ". ");                 /* the dBASE dot prompt */

        n = pal->conin_line(pal, line, (uint32_t)REPL_LINE_CAP);
        if (n < 0) break;                     /* EOF / keyboard error -> stop */
        line[(uint32_t)n < (uint32_t)REPL_LINE_CAP ? (uint32_t)n : REPL_LINE_CAP - 1u] = '\0';

        s = repl_trim(line);
        if (s[0] == '\0') continue;           /* blank line -> re-prompt */

        /* (1) REPL-terminating verbs. */
        if (repl_verb_eq(s, "QUIT", (const char **)0) ||
            repl_verb_eq(s, "EXIT", (const char **)0))
            break;

        /* (2) work-area verbs the REPL owns (no module / spine owns USE/CLOSE). */
        if (repl_verb_eq(s, "USE", &rest)) {
            int ec = 0;
            rc = repl_do_use(ip, rest, &ec);
            if (rc != INTERP_OK) repl_render_error(pal, ec ? ec : 16);
            continue;
        }
        if (repl_verb_eq(s, "CLOSE", &rest)) {
            (void)repl_do_close(ip, rest);
            continue;
        }

        /* (3) DO <name> off disk: a bare "DO <name>" at the dot prompt runs
         * <name>.PRG from the data disk (dBASE: DO a disk program). Tried BEFORE
         * the generic proc_run so the .prg is loaded + executed; if no such file
         * exists we fall through (an in-source PROCEDURE of that name, else #16).
         * This is the initech-9a0f keystone: `DO Y2KACCT` runs the Initech AR
         * aging app (with its enforced Y2K bug, bead 586.1) INSIDE InitechOS. */
        {
            int dofile_ec = 0;
            int dofile_rc = INTERP_OK;
            if (repl_try_do_file(ip, s, &dofile_rc, &dofile_ec)) {
                if (dofile_ec != 0) {
                    repl_render_error(pal, dofile_ec);   /* a load fault (#18/#43/#1) */
                } else if (dofile_rc != INTERP_OK) {
                    int code = samir_last_error(ip);
                    repl_render_error(pal, code != 0 ? code : 16);
                }
                continue;                                /* the .prg was handled */
            }
        }

        /* (4) everything else -> the proc module's program driver, which wraps
         * samir_do (the S5.3 spine + the registered hook chain) so DO/PROCEDURE
         * and ON ERROR work at the dot prompt. proc_run sets samir_last_error and
         * fires an installed ON ERROR trap once on a runtime error. */
        rc = proc_run(ip, s);
        if (rc != INTERP_OK) {
            int code = samir_last_error(ip);
            repl_render_error(pal, code != 0 ? code : 16);
            /* continue the loop -- a bad line never aborts the session. */
        }
    }

    wa_close_all(xb_interp_env(ip));
    return INTERP_OK;
}

/* ===================================================================== */
/* Standalone entry point (GUARDED -- engine stays freestanding-linkable). */
/* ===================================================================== */

#ifdef SAMIR_MAIN_STANDALONE
/*
 * A thin entry point for a linked SAMIR binary. It needs a concrete PAL, which
 * the linker supplies: pal_host on the factory, pal_milton on Milton (S8.2). To
 * keep this file free of any one PAL dependency, the integrator provides
 * samir_main_make_pal()/_free() at link time. The freestanding engine + the
 * oracle do NOT compile this block.
 */
extern samir_pal_t *samir_main_make_pal(void);
extern void         samir_main_free_pal(samir_pal_t *pal);

int main(void)
{
    samir_pal_t *pal = samir_main_make_pal();
    xb_interp *ip;
    int rc;

    if (!pal) return 2;
    ip = xb_interp_make(pal);
    if (!ip) { samir_main_free_pal(pal); return 3; }

    rc = samir_repl(pal, ip);

    xb_interp_free(ip);
    samir_main_free_pal(pal);
    return rc == INTERP_OK ? 0 : 1;
}
#endif /* SAMIR_MAIN_STANDALONE */
