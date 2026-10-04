/*
 * os/i123/core/sheet.c -- the Initech 123 worksheet model (THE ARTIFACT).
 *
 * bead: initech-9u8w. See i123.h for the contract.
 * Ref:  spec/i123/wk1_format.h (template, defaults, the typed-literal rule);
 *       ../lotus123-decomp/specs/file-formats/wk1-format.md Sec 3.3 (RANGE),
 *       Sec 3.6 (HIDVEC1), Sec 4.9 (INTEGER vs NUMBER).
 *
 * EXTENTS (RANGE and HIDVEC1). Over the 44 goldens neither record is a pure
 * function of the cells a sheet holds (4 shipped sheets disagree with any
 * bounding-box rule; this lane's survey), so the model carries them: a read
 * keeps the file's values verbatim, and an ENTRY grows them -- RANGE to
 * (0,0)..(max col, max row), the column's HIDVEC1 span to cover the row. On
 * the six minted fresh sheets that rule reproduces 123.EXE byte-for-byte
 * (test-i123-wk1-roundtrip). RANGE's (0,0) corner holds in 44/44. What 123.EXE
 * does on an ERASE is not minted [oracle-resolves]: the extents never shrink
 * here. A BLANK (format-only) entry grows RANGE but not HIDVEC1 [inferred:
 * the shipped files split both ways].
 *
 * Freestanding; ASCII-clean (Rule 12).
 */
#include "i123.h"
#include "i123/wk1_format.h"

/* ---- freestanding byte helpers ------------------------------------------ */
static void bcopy_up(uint8_t *d, const uint8_t *s, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}

static void bzero8(uint8_t *d, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) d[i] = 0u;
}

static uint32_t key_of(uint16_t col, uint16_t row)
{
    return ((uint32_t)row << 8) | (uint32_t)col;
}

/* Index of the first cell whose key >= key. */
static uint32_t lower_bound(const i123_sheet_t *sh, uint32_t key)
{
    uint32_t lo = 0, hi = sh->ncells;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2u;
        if (key_of(sh->cells[mid].col, sh->cells[mid].row) < key) lo = mid + 1u;
        else hi = mid;
    }
    return lo;
}

/* ---- the template (spec/i123/wk1_format.h Sec 9) ------------------------ */
static const uint8_t g_template[] = WK1_DEFAULT_TEMPLATE_INIT;
static const uint8_t g_win1_default[32] = WK1_W1_DEFAULT_INIT;

#define CARRY_PLACEHOLDER 0xFFFFu

static int carry_put(i123_sheet_t *sh, uint16_t op, uint16_t len,
                     const uint8_t *body)
{
    uint32_t need = WK1_REC_HDR + (len == CARRY_PLACEHOLDER ? 0u : len);
    if (sh->carry_len + need > I123_CARRY_CAP) return I123_E_FULL;
    sh->carry[sh->carry_len + 0] = (uint8_t)(op & 0xFFu);
    sh->carry[sh->carry_len + 1] = (uint8_t)(op >> 8);
    sh->carry[sh->carry_len + 2] = (uint8_t)(len & 0xFFu);
    sh->carry[sh->carry_len + 3] = (uint8_t)(len >> 8);
    if (len != CARRY_PLACEHOLDER && body != (const uint8_t *)0)
        bcopy_up(sh->carry + sh->carry_len + WK1_REC_HDR, body, len);
    sh->carry_len += need;
    return I123_OK;
}

/* Decode the run-length template into the carry. A run total that disagrees
 * with the record length is a spec-data bug: refused (Rule 2). */
