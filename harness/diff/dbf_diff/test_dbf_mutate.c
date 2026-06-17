/*
 * harness/diff/dbf_diff/test_dbf_mutate.c -- host oracle for S1.5 record
 * mutation verbs: dbf_append_blank, dbf_replace, dbf_delete/recall, dbf_pack,
 * dbf_zap. COMPLETES the Phase-1 .dbf codec.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here; engine is freestanding.
 * Mirrors test_dbf_roundtrip.c: seed/test_assert.h harness (CHECK /
 * TEST_HARNESS / TEST_SUMMARY), host PAL via pal_host_make.
 *
 * Tier-0 self-contained (creates its own tables; no external golden needed).
 * The independence barrier (dbf_ref.py read-back) is wired as oracle 6.
 *
 * Oracles (this file):
 *   1. APPEND_BLANK: verify blank fields (spaces / '?' for L), delete flag
 *      0x20, nrec bump, and that dbf_read_rec decodes the blank correctly.
 *   2. REPLACE (coercion success): C<-C truncate/pad; N<-N round-trip +
 *      overflow '*'-fill; D<-D; L<-L. Flush and read back to confirm bytes.
 *   3. REPLACE (coercion mismatch): cross-type attempts (N<-C, C<-N, D<-C,
 *      L<-N) each return -DBF_ERR_MISMATCH (fail loud; plan S1.5).
 *   4. DELETE / RECALL: dbf_delete sets the 0x2A flag; dbf_read_rec reports
 *      deleted=1. dbf_recall clears it back to 0x20. Flush and read back.
 *   5. PACK: append 4 records, delete records 1 and 3, pack; assert nrec=2,
 *      survivors are the original records 2 and 4, values intact.
 *   6. ZAP: nrec goes to 0; schema (nfields, record_length, version) intact;
 *      dbf_read_rec on recno 1 returns -DBF_ERR_BAD_RECNO.
 *   7. DETERMINISM (Rule 11): two identical sequences on separate arenas
 *      produce byte-identical output after flush.
 *   8. INDEPENDENCE BARRIER: the packed file from oracle 5 is left at a
 *      fixed path; dbf_ref.py --records reads it back (loud-skip if absent).
 *
 * Mutation hook (Rule 6): -DDBF_MUTATE_DELFLAG causes dbf_delete to write
 * 0x20 instead of 0x2A. Oracle 4 asserts the flag reads back as 0x2A; with
 * the mutant it stays 0x20 -> CHECK fails -> exit non-zero (RED). Oracle 5
 * (pack) also goes RED because no records are recognised as deleted.
 *
 * Compile + run (self-grade, NOT make):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbf.c os/samir/core/value.c os/samir/core/rt.c \
 *     os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbf_mutate.c -o /tmp/test_dbf_mutate \
 *     && /tmp/test_dbf_mutate ../dbase3-decomp
 *
 * Mutant (must exit non-zero / RED):
 *   gcc -std=c11 -Wall -Wextra -Werror -DDBF_MUTATE_DELFLAG \
 *     -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbf.c os/samir/core/value.c os/samir/core/rt.c \
 *     os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbf_mutate.c -o /tmp/test_dbf_mutate_mut \
 *     && /tmp/test_dbf_mutate_mut ../dbase3-decomp; echo "mutant exit=$?"
 *
 * ASCII-clean (Rule 12). Deterministic output (Rule 11): fixed /tmp paths.
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 6 (delete flag 0x20/0x2A),
 *     sec 8 (nrec invariant; PACK/ZAP).
 *   - spec/samir/xbase_coercion.json assignment_coercion (C<-C truncate/pad;
 *     N<-N stars_fill; cross-type -> mismatch).
 *   - os/samir/include/samir/dbf.h S1.5 contract (all new verbs + coercion).
 *   - os/samir/include/samir/value.h (xb_c/xb_n/xb_d/xb_l/xb_u).
 *   - os/samir/include/samir/rt.h (jdn_from_ymd, dec_format for overflow check).
 *   - harness/diff/dbf_diff/dbf_ref.py (independence barrier).
 *   - docs/plans/SAMIR-implementation-plan.md S1.5 oracle + ARB rider (a).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined(__unix__) || defined(__APPLE__)
#  include <sys/wait.h>
#endif

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/dbf.h"         /* all S1.1-S1.5 functions */
#include "samir/value.h"       /* xb_c/xb_n/xb_d/xb_l/xb_u */
#include "samir/rt.h"          /* jdn_from_ymd */
#include "samir/dbf_format.h"  /* DBF_REC_DELETE_LIVE/DELETED */

TEST_HARNESS();

/* pal_host.c surface (not in a header). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* Fixed injected date (Rule 11). */
#define MUT_YY  85
#define MUT_MM  10
#define MUT_DD  30

