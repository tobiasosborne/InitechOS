/* command.c -- InitechDOS COMMAND.COM-alike interactive shell (the A:\> REPL).
 *
 * beads: initech-7pc. M2 capstone (PRD Sec 6.1). See command.h for the full
 *        citation block + the host-testability seam rationale.
 * Ref:   ADR-0003 DEC-11/DEC-12 + Appendix D; DOS 3.3 COMMAND.COM behaviour;
 *        spec/dos_messages.json (MSG-DOS-0002/0003); spec/find_data.h (the
 *        43-byte DTA find-record); spec/int21h_calling_convention.json (DEC-04a
 *        flat ABI: AH=func, EBX=handle, ECX=count, EDX=flat ptr, EAX=ret, CF).
 *        CLAUDE.md Law 1/2/3/4, Rule 2 (fail loud), Rule 11, Rule 12.
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. The
 * pure logic (cmd_*) carries NO asm + NO I/O so the SAME TU compiles HOSTED for
 * os/milton/test_command.c. The REPL + its `int $0x21` wrappers are kernel-only,
 * guarded by COMMAND_KERNEL_REPL so the host build never sees inline asm.
 */

#include "command.h"
#include "dos_structs.h"   /* DIR_ATTR_DIRECTORY -- the DIR formatter's attr bit */
#include "dos_messages.h"  /* MSG_DOS_* controlled diagnostics (DEC-13; -Ibuild) */
#include "env.h"           /* env_store_t, env_init/set/get/unset/count/entry    */

/* ---- PURE shell logic (host-testable; the test_command.c oracle drives these) */

