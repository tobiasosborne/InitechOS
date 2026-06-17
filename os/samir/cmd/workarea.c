/*
 * os/samir/cmd/workarea.c -- SAMIR (InitechBase) work-area model + interpreter
 *                            foundation. Step S5.1 (Phase 5 convergence point).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib). Uses
 * ONLY <stdint.h> and the samir/ engine headers. No libc, no int 0x21 -- every
 * byte of OS contact goes through the PAL vtable carried by the env. Memory is
 * bump-allocated from the PAL arena (no malloc).
 *
 * This single translation unit implements BOTH:
 *   - os/samir/include/samir/workarea.h  (the 10-area model + USE/CLOSE/SELECT +
 *     the wa_resolve glue), and
 *   - os/samir/include/samir/interp.h    (the xb_interp handle + xb_interp_eval_str).
 * They are co-located because the interpreter handle simply wraps the wa_env +
 * the eval ctx; keeping them in one unit avoids exporting the internal layouts.
 * samir_do (S5.3) and samir_repl (S5.8) are DECLARED in interp.h but NOT defined
 * here (deferred; the S5.1 oracle does not reference them).
 *
 * dBASE III PLUS 1.1 ONLY (plan Sec 2.C).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.1 contract + Sec 8.2.
 *   - os/samir/include/samir/{workarea.h,interp.h,dbf.h,dbt.h,ndx.h,eval.h,
 *     value.h,rt.h,pal.h} (every consumed contract).
 *   - HELP.DBS @SELECT (10 areas, A..J), @USE (ALIAS / INDEX list, first index
 *     = master).
 *
 * ASCII-clean (Rule 12). Reproducible (Rule 11). Fail loud (Rule 2).
 */

#include <stdint.h>

#include "samir/workarea.h"
#include "samir/interp.h"
#include "samir/dbf.h"
#include "samir/dbt.h"
#include "samir/ndx.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/pal.h"

/* ===================================================================== */
/* Internal layouts                                                       */
/* ===================================================================== */

/*
 * One work area: an open .dbf + its optional .dbt + up to NDX_PER_AREA .ndx +
 * the RECNO cursor + a stable decoded-record cache.
 *
 * rec_cache / rec_bytes are the lifetime fix described in workarea.h: the dbf
 * record buffer is overwritten per dbf_read_rec, so we copy the raw C/M bytes
 * into rec_bytes (a private record_length buffer) and re-point the cached C/M
 * xb_vals into it. cache_recno tracks which record is currently cached (0 =
 * none). memo_buf is per-resolve scratch for a memo field's text.
 */
struct work_area {
    int          open;                 /* 1 if a table is open here */
    dbf_table   *tbl;                  /* the .dbf codec handle */
    dbt_file    *memo;                 /* the .dbt handle, or NULL (no memo) */
    ndx_index   *idx[NDX_PER_AREA];    /* attached indexes */
    int          nidx;                 /* number of attached indexes */
    int          master;               /* master order: 0 = natural, 1..nidx */

    uint32_t     recno;                /* 1-based current record (0 if closed) */
    uint32_t     nrec;                 /* cached dbf_nrec */
    int          eof;                  /* end-of-file flag */
    int          bof;                  /* begin-of-file flag */

    int          nfields;              /* cached dbf_nfields */
    char         alias[WA_ALIAS_CAP];  /* NUL-terminated alias */

    /* stable decoded-record cache (see lifetime note above) */
    xb_val      *rec_cache;            /* [nfields] decoded values */
    char        *rec_bytes;            /* [record_length] private raw copy */
    uint32_t     rec_bytes_len;        /* record_length */
    uint32_t     cache_recno;          /* recno currently in the cache, 0 = none */
    int          cache_deleted;        /* delete flag of the cached record */

    char        *memo_buf;             /* per-area memo text scratch */
    uint32_t     memo_buf_cap;         /* capacity of memo_buf */
};

/* The whole interpreter environment: 10 areas + the selected area. */
struct wa_env {
    samir_pal_t *pal;
    work_area    area[WA_NAREAS];
    int          cur;                  /* 0-based index of the selected area */
};

/* The interpreter handle wraps the env + the eval ctx + the scratch arena. */
struct xb_interp {
    samir_pal_t *pal;
    void        *mark;                 /* arena mark taken at make (for free) */
    wa_env      *env;
    xb_ctx       ctx;
    char        *scratch;              /* eval scratch arena */
};

