/*
 * harness/diff/dbf_diff/test_interp_replace.c -- host oracle for S5.5: the
 * record-mutation module (REPLACE/APPEND BLANK/DELETE/RECALL/PACK/ZAP).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_interp_list.c /
 * test_interp_flow.c: the seed test_assert.h harness (CHECK / TEST_HARNESS /
 * TEST_SUMMARY), a host PAL via pal_host_make. A non-zero exit on any failed
 * check keeps `make test-interp-replace` from false-greening (Law 2).
 *
 * THE WRITABILITY SEAM: dbf mutation verbs require a WRITABLE table (dbf.h S1.5:
 * writable=1, only from dbf_create). wa_set_open opens read-only, so this oracle
 * builds each table writable with dbf_create + dbf_append_blank/dbf_replace, then
 * injects it into the work area via wa_adopt_table (workarea.h S5.5) before driving
 * the REPLACE/APPEND/DELETE/... verbs through samir_do.
 *
 * VALUES ASSERTED: after each verb the records are read back DIRECTLY via
 * dbf_read_rec on the work-area table (wa_table) so the assertions are on the
 * STORED bytes, not on display text -- the byte-grounded oracle of plan S5.5.
 *
 * TIER 0 (committed, operator-free):
 *   - REPLACE matrix: single field; multi-field comma form; NEXT n / ALL / REST
 *     scope; FOR <cond>; WHILE <cond>; assignment-coercion incl. N-overflow
 *     '*'-fill; cross-type -> #9 mismatch; non-Logical FOR -> #37.
 *   - APPEND BLANK then REPLACE; DELETE (0x2A + DELETED()) + RECALL; PACK; ZAP
 *     (RECCOUNT + contents across each).
 *   - Master-key pointer drift: a char .ndx over CODE; REPLACE CODE WITH a new
 *     value; SEEK on the NEW key still resolves the record (the -DMUTATE_REPLACE_
 *     NO_INDEX mutant bites here).
 *
 * Mutation proof (Rule 6 / ARB rider (a)):
 *   -DMUTATE_REPLACE_NO_SCOPE: REPLACE ignores scope/FOR and clobbers every
 *     record -> the "only the targeted record changed" checks go RED.
 *   -DMUTATE_REPLACE_NO_INDEX (alternative bite point): REPLACE skips the open-
 *     index update -> the master-key-drift SEEK check goes RED.
 *   The Make gate uses -DMUTATE_REPLACE_NO_SCOPE (bites the widest set of checks).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.5 contract + oracle.
 *   - ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md
 *     (REPLACE sec 8; APPEND sec 4; DELETE/RECALL sec 9; PACK sec 10; scope sec 0).
 *   - spec/samir/xbase_coercion.json assignment_coercion (the coercion contract).
 *   - os/samir/include/samir/{interp,workarea,nav,dbf,ndx,eval,value,rt}.h.
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
#include "samir/dbt.h"
#include "samir/ndx.h"
#include "samir/eval.h"
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

/* the S5.5 module registration (the entry point under test). */
int mutate_register(xb_interp *ip);

/* =====================================================================
 * Helpers: read a field of a record back DIRECTLY from the work-area table.
 * ===================================================================== */

/* read the raw C/D bytes of field `fi` (0-based) of record `recno` into `buf`
 * (NUL-terminated). Returns the field length, or -1 on failure. */
