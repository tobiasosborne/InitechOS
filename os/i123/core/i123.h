/*
 * os/i123/core/i123.h -- Initech 123 core: the worksheet model, the .WK1
 * codec and the cell display rules (THE ARTIFACT; ADR-0002 C).
 *
 * bead: initech-9u8w (I123 P1 -- grid, labels, numbers, .WK1 round-trip).
 * Plan: docs/plans/INITECH-123-plan.md Sec 3.3 (one core, no OS calls in
 *       core/fs), Sec 4 (feature inventory), Sec 8 (sparse store: populated
 *       cells only, never a dense 256 x 8192 array).
 * Ref:  spec/i123/wk1_format.h (the locked record set, re-derived from the
 *       real 1-2-3 R2.2 goldens in ../lotus123-decomp);
 *       ../lotus123-decomp/specs/file-formats/wk1-format.md;
 *       ../lotus123-decomp/specs/functions/function-value-table.md Sec 3
 *       (the General-format display facts graded by test-i123-wk1-roundtrip).
 *
 * WHAT THE CORE IS (and is not). The core is a pure library over memory the
 * CALLER supplies -- no allocator, no file system, no Toolbox, no libc. The
 * .WK1 codec reads from and writes to a byte buffer; whoever owns the disk
 * (the host oracle, the tenant's future /File verbs) moves the bytes. That is
 * why P1 needs no PAL vtable (plan Sec 3.3 names one for the OS seams; P1 has
 * none: the clock and file I/O arrive with /File in P3).
 *
 * THE STORE. Populated cells only, kept in one array sorted ROW-MAJOR by
 * (row, col) -- the order 123.EXE writes them (wk1-format.md Sec 1.1, 44/44)
 * -- found by binary search. Variable bytes (label text, formula RPN, a
 * string formula's result) live in a byte pool, compacted when it fills.
 *
 * FORMULAS ARE CARRIED, NOT EVALUATED (P2 owns evaluation, bead
 * initech-94ah): a FORMULA record's format byte, 8 raw cache bytes, RPN code
 * and its following STRING record are kept verbatim and written back byte-
 * exact. The cache is kept as RAW BYTES, never as a double, because 123.EXE
 * caches a string formula's value as a SIGNALLING NaN (MINT01 @0x613
 * `fd 9f 0a 00 00 00 f0 7f`, mantissa bit 51 clear) and an x87 FLD/FSTP
 * pair would quiet it -- one changed byte in every string formula on save.
 *
 * Freestanding: <stdint.h> only; dual-compiled (host oracle, tenant).
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#ifndef INITECH_I123_H
#define INITECH_I123_H

#include <stdint.h>

/* ---- results (Rule 2: a malformed file is refused, never half-loaded) ---- */
#define I123_OK          0
#define I123_E_ARG     (-1)   /* a bad argument (address off-sheet, width 0 ...) */
#define I123_E_FULL    (-2)   /* the cell array or the pool is full             */
#define I123_E_FORMAT  (-3)   /* the file is not a well-formed R2.2 .WK1         */
#define I123_E_DUP     (-4)   /* a model-owned record or a cell occurs twice    */
#define I123_E_SPACE   (-5)   /* the output buffer is too small                 */
#define I123_E_VALUE   (-6)   /* typed text is not a value the parser accepts   */

/* ---- cell kinds ---------------------------------------------------------- */
#define I123_K_BLANK     1u   /* formatted, no content (WK1 BLANK)              */
#define I123_K_INTEGER   2u   /* WK1 INTEGER: i16 in v[0..1]                    */
#define I123_K_NUMBER    3u   /* WK1 NUMBER: f64 LE in v[0..7]                  */
#define I123_K_LABEL     4u   /* WK1 LABEL: pool = prefix + text (no NUL)       */
#define I123_K_FORMULA   5u   /* WK1 FORMULA: v = cache bytes, pool = RPN code
                               * [+ the STRING result text when has_str]        */

typedef struct i123_cell {
    uint16_t col, row;
    uint8_t  kind;
    uint8_t  fmt;             /* the record's format byte, verbatim             */
    uint8_t  has_str;         /* FORMULA followed by a STRING record            */
    uint8_t  str_fmt;         /* that STRING record's own format byte           */
    uint16_t len;             /* pool bytes: label prefix+text / RPN code       */
    uint16_t slen;            /* STRING result text bytes (has_str)             */
    uint32_t off;             /* pool offset of the blob (len + slen bytes)     */
    uint8_t  v[8];            /* INTEGER / NUMBER / FORMULA-cache raw bytes     */
} i123_cell_t;

#define I123_CARRY_CAP   4096u   /* the largest non-cell record set over the 44
                                  * goldens is 2726 B (QSTARGET), plus the
                                  * placeholders                                */

