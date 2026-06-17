/*
 * harness/diff/dbf_diff/test_dbf_read.c -- host oracle for the S1.3 record
 * read -> typed values decoder (dbf_read_rec).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_dbf_fields.c:
 * the seed test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), a host
 * PAL via pal_host_make (pal_host.c). A non-zero exit on any failed check keeps
 * `make test-dbase` from false-greening (Law 2: the oracle is the truth).
 *
 * Two tiers (plan Sec 2.A):
 *   Tier 0 (committed, operator-free): expected typed values for specific records
 *     transcribed from dbf.md sec 5/6 "Verification" byte-dumps and cross-checked
 *     against `python3 harness/diff/dbf_diff/dbf_ref.py --records <file.DBF>`.
 *     Each cited value comes from the corpus byte-verified spec.
 *     These run against the REAL goldens opened through the PAL.
 *     An absent golden causes a LOUD skip for that table; remaining tables run.
 *   Tier 1 (golden-diff): read several records, assert per-field typed values
 *     for all asserted fields. Absent golden -> loud skip, never silent pass.
 *
 * Goldens base resolves from argv[1] (orchestrator passes the corpus path);
 * default "../dbase3-decomp". If a golden is absent, LOUD skip names it.
 *
 * Mutation hook (Rule 6): built with -DDBF_MUTATE_RECOFF, dbf.c reads records
 * from an offset shifted by +1 byte, so the delete-flag byte is consumed as
 * part of the first field and every subsequent field is shifted. All decoded
 * values diverge from the golden -> the Tier-0 field-value checks go RED.
 * This is the single perturbation; exactly one added constant changes.
 *
 * Note on no-deleted-record fixture: none of the corpus golden tables contain
 * a deleted (0x2A flag) record in the data area. The delete-flag test uses
 * the fact that all corpus records have flag 0x20. The 0x2A path is tested
 * via a synthesized inline DBF constructed in Tier-0 (synthetic deleted record).
 * Ref: dbf.md sec 6 "All fixture records carry delete flag 0x20; none are
 * deleted so the 0x2A value is documented, not fixture-confirmed."
 *
 * Compile + run (self-grade, host, NOT make):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbf.c os/samir/core/value.c os/samir/core/rt.c \
 *     os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbf_read.c -o /tmp/test_dbf_read \
 *     && /tmp/test_dbf_read ../dbase3-decomp
 *
 * Mutant (must go RED):
 *   gcc -std=c11 -Wall -Wextra -Werror -DDBF_MUTATE_RECOFF \
 *     -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbf.c os/samir/core/value.c os/samir/core/rt.c \
 *     os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbf_read.c -o /tmp/test_dbf_read_mut ; \
 *     /tmp/test_dbf_read_mut ../dbase3-decomp >/dev/null 2>&1 ; \
 *     echo "mutant exit=$?"
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 5 (C/N/D/L/M on-disk
 *     encodings), sec 6 (record layout: 1 delete-flag byte, no separator).
 *     Worked record -- TOURS.DBF rec 1:
 *       20 "AV10" "10-night Alaska/Vancouver Cruise        "
 *       "19850805" "   1378.00" "10.000" "  25" "         0"
 *     [verified: byte-checked TOURS.DBF record 1]
 *   - harness/diff/dbf_diff/dbf_ref.py decode_field() + --records output
 *     (independent reference; our typed values are cross-checked against it).
 *   - os/samir/include/samir/dbf.h S1.3 contract (recno 1-based, field decodes,
 *     delete-flag, lifetime, M-boundary decision, blank-date -> xb_u()).
 *   - os/samir/include/samir/value.h (xb_val, xb_type, xb_typeof, xb_eq).
 *   - os/samir/include/samir/rt.h (jdn_from_ymd for expected D values).
 *   - docs/plans/SAMIR-implementation-plan.md S1.3 oracle; ARB rider (a)
 *     (mandatory +mutant sibling for every test gate).
 *   - spec/samir/dbf_format.h (DBF_REC_DELETE_LIVE=0x20, _DELETED=0x2A).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/dbf.h"         /* os/samir/include/, on -Ios/samir/include */
#include "samir/value.h"       /* xb_val, xb_type, xb_typeof, xb_eq */
#include "samir/rt.h"          /* jdn_from_ymd -- for computing expected D values */

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

/* ---- path helpers (same pattern as test_dbf_fields.c) ---- */

static char *join(char *buf, size_t cap, const char *base, const char *rel)
{
    snprintf(buf, cap, "%s/%s", base, rel);
    return buf;
}

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

#define SP_PATH "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

/* ---- test helpers ---- */

