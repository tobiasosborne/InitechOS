/*
 * harness/proptest/test_stdfile.c -- FLAIR Standard File (SFGetFile) host
 * oracle (initech-gymo).
 *
 * beads:  initech-gymo (FLAIR Phase 4.5 -- SFGetFile first cut).
 * Ref:    stdfile.h / stdfile.c (the API under test).
 *         IM-I Standard File Package (SFGetFile / SFReply): a modal list of the
 *         current directory; Open returns on a file and descends on a folder;
 *         SFReply.good / SFReply.fName.
 *         CLAUDE.md Law 2 (oracle truth -- NOT by-construction), Rule 1
 *         (Red->Green->Refactor), Rule 6 (mutation-proven), Rule 12 (ASCII).
 *
 * ANTI-BY-CONSTRUCTION (Law 2, the central discipline):
 *   The INDEPENDENT golden is a hand-authored FIXTURE PROVIDER (the directory
 *   source the SUT navigates) PLUS an op/expected sequence whose EXPECTED values
 *   are LITERAL constants in THIS file -- never read back from the
 *   FlairStandardFile / FlairList internals. The data flows:
 *
 *      fixture provider (golden)  ->  SUT (open/navigate/select)  ->  FlairList
 *      cell  ->  read back through the SUT  ->  compared to a LITERAL.
 *
 *   So a wrong row, a missing re-enumeration, or a wrong returned entry shows up
 *   as a mismatch against an independent literal. (Same pattern as test_list.c:
 *   "Drive A" is both injected and checked, but the EXPECTED side is a literal.)
 *
 *   Independent fixture (the only directory truth in this test):
 *     "/"      -> [ "REPORT.DBF"(file), "DATA"(dir), "TPS.TXT"(file) ]
 *     "/DATA"  -> [ "Q1.WKS"(file), "Q2.WKS"(file) ]
 *
 * COORDINATE CONVENTION (from list.h; do NOT use positional initializers):
 *   Cell = flair_point_t { int16_t v; int16_t h; }; Cell.h = COLUMN, .v = ROW.
 *   Pixel point likewise: flair_point_t { .v = y, .h = x }.
 *
 * MUTANT COVERAGE (Rule 6 -- each mutant must COMPILE and go RED):
 *   SF_MUT_RETURN_WRONG_CELL  doOpen returns entry 0 regardless of selection ->
 *                             step 5 returns "REPORT.DBF" not "TPS.TXT" -> RED.
 *   SF_MUT_NO_NAVIGATE        navigate does not re-enumerate -> step 3 count
 *                             stays 3 / row 0 != "Q1.WKS" -> RED.
 *   SF_MUT_CANCEL_GOOD        doCancel sets good=1 -> step 6 good != 0 -> RED.
 *
 * Compile (host, green):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *      -Ispec -Ios/flair -Ios/flair/atkinson -Iseed \
 *      -o /tmp/laneF/test_stdfile \
 *      harness/proptest/test_stdfile.c os/flair/stdfile.c os/flair/list.c \
 *      os/flair/heap.c os/flair/atkinson/region.c
 *
 * Compile each mutant (each must exit non-zero = RED):
 *   cc ... -DSF_MUT_RETURN_WRONG_CELL -o /tmp/laneF/test_stdfile_mut_cell
 *   cc ... -DSF_MUT_NO_NAVIGATE       -o /tmp/laneF/test_stdfile_mut_nav
 *   cc ... -DSF_MUT_CANCEL_GOOD       -o /tmp/laneF/test_stdfile_mut_cancel
 *
 * Kernel type-check (compile only, no link):
 *   gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra -Werror \
 *       -Ispec -Ios/milton -Ios/flair -Ios/flair/atkinson \
 *       -c os/flair/stdfile.c
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stdfile.h"      /* FlairStandardFile, FlairSF* API   (-Ios/flair)     */
#include "test_assert.h"  /* TEST_HARNESS / CHECK / TEST_SUMMARY (-Iseed)       */

TEST_HARNESS();

/* --------------------------------------------------------------------------
 * The INDEPENDENT fixture provider (the golden directory source).
 *
 * This stands in for fat12_read_root_dir + fat12_format_83 on the real system.
 * It is the ONLY directory truth the SUT sees; the SUT must faithfully move its
 * entries into the FlairList and navigate between its two directories.
 * -------------------------------------------------------------------------- */
static void fx_set(FlairSFEntry *e, const char *name, uint8_t is_dir,
                   uint32_t size)
{
    /* host-side helper: libc string.h is fine in the harness (Law 3 forbids it
     * only in the ARTIFACT, stdfile.c). */
    memset(e->name, 0, sizeof(e->name));
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->is_dir = is_dir;
    e->size   = size;
}

