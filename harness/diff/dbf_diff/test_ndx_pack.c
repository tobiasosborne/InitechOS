/*
 * harness/diff/dbf_diff/test_ndx_pack.c -- host oracle for initech-x87g:
 *   PACK must REINDEX every open .ndx (survivors are renumbered).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_interp_replace.c
 * (the S5.5 mutation oracle): seed test_assert.h harness (CHECK / TEST_HARNESS /
 * TEST_SUMMARY), a host PAL via pal_host_make, a WRITABLE table built with
 * dbf_create + dbf_append_rec, .ndx built with ndx_build + ndx_open_rw, injected
 * into the work area via wa_adopt_table, and the PACK verb driven through samir_do.
 *
 * THE BUG (initech-x87g): dBASE III PLUS PACK physically removes deleted records
 * AND *renumbers the survivors*, so their physical recnos CHANGE. ASSIST.HLP:120
 * "...and adjusts all open index files"; data-definition-and-manipulation.md
 * sec.10 "rebuilds/adjusts all open index files so they stay consistent". A per-
 * deleted-recno ndx_delete_key is INSUFFICIENT (survivor entries would still carry
 * the OLD recnos); the only correct fix is a FULL REINDEX of every open .ndx from
 * the freshly-packed table. Before the fix, m_pack calls dbf_pack + wa_refresh +
 * wa_nav_reset but never touches the open .ndx, so post-PACK SEEK resolves the
 * STALE physical recno (wrong record; deleted keys still "found").
 *
 * INDEPENDENT GRADING (Law 2): the expected post-PACK recnos are HAND-AUTHORED
 * from the III+ renumber-survivors semantics (delete rec 2 of 5 -> survivors in
 * original order become recnos 1,2,3,4), NOT read back from the index under test.
 * Each SEEK is graded against those constants AND cross-checked by reading the
 * .dbf record at the returned recno (dbf_read_rec -- a different code path than
 * the .ndx SEEK). Two indexes (char CODE + numeric AMT) prove the reindex loop
 * enumerates ALL open indexes, not just the master.
 *
 * Mutation proof (Rule 6): build with -DMUTATE_PACK_NO_REINDEX -> m_pack skips the
 * reindex -> the stale-recno SEEK checks go RED (make test-ndx-pack-mutant).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md sec.10
 *     (PACK: removes deleted records, compacts, rebuilds/adjusts all open index
 *     files; record pointer -> record 1).
 *   - os/samir/include/samir/ndx.h (ndx_build / ndx_rebuild / ndx_seek);
 *     os/samir/include/samir/workarea.h (wa_index_count / wa_index / wa_adopt_table).
 *   - initech-x87g.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "test_assert.h"          /* seed/, on -Iseed */
#include "samir/interp.h"         /* os/samir/include/, on -Ios/samir/include */
#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/dbf.h"
#include "samir/ndx.h"
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

/* pal_host.c surface (not declared in a header; declare what we use). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* the S5.5 module registration (mutate_cmd_hook -- PACK lives here). */
int mutate_register(xb_interp *ip);

/* -----------------------------------------------------------------------
 * Read a C field of a record back DIRECTLY from the table (independent path).
 * ----------------------------------------------------------------------- */
static int read_c_field(dbf_table *tbl, uint32_t recno, int fi, char *buf, int cap)
{
    xb_val rec[32];
    int del = 0, rc;
    if (!tbl) return -1;
    rc = dbf_read_rec(tbl, recno, rec, &del);
    if (rc != DBF_OK) return -1;
    if (rec[fi].t == XB_C || rec[fi].t == XB_M) {
        int n = (int)rec[fi].u.c.len;
        int i;
        if (n > cap - 1) n = cap - 1;
        for (i = 0; i < n; i++) buf[i] = rec[fi].u.c.p[i];
        buf[n] = '\0';
        return n;
    }
    buf[0] = '\0';
    return 0;
}

/* -----------------------------------------------------------------------
 * ndx_build key providers: render CODE (char) / AMT (numeric) of `g_kp_tbl`.
 * ----------------------------------------------------------------------- */
static dbf_table *g_kp_tbl;

static int kp_code(void *user, uint32_t recno, uint8_t *key_out, uint16_t key_len)
{
    xb_val rec[8];
    int del = 0;
    (void)user;
    if (dbf_read_rec(g_kp_tbl, recno, rec, &del) != DBF_OK) return -1;
    memset(key_out, ' ', key_len);
    if (rec[0].t == XB_C && rec[0].u.c.p) {
        uint16_t n = rec[0].u.c.len;
        if (n > key_len) n = key_len;
        memcpy(key_out, rec[0].u.c.p, n);
    }
    return 0;
}