char cmd_upcase_char(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

void cmd_upcase_str(char *s)
{
    if (s == 0) {
        return;
    }
    for (; *s; s++) {
        *s = cmd_upcase_char(*s);
    }
}

static int is_space(char c)
{
    return c == ' ' || c == '\t';
}

cmd_kind_t cmd_classify(const char *upper_command)
{
    if (upper_command == 0 || upper_command[0] == '\0') {
        return CMD_EMPTY;
    }
    /* A tiny hand-rolled strcmp keeps this freestanding (no libc). */
    struct { const char *name; cmd_kind_t kind; } table[] = {
        { "DIR",   CMD_DIR  },
        { "TYPE",  CMD_TYPE },
        { "CD",    CMD_CD   },
        { "CHDIR", CMD_CD   },   /* CHDIR is the long form of CD (DOS) */
#ifndef CMD_MUTATE_NO_MDRD
        /* MD/RD + their long forms map to the directory built-ins (beads
         * initech-ut6d). The CMD_MUTATE_NO_MDRD mutant DROPS these four rows so
         * "MD"/"RD" fall through to CMD_EXTERNAL -> the classify oracle goes RED
         * (Rule 6). NEVER dropped in a real build. */
        { "MD",    CMD_MD   },
        { "MKDIR", CMD_MD   },   /* MKDIR is the long form of MD (DOS) */
        { "RD",    CMD_RD   },
        { "RMDIR", CMD_RD   },   /* RMDIR is the long form of RD (DOS) */
#endif
        { "CLS",   CMD_CLS  },
        { "VER",   CMD_VER  },
        { "ECHO",  CMD_ECHO },
        { "BREAK", CMD_BREAK },  /* BREAK [ON|OFF] -- CTRL-BREAK state (AH=33h; DEC-16) */
        { "EXIT",  CMD_EXIT },
#ifndef CMD_MUTATE_NO_SET
        /* SET [NAME[=VALUE]] -- list/assign/clear/query env vars (beads
         * initech-1i0x Tranche E inc 2). The CMD_MUTATE_NO_SET mutant DROPS
         * this row so "SET" falls through to CMD_EXTERNAL -> the classify
         * oracle goes RED (Rule 6). NEVER dropped in a real build. */
        { "SET",    CMD_SET    },
#endif
#ifndef CMD_MUTATE_NO_PROMPT
        /* PROMPT [template] -- set/reset the $P$G prompt string (beads
         * initech-dibc; ADR-0003 DEC-12). The CMD_MUTATE_NO_PROMPT mutant
         * DROPS this row so "PROMPT" falls through to CMD_EXTERNAL -> the
         * classify oracle goes RED (Rule 6). NEVER dropped in a real build. */
        { "PROMPT", CMD_PROMPT },
#endif
#ifndef CMD_MUTATE_NO_COPY
        /* COPY <src> <dst> -- single-file copy via 3Dh/3Ch/3Fh/40h/3Eh (beads
         * initech-hpls, Tranche F). The CMD_MUTATE_NO_COPY mutant DROPS this row
         * so "COPY" falls through to CMD_EXTERNAL -> the classify oracle goes RED
         * (Rule 6). NEVER dropped in a real build. */
        { "COPY",  CMD_COPY },
#endif
#ifndef CMD_MUTATE_NO_DEL
        /* DEL/ERASE <name> -- delete file(s) via 41h (wildcard: 4Eh/4Fh loop)
         * (beads initech-hpls, Tranche F). The CMD_MUTATE_NO_DEL mutant DROPS
         * BOTH rows so "DEL"/"ERASE" fall through to CMD_EXTERNAL -> the classify
         * oracle goes RED (Rule 6). NEVER dropped in a real build. */
        { "DEL",   CMD_DEL  },
        { "ERASE", CMD_DEL  },   /* ERASE is the long form of DEL (DOS) */
#endif
#ifndef CMD_MUTATE_NO_REN
        /* REN/RENAME <old> <new> -- same-dir dir-entry rename via 56h (beads
         * initech-fyox, Tranche F). The CMD_MUTATE_NO_REN mutant DROPS BOTH rows
         * so "REN"/"RENAME" fall through to CMD_EXTERNAL -> the classify oracle
         * goes RED (Rule 6). NEVER dropped in a real build. */
        { "REN",    CMD_REN },
        { "RENAME", CMD_REN },   /* RENAME is the long form of REN (DOS) */
#endif
#ifndef CMD_MUTATE_NO_DATE
        /* DATE [MM-DD-YY[YY]] -- show/set the system date via 2Ah/2Bh (beads
         * initech-uy4l, Tranche F). The CMD_MUTATE_NO_DATE mutant DROPS this row
         * so "DATE" falls through to CMD_EXTERNAL -> the classify oracle goes RED
         * (Rule 6). NEVER dropped in a real build. */
        { "DATE",  CMD_DATE },
#endif
#ifndef CMD_MUTATE_NO_TIME
        /* TIME [HH:MM[:SS]] -- show/set the system time via 2Ch/2Dh (beads
         * initech-uy4l, Tranche F). The CMD_MUTATE_NO_TIME mutant DROPS this row
         * so "TIME" falls through to CMD_EXTERNAL -> the classify oracle goes RED
         * (Rule 6). NEVER dropped in a real build. */
        { "TIME",  CMD_TIME },
#endif
    };
    for (unsigned i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        const char *a = upper_command;
        const char *b = table[i].name;
        while (*a && *b && *a == *b) {
            a++; b++;
        }
        if (*a == '\0' && *b == '\0') {
            return table[i].kind;
        }
    }
#ifdef CMD_MUTATE_BADCMD_BUILTIN
    /* MUTANT (Rule 6; make test-command-mutant only): treat an unknown command
     * word as a built-in (CMD_DIR) instead of CMD_EXTERNAL, so "badcmd" would
     * never reach EXEC / the Bad command diagnostic -> the classify oracle goes
     * RED. NEVER in a real build. */
    return CMD_DIR;
#else
    return CMD_EXTERNAL;
#endif
}

void cmd_parse(const char *line, cmd_line_t *out)
{
    int i;

    /* Defensive init (Rule 2): never leave the output uninitialized. */
    out->command[0] = '\0';
    out->arg[0]     = '\0';
    out->tail[0]    = '\0';
    out->kind       = CMD_EMPTY;

    if (line == 0) {
        return;
    }

    const char *p = line;
    while (is_space(*p)) {           /* skip leading whitespace */
        p++;
    }

    /* Copy the command word (up to the first space), upper-casing as we go.
     * Clamp to CMD_TOKEN_MAX-1 so an absurdly long word cannot overflow. */
    i = 0;
    while (*p && !is_space(*p) && i < CMD_TOKEN_MAX - 1) {
#ifdef CMD_MUTATE_NO_UPCASE
        /* MUTANT (Rule 6; make test-command-mutant only): copy the command word
         * WITHOUT upper-casing, so a lowercase "dir" no longer matches the
         * upper-case dispatch table -> the parse oracle goes RED. NEVER in a
         * real build. */
        out->command[i++] = *p;
#else
        out->command[i++] = cmd_upcase_char(*p);
#endif
        p++;
    }
    out->command[i] = '\0';
    /* If the word was longer than the clamp, swallow the rest of it so it does
     * not bleed into arg (it is already a non-recognized command anyway). */
    while (*p && !is_space(*p)) {
        p++;
    }

    /* DOS command tail (initech-456): capture the verbatim remainder -- INCLUDING
     * the leading separator space(s) -- BEFORE arg's leading-space trim below.
     * This is exactly what real DOS copies to the child PSP:80h. `arg` stays the
     * trimmed form the builtins want; `tail` is what an EXEC'd child reads. */
    {
        int t = 0;
        const char *q = p;
        while (*q && t < CMD_LINE_MAX - 1) {
            out->tail[t++] = *q++;
        }
        out->tail[t] = '\0';
    }

    while (is_space(*p)) {           /* skip the spaces after the command */
        p++;
    }

    /* Copy the rest verbatim (case preserved -- ECHO needs it). */
    i = 0;
    while (*p && i < CMD_LINE_MAX - 1) {
        out->arg[i++] = *p++;
    }
    out->arg[i] = '\0';

    out->kind = cmd_classify(out->command);
}

int cmd_first_token(const char *arg, char *out)
{
    int i = 0;
    const char *p = arg;

    out[0] = '\0';
    if (arg == 0) {
        return 0;
    }
    while (is_space(*p)) {            /* skip any leading spaces */
        p++;
    }
    while (*p && !is_space(*p) && i < CMD_TOKEN_MAX - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return i;
}

int cmd_append_com(const char *name, char *out)
{
    int n = 0;
    int has_dot = 0;
    const char *p;

    if (name == 0) {
        out[0] = '\0';
        return 0;
    }
    for (p = name; *p; p++) {
        if (*p == '.') {
            has_dot = 1;
        }
    }

    /* Copy the name. */
    for (p = name; *p && n < CMD_TOKEN_MAX - 1; p++) {
        out[n++] = *p;
    }
    if (*p != '\0') {
        /* The name itself did not fit -- never truncate into a wrong file. */
        out[0] = '\0';
        return 0;
    }

#ifdef CMD_MUTATE_COM_ALWAYS
    /* MUTANT (Rule 6; make test-command-mutant only): append ".COM" ALWAYS, even
     * when the name already carries an extension, so "GREET.COM" becomes
     * "GREET.COM.COM" -> the appender oracle goes RED. NEVER in a real build. */
    has_dot = 0;
#endif
    if (!has_dot) {
        /* Append ".COM" only if it fits (n + 4 chars + NUL). */
        const char *ext = ".COM";
        if (n + 4 >= CMD_TOKEN_MAX) {
            out[0] = '\0';
            return 0;
        }
        for (const char *e = ext; *e; e++) {
            out[n++] = *e;
        }
    }
    out[n] = '\0';
    return 1;
}

/* Append an unsigned decimal to out[*pos], right-justified in `width` columns
 * (space-padded). Pure formatting helper for the DIR line. */
static void put_u_field(char *out, int *pos, uint32_t v, int width)
{
    char tmp[10];
    int n = 0;
    int pad;

    if (v == 0) {
        tmp[n++] = '0';
    } else {
        while (v > 0 && n < (int)sizeof(tmp)) {
            tmp[n++] = (char)('0' + (v % 10u));
            v /= 10u;
        }
    }
    for (pad = width - n; pad > 0; pad--) {
        out[(*pos)++] = ' ';
    }
    while (n > 0) {
        out[(*pos)++] = tmp[--n];
    }
}

int cmd_format_dir_line(const char *fname, uint32_t fsize, uint8_t attr,
                        char *out)
{
    int pos = 0;
    int i;

    /* Left-justify the 8.3 name in a 13-column field (NAME.EXT, max 12 + pad).
     * This is a plausible period-DOS DIR layout (Law 4), not byte-exact DOS. */
    for (i = 0; fname[i] != '\0' && i < 12; i++) {
        out[pos++] = fname[i];
    }
    for (; i < 13; i++) {
        out[pos++] = ' ';
    }

    if (attr & DIR_ATTR_DIRECTORY) {
        const char *d = "      <DIR>";   /* right-ish DIR marker in the size col */
        for (const char *q = d; *q; q++) {
            out[pos++] = *q;
        }
    } else {
        put_u_field(out, &pos, fsize, 11);
    }
    out[pos++] = '\n';
    out[pos]   = '\0';
    return pos;
}

/* cmd_set_parse: PURE SET argument parser (beads initech-1i0x Tranche E inc 2).
 * Ref: DOS 3.3 COMMAND.COM SET behaviour (DEC-11 / Appendix D).
 *   ""          -> SET_OP_LIST  (print all NAME=VALUE pairs from the env).
 *   "NAME=VAL"  -> SET_OP_ASSIGN name="NAME" value="VAL" (value verbatim, may
 *                  contain further '=' characters; only the first '=' splits).
 *   "NAME="     -> SET_OP_CLEAR name="NAME" value="" (empty value -> unset).
 *   "NAME"      -> SET_OP_QUERY name="NAME" value="" (no '=' -> query current).
 * The name is copied verbatim (NOT upcased here; env_set handles upcasing).
 * Clamps: name to CMD_TOKEN_MAX-1 bytes; value to CMD_LINE_MAX-1 bytes. */
void cmd_set_parse(const char *arg, set_cmd_t *out)
{
    int ni;
    int vi;
    const char *p;

    /* Defensive init (Rule 2). */
    out->op       = SET_OP_LIST;
    out->name[0]  = '\0';
    out->value[0] = '\0';

    if (arg == 0) {
        return;
    }

    /* Skip leading whitespace (DOS SET strips leading spaces from the arg). */
    p = arg;
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    /* Empty or all-whitespace arg -> LIST. */
    if (*p == '\0') {
        out->op = SET_OP_LIST;
        return;
    }

    /* Scan for '=' to determine the op.  Copy the name portion as we go.
     * Stop at '=' (delimiter), NUL (end of input), or CMD_TOKEN_MAX-1 bytes. */
    ni = 0;
    while (*p != '\0' && *p != '=' && ni < CMD_TOKEN_MAX - 1) {
        out->name[ni++] = *p++;
    }
    out->name[ni] = '\0';

    /* If we hit NUL before '=', there is no '=' at all -> QUERY. */
    if (*p == '\0') {
        out->op = SET_OP_QUERY;
        return;
    }

    /* *p == '='. Advance past it to the value. */
    p++;   /* skip the '=' */

    /* Copy the value verbatim (may contain further '=', spaces, etc.).
     * An empty string here means SET_OP_CLEAR. */
    vi = 0;
    while (*p != '\0' && vi < CMD_LINE_MAX - 1) {
        out->value[vi++] = *p++;
    }
    out->value[vi] = '\0';

    if (vi == 0) {
        out->op = SET_OP_CLEAR;
    } else {
        out->op = SET_OP_ASSIGN;
    }
}

/* ---- Tranche-F PURE parsing/formatting (host-testable) --------------------
 * Ref: DOS 3.3 COMMAND.COM (DEC-11 / Appendix D); spec/int21h_register.json.
 * These carry NO asm + NO I/O so test_command.c links them directly; the REPL
 * (below, COMMAND_KERNEL_REPL) wires them to the matching int $0x21 calls. */

/* cmd_pair_parse: split `arg` into its first two whitespace-delimited tokens,
 * each UPPER-CASED (DOS upcases 8.3 names). `ok` is 1 iff BOTH are non-empty.
 * Used by COPY <src> <dst> (initech-hpls) and REN <old> <new> (initech-fyox). */
void cmd_pair_parse(const char *arg, cmd_pair_t *out)
{
    const char *p;
    int i;

    /* Defensive init (Rule 2). */
    out->first[0]  = '\0';
    out->second[0] = '\0';
    out->ok        = 0;

    if (arg == 0) {
        return;
    }

    p = arg;
    while (is_space(*p)) {            /* skip leading spaces */
        p++;
    }
    i = 0;
    while (*p && !is_space(*p) && i < CMD_TOKEN_MAX - 1) {
        out->first[i++] = cmd_upcase_char(*p);
        p++;
    }
    out->first[i] = '\0';
    while (*p && !is_space(*p)) {     /* swallow an overlong first token's tail */
        p++;
    }

    while (is_space(*p)) {            /* skip the separator before token two */
        p++;
    }
    i = 0;
    while (*p && !is_space(*p) && i < CMD_TOKEN_MAX - 1) {
        out->second[i++] = cmd_upcase_char(*p);
        p++;
    }
    out->second[i] = '\0';

    /* Both operands required (a missing src or dst is a parameter-missing error). */
    out->ok = (out->first[0] != '\0' && out->second[0] != '\0') ? 1 : 0;
}

/* cmd_has_wildcard: 1 if `name` carries a DOS wildcard ('*' or '?'). DEL/ERASE
 * routes a plain name to a single UNLINK and a pattern to a FINDFIRST/NEXT loop. */
int cmd_has_wildcard(const char *name)
{
    if (name == 0) {
        return 0;
    }
    for (const char *p = name; *p; p++) {
        if (*p == '*' || *p == '?') {
            return 1;
        }
    }
    return 0;
}

/* Append `v` to out[*pos] as exactly `width` zero-padded decimal digits (DOS
 * date/time fields are fixed-width: MM, DD, HH, etc.). Truncates the high digits
 * if v exceeds the field width (callers range-validate first; defensive). */
static void put_u_pad0(char *out, int *pos, uint32_t v, int width)
{
    char tmp[10];
    int n = 0;
    int k;

    if (width > (int)sizeof(tmp)) {
        width = (int)sizeof(tmp);
    }
    for (k = 0; k < width; k++) {
        tmp[k] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    /* tmp holds the digits least-significant-first; emit reversed. */
    for (n = width - 1; n >= 0; n--) {
        out[(*pos)++] = tmp[n];
    }
}

int cmd_format_date(uint8_t dow, uint16_t year, uint8_t mon, uint8_t day,
                    char *out)
{
    /* DOS day-of-week abbreviations (0=Sun..6=Sat), as DATE prints them. */
    static const char *const days[7] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    const char *prefix = "Current date is ";
    const char *dname  = (dow <= 6u) ? days[dow] : "Sun";
    int pos = 0;
    int i;

    for (i = 0; prefix[i] != '\0'; i++) {
        out[pos++] = prefix[i];
    }
    for (i = 0; dname[i] != '\0'; i++) {
        out[pos++] = dname[i];
    }
    out[pos++] = ' ';
    put_u_pad0(out, &pos, (uint32_t)mon, 2);     /* MM */
    out[pos++] = '-';
    put_u_pad0(out, &pos, (uint32_t)day, 2);     /* DD */
    out[pos++] = '-';
    put_u_pad0(out, &pos, (uint32_t)year, 4);    /* YYYY */
    out[pos]   = '\0';
    return pos;
}

int cmd_format_time(uint8_t hh, uint8_t mi, uint8_t ss, uint8_t cs, char *out)
{
    const char *prefix = "Current time is ";
    int pos = 0;
    int i;

    for (i = 0; prefix[i] != '\0'; i++) {
        out[pos++] = prefix[i];
    }
    put_u_pad0(out, &pos, (uint32_t)hh, 2);       /* HH */
    out[pos++] = ':';
    put_u_pad0(out, &pos, (uint32_t)mi, 2);       /* MM */
    out[pos++] = ':';
    put_u_pad0(out, &pos, (uint32_t)ss, 2);       /* SS */
    out[pos++] = '.';
    put_u_pad0(out, &pos, (uint32_t)cs, 2);       /* cc (centiseconds) */
    out[pos]   = '\0';
    return pos;
}

/* Parse an unsigned decimal field of 1..`maxdig` digits starting at *pp; advance
 * *pp past it. Returns 1 on at least one digit consumed (value in *out), else 0. */
static int parse_u_field(const char **pp, uint32_t *out, int maxdig)
{
    const char *p = *pp;
    uint32_t v = 0;
    int n = 0;

    while (*p >= '0' && *p <= '9' && n < maxdig) {
        v = v * 10u + (uint32_t)(*p - '0');
        p++;
        n++;
    }
    if (n == 0) {
        return 0;
    }
    *out = v;
    *pp  = p;
    return 1;
}

/* Days-in-month with a Gregorian leap-year check (the SET DATE validation also
 * runs in the kernel's rtc_encode, but DOS COMMAND.COM rejects a bad date BEFORE
 * issuing 2Bh, so we validate here too -- Law 1: match DOS reprompt behaviour). */
static int days_in_month(uint16_t year, uint8_t mon)
{
    static const uint8_t dim[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    if (mon < 1u || mon > 12u) {
        return 0;
    }
    if (mon == 2u) {
        int leap = ((year % 4u) == 0u && (year % 100u) != 0u) ||
                   ((year % 400u) == 0u);
        return leap ? 29 : 28;
    }
    return (int)dim[mon - 1];
}

int cmd_parse_date(const char *arg, uint16_t *year, uint8_t *mon, uint8_t *day)
{
    const char *p;
    uint32_t mm = 0, dd = 0, yy = 0;
    char sep;

    if (arg == 0) {
        return 0;
    }
    p = arg;
    while (*p == ' ' || *p == '\t') {     /* skip leading whitespace */
        p++;
    }

    if (!parse_u_field(&p, &mm, 2)) {
        return 0;
    }
    if (*p != '-' && *p != '/') {         /* DOS accepts '-' or '/' */
        return 0;
    }
    sep = *p++;
    if (!parse_u_field(&p, &dd, 2)) {
        return 0;
    }
    if (*p != sep) {                      /* the two separators must match */
        return 0;
    }
    p++;
    if (!parse_u_field(&p, &yy, 4)) {
        return 0;
    }
    /* Trailing whitespace OK; anything else is a malformed date. */
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p != '\0') {
        return 0;
    }

    /* DOS two-digit-year windowing: 80..99 -> 19xx, 00..79 -> 20xx. */
    if (yy < 100u) {
        yy = (yy >= 80u) ? (1900u + yy) : (2000u + yy);
    }
    /* DOS DATE accepts years 1980..2099. */
    if (yy < 1980u || yy > 2099u) {
        return 0;
    }
    if (mm < 1u || mm > 12u) {
        return 0;
    }
    if (dd < 1u || (int)dd > days_in_month((uint16_t)yy, (uint8_t)mm)) {
        return 0;
    }

    *year = (uint16_t)yy;
    *mon  = (uint8_t)mm;
    *day  = (uint8_t)dd;
    return 1;
}

int cmd_parse_time(const char *arg, uint8_t *hh, uint8_t *mi, uint8_t *ss)
{
    const char *p;
    uint32_t h = 0, m = 0, s = 0;

    if (arg == 0) {
        return 0;
    }
    p = arg;
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    if (!parse_u_field(&p, &h, 2)) {
        return 0;
    }
    if (*p != ':') {
        return 0;
    }
    p++;
    if (!parse_u_field(&p, &m, 2)) {
        return 0;
    }
    if (*p == ':') {                      /* seconds are optional (HH:MM[:SS]) */
        p++;
        if (!parse_u_field(&p, &s, 2)) {
            return 0;
        }
    }
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p != '\0') {
        return 0;
    }

    if (h > 23u || m > 59u || s > 59u) {
        return 0;
    }
    *hh = (uint8_t)h;
    *mi = (uint8_t)m;
    *ss = (uint8_t)s;
    return 1;
}

/* cmd_render_prompt: PURE $-metacharacter renderer (beads initech-dibc).
 * Ref: ADR-0003 DEC-12 (the $P$G prompt + version 3.30); DOS 3.3 COMMAND.COM
 *      PROMPT command + $-metacharacter set (Appendix D / DEC-11).
 *
 * Expands each "$X" sequence in `templ` according to `ctx`, writing into `out`
 * with a hard cap at `cap`-1 characters (always NUL-terminates).  Unknown "$X"
 * codes emit X literally -- this matches real DOS 3.3 COMMAND.COM behaviour
 * (undocumented metacharacters pass the character through rather than being
 * silently dropped, so custom prompt strings like "$Pmy$G" still render their
 * literal chars).  PURE: no asm, no I/O; safe for the host test oracle. */
int cmd_render_prompt(const char *templ, const prompt_ctx_t *ctx,
                      char *out, int cap)
{
    int pos = 0;
    const char *p;

    /* Defensive: if cap is nonsensical, write nothing. */
    if (out == 0 || cap <= 0) {
        return 0;
    }
    out[0] = '\0';

    if (templ == 0 || ctx == 0) {
        return 0;
    }

/* Helper: append one character, stopping when `pos` reaches cap-1. */
#define PROMPT_PUTC(c) do { \
    if (pos < cap - 1) { out[pos++] = (char)(c); } \
} while (0)

    for (p = templ; *p != '\0'; p++) {
        if (*p != '$') {
            PROMPT_PUTC(*p);
            continue;
        }

        /* '$' found: look at the next character. */
        p++;
        if (*p == '\0') {
            /* Trailing '$' with no following char: emit nothing and stop. */
            break;
        }

        switch (*p) {
            case 'P': case 'p':
                /* $P -> drive + ":" + "\" + cwd (DOS root-relative).
                 * ctx->cwd is the AH=47h root-relative string ("" for root or
                 * "SUB" for a subdir).  We emit: drive ':' '\' [cwd].
                 * Ref: ADR-0003 DEC-12 -- "A:\" at root, "A:\SUB" in subdir. */
                PROMPT_PUTC(ctx->drive);
                PROMPT_PUTC(':');
                PROMPT_PUTC('\\');
                if (ctx->cwd != 0) {
                    const char *q = ctx->cwd;
                    while (*q != '\0') {
                        PROMPT_PUTC(*q);
                        q++;
                    }
                }
                break;

            case 'G': case 'g':
                /* $G -> ">" (greater-than; the DOS prompt arrow). */
                PROMPT_PUTC('>');
                break;

            case 'L': case 'l':
                /* $L -> "<" (less-than). */
                PROMPT_PUTC('<');
                break;

            case 'B': case 'b':
                /* $B -> "|" (pipe). */
                PROMPT_PUTC('|');
                break;

            case '$':
                /* $$ -> "$" (literal dollar sign). */
                PROMPT_PUTC('$');
                break;

            case 'Q': case 'q':
                /* $Q -> "=" (equals sign). */
                PROMPT_PUTC('=');
                break;

            case 'N': case 'n':
                /* $N -> the single drive letter (e.g. "A"). */
                PROMPT_PUTC(ctx->drive);
                break;

            case 'T': case 't': {
                /* $T -> "HH:MM:SS.CC" (the current time from ctx).
                 * Each field is zero-padded to two digits. */
                char tmp[12];
                int n = 0;
                /* HH */
                tmp[n++] = (char)('0' + ((ctx->hour   / 10) % 10));
                tmp[n++] = (char)('0' + ( ctx->hour   % 10));
                tmp[n++] = ':';
                /* MM */
                tmp[n++] = (char)('0' + ((ctx->minute / 10) % 10));
                tmp[n++] = (char)('0' + ( ctx->minute % 10));
                tmp[n++] = ':';
                /* SS */
                tmp[n++] = (char)('0' + ((ctx->second / 10) % 10));
                tmp[n++] = (char)('0' + ( ctx->second % 10));
                tmp[n++] = '.';
                /* CC */
                tmp[n++] = (char)('0' + ((ctx->centisec / 10) % 10));
                tmp[n++] = (char)('0' + ( ctx->centisec % 10));
                tmp[n]   = '\0';
                for (int i = 0; i < n; i++) {
                    PROMPT_PUTC(tmp[i]);
                }
                break;
            }

            case 'D': case 'd': {
                /* $D -> "mm-dd-yyyy" (the current date from ctx).
                 * DOS DATE command uses MM-DD-YYYY; mirrors cmd_format_date.
                 * We emit the raw numeric form (no day-of-week; the prompt just
                 * needs the date, not the "Current date is ..." wrapper). */
                char tmp[11];
                int n = 0;
                /* MM */
                tmp[n++] = (char)('0' + ((ctx->month / 10) % 10));
                tmp[n++] = (char)('0' + ( ctx->month % 10));
                tmp[n++] = '-';
                /* DD */
                tmp[n++] = (char)('0' + ((ctx->day   / 10) % 10));
                tmp[n++] = (char)('0' + ( ctx->day   % 10));
                tmp[n++] = '-';
                /* YYYY (four digits; DOS years 1980-2099 are always four-digit) */
                tmp[n++] = (char)('0' + ((ctx->year / 1000) % 10));
                tmp[n++] = (char)('0' + ((ctx->year / 100)  % 10));
                tmp[n++] = (char)('0' + ((ctx->year / 10)   % 10));
                tmp[n++] = (char)('0' + ( ctx->year         % 10));
                tmp[n]   = '\0';
                for (int i = 0; i < n; i++) {
                    PROMPT_PUTC(tmp[i]);
                }
                break;
            }

            case '_':
                /* $_ -> CRLF (carriage-return + line-feed). */
                PROMPT_PUTC('\r');
                PROMPT_PUTC('\n');
                break;

            case 'E': case 'e':
                /* $E -> ESC (0x1B; ANSI escape sequence introducer). */
                PROMPT_PUTC(0x1B);
                break;

            case 'H': case 'h':
                /* $H -> backspace (0x08). */
                PROMPT_PUTC(0x08);
                break;

            default:
                /* Unknown "$X": emit X literally (DOS 3.3 behaviour -- unknown
                 * metacharacters pass the character through, not dropped). */
                PROMPT_PUTC(*p);
                break;
        }
    }

#undef PROMPT_PUTC

    out[pos] = '\0';
    return pos;
}

/* ===========================================================================
 * PATH-directory search planner (PURE, host-testable).
 * cmd_path_candidates: compute the ordered candidate list for an external
 * command word, following the DOS 3.3 PATH search order.
 * Ref: ADR-0003 DEC-11 / Appendix D; DOS 3.3 COMMAND.COM PATH search.
 * CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Rule 2 (fail loud),
 * Rule 11 (deterministic), Rule 12 (ASCII).
 * ===========================================================================*/

/* Determine if `word` is an explicit path (contains a ':' drive prefix or a
 * '\\' directory separator).  An explicit path is used verbatim (with .COM
 * appended if no '.' is present).  Returns 1 if explicit, 0 if simple. */
static int is_explicit_path(const char *word)
{
    const char *p;
    if (word == 0) {
        return 0;
    }
    for (p = word; *p != '\0'; p++) {
        if (*p == '\\' || *p == ':') {
            return 1;
        }
    }
    return 0;
}

/* Return 1 if `word` already has an extension (a '.' after the last '\\'),
 * 0 otherwise.  Applies to both simple names ("FOO.COM") and explicit paths
 * ("A:\\X\\FOO.COM"). */
static int has_extension(const char *word)
{
    const char *last_dot = 0;
    const char *p;
    if (word == 0) {
        return 0;
    }
    for (p = word; *p != '\0'; p++) {
        if (*p == '.') {
            last_dot = p;
        } else if (*p == '\\') {
            last_dot = 0;   /* a dot before a backslash is NOT an extension dot */
        }
    }
    return (last_dot != 0) ? 1 : 0;
}

/* Append `suffix` to `dst` (already holding some prefix text of length `n`),
 * writing into dst[CMD_PATH_MAX_LEN].  Returns the new length on success, or
 * -1 if the result would not fit (never overflows -- Rule 2 / fail loud). */
static int path_cat(char *dst, int n, const char *suffix)
{
    const char *p = suffix;
    while (*p != '\0') {
        if (n >= CMD_PATH_MAX_LEN - 1) {
            return -1;   /* would overflow; caller skips this entry */
        }
        dst[n++] = *p++;
    }
    dst[n] = '\0';
    return n;
}

/* Split `path_value` on ';' into dirs[].  Each dir is stored as an ASCIIZ
 * string.  Returns the number of dirs extracted (bounded to CMD_PATH_MAX_DIRS;
 * extras silently discarded -- Rule 2: never overflow the array). */
static int split_path(const char *path_value,
                      char dirs[CMD_PATH_MAX_DIRS][CMD_PATH_MAX_LEN])
{
    int nd = 0;
    int di = 0;
    const char *p;

    if (path_value == 0 || path_value[0] == '\0') {
        return 0;
    }

    p = path_value;
    while (*p != '\0' && nd < CMD_PATH_MAX_DIRS) {
        if (*p == ';') {
            /* Emit the current token if non-empty. */
            if (di > 0) {
                dirs[nd][di] = '\0';
                nd++;
                di = 0;
            }
            p++;
        } else {
            if (di < CMD_PATH_MAX_LEN - 1) {
                dirs[nd][di++] = *p;
            }
            /* else: dir name too long -- silently drop characters (Rule 2:
             * never overflow; the resulting truncated dir will not be found). */
            p++;
        }
    }
    /* Flush the last token. */
    if (di > 0 && nd < CMD_PATH_MAX_DIRS) {
        dirs[nd][di] = '\0';
        nd++;
    }
    return nd;
}

/* Build one candidate "dir + '\\' + word + ext" into out->entries[out->count].
 * `dir`  may be NULL/empty for the CWD bare-root case (we treat empty dir as
 *        already containing the backslash entry: caller passes cwd).
 * `word` is the command word (not yet appended with ext).
 * `ext`  is ".COM", ".EXE", ".BAT", or "" (if word already has extension).
 * Returns 1 on success (entry stored), 0 on overflow (skipped -- Rule 2). */
static int emit_candidate(cmd_path_iter_t *out,
                          const char *dir, const char *word, const char *ext)
{
    char *dst;
    int   n = 0;

    if (out->count >= CMD_PATH_MAX_DIRS + 1) {
        return 0;   /* array full -- skip silently (Rule 2) */
    }

    dst = out->entries[out->count];
    dst[0] = '\0';

    /* Append dir (if present and non-empty). */
    if (dir != 0 && dir[0] != '\0') {
        n = path_cat(dst, n, dir);
        if (n < 0) { return 0; }
        /* Ensure a trailing backslash between dir and word. */
        if (n > 0 && dst[n - 1] != '\\') {
            if (n >= CMD_PATH_MAX_LEN - 1) { return 0; }
            dst[n++] = '\\';
            dst[n]   = '\0';
        }
    }

    /* Append the command word. */
    n = path_cat(dst, n, word);
    if (n < 0) { return 0; }

    /* Append the extension (may be "" if word already has one). */
    if (ext != 0 && ext[0] != '\0') {
        n = path_cat(dst, n, ext);
        if (n < 0) { return 0; }
    }

    out->count++;
    return 1;
}

int cmd_path_candidates(const char *word, const char *path_value,
                        const char *cwd, cmd_path_iter_t *out)
{
    /* Defensive init (Rule 2). */
    out->count = 0;

    if (word == 0 || word[0] == '\0') {
        return 0;
    }

    if (is_explicit_path(word)) {
        /* Explicit path: use verbatim.  If no extension present, append .COM.
         * One candidate total (the explicit path -- no PATH search). */
        char entry[CMD_PATH_MAX_LEN];
        int n = 0;
        const char *p = word;

        entry[0] = '\0';
        while (*p != '\0' && n < CMD_PATH_MAX_LEN - 1) {
            entry[n++] = *p++;
        }
        if (*p != '\0') {
            /* Word too long to fit -- fail loud: return 0 candidates (Rule 2). */
            return 0;
        }
        entry[n] = '\0';

        if (!has_extension(word)) {
            /* No extension: append .COM if it fits. */
            if (n + 4 >= CMD_PATH_MAX_LEN) {
                return 0;   /* would overflow -- skip */
            }
            entry[n++] = '.';
            entry[n++] = 'C';
            entry[n++] = 'O';
            entry[n++] = 'M';
            entry[n]   = '\0';
        }

        /* Store the single explicit-path candidate. */
        {
            int i;
            for (i = 0; i < n && i < CMD_PATH_MAX_LEN - 1; i++) {
                out->entries[0][i] = entry[i];
            }
            out->entries[0][i] = '\0';
        }
        out->count = 1;
        return 1;
    }

    /* Simple name (no '\\' or ':'): generate extension rounds.
     * Order: .COM round (CWD then PATH dirs), .EXE round, .BAT round.
     * Ref: DOS 3.3 PATH search -- .COM is tried before .EXE before .BAT in
     * each dir, then the next dir (ADR-0003 DEC-11). */
    {
        /* Determine the effective extension suffix for the word. */
        const char *ext_com = ".COM";
        const char *ext_exe = ".EXE";
        const char *ext_bat = ".BAT";

        /* Split PATH into individual directory strings. */
        char path_dirs[CMD_PATH_MAX_DIRS][CMD_PATH_MAX_LEN];
        int  nd = split_path(path_value, path_dirs);

#ifdef CMD_MUTATE_NO_PATH
        /* MUTANT (Rule 6; make test-command-mutant only): suppress the PATH-
         * dir loop so the split result is "used" and no unused-variable
         * warning fires.  Only CWD candidates are emitted -> PATH tests go
         * RED.  NEVER compiled in a real build. */
        (void)nd;
        (void)path_dirs;
#endif

        if (has_extension(word)) {
            /* Word already has extension: emit CWD + each PATH dir, no ext. */
            emit_candidate(out, cwd, word, "");
#ifndef CMD_MUTATE_NO_PATH
            /* PATH-dir candidates (mutant: -DCMD_MUTATE_NO_PATH drops this
             * block so only the CWD candidate is emitted -> PATH tests go RED;
             * NEVER dropped in a real build -- CLAUDE.md Rule 6). */
            {
                int di;
                for (di = 0; di < nd; di++) {
                    emit_candidate(out, path_dirs[di], word, "");
                }
            }
#endif
            return out->count;
        }

        /* .COM round: CWD first, then PATH dirs. */
        emit_candidate(out, cwd, word, ext_com);
#ifndef CMD_MUTATE_NO_PATH
        {
            int di;
            for (di = 0; di < nd; di++) {
                emit_candidate(out, path_dirs[di], word, ext_com);
            }
        }
#endif

        /* .EXE round: CWD first, then PATH dirs. */
        emit_candidate(out, cwd, word, ext_exe);
#ifndef CMD_MUTATE_NO_PATH
        {
            int di;
            for (di = 0; di < nd; di++) {
                emit_candidate(out, path_dirs[di], word, ext_exe);
            }
        }
#endif

        /* .BAT round: CWD first, then PATH dirs.
         * NOTE: .BAT execution is deferred (beads xw1); these candidates are
         * planned and returned so the caller can report "Bad command or file
         * name" after exhausting all three extension types rather than stopping
         * at .EXE.  The REPL's run_external ignores .BAT entries for now. */
        emit_candidate(out, cwd, word, ext_bat);
#ifndef CMD_MUTATE_NO_PATH
        {
            int di;
            for (di = 0; di < nd; di++) {
                emit_candidate(out, path_dirs[di], word, ext_bat);
            }
        }
#endif
    }

    return out->count;
}

/* ===========================================================================
 * The REPL (kernel-only). Everything below dogfoods the INT 21h API via real
 * `int $0x21` calls -- the authentic COMMAND.COM design + proof the OS API is a
 * usable surface (the brief's central design point). Compiled out of the host
 * build (test_command.c never defines COMMAND_KERNEL_REPL) so the pure logic
 * above stays asm-free + host-testable (Law 3).
 * ===========================================================================*/
#ifdef COMMAND_KERNEL_REPL

#include "find_data.h"   /* find_data_t (43-byte DTA record; offsets DIR reads) */
#include "int21.h"       /* INT21_CWD_MAX -- the AH=47h root-relative CWD bound */
#include "memory_map.h"  /* ENV_BLOCK / ENV_BLOCK_CAP -- EXEC env inheritance (inc 3) */
#include "batch.h"       /* the .BAT parser/expander + IF/FOR decision helpers   */

/* DEC-04a flat ABI (spec/int21h_calling_convention.json): AH=function in EAX,
 * EBX=handle, ECX=count, EDX=flat ptr, EAX=return, CF=error in EFLAGS. The
 * wrappers below issue a literal `int $0x21` honoring exactly that contract --
 * the SAME path a real DOS COMMAND.COM uses (it is itself a .COM that calls the
 * INT 21h services). We capture CF with `sbb` (carry -> 0xFFFFFFFF/0). */

/* AH=09h DISPLAY STRING: EDX -> '$'-terminated string -> CON. */
static void dos_print(const char *dollar_terminated)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x0900u), "d"((uint32_t)(uintptr_t)dollar_terminated)
        : "cc", "memory");
}