static int install_template(i123_sheet_t *sh)
{
    uint32_t i = 0, n = (uint32_t)sizeof g_template;
    sh->carry_len = 0;
    while (i < n) {
        uint16_t op, len;
        if (i + 4u > n) return I123_E_FORMAT;
        op  = (uint16_t)(g_template[i] | (g_template[i + 1] << 8));
        len = (uint16_t)(g_template[i + 2] | (g_template[i + 3] << 8));
        i += 4u;
        if (len == 0u) {
            if (carry_put(sh, op, CARRY_PLACEHOLDER, (const uint8_t *)0) != I123_OK)
                return I123_E_FULL;
            continue;
        }
        if (sh->carry_len + WK1_REC_HDR + len > I123_CARRY_CAP) return I123_E_FULL;
        {
            uint32_t at = sh->carry_len + WK1_REC_HDR, got = 0;
            for (;;) {
                uint8_t cnt, byte;
                if (i >= n) return I123_E_FORMAT;
                cnt = g_template[i++];
                if (cnt == 0u) break;
                if (i >= n) return I123_E_FORMAT;
                byte = g_template[i++];
                if (got + cnt > len) return I123_E_FORMAT;
                for (uint32_t k = 0; k < cnt; k++) sh->carry[at + got + k] = byte;
                got += cnt;
            }
            if (got != len) return I123_E_FORMAT;
            if (carry_put(sh, op, len, (const uint8_t *)0) != I123_OK)
                return I123_E_FULL;
            /* carry_put copied nothing (body NULL); the runs are already in
             * place at the record's body. */
        }
    }
    sh->carry_cellpos = sh->carry_len;
    return I123_OK;
}

/* ---- init / clear --------------------------------------------------------- */
static int sheet_reset(i123_sheet_t *sh)
{
    sh->ncells = 0;
    sh->pool_used = 0;
    sh->pool_live = 0;
    sh->bof_version = (uint16_t)WK1_BOF_VERSION;
    sh->calcmode    = (uint8_t)WK1_DEF_CALCMODE;
    sh->calcorder   = (uint8_t)WK1_DEF_CALCORDER;
    sh->calccount   = (uint8_t)WK1_DEF_CALCCOUNT;
    sh->split       = (uint8_t)WK1_DEF_SPLIT;
    sh->sync        = (uint8_t)WK1_DEF_SYNC;
    sh->protec      = (uint8_t)WK1_DEF_PROTEC;
    sh->labelfmt    = (uint8_t)WK1_DEF_LABELFMT;
    sh->unformatted = (uint8_t)WK1_DEF_UNFORMATTED;
    bcopy_up(sh->win1, g_win1_default, 32u);
    bzero8(sh->colw, 256u);
    for (int i = 0; i < 4; i++) sh->range[i] = 0;
    sh->range_set = 0;
    bzero8(sh->hid_set, 256u);
    for (int i = 0; i < 256; i++) { sh->hid_first[i] = 0; sh->hid_last[i] = 0; }
    sh->err_off = 0;
    return install_template(sh);
}

int i123_sheet_init(i123_sheet_t *sh, void *mem, uint32_t len, uint32_t cell_cap)
{
    uintptr_t p, end;
    uint32_t cells_bytes;

    if (sh == (i123_sheet_t *)0 || mem == (void *)0 || cell_cap == 0u)
        return I123_E_ARG;
    p   = ((uintptr_t)mem + 7u) & ~(uintptr_t)7u;
    end = (uintptr_t)mem + len;
    cells_bytes = cell_cap * (uint32_t)sizeof(i123_cell_t);
    if (p > end || end - p < (uintptr_t)cells_bytes + I123_CARRY_CAP + 256u)
        return I123_E_ARG;
    sh->cells = (i123_cell_t *)p;
    sh->cap   = cell_cap;
    p += cells_bytes;
    sh->carry = (uint8_t *)p;
    p += I123_CARRY_CAP;
    sh->pool     = (uint8_t *)p;
    sh->pool_cap = (uint32_t)(end - p);
    return sheet_reset(sh);
}

void i123_sheet_clear(i123_sheet_t *sh)
{
    (void)sheet_reset(sh);
}

/* ---- the pool -------------------------------------------------------------- */

/* Slide every live blob down to the bottom of the pool, in offset order
 * (moving a blob down never overwrites one not yet moved). O(n^2) selection:
 * a compaction is rare (only when the pool fills) and needs no scratch. */
