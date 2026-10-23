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
    /* Reference pp.46-47. COMMAND and COMMAND.COM name the native secondary
     * entry; a foreign 16-bit interpreter is never executed (DEC-08). */
    const char *s=upper_command;
    if (s[0]=='C' && s[1]=='O' && s[2]=='M' && s[3]=='M' &&
        s[4]=='A' && s[5]=='N' && s[6]=='D' &&
        (!s[7] || (s[7]=='.' && s[8]=='C' && s[9]=='O' && s[10]=='M' && !s[11])))
        return CMD_COMMAND;
    /* A tiny hand-rolled strcmp keeps this freestanding (no libc). */
    struct { const char *name; cmd_kind_t kind; } table[] = {
        { "COMMAND", CMD_COMMAND }, /* Reference pp.46-47; the early check above
                                      * also admits COMMAND.COM -- the row keeps
                                      * the shipped-command inventory honest. */
        { "DIR",   CMD_DIR  },
#ifndef CMD_MUTATE_PATH_COMMAND
        { "PATH",  CMD_PATH }, /* MS-DOS 3.3 Reference p.97 / PDF182. */
#endif
#ifndef DU_MUTATE_VOL
        { "VOL",   CMD_VOL  }, /* MS-DOS 3.3 User's Guide p.126 / PDF211 */
#endif
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
        /* MS-DOS 3.3 Reference p.57 (DIR[/P][/W]), p.97 (PATH;),
         * Guide p.48 (PATH=...). Preserve the adjacent delimiter as the
         * operand; executable path names retain the ordinary tokenizer. */
        if ((*p == '/' && i == 3 && out->command[0] == 'D' &&
             out->command[1] == 'I' && out->command[2] == 'R') ||
            ((*p == ';' || *p == '=') && i == 4 && out->command[0] == 'P' &&
             out->command[1] == 'A' && out->command[2] == 'T' && out->command[3] == 'H'))
            break;
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
    if (i == CMD_TOKEN_MAX - 1)
        while (*p && !is_space(*p)) p++;

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

static void put_text(char *out, int *pos, const char *s)
{
    while (*s) out[(*pos)++] = *s++;
}

#ifndef CMD_MUTATE_DIR_NO_DATE
static void put_two(char *out, int *pos, unsigned n)
{
    out[(*pos)++] = (char)('0' + n/10u);
    out[(*pos)++] = (char)('0' + n%10u);
}
#endif

/* Manual p.28 / PDF 41, measured zero-based columns documented in the
 * independent dos33_dir.golden. Tech Ref pp.5-12..13 / PDF 119..120
 * defines packed words; the displayed noon example is 12:00p. */
int cmd_format_dir_line(const char *fname, uint32_t fsize, uint8_t attr,
                        uint16_t fdate, uint16_t ftime, char *out)
{
    int pos = 0;
    int i = 0;
    const char *ext = 0;
    /* '.' and '..' are names in their own right, never extensions. */
    if (fname[0] != '.') {
        for (const char *p=fname; *p; p++) if (*p=='.') { ext=p+1; break; }
    }
    while (fname[i] && i<8 && (!ext || fname+i<ext-1)) { out[pos++]=fname[i++]; }
    while (pos<9) out[pos++]=' ';
    i=0;
    if (ext) while (ext[i] && i<3) out[pos++]=ext[i++];
    while (pos<13) out[pos++]=' ';
#ifdef CMD_MUTATE_DIR_JOINED
    for (i=0;i<13;i++) out[i]=' ';
    for (i=0;fname[i] && i<12;i++) out[i]=fname[i];
#endif
    if (attr & DIR_ATTR_DIRECTORY) {
        put_text(out,&pos,"   " MSG_DOS_0026);
    } else {
        put_u_field(out, &pos, fsize, 8);
    }
#ifndef CMD_MUTATE_DIR_NO_DATE
    put_text(out,&pos,"  ");
    put_u_field(out,&pos,(fdate>>5)&15u,2);
    out[pos++]='-'; put_two(out,&pos,fdate&31u);
    out[pos++]='-'; put_two(out,&pos,(1980u+(fdate>>9))%100u);
    put_text(out,&pos,"  ");
    unsigned hour=(ftime>>11)&31u;
    put_u_field(out,&pos,hour%12u ? hour%12u : 12u,2);
    out[pos++]=':'; put_two(out,&pos,(ftime>>5)&63u);
    out[pos++]=hour>=12u ? 'p' : 'a';
#else
    (void)fdate; (void)ftime;
#endif
    out[pos++] = '\n';
    out[pos]   = '\0';
    return pos;
}

int cmd_format_dir_footer(uint32_t files, uint32_t free_bytes, char *out)
{
    int pos=0;
    put_u_field(out,&pos,files,9);
    put_text(out,&pos,MSG_DOS_0024);
#ifdef CMD_MUTATE_DIR_LOWERCASE
    out[10]='f';
#endif
    /* p.29 / PDF 42: count ends at 8, File(s) starts at 10, free-byte
     * number ends at 26 (the scanned illustration is the layout source). */
    put_u_field(out,&pos,free_bytes,9);
    put_text(out,&pos,MSG_DOS_0025);
    out[pos++]='\n'; out[pos]=0;
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

/* MS-DOS 3.3 Reference pp.50-52: + joins sources; /A,/B apply to the
 * preceding operand and persist; /V is global. Parse fully before any I/O.
 * Offsets into one command-sized arena avoid a source-count filesystem cap. */
int cmd_copy_parse(const char *arg, cmd_copy_t *p)
{
    unsigned used=0, mode=0; int need=1, last=-1;
    p->count=0; p->dest[0]=0; p->dest_mode=0; p->verify=0; p->joined=0;
    while (*arg) {
        while (is_space(*arg)) arg++;
        if (!*arg) break;
        if (*arg=='/') {
            char c=cmd_upcase_char(*++arg);
            if (!c) return 0;
            arg++;
            if (*arg && !is_space(*arg) && *arg!='/' && *arg!='+') return 0;
            if (c=='V') { p->verify=1; continue; }
            if (c!='A' && c!='B') return 0;
            mode=c=='A' ? 1u:2u;
#ifdef CMD_MUTATE_COPY_SWITCH
            mode=0;
#endif
            if(last>=0) p->mode[last]=(uint8_t)mode;
            else if(last==-2) p->dest_mode=(uint8_t)mode;
            continue;
        }
        if (*arg=='+') {
            if (need || p->dest[0]) return 0;
            need=1; p->joined=1; arg++; continue;
        }
        char token[CMD_TOKEN_MAX]; unsigned n=0;
        while(*arg && !is_space(*arg) && *arg!='+' && *arg!='/') {
            if(n+1>=sizeof token)return 0;
            token[n++]=cmd_upcase_char(*arg++);
        }
        token[n]=0;
        if(!n)return 0;
        if(need) {
            if(p->count>=CMD_COPY_MAX || used+n+1>sizeof p->names)return 0;
            last=(int)p->count;
            p->offset[p->count]=(uint8_t)used;
            p->mode[p->count++]=(uint8_t)mode;
            for(unsigned i=0;i<=n;i++)p->names[used++]=token[i];
            need=0;
        } else {
            if(p->dest[0])return 0;
            for(unsigned i=0;i<=n;i++)p->dest[i]=token[i];
            p->dest_mode=(uint8_t)mode; last=-2;
        }
    }
    return p->count && !need;
}

/* Reference p.104: wildcard output positions retain the source character.
 * Operate on independent 8/3 padded fields, then strip trailing pad spaces. */
int cmd_name_map(const char *source,const char *pattern,char out[13])
{
    char src[11],dst[11]; unsigned field=0,pos=0;
    for(unsigned i=0;i<11;i++)src[i]=dst[i]=' ';
    for(const char *s=source;*s;s++) {
        if(*s=='.'){field=8;pos=0;continue;}
        if(pos>=(field?3u:8u))return 0;
        src[field+pos++]=cmd_upcase_char(*s);
    }
    field=0;pos=0;
    for(const char *s=pattern;*s;s++) {
        if(*s=='.'){field=8;pos=0;continue;}
        unsigned cap=field?3u:8u;
        if(*s=='*') {
            while(pos<cap){dst[field+pos]=src[field+pos];pos++;}
            while(s[1] && s[1]!='.')s++;
            continue;
        }
        if(pos>=cap)return 0;
#ifdef CMD_MUTATE_REN_MAP
        dst[field+pos]=*s=='?' ? 'X':cmd_upcase_char(*s);
#else
        dst[field+pos]=*s=='?' ? src[field+pos]:cmd_upcase_char(*s);
#endif
        pos++;
    }
    unsigned n=0,end=8;
    while(end && dst[end-1]==' ')end--;
    if(!end)return 0;
    for(unsigned i=0;i<end;i++)out[n++]=dst[i];
    end=11;while(end>8 && dst[end-1]==' ')end--;
    if(end>8){out[n++]='.';for(unsigned i=8;i<end;i++)out[n++]=dst[i];}
    out[n]=0;return 1;
}

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

/* Legacy pure text predicate (kept for the existing test-command oracle).
 * COPY's data-preservation guard uses int21_same_file on OPEN handles instead;
 * this incomplete text predicate must NEVER authorize a destructive create.
 * cmd_same_file: 1 iff operands `a` and `b` PROVABLY name the SAME file, judged
 * from the operand TEXT alone. This drives COPY's real-DOS-3.3 "File cannot be
 * copied onto itself" guard (bead initech-ojxn): a false POSITIVE would refuse a
 * legitimate copy, so the predicate NEVER reports "same" unless it can prove it;
 * a false NEGATIVE on an exotic path form is a documented residual (follow-up
 * bead), NOT a wrong refusal. InitechDOS is single-drive (A:) and exposes no
 * AH=60h TRUENAME to the shell (only AH=47h GET-CWD + a CHDIR-scoped directory
 * canon exist), so canonicalization here is text-level:
 *   - strip a leading drive prefix "X:"; the default (no prefix) drive is 'A';
 *     if the two drives DIFFER the operands are on different volumes -> 0;
 *   - strip a leading ".\\" current-directory prefix;
 *   - if NEITHER remainder has a path separator '\\' (both are bare CWD leaves),
 *     fold each to its 11-byte 8.3 key (NAME padded to 8 + EXT padded to 3, the
 *     on-disk FAT form) and compare -> catches case, FOO vs FOO., A:FOO vs FOO,
 *     FOO vs .\\FOO;
 *   - else compare the remainders VERBATIM (case already folded by the caller) ->
 *     catches identical subdir paths (SUB\\A.TXT vs SUB\\A.TXT), but NOT
 *     relative-vs-absolute (\\FOO vs FOO) -- the residual (follow-up bead).
 * Ref: bead initech-ojxn; DOS 3.3 COMMAND.COM COPY same-name check. PURE. */
#ifdef CMD_MUTATE_NO_SAMEFILE
/* MUTANT (Rule 6; make test-command-mutant only): the same-file predicate always
 * reports "different", so COPY's onto-itself guard never fires -> the same-file
 * detection test goes RED, AND (in the kernel) COPY FOO.TXT FOO.TXT would
 * dos_creat/TRUNCATE the source to zero -- the EXACT initech-ojxn data-loss
 * regression. NEVER in a real build. */
int cmd_same_file(const char *a, const char *b)
{
    (void)a;
    (void)b;
    return 0;
}
#else
/* Skip a leading drive prefix "X:"; write the (upcased) drive letter to *drive
 * ('A' when none is present -- the single InitechDOS volume) and return the
 * pointer past the prefix. */
static const char *cmd_skip_drive(const char *s, char *drive)
{
    if (s[0] != '\0' && s[1] == ':') {
        *drive = cmd_upcase_char(s[0]);
        return s + 2;
    }
    *drive = 'A';
    return s;
}

/* Skip a single leading ".\\" current-directory prefix (DOS path sep is '\\'). */
static const char *cmd_skip_dotslash(const char *s)
{
    if (s[0] == '.' && s[1] == '\\') {
        return s + 2;
    }
    return s;
}

/* 1 if `s` contains a path separator '\\' (so it is NOT a bare CWD leaf). */
static int cmd_has_sep(const char *s)
{
    for (; *s; s++) {
        if (*s == '\\') {
            return 1;
        }
    }
    return 0;
}

/* Fold a bare 8.3 leaf into its 11-byte on-disk key: NAME (<=8, space-padded) +
 * EXT (<=3, space-padded), split on the FIRST '.', UPCASED (DOS is case-blind).
 * Overlong NAME/EXT are truncated exactly as the FAT resolver stores them, so
 * FOO, FOO. and FOO.<junk> fold identically only where the 8.3 volume would
 * agree. `key` holds 11 bytes. */
static void cmd_fold_83(const char *s, char key[11])
{
    int i = 0;
    int k;

    for (k = 0; k < 11; k++) {
        key[k] = ' ';
    }
    /* NAME: up to 8 chars until '.' or end. */
    while (s[i] != '\0' && s[i] != '.' && i < 8) {
        key[i] = cmd_upcase_char(s[i]);
        i++;
    }
    /* Swallow the tail of an overlong NAME up to the '.'. */
    while (s[i] != '\0' && s[i] != '.') {
        i++;
    }
    if (s[i] == '.') {
        int e = 0;
        i++;                        /* past the '.' */
        while (s[i] != '\0' && s[i] != '.' && e < 3) {
            key[8 + e] = cmd_upcase_char(s[i]);
            i++;
            e++;
        }
    }
}

int cmd_same_file(const char *a, const char *b)
{
    char da;
    char db;
    const char *pa;
    const char *pb;
    int k;

    if (a == 0 || b == 0) {
        return 0;
    }
    pa = cmd_skip_drive(a, &da);
    pb = cmd_skip_drive(b, &db);
    if (da != db) {
        return 0;                   /* different volumes -> different files */
    }
    pa = cmd_skip_dotslash(pa);
    pb = cmd_skip_dotslash(pb);

    if (!cmd_has_sep(pa) && !cmd_has_sep(pb)) {
        char ka[11];
        char kb[11];
        cmd_fold_83(pa, ka);
        cmd_fold_83(pb, kb);
        for (k = 0; k < 11; k++) {
            if (ka[k] != kb[k]) {
                return 0;
            }
        }
        return 1;
    }
    /* A separator remains: compare the remainders case-insensitively (DOS paths
     * are case-blind). This catches identical subdir paths but does NOT resolve
     * relative-vs-absolute -- the documented residual (follow-up bead). */
    for (k = 0;; k++) {
        if (cmd_upcase_char(pa[k]) != cmd_upcase_char(pb[k])) {
            return 0;
        }
        if (pa[k] == '\0') {
            return 1;
        }
    }
}
#endif

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
    /* Guide pp.19-20 / PDF32-33: month and hour have no leading zero. */
    put_u_pad0(out, &pos, (uint32_t)mon, mon >= 10u ? 2 : 1);
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
    put_u_pad0(out, &pos, (uint32_t)hh, hh >= 10u ? 2 : 1);
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

/* ---- OUTPUT redirection parser (PURE, host-testable) ----------------------
 * cmd_redir_parse: see command.h for the full citation + semantics block.
 *
 * Ref: PRD Sec 6.1 (COMMAND.COM personality); MS-DOS 3.3 Tech Ref Ch.6 (the
 *   shell strips `> file` / `>> file` and DUP2's the file onto handle 1).
 *   beads initech-hsct (OUTPUT increment: `>` create/truncate, `>>` append).
 *
 * Implementation: a single left-to-right scan that records the position of the
 * LAST `>` operator (and whether it was `>>`) plus the bounds of the target
 * token that follows it.  We then rebuild `clean_out` = [prefix before op] +
 * " " (one space, only if both sides are non-empty) + [suffix after target],
 * with leading/trailing whitespace trimmed.  Pure: no asm, no I/O.
 *
 * MUTATION hooks (CLAUDE.md Rule 6; the test_redir_parse oracle proves they
 * bite):
 *   CMD_MUTATE_REDIR_NO_APPEND -- treat `>>` as a single `>` (truncate), so the
 *                                 append flag is never set -> the `>>`-append
 *                                 assertion goes RED.  NEVER in a real build.
 *   CMD_MUTATE_REDIR_KEEP_TARGET -- leave the target token IN clean_out, so the
 *                                   redirected command would mis-parse its args
 *                                   -> the clean-command assertion goes RED.
 *                                   NEVER in a real build. */
static int redir_is_space(char c)
{
    return c == ' ' || c == '\t';
}

/* Strip ONE redirection operator (`opch` == '>' or '<') off `line`.  This is the
 * generic engine cmd_redir_parse runs twice -- once for OUTPUT '>' (allow_append
 * = 1, so `>>` sets *append_out), once for INPUT '<' (allow_append = 0; there is
 * no `<<`).  It reproduces the original hsct single-`>` split byte-for-byte when
 * called with opch='>' -- the OUTPUT parse oracle is unchanged.
 *
 *   - Scans left to right; the LAST `opch` operator wins.
 *   - The TARGET token is the next whitespace-delimited run after the operator,
 *     stopping at EITHER redirect char ('>' or '<') so a combined `cmd<in>out`
 *     splits cleanly regardless of order.
 *   - `clean_out` = prefix [0,op_at) + suffix after the target, whitespace-tidied.
 *   - Returns 1 iff `opch` was present (target_out valid), else 0 (clean_out is a
 *     clamped copy of `line`, target_out == "").
 *
 * Rule 2: every write is bounded by its cap; NUL-terminated within the buffer,
 * never past the end.  The mutation hooks below are the SAME ones the hsct oracle
 * proves bite (they act on whichever pass is running; the parse-mutant tests use
 * `>` lines). */
static int redir_strip_one(const char *line, char opch, int allow_append,
                           char *clean_out, uint32_t clean_cap,
                           char *target_out, uint32_t target_cap,
                           int *append_out)
{
    if (append_out != 0) {
        *append_out = 0;
    }
    if (target_out != 0 && target_cap > 0u) {
        target_out[0] = '\0';
    }

    if (line == 0) {
        if (clean_out != 0 && clean_cap > 0u) {
            clean_out[0] = '\0';
        }
        return 0;
    }

    /* Find the LAST `opch` operator. op_at = index of the FIRST char of the
     * winning operator; op_len = 1, or 2 for a `>>` when allow_append. */
    int op_at = -1;
    int op_len = 0;
    {
        uint32_t i = 0;
        while (line[i] != '\0') {
            if (line[i] == opch) {
                op_at = (int)i;
                if (allow_append && line[i + 1u] == opch) {
                    op_len = 2;
                    i += 2u;     /* consume both so a later lone op wins fresh */
                } else {
                    op_len = 1;
                    i += 1u;
                }
            } else {
                i += 1u;
            }
        }
    }

    if (op_at < 0) {
        /* Operator absent: clean_out is a clamped copy of line; target empty. */
        uint32_t j = 0;
        if (clean_out != 0 && clean_cap > 0u) {
            while (line[j] != '\0' && j + 1u < clean_cap) {
                clean_out[j] = line[j];
                j++;
            }
            clean_out[j] = '\0';
        }
        return 0;
    }

#ifdef CMD_MUTATE_REDIR_NO_APPEND
    /* MUTANT: force every operator to single width (truncate) -- the append flag
     * is never set; the `>>` oracle case goes RED.  NEVER in a real build. */
    op_len = 1;
#endif

    if (append_out != 0) {
        *append_out = (op_len == 2) ? 1 : 0;
    }

    /* Locate the target token: skip whitespace after the operator, then take a
     * run of non-whitespace, stopping at EITHER redirect char. */
    uint32_t after = (uint32_t)op_at + (uint32_t)op_len;
    while (line[after] != '\0' && redir_is_space(line[after])) {
        after++;
    }
    uint32_t tgt_start = after;
    uint32_t tgt_end = tgt_start;
    while (line[tgt_end] != '\0' && !redir_is_space(line[tgt_end]) &&
           line[tgt_end] != '>' && line[tgt_end] != '<') {
        tgt_end++;
    }

    /* Copy the target token (clamped, NUL-terminated). */
    if (target_out != 0 && target_cap > 0u) {
        uint32_t k = 0;
        uint32_t s = tgt_start;
        while (s < tgt_end && k + 1u < target_cap) {
            target_out[k] = line[s];
            k++;
            s++;
        }
        target_out[k] = '\0';
    }

    /* Rebuild clean_out = prefix [0, op_at) + suffix [tgt_end, end), with one
     * separating space inserted only when both sides have non-space content, and
     * the whole result whitespace-trimmed at both ends (Rule 2, bounded). */
    if (clean_out != 0 && clean_cap > 0u) {
        uint32_t w = 0;

        {
            uint32_t p = 0;
            while (p < (uint32_t)op_at && w + 1u < clean_cap) {
                clean_out[w++] = line[p++];
            }
        }
        while (w > 0u && redir_is_space(clean_out[w - 1u])) {
            w--;
        }
        uint32_t prefix_len = w;

        uint32_t suf = tgt_end;
        while (line[suf] != '\0' && redir_is_space(line[suf])) {
            suf++;
        }

#ifdef CMD_MUTATE_REDIR_KEEP_TARGET
        /* MUTANT: rewind the suffix start back over the target token so the
         * target filename stays in clean_out -> the clean-command oracle goes
         * RED.  NEVER in a real build. */
        suf = tgt_start;
#endif

        if (line[suf] != '\0' && prefix_len > 0u && w + 1u < clean_cap) {
            clean_out[w++] = ' ';
        }

        while (line[suf] != '\0' && w + 1u < clean_cap) {
            clean_out[w++] = line[suf++];
        }

        while (w > 0u && redir_is_space(clean_out[w - 1u])) {
            w--;
        }
        clean_out[w] = '\0';
    }

    return 1;
}

int cmd_redir_parse(const char *line,
                    char *clean_out, uint32_t clean_cap,
                    char *target_out, uint32_t target_cap,
                    int *append_out,
                    char *in_target_out, uint32_t in_target_cap)
{
    /* Two independent passes (beads initech-bsy.7).  PASS 1 strips the OUTPUT
     * `>`/`>>` operator+target into `target_out` and produces an intermediate
     * clean line in `tmp`.  PASS 2 strips the INPUT `<` operator+target from that
     * intermediate into `in_target_out`, producing the final `clean_out`.  Doing
     * OUTPUT first means `cmd < in > out` and `cmd > out < in` BOTH resolve: the
     * '>' target scan stops at the '<' (and vice-versa), so neither pass swallows
     * the other's operator.  The intermediate is bounded by CMD_LINE_MAX -- the
     * shell's hard line cap (command.h); a real command line never exceeds it. */
    char tmp[CMD_LINE_MAX];

    /* Defensive default so an early/absent-input return leaves in_target well
     * formed (Rule 2). */
    if (in_target_out != 0 && in_target_cap > 0u) {
        in_target_out[0] = '\0';
    }

    int out_has = redir_strip_one(line, '>', 1 /*allow_append*/,
                                  tmp, (uint32_t)sizeof(tmp),
                                  target_out, target_cap, append_out);

#ifdef CMD_MUTATE_REDIR_NO_LT
    /* MUTANT (Rule 6; beads initech-bsy.7): drop the `<` INPUT scan entirely --
     * clean_out is just the OUTPUT-stripped line, in_target stays empty, and
     * CMD_REDIR_IN is never returned.  The `<` parse oracle + the GOBBLE emu gate
     * both go RED (the child reads the keyboard, not the file).  NEVER in a real
     * build. */
    {
        uint32_t j = 0;
        if (clean_out != 0 && clean_cap > 0u) {
            while (tmp[j] != '\0' && j + 1u < clean_cap) {
                clean_out[j] = tmp[j];
                j++;
            }
            clean_out[j] = '\0';
        }
    }
    return out_has ? CMD_REDIR_OUT : 0;
#else
    int in_has = redir_strip_one(tmp, '<', 0 /*no `<<`*/,
                                 clean_out, clean_cap,
                                 in_target_out, in_target_cap, 0 /*no append*/);

    return (out_has ? CMD_REDIR_OUT : 0) | (in_has ? CMD_REDIR_IN : 0);
#endif
}

/* cmd_pipe_split: split `line` on the `|` PIPE operator into ordered stages.
 * See command.h for the full citation + semantics block (beads initech-bsy.8).
 *
 * Ref: PRD Sec 6.1; MS-DOS 3.3 Tech Ref Ch.6 (`a | b` is a TEMP-FILE pipe; the
 *   shell has no quoting so every `|` is a separator; `a | b | c` composes
 *   left-to-right).  Pure: no asm, no I/O -- host-testable in test_redir_parse.c.
 *
 * Implementation: one left-to-right scan.  We emit a stage each time we hit a
 * `|` (copying the span since the last cut, whitespace-trimmed) until either the
 * line ends or we reach CMD_PIPE_MAX_STAGES-1 stages; the FINAL stage swallows
 * the entire remaining tail verbatim (embedded `|` preserved -- bounded + no
 * lost byte, Rule 2).  Each copy is clamped to CMD_LINE_MAX and NUL-terminated. */
int cmd_pipe_split(const char *line, cmd_pipeline_t *out)
{
    if (out == 0) {
        return 0;
    }

    out->nstages = 0;

    if (line == 0) {
        out->stage[0][0] = '\0';
        out->nstages = 1;
        return 1;
    }

    uint32_t seg_start = 0;   /* index of the first char of the current stage span */
    uint32_t i = 0;
    for (;;) {
        int at_end = (line[i] == '\0');

#ifdef CMD_MUTATE_PIPE_NO_SPLIT
        /* MUTANT (Rule 6; beads initech-bsy.8): never treat `|` as a separator,
         * so the whole line collapses into ONE stage -- the multi-stage parse
         * assertions + the GREET|GOBBLE emu gate go RED.  NEVER in a real build. */
        int is_sep = 0;
#else
        int is_sep = (line[i] == '|');
#endif

        /* Cut a stage at a `|` OR at end-of-line -- UNLESS this is the last slot
         * we may fill, in which case we keep scanning so the remainder (any
         * embedded `|` included) folds into the final stage span. */
        int last_slot = (out->nstages == CMD_PIPE_MAX_STAGES - 1);

        if (at_end || (is_sep && !last_slot)) {
            /* Trim leading whitespace of [seg_start, i). */
            uint32_t s = seg_start;
            uint32_t e = i;
            while (s < e && redir_is_space(line[s])) {
                s++;
            }
            /* Trim trailing whitespace. */
            while (e > s && redir_is_space(line[e - 1u])) {
                e--;
            }
            /* Copy [s, e) into the current stage, clamped + NUL-terminated. */
            char *dst = out->stage[out->nstages];
            uint32_t k = 0;
            while (s < e && k + 1u < (uint32_t)CMD_LINE_MAX) {
                dst[k] = line[s];
                k++;
                s++;
            }
            dst[k] = '\0';
            out->nstages++;

            if (at_end) {
                break;
            }
            seg_start = i + 1u;   /* next stage starts after the `|` */
        }

        i++;
    }

    /* A line that is all whitespace / empty still yields exactly one ("") stage. */
    if (out->nstages == 0) {
        out->stage[0][0] = '\0';
        out->nstages = 1;
    }

    return out->nstages;
}

/* ===========================================================================
 * The REPL (kernel-only). Everything below dogfoods the INT 21h API via real
 * `int $0x21` calls -- the authentic COMMAND.COM design + proof the OS API is a
 * usable surface (the brief's central design point). Compiled out of the host
 * build (test_command.c never defines COMMAND_KERNEL_REPL) so the pure logic
 * above stays asm-free + host-testable (Law 3).
 * ===========================================================================*/
#if defined(COMMAND_KERNEL_REPL) || defined(COMMAND_FILE_HOST)

#include "find_data.h"   /* find_data_t (43-byte DTA record; offsets DIR reads) */
#include "int21.h"       /* INT21_CWD_MAX -- the AH=47h root-relative CWD bound */
#include "memory_map.h"  /* ENV_BLOCK / ENV_BLOCK_CAP -- EXEC env inheritance (inc 3) */
#include "batch.h"       /* the .BAT parser/expander + IF/FOR decision helpers   */
#include "fileio_fat.h"  /* mounted volume identity for DIR (M017) */

/* DEC-04a flat ABI (spec/int21h_calling_convention.json): AH=function in EAX,
 * EBX=handle, ECX=count, EDX=flat ptr, EAX=return, CF=error in EFLAGS. The
 * wrappers below issue a literal `int $0x21` honoring exactly that contract --
 * the SAME path a real DOS COMMAND.COM uses (it is itself a .COM that calls the
 * INT 21h services). We capture CF with `sbb` (carry -> 0xFFFFFFFF/0). */

#ifndef COMMAND_FILE_HOST
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

/* AH=4Eh FINDFIRST: return 0 on a hit, otherwise the DOS error code. DEL needs
 * to distinguish exhaustion from a failed search; legacy callers use the bool
 * wrapper below. */
static uint16_t dos_findfirst_result(const char *spec, uint16_t attr)
{
    uint32_t ax = 0x4E00u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "c"((uint32_t)attr), "d"((uint32_t)(uintptr_t)spec)
        : "cc", "memory");
    return carry == 0u ? 0u : (uint16_t)ax;
}

static int dos_findfirst(const char *spec, uint16_t attr)
{
    return dos_findfirst_result(spec, attr) == 0u;
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

/* Sentinel returned by dos_read on a CF=1 (error) completion of AH=3Fh. Every
 * caller in this file passes a `count` bounded well under this value (128-byte
 * TYPE/COPY chunks, the BATCH_FILE_MAX=4096 script cap), so a genuine byte
 * count from AH=3Fh can never collide with it -- it is unambiguously "the read
 * failed", distinct from the legitimate got==0 EOF case. */
#define DOS_READ_ERROR 0xFFFFFFFFu

/* AH=3Fh READ: EBX=handle, ECX=count, EDX=buffer. Returns bytes read (0=EOF),
 * or DOS_READ_ERROR on a CF=1 completion (bad handle / device fault / corrupt
 * chain -- int21.c do_read's set_ax(f, err); cf_set(f) legs). Ref: bead
 * initech-winh (P1) -- the sibling wrappers immediately below/above
 * (dos_open ~1565, dos_creat ~1608, dos_write_h ~1628, dos_open_mode ~1648)
 * all capture CF with `sbb` and refuse to hand the caller a raw AX-on-error;
 * this one used a bare `int $0x21` with NO carry capture, so on a read error
 * (CF=1, AX=DOS error code, e.g. 0x0005 ACCESS_DENIED for a PRN read) the
 * error code came back looking like a byte count. Every caller here treats
 * ANY nonzero return as "more real data, keep reading" (only `got == 0u`
 * meant EOF) -- so a CF=1/AX=5 completion read as "5 bytes" and the caller
 * flushed 5 bytes of whatever was sitting in its on-stack chunk buffer
 * (uninitialized on the very first read) to the destination, then looped
 * back for another read, which failed the SAME way -- an infinite loop
 * spewing garbage, never reaching EOF. Confirmed via `TYPE PRN` under the
 * QEMU harness (PRN's read handler -- os/milton/devices.c dev_prn_handler,
 * DEVCMD_READ -- always sets DEVST_ERROR, "PRN is a write-only device; a
 * read from it is an error", Eggebrecht "Writing MS-DOS Device Drivers"
 * Ch.4): the unfixed build never printed SHELL-EXIT and its serial log filled
 * with a repeating 5-byte garbage unit. */
static uint32_t dos_read(int handle, uint8_t *buf, uint32_t count)
{
    uint32_t ax = 0x3F00u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "b"((uint32_t)handle), "c"(count), "d"((uint32_t)(uintptr_t)buf)
        : "cc", "memory");
#ifdef CMD_MUTATE_NO_READ_CF
    /* MUTANT (Rule 6; make test-readerr-winh-mutant only): pretend the CF
     * capture above never happened -- hand the caller the raw EAX exactly as
     * the pre-fix code did, so a read error (CF=1, AX=DOS error code) is
     * indistinguishable from a successful short read again. Reproduces
     * initech-winh verbatim; the oracle must go RED. NEVER define in a real
     * build. (void) keeps -Werror=unused-variable quiet about `carry`. */
    (void)carry;
    return ax;
#else
    if (carry != 0u) {
        return DOS_READ_ERROR;   /* CF set -- do NOT hand back AX as a count */
    }
    return ax;   /* EAX = bytes read (0 = EOF) */
#endif
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

/* IBM DOS 3.30 Technical Reference p.6-208 (PDF331), ADR-0003 Appendix A:
 * AH=57h AL=0 gets / AL=1 sets CX=packed time, DX=packed date by handle.
 * Keep both registers as outputs: GET returns them through the interrupt. */
static int dos_filetime(int handle, unsigned set, uint16_t *time, uint16_t *date)
{
    uint32_t ax = 0x5700u | set, cx = *time, dx = *date, carry;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry), "+c"(cx), "+d"(dx)
        : "b"((uint32_t)handle)
        : "cc", "memory");
    if (carry) return -(int)(ax & 0xFFFFu);
    *time = (uint16_t)cx; *date = (uint16_t)dx;
    return 0;
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

/* AH=5Bh CREATE NEW FILE: CREAT that refuses an existing name (CF set, AX=50h
 * file exists) instead of truncating it. Same return shape as dos_creat.
 * Ref: IBM DOS 3.30 Technical Reference p.6-215; the pipe temporaries
 * (initech-66oc, audit L001) must never clobber a user file. */
static int dos_creatnew(const char *path)
{
    uint32_t ax = 0x5B00u;
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
 * bytes written (EAX) on success, or DOS_WRITE_ERROR on a CF error. A count
 * below the request with CF clear is the full-disk answer (IBM DOS 3.30 TR
 * pp.6-139/6-140); COPY reports the two differently (initech-66oc). This is the
 * FILE-handle counterpart of the console dos_write above (which pins handle 1). */
#define DOS_WRITE_ERROR 0xFFFFFFFFu
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
        return DOS_WRITE_ERROR;   /* CF set -- not a byte count */
    }
    return ax;           /* EAX = bytes written */
}

