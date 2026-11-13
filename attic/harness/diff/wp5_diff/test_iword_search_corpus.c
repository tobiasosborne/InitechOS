/* Independent read-only search oracle over REAL WP.EXE documents extracted
 * by both MILTON and mtools from identical FAT images (search.sh).
 * Ref: WP 5.1 Reference pp. 571-573; MINT01..32 and mint-results-001..004.
 * No expected match positions come from artifact unit/search helpers. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "iword.h"
static uint8_t file[65536], saved[65536], add[65536], after[65536];
static iw_piece_t pc[8192], qp;
static uint32_t offsets[65536];
static uint32_t rd16(const uint8_t *s) { return s[0] | (uint32_t)s[1] << 8; }
static uint32_t rd32(const uint8_t *s) { return rd16(s) | rd16(s+2) << 16; }
static unsigned length(const uint8_t *b, unsigned n, unsigned p)
{
    static const unsigned fixed[16] = {4,9,11,3,3,0,6};
    unsigned c = b[p], k;
    if (c < 0xC0) return 1;
    if (c < 0xD0) k = fixed[c - 0xC0];
    else { if (p+4 > n) return 0; k = 4 + rd16(b+p+2); }
    return k && p+k <= n && b[p+k-1] == c ? k : 0;
}
static int equal(const uint8_t *b, unsigned p, const uint8_t *q, unsigned n)
{
    for (unsigned i = 0; i < n; i++) {
        int a = b[p+i], c = q[i];
        if (n == 1 && c >= 'a' && c <= 'z' && a >= 'A' && a <= 'Z') a += 32;
        if (a != c) return 0;
    }
    return 1;
}
int main(int argc, char **argv)
{
    iw_doc_t d, q;
    uint32_t size, off, n, nu = 0, checks = 0;
    FILE *f;
    if (argc != 2 || !(f = fopen(argv[1], "rb"))) return 2;
    size = (uint32_t)fread(file, 1, sizeof file, f);
    if (ferror(f) || !feof(f) || fclose(f) || size < 327) return 2;
    off = rd32(file+4); if (off > size) return 2;
    n = size-off; memcpy(saved, file, size);
    for (unsigned p = 0; p < n; ) {
        unsigned k = length(file+off, n, p);
        if (!k) return 2;
        offsets[nu++] = p; p += k;
    }
    iw_doc_init(&d, add, sizeof add, pc, 8192, sizeof add);
    iw_doc_load(&d, file+off, n);
    for (unsigned t = 0; t < nu; t++) {
        unsigned p = offsets[t], c = file[off+p], k = length(file+off, n, p);
        uint8_t query[3];
        if (!((c >= 32 && c < 127) || c == 0xC3 || c == 0xC4)) continue;
        memcpy(query, file+off+p, k);
        if (c >= 'A' && c <= 'Z') query[0] += 32;
        iw_doc_init(&q, 0, 0, &qp, 1, sizeof query); iw_doc_load(&q, query, k);
        for (int back = 0; back <= 1; back++) for (unsigned way = 0; way < 3; way++) {
            unsigned from = way == 0 ? 0 : way == 1 ? n : p;
            unsigned rs = 0, re = 0, s = 0, e = 0;
            int found = 0;
            for (unsigned i = 0; i < nu; i++) {
                unsigned pos = offsets[i];
                if ((!back && pos < from) || (back && (pos >= from || pos+k > from))) continue;
                if (length(file+off, n, pos) != k || !equal(file+off, pos, query, k)) continue;
                rs = pos; re = pos+k; found = 1;
                if (!back) break;
            }
            int rc = iw_search(&d, &q, from, back, &s, &e);
            checks++;
            if ((rc == IW_OK) != found || (found && (s != rs || e != re))) {
                fprintf(stderr, "RED-REASON search-corpus: token %u direction %d from %u got %d %u..%u expected %d %u..%u\n", t, back, from, rc, s, e, found, rs, re); return 1;
            }
        }
    }
    if (memcmp(saved, file, size) || iw_doc_read(&d, 0, after, n) != n || memcmp(after, saved+off, n)) {
        fputs("RED-REASON search-corpus: document bytes changed\n", stderr); return 1;
    }
    printf("search-corpus: %u checks, bytes unchanged\n", checks);
    return 0;
}
