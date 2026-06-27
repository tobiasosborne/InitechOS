/*
 * os/flair/list.h -- FLAIR List Manager: FlairList, Cell/ListBounds, selection.
 *
 * beads:  initech-77dj (FLAIR Phase 4.5 -- List Manager first cut).
 * Ref:    list-manager.md Sec 1-5 (VERBATIM ListRec layout, Cell/ListBounds
 *         cell-coordinate model, selFlags/listFlags, LAddRow/LSetCell/LGetCell/
 *         LClick, the selection model). Ground truth: A+B verified,
 *         im-toolbox-records-verbatim.md + GWU adalib lists.html.
 * Ref:    spec/grafport.h -- flair_point_t = QuickDraw Point (v=row at 0,
 *         h=col at 2). REUSED as Cell. (Do NOT define a second Point type;
 *         event_model.h Sec 1 directive.)
 * Ref:    spec/region_algebra.h -- rgn_rect_t = QuickDraw Rect (t,l,b,r).
 *         REUSED as ListBounds (cell-coord rect) and for rView (pixel rect).
 * Ref:    CLAUDE.md Law 1 (ground truth before code), Law 2 (oracle truth,
 *         anti-by-construction), Law 3 (artifact C, freestanding, no libc),
 *         Rule 2 (fail-loud, never silent truncation), Rule 8 (locked spec),
 *         Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * REDUCED FIRST CUT (initech-77dj):
 *   Static cell store -- no DataHandle/heap allocation.
 *   Caps: FLAIR_LIST_MAX_ROWS x FLAIR_LIST_MAX_COLS x FLAIR_LIST_CELL_MAX.
 *   selFlags: lOnlyOne implemented; lExtendDrag/lNoDisjoint reserved.
 *   LAddRow: append-only (afterRow ignored); no LDelRow, no LScroll.
 *   FlairLClick: single-click, no drag, no double-click timing (clikTime).
 *   Follow-up: full M5+ semantics (scrolling, drag, LDEF dispatch) in a
 *   follow-on issue once the Standard-File chrome is scoped.
 *
 * ARTIFACT code: freestanding, dual-compiles for the host oracle
 * (gcc -std=c11 -Wall -Wextra -Werror) and the kernel type-check
 * (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra -Werror).
 * Only <stdint.h> and <stddef.h> (freestanding headers). No malloc, no libc.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef FLAIR_LIST_H
#define FLAIR_LIST_H

#include <stdint.h>
#include <stddef.h>

#include "grafport.h"        /* flair_point_t = QuickDraw Point; -Ispec       */
#include "region_algebra.h"  /* rgn_rect_t    = QuickDraw Rect;  -Ispec       */

/* ==========================================================================
 * First-cut reduction caps.
 *
 * These are locked spec-data (CLAUDE.md Rule 8). Raise only with a beads
 * issue and worklog note -- never a silent bump to make one test pass.
 *
 * Sizing rationale:
 *   ROWS 32: comfortably covers a Standard-File dialog list (~20 items typical,
 *            with headroom). Real dataBounds.bottom can be < 32.
 *   COLS  8: most FLAIR lists are single-column (Finder, Standard-File). Eight
 *            columns covers multi-column outlier cases in the first cut.
 *   CELL 64: a Mac filename is 255 chars max, but the first-cut display cells
 *            hold short labels (8.3 names, drive letters, "Trash", etc.).
 *            Follow-up M5+ issue to raise if needed.
 *
 * Static store per FlairList: 32*8*64 = 16384 + 256 + 256 = ~16 KB.
 * Kernel placement: one or two lists in the FLAIR heap (4 MB window) is fine.
 * ========================================================================== */
#define FLAIR_LIST_MAX_ROWS  32    /* max rows in dataBounds (first-cut cap)   */
#define FLAIR_LIST_MAX_COLS   8    /* max cols in dataBounds (first-cut cap)   */
#define FLAIR_LIST_CELL_MAX  64    /* max bytes of data per cell (first cut)   */

_Static_assert(FLAIR_LIST_MAX_ROWS >= 2,
               "need at least 2 rows for the selection oracle");
_Static_assert(FLAIR_LIST_MAX_COLS >= 1,
               "need at least 1 col for the standard single-column list");
_Static_assert(FLAIR_LIST_CELL_MAX >= 8,
               "cell max must hold at least a short label");

/* ==========================================================================
 * Cell and ListBounds -- the cell-coordinate model.
 *
 * [Ref: list-manager.md Sec 2, verified A+B]
 *
 *   Cell      = QuickDraw Point: h = COLUMN, v = ROW; first cell = (0,0).
 *   ListBounds = QuickDraw Rect in CELL coordinates (not pixels).
 *
 * These are STRICT ALIASES of the existing FLAIR types -- NO second Point
 * or Rect type is defined (event_model.h Sec 1 directive, drawing_ops.h
 * coordinate contract).
 *
 *   Cell       reuses flair_point_t (h=col, v=row; grafport.h Sec 2)
 *   ListBounds reuses rgn_rect_t   (top/left/bottom/right; region_algebra.h)
 *
 * ListBounds semantics in cell coords:
 *   dataBounds: top=first_row, left=first_col, bottom=one_past_last_row,
 *               right=one_past_last_col.  (Half-open, like pixel rects.)
 *   visible:    same shape, sub-rect of dataBounds that is currently shown.
 * ========================================================================== */