/* IBM DOS 3.30 TR p.6-140: zero-count AH=40h truncates at the current
 * position. Capture CF separately: zero bytes is also the success result. */
static int dos_seteof(int handle)
{
    uint32_t ax=0x4000u,carry;
    __asm__ __volatile__("int $0x21\n\t" "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "b"((uint32_t)handle), "c"(0u), "d"(0u)
        : "cc", "memory");
    return carry ? -1:0;
}

/* AH=3Dh OPEN with an explicit AL access mode. dos_open above pins AL=00 (read);
 * `>>` append needs the file opened RDWR (AL=02) so LSEEK-to-end + WRITE land in
 * the existing file. Returns the handle (>=0) on success, or -(error) on CF.
 * Ref: DOS 3.3 PRM AH=3Dh; beads initech-hsct (`>>` open-existing path). */
static int dos_open_mode(const char *path, uint8_t mode)
{
    uint32_t ax = 0x3D00u | (uint32_t)mode;   /* AH=3Dh, AL=mode */
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

/* AH=45h DUP: duplicate handle EBX into the lowest free JFT slot. Returns the
 * NEW handle (>=0, AL) on success (CF clear), or -1 on failure (CF set). Used by
 * the redirect driver to SAVE the current stdout (handle 1) before repointing
 * it at the target file, so it can be restored afterward.
 * Ref: DOS 3.3 PRM AH=45h; int21.c do_dup; beads initech-hsct. */
static int dos_dup(int handle)
{
    uint32_t ax = 0x4500u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "b"((uint32_t)handle)
        : "cc", "memory");
    if (carry != 0u) {
        return -1;
    }
    return (int)(ax & 0xFFu);   /* AL = new handle */
}