static int fixture_provider(const char *path, FlairSFEntry *out, int16_t max,
                            void *user)
{
    (void)user;
    if (strcmp(path, "/") == 0) {
        if (max < 3) return -1;
        fx_set(&out[0], "REPORT.DBF", 0, 2048);
        fx_set(&out[1], "DATA",       1, 0);
        fx_set(&out[2], "TPS.TXT",    0, 512);
        return 3;
    }
    if (strcmp(path, "/DATA") == 0) {
        if (max < 2) return -1;
        fx_set(&out[0], "Q1.WKS", 0, 100);
        fx_set(&out[1], "Q2.WKS", 0, 200);
        return 2;
    }
    return -1;   /* unknown path -> provider error */
}

/* row_equals: read FlairList row `row` (column 0) back THROUGH the SUT's list
 * and compare its bytes to the LITERAL `want` (independent golden). */
static int row_equals(const FlairStandardFile *sf, int16_t row,
                      const char *want)
{
    uint8_t buf[FLAIR_SF_NAME_MAX + 8];
    int16_t n;
    int16_t wlen = (int16_t)strlen(want);
    Cell    c;
    c.h = 0;
    c.v = row;

    memset(buf, 0, sizeof(buf));
    n = FlairLGetCell(&sf->list, buf, (int16_t)sizeof(buf), c);
    if (n != wlen) {
        return 0;
    }
    return memcmp(buf, want, (size_t)wlen) == 0;
}