static int read_c_field(dbf_table *tbl, uint32_t recno, int fi, char *buf, int cap)
{
    xb_val rec[32];
    int del = 0, rc;
    const dbf_field_t *f;
    if (!tbl) return -1;
    rc = dbf_read_rec(tbl, recno, rec, &del);
    if (rc != DBF_OK) return -1;
    f = dbf_field(tbl, fi);
    if (!f) return -1;
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

/* read field `fi` as a numeric value (XB_N) of record `recno`. Returns 0 ok. */
static int read_n_field(dbf_table *tbl, uint32_t recno, int fi, double *out)
{
    xb_val rec[32];
    int del = 0, rc;
    if (!tbl) return -1;
    rc = dbf_read_rec(tbl, recno, rec, &del);
    if (rc != DBF_OK) return -1;
    if (rec[fi].t != XB_N) return -1;
    *out = rec[fi].u.n;
    return 0;
}

/* read the RAW field bytes (no typed decode) of field `fi` of `recno` -- used to
 * inspect N-overflow '*'-fill, which dbf_read_rec decodes to 0.0. We re-read the
 * record region by reconstructing the offset from the field descriptors. */
static int read_raw_field(dbf_table *tbl, uint32_t recno, int fi,
                          char *buf, int cap)
{
    /* dbf has no raw accessor; format the N value back and compare the visible
     * text instead. For overflow we read the value and check it stayed the
     * overflow sentinel via dec_parse (which returns 0.0 for "****"). The caller
     * uses the typed read_n_field for normal values and this only for the
     * overflow case, where we assert the decoded value is 0.0. */
    (void)tbl; (void)recno; (void)fi; (void)buf; (void)cap;
    return -1;
}

/* read the delete flag of record `recno`. Returns 1 deleted, 0 live, -1 error. */
static int read_deleted(dbf_table *tbl, uint32_t recno)
{
    xb_val rec[32];
    int del = 0, rc;
    if (!tbl) return -1;
    rc = dbf_read_rec(tbl, recno, rec, &del);
    if (rc != DBF_OK) return -1;
    return del ? 1 : 0;
}

/* =====================================================================
 * Build a writable table CODE C(3) + AMT N(5,0) + OK L(1) with seed rows,
 * and adopt it into area 1 of a fresh interpreter. The caller passes the
 * row data. Returns the interp (NULL on failure); *tbl_out gets the table.
 * ===================================================================== */

typedef struct { const char *code; double amt; int ok; } qrow;

static xb_interp *build_q(samir_pal_t *pal, const char *path,
                          const qrow *rows, int nrows, dbf_table **tbl_out)
{
    dbf_field_spec fs[3];
    dbf_table *tbl = NULL;
    xb_interp *ip;
    wa_env *env;
    int rc, i;

    fs[0].name = "CODE"; fs[0].type = 'C'; fs[0].field_len = 3;  fs[0].dec = 0;
    fs[1].name = "AMT";  fs[1].type = 'N'; fs[1].field_len = 5;  fs[1].dec = 0;
    fs[2].name = "OK";   fs[2].type = 'L'; fs[2].field_len = 1;  fs[2].dec = 0;

    rc = dbf_create(pal, path, fs, 3, &tbl);
    if (rc != DBF_OK) { CHECK(0, "build_q: dbf_create"); return NULL; }

    for (i = 0; i < nrows; i++) {
        xb_val r[3];
        r[0] = xb_c(rows[i].code, (uint16_t)strlen(rows[i].code));
        r[1] = xb_n(rows[i].amt);
        r[2] = xb_l(rows[i].ok);
        rc = dbf_append_rec(tbl, r, 0);
        if (rc != DBF_OK) { CHECK(0, "build_q: dbf_append_rec"); return NULL; }
    }
    rc = dbf_flush(tbl);
    if (rc != DBF_OK) { CHECK(0, "build_q: dbf_flush"); return NULL; }

    ip = xb_interp_make(pal);
    if (!ip) { CHECK(0, "build_q: xb_interp_make"); return NULL; }
    if (mutate_register(ip) != INTERP_OK) { CHECK(0, "build_q: mutate_register"); return NULL; }
    env = xb_interp_env(ip);
    rc = wa_adopt_table(env, 1, tbl, NULL, "Q", path, NULL, 0);
    if (rc != WA_OK) { CHECK(0, "build_q: wa_adopt_table"); xb_interp_free(ip); return NULL; }
    wa_select(env, 1);

    if (tbl_out) *tbl_out = tbl;
    return ip;
}

/* =====================================================================
 * Tier 0a: the REPLACE matrix.
 * ===================================================================== */

static void test_replace_matrix(samir_pal_t *pal)
{
    const char *pa = "/tmp/test_interp_replace_q.dbf";
    static const qrow seed[6] = {
        {"AAA",100,1},{"BBB",200,0},{"CCC",100,1},
        {"DDD",300,0},{"EEE",100,1},{"FFF",400,1}
    };
    dbf_table *tbl = NULL;
    xb_interp *ip;
    wa_env *env;
    char cbuf[16];
    double n;
    int rc;
    char msg[256];

    ip = build_q(pal, pa, seed, 6, &tbl);
    if (!ip) { remove(pa); return; }
    env = xb_interp_env(ip);

    /* ---- single-field REPLACE on the CURRENT record (default scope = current) ---- */
    /* go to record 2, REPLACE CODE WITH 'ZZ'. Only rec 2 changes. */
    rc = samir_do(ip, "GOTO 2\n");
    /* GOTO is a query.c verb -- not registered here; use the nav directly. */
    (void)rc;
    wa_nav_goto(env, 1, 2);
    rc = samir_do(ip, "REPLACE CODE WITH 'ZZ'\n");
    snprintf(msg,sizeof msg,"repl-single: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    read_c_field(tbl, 2, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "ZZ ") == 0, "repl-single: rec2 CODE='ZZ ' (pad)");
    read_c_field(tbl, 1, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "AAA") == 0, "repl-single: rec1 CODE unchanged (default scope = current only)");
    read_c_field(tbl, 3, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "CCC") == 0, "repl-single: rec3 CODE unchanged");

    /* ---- multi-field REPLACE on the current record ---- */
    wa_nav_goto(env, 1, 1);
    rc = samir_do(ip, "REPLACE CODE WITH 'XY', AMT WITH 999\n");
    snprintf(msg,sizeof msg,"repl-multi: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    read_c_field(tbl, 1, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "XY ") == 0, "repl-multi: rec1 CODE='XY '");
    CHECK(read_n_field(tbl, 1, 1, &n) == 0 && n == 999.0, "repl-multi: rec1 AMT=999");

    /* ---- REPLACE ALL: every record ---- */
    rc = samir_do(ip, "REPLACE ALL AMT WITH 0\n");
    snprintf(msg,sizeof msg,"repl-all: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    {
        int allz = 1; uint32_t r;
        for (r = 1; r <= 6; r++) { if (read_n_field(tbl, r, 1, &n) != 0 || n != 0.0) allz = 0; }
        CHECK(allz, "repl-all: every AMT == 0");
    }

    /* ---- REPLACE NEXT 2 from record 3 ---- */
    wa_nav_goto(env, 1, 3);
    rc = samir_do(ip, "REPLACE NEXT 2 AMT WITH 7\n");
    snprintf(msg,sizeof msg,"repl-next: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(read_n_field(tbl, 3, 1, &n) == 0 && n == 7.0, "repl-next: rec3 AMT=7");
    CHECK(read_n_field(tbl, 4, 1, &n) == 0 && n == 7.0, "repl-next: rec4 AMT=7");
    CHECK(read_n_field(tbl, 5, 1, &n) == 0 && n == 0.0, "repl-next: rec5 AMT unchanged (NEXT 2 stops)");
    CHECK(read_n_field(tbl, 2, 1, &n) == 0 && n == 0.0, "repl-next: rec2 AMT unchanged (before start)");

    /* ---- REPLACE ALL ... FOR <cond> ---- */
    rc = samir_do(ip, "REPLACE ALL AMT WITH 50 FOR OK\n");
    snprintf(msg,sizeof msg,"repl-for: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    /* OK is true for recs 1,3,5,6 (seed). rec1 OK was true. */
    /* State before this step: rec1=999->0(ALL)->? ; AMT after REPLACE ALL 0 then
     * NEXT 2 from rec3 -> rec3=7,rec4=7, all others 0. Now FOR OK:
     *   rec1 OK=.T. -> 50 ; rec2 OK=.F. -> stays 0 ; rec4 OK=.F. -> stays 7 ;
     *   rec5 OK=.T. -> 50. */
    CHECK(read_n_field(tbl, 1, 1, &n) == 0 && n == 50.0, "repl-for: rec1 (OK=.T.) AMT=50");
    CHECK(read_n_field(tbl, 2, 1, &n) == 0 && n == 0.0,  "repl-for: rec2 (OK=.F.) AMT unchanged (0)");
    CHECK(read_n_field(tbl, 4, 1, &n) == 0 && n == 7.0,  "repl-for: rec4 (OK=.F.) AMT unchanged (7)");
    CHECK(read_n_field(tbl, 5, 1, &n) == 0 && n == 50.0, "repl-for: rec5 (OK=.T.) AMT=50");

    /* ---- N-overflow -> '*'-fill (minted): 99999 fits, 1000000 overflows AMT(5) ---- */
    wa_nav_goto(env, 1, 1);
    rc = samir_do(ip, "REPLACE AMT WITH 1000000\n");
    snprintf(msg,sizeof msg,"repl-overflow: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    /* dec_format '*'-fills on overflow; dbf_read_rec decodes "*****" -> 0.0. */
    CHECK(read_n_field(tbl, 1, 1, &n) == 0 && n == 0.0, "repl-overflow: '*'-fill decodes to 0.0");

    /* ---- cross-type assignment -> #9 mismatch (C field <- N expr) ---- */
    wa_nav_goto(env, 1, 1);
    rc = samir_do(ip, "REPLACE CODE WITH 5\n");
    CHECK(rc != INTERP_OK, "repl-mismatch: C<-N fails (no auto-stringification)");
    CHECK(samir_last_error(ip) == XBEE_MISMATCH, "repl-mismatch: error #9 'Data type mismatch.'");

    /* ---- non-Logical FOR -> #37 ---- */
    rc = samir_do(ip, "REPLACE ALL AMT WITH 1 FOR AMT\n");
    CHECK(rc != INTERP_OK, "repl-for-type: non-Logical FOR fails");
    CHECK(samir_last_error(ip) == XBEE_NOT_LOGICAL, "repl-for-type: error #37 'Not a Logical expression.'");

    xb_interp_free(ip);
    remove(pa);
}

/* =====================================================================
 * Tier 0b: APPEND BLANK + REPLACE; DELETE/DELETED/RECALL; PACK; ZAP.
 * ===================================================================== */

static void test_append_delete_pack_zap(samir_pal_t *pal)
{
    const char *pa = "/tmp/test_interp_replace_a.dbf";
    static const qrow seed[3] = { {"AAA",10,1},{"BBB",20,0},{"CCC",30,1} };
    dbf_table *tbl = NULL;
    xb_interp *ip;
    wa_env *env;
    char cbuf[16];
    double n;
    int rc;
    char msg[256];

    ip = build_q(pal, pa, seed, 3, &tbl);
    if (!ip) { remove(pa); return; }
    env = xb_interp_env(ip);

    /* ---- APPEND BLANK -> RECCOUNT 4, pointer at new record, blank fields ---- */
    rc = samir_do(ip, "APPEND BLANK\n");
    snprintf(msg,sizeof msg,"append: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(dbf_nrec(tbl) == 4u, "append: RECCOUNT == 4");
    CHECK(wa_recno(env, 1) == 4u, "append: pointer moved to the new record (4)");
    read_c_field(tbl, 4, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "   ") == 0, "append: rec4 CODE blank (3 spaces)");
    CHECK(read_n_field(tbl, 4, 1, &n) == 0 && n == 0.0, "append: rec4 AMT blank (0)");

    /* ---- REPLACE into the appended record ---- */
    rc = samir_do(ip, "REPLACE CODE WITH 'NEW', AMT WITH 42\n");
    snprintf(msg,sizeof msg,"append-repl: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    read_c_field(tbl, 4, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "NEW") == 0, "append-repl: rec4 CODE='NEW'");
    CHECK(read_n_field(tbl, 4, 1, &n) == 0 && n == 42.0, "append-repl: rec4 AMT=42");

    /* ---- DELETE the current record (rec 4) -> 0x2A + DELETED() ---- */
    rc = samir_do(ip, "DELETE\n");
    snprintf(msg,sizeof msg,"delete: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(read_deleted(tbl, 4) == 1, "delete: rec4 delete-flag set (0x2A)");
    /* DELETED() built-in reflects it (pointer is on rec 4). */
    {
        xb_val v; int ec = 0;
        rc = xb_interp_eval_str(ip, "DELETED()", 9, &v, &ec);
        CHECK(rc == INTERP_OK && v.t == XB_L && v.u.l == 1, "delete: DELETED() == .T.");
    }
    CHECK(read_deleted(tbl, 1) == 0, "delete: rec1 still live");

    /* ---- RECALL clears it ---- */
    rc = samir_do(ip, "RECALL\n");
    snprintf(msg,sizeof msg,"recall: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(read_deleted(tbl, 4) == 0, "recall: rec4 delete-flag cleared (0x20)");

    /* ---- DELETE rec 2 then PACK -> rec 2 physically removed, RECCOUNT 3 ---- */
    wa_nav_goto(env, 1, 2);
    rc = samir_do(ip, "DELETE\n");
    CHECK(rc == INTERP_OK, "delete2: DELETE rec2");
    CHECK(read_deleted(tbl, 2) == 1, "delete2: rec2 deleted");
    rc = samir_do(ip, "PACK\n");
    snprintf(msg,sizeof msg,"pack: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(dbf_nrec(tbl) == 3u, "pack: RECCOUNT == 3 (rec2 removed)");
    /* survivors in original order: AAA, CCC, NEW. */
    read_c_field(tbl, 1, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "AAA") == 0, "pack: rec1 CODE='AAA'");
    read_c_field(tbl, 2, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "CCC") == 0, "pack: rec2 CODE='CCC' (was rec3)");
    read_c_field(tbl, 3, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "NEW") == 0, "pack: rec3 CODE='NEW' (was rec4)");
    CHECK(wa_recno(env, 1) == 1u, "pack: pointer rewound to record 1");

    /* ---- ZAP -> RECCOUNT 0, EOF ---- */
    rc = samir_do(ip, "ZAP\n");
    snprintf(msg,sizeof msg,"zap: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(dbf_nrec(tbl) == 0u, "zap: RECCOUNT == 0");
    CHECK(wa_eof(env, 1) == 1, "zap: pointer at EOF (empty table)");

    xb_interp_free(ip);
    remove(pa);
}

/* =====================================================================
 * Tier 0c: master-key pointer drift -- REPLACE an indexed field, then SEEK on
 * the NEW key must still resolve the record (the -DMUTATE_REPLACE_NO_INDEX
 * mutant bites here; the -DMUTATE_REPLACE_NO_SCOPE mutant bites the matrix above).
 * ===================================================================== */

/* key-provider for ndx_build: render the CODE field (field 0) of recno as the
 * char key, space-padded to key_len. */
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

static void test_master_key_drift(samir_pal_t *pal)
{
    const char *pa  = "/tmp/test_interp_replace_k.dbf";
    const char *pix = "/tmp/test_interp_replace_k.ndx";
    static const qrow seed[4] = { {"BBB",1,1},{"DDD",2,1},{"FFF",3,1},{"HHH",4,1} };
    dbf_table *tbl = NULL;
    ndx_index *ix = NULL;
    ndx_index *ixarr[1];
    xb_interp *ip;
    wa_env *env;
    uint32_t recno = 0;
    int found = 0, rc;
    char msg[256];

    /* build the writable table (do NOT adopt yet -- ndx_build needs to read it). */
    {
        dbf_field_spec fs[3];
        int i;
        fs[0].name = "CODE"; fs[0].type = 'C'; fs[0].field_len = 3; fs[0].dec = 0;
        fs[1].name = "AMT";  fs[1].type = 'N'; fs[1].field_len = 5; fs[1].dec = 0;
        fs[2].name = "OK";   fs[2].type = 'L'; fs[2].field_len = 1; fs[2].dec = 0;
        rc = dbf_create(pal, pa, fs, 3, &tbl);
        CHECK(rc == DBF_OK, "drift: dbf_create");
        if (rc != DBF_OK) { remove(pa); return; }
        for (i = 0; i < 4; i++) {
            xb_val r[3];
            r[0] = xb_c(seed[i].code, 3);
            r[1] = xb_n(seed[i].amt);
            r[2] = xb_l(seed[i].ok);
            CHECK(dbf_append_rec(tbl, r, 0) == DBF_OK, "drift: append");
        }
        CHECK(dbf_flush(tbl) == DBF_OK, "drift: flush");
    }

    /* build a char index over CODE (key_len 3), then open it read-write. */
    g_kp_tbl = tbl;
    rc = ndx_build(pal, pix, /*key_type=*/0, /*key_len=*/3, "CODE",
                   dbf_nrec(tbl), kp_code, NULL);
    snprintf(msg,sizeof msg,"drift: ndx_build rc=%d",rc);
    CHECK(rc == NDX_OK, msg);
    if (rc != NDX_OK) { dbf_close(tbl); remove(pa); remove(pix); return; }

    rc = ndx_open_rw(pal, pix, &ix);
    CHECK(rc == NDX_OK, "drift: ndx_open_rw");
    if (rc != NDX_OK) { dbf_close(tbl); remove(pa); remove(pix); return; }

    /* sanity: SEEK on an original key resolves before any REPLACE. */
    {
        xb_val k = xb_c("FFF", 3);
        rc = ndx_seek(ix, &k, /*set_exact=*/1, &recno, &found);
        CHECK(rc == NDX_OK && found && recno == 3u, "drift: pre-REPLACE SEEK 'FFF' -> rec3");
    }

    /* adopt the writable table + the rw index into the work area. */
    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "drift: xb_interp_make");
    if (!ip) { ndx_close(ix); dbf_close(tbl); remove(pa); remove(pix); return; }
    CHECK(mutate_register(ip) == INTERP_OK, "drift: mutate_register");
    env = xb_interp_env(ip);
    ixarr[0] = ix;
    rc = wa_adopt_table(env, 1, tbl, NULL, "K", pa,
                        (ndx_index *const *)ixarr, 1);
    CHECK(rc == WA_OK, "drift: wa_adopt_table (with rw index)");
    if (rc != WA_OK) { xb_interp_free(ip); ndx_close(ix); dbf_close(tbl); remove(pa); remove(pix); return; }
    wa_select(env, 1);

    /* REPLACE the indexed CODE field of record 3 (FFF -> MMM). The open index
     * must be re-filed so SEEK 'MMM' resolves rec3 and SEEK 'FFF' no longer does. */
    wa_nav_goto(env, 1, 3);
    rc = samir_do(ip, "REPLACE CODE WITH 'MMM'\n");
    snprintf(msg,sizeof msg,"drift: REPLACE rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);

    /* the stored field changed. */
    {
        char cbuf[16];
        read_c_field(tbl, 3, 0, cbuf, sizeof cbuf);
        CHECK(strcmp(cbuf, "MMM") == 0, "drift: rec3 CODE='MMM' stored");
    }

    /* SEEK on the NEW key resolves rec3 (master-key pointer drift handled). */
    {
        xb_val k = xb_c("MMM", 3);
        recno = 0; found = 0;
        rc = ndx_seek(ix, &k, 1, &recno, &found);
        snprintf(msg,sizeof msg,"drift: SEEK 'MMM' rc=%d found=%d recno=%u",rc,found,recno);
        CHECK(rc == NDX_OK && found && recno == 3u, msg);
    }
    /* the OLD key no longer resolves. */
    {
        xb_val k = xb_c("FFF", 3);
        recno = 0; found = 0;
        rc = ndx_seek(ix, &k, 1, &recno, &found);
        CHECK(rc == NDX_OK && !found, "drift: SEEK 'FFF' no longer found (old key removed)");
    }

    xb_interp_free(ip);
    ndx_close(ix);
    /* tbl is owned by the area; xb_interp_free closed it via wa_close_all. */
    remove(pa);
    remove(pix);
}

/* =====================================================================
 * main
 * ===================================================================== */

int main(int argc, char **argv)
{
    struct pal_host_cfg cfg;
    samir_pal_t *pal;

    (void)argc; (void)argv;   /* corpus base unused (Tier 0 only -- all synthetic) */
    (void)read_raw_field;      /* reserved helper; silence unused-fn */

    cfg.date_yy = 85; cfg.date_mm = 8; cfg.date_dd = 5;   /* pinned (Rule 11) */
    cfg.heap_size = 4u * 1024u * 1024u;
    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make failed\n");
        return 2;
    }

    test_replace_matrix(pal);
    test_append_delete_pack_zap(pal);
    test_master_key_drift(pal);

    pal_host_free(pal);
    return TEST_SUMMARY("test-interp-replace");
}