static void pool_compact(i123_sheet_t *sh)
{
    uint32_t dst = 0, scan = 0;
    for (;;) {
        i123_cell_t *best = (i123_cell_t *)0;
        for (uint32_t i = 0; i < sh->ncells; i++) {
            i123_cell_t *c = &sh->cells[i];
            uint32_t sz = (uint32_t)c->len + (uint32_t)c->slen;
            if (sz == 0u || c->off < scan) continue;
            if (best == (i123_cell_t *)0 || c->off < best->off) best = c;
        }
        if (best == (i123_cell_t *)0) break;
        {
            uint32_t sz = (uint32_t)best->len + (uint32_t)best->slen;
            scan = best->off + 1u;
            if (best->off != dst) {
                for (uint32_t k = 0; k < sz; k++)
                    sh->pool[dst + k] = sh->pool[best->off + k];
                best->off = dst;
            }
            dst += sz;
        }
    }
    sh->pool_used = dst;
    sh->pool_live = dst;
}

static int pool_alloc(i123_sheet_t *sh, uint32_t n, uint32_t *off)
{
    if (n == 0u) { *off = 0; return I123_OK; }
    if (sh->pool_used + n > sh->pool_cap) {
        pool_compact(sh);
        if (sh->pool_used + n > sh->pool_cap) return I123_E_FULL;
    }
    *off = sh->pool_used;
    sh->pool_used += n;
    sh->pool_live += n;
    return I123_OK;
}

/* ---- cells ----------------------------------------------------------------- */
const i123_cell_t *i123_find(const i123_sheet_t *sh, uint16_t col, uint16_t row)
{
    uint32_t k = key_of(col, row);
    uint32_t i = lower_bound(sh, k);
    if (i < sh->ncells && key_of(sh->cells[i].col, sh->cells[i].row) == k)
        return &sh->cells[i];
    return (const i123_cell_t *)0;
}

const uint8_t *i123_cell_bytes(const i123_sheet_t *sh, const i123_cell_t *c)
{
    return sh->pool + c->off;
}

/* Store a cell (insert or replace). b1/n1 = label prefix+text or RPN code;
 * b2/n2 = a string formula's result text. Exported to the codec through
 * i123_put_cell (wk1.c declares it extern; not part of the public API). */
int i123_put_cell(i123_sheet_t *sh, uint16_t col, uint16_t row, uint8_t kind,
                  uint8_t fmt, const uint8_t v[8], const uint8_t *b1, uint32_t n1,
                  int has_str, uint8_t str_fmt, const uint8_t *b2, uint32_t n2)
{
    uint32_t k, i, off = 0;
    int exists, rc;
    i123_cell_t *c;

    if (col >= I123_MAX_COLS || row >= I123_MAX_ROWS) return I123_E_ARG;
    if (n1 > 0xFFFFu || n2 > 0xFFFFu) return I123_E_ARG;
    k = key_of(col, row);
    i = lower_bound(sh, k);
    exists = (i < sh->ncells && key_of(sh->cells[i].col, sh->cells[i].row) == k);
    if (!exists && sh->ncells >= sh->cap) return I123_E_FULL;

    rc = pool_alloc(sh, n1 + n2, &off);
    if (rc != I123_OK) return rc;
    /* pool_alloc may have compacted: re-derive nothing -- indices are stable,
     * only blob offsets moved. */
    if (n1) bcopy_up(sh->pool + off, b1, n1);
    if (n2) bcopy_up(sh->pool + off + n1, b2, n2);

    if (exists) {
        c = &sh->cells[i];
        sh->pool_live -= (uint32_t)c->len + (uint32_t)c->slen;
    } else {
        for (uint32_t j = sh->ncells; j > i; j--) sh->cells[j] = sh->cells[j - 1u];
        sh->ncells++;
        c = &sh->cells[i];
    }
    c->col = col;
    c->row = row;
    c->kind = kind;
    c->fmt = fmt;
    c->has_str = (uint8_t)(has_str ? 1u : 0u);
    c->str_fmt = str_fmt;
    c->len = (uint16_t)n1;
    c->slen = (uint16_t)n2;
    c->off = off;
    for (int b = 0; b < 8; b++) c->v[b] = v ? v[b] : 0u;
    return I123_OK;
}

