/*
 * os/i123/fs/wk1.c -- the .WK1 reader and writer (THE ARTIFACT).
 *
 * bead: initech-9u8w. See os/i123/core/i123.h for the contract.
 * Ref:  spec/i123/wk1_format.h (every opcode / length / default used here);
 *       ../lotus123-decomp/specs/file-formats/wk1-format.md Sec 1 (framing),
 *       Sec 1.1 (record order), Sec 4 (cell records), Sec 4.6 (STRING follows
 *       its FORMULA), Sec 9 (what a conforming writer emits).
 *
 * THE CARRY. Records the model does not own (print/sort/query ranges, named
 * ranges, headers, fonts, graph settings, and anything unknown) are kept
 * BYTE-EXACT in file order; the model-owned records keep a placeholder at
 * their file position and are regenerated from the model on write, WINDOW1's
 * placeholder emitting the COLW1 group after it (spec Sec 3.5: COLW1 directly
 * follows WINDOW1, ascending, 44/44). The cells are written where the file
 * had its cell run (one contiguous run, 44/44), in row-major order. So a sheet
 * read from a 123.EXE file and written unchanged reproduces the file byte-
 * for-byte -- graded over all 44 goldens -- and a fresh sheet writes the
 * MINT01 record set.
 *
 * Freestanding; ASCII-clean (Rule 12).
 */
#include "i123.h"
#include "i123/wk1_format.h"

/* sheet.c's internal store verb (not public API). */
int i123_put_cell(i123_sheet_t *sh, uint16_t col, uint16_t row, uint8_t kind,
                  uint8_t fmt, const uint8_t v[8], const uint8_t *b1, uint32_t n1,
                  int has_str, uint8_t str_fmt, const uint8_t *b2, uint32_t n2);

#define CARRY_PLACEHOLDER 0xFFFFu
#define CELLPOS_UNSET     0xFFFFFFFFu

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

/* ---- reader ---------------------------------------------------------------- */
static int is_cell_op(uint16_t op)
{
    return op == WK1_OP_BLANK || op == WK1_OP_INTEGER || op == WK1_OP_NUMBER ||
           op == WK1_OP_LABEL || op == WK1_OP_FORMULA;
}

static int owned_byte(i123_sheet_t *sh, uint16_t op, uint8_t **slot)
{
    switch (op) {
    case WK1_OP_CALCMODE:    *slot = &sh->calcmode;    return 1;
    case WK1_OP_CALCORDER:   *slot = &sh->calcorder;   return 1;
    case WK1_OP_SPLIT:       *slot = &sh->split;       return 1;
    case WK1_OP_SYNC:        *slot = &sh->sync;        return 1;
    case WK1_OP_PROTEC:      *slot = &sh->protec;      return 1;
    case WK1_OP_LABELFMT:    *slot = &sh->labelfmt;    return 1;
    case WK1_OP_CALCCOUNT:   *slot = &sh->calccount;   return 1;
    case WK1_OP_UNFORMATTED: *slot = &sh->unformatted; return 1;
    default: return 0;
    }
}

/* A dedicated bit per owned singleton, for the duplicate check. */
static uint32_t owned_bit(uint16_t op)
{
    switch (op) {
    case WK1_OP_CALCMODE:    return 1u << 0;
    case WK1_OP_CALCORDER:   return 1u << 1;
    case WK1_OP_SPLIT:       return 1u << 2;
    case WK1_OP_SYNC:        return 1u << 3;
    case WK1_OP_PROTEC:      return 1u << 4;
    case WK1_OP_LABELFMT:    return 1u << 5;
    case WK1_OP_CALCCOUNT:   return 1u << 6;
    case WK1_OP_UNFORMATTED: return 1u << 7;
    case WK1_OP_RANGE:       return 1u << 8;
    case WK1_OP_HIDVEC1:     return 1u << 9;
    case WK1_OP_WINDOW1:     return 1u << 10;
    default: return 0u;
    }
}

static int carry_rec(i123_sheet_t *sh, uint16_t op, uint32_t len, const uint8_t *body)
{
    uint32_t need = WK1_REC_HDR + (len == CARRY_PLACEHOLDER ? 0u : len);
    if (sh->carry_len + need > I123_CARRY_CAP) return I123_E_FULL;
    sh->carry[sh->carry_len + 0] = (uint8_t)(op & 0xFFu);
    sh->carry[sh->carry_len + 1] = (uint8_t)(op >> 8);
    sh->carry[sh->carry_len + 2] = (uint8_t)(len & 0xFFu);
    sh->carry[sh->carry_len + 3] = (uint8_t)((len >> 8) & 0xFFu);
    if (len != CARRY_PLACEHOLDER)
        for (uint32_t i = 0; i < len; i++)
            sh->carry[sh->carry_len + WK1_REC_HDR + i] = body[i];
    sh->carry_len += need;
    return I123_OK;
}