/* AH=46h DUP2 (FORCEDUP): force handle `dst` to alias handle `src`. This is THE
 * I/O-redirection primitive: dos_dup2(file_h, 1) repoints stdout at the file so
 * every AH=09h/02h/06h/40h-handle-1 write (k36g) lands in the file. Returns 0 on
 * success (CF clear), or -1 on failure (CF set; bad src/dst).
 * Ref: DOS 3.3 PRM AH=46h; int21.c do_dup2; beads initech-hsct. */
static int dos_dup2(int src, int dst)
{
    uint32_t ax = 0x4600u;
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "b"((uint32_t)src), "c"((uint32_t)dst)
        : "cc", "memory");
    if (carry != 0u) {
        return -1;
    }
    return 0;
}

/* AH=42h AL=02h LSEEK-FROM-END: move handle `handle`'s file pointer to EOF so a
 * subsequent WRITE appends. ECX:EDX offset = 0 (seek exactly to end). Returns 0
 * on success (CF clear), or -1 on failure (CF set). Used by the `>>` append path
 * after opening the existing file RDWR.
 * Ref: DOS 3.3 PRM AH=42h; int21.c do_lseek (whence 2 = from end); initech-hsct. */
static int dos_lseek_end(int handle)
{
    uint32_t ax = 0x4202u;   /* AH=42h, AL=02 (from end) */
    uint32_t carry = 0;
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "b"((uint32_t)handle), "c"(0u), "d"(0u)
        : "cc", "memory");
    if (carry != 0u) {
        return -1;
    }
    return 0;
}

/* AH=42h absolute seek, ADR-0003 App A; COPY /V and p.52 append EOF. */
static int dos_seek_absolute(int handle, uint32_t offset)
{
    uint32_t ax=0x4200u,carry;
    __asm__ __volatile__("int $0x21\n\t" "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "b"((uint32_t)handle), "c"(offset>>16), "d"(offset&65535u)
        : "cc", "memory");
    return carry ? -1:0;
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
            dos_print(MSG_DOS_0031 "\r\n$");
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

/* Set to 1 when a nonpermanent processor accepts EXIT, from the prompt or a
 * batch. The permanent primary ignores it. The batch interpreter
 * checks this after each dispatched line and unwinds (CALL nests included), and
 * command_repl's main loop returns on it.  File scope so dispatch_line (the
 * setter) and command_repl / run_batch (the checkers) share one flag. */
static int g_shell_exit = 0;
static int g_shell_permanent = 1;
static int21_break_recovery_t g_shell_break;

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
static uint32_t dos_getcwd(uint8_t drive, char *buf)
{
    uint32_t ax = 0x4700u, carry;
    buf[0] = '\0';
    __asm__ __volatile__(
        "int $0x21\n\t"
        "sbb %1, %1"
        : "+a"(ax), "=r"(carry)
        : "d"((uint32_t)drive), "S"((uint32_t)(uintptr_t)buf)
        : "cc", "memory");
    return carry;   /* nonzero: invalid drive (AX=000Fh) */
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
 * SET-specific diagnostics also use the controlled catalogue (initech-bcg.13). */
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
                dos_print(MSG_DOS_0031 "\r\n$");
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
                dos_print(MSG_DOS_0032 "\r\n$");
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
        dos_print(MSG_DOS_0031 "\r\n$");
    }
    /* Success is SILENT (DOS PROMPT prints nothing on success). */
}

/* sy5f: PATH and SET PATH share the master environment used by EXEC/BAT
 * resolution. Ref: ADR-0003 DEC-11; MS-DOS 3.3 Reference p.97 / PDF182,
 * p.348 / PDF433 (empty-path diagnostic; task's DOS spelling "No Path"). */
static void builtin_path(const char *arg)
{
    char value[CMD_LINE_MAX];
    while (is_space(*arg) || *arg == '=') arg++;
    if (!*arg) {
        const char *path = env_get(&g_master_env, "PATH");
        if (path && *path) {
            dos_puts_raw("PATH="); dos_puts_raw(path); dos_print("\r\n$");
        } else dos_print("No Path\r\n$");
        return;
    }
    int n = 0;
    while (*arg && n < CMD_LINE_MAX - 1) value[n++] = cmd_upcase_char(*arg++);
    while (n && is_space(value[n-1])) n--;
    value[n] = 0;
    if (n == 1 && value[0] == ';') env_unset(&g_master_env, "PATH");
    else if (!env_set(&g_master_env, "PATH", value))
        dos_print(MSG_DOS_0031 "\r\n$");
}

/* DIR: existing FINDFIRST/FINDNEXT operand behavior, DOS 3.3 display.
 * Ref: User Guide pp.28-29 / PDF 41-42; controlled fragments from the
 * deliberate LOOK Rule-8 amendment to spec/dos_messages.json. DTA supplies
 * the packed timestamp, AH=36h the FAT free count, mounted root the label. */
static void cwd_display(char *out);
/* VOL reuses DIR's root-label reader and controlled text. Only the mounted
 * drive A exists (INT21 GETCWD/GETSPACE contract). MS 3.3 p.126 / PDF211. */
/* UG p26 / Reference pp57-58: explicit paths do not change the default.
 * The approved catalogue fixes spacing; substitute only the selected letter. */
static void print_volume_identity(char drive, const char *label)
{
    const char *source = label[0] ? MSG_DOS_0021 : MSG_DOS_0022;
    char text[64]; unsigned i=0;
    do { text[i]=source[i]; } while(source[i++]);
    text[17]=drive;
    dos_puts_raw(text);
    if(label[0]) dos_puts_raw(label);
}

static void builtin_vol(const char *arg)
{
    while (is_space(*arg)) arg++;
    if (*arg && arg[1]!=':') { dos_print(MSG_DOS_0004 "\r\n$"); return; }
    const char *selected=arg;
    if (int21_select_path(&selected)) { dos_print(MSG_DOS_0004 "\r\n$"); return; }
    while (is_space(*selected)) selected++;
    if (*selected) { dos_print(MSG_DOS_0010 "\r\n$"); return; }
    char drive=(char)('A'+int21_io_drive());
    char label[12];
    if (fileio_fat_volume_label(label)<0) {
        dos_print(MSG_DOS_0009 "\r\n$");
        return;
    }
    dos_print("\r\n$");
    print_volume_identity(drive, label);
    dos_print("\r\n$");
}
/* e47v: MS-DOS 3.3 Reference pp.57-58 / PDF142-143: one qualified
 * operand, filename.* / *.ext shorthand, /P and five-column /W. Directory
 * canonicalization uses the backend without CHDIR, so DIR never changes CWD. */
