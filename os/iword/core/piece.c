/*
 * os/iword/core/piece.c -- InitechWord's piece table and the WordPerfect 5.1
 * unit walk (THE ARTIFACT; bead initech-l4tj, IWORD P1).
 *
 * THE PIECE TABLE. The document is the concatenation of pieces, each a span
 * of the ORIGINAL buffer (the body as read from disk, never written) or of
 * the ADD buffer (append-only: every byte ever typed). An insert appends its
 * bytes to the add buffer and splits at most one piece; a delete trims or
 * drops pieces; nothing is ever moved in either buffer. Adjacent pieces that
 * are contiguous in the same buffer are merged (typing a word grows ONE
 * piece). Ref: plan docs/plans/INITECH-WORD-plan.md Sec 4 ("a piece table
 * of paragraphs/runs/codes"; FlairTE's flat 4 KB buffer is too small).
 *
 * THE UNIT WALK. spec/iword/wp5_format.h (cited to ../wordperfect51-decomp
 * specs/file-formats/wp5-body-codes.md Sec 1-4). Only verified units are
 * well-formed; iw_unit_len is the codec's validator. On a loaded document
 * (always well-formed) the walk reads byte classes without re-validating.
 *
 * Freestanding (no libc). Deterministic. ASCII-clean.
 */
#include "iword.h"
#include "iword/wp5_format.h"

static const uint8_t FIXED_LEN[16] = IW_WP5_FIXED_LEN_INIT;

static void mcopy(uint8_t *d, const uint8_t *s, uint32_t n)
{
    while (n--) *d++ = *s++;
}

/* Move pieces [from, npc) to start at `to` (either direction). */
static void pmove(iw_doc_t *d, uint32_t to, uint32_t from)
{
    uint32_t n = d->npc - from, i;
    if (to < from) for (i = 0; i < n; i++) d->pc[to + i] = d->pc[from + i];
    else if (to > from) for (i = n; i > 0; i--) d->pc[to + i - 1] = d->pc[from + i - 1];
}

static const uint8_t *pbytes(const iw_doc_t *d, const iw_piece_t *p)
{
    return (p->off & IW_PIECE_ADD) ? d->add + (p->off & ~IW_PIECE_ADD) : d->orig + p->off;
}

int iw_doc_init(iw_doc_t *d, uint8_t *add, uint32_t add_cap,
                iw_piece_t *pc, uint32_t pc_cap, uint32_t max_len)
{
    d->orig = add;           /* an empty original: never read */
    d->orig_len = 0;
    d->add = add;
    d->add_len = 0;
    d->add_cap = add_cap;
    d->pc = pc;
    d->npc = 0;
    d->pc_cap = pc_cap;
    d->len = 0;
    d->max_len = max_len;
    return IW_OK;
}

void iw_doc_load(iw_doc_t *d, const uint8_t *orig, uint32_t n)
{
    d->orig = orig;
    d->orig_len = n;
    d->add_len = 0;
    d->npc = 0;
    d->len = n;
    if (n && d->pc_cap) {
        d->pc[0].off = 0;
        d->pc[0].len = n;
        d->npc = 1;
    }
}

/* Locate `pos`: the piece index holding it and the offset inside; pos ==
 * len yields (npc, 0). */
static void locate(const iw_doc_t *d, uint32_t pos, uint32_t *pi, uint32_t *po)
{
    uint32_t i = 0;
    while (i < d->npc && pos >= d->pc[i].len) { pos -= d->pc[i].len; i++; }
    *pi = i;
    *po = pos;
}

int iw_doc_insert(iw_doc_t *d, uint32_t pos, const uint8_t *s, uint32_t n)
{
    uint32_t pi, po, a;
    if (pos > d->len) return IW_E_RANGE;
    if (n == 0u) return IW_OK;
    if (d->len + n > d->max_len || d->add_len + n > d->add_cap) return IW_E_FULL;
    a = d->add_len;
    locate(d, pos, &pi, &po);
    /* Typing at the end of the piece that ends exactly at the add buffer's
     * tail grows that piece (the common case: one piece per typed run). */
    if (po == 0u && pi > 0u) {
        iw_piece_t *p = &d->pc[pi - 1u];
        if ((p->off & IW_PIECE_ADD) && (p->off & ~IW_PIECE_ADD) + p->len == a) {
            mcopy(d->add + a, s, n);
            d->add_len += n;
            p->len += n;
            d->len += n;
            return IW_OK;
        }
    }
    if (po == 0u) {
        if (d->npc + 1u > d->pc_cap) return IW_E_FULL;
        mcopy(d->add + a, s, n);
        pmove(d, pi + 1u, pi);
        d->npc++;
        d->pc[pi].off = IW_PIECE_ADD | a;
        d->pc[pi].len = n;
    } else {
        iw_piece_t left = d->pc[pi];
        if (d->npc + 2u > d->pc_cap) return IW_E_FULL;
        mcopy(d->add + a, s, n);
        pmove(d, pi + 3u, pi + 1u);
        d->npc += 2u;
        d->pc[pi].len = po;
        d->pc[pi + 1u].off = IW_PIECE_ADD | a;
        d->pc[pi + 1u].len = n;
#ifdef IWORD_MUT_SPLIT_OFF
        /* MUTANT (Rule 6; test-iword-core-mutant): the right half starts one
         * byte late -- a byte is lost and the tail is shifted. NEVER real. */
        d->pc[pi + 2u].off = left.off + po + 1u;
        d->pc[pi + 2u].len = left.len - po;
#else
        d->pc[pi + 2u].off = left.off + po;
        d->pc[pi + 2u].len = left.len - po;
#endif
    }
    d->add_len += n;
    d->len += n;
    return IW_OK;
}