/* One cell record (and, for a FORMULA, the STRING record right after it). */
static int read_cell(i123_sheet_t *sh, uint16_t op, const uint8_t *b, uint32_t ln,
                     const uint8_t *next, uint32_t next_avail, uint32_t *consumed_next)
{
    uint8_t fmt;
    uint16_t col, row;
    *consumed_next = 0;
    if (ln < WK1_CELL_PREFIX) return I123_E_FORMAT;
    fmt = b[0];
    col = rd16(b + 1);
    row = rd16(b + 3);
    if (col >= I123_MAX_COLS || row >= I123_MAX_ROWS) return I123_E_FORMAT;
    if (i123_find(sh, col, row) != (const i123_cell_t *)0) return I123_E_DUP;

    switch (op) {
    case WK1_OP_BLANK:
        if (ln != WK1_LEN_BLANK) return I123_E_FORMAT;
        return i123_put_cell(sh, col, row, I123_K_BLANK, fmt, (const uint8_t *)0,
                             (const uint8_t *)0, 0, 0, 0, (const uint8_t *)0, 0);
    case WK1_OP_INTEGER: {
        uint8_t v[8] = { 0 };
        if (ln != WK1_LEN_INTEGER) return I123_E_FORMAT;
        v[0] = b[5]; v[1] = b[6];
        return i123_put_cell(sh, col, row, I123_K_INTEGER, fmt, v,
                             (const uint8_t *)0, 0, 0, 0, (const uint8_t *)0, 0);
    }
    case WK1_OP_NUMBER:
        if (ln != WK1_LEN_NUMBER) return I123_E_FORMAT;
        return i123_put_cell(sh, col, row, I123_K_NUMBER, fmt, b + 5,
                             (const uint8_t *)0, 0, 0, 0, (const uint8_t *)0, 0);
    case WK1_OP_LABEL: {
        /* prefix + text + exactly one NUL, at the end */
        uint32_t n = ln - WK1_CELL_PREFIX;   /* prefix + text + NUL */
        if (n < 2u || b[ln - 1u] != 0u) return I123_E_FORMAT;
        for (uint32_t i = WK1_CELL_PREFIX; i + 1u < ln; i++)
            if (b[i] == 0u) return I123_E_FORMAT;
        return i123_put_cell(sh, col, row, I123_K_LABEL, fmt, (const uint8_t *)0,
                             b + WK1_CELL_PREFIX, n - 1u, 0, 0, (const uint8_t *)0, 0);
    }
    case WK1_OP_FORMULA: {
        uint32_t code_len;
        const uint8_t *str = (const uint8_t *)0;
        uint32_t slen = 0;
        uint8_t sfmt = 0;
        int has = 0;
        if (ln < WK1_FORMULA_FIXED) return I123_E_FORMAT;
        code_len = rd16(b + 13);
        if (WK1_FORMULA_FIXED + code_len != ln) return I123_E_FORMAT;
        /* Its STRING record, if the next record is one for the same cell. */
        if (next_avail >= WK1_REC_HDR && rd16(next) == WK1_OP_STRING) {
            uint32_t sl = rd16(next + 2);
            const uint8_t *sb = next + WK1_REC_HDR;
            if (sl < WK1_CELL_PREFIX + 1u || WK1_REC_HDR + sl > next_avail)
                return I123_E_FORMAT;
            if (rd16(sb + 1) != col || rd16(sb + 3) != row) return I123_E_FORMAT;
            if (sb[sl - 1u] != 0u) return I123_E_FORMAT;
            for (uint32_t i = WK1_CELL_PREFIX; i + 1u < sl; i++)
                if (sb[i] == 0u) return I123_E_FORMAT;
            sfmt = sb[0];
            str = sb + WK1_CELL_PREFIX;
            slen = sl - WK1_CELL_PREFIX - 1u;
            has = 1;
            *consumed_next = WK1_REC_HDR + sl;
        }
        return i123_put_cell(sh, col, row, I123_K_FORMULA, fmt, b + 5,
                             b + WK1_FORMULA_FIXED, code_len, has, sfmt, str, slen);
    }
    default:
        return I123_E_FORMAT;
    }
}