/* Fixed /tmp paths (Rule 11; never a wall-clock or PID in the path). */
#define MUT_PATH_BLANK  "/tmp/samir_mut_blank.dbf"
#define MUT_PATH_REPL   "/tmp/samir_mut_repl.dbf"
#define MUT_PATH_DELFLAG "/tmp/samir_mut_del.dbf"
#define MUT_PATH_PACK   "/tmp/samir_mut_pack.dbf"
#define MUT_PATH_ZAP    "/tmp/samir_mut_zap.dbf"
#define MUT_PATH_DET_A  "/tmp/samir_mut_det_a.dbf"
#define MUT_PATH_DET_B  "/tmp/samir_mut_det_b.dbf"
/* Independence barrier path -- left for dbf_ref.py. */
#define MUT_PATH_REF    "/tmp/samir_mutate_packed.dbf"

/* Shared schema: NAME C(8), SCORE N(6,2), JOINED D(8), ACTIVE L(1). */
static const dbf_field_spec g_schema[4] = {
    { "NAME",   'C', 8u, 0u },
    { "SCORE",  'N', 6u, 2u },
    { "JOINED", 'D', 8u, 0u },
    { "ACTIVE", 'L', 1u, 0u }
};
#define G_NFIELDS  4

static samir_pal_t *make_pal(void)
{
    struct pal_host_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = MUT_YY;
    cfg.date_mm   = MUT_MM;
    cfg.date_dd   = MUT_DD;
    cfg.heap_size = 512u * 1024u;
    return pal_host_make(cfg);
}

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

static uint8_t *slurp(const char *path, long *len)
{
    FILE *f;
    long sz;
    uint8_t *buf;
    size_t got;
    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    buf = (uint8_t *)malloc((size_t)sz + 1u);
    if (!buf) { fclose(f); return NULL; }
    got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(buf); return NULL; }
    *len = sz;
    return buf;
}

/* ============================================================
 * Helper: create a fresh table with the shared schema and append N records.
 * Returns the table handle or NULL on failure (failures are CHECKed by caller).
 * ============================================================ */
static dbf_table *make_table(samir_pal_t *pal, const char *path)
{
    dbf_table *tbl = NULL;
    int rc = dbf_create(pal, path, g_schema, G_NFIELDS, &tbl);
    if (rc != DBF_OK || !tbl) {
        CHECK(0, "helper make_table: dbf_create failed");
        return NULL;
    }
    return tbl;
}

/* Append a record with specific values. */
static void append_rec(dbf_table *tbl,
                        const char *name, double score,
                        int32_t jdn, int active,
                        int deleted_flag)
{
    xb_val rec[4];
    rec[0] = xb_c(name, (uint16_t)strlen(name));
    rec[1] = xb_n(score);
    rec[2] = xb_d((double)jdn);
    rec[3] = xb_l(active);
    dbf_append_rec(tbl, rec, deleted_flag);
}

/* ============================================================
 * Oracle 1: APPEND_BLANK
 *
 * Create a table, append_blank, flush, re-open, read back.
 * Assert: delete flag 0x20 (live), C=spaces, N=blank (dec_parse -> 0.0),
 * D=xb_u (blank date), L=xb_u ('?' on disk).
 * ============================================================ */
static void test_append_blank(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    int rc;
    char msg[256];
    xb_val out[4];
    int deleted;

    tbl = make_table(pal, MUT_PATH_BLANK);
    if (!tbl) return;

    /* Also append one normal record so we can verify blank is record 2. */
    append_rec(tbl, "Alice", 99.5, jdn_from_ymd(1985, 8, 5), 1, 0);

    rc = dbf_append_blank(tbl);
    snprintf(msg, sizeof(msg), "append_blank: rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    snprintf(msg, sizeof(msg), "append_blank: nrec after blank append == 2 (got %u)",
             dbf_nrec(tbl));
    CHECK(dbf_nrec(tbl) == 2u, msg);

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "append_blank: flush");
    dbf_close(tbl);
    tbl = NULL;

    /* Re-open and read back the blank record (recno 2). */
    rc = dbf_open(pal, MUT_PATH_BLANK, &tbl);
    snprintf(msg, sizeof(msg), "append_blank: re-open rc=%d", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    CHECK(dbf_nrec(tbl) == 2u, "append_blank: nrec==2 after re-open");

    deleted = -1;
    rc = dbf_read_rec(tbl, 2u, out, &deleted);
    CHECK(rc == DBF_OK, "append_blank: read rec2 (blank)");
    if (rc == DBF_OK) {
        /* Delete flag: 0x20 (live, not deleted). */
        CHECK(deleted == 0,
              "append_blank: blank record is LIVE (delete flag 0x20)");

        /* NAME C(8): 8 spaces. */
        CHECK(out[0].t == XB_C && out[0].u.c.len == 8u &&
              memcmp(out[0].u.c.p, "        ", 8) == 0,
              "append_blank: NAME is 8 spaces");

        /* SCORE N(6,2): all-spaces -> dec_parse -> 0.0. */
        CHECK(out[1].t == XB_N && out[1].u.n == 0.0,
              "append_blank: SCORE is 0.0 (blank N -> dec_parse -> 0.0)");

        /* JOINED D(8): 8 spaces -> blank date -> xb_u().
         * Ref: dbf.md sec 5 D; dbf.h S1.3 D-decode note. */
        CHECK(out[2].t == XB_U,
              "append_blank: JOINED is xb_u (blank D -> 8 spaces on disk)");

        /* ACTIVE L(1): '?' on disk -> xb_u() (uninitialised logical).
         * Ref: dbf.md sec 5 L; dbf_read_rec L-decode '?' -> xb_u. */
        CHECK(out[3].t == XB_U,
              "append_blank: ACTIVE is xb_u ('?' blank logical)");
    }

    dbf_close(tbl);
}

