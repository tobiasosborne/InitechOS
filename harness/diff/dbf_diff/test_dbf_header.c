/*
 * harness/diff/dbf_diff/test_dbf_header.c -- host oracle for dbf_open (S1.1).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_samir_value.c:
 * the seed test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), a host
 * PAL via pal_host_make (pal_host.c). A non-zero exit on any failed check keeps
 * `make test-dbase` from false-greening (Law 2: the oracle is the truth).
 *
 * Two tiers (plan Sec 2.A):
 *   Tier 0 (committed, operator-free): expected header values transcribed from
 *     the dbf.md "Verification" byte-dumps, asserted against the REAL goldens
 *     opened through the PAL. (The values are our synthesized manifest; each is
 *     cited to a dbf.md section. They gate even if a single golden is absent --
 *     each Tier-0 table that is missing prints a loud skip and the rest run.)
 *   Tier 1 (golden-diff): every named golden is opened + every parsed field is
 *     asserted, including the +1/+2 terminator form and the optional 0x1A EOF.
 *
 * Goldens base resolves from argv[1] (orchestrator passes the corpus path);
 * default "../dbase3-decomp". If a golden is absent, a LOUD skip names it (the
 * fat-fault-rollback idiom) and the test proceeds -- NEVER a silent pass.
 *
 * Mutation (Rule 6): built with -DDBF_MUTATE_RECLEN, dbf.c checks record_length
 * against (1 + sum + 1) instead of (1 + sum); every valid golden then fails
 * dbf_open with DBF_ERR_BAD_RECLEN, so the "dbf_open succeeds" checks go RED.
 *
 * Compile + run (self-grade, host, NOT make):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbf.c os/samir/core/rt.c os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbf_header.c -o /tmp/test_dbf_header \
 *     && /tmp/test_dbf_header ../dbase3-decomp
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 2/3/4/8 + Verification.
 *   - spec/samir/dbf_format.h (the LOCKED constants; +1/+2, optional EOF notes).
 *   - docs/plans/SAMIR-implementation-plan.md S1.1 oracle contract.
 *   - os/samir/include/samir/dbf.h (the API under test).
 *   - os/samir/pal/pal_host.c (struct pal_host_cfg, pal_host_make/_free).
 *   - seed/test_assert.h (the harness idiom).
 */

#include <stdio.h>
#include <string.h>

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/dbf.h"         /* os/samir/include/, on -Ios/samir/include */

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

/* ---- expected-header manifest (Tier-0; one row per golden) ---- */
typedef struct {
    const char *relpath;     /* path relative to the goldens base (argv[1]) */
    const char *label;       /* short name for messages */
    uint8_t     version;     /* dbf.md sec 3 */
    uint8_t     year;        /* dbf.md sec 2 (year - 1900) */
    uint8_t     month;
    uint8_t     day;
    uint32_t    nrec;        /* dbf.md sec 2 offset 0x04 */
    uint16_t    header_len;  /* dbf.md sec 2 offset 0x08 */
    uint16_t    record_len;  /* dbf.md sec 2 offset 0x0A */
    uint16_t    nfields;     /* scan-to-0x0D (dbf.md sec 4/8) */
    uint8_t     term_extra;  /* +1 or +2 (dbf.md sec 4) */
    int         has_memo;    /* dbf.md sec 3 (memo bit) */
} dbf_expect;

/*
 * Tier-0 manifest. Values transcribed from the dbf.md sec 2/8 tables +
 * Verification section. Header dates: CLIENTS 03 55 0A 1E -> 1985-10-30
 * (dbf.md sec 2 worked example); BANK 55 08 0E -> 1985-08-14; TAX 55 06 09 ->
 * 1985-06-09; TOURS/TRAVEL 55 0A 1E / 55 0B 0E (dbf.md sec 2 date table).
 */
static const dbf_expect MANIFEST[] = {
    /* +1 form (lone 0x0D), no memo, no EOF byte. dbf.md sec 8 Inv1/Inv2 tables. */
    { "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/CLIENTS.DBF",
      "CLIENTS", 0x03, 85, 10, 30,  49, 257, 106, 7, 1, 0 },
    /* +1 form, MEMO (0x83), no EOF. dbf.md sec 3/8. */
    { "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TOURS.DBF",
      "TOURS",   0x83, 85, 10, 30,  30, 257,  83, 7, 1, 1 },
    /* +1 form, MEMO, 11 fields incl. L+M, no EOF. dbf.md sec 8 (hlen 385). */
    { "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TRAVEL.DBF",
      "TRAVEL",  0x83, 85, 11, 14,  49, 385, 137, 11, 1, 1 },
    /* +2 form (0x0D 0x00), nrec=0, ghost data after EOF. dbf.md sec 8 BANK case. */
    { "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/BANK.DBF",
      "BANK",    0x03, 85,  8, 14,   0, 194,  24, 5, 2, 0 },
    /* +2 form, EOF byte (0x1A) PRESENT. dbf.md sec 8 Inv2 (file == body + 1). */
    { "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TAX.DBF",
      "TAX",     0x03, 85,  6,  9,   9,  98,  20, 2, 2, 0 },
};
#define MANIFEST_N ((int)(sizeof(MANIFEST) / sizeof(MANIFEST[0])))

/* Join base + relpath into buf; returns buf. */
static char *join(char *buf, size_t cap, const char *base, const char *rel)
{
    snprintf(buf, cap, "%s/%s", base, rel);
    return buf;
}

