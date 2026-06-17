/*
 * harness/diff/dbf_diff/test_dbf_fields.c -- host oracle for the S1.2
 * field-descriptor array decoder (dbf_field / dbf_field_t).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_dbf_header.c:
 * the seed test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), a host
 * PAL via pal_host_make (pal_host.c). A non-zero exit on any failed check keeps
 * `make test-dbase` from false-greening (Law 2: the oracle is the truth).
 *
 * Two tiers (plan Sec 2.A):
 *   Tier 0 (committed, operator-free): expected field descriptor arrays
 *     transcribed from the dbf.md sec 4/5 "Verification" byte-dumps and cross-
 *     checked against `python3 harness/diff/dbf_diff/dbf_ref.py --schema <file>`.
 *     These are our synthesized assertion manifests; each value is cited to a
 *     dbf.md section. They run against the REAL goldens opened through the PAL.
 *     Each absent golden causes a LOUD skip for that table; remaining tables run.
 *   Tier 1 (golden-diff): every named golden is opened; every field descriptor
 *     (name, type, field_len, dec_count) is asserted for all nfields descriptors.
 *     Out-of-range access is also tested (fail-loud NULL).
 *
 * Goldens base resolves from argv[1] (orchestrator passes the corpus path);
 * default "../dbase3-decomp". If a golden is absent a LOUD skip names it (the
 * fat-fault-rollback idiom) and the test proceeds -- NEVER a silent pass.
 *
 * Mutation hook (Rule 6): built with -DDBF_MUTATE_STRIDE, dbf.c decodes
 * descriptors with a 48-byte stride (the dBASE-7 form) instead of 32. Every
 * descriptor after the first is decoded from shifted bytes, so the decoded
 * name/type/length diverge from the golden -> the Tier-0 field-name and
 * field-type checks go RED (fields[1..] will carry garbage or wrong values).
 *
 * Compile + run (self-grade, host, NOT make):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbf.c os/samir/core/rt.c os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbf_fields.c -o /tmp/test_dbf_fields \
 *     && /tmp/test_dbf_fields ../dbase3-decomp
 *
 * Mutant (must go RED):
 *   gcc -std=c11 -Wall -Wextra -Werror -DDBF_MUTATE_STRIDE \
 *     -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbf.c os/samir/core/rt.c os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbf_fields.c -o /tmp/test_dbf_fields_mut ; \
 *     /tmp/test_dbf_fields_mut ../dbase3-decomp ; echo "mutant exit=$?"
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 4 (32-byte field descriptor
 *     + CLIENTS FIRSTNAME worked example) and sec 5 (field type codes C/N/D/L/M).
 *     "Verification" section: all field names, types, lengths, dec counts for
 *     CLIENTS, TOURS, TRAVEL, BANK, TAX are byte-confirmed there.
 *   - spec/samir/dbf_format.h (the LOCKED offset constants; DBF_DESC_STRIDE=32,
 *     DBF_DESC_NAME_SIZE=11, DBF_DESC_TERMINATOR=0x0D).
 *   - docs/plans/SAMIR-implementation-plan.md S1.2 oracle contract.
 *   - os/samir/include/samir/dbf.h (dbf_field_t, dbf_field, dbf_nfields).
 *   - os/samir/pal/pal_host.c (struct pal_host_cfg, pal_host_make/_free).
 *   - seed/test_assert.h (the harness idiom).
 *   - Cross-checked by: `python3 harness/diff/dbf_diff/dbf_ref.py --schema <file>`
 *     (independent reader; agreement = independence barrier for Tier-0 manifests).
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

/* ---- per-field expected descriptor (used in Tier-0 manifests) ---- */
typedef struct {
    const char *name;     /* NUL-terminated field name */
    char        type;     /* 'C', 'N', 'D', 'L', or 'M' */
    uint8_t     len;      /* field length in record */
    uint8_t     dec;      /* decimal count */
} field_expect;