/* Sizes for the resolve memo scratch + the per-table caches. */
#define WA_MEMO_BUF_CAP   2048u   /* a III+ memo per resolve fits comfortably */

/* ===================================================================== */
/* Small helpers (freestanding; no libc)                                  */
/* ===================================================================== */

/* ASCII upper-case of one byte. */
static char up1(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - 'a' + 'A');
    return c;
}

/*
 * ci_eq_n: case-insensitive equality of `a` (NUL-terminated) against the byte
 * slice (b, blen). Both folded to upper-case. True iff same length and bytes.
 */
static int ci_eq_n(const char *a, const char *b, uint16_t blen)
{
    uint16_t i;
    for (i = 0; i < blen; i++) {
        if (a[i] == '\0')
            return 0;                 /* a shorter than the slice */
        if (up1(a[i]) != up1(b[i]))
            return 0;
    }
    return a[blen] == '\0';            /* a must end exactly at blen */
}

/* ci_eq: case-insensitive equality of two NUL-terminated strings. */
static int ci_eq(const char *a, const char *b)
{
    uint32_t i = 0;
    for (;;) {
        char ca = up1(a[i]);
        char cb = up1(b[i]);
        if (ca != cb)
            return 0;
        if (ca == '\0')
            return 1;
        i++;
    }
}

/*
 * derive_alias: fill `dst` (cap bytes, NUL-terminated) with the upper-cased base
 * name of a path: the substring after the last '/' or '\\', before the last '.'.
 * E.g. "/x/TRAVEL.DBF" -> "TRAVEL". Truncates to cap-1.
 */
static void derive_alias(char *dst, uint32_t cap, const char *path)
{
    uint32_t start = 0, end, i, n;
    /* find start = byte after last separator */
    for (i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/' || path[i] == '\\')
            start = i + 1;
    }
    end = i;                            /* i == length */
    /* find end = last '.' at or after start */
    for (n = start; path[n] != '\0'; n++) {
        if (path[n] == '.')
            end = n;
    }
    n = end - start;
    if (n > cap - 1)
        n = cap - 1;
    for (i = 0; i < n; i++)
        dst[i] = up1(path[start + i]);
    dst[n] = '\0';
}

/*
 * derive_dbt_path: build the sibling .dbt path from a .dbf path into `dst`
 * (cap bytes). The .dbf and .dbt share a base name and differ only in the final
 * extension byte ('F' -> 'T'), so we copy the whole path verbatim and flip only
 * that last extension character, PRESERVING ITS CASE: "TRAVEL.DBF" -> "TRAVEL.DBT"
 * and "travel.dbf" -> "travel.dbt". This matters on a case-sensitive host PAL
 * (Linux): a hard-coded lowercase ".dbt" would miss an uppercase ".DBT" golden.
 * If the path has no extension matching .db[fF], appends ".dbt".
 * Returns 0 on success, -1 if it would overflow `dst`.
 */
static int derive_dbt_path(char *dst, uint32_t cap, const char *dbf)
{
    uint32_t len = rt_strlen(dbf);
    uint32_t dot = len;               /* index of last '.', or len if none */
    uint32_t i;
    char last;

    for (i = 0; i < len; i++) {
        if (dbf[i] == '.')
            dot = i;
    }

    /* If the path ends in ".db[fF]" (a 4-char extension), flip 'F'/'f' -> 'T'/'t'
     * keeping the rest verbatim (preserves directory + base-name + ext case). */
    if (dot < len && (len - dot) == 4u &&
        (dbf[dot + 1] == 'd' || dbf[dot + 1] == 'D') &&
        (dbf[dot + 2] == 'b' || dbf[dot + 2] == 'B') &&
        (dbf[dot + 3] == 'f' || dbf[dot + 3] == 'F')) {
        if (len + 1u > cap)
            return -1;
        rt_memcpy(dst, dbf, len);
        last = dbf[len - 1];
        dst[len - 1] = (last == 'F') ? 'T' : 't';   /* preserve case */
        dst[len] = '\0';
        return 0;
    }

    /* Fallback: replace any trailing extension (or append) with ".dbt". */
    if (dot + 4u + 1u > cap)
        return -1;
    rt_memcpy(dst, dbf, dot);
    dst[dot + 0] = '.';
    dst[dot + 1] = 'd';
    dst[dot + 2] = 'b';
    dst[dot + 3] = 't';
    dst[dot + 4] = '\0';
    return 0;
}

