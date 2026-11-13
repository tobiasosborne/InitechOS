/* Read-only code-aware Search. Ref: WP 5.1 Reference printed pp. 571-573:
 * case rule, literal code tokens and direction. Unit boundaries / attribute
 * bytes: MINT06/MINT08, spec/iword/wp5_format.h. Never edits document bytes.
 * This does NOT implement Replace: no replace/undelete byte mint exists. */
#include "iword.h"

static int same_unit(const iw_doc_t *d, uint32_t p, const iw_doc_t *q, uint32_t t)
{
    uint32_t dn = iw_unit_next(d, p) - p, qn = iw_unit_next(q, t) - t;
    int a = iw_doc_byte(d, p), b = iw_doc_byte(q, t);
    if (dn != qn) return 0;
#ifndef IWORD_MUT_SEARCH_CASE
    /* A lower-case query accepts either case; uppercase is literal. */
    if (qn == 1 && b >= 'a' && b <= 'z' && a >= 'A' && a <= 'Z') a += 'a' - 'A';
#endif
#ifdef IWORD_MUT_SEARCH_CODE
    /* Mutant: mistake attribute-off for attribute-on. */
    if (b == 0xC3 && a == 0xC4) return iw_doc_byte(d, p+1) == iw_doc_byte(q, t+1);
#endif
    if (a != b) return 0;
    /* Tab / indent carry their position in the document; the Search token
     * names the feature, not its screen coordinate (Reference p. 572). */
    if (a == 0xC2 || (a == 0xC1 && iw_doc_byte(q, t + 1) == 2))
        return a == 0xC2 || iw_doc_byte(d, p + 1) == 2;
    for (uint32_t i = 1; i < qn; i++)
        if (iw_doc_byte(d, p + i) != iw_doc_byte(q, t + i)) return 0;
    return 1;
}

static int match(const iw_doc_t *d, uint32_t p, const iw_doc_t *q, uint32_t *end)
{
    uint32_t t = 0;
    while (t < q->len) {
        if (p == d->len || !same_unit(d, p, q, t)) return 0;
        p = iw_unit_next(d, p); t = iw_unit_next(q, t);
    }
    *end = p; return 1;
}

int iw_search(const iw_doc_t *d, const iw_doc_t *q, uint32_t from, int back,
              uint32_t *start, uint32_t *end)
{
    uint32_t p, e;
#ifdef IWORD_MUT_SEARCH_WRITE
    if (d->len) (void)iw_doc_set((iw_doc_t *)d, 0, '!');
#endif
    if (!q->len || !iw_is_boundary(d, from)) return IW_E_RANGE;
#ifdef IWORD_MUT_SEARCH_BACK
    back = 0;
#endif
    p = from;
    if (back) {
        while (p) {
            p = iw_unit_prev(d, p);
            if (match(d, p, q, &e) && e <= from) { *start = p; *end = e; return IW_OK; }
        }
    } else {
        while (p < d->len) {
            if (match(d, p, q, &e)) { *start = p; *end = e; return IW_OK; }
            p = iw_unit_next(d, p);
        }
    }
    return IW_E_RANGE;
}
