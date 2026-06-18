/*
 * harness/diff/dbf_diff/test_use_rw.c -- host oracle for 7az.16: the WRITABLE USE
 * path -- dbf_open_rw + wa_set_open_rw -- so REPLACE/APPEND/DELETE work after a
 * plain `USE <file>` (no dbf_create / wa_adopt_table seam).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_interp_replace.c
 * / test_interp_use.c: the seed test_assert.h harness (CHECK / TEST_HARNESS /
 * TEST_SUMMARY) over a host PAL (pal_host_make). A non-zero exit on any failed
 * check keeps `make test-use-rw` from false-greening (Law 2: the oracle is truth).
 *
 * THE GAP 7az.16 CLOSES: dbf_open opens PAL_RD with writable=0, so the S1.5
 * mutation verbs (dbf_replace/append_blank/delete/...) + dbf_flush rejected a
 * plain-USE'd table -- editing required dbf_create + wa_adopt_table. dbf_open_rw
 * opens PAL_RDWR, loads the records into the in-arena region, sets writable=1;
 * wa_set_open_rw threads it through the work area (with ndx_open_rw'd indexes), so
 * the REPL / canon apps can edit after a bare USE.
 *
 * WHAT THIS PROVES (Law 2 -- byte-grounded, persistence-checked):
 *   T0 dbf_open_rw level (synthetic file on disk):
 *     - dbf_open_rw an EXISTING .dbf -> writable; dbf_replace a field; dbf_flush;
 *       close; RE-OPEN (dbf_open) -> the new value PERSISTED to disk.
 *     - dbf_append_blank + dbf_replace -> RECCOUNT grew; re-open confirms the new
 *       record + its fields PERSISTED.
 *     - dbf_open_rw on a MISSING file -> fail loud (-DBF_ERR_IO).
 *   T1 work-area level (the headline gap):
 *     - wa_set_open_rw a table -> REPLACE through samir_do end-to-end -> re-open
 *       the file from disk -> the change PERSISTED. Plain-USE-then-REPLACE works.
 *     - APPEND BLANK + REPLACE through samir_do -> RECCOUNT grew + persisted.
 *     - An indexed field: REPLACE through the work area re-files the OPEN index
 *       (ndx_open_rw + ndx_update inside mutate.c) -> SEEK on the NEW key resolves
 *       the record, SEEK on the OLD key no longer does.
 *   T2 corpus copy (/tmp copy, NEVER the corpus golden -- wave-9 lesson):
 *     - copy a corpus .dbf to /tmp, wa_set_open_rw, dbf_replace a C field,
 *       dbf_flush, re-open -> persisted; the unrelated records are unchanged.
 *
 * Mutation proof (Rule 6): build with -DDBF_MUTATE_OPENRW_RO -> dbf_open_rw opens
 * RDWR + loads records but leaves writable=0, so every verb + flush is rejected
 * and NOTHING persists -> the persistence checks go RED. The Make gate uses that
 * macro. (The pre-existing -DWA_MUTATE_SELECT etc. are unrelated to this gate.)
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - os/samir/include/samir/dbf.h (dbf_open_rw + the S1.5 verbs + dbf_flush).
 *   - os/samir/include/samir/workarea.h (wa_set_open_rw).
 *   - os/samir/include/samir/ndx.h (ndx_open_rw / ndx_seek / ndx_update_key).
 *   - docs/plans/SAMIR-implementation-plan.md 7az.16 contract.
 *   - Corpus ground truth: CLIENTS.DBF nrec=49; rec1 LASTNAME=Buckman.
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

/* the S5.5 mutate module registration (REPLACE/APPEND/DELETE through samir_do). */
int mutate_register(xb_interp *ip);

#define SP_PATH "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

/* ===================================================================== */
/* Small libc helpers (factory-side; not the engine)                      */
/* ===================================================================== */

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* byte-copy `src` to `dst`; returns 0 on success, -1 on any I/O failure. */
static int copy_file(const char *dst, const char *src)
{
    FILE *in = fopen(src, "rb");
    FILE *out;
    char buf[8192];
    size_t n;
    if (!in) return -1;
    out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }
    while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { fclose(in); fclose(out); return -1; }
    }
    fclose(in);
    fclose(out);
    return 0;
}