/* Letter A..J -> 1..10, else 0 (not an area letter). Single-char only. */
static int alias_letter_area(const char *s)
{
    char c;
    if (s[0] == '\0' || s[1] != '\0')
        return 0;
    c = up1(s[0]);
    if (c >= 'A' && c <= 'J')
        return (c - 'A') + 1;
    return 0;
}

/* ===================================================================== */
/* Record cache                                                           */
/* ===================================================================== */

/*
 * wa_touch_record: ensure area `wa`'s decoded record for its current RECNO is in
 * rec_cache, with C/M byte slices re-pointed into the stable rec_bytes copy.
 *
 * On an EMPTY table (nrec==0 / eof) leaves cache_recno=0; the resolver yields
 * blank values in that case (handled in wa_resolve). Returns WA_OK or a negative
 * codec/wa error.
 */
static int wa_touch_record(work_area *wa)
{
    int rc, fi;
    int deleted = 0;

    if (!wa->open)
        return -WA_ERR_EMPTY;
    if (wa->nrec == 0u || wa->eof)
        return WA_OK;                  /* nothing to cache; blank-at-EOF */
    if (wa->cache_recno == wa->recno)
        return WA_OK;                  /* already cached */

    rc = dbf_read_rec(wa->tbl, wa->recno, wa->rec_cache, &deleted);
    if (rc != DBF_OK)
        return rc;                     /* propagate the negative codec error */
    wa->cache_deleted = deleted;

    /*
     * Copy raw C/M bytes into the stable buffer and re-point the cached vals.
     * The dbf record buffer is overwritten on the next read; rec_bytes is ours.
     * We pack the C/M byte runs contiguously into rec_bytes in field order.
     */
    {
        uint32_t off = 0;
        for (fi = 0; fi < wa->nfields; fi++) {
            xb_val *v = &wa->rec_cache[fi];
            if (v->t == XB_C || v->t == XB_M) {
                uint16_t l = v->u.c.len;
                if (off + l > wa->rec_bytes_len)
                    return -WA_ERR_NOMEM;   /* should not happen: bytes sized to reclen */
                if (l > 0 && v->u.c.p != (char *)0)
                    rt_memcpy(wa->rec_bytes + off, v->u.c.p, l);
                v->u.c.p = wa->rec_bytes + off;
                off += l;
            }
        }
    }
    wa->cache_recno = wa->recno;
    return WA_OK;
}

/* ===================================================================== */
/* Environment lifecycle                                                  */
/* ===================================================================== */

wa_env *wa_env_make(samir_pal_t *pal)
{
    wa_env *env;
    int a;

    if (pal == (samir_pal_t *)0)
        return (wa_env *)0;

    env = (wa_env *)pal->alloc(pal, (uint32_t)sizeof(*env));
    if (env == (wa_env *)0)
        return (wa_env *)0;

    env->pal = pal;
    env->cur = 0;                      /* default selected area = 1 (0-based 0) */
    for (a = 0; a < WA_NAREAS; a++) {
        work_area *wa = &env->area[a];
        int k;
        wa->open = 0;
        wa->tbl = (dbf_table *)0;
        wa->memo = (dbt_file *)0;
        for (k = 0; k < NDX_PER_AREA; k++)
            wa->idx[k] = (ndx_index *)0;
        wa->nidx = 0;
        wa->master = 0;
        wa->recno = 0u;
        wa->nrec = 0u;
        wa->eof = 0;
        wa->bof = 0;
        wa->nfields = 0;
        wa->alias[0] = '\0';
        wa->rec_cache = (xb_val *)0;
        wa->rec_bytes = (char *)0;
        wa->rec_bytes_len = 0u;
        wa->cache_recno = 0u;
        wa->cache_deleted = 0;
        wa->memo_buf = (char *)0;
        wa->memo_buf_cap = 0u;
    }
    return env;
}

samir_pal_t *wa_env_pal(const wa_env *env)
{
    return env ? env->pal : (samir_pal_t *)0;
}

/* ===================================================================== */
/* USE / CLOSE / SELECT                                                   */
/* ===================================================================== */