/* Print an ASCIIZ string by writing it to stdout (handle 1) via AH=40h. Used
 * for output that is not naturally '$'-terminated (it may contain '$'). */
static void dos_write(const char *s, uint32_t len)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x4000u), "b"(1u), "c"(len), "d"((uint32_t)(uintptr_t)s)
        : "cc", "memory");
}

static uint32_t str_len(const char *s)
{
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static void dos_puts_raw(const char *s)
{
    dos_write(s, str_len(s));
}

/* AH=0Ah BUFFERED INPUT: EDX -> the buffered-input block (byte0=max incl CR,
 * byte1=count written, bytes2..=chars+CR). Blocks until Enter (the keyboard
 * IRQ wakes the kernel's blocking conin; --keys injection drives it in the
 * oracle). */
static void dos_getline(uint8_t *buf)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x0A00u), "d"((uint32_t)(uintptr_t)buf)
        : "cc", "memory");
}

/* AH=1Ah SET DTA: EDX -> the new Disk Transfer Area (where FINDFIRST/NEXT write
 * the 43-byte find-record). */
static void dos_setdta(void *dta)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x1A00u), "d"((uint32_t)(uintptr_t)dta)
        : "cc", "memory");
}

/* AH=4Eh FINDFIRST: EDX -> ASCIIZ file spec, ECX = attribute mask. Returns 1 on
 * a hit (CF clear), 0 on no-match (CF set). */
static int dos_findfirst(const char *spec, uint16_t attr)
{
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %0, %0"
        : "=r"(carry)
        : "a"(0x4E00u), "c"((uint32_t)attr), "d"((uint32_t)(uintptr_t)spec)
        : "cc", "memory");
    return carry == 0u;
}

/* AH=4Fh FINDNEXT: continue the active search. Returns 1 on a hit, 0 when done. */
static int dos_findnext(void)
{
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %0, %0"
        : "=r"(carry)
        : "a"(0x4F00u)
        : "cc", "memory");
    return carry == 0u;
}

/* AH=3Dh OPEN: EDX -> ASCIIZ path, AL = mode (0 = read). Returns the handle
 * (>=0) on success, or -(error) on failure (CF set). */
static int dos_open(const char *path)
{
    uint32_t ax = 0x3D00u;   /* AH=3Dh, AL=00 (read) */
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)(uintptr_t)path)
        : "cc", "memory");
    if (carry != 0u) {
        return -(int)(ax & 0xFFFFu);
    }
    return (int)(ax & 0xFFFFu);
}

/* AH=3Fh READ: EBX=handle, ECX=count, EDX=buffer. Returns bytes read (0=EOF). */
static uint32_t dos_read(int handle, uint8_t *buf, uint32_t count)
{
    uint32_t ax = 0x3F00u;
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax)
        : "b"((uint32_t)handle), "c"(count), "d"((uint32_t)(uintptr_t)buf)
        : "cc", "memory");
    return ax;   /* EAX = bytes read */
}

/* AH=3Eh CLOSE: EBX=handle. */
static void dos_close(int handle)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x3E00u), "b"((uint32_t)handle)
        : "cc", "memory");
}

/* AH=3Ch CREAT (CREATE/TRUNCATE FILE): EDX -> ASCIIZ 8.3 path, CX = attribute
 * (0 = normal). Returns the write handle (>=0) on success (CF clear), or
 * -(error) on failure (CF set; e.g. 0x0005 access denied / 0x0003 path). The
 * dispatcher's do_creat owns the FAT12 create + SFT/JFT slots (beads
 * initech-0qh); COPY's destination side calls it. */
static int dos_creat(const char *path)
{
    uint32_t ax = 0x3C00u;   /* AH=3Ch, AL=00 */
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "c"(0u), "d"((uint32_t)(uintptr_t)path)
        : "cc", "memory");
    if (carry != 0u) {
        return -(int)(ax & 0xFFFFu);
    }
    return (int)(ax & 0xFFFFu);
}

