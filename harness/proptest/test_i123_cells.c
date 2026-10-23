/* Initech 123 cell-store capacity oracle, bead initech-3s12 (I123 B02; audit
 * docs/audits/2026-10-23-i123/REPORT.md L010 + L032), over the real face_a.c
 * + worksheet core and the recording Toolbox mock of test_i123_quit.c.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2 -- never from sheet.c):
 *   - L010 (the audit's RED steps, evidence/capacity-serial.log, CAPERA.WK1):
 *     a full 2048-cell sheet, /Range Erase A1, then an entry in A2049. The
 *     store holds 2048 cells (face_a.c SHEET_CELLS, the accepted limit);
 *     ../lotus123-decomp/specs/file-formats/wk1-format.md Sec 4.1: content-
 *     free cells with the DEFAULT format are not records, BLANK records are
 *     formatted-but-empty cells; Lotus Reference p. 10-3 (PDF 221): Erase
 *     removes the data and keeps the format and protection. So an ordinary
 *     (default 0xFF: protected, global format) cell erased is ABSENT and its
 *     slot is free; a Fixed-2 cell or an unprotected cell erased stays a
 *     BLANK carrying exactly that byte.
 *   - L032 (evidence/POOL135.WK1, pool-serial.log): 135 labels of 240
 *     characters (241 bytes each with the prefix) near the 32768-byte pool;
 *     replacing one with another 240 (or fewer) characters changes nothing
 *     about the final live size, so it must succeed. A replacement whose
 *     FINAL live size overflows the pool must fail with the old bytes
 *     intact. The pool size is read from the sheet as the capacity INPUT
 *     (face_a.c SHEET_MEM); every expected byte is the test's own.
 * Mutants (Rule 6; -D on the core build, test-i123-cells-mutant):
 *   I123_MUT_DEFAULT_BLANK        default format stored as a BLANK   -> C1
 *   I123_MUT_ALLOC_BEFORE_RECLAIM new blob allocated before the old
 *                                 one is released                     -> C5
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main

static void f5(const char *addr) { ev(keyDown, 0x3F << 8); tks(addr); tk('\r'); }

/* A 240-character label text of letter `ch` for row r (row-unique head). */
static void label_text(char *s, unsigned n, char ch, unsigned r)
{
    for (unsigned i = 0; i < n; i++) s[i] = ch;
    if (n >= 4) { s[0] = (char)('0' + r / 100 % 10); s[1] = (char)('0' + r / 10 % 10); s[2] = (char)('0' + r % 10); }
    s[n] = 0;
}
/* Does cell (0, r) hold exactly the left label `t`? */
static int holds(unsigned r, const char *t)
{
    const i123_cell_t *c = i123_find(&g_sheet, 0, (uint16_t)r);
    size_t n = strlen(t);
    return c && c->kind == I123_K_LABEL && c->len == n + 1 &&
           g_sheet.pool[c->off] == '\'' && !memcmp(g_sheet.pool + c->off + 1, t, n);
}

/* C1/C2: L010 -- a full store, erase the ordinary A1, enter in A2049. */
static void t_capacity(void)
{
    for (unsigned r = 0; r < 2048; r++)
        if (i123_set_number(&g_sheet, 0, (uint16_t)r, (double)(r + 1)) != I123_OK) break;
    CHECK(g_sheet.ncells == 2048 && g_sheet.cap == 2048, "R0", "setup: A1..A2048 fill the 2048-cell store");
    CHECK(i123_find(&g_sheet, 0, 0)->fmt == 0xFF, "R0", "setup: A1 carries the default byte 0xFF");
    tks("/rea1\r");
    CHECK(g_mode == M_READY && !i123_find(&g_sheet, 0, 0) && g_sheet.ncells == 2047, "C1",
          "erasing the ordinary A1 leaves it absent (no default BLANK) and frees its slot");
    f5("a2049");
    tks("9\r");
    {
        const i123_cell_t *c = i123_find(&g_sheet, 0, 2048);
        CHECK(g_mode == M_READY && c && c->kind == I123_K_INTEGER && c->v[0] == 9, "C2",
              "after the erase an entry in A2049 is stored (READY, INTEGER 9)");
    }
    CHECK(g_sheet.ncells == 2048 && i123_find(&g_sheet, 0, 2047) != 0, "C2", "A2..A2049 held, 2048 cells");
}