static void grow_extents(i123_sheet_t *sh, uint16_t col, uint16_t row, uint8_t kind)
{
    if (!sh->range_set) {
        sh->range[0] = 0; sh->range[1] = 0;
        sh->range[2] = (int16_t)col; sh->range[3] = (int16_t)row;
        sh->range_set = 1;
    } else {
        if ((int16_t)col > sh->range[2]) sh->range[2] = (int16_t)col;
        if ((int16_t)row > sh->range[3]) sh->range[3] = (int16_t)row;
    }
    if (kind == I123_K_BLANK) return;
    if (!sh->hid_set[col]) {
        sh->hid_set[col] = 1;
        sh->hid_first[col] = row;
        sh->hid_last[col] = row;
    } else {
        if (row < sh->hid_first[col]) sh->hid_first[col] = row;
        if (row > sh->hid_last[col]) sh->hid_last[col] = row;
    }
}

/* The format a new entry takes: an existing cell keeps its own (a formatted
 * BLANK typed into keeps the format); a new cell gets WK1_FMT_FRESH_CELL,
 * the byte on every cell 123.EXE wrote in the minted set. */
static uint8_t entry_fmt(const i123_sheet_t *sh, uint16_t col, uint16_t row)
{
    const i123_cell_t *c = i123_find(sh, col, row);
    return c ? c->fmt : (uint8_t)WK1_FMT_FRESH_CELL;
}

#define I123_LABEL_MAX 240u   /* 1-2-3's label ceiling; the corpus' longest is
                               * 78 [oracle-resolves] -- a guard, not a claim */

int i123_set_label(i123_sheet_t *sh, uint16_t col, uint16_t row,
                   uint8_t prefix, const char *text, uint32_t len)
{
    uint8_t buf[1 + I123_LABEL_MAX];
    int rc;
    if (col >= I123_MAX_COLS || row >= I123_MAX_ROWS) return I123_E_ARG;
    if (len > I123_LABEL_MAX) return I123_E_ARG;
    buf[0] = prefix;
    for (uint32_t i = 0; i < len; i++) {
        if (text[i] == '\0') return I123_E_ARG;   /* a NUL ends a WK1 label */
        buf[1 + i] = (uint8_t)text[i];
    }
    rc = i123_put_cell(sh, col, row, I123_K_LABEL, entry_fmt(sh, col, row),
                       (const uint8_t *)0, buf, 1u + len, 0, 0, (const uint8_t *)0, 0);
    if (rc == I123_OK) grow_extents(sh, col, row, I123_K_LABEL);
    return rc;
}

/* A typed number: INTEGER when integral within WK1_INT_MIN..WK1_INT_MAX
 * (MINT05: 32767 INTEGER, 32768 / -32768 / -32769 NUMBER), else NUMBER. */
int i123_set_number(i123_sheet_t *sh, uint16_t col, uint16_t row, double x)
{
    uint8_t v[8];
    uint8_t kind;
    int rc;
    if (col >= I123_MAX_COLS || row >= I123_MAX_ROWS) return I123_E_ARG;
    if (x >= (double)WK1_INT_MIN && x <= (double)WK1_INT_MAX &&
        (double)(int32_t)x == x) {
        int32_t iv = (int32_t)x;
        v[0] = (uint8_t)((uint32_t)iv & 0xFFu);
        v[1] = (uint8_t)(((uint32_t)iv >> 8) & 0xFFu);
        for (int b = 2; b < 8; b++) v[b] = 0u;
        kind = I123_K_INTEGER;
    } else {
        union { double d; uint8_t b[8]; } u;
        u.d = x;
        for (int b = 0; b < 8; b++) v[b] = u.b[b];
        kind = I123_K_NUMBER;
    }
    rc = i123_put_cell(sh, col, row, kind, entry_fmt(sh, col, row), v,
                       (const uint8_t *)0, 0, 0, 0, (const uint8_t *)0, 0);
    if (rc == I123_OK) grow_extents(sh, col, row, kind);
    return rc;
}

