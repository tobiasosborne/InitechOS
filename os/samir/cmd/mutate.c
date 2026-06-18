/*
 * os/samir/cmd/mutate.c -- SAMIR (InitechBase) record-mutation command module.
 *                           Step S5.5 (Phase 5 interpreter).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib). Uses
 * ONLY <stdint.h> and the samir/ engine headers. No libc, no int 0x21 -- every
 * byte of OS contact goes through the PAL carried by the interpreter. Memory is
 * fixed/static (no malloc). ASCII-clean (Rule 12). Fail loud (Rule 2).
 * Reproducible (Rule 11): deterministic writes (dbf_flush rewrites the whole
 * image; no clock/host-path/RNG leak).
 *
 * dBASE III PLUS 1.1 ONLY (plan Sec 2.C).
 *
 * WHAT THIS IS (the S5.5 contract): a single xb_cmd_hook registered into the
 * executor's command-hook CHAIN (interp.h xb_interp_add_cmd_hook), exactly the
 * shape query.c (S5.4) uses. It adds the record-MUTATION verbs WITHOUT editing
 * flow.c:
 *
 *   REPLACE [<scope>] <f1> WITH <e1> [, <f2> WITH <e2> ...]
 *           [FOR <cond>] [WHILE <cond>]
 *   APPEND BLANK
 *   DELETE  [<scope>] [FOR <cond>] [WHILE <cond>]
 *   RECALL  [<scope>] [FOR <cond>] [WHILE <cond>]
 *   PACK
 *   ZAP
 *
 * STORE / "<name> = <expr>" (memvar assignment) are NOT here -- they are the
 * S5.3 spine (flow.c). REPLACE writes to FIELDS of the selected table's records;
 * it never touches memvars (plan S5.5: "REPLACE writes to FIELDS").
 *
 * SCOPE / FOR / WHILE (data-definition-and-manipulation.md sec 0 + 8/9):
 *   scope = RECORD <n> | NEXT <n> | REST | ALL.
 *   REPLACE / DELETE / RECALL DEFAULT scope = the CURRENT record only (no FOR/
 *     WHILE/scope) [verified: HELP.DBS:1023 + :555/:988]. With FOR (and no
 *     explicit scope) the scope becomes ALL (implicit GO TOP).
 *   FOR <cond>:   visit EVERY record in scope; process those where cond is .T.
 *                 (does NOT stop at the first false).
 *   WHILE <cond>: process records only WHILE cond is .T.; STOP at the first false.
 *   A <cond> MUST evaluate to Logical (XB_L) -- else fail loud #37 (no
 *   truthiness), the same headline rule as the spine + query module.
 *
 * REPLACE assignment-coercion (spec/samir/xbase_coercion.json assignment_coercion,
 * applied by dbf_replace -- self-contained in dbf.c, no eval.c dependency):
 *   C <- C : ok (truncate-to-width / space-pad).
 *   N <- N : ok; dec_format -> '*'-fill on width overflow (NOT an error; the
 *            dBASE III+ minted overflow behavior, "on_overflow: stars_fill").
 *   D <- D : ok.   L <- L : ok.   M <- C : ok (memo stores text).
 *   cross-type (C<-N, N<-C, D<-C, L<-N, ...) : fail loud #9 "Data type mismatch."
 *            -- there is NO auto-stringification (use STR()/VAL()/CTOD()/DTOC()).
 *
 * INDEX MAINTENANCE (master-key pointer drift):
 *   On the SINGLE-record (default-scope) REPLACE form, every open .ndx whose key
 *   expression references the table is kept current: the on-disk key bytes are
 *   computed from the index key expression BEFORE and AFTER the field write, and
 *   ndx_update_key(old,new,recno) re-files the entry so a later SEEK still
 *   resolves the record (data-definition-and-manipulation.md sec 8: "Open indexes
 *   whose key references a replaced field ARE updated on the single-record form").
 *   APPEND BLANK inserts the blank record's key into every open index
 *   (ndx_insert_key). The indexes are opened ndx_open_rw by the caller (S5.5
 *   adopts the table writable -- wa_adopt_table -- with ndx_open_rw'd indexes).
 *
 * WRITE MODEL (the writability seam): the dbf mutation verbs require a WRITABLE
 *   table (dbf.h S1.5: writable=1, set only by dbf_create). wa_set_open opens
 *   read-only, so a table the REPLACE/APPEND/DELETE verbs operate on is created
 *   writable and injected into the work area via wa_adopt_table (workarea.h S5.5).
 *   After each verb mutates the in-arena rec_region, this module dbf_flush()es
 *   (deterministic whole-file rewrite) so dbf_read_rec sees the new bytes, then
 *   wa_refresh()es the area's cache + nrec. A NON-writable table -> fail loud #41
 *   "Cannot write to a read-only file." (mapped from the codec's -DBF_ERR_IO).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md
 *     (REPLACE sec 8; APPEND sec 4; DELETE/RECALL sec 9; PACK sec 10; ZAP; the
 *     shared scope/FOR/WHILE grammar sec 0; the type-compatibility rule sec 0).
 *   - spec/samir/xbase_coercion.json assignment_coercion (the locked contract
 *     dbf_replace implements).
 *   - spec/samir/dbase_msg_codes.tsv (#9 mismatch, #37 not logical, #41 read-only).
 *   - os/samir/include/samir/interp.h (the xb_cmd_hook contract + chain).
 *   - os/samir/include/samir/workarea.h (wa_table/wa_recno/wa_nrec/wa_index/
 *     wa_index_count/wa_adopt_table/wa_refresh).
 *   - os/samir/include/samir/nav.h (wa_nav_go_top/goto/skip; wa_nav_reset).
 *   - os/samir/include/samir/dbf.h (dbf_replace/append_blank/delete/recall/pack/
 *     zap/flush; the assignment-coercion contract).
 *   - os/samir/include/samir/ndx.h (ndx_update_key/insert_key/key_type/key_length/
 *     key_expr -- index maintenance).
 *   - os/samir/include/samir/dbt.h (dbt_append -- memo writes for M fields).
 *   - os/samir/include/samir/eval.h (xb_lex/xb_parse/xb_eval; XBEE_MISMATCH/
 *     XBEE_NOT_LOGICAL; the WITH/FOR/WHILE expression path).
 */

