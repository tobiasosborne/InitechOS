1: /* Read-only code-aware Search. Ref: WP 5.1 Reference printed pp. 571-573:
2:  * case rule, literal code tokens and direction. Unit boundaries / attribute
3:  * bytes: MINT06/MINT08, spec/iword/wp5_format.h. Never edits document bytes.
4:  * This does NOT implement Replace: no replace/undelete byte mint exists. */
5: #include "iword.h"
6: 
7: static int same_unit(const iw_doc_t *d, uint32_t p, const iw_doc_t *q, uint32_t t)
8: {
9:     uint32_t dn = iw_unit_next(d, p) - p, qn = iw_unit_next(q, t) - t;
10:     int a = iw_doc_byte(d, p), b = iw_doc_byte(q, t);
11:     if (dn != qn) return 0;
12: #ifndef IWORD_MUT_SEARCH_CASE
13:     /* A lower-case query accepts either case; uppercase is literal. */
14:     if (qn == 1 && b >= 'a' && b <= 'z' && a >= 'A' && a <= 'Z') a += 'a' - 'A';
15: #endif
16: #ifdef IWORD_MUT_SEARCH_CODE
17:     /* Mutant: mistake attribute-off for attribute-on. */
18:     if (b == 0xC3 && a == 0xC4) return iw_doc_byte(d, p+1) == iw_doc_byte(q, t+1);
19: #endif
20:     if (a != b) return 0;
21:     /* Tab / indent carry their position in the document; the Search token
22:      * names the feature, not its screen coordinate (Reference p. 572). */
23:     if (a == 0xC2 || (a == 0xC1 && iw_doc_byte(q, t + 1) == 2))
24:         return a == 0xC2 || iw_doc_byte(d, p + 1) == 2;
25:     for (uint32_t i = 1; i < qn; i++)
26:         if (iw_doc_byte(d, p + i) != iw_doc_byte(q, t + i)) return 0;
27:     return 1;
28: }
29: 
30: static int match(const iw_doc_t *d, uint32_t p, const iw_doc_t *q, uint32_t *end)
31: {
32:     uint32_t t = 0;
33:     while (t < q->len) {
34:         if (p == d->len || !same_unit(d, p, q, t)) return 0;
35:         p = iw_unit_next(d, p); t = iw_unit_next(q, t);
36:     }
37:     *end = p; return 1;
38: }
39: 
40: int iw_search(const iw_doc_t *d, const iw_doc_t *q, uint32_t from, int back,
41:               uint32_t *start, uint32_t *end)
42: {
43:     uint32_t p, e;
44: #ifdef IWORD_MUT_SEARCH_WRITE
45:     if (d->len) (void)iw_doc_set((iw_doc_t *)d, 0, '!');
46: #endif
47:     if (!q->len || !iw_is_boundary(d, from)) return IW_E_RANGE;
48: #ifdef IWORD_MUT_SEARCH_BACK
49:     back = 0;
50: #endif
51:     p = from;
52:     if (back) {
53:         while (p) {
54:             p = iw_unit_prev(d, p);
55:             if (match(d, p, q, &e) && e <= from) { *start = p; *end = e; return IW_OK; }
56:         }
57:     } else {
58:         while (p < d->len) {
59:             if (match(d, p, q, &e)) { *start = p; *end = e; return IW_OK; }
60:             p = iw_unit_next(d, p);
61:         }
62:     }
63:     return IW_E_RANGE;
64: }