/* ============================================================
 * Oracle 2: REPLACE -- coercion success cases
 *
 * Build a table, append one blank record, then replace each field:
 *   NAME  <- "Hi" (C shorter -> pad to 8)
 *   SCORE <- 42.75 (N, normal value)
 *   JOINED <- JDN(1999,12,31) (D)
 *   ACTIVE <- 1 (L)
 * Then flush and read back to confirm byte round-trip.
 *
 * Also test N overflow ('*'-fill): replace SCORE with a huge value that
 * doesn't fit in 6 columns -> dec_format '*'-fills, returns DBF_OK.
 * After flush and re-open SCORE reads back as 0.0 (dec_parse on '*' chars
 * returns 0.0 -- the defined overflow semantics).
 * Ref: xbase_coercion.json assignment_coercion target:N on_overflow:stars_fill.
 * ============================================================ */
static void test_replace_ok(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    int rc;
    char msg[256];
    int32_t jdn99 = jdn_from_ymd(1999, 12, 31);
    xb_val v;
    xb_val out[4];
    int deleted;

    tbl = make_table(pal, MUT_PATH_REPL);
    if (!tbl) return;

    /* Append one blank record. */
    rc = dbf_append_blank(tbl);
    CHECK(rc == DBF_OK, "replace-ok: append_blank");

    /* Replace NAME with a short string (padding). */
    v = xb_c("Hi", 2u);
    rc = dbf_replace(tbl, 1u, 0, &v);
    snprintf(msg, sizeof(msg), "replace-ok: NAME <- 'Hi' rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    /* Replace SCORE with 42.75. */
    v = xb_n(42.75);
    rc = dbf_replace(tbl, 1u, 1, &v);
    snprintf(msg, sizeof(msg), "replace-ok: SCORE <- 42.75 rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    /* Replace JOINED with a date. */
    v = xb_d((double)jdn99);
    rc = dbf_replace(tbl, 1u, 2, &v);
    snprintf(msg, sizeof(msg), "replace-ok: JOINED <- JDN(1999-12-31) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    /* Replace ACTIVE with T. */
    v = xb_l(1);
    rc = dbf_replace(tbl, 1u, 3, &v);
    snprintf(msg, sizeof(msg), "replace-ok: ACTIVE <- T rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "replace-ok: flush");
    dbf_close(tbl);
    tbl = NULL;

    /* Re-open and read back. */
    rc = dbf_open(pal, MUT_PATH_REPL, &tbl);
    CHECK(rc == DBF_OK && tbl != NULL, "replace-ok: re-open");
    if (rc != DBF_OK || !tbl) return;

    deleted = -1;
    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    CHECK(rc == DBF_OK, "replace-ok: read rec1");
    if (rc == DBF_OK) {
        /* NAME: "Hi" + 6 trailing spaces = "Hi      " (8 bytes). */
        CHECK(out[0].t == XB_C && out[0].u.c.len == 8u &&
              memcmp(out[0].u.c.p, "Hi      ", 8) == 0,
              "replace-ok: NAME == 'Hi      ' (padded to 8)");

        /* SCORE: 42.75 round-trips through dec_format(42.75,6,2)/dec_parse. */
        CHECK(out[1].t == XB_N && out[1].u.n == 42.75,
              "replace-ok: SCORE == 42.75");

        /* JOINED: JDN(1999-12-31) round-trips. */
        CHECK(out[2].t == XB_D && out[2].u.d == (double)jdn99,
              "replace-ok: JOINED == JDN(1999-12-31)");

        /* ACTIVE: T -> xb_l(1). */
        CHECK(out[3].t == XB_L && out[3].u.l == 1u,
              "replace-ok: ACTIVE == T");
    }
    dbf_close(tbl);
    tbl = NULL;

    /* --- Overflow test: replace SCORE with 999999.99 into a N(6,2) field ---
     * 999999.99 needs 9 chars; field is 6; dec_format '*'-fills -> DBF_OK.
     * Re-read: dec_parse on "******" -> 0.0.
     * Ref: xbase_coercion.json assignment_coercion target:N on_overflow:stars_fill.
     * Ref: rt.h dec_format "Width overflow: fills with '*'". */
    tbl = make_table(pal, MUT_PATH_REPL);
    if (!tbl) return;
    rc = dbf_append_blank(tbl);
    CHECK(rc == DBF_OK, "replace-overflow: append_blank");

    v = xb_n(999999.99);   /* overflow: 9 significant chars > 6-col field */
    rc = dbf_replace(tbl, 1u, 1, &v);
    snprintf(msg, sizeof(msg),
             "replace-overflow: SCORE <- 999999.99 (overflow) rc=%d (want DBF_OK)", rc);
    CHECK(rc == DBF_OK, msg);   /* stars_fill is NOT an error at this layer */

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "replace-overflow: flush");
    dbf_close(tbl);
    tbl = NULL;

    rc = dbf_open(pal, MUT_PATH_REPL, &tbl);
    CHECK(rc == DBF_OK && tbl != NULL, "replace-overflow: re-open");
    if (rc != DBF_OK || !tbl) return;

    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    CHECK(rc == DBF_OK, "replace-overflow: read rec1");
    if (rc == DBF_OK) {
        /* dec_parse on '***...' returns 0.0 (non-digit -> 0.0 per rt.h). */
        CHECK(out[1].t == XB_N && out[1].u.n == 0.0,
              "replace-overflow: SCORE reads back as 0.0 after *-fill");
    }
    dbf_close(tbl);

    /* --- C truncation test: NAME C(8) <- "LongNameHere" (12 chars > 8) --- */
    tbl = make_table(pal, MUT_PATH_REPL);
    if (!tbl) return;
    rc = dbf_append_blank(tbl);
    CHECK(rc == DBF_OK, "replace-truncate: append_blank");

    v = xb_c("LongNameHere", 12u);
    rc = dbf_replace(tbl, 1u, 0, &v);
    snprintf(msg, sizeof(msg), "replace-truncate: NAME <- 'LongNameHere' rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "replace-truncate: flush");
    dbf_close(tbl);
    tbl = NULL;

    rc = dbf_open(pal, MUT_PATH_REPL, &tbl);
    CHECK(rc == DBF_OK && tbl != NULL, "replace-truncate: re-open");
    if (rc != DBF_OK || !tbl) return;

    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    CHECK(rc == DBF_OK, "replace-truncate: read rec1");
    if (rc == DBF_OK) {
        /* NAME must be "LongName" (first 8 of 12); no trailing spaces because
         * the 12-char input fills the whole 8-byte field exactly to truncation. */
        CHECK(out[0].t == XB_C && out[0].u.c.len == 8u &&
              memcmp(out[0].u.c.p, "LongName", 8) == 0,
              "replace-truncate: NAME == 'LongName' (truncated to field_len 8)");
    }
    dbf_close(tbl);
}

/* ============================================================
 * Oracle 3: REPLACE -- cross-type mismatch -> -DBF_ERR_MISMATCH
 *
 * Per xbase_coercion.json assignment_coercion: C<-N, N<-C, D<-C, L<-N are
 * all errors (result:"error", error:"mismatch"). dbf_replace must return
 * -DBF_ERR_MISMATCH for each. The table is NOT flushed (no disk writes needed
 * to test the coercion gate). Ref: plan S1.5 "cross-type -> fail-loud".
 * ============================================================ */
static void test_replace_mismatch(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    int rc;
    char msg[256];
    xb_val v;

    tbl = make_table(pal, MUT_PATH_REPL);
    if (!tbl) return;
    rc = dbf_append_blank(tbl);
    CHECK(rc == DBF_OK, "replace-mismatch: append_blank");

    /* C <- N: mismatch. Ref: xbase_coercion.json target:C, expr:N, error:mismatch. */
    v = xb_n(42.0);
    rc = dbf_replace(tbl, 1u, 0, &v);    /* field 0 is NAME C(8) */
    snprintf(msg, sizeof(msg),
             "replace-mismatch: C <- N -> -DBF_ERR_MISMATCH (got %d)", rc);
    CHECK(rc == -DBF_ERR_MISMATCH, msg);

    /* N <- C: mismatch. Ref: xbase_coercion.json target:N, expr:C, error:mismatch. */
    v = xb_c("42", 2u);
    rc = dbf_replace(tbl, 1u, 1, &v);    /* field 1 is SCORE N(6,2) */
    snprintf(msg, sizeof(msg),
             "replace-mismatch: N <- C -> -DBF_ERR_MISMATCH (got %d)", rc);
    CHECK(rc == -DBF_ERR_MISMATCH, msg);

    /* D <- C: mismatch. Ref: xbase_coercion.json target:D, expr:C, error:mismatch. */
    v = xb_c("19990101", 8u);
    rc = dbf_replace(tbl, 1u, 2, &v);    /* field 2 is JOINED D(8) */
    snprintf(msg, sizeof(msg),
             "replace-mismatch: D <- C -> -DBF_ERR_MISMATCH (got %d)", rc);
    CHECK(rc == -DBF_ERR_MISMATCH, msg);

    /* L <- N: mismatch. Ref: xbase_coercion.json target:L, no expr:N row -> mismatch. */
    v = xb_n(1.0);
    rc = dbf_replace(tbl, 1u, 3, &v);    /* field 3 is ACTIVE L(1) */
    snprintf(msg, sizeof(msg),
             "replace-mismatch: L <- N -> -DBF_ERR_MISMATCH (got %d)", rc);
    CHECK(rc == -DBF_ERR_MISMATCH, msg);

    dbf_close(tbl);
}

/* ============================================================
 * Oracle 4: DELETE / RECALL
 *
 * Append 2 records, delete record 1 (flag -> 0x2A), flush, re-open.
 * Assert: dbf_read_rec rec1 -> deleted=1.
 * Then recall record 1 (flag -> 0x20), flush, re-open.
 * Assert: dbf_read_rec rec1 -> deleted=0.
 *
 * This oracle bites hardest with -DDBF_MUTATE_DELFLAG: dbf_delete writes 0x20
 * instead of 0x2A, so the deleted=1 check fails -> RED.
 * ============================================================ */
static void test_delete_recall(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    int rc;
    char msg[256];
    xb_val out[4];
    int deleted;

    tbl = make_table(pal, MUT_PATH_DELFLAG);
    if (!tbl) return;

    append_rec(tbl, "Alpha", 10.0, jdn_from_ymd(1985, 1, 1), 1, 0);
    append_rec(tbl, "Beta",  20.0, jdn_from_ymd(1990, 6, 15), 0, 0);

    /* Delete record 1. */
    rc = dbf_delete(tbl, 1u);
    snprintf(msg, sizeof(msg), "delete-recall: dbf_delete(1) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "delete-recall: flush after delete");
    dbf_close(tbl);
    tbl = NULL;

    /* Re-open and check the delete flag on record 1. */
    rc = dbf_open(pal, MUT_PATH_DELFLAG, &tbl);
    CHECK(rc == DBF_OK && tbl != NULL, "delete-recall: re-open after delete");
    if (rc != DBF_OK || !tbl) return;

    deleted = -1;
    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    CHECK(rc == DBF_OK, "delete-recall: read rec1 after delete");
    if (rc == DBF_OK) {
        /* THIS check is the one DBF_MUTATE_DELFLAG breaks (writes 0x20 -> 0). */
        snprintf(msg, sizeof(msg),
                 "delete-recall: rec1 deleted==1 (flag 0x2A) "
                 "[DBF_MUTATE_DELFLAG -> deleted==0 -> RED]");
        CHECK(deleted == 1, msg);
    }

    /* Record 2 must still be live. */
    deleted = -1;
    rc = dbf_read_rec(tbl, 2u, out, &deleted);
    CHECK(rc == DBF_OK && deleted == 0,
          "delete-recall: rec2 still LIVE after delete(1)");

    dbf_close(tbl);
    tbl = NULL;

    /* Now RECALL record 1. We need a writable table; re-create (use same path). */
    tbl = make_table(pal, MUT_PATH_DELFLAG);
    if (!tbl) return;
    append_rec(tbl, "Alpha", 10.0, jdn_from_ymd(1985, 1, 1), 1, 0);
    append_rec(tbl, "Beta",  20.0, jdn_from_ymd(1990, 6, 15), 0, 0);
    rc = dbf_delete(tbl, 1u);
    CHECK(rc == DBF_OK, "delete-recall: delete before recall setup");
    rc = dbf_recall(tbl, 1u);
    snprintf(msg, sizeof(msg), "delete-recall: dbf_recall(1) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "delete-recall: flush after recall");
    dbf_close(tbl);
    tbl = NULL;

    rc = dbf_open(pal, MUT_PATH_DELFLAG, &tbl);
    CHECK(rc == DBF_OK && tbl != NULL, "delete-recall: re-open after recall");
    if (rc != DBF_OK || !tbl) return;

    deleted = -1;
    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    CHECK(rc == DBF_OK, "delete-recall: read rec1 after recall");
    if (rc == DBF_OK) {
        CHECK(deleted == 0,
              "delete-recall: rec1 LIVE after recall (flag 0x20)");
        /* Also verify name value is intact after delete+recall cycle. */
        CHECK(out[0].t == XB_C && memcmp(out[0].u.c.p, "Alpha   ", 8) == 0,
              "delete-recall: rec1 NAME intact after recall");
    }

    dbf_close(tbl);
}

/* ============================================================
 * Oracle 5: PACK
 *
 * Append 4 records. Delete records 1 and 3. Pack.
 * Assert: nrec=2, survivors are original records 2 and 4 (in that order),
 * values intact. dbf_open on the flushed file confirms nrec and values.
 *
 * With -DDBF_MUTATE_DELFLAG: dbf_delete leaves all records live, so pack
 * keeps all 4 -> nrec==4 != 2 -> RED.
 * ============================================================ */
static void test_pack(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    int rc;
    char msg[256];
    int32_t jdn1 = jdn_from_ymd(1980, 1,  1);
    int32_t jdn2 = jdn_from_ymd(1985, 6,  15);
    int32_t jdn3 = jdn_from_ymd(1990, 12, 31);
    int32_t jdn4 = jdn_from_ymd(1999, 3,  7);
    xb_val out[4];
    int deleted;

    tbl = make_table(pal, MUT_PATH_PACK);
    if (!tbl) return;

    append_rec(tbl, "R1",   1.0, jdn1, 1, 0);   /* record 1: to be deleted */
    append_rec(tbl, "R2",   2.0, jdn2, 0, 0);   /* record 2: survivor */
    append_rec(tbl, "R3",   3.0, jdn3, 1, 0);   /* record 3: to be deleted */
    append_rec(tbl, "R4",   4.0, jdn4, 0, 0);   /* record 4: survivor */

    CHECK(dbf_nrec(tbl) == 4u, "pack: nrec==4 before delete");

    rc = dbf_delete(tbl, 1u);
    CHECK(rc == DBF_OK, "pack: delete rec1");
    rc = dbf_delete(tbl, 3u);
    CHECK(rc == DBF_OK, "pack: delete rec3");

    rc = dbf_pack(tbl);
    snprintf(msg, sizeof(msg), "pack: dbf_pack rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    snprintf(msg, sizeof(msg),
             "pack: nrec==2 after pack (got %u) "
             "[DBF_MUTATE_DELFLAG -> 4 -> RED]",
             dbf_nrec(tbl));
    CHECK(dbf_nrec(tbl) == 2u, msg);

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "pack: flush after pack");
    dbf_close(tbl);
    tbl = NULL;

    /* Re-open and verify survivors. */
    rc = dbf_open(pal, MUT_PATH_PACK, &tbl);
    CHECK(rc == DBF_OK && tbl != NULL, "pack: re-open packed file");
    if (rc != DBF_OK || !tbl) return;

    snprintf(msg, sizeof(msg),
             "pack: nrec==2 in re-opened file (got %u)", dbf_nrec(tbl));
    CHECK(dbf_nrec(tbl) == 2u, msg);

    /* Survivor 1 (was record 2): NAME=="R2", SCORE==2.0, JOINED==jdn2, ACTIVE==F. */
    deleted = -1;
    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    CHECK(rc == DBF_OK, "pack: read survivor1 (was R2)");
    if (rc == DBF_OK) {
        CHECK(deleted == 0, "pack: survivor1 is LIVE");
        CHECK(out[0].t == XB_C && memcmp(out[0].u.c.p, "R2      ", 8) == 0,
              "pack: survivor1 NAME=='R2      '");
        CHECK(out[1].t == XB_N && out[1].u.n == 2.0,
              "pack: survivor1 SCORE==2.0");
        CHECK(out[2].t == XB_D && out[2].u.d == (double)jdn2,
              "pack: survivor1 JOINED==JDN(1985-06-15)");
        CHECK(out[3].t == XB_L && out[3].u.l == 0u,
              "pack: survivor1 ACTIVE==F");
    }

    /* Survivor 2 (was record 4): NAME=="R4", SCORE==4.0, JOINED==jdn4, ACTIVE==F. */
    deleted = -1;
    rc = dbf_read_rec(tbl, 2u, out, &deleted);
    CHECK(rc == DBF_OK, "pack: read survivor2 (was R4)");
    if (rc == DBF_OK) {
        CHECK(deleted == 0, "pack: survivor2 is LIVE");
        CHECK(out[0].t == XB_C && memcmp(out[0].u.c.p, "R4      ", 8) == 0,
              "pack: survivor2 NAME=='R4      '");
        CHECK(out[1].t == XB_N && out[1].u.n == 4.0,
              "pack: survivor2 SCORE==4.0");
        CHECK(out[2].t == XB_D && out[2].u.d == (double)jdn4,
              "pack: survivor2 JOINED==JDN(1999-03-07)");
        CHECK(out[3].t == XB_L && out[3].u.l == 0u,
              "pack: survivor2 ACTIVE==F");
    }

    /* No record 3 exists (only 2 after pack). */
    rc = dbf_read_rec(tbl, 3u, out, &deleted);
    snprintf(msg, sizeof(msg),
             "pack: read rec3 -> -DBF_ERR_BAD_RECNO (got %d)", rc);
    CHECK(rc == -DBF_ERR_BAD_RECNO, msg);

    /* Leave this file at MUT_PATH_REF for the independence barrier (oracle 8). */
    dbf_close(tbl);
    tbl = NULL;

    /* Copy packed file to the reference path for dbf_ref.py.
     * Read raw bytes and write to MUT_PATH_REF. */
    {
        uint8_t *buf = NULL;
        long len = 0;
        buf = slurp(MUT_PATH_PACK, &len);
        if (buf && len > 0) {
            FILE *out_f = fopen(MUT_PATH_REF, "wb");
            if (out_f) {
                fwrite(buf, 1, (size_t)len, out_f);
                fclose(out_f);
            }
            free(buf);
        }
    }
}

/* ============================================================
 * Oracle 6: ZAP
 *
 * Create a table, append 3 records, zap, flush, re-open.
 * Assert: nrec=0; schema intact (nfields, record_length, version);
 * dbf_read_rec(1) -> -DBF_ERR_BAD_RECNO.
 * ============================================================ */
static void test_zap(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    int rc;
    char msg[256];
    xb_val out[4];
    int deleted;

    tbl = make_table(pal, MUT_PATH_ZAP);
    if (!tbl) return;

    append_rec(tbl, "X1", 1.0, jdn_from_ymd(1990, 1, 1), 1, 0);
    append_rec(tbl, "X2", 2.0, jdn_from_ymd(1991, 1, 1), 0, 0);
    append_rec(tbl, "X3", 3.0, jdn_from_ymd(1992, 1, 1), 1, 0);

    CHECK(dbf_nrec(tbl) == 3u, "zap: nrec==3 before zap");

    rc = dbf_zap(tbl);
    snprintf(msg, sizeof(msg), "zap: dbf_zap rc=%d", rc);
    CHECK(rc == DBF_OK, msg);

    snprintf(msg, sizeof(msg),
             "zap: nrec==0 immediately after zap (got %u)", dbf_nrec(tbl));
    CHECK(dbf_nrec(tbl) == 0u, msg);

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "zap: flush after zap");
    dbf_close(tbl);
    tbl = NULL;

    rc = dbf_open(pal, MUT_PATH_ZAP, &tbl);
    CHECK(rc == DBF_OK && tbl != NULL, "zap: re-open after zap");
    if (rc != DBF_OK || !tbl) return;

    snprintf(msg, sizeof(msg),
             "zap: nrec==0 in re-opened file (got %u)", dbf_nrec(tbl));
    CHECK(dbf_nrec(tbl) == 0u, msg);

    /* Schema must be intact. */
    CHECK(dbf_nfields(tbl) == (uint16_t)G_NFIELDS,
          "zap: nfields intact after zap");
    CHECK(dbf_record_length(tbl) == (uint16_t)(1 + 8 + 6 + 8 + 1),
          "zap: record_length intact after zap (==24)");
    CHECK(dbf_version(tbl) == 0x03u,
          "zap: version intact after zap (0x03 no-memo)");

    /* Trying to read any record must fail. */
    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    snprintf(msg, sizeof(msg),
             "zap: read rec1 -> -DBF_ERR_BAD_RECNO (got %d)", rc);
    CHECK(rc == -DBF_ERR_BAD_RECNO, msg);

    dbf_close(tbl);
}

/* ============================================================
 * Oracle 7: DETERMINISM (Rule 11)
 *
 * Two identical mutation sequences on separate arenas must produce
 * byte-identical output after flush. Covers append_blank + replace + flush.
 * Ref: CLAUDE.md Rule 11 (deterministic); plan S1.5.
 * ============================================================ */
static void build_det_mutate(samir_pal_t *pal, const char *path)
{
    dbf_table *tbl = NULL;
    xb_val v;
    int rc;

    rc = dbf_create(pal, path, g_schema, G_NFIELDS, &tbl);
    if (rc != DBF_OK || !tbl) return;

    /* Append blank then replace fields -- deterministic sequence. */
    dbf_append_blank(tbl);
    v = xb_c("Bob", 3u);
    dbf_replace(tbl, 1u, 0, &v);
    v = xb_n(-3.14);
    dbf_replace(tbl, 1u, 1, &v);
    v = xb_d((double)jdn_from_ymd(2000, 1, 1));
    dbf_replace(tbl, 1u, 2, &v);
    v = xb_l(0);
    dbf_replace(tbl, 1u, 3, &v);

    /* Append a second blank record. */
    dbf_append_blank(tbl);

    /* Delete first record, recall it, delete again (round-trip to 0x2A). */
    dbf_delete(tbl, 1u);
    dbf_recall(tbl, 1u);
    dbf_delete(tbl, 1u);

    dbf_flush(tbl);
    dbf_close(tbl);
}

static void test_determinism(void)
{
    samir_pal_t *pal_a, *pal_b;
    uint8_t *a = NULL, *b = NULL;
    long la = 0, lb = 0;
    char msg[256];

    /* Two independent PAL arenas with the same injected date. */
    {
        struct pal_host_cfg cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.date_yy   = MUT_YY;
        cfg.date_mm   = MUT_MM;
        cfg.date_dd   = MUT_DD;
        cfg.heap_size = 256u * 1024u;
        pal_a = pal_host_make(cfg);
        pal_b = pal_host_make(cfg);
    }
    CHECK(pal_a && pal_b, "determinism: pal_host_make (two arenas)");
    if (!pal_a || !pal_b) {
        if (pal_a) pal_host_free(pal_a);
        if (pal_b) pal_host_free(pal_b);
        return;
    }

    build_det_mutate(pal_a, MUT_PATH_DET_A);
    build_det_mutate(pal_b, MUT_PATH_DET_B);

    a = slurp(MUT_PATH_DET_A, &la);
    b = slurp(MUT_PATH_DET_B, &lb);
    CHECK(a != NULL && b != NULL, "determinism: both files readable");
    if (a && b) {
        snprintf(msg, sizeof(msg),
                 "determinism: sizes equal (a=%ld b=%ld)", la, lb);
        CHECK(la == lb, msg);
        if (la == lb) {
            snprintf(msg, sizeof(msg),
                     "determinism: byte-identical (%ld bytes)", la);
            CHECK(memcmp(a, b, (size_t)la) == 0, msg);
        }
    }

    free(a); free(b);
    pal_host_free(pal_a);
    pal_host_free(pal_b);
}

/* ============================================================
 * Oracle 8: INDEPENDENCE BARRIER -- dbf_ref.py read-back
 *
 * Oracle 5 (pack) wrote the packed file to MUT_PATH_REF. Shell out to
 * dbf_ref.py --records and gate on exit 0. Loud-skip if python3 absent.
 * ============================================================ */
static void test_independence_barrier(void)
{
    char cmd[1024];
    int ret;

    if (!file_exists(MUT_PATH_REF)) {
        fprintf(stderr,
                "  SKIP (LOUD): no packed file at %s "
                "(pack oracle did not run)\n", MUT_PATH_REF);
        return;
    }

    snprintf(cmd, sizeof(cmd),
             "python3 harness/diff/dbf_diff/dbf_ref.py --records %s "
             ">/dev/null 2>&1", MUT_PATH_REF);
    ret = system(cmd);

    if (ret == -1) {
        fprintf(stderr,
                "  SKIP (LOUD): could not invoke python3 for dbf_ref.py; "
                "orchestrator: run\n"
                "    python3 harness/diff/dbf_diff/dbf_ref.py --records %s\n",
                MUT_PATH_REF);
        return;
    }

#if defined(WIFEXITED)
    if (WIFEXITED(ret)) {
        int code = WEXITSTATUS(ret);
        if (code == 127) {
            fprintf(stderr,
                    "  SKIP (LOUD): python3/dbf_ref.py not found (exit 127); "
                    "orchestrator: run\n"
                    "    python3 harness/diff/dbf_diff/dbf_ref.py --records %s\n",
                    MUT_PATH_REF);
            return;
        }
        CHECK(code == 0,
              "independence: dbf_ref.py --records read packed SAMIR file (exit 0)");
        return;
    }
#endif
    CHECK(ret == 0,
          "independence: dbf_ref.py --records read packed SAMIR file (exit 0)");
}

/* ============================================================
 * main
 * ============================================================ */
int main(int argc, char **argv)
{
    samir_pal_t *pal;

    /* `base` arg (dbase3-decomp path) is accepted but not needed for Tier-0. */
    (void)argc; (void)argv;

    pal = make_pal();
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* Oracle 1: blank record append. */
    test_append_blank(pal);

    /* Oracle 2: replace -- coercion success (C truncate/pad, N round-trip +
     * overflow, D, L). */
    test_replace_ok(pal);

    /* Oracle 3: replace -- cross-type mismatch -> -DBF_ERR_MISMATCH. */
    test_replace_mismatch(pal);

    /* Oracle 4: delete (0x2A) + recall (0x20). Mutant bites here. */
    test_delete_recall(pal);

    /* Oracle 5: pack -- survivors intact, nrec correct, deleted gone. */
    test_pack(pal);

    /* Oracle 6: zap -- nrec=0, schema intact. */
    test_zap(pal);

    /* Oracle 7: determinism -- two sequences produce byte-identical output. */
    test_determinism();

    /* Oracle 8: independence barrier via dbf_ref.py. */
    test_independence_barrier();

    pal_host_free(pal);
    return TEST_SUMMARY("test-dbf-mutate");
}