/* AH=40h WRITE TO FILE/DEVICE: EBX=handle, ECX=count, EDX=flat ptr. Returns the
 * bytes written (EAX) on success, or 0 on a CF error (the COPY loop treats a
 * short/zero write as a failure). This is the FILE-handle counterpart of the
 * console dos_write above (which pins handle 1). */
static uint32_t dos_write_h(int handle, const uint8_t *buf, uint32_t count)
{
    uint32_t ax = 0x4000u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "b"((uint32_t)handle), "c"(count), "d"((uint32_t)(uintptr_t)buf)
        : "cc", "memory");
    if (carry != 0u) {
        return 0u;       /* CF set -> treat as a failed write */
    }
    return ax;           /* EAX = bytes written */
}

/* AH=41h UNLINK (DELETE FILE): EDX -> ASCIIZ path. Returns 0 on success (CF
 * clear), or the DOS error code (CF set; 0x0002 not found / 0x0005 error). */
static uint16_t dos_unlink(const char *path)
{
    uint32_t ax = 0x4100u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)(uintptr_t)path)
        : "cc", "memory");
    if (carry != 0u) {
        return (uint16_t)(ax & 0xFFFFu);
    }
    return 0u;
}

/* AH=56h RENAME (SAME-directory dir-entry rename): EDX -> OLD ASCIIZ path,
 * EDI -> NEW ASCIIZ path (the flat-ABI second pointer, beads initech-gnrc).
 * Returns 0 on success (CF clear), or the DOS error code (CF set; 0x0002 not
 * found / 0x0005 dest exists / 0x0011 cross-dir). */
static uint16_t dos_rename(const char *old_path, const char *new_path)
{
    uint32_t ax = 0x5600u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)(uintptr_t)old_path), "D"((uint32_t)(uintptr_t)new_path)
        : "cc", "memory");
    if (carry != 0u) {
        return (uint16_t)(ax & 0xFFFFu);
    }
    return 0u;
}

/* AH=2Ah GET DATE: CX=year(full), DH=month, DL=day, AL=day-of-week (0=Sun).
 * No error path (DOS 2Ah always succeeds). */
static void dos_getdate(uint16_t *year, uint8_t *mon, uint8_t *day, uint8_t *dow)
{
    uint32_t ax = 0x2A00u;
    uint32_t cx = 0, dx = 0;
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax), "=c"(cx), "=d"(dx)
        :
        : "cc", "memory");
    *year = (uint16_t)(cx & 0xFFFFu);
    *mon  = (uint8_t)((dx >> 8) & 0xFFu);   /* DH */
    *day  = (uint8_t)(dx & 0xFFu);          /* DL */
    *dow  = (uint8_t)(ax & 0xFFu);          /* AL */
}

/* AH=2Bh SET DATE: CX=year(full), DH=month, DL=day. Returns 1 if AL=0 (success),
 * 0 if AL=0xFF (invalid date rejected by the clock seam). */
static int dos_setdate(uint16_t year, uint8_t mon, uint8_t day)
{
    uint32_t ax = 0x2B00u;
    uint32_t dx = ((uint32_t)mon << 8) | (uint32_t)day;   /* DH=mon, DL=day */
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax)
        : "c"((uint32_t)year), "d"(dx)
        : "cc", "memory");
    return ((ax & 0xFFu) == 0u) ? 1 : 0;    /* AL: 0 success / 0xFF invalid */
}

/* AH=2Ch GET TIME: CH=hour, CL=min, DH=sec, DL=centiseconds. No error path. */
static void dos_gettime(uint8_t *hh, uint8_t *mi, uint8_t *ss, uint8_t *cs)
{
    uint32_t ax = 0x2C00u;
    uint32_t cx = 0, dx = 0;
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax), "=c"(cx), "=d"(dx)
        :
        : "cc", "memory");
    *hh = (uint8_t)((cx >> 8) & 0xFFu);     /* CH */
    *mi = (uint8_t)(cx & 0xFFu);            /* CL */
    *ss = (uint8_t)((dx >> 8) & 0xFFu);     /* DH */
    *cs = (uint8_t)(dx & 0xFFu);            /* DL (centiseconds; RTC reports 0) */
}

/* AH=2Dh SET TIME: CH=hour, CL=min, DH=sec, DL=centiseconds(ignored). Returns 1
 * if AL=0 (success), 0 if AL=0xFF (invalid time rejected). */
static int dos_settime(uint8_t hh, uint8_t mi, uint8_t ss)
{
    uint32_t ax = 0x2D00u;
    uint32_t cx = ((uint32_t)hh << 8) | (uint32_t)mi;     /* CH=hh, CL=mi */
    uint32_t dx = ((uint32_t)ss << 8);                    /* DH=ss, DL=0  */
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax)
        : "c"(cx), "d"(dx)
        : "cc", "memory");
    return ((ax & 0xFFu) == 0u) ? 1 : 0;
}

/* The master environment store for COMMAND.COM (beads initech-1i0x Tranche E
 * inc 2/3). File scope so builtin_set, command_repl, AND the EXEC path (dos_exec)
 * can all reach it. Seeded once at REPL entry with the canonical startup variables
 * (COMSPEC, PROMPT, PATH). On EXEC (inc 3) dos_exec serializes it into the child's
 * env block (ENV_BLOCK) so the child PSP env_seg points at the inherited copy.
 * Declared HERE (before dos_exec) so the EXEC wrapper can serialize it; builtin_set
 * + command_repl (below) reach it as the same file-scope object. */
static env_store_t g_master_env;

/* AH=4Bh EXEC (AL=00 load+execute): EDX -> ASCIIZ program path, EBX -> the EXEC
 * parameter block (exec_param_block_t). `tail` is the verbatim DOS command tail
 * (leading separator + args, or "" / NULL for none); we frame it as the DOS
 * {count, text, CR} block the child reads at PSP:80h (initech-456). Returns 0 on
 * a clean run (CF clear), or the DOS error code (CF set; e.g. 0x0002 not found).
 *
 * ENV INHERITANCE (beads initech-1i0x Tranche E inc 3): BEFORE the syscall we
 * serialize the master environment into the dedicated ENV_BLOCK region (the DOS
 * env-block format: "NAME=VALUE\0" entries + a final extra NUL) and set
 * blk.env_block = ENV_BLOCK (the FLAT-linear contract of exec_param_block_t.env_
 * block; psp.c stores it as env_seg via flat_to_fake_paragraph). The loader then
 * does NOT overwrite that block and the child PSP env_seg points at the inherited
 * copy -- a per-process COPY by construction (a separate physical region from the
 * shell's g_master_env). The env is serialized into THE SAME ENV_BLOCK the loader
 * would otherwise stub empty; the loader's inc-3 conditional respects whichever we
 * pass. If the env is somehow too large for ENV_BLOCK_CAP (impossible with the
 * 513-byte env.h ceiling vs the 4096-byte region, but checked for fail-loud
 * discipline), we DO NOT write a truncated/garbage block: print a diagnostic and
 * forward env_block=0 (inherit-empty) -- the cleanest fail-loud choice (the child
 * still runs, with an empty env, never a corrupt one). Rule 2. */
static uint16_t dos_exec(const char *path, const char *tail)
{
    /* DOS-format command tail: byte0 = count, bytes1..count = text, then CR.
     * Clamp the text to the PSP cmd_tail capacity (psp_build clamps again). */
    uint8_t tailblk[1u + CMD_LINE_MAX + 1u];
    uint32_t n = 0;
    if (tail != 0) {
        while (tail[n] != '\0' && n < CMD_LINE_MAX) {
            tailblk[1u + n] = (uint8_t)tail[n];
            n++;
        }
    }
    tailblk[0]      = (uint8_t)n;     /* count of text chars (n <= 128)           */
    tailblk[1u + n] = 0x0Du;          /* CR terminator (real-DOS PSP:80h)         */

    /* Serialize the master env into the ENV_BLOCK region (inc 3). On overflow
     * (env_serialize returns 0) fail loud: do NOT point the child at a half-written
     * block -- forward env_block=0 so the loader synthesizes a clean empty env. */
    uint32_t env_block = (uint32_t)ENV_BLOCK;
    {
        int wrote = env_serialize(&g_master_env, (uint8_t *)(uintptr_t)ENV_BLOCK,
                                  (int)ENV_BLOCK_CAP);
        if (wrote == 0) {
            dos_print("Out of environment space\r\n$");
            env_block = 0u;           /* inherit-empty (clean), never a garbage env */
        }
    }

    exec_param_block_t blk;
    blk.env_block = env_block;        /* ENV_BLOCK (populated) or 0 (inherit-empty) */
    blk.cmd_tail  = (uint32_t)(uintptr_t)tailblk;
    blk.fcb1      = 0u;
    blk.fcb2      = 0u;

    uint32_t ax = 0x4B00u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)(uintptr_t)path), "b"((uint32_t)(uintptr_t)&blk)
        : "cc", "memory");
    if (carry != 0u) {
        return (uint16_t)(ax & 0xFFFFu);
    }
    return 0u;
}

/* AH=4Dh GET RETURN CODE (of the last AH=4Bh EXEC child): AL = the child's exit
 * code, AH = the termination type (0 = normal exit; the only kind this
 * milestone). DOS consumes the value on read (a second 4Dh reads 0), so the
 * batch ERRORLEVEL state must latch AL into g_errorlevel right after each EXEC.
 * Ref: DOS 3.3 PRM AH=4Dh; int21.c do_get_return_code (AL=g_last_child_rc). */
static uint8_t dos_get_errorlevel(void)
{
    uint32_t ax = 0x4D00u;     /* AH=4Dh, AL=00 */
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax)
        :
        : "cc", "memory");
    return (uint8_t)(ax & 0xFFu);     /* AL = the child's exit code */
}

/* AH=08h CHARACTER INPUT, NO ECHO: block for one keystroke, AL=char.  Used by
 * the batch PAUSE directive (DOS PAUSE waits for a key without echoing it).
 * Ref: DOS 3.3 PRM AH=08h; int21.c do_conin_noecho. */
static uint8_t dos_conin(void)
{
    uint32_t ax = 0x0800u;
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax)
        :
        : "cc", "memory");
    return (uint8_t)(ax & 0xFFu);     /* AL = the char read */
}

/* The batch ERRORLEVEL: the exit code of the most recently EXEC'd external
 * program, latched from AH=4Dh after each dos_exec.  `IF ERRORLEVEL n` tests
 * `g_errorlevel >= n` (DOS semantics).  Internal built-ins do NOT change it
 * (DOS only sets ERRORLEVEL from a child process's exit code).  File scope so
 * run_external (the latch site) and run_batch (the IF-eval site) share it. */
static uint8_t g_errorlevel = 0u;

/* Set to 1 when EXIT is dispatched -- from the interactive prompt OR from
 * inside a .BAT (DOS EXIT ends COMMAND.COM either way).  The batch interpreter
 * checks this after each dispatched line and unwinds (CALL nests included), and
 * command_repl's main loop returns on it.  File scope so dispatch_line (the
 * setter) and command_repl / run_batch (the checkers) share one flag. */
static int g_shell_exit = 0;

/* AH=30h GET VERSION: AL=major, AH=minor. */
static void dos_getver(uint8_t *major, uint8_t *minor)
{
    uint32_t ax = 0x3000u;
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax)
        :
        : "cc", "memory");
    *major = (uint8_t)(ax & 0xFFu);
    *minor = (uint8_t)((ax >> 8) & 0xFFu);
}

/* AH=33h AL=00h GET CTRL-BREAK STATE (beads initech-er3h; ADR-0003 Amendment
 * DEC-16). Returns the current BREAK flag in DL (0=OFF, 1=ON). The shell BREAK
 * built-in dogfoots the OS API (a real `int $0x21`) exactly as VER dogfoots
 * AH=30h, so a flag change anywhere (CONFIG.SYS, AH=33h SET) is reflected. */
static uint8_t dos_get_break(void)
{
    uint32_t ax = 0x3300u;     /* AH=33h, AL=00h GET */
    uint32_t dx = 0;
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax), "=d"(dx)
        : "d"(0u)
        : "cc", "memory");
    return (uint8_t)(dx & 0xFFu);
}

/* AH=33h AL=01h SET CTRL-BREAK STATE (beads initech-er3h; DEC-16). DL = the new
 * flag (the dispatcher NORMALIZES non-zero -> ON). Writes the SAME g_break_flag
 * AH=33h GET / CONFIG.SYS BREAK= read. */
static void dos_set_break(uint8_t on)
{
    uint32_t ax = 0x3301u;     /* AH=33h, AL=01h SET */
    __asm__ __volatile__(
        "int $0x21"
        : "+a"(ax)
        : "d"((uint32_t)on)
        : "cc", "memory");
}

/* AH=39h MKDIR (CREATE DIRECTORY): EDX -> ASCIIZ path. Returns 0 on success, or
 * the DOS error code (CF set) -- e.g. 0x0005 ACCESS_DENIED (name exists /
 * non-root parent / no write backend), 0x0003 PATH_NOT_FOUND. The dispatcher's
 * do_mkdir owns the FAT12 write path (beads initech-u6wa); we only call it. */
static uint16_t dos_mkdir(const char *path)
{
    uint32_t ax = 0x3900u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)(uintptr_t)path)
        : "cc", "memory");
    if (carry != 0u) {
        return (uint16_t)(ax & 0xFFFFu);
    }
    return 0u;
}

/* AH=3Ah RMDIR (REMOVE DIRECTORY): EDX -> ASCIIZ path. Returns 0 on success, or
 * the DOS error code (CF set) -- 0x0003 PATH_NOT_FOUND (missing / not a dir),
 * 0x0005 ACCESS_DENIED (non-empty), 0x0010 CURRENT_DIR (root / the CWD). The
 * dispatcher's do_rmdir owns the checks (beads initech-u6wa); we only call it. */
static uint16_t dos_rmdir(const char *path)
{
    uint32_t ax = 0x3A00u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)(uintptr_t)path)
        : "cc", "memory");
    if (carry != 0u) {
        return (uint16_t)(ax & 0xFFFFu);
    }
    return 0u;
}

/* AH=3Bh CHDIR (CHANGE CURRENT DIRECTORY): EDX -> ASCIIZ path. Returns 0 on
 * success, or the DOS error code (CF set) -- 0x0003 PATH_NOT_FOUND (missing dir
 * / CHDIR into a file). The dispatcher's do_chdir resolves "" / "\\" / "." to a
 * no-op-at-root success and walks ".."/relative/absolute (beads initech-u6wa). */
static uint16_t dos_chdir(const char *path)
{
    uint32_t ax = 0x3B00u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)(uintptr_t)path)
        : "cc", "memory");
    if (carry != 0u) {
        return (uint16_t)(ax & 0xFFFFu);
    }
    return 0u;
}

/* AH=47h GET CURRENT DIRECTORY: DL=drive (0=default, 1=A:), ESI -> a 64-byte
 * buffer the dispatcher fills with the canonical ROOT-RELATIVE CWD path (NO
 * leading '\', NO drive; the root is the empty string). `buf` must hold >=
 * INT21_CWD_MAX (64) bytes. The buffer is left a valid ASCIIZ string on entry so
 * a failing call (invalid drive) leaves it empty rather than uninitialized. */