/* close_handles: close the codec handles of an open area, in reverse order. */
static void close_handles(work_area *wa)
{
    int k;
    for (k = wa->nidx - 1; k >= 0; k--) {
        if (wa->idx[k]) {
            (void)ndx_close(wa->idx[k]);
            wa->idx[k] = (ndx_index *)0;
        }
    }
    wa->nidx = 0;
    if (wa->memo) {
        (void)dbt_close(wa->memo);
        wa->memo = (dbt_file *)0;
    }
    if (wa->tbl) {
        (void)dbf_close(wa->tbl);
        wa->tbl = (dbf_table *)0;
    }
}

int wa_set_open(wa_env *env, int area, const char *name,
                const char *alias, const wa_index_list *idx)
{
    work_area *wa;
    samir_pal_t *pal;
    int rc, i;
    char dbtpath[256];
    uint16_t reclen;

    if (!env || area < 1 || area > WA_NAREAS)
        return -WA_ERR_BAD_AREA;
    if (idx && idx->count > NDX_PER_AREA)
        return -WA_ERR_TOO_MANY;
    if (idx && idx->count < 0)
        return -WA_ERR_TOO_MANY;

    wa = &env->area[area - 1];
    if (wa->open)
        return -WA_ERR_OCCUPIED;       /* caller must CLOSE first (fail loud) */

    pal = env->pal;

    /* --- open the .dbf --- */
    rc = dbf_open(pal, name, &wa->tbl);
    if (rc != DBF_OK) {
        wa->tbl = (dbf_table *)0;
        return rc;                     /* propagate the codec error (negative) */
    }
    wa->nrec    = dbf_nrec(wa->tbl);
    wa->nfields = (int)dbf_nfields(wa->tbl);
    reclen      = dbf_record_length(wa->tbl);

    /* --- allocate the stable record cache from the arena --- */
    wa->rec_cache = (xb_val *)pal->alloc(pal,
        (uint32_t)sizeof(xb_val) * (uint32_t)(wa->nfields > 0 ? wa->nfields : 1));
    wa->rec_bytes = (char *)pal->alloc(pal, reclen > 0 ? (uint32_t)reclen : 1u);
    wa->memo_buf  = (char *)pal->alloc(pal, WA_MEMO_BUF_CAP);
    if (!wa->rec_cache || !wa->rec_bytes || !wa->memo_buf) {
        close_handles(wa);
        wa->tbl = (dbf_table *)0;
        return -WA_ERR_NOMEM;
    }
    wa->rec_bytes_len = (uint32_t)reclen;
    wa->memo_buf_cap  = WA_MEMO_BUF_CAP;
    wa->cache_recno   = 0u;

    /* --- open the sibling .dbt iff the table has a memo --- */
    if (dbf_has_memo(wa->tbl)) {
        if (derive_dbt_path(dbtpath, sizeof(dbtpath), name) != 0) {
            close_handles(wa);
            return -WA_ERR_IO;
        }
        rc = dbt_open(pal, dbtpath, /*is_iv_dialect=*/0, &wa->memo);
        if (rc != DBT_OK) {
            /* A memo .dbf with a missing/unreadable .dbt: fail loud (Rule 2). */
            wa->memo = (dbt_file *)0;
            close_handles(wa);
            return rc;                 /* propagate the dbt error */
        }
    }

    /* --- open each named index (the first becomes master order 1) --- */
    wa->nidx = 0;
    if (idx && idx->count > 0) {
        for (i = 0; i < idx->count; i++) {
            ndx_index *ix = (ndx_index *)0;
            rc = ndx_open(pal, idx->names[i], &ix);
            if (rc != NDX_OK) {
                close_handles(wa);
                return rc;             /* propagate the ndx error */
            }
            wa->idx[i] = ix;
            wa->nidx = i + 1;
        }
        wa->master = 1;                /* first index in the USE list = master */
    } else {
        wa->master = 0;                /* natural order */
    }

    /* --- alias --- */
    if (alias && alias[0] != '\0') {
        uint32_t k = 0;
        while (alias[k] != '\0' && k < (uint32_t)(WA_ALIAS_CAP - 1)) {
            wa->alias[k] = up1(alias[k]);
            k++;
        }
        wa->alias[k] = '\0';
    } else {
        derive_alias(wa->alias, WA_ALIAS_CAP, name);
    }

    /* --- cursor: RECNO=1 (the S5.1 headline contract) --- */
    wa->recno = 1u;                    /* dBASE positions at record 1 on USE */
    wa->bof   = 0;
    wa->eof   = (wa->nrec == 0u) ? 1 : 0;  /* empty table => EOF */
    wa->open  = 1;

    return WA_OK;
}

