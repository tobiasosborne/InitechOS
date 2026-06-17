/*
 * harness/diff/dbf_diff/test_dbf_roundtrip.c -- host oracle for the S1.4
 * deterministic .dbf write + round-trip (dbf_create / dbf_append_rec / dbf_flush).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_dbf_read.c:
 * the seed test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), a host
 * PAL via pal_host_make (pal_host.c). A non-zero exit on any failed check keeps
 * `make test-dbase` from false-greening (Law 2: the oracle is the truth).
 *
 * S1.4 is a DIFFERENTIAL / port-and-verify step (CLAUDE.md TDD shape 2): the
 * writer is graded BIDIRECTIONALLY -- our bytes must read back through our own
 * S1.1-S1.3 reader AND match a hand-computed Tier-0 template AND (where present)
 * a real golden under the dbf_normalization.json mask. The mutation gate proves
 * the golden bites.
 *
 * Oracles (this file):
 *   1. DETERMINISM (Rule 11): create+flush the SAME table twice to two temp
 *      paths and assert the files are byte-identical. Same inputs + same injected
 *      date => identical bytes (no malloc-address / timestamp leakage).
 *   2. TIER-0 hand-computed template: a tiny no-memo schema (one C field, one N
 *      field) flushed empty (nrec=0). The header + both descriptors + terminator
 *      are compared byte-for-byte against a hand-computed expected image, each
 *      byte cited to dbf.md / dbf_normalization.json. This is the operator-free,
 *      day-1 assertion (no external golden).
 *   3. C ROUND-TRIP: re-open the written file with dbf_open + dbf_field +
 *      dbf_read_rec (S1.1-S1.3) and assert the schema (version, nfields, field
 *      names/types/lengths/decs, header_length, record_length, nrec) and the
 *      decoded record values match what was appended -- for a NO-MEMO schema
 *      (all of C/N/D/L) and a MEMO-bearing schema (the 0x83 case).
 *   4. NORMALIZATION-MASKED GOLDEN cmp: read a real golden (CLIENTS.DBF) with the
 *      C reader, re-create+append+flush an identical-schema table with identical
 *      records, then compare the re-written header + descriptors against the
 *      golden with every NORMALIZE byte (dbf_normalization.json) masked to 0 --
 *      the MEANINGFUL bytes must match. Loud-skip if the golden is absent.
 *   5. INDEPENDENCE BARRIER (optional): a SAMIR-written file is left at a known
 *      path so the orchestrator can run dbf_ref.py against it; if python3 is
 *      available this test also shells out (system()) and gates on its exit.
 *
 * Mutation hook (Rule 6): built with -DDBF_MUTATE_VERSION, dbf_flush drops the
 * 0x80 memo bit, so a memo-bearing schema (oracle 3, memo case) reads back with
 * has_memo=false / version 0x03 -> the round-trip check goes RED. The no-memo
 * cases are unaffected -- only the memo schema bites, which is the point.
 *
 * Compile + run (self-grade, host, NOT make):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbf.c os/samir/core/value.c os/samir/core/rt.c \
 *     os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbf_roundtrip.c -o /tmp/test_dbf_roundtrip \
 *     && /tmp/test_dbf_roundtrip ../dbase3-decomp
 *
 * Mutant (must go RED):
 *   gcc -std=c11 -Wall -Wextra -Werror -DDBF_MUTATE_VERSION \
 *     -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbf.c os/samir/core/value.c os/samir/core/rt.c \
 *     os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbf_roundtrip.c -o /tmp/test_dbf_roundtrip_mut ; \
 *     /tmp/test_dbf_roundtrip_mut ../dbase3-decomp >/dev/null 2>&1 ; \
 *     echo "mutant exit=$?"
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11): all
 * temp paths are fixed deterministic names under /tmp.
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 2 (header), sec 3 (version
 *     byte / memo bit), sec 4 (descriptor + +1 terminator + CLIENTS FIRSTNAME
 *     worked example), sec 5 (C/N/D/L/M encodings), sec 6 (record layout),
 *     sec 8 (invariants + optional 0x1A EOF).
 *   - spec/samir/dbf_normalization.json (MEANINGFUL vs NORMALIZE byte map; the
 *     masked-cmp uses it for the golden comparison).
 *   - spec/samir/dbf_format.h (offsets; DBF_VERSION_NO_MEMO=0x03/_WITH_MEMO=0x83).
 *   - os/samir/include/samir/dbf.h S1.4 contract (dbf_field_spec, dbf_create,
 *     dbf_append_rec, dbf_flush; +1 form, injected date, trailing 0x1A).
 *   - os/samir/include/samir/value.h (xb_c/xb_n/xb_d/xb_l/xb_m/xb_u).
 *   - os/samir/include/samir/rt.h (jdn_from_ymd for D values).
 *   - harness/diff/dbf_diff/dbf_ref.py (the independent reader the orchestrator
 *     runs against the SAMIR-written file at the known path below).
 *   - docs/plans/SAMIR-implementation-plan.md S1.4 oracle; ARB rider (a) (+mutant).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined(__unix__) || defined(__APPLE__)
#  include <sys/wait.h>        /* WIFEXITED/WEXITSTATUS for the system() gate */
#endif

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/dbf.h"         /* os/samir/include/, on -Ios/samir/include */
#include "samir/value.h"       /* xb_c/xb_n/xb_d/xb_l/xb_m/xb_u */
#include "samir/rt.h"          /* jdn_from_ymd -- expected D values */