static int read_body(i123_sheet_t *sh, const uint8_t *buf, uint32_t len)
{
    uint32_t off = 0, seen = 0;
    int have_colw = 0;

    sh->carry_len = 0;
    sh->carry_cellpos = CELLPOS_UNSET;

    /* BOF first */
    if (len < WK1_REC_HDR + WK1_LEN_BOF || rd16(buf) != WK1_OP_BOF ||
        rd16(buf + 2) != WK1_LEN_BOF)
        return I123_E_FORMAT;
    sh->bof_version = rd16(buf + 4);
    if (sh->bof_version != WK1_BOF_VERSION) return I123_E_FORMAT;  /* R2.2 only (Law 3) */
    off = WK1_REC_HDR + WK1_LEN_BOF;

    for (;;) {
        uint16_t op;
        uint32_t ln;
        const uint8_t *b;
        uint8_t *slot;
        int rc = I123_OK;

        sh->err_off = off;
        if (off + WK1_REC_HDR > len) return I123_E_FORMAT;   /* no EOF */
        op = rd16(buf + off);
        ln = rd16(buf + off + 2);
        if (off + WK1_REC_HDR + ln > len) return I123_E_FORMAT;
        b = buf + off + WK1_REC_HDR;

        if (op == WK1_OP_EOF) {
            if (ln != 0u) return I123_E_FORMAT;
            if (off + WK1_REC_HDR != len) return I123_E_FORMAT;   /* trailing bytes */
            break;
        }
        if (owned_bit(op) != 0u) {
            if (seen & owned_bit(op)) return I123_E_DUP;
            seen |= owned_bit(op);
        }
        if (owned_byte(sh, op, &slot)) {
            if (ln != WK1_LEN_ONE) return I123_E_FORMAT;
            *slot = b[0];
            rc = carry_rec(sh, op, CARRY_PLACEHOLDER, (const uint8_t *)0);
        } else if (op == WK1_OP_RANGE) {
            if (ln != WK1_LEN_RANGE) return I123_E_FORMAT;
            for (int i = 0; i < 4; i++) sh->range[i] = (int16_t)rd16(b + 2 * i);
            sh->range_set = 1;
            rc = carry_rec(sh, op, CARRY_PLACEHOLDER, (const uint8_t *)0);
        } else if (op == WK1_OP_HIDVEC1) {
            if (ln % WK1_HIDVEC_TRIPLE != 0u) return I123_E_FORMAT;
            for (uint32_t i = 0; i < ln; i += WK1_HIDVEC_TRIPLE) {
                uint16_t c = rd16(b + i);
                if (c >= I123_MAX_COLS) return I123_E_FORMAT;
                if (sh->hid_set[c]) return I123_E_DUP;
                /* ascending is what the writer emits; a file that is not
                 * would not round-trip -- refuse it rather than reorder */
                if (i > 0u && c <= rd16(b + i - WK1_HIDVEC_TRIPLE)) return I123_E_FORMAT;
                sh->hid_set[c] = 1;
                sh->hid_first[c] = rd16(b + i + 2);
                sh->hid_last[c] = rd16(b + i + 4);
            }
            rc = carry_rec(sh, op, CARRY_PLACEHOLDER, (const uint8_t *)0);
        } else if (op == WK1_OP_WINDOW1) {
            if (ln != WK1_LEN_WINDOW1) return I123_E_FORMAT;
            for (uint32_t i = 0; i < WK1_LEN_WINDOW1; i++) sh->win1[i] = b[i];
            rc = carry_rec(sh, op, CARRY_PLACEHOLDER, (const uint8_t *)0);
        } else if (op == WK1_OP_COLW1) {
            uint16_t c;
            if (ln != WK1_LEN_COLW1) return I123_E_FORMAT;
            c = rd16(b);
            if (c >= I123_MAX_COLS || b[2] == 0u) return I123_E_FORMAT;
            if (sh->colw[c] != 0u) return I123_E_DUP;
            sh->colw[c] = b[2];
            have_colw = 1;
        } else if (is_cell_op(op)) {
            uint32_t extra = 0;
            if (sh->carry_cellpos == CELLPOS_UNSET) sh->carry_cellpos = sh->carry_len;
            rc = read_cell(sh, op, b, ln, b + ln, len - (off + WK1_REC_HDR + ln), &extra);
            off += extra;
        } else if (op == WK1_OP_STRING) {
            return I123_E_FORMAT;   /* a STRING not right after its FORMULA */
        } else {
            rc = carry_rec(sh, op, ln, b);
        }
        if (rc != I123_OK) return rc;
        off += WK1_REC_HDR + ln;
    }
    if (sh->carry_cellpos == CELLPOS_UNSET) sh->carry_cellpos = sh->carry_len;
    /* COLW1 is written after WINDOW1; without one it would be lost. */
    if (have_colw && !(seen & owned_bit(WK1_OP_WINDOW1))) return I123_E_FORMAT;
    return I123_OK;
}