typedef flair_point_t Cell;        /* h=col, v=row [list-manager.md Sec 2]    */
typedef rgn_rect_t    ListBounds;  /* Rect in cell coords [list-manager.md Sec 2]*/

/* ==========================================================================
 * selFlags bit constants (list-manager.md Sec 4.1, verified A+B).
 *
 * Values carried VERBATIM from Inside Macintosh / GWU adalib lists.html.
 * lOnlyOne = -128 (0x80 as a signed byte) -- the high bit makes it negative.
 * The remaining three (lNoExtend/lNoRect/lUseSense/lNoNilHilite) are
 * web-corroborated [GWU lists.html + IM MoreToolbox-214] but not yet in the
 * local im-toolbox-records-verbatim.md cache (Sec 7 certainty ledger).
 * ========================================================================== */
#define FLAIR_LSEL_ONLY_ONE      ((int8_t)(-128)) /* lOnlyOne    = 0x80 signed */
#define FLAIR_LSEL_EXTEND_DRAG   ((int8_t)(64))   /* lExtendDrag = 0x40        */
#define FLAIR_LSEL_NO_DISJOINT   ((int8_t)(32))   /* lNoDisjoint = 0x20        */
#define FLAIR_LSEL_NO_EXTEND     ((int8_t)(16))   /* lNoExtend   = 0x10        */
#define FLAIR_LSEL_NO_RECT       ((int8_t)(8))    /* lNoRect     = 0x08        */
#define FLAIR_LSEL_USE_SENSE     ((int8_t)(4))    /* lUseSense   = 0x04        */
#define FLAIR_LSEL_NO_NIL_HILITE ((int8_t)(2))    /* lNoNilHilite = 0x02       */

/* Build-time pin: lOnlyOne must equal -128 (Sec 4.1, A+B verified). */
_Static_assert(FLAIR_LSEL_ONLY_ONE == (int8_t)(-128),
               "lOnlyOne == -128 [list-manager.md Sec 4.1, A+B]");

/* ==========================================================================
 * FlairList -- the FLAIR list record (reduced first-cut; initech-77dj).
 *
 * Field names follow the IM ListRec source order [list-manager.md Sec 1].
 * Handle / heap fields are collapsed per the FLAIR deviation pattern
 * (list-manager.md Sec 6: no DataHandle, no GrafPtr ownership, no
 * ControlHandle for vScroll/hScroll -- those are separate Control objects).
 *
 * Cell data store: a flat static 3-D array cells[row][col][bytes], indexed
 * as cells[c.v][c.h][0..len-1] (ROW-major = [c.v][c.h]).  First cut: no
 * DataHandle / cellArray variable-length area (IM ListRec fields `cells` and
 * `cellArray`); replace with arena allocation in M5+ full build.
 * ========================================================================== */
typedef struct FlairList {
    /* --- Pixel-space fields (IM order) ------------------------------------ */
    rgn_rect_t   rView;      /* list's display rect, pixel coords (IM: rView) */
    Cell         cellSize;   /* cell pixel size: h=width, v=height (IM: cellSize) */

    /* --- Cell-space fields (IM order) ------------------------------------- */
    ListBounds   visible;    /* visible cell-coord rect (IM: visible)         */
    int8_t       selFlags;   /* selection-behavior bits (IM: selFlags; Sec 4.1)*/
    uint8_t      lActive;    /* 1 if the list is active (IM: lActive)         */
    Cell         lastClick;  /* last cell clicked (IM: lastClick)             */
    ListBounds   dataBounds; /* allocated grid in cell coords (IM: dataBounds)*/

    /* --- Cell data store (first-cut static arrays; IM: cells+cellArray) --- */
    /* Indexed [row][col] = [c.v][c.h].  Mutation oracle: see LIST_MUT_* in
     * list.c.  The store is canonical ROW-MAJOR; the mutation swaps the write
     * path only, leaving the read path canonical, so a written cell is
     * irrecoverable at its correct location (step 2 goes RED). */
    uint8_t cells[FLAIR_LIST_MAX_ROWS][FLAIR_LIST_MAX_COLS][FLAIR_LIST_CELL_MAX];
    uint8_t cell_len[FLAIR_LIST_MAX_ROWS][FLAIR_LIST_MAX_COLS]; /* data len    */
    uint8_t selected[FLAIR_LIST_MAX_ROWS][FLAIR_LIST_MAX_COLS]; /* 0/1 sel bit */
} FlairList;

/* ==========================================================================
 * Error codes -- fail-loud (CLAUDE.md Rule 2; never silent truncation).
 * All negative to distinguish from valid byte counts / booleans.
 * ========================================================================== */
