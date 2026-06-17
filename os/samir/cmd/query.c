/*
 * os/samir/cmd/query.c -- SAMIR (InitechBase) query / display command module.
 *                          Step S5.4 (Phase 5 interpreter).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib). Uses
 * ONLY <stdint.h> and the samir/ engine headers. No libc, no int 0x21 -- every
 * byte of OS contact goes through the PAL carried by the interpreter (console
 * output via pal->conout). Memory is fixed/static (no malloc). ASCII-clean
 * (Rule 12). Fail loud (Rule 2). Reproducible (Rule 11).
 *
 * dBASE III PLUS 1.1 ONLY (plan Sec 2.C).
 *
 * WHAT THIS IS (the S5.4 contract): a single xb_cmd_hook registered into the
 * executor's command-hook CHAIN (interp.h xb_interp_add_cmd_hook). It adds the
 * QUERY/DISPLAY verbs without editing flow.c:
 *
 *   LIST    [<scope>] [<expr list>] [FOR <cond>] [WHILE <cond>] [OFF]
 *   DISPLAY [<scope>] [<expr list>] [FOR <cond>] [WHILE <cond>] [OFF]
 *   ?  [<expr list>]      (leading CR/LF, then the values, then a newline)
 *   ?? [<expr list>]      (no leading CR/LF)
 *   LOCATE [<scope>] [FOR <cond>] [WHILE <cond>]   (first match; sets RECNO+FOUND)
 *   CONTINUE                                        (re-applies the LOCATE FOR only)
 *   SEEK <expr>           (indexed search on the master index; sets RECNO+FOUND)
 *   FIND <literal>        (legacy indexed search; argument is an unquoted literal)
 *
 * SCOPE / FOR / WHILE (navigation-query-display.md "Scope, FOR, and WHILE"):
 *   scope = RECORD <n> | NEXT <n> | REST | ALL.
 *   LIST default scope = ALL (implicit GO TOP); DISPLAY default = current record.
 *   LOCATE default scope = ALL (implicit GO TOP).
 *   FOR <cond>:   visit EVERY record in scope; process those where cond is .T.
 *                 (does NOT stop at the first false).
 *   WHILE <cond>: process records only WHILE cond is .T.; STOP at the first false.
 *   A <cond> MUST evaluate to Logical (XB_L) -- else fail loud #37 (no truthiness).
 *
 * LOCATE / CONTINUE (the headline S5.4 rule):
 *   LOCATE FOR <cond> [scope] finds the FIRST matching record (sets RECNO, FOUND
 *   .T., EOF .F.); on no match the area is at EOF, FOUND .F., EOF .T.
 *   CONTINUE re-applies ONLY the stored FOR condition (NOT the WHILE, NOT the
 *   scope) from the record AFTER the current one, forward to the next match or
 *   EOF. CONTINUE with no prior LOCATE in this area is fail loud #42 "CONTINUE
 *   without LOCATE." [navigation-query-display.md CONTINUE: "re-applies only the
 *   FOR condition ... does NOT re-apply the original scope ... or the WHILE".]
 *
 * SEEK / FIND:
 *   Resolve through the master (controlling) index via ndx_seek; set RECNO to the
 *   landing recno and FOUND per the match, EOF on no match. No master index =>
 *   fail loud #26 "Database is not indexed." SEEK takes an EXPRESSION (evaluated;
 *   its type must match the key type); FIND takes a bare unquoted literal char
 *   string. SET EXACT (ctx->set_exact) governs the char begins-with vs exact rule
 *   inside ndx_seek (III+ default OFF).
 *
 * FOUND() (un-gating S3.6b): every SEEK/FIND/LOCATE/CONTINUE calls wa_set_found
 * so the FOUND() built-in (read through the DB-cursor vtable wac_found) reflects
 * the last search. GO/SKIP/LIST/DISPLAY/? do NOT touch FOUND().
 *
 * GATED (loud-skip; plan Sec 7 / GAPS secP): SET FILTER / SET DELETED interaction
 *   with scope (SET FILTER/DELETED are S5.6 -- not yet implemented; the walk sees
 *   every physical record), and any mint-only transcript edge. The oracle prints
 *   a loud skip for those, never a silent pass.
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/commands/navigation-query-display.md (LIST/DISPLAY,
 *     ?/??, LOCATE, CONTINUE, SEEK, FIND; the scope/FOR/WHILE grammar; pointer +
 *     FOUND/EOF effects; the CONTINUE FOR-only rule).
 *   - spec/samir/dbase_msg_codes.tsv (#26 not indexed, #37 not logical, #42
 *     CONTINUE without LOCATE).
 *   - os/samir/include/samir/interp.h (the xb_cmd_hook contract + chain).
 *   - os/samir/include/samir/workarea.h (wa_recno/wa_nrec/wa_eof/wa_goto/
 *     wa_table/wa_index/wa_master_order/wa_set_found).
 *   - os/samir/include/samir/nav.h (wa_nav_go_top/goto/skip).
 *   - os/samir/include/samir/ndx.h (ndx_seek on the master index).
 *   - os/samir/include/samir/eval.h (xb_lex/xb_parse/xb_eval; xb_ctx; set_exact).
 *   - os/samir/include/samir/pal.h (conout -- the only OS surface here).
 */

#include <stdint.h>

#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/dbf.h"
#include "samir/ndx.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/pal.h"

/* ===================================================================== */
/* Tunables (bounded -- Rule 2 fail-loud on overflow, never UB)            */
/* ===================================================================== */

#define Q_MAX_TOKS     128   /* per-expression token pool */
#define Q_MAX_NODES    128   /* per-expression node pool */
#define Q_MAX_EXPR     254   /* one expression / condition working copy */
#define Q_MAX_FORLEN   254   /* stored LOCATE FOR condition text */
#define Q_MAX_FIELDS   128   /* III+ field cap (for the default LIST field list) */
#define Q_LINEBUF      512   /* one rendered LIST/? output line */
#define Q_REGISTRY     16    /* concurrent interpreters with query state */