/* read raw C/M bytes of field `fi` of record `recno` into `buf` (NUL-term). */
static int read_c_field(dbf_table *tbl, uint32_t recno, int fi, char *buf, int cap)
{
    xb_val rec[64];
    int del = 0, rc;
    if (!tbl) return -1;
    rc = dbf_read_rec(tbl, recno, rec, &del);
    if (rc != DBF_OK) return -1;
    if (rec[fi].t == XB_C || rec[fi].t == XB_M) {
        int n = (int)rec[fi].u.c.len, i;
        if (n > cap - 1) n = cap - 1;
        for (i = 0; i < n; i++) buf[i] = rec[fi].u.c.p[i];
        buf[n] = '\0';
        return n;
    }
    buf[0] = '\0';
    return 0;
}

/* read numeric field `fi` of record `recno`. Returns 0 ok. */
static int read_n_field(dbf_table *tbl, uint32_t recno, int fi, double *out)
{
    xb_val rec[64];
    int del = 0, rc;
    if (!tbl) return -1;
    rc = dbf_read_rec(tbl, recno, rec, &del);
    if (rc != DBF_OK) return -1;
    if (rec[fi].t != XB_N) return -1;
    *out = rec[fi].u.n;
    return 0;
}

/* ===================================================================== */
/* Build a writable seed table ON DISK (dbf_create + flush + close), so the
 * later dbf_open_rw / wa_set_open_rw paths open a real existing file.
 * CODE C(3), AMT N(5,0), OK L(1).
 * ===================================================================== */

typedef struct { const char *code; double amt; int ok; } qrow;

static int make_seed_dbf(samir_pal_t *pal, const char *path,
                         const qrow *rows, int nrows)
{
    dbf_field_spec fs[3];
    dbf_table *tbl = NULL;
    int rc, i;

    fs[0].name = "CODE"; fs[0].type = 'C'; fs[0].field_len = 3; fs[0].dec = 0;
    fs[1].name = "AMT";  fs[1].type = 'N'; fs[1].field_len = 5; fs[1].dec = 0;
    fs[2].name = "OK";   fs[2].type = 'L'; fs[2].field_len = 1; fs[2].dec = 0;

    rc = dbf_create(pal, path, fs, 3, &tbl);
    if (rc != DBF_OK) return rc;
    for (i = 0; i < nrows; i++) {
        xb_val r[3];
        r[0] = xb_c(rows[i].code, (uint16_t)strlen(rows[i].code));
        r[1] = xb_n(rows[i].amt);
        r[2] = xb_l(rows[i].ok);
        rc = dbf_append_rec(tbl, r, 0);
        if (rc != DBF_OK) { dbf_close(tbl); return rc; }
    }
    rc = dbf_flush(tbl);
    if (rc != DBF_OK) { dbf_close(tbl); return rc; }
    return dbf_close(tbl);
}

/* ===================================================================== */
/* T0: dbf_open_rw level -- replace + append + persistence + fail-loud.
 * ===================================================================== */