static int kp_amt(void *user, uint32_t recno, uint8_t *key_out, uint16_t key_len)
{
    xb_val rec[8];
    int del = 0;
    double d;
    (void)user;
    if (dbf_read_rec(g_kp_tbl, recno, rec, &del) != DBF_OK) return -1;
    d = (rec[1].t == XB_N) ? rec[1].u.n : 0.0;
    memset(key_out, 0, key_len);
    memcpy(key_out, &d, (key_len < 8u) ? key_len : 8u);
    return 0;
}

/* -----------------------------------------------------------------------
 * SEEK helpers: return recno (0 if not found) via out params.
 * ----------------------------------------------------------------------- */
static int seek_char(ndx_index *ix, const char *s, uint32_t *recno, int *found)
{
    xb_val k = xb_c(s, (uint16_t)strlen(s));
    *recno = 0; *found = 0;
    return ndx_seek(ix, &k, /*set_exact=*/1, recno, found);
}

static int seek_num(ndx_index *ix, double v, uint32_t *recno, int *found)
{
    xb_val k = xb_n(v);
    *recno = 0; *found = 0;
    return ndx_seek(ix, &k, /*set_exact=*/0, recno, found);
}

/* =====================================================================
 * The oracle: 5-record table (CODE C(3) + AMT N(5,0) + OK L(1)), a char .ndx
 * over CODE and a numeric .ndx over AMT. DELETE record 2, PACK, then SEEK each
 * surviving key -- must land on the correct RENUMBERED record.
 * ===================================================================== */

typedef struct { const char *code; double amt; } prow;