/* dBASE catalog ordinals used by this module (mirrors eval.h XBEE_* where they
 * exist; the two below are not in eval.h -- they are command-level codes). */
#define Q_MSG_NOT_INDEXED  26   /* "Database is not indexed." */
#define Q_MSG_NO_LOCATE    42   /* "CONTINUE without LOCATE." */
/* XBEE_NOT_LOGICAL (#37) and XBEE_MISMATCH (#9) come from eval.h. */

/* ===================================================================== */
/* Per-interpreter query state (the LOCATE FOR memory for CONTINUE).       */
/* Kept in a bounded static registry, same idiom as flow.c -- the engine   */
/* is single-threaded cooperative; no malloc.                              */
/* ===================================================================== */

typedef struct {
    int      have_locate;            /* 1 once a LOCATE has run in some area */
    int      locate_area;            /* the area the last LOCATE was run in */
    char     for_text[Q_MAX_FORLEN + 1]; /* the stored FOR condition, NUL-term */
    int      has_for;                /* 1 if the stored LOCATE had a FOR clause */
} query_state;

static xb_interp   *g_key[Q_REGISTRY];
static query_state  g_val[Q_REGISTRY];
static int          g_n;

static query_state *query_state_for(xb_interp *ip)
{
    int i;
    if (!ip)
        return (query_state *)0;
    for (i = 0; i < g_n; i++) {
        if (g_key[i] == ip)
            return &g_val[i];
    }
    if (g_n >= Q_REGISTRY)
        return (query_state *)0;        /* too many live interpreters: fail loud */
    g_key[g_n] = ip;
    g_val[g_n].have_locate = 0;
    g_val[g_n].locate_area = 0;
    g_val[g_n].for_text[0] = '\0';
    g_val[g_n].has_for     = 0;
    return &g_val[g_n++];
}

/* ===================================================================== */
/* Small ASCII helpers (freestanding; no libc)                            */
/* ===================================================================== */

static char q_up1(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - 'a' + 'A');
    return c;
}

static int q_isspace(char c) { return c == ' ' || c == '\t'; }

static int q_strlen(const char *s)
{
    int n = 0;
    while (s[n] != '\0') n++;
    return n;
}

/* case-insensitive equality of two NUL-terminated strings. */
static int q_ci_eq(const char *a, const char *b)
{
    int i = 0;
    for (;;) {
        char ca = q_up1(a[i]);
        char cb = q_up1(b[i]);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
        i++;
    }
}

/* skip leading blanks; return pointer past them. */
static const char *q_skip_ws(const char *s)
{
    while (*s != '\0' && q_isspace(*s)) s++;
    return s;
}

/* write n bytes to the console. */
static void q_putn(xb_interp *ip, const char *s, uint32_t n)
{
    samir_pal_t *pal = xb_interp_pal(ip);
    if (pal && pal->conout && n > 0u)
        pal->conout(pal, s, n);
}

/* ===================================================================== */
/* Expression evaluation through the interp ctx (carries flow's resolver) */
/* ===================================================================== */

/*
 * q_eval: lex+parse+eval the NUL-terminated `expr` through ip's ctx. On success
 * returns 0 with *out set; else a negative interp_err with *ec the catalog
 * ordinal / stage detail.
 */
static int q_eval(xb_interp *ip, const char *expr, xb_val *out, int *ec)
{
    xb_token toks[Q_MAX_TOKS];
    xb_node  pool[Q_MAX_NODES];
    xb_ctx  *ctx = xb_interp_ctx(ip);
    int ntok, root, rc, e = 0;
    uint32_t len = 0u;

    while (expr[len] != '\0') len++;

    ntok = xb_lex(expr, len, toks, (uint32_t)Q_MAX_TOKS, &e);
    if (ntok < 0) { if (ec) *ec = e; return -INTERP_ERR_LEX; }

    root = xb_parse(toks, (uint32_t)ntok, pool, (uint32_t)Q_MAX_NODES, &e);
    if (root < 0) { if (ec) *ec = e; return -INTERP_ERR_PARSE; }

    rc = xb_eval(pool, root, ctx, out, &e);
    if (rc != 0) { if (ec) *ec = e; return -INTERP_ERR_EVAL; }

    if (ec) *ec = 0;
    return INTERP_OK;
}

/*
 * q_eval_cond: evaluate `expr` and REQUIRE a Logical result (*truth gets 0/1).
 * A non-Logical result is fail-loud error #37 (XBEE_NOT_LOGICAL) -- no
 * truthiness (navigation-query-display.md "Not a Logical expression.").
 */
static int q_eval_cond(xb_interp *ip, const char *expr, int *truth, int *ec)
{
    xb_val v;
    int rc = q_eval(ip, expr, &v, ec);
    if (rc != INTERP_OK)
        return rc;
    if (v.t != XB_L) {
        if (ec) *ec = XBEE_NOT_LOGICAL;
        return -INTERP_ERR_EVAL;
    }
    *truth = v.u.l ? 1 : 0;
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* Value -> display text rendering (for LIST/DISPLAY/? )                   */
/* ===================================================================== */

/* append `n` bytes of `s` to buf[*pos], bounded by cap. */
static void q_append(char *buf, uint32_t *pos, uint32_t cap, const char *s, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n && *pos < cap; i++)
        buf[(*pos)++] = s[i];
}

static void q_append_cstr(char *buf, uint32_t *pos, uint32_t cap, const char *s)
{
    q_append(buf, pos, cap, s, (uint32_t)q_strlen(s));
}

