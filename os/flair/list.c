/*
 * os/flair/list.c -- FLAIR List Manager implementation (first cut).
 *
 * beads:  initech-77dj (FLAIR Phase 4.5 -- List Manager).
 * Ref:    list-manager.md Sec 1-5; list.h (types + API contract).
 *         CLAUDE.md Law 1 (ground truth), Law 3 (artifact C, freestanding),
 *         Rule 1 (RED->GREEN->Refactor), Rule 2 (fail-loud, no silent
 *         truncation), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * MUTANT COVERAGE (CLAUDE.md Rule 6 -- each mutant must compile and go RED):
 *
 *   LIST_MUT_CELL_INDEX_SWAP
 *     In FlairLSetCell: index the store as cells[c.h][c.v] (COL-major) instead
 *     of cells[c.v][c.h] (ROW-major canonical).  FlairLGetCell is NOT mutated:
 *     it always reads the canonical [row][col] location.  The write goes to the
 *     wrong memory location, so the canonical read finds the cell empty.
 *     Detects at: oracle step 2, FlairLGetCell((0,2)) expects "Trash" (5 bytes)
 *     but the cell at cells[2][0] was never written -> returns 0 bytes -> RED.
 *
 *   LIST_MUT_HIT_OFFBYONE
 *     In FlairLClick: omit the rView.top / rView.left subtraction from the
 *     pixel->cell computation.  The hit maps to a different row.
 *     Detects at: oracle step 4, localPt=(15,38) should map to (h=0,v=1) but
 *     maps to (h=0,v=2) (38/16=2 vs (38-20)/16=1) -> hit.v==2 != 1 -> RED.
 *
 *   LIST_MUT_NO_DESELECT
 *     In FlairLClick: with lOnlyOne set, skip the deselect-all loop.  The
 *     previously-selected cell is not cleared.
 *     Detects at: oracle step 5, after clicking (0,2), FlairLGetSelect((0,0))
 *     should be 0 but stays 1 -> RED.
 *
 * ARTIFACT code: freestanding, no libc, no malloc.  <stdint.h>/<stddef.h> only
 * (via list.h -> grafport.h / region_algebra.h).  Dual-compiles for the host
 * oracle and -m32 -ffreestanding -nostdlib kernel type-check.
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdint.h>
#include <stddef.h>

#include "list.h"

/* --------------------------------------------------------------------------
 * Freestanding helpers -- no libc, no string.h (Law 3).
 * -------------------------------------------------------------------------- */

/* list_memset: fill n bytes at dst with val. */
static void list_memset(void *dst, uint8_t val, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    size_t   i;
    for (i = 0; i < n; i++) {
        d[i] = val;
    }
}