int main(void)
{
    static FlairStandardFile sf;   /* ~17 KB: keep it in BSS, not the stack     */
    FlairSFReply reply;
    Cell         cs;
    rgn_rect_t   rv;
    int          r;
    int16_t      n;

    /* List geometry -- LITERALS (NOT derived from any SUT state).
     *   cellSize: h=200 px wide, v=16 px high.
     *   rView:    top=40, left=20, bottom=40+3*16=88, right=20+200=220. */
    cs.h = 200;
    cs.v = 16;
    rv.top    = 40;
    rv.left   = 20;
    rv.bottom = 88;
    rv.right  = 220;

    /* -----------------------------------------------------------------------
     * STEP 1: FlairSF_open at "/" -> count==3; 3 list rows; row 0 == "REPORT.DBF"
     *         (literal), row 1 == "DATA" (literal).
     * ----------------------------------------------------------------------- */
    r = FlairSF_open(&sf, fixture_provider, NULL, cs, rv);
    CHECK(r == FLAIR_SF_OK, "step-1: FlairSF_open(\"/\") returns FLAIR_SF_OK");

    n = FlairSF_count(&sf);
    CHECK(n == 3, "step-1: count == 3 at root (literal)");

    /* The FlairList must have 3 rows (dataBounds.bottom is the row count). */
    CHECK(sf.list.dataBounds.bottom == 3,
          "step-1: FlairList has 3 rows (dataBounds.bottom == 3, literal)");
    CHECK(sf.list.dataBounds.right == 1,
          "step-1: FlairList is single-column (dataBounds.right == 1, literal)");

    /* Row cells equal the independent literals (read back through the list). */
    CHECK(row_equals(&sf, 0, "REPORT.DBF"),
          "step-1: list row 0 cell == \"REPORT.DBF\" (literal bytes)");
    CHECK(row_equals(&sf, 1, "DATA"),
          "step-1: list row 1 cell == \"DATA\" (literal bytes)");
    CHECK(row_equals(&sf, 2, "TPS.TXT"),
          "step-1: list row 2 cell == \"TPS.TXT\" (literal bytes)");

    /* -----------------------------------------------------------------------
     * STEP 2: select(0); doOpen -> good==1, fName=="REPORT.DBF" (literal),
     *         is_dir==0.
     * ----------------------------------------------------------------------- */
    FlairSF_select(&sf, 0);
    memset(&reply, 0xCC, sizeof(reply));
    FlairSF_doOpen(&sf, &reply);

    CHECK(reply.good == 1, "step-2: doOpen on a file -> reply.good == 1");
    CHECK_STR_EQ(reply.fName, "REPORT.DBF",
                 "step-2: reply.fName == \"REPORT.DBF\" (literal)");
    CHECK(reply.is_dir == 0, "step-2: reply.is_dir == 0 (a file)");

    /* -----------------------------------------------------------------------
     * STEP 3: navigate(1) into the "DATA" folder -> count==2; row 0 == "Q1.WKS";
     *         select(0); doOpen -> fName=="Q1.WKS".
     *   FAILS under SF_MUT_NO_NAVIGATE (count stays 3 / row 0 == "REPORT.DBF").
     * ----------------------------------------------------------------------- */
    r = FlairSF_navigate(&sf, 1);
    CHECK(r == FLAIR_SF_OK, "step-3: FlairSF_navigate(1) [DATA] returns OK");

    n = FlairSF_count(&sf);
    CHECK(n == 2,
          "step-3: count == 2 inside DATA (literal) -- FAILS SF_MUT_NO_NAVIGATE");
    CHECK(row_equals(&sf, 0, "Q1.WKS"),
          "step-3: list row 0 == \"Q1.WKS\" -- FAILS SF_MUT_NO_NAVIGATE");
    CHECK(row_equals(&sf, 1, "Q2.WKS"),
          "step-3: list row 1 == \"Q2.WKS\" (literal)");

    FlairSF_select(&sf, 0);
    memset(&reply, 0xCC, sizeof(reply));
    FlairSF_doOpen(&sf, &reply);
    CHECK(reply.good == 1, "step-3: doOpen on Q1.WKS -> good == 1");
    CHECK_STR_EQ(reply.fName, "Q1.WKS",
                 "step-3: reply.fName == \"Q1.WKS\" (literal)");

    /* -----------------------------------------------------------------------
     * STEP 4: up -> back at "/", count==3 again; row 0 == "REPORT.DBF".
     * ----------------------------------------------------------------------- */
    r = FlairSF_up(&sf);
    CHECK(r == FLAIR_SF_OK, "step-4: FlairSF_up() returns OK");

    n = FlairSF_count(&sf);
    CHECK(n == 3, "step-4: count == 3 back at root (literal)");
    CHECK(row_equals(&sf, 0, "REPORT.DBF"),
          "step-4: list row 0 == \"REPORT.DBF\" again (literal)");

    /* -----------------------------------------------------------------------
     * STEP 5: click a local point hitting ROW 2 -> selects "TPS.TXT";
     *         doOpen -> fName=="TPS.TXT".
     *   Local point (LITERAL arithmetic, NOT from SUT state):
     *     v = rView.top(40) + 2*cellSize.v(16) + 4 = 76  (in [72,88) -> row 2)
     *     h = rView.left(20) + 10               = 30  (in [20,220) -> col 0)
     *     row_off = (76-40)/16 = 2; col_off = (30-20)/200 = 0 -> cell (h=0,v=2).
     *   FAILS under SF_MUT_RETURN_WRONG_CELL (returns entry 0 "REPORT.DBF").
     * ----------------------------------------------------------------------- */
    {
        flair_point_t pt;
        pt.v = 76;   /* y -> row 2 */
        pt.h = 30;   /* x -> col 0 */

        r = FlairSF_click(&sf, pt);
        CHECK(r == 2, "step-5: FlairSF_click(row 2) returns hit row 2 (literal)");

        memset(&reply, 0xCC, sizeof(reply));
        FlairSF_doOpen(&sf, &reply);
        CHECK(reply.good == 1, "step-5: doOpen on TPS.TXT -> good == 1");
        CHECK_STR_EQ(reply.fName, "TPS.TXT",
                     "step-5: reply.fName == \"TPS.TXT\" "
                     "-- FAILS SF_MUT_RETURN_WRONG_CELL");
        CHECK(reply.is_dir == 0, "step-5: reply.is_dir == 0 (a file)");
    }

    /* -----------------------------------------------------------------------
     * STEP 6: doCancel -> reply.good == 0.
     *   FAILS under SF_MUT_CANCEL_GOOD (good == 1).
     * ----------------------------------------------------------------------- */
    memset(&reply, 0xCC, sizeof(reply));
    FlairSF_doCancel(&sf, &reply);
    CHECK(reply.good == 0,
          "step-6: doCancel -> reply.good == 0 -- FAILS SF_MUT_CANCEL_GOOD");

    /* -----------------------------------------------------------------------
     * STEP 7: doOpen with a FOLDER selected -> the documented IM folder rule:
     *         NAVIGATE into the folder, leave reply.good == 0 (not-confirmed).
     *   We are at root (3 entries). Select row 1 ("DATA", a folder), doOpen:
     *     reply.good == 0 (not confirmed) AND count becomes 2 (descended into
     *     DATA) AND row 0 == "Q1.WKS".
     *   ALSO FAILS under SF_MUT_NO_NAVIGATE (doOpen's navigate is the no-op,
     *   count stays 3) -- a second independent witness for that mutant.
     * ----------------------------------------------------------------------- */
    FlairSF_select(&sf, 1);            /* "DATA" -- the folder */
    memset(&reply, 0xCC, sizeof(reply));
    FlairSF_doOpen(&sf, &reply);

    CHECK(reply.good == 0,
          "step-7: doOpen on a FOLDER -> good == 0 (navigate, not confirm; IM)");
    CHECK(reply.is_dir == 1, "step-7: reply.is_dir == 1 (the folder)");
    CHECK_STR_EQ(reply.fName, "DATA",
                 "step-7: reply.fName == \"DATA\" (the navigated folder)");

    n = FlairSF_count(&sf);
    CHECK(n == 2,
          "step-7: doOpen-on-folder DESCENDED -> count == 2 (inside DATA)");
    CHECK(row_equals(&sf, 0, "Q1.WKS"),
          "step-7: after folder-Open, row 0 == \"Q1.WKS\" (descended into DATA)");

    return TEST_SUMMARY("test-stdfile");
}