/* format an unsigned integer right-justified in `width` (space-padded) into
 * buf[*pos]. If the number is wider than `width` it is written in full. */
static void q_append_uint_rj(char *buf, uint32_t *pos, uint32_t cap,
                             uint32_t v, int width)
{
    char tmp[12];
    int n = 0, i, pad;
    if (v == 0u) {
        tmp[n++] = '0';
    } else {
        char rev[12];
        int r = 0;
        while (v > 0u && r < (int)sizeof(rev)) {
            rev[r++] = (char)('0' + (v % 10u));
            v /= 10u;
        }
        for (i = r - 1; i >= 0; i--)
            tmp[n++] = rev[i];
    }
    pad = width - n;
    for (i = 0; i < pad; i++)
        q_append(buf, pos, cap, " ", 1u);
    q_append(buf, pos, cap, tmp, (uint32_t)n);
}

/*
 * q_render_val: render one xb_val to display text into buf[*pos] (bounded by cap).
 * dBASE display forms (navigation-query-display.md "Output presentation" + ?/??):
 *   C/M : the characters as-is (raw bytes).
 *   N   : right-formatted; we use dec_format with a generous width then trim the
 *         leading spaces so a bare "?" prints a compact number (matching the
 *         dot-prompt's left-trimmed numeric echo). dec places from the field's
 *         own dec_count is not known here, so we render with 0 decimals for an
 *         integral value and up to a small precision otherwise.
 *   D   : YYYYMMDD/AMERICAN -- we render MM/DD/YY (SET DATE AMERICAN default).
 *   L   : "T" / "F" (LIST column form; ? prints .T./.F. but the oracle asserts
 *         on field values, so we use the LIST "T"/"F" form here uniformly).
 *   U   : blank (an unset value renders empty).
 */
static void q_render_val(char *buf, uint32_t *pos, uint32_t cap, const xb_val *v)
{
    switch (v->t) {
    case XB_C:
    case XB_M:
        q_append(buf, pos, cap, v->u.c.p ? v->u.c.p : "", (uint32_t)v->u.c.len);
        break;
    case XB_N: {
        /* Render the numeric value. Choose 0 decimals if it is integral, else 2.
         * dec_format ties-to-+inf; we left-trim the field-padding spaces. */
        char tmp[40];
        int width = 20, dec = 0, w, lead = 0;
        double n = v->u.n, fr;
        /* integral test: fractional part within a small epsilon */
        fr = n - (double)(int64_t)n;
        if (fr < 0.0) fr = -fr;
        if (fr > 1e-9) dec = 2;
        w = dec_format(n, width, dec, tmp);   /* writes exactly `width` bytes */
        while (lead < w && tmp[lead] == ' ') lead++;
        q_append(buf, pos, cap, tmp + lead, (uint32_t)(w - lead));
        break;
    }
    case XB_D: {
        /* JDN -> MM/DD/YY (SET DATE AMERICAN, the III+ default, CENTURY OFF). */
        int32_t y, m, d;
        char dt[9];
        int k = 0;
        ymd_from_jdn((int32_t)v->u.d, &y, &m, &d);
        dt[k++] = (char)('0' + (m / 10) % 10);
        dt[k++] = (char)('0' + m % 10);
        dt[k++] = '/';
        dt[k++] = (char)('0' + (d / 10) % 10);
        dt[k++] = (char)('0' + d % 10);
        dt[k++] = '/';
        dt[k++] = (char)('0' + (y / 10) % 10);
        dt[k++] = (char)('0' + y % 10);
        q_append(buf, pos, cap, dt, (uint32_t)k);
        break;
    }
    case XB_L:
        q_append(buf, pos, cap, v->u.l ? "T" : "F", 1u);
        break;
    case XB_U:
    default:
        /* blank */
        break;
    }
}

/* ===================================================================== */
/* The LIST/DISPLAY <expression list> parser                              */
/*                                                                        */
/* An expression list is comma-separated; commas inside string literals    */
/* and parens are NOT separators. We split into individual expression       */
/* strings, evaluate each per record, and render with one space between.    */
/* ===================================================================== */

/*
 * q_next_expr: copy the next top-level comma-delimited expression from *p into
 * `dst` (cap bytes, NUL-terminated), trimming surrounding blanks, and advance *p
 * past the comma. Returns 1 if an expression was produced, 0 at end of list.
 * Sets *ok=0 (and returns 0) on overflow (fail loud at the caller).
 */
static int q_next_expr(const char **p, char *dst, uint32_t cap, int *ok)
{
    const char *s = q_skip_ws(*p);
    uint32_t n = 0;
    int in_s = 0, in_d = 0, in_b = 0, depth = 0;
    uint32_t end;

    *ok = 1;
    if (*s == '\0') { *p = s; return 0; }

    while (*s != '\0') {
        char c = *s;
        if (in_s) { if (c == '\'') in_s = 0; }
        else if (in_d) { if (c == '"') in_d = 0; }
        else if (in_b) { if (c == ']') in_b = 0; }
        else if (c == '\'') in_s = 1;
        else if (c == '"') in_d = 1;
        else if (c == '[') in_b = 1;
        else if (c == '(') depth++;
        else if (c == ')') { if (depth > 0) depth--; }
        else if (c == ',' && depth == 0) break;
        if (n >= cap - 1u) { *ok = 0; return 0; }
        dst[n++] = c;
        s++;
    }
    /* trim trailing blanks */
    end = n;
    while (end > 0u && q_isspace(dst[end - 1])) end--;
    dst[end] = '\0';
    /* advance past the comma if present */
    if (*s == ',') s++;
    *p = s;
    return 1;
}

/* ===================================================================== */
/* Clause extraction: pull FOR/WHILE/OFF and the leading scope off `args`  */
/* ===================================================================== */