int i123_wk1_read(i123_sheet_t *sh, const uint8_t *buf, uint32_t len)
{
    int rc;
    uint32_t err_off;
    if (sh == (i123_sheet_t *)0 || buf == (const uint8_t *)0) return I123_E_ARG;
    i123_sheet_clear(sh);
    rc = read_body(sh, buf, len);
    if (rc != I123_OK) {
        err_off = sh->err_off;
        i123_sheet_clear(sh);        /* never half-loaded (Rule 2) */
        sh->err_off = err_off;
    }
    return rc;
}

/* ---- writer ---------------------------------------------------------------- */
typedef struct {
    uint8_t *out;
    uint32_t cap, pos;
    int      err;
} wr_t;

static void put8(wr_t *w, uint8_t v)
{
    if (w->pos >= w->cap) { w->err = 1; return; }
    w->out[w->pos++] = v;
}

static void put16(wr_t *w, uint16_t v)
{
    put8(w, (uint8_t)(v & 0xFFu));
    put8(w, (uint8_t)(v >> 8));
}

static void put_hdr(wr_t *w, uint16_t op, uint32_t len)
{
    put16(w, op);
    put16(w, (uint16_t)len);
}

static void put_bytes(wr_t *w, const uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) put8(w, p[i]);
}

static void put_owned(wr_t *w, const i123_sheet_t *sh, uint16_t op)
{
    uint8_t one = 0;
    switch (op) {
    case WK1_OP_CALCMODE:    one = sh->calcmode;    break;
    case WK1_OP_CALCORDER:   one = sh->calcorder;   break;
    case WK1_OP_SPLIT:       one = sh->split;       break;
    case WK1_OP_SYNC:        one = sh->sync;        break;
    case WK1_OP_PROTEC:      one = sh->protec;      break;
    case WK1_OP_LABELFMT:    one = sh->labelfmt;    break;
    case WK1_OP_CALCCOUNT:   one = sh->calccount;   break;
    case WK1_OP_UNFORMATTED: one = sh->unformatted; break;
    case WK1_OP_RANGE:
        /* An empty fresh sheet (never minted) writes (0,0)-(0,0)
         * [oracle-resolves]. */
        put_hdr(w, op, WK1_LEN_RANGE);
        for (int i = 0; i < 4; i++)
            put16(w, (uint16_t)(sh->range_set ? sh->range[i] : 0));
        return;
    case WK1_OP_HIDVEC1: {
        uint32_t n = 0;
        for (int c = 0; c < 256; c++) if (sh->hid_set[c]) n++;
        put_hdr(w, op, n * WK1_HIDVEC_TRIPLE);
        for (int c = 0; c < 256; c++) {
            if (!sh->hid_set[c]) continue;
            put16(w, (uint16_t)c);
            put16(w, sh->hid_first[c]);
            put16(w, sh->hid_last[c]);
        }
        return;
    }
    case WK1_OP_WINDOW1: {
        /* ncols is DERIVED (spec Sec 3: 44/44): the whole columns from the
         * left column that fit the 72-character worksheet area. */
        uint32_t nc = i123_cols_fitting(sh, rd16(sh->win1 + WK1_W1_LEFT),
                                        I123_SCREEN_COLS_CHARS);
        put_hdr(w, op, WK1_LEN_WINDOW1);
        for (uint32_t i = 0; i < WK1_LEN_WINDOW1; i++) {
            if (i == WK1_W1_NCOLS)          put8(w, (uint8_t)(nc & 0xFFu));
            else if (i == WK1_W1_NCOLS + 1) put8(w, (uint8_t)(nc >> 8));
            else                            put8(w, sh->win1[i]);
        }
        for (int c = 0; c < 256; c++) {
            if (sh->colw[c] == 0u) continue;
            put_hdr(w, WK1_OP_COLW1, WK1_LEN_COLW1);
            put16(w, (uint16_t)c);
            put8(w, sh->colw[c]);
        }
        return;
    }
    default:
        w->err = 1;   /* a placeholder for an op the model does not own */
        return;
    }
    put_hdr(w, op, WK1_LEN_ONE);
    put8(w, one);
}

