/*
 * os/iword/fs/wp5.c -- the WordPerfect 5.1 document codec of InitechWord
 * (THE ARTIFACT; bead initech-l4tj, IWORD P1).
 *
 * Ref: spec/iword/wp5_format.h, which cites ../wordperfect51-decomp
 *      specs/file-formats/wp5-header-and-prefix.md (the 16-byte header, the
 *      0x147-byte prefix of every minted document, Sec 4 "emit the 327-byte
 *      prefix verbatim, then the body") and wp5-body-codes.md (the units).
 *
 * READ validates, it does not convert: the header must be a WP 5.1 document
 * (magic, product 1, file type 0x0A, version 0.1, not encrypted, a docstart
 * inside the file) and the body must be a sequence of VERIFIED units, every
 * group's trailer mirroring its header. Anything else is REFUSED with the
 * offset (Rule 2) -- the corpus' [oracle-resolves] codes are not guessed at.
 * The prefix (header + packets) and the body are returned as spans of the
 * caller's buffer; the app keeps the prefix opaque and writes it back
 * unchanged, so a document WordPerfect wrote comes back byte for byte.
 *
 * WRITE is prefix + the piece table's bytes. A new document gets the
 * default prefix (the bytes every minted document carries).
 *
 * Freestanding (no libc). Deterministic. ASCII-clean.
 */
#include "iword.h"
#include "iword/wp5_format.h"

static const uint8_t DEFAULT_PREFIX[IW_WP5_DEFAULT_DOCSTART] = IW_WP5_DEFAULT_PREFIX_INIT;
_Static_assert(sizeof DEFAULT_PREFIX == 327u, "the minted prefix is 327 bytes (wp5-header-and-prefix.md Sec 2)");

const uint8_t *iw_wp5_default_prefix(uint32_t *len)
{
    *len = (uint32_t)sizeof DEFAULT_PREFIX;
    return DEFAULT_PREFIX;
}

int iw_wp5_read(const uint8_t *f, uint32_t n, iw_wp5_t *out)
{
    uint32_t start, p;
    out->prefix = f;
    out->prefix_len = 0;
    out->body = f;
    out->body_len = 0;
    out->err_off = 0;
    if (n < IW_WP5_HEADER_BYTES || f[0] != IW_WP5_MAGIC0 || f[1] != IW_WP5_MAGIC1 ||
        f[2] != IW_WP5_MAGIC2 || f[3] != IW_WP5_MAGIC3) return IW_E_FORMAT;
    out->err_off = IW_WP5_OFF_PRODUCT;
    if (f[IW_WP5_OFF_PRODUCT] != IW_WP5_PRODUCT || f[IW_WP5_OFF_FILETYPE] != IW_WP5_FILETYPE_DOC ||
        f[IW_WP5_OFF_MAJOR] != IW_WP5_MAJOR || f[IW_WP5_OFF_MINOR] != IW_WP5_MINOR) return IW_E_FORMAT;
    out->err_off = IW_WP5_OFF_ENCRYPT;
    if (f[IW_WP5_OFF_ENCRYPT] != 0u || f[IW_WP5_OFF_ENCRYPT + 1u] != 0u) return IW_E_FORMAT;
    out->err_off = IW_WP5_OFF_DOCSTART;
    start = (uint32_t)f[4] | ((uint32_t)f[5] << 8) | ((uint32_t)f[6] << 16) | ((uint32_t)f[7] << 24);
    if (start < IW_WP5_HEADER_BYTES || start > n) return IW_E_FORMAT;
    for (p = start; p < n; ) {
        uint32_t u = iw_unit_len(f, n, p);
        if (u == 0u) { out->err_off = p; return IW_E_CODE; }
        p += u;
    }
    out->err_off = 0;
    out->prefix_len = start;
    out->body = f + start;
    out->body_len = n - start;
    return IW_OK;
}

int iw_wp5_write(const uint8_t *prefix, uint32_t prefix_len, const iw_doc_t *d,
                 uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t i, n;
    *out_len = 0;
    if (prefix_len > cap || d->len > cap - prefix_len) return IW_E_FULL;
    for (i = 0; i < prefix_len; i++) out[i] = prefix[i];
    n = iw_doc_read(d, 0, out + prefix_len, d->len);
    if (n != d->len) return IW_E_RANGE;
#ifdef IWORD_MUT_DROP_CODE
    /* MUTANT (Rule 6; test-iword-wp5-roundtrip-mutant): the writer drops
     * every soft-return code. NEVER real. */
    {
        uint32_t r = prefix_len, w = prefix_len;
        while (r < prefix_len + n) {
            uint32_t u = iw_unit_len(out, prefix_len + n, r);
            if (u == 0u) u = 1u;
            if (!(u == 1u && out[r] == IW_WP5_SRT)) for (i = 0; i < u; i++) out[w++] = out[r + i];
            r += u;
        }
        n = w - prefix_len;
    }
#endif
    *out_len = prefix_len + n;
    return IW_OK;
}

uint32_t iw_wp5_text(const iw_doc_t *d, char *out, uint32_t cap)
{
    uint32_t p = 0, n = 0;
    while (p < d->len) {
        if (iw_unit_kind(d, p) == IW_U_TEXT) {
            if (n < cap) out[n] = (char)iw_doc_byte(d, p);
            n++;
        }
        p = iw_unit_next(d, p);
    }
    if (cap) out[n < cap ? n : cap - 1u] = 0;
    return n;
}