/* ---- per-table manifest (Tier-0 + Tier-1) ---- */
typedef struct {
    const char         *relpath;   /* path relative to the goldens base (argv[1]) */
    const char         *label;     /* short name for messages */
    int                 nfields;   /* expected field count */
    const field_expect *fields;    /* expected field descriptors (nfields entries) */
} table_expect;

/*
 * Tier-0 field manifests.
 *
 * All values transcribed from ../dbase3-decomp/specs/file-formats/dbf.md
 * sec 4 "Verification" section + sec 5 (type codes), and cross-checked
 * independently against:
 *   python3 harness/diff/dbf_diff/dbf_ref.py --schema <file.DBF>
 * Every name/type/len/dec value below is byte-verified against the real golden.
 *
 * CLIENTS: sec 4 worked example (FIRSTNAME C(20) at offset 0x20) + Verification:
 *   FIRSTNAME C 20 0, LASTNAME C 20 0, ADDRESS C 25 0, CITY C 20 0,
 *   STATE C 2 0, ZIPCODE C 5 0, PHONE C 13 0.
 *   Note: ADDRESS is 25 bytes, not 20 -- the worked example in sec 4 shows only
 *   FIRSTNAME; full schema from Verification + dbf_ref.py.
 *   Ref: dbf.md sec 4 Verification + `dbf_ref.py --schema CLIENTS.DBF`.
 *
 * TOURS: sec 6 worked record (dbf.md sec 6: TRAVELCODE C4, TRAVELPLAN C40,
 *   DEPARTURE D8, UNITCOST N10/2, PERCENT N6/3, SEATS_OPEN N4/0, NOTE M10) +
 *   Verification.
 *   Ref: dbf.md sec 5 (D/N/M types), sec 6 (packed record), Verification.
 *
 * TRAVEL: 11 fields incl. L (PAID) and M (NOTES) -- dbf.md sec 5 L/M + Verification.
 *   Ref: dbf.md sec 5, Verification + `dbf_ref.py --schema TRAVEL.DBF`.
 *
 * BANK: +2 terminator form, nrec=0 (dbf.md sec 8 BANK special case).
 *   DATE D 8 0, AMT N 10 2, NUM N 2 0, NUMWITH N 2 0, CLEAR L 1 0.
 *   Ref: dbf.md sec 8 Inv2 BANK row + `dbf_ref.py --schema BANK.DBF`.
 *
 * TAX: +2 terminator form, 2 fields.
 *   CODE N 1 0, TITLE C 18 0.
 *   Ref: dbf.md sec 8 Inv2 TAX row + `dbf_ref.py --schema TAX.DBF`.
 */

static const field_expect CLIENTS_FIELDS[] = {
    /* Ref: dbf.md sec 4 worked example (FIRSTNAME C 20) + Verification. */
    { "FIRSTNAME", 'C', 20,  0 },
    { "LASTNAME",  'C', 20,  0 },
    { "ADDRESS",   'C', 25,  0 },
    { "CITY",      'C', 20,  0 },
    { "STATE",     'C',  2,  0 },
    { "ZIPCODE",   'C',  5,  0 },
    { "PHONE",     'C', 13,  0 },
};
#define CLIENTS_NFIELDS 7

static const field_expect TOURS_FIELDS[] = {
    /* Ref: dbf.md sec 6 worked record + sec 5 (D/N/M type codes) + Verification.
     * UNITCOST N(10,2): "   1378.00" (3 leading spaces, 2 decimal places).
     * PERCENT  N(6,3):  "10.000" (3 decimal places, field length 6).
     * SEATS_OPEN N(4,0): "  25" (no decimal point, field length 4).
     * NOTE M(10): 10-byte ASCII right-justified decimal block number (dbf.md sec 5 M). */
    { "TRAVELCODE", 'C',  4, 0 },
    { "TRAVELPLAN", 'C', 40, 0 },
    { "DEPARTURE",  'D',  8, 0 },
    { "UNITCOST",   'N', 10, 2 },
    { "PERCENT",    'N',  6, 3 },
    { "SEATS_OPEN", 'N',  4, 0 },
    { "NOTE",       'M', 10, 0 },
};
#define TOURS_NFIELDS 7