static void dos_getcwd(uint8_t drive, char *buf)
{
    buf[0] = '\0';
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x4700u), "d"((uint32_t)drive), "S"((uint32_t)(uintptr_t)buf)
        : "cc", "memory");
}

/* The shell's DTA: a dedicated 43-byte find-record buffer FINDFIRST/NEXT write
 * into (bound via AH=1Ah). File scope so it outlives each DIR invocation. */
static find_data_t g_shell_dta;

/* A scratch line buffer for output formatting (DIR lines, etc.). */
static char g_out[64];

/* ---- the controlled diagnostics (spec/dos_messages.json, ADR-0003 App C) ----
 * The controlled vocabulary (DEC-13) has a SINGLE source of truth: the generated
 * build/dos_messages.h catalogue (MSG_DOS_*). No inline literal text lives here;
 * the use-sites reference the catalogue macros directly. */

/* ---- the built-in command handlers ---------------------------------------- */

/* SET [NAME[=VALUE]] (beads initech-1i0x Tranche E inc 2).
 * Ref: DOS 3.3 COMMAND.COM SET (DEC-11 / Appendix D).
 *   LIST   -> print every "NAME=VALUE\r\n" from the master env.
 *   ASSIGN -> env_set(name, value); on overflow print env-full message.
 *   CLEAR  -> env_unset(name); silent on success (DOS SET NAME= is silent).
 *   QUERY  -> env_get(name); print "NAME=value\r\n" if found, else error.
 * "Out of environment space" and "Environment variable not defined" are not in
 * the controlled catalogue (dos_messages.json) so they are inlined here --
 * they are SET-specific; the catalogue covers the general DOS diagnostics. */
static void builtin_set(const char *arg)
{
    set_cmd_t sc;

    cmd_set_parse(arg, &sc);

    switch (sc.op) {

        case SET_OP_LIST: {
            /* Print every entry as "NAME=VALUE\r\n" via AH=40h (raw write).
             * env_entry returns the full "NAME=VALUE" ASCIIZ string. */
            int count = env_count(&g_master_env);
            int i;
            for (i = 0; i < count; i++) {
                const char *entry = env_entry(&g_master_env, i);
                if (entry != 0) {
                    dos_puts_raw(entry);
                    dos_print("\r\n$");
                }
            }
            break;
        }

        case SET_OP_ASSIGN: {
            /* UPSERT; env_set upcases the name (DOS semantics). */
            if (!env_set(&g_master_env, sc.name, sc.value)) {
                /* Overflow: the new entry would not fit in the 512-byte arena. */
                dos_print("Out of environment space\r\n$");
            }
            break;
        }

        case SET_OP_CLEAR: {
            /* Remove the variable (silent on success; silent if already absent,
             * matching DOS SET NAME= behaviour -- it does not error on missing). */
            env_unset(&g_master_env, sc.name);
            break;
        }

        case SET_OP_QUERY: {
            /* Show the current value if defined, else print an error. */
            const char *val = env_get(&g_master_env, sc.name);
            if (val != 0) {
                /* Print "NAME=value\r\n". sc.name is verbatim from the arg
                 * (not upcased), but DOS SET QUERY mirrors the stored name.
                 * Use env_get's result to walk back to the full stored entry. */
                int count = env_count(&g_master_env);
                int i;
                for (i = 0; i < count; i++) {
                    const char *entry = env_entry(&g_master_env, i);
                    if (entry != 0) {
                        /* Find the stored name portion and check it matches. */
                        const char *eq = entry;
                        while (*eq != '\0' && *eq != '=') {
                            eq++;
                        }
                        if (*eq == '=' && (eq + 1) == val) {
                            dos_puts_raw(entry);
                            dos_print("\r\n$");
                            break;
                        }
                    }
                }
            } else {
                /* Not defined: mirror the DOS "Variable not defined" phrasing. */
                dos_print("Environment variable not defined\r\n$");
            }
            break;
        }
    }
}

/* PROMPT [template] (beads initech-dibc; ADR-0003 DEC-12).
 *   PROMPT <template> -> env_set(&g_master_env, "PROMPT", template).
 *   PROMPT (no arg)   -> env_set(&g_master_env, "PROMPT", "$P$G")  (reset to
 *                        the DOS 3.3 default prompt; DEC-12 establishes "$P$G"
 *                        as the InitechDOS default, matching DOS 3.3 on a
 *                        single-floppy system).
 * Silent on success (DOS PROMPT prints nothing). On env overflow: "Out of
 * environment space" (matching the SET built-in's phrasing; SET-specific, not
 * in the MSG-DOS-NNNN catalogue). The REPL reads back env_get("PROMPT") at
 * every prompt print, so the new string takes effect immediately next loop. */
static void builtin_prompt(const char *arg)
{
    const char *templ;
    const char *p;

    /* Skip leading whitespace to determine if the arg is empty. */
    p = arg;
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    /* Empty (or all-whitespace) arg -> reset to "$P$G" (the DOS 3.3 default).
     * Ref: ADR-0003 DEC-12: the InitechDOS prompt default is "$P$G". */
    templ = (*p == '\0') ? "$P$G" : p;

    if (!env_set(&g_master_env, "PROMPT", templ)) {
        dos_print("Out of environment space\r\n$");
    }
    /* Success is SILENT (DOS PROMPT prints nothing on success). */
}

/* DIR: FINDFIRST "*.*" then loop FINDNEXT, formatting each find-record into a
 * DOS-like line. A header ("Directory of A:\") + a file-count footer frame it
 * (Law 4 -- plausibly period DOS). The find-record fields are read via the
 * spec/find_data.h struct (real-DOS offsets fname@0x1E, fsize@0x1A, attr@0x15). */
static void builtin_dir(void)
{
    int n;
    uint32_t files = 0;

    dos_setdta(&g_shell_dta);
    dos_print("\r\n Directory of A:\\\r\n\r\n$");

    /* Search attribute 0x10 (DIRECTORY) so subdir entries are listed too; the
     * dispatcher already includes plain files. */
    if (!dos_findfirst("*.*", DIR_ATTR_DIRECTORY)) {
        dos_print(MSG_DOS_0003 "\r\n$");
        return;
    }
    do {
        n = cmd_format_dir_line(g_shell_dta.fname, g_shell_dta.fsize,
                                g_shell_dta.attr, g_out);
        /* cmd_format_dir_line ends the line with '\n'; the CON console + serial
         * treat '\n' as newline (the banner uses '\n' too). */
        dos_write(g_out, (uint32_t)n);
        files++;
    } while (dos_findnext());

    /* File-count footer. */
    {
        int pos = 0;
        put_u_field(g_out, &pos, files, 0);
        g_out[pos] = '\0';
        dos_print("\r\n$");
        dos_puts_raw(g_out);
        dos_print(" file(s)\r\n$");
    }
}

/* TYPE <file>: OPEN the named file, READ it in chunks, WRITE each chunk to
 * stdout (handle 1), CLOSE. File-not-found => the controlled diagnostic. */
static void builtin_type(const char *arg)
{
    char name[CMD_TOKEN_MAX];
    int handle;
    uint8_t chunk[128];
    uint32_t got;

    if (cmd_first_token(arg, name) == 0) {
        /* No filename given -- MSG-DOS-0011 "Required parameter missing". */
        dos_print(MSG_DOS_0011 "\r\n$");
        return;
    }
    cmd_upcase_str(name);   /* DOS upcases 8.3 names */

    handle = dos_open(name);
    if (handle < 0) {
        dos_print(MSG_DOS_0003 "\r\n$");
        return;
    }
    for (;;) {
        got = dos_read(handle, chunk, sizeof(chunk));
        if (got == 0u) {
            break;          /* EOF */
        }
        dos_write((const char *)chunk, got);
    }
    dos_close(handle);
    dos_print("\r\n$");     /* ensure a trailing newline after the file body */
}

/* Compose the displayable CWD ("A:\" + the root-relative path AH=47h reports)
 * into `out`. The root is the empty root-relative string -> "A:\"; a subdir
 * "SUB" -> "A:\SUB". `out` must hold >= 3 (drive+root '\') + INT21_CWD_MAX-1 + 1
 * (NUL) bytes. The result is ASCIIZ; no trailing CRLF (callers add framing). Used
 * by BOTH the $P$G prompt and CD-with-no-arg so the two never drift. */
static void cwd_display(char *out)
{
    char rel[INT21_CWD_MAX];   /* root-relative CWD text (no drive, no leading \) */
    int n = 0;
    int i = 0;

    dos_getcwd(0u, rel);              /* DL=0 -> the default drive (A:) */

    out[n++] = 'A';
    out[n++] = ':';
    out[n++] = '\\';                  /* the drive + root separator -> "A:\" */
    while (rel[i] != '\0' && i < (int)(INT21_CWD_MAX - 1)) {
        out[n++] = rel[i++];          /* append the root-relative subpath */
    }
    out[n] = '\0';
}

/* CD/CHDIR: no arg => print the current directory ("A:\" or "A:\SUB"). A path arg
 * => AH=3Bh CHDIR; on success DOS is SILENT (the change shows at the next prompt),
 * on failure print the controlled "Invalid directory" (MSG-DOS-0018). The
 * dispatcher's do_chdir resolves ""/"\\"/"." to a no-op-at-root success and walks
 * ".."/relative/absolute subpaths (beads initech-u6wa / initech-ut6d). */
static void builtin_cd(const char *arg)
{
    char tok[CMD_TOKEN_MAX];
    int len = cmd_first_token(arg, tok);

    if (len == 0) {
        /* No arg -> print the current directory (DOS 'CD' alone reports it).
         * Worst-case index math: cwd_display writes "A:\" (3) + up to
         * (INT21_CWD_MAX-1) relative-path bytes + NUL = 3 + INT21_CWD_MAX bytes,
         * so a bare [3 + INT21_CWD_MAX] buffer is exactly full. The +4 slack
         * leaves headroom (defensive, Rule 2; behavior unchanged). */
        char disp[3 + INT21_CWD_MAX + 4];
        cwd_display(disp);
        dos_puts_raw(disp);
        dos_print("\r\n$");
        return;
    }
    cmd_upcase_str(tok);              /* DOS upcases 8.3 path components */
    if (dos_chdir(tok) != 0u) {
        dos_print(MSG_DOS_0018 "\r\n$");   /* "Invalid directory" */
    }
    /* Success is SILENT -- the new path shows at the next prompt (real DOS 3.3). */
}

/* MD/MKDIR <path>: AH=39h CREATE DIRECTORY. No arg => "Required parameter
 * missing" (MSG-DOS-0011). On success DOS prints NOTHING; any failure (name
 * exists / no write backend / disk full / bad parent) => the single controlled
 * "Unable to create directory" (MSG-DOS-0017) -- real DOS uses ONE message per
 * command, not per error code (DEC-13). */
static void builtin_md(const char *arg)
{
    char path[CMD_TOKEN_MAX];

    if (cmd_first_token(arg, path) == 0) {
        dos_print(MSG_DOS_0011 "\r\n$");   /* "Required parameter missing" */
        return;
    }
    cmd_upcase_str(path);            /* DOS upcases 8.3 path components */
    if (dos_mkdir(path) != 0u) {
        dos_print(MSG_DOS_0017 "\r\n$");   /* "Unable to create directory" */
    }
    /* Success is SILENT (DOS MD prints nothing on success). */
}

/* RD/RMDIR <path>: AH=3Ah REMOVE DIRECTORY. No arg => "Required parameter
 * missing" (MSG-DOS-0011). On success DOS prints NOTHING; any failure (missing /
 * not a directory / not empty / the root or current dir) => the single controlled
 * "Invalid path, not directory, or directory not empty" (MSG-DOS-0019). */
static void builtin_rd(const char *arg)
{
    char path[CMD_TOKEN_MAX];

    if (cmd_first_token(arg, path) == 0) {
        dos_print(MSG_DOS_0011 "\r\n$");   /* "Required parameter missing" */
        return;
    }
    cmd_upcase_str(path);            /* DOS upcases 8.3 path components */
#ifdef CMD_MUTATE_RD_NOOP
    /* MUTANT (Rule 6; make test-ut6d-mutant only): SKIP the AH=3Ah RMDIR entirely
     * but stay silent-as-if-success, so RD becomes a no-op that LOOKS like it
     * worked -- SUB persists on the volume. The re-`cd sub` after RD then SUCCEEDS
     * (the prompt returns to "A:\SUB>") and the controlled "Invalid directory"
     * diagnostic is ABSENT -> the test-ut6d RD-removal assertion goes RED. This is
     * exactly the "silent RD failure" the old gate could not catch. NEVER in a
     * real build. */
    (void)path;
    (void)&dos_rmdir;   /* keep dos_rmdir "used" without calling it (-Werror) */
#else
    if (dos_rmdir(path) != 0u) {
        dos_print(MSG_DOS_0019 "\r\n$");   /* "Invalid path, ... not empty" */
    }
#endif
    /* Success is SILENT (DOS RD prints nothing on success). */
}

/* CLS: clear the screen. The console exposes no clear hook this milestone, so we
 * emit a form feed (0x0C) -- a period-authentic DOS CLS signal -- followed by a
 * page of blank lines as a simple, deterministic fallback. Kept minimal. */
static void builtin_cls(void)
{
    /* Form feed first (a real terminal/console would clear on this). */
    dos_print("\f$");
    for (int i = 0; i < 25; i++) {
        dos_print("\r\n$");
    }
}

/* VER: print the InitechDOS version line. Source the numbers from AH=30h
 * GETVER (3.30; ADR-0003 DEC-12) rather than hard-coding, so a GETVER change
 * is reflected. */
static void builtin_ver(void)
{
    uint8_t major = 0, minor = 0;
    int pos = 0;

    dos_getver(&major, &minor);
    dos_print("\r\nInitechDOS Version $");
    put_u_field(g_out, &pos, (uint32_t)major, 0);
    g_out[pos++] = '.';
    /* minor is the two-digit fractional part (30 -> ".30"); pad to 2 digits. */
    if (minor < 10u) {
        g_out[pos++] = '0';
    }
    put_u_field(g_out, &pos, (uint32_t)minor, 0);
    g_out[pos] = '\0';
    dos_puts_raw(g_out);
    dos_print("\r\n$");
}

/* ECHO <text>: print the text. With no arg, print the on/off status (minimal --
 * DOS prints "ECHO is on"). */
static void builtin_echo(const char *arg)
{
    char first[CMD_TOKEN_MAX];
    if (cmd_first_token(arg, first) == 0) {
        dos_print("ECHO is on\r\n$");
        return;
    }
    dos_puts_raw(arg);
    dos_print("\r\n$");
}

/* BREAK [ON|OFF] (beads initech-er3h; ADR-0003 Amendment DEC-16). With no arg,
 * report the current CTRL-BREAK state ("BREAK is on" / "BREAK is off" -- the
 * built-in's own string, DEC-16 Sec 3.3; not a controlled MSG-DOS-NNNN). With
 * "ON"/"OFF" (case-insensitive), set it via AH=33h SET. Any other argument is a
 * lenient no-op report of the unchanged state (period DOS does not abort on a
 * bad BREAK argument). All I/O dogfoots INT 21h (AH=33h GET/SET + AH=09h). */
