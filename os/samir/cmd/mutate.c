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
 *   PACK physically removes deleted records AND renumbers the survivors, so a
 *   per-recno ndx_delete_key cannot restore consistency (every surviving entry
 *   would still carry a STALE recno). m_pack therefore FULLY REINDEXES every
 *   open .ndx from the freshly-packed table (ndx_rebuild, one call per index,
 *   with a get_key callback that re-evaluates that index's OWN key expression
 *   against physical record N of the packed table via wa_nav_goto + m_encode_key)
 *   -- matching III+ "rebuilds/adjusts all open index files" on PACK
 *   (data-definition-and-manipulation.md sec.10 [ASSIST.HLP:120]). initech-x87g.
 *
 * WRITE MODEL (the writability seam): the dbf mutation verbs require a WRITABLE
 *   table (dbf.h S1.5: writable=1, set only by dbf_create). wa_set_open opens
 *   read-only, so a table the REPLACE/APPEND/DELETE verbs operate on is created
 *   writable and injected into the work area via wa_adopt_table (workarea.h S5.5).
 *   After each verb mutates the in-arena rec_region, this module dbf_flush()es
 *   (deterministic whole-file rewrite) so dbf_read_rec sees the new bytes, then
 *   wa_refresh()es the area's cache + nrec. A NON-writable table -> fail loud #111
 *   "Cannot write to a read-only file." (mapped from the codec's -DBF_ERR_IO).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md
 *     (REPLACE sec 8; APPEND sec 4; DELETE/RECALL sec 9; PACK sec 10; ZAP; the
 *     shared scope/FOR/WHILE grammar sec 0; the type-compatibility rule sec 0).
 *   - spec/samir/xbase_coercion.json assignment_coercion (the locked contract
 *     dbf_replace implements).
 *   - spec/samir/dbase_msg_codes.tsv (#9 mismatch, #37 not logical, #111 read-only).
 *   - os/samir/include/samir/interp.h (the xb_cmd_hook contract + chain).
 *   - os/samir/include/samir/workarea.h (wa_table/wa_recno/wa_nrec/wa_index/
 *     wa_index_count/wa_adopt_table/wa_refresh).
 *   - os/samir/include/samir/nav.h (wa_nav_go_top/goto/skip; wa_nav_reset).
 *   - os/samir/include/samir/dbf.h (dbf_replace/append_blank/delete/recall/pack/
 *     zap/flush; the assignment-coercion contract).
 *   - os/samir/include/samir/ndx.h (ndx_update_key/insert_key/ndx_rebuild/
 *     key_type/key_length/key_expr -- index maintenance).
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
#include "samir/set.h"
#include "samir/ovl.h"

/* ===================================================================== */
/* Tunables (bounded -- Rule 2 fail-loud on overflow, never UB)            */
/* ===================================================================== */

#define M_MAX_TOKS     128   /* per-expression token pool */
#define M_MAX_NODES    128   /* per-expression node pool */
#define M_MAX_EXPR     254   /* one expression / condition working copy */
#define M_MAX_KEY      256   /* on-disk index key bytes (>= ndx_key_length) */

/* dBASE catalog ordinals used by this module (mirrors eval.h XBEE_* where they
 * exist; the read-only code is command-level, not in eval.h). */
#define M_MSG_READONLY     111  /* "Cannot write to a read-only file." (catalog #111;
                                 * #41 is ".DBT file cannot be opened." -- fixed 7az.18) */
/* Catalog #56 and #29 from real DBASE.MSG; #1001 is an internal device
 * failure, authored, no local reference. Keep it outside catalog ordinals. */
static int m_write_error(int rc)
{
    if (rc == -DBF_ERR_NOSPC) return 56;
    if (rc == -DBF_ERR_ACCESS) return 29;
    if (rc == -DBF_ERR_WRITE) return 1001;
    return M_MSG_READONLY;
}

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

/* Recognize a grammar keyword, returning the typed word length. */
static int m_match_kw(const char *s, const char *kw)
{
    return rt_kw_match(s, kw);
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

    rc = xb_interp_eval(ip, key_expr, &v, ec);
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
        /* numeric/date: 8-byte LE double (JDN for D; the blank date keys as
         * 1E+100, sorting last -- real III+ BLANKDAT.NDX, datatype-mint;
         * audit 2026-10-23-samir L016, initech-ylqs). */
        if (v.t == XB_N)      d = v.u.n;
        else if (v.t == XB_D) d = xb_date_key(v.u.d);
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

#ifndef SAMIR_DML_MUTANT_abbrscope
/* Real III+ abbrscope mint: a trailing scope follows a COMPLETE WITH
 * expression, never a comma/field or an expression operand. Parse syntax
 * only (no evaluation or side effects), using the ordinary expression
 * grammar so quoted commas, function arguments and operators stay intact. */
static int m_after_replace_expr(const char *s, const char *end)
{
    /* This span can contain several individually bounded expressions. */
    xb_token toks[M_MAX_EXPR + 1];
    xb_node nodes[M_MAX_NODES];
    int nt, ec, i, pair = 0, depth = 0;

    nt = xb_lex(s, (uint32_t)(end - s), toks, M_MAX_EXPR + 1, &ec);
    if (nt < 0) return 0;
    for (i = 0; i < nt - 1; i++) {
        if (toks[i].type == XBT_LPAREN) depth++;
        else if (toks[i].type == XBT_RPAREN) depth--;
        else if (toks[i].type == XBT_COMMA && depth == 0) pair = i + 1;
    }
    if (nt - pair < 4 || toks[pair].type != XBT_IDENT ||
        toks[pair + 1].type != XBT_IDENT ||
        !m_match_kw(s + toks[pair + 1].offset, "WITH")) return 0;
    return xb_parse(toks + pair + 2, (uint32_t)(nt - pair - 2),
                    nodes, M_MAX_NODES, &ec) >= 0;
}
#endif

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
    char quote = 0;
    const char *for_at = (const char *)0;
    const char *while_at = (const char *)0;
    const char *scope_at = (const char *)0;
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
        if (rt_literal_byte(c, &quote)) { prev = c; p++; continue; }
        if (m_isspace(prev)) {
            /* Real abbrev/abbrfull mint: REPLACE ... RECORD 1 WHILE ... .
             * Scope is a clause after the replacement list as well as a
             * prefix; keep it out of the WITH expression in either spelling. */
            if (!scope_at && !for_at && !while_at &&
                rt_kw_word(p, "ALL\0REST\0NEXT\0RECORD\0", (int *)0) >= 0
#ifndef SAMIR_DML_MUTANT_abbrscope
                && (p == s || m_after_replace_expr(s, p))
#endif
               )
                scope_at = p;
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

        /* Consume a leading scope or bound the head at a trailing scope. */
        {
            const char *h = m_skip_ws(s);
            int sk;
            /* ALL REST NEXT RECORD in MS_ALL.. order; NEXT/RECORD take n */
            int k = scope_at ? rt_kw_word(scope_at, "ALL\0REST\0NEXT\0RECORD\0", &sk) : -1;
            if (k >= 0) {
                const char *end = m_skip_ws(scope_at + sk);
                mc->scope_kind = MS_ALL + k;
                if (mc->scope_kind >= MS_NEXT) {
                    mc->scope_n = m_parse_uint(&end); end = m_skip_ws(end);
                }
                if (scope_at == h) h = end;
                else if (scope_at < head_end) head_end = scope_at;
            }
            if (rt_copy_span(mc->head, M_MAX_EXPR, h, head_end) < 0)
                return -1;
        }
    }

    /* FOR <cond>: from after "FOR" to WHILE (or end). */
    if (for_at) {
        const char *fs = m_skip_ws(for_at + 3);
        const char *fe = fs + m_strlen(fs);
        int n;
        if (while_at && while_at > fs && while_at < fe) fe = while_at;
        n = rt_copy_span(mc->forcond, M_MAX_EXPR, fs, fe);
        if (n < 0) return -1;
        mc->has_for = (n > 0);
    }

    /* WHILE <cond>: from after "WHILE" to FOR (or end). */
    if (while_at) {
        const char *ws = m_skip_ws(while_at + m_match_kw(while_at, "WHILE"));
        const char *we = ws + m_strlen(ws);
        int n;
        if (for_at && for_at > ws && for_at < we) we = for_at;
        n = rt_copy_span(mc->whilecond, M_MAX_EXPR, ws, we);
        if (n < 0) return -1;
        mc->has_while = (n > 0);
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
        int elen;
        xb_val v;
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
        elen = rt_copy_expr(&p, exprbuf, M_MAX_EXPR);
        if (elen <= 0) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }

        /* resolve the field name. */
        field_idx = m_field_index(env, area, fstart, fnlen);
        if (field_idx < 0) {
            /* not a field of this table -> #9 mismatch (a bad target). */
            if (ec) *ec = XBEE_MISMATCH;
            return -INTERP_ERR_EVAL;
        }

        /* evaluate the WITH expression. */
        rc = xb_interp_eval(ip, exprbuf, &v, ec);
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
    if (rc != DBF_OK) { if (ec) *ec = m_write_error(rc); return -INTERP_ERR_EVAL; }
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

static int m_replace_impl(xb_interp *ip, const char *args, int *ec)
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
        if (rc != NAV_OK) { if (ec) *ec = wa_view_error(env); return -INTERP_ERR_EVAL; }
        return m_apply_replace_one(ip, area, mc.scope_n, mc.head, ec);
    }

    /* multi-record scopes: ALL (GO TOP) / REST / NEXT n. */
    if (kind == MS_ALL) {
        rc = wa_nav_go_top(env, area);
        if (rc != NAV_OK) { if (ec) *ec = wa_view_error(env); return -INTERP_ERR_EVAL; }
    }
    {
        uint32_t visited = 0u;
        uint32_t limit = (kind == MS_NEXT) ? mc.scope_n : 0u;  /* 0 = unbounded */

        while (!wa_eof(env, area)) {
            rc=wa_visible(env,area);
            if(rc<0) { if(ec) *ec=wa_view_error(env); return -INTERP_ERR_EVAL; }
            if(!rc) {
                rc=wa_nav_skip(env,area,1);
                if(rc) { if(ec) *ec=wa_view_error(env); return -INTERP_ERR_EVAL; }
                continue;
            }
            uint32_t recno = wa_recno(env, area);
            if (kind == MS_NEXT && visited >= limit)
                break;
            visited++;

            if (mc.has_while) {
                int t = 0;
                rc = xb_interp_cond(ip, mc.whilecond, &t, ec);
                if (rc != INTERP_OK) return rc;
                if (!t) break;                 /* WHILE false: STOP */
            }
            if (mc.has_for) {
                int t = 0;
                rc = xb_interp_cond(ip, mc.forcond, &t, ec);
                if (rc != INTERP_OK) return rc;
                if (t) {
                    rc = m_apply_replace_one(ip, area, recno, mc.head, ec);
                    if (rc != INTERP_OK) return rc;
                }
            } else {
                rc = m_apply_replace_one(ip, area, recno, mc.head, ec);
                if (rc != INTERP_OK) return rc;
            }
            rc=wa_nav_skip(env, area, 1);
            if(rc) { if(ec) *ec=wa_view_error(env); return -INTERP_ERR_EVAL; }
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
    if (m_match_kw(s, "BLANK") <= 0) { if (ec) *ec = *s ? 36 : 0; return -INTERP_ERR_SYNTAX; }

    rc = dbf_append_blank_commit(tbl);
    if (rc != DBF_OK) { if (ec) *ec = m_write_error(rc); return -INTERP_ERR_EVAL; }

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
static int m_delete_recall_impl(xb_interp *ip, const char *args, int want_delete, int *ec)
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
        if (rc != DBF_OK) { if (ec) *ec = m_write_error(rc); return -INTERP_ERR_EVAL; }
        (void)wa_refresh(env, area, recno);
        if (ec) *ec = 0;
        return INTERP_OK;
    }

    if (kind == MS_RECORD) {
        rc = wa_nav_goto(env, area, mc.scope_n);
        if (rc != NAV_OK) { if (ec) *ec = wa_view_error(env); return -INTERP_ERR_EVAL; }
        rc = want_delete ? dbf_delete(tbl, mc.scope_n) : dbf_recall(tbl, mc.scope_n);
        if (rc != DBF_OK) { if (ec) *ec = (rc == -DBF_ERR_IO) ? M_MSG_READONLY : 0; return -INTERP_ERR_EVAL; }
        rc = dbf_flush(tbl);
        if (rc != DBF_OK) { if (ec) *ec = m_write_error(rc); return -INTERP_ERR_EVAL; }
        (void)wa_refresh(env, area, mc.scope_n);
        if (ec) *ec = 0;
        return INTERP_OK;
    }

    /* multi-record scopes: ALL (GO TOP) / REST / NEXT n. */
    if (kind == MS_ALL) {
        rc = wa_nav_go_top(env, area);
        if (rc != NAV_OK) { if (ec) *ec = wa_view_error(env); return -INTERP_ERR_EVAL; }
    }
    {
        uint32_t visited = 0u;
        uint32_t limit = (kind == MS_NEXT) ? mc.scope_n : 0u;

        while (!wa_eof(env, area)) {
            rc=wa_visible(env,area);
            if(rc<0) { if(ec) *ec=wa_view_error(env); return -INTERP_ERR_EVAL; }
            if(!rc) {
                rc=wa_nav_skip(env,area,1);
                if(rc) { if(ec) *ec=wa_view_error(env); return -INTERP_ERR_EVAL; }
                continue;
            }
            uint32_t recno = wa_recno(env, area);
            if (kind == MS_NEXT && visited >= limit)
                break;
            visited++;

            if (mc.has_while) {
                int t = 0;
                rc = xb_interp_cond(ip, mc.whilecond, &t, ec);
                if (rc != INTERP_OK) return rc;
                if (!t) break;
            }
            {
                int do_it = 1;
                if (mc.has_for) {
                    int t = 0;
                    rc = xb_interp_cond(ip, mc.forcond, &t, ec);
                    if (rc != INTERP_OK) return rc;
                    do_it = t;
                }
                if (do_it) {
                    rc = want_delete ? dbf_delete(tbl, recno)
                                     : dbf_recall(tbl, recno);
                    if (rc != DBF_OK) { if (ec) *ec = (rc == -DBF_ERR_IO) ? M_MSG_READONLY : 0; return -INTERP_ERR_EVAL; }
                }
            }
            rc=wa_nav_skip(env, area, 1);
            if(rc) { if(ec) *ec=wa_view_error(env); return -INTERP_ERR_EVAL; }
        }
        rc = dbf_flush(tbl);
        if (rc != DBF_OK) { if (ec) *ec = m_write_error(rc); return -INTERP_ERR_EVAL; }
        (void)wa_refresh(env, area, 0u);
    }
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* PACK / ZAP                                                              */
/* ===================================================================== */

/*
 * m_pack_kp_ctx / m_pack_get_key: the ndx_key_provider (ndx.h) that
 * ndx_rebuild drives once per open index during PACK's full REINDEX
 * (initech-x87g).
 *
 * PACK renumbers survivors, so the ONLY correct per-index maintenance is a
 * fresh rebuild from the packed table (ndx_rebuild), not a per-recno
 * delete. ndx_rebuild calls get_key(user, recno, key_out, key_len) for
 * recno = 1..nrec of the ALREADY-packed table (fs/ndx.c never evaluates
 * expressions itself -- the DECOUPLING note in ndx.h -- so the caller, here,
 * owns evaluating THIS index's key_expr). The callback positions the work
 * area's cursor at the PHYSICAL record `recno` (wa_nav_goto, which is always
 * physical regardless of master order) so wa_resolve/m_eval read the right
 * record's fields, then renders the on-disk key bytes with m_encode_key --
 * the exact same encoder used by REPLACE/APPEND for index maintenance.
 *
 * On any failure (bad GOTO or an eval/type fault) `failed` is set and the
 * catalog code (if any) captured in `ec`, and the callback returns -1 to
 * abort ndx_build/ndx_rebuild (fail loud; Rule 2).
 */
#ifndef MUTATE_PACK_NO_REINDEX   /* unused when the no-reindex mutant drops the call */
typedef struct {
    xb_interp  *ip;
    wa_env     *env;
    int         area;
    const char *key_expr;
    uint16_t    key_type;
    int         failed;
    int         ec;
} m_pack_kp_ctx;

static SAMIR_OVL_TEXT(file) int m_pack_get_key(void *user, uint32_t recno,
                          uint8_t *key_out, uint16_t key_len)
{
    m_pack_kp_ctx *c = (m_pack_kp_ctx *)user;
    int rc;

    rc = wa_nav_goto(c->env, c->area, recno);
    if (rc != NAV_OK) { c->failed = 1; c->ec = 0; return -1; }

    rc = m_encode_key(c->ip, c->key_expr, c->key_type, key_len, key_out, &c->ec);
    if (rc != INTERP_OK) { c->failed = 1; return -1; }

    return 0;
}
#endif /* !MUTATE_PACK_NO_REINDEX */

static SAMIR_OVL_TEXT(file) int m_pack(xb_interp *ip, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    dbf_table *tbl = wa_table(env, area);
    uint32_t new_nrec;
    int rc;

    if (!tbl) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    rc = dbf_pack(tbl);
    if (rc != DBF_OK) {
        if (rc == -DBF_ERR_IO) { if (ec) *ec = M_MSG_READONLY; }
        else { if (ec) *ec = 0; }
        return -INTERP_ERR_EVAL;
    }
    rc = dbf_flush(tbl);
    if (rc != DBF_OK) { if (ec) *ec = m_write_error(rc); return -INTERP_ERR_EVAL; }
    new_nrec = dbf_nrec(tbl);

    /* Refresh the work area's nrec + record cache BEFORE reindexing: the
     * get_key callback below drives wa_nav_goto over the FULL packed range
     * 1..new_nrec, which wa_goto bounds-checks against the area's (still
     * stale, pre-refresh) nrec. */
    (void)wa_refresh(env, area, 1u);

#ifndef MUTATE_PACK_NO_REINDEX
    /* PACK renumbers survivors -- a per-deleted-recno ndx_delete_key is
     * INSUFFICIENT (every surviving entry would still carry its OLD recno).
     * FULLY REINDEX every open .ndx from the freshly-packed table, matching
     * III+ "rebuilds/adjusts all open index files" on PACK. Ref:
     * ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md
     * sec.10 [ASSIST.HLP:120]. initech-x87g. */
    {
        int nidx = wa_index_count(env, area);
        int i;

        for (i = 0; i < nidx; i++) {
            ndx_index *ix = wa_index(env, area, i);
            m_pack_kp_ctx kctx;

            if (!ix) continue;

            kctx.ip       = ip;
            kctx.env      = env;
            kctx.area     = area;
            kctx.key_expr = ndx_key_expr(ix);
            kctx.key_type = ndx_key_type(ix);
            kctx.failed   = 0;
            kctx.ec       = 0;

            rc = ndx_rebuild(ix, new_nrec, m_pack_get_key, &kctx);
            if (rc != NDX_OK) {
                if (ec) *ec = kctx.failed ? kctx.ec : 0;
                return -INTERP_ERR_EVAL;    /* structural index fault: fail loud */
            }
        }
    }
#else
    /* MUTANT (Rule 6 -- -DMUTATE_PACK_NO_REINDEX): skip the open-index full
     * REINDEX after PACK. Every open .ndx then keeps its PRE-pack recnos, so
     * a SEEK for a renumbered survivor resolves the WRONG (or a now
     * out-of-range/still-present deleted) record -> the oracle's post-PACK
     * SEEK checks go RED. */
    (void)new_nrec;
#endif

    /* PACK rewinds to record 1 (data-definition sec 10). The reindex loop
     * above (get_key) walked the cursor across every packed record; restore
     * the documented end position + drop the now-stale materialised nav
     * sequence. */
    (void)wa_refresh(env, area, 1u);
    (void)wa_nav_reset(area);
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* Ref: Ashton-Tate Using dBase III Plus.pdf U5-284 (ZAP prompt) and
 * U5-257 (SAFETY defaults ON). Opened-path filename spelling is authored,
 * no local reference for an exact transcript. Line-framed Y/y acceptance and treating Escape,
 * empty/error/EOF as No are authored, no local reference for those edge rules.
 * Use the same cooked PAL as the REPL so a redirected answer consumes one line.
 * This helper can guard further destructive file commands when implemented. */
static SAMIR_OVL_TEXT(file) int m_confirm_file(xb_interp *ip,const char *verb,const dbf_table *tbl)
{
    samir_pal_t *pal=xb_interp_pal(ip);
    char answer[256];const char *s;uint32_t len;
    if (!set_get_safety(ip)) return 1;
#ifdef SAMIR_MUTATE_ZAP_SAFETY
    return 1; /* mutant: stored SAFETY has no destructive consumer */
#endif
    pal->conout(pal,verb,rt_strlen(verb));pal->conout(pal," ",1u);
    pal->conout(pal,dbf_name(tbl),rt_strlen(dbf_name(tbl)));
    pal->conout(pal,"? (Y/N)",7u);
    if (!pal->conin_line || pal->conin_line(pal,answer,sizeof answer)<0) {
        pal->conout(pal,"\n",1u);return 0;
    }
    pal->conout(pal,"\n",1u);
    s=m_skip_ws(answer);len=rt_strlen(s);
    while(len && (s[len-1]==' ' || s[len-1]=='\t'))len--;
    return len==1u && (s[0]=='Y' || s[0]=='y');
}

static SAMIR_OVL_TEXT(file) int m_zap(xb_interp *ip, int *ec)
{
    wa_env *env = xb_interp_env(ip);
    int area = wa_selected(env);
    dbf_table *tbl = wa_table(env, area);
    int rc;

    if (!tbl) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    if (!m_confirm_file(ip,"ZAP",tbl)) { if(ec)*ec=0;return INTERP_OK; }
    rc = dbf_zap(tbl);
    if (rc != DBF_OK) {
        if (rc == -DBF_ERR_IO) { if (ec) *ec = M_MSG_READONLY; }
        else { if (ec) *ec = 0; }
        return -INTERP_ERR_EVAL;
    }
    rc = dbf_flush(tbl);
    if (rc != DBF_OK) { if (ec) *ec = m_write_error(rc); return -INTERP_ERR_EVAL; }
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

/* DML-04/05, Using III+ U5-131/132/172. Command adapters retain
 * NDX's independent codec and evaluate keys through the same resolver as SEEK. */
static SAMIR_OVL_TEXT(file) int d_fail(int *ec,int code) { if(ec)*ec=code;return -INTERP_ERR_EVAL; }
static SAMIR_OVL_TEXT(file) const char *d_kw(const char *s,const char *kw)
{
    char quote=0;
    for(const char *p=s;*p;p++) {
        if(quote) { if(*p==quote)quote=0;continue; }
        if(*p=='\'' || *p=='"' || *p=='[') {quote=*p=='['?']':*p;continue;}
        if((p==s || m_isspace(p[-1])) && m_match_kw(p,kw))return p;
    }
    return 0;
}
static SAMIR_OVL_TEXT(file) int d_word(const char **p,char *out,unsigned cap)
{
    unsigned n=0;const char *s=m_skip_ws(*p);char quote=0;
    if(*s=='\'' || *s=='"') quote=*s++;
    while(*s && (quote?*s!=quote:!m_isspace(*s))) {
        if(n+1>=cap) return 0;
        out[n++]=*s++;
    }
    if(quote) { if(*s!=quote)return 0;s++; }
    out[n]=0;*p=m_skip_ws(s);return n!=0;
}
static SAMIR_OVL_TEXT(file) int d_name(char *name,unsigned cap,const char *ext)
{
    unsigned i,n=rt_strlen(name);int dot=0;
    for(i=0;i<n;i++) { name[i]=m_up1(name[i]);if(name[i]=='.')dot=1; }
    if(!dot) { if(n+rt_strlen(ext)>=cap)return 0;rt_memcpy(name+n,ext,rt_strlen(ext)+1); }
    return 1;
}
/* INDEX ON defaults its extension, but explicit extension bytes belong to
 * the caller. GOLD.OUT pins only the default .ndx display; preserving an
 * explicit extension is the inferred name rule pending a separate III+ mint.
 * Keep the existing uppercase I/O default, with provenance on the handle. */
static int d_index_name(char *name, unsigned cap, int *defaulted)
{
    unsigned i, n = rt_strlen(name), dot = n;
    for (i = 0; i < n; i++) {
        if (name[i] == '/' || name[i] == '\\' || name[i] == ':') dot = n;
        else if (name[i] == '.' && dot == n) dot = i;
    }
    for (i = 0; i < dot; i++) name[i] = m_up1(name[i]);
    *defaulted = dot == n;
    if (*defaulted) {
        if (n + 5u > cap) return 0;
        rt_memcpy(name + n, ".NDX", 5u);
    }
    return 1;
}
/* Existing destinations stay intact under SAFETY until an independently
 * minted overwrite-prompt extension is available. No guessed prompt bytes. */
static SAMIR_OVL_TEXT(file) int d_destination(xb_interp *ip,const char *name,int *ec)
{
    samir_pal_t *p=xb_interp_pal(ip);pal_fd fd;
    wa_env *env=xb_interp_env(ip);
    for(int a=1;a<=WA_NAREAS;a++) {
        dbf_table *t=wa_table(env,a);
        if(t && m_ci_eq(dbf_name(t),name))return d_fail(ec,3);
    }
    fd=p->open(p,name,PAL_RD);
    if(fd>=0) { p->close(p,fd);if(set_get_safety(ip))return d_fail(ec,7); }
    else if(fd!=-PAL_ENOENT)return d_fail(ec,29);
    return 0;
}
typedef struct { xb_interp *ip;const char *expr;uint16_t type;int ec; } d_key_ctx;
static SAMIR_OVL_TEXT(file) int d_key(void *user,uint32_t rec,uint8_t *key,uint16_t len)
{
    d_key_ctx *c=user;wa_env *e=xb_interp_env(c->ip);
    if(wa_nav_goto(e,wa_selected(e),rec)!=NAV_OK)return -1;
    return m_encode_key(c->ip,c->expr,c->type,len,key,&c->ec);
}
static SAMIR_OVL_TEXT(file) int d_index(xb_interp *ip,const char *args,int rebuild,int *ec)
{
    wa_env *env=xb_interp_env(ip);samir_pal_t *pal=xb_interp_pal(ip);
    int area=wa_selected(env),rc;dbf_table *t=wa_table(env,area);
    uint32_t keep=wa_recno(env,area),count=t?dbf_nrec(t):0;
    d_key_ctx ctx={ip,0,0,0};
    if(!t)return d_fail(ec,52);
    if(rebuild) {
        if(*m_skip_ws(args))return d_fail(ec,10);
#ifdef SAMIR_DML_MUTANT_reindex
        if(count)count--; /* omit last record from every rebuilt index */
#endif
        for(int i=0;i<wa_index_count(env,area);i++) {
            ndx_index *ix=wa_index(env,area,i);ctx.expr=ndx_key_expr(ix);ctx.type=ndx_key_type(ix);
            rc=ndx_rebuild(ix,count,d_key,&ctx);
            if(rc) { wa_nav_goto(env,area,keep);return d_fail(ec,ctx.ec?ctx.ec:114); }
        }
    } else {
        const char *p=m_skip_ws(args),*to;char expr[255],name[128];xb_val v;ndx_index *ix;
        unsigned n;int defaulted, k=m_match_kw(p,"ON");
        if(!k)return d_fail(ec,10);
        p=m_skip_ws(p+k);to=d_kw(p,"TO");
        if(!to)return d_fail(ec,10);
        n=(unsigned)(to-p);while(n && m_isspace(p[n-1]))n--;
        if(!n || n>220)return d_fail(ec,10);
        rt_memcpy(expr,p,n);expr[n]=0;p=m_skip_ws(to+2);
        if(!d_word(&p,name,sizeof name) || *p || !d_index_name(name,sizeof name,&defaulted))return d_fail(ec,10);
        if(d_destination(ip,name,ec))return -INTERP_ERR_EVAL;
        rc=xb_interp_eval(ip,expr,&v,ec);if(rc)return rc;
        ctx.expr=expr;ctx.type=v.t==XB_C?0:1;
        if(v.t!=XB_C && v.t!=XB_N && v.t!=XB_D)return d_fail(ec,9);
#ifdef SAMIR_DML_MUTANT_index
        if(count)count--; /* last physical record has no index entry */
#endif
        rc=ndx_build(pal,name,ctx.type,v.t==XB_C?v.u.c.len:8,expr,count,d_key,&ctx);
        if(rc) { wa_nav_goto(env,area,keep);return d_fail(ec,ctx.ec?ctx.ec:29); }
        rc=ndx_open_rw(pal,name,&ix);
        if(rc)return d_fail(ec,114);
        ndx_set_name_defaulted(ix,defaulted);
        if(wa_attach_index(env,area,ix)!=WA_OK) {ndx_close(ix);return d_fail(ec,28);}
    }
    wa_nav_reset(area);
    rc=wa_nav_index_top(env,area);
    return rc==NAV_OK?INTERP_OK:d_fail(ec,114);
}

/* SORT/TOTAL: Using III+ U5-269/270/276; real DML-06 tie order and
 * DML-07 consecutive groups (non-numeric fields come from the first row). */
static SAMIR_OVL_TEXT(file) int d_schema(dbf_table *t,dbf_field_spec *s,int *ec)
{
    int n=dbf_nfields(t);
    if(n>128)return d_fail(ec,43);
    for(int i=0;i<n;i++) {
        const dbf_field_t *f=dbf_field(t,i);
        if(f->type=='M')return d_fail(ec,34);
        s[i].name=f->name;s[i].type=f->type;s[i].field_len=f->field_len;s[i].dec=f->dec_count;
    }
    return n;
}
static SAMIR_OVL_TEXT(file) int d_field(dbf_table *t,const char *name)
{ for(int i=0;i<dbf_nfields(t);i++)if(m_ci_eq(name,dbf_field(t,i)->name))return i;return -1; }
typedef struct {uint32_t rec;double number;char key[100];} d_sort_row;
static SAMIR_OVL_TEXT(file) int d_sort(xb_interp *ip,const char *args,int *ec)
{
    wa_env *env=xb_interp_env(ip);int area=wa_selected(env),rc=0,fi,nf,del;
    samir_pal_t *pal=xb_interp_pal(ip);dbf_table *src=wa_table(env,area),*out=0;
    dbf_field_spec schema[128];xb_val row[128];char name[128],field[32];
    const char *p=m_skip_ws(args);d_sort_row *rows;uint32_t n=0,count;
    if(!src)return d_fail(ec,52);
    if(!m_match_kw(p,"TO"))return d_fail(ec,10);
    p=m_skip_ws(p+2);
    if(!d_word(&p,name,sizeof name) || !m_match_kw(p,"ON"))return d_fail(ec,10);
    p=m_skip_ws(p+2);
    if(!d_word(&p,field,sizeof field) || *p || !d_name(name,sizeof name,".DBF"))return d_fail(ec,10);
    fi=d_field(src,field);if(fi<0)return d_fail(ec,48);
    if(dbf_field(src,fi)->type=='M' || dbf_field(src,fi)->type=='L')return d_fail(ec,9);
    nf=d_schema(src,schema,ec);if(nf<0)return nf;
    if(d_destination(ip,name,ec))return -INTERP_ERR_EVAL;
    count=dbf_nrec(src);
    if(count>0x1000000u/sizeof(*rows))return d_fail(ec,43);
    rows=pal->acquire(pal,(count?count:1)*sizeof(*rows));if(!rows)return d_fail(ec,43);
    wa_nav_index_top(env,area);
    while(!wa_eof(env,area)) {
        uint32_t r=wa_recno(env,area);
        if(dbf_read_rec(src,r,row,&del)) {rc=29;break;}
        rows[n].rec=r;
        if(row[fi].t==XB_C) {
            if(row[fi].u.c.len>100) {rc=23;break;}
            rt_memcpy(rows[n].key,row[fi].u.c.p,row[fi].u.c.len);
        } else rows[n].number=row[fi].t==XB_D?row[fi].u.d:row[fi].u.n;
        n++;
        if(wa_nav_skip(env,area,1)!=NAV_OK){rc=29;break;}
    }
    for(uint32_t i=1;!rc && i<n;i++) {
        d_sort_row v=rows[i];uint32_t j=i;
        while(j) {
            int cmp;
            if(dbf_field(src,fi)->type=='C')cmp=rt_memcmp(rows[j-1].key,v.key,dbf_field(src,fi)->field_len);
            else cmp=(rows[j-1].number>v.number)-(rows[j-1].number<v.number);
#ifdef SAMIR_DML_MUTANT_sort
            cmp=-cmp; /* descending instead of the documented ascending default */
#endif
            if(cmp<=0)break;
            rows[j]=rows[j-1];j--;
        }
        rows[j]=v;
    }
    if(!rc && dbf_create(pal,name,schema,nf,&out))rc=29;
    for(uint32_t i=0;!rc && i<n;i++) {
        if(dbf_read_rec(src,rows[i].rec,row,&del) || dbf_append_rec(out,row,0))rc=29;
    }
    if(!rc && dbf_flush(out))rc=56;
    if(out)dbf_close(out);
    pal->release(pal,rows);
    return rc?d_fail(ec,rc):INTERP_OK;
}
static SAMIR_OVL_TEXT(file) int d_total(xb_interp *ip,const char *args,int *ec)
{
    wa_env *env=xb_interp_env(ip);int area=wa_selected(env),nf,fi,del,rc=0,have=0;
    samir_pal_t *pal=xb_interp_pal(ip);dbf_table *src=wa_table(env,area),*out=0;
    dbf_field_spec schema[128];xb_val row[128],group[128],last=xb_u();
    char name[128],field[32],strings[4000],key[254];const char *p=m_skip_ws(args);
    if(!src)return d_fail(ec,52);
    if(!m_match_kw(p,"ON"))return d_fail(ec,10);
    p=m_skip_ws(p+2);
    if(!d_word(&p,field,sizeof field) || !m_match_kw(p,"TO"))return d_fail(ec,10);
    p=m_skip_ws(p+2);
    if(!d_word(&p,name,sizeof name) || *p || !d_name(name,sizeof name,".DBF"))return d_fail(ec,10);
    fi=d_field(src,field);if(fi<0)return d_fail(ec,48);
    nf=d_schema(src,schema,ec);if(nf<0)return nf;
    if(d_destination(ip,name,ec))return -INTERP_ERR_EVAL;
    if(dbf_create(pal,name,schema,nf,&out))return d_fail(ec,29);
    wa_nav_index_top(env,area);
    while(!wa_eof(env,area)) {
        if(dbf_read_rec(src,wa_recno(env,area),row,&del)) {rc=29;break;}
        if(!have || !xb_eq(&last,&row[fi])) {
            unsigned used=0;
            if(have && dbf_append_rec(out,group,0)) {rc=29;break;}
            for(int i=0;i<nf;i++) {
                group[i]=row[i];
                if(row[i].t==XB_C) {
                    if(used+row[i].u.c.len>sizeof strings) {rc=43;break;}
                    rt_memcpy(strings+used,row[i].u.c.p,row[i].u.c.len);
                    group[i].u.c.p=strings+used;used+=row[i].u.c.len;
                }
            }
            last=row[fi];
            if(last.t==XB_C) {rt_memcpy(key,last.u.c.p,last.u.c.len);last.u.c.p=key;}
            have=1;
        } else for(int i=0;i<nf;i++)if(row[i].t==XB_N) {
#ifndef SAMIR_DML_MUTANT_total
            group[i].u.n+=row[i].u.n;
#endif
        }
        if(rc || wa_nav_skip(env,area,1)!=NAV_OK){if(!rc)rc=29;break;}
    }
    if(!rc && have && dbf_append_rec(out,group,0))rc=29;
    if(!rc && dbf_flush(out))rc=56;
    dbf_close(out);
    return rc?d_fail(ec,rc):INTERP_OK;
}

/* Closed ASCII file verbs. Using III+ U5-59/121/175/278;
 * real DML-11..14: TYPE stops at DOS EOF, COPY FILE preserves every byte. */
/* kind: 0 TYPE, 1 RENAME, 2 ERASE, 3 COPY (mutate_cmd_hook's table order). */
static SAMIR_OVL_TEXT(file) int d_file(xb_interp *ip,int kind,const char *args,int *ec)
{
    const char *p=m_skip_ws(args);char src[128],dst[128],buf[512];int copy=kind==3;
    int type=kind==0,erase=kind==2,rc=0;
    samir_pal_t *pal=xb_interp_pal(ip);pal_fd in,out=-1;int32_t n;uint32_t total=0;
    if(copy) {if(!m_match_kw(p,"FILE"))return CMD_UNKNOWN;p=m_skip_ws(p+4);}
    if(!d_word(&p,src,sizeof src) || !d_name(src,sizeof src,""))return d_fail(ec,10);
    if(!erase && !type) {
        if(!m_match_kw(p,"TO"))return d_fail(ec,10);
        p=m_skip_ws(p+2);
        if(!d_word(&p,dst,sizeof dst) || !d_name(dst,sizeof dst,""))return d_fail(ec,10);
    }
    if(*p)return d_fail(ec,10);
    for(int a=1;a<=WA_NAREAS;a++) {
        dbf_table *t=wa_table(xb_interp_env(ip),a);
        if(t && m_ci_eq(src,dbf_name(t)))return d_fail(ec,3);
    }
    in=pal->open(pal,src,PAL_RD);if(in<0)return d_fail(ec,in==-PAL_ENOENT?1:29);
    if(erase) {
        pal->close(pal,in);
#ifdef SAMIR_DML_MUTANT_erase
        return INTERP_OK; /* leave the file present */
#endif
        rc=pal->remove(pal,src);return rc?d_fail(ec,29):INTERP_OK;
    }
    if(!copy && !type) {
        pal->close(pal,in);
        out=pal->open(pal,dst,PAL_RD);if(out>=0){pal->close(pal,out);return d_fail(ec,7);}
#ifdef SAMIR_DML_MUTANT_rename
        return INTERP_OK; /* skip the actual filesystem rename */
#endif
        rc=pal->rename(pal,src,dst);return rc?d_fail(ec,29):INTERP_OK;
    }
    if(copy) {
        if(d_destination(ip,dst,ec)) {pal->close(pal,in);return -INTERP_ERR_EVAL;}
        out=pal->open(pal,dst,PAL_RDWR|PAL_CREATE|PAL_TRUNC);
        if(out<0){pal->close(pal,in);return d_fail(ec,29);}
    } else xb_interp_output(ip,"\r\n",2);
    while((n=pal->read(pal,in,buf,sizeof buf))>0) {
        if(type) {
            int32_t j=0;while(j<n && (unsigned char)buf[j]!=26)j++;
#ifdef SAMIR_DML_MUTANT_type
            j=0; /* suppress ASCII contents */
#endif
            xb_interp_output(ip,buf,(uint32_t)j);
            if(j<n)break;
        } else {
#ifdef SAMIR_DML_MUTANT_copyfile
            buf[0]^=1; /* copied bytes differ even though state is preserved */
#endif
            if(pal->write(pal,out,buf,(uint32_t)n)!=n){rc=56;break;}
            total+=(uint32_t)n;
        }
    }
    if(n<0)rc=29;
    /* DML-11 real COPY FILE rounds the DOS block copy to 512 bytes,
     * zero-filling its final short block; the source is left byte-identical. */
    if(copy && !rc && total%512) {
        uint32_t pad=512-total%512;rt_memset(buf,0,pad);
        if(pal->write(pal,out,buf,pad)!=(int32_t)pad)rc=56;
    }

    if(out>=0 && pal->close(pal,out))rc=29;
    pal->close(pal,in);
    return rc?d_fail(ec,rc):INTERP_OK;
}

/* RUN / ! <system command> (initech-n8q8). HELP.DBS _RUN/! via dbase3-decomp
 * specs/commands/programming-and-io.md Sec 16: the rest of the line goes
 * LITERALLY to a second COMMAND.COM, which owns any `> file` redirection;
 * control then returns to dBASE. Real III+ DML-16 (mint/golden/run/GOLD.OUT):
 * RUN puts two CR LF pairs in the ALTERNATE transcript and RECNO()/RECCOUNT()
 * survive it (the table is never closed). The golden cannot show which side of
 * the child each pair falls on (the child's output went to its own file);
 * one before and one after is AUTHORED. Error 92 "Unable to load COMMAND.COM."
 * (spec/samir/dbase_msg_codes.tsv) when the PAL has no processor. */
static SAMIR_OVL_TEXT(file) int d_run(xb_interp *ip,const char *args,int *ec)
{
    samir_pal_t *pal=xb_interp_pal(ip);
#ifdef SAMIR_DML_MUTANT_run
    (void)pal;(void)args;(void)ec;
    return INTERP_OK; /* RUN_NOEXEC: no CR LFs, no child, no VERSION.TXT */
#else
    xb_interp_output(ip,"\r\n",2);
    if(!pal->exec || pal->exec(pal,args)<0)return d_fail(ec,92);
    xb_interp_output(ip,"\r\n",2);
    return INTERP_OK;
#endif
}

/* APPEND FROM DBF: Using III+ U5-28 (name matching, differing field order,
 * deleted-row rules and per-field truncation); DML-08 is the independent file.
 * Each record is persisted before proceeding. Previously appended records are
 * never rolled back if a later input/write fails; there is no invented batch
 * transaction. The blank-append primitive protects only its own record. */
static SAMIR_OVL_TEXT(file) int d_append_from(xb_interp *ip,const char *args,int *ec)
{
    wa_env *env=xb_interp_env(ip);int area=wa_selected(env),del,rc=0,nf,ns;
    dbf_table *out=wa_table(env,area),*src=0;const char *p=m_skip_ws(args);
    char name[128];int map[128];xb_val input[128];
    if(!out)return d_fail(ec,52);
    p=m_skip_ws(p+4);
    if(!d_word(&p,name,sizeof name) || *p || !d_name(name,sizeof name,".DBF"))return d_fail(ec,10);
    if(m_ci_eq(name,dbf_name(out)))return d_fail(ec,3);
    rc=dbf_open(xb_interp_pal(ip),name,&src);if(rc)return d_fail(ec,rc==-DBF_ERR_NOENT?1:15);
    nf=dbf_nfields(out);ns=dbf_nfields(src);
    if(nf>128 || ns>128){rc=43;goto done;}
    for(int i=0;i<nf;i++) {
        const dbf_field_t *f=dbf_field(out,i);map[i]=d_field(src,f->name);
        if(f->type=='M'){rc=34;goto done;}
        if(map[i]>=0 && dbf_field(src,map[i])->type!=f->type){rc=9;goto done;}
    }
    for(uint32_t r=1;r<=dbf_nrec(src);r++) {
        if(dbf_read_rec(src,r,input,&del)){rc=29;break;}
        if(del && wa_get_deleted(env))continue;
#ifdef SAMIR_DML_MUTANT_append
        if(r==dbf_nrec(src))break; /* drop the last incoming record */
#endif
        /* Commit one blank through the existing index-maintaining adapter,
         * then fill matching fields. A failing later field leaves this row. */
        if(m_append(ip,"BLANK",ec)){rc=*ec;break;}
        uint32_t rec=dbf_nrec(out);
        for(int i=0;i<nf;i++)if(map[i]>=0) {
            int code=dbf_replace(out,rec,i,&input[map[i]]);
            if(code){rc=code==-39?39:m_write_error(code);break;}
        }
        if(dbf_flush(out) && !rc)rc=56;
        wa_refresh(env,area,rec);
        /* Rebuild attached indexes after the complete row, using their own
         * codec and expression. This includes a partially filled failure row. */
        for(int i=0;i<wa_index_count(env,area);i++) {
            ndx_index *ix=wa_index(env,area,i);d_key_ctx c={ip,ndx_key_expr(ix),ndx_key_type(ix),0};
            if(ndx_rebuild(ix,dbf_nrec(out),d_key,&c) && !rc)rc=c.ec?c.ec:114;
        }
        wa_nav_reset(area);wa_nav_goto(env,area,rec);
        if(rc)break;
    }
 done:
    dbf_close(src);
    return rc?d_fail(ec,rc):INTERP_OK;
}

/* Overlay FILE (initech-5etom.1; docs/design/samir-overlays.md Sec 3): the
 * cold mutation verbs. `k` is mutate_cmd_hook's keyword index; each case
 * returns exactly what the hook returned for that verb before the split. */
static SAMIR_OVL_TEXT(file) int mutate_ovl_entry(int k, xb_interp *ip, const char *args, int *ec)
{
    int rc;
    switch (k) {
    case 0: case 1: case 2: case 3:             /* TYPE RENAME ERASE COPY FILE */
        return d_file(ip,k,args,ec);
    case 4: case 5:                             /* RUN ! */
        return d_run(ip,args,ec);
    case 6:  return d_sort(ip,args,ec);
    case 7:  return d_total(ip,args,ec);
    case 8: case 9:                             /* INDEX REINDEX */
        return d_index(ip,args,k==9,ec);
    case 11: rc = d_append_from(ip,args,ec); break;   /* APPEND FROM */
    case 14: rc = m_pack(ip, ec); break;
    case 15: rc = m_zap(ip, ec); break;
    default: return CMD_UNKNOWN;
    }
    return (rc == INTERP_OK) ? CMD_OK : rc;
}
SAMIR_OVL_HEADER(file, SAMIR_OVL_FILE, mutate_ovl_entry);

static int m_replace(xb_interp *ip, const char *args, int *ec)
{
    m_clauses mc;
    wa_env *env=xb_interp_env(ip);
    int old,rc;
    if(m_split_clauses(args,&mc)) { if(ec) *ec=10; return -INTERP_ERR_SYNTAX; }
    old=wa_include_deleted(env,mc.scope_kind==MS_RECORD || mc.scope_kind==MS_NEXT || (mc.scope_kind==MS_DEFAULT && !mc.has_for && !mc.has_while));
    rc=m_replace_impl(ip,args,ec);
    wa_include_deleted(env,old);
    return rc;
}

static int m_delete_recall(xb_interp *ip, const char *args, int want_delete, int *ec)
{
    m_clauses mc;
    wa_env *env=xb_interp_env(ip);
    int old,rc;
    if(m_split_clauses(args,&mc)) { if(ec) *ec=10; return -INTERP_ERR_SYNTAX; }
    old=wa_include_deleted(env,mc.scope_kind==MS_RECORD || mc.scope_kind==MS_NEXT || (mc.scope_kind==MS_DEFAULT && !mc.has_for && !mc.has_while));
    rc=m_delete_recall_impl(ip,args,want_delete,ec);
    wa_include_deleted(env,old);
    return rc;
}
static int mutate_cmd_hook(void *user, xb_interp *ip,
                           const char *verb, const char *args, int *err_code)
{
    int k, rc;
    (void)user;
    if (err_code) *err_code = 0;
    if (!ip || !verb)
        return CMD_UNKNOWN;

    k = rt_kw_index(verb, "TYPE\0RENAME\0ERASE\0COPY\0RUN\0!\0SORT\0TOTAL\0INDEX\0"
                          "REINDEX\0REPLACE\0APPEND\0DELETE\0RECALL\0PACK\0ZAP\0");
    switch (k) {
    case 3:                                     /* COPY: only COPY FILE is ours */
        if (!m_match_kw(m_skip_ws(args), "FILE")) return CMD_UNKNOWN;
        /* fall through */
    case 0: case 1: case 2:                     /* TYPE RENAME ERASE */
    case 4: case 5: case 6: case 7: case 8: case 9: /* RUN ! SORT TOTAL INDEX REINDEX */
    case 14: case 15:                           /* PACK ZAP */
        return SAMIR_OVL_CALL(SAMIR_OVL_FILE, mutate_ovl_entry, k, ip, args, err_code);
    case 10: rc = m_replace(ip, args, err_code); break;
    case 11:
        if (m_match_kw(m_skip_ws(args),"FROM"))  /* APPEND FROM is cold */
            return SAMIR_OVL_CALL(SAMIR_OVL_FILE, mutate_ovl_entry, k, ip, args, err_code);
        rc = m_append(ip, args, err_code);
        break;
    case 12: rc = m_delete_recall(ip, args, /*want_delete=*/1, err_code); break;
    case 13: rc = m_delete_recall(ip, args, /*want_delete=*/0, err_code); break;
    default: return CMD_UNKNOWN;                /* not a mutation verb */
    }
    return (rc == INTERP_OK) ? CMD_OK : rc;
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