static const field_expect TRAVEL_FIELDS[] = {
    /* Ref: dbf.md sec 5 (L/M types), Verification + `dbf_ref.py --schema TRAVEL.DBF`.
     * PAID L(1): 1 byte 'T'/'F' (dbf.md sec 5 L).
     * NOTES M(10): 10-byte ASCII memo pointer (dbf.md sec 5 M). */
    { "FIRSTNAME",  'C', 20, 0 },
    { "LASTNAME",   'C', 20, 0 },
    { "PHONE",      'C', 13, 0 },
    { "TRAVELCODE", 'C',  4, 0 },
    { "TRAVELPLAN", 'C', 40, 0 },
    { "DEPARTURE",  'D',  8, 0 },
    { "COST",       'N', 10, 2 },
    { "PAID",       'L',  1, 0 },
    { "AGENT",      'C',  2, 0 },
    { "RESERVDATE", 'D',  8, 0 },
    { "NOTES",      'M', 10, 0 },
};
#define TRAVEL_NFIELDS 11

static const field_expect BANK_FIELDS[] = {
    /* Ref: dbf.md sec 4 Verification (BANK @0x20 DATE D len 8, @0x40 AMT N len 10 dec 2)
     * + dbf.md sec 8 BANK special case (nrec=0, +2 form) + `dbf_ref.py --schema BANK.DBF`. */
    { "DATE",    'D',  8, 0 },
    { "AMT",     'N', 10, 2 },
    { "NUM",     'N',  2, 0 },
    { "NUMWITH", 'N',  2, 0 },
    { "CLEAR",   'L',  1, 0 },
};
#define BANK_NFIELDS 5

static const field_expect TAX_FIELDS[] = {
    /* Ref: dbf.md sec 8 Inv2 TAX row (2 fields, +2 term, hlen=98, rlen=20)
     * + `dbf_ref.py --schema TAX.DBF`.
     * record_length 20 = 1 + CODE(1) + TITLE(18) = 1 + 1 + 18 = 20.
     * Ref: dbf.md sec 8 Invariant 1b. */
    { "CODE",  'N',  1, 0 },
    { "TITLE", 'C', 18, 0 },
};
#define TAX_NFIELDS 2

#define SP_PATH "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

static const table_expect MANIFEST[] = {
    { SP_PATH "/CLIENTS.DBF", "CLIENTS",
      CLIENTS_NFIELDS, CLIENTS_FIELDS },
    { SP_PATH "/TOURS.DBF",   "TOURS",
      TOURS_NFIELDS,   TOURS_FIELDS },
    { SP_PATH "/TRAVEL.DBF",  "TRAVEL",
      TRAVEL_NFIELDS,  TRAVEL_FIELDS },
    { SP_PATH "/BANK.DBF",    "BANK",
      BANK_NFIELDS,    BANK_FIELDS },
    { SP_PATH "/TAX.DBF",     "TAX",
      TAX_NFIELDS,     TAX_FIELDS },
};
#define MANIFEST_N ((int)(sizeof(MANIFEST) / sizeof(MANIFEST[0])))

/* ---- helpers ---- */

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

/*
 * check_table_fields: open one golden, assert nfields and each field descriptor.
 * Also asserts that dbf_field(tbl, nfields) (out of range) returns NULL (fail loud).
 */