typedef struct i123_sheet {
    i123_cell_t *cells;
    uint32_t     ncells, cap;
    uint8_t     *pool;
    uint32_t     pool_used, pool_cap, pool_live;
    uint8_t     *carry;          /* non-cell records in file order (see wk1.c) */
    uint32_t     carry_len, carry_cellpos;
    /* model-owned header state (spec/i123/wk1_format.h Sec 2) */
    uint16_t     bof_version;
    uint8_t      calcmode, calcorder, calccount, split, sync, protec;
    uint8_t      labelfmt, unformatted;
    uint8_t      win1[32];       /* WINDOW1 body; ncols is re-derived on write */
    uint8_t      colw[256];      /* per-column width, 0 = no COLW1 record      */
    int16_t      range[4];       /* RANGE c0,r0,c1,r1                          */
    uint8_t      range_set;
    uint8_t      hid_set[256];   /* HIDVEC1: column has a span                 */
    uint16_t     hid_first[256], hid_last[256];
    uint32_t     err_off;        /* file offset of the record a read refused   */
} i123_sheet_t;

/* ---- the model ----------------------------------------------------------- */

/* Carve `mem` (len bytes) into `cell_cap` cells, the carry area and the pool,
 * and make a FRESH sheet: the MINT01 record template and defaults
 * (spec/i123/wk1_format.h Sec 4, Sec 9), cursor A1, no cells. */
int  i123_sheet_init(i123_sheet_t *sh, void *mem, uint32_t len, uint32_t cell_cap);
void i123_sheet_clear(i123_sheet_t *sh);           /* back to a fresh sheet */

const i123_cell_t *i123_find(const i123_sheet_t *sh, uint16_t col, uint16_t row);
const uint8_t     *i123_cell_bytes(const i123_sheet_t *sh, const i123_cell_t *c);

/* Entry verbs: what typing into a cell does. Each grows the RANGE / HIDVEC1
 * extents the way the minted fresh sheets show (wk1.c "EXTENTS"). */
int  i123_set_label(i123_sheet_t *sh, uint16_t col, uint16_t row,
                    uint8_t prefix, const char *text, uint32_t len);
int  i123_set_number(i123_sheet_t *sh, uint16_t col, uint16_t row, double x);
int  i123_set_formula_raw(i123_sheet_t *sh, uint16_t col, uint16_t row,
                          uint8_t fmt, const uint8_t cache[8],
                          const uint8_t *code, uint32_t code_len,
                          const char *str, uint32_t str_len);
int  i123_erase(i123_sheet_t *sh, uint16_t col, uint16_t row);
int  i123_set_cell_format(i123_sheet_t *sh, uint16_t col, uint16_t row, uint8_t fmt);

int      i123_set_colwidth(i123_sheet_t *sh, uint16_t col, uint8_t width); /* 0 = reset */
uint32_t i123_colwidth(const i123_sheet_t *sh, uint16_t col);
void     i123_set_cursor(i123_sheet_t *sh, uint16_t col, uint16_t row);
uint16_t i123_cursor_col(const i123_sheet_t *sh);
uint16_t i123_cursor_row(const i123_sheet_t *sh);
void     i123_set_view(i123_sheet_t *sh, uint16_t left, uint16_t top);
uint16_t i123_view_left(const i123_sheet_t *sh);
uint16_t i123_view_top(const i123_sheet_t *sh);
/* Whole columns, from `left`, that fit in `chars` characters. */
uint32_t i123_cols_fitting(const i123_sheet_t *sh, uint16_t left, uint32_t chars);

/* ---- the .WK1 codec (os/i123/fs/wk1.c) ------------------------------------ */

/* Replace the sheet's contents with the file in buf[0..len). On error the
 * sheet is left FRESH (never half-loaded) and sh->err_off names the record. */
int  i123_wk1_read(i123_sheet_t *sh, const uint8_t *buf, uint32_t len);
/* Write the sheet; *out_len receives the byte count. */
int  i123_wk1_write(const i123_sheet_t *sh, uint8_t *out, uint32_t cap,
                    uint32_t *out_len);

/* ---- display (os/i123/core/fmt.c) ----------------------------------------- */

/* The cell's own text in a column `width` characters wide: exactly `width`
 * characters + NUL in out[]. *spill receives the length of a label's text
 * when it is LEFT-drawn and longer than the column (the caller lets it run
 * into empty cells to the right), else 0. */
void i123_cell_text(const i123_sheet_t *sh, const i123_cell_t *c,
                    uint32_t width, char *out, uint32_t *spill);
/* One worksheet row as the screen shows it: `nchars` characters from column
 * `left` (whole columns only; a left label spills into empty neighbours),
 * + NUL in line[]. */
void i123_render_row(const i123_sheet_t *sh, uint16_t row, uint16_t left,
                     uint32_t nchars, char *line);
/* The control-panel content of a cell as line 1 shows it after "A1: ":
 * a label with its prefix, a number in up to 15 significant digits. */
void i123_cell_content(const i123_sheet_t *sh, const i123_cell_t *c,
                       char *out, uint32_t cap);
/* A cell address as text ("A1", "IV8192"); returns the length. */
uint32_t i123_addr_text(uint16_t col, uint16_t row, char *out);
/* The column letters ("A".."IV"); returns the length. */
uint32_t i123_col_name(uint16_t col, char *out);

/* Typed-value parser (a numeric literal: sign, digits, point, exponent).
 * Formulas are P2: anything else is I123_E_VALUE (1-2-3 beeps into EDIT). */
int  i123_parse_value(const char *s, uint32_t len, double *out);

/* Exact width-format of a number for format byte `fmt` (already resolved
 * against the global default), exactly `width` chars + NUL. */
void i123_format_number(double x, uint8_t fmt, uint32_t width, char *out);

#endif /* INITECH_I123_H */