static void builtin_dir(const char *arg)
{
    int n;
    uint32_t files = 0;
    char label[12], disp[3 + INT21_CWD_MAX + 4];
    uint32_t ax=0x3600u, bx, cx, dx=0u;
    char spec[CMD_LINE_MAX], parent[CMD_LINE_MAX];
    int wide = 0, page = 0, len = 0, rows = 6, column = 0;
#ifdef CMD_MUTATE_DIR_ARGS
    arg = ""; /* Rule 6: restore the ignored-operand regression. */
#endif
    while (*arg) {
        if (is_space(*arg)) { arg++; continue; }
        if (*arg == '/') {
            char option = cmd_upcase_char(*++arg);
            if (option == 'W') wide = 1;
            else if (option == 'P') page = 1;
            else { dos_print(MSG_DOS_0010 "\r\n$"); return; }
            arg++;
            continue;
        }
        if (len) { dos_print(MSG_DOS_0010 "\r\n$"); return; }
        while (*arg && !is_space(*arg) && *arg != '/') {
            if (len >= CMD_LINE_MAX - 5) {
                dos_print(MSG_DOS_0010 "\r\n$"); return;
            }
            spec[len++] = cmd_upcase_char(*arg++);
        }
    }
    spec[len] = 0;
    const char *selected = spec;
    if (int21_select_path(&selected)) {
        dos_print(MSG_DOS_0004 "\r\n$"); return;
    }
    disp[0] = (char)('A'+int21_io_drive()); disp[1] = ':'; disp[2] = '\\';
    if (int21_directory_path(spec, disp+3, INT21_CWD_MAX)) {
        if (len && spec[len-1] != '\\' && spec[len-1] != ':') spec[len++] = '\\';
        spec[len++] = '*'; spec[len++] = '.'; spec[len++] = '*'; spec[len] = 0;
    } else {
        int leaf = 0;
        for (int i=0; i<len; i++) if (spec[i] == '\\' || spec[i] == ':') leaf = i+1;
        for (int i=0; i<leaf; i++) parent[i] = spec[i];
        parent[leaf] = 0;
        if (!int21_directory_path(parent, disp+3, INT21_CWD_MAX)) {
            dos_print(MSG_DOS_0003 "\r\n$"); return;
        }
        if (spec[leaf] == '.') {
            for (int i=len; i>=leaf; i--) spec[i+1] = spec[i];
            spec[leaf] = '*'; len++;
        }
        int dot = 0;
        for (int i=leaf; i<len; i++) if (spec[i] == '.') dot = 1;
        if (!dot) { spec[len++] = '.'; spec[len++] = '*'; spec[len] = 0; }
    }

    dx=(uint32_t)(disp[0]-'A'+1);
    /* AH=36h counts live FAT free clusters; never infer space from file sizes.
     * Technical Reference AH=36h; the host gate compares this backend to mdir. */
    __asm__ __volatile__("int $0x21" : "+a"(ax), "=b"(bx), "=c"(cx), "+d"(dx) : : "cc", "memory");
    if ((ax&0xFFFFu)==0xFFFFu) {
        dos_print(MSG_DOS_0004 "\r\n$");
        return;
    }
    if (fileio_fat_volume_label(label)<0) {
        int pos=0;
        for (const char *p=MSG_DOS_0007; *p; p++) {
            if (*p=='%' && p[1]=='c') { g_out[pos++]='A'; p++; }
            else g_out[pos++]=*p;
        }
        g_out[pos]=0;
        dos_puts_raw(g_out);
        dos_print("\r\n$");
        return;
    }
    uint32_t free_bytes=(ax&0xFFFFu)*(bx&0xFFFFu)*(cx&0xFFFFu);

    dos_setdta(&g_shell_dta);
    dos_print("\r\n$");
#ifndef CMD_MUTATE_DIR_NO_VOLUME
    print_volume_identity(disp[0], label);
    dos_print("\r\n\r\n$");
#endif
    dos_puts_raw(MSG_DOS_0023);
    dos_puts_raw(disp);
    dos_print("\r\n\r\n$");

    /* Search attribute 0x10 (DIRECTORY) so subdir entries are listed too; the
     * dispatcher already includes plain files. */
    if (!dos_findfirst(spec, DIR_ATTR_DIRECTORY)) {
        dos_print(MSG_DOS_0003 "\r\n$");
        return;
    }
    do {
        if (page && rows >= 23) {
            dos_print(MSG_DOS_0016 "$" );
            (void)dos_conin();
            dos_print("\r\n$"); rows = 0;
        }
        if (wide) {
            char cell[17];
            int pos = 0;
            if (g_shell_dta.attr & DIR_ATTR_DIRECTORY) cell[pos++] = '[';
            for (int i=0; g_shell_dta.fname[i] && pos<15; i++) cell[pos++] = g_shell_dta.fname[i];
            if (g_shell_dta.attr & DIR_ATTR_DIRECTORY) cell[pos++] = ']';
            /* Last column ends at its name: padding to column 80 would wrap
             * the console before the explicit newline. */
            if (column != 4) while (pos < 16) cell[pos++] = ' ';
            cell[16] = 0;
            dos_write(cell, (uint32_t)pos);
            if (++column == 5) { dos_print("\r\n$"); column = 0; rows++; }
        } else {
        n = cmd_format_dir_line(g_shell_dta.fname, g_shell_dta.fsize,
                                g_shell_dta.attr, g_shell_dta.fdate,
                                g_shell_dta.ftime, g_out);
        /* cmd_format_dir_line ends the line with '\n'; the CON console + serial
         * treat '\n' as newline (the banner uses '\n' too). */
        dos_write(g_out, (uint32_t)n);
        rows++;
        }
        files++;
    } while (dos_findnext());

    if (wide && column) dos_print("\r\n$");

    dos_print("\r\n$");
    n=cmd_format_dir_footer(files,free_bytes,g_out);
    dos_write(g_out,(uint32_t)n);
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
        dos_print(handle == -15 ? MSG_DOS_0004 "\r\n$" : MSG_DOS_0003 "\r\n$");
        return;
    }
    for (;;) {
        got = dos_read(handle, chunk, sizeof(chunk));
        if (got == DOS_READ_ERROR) {
            /* initech-winh: a read failure is NOT a byte count -- abort
             * rather than flush `chunk` (possibly never written this call)
             * to stdout as if it were real file content. */
            dos_close(handle);
            dos_print(MSG_DOS_0002 "\r\n$");   /* catch-all diagnostic */
            return;
        }
        if (got == 0u) {
            break;          /* EOF */
        }
        dos_write((const char *)chunk, got);
    }
    dos_close(handle);
    /* A redirected TYPE is file bytes, without console-only line framing.
     * MS-DOS 3.3 Reference TYPE/redirection; IOCTL IBM TR pp6-148..149. */
    uint32_t ax=0x4400u, info;
    __asm__ __volatile__("int $0x21" : "+a"(ax), "=d"(info) : "b"(1u) : "cc","memory");
    if(info & 0x80u) dos_print("\r\n$");
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

    out[n++] = (char)('A'+int21_current_drive());
    out[n++] = ':';
    out[n++] = '\\';                  /* the drive + root separator -> "A:\" */
    while (rel[i] != '\0' && i < (int)(INT21_CWD_MAX - 1)) {
        out[n++] = rel[i++];          /* append the root-relative subpath */
    }
    out[n] = '\0';
}

static char prompt_drive(void)
{
#ifdef CMD_MUTATE_VOLUME_PROMPT
    return 'A'; /* Rule 6: a C default must produce a C prompt. */
#else
    return (char)('A'+int21_current_drive());
#endif
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
#ifndef CMD_MUTATE_CD_DRIVE
    if (len == 2 && tok[1] == ':') {
        /* "CHDIR b: ... MS-DOS displays b:\user\pete" -- drive d's own
         * directory; the default drive is unchanged (MS-DOS 3.3 Reference
         * p.42 / PDF127; audit 2026-10-23-dos L011). AH=47h DL=1 is A:. */
        char disp[3 + INT21_CWD_MAX + 4];
        uint8_t d = (uint8_t)(tok[0] - 'A');
        if (d > 25u || dos_getcwd((uint8_t)(d + 1u), disp + 3) != 0u) {
            dos_print(MSG_DOS_0004 "\r\n$");
            return;
        }
        disp[0] = tok[0]; disp[1] = ':'; disp[2] = '\\';
        dos_puts_raw(disp);
        dos_print("\r\n$");
        return;
    }
#endif
    uint16_t err = dos_chdir(tok);
    if (err != 0u) dos_print(err == 15u ? MSG_DOS_0004 "\r\n$" : MSG_DOS_0018 "\r\n$");
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

/* CLS: CON's form-feed operation clears and homes without an ANSI driver.
 * Ref: DOS 3.3 User's Guide p.45 / PDF 130, Technical Reference p.3-13 /
 * PDF 82. Scrolling 25 newlines would leave the prompt on the last row. */
static void builtin_cls(void)
{
    /* Ref: DOS 3.3 User's Guide p.45 / PDF 130; Technical Reference
     * p.3-13 / PDF 82 (erase display homes). The CON console handles this
     * control byte directly, including when ANSI.SYS is not loaded. */
    dos_print("\f$");
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
        dos_print(MSG_DOS_0027 "\r\n$");
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
        dos_print(MSG_DOS_0029 "\r\n$");
    } else {
        dos_print(MSG_DOS_0030 "\r\n$");
    }
}

#else
/* Factory adapter issues the same calls to the actual INT21/FAT backend.
 * PRD 6.1: mtools on identical images is the independent oracle. */
#include "shell_files_host.h"
#endif

static uint32_t copy_verify_offset;
/* COPY execution. Law 1: MS-DOS 3.3 Reference pp.50-52; ADR-0003 App A.
 * Resolve identity before CREAT; enumerate by ordinal and restart after file
 * calls (one kernel find context). No roster limit on wildcard source files. */
static int copy_path(char out[CMD_LINE_MAX],const char *spec,const char *leaf)
{
    unsigned prefix=0,n=0;
    for(unsigned i=0;spec[i];i++)if(spec[i]=='\\' || spec[i]==':')prefix=i+1;
    while(leaf[n])n++;
    if(prefix+n>=CMD_LINE_MAX)return 0;
    for(unsigned i=0;i<prefix;i++)out[i]=spec[i];
    for(unsigned i=0;i<=n;i++)out[prefix+i]=leaf[i];
    return 1;
}
static int copy_nth(const char *spec,unsigned n,char leaf[13])
{
    dos_setdta(&g_shell_dta);
    if(!dos_findfirst(spec,0))return 0;
    while(n--)if(!dos_findnext())return 0;
    for(unsigned i=0;i<13;i++)leaf[i]=g_shell_dta.fname[i];
    return 1;
}
static int copy_transfer(const char *source,const char *dest,int ascii,int append,
                         int verify,int preserve)
{
    uint8_t chunk[128],check[128];
    int src=dos_open(source),dst=-1;
    if(src<0){dos_print(src == -15 ? MSG_DOS_0004 "\r\n$" : MSG_DOS_0003 "\r\n$");return 0;}
    int probe=dos_open(dest);
    if(probe == -15) { dos_close(src); dos_print(MSG_DOS_0004 "\r\n$"); return 0; }
    int same=probe>=0 && int21_same_file((uint16_t)src,(uint16_t)probe);
#ifdef CMD_MUTATE_NO_SAMEFILE
    same=0;
#endif
    if(probe>=0)dos_close(probe);
    if(probe<0 && probe!=-(int)INT21_ERR_FILE_NOT_FOUND && probe!=-(int)INT21_ERR_PATH_NOT_FOUND)goto denied;
    if(same) {
        dos_close(src);
        dos_print(MSG_DOS_0020 "\r\n$");
        int n=0;put_u_field(g_out,&n,0,9);dos_write(g_out,(uint32_t)n);
        dos_puts_raw(MSG_DOS_0061);dos_print("\r\n$");
        return 0;
    }
    dst=append ? dos_open_mode(dest,2):dos_creat(dest);
    if(dst<0)goto denied;
    if(append && dos_lseek_end(dst)<0)goto denied;
    for(;;) {
        uint32_t got=dos_read(src,chunk,sizeof chunk);
        if(got==DOS_READ_ERROR)goto failed;
        if(!got)break;
        uint32_t take=got;
#ifndef CMD_MUTATE_COPY_ASCII
        if(ascii)for(uint32_t i=0;i<got;i++)if(chunk[i]==26){take=i;break;}
#else
        (void)ascii;
#endif
        /* A short count is the full-disk answer (TR pp.6-139/6-140): say so
         * with the [MS-DOS] message, Guide App. F p.337 (initech-66oc, L004). */
        uint32_t put=dos_write_h(dst,chunk,take);
        if(put==DOS_WRITE_ERROR)goto failed;
        if(put!=take)goto full;
        if(verify && take) {
            /* READ back the just-written extent through a separate read
             * handle. /V checks recorded data, Reference pp.50-51. */
            int v=dos_open(dest);
            if(v<0)goto failed;
            uint32_t skip=0,offset=copy_verify_offset;
            while(skip<offset) {
                uint32_t n=offset-skip;
                if(n>sizeof check)n=sizeof check;
                if(dos_read(v,check,n)!=n){dos_close(v);goto failed;}
                skip+=n;
            }
            uint32_t n=dos_read(v,check,take);
            dos_close(v);
            if(n!=take)goto failed;
#ifndef CMD_MUTATE_COPY_VERIFY
            for(uint32_t i=0;i<take;i++)if(check[i]!=chunk[i])goto failed;
#endif
        }
        copy_verify_offset+=take;
        if(take<got)break;
        if(g_shell_break.interrupted)goto failed;
    }
    if(preserve) {
        uint16_t time=0,date=0;
        int stamp=dos_filetime(src,0,&time,&date);
        if(stamp==0) {
#ifdef CMD_MUTATE_COPY_TIME
            time=0;
#endif
#ifdef CMD_MUTATE_COPY_DATE
            date=0;
#endif
            stamp=dos_filetime(dst,1,&time,&date);
        }
        if(stamp && stamp!=-(int)INT21_ERR_INVALID_FUNCTION)goto failed;
    }
    dos_close(src);dos_close(dst);return !g_shell_break.interrupted;
denied:
    dos_print(MSG_DOS_0009 "\r\n$");
    dos_close(src);if(dst>=0)dos_close(dst);return 0;
full:
#ifndef CMD_MUTATE_COPY_FULL_BADCMD
    dos_close(src);dos_close(dst);
    dos_print(MSG_DOS_0005 "\r\n$");   /* "Insufficient disk space" */
    return 0;
#endif
failed:
    dos_close(src);if(dst>=0)dos_close(dst);
    if(!g_shell_break.interrupted)dos_print(MSG_DOS_0002 "\r\n$");
    return 0;
}