static void check_table_fields(samir_pal_t *pal, const char *base,
                                const table_expect *te)
{
    char path[1024];
    char msg[256];
    dbf_table *tbl = NULL;
    const dbf_field_t *fd;
    int rc, i;

    join(path, sizeof(path), base, te->relpath);

    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): golden absent: %s -- pass corpus base as argv[1]\n",
                path);
        return;
    }

    rc = dbf_open(pal, path, &tbl);
    snprintf(msg, sizeof(msg), "%s: dbf_open succeeds (rc=%d)", te->label, rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || tbl == NULL)
        return;

    /* Tier-0: field count matches manifest. */
    snprintf(msg, sizeof(msg), "%s: nfields == %d", te->label, te->nfields);
    CHECK((int)dbf_nfields(tbl) == te->nfields, msg);

    /* Tier-0 + Tier-1: per-field name/type/len/dec. */
    for (i = 0; i < te->nfields; i++) {
        const field_expect *fe = &te->fields[i];
        fd = dbf_field(tbl, i);

        snprintf(msg, sizeof(msg), "%s: field[%d] not NULL", te->label, i);
        CHECK(fd != NULL, msg);
        if (fd == NULL)
            continue;

        /* Name: dbf.md sec 4 "read to first NUL; ignore trailing garbage". */
        snprintf(msg, sizeof(msg), "%s: field[%d].name == \"%s\" (got \"%s\")",
                 te->label, i, fe->name, fd->name);
        CHECK(strcmp(fd->name, fe->name) == 0, msg);

        /* Type: one of C/N/D/L/M (dbf.md sec 5). */
        snprintf(msg, sizeof(msg), "%s: field[%d].type == '%c' (got '%c')",
                 te->label, i, fe->type, fd->type);
        CHECK(fd->type == fe->type, msg);

        /* Field length (dbf.md sec 4 offset 0x10). */
        snprintf(msg, sizeof(msg), "%s: field[%d].field_len == %u (got %u)",
                 te->label, i, (unsigned)fe->len, (unsigned)fd->field_len);
        CHECK(fd->field_len == fe->len, msg);

        /* Decimal count (dbf.md sec 4 offset 0x11; 0 for C/D/L/M). */
        snprintf(msg, sizeof(msg), "%s: field[%d].dec_count == %u (got %u)",
                 te->label, i, (unsigned)fe->dec, (unsigned)fd->dec_count);
        CHECK(fd->dec_count == fe->dec, msg);
    }

    /* Out-of-range access: dbf_field(tbl, nfields) must return NULL (fail loud;
     * Rule 2 -- a caller that ignores the NULL contract has a bug). */
    fd = dbf_field(tbl, dbf_nfields(tbl));
    snprintf(msg, sizeof(msg),
             "%s: dbf_field(tbl, nfields=%d) is NULL (out-of-range fail-loud)",
             te->label, (int)dbf_nfields(tbl));
    CHECK(fd == NULL, msg);

    /* Also test negative index. */
    fd = dbf_field(tbl, -1);
    snprintf(msg, sizeof(msg), "%s: dbf_field(tbl, -1) is NULL", te->label);
    CHECK(fd == NULL, msg);

    rc = dbf_close(tbl);
    snprintf(msg, sizeof(msg), "%s: dbf_close succeeds (rc=%d)", te->label, rc);
    CHECK(rc == DBF_OK, msg);
}

int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    int i;
    int any_present = 0;
    char path[1024];

    /* Injected fixed date (Rule 11; the PAL requires a date even though S1.2
     * does not use it). Arena sized generously: the largest table (TRAVEL, 11
     * fields) needs ~11*sizeof(dbf_field_t) + sizeof(dbf_table), well under 64k. */
    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy    = 99;
    cfg.date_mm    = 12;
    cfg.date_dd    = 31;
    cfg.heap_size  = 64u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* ---- Tier 0 + Tier 1: every golden, each loud-skipped if absent ---- */
    for (i = 0; i < MANIFEST_N; i++) {
        if (file_exists(join(path, sizeof(path), base, MANIFEST[i].relpath)))
            any_present = 1;
        check_table_fields(pal, base, &MANIFEST[i]);
    }

    if (!any_present) {
        fprintf(stderr,
                "  SKIP (LOUD): no goldens found under base '%s'\n"
                "               expected e.g. %s/%s\n"
                "               (pass the corpus base as argv[1]; mutation hook"
                " still covers the stride perturbation)\n",
                base, base, MANIFEST[0].relpath);
    }

    pal_host_free(pal);
    return TEST_SUMMARY("test-dbf-fields");
}
