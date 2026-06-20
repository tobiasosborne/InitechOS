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

/* External command: append .COM (if no extension), upcase, AH=4Bh EXEC. On a
 * not-found / load failure => the controlled "Bad command or file name". On a
 * clean run control simply returns to the prompt. */
static void run_external(const char *command, const char *tail)
{
    char prog[CMD_TOKEN_MAX];
    uint16_t err;

    /* `command` is already upper-cased by cmd_parse. Append .COM if needed. */
    if (!cmd_append_com(command, prog)) {
        dos_print(MSG_DOS_0002 "\r\n$");
        return;
    }
    /* Defensive: upcase again (cmd_append_com preserves what it was given). */
    cmd_upcase_str(prog);

    /* `tail` is the verbatim DOS command tail -> the child's PSP:80h (initech-456). */
    err = dos_exec(prog, tail);
    if (err != 0u) {
        /* Not found / bad format / nested -> the canonical DOS diagnostic. */
        dos_print(MSG_DOS_0002 "\r\n$");
        return;
    }
    /* Clean run: control is back; the next prompt follows. */
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

void command_repl(void)
{
    char line[CMD_LINE_MAX];
    cmd_line_t parsed;

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
        cmd_parse(line, &parsed);

        switch (parsed.kind) {
            case CMD_EMPTY:
                break;                       /* blank line -> just re-prompt */
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
                return;
            case CMD_EXTERNAL:
            default:
                run_external(parsed.command, parsed.tail);
                break;
        }
    }
}

#endif /* COMMAND_KERNEL_REPL */