#define FLAIR_LIST_OK          0    /* success                                 */
#define FLAIR_LIST_ERR_OVERFLOW (-1) /* would exceed FLAIR_LIST_MAX_ROWS/COLS */
#define FLAIR_LIST_ERR_BOUNDS  (-2)  /* cell outside dataBounds               */
#define FLAIR_LIST_ERR_DATA_LEN (-3) /* data len > FLAIR_LIST_CELL_MAX        */
#define FLAIR_LIST_ERR_BUF     (-4)  /* output buffer too small for cell data */
#define FLAIR_LIST_ERR_NULL    (-5)  /* NULL pointer argument                 */

/* ==========================================================================
 * Public API -- FLAIR subset of the List Manager (list-manager.md Sec 5).
 * ========================================================================== */

/*
 * FlairList_init -- bind the list to a pixel rect and cell size; zero everything.
 *
 * After init: dataBounds = visible = {0,0,0,0} (0 rows, 0 cols).
 * The list is active (lActive=1), selFlags=0, lastClick=(0,0).
 * Ref: LNew semantics [list-manager.md Sec 5].
 */
void FlairList_init(FlairList *l, Cell cellSize, rgn_rect_t rView);

/*
 * FlairLAddRow -- insert count rows AFTER afterRow (-1 = prepend before all).
 *
 * First-cut: always appends (afterRow is recorded but cell-shifting is not yet
 * implemented -- follow-up M5+ issue).  Auto-creates 1 column if dataBounds
 * has zero columns (Standard-File single-column default).
 * Returns FLAIR_LIST_OK (0) on success.
 * Returns FLAIR_LIST_ERR_OVERFLOW if adding count rows would exceed MAX_ROWS.
 * Returns FLAIR_LIST_ERR_NULL if l is NULL.
 * Ref: LAddRow [list-manager.md Sec 5]. Fail-loud (Rule 2).
 */
int FlairLAddRow(FlairList *l, int16_t count, int16_t afterRow);

/*
 * FlairLSetCell -- store len bytes of data into cell c.
 *
 * Returns FLAIR_LIST_OK (0) on success.
 * Fail-loud (Rule 2):
 *   FLAIR_LIST_ERR_NULL    if l or data is NULL.
 *   FLAIR_LIST_ERR_BOUNDS  if c is outside dataBounds (no write occurs).
 *   FLAIR_LIST_ERR_DATA_LEN if len > FLAIR_LIST_CELL_MAX or len < 0.
 * Ref: LSetCell [list-manager.md Sec 5].
 */
int FlairLSetCell(FlairList *l, const void *data, int16_t len, Cell c);

/*
 * FlairLGetCell -- copy cell c's data into buf (bufsz bytes available).
 *
 * Returns the byte count (>= 0) on success.  Returns 0 for an empty
 * (zero-length) cell -- this is success, not an error.
 * Fail-loud (Rule 2):
 *   FLAIR_LIST_ERR_NULL   if l or buf is NULL.
 *   FLAIR_LIST_ERR_BOUNDS if c is outside dataBounds.
 *   FLAIR_LIST_ERR_BUF    if bufsz < cell data length (NO partial copy).
 * Ref: LGetCell [list-manager.md Sec 5].
 */
int16_t FlairLGetCell(const FlairList *l, void *buf, int16_t bufsz, Cell c);

/*
 * FlairLClick -- hit-test a local pixel point against the cell grid.
 *
 * Computes (col, row) from the pixel point relative to rView origin and
 * cellSize; checks the result against dataBounds.  If a cell is hit, applies
 * selFlags (lOnlyOne: deselect all, then select the hit cell), records
 * lastClick, and sets *hit_out.
 *
 * Returns 1 if a cell was hit (*hit_out set); 0 if the point is outside
 * rView or outside dataBounds (no selection change on miss).
 * Ref: LClick [list-manager.md Sec 5]; selFlags [Sec 4.1].
 */
int FlairLClick(FlairList *l, flair_point_t localPt, Cell *hit_out);

/*
 * FlairLGetSelect -- query the selection state of cell c.
 *
 * Returns 1 if selected, 0 if not selected.
 * Returns FLAIR_LIST_ERR_BOUNDS if c is outside dataBounds.
 * Ref: LGetSelect [list-manager.md Sec 5].
 */
int FlairLGetSelect(const FlairList *l, Cell c);

/*
 * FlairLSetSelect -- set (sel != 0) or clear (sel == 0) the selection of c.
 *
 * Returns FLAIR_LIST_OK (0) on success.
 * Returns FLAIR_LIST_ERR_BOUNDS if c is outside dataBounds (no change).
 * Returns FLAIR_LIST_ERR_NULL if l is NULL.
 * Ref: LSetSelect [list-manager.md Sec 5].
 */
int FlairLSetSelect(FlairList *l, int sel, Cell c);

#endif /* FLAIR_LIST_H */