#include <stdint.h>

#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/dbf.h"
#include "samir/dbt.h"
#include "samir/ndx.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/pal.h"

/* ===================================================================== */
/* Tunables (bounded -- Rule 2 fail-loud on overflow, never UB)            */
/* ===================================================================== */

#define M_MAX_TOKS     128   /* per-expression token pool */
#define M_MAX_NODES    128   /* per-expression node pool */
#define M_MAX_EXPR     254   /* one expression / condition working copy */
#define M_MAX_KEY      256   /* on-disk index key bytes (>= ndx_key_length) */

/* dBASE catalog ordinals used by this module (mirrors eval.h XBEE_* where they
 * exist; the read-only code is command-level, not in eval.h). */
#define M_MSG_READONLY     41   /* "Cannot write to a read-only file." */
/* XBEE_NOT_LOGICAL (#37) and XBEE_MISMATCH (#9) come from eval.h. */

/* Scope kinds (mirror query.c). */
enum { MS_DEFAULT = 0, MS_ALL, MS_REST, MS_NEXT, MS_RECORD };

/* ===================================================================== */
/* Small ASCII helpers (freestanding; no libc)                            */
/* ===================================================================== */

static char m_up1(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - 'a' + 'A');
    return c;
}

static int m_isspace(char c) { return c == ' ' || c == '\t'; }

static int m_strlen(const char *s)
{
    int n = 0;
    while (s[n] != '\0') n++;
    return n;
}

static int m_ci_eq(const char *a, const char *b)
{
    int i = 0;
    for (;;) {
        char ca = m_up1(a[i]);
        char cb = m_up1(b[i]);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
        i++;
    }
}

static const char *m_skip_ws(const char *s)
{
    while (*s != '\0' && m_isspace(*s)) s++;
    return s;
}

/* match a whole-word keyword at s (case-insensitive); return word length if it
 * is `kw` followed by end-of-string or a blank, else 0. */
static int m_match_kw(const char *s, const char *kw)
{
    int i = 0;
    while (kw[i] != '\0') {
        if (m_up1(s[i]) != m_up1(kw[i]))
            return 0;
        i++;
    }
    if (s[i] != '\0' && !m_isspace(s[i]))
        return 0;
    return i;
}

/* parse an unsigned integer at *s; advance *s past it; return the value. */
static uint32_t m_parse_uint(const char **s)
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

/* ===================================================================== */
/* Expression evaluation through the interp ctx (carries flow's resolver) */
/* ===================================================================== */

/*
 * m_eval: lex+parse+eval the NUL-terminated `expr` through ip's ctx. On success
 * returns INTERP_OK with *out set; else a negative interp_err with *ec the
 * catalog ordinal / stage detail. Same shape as query.c q_eval / flow.c eval_expr.
 */
static int m_eval(xb_interp *ip, const char *expr, xb_val *out, int *ec)
{
    xb_token toks[M_MAX_TOKS];
    xb_node  pool[M_MAX_NODES];
    xb_ctx  *ctx = xb_interp_ctx(ip);
    int ntok, root, rc, e = 0;
    uint32_t len = 0u;

    while (expr[len] != '\0') len++;

    ntok = xb_lex(expr, len, toks, (uint32_t)M_MAX_TOKS, &e);
    if (ntok < 0) { if (ec) *ec = e; return -INTERP_ERR_LEX; }

    root = xb_parse(toks, (uint32_t)ntok, pool, (uint32_t)M_MAX_NODES, &e);
    if (root < 0) { if (ec) *ec = e; return -INTERP_ERR_PARSE; }

    rc = xb_eval(pool, root, ctx, out, &e);
    if (rc != 0) { if (ec) *ec = e; return -INTERP_ERR_EVAL; }

    if (ec) *ec = 0;
    return INTERP_OK;
}

