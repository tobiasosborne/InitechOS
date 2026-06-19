/*
 * harness/diff/dbf_diff/mint_invoice_dbf.c -- deterministic INVOICE.DBF minter
 * for the S7.1 CANON in-emulator gate (initech-9a0f): the Initech AR aging app
 * (Y2KACCT.PRG) and its ENFORCED Year-2000 bug (bead 586.1) running INSIDE
 * InitechOS on QEMU.
 *
 * FACTORY host tool (CLAUDE.md Law 3): libc OK here. It links the SAMIR host dbf
 * writer (dbf.c + value.c + rt.c) over the host PAL (pal_host.c) and emits one
 * deterministic INVOICE.DBF to a caller-supplied path (argv[1], default
 * build/INVOICE.DBF). This is the SAME writer the engine ships, so the fixture is
 * read back BIT-FOR-BIT by the in-emulator SAMIR.COM reader -- one writer, no
 * second encoder to drift (mirrors mint_clients_dbf.c).
 *
 * THE CANON BUG IS SEEDED HERE (Law 4 -- "the *wrong* rollover matches real
 * dBASE; enforced, not fixed"). The schema + records + Y2K data-entry rule MIRROR
 * test_canon_y2k.c make_invoices EXACTLY so the in-emu `DO Y2KACCT` run produces
 * the IDENTICAL buggy aging report the host gate test-canon-y2k asserts:
 *
 *   Schema (5 fields; reusable AR ledger -- see canon/y2k_accounting.prg):
 *     INVNO    C(5)
 *     CUST     C(10)
 *     AMOUNT   N(10,2)
 *     DUEDATE  D
 *     PAID     L
 *
 *   Records (4), DUEDATE keyed two-digit MM/DD/YY (the clerk's data entry):
 *     A1001 12/15/99 (1999)  1250.00  PAID=.F.
 *     A1002 01/05/00 (2000)   875.50  PAID=.F.   <- keyed "00" -> stored BASE-1900
 *     A1003 11/30/99 (1999)  4400.00  PAID=.T.
 *     A1004 02/10/00 (2000)    99.99  PAID=.F.   <- keyed "00" -> stored BASE-1900
 *
 * THE Y2K STORE RULE (the bug, enforced): a year-2000 due date keyed "00" is
 * parsed BASE-1900 by CTOD under SET CENTURY OFF (os/samir/core/fn_builtins.c
 * fn_ctod_impl: "if (yearlen == 2) yy += 1900;"), so it is STORED as a 1900 date.
 * We replicate exactly that at data entry: year 2000 -> jdn_from_ymd(1900, ..).
 * The aging arithmetic (AGE = ASOF - DUEDATE) is then wrong by ~100 years, and
 * the report under-reports overdue debt -- the genuine period failure mode.
 *
 *   -DMINT_INVOICE_Y2K_FIXED (the DO-read-independent canon mutant, Rule 6): store
 *     the year-2000 due dates with their CORRECT century. The aging then becomes
 *     correct, the 1999 invoices flag OVERDUE, and the in-emu report no longer
 *     matches the buggy canon values -> test-samir-canon-y2k goes RED. A "fix" to
 *     the rollover breaks the gate (Law 4); this is the data-path twin of the host
 *     gate's -DCANON_Y2K_FIXED.
 *
 * Determinism (Rule 11): identical inputs + an INJECTED last-update date =>
 * byte-identical output every run. No wall clock, no host paths. The .dbf is a
 * build intermediate, NOT committed.
 *
 * Ref (Law 1):
 *   - harness/diff/dbf_diff/test_canon_y2k.c make_invoices (the schema + records +
 *     Y2K store rule this MIRRORS bit-for-bit -- one canon, two seed paths).
 *   - harness/diff/dbf_diff/mint_clients_dbf.c (the dbf_create+append+flush +
 *     injected-date pattern this follows).
 *   - os/samir/include/samir/{dbf.h,value.h,rt.h} (dbf_create, xb_c/n/d/l, jdn).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "samir/dbf.h"     /* dbf_create / dbf_append_rec / dbf_flush / dbf_close */