static void put_cells(wr_t *w, const i123_sheet_t *sh)
{
    for (uint32_t i = 0; i < sh->ncells; i++) {
        const i123_cell_t *c = &sh->cells[i];
        const uint8_t *blob = sh->pool + c->off;
        switch (c->kind) {
        case I123_K_BLANK:
            put_hdr(w, WK1_OP_BLANK, WK1_LEN_BLANK);
            break;
        case I123_K_INTEGER:
            put_hdr(w, WK1_OP_INTEGER, WK1_LEN_INTEGER);
            break;
        case I123_K_NUMBER:
            put_hdr(w, WK1_OP_NUMBER, WK1_LEN_NUMBER);
            break;
        case I123_K_LABEL:
#ifdef I123_MUT_LABEL_OPCODE
            /* MUTANT (Rule 6; test-i123-wk1-roundtrip-mutant): the LABEL
             * record id written as STRING's -- every rewritten golden differs
             * at its first label and the independent reader sees STRING
             * records where the sheet holds labels. NEVER in a real build. */
            put_hdr(w, WK1_OP_STRING, WK1_CELL_PREFIX + (uint32_t)c->len + 1u);
#else
            put_hdr(w, WK1_OP_LABEL, WK1_CELL_PREFIX + (uint32_t)c->len + 1u);
#endif
            break;
        case I123_K_FORMULA:
            put_hdr(w, WK1_OP_FORMULA, WK1_FORMULA_FIXED + (uint32_t)c->len);
            break;
        default:
            w->err = 1;
            return;
        }
        put8(w, c->fmt);
        put16(w, c->col);
        put16(w, c->row);
        switch (c->kind) {
        case I123_K_INTEGER: put8(w, c->v[0]); put8(w, c->v[1]); break;
        case I123_K_NUMBER:  put_bytes(w, c->v, 8u); break;
        case I123_K_LABEL:   put_bytes(w, blob, c->len); put8(w, 0u); break;
        case I123_K_FORMULA:
            put_bytes(w, c->v, 8u);
            put16(w, c->len);
            put_bytes(w, blob, c->len);
            if (c->has_str) {
                put_hdr(w, WK1_OP_STRING, WK1_CELL_PREFIX + (uint32_t)c->slen + 1u);
                put8(w, c->str_fmt);
                put16(w, c->col);
                put16(w, c->row);
                put_bytes(w, blob + c->len, c->slen);
                put8(w, 0u);
            }
            break;
        default: break;
        }
    }
}

int i123_wk1_write(const i123_sheet_t *sh, uint8_t *out, uint32_t cap,
                   uint32_t *out_len)
{
    wr_t w;
    uint32_t p = 0;
    int cells_done = 0;

    if (sh == (const i123_sheet_t *)0 || out == (uint8_t *)0 || out_len == (uint32_t *)0)
        return I123_E_ARG;
    w.out = out; w.cap = cap; w.pos = 0; w.err = 0;

    put_hdr(&w, WK1_OP_BOF, WK1_LEN_BOF);
    put16(&w, sh->bof_version);
    while (p < sh->carry_len) {
        uint16_t op, ln;
        if (!cells_done && p == sh->carry_cellpos) { put_cells(&w, sh); cells_done = 1; }
        op = rd16(sh->carry + p);
        ln = rd16(sh->carry + p + 2);
        if (ln == CARRY_PLACEHOLDER) {
            put_owned(&w, sh, op);
            p += WK1_REC_HDR;
        } else {
            put_hdr(&w, op, ln);
            put_bytes(&w, sh->carry + p + WK1_REC_HDR, ln);
            p += WK1_REC_HDR + ln;
        }
    }
    if (!cells_done) put_cells(&w, sh);
    put_hdr(&w, WK1_OP_EOF, 0u);
    if (w.err) return (w.pos >= w.cap) ? I123_E_SPACE : I123_E_FORMAT;
    *out_len = w.pos;
    return I123_OK;
}