int wa_close(wa_env *env, int area)
{
    work_area *wa;
    if (!env || area < 1 || area > WA_NAREAS)
        return -WA_ERR_BAD_AREA;
    wa = &env->area[area - 1];
    if (!wa->open)
        return WA_OK;                  /* CLOSE on a closed area is harmless */

    close_handles(wa);
    wa->open = 0;
    wa->nidx = 0;
    wa->master = 0;
    wa->recno = 0u;
    wa->nrec = 0u;
    wa->eof = 0;
    wa->bof = 0;
    wa->nfields = 0;
    wa->alias[0] = '\0';
    wa->cache_recno = 0u;
    wa->rec_cache = (xb_val *)0;
    wa->rec_bytes = (char *)0;
    wa->rec_bytes_len = 0u;
    wa->memo_buf = (char *)0;
    wa->memo_buf_cap = 0u;
    return WA_OK;
}

int wa_close_all(wa_env *env)
{
    int a;
    if (!env)
        return -WA_ERR_BAD_AREA;
    for (a = 1; a <= WA_NAREAS; a++)
        (void)wa_close(env, a);
    return WA_OK;
}

int wa_select(wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS)
        return -WA_ERR_BAD_AREA;
#ifdef WA_MUTATE_SELECT
    /* MUTANT: store the 1-based number, not (area-1). SELECT n selects area n+1;
     * the resolve hook then reads the WRONG area. Rule 6 / ARB rider (a). */
    env->cur = area;
#else
    env->cur = area - 1;
#endif
    return WA_OK;
}

int wa_select_alias(wa_env *env, const char *alias)
{
    int a, letter;
    if (!env || !alias)
        return -WA_ERR_BAD_AREA;

    /* 1) match an explicit alias on an open area. */
    for (a = 0; a < WA_NAREAS; a++) {
        if (env->area[a].open && ci_eq(env->area[a].alias, alias)) {
            env->cur = a;
            return WA_OK;
        }
    }
    /* 2) match the A..J letter form against an OPEN area. */
    letter = alias_letter_area(alias);
    if (letter >= 1 && letter <= WA_NAREAS && env->area[letter - 1].open) {
        env->cur = letter - 1;
        return WA_OK;
    }
    return -WA_ERR_NO_ALIAS;
}

int wa_set_order(wa_env *env, int area, int order)
{
    work_area *wa;
    if (!env || area < 1 || area > WA_NAREAS)
        return -WA_ERR_BAD_AREA;
    wa = &env->area[area - 1];
    if (!wa->open)
        return -WA_ERR_EMPTY;
    if (order < 0 || order > wa->nidx)
        return -WA_ERR_BAD_ORDER;
    wa->master = order;
    return WA_OK;
}

int wa_goto(wa_env *env, int area, uint32_t recno)
{
    work_area *wa;
    int rc;
    if (!env || area < 1 || area > WA_NAREAS)
        return -WA_ERR_BAD_AREA;
    wa = &env->area[area - 1];
    if (!wa->open)
        return -WA_ERR_EMPTY;
    if (recno < 1u || recno > wa->nrec)
        return -DBF_ERR_BAD_RECNO;     /* fail loud; cursor unchanged */
    wa->recno = recno;
    wa->eof = 0;
    wa->bof = 0;
    wa->cache_recno = 0u;              /* invalidate cache; next resolve re-reads */
    rc = wa_touch_record(wa);
    return rc;
}

/* ===================================================================== */
/* The RESOLVE GLUE                                                        */
/* ===================================================================== */