static void builtin_break(const char *arg)
{
    char first[CMD_TOKEN_MAX];
    int  n = cmd_first_token(arg, first);

    if (n != 0) {
        cmd_upcase_str(first);
        /* A bare hand-rolled compare keeps this freestanding (no libc). */
        if (first[0] == 'O' && first[1] == 'N' && first[2] == '\0') {
            dos_set_break(1u);
        } else if (first[0] == 'O' && first[1] == 'F' &&
                   first[2] == 'F' && first[3] == '\0') {
            dos_set_break(0u);
        }
        /* else: unrecognized arg -> fall through and just report the state. */
    }

    /* Report the (possibly just-changed) state, read back via AH=33h GET so the
     * report reflects the live g_break_flag (DEC-16 single source of truth). */
    if (dos_get_break() != 0u) {
        dos_print("BREAK is on\r\n$");
    } else {
        dos_print("BREAK is off\r\n$");
    }
}

/* COPY <src> <dst> (beads initech-hpls, Tranche F). Single-file copy:
 *   OPEN src (3Dh) -> CREAT dst (3Ch) -> READ/WRITE loop (3Fh/40h) -> CLOSE both
 *   (3Eh). On success: "        1 file(s) copied" (the DOS COPY footer, Law 4).
 * Errors: a missing operand -> "Required parameter missing" (MSG-DOS-0011); a
 * missing/unopenable source -> "File not found" (MSG-DOS-0003); a destination
 * that cannot be created or a short write -> "Bad command or file name"
 * (MSG-DOS-0002, the catch-all COMMAND.COM diagnostic). Wildcard COPY (src
 * patterns / dir destinations) is DEFERRED -- single-file is the must (the
 * follow-up bead). Ref: DOS 3.3 COPY; spec/int21h_register.json 3Dh/3Ch/3Fh/40h. */
static void builtin_copy(const char *arg)
{
    cmd_pair_t pair;
    int  src_h, dst_h;
    uint8_t chunk[128];
    uint32_t got;

    cmd_pair_parse(arg, &pair);
    if (!pair.ok) {
        dos_print(MSG_DOS_0011 "\r\n$");        /* "Required parameter missing" */
        return;
    }

    src_h = dos_open(pair.first);
    if (src_h < 0) {
        dos_print(MSG_DOS_0003 "\r\n$");        /* "File not found" */
        return;
    }
    dst_h = dos_creat(pair.second);
    if (dst_h < 0) {
        dos_close(src_h);
        dos_print(MSG_DOS_0002 "\r\n$");        /* "Bad command or file name" */
        return;
    }

    for (;;) {
        got = dos_read(src_h, chunk, sizeof(chunk));
        if (got == 0u) {
            break;                              /* EOF -> the copy is complete */
        }
        if (dos_write_h(dst_h, chunk, got) != got) {
            /* A short/failed write (disk full / access) -- fail loud (Rule 2). */
            dos_close(src_h);
            dos_close(dst_h);
            dos_print(MSG_DOS_0002 "\r\n$");
            return;
        }
    }

    dos_close(src_h);
    dos_close(dst_h);
    /* DOS COPY footer: a count column then " file(s) copied" (Law 4). */
    dos_print("        1 file(s) copied\r\n$");
}

/* DEL / ERASE <name> (beads initech-hpls, Tranche F).
 *   plain name    -> UNLINK (41h).
 *   wildcard name -> FINDFIRST/NEXT (4Eh/4Fh) collecting matches, then UNLINK
 *                    each (the DTA name is the formatted 8.3 leaf). We COLLECT
 *                    the matches first, then delete -- deleting under an active
 *                    FINDNEXT cursor would mutate the directory mid-walk.
 * No arg -> "Required parameter missing" (MSG-DOS-0011). A plain name that does
 * not exist -> "File not found" (MSG-DOS-0003). DOS prompts "Are you sure (Y/N)?"
 * only for the bare "DEL *.*" form; that prompt is DEFERRED (the wildcard delete
 * itself runs). Ref: DOS 3.3 DEL/ERASE; spec/int21h_register.json 41h/4Eh/4Fh. */
static void builtin_del(const char *arg)
{
    char name[CMD_TOKEN_MAX];

    if (cmd_first_token(arg, name) == 0) {
        dos_print(MSG_DOS_0011 "\r\n$");        /* "Required parameter missing" */
        return;
    }
    cmd_upcase_str(name);                       /* DOS upcases 8.3 names */

    if (!cmd_has_wildcard(name)) {
        /* Plain name: a single UNLINK. Not-found -> "File not found". */
        if (dos_unlink(name) != 0u) {
            dos_print(MSG_DOS_0003 "\r\n$");
        }
        return;
    }

    /* Wildcard: collect the matching 8.3 names into a fixed roster, then delete.
     * (Collect-then-delete: UNLINK mutates the directory the search walks, so we
     * must not delete while a FINDNEXT cursor is live over the same dir.) */
    {
        char roster[16][13];                    /* up to 16 names per DEL pass */
        int  count = 0;

        dos_setdta(&g_shell_dta);
        if (!dos_findfirst(name, 0u)) {
            /* No match for the pattern -> "File not found" (DOS DEL *.foo). */
            dos_print(MSG_DOS_0003 "\r\n$");
            return;
        }
        do {
            if (count < 16) {
                int i;
                for (i = 0; i < 12 && g_shell_dta.fname[i] != '\0'; i++) {
                    roster[count][i] = g_shell_dta.fname[i];
                }
                roster[count][i] = '\0';
                count++;
            }
        } while (dos_findnext());

        for (int j = 0; j < count; j++) {
            (void)dos_unlink(roster[j]);        /* best-effort per match (DOS) */
        }
    }
}

/* REN / RENAME <old> <new> (beads initech-fyox, Tranche F). AH=56h same-dir
 * dir-entry rename. DOS prints NOTHING on success; a missing operand ->
 * "Required parameter missing" (MSG-DOS-0011); any failure (source missing /
 * dest exists / no backend) -> "Bad command or file name" (MSG-DOS-0002, the
 * COMMAND.COM catch-all). Cross-directory MOVE is a separate kernel bead
 * (initech-ycb3); REN here is in-place only. Ref: DOS 3.3 REN; 56h. */
static void builtin_ren(const char *arg)
{
    cmd_pair_t pair;

    cmd_pair_parse(arg, &pair);
    if (!pair.ok) {
        dos_print(MSG_DOS_0011 "\r\n$");        /* "Required parameter missing" */
        return;
    }
    if (dos_rename(pair.first, pair.second) != 0u) {
        dos_print(MSG_DOS_0002 "\r\n$");        /* "Bad command or file name" */
        return;
    }
    /* Success is SILENT (DOS REN prints nothing). */
}

/* DATE [MM-DD-YY[YY]] (beads initech-uy4l, Tranche F).
 *   no arg -> AH=2Ah GET, print "Current date is Day MM-DD-YYYY".
 *   arg    -> AH=2Bh SET after validating the date (DOS rejects a bad date and
 *             reprompts; we print "Invalid date" and skip the SET, then still
 *             report the unchanged current date). Ref: DOS 3.3 DATE; 2Ah/2Bh. */
static void builtin_date(const char *arg)
{
    char first[CMD_TOKEN_MAX];
    uint16_t year;
    uint8_t  mon, day, dow;
    int len;

    if (cmd_first_token(arg, first) != 0) {
        uint16_t syear;
        uint8_t  smon, sday;
        if (cmd_parse_date(first, &syear, &smon, &sday)) {
            if (!dos_setdate(syear, smon, sday)) {
                dos_print("Invalid date\r\n$");
            }
        } else {
            dos_print("Invalid date\r\n$");
        }
    }

    /* Report the (possibly just-set) current date, read back via AH=2Ah. */
    dos_getdate(&year, &mon, &day, &dow);
    len = cmd_format_date(dow, year, mon, day, g_out);
    dos_write(g_out, (uint32_t)len);
    dos_print("\r\n$");
}

/* TIME [HH:MM[:SS]] (beads initech-uy4l, Tranche F).
 *   no arg -> AH=2Ch GET, print "Current time is HH:MM:SS.cc".
 *   arg    -> AH=2Dh SET after validating; bad time -> "Invalid time" + skip.
 * Ref: DOS 3.3 TIME; spec/int21h_register.json 2Ch/2Dh. */
static void builtin_time(const char *arg)
{
    char first[CMD_TOKEN_MAX];
    uint8_t hh, mi, ss, cs;
    int len;

    if (cmd_first_token(arg, first) != 0) {
        uint8_t shh, smi, sss;
        if (cmd_parse_time(first, &shh, &smi, &sss)) {
            if (!dos_settime(shh, smi, sss)) {
                dos_print("Invalid time\r\n$");
            }
        } else {
            dos_print("Invalid time\r\n$");
        }
    }

    /* Report the (possibly just-set) current time, read back via AH=2Ch. */
    dos_gettime(&hh, &mi, &ss, &cs);
    len = cmd_format_time(hh, mi, ss, cs, g_out);
    dos_write(g_out, (uint32_t)len);
    dos_print("\r\n$");
}

/* Forward declarations for the .BAT execution path (beads initech-xw1).
 * run_external dispatches a .BAT candidate to run_batch_invoke (the argv
 * builder), which frames the command tail into %0..%9 and calls run_batch.
 * Both are defined below, after the dispatcher run_batch reuses. */
static void run_batch(const char *path, const char *const argv[], int argc);
static void run_batch_invoke(const char *path, const char *tail);

/* Return 1 if `name` ends in the upper-cased extension ".BAT", else 0.  The
 * candidate paths cmd_path_candidates emits are upper-cased by run_external
 * before this test (DOS is case-insensitive; the FAT layer stores upper case),
 * so a plain suffix compare suffices.  PURE helper (no I/O). */
static int bat_ends_with_bat(const char *name)
{
    int n = 0;
    while (name[n] != '\0') {
        n++;
    }
    if (n < 4) {
        return 0;
    }
    return (name[n - 4] == '.' && name[n - 3] == 'B' &&
            name[n - 2] == 'A' && name[n - 1] == 'T') ? 1 : 0;
}

/* External command: search PATH directories for `command` (.COM, .EXE, .BAT
 * order), AH=4Bh EXEC the first .COM/.EXE match, or run the first .BAT match
 * through the batch interpreter (beads initech-xw1).  On exhausting all
 * candidates => "Bad command or file name" (MSG-DOS-0002).
 * Ref: ADR-0003 DEC-11 / Appendix D; DOS 3.3 COMMAND.COM PATH search.
 * Ref: cmd_path_candidates (above) for the candidate planning logic. */
static void run_external(const char *command, const char *tail)
{
    cmd_path_iter_t cands;
    char cwd[3 + INT21_CWD_MAX]; /* "A:\" + root-relative + NUL */
    const char *path_val;
    int i;

    /* Build the absolute CWD string ("A:\" at root, "A:\SUB" in a subdir)
     * for use by cmd_path_candidates.  cwd_display assembles this from the
     * AH=47h root-relative path -- the same helper the $P$G renderer uses. */
    cwd_display(cwd);

    /* Read PATH from the master environment.  env_get returns NULL when the
     * variable is absent; treat that identically to an empty string (no PATH
     * dirs, CWD-only search -- the fail-safe DOS behaviour). */
    path_val = env_get(&g_master_env, "PATH");

    /* Plan the ordered candidates.  `command` is already upper-cased by
     * cmd_parse, and cmd_path_candidates upcases the word in the candidate
     * strings it builds (they inherit `command`'s case as-is). */
    cmd_path_candidates(command, path_val, cwd, &cands);

    /* Try each candidate in order; stop at the first successful EXEC / .BAT. */
    for (i = 0; i < cands.count; i++) {
        char *prog = cands.entries[i];
        uint16_t err;

        /* Upcase the full candidate path (DOS is case-insensitive; the kernel
         * FAT layer stores names in upper case). */
        cmd_upcase_str(prog);

        if (bat_ends_with_bat(prog)) {
            /* A .BAT candidate (beads initech-xw1): if the file exists, run it
             * through the batch interpreter rather than AH=4Bh EXEC (a .BAT is
             * not a loadable image).  Probe with AH=3Dh OPEN; on a hit, build
             * argv (argv[0] = the batch path, argv[1..9] = the params from the
             * tail) and interpret.  A .BAT never feeds back into ERRORLEVEL of
             * the OUTER context here (DOS sets ERRORLEVEL only on EXIT/child
             * exit); run_batch latches it internally per EXEC'd line. */
            int fh = dos_open(prog);
            if (fh >= 0) {
                dos_close(fh);
                run_batch_invoke(prog, tail);
                return;
            }
            /* .BAT not present at this candidate path -> try the next. */
            continue;
        }

        /* `tail` is the verbatim DOS command tail -> the child's PSP:80h
         * (initech-456). */
        err = dos_exec(prog, tail);
        if (err == 0u) {
            /* Clean run: control is back.  Latch the child's exit code into the
             * batch ERRORLEVEL (AH=4Dh) so a subsequent IF ERRORLEVEL sees it
             * (DOS sets ERRORLEVEL from the child's exit code). */
            g_errorlevel = dos_get_errorlevel();
            return;
        }
        /* Non-zero: file not found / bad format / load error -> try next. */
    }

    /* All candidates exhausted (or list empty) -> the canonical diagnostic. */
    dos_print(MSG_DOS_0002 "\r\n$");
}

/* Read one line from the keyboard via AH=0Ah into `out` (ASCIIZ, CR stripped).
 * The AH=0Ah block: byte0 = max incl CR (we preset it), byte1 = count written,
 * bytes2.. = chars + CR. We copy the `count` chars out and NUL-terminate. */
static void read_line(char *out)
{
    /* +2 for the AH=0Ah header (max + count) byte slots. */
    uint8_t blk[CMD_LINE_MAX + 2];
    uint8_t count;
    uint8_t i;

    blk[0] = (uint8_t)CMD_LINE_MAX;   /* max length including the CR */
    blk[1] = 0;
    dos_getline(blk);

    count = blk[1];
    for (i = 0; i < count && i < (uint8_t)(CMD_LINE_MAX - 1); i++) {
        out[i] = (char)blk[2 + i];
    }
    out[i] = '\0';
}

/* Parse one command `line` and dispatch it to the matching built-in or to
 * run_external.  This is the SHARED dispatch the interactive REPL AND the .BAT
 * interpreter (run_batch's BL_COMMAND path) both call, so internal built-ins
 * and external programs behave identically whether typed or scripted (beads
 * initech-xw1; the refactor split out of command_repl's old inline switch).
 *
 * Returns 1 if the line was EXIT (the shell should terminate -- DOS EXIT ends
 * COMMAND.COM whether typed at the prompt or reached inside a .BAT), else 0.
 * The caller (command_repl) returns on a 1; run_batch propagates the 1 up so a
 * batch EXIT tears down the whole shell (authentic DOS behaviour). */