static void builtin_copy(const char *arg)
{
    cmd_copy_t plan;
    if(!cmd_copy_parse(arg,&plan)) {
        const char *p=arg;while(is_space(*p))p++;
        dos_print(*p ? MSG_DOS_0010 "\r\n$":MSG_DOS_0011 "\r\n$");return;
    }
    char dest[CMD_LINE_MAX],base[13],source[CMD_LINE_MAX],mapped[13];
    int omitted=!plan.dest[0];
    if(omitted && plan.joined) {
        const char *first=plan.names+plan.offset[0];
        if(cmd_has_wildcard(first)){dos_print(MSG_DOS_0002 "\r\n$");return;}
        for(unsigned i=0;(dest[i]=first[i])!=0;i++){}
    } else if(omitted) { dest[0]='.';dest[1]=0; }
    else for(unsigned i=0;(dest[i]=plan.dest[i])!=0;i++){}
    int directory=int21_directory_exists(dest);
#ifdef CMD_MUTATE_COPY_DIR_RAW
    directory=0;
#endif
    int per_file=directory || cmd_has_wildcard(dest);
    int combined=plan.joined || (!per_file && cmd_has_wildcard(plan.names));
    unsigned done=0;
    int append=omitted && plan.joined;
    /* The warning on Reference p.51 applies to every source, including a
     * later + operand. Probe explicit identities BEFORE the first CREAT. */
    if(!per_file) {
        int target=dos_open(dest);
        for(unsigned s=0;s<plan.count;s++) {
            const char *name=plan.names+plan.offset[s];
            if(cmd_has_wildcard(name))continue;
            int h=dos_open(name);
            int same=h>=0 && target>=0 && int21_same_file((uint16_t)h,(uint16_t)target);
#ifdef CMD_MUTATE_NO_SAMEFILE
            same=0;
#endif
            if(h>=0)dos_close(h);
            if(h<0 || (same && !(append && s==0))) {
                if(target>=0)dos_close(target);
                dos_print(same ? MSG_DOS_0020 "\r\n$":MSG_DOS_0003 "\r\n$");
                if(same) {
                    int n=0;put_u_field(g_out,&n,0,9);dos_write(g_out,(uint32_t)n);
                    dos_puts_raw(MSG_DOS_0061);dos_print("\r\n$");
                }
                return;
            }
        }
        if(target>=0)dos_close(target);
    }
    copy_verify_offset=0;
    /* Omitted-target concatenation retains the first source, then appends.
     * With /A, replace its first EOF by the following sources (p.52). */
    if(append) {
        int h=dos_open(dest);uint8_t c;
        if(h<0){dos_print(MSG_DOS_0003 "\r\n$");return;}
        uint32_t got;
        while((got=dos_read(h,&c,1))==1 && (plan.mode[0]==2 || c!=26))copy_verify_offset++;
        dos_close(h);
        if(got==DOS_READ_ERROR){dos_print(MSG_DOS_0009 "\r\n$");return;}
        /* Existing EOF removal needs a positioned truncate before append. */
        h=dos_open_mode(dest,2);
        if(h<0 || dos_seek_absolute(h,copy_verify_offset)<0 || dos_seteof(h)<0) {
            if(h>=0)dos_close(h);
            dos_print(MSG_DOS_0009 "\r\n$");return;
        }
        dos_close(h);
    }
    unsigned outer=0;
    do {
        if(per_file) {
            const char *first=plan.names+plan.offset[0];
#ifdef CMD_MUTATE_COPY_WILD
            if(cmd_has_wildcard(first))break;
#endif
            if(!copy_nth(first,outer++,base))break;
        }
        char target[CMD_LINE_MAX];
        if(per_file) {
            if(directory) {
                unsigned n=str_len(dest),k=str_len(base);
                if(n+k+2>sizeof target)return;
                for(unsigned i=0;i<n;i++)target[i]=dest[i];
                if(n && dest[n-1]!='\\' && dest[n-1]!=':')target[n++]='\\';
                for(unsigned i=0;i<=k;i++)target[n+i]=base[i];
            } else {
                const char *leaf=dest;
                for(const char *p=dest;*p;p++)if(*p=='\\' || *p==':')leaf=p+1;
                if(!cmd_name_map(base,leaf,mapped) || !copy_path(target,dest,mapped))return;
            }
            append=0;copy_verify_offset=0;
        } else for(unsigned i=0;(target[i]=dest[i])!=0;i++){}
        for(unsigned s=(omitted && plan.joined)?1u:0u;s<plan.count;s++) {
#ifdef CMD_MUTATE_COPY_PLUS
            if(s)break;
#endif
            const char *spec=plan.names+plan.offset[s];
            unsigned ordinal=0;
            do {
                if(per_file) {
                    if(s==0) { if(!copy_path(source,spec,base))return; }
                    else if(cmd_has_wildcard(spec)) {
                        const char *leaf=spec;
                        for(const char *p=spec;*p;p++)if(*p=='\\' || *p==':')leaf=p+1;
                        if(!cmd_name_map(base,leaf,mapped) || !copy_path(source,spec,mapped))return;
                    } else for(unsigned i=0;(source[i]=spec[i])!=0;i++){}
                } else if(cmd_has_wildcard(spec)) {
#ifdef CMD_MUTATE_COPY_WILD
                    break;
#endif
                    if(!copy_nth(spec,ordinal++,base))break;
                    if(!copy_path(source,spec,base))return;
                } else for(unsigned i=0;(source[i]=spec[i])!=0;i++){}
                int ascii=plan.mode[s]==1 || (!plan.mode[s] && combined);
                if(!copy_transfer(source,target,ascii,append,(int)plan.verify,!combined))return;
                append=1;
                if(per_file || !cmd_has_wildcard(spec))break;
            } while(!g_shell_break.interrupted);
        }
        if(!append) {dos_print(MSG_DOS_0003 "\r\n$");return;}
        int textmode=plan.dest_mode==1 || (!plan.dest_mode && combined);
#ifndef CMD_MUTATE_COPY_EOF
        if(textmode) {
            uint8_t eof=26;int h=dos_open_mode(target,2);
            if(h<0 || dos_lseek_end(h)<0 || dos_write_h(h,&eof,1)!=1) {
                if(h>=0)dos_close(h);
                dos_print(MSG_DOS_0009 "\r\n$");return;
            }
            dos_close(h);
            if(!combined) {
                int s=dos_open(source); h=dos_open_mode(target,2);
                uint16_t time=0,date=0;
                if(s>=0 && h>=0 && !dos_filetime(s,0,&time,&date)) {
#ifdef CMD_MUTATE_COPY_TIME
                    time=0;
#endif
#ifdef CMD_MUTATE_COPY_DATE
                    date=0;
#endif
                    (void)dos_filetime(h,1,&time,&date);
                }
                if(s>=0)dos_close(s);
                if(h>=0)dos_close(h);
            }
        }
#else
        (void)textmode;
#endif
        done++;
    } while(per_file && !g_shell_break.interrupted);
    {
        int n=0;put_u_field(g_out,&n,done,9);dos_write(g_out,(uint32_t)n);
        dos_puts_raw(MSG_DOS_0061);dos_print("\r\n$");
    }
}

#ifndef COMMAND_FILE_HOST
/* Preserve the operand's parent (including drive/CWD/dots) when substituting
 * the DTA's matched leaf. Both components are bounded: 63 + 12 < 128 bytes.
 * Ref: MS-DOS 3.3 User's Reference pp. 13, 56; audit K03 / initech-8uad. */
static void del_path(char out[CMD_LINE_MAX], const char *spec, int prefix,
                     const char *leaf)
{
    int i;
    for (i = 0; i < prefix; i++) out[i] = spec[i];
    for (i = 0; i < 12 && leaf[i]; i++) out[prefix + i] = leaf[i];
    out[prefix + i] = '\0';
}

/* Failed/unchanged deletion: enumerate the remaining names WITHOUT mutation.
 * The failure and every remaining target are visible; never retry forever.
 * Ref: CLAUDE.md Rule 2; MS-DOS 3.3 User's Reference p. 56 (specified files). */
static void del_remaining(const char *spec, int prefix)
{
    char path[CMD_LINE_MAX];
    dos_print(MSG_DOS_0009 "\r\n$");
    if (dos_findfirst(spec, 0u)) {
        do {
            del_path(path, spec, prefix, g_shell_dta.fname);
            dos_puts_raw(path);
            dos_print("\r\n$");
        } while (dos_findnext());
    } else {
        dos_puts_raw(spec);
        dos_print("\r\n$");
    }
}

/* DEL / ERASE: restart FINDFIRST after each successful UNLINK. No roster cap
 * and no surviving-entry index held across mutation (fileio_fat enumerates
 * live entries, so FINDNEXT after unlink could skip the next survivor).
 * Ref: MS-DOS 3.3 User's Reference pp. 13, 56; K03/K04, 8uad/p4h7. */
static void builtin_del(const char *arg)
{
    char name[CMD_TOKEN_MAX];

    if (cmd_first_token(arg, name) == 0) {
        dos_print(MSG_DOS_0011 "\r\n$");        /* "Required parameter missing" */
        return;
    }
    cmd_upcase_str(name);                       /* DOS upcases 8.3 names */

    int prefix = 0;
    for (int i = 0; name[i]; i++)
        if (name[i] == '\\' || name[i] == ':') prefix = i + 1;
#ifndef CMD_MUTATE_DEL_NO_CONFIRM
    const char *pattern = name + prefix;
    if (pattern[0] == '*' && pattern[1] == '.' &&
        pattern[2] == '*' && pattern[3] == '\0') {
        /* Ref: MS-DOS 3.3 User's Reference pp. 13, 56 (qualified *.* + exact
         * prompt); IBM DOS 3.10 Reference p. 7-78 (y/n then Enter).
         * Audit K15 / initech-jzhh: ask BEFORE enumeration or any mutation.
         * Only a single Y (case-insensitive) grants deletion; everything else
         * returns without touching the disk. MSG-DOS-0012 is already locked. */
        uint8_t answer[CMD_LINE_MAX + 2];
        answer[0] = CMD_LINE_MAX;
        answer[1] = 0u;
        dos_print(MSG_DOS_0012 " $");
        dos_getline(answer);
        int yes = answer[1] == 1u && cmd_upcase_char((char)answer[2]) == 'Y';
#ifdef CMD_MUTATE_DEL_YES_ALWAYS
        yes = 1; /* Rule 6: non-Y refusal must bite on disk preservation. */
#endif
#ifdef CMD_MUTATE_DEL_REFUSE_Y
        yes = 0; /* Rule 6: approved deletion must actually take place. */
#endif
        if (!yes) return;
    }
#endif

    if (!cmd_has_wildcard(name)) {
        /* Plain name: a single UNLINK. Not-found -> "File not found". */
        uint16_t error = dos_unlink(name);
        if (error != 0u) {
            /* MS-DOS_3.3_Users_Guide_198707.pdf, Reference p. 308. */
            dos_print(error == INT21_ERR_ACCESS_DENIED
                      ? MSG_DOS_0009 "\r\n$" : MSG_DOS_0003 "\r\n$");
        }
        return;
    }

    /* One matched leaf at a time. Repeated first leaf detects a backend that
     * reports success without removing anything, rather than looping forever. */
    {
        char previous[13];
        char path[CMD_LINE_MAX];
        int deleted = 0;

        dos_setdta(&g_shell_dta);
        for (;;) {
            uint16_t result = dos_findfirst_result(name, 0u);
            int protected = 0;
            /* The backend's enumeration index counts surviving entries.
             * Skip protected matches without mutation, then RESTART after
             * each successful unlink (otherwise the next entry is skipped).
             * MS-DOS_3.3_Users_Guide_198707.pdf, Reference pp. 33, 56.
             * Once only protected entries remain, report them once. */
#ifndef CMD_MUTATE_DEL_STOP_PROTECTED
            while (result == 0u && (g_shell_dta.attr & DIR_ATTR_READONLY) != 0u) {
                protected = 1;
                result = dos_findnext() ? 0u : INT21_ERR_NO_MORE_FILES;
            }
#endif
            if (result != 0u) {
                if (protected) {
                    del_remaining(name, prefix);
                } else if (result == INT21_ERR_FILE_NOT_FOUND || result == INT21_ERR_NO_MORE_FILES) {
                    if (!deleted) dos_print(MSG_DOS_0003 "\r\n$");
                } else {
                    del_remaining(name, prefix);
                }
                return;
            }
            if (deleted && cmd_same_file(previous, g_shell_dta.fname)) {
                del_remaining(name, prefix);
                return;
            }
            int i;
            for (i = 0; i < 12 && g_shell_dta.fname[i]; i++)
                previous[i] = g_shell_dta.fname[i];
            previous[i] = '\0';
#ifdef CMD_MUTATE_DEL_LOSE_PARENT
            del_path(path, name, 0, previous); /* Rule 6: original wrong target. */
#else
            del_path(path, name, prefix, previous);
#endif
            uint16_t error;
#ifdef CMD_TEST_DEL_DENIED
            error = INT21_ERR_ACCESS_DENIED; /* factory-injected write refusal */
#else
            error = dos_unlink(path);
#endif
            if (error != 0u) {
#ifndef CMD_MUTATE_DEL_SILENT_ERROR
                del_remaining(name, prefix);
#endif
                return;
            }
            deleted++;
#ifdef CMD_MUTATE_DEL_CAP16
            if (deleted == 16) return; /* Rule 6: original silent remainder. */
#endif
        }
    }
}

#endif
/* REN / RENAME <old> <new> (beads initech-fyox, Tranche F). AH=56h same-dir
 * dir-entry rename. DOS prints NOTHING on success; a missing operand ->
 * "Required parameter missing" (MSG-DOS-0011); collision/missing source ->
 * "Duplicate file name" (MS-DOS 3.3 Reference p.325). The p.104 wildcard
 * substitution retains matching character positions. REN stays in place. */
static void builtin_ren(const char *arg)
{
    cmd_pair_t pair;cmd_pair_parse(arg,&pair);
    if(!pair.ok){dos_print(MSG_DOS_0011 "\r\n$");return;}
    /* p.104: destination has no path; its optional drive is ignored. */
    const char *pattern=pair.second;
    if(pattern[0] && pattern[1]==':')pattern+=2;
    for(const char *p=pattern;*p;p++)if(*p=='\\') {
        dos_print(MSG_DOS_0002 "\r\n$");return;
    }
#ifdef CMD_MUTATE_REN_ENUM
    if(dos_rename(pair.first,pair.second))dos_print(MSG_DOS_0002 "\r\n$");
#else
    /* Rename changes no directory slots. Restart by the enumeration ordinal,
     * accounting for whether the renamed entry still matches; a retained
     * pattern match must never be processed again. */
    unsigned ordinal=0;int found=0;
    char leaf[13],mapped[13],old[CMD_LINE_MAX],newname[CMD_LINE_MAX];
    while(copy_nth(pair.first,ordinal,leaf)) {
        found=1;
        if(!cmd_name_map(leaf,pattern,mapped) || !copy_path(old,pair.first,leaf) ||
           !copy_path(newname,pair.first,mapped))break;
        if(cmd_same_file(old,newname)){ordinal++;continue;}
        if(dos_rename(old,newname)) {
            dos_print(MSG_DOS_0062 "\r\n$");ordinal++;continue;
        }
        /* Re-enumerate until the current leaf. If the new spelling remains
         * selected, it occupies this ordinal; otherwise the next moves here.
         * The pure DOS matcher is the INT21 service, not a second shell glob. */
        char current[13];
        if(copy_nth(pair.first,ordinal,current) && cmd_same_file(current,mapped))ordinal++;
    }
    if(!found)dos_print(MSG_DOS_0062 "\r\n$");
#endif
}