int wa_resolve(void *user, const char *name, uint16_t len, xb_val *out)
{
    wa_env *env = (wa_env *)user;
    work_area *wa;
    int fi, found = -1;
    int rc;

    if (!env || !out)
        return 1;                      /* unbound (no env / no out) */

#ifdef WA_MUTATE_RESOLVE_AREA
    /* MUTANT: ignore the selected area; always read area 0 (1). Rule 6. */
    wa = &env->area[0];
#else
    wa = &env->area[env->cur];
#endif

    if (!wa->open)
        return 1;                      /* unbound: no table in selected area */

    /* match the field name (case-insensitive) against the descriptors. */
    for (fi = 0; fi < wa->nfields; fi++) {
        const dbf_field_t *f = dbf_field(wa->tbl, fi);
        if (f && ci_eq_n(f->name, name, len)) {
            found = fi;
            break;
        }
    }
    if (found < 0)
        return 1;                      /* unbound: not a field of this table */

    /* blank-at-EOF: an empty table (or EOF) yields the field's blank value. */
    if (wa->nrec == 0u || wa->eof) {
        const dbf_field_t *f = dbf_field(wa->tbl, found);
        char t = f ? f->type : 'C';
        switch (t) {
        case 'N': *out = xb_n(0.0);                break;
        case 'L': *out = xb_l(0);                  break;
        case 'D': *out = xb_u();                   break;  /* blank date */
        case 'M': *out = xb_m(wa->memo_buf, 0u);   break;  /* empty memo */
        case 'C':
        default:  *out = xb_c(wa->rec_bytes, 0u);  break;  /* empty string */
        }
        return 0;
    }

    /* ensure the current record is in the stable cache. */
    rc = wa_touch_record(wa);
    if (rc != WA_OK)
        return 1;                      /* I/O / decode failure -> unbound */

    /* a memo field resolves THROUGH the .dbt. */
    {
        const dbf_field_t *f = dbf_field(wa->tbl, found);
        if (f && f->type == 'M') {
            xb_val *raw = &wa->rec_cache[found];   /* XB_M raw 10-byte ptr, or XB_U */
            uint32_t block;
            uint8_t *mbuf = (uint8_t *)0;
            uint32_t mlen = 0u;

            if (raw->t != XB_M || raw->u.c.len == 0u || !wa->memo) {
                *out = xb_m(wa->memo_buf, 0u);     /* no memo for this record */
                return 0;
            }
            block = (uint32_t)dec_parse(raw->u.c.p, (int)raw->u.c.len);
            if (block == 0u) {
                *out = xb_m(wa->memo_buf, 0u);     /* block 0 = no memo */
                return 0;
            }
            rc = dbt_read(wa->memo, block, &mbuf, &mlen);
            if (rc != DBT_OK) {
                *out = xb_m(wa->memo_buf, 0u);     /* unreadable memo -> empty */
                return 0;
            }
            /* copy the memo bytes into the area's stable memo scratch. */
            if (mlen > wa->memo_buf_cap)
                mlen = wa->memo_buf_cap;
            if (mlen > 0u)
                rt_memcpy(wa->memo_buf, mbuf, mlen);
            *out = xb_m(wa->memo_buf, (uint16_t)mlen);
            return 0;
        }
    }

    /* plain field: return the cached xb_val (C/N/D/L). */
    *out = wa->rec_cache[found];
    return 0;
}

void wa_bind_ctx(wa_env *env, xb_ctx *ctx)
{
    if (!ctx)
        return;
    ctx->resolve = wa_resolve;
    ctx->user    = env;
}

/* ===================================================================== */
/* Accessors                                                              */
/* ===================================================================== */

int wa_selected(const wa_env *env)
{
    if (!env)
        return 0;
    return env->cur + 1;               /* 0-based -> 1-based */
}

int wa_is_open(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS)
        return 0;
    return env->area[area - 1].open;
}

uint32_t wa_recno(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return 0u;
    return env->area[area - 1].recno;
}

uint32_t wa_nrec(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return 0u;
    return env->area[area - 1].nrec;
}

int wa_eof(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return 0;
    return env->area[area - 1].eof;
}

int wa_bof(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return 0;
    return env->area[area - 1].bof;
}

void wa_nav_set_eof(wa_env *env, int area, int val)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return;
    env->area[area - 1].eof = val ? 1 : 0;
}

void wa_nav_set_bof(wa_env *env, int area, int val)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return;
    env->area[area - 1].bof = val ? 1 : 0;
}

const char *wa_alias(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return "";
    return env->area[area - 1].alias;
}

int wa_master_order(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return 0;
    return env->area[area - 1].master;
}

int wa_index_count(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return 0;
    return env->area[area - 1].nidx;
}

dbf_table *wa_table(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return (dbf_table *)0;
    return env->area[area - 1].tbl;
}