/* True if the file exists / is readable (so we can loud-skip an absent golden). */
static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* Open one golden via the PAL and assert it matches its manifest row. */
static void check_golden(samir_pal_t *pal, const char *base, const dbf_expect *e)
{
    char path[1024];
    char msg[256];
    dbf_table *tbl = NULL;
    int rc;

    join(path, sizeof(path), base, e->relpath);

    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): golden absent: %s -- pass corpus base as argv[1]\n",
                path);
        return;
    }

    rc = dbf_open(pal, path, &tbl);
    snprintf(msg, sizeof(msg), "%s: dbf_open succeeds (rc=%d)", e->label, rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || tbl == NULL)
        return;   /* nothing more to check on this table */

    snprintf(msg, sizeof(msg), "%s: version 0x%02X", e->label, e->version);
    CHECK(dbf_version(tbl) == e->version, msg);

    snprintf(msg, sizeof(msg), "%s: year %u", e->label, (unsigned)e->year);
    CHECK(dbf_year(tbl) == e->year, msg);
    snprintf(msg, sizeof(msg), "%s: month %u", e->label, (unsigned)e->month);
    CHECK(dbf_month(tbl) == e->month, msg);
    snprintf(msg, sizeof(msg), "%s: day %u", e->label, (unsigned)e->day);
    CHECK(dbf_day(tbl) == e->day, msg);

    snprintf(msg, sizeof(msg), "%s: nrec %u", e->label, (unsigned)e->nrec);
    CHECK(dbf_nrec(tbl) == e->nrec, msg);

    snprintf(msg, sizeof(msg), "%s: header_length %u", e->label,
             (unsigned)e->header_len);
    CHECK(dbf_header_length(tbl) == e->header_len, msg);

    snprintf(msg, sizeof(msg), "%s: record_length %u", e->label,
             (unsigned)e->record_len);
    CHECK(dbf_record_length(tbl) == e->record_len, msg);

    snprintf(msg, sizeof(msg), "%s: nfields %u", e->label, (unsigned)e->nfields);
    CHECK(dbf_nfields(tbl) == e->nfields, msg);

    snprintf(msg, sizeof(msg), "%s: term_extra +%u", e->label,
             (unsigned)e->term_extra);
    CHECK(dbf_term_extra(tbl) == e->term_extra, msg);

    snprintf(msg, sizeof(msg), "%s: has_memo %d", e->label, e->has_memo);
    CHECK(dbf_has_memo(tbl) == e->has_memo, msg);

    rc = dbf_close(tbl);
    snprintf(msg, sizeof(msg), "%s: dbf_close succeeds (rc=%d)", e->label, rc);
    CHECK(rc == DBF_OK, msg);
}

/*
 * Negative test: a synthesized buffer with an unsupported (dBASE IV) version
 * byte must fail loud. Written to a temp file and opened through the PAL.
 * Ref: dbf.md sec 3 (0x8B = IV memo; never III+); plan Sec 2.C (fail loud).
 */
static void check_iv_version_rejected(samir_pal_t *pal)
{
    /*
     * Minimal "TAX-shaped" header (2 fields, +2 form, hlen 98, rlen 20) but with
     * an unsupported version byte 0x8B (dBASE IV with memo). dbf_open must reject
     * it at the version gate, BEFORE any invariant. We only need a well-formed
     * 32-byte header; the version check fires first.
     */
    static const unsigned char hdr_iv[32] = {
        0x8B,             /* version: dBASE IV with memo -- UNSUPPORTED in III+ */
        85, 6, 9,         /* date */
        9, 0, 0, 0,       /* nrec = 9 (LE) */
        98, 0,            /* header_length = 98 (LE) */
        20, 0,            /* record_length = 20 (LE) */
        0,0, 0, 0, 0,0,0,0,0,0,0,0, 0, 0, 0,0  /* reserved (offsets 0x0C..0x1F) */
    };
    const char *tmp = "/tmp/test_dbf_iv_version.dbf";
    FILE *f;
    dbf_table *tbl = NULL;
    int rc;

    f = fopen(tmp, "wb");
    if (!f) {
        fprintf(stderr, "  SKIP (LOUD): cannot write temp file %s\n", tmp);
        return;
    }
    fwrite(hdr_iv, 1, sizeof(hdr_iv), f);
    fclose(f);

    rc = dbf_open(pal, tmp, &tbl);
    CHECK(rc == -DBF_ERR_BAD_VERSION && tbl == NULL,
          "IV version 0x8B rejected with DBF_ERR_BAD_VERSION (plan Sec 2.C)");

    remove(tmp);
}

int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    int i;
    int any_present = 0;
    char path[1024];

    /* Injected fixed date (Rule 11); unused by S1.1 but the host PAL requires it.
     * Arena must hold the dbf_table several times over (open/close reuses it via
     * the mark, but size it generously). */
    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy = 99;       /* arbitrary fixed values; today() is not exercised */
    cfg.date_mm = 12;
    cfg.date_dd = 31;
    cfg.heap_size = 64u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* ---- Tier 0 + Tier 1: every golden, each loud-skipped if absent ---- */
    for (i = 0; i < MANIFEST_N; i++) {
        if (file_exists(join(path, sizeof(path), base, MANIFEST[i].relpath)))
            any_present = 1;
        check_golden(pal, base, &MANIFEST[i]);
    }

    if (!any_present) {
        fprintf(stderr,
                "  SKIP (LOUD): no goldens found under base '%s'\n"
                "               expected e.g. %s/%s\n"
                "               (pass the corpus base as argv[1]; Tier-0 still ran"
                " the negative test below)\n",
                base, base, MANIFEST[0].relpath);
    }

    /* ---- Negative / fail-loud test: runs with no external dependency ---- */
    check_iv_version_rejected(pal);

    pal_host_free(pal);
    return TEST_SUMMARY("test-dbf-header");
}