#ifndef COMMAND_FILE_HOST
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
                dos_print(MSG_DOS_0033 "\r\n$");
            }
        } else {
            dos_print(MSG_DOS_0033 "\r\n$");
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
                dos_print(MSG_DOS_0034 "\r\n$");
            }
        } else {
            dos_print(MSG_DOS_0034 "\r\n$");
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
    static uint8_t blk[CMD_LINE_MAX + 2];
    uint8_t count;
    uint8_t i;

    blk[0] = (uint8_t)CMD_LINE_MAX;   /* max length including the CR */
    /* Ref: MS-DOS 3.3 User's Guide pp.154-155: retain count AND text for
     * AH=0Ah's F3 template; Return replaces them with the submitted line. */
    dos_getline(blk);

    count = blk[1];
    for (i = 0; i < count && i < (uint8_t)(CMD_LINE_MAX - 1); i++) {
        out[i] = (char)blk[2 + i];
    }
    out[i] = '\0';
}

/* qskz: the primary processor without AUTOEXEC asks for DATE, then TIME.
 * Empty answers accept the clock; invalid answers reprompt, successful ones
 * use AH=2Bh/2Dh. Ref: MS-DOS 3.3 Guide pp.19-20 / PDF32-33;
 * Reference pp.54-55,120-121,135 / PDF139-140,205-206,220;
 * ADR-0003 Appendix A (0Ah,2Ah-2Dh). Secondary processors do not boot DOS. */
static void startup_datetime(void)
{
    char answer[CMD_LINE_MAX];
    uint16_t year;
    uint8_t mon, day, dow, hh, mi, ss, cs;
    dos_getdate(&year, &mon, &day, &dow);
    cmd_format_date(dow, year, mon, day, g_out);
    dos_puts_raw(g_out); dos_print("\r\n$");
    for (;;) {
        dos_print("Enter new date (mm-dd-yy): $");
        read_line(answer);
        if (!answer[0]) break;
        if (cmd_parse_date(answer, &year, &mon, &day) && dos_setdate(year, mon, day)) break;
        dos_print(MSG_DOS_0033 "\r\n$");
    }
    dos_gettime(&hh, &mi, &ss, &cs);
    cmd_format_time(hh, mi, ss, cs, g_out);
    dos_puts_raw(g_out); dos_print("\r\n$");
    for (;;) {
        dos_print("Enter new time: $");
        read_line(answer);
        if (!answer[0]) break;
        if (cmd_parse_time(answer, &hh, &mi, &ss) && dos_settime(hh, mi, ss)) break;
        dos_print(MSG_DOS_0034 "\r\n$");
    }
}

/* Parse one command `line` and dispatch it to the matching built-in or to
 * run_external.  This is the SHARED dispatch the interactive REPL AND the .BAT
 * interpreter (run_batch's BL_COMMAND path) both call, so internal built-ins
 * and external programs behave identically whether typed or scripted (beads
 * initech-xw1; the refactor split out of command_repl's old inline switch).
 *
 * Returns 1 when a nonpermanent processor accepts EXIT, else 0. The REPL
 * returns on 1; run_batch unwinds the secondary's batches. A permanent
 * processor returns 0 for EXIT and continues without a diagnostic. */
static int dispatch_control(const char *line);
static void builtin_command(const char *arg);

