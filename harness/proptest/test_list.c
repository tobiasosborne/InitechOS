/*
 * harness/proptest/test_list.c -- FLAIR List Manager host oracle (initech-77dj).
 *
 * beads:  initech-77dj (FLAIR Phase 4.5 -- List Manager first cut).
 * Ref:    list.h / list.c (the API under test).
 *         list-manager.md Sec 1-5 (ListRec, Cell/ListBounds, selFlags, API).
 *         spec/grafport.h: flair_point_t has v (row) at offset 0, h (col) at 2.
 *         Cell = flair_point_t: .h = COLUMN, .v = ROW [list-manager.md Sec 2].
 *         CLAUDE.md Law 2 (oracle truth -- NOT by-construction), Rule 1
 *         (Red->Green->Refactor), Rule 6 (mutation-proven), Rule 12 (ASCII).
 *
 * COORDINATE CONVENTION (critical -- do NOT use positional initializers):
 *   Cell   = flair_point_t { int16_t v; int16_t h; }   (v is FIRST in struct)
 *   Cell.h = COLUMN, Cell.v = ROW [list-manager.md Sec 2].
 *   Always use designated initializers: (Cell){ .h=col, .v=row }.
 *   Example: cell at col=0, row=2  -->  (Cell){ .h=0, .v=2 }  =  { v=2, h=0 }.
 *   flair_point_t pixel points likewise: { .v=y_pixels, .h=x_pixels }.
 *
 * ANTI-BY-CONSTRUCTION (Law 2, the central discipline):
 *   Expected bytes are LITERAL CONSTANTS in this file -- NOT read back from
 *   FlairList struct internals.  The independent golden:
 *     "Drive A" == { 0x44,0x72,0x69,0x76,0x65,0x20,0x41 }  (7 bytes)
 *     "Trash"   == { 0x54,0x72,0x61,0x73,0x68 }             (5 bytes)
 *   Cell addresses in the hit test:  h==0, v==1  (spelled as integer literals).
 *   dataBounds after LAddRow(3):  bottom==3, right==1  (integer literals).
 *
 * MUTANT COVERAGE (Rule 6 -- each mutant must COMPILE and go RED):
 *
 *   LIST_MUT_CELL_INDEX_SWAP  [FlairLSetCell write path swapped]
 *     Write to "Trash" at Cell{.h=0,.v=2} goes to cells[c.h=0][c.v=2]=cells[0][2].
 *     GetCell canonical reads cells[c.v=2][c.h=0]=cells[2][0] (unwritten -> 0).
 *     -> step 2b CHECK(r16 == 5) FAILS (r16==0) -> RED.
 *
 *   LIST_MUT_HIT_OFFBYONE  [FlairLClick omits rView.top/left]
 *     Pixel (h=15,v=38), rView.top=20, cellSize.v=16:
 *     canonical row=(38-20)/16=1; mutant row=38/16=2.
 *     -> step 4a CHECK(hit.v == 1) FAILS (hit.v==2) -> RED.
 *
 *   LIST_MUT_NO_DESELECT  [lOnlyOne skips deselect loop]
 *     After clicking (0,0) then (0,2), (0,0) must be deselected.
 *     -> step 5b CHECK(FlairLGetSelect((0,0)) == 0) FAILS (returns 1) -> RED.
 *
 * Compile (host, green):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *      -Ispec -Ios/flair -Ios/flair/atkinson -Iseed \
 *      -o /tmp/laneE/test_list \
 *      harness/proptest/test_list.c os/flair/list.c os/flair/heap.c
 *
 * Compile each mutant (each must exit non-zero = RED):
 *   cc ... -DLIST_MUT_CELL_INDEX_SWAP  -o /tmp/laneE/test_list_mut_swap
 *   cc ... -DLIST_MUT_HIT_OFFBYONE     -o /tmp/laneE/test_list_mut_obo
 *   cc ... -DLIST_MUT_NO_DESELECT      -o /tmp/laneE/test_list_mut_nosel
 *
 * Kernel type-check (compile only, no link):
 *   gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra -Werror \
 *       -Ispec -Ios/milton -Ios/flair -Ios/flair/atkinson \
 *       -c os/flair/list.c
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"         /* FlairList, Cell, FLAIR_LSEL_* etc.  (-Ios/flair)  */
#include "test_assert.h"  /* TEST_HARNESS / CHECK / TEST_SUMMARY (-Iseed)       */