int iw_doc_delete(iw_doc_t *d, uint32_t pos, uint32_t n)
{
    uint32_t pi, po;
    if (pos > d->len || n > d->len - pos) return IW_E_RANGE;
    if (n == 0u) return IW_OK;
    locate(d, pos, &pi, &po);
    if (po > 0u) {
        iw_piece_t *p = &d->pc[pi];
        if (po + n < p->len) {               /* strictly inside one piece: split */
            iw_piece_t right;
            if (d->npc + 1u > d->pc_cap) return IW_E_FULL;
            right.off = p->off + po + n;
            right.len = p->len - po - n;
            p->len = po;
            pmove(d, pi + 2u, pi + 1u);
            d->npc++;
            d->pc[pi + 1u] = right;
            d->len -= n;
            return IW_OK;
        }
        n -= p->len - po;                    /* trim this piece's tail */
        d->len -= p->len - po;
        p->len = po;
        pi++;
    }
    {
        uint32_t first = pi;
        while (n > 0u && pi < d->npc && d->pc[pi].len <= n) {   /* whole pieces */
            n -= d->pc[pi].len;
            d->len -= d->pc[pi].len;
            pi++;
        }
        if (n > 0u) {                         /* trim the next piece's head */
            d->pc[pi].off += n;
            d->pc[pi].len -= n;
            d->len -= n;
        }
        if (pi > first) {
            pmove(d, first, pi);
            d->npc -= pi - first;
        }
    }
    return IW_OK;
}

int iw_doc_set(iw_doc_t *d, uint32_t pos, uint8_t b)
{
    uint32_t pi, po;
    int rc;
    if (pos >= d->len) return IW_E_RANGE;
    locate(d, pos, &pi, &po);
    if (pbytes(d, &d->pc[pi])[po] == b) return IW_OK;
    /* A byte the add buffer holds is overwritten in place: the add buffer is
     * append-only for SHARING, and only one piece ever points at a byte. */
    if (d->pc[pi].off & IW_PIECE_ADD) {
        d->add[(d->pc[pi].off & ~IW_PIECE_ADD) + po] = b;
        return IW_OK;
    }
    if (d->pc_cap < d->npc + 3u || d->add_len + 1u > d->add_cap) return IW_E_FULL;
    rc = iw_doc_delete(d, pos, 1);
    if (rc == IW_OK) rc = iw_doc_insert(d, pos, &b, 1);
    return rc;
}

int iw_doc_byte(const iw_doc_t *d, uint32_t pos)
{
    uint32_t pi, po;
    if (pos >= d->len) return -1;
    locate(d, pos, &pi, &po);
    return pbytes(d, &d->pc[pi])[po];
}

uint32_t iw_doc_read(const iw_doc_t *d, uint32_t pos, uint8_t *out, uint32_t n)
{
    uint32_t pi, po, got = 0;
    if (pos >= d->len) return 0;
    if (n > d->len - pos) n = d->len - pos;
    locate(d, pos, &pi, &po);
    while (got < n && pi < d->npc) {
        uint32_t k = d->pc[pi].len - po;
        if (k > n - got) k = n - got;
        mcopy(out + got, pbytes(d, &d->pc[pi]) + po, k);
        got += k;
        pi++;
        po = 0;
    }
    return got;
}

void iw_iter_at(iw_iter_t *it, const iw_doc_t *d, uint32_t pos)
{
    it->d = d;
    if (pos > d->len) pos = d->len;
    it->pos = pos;
    locate(d, pos, &it->pi, &it->po);
}

int iw_iter_next(iw_iter_t *it)
{
    const iw_doc_t *d = it->d;
    int b;
    while (it->pi < d->npc && it->po >= d->pc[it->pi].len) { it->pi++; it->po = 0; }
    if (it->pi >= d->npc) return -1;
    b = pbytes(d, &d->pc[it->pi])[it->po];
    it->po++;
    it->pos++;
    return b;
}

/* ---- units ---------------------------------------------------------------- */
static int is_single(uint8_t b)
{
    return b == IW_WP5_HRT || b == IW_WP5_SRT || b == IW_WP5_HPG ||
           b == IW_WP5_HSPACE || b == IW_WP5_END_CENTER;
}