static int dispatch_line(const char *line)
{
    cmd_line_t parsed;

    cmd_parse(line, &parsed);

    switch (parsed.kind) {
        case CMD_EMPTY:
            break;                       /* blank line -> nothing to do */
        case CMD_DIR:
            builtin_dir();
            break;
        case CMD_TYPE:
            builtin_type(parsed.arg);
            break;
        case CMD_CD:
            builtin_cd(parsed.arg);
            break;
        case CMD_MD:
            builtin_md(parsed.arg);
            break;
        case CMD_RD:
            builtin_rd(parsed.arg);
            break;
        case CMD_CLS:
            builtin_cls();
            break;
        case CMD_VER:
            builtin_ver();
            break;
        case CMD_ECHO:
            builtin_echo(parsed.arg);
            break;
        case CMD_BREAK:
            builtin_break(parsed.arg);
            break;
        case CMD_SET:
            builtin_set(parsed.arg);
            break;
        case CMD_PROMPT:
            builtin_prompt(parsed.arg);
            break;
        case CMD_COPY:
            builtin_copy(parsed.arg);
            break;
        case CMD_DEL:
            builtin_del(parsed.arg);
            break;
        case CMD_REN:
            builtin_ren(parsed.arg);
            break;
        case CMD_DATE:
            builtin_date(parsed.arg);
            break;
        case CMD_TIME:
            builtin_time(parsed.arg);
            break;
        case CMD_EXIT:
            /* Grep-able clean-exit marker. Plain '\n' (no CR) so the serial
             * line is exactly "SHELL-EXIT" -- the oracle's ^SHELL-EXIT$
             * anchored match would miss a trailing CR (the same serial-clean
             * discipline kmain uses for its markers). */
            dos_print("SHELL-EXIT\n$");
            g_shell_exit = 1;            /* tear-down flag (batch + REPL check it) */
            return 1;                    /* signal: the shell should exit */
        case CMD_EXTERNAL:
        default:
            run_external(parsed.command, parsed.tail);
            break;
    }
    return 0;
}

/* ---- the .BAT interpreter (beads initech-xw1) ----------------------------
 * run_batch reads a whole .BAT file into a buffer, splits it on CR/LF, and
 * interprets each line per DOS 3.3 batch semantics (MS-DOS 3.3 Tech Ref Ch.3):
 * ECHO state, @-suppression, parameter/%VAR% expansion, GOTO/:label, IF, FOR,
 * SHIFT, CALL, PAUSE, and plain commands (fed back through dispatch_line).
 * I/O lives HERE (the file read, the CON writes); the pure decision helpers
 * (batch_classify / batch_expand / batch_eval_if / batch_for_*) live in batch.c
 * and are host-tested by test_batch_exec.c (Law 2/Law 3 seam). */

/* The maximum bytes of a .BAT file we read into memory at once.  4 KiB covers
 * AUTOEXEC.BAT and ordinary scripts; a larger file is truncated at this bound
 * (fail-safe: we interpret what we read, never overrun). */
#define BATCH_FILE_MAX  4096

/* The CALL nesting cap (Rule 2: bound recursion so a self-CALL'ing .BAT cannot
 * blow the stack).  8 levels is far beyond any real DOS batch nest. */
#define BATCH_NEST_MAX  8

/* Per-shell batch nesting depth (incremented on entry to run_batch, decremented
 * on exit).  A CALL past BATCH_NEST_MAX is refused fail-loud. */
static int g_batch_depth = 0;

/* ONE shared file buffer (BSS, not the stack -- a 4 KiB buffer is far too large
 * to live on the kernel stack, Rule 2).  A CALL'd .BAT shares this single buffer
 * with its caller: the child overwrites it while it runs, and on return the
 * parent RE-READS its own file (run_batch reloads after each CALL).  This is how
 * real COMMAND.COM behaves -- it re-reads batch files from disk rather than
 * holding every nesting level resident -- and it keeps the batch footprint to a
 * single BATCH_FILE_MAX buffer instead of BATCH_NEST_MAX copies, which would
 * otherwise overrun the kernel window into PROGRAM_BASE (the kernel-end guard).
 * The per-level resume state (pos/len/argv view) lives on each run_batch frame's
 * C stack, so it survives the nested CALL untouched. */
static char g_batch_buf[BATCH_FILE_MAX];

/* EXIST probe for batch_eval_if: AH=3Dh OPEN the spec; on success close it and
 * report 1 (exists), else 0.  Matches the dos_open-based prober the bead spec
 * calls for.  `ctx` is unused (the file system is global). */
static int batch_exist_probe(const char *spec, void *ctx)
{
    int fh;
    (void)ctx;
    if (spec == 0 || spec[0] == '\0') {
        return 0;
    }
    fh = dos_open(spec);
    if (fh >= 0) {
        dos_close(fh);
        return 1;
    }
    return 0;
}

/* env_get adapter matching batch_env_lookup_fn: forward %VAR% lookups to the
 * shell's master environment.  `ctx` is the env_store_t*. */
static const char *batch_env_cb(const char *name, void *ctx)
{
    return env_get((const env_store_t *)ctx, name);
}

/* Echo a batch line to CON with the DOS prompt prefix (the rendered $P$G + the
 * line text + CRLF), as real COMMAND.COM does when ECHO is ON.  We reuse the
 * prompt-render path so the echoed prefix matches the interactive prompt. */
static void batch_echo_line(const char *text)
{
    const char *templ = env_get(&g_master_env, "PROMPT");
    char cwd_rel[INT21_CWD_MAX];
    prompt_ctx_t pctx;
    char rendered[INT21_CWD_MAX + 32];
    int rlen;

    if (templ == 0 || templ[0] == '\0') {
        templ = "$P$G";
    }
    dos_getcwd(0u, cwd_rel);
    pctx.drive = 'A';
    pctx.cwd   = cwd_rel;
    {
        uint8_t hh = 0, mi = 0, ss = 0, cs = 0;
        dos_gettime(&hh, &mi, &ss, &cs);
        pctx.hour = (int)hh; pctx.minute = (int)mi;
        pctx.second = (int)ss; pctx.centisec = (int)cs;
    }
    pctx.year = 0; pctx.month = 0; pctx.day = 0;

    rlen = cmd_render_prompt(templ, &pctx, rendered, (int)(sizeof(rendered) - 1));
    rendered[rlen] = '\0';
    dos_puts_raw(rendered);       /* the prompt prefix (e.g. "A:\>")            */
    dos_puts_raw(text);           /* the (expanded) command text                */
    dos_print("\r\n$");
}

/* Extract the line that starts at byte offset `*pos` in `buf` (length `len`)
 * into `out` (capacity BATCH_LINE_MAX), advance `*pos` past the line and its
 * EOL sequence (CR, LF, or CRLF), and return 1.  Return 0 at end of buffer
 * (out[0] = '\0').  An over-long line is truncated into `out` but `*pos` is
 * still advanced past the whole physical line (Rule 2: never bleed). */
static int batch_next_line(const char *buf, int len, int *pos, char *out)
{
    int i = *pos;
    int n = 0;

    out[0] = '\0';
    if (i >= len) {
        return 0;
    }
    while (i < len && buf[i] != '\r' && buf[i] != '\n' &&
           n < BATCH_LINE_MAX - 1) {
        out[n++] = buf[i++];
    }
    out[n] = '\0';
    /* Skip any remaining chars of an over-long line. */
    while (i < len && buf[i] != '\r' && buf[i] != '\n') {
        i++;
    }
    /* Consume the EOL sequence (CRLF, lone CR, or lone LF). */
    if (i < len && buf[i] == '\r') {
        i++;
    }
    if (i < len && buf[i] == '\n') {
        i++;
    }
    *pos = i;
    return 1;
}

/* Find the start offset of the first line whose label matches `target`,
 * scanning `buf` (length `len`) from the top.  Returns that line's byte offset
 * (the position to resume interpretation from), or -1 if no matching :label is
 * found.  Used for GOTO (DOS rescans the whole file from the top). */
static int batch_find_label(const char *buf, int len, const char *target)
{
    int pos = 0;
    char line[BATCH_LINE_MAX];
    while (pos < len) {
        int line_start = pos;
        if (!batch_next_line(buf, len, &pos, line)) {
            break;
        }
        if (batch_label_matches(line, target)) {
            return line_start;
        }
    }
    return -1;
}

/* Read the whole .BAT at `path` into `buf` (capacity BATCH_FILE_MAX), returning
 * the byte count, or -1 if the file cannot be opened.  A file larger than the
 * bound is truncated (fail-safe: we interpret what we read, never overrun).
 * run_batch calls this once on entry AND again after every CALL -- the nested
 * CALL shares g_batch_buf, so the parent must reload its own contents before
 * resuming (its pos/len survive on the stack; only the shared bytes were lost). */
static int batch_load_file(const char *path, char *buf)
{
    int fh = dos_open(path);
    uint32_t total = 0;
    if (fh < 0) {
        return -1;
    }
    for (;;) {
        uint32_t got;
        if (total >= (uint32_t)BATCH_FILE_MAX) {
            break;   /* truncate at the bound (fail safe) */
        }
        got = dos_read(fh, (uint8_t *)(buf + total),
                       (uint32_t)BATCH_FILE_MAX - total);
        if (got == 0u) {
            break;   /* EOF */
        }
        total += got;
    }
    dos_close(fh);
    return (int)total;
}

/* Resolve a CALL'd batch name (`word`, e.g. "SETUP" or "SETUP.BAT") to a
 * concrete EXISTING .BAT file path, written into `out` (capacity
 * CMD_PATH_MAX_LEN).  Plans the CWD/PATH candidates (the same planner the
 * external-command search uses), probes each .BAT candidate with AH=3Dh OPEN,
 * and returns 1 with the first hit in `out`, or 0 if none exists.  `word` is
 * assumed upper-cased.  Ref: DOS 3.3 CALL resolves like an external command. */
static int resolve_bat_path(const char *word, char *out)
{
    cmd_path_iter_t cands;
    char cwd[3 + INT21_CWD_MAX];
    const char *path_val;
    int i;

    cwd_display(cwd);
    path_val = env_get(&g_master_env, "PATH");
    cmd_path_candidates(word, path_val, cwd, &cands);

    for (i = 0; i < cands.count; i++) {
        char *prog = cands.entries[i];
        cmd_upcase_str(prog);
        if (!bat_ends_with_bat(prog)) {
            continue;   /* CALL targets are batch files */
        }
        {
            int fh = dos_open(prog);
            if (fh >= 0) {
                int n = 0;
                dos_close(fh);
                while (prog[n] != '\0' && n < CMD_PATH_MAX_LEN - 1) {
                    out[n] = prog[n];
                    n++;
                }
                out[n] = '\0';
                return 1;
            }
        }
    }
    return 0;
}

/* Interpret one .BAT file.
 *
 *   `path` -- the ASCIIZ file path (already a resolved candidate, upcased).
 *   `argv` -- argv[0] = the batch name (for %0), argv[1..9] = the positional
 *             parameters; `argc` is the count.  Any argv[i] may be NULL.
 *
 * Reads the whole file (bounded by BATCH_FILE_MAX), then walks it line by line.
 * SHIFT is tracked as an argv offset (argv[1+shift]..); GOTO rescans from the
 * top; CALL recurses (depth-capped); EXIT sets g_shell_exit and unwinds.  ECHO
 * state starts ON (DOS default) but AUTOEXEC.BAT typically flips it OFF on the
 * first line.  Ref: MS-DOS 3.3 Tech Ref Ch.3; beads initech-xw1. */