/*
 * A LIST/DISPLAY/LOCATE argument string is:
 *   [<scope>] [<expr list>] [FOR <cond>] [WHILE <cond>] [OFF]
 * We locate the top-level keywords FOR, WHILE, OFF (whole words, outside string
 * literals), split the string at them, and recover the scope + expr list from the
 * head. The clause keywords never appear inside a field/expr name as a whole word
 * at the top level in the corpus idioms, so whole-word top-level matching is safe.
 */

typedef struct {
    /* scope */
    int      scope_kind;     /* QS_DEFAULT / QS_ALL / QS_REST / QS_NEXT / QS_RECORD */
    uint32_t scope_n;        /* NEXT n / RECORD n count */
    /* clauses (pointers into a working copy, NUL-terminated each) */
    char     exprlist[Q_MAX_EXPR + 1];
    int      has_exprlist;
    char     forcond[Q_MAX_EXPR + 1];
    int      has_for;
    char     whilecond[Q_MAX_EXPR + 1];
    int      has_while;
    int      off;            /* OFF -> suppress the recno column */
} query_clauses;

enum { QS_DEFAULT = 0, QS_ALL, QS_REST, QS_NEXT, QS_RECORD };

/* match a whole-word keyword at s (case-insensitive); return word length if it
 * is `kw` followed by end-of-string or a blank, else 0. */
static int q_match_kw(const char *s, const char *kw)
{
    int i = 0;
    while (kw[i] != '\0') {
        if (q_up1(s[i]) != q_up1(kw[i]))
            return 0;
        i++;
    }
    if (s[i] != '\0' && !q_isspace(s[i]))
        return 0;
    return i;
}

/* parse an unsigned integer at *s; advance *s past it; return the value. */
static uint32_t q_parse_uint(const char **s)
{
    uint32_t v = 0;
    const char *p = *s;
    while (*p >= '0' && *p <= '9') {
        v = v * 10u + (uint32_t)(*p - '0');
        p++;
    }
    *s = p;
    return v;
}

/*
 * q_split_clauses: parse `args` into scope + expr-list + FOR + WHILE + OFF.
 * Returns 0 on success, non-zero on overflow (fail loud at the caller).
 *
 * Strategy: walk the string token by token at the top level (outside string
 * literals), recording the byte offsets of the first top-level FOR / WHILE / OFF
 * keywords. The head (before the earliest clause keyword) holds the optional
 * scope then the optional expression list.
 */
static int q_split_clauses(const char *args, query_clauses *qc)
{
    const char *s = q_skip_ws(args);
    int in_s = 0, in_d = 0, in_b = 0;
    const char *for_at = (const char *)0;
    const char *while_at = (const char *)0;
    const char *off_at = (const char *)0;
    const char *p = s;
    char prev = ' ';

    qc->scope_kind   = QS_DEFAULT;
    qc->scope_n      = 0u;
    qc->exprlist[0]  = '\0';
    qc->has_exprlist = 0;
    qc->forcond[0]   = '\0';
    qc->has_for      = 0;
    qc->whilecond[0] = '\0';
    qc->has_while    = 0;
    qc->off          = 0;

    /* locate the first top-level FOR / WHILE / OFF keywords. */
    while (*p != '\0') {
        char c = *p;
        if (in_s) { if (c == '\'') in_s = 0; prev = c; p++; continue; }
        if (in_d) { if (c == '"')  in_d = 0; prev = c; p++; continue; }
        if (in_b) { if (c == ']')  in_b = 0; prev = c; p++; continue; }
        if (c == '\'') { in_s = 1; prev = c; p++; continue; }
        if (c == '"')  { in_d = 1; prev = c; p++; continue; }
        if (c == '[')  { in_b = 1; prev = c; p++; continue; }
        if (q_isspace(prev)) {
            if (!for_at && q_match_kw(p, "FOR"))     for_at = p;
            if (!while_at && q_match_kw(p, "WHILE")) while_at = p;
            if (!off_at && q_match_kw(p, "OFF"))     off_at = p;
        }
        prev = c;
        p++;
    }

    /* OFF present? */
    if (off_at)
        qc->off = 1;

    /* the head is everything before the earliest of FOR / WHILE / OFF. */
    {
        const char *head_end = (const char *)0;
        const char *cands[3];
        int i;
        cands[0] = for_at; cands[1] = while_at; cands[2] = off_at;
        for (i = 0; i < 3; i++) {
            if (cands[i] && (!head_end || cands[i] < head_end))
                head_end = cands[i];
        }
        if (!head_end) {
            /* no clause kw: head is the whole string. */
            head_end = s + q_strlen(s);
        }

        /* ---- parse the head: [<scope>] [<expr list>] ---- */
        {
            const char *h = q_skip_ws(s);
            int sk;
            /* scope keywords */
            if ((sk = q_match_kw(h, "ALL")) > 0) {
                qc->scope_kind = QS_ALL; h = q_skip_ws(h + sk);
            } else if ((sk = q_match_kw(h, "REST")) > 0) {
                qc->scope_kind = QS_REST; h = q_skip_ws(h + sk);
            } else if ((sk = q_match_kw(h, "NEXT")) > 0) {
                qc->scope_kind = QS_NEXT; h = q_skip_ws(h + sk);
                qc->scope_n = q_parse_uint(&h); h = q_skip_ws(h);
            } else if ((sk = q_match_kw(h, "RECORD")) > 0) {
                qc->scope_kind = QS_RECORD; h = q_skip_ws(h + sk);
                qc->scope_n = q_parse_uint(&h); h = q_skip_ws(h);
            }
            /* the remainder of the head (up to head_end) is the expr list. */
            {
                uint32_t n = 0;
                while (h < head_end && *h != '\0') {
                    if (n >= Q_MAX_EXPR) return -1;
                    qc->exprlist[n++] = *h++;
                }
                while (n > 0u && q_isspace(qc->exprlist[n - 1])) n--;
                qc->exprlist[n] = '\0';
                qc->has_exprlist = (n > 0u);
            }
        }
    }

    /* ---- FOR <cond>: from after "FOR" to the next of WHILE/OFF (or end). ---- */
    if (for_at) {
        const char *fs = q_skip_ws(for_at + 3);
        const char *fe = fs + q_strlen(fs);
        uint32_t n = 0;
        if (while_at && while_at > fs && while_at < fe) fe = while_at;
        if (off_at && off_at > fs && off_at < fe) fe = off_at;
        while (fs < fe && *fs != '\0') {
            if (n >= Q_MAX_EXPR) return -1;
            qc->forcond[n++] = *fs++;
        }
        while (n > 0u && q_isspace(qc->forcond[n - 1])) n--;
        qc->forcond[n] = '\0';
        qc->has_for = (n > 0u);
    }

    /* ---- WHILE <cond>: from after "WHILE" to OFF (or end). ---- */
    if (while_at) {
        const char *ws = q_skip_ws(while_at + 5);
        const char *we = ws + q_strlen(ws);
        uint32_t n = 0;
        if (off_at && off_at > ws && off_at < we) we = off_at;
        if (for_at && for_at > ws && for_at < we) we = for_at;
        while (ws < we && *ws != '\0') {
            if (n >= Q_MAX_EXPR) return -1;
            qc->whilecond[n++] = *ws++;
        }
        while (n > 0u && q_isspace(qc->whilecond[n - 1])) n--;
        qc->whilecond[n] = '\0';
        qc->has_while = (n > 0u);
    }

    return 0;
}