uint32_t iw_unit_len(const uint8_t *b, uint32_t n, uint32_t pos)
{
    uint8_t c;
    if (pos >= n) return 0;
    c = b[pos];
    if (c >= IW_WP5_TEXT_LO && c <= IW_WP5_TEXT_HI) return 1;
    if (c < IW_WP5_FIXED_LO) return is_single(c) ? 1u : 0u;
    if (c <= IW_WP5_FIXED_HI) {
        uint32_t L = FIXED_LEN[c - IW_WP5_FIXED_LO];
        if (L == 0u || n - pos < L || b[pos + L - 1u] != c) return 0;
        return L;
    }
    {
        uint32_t size, total;
        if (n - pos < IW_WP5_VAR_HDR + IW_WP5_VAR_TRAILER) return 0;
        size = (uint32_t)b[pos + 2] | ((uint32_t)b[pos + 3] << 8);
        if (size < IW_WP5_VAR_TRAILER) return 0;
        total = IW_WP5_VAR_HDR + size;
        if (n - pos < total) return 0;
        if (b[pos + total - 1u] != c || b[pos + total - 2u] != b[pos + 1] ||
            b[pos + total - 4u] != b[pos + 2] || b[pos + total - 3u] != b[pos + 3]) return 0;
        return total;
    }
}

static uint32_t unit_len_doc(const iw_doc_t *d, uint32_t pos)
{
    int c = iw_doc_byte(d, pos);
    if (c < 0) return 0;
    if (c >= (int)IW_WP5_FIXED_LO && c <= (int)IW_WP5_FIXED_HI) return FIXED_LEN[c - IW_WP5_FIXED_LO];
    if (c >= (int)IW_WP5_VAR_LO) {
        int lo = iw_doc_byte(d, pos + 2u), hi = iw_doc_byte(d, pos + 3u);
        return IW_WP5_VAR_HDR + ((uint32_t)lo | ((uint32_t)hi << 8));
    }
    return 1;
}

int iw_unit_kind(const iw_doc_t *d, uint32_t pos)
{
    int c = iw_doc_byte(d, pos);
    if (c >= (int)IW_WP5_TEXT_LO && c <= (int)IW_WP5_TEXT_HI) return IW_U_TEXT;
    if (c == IW_WP5_HRT) return IW_U_HRT;
    if (c == IW_WP5_SRT) return IW_U_SRT;
    if (c == IW_WP5_HPG) return IW_U_HPG;
    if (c == IW_WP5_HSPACE) return IW_U_HSPACE;
    /* slice 2 (bead initech-jojk; spec/iword/wp5_format.h "tabs, indents and
     * hard pages"): the codes that take columns or that WP keeps beside one */
    if (c == IW_WP5_C1 && iw_doc_byte(d, pos + 1u) == (int)IW_WP5_C1_TAB) return IW_U_TAB;
    if (c == IW_WP5_INDENT) return IW_U_INDENT;
    if (c == IW_WP5_END_INDENT) return IW_U_HIDDEN;
    if (c == IW_WP5_PAGE_GRP && iw_doc_byte(d, pos + 1u) == (int)IW_WP5_PAGE_SUB) return IW_U_HIDDEN;
    return IW_U_CODE;
}

uint32_t iw_unit_next(const iw_doc_t *d, uint32_t pos)
{
    uint32_t n;
    if (pos >= d->len) return d->len;
    n = unit_len_doc(d, pos);
    if (n == 0u || n > d->len - pos) return d->len;
    return pos + n;
}

uint32_t iw_unit_prev(const iw_doc_t *d, uint32_t pos)
{
    int c;
    if (pos == 0u) return 0;
    if (pos > d->len) pos = d->len;
    c = iw_doc_byte(d, pos - 1u);
    if (c >= (int)IW_WP5_FIXED_LO && c <= (int)IW_WP5_FIXED_HI) {
        uint32_t L = FIXED_LEN[c - IW_WP5_FIXED_LO];
        return L && L <= pos ? pos - L : 0u;
    }
    if (c >= (int)IW_WP5_VAR_LO) {
#ifdef IWORD_MUT_PREV_VAR
        /* MUTANT (Rule 6; test-iword-core-mutant): the backward walk treats
         * a variable group's last byte as a one-byte unit. NEVER real. */
        return pos - 1u;
#else
        /* The mirrored trailer: [size lo][size hi][sub][code] at the end. */
        int lo = iw_doc_byte(d, pos - 4u), hi = iw_doc_byte(d, pos - 3u);
        uint32_t total = IW_WP5_VAR_HDR + ((uint32_t)lo | ((uint32_t)hi << 8));
        return total <= pos ? pos - total : 0u;
#endif
    }
    return pos - 1u;
}

int iw_is_boundary(const iw_doc_t *d, uint32_t pos)
{
    uint32_t p = 0;
    if (pos > d->len) return 0;
    while (p < pos) p = iw_unit_next(d, p);
    return p == pos;
}