static int dispatch_line(const char *line)
{
    cmd_line_t parsed;

    if (dispatch_control(line)) return g_shell_exit;

    cmd_parse(line, &parsed);
    const char *drive_line=line;
    while(is_space(*drive_line)) drive_line++;
    if (drive_line[0] && drive_line[1]==':') {
        const char *end=drive_line+2;
        while(is_space(*end)) end++;
        unsigned drive=(unsigned char)cmd_upcase_char(drive_line[0])-'A';
        if(!*end && drive<26u) {
            /* Real SELDISK/GETDISK, IBM DOS 3.30 TR AH=0Eh/19h. */
            uint32_t ax=0x0e00u, dx=drive;
            __asm__ __volatile__("int $0x21" : "+a"(ax) : "d"(dx) : "cc","memory");
            ax=0x1900u;
            __asm__ __volatile__("int $0x21" : "+a"(ax) : : "cc","memory");
            if ((ax&255u)!=drive) {
#ifdef CMD_MUTATE_VOLUME_MESSAGE
                dos_print(MSG_DOS_0002 "\r\n$");
#else
                dos_print(MSG_DOS_0004 "\r\n$");
#endif
            }
            return 0;
        }
    }

    switch (parsed.kind) {
        case CMD_EMPTY:
            break;                       /* blank line -> nothing to do */
        case CMD_DIR:
            builtin_dir(parsed.arg);
            break;
        case CMD_PATH:
            builtin_path(parsed.arg);
            break;
        case CMD_VOL:
            builtin_vol(parsed.arg);
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
        case CMD_COMMAND:
            builtin_command(parsed.arg);
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
            /* MS-DOS_3.3_Users_Guide_198707.pdf, Reference pp. 46, 66;
             * IBM 6138519_DOS_3.10_Reference_Feb85.pdf pp. 7-53/7-54.
             * No previous processor exists for the permanent primary.
             * The shared dispatcher also covers AUTOEXEC, IF, FOR and pipes.
             * L035 / initech-pbru. Ignoring EXIT emits no diagnostic. */
#ifndef CMD_MUTATE_PERMANENT_EXIT
            if (g_shell_permanent) break;
#endif
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

/* ---- OUTPUT redirect + dispatch (beads initech-hsct) ---------------------
 * run_output_and_dispatch owns the `>`/`>>` half: it opens the OUTPUT target,
 * DUP2's it onto stdout (handle 1), dispatches the CLEAN line, then RESTORES
 * handle 1 to CON on EVERY path.  When out_has==0 (an INPUT-only line, e.g.
 * `SORT < IN`) it just dispatches `clean` with stdout untouched.  Factored out of
 * run_with_redirect so the INPUT (`<`) wrapper can bracket it without disturbing
 * this proven OUTPUT logic (the test-hsct-redir + test-bsy9-redir gates).
 *
 * `raw_line` is the ORIGINAL command line, dispatched verbatim ONLY under the
 * CMD_MUTATE_REDIR_BYPASS mutant (which runs the command unredirected).  `clean`
 * is the fully-stripped command dispatched in a real build.
 *
 * BUILTINS emit via AH=09h/02h/06h -> stdout_emit -> the redirectable handle 1
 * (k36g); an EXTERNAL .COM EXEC child inherits the parent JFT (beads bsy.9) so
 * ITS handle 1 also lands in the file -- proven by test-bsy9-redir.
 *
 * Ref: PRD Sec 6.1; MS-DOS 3.3 Tech Ref Ch.6 (the shell DUP2's stdout around the
 *   command); spec/int21h_calling_convention.json (AH=3Ch/3Dh/3Eh/42h/45h/46h).
 *
 * MUTATION hooks (Rule 6):
 *   CMD_MUTATE_REDIR_NO_RESTORE -- skip the restore, leaving handle 1 on the file.
 *   CMD_MUTATE_REDIR_BYPASS     -- restore CON *before* dispatching + dispatch the
 *     RAW line, so `ECHO X > FILE` runs UNREDIRECTED (test-hsct-redir goes RED).
 * Both NEVER in a real build. */
/* 1 iff a write through the redirect/pipe file handle `h` (shared with the
 * command's handle 1, builtin or EXEC child) came up short or failed.
 * MUTATION hook (Rule 6): CMD_MUTATE_REDIR_IGNORE_FULL never sees a fault, so a
 * full-disk redirect or pipe is silent again (test-redir-full-emu RED). */
static int redir_write_failed(int h)
{
#ifdef CMD_MUTATE_REDIR_IGNORE_FULL
    (void)h;
    return 0;
#else
    return int21_write_fault((uint16_t)h);
#endif
}

static int run_output_and_dispatch(const char *raw_line, const char *clean,
                                   const char *target, int append, int out_has)
{
#ifndef CMD_MUTATE_REDIR_BYPASS
    (void)raw_line;   /* only the BYPASS mutant dispatches the RAW line */
#endif
    if (!out_has) {
        /* INPUT-only line: stdout stays on CON; just run the clean command. */
        return dispatch_line(clean);
    }

    /* A redirect with no target (e.g. "echo hi >") is a syntax error in DOS.
     * Fail loud: print the diagnostic and abort the line (do not dispatch). */
    if (target[0] == '\0') {
        dos_print(MSG_DOS_0011 "\r\n$");   /* "Required parameter missing" */
        return 0;
    }

    /* Open the target.  `>` create/truncate via AH=3Ch; `>>` open-existing-RDWR
     * + LSEEK-to-end, else create if absent (DOS `>>` semantics). */
    int file_h;
    if (append) {
        file_h = dos_open_mode(target, 0x02u);   /* AL=02 RDWR */
        if (file_h < 0) {
            /* Not found (or unopenable) -> create it fresh (append-to-empty). */
            file_h = dos_creat(target);
        } else {
            if (dos_lseek_end(file_h) < 0) {
                dos_close(file_h);
                dos_print(MSG_DOS_0002 "\r\n$");   /* "Bad command or file name" */
                return 0;
            }
        }
    } else {
        file_h = dos_creat(target);              /* create / truncate */
    }

    if (file_h < 0) {
        /* Could not open/create the redirection target.  Authentic DOS prints an
         * error and does NOT run the command.  The [MS-DOS] message for this is
         * "File creation error": the name is a read-only file, a directory, or
         * there is no room for it (MS-DOS 3.3 User's Guide App. F, printed
         * p.334; initech-66oc, audit L027). */
#ifdef CMD_MUTATE_REDIR_CREATE_BADCMD
        dos_print(MSG_DOS_0002 "\r\n$");   /* MUTANT: the pre-66oc catch-all */
#else
        dos_print(MSG_DOS_0046 "\r\n$");           /* "File creation error" */
#endif
        return 0;
    }

    /* Save the current stdout (handle 1) so we can restore CON afterward, then
     * force handle 1 onto the file.  If either step fails, fail loud and do NOT
     * run the command unredirected (Rule 2). */
    int saved = dos_dup(1);
    if (saved < 0) {
        dos_close(file_h);
        dos_print(MSG_DOS_0002 "\r\n$");
        return 0;
    }
    if (dos_dup2(file_h, 1) < 0) {
        dos_close(saved);
        dos_close(file_h);
        dos_print(MSG_DOS_0002 "\r\n$");
        return 0;
    }

#ifdef CMD_MUTATE_REDIR_BYPASS
    /* MUTANT: restore CON BEFORE dispatching, and dispatch the RAW line -- the
     * command runs unredirected (output leaks to screen, the file stays empty).
     * The emu gate's RZZHELLO-count==2 assertion goes RED.  NEVER in a real
     * build. */
    dos_dup2(saved, 1);
    int rc = dispatch_line(raw_line);
    dos_close(saved);
    dos_close(file_h);
    return rc;
#else
    /* Dispatch the cleaned command.  Builtins emit via stdout_emit -> handle 1
     * -> the file (k36g).  An external EXEC child inherits the parent JFT (bsy.9)
     * so its handle 1 also lands in the file.  rc==1 means EXIT was issued (we
     * still restore). */
    int rc = dispatch_line(clean);

#ifndef CMD_MUTATE_REDIR_NO_RESTORE
    /* Restore handle 1 to CON on EVERY path (including EXIT / child EXEC return).
     * The EXEC child cannot clobber the parent's `saved` slot: `saved` lives in
     * the PARENT's JFT (a free slot DUP'd from handle 1), the child gets a fresh
     * per-process JFT framed by the loader, and on the child's return g_cur_psp
     * is restored to the parent whose JFT is intact -- so dos_dup2(saved,1)
     * repoints handle 1 back at CON's SFT entry. */
    dos_dup2(saved, 1);
#endif
    dos_close(saved);
    /* A redirected write that the full disk refused (AH=40h short count, IBM
     * DOS 3.30 TR pp.6-139/6-140) must not pass as an empty success: report
     * it on CON, now that handle 1 is CON again.  "Insufficient disk space"
     * is the [MS-DOS] full-disk message (User's Guide App. F, printed p.337;
     * initech-66oc, audit L002). */
    if (redir_write_failed(file_h)) {
        dos_print(MSG_DOS_0005 "\r\n$");           /* "Insufficient disk space" */
    }
    dos_close(file_h);

    return rc;
#endif /* CMD_MUTATE_REDIR_BYPASS */
}

/* ---- I/O redirection driver (beads initech-hsct `>`/`>>` + initech-bsy.7 `<`)
 * run_with_redirect wraps dispatch_line with BOTH stdin and stdout redirection:
 * it parses `>`/`>>`/`<` off the line, sets up the INPUT redirect (open the
 * source READ-only via AH=3Dh, save handle 0 via AH=45h DUP, force handle 0 onto
 * the file via AH=46h DUP2), runs the OUTPUT+dispatch layer on the CLEAN line,
 * then RESTORES handle 0 to CON on EVERY path (and closes the source).
 *
 * The nesting is symmetric: handle 0 is set up FIRST (outer) and torn down LAST;
 * handle 1 is set up + torn down INSIDE run_output_and_dispatch (inner).  So on
 * every exit -- syntax error, open failure, dispatch, EXIT, EXEC-child return --
 * both handles return to CON and both file handles are closed (no JFT/SFT leak).
 *
 * WHO READS handle 0:  an EXTERNAL .COM EXEC child inherits the parent JFT (beads
 * bsy.9), so a child that reads stdin via AH=3Fh handle 0 (e.g. GOBBLE.COM, the
 * SORT/MORE/FIND filters once initech-m0dc lands) reads the FILE -- proven by the
 * test-bsy7-redir emu gate.  The REPL's own line reader uses AH=0Ah, which in
 * this kernel reads the keyboard device directly (NOT via the JFT handle 0), so a
 * transient `<` redirect never disturbs the interactive prompt; the handle-0
 * restore is nonetheless done for correctness (no leaked file/SFT handle).
 *
 * Ref: PRD Sec 6.1; MS-DOS 3.3 Tech Ref Ch.6 (the shell opens the `<` file and
 *   DUP2's it onto handle 0; a failed open -> "File not found", command NOT run);
 *   spec/int21h_calling_convention.json (AH=3Dh/3Eh/45h/46h).  Prereq: bsy.9
 *   (child inherits parent JFT) makes the EXEC-child `<` redirect actually flow.
 *
 * Returns run_output_and_dispatch's return code (1 = EXIT).  On an input-open
 * failure: prints the DOS "File not found" diagnostic and does NOT run the
 * command (Rule 2 fail-loud -- never silently run with the wrong stdin).
 *
 * MUTATION hook (Rule 6): CMD_MUTATE_REDIR_NO_LT (in cmd_redir_parse) drops the
 * `<` parse so the child reads the keyboard, not the file -- the test-bsy7-redir
 * emu gate + the host `<` parse oracle both go RED.  NEVER in a real build. */
static int run_with_redirect(const char *line)
{
    char clean[CMD_LINE_MAX];
    char target[CMD_LINE_MAX];
    char in_target[CMD_LINE_MAX];
    int  append = 0;

    int has = cmd_redir_parse(line, clean, (uint32_t)sizeof(clean),
                              target, (uint32_t)sizeof(target), &append,
                              in_target, (uint32_t)sizeof(in_target));
    if (!has) {
        return dispatch_line(line);     /* no redirect -> unchanged behaviour */
    }

    int out_has = (has & CMD_REDIR_OUT) != 0;
    int in_has  = (has & CMD_REDIR_IN) != 0;

    /* ---- INPUT (`<`) setup: open READ, save handle 0, DUP2 the file onto 0 ---- */
    int in_fh = -1;
    int in_saved = -1;
    if (in_has) {
        /* `cmd <` with no source is a DOS syntax error (Required parameter). */
        if (in_target[0] == '\0') {
            dos_print(MSG_DOS_0011 "\r\n$");        /* "Required parameter missing" */
            return 0;
        }
        /* AH=3Dh AL=00 OPEN-for-READ.  A nonexistent source is the DOS 3.3
         * behaviour: print "File not found" and do NOT run the command. */
        in_fh = dos_open(in_target);
        if (in_fh < 0) {
            dos_print(MSG_DOS_0003 "\r\n$");        /* "File not found" */
            return 0;
        }
        /* Save the current stdin (handle 0) so we can restore CON afterward. */
        in_saved = dos_dup(0);
        if (in_saved < 0) {
            dos_close(in_fh);
            dos_print(MSG_DOS_0002 "\r\n$");        /* "Bad command or file name" */
            return 0;
        }
        /* Force handle 0 onto the source file.  On failure, tear down cleanly and
         * do NOT run the command with the wrong stdin (Rule 2). */
        if (dos_dup2(in_fh, 0) < 0) {
            dos_close(in_saved);
            dos_close(in_fh);
            dos_print(MSG_DOS_0002 "\r\n$");        /* "Bad command or file name" */
            return 0;
        }
    }

    /* ---- OUTPUT (`>`/`>>`) + dispatch of the CLEAN line (inner layer) ---- */
    int rc = run_output_and_dispatch(line, clean, target, append, out_has);

    /* ---- INPUT restore on EVERY path (including EXIT / EXEC-child return) ---- */
    if (in_has) {
        /* The EXEC child cannot clobber the parent's `in_saved`: it lives in the
         * PARENT's JFT and the child runs under its own per-process JFT (bsy.9),
         * restored to the parent on return -- dos_dup2(in_saved,0) repoints
         * handle 0 back at CON's SFT entry. */
        dos_dup2(in_saved, 0);
        dos_close(in_saved);
        dos_close(in_fh);
    }

    return rc;
}

/* ---- PIPE executor (beads initech-bsy.8) ---------------------------------
 * Authentic DOS 3.3 implements `a | b` as a TEMP-FILE pipe (MS-DOS 3.3 Tech Ref
 * Ch.6): COMMAND.COM runs `a` with stdout redirected to a temporary file, then
 * runs `b` with stdin from that file, then DELETES the temp.  `a | b | c`
 * composes left-to-right (each interior stage feeds the next via its own temp).
 *
 * We COMPOSE the proven redirect machinery rather than duplicate it: for each
 * stage we bracket run_with_redirect (which peels the stage's OWN `<`/`>`/`>>`
 * and restores per-invocation) with an OUTER stdin (from the previous stage's
 * temp) and an OUTER stdout (to this stage's temp).  Because the OUTER handles
 * are set up BEFORE run_with_redirect and torn down AFTER, and the stage's own
 * redirects nest INSIDE, DOS precedence falls out for free: an explicit `a > f`
 * inside a piped stage wins for handle 1 (the temp stays empty) exactly as real
 * DOS does.  For `a < in | b > out` the `< in` is stage 0's own redirect and the
 * `> out` is the last stage's own redirect -- both handled by cmd_redir_parse
 * inside their stage; the pipe only supplies the inter-stage temps.
 *
 * STAGE-FAILURE semantics (CITED): DOS pipes do NOT check a stage's exit code --
 * `a` is always run to completion (writing the temp), then `b` is always run on
 * whatever the temp contains, even if `a` failed (MS-DOS 3.3 Tech Ref Ch.6: the
 * pipe is a straight temp-file substitution, not a conditional).  So a failing
 * left stage still feeds its PARTIAL temp to the right stage; we never short-
 * circuit on a stage's return except for a mid-pipeline EXIT builtin (rc==1),
 * which tears the shell down (ASSUMED: `exit` inside a pipe is degenerate; we
 * stop the pipeline but still delete every temp).
 *
 * TEMP-FILE NAME SCHEME: real MS-DOS COMMAND.COM writes pipe intermediates to a
 * file with the DOS ".$$$" TEMPORARY extension (the ".$$$" convention is DOS-
 * wide; MS-DOS 3.3 Tech Ref Ch.6).  DOS 3.3 predates the TEMP-environment
 * placement of later versions, so the intermediate lands in the CURRENT
 * directory of the default drive (single-drive A: here).  We name successive
 * intermediates PIPE1.$$$ .. PIPE7.$$$.
 *   CITED : the ".$$$" temporary extension + current-drive-root placement.
 *   ASSUMED: the exact "PIPEn" stem (the real 3.3 stem is not byte-documented).
 * A temporary is created with AH=5Bh CREATE NEW FILE, never AH=3Ch: when a user
 * file already owns PIPEn.$$$ the call fails (TR p.6-215) and the next name
 * PIPEnkkk.$$$ (kkk = 001..999) is tried, so a pipe never truncates or deletes
 * a file it did not create (TR p.6-213 "unique filenames" for temporaries;
 * initech-66oc, audit L001).  Only names this pipeline created are deleted.
 *
 * FAIL-LOUD (Rule 2): a temp-file CREATE failure (disk full) prints the DOS
 * "Insufficient disk space" diagnostic (MSG-DOS-0005), deletes any temps already
 * made, and aborts the pipeline -- never runs a stage with a bogus stdout.  A
 * producer whose writes into the temporary came up short (a full disk; AH=40h
 * short count, TR pp.6-139/6-140) is the same failure: the message is printed
 * and the consumer is NOT handed the truncated intermediate (initech-66oc,
 * audit L003).  The [MS-DOS] wording "Intermediate file error during pipe"
 * (User's Guide App. F, printed p.337) is not in the locked DEC-13 catalogue,
 * so the catalogue's full-disk message stands in for it.
 *
 * MUTATION hook (Rule 6): CMD_MUTATE_PIPE_NO_STDIN drops the OUTER stdin repoint
 * for stages 2..n, so a consumer reads the KEYBOARD instead of the pipe -- the
 * GREET|GOBBLE emu gate goes RED (GREETINGS never flows through the temp).
 * NEVER in a real build. */

/* Create the pipe temporary for intermediate `idx` (0-based) with AH=5Bh,
 * writing its name into `buf` (>= 13 bytes).  Candidate 0 is "PIPE<idx+1>.$$$";
 * when that name exists, candidates k = 1..999 are "PIPE<idx+1><kkk>.$$$".
 * Returns the handle, or <0 (the DOS error) when no name could be created.
 * MUTATION hook (Rule 6): CMD_MUTATE_PIPE_CLOBBER creates candidate 0 with
 * AH=3Ch (the pre-66oc truncate), so a user PIPE1.$$$ is destroyed again. */
static int pipe_temp_create(int idx, char *buf)
{
    for (int k = 0; k <= 999; k++) {
        int n = 0;
        buf[n++] = 'P';
        buf[n++] = 'I';
        buf[n++] = 'P';
        buf[n++] = 'E';
        buf[n++] = (char)('1' + idx);   /* PIPE1.. (idx 0-based, <= 7) */
        if (k > 0) {
            buf[n++] = (char)('0' + k / 100);
            buf[n++] = (char)('0' + (k / 10) % 10);
            buf[n++] = (char)('0' + k % 10);
        }
        buf[n++] = '.';
        buf[n++] = '$';
        buf[n++] = '$';
        buf[n++] = '$';
        buf[n] = '\0';
#ifdef CMD_MUTATE_PIPE_CLOBBER
        (void)&dos_creatnew;   /* keep it "used" under the mutant (-Werror) */
        return dos_creat(buf);
#else
        int h = dos_creatnew(buf);
        if (h != -(int)INT21_ERR_FILE_EXISTS) {
            return h;   /* created, or a failure no other name can cure */
        }
#endif
    }
    return -(int)INT21_ERR_FILE_EXISTS;
}

/* Execute a multi-stage pipeline.  Returns 1 iff a stage issued EXIT (tear the
 * REPL down), else 0.  Deletes every temp on EVERY path (Rule 2). */
static int run_pipeline(cmd_pipeline_t *pl)
{
    char tname[CMD_PIPE_MAX_STAGES][13];   /* tname[i] = temp AFTER stage i     */
    int  created[CMD_PIPE_MAX_STAGES];     /* 1 iff we created (must delete) it  */
    int  rc = 0;
    int  i;

    for (i = 0; i < CMD_PIPE_MAX_STAGES; i++) {
        created[i] = 0;
    }

    for (i = 0; i < pl->nstages; i++) {
        int is_first = (i == 0);
        int is_last  = (i == pl->nstages - 1);

        int out_h = -1, out_saved = -1;   /* OUTER stdout -> temp[i]            */
        int in_h  = -1, in_saved  = -1;   /* OUTER stdin  <- temp[i-1]          */

        /* ---- OUTER stdout: an interior/producer stage writes to temp[i] ---- */
        if (!is_last) {
            out_h = pipe_temp_create(i, tname[i]);
            if (out_h < 0) {
                /* Disk full / cannot create the intermediate -> fail loud. */
                dos_print(MSG_DOS_0005 "\r\n$");   /* "Insufficient disk space" */
                goto cleanup;
            }
            created[i] = 1;
            out_saved = dos_dup(1);
            if (out_saved < 0) {
                dos_close(out_h);
                dos_print(MSG_DOS_0002 "\r\n$");    /* "Bad command or file name" */
                goto cleanup;
            }
            if (dos_dup2(out_h, 1) < 0) {
                dos_close(out_saved);
                dos_close(out_h);
                dos_print(MSG_DOS_0002 "\r\n$");
                goto cleanup;
            }
        }

        /* ---- OUTER stdin: a consumer stage reads from temp[i-1] ---- */
#ifndef CMD_MUTATE_PIPE_NO_STDIN
        if (!is_first) {
            in_h = dos_open(tname[i - 1]);   /* AH=3Dh AL=00 read the intermediate */
            if (in_h >= 0) {
                in_saved = dos_dup(0);
                if (in_saved >= 0) {
                    if (dos_dup2(in_h, 0) < 0) {
                        dos_close(in_saved);
                        in_saved = -1;
                        dos_close(in_h);
                        in_h = -1;
                    }
                } else {
                    dos_close(in_h);
                    in_h = -1;
                }
            }
        }
#else
        /* MUTANT: the consumer never gets its stdin repointed at the temp, so it
         * reads the keyboard -- the piped data never flows (gate RED). */
        (void)is_first;
#endif

        /* ---- run the stage; its OWN `<`/`>`/`>>` are peeled INSIDE (nested) ---- */
        rc = run_with_redirect(pl->stage[i]);

        /* ---- tear down the OUTER handles (stdin first, then stdout) ---- */
        if (in_saved >= 0) {
            dos_dup2(in_saved, 0);
            dos_close(in_saved);
        }
        if (in_h >= 0) {
            dos_close(in_h);
        }
        if (out_saved >= 0) {
            dos_dup2(out_saved, 1);
            dos_close(out_saved);
        }
#ifdef CMD_MUTATE_PIPE_IGNORE_FULL
        int lost = 0;   /* MUTANT (Rule 6): a short intermediate passes silently */
#else
        int lost = out_h >= 0 && redir_write_failed(out_h);
#endif
        if (out_h >= 0) {
            dos_close(out_h);
        }

        if (rc == 1 || g_shell_break.interrupted) {
            /* EXIT or ^C stops later stages AFTER handle restoration. */
            break;
        }
        if (lost) {
            /* The intermediate is short: report it on CON and stop, never
             * feed the consumer a truncated pipe (audit L003). */
            dos_print(MSG_DOS_0005 "\r\n$");   /* "Insufficient disk space" */
            break;
        }
    }

cleanup:
    /* Delete every intermediate on EVERY path (incl. disk-full abort + EXIT).
     * Authentic DOS removes the pipe temp(s) when the pipeline finishes. */
    for (i = 0; i < CMD_PIPE_MAX_STAGES; i++) {
        if (created[i]) {
            (void)dos_unlink(tname[i]);
        }
    }
    return rc;
}

/* Top-level command-line entry: split on `|`, run the pipeline if there is one,
 * else fall through to the proven single-command redirect driver UNCHANGED (the
 * no-pipe path is byte-identical to before -- run_with_redirect on the RAW line).
 * All the shell's user-command call sites (the REPL + every .BAT dispatch leg)
 * route through here so pipes work everywhere redirects do. */
static int run_pipeline_or_redirect(const char *line)
{
    cmd_pipeline_t pl;

    int n = cmd_pipe_split(line, &pl);
    if (n <= 1) {
        return run_with_redirect(line);   /* no `|` -> unchanged behaviour */
    }
    return run_pipeline(&pl);
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
 * (fail-safe: we interpret what we read, never overrun).  Was cut to 2 KiB
 * during the kernel-window crunch (env+mz+batch+mvg+device-chain); restored
 * to 4 KiB by bead initech-o0td (whole-map +0x8000 shift gives 32 KiB of
 * kernel headroom -- the extra 2 KiB .bss here is well within that budget). */
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

/* PROMPT $D uses the same AH=2Ah clock seam as DATE, for both echoed
 * batch lines and interactive prompts. MS-DOS 3.3 Reference p.101;
 * ADR-0003 Appendix A (2Ah-2Dh). */
static void prompt_date(prompt_ctx_t *ctx)
{
    uint16_t year=0;
    uint8_t mon=0, day=0, dow=0;
#ifndef CMD_MUTATE_PROMPT_DATE
    dos_getdate(&year,&mon,&day,&dow);
#else
    (void)dow;
#endif
    ctx->year=year; ctx->month=mon; ctx->day=day;
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
    pctx.drive = prompt_drive();
    pctx.cwd   = cwd_rel;
    {
        uint8_t hh = 0, mi = 0, ss = 0, cs = 0;
        dos_gettime(&hh, &mi, &ss, &cs);
        pctx.hour = (int)hh; pctx.minute = (int)mi;
        pctx.second = (int)ss; pctx.centisec = (int)cs;
    }
    prompt_date(&pctx);

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
        if (got == DOS_READ_ERROR) {
            /* initech-winh: a read failure is NOT a byte count -- abort the
             * WHOLE load (like the can't-open case above) rather than advance
             * `total` by the error code and hand run_batch a truncated buffer
             * with a corrupt tail to interpret as script text. The `len < 0`
             * fail-safe at both call sites (run_batch / the CALL reload) then
             * takes the SAME path as "could not open". */
            dos_close(fh);
            return -1;
        }
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

/* A batch frame owns control flow, positional parameters and ECHO state.
 * IF/FOR bodies enter the same dispatcher as physical lines and prompt input.
 * Ref: MS-DOS 3.3 Reference pp.141-149,152; ADR-0003 Appendix A. */
typedef struct batch_frame {
    const char *path;
    const char *view[10];
    int count, pos, len, echo_on;
    unsigned branch;
    struct batch_frame *previous;
} batch_frame_t;
static batch_frame_t *g_batch_frame;
static unsigned g_body_depth;
/* Snapshot names before executing bodies: DIR, nested FOR and CALL all use
 * FINDFIRST/NEXT too. One bounded arena, reclaimed on FOR return, avoids
 * overwriting a live enumeration or putting file lists on the kernel stack. */
static char g_for_names[BATCH_FILE_MAX];
static unsigned g_for_used;

static void control_body(const char *line)
{
    if (g_body_depth >= BATCH_NEST_MAX) {
        dos_print(MSG_DOS_0037 "\r\n$");
        return;
    }
    g_body_depth++;
    (void)run_pipeline_or_redirect(line);
    g_body_depth--;
}

static int for_store(const char *name)
{
    unsigned n = str_len(name) + 1u;
    if (n > sizeof(g_for_names) - g_for_used) {
        dos_print(MSG_DOS_0035 "\r\n$");
        return 0;
    }
    for (unsigned i = 0; i < n; i++) g_for_names[g_for_used++] = name[i];
    return 1;
}

static void control_for(const char *args)
{
    char var[BATCH_LABEL_MAX], set[BATCH_LINE_MAX], tmpl[BATCH_LINE_MAX];
    char tok[BATCH_LINE_MAX];
    int cur = 0;
    unsigned mark = g_for_used;
    batch_frame_t *frame = g_batch_frame;
    unsigned branch = frame ? frame->branch : 0u;
    if (!batch_for_parse(args, var, set, tmpl)) return;
    /* Physical batch lines have already collapsed %% to %. Interactive FOR
     * uses % directly (Reference pp.144-145). */
    if (var[0] != '%' || !var[1] || var[2] ||
        (var[1] >= '0' && var[1] <= '9')) return;
    while (batch_for_next_token(set, &cur, tok, sizeof(tok))) {
#ifndef CMD_MUTATE_FOR_WILDCARD
        if (cmd_has_wildcard(tok)) {
            char name[BATCH_LINE_MAX + 13];
            unsigned prefix = 0;
            for (unsigned i=0; tok[i]; i++)
                if (tok[i]=='\\' || tok[i]=='/' || tok[i]==':') prefix=i+1;
            dos_setdta(&g_shell_dta);
            if (dos_findfirst(tok, 0u)) do {
                /* AH=4Eh attr=0 selects files. Keep any supplied path prefix
                 * when substituting the matched 8.3 name. */
                del_path(name, tok, (int)prefix, g_shell_dta.fname);
                if (!for_store(name)) goto done;
            } while (dos_findnext());
            break; /* Reference p.144: wildcard set has one member. */
        }
#endif
        if (!for_store(tok)) goto done;
    }
    {
        unsigned end = g_for_used;
        for (unsigned p=mark; p<end && !g_shell_exit && !g_shell_break.interrupted;
             p += str_len(g_for_names+p)+1u) {
            char cmd[BATCH_LINE_MAX];
            if (batch_for_subst(tmpl,var,g_for_names+p,cmd,sizeof(cmd)) >= 0)
                control_body(cmd);
            if (frame && frame->branch != branch) break; /* GOTO leaves FOR */
        }
    }
done:
    g_for_used = mark;
}

/* Return 1 for control commands, 0 for ordinary built-ins/executables. */
static int dispatch_control(const char *line)
{
    batch_parsed_t bp;
    batch_frame_t *frame = g_batch_frame;
    const char *args = line;
#ifdef CMD_MUTATE_BATCH_BODY
    if (g_body_depth) return 0;
#endif
    while (*args==' ' || *args=='\t' || *args=='@') args++;
    batch_classify(args,&bp);
    while (*args && *args!=' ' && *args!='\t') args++;
    while (*args==' ' || *args=='\t') args++;
    switch (bp.kind) {
    case BL_BLANK: case BL_REM: case BL_LABEL: return 1;
    case BL_ECHO_ON: case BL_ECHO_OFF:
        if (frame) frame->echo_on = bp.kind == BL_ECHO_ON;
        return frame != 0;
    case BL_ECHO_TEXT:
        if (frame && !bp.echo_text[0]) {
            dos_print(frame->echo_on ? MSG_DOS_0027 "\r\n$" : MSG_DOS_0028 "\r\n$");
            return 1;
        }
        return 0;
    case BL_IF: {
        const char *body = 0;
        if (batch_eval_if(args,g_errorlevel,batch_exist_probe,0,&body) && body && *body)
            control_body(body);
        return 1;
    }
    case BL_FOR:
#ifdef CMD_MUTATE_FOR_PROMPT
        if (!frame) return 0;
#endif
        control_for(args);
        return 1;
    case BL_GOTO:
        if (frame) {
            int target=batch_find_label(g_batch_buf,frame->len,bp.goto_target);
            if (target<0) dos_print(MSG_DOS_0036 "\r\n$");
            frame->pos = target<0 ? frame->len : target;
            frame->branch++;
        }
        return 1;
    case BL_SHIFT:
        /* Reference p.151: SHIFT also discards the original %0. */
        if (frame && frame->count>0) {
#ifdef CMD_MUTATE_SHIFT_ZERO
            int first=1;
#else
            int first=0;
#endif
            for (int k=first;k<frame->count-1;k++) frame->view[k]=frame->view[k+1];
            frame->count--;
        }
        return 1;
    case BL_CALL: {
        char callee[CMD_TOKEN_MAX], resolved[CMD_PATH_MAX_LEN];
        int n=0;
        while (*args && *args!=' ' && *args!='\t') {
            if (n<CMD_TOKEN_MAX-1) callee[n++]=*args;
            args++;
        }
        callee[n]=0;
        cmd_upcase_str(callee);
        if (resolve_bat_path(callee,resolved)) {
            run_batch_invoke(resolved,args);
            if (frame) frame->len=batch_load_file(frame->path,g_batch_buf);
        } else dos_print(MSG_DOS_0002 "\r\n$");
        return 1;
    }
    case BL_PAUSE:
        /* The manual's batch PAUSE wording differs from DIR /P's message. */
        dos_print("Strike a key when ready . . .\r\n$");
        (void)dos_conin();
        return 1;
    default: return 0;
    }
}

static void run_batch(const char *path, const char *const argv[], int argc)
{
    batch_frame_t frame;
    if (g_batch_depth >= BATCH_NEST_MAX) {
        dos_print(MSG_DOS_0037 "\r\n$"); return;
    }
    frame.path=path; frame.count=argc>10 ? 10 : argc;
    for (int k=0;k<frame.count;k++) frame.view[k]=argv[k];
    frame.pos=0; frame.echo_on=1; frame.branch=0;
    frame.len=batch_load_file(path,g_batch_buf);
    frame.previous=g_batch_frame;
    g_batch_frame=&frame;
    g_batch_depth++;
    while (frame.pos<frame.len && !g_shell_exit && !g_shell_break.interrupted) {
        char line[BATCH_LINE_MAX], expanded[BATCH_LINE_MAX];
        batch_parsed_t bp;
        if (!batch_next_line(g_batch_buf,frame.len,&frame.pos,line)) break;
        batch_classify(line,&bp);
        if (bp.kind==BL_BLANK || bp.kind==BL_REM || bp.kind==BL_LABEL) continue;
        const char *raw=line;
        while (*raw==' ' || *raw=='\t') raw++;
        if (*raw=='@') raw++;
        if (batch_expand(raw,frame.view,frame.count,batch_env_cb,&g_master_env,
                         expanded,sizeof(expanded))<0) continue;
        /* Preserve the established batch line-echo policy. Control bodies
         * execute after this single physical-line parameter expansion. */
        if (frame.echo_on && !bp.at_suppressed && bp.kind==BL_COMMAND)
            batch_echo_line(expanded);
        (void)run_pipeline_or_redirect(expanded);
    }
    g_batch_depth--;
    g_batch_frame=frame.previous;
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

/* Reference pp.46-47: /C creates a private environment, executes one command
 * and returns automatically. Existing redirect/pipe scopes save and restore
 * the parent's handles; EXIT and batch control are isolated as well. */
static void builtin_command(const char *arg)
{
    static unsigned depth;
    while(is_space(*arg))arg++;
    if(arg[0]!='/' || cmd_upcase_char(arg[1])!='C' ||
       (arg[2] && !is_space(arg[2]))) {
        dos_print(MSG_DOS_0038 "\r\n$");return;
    }
    arg+=2;while(is_space(*arg))arg++;
    if(depth>=8){dos_print(MSG_DOS_0035 "\r\n$");return;}
    env_store_t env=g_master_env;
    int exit=g_shell_exit,permanent=g_shell_permanent;
    uint8_t errorlevel=g_errorlevel;
    batch_frame_t *frame=g_batch_frame;
    g_shell_exit=0;g_shell_permanent=0;g_batch_frame=0;
    depth++;
#ifndef CMD_MUTATE_COMMAND_C
    run_with_redirect(arg);
#endif
    depth--;
#ifndef CMD_MUTATE_COMMAND_ENV
    g_master_env=env;
#else
    (void)env;
#endif
    g_shell_exit=exit;g_shell_permanent=permanent;
    g_errorlevel=errorlevel;g_batch_frame=frame;
}

/* EXEC of COMSPEC by a resident program (initech-n8q8). The interpreter is
 * resident in this kernel (ADR-0003 DEC-03/DEC-11), so the "second copy" of
 * COMMAND.COM a dBASE RUN asks for is a secondary processor of the same code:
 * the PSP:80h tail (no NUL; it ends at its length or the CR) is run as
 * `COMMAND <tail>`, with /C's private environment, EXIT/batch isolation and
 * redirect scopes saved and restored around it (Reference pp. 46-47). */
void command_exec_secondary(const char *tail, uint32_t len)
{
    char line[CMD_LINE_MAX];
    uint32_t i;
    for (i = 0u; tail && i < len && i < CMD_LINE_MAX - 1u && tail[i] != '\r'; i++)
        line[i] = tail[i];
    line[i] = '\0';
    builtin_command(line);
}

void command_repl(int permanent)
{
    char line[CMD_LINE_MAX];
    g_shell_permanent = permanent != 0;
    g_shell_exit = 0;
    /* COMMAND.COM is resident in the kernel PSP. An INT 23h during a built-in
     * must unwind the command's normal CLOSE/DUP2 epilogues, then reprompt;
     * terminating this PSP would destroy saved redirection handles and halt.
     * Ref: 80X0945_DOS_3.30_Technical_Reference_Apr87.pdf pp. 6-14/15;
     * MS-DOS_3.3_Users_Guide_198707.pdf p. 159. A child EXEC uses its own PSP. */
    g_shell_break.owner = int21_get_psp();
    g_shell_break.interrupted = 0u;
    int21_break_recovery_t *previous_break = int21_set_break_recovery(&g_shell_break);

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
     * AUTOEXEC.BAT runs an EXIT, only a nonpermanent processor unwinds; the
     * primary continues the batch. Recheck before entering the loop.
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
        else if (g_shell_permanent) {
#ifndef CMD_MUTATE_STARTUP
            startup_datetime();
#else
            (void)&startup_datetime; /* Rule 6: omit both boot prompts. */
#endif
        }
    }
    if (g_shell_exit) {
        int21_set_break_recovery(previous_break);
        return;     /* AUTOEXEC.BAT issued EXIT -> the shell is done */
    }

    /* A serial marker the oracle gates key-injection on (mirrors KBD-ECHO-READY
     * / CONIN-PROG-READY). The kernel's CON sink also fans this to serial via
     * the AH=09h banner, but a distinct grep-able marker keeps the gate robust;
     * kmain prints SHELL-READY before calling us, so we don't duplicate it. */
    for (;;) {
        g_shell_break.interrupted = 0u;
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
            pctx.drive    = prompt_drive();
            pctx.cwd      = cwd_rel;   /* root-relative, passed to $P expander */
            /* Time and date come from the live INT21 clock seam. */
            {
                uint8_t hh = 0, mi = 0, ss = 0, cs = 0;
                dos_gettime(&hh, &mi, &ss, &cs);
                pctx.hour     = (int)hh;
                pctx.minute   = (int)mi;
                pctx.second   = (int)ss;
                pctx.centisec = (int)cs;
            }
            prompt_date(&pctx);

            /* Render into a bounded scratch buffer, then write the exact
             * byte count so an embedded dollar cannot terminate output. */
            char rendered[INT21_CWD_MAX + 32];  /* generous; no overflow possible */
            int rlen = cmd_render_prompt(templ, &pctx,
                                         rendered, (int)(sizeof(rendered) - 1));
            /* Reference p.101: $$ is a literal dollar. AH=40h writes
             * exactly the rendered bytes, including every embedded '$'. */
#ifdef CMD_MUTATE_PROMPT_DOLLAR
            rendered[rlen++] = '$';
            rendered[rlen] = '\0';
            dos_print(rendered);
#else
            dos_write(rendered, (uint32_t)rlen);
#endif
        }
#endif

        read_line(line);
        if (g_shell_break.interrupted) continue; /* ^C at prompt discards line */

        /* Dispatch through the SHARED path the .BAT interpreter also uses
         * (beads initech-xw1), via the PIPE + I/O-redirection driver: pipes
         * (`a | b`, initech-bsy.8) split into temp-file stages, and each stage's
         * `>`/`>>`/`<` (initech-hsct + bsy.7) repoint its stdout/stdin.  A plain
         * line with neither passes straight through to dispatch_line.  A return
         * value of 1 means EXIT was issued (print already emitted); tear the REPL
         * down. */
        if (run_pipeline_or_redirect(line)) {
            int21_set_break_recovery(previous_break);
            return;
        }
    }
}

#endif /* !COMMAND_FILE_HOST */
#endif /* COMMAND_KERNEL_REPL || COMMAND_FILE_HOST */
