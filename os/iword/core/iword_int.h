/*
 * os/iword/core/iword_int.h -- InitechWord core INTERNALS shared by the line
 * layout (edit.c) and the page features (page.c): the format in effect and
 * the line walk with its pagination (THE ARTIFACT; bead initech-8q6j, IWORD
 * P4). Not part of the core's API (iword.h); no face includes it.
 *
 * Freestanding (no libc). Deterministic (Rule 11). ASCII-clean (Rule 12).
 */
#ifndef INITECH_IWORD_INT_H
#define INITECH_IWORD_INT_H

#include "iword.h"

/* The format in effect (P2 slice 3: margins, justification, spacing; P4:
 * the header / footer definitions, newspaper columns, tables --
 * spec/iword/wp5_page.h). */
typedef struct fmt {
    uint32_t lm, rm, just, set;
    uint32_t sp;                 /* line spacing, 8.8 fixed (slice 3, MINT25) */
    /* P4 */
    uint8_t  hocc[4];            /* header / footer definitions in effect:
                                  * IW_HF_EVERY / ODD / EVEN, 0 none       */
    uint8_t  hlines[4];          /* ... and their lines                     */
    uint8_t  colon, ncols, col, tbl;
    uint32_t slm, srm, sset;     /* the margins [Col On] put aside          */
    uint32_t coldef;             /* the [Col Def] in effect (a position)    */
    uint32_t tdef;               /* the table's [Tbl Def] (tbl != 0)        */
    uint32_t tcols;
} fmt_t;

/* The walk down the lines, paginated (P4: the pages hold what the headers,
 * footers and columns leave; spec/iword/wp5_page.h "the page"). y is where
 * the line starts below the top margin, in 1/256ths of a line (a header's
 * space included: the status line's Ln is 1" + y). */
typedef struct walk {
    iw_line_t ln;
    uint32_t  index, page, page_line;
    uint32_t  y;
    fmt_t     fin, fout;         /* the format at the line's start / end */
    /* P4 */
    uint32_t  top, bottom;       /* the page's text: [top, bottom) in y units */
    uint32_t  col_top, col_max;  /* newspaper columns: the block's top, the
                                  * furthest a column reached               */
    uint8_t   phocc[4], phlines[4];   /* the definitions this page uses     */
    uint8_t   soft;              /* this line starts a page (or column) the
                                  * text flowed onto -- not a hard page     */
    uint8_t   pad[3];
} walk_t;

#define IW_Y_LINE   256u         /* one line in y units                     */
/* WPU <-> y units (200 WPU a line) */
#define IW_WPU_TO_Y(v) (((uint32_t)(v) * 256u + 100u) / 200u)
#define IW_Y_TO_WPU(y) (((uint32_t)(y) * 200u + 128u) / 256u)

void     iw__walk_first(const iw_ed_t *e, walk_t *w);
int      iw__walk_next(const iw_ed_t *e, walk_t *w);
void     iw__fmt_at(const iw_doc_t *d, uint32_t pos, fmt_t *f);
/* The header / footer space (WPU) a page of parity `odd` takes under the
 * definitions occ / lines (0 when none applies). */
uint32_t iw__hf_space(const uint8_t *occ, const uint8_t *lines, int footer, int odd);
/* The relative column a [Cell] of index idx starts its text at (the table
 * at tdef), and the WPU x of its left edge. */
uint32_t iw__cell_left(const iw_doc_t *d, uint32_t tdef, uint32_t idx);
/* page.c's part of iw_ed_fixup: notes, tables (run after the layout's). */
int      iw__p4_fixup(iw_ed_t *e);
/* The paper's wrap (a character ENDS by the right margin) on / off for the
 * layout (iw_print's copy only). */
void     iw__print_wrap(int on);
/* The digits of n (the width of a note's number in the text). */
uint32_t iw__digits(uint32_t n);

#endif /* INITECH_IWORD_INT_H */