#include "samir/dbf_format.h"  /* DBF_* offset constants, on -Ispec */

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

/* Deterministic temp paths (fixed names; Rule 11). */
#define RT_PATH_A      "/tmp/samir_rt_a.dbf"
#define RT_PATH_B      "/tmp/samir_rt_b.dbf"
#define RT_PATH_TMPL   "/tmp/samir_rt_tmpl.dbf"
#define RT_PATH_MEMO   "/tmp/samir_rt_memo.dbf"
#define RT_PATH_GOLD   "/tmp/samir_rt_gold.dbf"
/* The file LEFT for the orchestrator's dbf_ref.py independence check. */
#define RT_PATH_REF    "/tmp/samir_written.dbf"

/* Fixed injected date for all written tables (Rule 11). 1985-10-30 = the
 * CLIENTS.DBF last-update date (dbf.md sec 2 worked example), so a golden cmp
 * that includes the date bytes would match too. YY = year-1900 = 85. */
#define RT_YY  85
#define RT_MM  10
#define RT_DD  30

/* ---- path helpers ---- */

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

/* Read an entire file into a malloc'd buffer; *len set to size. NULL on error. */
static uint8_t *slurp(const char *path, long *len)
{
    FILE   *f;
    long    sz;
    uint8_t *buf;
    size_t  got;

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

#define SP_PATH "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

/* ============================================================
 * Oracle 1: DETERMINISM (Rule 11)
 *
 * Create+flush the SAME schema + SAME records twice to two temp paths and
 * assert the resulting files are byte-identical. This catches any malloc-
 * address or wall-clock leakage. Each PAL gets its own arena, so the record
 * region lives at a different host address each run -- the bytes must still
 * match (the writer never serializes a pointer).
 * ============================================================ */
static void build_det_table(samir_pal_t *pal, const char *path)
{
    dbf_table *tbl = NULL;
    int rc;
    /* Schema: FNAME C(10), AMOUNT N(8,2), WHEN D(8), OK L(1). No memo. */
    dbf_field_spec fields[4] = {
        { "FNAME",  'C', 10u, 0u },
        { "AMOUNT", 'N',  8u, 2u },
        { "WHEN",   'D',  8u, 0u },
        { "OK",     'L',  1u, 0u }
    };
    xb_val rec[4];

    rc = dbf_create(pal, path, fields, 4, &tbl);
    if (rc != DBF_OK || !tbl) return;

    rec[0] = xb_c("Alice", 5u);
    rec[1] = xb_n(123.45);
    rec[2] = xb_d((double)jdn_from_ymd(1985, 8, 5));
    rec[3] = xb_l(1);
    dbf_append_rec(tbl, rec, 0);

    rec[0] = xb_c("Bob", 3u);
    rec[1] = xb_n(-7.5);
    rec[2] = xb_d((double)jdn_from_ymd(1999, 12, 31));
    rec[3] = xb_l(0);
    dbf_append_rec(tbl, rec, 0);

    dbf_flush(tbl);
    dbf_close(tbl);
}

static void test_determinism(void)
{
    struct pal_host_cfg cfg;
    samir_pal_t *pal_a, *pal_b;
    uint8_t *a = NULL, *b = NULL;
    long la = 0, lb = 0;
    char msg[256];

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy = RT_YY; cfg.date_mm = RT_MM; cfg.date_dd = RT_DD;
    cfg.heap_size = 256u * 1024u;

    pal_a = pal_host_make(cfg);
    pal_b = pal_host_make(cfg);
    CHECK(pal_a && pal_b, "determinism: pal_host_make (two arenas)");
    if (!pal_a || !pal_b) {
        if (pal_a) pal_host_free(pal_a);
        if (pal_b) pal_host_free(pal_b);
        return;
    }

    build_det_table(pal_a, RT_PATH_A);
    build_det_table(pal_b, RT_PATH_B);

    a = slurp(RT_PATH_A, &la);
    b = slurp(RT_PATH_B, &lb);
    CHECK(a != NULL && b != NULL, "determinism: both files readable");
    if (a && b) {
        snprintf(msg, sizeof(msg),
                 "determinism: file sizes equal (a=%ld b=%ld)", la, lb);
        CHECK(la == lb, msg);
        if (la == lb) {
            snprintf(msg, sizeof(msg),
                     "determinism: write-twice byte-identical (%ld bytes)", la);
            CHECK(memcmp(a, b, (size_t)la) == 0, msg);
        }
    }

    free(a); free(b);
    pal_host_free(pal_a);
    pal_host_free(pal_b);
}

/* ============================================================
 * Oracle 2: TIER-0 hand-computed template (operator-free, day-1)
 *
 * Schema: ONE C(5) field "CODE" + ONE N(6,2) field "PRICE". No memo, nrec=0.
 * Hand-computed expected image (header + 2 descriptors + 0x0D term + 0x1A EOF):
 *
 *   header_length = 32 + 32*2 + 1 = 97  (0x61) (the +1 form)
 *   record_length = 1 + 5 + 6     = 12  (0x0C)
 *   version       = 0x03 (no memo)
 *   date          = YY=85 (0x55), MM=10 (0x0A), DD=30 (0x1E)  (RT_YY/MM/DD)
 *   nrec          = 0
 *
 * Every NORMALIZE byte is 0x00 (dbf_normalization.json): header 0x0C..0x1F,
 * descriptor RAM addr 0x0C..0x0F, work-area 0x14, reserved/flag bytes.
 * Ref: dbf.md sec 2 (header), sec 4 (descriptor + +1 term), sec 8.
 * ============================================================ */
static void test_template(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    int rc;
    uint8_t *got = NULL;
    long glen = 0;
    char msg[256];

    dbf_field_spec fields[2] = {
        { "CODE",  'C', 5u, 0u },
        { "PRICE", 'N', 6u, 2u }
    };

    /* Hand-computed expected bytes. header_length = 97 (= 32 + 64 + 1): the
     * header region is bytes 0x00..0x60 inclusive, the lone 0x0D terminator
     * sitting at offset 0x60. With nrec=0, the trailing 0x1A EOF sits at the
     * record-area start = header_length + 0*record_length = 97 = 0x61. So the
     * whole file is bytes 0x00..0x61 = 98 bytes. (dbf.md sec 2/4/8.) */
    uint8_t exp[98];
    memset(exp, 0, sizeof(exp));

    /* --- header (offset 0x00..0x1F) --- */
    exp[0x00] = 0x03;        /* version: no memo (dbf.md sec 3) */
    exp[0x01] = RT_YY;       /* year - 1900 = 85 (dbf.md sec 2) */
    exp[0x02] = RT_MM;       /* month 10 */
    exp[0x03] = RT_DD;       /* day 30 */
    /* nrec = 0 -> bytes 0x04..0x07 all zero (already memset) */
    exp[0x08] = 97; exp[0x09] = 0;   /* header_length = 97 (0x61), u16 LE */
    exp[0x0A] = 12; exp[0x0B] = 0;   /* record_length = 12 (0x0C), u16 LE */
    /* 0x0C..0x1F: all NORMALIZE -> 0x00 (already memset) */

    /* --- descriptor 0: "CODE" C len 5 dec 0, at file offset 0x20 --- */
    {
        int o = 0x20;
        exp[o + 0x00] = 'C'; exp[o + 0x01] = 'O'; exp[o + 0x02] = 'D';
        exp[o + 0x03] = 'E';             /* name "CODE\0\0\0..." */
        exp[o + 0x0B] = 'C';             /* type */
        /* 0x0C..0x0F RAM addr: 0x00000000 (NORMALIZE) */
        exp[o + 0x10] = 5;               /* field_len */
        exp[o + 0x11] = 0;               /* dec */
        /* 0x12..0x1F: 0x00 (NORMALIZE: work-area, reserved, flags) */
    }

    /* --- descriptor 1: "PRICE" N len 6 dec 2, at file offset 0x40 --- */
    {
        int o = 0x40;
        exp[o + 0x00] = 'P'; exp[o + 0x01] = 'R'; exp[o + 0x02] = 'I';
        exp[o + 0x03] = 'C'; exp[o + 0x04] = 'E'; /* name "PRICE\0..." */
        exp[o + 0x0B] = 'N';             /* type */
        exp[o + 0x10] = 6;               /* field_len */
        exp[o + 0x11] = 2;               /* dec */
    }

    /* --- terminator + EOF --- */
    exp[0x60] = 0x0D;   /* 0x0D terminator at 32+64 = 96 = 0x60 (the +1 form) */
    exp[0x61] = 0x1A;   /* 0x1A EOF at header_length(97)+0*reclen = 97 = 0x61 */

    rc = dbf_create(pal, RT_PATH_TMPL, fields, 2, &tbl);
    snprintf(msg, sizeof(msg), "template: dbf_create rc=%d", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    rc = dbf_flush(tbl);
    snprintf(msg, sizeof(msg), "template: dbf_flush rc=%d", rc);
    CHECK(rc == DBF_OK, msg);
    dbf_close(tbl);

    got = slurp(RT_PATH_TMPL, &glen);
    CHECK(got != NULL, "template: written file readable");
    if (got) {
        snprintf(msg, sizeof(msg),
                 "template: file size == 98 (got %ld) [hdr-region97 (incl 0x0D) + eof1, 0 recs]",
                 glen);
        CHECK(glen == 98, msg);
        if (glen == 98) {
            int i, firstdiff = -1;
            for (i = 0; i < 98; i++) {
                if (got[i] != exp[i]) { firstdiff = i; break; }
            }
            snprintf(msg, sizeof(msg),
                     "template: header+descriptors+term+eof byte-match "
                     "(first diff at offset 0x%X: got 0x%02X exp 0x%02X)",
                     firstdiff < 0 ? 0 : firstdiff,
                     firstdiff < 0 ? 0 : got[firstdiff],
                     firstdiff < 0 ? 0 : exp[firstdiff]);
            CHECK(firstdiff < 0, msg);
        }
        free(got);
    }
}

/* ============================================================
 * Oracle 3a: C ROUND-TRIP, no-memo schema (C/N/D/L)
 *
 * Build a table, flush, re-open with dbf_open, assert schema + record values.
 * ============================================================ */
static void test_roundtrip_nomemo(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    int rc;
    char msg[256];
    int32_t jdn1 = jdn_from_ymd(1985, 8, 5);
    int32_t jdn2 = jdn_from_ymd(1999, 12, 31);

    dbf_field_spec fields[4] = {
        { "FNAME",  'C', 10u, 0u },
        { "AMOUNT", 'N',  8u, 2u },
        { "WHEN",   'D',  8u, 0u },
        { "OK",     'L',  1u, 0u }
    };
    xb_val rec[4];

    rc = dbf_create(pal, RT_PATH_REF, fields, 4, &tbl);
    snprintf(msg, sizeof(msg), "roundtrip-nomemo: dbf_create rc=%d", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    rec[0] = xb_c("Alice", 5u);
    rec[1] = xb_n(123.45);
    rec[2] = xb_d((double)jdn1);
    rec[3] = xb_l(1);
    rc = dbf_append_rec(tbl, rec, 0);
    CHECK(rc == DBF_OK, "roundtrip-nomemo: append rec1");

    rec[0] = xb_c("Bob", 3u);
    rec[1] = xb_n(-7.5);
    rec[2] = xb_d((double)jdn2);
    rec[3] = xb_l(0);
    rc = dbf_append_rec(tbl, rec, 0);
    CHECK(rc == DBF_OK, "roundtrip-nomemo: append rec2");

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "roundtrip-nomemo: dbf_flush");
    dbf_close(tbl);

    /* Re-open from disk and verify the schema + values. */
    tbl = NULL;
    rc = dbf_open(pal, RT_PATH_REF, &tbl);
    snprintf(msg, sizeof(msg), "roundtrip-nomemo: dbf_open rc=%d", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    /* Schema invariants. */
    snprintf(msg, sizeof(msg), "roundtrip-nomemo: version 0x03 (got 0x%02X)",
             dbf_version(tbl));
    CHECK(dbf_version(tbl) == 0x03u, msg);
    CHECK(dbf_has_memo(tbl) == 0, "roundtrip-nomemo: has_memo==0");
    CHECK(dbf_nfields(tbl) == 4u, "roundtrip-nomemo: nfields==4");
    CHECK(dbf_nrec(tbl) == 2u, "roundtrip-nomemo: nrec==2");
    CHECK(dbf_record_length(tbl) == (uint16_t)(1 + 10 + 8 + 8 + 1),
          "roundtrip-nomemo: record_length==28");
    CHECK(dbf_header_length(tbl) == (uint16_t)(32 + 32 * 4 + 1),
          "roundtrip-nomemo: header_length==161 (+1 form)");
    CHECK(dbf_term_extra(tbl) == 1u, "roundtrip-nomemo: +1 terminator");
    CHECK(dbf_year(tbl) == RT_YY && dbf_month(tbl) == RT_MM &&
          dbf_day(tbl) == RT_DD, "roundtrip-nomemo: injected date round-trips");

    /* Field descriptors. */
    {
        const dbf_field_t *f0 = dbf_field(tbl, 0);
        const dbf_field_t *f1 = dbf_field(tbl, 1);
        const dbf_field_t *f2 = dbf_field(tbl, 2);
        const dbf_field_t *f3 = dbf_field(tbl, 3);
        CHECK(f0 && strcmp(f0->name, "FNAME") == 0 && f0->type == 'C' &&
              f0->field_len == 10u && f0->dec_count == 0u,
              "roundtrip-nomemo: field0 FNAME C(10,0)");
        CHECK(f1 && strcmp(f1->name, "AMOUNT") == 0 && f1->type == 'N' &&
              f1->field_len == 8u && f1->dec_count == 2u,
              "roundtrip-nomemo: field1 AMOUNT N(8,2)");
        CHECK(f2 && strcmp(f2->name, "WHEN") == 0 && f2->type == 'D' &&
              f2->field_len == 8u,
              "roundtrip-nomemo: field2 WHEN D(8)");
        CHECK(f3 && strcmp(f3->name, "OK") == 0 && f3->type == 'L' &&
              f3->field_len == 1u,
              "roundtrip-nomemo: field3 OK L(1)");
    }

    /* Record 1 values. */
    {
        xb_val out[4];
        int deleted = -1;
        rc = dbf_read_rec(tbl, 1u, out, &deleted);
        CHECK(rc == DBF_OK, "roundtrip-nomemo: read rec1");
        if (rc == DBF_OK) {
            CHECK(deleted == 0, "roundtrip-nomemo: rec1 live");
            /* FNAME: raw C(10), "Alice" + 5 trailing spaces. */
            CHECK(out[0].t == XB_C && out[0].u.c.len == 10u &&
                  memcmp(out[0].u.c.p, "Alice     ", 10) == 0,
                  "roundtrip-nomemo: rec1 FNAME='Alice     '");
            /* AMOUNT: 123.45 round-trips through dec_format/dec_parse. */
            CHECK(out[1].t == XB_N && out[1].u.n == 123.45,
                  "roundtrip-nomemo: rec1 AMOUNT==123.45");
            CHECK(out[2].t == XB_D && out[2].u.d == (double)jdn1,
                  "roundtrip-nomemo: rec1 WHEN==JDN(1985-08-05)");
            CHECK(out[3].t == XB_L && out[3].u.l == 1u,
                  "roundtrip-nomemo: rec1 OK==T");
        }
    }

    /* Record 2 values (negative N + L false). */
    {
        xb_val out[4];
        int deleted = -1;
        rc = dbf_read_rec(tbl, 2u, out, &deleted);
        CHECK(rc == DBF_OK, "roundtrip-nomemo: read rec2");
        if (rc == DBF_OK) {
            CHECK(out[0].t == XB_C && memcmp(out[0].u.c.p, "Bob       ", 10) == 0,
                  "roundtrip-nomemo: rec2 FNAME='Bob       '");
            CHECK(out[1].t == XB_N && out[1].u.n == -7.5,
                  "roundtrip-nomemo: rec2 AMOUNT==-7.5");
            CHECK(out[2].t == XB_D && out[2].u.d == (double)jdn2,
                  "roundtrip-nomemo: rec2 WHEN==JDN(1999-12-31)");
            CHECK(out[3].t == XB_L && out[3].u.l == 0u,
                  "roundtrip-nomemo: rec2 OK==F");
        }
    }

    dbf_close(tbl);
}

/* ============================================================
 * Oracle 3b: C ROUND-TRIP, MEMO-bearing schema (the 0x83 / mutant case)
 *
 * Build a table with an M field, flush, re-open, assert version 0x83 +
 * has_memo==true + the M pointer round-trips. Under -DDBF_MUTATE_VERSION the
 * memo bit is dropped -> version reads back 0x03 / has_memo==false -> RED.
 *
 * S1.4->S2.2 boundary: no .dbt is written. We append a numeric block number
 * (xb_n) for the M field; the reader returns xb_m(raw,10) for block>0 and
 * xb_u() for block 0. We verify the M pointer text and has_memo.
 * ============================================================ */
static void test_roundtrip_memo(samir_pal_t *pal)
{
    dbf_table *tbl = NULL;
    int rc;
    char msg[256];

    dbf_field_spec fields[2] = {
        { "TITLE", 'C', 8u, 0u },
        { "NOTE",  'M', 10u, 0u }
    };
    xb_val rec[2];

    rc = dbf_create(pal, RT_PATH_MEMO, fields, 2, &tbl);
    snprintf(msg, sizeof(msg), "roundtrip-memo: dbf_create rc=%d", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    /* rec1: TITLE="First", NOTE=block 1 (a memo pointer). */
    rec[0] = xb_c("First", 5u);
    rec[1] = xb_n(1.0);                 /* block number 1 */
    rc = dbf_append_rec(tbl, rec, 0);
    CHECK(rc == DBF_OK, "roundtrip-memo: append rec1");

    /* rec2: TITLE="Second", NOTE=no memo (xb_u -> 10 spaces -> block 0). */
    rec[0] = xb_c("Second", 6u);
    rec[1] = xb_u();
    rc = dbf_append_rec(tbl, rec, 0);
    CHECK(rc == DBF_OK, "roundtrip-memo: append rec2");

    rc = dbf_flush(tbl);
    CHECK(rc == DBF_OK, "roundtrip-memo: dbf_flush");
    dbf_close(tbl);

    /* Re-open and assert the memo bit. THIS is what the mutant breaks. */
    tbl = NULL;
    rc = dbf_open(pal, RT_PATH_MEMO, &tbl);
    snprintf(msg, sizeof(msg), "roundtrip-memo: dbf_open rc=%d", rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    snprintf(msg, sizeof(msg),
             "roundtrip-memo: version 0x83 (got 0x%02X) [DBF_MUTATE_VERSION -> 0x03 RED]",
             dbf_version(tbl));
    CHECK(dbf_version(tbl) == 0x83u, msg);
    CHECK(dbf_has_memo(tbl) == 1, "roundtrip-memo: has_memo==1 (mutant: 0 -> RED)");
    CHECK(dbf_nfields(tbl) == 2u, "roundtrip-memo: nfields==2");
    CHECK(dbf_nrec(tbl) == 2u, "roundtrip-memo: nrec==2");

    /* rec1 NOTE -> block 1 -> xb_m(raw,10); rec2 -> block 0 -> xb_u(). */
    {
        xb_val out[2];
        int deleted = -1;
        rc = dbf_read_rec(tbl, 1u, out, &deleted);
        CHECK(rc == DBF_OK, "roundtrip-memo: read rec1");
        if (rc == DBF_OK) {
            CHECK(out[0].t == XB_C && memcmp(out[0].u.c.p, "First   ", 8) == 0,
                  "roundtrip-memo: rec1 TITLE='First   '");
            CHECK(out[1].t == XB_M && out[1].u.c.len == 10u &&
                  out[1].u.c.p[9] == '1',
                  "roundtrip-memo: rec1 NOTE -> block 1 (raw[9]=='1')");
        }
        rc = dbf_read_rec(tbl, 2u, out, &deleted);
        CHECK(rc == DBF_OK, "roundtrip-memo: read rec2");
        if (rc == DBF_OK) {
            CHECK(out[1].t == XB_U, "roundtrip-memo: rec2 NOTE -> xb_u (no memo)");
        }
    }

    dbf_close(tbl);
}

/* ============================================================
 * Oracle 4: NORMALIZATION-MASKED golden cmp (Tier 1)
 *
 * Read CLIENTS.DBF with the C reader, re-create+append+flush an identical
 * schema with identical record values, then compare the re-written header +
 * descriptor region against the golden with every NORMALIZE byte masked to 0
 * (dbf_normalization.json). The MEANINGFUL bytes must match.
 *
 * NORMALIZE mask (dbf_normalization.json + dbf.md sec 4 name-tail rule):
 *   header: 0x0C..0x1F (reserved/MDX/LDID/multiuser).
 *   per-descriptor (stride 32): 0x0C..0x0F (RAM addr), 0x12..0x13, 0x14,
 *     0x15..0x16, 0x17, 0x18..0x1E, 0x1F.  (i.e. everything except name 0x00..0x0A,
 *     type 0x0B, field_len 0x10, dec 0x11.)
 *   NAME TAIL: within the 11-byte name (0x00..0x0A), only the bytes up to AND
 *     INCLUDING the first 0x00 are MEANINGFUL; bytes AFTER the first NUL are
 *     stale garbage in genuine III+ (dbf.md sec 4: "unused bytes after the
 *     terminating 0x00 can contain stale bytes" -- e.g. CLIENTS ADDRESS holds
 *     "ADDRESS\0HO\0"). Our writer NUL-pads them; the golden may not. So we
 *     mask the name-tail past the first NUL when comparing.
 * The last-update DATE (header 0x01..0x03) is MEANINGFUL; we inject CLIENTS'
 * real date (85/10/30) so it matches.
 *
 * Loud-skip if the golden is absent (never a silent pass).
 * ============================================================ */
static int desc_norm_byte(int off_in_desc)
{
    /* 1 if this descriptor byte is NORMALIZE (masked), 0 if MEANINGFUL.
     * (Name-tail-past-NUL masking is handled by the caller, which has the
     * name bytes to find the first NUL.) */
    if (off_in_desc <= 0x0A) return 0;          /* name 0x00..0x0A */
    if (off_in_desc == 0x0B) return 0;          /* type */
    if (off_in_desc == 0x10) return 0;          /* field_len */
    if (off_in_desc == 0x11) return 0;          /* dec */
    return 1;                                    /* everything else NORMALIZE */
}

static void test_golden_masked(samir_pal_t *pal, const char *base)
{
    char path[1024];
    dbf_table *gold = NULL, *mine = NULL;
    int rc;
    char msg[256];
    int nf, i;
    dbf_field_spec *specs = NULL;
    uint8_t *g = NULL, *m = NULL;
    long glen = 0, mlen = 0;
    uint32_t hdr_len_cmp;

    join(path, sizeof(path), base, SP_PATH "/CLIENTS.DBF");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): golden absent: %s\n", path);
        return;
    }

    rc = dbf_open(pal, path, &gold);
    snprintf(msg, sizeof(msg), "golden-masked: dbf_open CLIENTS rc=%d", rc);
    CHECK(rc == DBF_OK && gold != NULL, msg);
    if (rc != DBF_OK || !gold) return;

    nf = (int)dbf_nfields(gold);
    specs = (dbf_field_spec *)calloc((size_t)nf, sizeof(dbf_field_spec));
    CHECK(specs != NULL, "golden-masked: alloc specs");
    if (!specs) { dbf_close(gold); return; }

    /* Build a schema mirroring the golden's descriptors. The dbf_field_t names
     * are stable until dbf_close(gold) -- we keep gold open while creating. */
    for (i = 0; i < nf; i++) {
        const dbf_field_t *f = dbf_field(gold, i);
        specs[i].name      = f->name;     /* points into gold's arena (alive) */
        specs[i].type      = f->type;
        specs[i].field_len = f->field_len;
        specs[i].dec       = f->dec_count;
    }

    rc = dbf_create(pal, RT_PATH_GOLD, specs, nf, &mine);
    snprintf(msg, sizeof(msg), "golden-masked: dbf_create mirror rc=%d", rc);
    CHECK(rc == DBF_OK && mine != NULL, msg);
    if (rc != DBF_OK || !mine) { free(specs); dbf_close(gold); return; }

    /* Copy every golden record into the new table (so the schema region we
     * compare is generated by a real append path, not just an empty file). */
    {
        uint32_t n = dbf_nrec(gold);
        uint32_t r;
        xb_val *out = (xb_val *)calloc((size_t)nf, sizeof(xb_val));
        CHECK(out != NULL, "golden-masked: alloc rec buffer");
        if (out) {
            for (r = 1u; r <= n; r++) {
                int del = 0;
                rc = dbf_read_rec(gold, r, out, &del);
                if (rc != DBF_OK) break;
                /* out[] C/M pointers reference gold's rec_buf (valid until the
                 * next read on gold); dbf_append_rec copies immediately. */
                dbf_append_rec(mine, out, del);
            }
            free(out);
        }
    }

    rc = dbf_flush(mine);
    CHECK(rc == DBF_OK, "golden-masked: dbf_flush mirror");

    /* Slurp both files and compare the header + descriptor region (up to and
     * including the 0x0D terminator) with NORMALIZE bytes masked. */
    g = slurp(path, &glen);
    m = slurp(RT_PATH_GOLD, &mlen);
    CHECK(g != NULL && m != NULL, "golden-masked: both files readable");

    /* Compare only the header (32) + descriptors (32*nf). The terminator region
     * differs (golden CLIENTS is +1, we emit +1 too, so the 0x0D matches; but
     * to be robust we compare exactly the descriptor area which both share). */
    hdr_len_cmp = (uint32_t)DBF_HDR_SIZE + (uint32_t)DBF_DESC_STRIDE * (uint32_t)nf;

    if (g && m && glen >= (long)hdr_len_cmp && mlen >= (long)hdr_len_cmp) {
        int firstdiff = -1;
        uint32_t off;
        for (off = 0u; off < hdr_len_cmp; off++) {
            int masked = 0;
            if (off < (uint32_t)DBF_HDR_SIZE) {
                /* header: 0x0C..0x1F NORMALIZE */
                if (off >= 0x0Cu) masked = 1;
            } else {
                uint32_t desc_base = (uint32_t)DBF_HDR_SIZE +
                    ((off - (uint32_t)DBF_HDR_SIZE) / (uint32_t)DBF_DESC_STRIDE)
                    * (uint32_t)DBF_DESC_STRIDE;
                int od = (int)(off - desc_base);
                masked = desc_norm_byte(od);
                /* Name-tail-past-NUL masking (dbf.md sec 4): within the 11-byte
                 * name (0x00..0x0A), mask bytes strictly after our first NUL --
                 * those are stale garbage in the golden. Use MINE (NUL-padded,
                 * canonical) to locate the first NUL. */
                if (!masked && od <= 0x0A) {
                    uint32_t k;
                    int past_nul = 0;
                    for (k = 0u; k <= (uint32_t)od; k++) {
                        if (m[desc_base + k] == 0x00u && (int)k < od) {
                            past_nul = 1;
                            break;
                        }
                    }
                    if (past_nul) masked = 1;
                }
            }
            if (masked) continue;
            if (g[off] != m[off]) { firstdiff = (int)off; break; }
        }
        snprintf(msg, sizeof(msg),
                 "golden-masked: CLIENTS MEANINGFUL bytes match "
                 "(first diff at 0x%X: golden 0x%02X mine 0x%02X)",
                 firstdiff < 0 ? 0 : firstdiff,
                 firstdiff < 0 ? 0 : g[firstdiff],
                 firstdiff < 0 ? 0 : m[firstdiff]);
        CHECK(firstdiff < 0, msg);
    } else {
        CHECK(0, "golden-masked: files too short for header+descriptor cmp");
    }

    free(g); free(m); free(specs);
    dbf_close(mine);
    dbf_close(gold);
}

/* ============================================================
 * Oracle 5: INDEPENDENCE BARRIER -- dbf_ref.py read-back (optional system())
 *
 * The no-memo round-trip (oracle 3a) left a SAMIR-written file at RT_PATH_REF.
 * If python3 + dbf_ref.py are available, shell out and gate on the exit code
 * (a non-zero exit = the independent reader rejected our bytes). If not
 * available, LOUD-skip and tell the orchestrator the exact command to run.
 * Ref: harness/diff/dbf_diff/dbf_ref.py --records.
 * ============================================================ */
static void test_independence_barrier(void)
{
    char cmd[1024];
    int  ret;

    if (!file_exists(RT_PATH_REF)) {
        fprintf(stderr,
                "  SKIP (LOUD): no SAMIR-written file at %s "
                "(roundtrip-nomemo did not run)\n", RT_PATH_REF);
        return;
    }

    /* Resolve dbf_ref.py relative to this source file's directory at build time.
     * The orchestrator runs from the repo root, so the relative path holds. */
    snprintf(cmd, sizeof(cmd),
             "python3 harness/diff/dbf_diff/dbf_ref.py --records %s "
             ">/dev/null 2>&1", RT_PATH_REF);
    ret = system(cmd);

    if (ret == -1) {
        fprintf(stderr,
                "  SKIP (LOUD): could not invoke python3 for dbf_ref.py; "
                "orchestrator: run\n"
                "    python3 harness/diff/dbf_diff/dbf_ref.py --records %s\n",
                RT_PATH_REF);
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
                    RT_PATH_REF);
            return;
        }
        CHECK(code == 0,
              "independence: dbf_ref.py --records read back SAMIR file (exit 0)");
        return;
    }
#endif
    /* Fallback: treat a zero raw return as success. */
    CHECK(ret == 0,
          "independence: dbf_ref.py --records read back SAMIR file (exit 0)");
}

/* ============================================================
 * main
 * ============================================================ */

int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;

    /* Injected fixed date for all writers (Rule 11): 1985-10-30 (CLIENTS date,
     * so a date-inclusive golden cmp matches). Arena sized generously for the
     * golden-mirror (CLIENTS: 49 records * 106 bytes * doubling). */
    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = RT_YY;
    cfg.date_mm   = RT_MM;
    cfg.date_dd   = RT_DD;
    cfg.heap_size = 512u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* Oracle 1: determinism (own PALs internally). */
    test_determinism();

    /* Oracle 2: Tier-0 hand-computed template (operator-free). */
    test_template(pal);

    /* Oracle 3a/3b: C round-trip (no-memo + memo). 3a leaves RT_PATH_REF. */
    test_roundtrip_nomemo(pal);
    test_roundtrip_memo(pal);

    /* Oracle 4: normalization-masked golden cmp (loud-skip if absent). */
    test_golden_masked(pal, base);

    /* Oracle 5: independence barrier via dbf_ref.py (optional system()). */
    test_independence_barrier();

    pal_host_free(pal);
    return TEST_SUMMARY("test-dbf-roundtrip");
}