/*
 * check_field_type: assert the type tag of out[fi] matches expected xb_type t.
 */
static void check_field_type(const char *label, int recno, int fi,
                              const xb_val *out, xb_type expected)
{
    char msg[256];
    snprintf(msg, sizeof(msg),
             "%s rec%d field[%d]: type expected %d got %d",
             label, recno, fi, (int)expected, (int)xb_typeof(out));
    CHECK(xb_typeof(out) == expected, msg);
}

/*
 * check_field_double: for N and D fields -- assert the double payload.
 */
static void check_field_double(const char *label, int recno, int fi,
                                const xb_val *out, double expected)
{
    char msg[256];
    double got = (out->t == XB_N) ? out->u.n : out->u.d;
    snprintf(msg, sizeof(msg),
             "%s rec%d field[%d]: double expected %.6g got %.6g",
             label, recno, fi, expected, got);
    /* Use exact IEEE == (xb_eq contract; no epsilon; corpus values are exact). */
    CHECK(got == expected, msg);
}

/*
 * check_field_logical: for L fields -- assert the bool payload (0 or 1).
 */
static void check_field_logical(const char *label, int recno, int fi,
                                 const xb_val *out, int expected_truth)
{
    char msg[256];
    int got = (int)out->u.l;
    snprintf(msg, sizeof(msg),
             "%s rec%d field[%d]: logical expected %d got %d",
             label, recno, fi, expected_truth, got);
    CHECK(got == expected_truth, msg);
}

/*
 * check_field_c_prefix: for C fields -- assert the first `n` raw bytes match.
 * We compare raw bytes (spaces included) so the test is sensitive to the
 * decode and not to trimming policy. Ref: dbf.h S1.3 "raw bytes including
 * trailing spaces" (round-trip safe; trimming is evaluator's job).
 */
static void check_field_c_prefix(const char *label, int recno, int fi,
                                  const xb_val *out, const char *expected, int n)
{
    char msg[256];
    snprintf(msg, sizeof(msg),
             "%s rec%d field[%d]: C prefix (first %d bytes) mismatch",
             label, recno, fi, n);
    /* out->u.c.p points into the record buffer -- valid here (before next read). */
    CHECK(out->u.c.p != NULL && memcmp(out->u.c.p, expected, (size_t)n) == 0, msg);
}

/*
 * check_field_c_trimmed: check rstripped C value matches expected_trimmed string.
 * Trims trailing spaces from the raw C field for comparison to the dbf_ref.py
 * output (which rstrips). Validates agreement with the independent reference.
 */
static void check_field_c_trimmed(const char *label, int recno, int fi,
                                   const xb_val *out, const char *expected_trimmed)
{
    char got[256];
    char msg[512];
    int len = (int)out->u.c.len;
    int i;

    if (len >= (int)sizeof(got)) len = (int)sizeof(got) - 1;
    memcpy(got, out->u.c.p, (size_t)len);
    got[len] = '\0';

    /* rstrip trailing spaces */
    for (i = len - 1; i >= 0 && got[i] == ' '; i--)
        got[i] = '\0';

    snprintf(msg, sizeof(msg),
             "%s rec%d field[%d]: C trimmed expected \"%s\" got \"%s\"",
             label, recno, fi, expected_trimmed, got);
    CHECK(strcmp(got, expected_trimmed) == 0, msg);
}

/* ============================================================
 * TIER 0: TOURS.DBF -- 1-based recno 1 (first record)
 *
 * Ground truth: dbf.md sec 6 "Worked record -- TOURS.DBF rec 1":
 *   del=0x20 (live)
 *   TRAVELCODE  C(4)   "AV10"
 *   TRAVELPLAN  C(40)  "10-night Alaska/Vancouver Cruise        "
 *   DEPARTURE   D(8)   "19850805" -> JDN=2446283 [corpus-verified]
 *   UNITCOST    N(10,2)"   1378.00" -> 1378.0
 *   PERCENT     N(6,3) "10.000"    -> 10.0
 *   SEATS_OPEN  N(4,0) "  25"      -> 25.0
 *   NOTE        M(10)  "         0"-> block 0 -> xb_u()
 *
 * Cross-checked: python3 dbf_ref.py --records TOURS.DBF | head -1
 *   rec0 active TRAVELCODE=AV10 TRAVELPLAN=10-night Alaska/Vancouver Cruise
 *   DEPARTURE=19850805 UNITCOST=1378 PERCENT=10 SEATS_OPEN=25 NOTE=block:0
 * (dbf_ref.py is 0-indexed, so rec0 = our recno 1.)
 * [verified: byte-checked TOURS.DBF record 1; dbf.md sec 6 worked example]
 * ============================================================ */