static void run_batch(const char *path, const char *const argv[], int argc)
{
    char *buf;
    int   len;
    int   pos = 0;
    int   echo_on = 1;     /* ECHO defaults ON at batch entry (DOS 3.3)        */

    /* A LOCAL, shiftable view of the positional parameters.  SHIFT slides the
     * %1.. window left (argv[0]/%0 is unaffected in DOS SHIFT); we mutate this
     * view, never the caller's array.  view[0] = %0 (batch name); view[1..] =
     * the current %1.. parameters; vcount is the live count. */
    const char *view[10];
    int   vcount = (argc > 10) ? 10 : argc;
    {
        int k;
        for (k = 0; k < vcount; k++) {
            view[k] = argv[k];
        }
    }

    /* Depth cap (Rule 2): refuse runaway CALL recursion. */
    if (g_batch_depth >= BATCH_NEST_MAX) {
        dos_print("Batch nesting too deep\r\n$");
        return;
    }
    buf = g_batch_buf;          /* shared across nesting; reloaded after CALL  */
    g_batch_depth++;

    /* ---- read the whole file into the shared buffer --------------------- */
    len = batch_load_file(path, buf);
    if (len < 0) {
        /* Should not happen (the caller probed existence), but fail safe. */
        g_batch_depth--;
        return;
    }

    /* ---- interpret line by line ----------------------------------------- */
    while (pos < len) {
        char line[BATCH_LINE_MAX];
        char expanded[BATCH_LINE_MAX];
        batch_parsed_t bp;

        if (!batch_next_line(buf, len, &pos, line)) {
            break;
        }

        batch_classify(line, &bp);

        switch (bp.kind) {
            case BL_BLANK:
            case BL_REM:
            case BL_LABEL:
                /* No execution, no echo (REM/LABEL/blank are silent). */
                break;

            case BL_ECHO_ON:
                echo_on = 1;
                break;

            case BL_ECHO_OFF:
                echo_on = 0;
                break;

            case BL_ECHO_TEXT: {
                /* ECHO <text>: expand the text, then print it (ECHO text is
                 * always printed regardless of the ECHO ON/OFF state -- it is a
                 * command, not the line-echo).  Bare ECHO reports the state. */
                if (bp.echo_text[0] == '\0') {
                    dos_print(echo_on ? "ECHO is on\r\n$" : "ECHO is off\r\n$");
                } else {
                    if (batch_expand(bp.echo_text, view, vcount,
                                     batch_env_cb, &g_master_env,
                                     expanded, BATCH_LINE_MAX) >= 0) {
                        dos_puts_raw(expanded);
                    }
                    dos_print("\r\n$");
                }
                break;
            }

            case BL_GOTO: {
                int target = batch_find_label(buf, len, bp.goto_target);
                if (target < 0) {
                    /* DOS: "Label not found" aborts the batch. */
                    dos_print("Label not found\r\n$");
                    pos = len;       /* stop interpreting */
                } else {
                    pos = target;    /* resume from the label line */
                }
                break;
            }

            case BL_IF: {
                /* Expand the whole line first (so %1 / %VAR% resolve), strip the
                 * leading "IF " keyword, evaluate the condition, and on TRUE
                 * dispatch the remainder.  We expand the raw line (minus any '@')
                 * to keep quotes intact for the str==str form. */
                const char *raw = line;
                const char *cond = 0;
                if (bp.at_suppressed) {
                    /* skip the leading '@' (and ws) we already classified past */
                    while (*raw == ' ' || *raw == '\t') raw++;
                    if (*raw == '@') raw++;
                }
                if (batch_expand(raw, view, vcount, batch_env_cb, &g_master_env,
                                 expanded, BATCH_LINE_MAX) >= 0) {
                    /* Skip leading ws + the "IF" keyword + its trailing ws. */
                    const char *q = expanded;
                    while (*q == ' ' || *q == '\t') q++;
                    /* q now points at "IF..."; advance past "IF". */
                    if ((q[0] == 'I' || q[0] == 'i') &&
                        (q[1] == 'F' || q[1] == 'f') &&
                        (q[2] == ' ' || q[2] == '\t')) {
                        q += 2;
                        while (*q == ' ' || *q == '\t') q++;
                        if (batch_eval_if(q, g_errorlevel,
                                          batch_exist_probe, 0, &cond) &&
                            cond != 0 && cond[0] != '\0') {
                            /* Run the conditional body through the shared
                             * dispatch.  An EXIT inside it sets g_shell_exit,
                             * which the loop checks at the bottom to tear down. */
                            (void)dispatch_line(cond);
                        }
                    }
                }
                break;
            }

            case BL_FOR: {
                /* Expand the line, strip "FOR ", parse %%v IN (set) DO cmd, then
                 * iterate the set substituting %%v in cmd and dispatching each. */
                const char *raw = line;
                if (bp.at_suppressed) {
                    while (*raw == ' ' || *raw == '\t') raw++;
                    if (*raw == '@') raw++;
                }
                if (batch_expand(raw, view, vcount, batch_env_cb, &g_master_env,
                                 expanded, BATCH_LINE_MAX) >= 0) {
                    const char *q = expanded;
                    while (*q == ' ' || *q == '\t') q++;
                    if ((q[0] == 'F' || q[0] == 'f') &&
                        (q[1] == 'O' || q[1] == 'o') &&
                        (q[2] == 'R' || q[2] == 'r') &&
                        (q[3] == ' ' || q[3] == '\t')) {
                        char var[BATCH_LABEL_MAX];
                        char set[BATCH_LINE_MAX];
                        char tmpl[BATCH_LINE_MAX];
                        q += 3;
                        while (*q == ' ' || *q == '\t') q++;
                        if (batch_for_parse(q, var, set, tmpl)) {
                            int cur = 0;
                            char tok[BATCH_LINE_MAX];
                            while (!g_shell_exit &&
                                   batch_for_next_token(set, &cur, tok,
                                                        BATCH_LINE_MAX)) {
                                char cmd[BATCH_LINE_MAX];
                                if (batch_for_subst(tmpl, var, tok, cmd,
                                                    BATCH_LINE_MAX) >= 0) {
                                    dispatch_line(cmd);
                                }
                            }
                        }
                    }
                }
                break;
            }

            case BL_SHIFT:
                /* Shift the positional params left by one: %1 <- %2, %2 <- %3,
                 * ...  %0 (the batch name) is unaffected in DOS SHIFT.  We slide
                 * the LOCAL view, never the caller's array (Rule 3: no aliasing
                 * surprises).  Ref: MS-DOS 3.3 Tech Ref Ch.3 SHIFT. */
                if (vcount > 1) {
                    int k;
                    for (k = 1; k < vcount - 1; k++) {
                        view[k] = view[k + 1];
                    }
                    vcount--;
                }
                break;

            case BL_CALL: {
                /* CALL <batchfile> [args]: nested run_batch.  Expand the line,
                 * strip "CALL", take the first token as the .BAT path, and frame
                 * the remainder as its tail.  Depth is capped inside run_batch. */
                const char *raw = line;
                if (bp.at_suppressed) {
                    while (*raw == ' ' || *raw == '\t') raw++;
                    if (*raw == '@') raw++;
                }
                if (batch_expand(raw, view, vcount, batch_env_cb, &g_master_env,
                                 expanded, BATCH_LINE_MAX) >= 0) {
                    const char *q = expanded;
                    while (*q == ' ' || *q == '\t') q++;
                    if ((q[0] == 'C' || q[0] == 'c') &&
                        (q[1] == 'A' || q[1] == 'a') &&
                        (q[2] == 'L' || q[2] == 'l') &&
                        (q[3] == 'L' || q[3] == 'l') &&
                        (q[4] == ' ' || q[4] == '\t')) {
                        char callee[CMD_TOKEN_MAX];
                        int  cn = 0;
                        q += 4;
                        while (*q == ' ' || *q == '\t') q++;
                        /* First token = the callee .BAT name. */
                        while (*q != '\0' && *q != ' ' && *q != '\t' &&
                               cn < CMD_TOKEN_MAX - 1) {
                            callee[cn++] = *q++;
                        }
                        callee[cn] = '\0';
                        while (*q != '\0' && *q != ' ' && *q != '\t') {
                            q++;   /* swallow an over-long name */
                        }
                        /* The remainder (with its leading separator) is the
                         * callee's command tail -> its %1.. parameters. */
                        if (callee[0] != '\0') {
                            char resolved[CMD_PATH_MAX_LEN];
                            cmd_upcase_str(callee);
                            /* CALL targets are resolved relative to the CWD/PATH
                             * like any external; reuse the candidate planner to
                             * append .BAT and find the file. */
                            if (resolve_bat_path(callee, resolved)) {
                                run_batch_invoke(resolved, q);
                                /* The nested CALL shared g_batch_buf, so it
                                 * overwrote our contents.  Reload THIS file so
                                 * the loop resumes correctly (our pos/len/view
                                 * survived on the stack; only the bytes were
                                 * lost).  DOS likewise re-reads on CALL return.
                                 * If the file vanished (len<0), the `pos < len`
                                 * loop guard exits cleanly on the next turn. */
                                len = batch_load_file(path, buf);
                            } else {
                                dos_print(MSG_DOS_0002 "\r\n$");
                            }
                        }
                    }
                }
                break;
            }

            case BL_PAUSE:
                /* DOS PAUSE: print the controlled message (MSG-DOS-0016,
                 * "Press any key to continue . . .") and wait for one keystroke.
                 * AH=08h (char input, no echo) is the DOS PAUSE read; in the
                 * headless oracle the --keys injection satisfies it. */
                dos_print(MSG_DOS_0016 "\r\n$");
                (void)dos_conin();
                break;

            case BL_COMMAND:
            default: {
                /* A plain command line: echo it (when ECHO is ON and the line is
                 * not '@'-suppressed), expand %0..%9 / %VAR%, and dispatch it
                 * through the SAME path the interactive REPL uses. */
                const char *raw = line;
                if (bp.at_suppressed) {
                    while (*raw == ' ' || *raw == '\t') raw++;
                    if (*raw == '@') raw++;
                }
                if (batch_expand(raw, view, vcount, batch_env_cb, &g_master_env,
                                 expanded, BATCH_LINE_MAX) >= 0) {
                    if (echo_on && !bp.at_suppressed) {
                        batch_echo_line(expanded);
                    }
                    dispatch_line(expanded);
                }
                break;
            }
        }

        /* EXIT (typed in a dispatched line, or reached via IF/FOR/CALL) tears
         * the whole shell down: stop this batch and propagate up. */
        if (g_shell_exit) {
            break;
        }
    }

    g_batch_depth--;
}

/* Build a .BAT argv from a command word + a DOS command tail and run it.
 *
 *   `path` -- the resolved .BAT file path (used to OPEN/READ; also %0).
 *   `tail` -- the verbatim DOS command tail (leading separator + params), or
 *             NULL/"".  Tokenized into up to 9 positional parameters (%1..%9).
 *
 * The argv strings are copied into a local buffer (bounded), so the batch can
 * read %0..%9 without aliasing the caller's `tail`.  Ref: beads initech-xw1. */
static void run_batch_invoke(const char *path, const char *tail)
{
    /* argv[0] = path (%0); argv[1..9] = up to 9 params from the tail. */
    char        slots[10][CMD_TOKEN_MAX];
    const char *argv[10];
    int         argc = 1;
    const char *p = tail;
    int         i;

    /* %0 = the batch path (DOS exposes the script name as %0). */
    {
        int n = 0;
        const char *s = path;
        while (s != 0 && *s != '\0' && n < CMD_TOKEN_MAX - 1) {
            slots[0][n] = *s++;
            n++;
        }
        slots[0][n] = '\0';
    }
    argv[0] = slots[0];

    /* Tokenize the tail into %1..%9 (whitespace-delimited). */
    if (p != 0) {
        for (i = 1; i <= 9 && *p != '\0'; i++) {
            int n = 0;
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            if (*p == '\0') {
                break;
            }
            while (*p != '\0' && *p != ' ' && *p != '\t' &&
                   n < CMD_TOKEN_MAX - 1) {
                slots[i][n++] = *p++;
            }
            slots[i][n] = '\0';
            /* swallow an over-long token's tail */
            while (*p != '\0' && *p != ' ' && *p != '\t') {
                p++;
            }
            argv[argc++] = slots[i];
        }
    }
    /* Fill any unused slots with NULL (out-of-range %n -> empty, DOS 3.3). */
    for (i = argc; i < 10; i++) {
        argv[i] = 0;
    }

    run_batch(path, argv, argc);
}

void command_repl(void)
{
    char line[CMD_LINE_MAX];

    /* Seed the master environment once at REPL entry (beads initech-1i0x
     * Tranche E inc 2). These three variables match DOS 3.3 startup defaults
     * for a single-floppy system. Overflow is impossible here (well under 512
     * bytes), but we follow fail-loud discipline (Rule 2) with no-op silence:
     * env_set returns 0 on overflow; the REPL continues regardless (the OS
     * must boot even if seeding somehow fails on a future tiny-arena config). */
    env_init(&g_master_env);
    env_set(&g_master_env, "COMSPEC", "A:\\COMMAND.COM");
    env_set(&g_master_env, "PROMPT",  "$P$G");
    env_set(&g_master_env, "PATH",    "");

    /* AUTOEXEC.BAT (beads initech-xw1): on startup, real COMMAND.COM runs
     * A:\AUTOEXEC.BAT once before the first interactive prompt.  Probe for it
     * with AH=3Dh OPEN; if present, interpret it through run_batch (argv[0] =
     * the path, no positional params).  A typical AUTOEXEC begins "@ECHO OFF"
     * and sets PATH/PROMPT, so its side effects (env, prompt) persist into the
     * REPL because run_batch dispatches SET/PROMPT against g_master_env.  If
     * AUTOEXEC.BAT runs an EXIT it tears the shell down (g_shell_exit) -- DOS
     * behaviour -- so we re-check the flag before entering the loop.
     * Ref: MS-DOS 3.3 Tech Ref Ch.3; spec/dos_autoexec_bat_baseline.txt. */
    {
        const char *autoexec = "A:\\AUTOEXEC.BAT";
        int fh = dos_open(autoexec);
        if (fh >= 0) {
            dos_close(fh);
#ifndef CMD_MUTATE_NO_AUTOEXEC
            run_batch_invoke(autoexec, "");
#else
            /* MUTANT (Rule 6; make test-autoexec-mutant only): skip running
             * AUTOEXEC.BAT entirely, so its markers never reach serial and the
             * emu gate's marker assertions go RED -- proving the gate bites.
             * NEVER in a real build. */
            (void)autoexec;
#endif
        }
    }
    if (g_shell_exit) {
        return;     /* AUTOEXEC.BAT issued EXIT -> the shell is done */
    }

    /* A serial marker the oracle gates key-injection on (mirrors KBD-ECHO-READY
     * / CONIN-PROG-READY). The kernel's CON sink also fans this to serial via
     * the AH=09h banner, but a distinct grep-able marker keeps the gate robust;
     * kmain prints SHELL-READY before calling us, so we don't duplicate it. */
    for (;;) {
        /* $P$G prompt: read the PROMPT env var, build a prompt_ctx_t from the
         * live CWD (AH=47h) + clock (AH=2Ch), expand via cmd_render_prompt, then
         * print via AH=09h.  This replaces the old cwd_display+'>'+$' approach;
         * the default "$P$G" produces byte-identical "A:\>" / "A:\SUB>" output
         * so the test-shell prompt-band assertions continue to pass (DEC-12).
         * Ref: ADR-0003 DEC-12; beads initech-dibc. */
#ifdef CMD_MUTATE_NO_CWD_PROMPT
        /* MUTANT (Rule 6; make test-ut6d-mutant only): pin the prompt to the
         * root "A:\>" regardless of the CWD, so after `CD SUB` the prompt never
         * shows "A:\SUB>" -> the test-ut6d subdir-prompt assertion goes RED.
         * NEVER in a real build. */
        dos_print("A:\\>$");
#else
        {
            /* Retrieve the PROMPT template from the master environment.
             * Fall back to "$P$G" if the variable is absent/empty (a defensive
             * guard: command_repl seeds it to "$P$G" above, but a PROMPT CLEAR
             * via `SET PROMPT=` would leave env_get returning ""; in that case
             * DOS 3.3 also falls back to the bare drive letter, but "$P$G" is
             * more useful and matches the seeded default -- Law 4). */
            const char *templ = env_get(&g_master_env, "PROMPT");
            if (templ == 0 || templ[0] == '\0') {
                templ = "$P$G";
            }

            /* Build the prompt_ctx_t: CWD from AH=47h, time from AH=2Ch.
             * Date fields are populated for $D but are not needed for $P$G.
             * cwd_rel is the AH=47h root-relative path (no drive, no leading \):
             * "" at root, "SUB" for A:\SUB. */
            char cwd_rel[INT21_CWD_MAX];
            dos_getcwd(0u, cwd_rel);    /* DL=0 -> default drive (A:) */

            prompt_ctx_t pctx;
            pctx.drive    = 'A';
            pctx.cwd      = cwd_rel;   /* root-relative, passed to $P expander */
            /* Seed time fields (AH=2Ch); date fields default to 0 for $D if
             * $D is not in the template -- the REPL seeds them cheaply. */
            {
                uint8_t hh = 0, mi = 0, ss = 0, cs = 0;
                dos_gettime(&hh, &mi, &ss, &cs);
                pctx.hour     = (int)hh;
                pctx.minute   = (int)mi;
                pctx.second   = (int)ss;
                pctx.centisec = (int)cs;
            }
            pctx.year  = 0;
            pctx.month = 0;
            pctx.day   = 0;

            /* Render the prompt into a scratch buffer.  Size: INT21_CWD_MAX (64)
             * covers the CWD component; we add slack for the drive/colon/arrow
             * plus time/date fields and other metacharacter expansions.  The
             * rendered string is then printed via AH=09h (DOS '$'-terminated
             * string), so we append '$' after the NUL position. */
            char rendered[INT21_CWD_MAX + 32];  /* generous; no overflow possible */
            int rlen = cmd_render_prompt(templ, &pctx,
                                         rendered, (int)(sizeof(rendered) - 1));
            rendered[rlen++] = '$';    /* AH=09h '$'-terminator */
            rendered[rlen]   = '\0';
            dos_print(rendered);
        }
#endif

        read_line(line);

        /* Dispatch through the SHARED path the .BAT interpreter also uses
         * (beads initech-xw1).  A return value of 1 means EXIT was typed: print
         * was already emitted in dispatch_line; tear the REPL down. */
        if (dispatch_line(line)) {
            return;
        }
    }
}

#endif /* COMMAND_KERNEL_REPL */