/* ===================================================================== */
/* Rendering one record's output line for LIST/DISPLAY                     */
/* ===================================================================== */

/*
 * q_render_record: render one record (the area's CURRENT record) to a line and
 * conout it. `qc` carries the expr list + OFF flag. With no expr list the default
 * is all fields. The recno column (right-justified in 8, like the dBASE
 * "Record#" column) is prefixed unless OFF.
 * Returns 0 or a negative interp_err (with *ec set) on an eval/structural fault.
 */
static int q_render_record(xb_interp *ip, int area, const query_clauses *qc,
                           int *ec)
{
    wa_env *env = xb_interp_env(ip);
    char line[Q_LINEBUF];
    uint32_t pos = 0u;
    int rc;

    if (!qc->off) {
        q_append_uint_rj(line, &pos, sizeof(line), wa_recno(env, area), 8);
        q_append_cstr(line, &pos, sizeof(line), " ");
    }

    if (qc->has_exprlist) {
        const char *p = qc->exprlist;
        char expr[Q_MAX_EXPR + 1];
        int ok = 1, first = 1;
        while (q_next_expr(&p, expr, sizeof(expr), &ok)) {
            xb_val v;
            if (!first)
                q_append_cstr(line, &pos, sizeof(line), " ");
            first = 0;
            rc = q_eval(ip, expr, &v, ec);
            if (rc != INTERP_OK)
                return rc;
            q_render_val(line, &pos, sizeof(line), &v);
        }
        if (!ok) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    } else {
        /* default: all fields, separated by a single space. */
        dbf_table *tbl = wa_table(env, area);
        int nf = tbl ? (int)dbf_nfields(tbl) : 0;
        int fi;
        for (fi = 0; fi < nf && fi < Q_MAX_FIELDS; fi++) {
            const dbf_field_t *f = dbf_field(tbl, fi);
            xb_val v;
            if (!f) continue;
            if (fi > 0)
                q_append_cstr(line, &pos, sizeof(line), " ");
            rc = q_eval(ip, f->name, &v, ec);
            if (rc != INTERP_OK)
                return rc;
            q_render_val(line, &pos, sizeof(line), &v);
        }
    }

    q_append_cstr(line, &pos, sizeof(line), "\n");
    q_putn(ip, line, pos);
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* The shared scoped walk (LIST/DISPLAY engine)                           */
/* ===================================================================== */

/*
 * q_walk: walk the scope applying FOR/WHILE, calling q_render_record on every
 * processed record. `is_display_default` selects DISPLAY's "current record only"
 * default vs LIST's "ALL" default when scope_kind == QS_DEFAULT.
 *
 * Semantics (navigation-query-display.md evaluation order):
 *   - QS_RECORD n: position at physical record n, process just it (FOR applies).
 *   - QS_ALL / LIST default: GO TOP, walk forward to EOF.
 *   - QS_REST: from the current record to EOF.
 *   - QS_NEXT n: n records from the current record.
 *   - DISPLAY default (QS_DEFAULT): the current record only (no GO TOP, no walk).
 *   - WHILE: stop at the first record where WHILE is .F.
 *   - FOR: render only records where FOR is .T., but keep walking.
 * Leaves the pointer at EOF for the multi-record scopes.
 */
static int q_walk(xb_interp *ip, int area, const query_clauses *qc,
                  int is_display_default, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    uint32_t nrec = wa_nrec(env, area);
    int kind = qc->scope_kind;
    int rc;

    if (kind == QS_DEFAULT)
        kind = is_display_default ? QS_RECORD /* current */ : QS_ALL;

    /* QS_RECORD with scope_n==0 means "current record" (the DISPLAY default). */

    if (nrec == 0u) {
        /* empty table: nothing to show; pointer stays at EOF. */
        if (ec) *ec = 0;
        return INTERP_OK;
    }

    /* ---- single-record scopes ---- */
    if (kind == QS_RECORD) {
        uint32_t target = (qc->scope_kind == QS_RECORD) ? qc->scope_n
                                                        : wa_recno(env, area);
        if (qc->scope_kind == QS_RECORD) {
            rc = wa_nav_goto(env, area, target);
            if (rc != NAV_OK) { if (ec) *ec = Q_MSG_NOT_INDEXED /*range*/; return -INTERP_ERR_SYNTAX; }
        }
        /* WHILE on a single record: if .F., skip; FOR: if .F., skip. */
        if (qc->has_while) {
            int t = 0;
            rc = q_eval_cond(ip, qc->whilecond, &t, ec);
            if (rc != INTERP_OK) return rc;
            if (!t) { if (ec) *ec = 0; return INTERP_OK; }
        }
        if (qc->has_for) {
            int t = 0;
            rc = q_eval_cond(ip, qc->forcond, &t, ec);
            if (rc != INTERP_OK) return rc;
            if (!t) { if (ec) *ec = 0; return INTERP_OK; }
        }
        return q_render_record(ip, area, qc, ec);
    }

    /* ---- multi-record scopes: establish the start record ---- */
    if (kind == QS_ALL) {
        rc = wa_nav_go_top(env, area);
        if (rc != NAV_OK) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    }
    /* QS_REST / QS_NEXT start at the current record (no GO TOP). */

    {
        uint32_t produced = 0u;
        uint32_t limit = (kind == QS_NEXT) ? qc->scope_n : 0u; /* 0 = unbounded */
        uint32_t visited = 0u;

        while (!wa_eof(env, area)) {
            /* NEXT n bounds the number of records VISITED (not matched). */
            if (kind == QS_NEXT && visited >= limit)
                break;
            visited++;

            if (qc->has_while) {
                int t = 0;
                rc = q_eval_cond(ip, qc->whilecond, &t, ec);
                if (rc != INTERP_OK) return rc;
                if (!t) break;                 /* WHILE false: STOP the walk */
            }
            if (qc->has_for) {
                int t = 0;
                rc = q_eval_cond(ip, qc->forcond, &t, ec);
                if (rc != INTERP_OK) return rc;
                if (t) {
                    rc = q_render_record(ip, area, qc, ec);
                    if (rc != INTERP_OK) return rc;
                    produced++;
                }
                /* FOR false: skip this record but KEEP walking. */
            } else {
                rc = q_render_record(ip, area, qc, ec);
                if (rc != INTERP_OK) return rc;
                produced++;
            }

            (void)wa_nav_skip(env, area, 1);   /* advance in the active order */
        }
        (void)produced;
    }

    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* ? / ??  (output the expression list)                                   */
/* ===================================================================== */

/*
 * q_question: implement ? / ?? . `leading_nl` is 1 for ? (a leading CR/LF) and
 * 0 for ?? . The expression list is comma-separated; values are separated on
 * output by a single space; ? ends with a newline, ?? does not.
 * A bare ? prints a blank line; a bare ?? prints nothing.
 */
static int q_question(xb_interp *ip, const char *args, int leading_nl, int *ec)
{
    const char *p = q_skip_ws(args);
    char line[Q_LINEBUF];
    uint32_t pos = 0u;
    char expr[Q_MAX_EXPR + 1];
    int ok = 1, first = 1, any = 0, rc;

    if (leading_nl)
        q_append_cstr(line, &pos, sizeof(line), "\n");

    while (q_next_expr(&p, expr, sizeof(expr), &ok)) {
        xb_val v;
        any = 1;
        if (!first)
            q_append_cstr(line, &pos, sizeof(line), " ");
        first = 0;
        rc = q_eval(ip, expr, &v, ec);
        if (rc != INTERP_OK)
            return rc;
        q_render_val(line, &pos, sizeof(line), &v);
    }
    if (!ok) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

    /* ? with no expression list still emits the leading newline (a blank line);
     * ?? with no expression list emits nothing. */
    (void)any;
    if (pos > 0u || leading_nl)
        q_putn(ip, line, pos);

    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* LOCATE / CONTINUE                                                      */
/* ===================================================================== */

/*
 * q_locate_scan: scan forward from the area's CURRENT record applying the FOR
 * predicate (and, if `apply_while`, the WHILE bound). Stops at the first FOR
 * match (sets *matched=1, leaves the pointer there) or at EOF / WHILE-false
 * (*matched=0, pointer at EOF or the stopping record). `start_here` = 1 tests the
 * current record first; 0 skips one before testing (for CONTINUE).
 */
static int q_locate_scan(xb_interp *ip, int area, const char *forcond, int has_for,
                         const char *whilecond, int apply_while,
                         int start_here, int *matched, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int rc;

    *matched = 0;

    if (!start_here) {
        (void)wa_nav_skip(env, area, 1);
    }

    while (!wa_eof(env, area)) {
        if (apply_while && whilecond) {
            int t = 0;
            rc = q_eval_cond(ip, whilecond, &t, ec);
            if (rc != INTERP_OK) return rc;
            if (!t) break;                     /* WHILE false: stop (no match) */
        }
        if (has_for) {
            int t = 0;
            rc = q_eval_cond(ip, forcond, &t, ec);
            if (rc != INTERP_OK) return rc;
            if (t) { *matched = 1; if (ec) *ec = 0; return INTERP_OK; }
        } else {
            /* LOCATE with no FOR: positions on the first record of the scope. */
            *matched = 1; if (ec) *ec = 0; return INTERP_OK;
        }
        (void)wa_nav_skip(env, area, 1);
    }

    if (ec) *ec = 0;
    return INTERP_OK;                          /* exhausted: no match, at EOF */
}

static int q_locate(xb_interp *ip, query_state *qs, const char *args, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    query_clauses qc;
    int matched = 0, rc, kind;

    if (q_split_clauses(args, &qc) != 0) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

    /* LOCATE default scope = ALL: implicit GO TOP unless a scope restricts it. */
    kind = qc.scope_kind;
    if (kind == QS_DEFAULT || kind == QS_ALL) {
        rc = wa_nav_go_top(env, area);
        if (rc != NAV_OK) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    } else if (kind == QS_RECORD) {
        rc = wa_nav_goto(env, area, qc.scope_n);
        if (rc != NAV_OK) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    }
    /* QS_REST / QS_NEXT scan from the current record. */

    rc = q_locate_scan(ip, area, qc.forcond, qc.has_for,
                       qc.whilecond, qc.has_while, /*start_here=*/1,
                       &matched, ec);
    if (rc != INTERP_OK)
        return rc;

    /* Remember the FOR for CONTINUE (FOR-only -- NOT the WHILE or scope). */
    qs->have_locate = 1;
    qs->locate_area = area;
    qs->has_for     = qc.has_for;
    {
        int i;
        for (i = 0; i < Q_MAX_FORLEN && qc.forcond[i] != '\0'; i++)
            qs->for_text[i] = qc.forcond[i];
        qs->for_text[(i < Q_MAX_FORLEN) ? i : Q_MAX_FORLEN] = '\0';
    }

    /* FOUND() / EOF() per the match. */
    wa_set_found(env, area, matched);
    if (!matched)
        wa_nav_set_eof(env, area, 1);          /* exhausted scope -> EOF */

    if (ec) *ec = 0;
    return INTERP_OK;
}

static int q_continue(xb_interp *ip, query_state *qs, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    int matched = 0, rc;

    if (!qs->have_locate || qs->locate_area != area) {
        /* CONTINUE with no prior LOCATE in this area: fail loud #42. */
        if (ec) *ec = Q_MSG_NO_LOCATE;
        return -INTERP_ERR_SYNTAX;
    }

#ifdef QUERY_MUTATE_CONTINUE_SCOPE
    /* MUTANT (Rule 6 -- -DQUERY_MUTATE_CONTINUE_SCOPE): CONTINUE wrongly RESTARTS
     * from GO TOP and re-applies a scope/WHILE-like bound instead of resuming
     * the FOR-only scan from the record AFTER the current one. This re-finds the
     * SAME first match the LOCATE found, so a LOCATE/CONTINUE loop that should
     * walk distinct recnos instead loops on the first one -> the oracle's
     * "CONTINUE finds the NEXT match" recno checks go RED. */
    (void)wa_nav_go_top(env, area);
    rc = q_locate_scan(ip, area, qs->for_text, qs->has_for,
                       (const char *)0, /*apply_while=*/0, /*start_here=*/1,
                       &matched, ec);
#else
    /* CONTINUE re-applies ONLY the stored FOR, from the record AFTER the current
     * one (start_here=0 -> skip one first). NOT the WHILE, NOT the scope. */
    rc = q_locate_scan(ip, area, qs->for_text, qs->has_for,
                       (const char *)0, /*apply_while=*/0, /*start_here=*/0,
                       &matched, ec);
#endif
    if (rc != INTERP_OK)
        return rc;

    wa_set_found(env, area, matched);
    if (!matched)
        wa_nav_set_eof(env, area, 1);

    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* SEEK / FIND                                                            */
/* ===================================================================== */

/*
 * q_seek_key: SEEK/FIND on the master index. `key` is the search value (XB_C for
 * char/FIND, XB_N/XB_D for numeric/date). Resolves the recno via ndx_seek on the
 * master index, positions the area, sets FOUND/EOF. No master index -> #26.
 */
static int q_seek_key(xb_interp *ip, const xb_val *key, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    int master = wa_master_order(env, area);
    ndx_index *ix;
    xb_ctx *ctx = xb_interp_ctx(ip);
    uint32_t recno = 0u;
    int found = 0, rc;

    if (!wa_is_open(env, area) || master <= 0) {
        if (ec) *ec = Q_MSG_NOT_INDEXED;       /* #26 Database is not indexed. */
        return -INTERP_ERR_SYNTAX;
    }
    ix = wa_index(env, area, master - 1);
    if (!ix) {
        if (ec) *ec = Q_MSG_NOT_INDEXED;
        return -INTERP_ERR_SYNTAX;
    }

    rc = ndx_seek(ix, key, ctx ? ctx->set_exact : 0, &recno, &found);
    if (rc != NDX_OK) {
        if (ec) *ec = 0;
        return -INTERP_ERR_EVAL;               /* structural index fault */
    }

    if (found && recno >= 1u) {
        rc = wa_nav_goto(env, area, recno);    /* position; clears EOF/BOF */
        if (rc != NAV_OK) {
            wa_set_found(env, area, 0);
            wa_nav_set_eof(env, area, 1);
            if (ec) *ec = 0;
            return INTERP_OK;
        }
        wa_set_found(env, area, 1);
    } else {
        /* no match: pointer at EOF, FOUND .F. (III+ has no SOFTSEEK). */
        wa_set_found(env, area, 0);
        wa_nav_set_eof(env, area, 1);
    }

    if (ec) *ec = 0;
    return INTERP_OK;
}

static int q_seek(xb_interp *ip, const char *args, int *ec)
{
    xb_val key;
    char expr[Q_MAX_EXPR + 1];
    const char *s = q_skip_ws(args);
    uint32_t n = 0;
    int rc;

    while (s[n] != '\0' && n < Q_MAX_EXPR) { expr[n] = s[n]; n++; }
    expr[n] = '\0';
    while (n > 0u && q_isspace(expr[n - 1])) { n--; expr[n] = '\0'; }
    if (n == 0u) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

    rc = q_eval(ip, expr, &key, ec);           /* SEEK takes an EXPRESSION */
    if (rc != INTERP_OK)
        return rc;
    return q_seek_key(ip, &key, ec);
}

static int q_find(xb_interp *ip, const char *args, int *ec)
{
    /* FIND takes a bare, UNQUOTED literal character string (no quotes). Leading
     * blanks already trimmed by the verb splitter; trailing blanks are trimmed.
     * navigation-query-display.md FIND: "does not have to be in quotes." */
    const char *s = args;
    uint32_t len = (uint32_t)q_strlen(s);
    xb_val key;
    while (len > 0u && q_isspace(s[len - 1])) len--;
    if (len == 0u) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    /* The literal points into the caller's args buffer (valid for the call). */
    key = xb_c(s, (uint16_t)len);
    return q_seek_key(ip, &key, ec);
}

/* ===================================================================== */
/* GO / GOTO / SKIP command verbs                                         */
/*                                                                        */
/* The S5.2 nav primitives (nav.c wa_nav_go_top/goto/skip) had no command  */
/* wiring; LIST/LOCATE need GO TOP internally and the idioms use GO/SKIP    */
/* directly, so the query/navigation module owns these verbs too           */
/* (navigation-query-display.md groups GO/GOTO/SKIP with the query set).    */
/* ===================================================================== */

/*
 * q_go: GO/GOTO TOP | BOTTOM | [RECORD] <expN>. `args` is the operand text.
 * GO does NOT touch FOUND() (navigation-query-display.md GO row).
 */
static int q_go(xb_interp *ip, const char *args, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    const char *s = q_skip_ws(args);
    int rc;

    if (q_match_kw(s, "TOP") > 0) {
        rc = wa_nav_go_top(env, area);
    } else if (q_match_kw(s, "BOTTOM") > 0) {
        rc = wa_nav_go_bottom(env, area);
    } else {
        uint32_t n;
        int k = q_match_kw(s, "RECORD");
        if (k > 0) s = q_skip_ws(s + k);   /* optional noise keyword */
        if (*s < '0' || *s > '9') { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
        n = q_parse_uint(&s);
        rc = wa_nav_goto(env, area, n);
    }
    if (rc != NAV_OK) {
        if (ec) *ec = 5;                   /* #5 "Record is out of range." */
        return -INTERP_ERR_SYNTAX;
    }
    if (ec) *ec = 0;
    return INTERP_OK;
}

/*
 * q_skip: SKIP [<expN>] (default +1). Negative retreats. SKIP does NOT touch
 * FOUND() (navigation-query-display.md SKIP row).
 */
static int q_skip(xb_interp *ip, const char *args, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    const char *s = q_skip_ws(args);
    int32_t n = 1;                         /* default forward one record */
    int neg = 0;

    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') { s++; }
    if (*s >= '0' && *s <= '9') {
        uint32_t v = q_parse_uint(&s);
        n = (int32_t)v;
        if (neg) n = -n;
    }
    (void)wa_nav_skip(env, area, n);
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* The command hook (the S5.4 module entry point)                         */
/* ===================================================================== */

/*
 * query_cmd_hook: an xb_cmd_hook (interp.h). Dispatches the QUERY/DISPLAY verbs;
 * returns CMD_UNKNOWN for anything else so the chain can try the next module.
 *
 *   user : unused (the query state is keyed off `ip` in the static registry).
 *   ip   : the interpreter.
 *   verb : upper-cased leading keyword ("LIST", "DISPLAY", "?", "??", "LOCATE",
 *          "CONTINUE", "SEEK", "FIND").
 *   args : the remainder of the line (operands), NUL-terminated.
 */
static int query_cmd_hook(void *user, xb_interp *ip,
                          const char *verb, const char *args, int *err_code)
{
    query_state *qs;
    (void)user;
    if (err_code) *err_code = 0;
    if (!ip || !verb)
        return CMD_UNKNOWN;

    qs = query_state_for(ip);
    if (!qs) { if (err_code) *err_code = 0; return -INTERP_ERR_NOMEM; }

    if (q_ci_eq(verb, "LIST") || q_ci_eq(verb, "DISPLAY")) {
        query_clauses qc;
        wa_env *env = xb_interp_env(ip);
        int area = wa_selected(env);
        int is_display = q_ci_eq(verb, "DISPLAY");
        int rc;
        if (q_split_clauses(args, &qc) != 0) { if (err_code) *err_code = 0; return -INTERP_ERR_SYNTAX; }
        rc = q_walk(ip, area, &qc, /*is_display_default=*/is_display, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (q_ci_eq(verb, "?")) {
        int rc = q_question(ip, args, /*leading_nl=*/1, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (q_ci_eq(verb, "??")) {
        int rc = q_question(ip, args, /*leading_nl=*/0, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (q_ci_eq(verb, "LOCATE")) {
        int rc = q_locate(ip, qs, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (q_ci_eq(verb, "CONTINUE")) {
        int rc = q_continue(ip, qs, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (q_ci_eq(verb, "SEEK")) {
        int rc = q_seek(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (q_ci_eq(verb, "FIND")) {
        int rc = q_find(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (q_ci_eq(verb, "GO") || q_ci_eq(verb, "GOTO")) {
        int rc = q_go(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (q_ci_eq(verb, "SKIP")) {
        int rc = q_skip(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    return CMD_UNKNOWN;                         /* not a query verb */
}

/* ===================================================================== */
/* Public registration                                                    */
/* ===================================================================== */

/*
 * query_register: install the query/display module into ip's command-hook chain.
 * The interpreter / REPL calls this once after xb_interp_make. Returns INTERP_OK
 * or a negative interp_err (chain full).
 *
 * Declared in interp.h-adjacent fashion? No -- it is declared here as the module
 * entry. The REPL (S5.8) and the oracle call query_register(ip). To keep the
 * symbol visible to callers without widening interp.h this step, it is exported
 * with external linkage and declared by the caller via the prototype below.
 */
int query_register(xb_interp *ip);

int query_register(xb_interp *ip)
{
    return xb_interp_add_cmd_hook(ip, query_cmd_hook, (void *)0);
}