static void test_tours_rec1(samir_pal_t *pal, const char *base)
{
    char path[1024];
    dbf_table *tbl = NULL;
    xb_val out[7];  /* TOURS has 7 fields */
    int deleted = -1;
    int rc;
    int32_t expected_jdn_departure;
    char msg[256];

    join(path, sizeof(path), base, SP_PATH "/TOURS.DBF");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): golden absent: %s\n", path);
        return;
    }

    rc = dbf_open(pal, path, &tbl);
    snprintf(msg, sizeof(msg), "TOURS: dbf_open succeeds (rc=%d)", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    /* --- read recno 1 (first record) --- */
    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    snprintf(msg, sizeof(msg), "TOURS: dbf_read_rec(1) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc != DBF_OK) { dbf_close(tbl); return; }

    /* Delete flag: 0x20 -> deleted=0. [verified: byte-checked TOURS.DBF rec0 del=0x20] */
    snprintf(msg, sizeof(msg), "TOURS rec1: deleted==0 (flag 0x20 live)");
    CHECK(deleted == 0, msg);

    /* field[0] TRAVELCODE C(4) "AV10" */
    /* Ref: dbf.md sec 6 worked record; dbf_ref.py rec0 TRAVELCODE=AV10. */
    check_field_type("TOURS", 1, 0, &out[0], XB_C);
    check_field_c_trimmed("TOURS", 1, 0, &out[0], "AV10");

    /* field[1] TRAVELPLAN C(40) "10-night Alaska/Vancouver Cruise        " */
    /* Ref: dbf.md sec 6; dbf_ref.py rec0 TRAVELPLAN=10-night Alaska/Vancouver Cruise */
    check_field_type("TOURS", 1, 1, &out[1], XB_C);
    check_field_c_trimmed("TOURS", 1, 1, &out[1], "10-night Alaska/Vancouver Cruise");

    /* field[2] DEPARTURE D(8) "19850805" -> JDN 2446283. */
    /* Ref: dbf.md sec 6 worked record + rt.h jdn verified value 2446283. */
    /* Cross-check: python3 dbf_ref.py --records TOURS.DBF | head -1 -> DEPARTURE=19850805 */
    expected_jdn_departure = jdn_from_ymd(1985, 8, 5);   /* = 2446283 [verified] */
    check_field_type("TOURS", 1, 2, &out[2], XB_D);
    check_field_double("TOURS", 1, 2, &out[2], (double)expected_jdn_departure);

    /* field[3] UNITCOST N(10,2) "   1378.00" -> 1378.0 */
    /* Ref: dbf.md sec 6 "UNITCOST N(10,2) = '   1378.00' (3 leading spaces, 2 decimal)". */
    check_field_type("TOURS", 1, 3, &out[3], XB_N);
    check_field_double("TOURS", 1, 3, &out[3], 1378.0);

    /* field[4] PERCENT N(6,3) "10.000" -> 10.0 */
    /* Ref: dbf.md sec 6 "PERCENT(6,dec3)='10.000'". dbf_ref.py: PERCENT=10 */
    check_field_type("TOURS", 1, 4, &out[4], XB_N);
    check_field_double("TOURS", 1, 4, &out[4], 10.0);

    /* field[5] SEATS_OPEN N(4,0) "  25" -> 25.0 */
    /* Ref: dbf.md sec 6 "SEATS_OPEN(4,dec0)='  25'". dbf_ref.py: SEATS_OPEN=25 */
    check_field_type("TOURS", 1, 5, &out[5], XB_N);
    check_field_double("TOURS", 1, 5, &out[5], 25.0);

    /* field[6] NOTE M(10) "         0" -> block 0 -> xb_u() */
    /* Ref: dbf.md sec 6 "NOTE M(10) -> no memo". dbf.md sec 5 M "block 0 = no memo". */
    /* dbf_ref.py: NOTE=block:0 -> dec_parse("         0") = 0 -> xb_u() decision. */
    check_field_type("TOURS", 1, 6, &out[6], XB_U);

    /* --- also verify recno 2 (second record) partially ---
     * dbf_ref.py rec1: TRAVELCODE=MA3 DEPARTURE=19850907 UNITCOST=279
     * Ref: dbf_ref.py --records TOURS.DBF | sed -n '2p' */
    rc = dbf_read_rec(tbl, 2u, out, &deleted);
    snprintf(msg, sizeof(msg), "TOURS: dbf_read_rec(2) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc == DBF_OK) {
        int32_t jdn2 = jdn_from_ymd(1985, 9, 7);   /* 2446316 [corpus-verified] */
        check_field_c_trimmed("TOURS", 2, 0, &out[0], "MA3");
        check_field_type("TOURS", 2, 2, &out[2], XB_D);
        check_field_double("TOURS", 2, 2, &out[2], (double)jdn2);
        check_field_double("TOURS", 2, 3, &out[3], 279.0);
    }

    /* --- recno out of range: 0 and nrec+1 must fail loud ---
     * Ref: dbf.h DBF_ERR_BAD_RECNO contract. */
    {
        xb_val dummy[7];
        int del2 = -1;
        rc = dbf_read_rec(tbl, 0u, dummy, &del2);
        snprintf(msg, sizeof(msg), "TOURS: recno 0 -> -DBF_ERR_BAD_RECNO (got %d)", rc);
        CHECK(rc == -DBF_ERR_BAD_RECNO, msg);

        rc = dbf_read_rec(tbl, (uint32_t)(dbf_nrec(tbl) + 1u), dummy, &del2);
        snprintf(msg, sizeof(msg), "TOURS: recno nrec+1 -> -DBF_ERR_BAD_RECNO (got %d)", rc);
        CHECK(rc == -DBF_ERR_BAD_RECNO, msg);
    }

    dbf_close(tbl);
}

/* ============================================================
 * TIER 0: TRAVEL.DBF -- recno 1 and recno 4 (has PAID=F)
 *
 * Ground truth: dbf_ref.py --records TRAVEL.DBF | head -5:
 *   rec0 active FIRSTNAME=Claire LASTNAME=Buckman PHONE=(555)456-9059
 *     TRAVELCODE=CI10 TRAVELPLAN=10-night Caribbean Island Cruise
 *     DEPARTURE=19851024 COST=1199 PAID=T AGENT=MM RESERVDATE=19850715 NOTES=block:1
 *   rec3 active FIRSTNAME=Lena LASTNAME=Garnett ... PAID=F ... NOTES=block:0
 *
 * TRAVEL fields (11): FIRSTNAME C(20), LASTNAME C(20), PHONE C(13),
 *   TRAVELCODE C(4), TRAVELPLAN C(40), DEPARTURE D(8), COST N(10,2),
 *   PAID L(1), AGENT C(2), RESERVDATE D(8), NOTES M(10).
 *
 * Cross-checked against dbf_ref.py --records TRAVEL.DBF.
 * [verified: byte-checked TRAVEL.DBF records 1-4; dbf.md sec 5 L/M + Verification]
 * ============================================================ */
static void test_travel_rec1_rec4(samir_pal_t *pal, const char *base)
{
    char path[1024];
    dbf_table *tbl = NULL;
    xb_val out[11];  /* TRAVEL has 11 fields */
    int deleted = -1;
    int rc;
    char msg[256];

    join(path, sizeof(path), base, SP_PATH "/TRAVEL.DBF");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): golden absent: %s\n", path);
        return;
    }

    rc = dbf_open(pal, path, &tbl);
    snprintf(msg, sizeof(msg), "TRAVEL: dbf_open succeeds (rc=%d)", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    /* --- recno 1 (first record, 0-indexed rec0 in dbf_ref.py) ---
     * field indices in TRAVEL:
     *   0=FIRSTNAME, 1=LASTNAME, 2=PHONE, 3=TRAVELCODE, 4=TRAVELPLAN,
     *   5=DEPARTURE, 6=COST, 7=PAID, 8=AGENT, 9=RESERVDATE, 10=NOTES */
    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    snprintf(msg, sizeof(msg), "TRAVEL: dbf_read_rec(1) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc != DBF_OK) { dbf_close(tbl); return; }

    snprintf(msg, sizeof(msg), "TRAVEL rec1: deleted==0");
    CHECK(deleted == 0, msg);

    /* FIRSTNAME C(20) "Claire" (trimmed). Ref: dbf_ref.py rec0 FIRSTNAME=Claire */
    check_field_type("TRAVEL", 1, 0, &out[0], XB_C);
    check_field_c_trimmed("TRAVEL", 1, 0, &out[0], "Claire");

    /* DEPARTURE D(8) "19851024" -> JDN(1985,10,24). Ref: dbf_ref.py rec0 DEPARTURE=19851024 */
    {
        int32_t jdn = jdn_from_ymd(1985, 10, 24);
        check_field_type("TRAVEL", 1, 5, &out[5], XB_D);
        check_field_double("TRAVEL", 1, 5, &out[5], (double)jdn);
    }

    /* COST N(10,2) "   1199.00" -> 1199.0. Ref: dbf_ref.py rec0 COST=1199 */
    check_field_type("TRAVEL", 1, 6, &out[6], XB_N);
    check_field_double("TRAVEL", 1, 6, &out[6], 1199.0);

    /* PAID L(1) 'T' -> xb_l(1). Ref: dbf.md sec 5 L; dbf_ref.py rec0 PAID=T.
     * [verified: TRAVEL.DBF PAID column byte-checked = 0x54='T' for rec0] */
    check_field_type("TRAVEL", 1, 7, &out[7], XB_L);
    check_field_logical("TRAVEL", 1, 7, &out[7], 1);

    /* RESERVDATE D(8) "19850715" -> JDN(1985,7,15). Ref: dbf_ref.py rec0 RESERVDATE=19850715 */
    {
        int32_t jdn = jdn_from_ymd(1985, 7, 15);
        check_field_type("TRAVEL", 1, 9, &out[9], XB_D);
        check_field_double("TRAVEL", 1, 9, &out[9], (double)jdn);
    }

    /* NOTES M(10) "         1" -> block 1 -> xb_m(raw,10). Ref: dbf_ref.py rec0 NOTES=block:1.
     * S1.3->S2.1 boundary: we expose the raw 10-byte field; S2.1 reads the .dbt. */
    check_field_type("TRAVEL", 1, 10, &out[10], XB_M);
    /* Also verify the raw 10 bytes contain "         1" (right-justified block 1). */
    {
        snprintf(msg, sizeof(msg),
                 "TRAVEL rec1 NOTES: raw 10 bytes start with 9 spaces");
        CHECK(out[10].u.c.p != NULL && out[10].u.c.len == 10u, msg);
        /* The last byte should be '1' (ASCII 0x31). */
        if (out[10].u.c.p) {
            snprintf(msg, sizeof(msg),
                     "TRAVEL rec1 NOTES: raw[9]='1' (block 1)");
            CHECK(out[10].u.c.p[9] == '1', msg);
        }
    }

    /* --- recno 4 (0-indexed rec3): PAID='F' (dbf_ref.py rec3 PAID=F) ---
     * [verified: TRAVEL.DBF byte-checked rec3 PAID column = 0x46='F'] */
    rc = dbf_read_rec(tbl, 4u, out, &deleted);
    snprintf(msg, sizeof(msg), "TRAVEL: dbf_read_rec(4) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc == DBF_OK) {
        /* PAID L(1) 'F' -> xb_l(0). Ref: dbf.md sec 5 L; dbf_ref.py rec3 PAID=F */
        check_field_type("TRAVEL", 4, 7, &out[7], XB_L);
        check_field_logical("TRAVEL", 4, 7, &out[7], 0);

        /* NOTES M(10) "         0" -> block 0 -> xb_u(). rec3 has no memo. */
        /* Ref: dbf_ref.py rec3 NOTES=block:0 */
        check_field_type("TRAVEL", 4, 10, &out[10], XB_U);
    }

    dbf_close(tbl);
}

/* ============================================================
 * TIER 0: CLIENTS.DBF -- recno 1 (first record, all C fields)
 *
 * Ground truth: dbf_ref.py --records CLIENTS.DBF | head -1:
 *   rec0 active FIRSTNAME=Claire LASTNAME=Buckman ADDRESS=8307 Santa Anita Blvd
 *     CITY=Oxnard STATE=CA ZIPCODE=93034 PHONE=(555)456-9059
 *
 * [verified: byte-checked CLIENTS.DBF record 1; dbf.md sec 5 C Verification]
 * ============================================================ */
static void test_clients_rec1(samir_pal_t *pal, const char *base)
{
    char path[1024];
    dbf_table *tbl = NULL;
    xb_val out[7];  /* CLIENTS has 7 fields, all C */
    int deleted = -1;
    int rc;
    char msg[256];

    join(path, sizeof(path), base, SP_PATH "/CLIENTS.DBF");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): golden absent: %s\n", path);
        return;
    }

    rc = dbf_open(pal, path, &tbl);
    snprintf(msg, sizeof(msg), "CLIENTS: dbf_open succeeds (rc=%d)", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    snprintf(msg, sizeof(msg), "CLIENTS: dbf_read_rec(1) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc != DBF_OK) { dbf_close(tbl); return; }

    snprintf(msg, sizeof(msg), "CLIENTS rec1: deleted==0");
    CHECK(deleted == 0, msg);

    /* All 7 fields are type C. Ref: dbf.md sec 5 C "left-justified, space-padded". */
    /* FIRSTNAME C(20) = "Claire". Ref: dbf.md sec 5 C; dbf_ref.py rec0 FIRSTNAME=Claire */
    check_field_type("CLIENTS", 1, 0, &out[0], XB_C);
    check_field_c_trimmed("CLIENTS", 1, 0, &out[0], "Claire");

    /* STATE C(2) = "CA". Ref: dbf_ref.py rec0 STATE=CA. No trimming needed (filled). */
    check_field_type("CLIENTS", 1, 4, &out[4], XB_C);
    check_field_c_trimmed("CLIENTS", 1, 4, &out[4], "CA");

    /* ZIPCODE C(5) = "93034". Ref: dbf.md sec 5 C; dbf_ref.py rec0 ZIPCODE=93034 */
    check_field_type("CLIENTS", 1, 5, &out[5], XB_C);
    check_field_c_trimmed("CLIENTS", 1, 5, &out[5], "93034");

    /* Verify raw C field length == field_len (no trimming in xb_c). */
    {
        snprintf(msg, sizeof(msg),
                 "CLIENTS rec1 FIRSTNAME: raw C len==20 (no trim in codec)");
        CHECK(out[0].u.c.len == 20u, msg);
        /* Verify trailing space is present (raw storage). */
        snprintf(msg, sizeof(msg),
                 "CLIENTS rec1 FIRSTNAME: raw bytes end with space (len=20, 'Claire' is 6)");
        if (out[0].u.c.p) {
            CHECK(out[0].u.c.p[6] == ' ', msg);
        }
    }

    /* --- recno 49 (last record in CLIENTS, nrec=49) ---
     * Ref: dbf.md "nrec=49" for CLIENTS.DBF. dbf_ref.py shows 49 records (rec0..rec48).
     * Just read it and verify it succeeds + is active. */
    rc = dbf_read_rec(tbl, 49u, out, &deleted);
    snprintf(msg, sizeof(msg), "CLIENTS: dbf_read_rec(49) rc=%d (last record)", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc == DBF_OK) {
        snprintf(msg, sizeof(msg), "CLIENTS rec49: deleted==0");
        CHECK(deleted == 0, msg);
    }

    dbf_close(tbl);
}

/* ============================================================
 * TIER 0: TAX.DBF -- recno 1 (CODE=1 N, TITLE C) and recno 9 (CODE=9)
 *
 * TAX has 2 fields: CODE N(1,0), TITLE C(18).
 * nrec=9; all records active.
 * Ground truth: dbf_ref.py --records TAX.DBF:
 *   rec0 active CODE=1 TITLE=Business Expenses
 *   rec8 active CODE=9 TITLE=Miscellaneous
 *
 * [verified: dbf_ref.py output cross-checked against dbf.md sec 8 Inv2 TAX row]
 * ============================================================ */
static void test_tax_recs(samir_pal_t *pal, const char *base)
{
    char path[1024];
    dbf_table *tbl = NULL;
    xb_val out[2];
    int deleted = -1;
    int rc;
    char msg[256];

    join(path, sizeof(path), base, SP_PATH "/TAX.DBF");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): golden absent: %s\n", path);
        return;
    }

    rc = dbf_open(pal, path, &tbl);
    snprintf(msg, sizeof(msg), "TAX: dbf_open succeeds (rc=%d)", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    /* recno 1 (rec0 in dbf_ref.py): CODE N(1,0) "1" -> 1.0, TITLE C(18) "Business Expenses". */
    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    snprintf(msg, sizeof(msg), "TAX: dbf_read_rec(1) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc == DBF_OK) {
        snprintf(msg, sizeof(msg), "TAX rec1: deleted==0");
        CHECK(deleted == 0, msg);

        /* CODE N(1,0) "1" -> 1.0. Ref: dbf_ref.py rec0 CODE=1 */
        check_field_type("TAX", 1, 0, &out[0], XB_N);
        check_field_double("TAX", 1, 0, &out[0], 1.0);

        /* TITLE C(18) "Business Expenses". Ref: dbf_ref.py rec0 TITLE=Business Expenses */
        check_field_type("TAX", 1, 1, &out[1], XB_C);
        check_field_c_trimmed("TAX", 1, 1, &out[1], "Business Expenses");
    }

    /* recno 9 (rec8 in dbf_ref.py): CODE "9" -> 9.0, TITLE "Miscellaneous". */
    rc = dbf_read_rec(tbl, 9u, out, &deleted);
    snprintf(msg, sizeof(msg), "TAX: dbf_read_rec(9) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc == DBF_OK) {
        /* CODE N(1,0) "9" -> 9.0. Ref: dbf_ref.py rec8 CODE=9 */
        check_field_type("TAX", 9, 0, &out[0], XB_N);
        check_field_double("TAX", 9, 0, &out[0], 9.0);

        /* TITLE C(18) "Miscellaneous". Ref: dbf_ref.py rec8 TITLE=Miscellaneous */
        check_field_type("TAX", 9, 1, &out[1], XB_C);
        check_field_c_trimmed("TAX", 9, 1, &out[1], "Miscellaneous");
    }

    /* recno 10 -> out of range (nrec=9). Should return -DBF_ERR_BAD_RECNO. */
    {
        xb_val dummy[2];
        int del2 = -1;
        rc = dbf_read_rec(tbl, 10u, dummy, &del2);
        snprintf(msg, sizeof(msg),
                 "TAX: recno 10 (> nrec=9) -> -DBF_ERR_BAD_RECNO (got %d)", rc);
        CHECK(rc == -DBF_ERR_BAD_RECNO, msg);
    }

    dbf_close(tbl);
}