/*
 * m_eval_cond: evaluate `expr` and REQUIRE a Logical result (*truth gets 0/1).
 * A non-Logical result is fail-loud #37 (XBEE_NOT_LOGICAL) -- no truthiness.
 */
static int m_eval_cond(xb_interp *ip, const char *expr, int *truth, int *ec)
{
    xb_val v;
    int rc = m_eval(ip, expr, &v, ec);
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
/* On-disk index key encoding (for master-key pointer drift maintenance)  */
/* ===================================================================== */

/*
 * m_encode_key: evaluate the index key expression `key_expr` against the SELECTED
 * area's CURRENT record and render the result into `key_out` (key_len bytes) in
 * the .ndx on-disk encoding (ndx.md ss4.1 char / ss4.2 numeric/date):
 *   key_type 0 (char) : left-justified, space-padded to key_len; longer truncated.
 *   key_type 1 (N/D)  : 8-byte little-endian IEEE-754 double (the JDN for D).
 * Returns INTERP_OK or a negative interp_err (with *ec) on an eval/type fault.
 */
static int m_encode_key(xb_interp *ip, const char *key_expr,
                        uint16_t key_type, uint16_t key_len,
                        uint8_t *key_out, int *ec)
{
    xb_val v;
    int rc;
    uint32_t i;
    double d;

    rc = m_eval(ip, key_expr, &v, ec);
    if (rc != INTERP_OK)
        return rc;

    if (key_type == 0u) {
        /* char: left-justified, space-padded; longer truncated. */
        rt_memset(key_out, ' ', (uint32_t)key_len);
        if ((v.t == XB_C || v.t == XB_M) && v.u.c.p) {
            uint32_t n = (uint32_t)v.u.c.len;
            if (n > (uint32_t)key_len) n = (uint32_t)key_len;
            rt_memcpy(key_out, v.u.c.p, n);
        } else {
            if (ec) *ec = XBEE_MISMATCH;   /* char key expr must yield C */
            return -INTERP_ERR_EVAL;
        }
    } else {
        /* numeric/date: 8-byte LE double (JDN for D). */
        if (v.t == XB_N)      d = v.u.n;
        else if (v.t == XB_D) d = v.u.d;
        else {
            if (ec) *ec = XBEE_MISMATCH;
            return -INTERP_ERR_EVAL;
        }
        {
            uint8_t raw[8];
            rt_memcpy(raw, &d, 8u);        /* host LE; ndx.md ss4.2 raw LE double */
            for (i = 0u; i < 8u && i < (uint32_t)key_len; i++)
                key_out[i] = raw[i];
            for (; i < (uint32_t)key_len; i++)
                key_out[i] = 0u;
        }
    }
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* Clause extraction: pull FOR/WHILE and the leading scope off `args`      */
/* (mirrors query.c q_split_clauses, minus OFF/exprlist -- REPLACE keeps    */
/* its own field/WITH list in the head, see m_replace).                     */
/* ===================================================================== */

typedef struct {
    int      scope_kind;     /* MS_DEFAULT/MS_ALL/MS_REST/MS_NEXT/MS_RECORD */
    uint32_t scope_n;        /* NEXT n / RECORD n count */
    char     head[M_MAX_EXPR + 1];   /* text before FOR/WHILE (after the scope) */
    char     forcond[M_MAX_EXPR + 1];
    int      has_for;
    char     whilecond[M_MAX_EXPR + 1];
    int      has_while;
} m_clauses;

/*
 * m_split_clauses: parse `args` into scope + head + FOR + WHILE. The head is the
 * text after the optional leading scope and before the first top-level FOR/WHILE
 * keyword (for REPLACE it is the "<f> WITH <e>, ..." list; for DELETE/RECALL it
 * is empty). Returns 0 on success, non-zero on overflow (fail loud at caller).
 *
 * Top-level keyword scan ignores keywords inside string literals (', ", [).
 */
static int m_split_clauses(const char *args, m_clauses *mc)
{
    const char *s = m_skip_ws(args);
    int in_s = 0, in_d = 0, in_b = 0;
    const char *for_at = (const char *)0;
    const char *while_at = (const char *)0;
    const char *p = s;
    char prev = ' ';

    mc->scope_kind   = MS_DEFAULT;
    mc->scope_n      = 0u;
    mc->head[0]      = '\0';
    mc->forcond[0]   = '\0';
    mc->has_for      = 0;
    mc->whilecond[0] = '\0';
    mc->has_while    = 0;

    /* locate the first top-level FOR / WHILE keywords. */
    while (*p != '\0') {
        char c = *p;
        if (in_s) { if (c == '\'') in_s = 0; prev = c; p++; continue; }
        if (in_d) { if (c == '"')  in_d = 0; prev = c; p++; continue; }
        if (in_b) { if (c == ']')  in_b = 0; prev = c; p++; continue; }
        if (c == '\'') { in_s = 1; prev = c; p++; continue; }
        if (c == '"')  { in_d = 1; prev = c; p++; continue; }
        if (c == '[')  { in_b = 1; prev = c; p++; continue; }
        if (m_isspace(prev)) {
            if (!for_at && m_match_kw(p, "FOR"))     for_at = p;
            if (!while_at && m_match_kw(p, "WHILE")) while_at = p;
        }
        prev = c;
        p++;
    }

    /* head = everything before the earliest of FOR / WHILE. */
    {
        const char *head_end = (const char *)0;
        if (for_at)   head_end = for_at;
        if (while_at && (!head_end || while_at < head_end)) head_end = while_at;
        if (!head_end) head_end = s + m_strlen(s);

        /* parse the optional leading scope, then the remainder is the head. */
        {
            const char *h = m_skip_ws(s);
            int sk;
            if ((sk = m_match_kw(h, "ALL")) > 0) {
                mc->scope_kind = MS_ALL; h = m_skip_ws(h + sk);
            } else if ((sk = m_match_kw(h, "REST")) > 0) {
                mc->scope_kind = MS_REST; h = m_skip_ws(h + sk);
            } else if ((sk = m_match_kw(h, "NEXT")) > 0) {
                mc->scope_kind = MS_NEXT; h = m_skip_ws(h + sk);
                mc->scope_n = m_parse_uint(&h); h = m_skip_ws(h);
            } else if ((sk = m_match_kw(h, "RECORD")) > 0) {
                mc->scope_kind = MS_RECORD; h = m_skip_ws(h + sk);
                mc->scope_n = m_parse_uint(&h); h = m_skip_ws(h);
            }
            {
                uint32_t n = 0;
                while (h < head_end && *h != '\0') {
                    if (n >= M_MAX_EXPR) return -1;
                    mc->head[n++] = *h++;
                }
                while (n > 0u && m_isspace(mc->head[n - 1])) n--;
                mc->head[n] = '\0';
            }
        }
    }

    /* FOR <cond>: from after "FOR" to WHILE (or end). */
    if (for_at) {
        const char *fs = m_skip_ws(for_at + 3);
        const char *fe = fs + m_strlen(fs);
        uint32_t n = 0;
        if (while_at && while_at > fs && while_at < fe) fe = while_at;
        while (fs < fe && *fs != '\0') {
            if (n >= M_MAX_EXPR) return -1;
            mc->forcond[n++] = *fs++;
        }
        while (n > 0u && m_isspace(mc->forcond[n - 1])) n--;
        mc->forcond[n] = '\0';
        mc->has_for = (n > 0u);
    }

    /* WHILE <cond>: from after "WHILE" to FOR (or end). */
    if (while_at) {
        const char *ws = m_skip_ws(while_at + 5);
        const char *we = ws + m_strlen(ws);
        uint32_t n = 0;
        if (for_at && for_at > ws && for_at < we) we = for_at;
        while (ws < we && *ws != '\0') {
            if (n >= M_MAX_EXPR) return -1;
            mc->whilecond[n++] = *ws++;
        }
        while (n > 0u && m_isspace(mc->whilecond[n - 1])) n--;
        mc->whilecond[n] = '\0';
        mc->has_while = (n > 0u);
    }

    return 0;
}

/* ===================================================================== */
/* Field-name -> 0-based field index in the selected table                 */
/* ===================================================================== */

static int m_field_index(wa_env *env, int area, const char *name, uint32_t nlen)
{
    dbf_table *tbl = wa_table(env, area);
    int nf = tbl ? (int)dbf_nfields(tbl) : 0;
    int fi;
    for (fi = 0; fi < nf; fi++) {
        const dbf_field_t *f = dbf_field(tbl, fi);
        uint32_t i;
        if (!f) continue;
        for (i = 0; i < nlen; i++) {
            if (f->name[i] == '\0') break;
            if (m_up1(f->name[i]) != m_up1(name[i])) break;
        }
        if (i == nlen && f->name[i] == '\0')
            return fi;
    }
    return -1;
}

/* ===================================================================== */
/* Index maintenance: update every open index for the CURRENT record       */
/* ===================================================================== */

/*
 * m_index_after_replace: re-file the CURRENT record's entry in every open index,
 * given the SAVED pre-replace key bytes for each index in old_keys[]. Computes
 * the post-replace key by re-evaluating each index key expression (the record
 * cache was refreshed by the caller), then ndx_update_key(old,new,recno).
 *
 * old_keys: [nidx][M_MAX_KEY] -- the pre-replace key bytes captured by the caller
 *           BEFORE the field write (so the key reflects the OLD field value).
 * Returns INTERP_OK or a negative interp_err (with *ec) on a structural/eval fault.
 */
#ifndef MUTATE_REPLACE_NO_INDEX   /* unused when the no-index mutant drops the call */
static int m_index_after_replace(xb_interp *ip, int area, uint32_t recno,
                                 const uint8_t old_keys[][M_MAX_KEY], int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int n = wa_index_count(env, area);
    int i, rc;

    for (i = 0; i < n; i++) {
        ndx_index *ix = wa_index(env, area, i);
        uint8_t newk[M_MAX_KEY];
        uint16_t klen;
        if (!ix) continue;
        klen = ndx_key_length(ix);
        if (klen == 0u || (uint32_t)klen > (uint32_t)M_MAX_KEY) continue;
        rc = m_encode_key(ip, ndx_key_expr(ix), ndx_key_type(ix), klen, newk, ec);
        if (rc != INTERP_OK)
            return rc;
        rc = ndx_update_key(ix, old_keys[i], newk, recno);
        if (rc != NDX_OK) {
            if (ec) *ec = 0;
            return -INTERP_ERR_EVAL;       /* structural index fault: fail loud */
        }
    }
    if (ec) *ec = 0;
    return INTERP_OK;
}
#endif /* !MUTATE_REPLACE_NO_INDEX */

/* ===================================================================== */
/* REPLACE                                                                 */
/* ===================================================================== */

/*
 * m_apply_replace_one: apply the head's "<f1> WITH <e1>, <f2> WITH <e2> ..." list
 * to the CURRENT record (recno) of the selected area, evaluating each <e> through
 * ctx and dbf_replace'ing the field. Maintains every open index across the change
 * (master-key pointer drift): the per-index OLD key bytes are captured BEFORE the
 * writes, then ndx_update_key is called after.
 *
 * Returns INTERP_OK or a negative interp_err (with *ec). A cross-type assignment
 * is fail-loud #9 (mismatch, from dbf_replace's -DBF_ERR_MISMATCH).
 */
static int m_apply_replace_one(xb_interp *ip, int area, uint32_t recno,
                               const char *head, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    dbf_table *tbl = wa_table(env, area);
    const char *p = head;
    int rc;

    /* Capture each open index's OLD key bytes for THIS record before any write
     * (so ndx_update_key removes the correct stale entry). */
    int nidx = wa_index_count(env, area);
    static uint8_t old_keys[NDX_PER_AREA][M_MAX_KEY];
    int i;

    if (!tbl)
        return -INTERP_ERR_EVAL;

    for (i = 0; i < nidx; i++) {
        ndx_index *ix = wa_index(env, area, i);
        uint16_t klen;
        if (!ix) continue;
        klen = ndx_key_length(ix);
        if (klen == 0u || (uint32_t)klen > (uint32_t)M_MAX_KEY) continue;
        rc = m_encode_key(ip, ndx_key_expr(ix), ndx_key_type(ix), klen,
                          old_keys[i], ec);
        if (rc != INTERP_OK)
            return rc;
    }

    /* Walk the comma-separated "<field> WITH <expr>" pairs. */
    p = m_skip_ws(p);
    if (*p == '\0') { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

    while (*p != '\0') {
        const char *fstart;
        uint32_t fnlen = 0u;
        int field_idx;
        char exprbuf[M_MAX_EXPR + 1];
        uint32_t elen = 0u;
        xb_val v;
        int in_s = 0, in_d = 0, in_b = 0, depth = 0;
        const char *wpos;

        /* --- field name --- */
        p = m_skip_ws(p);
        fstart = p;
        while (*p != '\0' && !m_isspace(*p) && *p != ',') {
            fnlen++;
            p++;
        }
        if (fnlen == 0u) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

        /* --- "WITH" keyword --- */
        p = m_skip_ws(p);
        wpos = p;
        {
            int k = m_match_kw(wpos, "WITH");
            if (k <= 0) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
            p = m_skip_ws(wpos + k);
        }

        /* --- expression up to the next top-level comma --- */
        while (*p != '\0') {
            char c = *p;
            if (in_s) { if (c == '\'') in_s = 0; }
            else if (in_d) { if (c == '"') in_d = 0; }
            else if (in_b) { if (c == ']') in_b = 0; }
            else if (c == '\'') in_s = 1;
            else if (c == '"') in_d = 1;
            else if (c == '[') in_b = 1;
            else if (c == '(') depth++;
            else if (c == ')') { if (depth > 0) depth--; }
            else if (c == ',' && depth == 0) break;
            if (elen >= M_MAX_EXPR) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
            exprbuf[elen++] = c;
            p++;
        }
        while (elen > 0u && m_isspace(exprbuf[elen - 1])) elen--;
        exprbuf[elen] = '\0';
        if (elen == 0u) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

        /* resolve the field name. */
        field_idx = m_field_index(env, area, fstart, fnlen);
        if (field_idx < 0) {
            /* not a field of this table -> #9 mismatch (a bad target). */
            if (ec) *ec = XBEE_MISMATCH;
            return -INTERP_ERR_EVAL;
        }

        /* evaluate the WITH expression. */
        rc = m_eval(ip, exprbuf, &v, ec);
        if (rc != INTERP_OK)
            return rc;

        /* write the field (assignment-coercion happens in dbf_replace). */
        rc = dbf_replace(tbl, recno, field_idx, &v);
        if (rc != DBF_OK) {
            if (rc == -DBF_ERR_MISMATCH) { if (ec) *ec = XBEE_MISMATCH; }
            else if (rc == -DBF_ERR_IO)  { if (ec) *ec = M_MSG_READONLY; }
            else { if (ec) *ec = 0; }
            return -INTERP_ERR_EVAL;
        }

        /* advance past the comma. */
        p = m_skip_ws(p);
        if (*p == ',') { p++; continue; }
        break;
    }

    /* Persist + refresh so the new bytes are visible to resolve + the index key
     * re-evaluation reflects the NEW field value. */
    rc = dbf_flush(tbl);
    if (rc != DBF_OK) { if (ec) *ec = M_MSG_READONLY; return -INTERP_ERR_EVAL; }
    (void)wa_refresh(env, area, recno);

#ifdef MUTATE_REPLACE_NO_INDEX
    /* MUTANT (Rule 6 -- -DMUTATE_REPLACE_NO_INDEX): skip the open-index update
     * after a REPLACE. The index then still points at the OLD key, so a SEEK on
     * the NEW key after a key-field REPLACE no longer resolves the record ->
     * the oracle's master-key-pointer-drift checks go RED. */
    (void)old_keys;
#else
    /* Re-file every open index entry for this record (master-key pointer drift). */
    rc = m_index_after_replace(ip, area, recno,
                               (const uint8_t (*)[M_MAX_KEY])old_keys, ec);
    if (rc != INTERP_OK)
        return rc;
#endif

    if (ec) *ec = 0;
    return INTERP_OK;
}

static int m_replace(xb_interp *ip, const char *args, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    m_clauses mc;
    int kind, rc;

    if (!wa_is_open(env, area)) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    if (m_split_clauses(args, &mc) != 0) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    if (mc.head[0] == '\0') { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

    kind = mc.scope_kind;

#ifdef MUTATE_REPLACE_NO_SCOPE
    /* MUTANT (Rule 6 -- -DMUTATE_REPLACE_NO_SCOPE): ignore scope/FOR/WHILE and
     * REPLACE EVERY record unconditionally. A "REPLACE <f> WITH <e>" with no
     * scope (default = current record only) then clobbers all records, and a
     * "REPLACE NEXT 1 ..." touches more than one -> the oracle's "only the
     * targeted record(s) changed" checks go RED. */
    {
        uint32_t r;
        uint32_t nrec = wa_nrec(env, area);
        (void)kind;
        for (r = 1u; r <= nrec; r++) {
            (void)wa_nav_goto(env, area, r);
            rc = m_apply_replace_one(ip, area, r, mc.head, ec);
            if (rc != INTERP_OK) return rc;
        }
        if (ec) *ec = 0;
        return INTERP_OK;
    }
#else
    /* FOR with no explicit scope -> ALL (implicit GO TOP). */
    if (kind == MS_DEFAULT && (mc.has_for || mc.has_while))
        kind = MS_ALL;

    if (kind == MS_DEFAULT) {
        /* default: the CURRENT record only (data-definition sec 8). */
        uint32_t recno = wa_recno(env, area);
        if (wa_nrec(env, area) == 0u || wa_eof(env, area)) {
            if (ec) *ec = 0;
            return INTERP_OK;                    /* nothing to replace */
        }
        return m_apply_replace_one(ip, area, recno, mc.head, ec);
    }

    if (kind == MS_RECORD) {
        rc = wa_nav_goto(env, area, mc.scope_n);
        if (rc != NAV_OK) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
        return m_apply_replace_one(ip, area, mc.scope_n, mc.head, ec);
    }

    /* multi-record scopes: ALL (GO TOP) / REST / NEXT n. */
    if (kind == MS_ALL) {
        rc = wa_nav_go_top(env, area);
        if (rc != NAV_OK) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    }
    {
        uint32_t visited = 0u;
        uint32_t limit = (kind == MS_NEXT) ? mc.scope_n : 0u;  /* 0 = unbounded */

        while (!wa_eof(env, area)) {
            uint32_t recno = wa_recno(env, area);
            if (kind == MS_NEXT && visited >= limit)
                break;
            visited++;

            if (mc.has_while) {
                int t = 0;
                rc = m_eval_cond(ip, mc.whilecond, &t, ec);
                if (rc != INTERP_OK) return rc;
                if (!t) break;                 /* WHILE false: STOP */
            }
            if (mc.has_for) {
                int t = 0;
                rc = m_eval_cond(ip, mc.forcond, &t, ec);
                if (rc != INTERP_OK) return rc;
                if (t) {
                    rc = m_apply_replace_one(ip, area, recno, mc.head, ec);
                    if (rc != INTERP_OK) return rc;
                }
            } else {
                rc = m_apply_replace_one(ip, area, recno, mc.head, ec);
                if (rc != INTERP_OK) return rc;
            }
            (void)wa_nav_skip(env, area, 1);
        }
    }
    if (ec) *ec = 0;
    return INTERP_OK;
#endif
}

/* ===================================================================== */
/* APPEND BLANK                                                            */
/* ===================================================================== */

static int m_append(xb_interp *ip, const char *args, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    dbf_table *tbl = wa_table(env, area);
    const char *s = m_skip_ws(args);
    uint32_t newrec;
    int rc, i, nidx;

    if (!tbl) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

    /* III+ programmatic insert is APPEND BLANK; bare APPEND drops into the
     * full-screen editor (no terminal here) -- we accept BLANK only. */
    if (m_match_kw(s, "BLANK") <= 0) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

    rc = dbf_append_blank(tbl);
    if (rc != DBF_OK) {
        if (rc == -DBF_ERR_IO) { if (ec) *ec = M_MSG_READONLY; }
        else { if (ec) *ec = 0; }
        return -INTERP_ERR_EVAL;
    }
    rc = dbf_flush(tbl);
    if (rc != DBF_OK) { if (ec) *ec = M_MSG_READONLY; return -INTERP_ERR_EVAL; }

    /* APPEND BLANK moves the pointer to the new record (data-definition sec 4). */
    newrec = dbf_nrec(tbl);
    (void)wa_refresh(env, area, newrec);
    (void)wa_nav_reset(area);          /* the index seq is now stale */

    /* Insert the blank record's key into every open index. */
    nidx = wa_index_count(env, area);
    for (i = 0; i < nidx; i++) {
        ndx_index *ix = wa_index(env, area, i);
        uint8_t key[M_MAX_KEY];
        uint16_t klen;
        if (!ix) continue;
        klen = ndx_key_length(ix);
        if (klen == 0u || (uint32_t)klen > (uint32_t)M_MAX_KEY) continue;
        rc = m_encode_key(ip, ndx_key_expr(ix), ndx_key_type(ix), klen, key, ec);
        if (rc != INTERP_OK)
            return rc;
        rc = ndx_insert_key(ix, key, newrec);
        if (rc != NDX_OK) { if (ec) *ec = 0; return -INTERP_ERR_EVAL; }
    }

    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* DELETE / RECALL (scope/FOR/WHILE; default = current record)            */
/* ===================================================================== */

/*
 * m_delete_recall: shared engine for DELETE and RECALL. `want_delete`=1 sets the
 * delete flag (dbf_delete); 0 clears it (dbf_recall). Default scope = the current
 * record only; FOR (no explicit scope) -> ALL.
 */
static int m_delete_recall(xb_interp *ip, const char *args, int want_delete, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    dbf_table *tbl = wa_table(env, area);
    m_clauses mc;
    int kind, rc;

    if (!tbl || !wa_is_open(env, area)) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    if (m_split_clauses(args, &mc) != 0) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

    kind = mc.scope_kind;
    if (kind == MS_DEFAULT && (mc.has_for || mc.has_while))
        kind = MS_ALL;

    if (kind == MS_DEFAULT) {
        uint32_t recno = wa_recno(env, area);
        if (wa_nrec(env, area) == 0u || wa_eof(env, area)) {
            if (ec) *ec = 0;
            return INTERP_OK;
        }
        rc = want_delete ? dbf_delete(tbl, recno) : dbf_recall(tbl, recno);
        if (rc != DBF_OK) {
            if (rc == -DBF_ERR_IO) { if (ec) *ec = M_MSG_READONLY; }
            else { if (ec) *ec = 0; }
            return -INTERP_ERR_EVAL;
        }
        rc = dbf_flush(tbl);
        if (rc != DBF_OK) { if (ec) *ec = M_MSG_READONLY; return -INTERP_ERR_EVAL; }
        (void)wa_refresh(env, area, recno);
        if (ec) *ec = 0;
        return INTERP_OK;
    }

    if (kind == MS_RECORD) {
        rc = wa_nav_goto(env, area, mc.scope_n);
        if (rc != NAV_OK) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
        rc = want_delete ? dbf_delete(tbl, mc.scope_n) : dbf_recall(tbl, mc.scope_n);
        if (rc != DBF_OK) { if (ec) *ec = (rc == -DBF_ERR_IO) ? M_MSG_READONLY : 0; return -INTERP_ERR_EVAL; }
        rc = dbf_flush(tbl);
        if (rc != DBF_OK) { if (ec) *ec = M_MSG_READONLY; return -INTERP_ERR_EVAL; }
        (void)wa_refresh(env, area, mc.scope_n);
        if (ec) *ec = 0;
        return INTERP_OK;
    }

    /* multi-record scopes: ALL (GO TOP) / REST / NEXT n. */
    if (kind == MS_ALL) {
        rc = wa_nav_go_top(env, area);
        if (rc != NAV_OK) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    }
    {
        uint32_t visited = 0u;
        uint32_t limit = (kind == MS_NEXT) ? mc.scope_n : 0u;

        while (!wa_eof(env, area)) {
            uint32_t recno = wa_recno(env, area);
            if (kind == MS_NEXT && visited >= limit)
                break;
            visited++;

            if (mc.has_while) {
                int t = 0;
                rc = m_eval_cond(ip, mc.whilecond, &t, ec);
                if (rc != INTERP_OK) return rc;
                if (!t) break;
            }
            {
                int do_it = 1;
                if (mc.has_for) {
                    int t = 0;
                    rc = m_eval_cond(ip, mc.forcond, &t, ec);
                    if (rc != INTERP_OK) return rc;
                    do_it = t;
                }
                if (do_it) {
                    rc = want_delete ? dbf_delete(tbl, recno)
                                     : dbf_recall(tbl, recno);
                    if (rc != DBF_OK) { if (ec) *ec = (rc == -DBF_ERR_IO) ? M_MSG_READONLY : 0; return -INTERP_ERR_EVAL; }
                }
            }
            (void)wa_nav_skip(env, area, 1);
        }
        rc = dbf_flush(tbl);
        if (rc != DBF_OK) { if (ec) *ec = M_MSG_READONLY; return -INTERP_ERR_EVAL; }
        (void)wa_refresh(env, area, 0u);
    }
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* PACK / ZAP                                                              */
/* ===================================================================== */

static int m_pack(xb_interp *ip, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    dbf_table *tbl = wa_table(env, area);
    int rc;

    if (!tbl) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    rc = dbf_pack(tbl);
    if (rc != DBF_OK) {
        if (rc == -DBF_ERR_IO) { if (ec) *ec = M_MSG_READONLY; }
        else { if (ec) *ec = 0; }
        return -INTERP_ERR_EVAL;
    }
    rc = dbf_flush(tbl);
    if (rc != DBF_OK) { if (ec) *ec = M_MSG_READONLY; return -INTERP_ERR_EVAL; }
    /* PACK rewinds to record 1 (data-definition sec 10). */
    (void)wa_refresh(env, area, 1u);
    (void)wa_nav_reset(area);
    if (ec) *ec = 0;
    return INTERP_OK;
}

static int m_zap(xb_interp *ip, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    dbf_table *tbl = wa_table(env, area);
    int rc;

    if (!tbl) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    rc = dbf_zap(tbl);
    if (rc != DBF_OK) {
        if (rc == -DBF_ERR_IO) { if (ec) *ec = M_MSG_READONLY; }
        else { if (ec) *ec = 0; }
        return -INTERP_ERR_EVAL;
    }
    rc = dbf_flush(tbl);
    if (rc != DBF_OK) { if (ec) *ec = M_MSG_READONLY; return -INTERP_ERR_EVAL; }
    (void)wa_refresh(env, area, 0u);   /* empty -> EOF */
    (void)wa_nav_reset(area);
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* The command hook (the S5.5 module entry point)                         */
/* ===================================================================== */

/*
 * mutate_cmd_hook: an xb_cmd_hook (interp.h). Dispatches the mutation verbs;
 * returns CMD_UNKNOWN for anything else so the chain can try the next module.
 */
static int mutate_cmd_hook(void *user, xb_interp *ip,
                           const char *verb, const char *args, int *err_code)
{
    (void)user;
    if (err_code) *err_code = 0;
    if (!ip || !verb)
        return CMD_UNKNOWN;

    if (m_ci_eq(verb, "REPLACE")) {
        int rc = m_replace(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (m_ci_eq(verb, "APPEND")) {
        int rc = m_append(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (m_ci_eq(verb, "DELETE")) {
        int rc = m_delete_recall(ip, args, /*want_delete=*/1, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (m_ci_eq(verb, "RECALL")) {
        int rc = m_delete_recall(ip, args, /*want_delete=*/0, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (m_ci_eq(verb, "PACK")) {
        int rc = m_pack(ip, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (m_ci_eq(verb, "ZAP")) {
        int rc = m_zap(ip, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    return CMD_UNKNOWN;                         /* not a mutation verb */
}

/* ===================================================================== */
/* Public registration                                                    */
/* ===================================================================== */

/*
 * mutate_register: install the mutation module into ip's command-hook chain.
 * The interpreter / REPL / oracle calls this once after xb_interp_make. Returns
 * INTERP_OK or a negative interp_err (chain full). Mirrors query_register.
 */
int mutate_register(xb_interp *ip);

int mutate_register(xb_interp *ip)
{
    return xb_interp_add_cmd_hook(ip, mutate_cmd_hook, (void *)0);
}