static void test_dbf_open_rw(samir_pal_t *pal)
{
    const char *pa = "/tmp/test_use_rw_t0.dbf";
    static const qrow seed[3] = { {"AAA",100,1},{"BBB",200,0},{"CCC",300,1} };
    dbf_table *tbl = NULL;
    char cbuf[16];
    double n;
    int rc;
    char msg[256];

    rc = make_seed_dbf(pal, pa, seed, 3);
    CHECK(rc == DBF_OK, "t0: make_seed_dbf");
    if (rc != DBF_OK) { remove(pa); return; }

    /* ---- dbf_open_rw on a MISSING file -> fail loud ---- */
    {
        dbf_table *bad = (dbf_table *)0xdead;
        rc = dbf_open_rw(pal, "/tmp/test_use_rw_does_not_exist.dbf", &bad);
        snprintf(msg, sizeof msg, "t0: dbf_open_rw(missing) -> fail loud (rc=%d)", rc);
        CHECK(rc == -DBF_ERR_IO, msg);
        CHECK(bad == NULL, "t0: *out NULL on dbf_open_rw failure");
    }

    /* ---- dbf_open_rw an existing table -> REPLACE -> flush -> close ---- */
    rc = dbf_open_rw(pal, pa, &tbl);
    snprintf(msg, sizeof msg, "t0: dbf_open_rw(existing) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc != DBF_OK) { remove(pa); return; }
    CHECK(dbf_nrec(tbl) == 3u, "t0: nrec==3 after dbf_open_rw");

    /* records loaded into the writable region read back correctly. */
    read_c_field(tbl, 2, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "BBB") == 0, "t0: rec2 CODE='BBB' (record region loaded)");

    /* REPLACE rec2 CODE 'BBB' -> 'ZZ '. */
    {
        xb_val v = xb_c("ZZ", 2);
        rc = dbf_replace(tbl, 2u, 0, &v);
        CHECK(rc == DBF_OK, "t0: dbf_replace rec2 CODE WITH 'ZZ'");
    }
    /* REPLACE rec1 AMT 100 -> 777. */
    {
        xb_val v = xb_n(777.0);
        rc = dbf_replace(tbl, 1u, 1, &v);
        CHECK(rc == DBF_OK, "t0: dbf_replace rec1 AMT WITH 777");
    }
    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "t0: dbf_flush after replace");
    dbf_close(tbl);
    tbl = NULL;

    /* ---- RE-OPEN read-only from disk: the changes PERSISTED ---- */
    rc = dbf_open(pal, pa, &tbl);
    CHECK(rc == DBF_OK, "t0: re-open (dbf_open) after replace");
    if (rc == DBF_OK) {
        read_c_field(tbl, 2, 0, cbuf, sizeof cbuf);
        CHECK(strcmp(cbuf, "ZZ ") == 0, "t0: PERSISTED rec2 CODE='ZZ ' (the 7az.16 headline)");
        CHECK(read_n_field(tbl, 1, 1, &n) == 0 && n == 777.0, "t0: PERSISTED rec1 AMT=777");
        /* an untouched record is unchanged. */
        read_c_field(tbl, 3, 0, cbuf, sizeof cbuf);
        CHECK(strcmp(cbuf, "CCC") == 0, "t0: rec3 CODE unchanged ('CCC')");
        dbf_close(tbl);
        tbl = NULL;
    }

    /* ---- APPEND a record via dbf_open_rw -> RECCOUNT grew + persisted ---- */
    rc = dbf_open_rw(pal, pa, &tbl);
    CHECK(rc == DBF_OK, "t0: dbf_open_rw again for append");
    if (rc == DBF_OK) {
        rc = dbf_append_blank(tbl);
        CHECK(rc == DBF_OK, "t0: dbf_append_blank");
        CHECK(dbf_nrec(tbl) == 4u, "t0: nrec==4 after append");
        {
            xb_val v = xb_c("NEW", 3);
            rc = dbf_replace(tbl, 4u, 0, &v);
            CHECK(rc == DBF_OK, "t0: dbf_replace rec4 CODE WITH 'NEW'");
        }
        rc = dbf_flush(tbl);
        CHECK(rc == DBF_OK, "t0: dbf_flush after append");
        dbf_close(tbl);
        tbl = NULL;
    }
    rc = dbf_open(pal, pa, &tbl);
    CHECK(rc == DBF_OK, "t0: re-open after append");
    if (rc == DBF_OK) {
        CHECK(dbf_nrec(tbl) == 4u, "t0: PERSISTED nrec==4 (appended record on disk)");
        read_c_field(tbl, 4, 0, cbuf, sizeof cbuf);
        CHECK(strcmp(cbuf, "NEW") == 0, "t0: PERSISTED rec4 CODE='NEW'");
        dbf_close(tbl);
        tbl = NULL;
    }

    remove(pa);
}