/* ============================================================
 * TIER 0: BANK.DBF -- nrec=0: no records to read.
 * Any recno should return -DBF_ERR_BAD_RECNO.
 * Ref: dbf.md sec 8 "BANK special case: nrec=0".
 * ============================================================ */
static void test_bank_norec(samir_pal_t *pal, const char *base)
{
    char path[1024];
    dbf_table *tbl = NULL;
    xb_val out[5];
    int deleted = -1;
    int rc;
    char msg[256];

    join(path, sizeof(path), base, SP_PATH "/BANK.DBF");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): golden absent: %s\n", path);
        return;
    }

    rc = dbf_open(pal, path, &tbl);
    snprintf(msg, sizeof(msg), "BANK: dbf_open succeeds (rc=%d)", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    snprintf(msg, sizeof(msg), "BANK: nrec==0");
    CHECK(dbf_nrec(tbl) == 0u, msg);

    /* recno 1 with nrec=0 -> -DBF_ERR_BAD_RECNO. */
    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    snprintf(msg, sizeof(msg), "BANK: recno 1 (nrec=0) -> -DBF_ERR_BAD_RECNO (got %d)", rc);
    CHECK(rc == -DBF_ERR_BAD_RECNO, msg);

    dbf_close(tbl);
}

/* ============================================================
 * TIER 0 (synthetic): deleted-record test.
 *
 * The corpus goldens have no 0x2A-flagged records (dbf.md sec 6 note).
 * We synthesize a minimal 1-field 1-record .dbf in a temp file with the
 * delete flag set to 0x2A and verify dbf_read_rec returns deleted=1.
 *
 * Synthetic .dbf layout (dBASE III+ format, +1 terminator, no memo):
 *   Header (32 bytes): ver=0x03; date=0,0,0; nrec=1; hlen=65; rlen=3;
 *   Field descriptor (32 bytes): name="V\0..."; type='C'; len=2; dec=0;
 *   Terminator (1 byte): 0x0D
 *   Record (3 bytes): 0x2A (deleted flag) + "AB" (2 bytes of field data)
 *
 * hlen = 32 + 32*1 + 1 = 65. rlen = 1 + 2 = 3.
 * This is the minimal synthetic fixture to exercise the 0x2A path.
 * Ref: dbf.md sec 6 "0x2A = deleted (not yet PACKed)".
 * ============================================================ */