static void test_pack_reindex(samir_pal_t *pal)
{
    const char *pa   = "/tmp/test_ndx_pack.dbf";
    const char *pcx  = "/tmp/test_ndx_pack_code.ndx";
    const char *pnx  = "/tmp/test_ndx_pack_amt.ndx";
    /* distinct CODE + AMT so every SEEK is unambiguous. */
    static const prow seed[5] = {
        {"AAA", 10.0}, {"BBB", 20.0}, {"CCC", 30.0}, {"DDD", 40.0}, {"EEE", 50.0}
    };
    dbf_table *tbl = NULL;
    ndx_index *cix = NULL, *nix = NULL;
    ndx_index *ixarr[2];
    xb_interp *ip = NULL;
    wa_env *env;
    uint32_t recno = 0;
    int found = 0, rc, i;
    char msg[256], cbuf[16];

    /* --- build the writable table (do NOT adopt yet -- ndx_build reads it). --- */
    {
        dbf_field_spec fs[3];
        fs[0].name = "CODE"; fs[0].type = 'C'; fs[0].field_len = 3; fs[0].dec = 0;
        fs[1].name = "AMT";  fs[1].type = 'N'; fs[1].field_len = 5; fs[1].dec = 0;
        fs[2].name = "OK";   fs[2].type = 'L'; fs[2].field_len = 1; fs[2].dec = 0;
        rc = dbf_create(pal, pa, fs, 3, &tbl);
        CHECK(rc == DBF_OK, "pack: dbf_create");
        if (rc != DBF_OK) { remove(pa); return; }
        for (i = 0; i < 5; i++) {
            xb_val r[3];
            r[0] = xb_c(seed[i].code, 3);
            r[1] = xb_n(seed[i].amt);
            r[2] = xb_l(1);
            CHECK(dbf_append_rec(tbl, r, 0) == DBF_OK, "pack: dbf_append_rec");
        }
        CHECK(dbf_flush(tbl) == DBF_OK, "pack: dbf_flush");
    }

    /* --- build BOTH indexes over the 5-record table, then open them RW. --- */
    g_kp_tbl = tbl;
    rc = ndx_build(pal, pcx, /*char*/0, 3, "CODE", dbf_nrec(tbl), kp_code, NULL);
    CHECK(rc == NDX_OK, "pack: ndx_build CODE");
    rc = ndx_build(pal, pnx, /*num*/1, 8, "AMT", dbf_nrec(tbl), kp_amt, NULL);
    CHECK(rc == NDX_OK, "pack: ndx_build AMT");
    rc = ndx_open_rw(pal, pcx, &cix);
    CHECK(rc == NDX_OK, "pack: ndx_open_rw CODE");
    rc = ndx_open_rw(pal, pnx, &nix);
    CHECK(rc == NDX_OK, "pack: ndx_open_rw AMT");
    if (!cix || !nix) { goto cleanup; }

    /* sanity: pre-PACK SEEK resolves the ORIGINAL recnos. */
    seek_char(cix, "CCC", &recno, &found);
    CHECK(found && recno == 3u, "pack: pre-PACK SEEK 'CCC' -> rec3 (original)");
    seek_num(nix, 40.0, &recno, &found);
    CHECK(found && recno == 4u, "pack: pre-PACK SEEK 40 -> rec4 (original)");

    /* --- adopt the writable table + BOTH rw indexes into area 1. --- */
    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "pack: xb_interp_make");
    if (!ip) goto cleanup;
    CHECK(mutate_register(ip) == INTERP_OK, "pack: mutate_register");
    env = xb_interp_env(ip);
    ixarr[0] = cix; ixarr[1] = nix;
    rc = wa_adopt_table(env, 1, tbl, NULL, "P", pa,
                        (ndx_index *const *)ixarr, 2);
    CHECK(rc == WA_OK, "pack: wa_adopt_table (2 rw indexes)");
    if (rc != WA_OK) goto cleanup;
    wa_select(env, 1);
    CHECK(wa_index_count(env, 1) == 2, "pack: 2 open indexes enumerable");

    /* --- DELETE record 2 (BBB), then PACK. --- */
    wa_nav_goto(env, 1, 2);
    rc = samir_do(ip, "DELETE\n");
    CHECK(rc == INTERP_OK, "pack: DELETE rec2");
    rc = samir_do(ip, "PACK\n");
    snprintf(msg, sizeof msg, "pack: PACK rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(dbf_nrec(tbl) == 4u, "pack: RECCOUNT == 4 (rec2 removed)");

    /* survivors physically renumbered 1..4: AAA,CCC,DDD,EEE. */
    read_c_field(tbl, 2, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "CCC") == 0, "pack: physical rec2 == 'CCC' (was rec3)");

    /* ===== THE ORACLE: every surviving key SEEKs to its RENUMBERED recno. =====
     * Hand-authored expected recnos (Law 2): AAA:1 CCC:2 DDD:3 EEE:4; BBB gone. */
    {
        struct { const char *code; double amt; uint32_t want; } exp[4] = {
            {"AAA", 10.0, 1u}, {"CCC", 30.0, 2u},
            {"DDD", 40.0, 3u}, {"EEE", 50.0, 4u}
        };
        for (i = 0; i < 4; i++) {
            /* char index over CODE */
            rc = seek_char(cix, exp[i].code, &recno, &found);
            snprintf(msg, sizeof msg,
                     "pack: SEEK CODE '%s' -> rec%u (renumbered) rc=%d found=%d got=%u",
                     exp[i].code, exp[i].want, rc, found, recno);
            CHECK(rc == NDX_OK && found && recno == exp[i].want, msg);
            /* cross-check: the .dbf record at that recno really carries CODE. */
            if (found && recno >= 1u && recno <= dbf_nrec(tbl)) {
                read_c_field(tbl, recno, 0, cbuf, sizeof cbuf);
                snprintf(msg, sizeof msg,
                         "pack: rec%u CODE=='%s' (SEEK self-consistent)",
                         recno, exp[i].code);
                CHECK(strcmp(cbuf, exp[i].code) == 0, msg);
            }
            /* numeric index over AMT */
            rc = seek_num(nix, exp[i].amt, &recno, &found);
            snprintf(msg, sizeof msg,
                     "pack: SEEK AMT %.0f -> rec%u (renumbered) rc=%d found=%d got=%u",
                     exp[i].amt, exp[i].want, rc, found, recno);
            CHECK(rc == NDX_OK && found && recno == exp[i].want, msg);
        }
    }

    /* the DELETED key must be gone from BOTH indexes after PACK. */
    rc = seek_char(cix, "BBB", &recno, &found);
    CHECK(rc == NDX_OK && !found, "pack: SEEK CODE 'BBB' not found (packed out)");
    rc = seek_num(nix, 20.0, &recno, &found);
    CHECK(rc == NDX_OK && !found, "pack: SEEK AMT 20 not found (packed out)");

cleanup:
    if (ip) {
        xb_interp_free(ip);   /* closes the area -> closes tbl + both indexes */
    } else {
        if (cix) ndx_close(cix);
        if (nix) ndx_close(nix);
        if (tbl) dbf_close(tbl);
    }
    remove(pa);
    remove(pcx);
    remove(pnx);
}

/* =====================================================================
 * main
 * ===================================================================== */
int main(int argc, char **argv)
{
    struct pal_host_cfg cfg;
    samir_pal_t *pal;

    (void)argc; (void)argv;   /* all synthetic; corpus base unused */

    cfg.date_yy = 85; cfg.date_mm = 8; cfg.date_dd = 5;   /* pinned (Rule 11) */
    cfg.heap_size = 4u * 1024u * 1024u;
    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make failed\n");
        return 2;
    }

    test_pack_reindex(pal);

    pal_host_free(pal);
    return TEST_SUMMARY("test-ndx-pack");
}