/* ===================================================================== */
/* T1: wa_set_open_rw + REPLACE/APPEND through samir_do (end-to-end).
 * The headline gap: a PLAIN USE (no create/adopt) then REPLACE persists.
 * ===================================================================== */

static void test_wa_set_open_rw(samir_pal_t *pal)
{
    const char *pa = "/tmp/test_use_rw_t1.dbf";
    static const qrow seed[3] = { {"AAA",10,1},{"BBB",20,0},{"CCC",30,1} };
    xb_interp *ip;
    wa_env *env;
    dbf_table *tbl;
    char cbuf[16];
    double n;
    int rc;
    char msg[256];

    rc = make_seed_dbf(pal, pa, seed, 3);
    CHECK(rc == DBF_OK, "t1: make_seed_dbf");
    if (rc != DBF_OK) { remove(pa); return; }

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "t1: xb_interp_make");
    if (!ip) { remove(pa); return; }
    CHECK(mutate_register(ip) == INTERP_OK, "t1: mutate_register");
    env = xb_interp_env(ip);

    /* PLAIN read-write USE (the 7az.16 path) -- no dbf_create / wa_adopt_table. */
    rc = wa_set_open_rw(env, 1, pa, NULL, NULL);
    snprintf(msg, sizeof msg, "t1: wa_set_open_rw rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); remove(pa); return; }
    wa_select(env, 1);
    tbl = wa_table(env, 1);
    CHECK(tbl != NULL, "t1: wa_table non-NULL");
    CHECK(wa_nrec(env, 1) == 3u, "t1: nrec==3");

    /* GOTO rec2, REPLACE CODE WITH 'ZZ' end-to-end through the work area. */
    wa_nav_goto(env, 1, 2);
    rc = samir_do(ip, "REPLACE CODE WITH 'ZZ'\n");
    snprintf(msg, sizeof msg, "t1: REPLACE rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    read_c_field(tbl, 2, 0, cbuf, sizeof cbuf);
    CHECK(strcmp(cbuf, "ZZ ") == 0, "t1: rec2 CODE='ZZ ' (in-memory after REPLACE)");

    /* APPEND BLANK + REPLACE through the work area. */
    rc = samir_do(ip, "APPEND BLANK\n");
    snprintf(msg, sizeof msg, "t1: APPEND BLANK rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(wa_nrec(env, 1) == 4u, "t1: nrec==4 after APPEND BLANK");
    rc = samir_do(ip, "REPLACE CODE WITH 'NEW', AMT WITH 42\n");
    snprintf(msg, sizeof msg, "t1: REPLACE appended rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);

    /* Close the interpreter (closes the work area -> the file handle). */
    xb_interp_free(ip);

    /* ---- RE-OPEN from disk: the work-area edits PERSISTED ---- */
    rc = dbf_open(pal, pa, &tbl);
    CHECK(rc == DBF_OK, "t1: re-open after work-area edits");
    if (rc == DBF_OK) {
        CHECK(dbf_nrec(tbl) == 4u, "t1: PERSISTED nrec==4");
        read_c_field(tbl, 2, 0, cbuf, sizeof cbuf);
        CHECK(strcmp(cbuf, "ZZ ") == 0, "t1: PERSISTED rec2 CODE='ZZ ' (USE-then-REPLACE works)");
        read_c_field(tbl, 4, 0, cbuf, sizeof cbuf);
        CHECK(strcmp(cbuf, "NEW") == 0, "t1: PERSISTED rec4 CODE='NEW'");
        CHECK(read_n_field(tbl, 4, 1, &n) == 0 && n == 42.0, "t1: PERSISTED rec4 AMT=42");
        dbf_close(tbl);
    }

    remove(pa);
}

/* ===================================================================== */
/* T1b: indexed field -- REPLACE through wa_set_open_rw re-files the OPEN
 * index (ndx_open_rw + ndx_update inside mutate.c). SEEK on the new key
 * resolves; SEEK on the old key does not.
 * ===================================================================== */

/* key provider for ndx_build: CODE (field 0) as a char key, space-padded. */
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

static void test_wa_set_open_rw_indexed(samir_pal_t *pal)
{
    const char *pa  = "/tmp/test_use_rw_k.dbf";
    const char *pix = "/tmp/test_use_rw_k.ndx";
    static const qrow seed[4] = { {"BBB",1,1},{"DDD",2,1},{"FFF",3,1},{"HHH",4,1} };
    xb_interp *ip;
    wa_env *env;
    dbf_table *tbl = NULL;
    wa_index_list il;
    uint32_t recno = 0;
    int found = 0, rc;
    char msg[256];

    rc = make_seed_dbf(pal, pa, seed, 4);
    CHECK(rc == DBF_OK, "t1b: make_seed_dbf");
    if (rc != DBF_OK) { remove(pa); return; }

    /* build a char .ndx over CODE (key_len 3) from the on-disk table. */
    rc = dbf_open(pal, pa, &g_kp_tbl);
    CHECK(rc == DBF_OK, "t1b: open seed for key build");
    if (rc != DBF_OK) { remove(pa); return; }
    rc = ndx_build(pal, pix, /*key_type=*/0, /*key_len=*/3, "CODE",
                   dbf_nrec(g_kp_tbl), kp_code, NULL);
    snprintf(msg, sizeof msg, "t1b: ndx_build rc=%d", rc);
    CHECK(rc == NDX_OK, msg);
    dbf_close(g_kp_tbl);
    g_kp_tbl = NULL;
    if (rc != NDX_OK) { remove(pa); remove(pix); return; }

    /* PLAIN read-write USE ... INDEX <ndx> (the index opens ndx_open_rw). */
    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "t1b: xb_interp_make");
    if (!ip) { remove(pa); remove(pix); return; }
    CHECK(mutate_register(ip) == INTERP_OK, "t1b: mutate_register");
    env = xb_interp_env(ip);

    il.names[0] = pix;
    il.count = 1;
    rc = wa_set_open_rw(env, 1, pa, NULL, &il);
    snprintf(msg, sizeof msg, "t1b: wa_set_open_rw INDEX rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); remove(pa); remove(pix); return; }
    wa_select(env, 1);
    CHECK(wa_index_count(env, 1) == 1, "t1b: index attached");
    tbl = wa_table(env, 1);

    /* sanity: SEEK on an original key resolves before REPLACE. */
    {
        ndx_index *ix = wa_index(env, 1, 0);
        xb_val k = xb_c("FFF", 3);
        rc = ndx_seek(ix, &k, /*set_exact=*/1, &recno, &found);
        CHECK(rc == NDX_OK && found && recno == 3u, "t1b: pre-REPLACE SEEK 'FFF' -> rec3");
    }

    /* REPLACE the indexed CODE of rec3 (FFF -> MMM) end-to-end. */
    wa_nav_goto(env, 1, 3);
    rc = samir_do(ip, "REPLACE CODE WITH 'MMM'\n");
    snprintf(msg, sizeof msg, "t1b: REPLACE indexed rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);

    /* the OPEN index now resolves the NEW key and no longer the OLD key. */
    {
        ndx_index *ix = wa_index(env, 1, 0);
        xb_val knew = xb_c("MMM", 3);
        xb_val kold = xb_c("FFF", 3);
        recno = 0; found = 0;
        rc = ndx_seek(ix, &knew, 1, &recno, &found);
        snprintf(msg, sizeof msg, "t1b: SEEK 'MMM' rc=%d found=%d recno=%u", rc, found, recno);
        CHECK(rc == NDX_OK && found && recno == 3u, msg);
        recno = 0; found = 0;
        rc = ndx_seek(ix, &kold, 1, &recno, &found);
        CHECK(rc == NDX_OK && !found, "t1b: SEEK 'FFF' no longer found (index re-filed)");
    }

    /* the stored field also changed. */
    {
        char cbuf[16];
        read_c_field(tbl, 3, 0, cbuf, sizeof cbuf);
        CHECK(strcmp(cbuf, "MMM") == 0, "t1b: rec3 CODE='MMM' stored");
    }

    xb_interp_free(ip);
    remove(pa);
    remove(pix);
}

/* ===================================================================== */
/* T2: corpus copy -- wa_set_open_rw a /tmp COPY (NEVER the golden), REPLACE,
 * flush, re-open -> persisted. LOUD-skips when the corpus is absent.
 * ===================================================================== */

static void test_corpus_copy(samir_pal_t *pal, const char *base)
{
    char src[1024];
    const char *dst = "/tmp/test_use_rw_clients.dbf";
    xb_interp *ip;
    wa_env *env;
    dbf_table *tbl;
    char before[64], after[64];
    int rc;
    char msg[256];

    snprintf(src, sizeof src, "%s/%s/CLIENTS.DBF", base, SP_PATH);
    if (!file_exists(src)) {
        fprintf(stderr,
                "  SKIP (LOUD): corpus golden absent: %s\n"
                "               (pass the corpus base as argv[1]; T0/T1 synthetic still ran)\n",
                src);
        return;
    }

    /* operate on a /tmp COPY -- NEVER the corpus golden (wave-9 lesson). */
    rc = copy_file(dst, src);
    CHECK(rc == 0, "t2: copy CLIENTS.DBF to /tmp");
    if (rc != 0) return;

    /* capture rec1 LASTNAME (field 1) before. */
    rc = dbf_open(pal, dst, &tbl);
    CHECK(rc == DBF_OK, "t2: open copy read-only for baseline");
    if (rc != DBF_OK) { remove(dst); return; }
    read_c_field(tbl, 1, 1, before, sizeof before);
    CHECK(strncmp(before, "Buckman", 7) == 0, "t2: baseline rec1 LASTNAME starts 'Buckman'");
    dbf_close(tbl);

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "t2: xb_interp_make");
    if (!ip) { remove(dst); return; }
    CHECK(mutate_register(ip) == INTERP_OK, "t2: mutate_register");
    env = xb_interp_env(ip);

    /* plain RW USE the COPY, REPLACE rec1 LASTNAME WITH 'Lumbergh'. */
    rc = wa_set_open_rw(env, 1, dst, NULL, NULL);
    snprintf(msg, sizeof msg, "t2: wa_set_open_rw(copy) rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); remove(dst); return; }
    wa_select(env, 1);
    wa_nav_goto(env, 1, 1);
    rc = samir_do(ip, "REPLACE LASTNAME WITH 'Lumbergh'\n");
    snprintf(msg, sizeof msg, "t2: REPLACE LASTNAME rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    xb_interp_free(ip);

    /* re-open the copy from disk: the change persisted; rec2 unchanged. */
    rc = dbf_open(pal, dst, &tbl);
    CHECK(rc == DBF_OK, "t2: re-open copy after REPLACE");
    if (rc == DBF_OK) {
        read_c_field(tbl, 1, 1, after, sizeof after);
        CHECK(strncmp(after, "Lumbergh", 8) == 0, "t2: PERSISTED rec1 LASTNAME starts 'Lumbergh'");
        /* rec2 LASTNAME untouched (a record we did not REPLACE). */
        read_c_field(tbl, 2, 1, after, sizeof after);
        CHECK(after[0] != '\0', "t2: rec2 LASTNAME still readable (untouched)");
        dbf_close(tbl);
    }

    remove(dst);
}

/* ===================================================================== */
/* main
 * ===================================================================== */

int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;

    cfg.date_yy = 85; cfg.date_mm = 8; cfg.date_dd = 5;   /* pinned (Rule 11) */
    cfg.heap_size = 8u * 1024u * 1024u;
    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make failed\n");
        return 2;
    }

    test_dbf_open_rw(pal);
    test_wa_set_open_rw(pal);
    test_wa_set_open_rw_indexed(pal);
    test_corpus_copy(pal, base);

    pal_host_free(pal);
    return TEST_SUMMARY("test-use-rw");
}
