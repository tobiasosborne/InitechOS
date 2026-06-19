/*
 * harness/diff/dbf_diff/mint_clients_dbf.c -- deterministic CLIENTS.DBF minter
 * for the S8.2 in-emulator boot->USE->LIST gate (bead initech-hdlb; ADR-0009
 * DEC-08).
 *
 * FACTORY host tool (CLAUDE.md Law 3): libc OK here. It links the SAMIR host
 * dbf writer (dbf.c + value.c + rt.c) over the host PAL (pal_host.c) and emits
 * one deterministic .dbf to a caller-supplied path (argv[1], default
 * build/CLIENTS.DBF). This is the SAME writer the engine ships, so the fixture
 * is read back BIT-FOR-BIT by the in-emulator SAMIR.COM reader -- no second
 * .dbf encoder to drift (Rule 6 spirit: one writer, mutation-provable).
 *
 * Determinism (Rule 11): identical inputs + an INJECTED last-update date
 * (pal_host date_yy/_mm/_dd) => byte-identical output every run. No wall clock,
 * no host paths, no timestamps baked into the file. The minter is run by the
 * Makefile FAT-image recipe; the .dbf is a build intermediate, NOT committed.
 *
 * THE FIXTURE (CLIENTS.DBF) -- a tiny, all-ASCII, distinctive table so the
 * LIST rows are trivially asserted on serial + screendump:
 *
 *   Schema (3 fields):
 *     NAME  C(10)
 *     CITY  C(12)
 *     BAL   N(8,2)
 *   record_length = 1 (delete flag) + 10 + 12 + 8 = 31
 *   header_length = 32 + 3*32 + 1 = 129
 *
 *   Records (3, all live):
 *     recno  NAME        CITY          BAL
 *       1    PESTON      HONOLULU      1234.50
 *       2    WADDAMS     AUSTIN         -42.00
 *       3    LUMBERGH    DALLAS        7777.77
 *
 * The names/cities are unique tokens (PESTON/WADDAMS/LUMBERGH,
 * HONOLULU/AUSTIN/DALLAS) so a `grep -F` on the serial transcript is an exact,
 * non-fragile signal; the BAL values (1234.50 / -42.00 / 7777.77) exercise the
 * soft-float N formatting on-target.
 *
 * dBASE LIST renders each row as a right-justified recno column (width 8) then
 * the field columns, e.g. "       1 PESTON     HONOLULU      1234.50"
 * (see harness/diff/dbf_diff/test_interp_list.c). The gate asserts the field
 * tokens, not exact column math, so the fixture stays robust to spacing.
 *
 * Ref (Law 1):
 *   - os/samir/include/samir/dbf.h S1.4 (dbf_create / dbf_append_rec / dbf_flush;
 *     +1 terminator form; injected date; deterministic byte layout).
 *   - os/samir/include/samir/value.h (xb_c / xb_n).
 *   - os/samir/pal/pal_host.c (pal_host_make cfg: injected date, heap_size).
 *   - harness/diff/dbf_diff/test_dbf_roundtrip.c (the dbf_create+append+flush
 *     pattern this mirrors).
 *   - ADR-0009 DEC-08 (the S8.2 oracle; the fixture must be deterministic).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "samir/dbf.h"     /* -Ios/samir/include */
#include "samir/value.h"   /* xb_c / xb_n */

/* pal_host.c surface (not declared in a header; declare what we use). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* Fixed injected last-update date (Rule 11). 1993-07-19 -- a period-plausible
 * deterministic date; never a wall clock. YY = year-1900 = 93. */
#define MINT_YY  93
#define MINT_MM   7
#define MINT_DD  19

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "build/CLIENTS.DBF";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    dbf_table *tbl = NULL;
    int rc;
    xb_val rec[3];

    /* Schema: NAME C(10), CITY C(12), BAL N(8,2). */
    dbf_field_spec fields[3] = {
        { "NAME", 'C', 10u, 0u },
        { "CITY", 'C', 12u, 0u },
        { "BAL",  'N',  8u, 2u }
    };

    memset(&cfg, 0, sizeof cfg);
    cfg.date_yy = MINT_YY;
    cfg.date_mm = MINT_MM;
    cfg.date_dd = MINT_DD;
    cfg.heap_size = 0;             /* default arena */

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "mint_clients_dbf: pal_host_make failed\n");
        return 2;
    }

    rc = dbf_create(pal, path, fields, 3, &tbl);
    if (rc != DBF_OK || !tbl) {
        fprintf(stderr, "mint_clients_dbf: dbf_create('%s') rc=%d\n", path, rc);
        pal_host_free(pal);
        return 3;
    }

    /* Record 1: PESTON / HONOLULU / 1234.50 */
    rec[0] = xb_c("PESTON", 6u);
    rec[1] = xb_c("HONOLULU", 8u);
    rec[2] = xb_n(1234.50);
    rc = dbf_append_rec(tbl, rec, 0);
    if (rc != DBF_OK) { fprintf(stderr, "append rec1 rc=%d\n", rc); pal_host_free(pal); return 4; }

    /* Record 2: WADDAMS / AUSTIN / -42.00 */
    rec[0] = xb_c("WADDAMS", 7u);
    rec[1] = xb_c("AUSTIN", 6u);
    rec[2] = xb_n(-42.00);
    rc = dbf_append_rec(tbl, rec, 0);
    if (rc != DBF_OK) { fprintf(stderr, "append rec2 rc=%d\n", rc); pal_host_free(pal); return 5; }

    /* Record 3: LUMBERGH / DALLAS / 7777.77 */
    rec[0] = xb_c("LUMBERGH", 8u);
    rec[1] = xb_c("DALLAS", 6u);
    rec[2] = xb_n(7777.77);
    rc = dbf_append_rec(tbl, rec, 0);
    if (rc != DBF_OK) { fprintf(stderr, "append rec3 rc=%d\n", rc); pal_host_free(pal); return 6; }

    rc = dbf_flush(tbl);
    if (rc != DBF_OK) { fprintf(stderr, "flush rc=%d\n", rc); pal_host_free(pal); return 7; }

    dbf_close(tbl);
    pal_host_free(pal);

    fprintf(stderr, ">>> mint_clients_dbf: wrote %s (3 fields, 3 records, date %02d-%02d-%02d)\n",
            path, MINT_MM, MINT_DD, MINT_YY);
    return 0;
}