TEST_HARNESS();

int main(void)
{
    /*
     * Static storage: FlairList holds a ~16 KB cell store in fixed arrays.
     * Declare static to keep it in BSS rather than the stack.
     */
    static FlairList l;
    uint8_t  buf[128];
    int16_t  r16;
    int      r;
    Cell     hit;

    /* -----------------------------------------------------------------------
     * STEP 1: FlairList_init; LAddRow(3, -1) -> dataBounds literal checks.
     *
     * rView pixel geometry (all literals -- NOT derived from FlairList state):
     *   cellSize: h=100 (pixel width), v=16 (pixel height).
     *   rView:    top=20, left=10, bottom=20+3*16=68, right=10+100=110.
     *
     * Cell coord golden: 3 rows (bottom==3), 1 auto-col (right==1).
     * ----------------------------------------------------------------------- */
    {
        /* flair_point_t { v; h }: .v=pixel-height=16, .h=pixel-width=100. */
        Cell cs;
        cs.v = 16;    /* cell pixel height */
        cs.h = 100;   /* cell pixel width  */

        /* rgn_rect_t { top, left, bottom, right }. */
        rgn_rect_t rv;
        rv.top    = 20;
        rv.left   = 10;
        rv.bottom = 68;   /* 20 + 3*16 */
        rv.right  = 110;  /* 10 + 100  */

        FlairList_init(&l, cs, rv);

        r = FlairLAddRow(&l, 3, -1);
        CHECK(r == FLAIR_LIST_OK,
              "step-1: LAddRow(3,-1) returns FLAIR_LIST_OK");

        /* Literal expected values -- NOT derived from pre-add state. */
        CHECK(l.dataBounds.bottom == 3,
              "step-1: dataBounds.bottom == 3 (literal) after LAddRow(3)");
        CHECK(l.dataBounds.top == 0,
              "step-1: dataBounds.top == 0 (origin)");
        CHECK(l.dataBounds.right == 1,
              "step-1: dataBounds.right == 1 (auto single-column)");
        CHECK(l.dataBounds.left == 0,
              "step-1: dataBounds.left == 0 (origin)");
    }

    /* -----------------------------------------------------------------------
     * STEP 2a: LSetCell + LGetCell -- cell (col=0, row=0) = "Drive A" (7 B).
     *
     * Cell address: col=0, row=0 -> Cell{ .h=0, .v=0 }.
     * (Cell{ .h=0, .v=0 } same as positional {0,0} since both fields are 0.)
     *
     * Independent expected bytes (Law 2 -- NOT read from struct internals):
     *   'D'=0x44 'r'=0x72 'i'=0x69 'v'=0x76 'e'=0x65 ' '=0x20 'A'=0x41
     *
     * LIST_MUT_CELL_INDEX_SWAP: (h=0,v=0)->cells[0][0] under BOTH paths;
     * this step does not discriminate the mutant (same location for h=v=0).
     * The discriminator is step 2b (col=0, row=2 -> h=0, v=2: paths differ).
     * ----------------------------------------------------------------------- */
    {
        /* Independent golden: "Drive A" as 7 raw hex bytes. */
        static const uint8_t drive_a[7] = {
            0x44, 0x72, 0x69, 0x76, 0x65, 0x20, 0x41
        };
        Cell c00;
        c00.h = 0;   /* col */
        c00.v = 0;   /* row */

        r = FlairLSetCell(&l, drive_a, 7, c00);
        CHECK(r == FLAIR_LIST_OK,
              "step-2a: LSetCell(col=0,row=0, drive_a, 7) == FLAIR_LIST_OK");

        memset(buf, 0, sizeof(buf));
        r16 = FlairLGetCell(&l, buf, (int16_t)sizeof(buf), c00);

        /* Literal: 7 bytes. */
        CHECK(r16 == 7,
              "step-2a: LGetCell(col=0,row=0) returns 7 (byte count, literal)");

        /* Literal: each byte of "Drive A" independently verified. */
        CHECK(buf[0] == 0x44u, "step-2a: buf[0] == 0x44 ('D')");
        CHECK(buf[1] == 0x72u, "step-2a: buf[1] == 0x72 ('r')");
        CHECK(buf[2] == 0x69u, "step-2a: buf[2] == 0x69 ('i')");
        CHECK(buf[3] == 0x76u, "step-2a: buf[3] == 0x76 ('v')");
        CHECK(buf[4] == 0x65u, "step-2a: buf[4] == 0x65 ('e')");
        CHECK(buf[5] == 0x20u, "step-2a: buf[5] == 0x20 (' ')");
        CHECK(buf[6] == 0x41u, "step-2a: buf[6] == 0x41 ('A')");
    }

    /* -----------------------------------------------------------------------
     * STEP 2b: LSetCell + LGetCell -- cell (col=0, row=2) = "Trash" (5 B).
     *
     * Cell address: col=0, row=2 -> Cell{ .h=0, .v=2 }.
     *   (NOT positional (Cell){0,2}: that would give v=0,h=2 = row=0,col=2!)
     *
     * Independent expected bytes (Law 2):
     *   'T'=0x54 'r'=0x72 'a'=0x61 's'=0x73 'h'=0x68
     *
     * LIST_MUT_CELL_INDEX_SWAP DISCRIMINATOR:
     *   Cell{.h=0,.v=2}: canonical write -> cells[v=2][h=0] = cells[2][0].
     *   Mutant write -> cells[h=0][v=2] = cells[0][2] (wrong location).
     *   Canonical GetCell reads cells[2][0]:
     *     canonical: cell_len[2][0]==5 -> copies "Trash" -> returns 5.
     *     mutant:    cell_len[2][0]==0 (never written by mutant) -> returns 0.
     *   CHECK(r16 == 5) FAILS under mutant -> RED.
     * ----------------------------------------------------------------------- */
    {
        /* Independent golden: "Trash" as 5 raw hex bytes. */
        static const uint8_t trash_bytes[5] = {
            0x54, 0x72, 0x61, 0x73, 0x68
        };
        Cell c02;
        c02.h = 0;   /* col=0 */
        c02.v = 2;   /* row=2 */

        r = FlairLSetCell(&l, trash_bytes, 5, c02);
        CHECK(r == FLAIR_LIST_OK,
              "step-2b: LSetCell(col=0,row=2, trash, 5) == FLAIR_LIST_OK");

        memset(buf, 0, sizeof(buf));
        r16 = FlairLGetCell(&l, buf, (int16_t)sizeof(buf), c02);

        /* Literal: 5 bytes. FAILS under LIST_MUT_CELL_INDEX_SWAP (r16==0). */
        CHECK(r16 == 5,
              "step-2b: LGetCell(col=0,row=2) returns 5 -- FAILS MUT_CELL_INDEX_SWAP");

        /* Literal: each byte of "Trash" independently verified. */
        CHECK(buf[0] == 0x54u, "step-2b: buf[0] == 0x54 ('T')");
        CHECK(buf[1] == 0x72u, "step-2b: buf[1] == 0x72 ('r')");
        CHECK(buf[2] == 0x61u, "step-2b: buf[2] == 0x61 ('a')");
        CHECK(buf[3] == 0x73u, "step-2b: buf[3] == 0x73 ('s')");
        CHECK(buf[4] == 0x68u, "step-2b: buf[4] == 0x68 ('h')");
    }

    /* -----------------------------------------------------------------------
     * STEP 3: LGetCell on an EMPTY cell (col=0, row=1) -> 0 bytes (not error).
     *
     * Cell (col=0, row=1) was never written; must return 0 (empty), not error.
     * Sentinel byte verifies no partial copy on empty (Rule 2).
     * ----------------------------------------------------------------------- */
    {
        Cell c01;
        c01.h = 0;   /* col=0 */
        c01.v = 1;   /* row=1 */

        memset(buf, 0xCCu, sizeof(buf));    /* sentinel -- must not be written */
        r16 = FlairLGetCell(&l, buf, (int16_t)sizeof(buf), c01);

        CHECK(r16 == 0,
              "step-3: LGetCell(col=0,row=1) == 0 (empty cell; not error)");
        CHECK(buf[0] == 0xCCu,
              "step-3: sentinel buf[0] unchanged (no copy on empty cell)");
    }

    /* -----------------------------------------------------------------------
     * STEP 4a: FlairLClick hit test -- point WITHIN the grid.
     *
     * Literal inputs (NOT derived from FlairList state):
     *   cellSize.h=100, cellSize.v=16  (set in step 1)
     *   rView: top=20, left=10         (set in step 1)
     *   localPt: h=15 (x-pixels), v=38 (y-pixels)  [= rView.top+cellSize.v+2]
     *
     * Literal arithmetic (computed here, NOT from l):
     *   col_off = (localPt.h - rView.left) / cellSize.h = (15-10)/100 = 0
     *   row_off = (localPt.v - rView.top)  / cellSize.v = (38-20)/16  = 1
     *   hit = col_off + visible.left = 0, row_off + visible.top = 1
     *   -> expected Cell: h=0, v=1   (these are LITERAL CHECK values below)
     *
     * LIST_MUT_HIT_OFFBYONE DISCRIMINATOR:
     *   Canonical row = (38-20)/16 = 1.
     *   Mutant    row = 38/16 = 2.
     *   CHECK(hit.v == 1) FAILS under mutant (hit.v==2) -> RED.
     * ----------------------------------------------------------------------- */
    {
        /* flair_point_t { v (y), h (x) }: v=38 (y-pixels), h=15 (x-pixels). */
        flair_point_t local_in;
        local_in.v = 38;   /* y = rView.top(20) + 1*cellSize.v(16) + 2 */
        local_in.h = 15;   /* x = rView.left(10) + 5                   */

        memset(&hit, 0, sizeof(hit));
        r = FlairLClick(&l, local_in, &hit);

        CHECK(r == 1,
              "step-4a: LClick returns 1 (hit within grid)");

        /* Literal expected cell: col=0, row=1 -> h=0, v=1. */
        CHECK(hit.h == 0,
              "step-4a: hit.h == 0 (col 0, literal)");
        CHECK(hit.v == 1,
              "step-4a: hit.v == 1 (row 1, literal) -- FAILS MUT_HIT_OFFBYONE");

        /* lastClick must record the hit cell. */
        CHECK(l.lastClick.h == 0 && l.lastClick.v == 1,
              "step-4a: lastClick == {h=0,v=1} (literal)");

        /* Selection bit for (col=0,row=1): set (selFlags==0 = no lOnlyOne yet). */
        {
            Cell c01;
            c01.h = 0; c01.v = 1;
            CHECK(FlairLGetSelect(&l, c01) == 1,
                  "step-4a: (col=0,row=1) selected after click");
        }
    }

    /* -----------------------------------------------------------------------
     * STEP 4b: FlairLClick miss -- point ABOVE rView.top.
     *
     * localPt: v=15, h=15.  v=15 < rView.top=20 -> outside rView -> miss.
     * Returns 0; hit and lastClick unchanged from step 4a.
     * ----------------------------------------------------------------------- */
    {
        flair_point_t above;
        above.v = 15;   /* y=15 < rView.top=20 -> miss */
        above.h = 15;

        r = FlairLClick(&l, above, &hit);
        CHECK(r == 0,
              "step-4b: LClick above rView.top returns 0 (miss)");

        /* hit_out and lastClick must be unchanged from step 4a on a miss. */
        CHECK(hit.h == 0 && hit.v == 1,
              "step-4b: hit_out unchanged after miss (still {h=0,v=1})");
        CHECK(l.lastClick.h == 0 && l.lastClick.v == 1,
              "step-4b: lastClick unchanged after miss");
    }

    /* -----------------------------------------------------------------------
     * STEP 5: lOnlyOne selection model.
     *
     * Activate single-selection: selFlags = FLAIR_LSEL_ONLY_ONE = -128 = 0x80.
     *
     * 5a: Click pixel->cell(col=0,row=0). lOnlyOne deselects all (incl. (0,1)
     *     from step 4a), then selects (0,0).
     *     Literal checks: (0,0)==1; (0,1)==0.
     *
     * 5b: Click pixel->cell(col=0,row=2). lOnlyOne deselects all (incl. (0,0)),
     *     then selects (0,2).
     *     Literal checks: (0,0)==0; (0,2)==1; lastClick=={h=0,v=2}.
     *
     * LIST_MUT_NO_DESELECT DISCRIMINATOR:
     *   Mutant skips deselect loop in 5b; (0,0) remains 1.
     *   CHECK(FlairLGetSelect((0,0)) == 0) FAILS (returns 1) -> RED.
     * ----------------------------------------------------------------------- */
    {
        /* Pixel point -> cell (col=0, row=0):
         *   v in [rView.top, rView.top+cellSize.v) = [20, 36).  Use v=20.
         *   h in [rView.left, rView.left+cellSize.h) = [10, 110).  Use h=15.
         *   col_off=(15-10)/100=0, row_off=(20-20)/16=0 -> cell(h=0,v=0). */
        flair_point_t pt_r0;
        pt_r0.v = 20;   /* y=rView.top exactly; row_off=(20-20)/16=0 */
        pt_r0.h = 15;

        /* Pixel point -> cell (col=0, row=2):
         *   v in [rView.top+2*cellSize.v, rView.top+3*cellSize.v) = [52, 68).
         *   Use v=52: row_off=(52-20)/16=32/16=2.
         *   col_off=(15-10)/100=0 -> cell(h=0,v=2). */
        flair_point_t pt_r2;
        pt_r2.v = 52;   /* row_off=(52-20)/16=2 */
        pt_r2.h = 15;

        Cell c00, c01, c02;
        c00.h = 0; c00.v = 0;
        c01.h = 0; c01.v = 1;
        c02.h = 0; c02.v = 2;

        /* Activate single-selection mode. */
        l.selFlags = FLAIR_LSEL_ONLY_ONE;  /* (int8_t)(-128) = 0x80 */

        /* --- 5a: click cell (col=0, row=0) -------------------------------- */
        memset(&hit, 0, sizeof(hit));
        r = FlairLClick(&l, pt_r0, &hit);

        CHECK(r == 1,
              "step-5a: LClick(pt_r0) returns 1 (hit)");
        CHECK(hit.h == 0 && hit.v == 0,
              "step-5a: hit == {h=0,v=0} (literal)");
        CHECK(FlairLGetSelect(&l, c00) == 1,
              "step-5a: (col=0,row=0) selected");
        /* lOnlyOne must have deselected (col=0,row=1) from step 4a. */
        CHECK(FlairLGetSelect(&l, c01) == 0,
              "step-5a: (col=0,row=1) deselected by lOnlyOne");

        /* --- 5b: click cell (col=0, row=2) -------------------------------- */
        memset(&hit, 0, sizeof(hit));
        r = FlairLClick(&l, pt_r2, &hit);

        CHECK(r == 1,
              "step-5b: LClick(pt_r2) returns 1 (hit)");
        CHECK(hit.h == 0 && hit.v == 2,
              "step-5b: hit == {h=0,v=2} (literal)");

        /* lOnlyOne must have deselected (col=0,row=0). FAILS MUT_NO_DESELECT. */
        CHECK(FlairLGetSelect(&l, c00) == 0,
              "step-5b: (col=0,row=0) deselected -- FAILS LIST_MUT_NO_DESELECT");

        /* (col=0,row=2) must be selected. */
        CHECK(FlairLGetSelect(&l, c02) == 1,
              "step-5b: (col=0,row=2) selected");

        /* lastClick must record the last hit cell (literal). */
        CHECK(l.lastClick.h == 0 && l.lastClick.v == 2,
              "step-5b: lastClick == {h=0,v=2} (literal)");
    }

    /* -----------------------------------------------------------------------
     * STEP 6: Out-of-bounds LSetCell on a 1x3 grid -> fail-loud error.
     *
     * Grid: dataBounds = {top=0,left=0,bottom=3,right=1} (3 rows, 1 col).
     * Cell (col=5, row=5) = {.h=5, .v=5}: h=5 >= right=1 AND v=5 >= bottom=3.
     * FlairLSetCell must return FLAIR_LIST_ERR_BOUNDS and NOT corrupt data.
     *
     * After the rejected write: (col=0,row=0) must still have "Drive A".
     * ----------------------------------------------------------------------- */
    {
        static const uint8_t oob_bytes[3] = { 0xDE, 0xAD, 0xBE };
        Cell c55;
        Cell c00;
        c55.h = 5; c55.v = 5;
        c00.h = 0; c00.v = 0;

        r = FlairLSetCell(&l, oob_bytes, 3, c55);
        CHECK(r == FLAIR_LIST_ERR_BOUNDS,
              "step-6: LSetCell({h=5,v=5}) -> FLAIR_LIST_ERR_BOUNDS (-2)");

        /* (col=0,row=0) must be unaffected: "Drive A", 7 bytes. */
        memset(buf, 0, sizeof(buf));
        r16 = FlairLGetCell(&l, buf, (int16_t)sizeof(buf), c00);
        CHECK(r16 == 7,
              "step-6: (col=0,row=0) still 7 bytes after rejected OOB write");
        CHECK(buf[0] == 0x44u,
              "step-6: buf[0] == 0x44 ('D') -- data intact");
        CHECK(buf[6] == 0x41u,
              "step-6: buf[6] == 0x41 ('A') -- data intact");
    }

    /* -----------------------------------------------------------------------
     * STEP 7: Error path -- buffer too small for LGetCell (Rule 2 no-truncate).
     *
     * "Drive A" at (col=0,row=0) is 7 bytes.  A 4-byte buf is too small.
     * Must return FLAIR_LIST_ERR_BUF (-4) with NO partial copy.
     * ----------------------------------------------------------------------- */
    {
        uint8_t small[4];
        Cell c00;
        c00.h = 0; c00.v = 0;

        memset(small, 0xCCu, sizeof(small));
        r16 = FlairLGetCell(&l, small, 4, c00);

        CHECK(r16 == FLAIR_LIST_ERR_BUF,
              "step-7: LGetCell bufsz=4 for 7-byte cell -> FLAIR_LIST_ERR_BUF (-4)");

        /* No partial copy: all sentinels must be intact (Rule 2). */
        CHECK(small[0] == 0xCCu && small[1] == 0xCCu &&
              small[2] == 0xCCu && small[3] == 0xCCu,
              "step-7: no partial copy (sentinel bytes 0xCC unchanged)");
    }

    /* -----------------------------------------------------------------------
     * STEP 8: NULL argument -> fail-loud FLAIR_LIST_ERR_NULL (Rule 2).
     * ----------------------------------------------------------------------- */
    {
        uint8_t tbuf[8];
        Cell c00;
        c00.h = 0; c00.v = 0;

        r16 = FlairLGetCell(NULL, tbuf, (int16_t)sizeof(tbuf), c00);
        CHECK(r16 == FLAIR_LIST_ERR_NULL,
              "step-8: LGetCell(NULL list) -> FLAIR_LIST_ERR_NULL (-5)");

        r16 = FlairLGetCell(&l, NULL, (int16_t)sizeof(tbuf), c00);
        CHECK(r16 == FLAIR_LIST_ERR_NULL,
              "step-8: LGetCell(NULL buf) -> FLAIR_LIST_ERR_NULL (-5)");

        r = FlairLSetCell(NULL, tbuf, 1, c00);
        CHECK(r == FLAIR_LIST_ERR_NULL,
              "step-8: LSetCell(NULL list) -> FLAIR_LIST_ERR_NULL (-5)");
    }

    return TEST_SUMMARY("test-list");
}