dbt_file *wa_memo(const wa_env *env, int area)
{
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return (dbt_file *)0;
    return env->area[area - 1].memo;
}

ndx_index *wa_index(const wa_env *env, int area, int i)
{
    const work_area *wa;
    if (!env || area < 1 || area > WA_NAREAS || !env->area[area - 1].open)
        return (ndx_index *)0;
    wa = &env->area[area - 1];
    if (i < 0 || i >= wa->nidx)
        return (ndx_index *)0;
    return wa->idx[i];
}

/* ===================================================================== */
/* Interpreter handle (interp.h)                                          */
/* ===================================================================== */

xb_interp *xb_interp_make(samir_pal_t *pal)
{
    xb_interp *ip;
    void *mark;
    uint8_t yy = 0, mm = 1, dd = 1;

    if (!pal)
        return (xb_interp *)0;

    /* Take an arena mark up front so xb_interp_free can unwind everything. */
    mark = pal->alloc(pal, 0u);        /* a zero-byte alloc returns the bump ptr */

    ip = (xb_interp *)pal->alloc(pal, (uint32_t)sizeof(*ip));
    if (!ip)
        return (xb_interp *)0;
    ip->pal  = pal;
    ip->mark = mark;

    ip->env = wa_env_make(pal);
    if (!ip->env)
        return (xb_interp *)0;

    ip->scratch = (char *)pal->alloc(pal, INTERP_SCRATCH_CAP);
    if (!ip->scratch)
        return (xb_interp *)0;

    /* Initialise the eval ctx: SET EXACT OFF (III+ default), scratch, today. */
    ip->ctx.set_exact    = 0;
    ip->ctx.scratch      = ip->scratch;
    ip->ctx.scratch_cap  = INTERP_SCRATCH_CAP;
    ip->ctx.scratch_used = 0u;

    pal->today(pal, &yy, &mm, &dd);
    /* yy is two-digit; 00..99. CENTURY handling lives above the PAL; for the
     * injectable host clock we map 00..99 -> 1900..1999 (the III+ default
     * CENTURY OFF window starts at 1900). DATE() reproducibility (Rule 11). */
    ip->ctx.ctx_today = (double)jdn_from_ymd(1900 + (int)yy, (int)mm, (int)dd);

    /* Bind the resolve glue: field names resolve against the selected area. */
    wa_bind_ctx(ip->env, &ip->ctx);

    return ip;
}

void xb_interp_free(xb_interp *ip)
{
    if (!ip)
        return;
    (void)wa_close_all(ip->env);
    /* unwind the arena to the mark taken at make (frees env + scratch + caches). */
    ip->pal->reset(ip->pal, ip->mark);
}

wa_env *xb_interp_env(xb_interp *ip)
{
    return ip ? ip->env : (wa_env *)0;
}

xb_ctx *xb_interp_ctx(xb_interp *ip)
{
    return ip ? &ip->ctx : (xb_ctx *)0;
}

samir_pal_t *xb_interp_pal(xb_interp *ip)
{
    return ip ? ip->pal : (samir_pal_t *)0;
}

/* Bounded pools for one expression (sized for S5.1..S5.8 expression shapes). */
#define INTERP_MAX_TOKS  128
#define INTERP_MAX_NODES 128

int xb_interp_eval_str(xb_interp *ip, const char *expr, uint32_t len,
                       xb_val *out, int *err_code)
{
    xb_token toks[INTERP_MAX_TOKS];
    xb_node  pool[INTERP_MAX_NODES];
    int ntok, root, rc, ec = 0;

    if (err_code)
        *err_code = 0;
    if (!ip || !expr || !out)
        return -INTERP_ERR_EVAL;

    ntok = xb_lex(expr, len, toks, (uint32_t)INTERP_MAX_TOKS, &ec);
    if (ntok < 0) {
        if (err_code) *err_code = ec;
        return -INTERP_ERR_LEX;
    }

    root = xb_parse(toks, (uint32_t)ntok, pool, (uint32_t)INTERP_MAX_NODES, &ec);
    if (root < 0) {
        if (err_code) *err_code = ec;
        return -INTERP_ERR_PARSE;
    }

    rc = xb_eval(pool, root, &ip->ctx, out, &ec);
    if (rc != 0) {
        if (err_code) *err_code = ec;
        return -INTERP_ERR_EVAL;
    }
    return INTERP_OK;
}