int i123_set_formula_raw(i123_sheet_t *sh, uint16_t col, uint16_t row,
                         uint8_t fmt, const uint8_t cache[8],
                         const uint8_t *code, uint32_t code_len,
                         const char *str, uint32_t str_len)
{
    int rc = i123_put_cell(sh, col, row, I123_K_FORMULA, fmt, cache, code,
                           code_len, str != (const char *)0, fmt,
                           (const uint8_t *)str, str_len);
    if (rc == I123_OK) grow_extents(sh, col, row, I123_K_FORMULA);
    return rc;
}

int i123_erase(i123_sheet_t *sh, uint16_t col, uint16_t row)
{
    uint32_t k = key_of(col, row);
    uint32_t i = lower_bound(sh, k);
    if (i >= sh->ncells || key_of(sh->cells[i].col, sh->cells[i].row) != k)
        return I123_OK;   /* erasing an empty cell is a no-op, as in 1-2-3 */
    sh->pool_live -= (uint32_t)sh->cells[i].len + (uint32_t)sh->cells[i].slen;
    for (uint32_t j = i; j + 1u < sh->ncells; j++) sh->cells[j] = sh->cells[j + 1u];
    sh->ncells--;
    return I123_OK;
}

int i123_set_cell_format(i123_sheet_t *sh, uint16_t col, uint16_t row, uint8_t fmt)
{
    const i123_cell_t *c = i123_find(sh, col, row);
    int rc;
    if (c != (const i123_cell_t *)0) {
        ((i123_cell_t *)c)->fmt = fmt;
        return I123_OK;
    }
    rc = i123_put_cell(sh, col, row, I123_K_BLANK, fmt, (const uint8_t *)0,
                       (const uint8_t *)0, 0, 0, 0, (const uint8_t *)0, 0);
    if (rc == I123_OK) grow_extents(sh, col, row, I123_K_BLANK);
    return rc;
}

/* ---- columns, cursor, view (WINDOW1 / COLW1) ------------------------------- */
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static void wr16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v & 0xFFu); p[1] = (uint8_t)(v >> 8); }

int i123_set_colwidth(i123_sheet_t *sh, uint16_t col, uint8_t width)
{
    if (col >= I123_MAX_COLS || width > 240u) return I123_E_ARG;
    sh->colw[col] = width;
    return I123_OK;
}

uint32_t i123_colwidth(const i123_sheet_t *sh, uint16_t col)
{
    if (col >= I123_MAX_COLS) return 0;
    return sh->colw[col] ? sh->colw[col] : rd16(sh->win1 + WK1_W1_WIDTH);
}

void i123_set_cursor(i123_sheet_t *sh, uint16_t col, uint16_t row)
{
    wr16(sh->win1 + WK1_W1_CUR_COL, col);
    wr16(sh->win1 + WK1_W1_CUR_ROW, row);
}

uint16_t i123_cursor_col(const i123_sheet_t *sh) { return rd16(sh->win1 + WK1_W1_CUR_COL); }
uint16_t i123_cursor_row(const i123_sheet_t *sh) { return rd16(sh->win1 + WK1_W1_CUR_ROW); }

void i123_set_view(i123_sheet_t *sh, uint16_t left, uint16_t top)
{
    wr16(sh->win1 + WK1_W1_LEFT, left);
    wr16(sh->win1 + WK1_W1_TOP, top);
}

uint16_t i123_view_left(const i123_sheet_t *sh) { return rd16(sh->win1 + WK1_W1_LEFT); }
uint16_t i123_view_top(const i123_sheet_t *sh) { return rd16(sh->win1 + WK1_W1_TOP); }

uint32_t i123_cols_fitting(const i123_sheet_t *sh, uint16_t left, uint32_t chars)
{
    uint32_t tot = 0, n = 0;
    for (uint32_t c = left; c < I123_MAX_COLS; c++) {
        uint32_t w = i123_colwidth(sh, (uint16_t)c);
        if (tot + w > chars) break;
        tot += w;
        n++;
    }
    return n;
}