/* list_memcpy: copy n bytes from src to dst (non-overlapping). */
static void list_memcpy(void *dst, const void *src, size_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    size_t         i;
    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

/* --------------------------------------------------------------------------
 * Internal helpers.
 * -------------------------------------------------------------------------- */

/*
 * cell_in_bounds: 1 if cell c is within l->dataBounds, 0 otherwise.
 *
 * dataBounds is half-open [left, right) x [top, bottom) in cell coords,
 * consistent with the QuickDraw Rect convention (region_algebra.h Sec 4).
 */
static int cell_in_bounds(const FlairList *l, Cell c)
{
    return (c.h >= l->dataBounds.left  &&
            c.h <  l->dataBounds.right &&
            c.v >= l->dataBounds.top   &&
            c.v <  l->dataBounds.bottom);
}

/* --------------------------------------------------------------------------
 * FlairList_init
 * -------------------------------------------------------------------------- */
void FlairList_init(FlairList *l, Cell cellSize, rgn_rect_t rView)
{
    if (!l) return;

    /* Zero the entire record for deterministic initial state (Rule 11).
     * This clears cells[], cell_len[], selected[], all bounds, and selFlags. */
    list_memset(l, 0, sizeof(*l));

    l->cellSize = cellSize;
    l->rView    = rView;

    /* dataBounds and visible both start as {0,0,0,0}: a 0x0 empty grid.
     * Ref: LNew -- "returns a handle to an empty list" [list-manager.md Sec 5]. */
    l->dataBounds.top    = 0;
    l->dataBounds.left   = 0;
    l->dataBounds.bottom = 0;
    l->dataBounds.right  = 0;
    l->visible = l->dataBounds;

    l->selFlags    = 0;
    l->lActive     = 1;        /* list is active by default */
    l->lastClick.h = 0;
    l->lastClick.v = 0;
}

/* --------------------------------------------------------------------------
 * FlairLAddRow
 * -------------------------------------------------------------------------- */
int FlairLAddRow(FlairList *l, int16_t count, int16_t afterRow)
{
    int16_t cur_rows;
    int16_t new_rows;

    if (!l) return FLAIR_LIST_ERR_NULL;
    if (count <= 0) return FLAIR_LIST_OK; /* no-op for zero count */

    cur_rows = (int16_t)(l->dataBounds.bottom - l->dataBounds.top);
    new_rows = (int16_t)(cur_rows + count);

    if (new_rows > FLAIR_LIST_MAX_ROWS) return FLAIR_LIST_ERR_OVERFLOW;

    /* First-cut: append-only.  Full row insertion (shifting the cell store for
     * afterRow != dataBounds.bottom-1) is a follow-up M5+ issue. The test
     * calls this on an empty list (afterRow=-1 = "before all"), which is
     * identical to appending when cur_rows == 0. */
    (void)afterRow;

    l->dataBounds.bottom = (int16_t)(l->dataBounds.top + new_rows);

    /* Auto-create 1 column if the grid currently has zero columns.
     * Standard-File / Finder lists are single-column; this matches the
     * IM default: "a list with no columns is usually not useful." */
    if (l->dataBounds.right == l->dataBounds.left) {
        if (l->dataBounds.right - l->dataBounds.left < FLAIR_LIST_MAX_COLS) {
            l->dataBounds.right = (int16_t)(l->dataBounds.left + 1);
        }
    }

    /* visible tracks dataBounds (no scroll in first cut). */
    l->visible = l->dataBounds;

    return FLAIR_LIST_OK;
}

/* --------------------------------------------------------------------------
 * FlairLSetCell
 * -------------------------------------------------------------------------- */
int FlairLSetCell(FlairList *l, const void *data, int16_t len, Cell c)
{
    if (!l || !data) return FLAIR_LIST_ERR_NULL;
    if (!cell_in_bounds(l, c)) return FLAIR_LIST_ERR_BOUNDS;
    if (len < 0 || len > (int16_t)FLAIR_LIST_CELL_MAX) return FLAIR_LIST_ERR_DATA_LEN;

#ifdef LIST_MUT_CELL_INDEX_SWAP
    /*
     * MUTANT LIST_MUT_CELL_INDEX_SWAP: index the store as [col][row] =
     * [c.h][c.v] instead of the canonical [row][col] = [c.v][c.h].
     * The WRITE goes to the wrong memory location.  FlairLGetCell is NOT
     * mutated (it always reads [c.v][c.h]).  A subsequent FlairLGetCell on
     * the same cell finds cells[c.v][c.h] still zero (never written by the
     * mutated path) and returns 0 bytes.  Oracle step 2 checks for 5 bytes
     * ("Trash") -> RED.
     *
     * Ref: CLAUDE.md Rule 6 (mutation-proven golden).
     */
    list_memcpy(l->cells[c.h][c.v], data, (size_t)(uint16_t)len);
    l->cell_len[c.h][c.v] = (uint8_t)len;
#else
    /* Canonical: ROW-major indexing [row][col] = [c.v][c.h]. */
    list_memcpy(l->cells[c.v][c.h], data, (size_t)(uint16_t)len);
    l->cell_len[c.v][c.h] = (uint8_t)len;
#endif

    return FLAIR_LIST_OK;
}

/* --------------------------------------------------------------------------
 * FlairLGetCell
 * -------------------------------------------------------------------------- */
int16_t FlairLGetCell(const FlairList *l, void *buf, int16_t bufsz, Cell c)
{
    uint8_t actual_len;

    if (!l || !buf) return (int16_t)FLAIR_LIST_ERR_NULL;
    if (!cell_in_bounds(l, c)) return (int16_t)FLAIR_LIST_ERR_BOUNDS;

    /* Always reads the canonical [row][col] = [c.v][c.h] location.
     * NOT wrapped in LIST_MUT_CELL_INDEX_SWAP -- the mutation is in
     * FlairLSetCell only, making it detectable: write goes to [c.h][c.v]
     * but read comes from [c.v][c.h]. */
    actual_len = l->cell_len[c.v][c.h];

    if (actual_len == 0) {
        return 0; /* empty cell: success, no copy needed */
    }

    if (bufsz < (int16_t)actual_len) {
        return (int16_t)FLAIR_LIST_ERR_BUF; /* never partial-copy (Rule 2) */
    }

    list_memcpy(buf, l->cells[c.v][c.h], (size_t)actual_len);
    return (int16_t)actual_len;
}

/* --------------------------------------------------------------------------
 * FlairLClick
 * -------------------------------------------------------------------------- */
int FlairLClick(FlairList *l, flair_point_t localPt, Cell *hit_out)
{
    int16_t col_off;
    int16_t row_off;
    Cell    hit;

    if (!l || !hit_out) return 0;
    if (l->cellSize.h <= 0 || l->cellSize.v <= 0) return 0;

    /* Bounds check: localPt must be within the list's pixel rect (rView).
     * A point outside rView is a miss -- return 0 without changing selection.
     * Ref: LClick semantics [list-manager.md Sec 5]. */
    if (localPt.h <  l->rView.left  ||
        localPt.h >= l->rView.right  ||
        localPt.v <  l->rView.top    ||
        localPt.v >= l->rView.bottom) {
        return 0;
    }

    /* Compute cell offsets from the top-left of the visible area. */
#ifdef LIST_MUT_HIT_OFFBYONE
    /*
     * MUTANT LIST_MUT_HIT_OFFBYONE: omit rView.top / rView.left subtraction.
     * The pixel coordinates are used raw, giving a row/col that corresponds
     * to a different (wrong) cell.
     * Oracle step 4: localPt=(15,38), rView.top=20, cellSize.v=16:
     *   canonical row = (38-20)/16 = 1; mutant row = 38/16 = 2 -> hit.v=2.
     *   Test checks hit.v == 1 -> FAIL -> RED.
     * Ref: CLAUDE.md Rule 6.
     */
    col_off = (int16_t)(localPt.h / l->cellSize.h);
    row_off = (int16_t)(localPt.v / l->cellSize.v);
#else
    /* Canonical: subtract rView origin to get offset within the visible area. */
    col_off = (int16_t)((localPt.h - l->rView.left) / l->cellSize.h);
    row_off = (int16_t)((localPt.v - l->rView.top)  / l->cellSize.v);
#endif

    /* Map offset to absolute cell coordinates using visible's top-left.
     * (First cut: visible.top == dataBounds.top == 0; no scroll offset.) */
    hit.h = (int16_t)(l->visible.left + col_off);
    hit.v = (int16_t)(l->visible.top  + row_off);

    /* Bounds check against dataBounds. */
    if (!cell_in_bounds(l, hit)) return 0;

    /* Apply selFlags.
     * lOnlyOne (0x80): deselect ALL cells before selecting the hit cell.
     * Checked as an unsigned bit test to avoid signed-byte sign-extension. */
    if ((uint8_t)l->selFlags & 0x80u) {
#ifndef LIST_MUT_NO_DESELECT
        /* Canonical: deselect every cell in the grid (block-scoped vars). */
        {
            int16_t ri, ci;
            for (ri = l->dataBounds.top; ri < l->dataBounds.bottom; ri++) {
                for (ci = l->dataBounds.left; ci < l->dataBounds.right; ci++) {
                    l->selected[ri][ci] = 0;
                }
            }
        }
#else
        /*
         * MUTANT LIST_MUT_NO_DESELECT: skip the deselect loop.  With lOnlyOne,
         * previously-selected cells are NOT cleared.
         * Oracle step 5: click (0,0) then click (0,2); check (0,0) is now
         * deselected.  Mutant: (0,0) stays selected (1) -> FAIL -> RED.
         * Ref: CLAUDE.md Rule 6.
         */
        (void)0; /* suppress empty-body warning in the mutant path */
#endif
    }

    /* Select the hit cell and record lastClick. */
    l->selected[hit.v][hit.h] = 1;
    l->lastClick = hit;
    *hit_out = hit;
    return 1;
}

/* --------------------------------------------------------------------------
 * FlairLGetSelect
 * -------------------------------------------------------------------------- */
int FlairLGetSelect(const FlairList *l, Cell c)
{
    if (!l) return FLAIR_LIST_ERR_NULL;
    if (!cell_in_bounds(l, c)) return FLAIR_LIST_ERR_BOUNDS;
    return (int)l->selected[c.v][c.h];
}

/* --------------------------------------------------------------------------
 * FlairLSetSelect
 * -------------------------------------------------------------------------- */
int FlairLSetSelect(FlairList *l, int sel, Cell c)
{
    if (!l) return FLAIR_LIST_ERR_NULL;
    if (!cell_in_bounds(l, c)) return FLAIR_LIST_ERR_BOUNDS;
    l->selected[c.v][c.h] = (uint8_t)(sel ? 1 : 0);
    return FLAIR_LIST_OK;
}