#include "samir/value.h"   /* xb_c / xb_n / xb_d / xb_l */
#include "samir/rt.h"      /* jdn_from_ymd */

/* pal_host.c surface (not declared in a header; declare what we use). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* Fixed injected last-update date (Rule 11): 1993-07-19 (matches the sibling
 * mint_clients_dbf.c). YY = year-1900 = 93. Never a wall clock. */
#define MINT_YY  93
#define MINT_MM   7
#define MINT_DD  19

/* The 4 INVOICE records -- IDENTICAL to test_canon_y2k.c make_invoices. */
#define INV_N 4
static const char *INV_NO[INV_N]   = { "A1001", "A1002", "A1003", "A1004" };
static const char *INV_CUST[INV_N] = { "INITROingp", "ACME CORP ", "GLOBEX INC", "SOYLENT CO" };
static const double INV_AMT[INV_N]  = { 1250.00,  875.50,  4400.00,   99.99 };
static const int INV_YEAR[INV_N]   = { 1999,  2000,  1999,  2000 };
static const int INV_MON [INV_N]   = {   12,     1,    11,     2 };
static const int INV_DAY [INV_N]   = {   15,     5,    30,    10 };
static const char INV_PAID[INV_N]  = { 'F',   'F',   'T',   'F' };

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "build/INVOICE.DBF";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    dbf_table *tbl = NULL;
    int rc, i;

    dbf_field_spec fields[5] = {
        { "INVNO",   'C',  5u, 0u },
        { "CUST",    'C', 10u, 0u },
        { "AMOUNT",  'N', 10u, 2u },
        { "DUEDATE", 'D',  8u, 0u },
        { "PAID",    'L',  1u, 0u }
    };

    memset(&cfg, 0, sizeof cfg);
    cfg.date_yy = MINT_YY;
    cfg.date_mm = MINT_MM;
    cfg.date_dd = MINT_DD;
    cfg.heap_size = 0;             /* default arena */

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "mint_invoice_dbf: pal_host_make failed\n");
        return 2;
    }

    rc = dbf_create(pal, path, fields, 5, &tbl);
    if (rc != DBF_OK || !tbl) {
        fprintf(stderr, "mint_invoice_dbf: dbf_create('%s') rc=%d\n", path, rc);
        pal_host_free(pal);
        return 3;
    }

    for (i = 0; i < INV_N; i++) {
        xb_val r[5];
        int store_year = INV_YEAR[i];
        r[0] = xb_c(INV_NO[i], 5);
        r[1] = xb_c(INV_CUST[i], 10);
        r[2] = xb_n(INV_AMT[i]);
#ifdef MINT_INVOICE_Y2K_FIXED
        /* MUTANT: data entry stores the TRUE century for year-2000 dates -> the
         * aging becomes correct -> the in-emu report no longer matches canon. */
        /* (store_year stays the true year) */
#else
        /* CANON BUG: a year-2000 date keyed "00" parses base-1900 (CTOD rule). */
        if (store_year == 2000) store_year = 1900;
#endif
        r[3] = xb_d((double)jdn_from_ymd(store_year, INV_MON[i], INV_DAY[i]));
        r[4] = xb_l(INV_PAID[i] == 'T');
        rc = dbf_append_rec(tbl, r, 0);
        if (rc != DBF_OK) {
            fprintf(stderr, "mint_invoice_dbf: append rec %d rc=%d\n", i, rc);
            dbf_close(tbl); pal_host_free(pal); return 4;
        }
    }

    rc = dbf_flush(tbl);
    if (rc != DBF_OK) {
        fprintf(stderr, "mint_invoice_dbf: flush rc=%d\n", rc);
        dbf_close(tbl); pal_host_free(pal); return 5;
    }

    dbf_close(tbl);
    pal_host_free(pal);

    fprintf(stderr, ">>> mint_invoice_dbf: wrote %s (5 fields, %d records, "
                    "Y2K data-entry bug %s, date %02d-%02d-%02d)\n",
            path, INV_N,
#ifdef MINT_INVOICE_Y2K_FIXED
            "FIXED (mutant)",
#else
            "ENFORCED (canon)",
#endif
            MINT_MM, MINT_DD, MINT_YY);
    return 0;
}