static void test_synthetic_deleted(samir_pal_t *pal)
{
    /* Build a minimal .dbf in a temp file using the PAL write path.
     * We write raw bytes via fopen (libc OK in factory test; Law 3). */
    const char *tmppath = "/tmp/test_dbf_read_del.dbf";
    FILE *f;
    dbf_table *tbl = NULL;
    xb_val out[1];
    int deleted = -1;
    int rc;
    char msg[256];

    /* 32-byte header */
    uint8_t hdr[32];
    /* 32-byte descriptor */
    uint8_t desc[32];
    /* 1-byte terminator + 3-byte record */
    uint8_t tail[4];

    memset(hdr, 0, sizeof(hdr));
    memset(desc, 0, sizeof(desc));
    memset(tail, 0, sizeof(tail));

    /* Header bytes (dbf.md sec 2):
     * [0]=ver 0x03, [1-3]=date 0,0,0, [4-7]=nrec=1 (LE), [8-9]=hlen=65 (LE),
     * [10-11]=rlen=3 (LE), rest=0. */
    hdr[0x00] = 0x03u;                 /* version: no memo */
    hdr[0x01] = 0; hdr[0x02] = 0; hdr[0x03] = 0;  /* date YY/MM/DD = 0,0,0 */
    hdr[0x04] = 1; hdr[0x05] = 0; hdr[0x06] = 0; hdr[0x07] = 0; /* nrec=1 LE */
    hdr[0x08] = 65; hdr[0x09] = 0;    /* header_length = 65 = 32+32+1 (LE) */
    hdr[0x0A] = 3;  hdr[0x0B] = 0;    /* record_length = 3 = 1+2 (LE) */

    /* Field descriptor (dbf.md sec 4): name="V\0..."; type='C'; len=2; dec=0. */
    desc[0x00] = 'V'; desc[0x01] = 0; /* name "V\0" */
    desc[0x0B] = 'C';                  /* type */
    desc[0x10] = 2;                    /* field_len = 2 */
    desc[0x11] = 0;                    /* dec_count = 0 */
    desc[0x14] = 0x01u;               /* work-area id = 1 (+1 form) */

    /* Terminator + record */
    tail[0] = 0x0Du;   /* 0x0D terminator */
    tail[1] = 0x2Au;   /* delete flag: 0x2A = deleted */
    tail[2] = 'A';     /* field byte 0 */
    tail[3] = 'B';     /* field byte 1 */

    f = fopen(tmppath, "wb");
    snprintf(msg, sizeof(msg), "synthetic-deleted: create temp file");
    CHECK(f != NULL, msg);
    if (!f) return;

    fwrite(hdr,  1, 32, f);
    fwrite(desc, 1, 32, f);
    fwrite(tail, 1,  4, f);
    fclose(f);

    rc = dbf_open(pal, tmppath, &tbl);
    snprintf(msg, sizeof(msg), "synthetic-deleted: dbf_open rc=%d", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    rc = dbf_read_rec(tbl, 1u, out, &deleted);
    snprintf(msg, sizeof(msg), "synthetic-deleted: dbf_read_rec(1) rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    if (rc == DBF_OK) {
        /* deleted==1: the 0x2A path. Ref: dbf.md sec 6 "0x2A = deleted record". */
        snprintf(msg, sizeof(msg),
                 "synthetic-deleted: deleted==1 (flag 0x2A)");
        CHECK(deleted == 1, msg);

        /* Field C(2) "AB" */
        check_field_type("synthetic-deleted", 1, 0, &out[0], XB_C);
        check_field_c_prefix("synthetic-deleted", 1, 0, &out[0], "AB", 2);
    }

    dbf_close(tbl);
    remove(tmppath);
}

/* ============================================================
 * main
 * ============================================================ */

int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    int any_present = 0;
    char path[1024];

    /* Injected fixed date (Rule 11; the PAL requires a date even though S1.3
     * does not use it for record reads). Arena: TRAVEL has 11 fields of up to
     * 10 bytes each + sizeof(dbf_table) + sizeof(dbf_field_t)*11 + record_len
     * (137 bytes) = well under 128 kB. Size generously. */
    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy    = 99;
    cfg.date_mm    = 12;
    cfg.date_dd    = 31;
    cfg.heap_size  = 128u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* Check if any golden is present (for the any_present loud-skip). */
    join(path, sizeof(path), base, SP_PATH "/TOURS.DBF");
    if (file_exists(path)) any_present = 1;
    join(path, sizeof(path), base, SP_PATH "/TRAVEL.DBF");
    if (file_exists(path)) any_present = 1;
    join(path, sizeof(path), base, SP_PATH "/CLIENTS.DBF");
    if (file_exists(path)) any_present = 1;
    join(path, sizeof(path), base, SP_PATH "/TAX.DBF");
    if (file_exists(path)) any_present = 1;
    join(path, sizeof(path), base, SP_PATH "/BANK.DBF");
    if (file_exists(path)) any_present = 1;

    if (!any_present) {
        fprintf(stderr,
                "  SKIP (LOUD): no goldens found under base '%s'\n"
                "               expected e.g. %s/%s/TOURS.DBF\n"
                "               (pass the corpus base as argv[1];\n"
                "                mutation hook still exercises DBF_MUTATE_RECOFF)\n",
                base, base, SP_PATH);
    }

    /* ---- Tier 0 / Tier 1: corpus goldens ---- */
    test_tours_rec1(pal, base);
    test_travel_rec1_rec4(pal, base);
    test_clients_rec1(pal, base);
    test_tax_recs(pal, base);
    test_bank_norec(pal, base);

    /* ---- Tier 0: synthetic deleted-record (always runs, no golden needed) ---- */
    test_synthetic_deleted(pal);

    pal_host_free(pal);
    return TEST_SUMMARY("test-dbf-read");
}