/* C3/C4: formats and protection that differ from absence stay BLANKs. */
static void t_kept(void)
{
    tks("5"); right(); tks("6"); right(); tks("7\r");
    tks("/rff2\ra1\r");          /* A1 Fixed 2, protected: 0x82          */
    tks("/rub1\r");              /* B1 unprotected default: 0x7F         */
    tks("/rea1..c1\r");
    const i123_cell_t *a = i123_find(&g_sheet, 0, 0), *b = i123_find(&g_sheet, 1, 0);
    CHECK(a && a->kind == I123_K_BLANK && a->fmt == 0x82, "C3", "an erased Fixed-2 cell stays a BLANK 0x82");
    CHECK(b && b->kind == I123_K_BLANK && b->fmt == 0x7F, "C3", "an erased unprotected cell stays a BLANK 0x7F");
    CHECK(!i123_find(&g_sheet, 2, 0) && g_sheet.ncells == 2, "C4", "the erased ordinary C1 is absent; two BLANKs remain");
    tks("/rfrd1\r");             /* Format Reset on the empty D1: nothing to store */
    CHECK(!i123_find(&g_sheet, 3, 0), "C4", "a default format on an empty cell stores no BLANK");
}

/* C5..C8: L032 -- the string pool near its limit. */
static void t_pool(void)
{
    char t[260], y[260];
    uint32_t live = 0;
    for (unsigned r = 0; r < 135; r++) {
        label_text(t, 240, 'x', r);
        if (i123_set_label(&g_sheet, 0, (uint16_t)r, '\'', t, 240) != I123_OK) break;
        live += 241;
    }
    CHECK(g_sheet.ncells == 135 && live == 32535u, "R0", "setup: POOL135 -- 135 labels, 32535 bytes with prefixes");
    CHECK(g_sheet.pool_cap >= live && g_sheet.pool_cap - live < 241u, "R0",
          "setup: fewer than 241 bytes free, so a second copy of a label cannot fit");
    memset(y, 'y', 240); y[240] = 0;
    tks(y); tk('\r');
    CHECK(g_mode == M_READY && holds(0, y), "C5", "a same-length 240-character replacement of A1 is stored");
    label_text(t, 240, 'x', 1);
    CHECK(holds(1, t) && (label_text(t, 240, 'x', 134), holds(134, t)), "C5", "A2 and A135 keep their bytes");
    ev(keyDown, 0x50 << 8);      /* Down: A2 */
    tks("y\r");
    CHECK(g_mode == M_READY && holds(1, "y"), "C6", "a shorter replacement of A2 is stored");
}

/* C7/C8: a longer replacement that fits only once the old blob is gone, and
 * one whose final size cannot fit: refused, old bytes intact. */
static void t_grow(void)
{
    char t[260], z[260];
    uint32_t live = 0, fill;
    label_text(t, 100, 'a', 0);
    i123_set_label(&g_sheet, 0, 0, '\'', t, 100); live += 101;
    for (unsigned r = 1; r < 135; r++) {
        label_text(t, 240, 'x', r);
        i123_set_label(&g_sheet, 0, (uint16_t)r, '\'', t, 240); live += 241;
    }
    /* A136 takes all but 200 free bytes: A1 (101 -> 241, +140) fits the final
     * size, but not beside its old blob. */
    fill = g_sheet.pool_cap - live - 200u - 1u;
    CHECK(fill <= 240u, "R0", "setup: the A136 filler is a legal label");
    label_text(t, fill, 'f', 135);
    CHECK(i123_set_label(&g_sheet, 0, 135, '\'', t, fill) == I123_OK && g_sheet.ncells == 136, "R0", "setup: A136 filler stored");
    memset(z, 'z', 240); z[240] = 0;
    tks(z); tk('\r');
    CHECK(g_mode == M_READY && holds(0, z), "C7", "A1 grows 100 -> 240 characters within the final live size");
    label_text(t, fill, 'f', 135);
    CHECK(holds(135, t), "C7", "A136 keeps its bytes across the compaction");
    /* 60 bytes free now: A136 (fill) -> 240 characters needs 240 - fill > 60. */
    f5("a136");
    tks(z); tk('\r');
    CHECK(g_mode == M_EDIT, "C8", "a replacement whose final size overflows the pool is refused (EDIT)");
    CHECK(holds(135, t) && holds(0, z), "C8", "the refused A136 keeps its old bytes; A1 is intact");
    tk(3);
    CHECK(g_mode == M_READY && holds(135, t), "C8", "Ctrl-Break leaves the old A136");
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    m_dir = argv[1];
    g_ctr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (g_ctr == MAP_FAILED) return 2;
    launch("CAPACITY", t_capacity);
    launch("KEPT", t_kept);
    launch("POOL", t_pool);
    launch("GROW", t_grow);
    printf("i123-cells: %d checks, %d failures\n", g_ctr[0], g_ctr[1]);
    return g_ctr[1] ? 1 : 0;
}
