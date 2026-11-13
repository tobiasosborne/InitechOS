/* initech-qvgd, WL-0146 Part 2. Independent UI expectations from WP 5.1
 * Reference pp. 551-553, 697-698. Real corpus bytes enter through search.sh's
 * identical-image MILTON/mtools differential. Expected replacement uses the
 * pre-existing piece-table primitives, never the new replacement routine. */
#define main p12_first_main
#include "test_iword_face.c"
#undef main
static void reset_face(void)
{
    g_mode = M_EDIT; g_reveal = 0; g_current[0] = 0; g_home = 0;
    doc_reset(); paint_all();
}

static int body_is(const uint8_t *b, uint32_t n)
{
    uint8_t got[65536];
    return g_ed.doc.len == n && iw_doc_read(&g_ed.doc, 0, got, n) == n && !memcmp(b, got, n);
}
static void replace_start(int confirm, const char *q, const char *r)
{
    fkey(0x3C, ALT); tks(confirm ? "y" : "n");
    tks(q); fkey(0x3C, 0); tks(r); fkey(0x3C, 0);
}
static void replace_test(void)
{
    reset_face(); tks("Dog dog DOG"); g_ed.cur = 0;
    fkey(0x3C, ALT);
    CHECK(row_is(23, "w/Confirm? No (Yes)"), "replace", "confirmation choice prompt");
    tks("n"); CHECK(row_is(23, "-> Srch:"), "replace", "search prompt");
    tks("dog"); fkey(0x3C, 0);
    CHECK(row_is(23, "Replace with:"), "replace", "replacement prompt");
    tks("cat"); fkey(0x3C, 0);
    CHECK(body_is((const uint8_t *)"Cat cat CAT", 11), "replace", "case-preserving replace all");
    CHECK(g_ed.dirty, "replace", "replacement marks document dirty");
    reset_face(); tks("dog dog dog"); g_ed.cur = 0;
    replace_start(1, "dog", "fox"); tks("n"); tks("y"); fkey(0x3B, 0);
    CHECK(body_is((const uint8_t *)"dog fox dog", 11), "replace", "confirm No Yes Cancel affect only accepted match");
    reset_face(); tks("aaa"); g_ed.cur = 0;
    replace_start(0, "a", "aa");
    CHECK(body_is((const uint8_t *)"aaaaaa", 6), "replace", "replacement is not searched again");
    reset_face(); tks("dog dog");
    fkey(0x3C, ALT); tks("n"); fkey(0x48, 0); tks("dog"); fkey(0x3C, SHIFT); tks("x"); fkey(0x01, 0);
    CHECK(body_is((const uint8_t *)"x x", 3), "replace", "backward replace uses Up direction and Escape submits");
    reset_face(); fkey(0x40, 0); fkey(0x40, 0); tks("keep"); g_ed.cur = 0;
    fkey(0x3C, ALT); tks("n"); fkey(0x40, 0); fkey(0x40, 0); fkey(0x3C, 0); fkey(0x3C, 0);
    CHECK(body_is((const uint8_t *)"keep", 4), "replace", "empty replacement removes complete paired code tokens");
    reset_face(); tks("a"); g_ed.cur = 0;
    fkey(0x3C, ALT); tks("n"); tks("a"); fkey(0x3C, 0); fkey(0x0F, 0); fkey(0x3C, 0);
    CHECK(iw_unit_kind(&g_ed.doc, 0) == IW_U_TAB && g_ed.doc.len == 9,
          "replace", "replacement accepts a complete Tab code and recomputes its geometry");
    reset_face(); tks("keep"); g_ed.cur = 2; fkey(0x3C, ALT); fkey(0x3B, 0);
    CHECK(body_is((const uint8_t *)"keep", 4) && g_ed.cur == 2, "replace", "Cancel before replacement preserves document and cursor");
}

static void replace_corpus(const char *path)
{
    static uint8_t file[65536], model_add[65536], want[65536];
    static iw_piece_t model_pc[8192];
    iw_wp5_t w;
    FILE *f = fopen(path, "rb");
    CHECK(f != 0, "replace-corpus", "real file opens"); if (!f) return;
    uint32_t n = (uint32_t)fread(file, 1, sizeof file, f); fclose(f);
    CHECK(iw_wp5_read(file, n, &w) == IW_OK, "replace-corpus", "real WP file decodes");
    if (iw_wp5_read(file, n, &w) != IW_OK) return;
    /* Each eligible byte gets its own pristine document. The independent
     * model deletes ONE literal span, inserts the expected uppercase byte,
     * and runs P2's already mutation-proven reflow/bookkeeping. */
    for (uint32_t p = 0; p < w.body_len; ) {
        uint32_t k = iw_unit_len(w.body, w.body_len, p);
        if (!k) return;
        uint8_t c = w.body[p];
        int code = k == 3 && c == 0xC3 && (w.body[p+1] == 12 || w.body[p+1] == 14);
        if ((k == 1 && c >= 'a' && c <= 'z') || code) {
            iw_ed_t model; uint8_t r = (uint8_t)(c - 32); char q[2] = {(char)c, 0}, repl[2] = {(char)r, 0};
            reset_face(); memcpy(g_bank[1u-g_live], file, n); adopt(n, w.prefix_len); g_ed.cur = p;
            iw_doc_init(&model.doc, model_add, sizeof model_add, model_pc, 8192, 65536);
            iw_doc_load(&model.doc, w.body, w.body_len); iw_ed_init(&model, IW_WP5_WRAP_COLS);
            CHECK(iw_doc_delete(&model.doc, p, k) == IW_OK && (code || iw_doc_insert(&model.doc, p, &r, 1) == IW_OK),
                  "replace-corpus", "independent primitive model edits");
            model.cur = p+(code ? 0 : 1); (void)iw_reflow_all(&model.doc, model.width); (void)iw_ed_fixup(&model);
            if (code) {
                fkey(0x3C, ALT); tks("y"); fkey(w.body[p+1] == 12 ? 0x40 : 0x42, 0);
                fkey(0x3C, 0); fkey(0x3C, 0);
            } else replace_start(1, q, repl);
            tks("y"); fkey(0x3B, 0);
            iw_doc_read(&model.doc, 0, want, model.doc.len);
            CHECK(body_is(want, model.doc.len), "replace-corpus", "document bytes equal independent piece-table operation");
            /* Existing no-op Retrieve/save mint authority: same bytes and
             * same cursor serialize exactly as the old codec does. */
        }
        p += k;
    }
}
static void serial_replace(const char *path)
{
    FILE *f = fopen(path, "r"); char line[1200]; unsigned seen = 0;
    CHECK(f != 0, "replace", "emulator capture exists"); if (!f) return;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "y=368 s=\"w/Confirm? No (Yes) ")) seen |= 1;
        if (strstr(line, "y=368 s=\"Replace with:")) seen |= 2;
        if (strstr(line, "y=0 s=\"Cat cat ")) seen |= 4;
        if (strstr(line, "y=0 s=\"Cat fox ")) seen |= 8;
        if (strstr(line, "y=368 s=\"-> Srch:[BOLD][bold] ")) seen |= 16;
    }
    fclose(f); CHECK(seen == 31, "replace", "emulator prompts, accepted replacements and paired-code query");
}
static void undo_test(void)
{
    reset_face(); tks("abcd\b\b");
    fkey(0x3B, 0);
    CHECK(row_is(23, "Undelete 1 Restore; 2 Previous Deletion: 0"), "undo", "three-level prompt");
    CHECK(body_is((const uint8_t *)"ab", 2), "undo", "preview does not alter document bytes");
    CHECK(m_inv[0][2] && m_inv[0][3] && m_scr[0][2] == 'c' && m_scr[0][3] == 'd',
          "undo", "group preview is in reverse video at cursor");
    tks("1"); CHECK(body_is((const uint8_t *)"abcd", 4), "undo", "contiguous Backspace restores in original order");
    reset_face(); tks("abcd");
    for (int i = 0; i < 4; i++) {
        tk(8); if (i != 3) { fkey(0x4B, 0); fkey(0x4D, 0); }
    }
    fkey(0x3B, 0); tks("2"); tks("2"); tks("1");
    CHECK(body_is((const uint8_t *)"c", 1), "undo", "third retained group survives fourth deletion");
    fkey(0x3B, 0); tks("2"); tks("2"); tks("2"); tks("1");
    CHECK(body_is((const uint8_t *)"cc", 2), "undo", "oldest fourth deletion is unavailable");
    reset_face(); tks("abc"); g_ed.cur = 0; fkey(0x53, 0); fkey(0x53, 0);
    fkey(0x3B, 0); fkey(0x3B, 0);
    CHECK(body_is((const uint8_t *)"c", 1), "undo", "Cancel removes preview without inserting");
    fkey(0x3B, 0); tks("1");
    CHECK(body_is((const uint8_t *)"abc", 3), "undo", "contiguous Delete preserves order");
    reset_face(); tks("ab"); g_ed.cur = 0; fkey(0x3E, ALT); fkey(0x4D, 0); fkey(0x53, 0); tks("y");
    CHECK(body_is((const uint8_t *)"b", 1), "undo", "recording a block deletion preserves the selected span");
    fkey(0x3B, 0); tks("1");
    CHECK(body_is((const uint8_t *)"ab", 2), "undo", "block deletion restores through the mint-proven retrieve model");
    reset_face(); tks("abcd\b");
    fkey(0x3C, SHIFT); tks("a"); fkey(0x3C, 0);
    fkey(0x3C, 0); tks("bc"); fkey(0x3C, 0); tk(8);
    fkey(0x3B, 0); tks("1");
    CHECK(body_is((const uint8_t *)"abc", 3), "undo", "Search cursor motion separates deletion groups even when cursor returns");
}
static void undo_corpus(const char *path)
{
    static uint8_t file[65536]; iw_wp5_t w; FILE *f = fopen(path, "rb");
    CHECK(f != 0, "undo-corpus", "real document opens"); if (!f) return;
    uint32_t n = (uint32_t)fread(file, 1, sizeof file, f); fclose(f);
    CHECK(iw_wp5_read(file, n, &w) == IW_OK, "undo-corpus", "real WP document decodes");
    if (iw_wp5_read(file, n, &w) != IW_OK) return;
    for (uint32_t p = 0; p < w.body_len; ) {
        uint32_t k = iw_unit_len(w.body, w.body_len, p); if (!k) return;
        if (k == 1 && w.body[p] >= 'a' && w.body[p] <= 'z') {
            reset_face(); memcpy(g_bank[1u-g_live], file, n); adopt(n, w.prefix_len); g_ed.cur = p;
            fkey(0x53, 0); fkey(0x3B, 0); tks("1");
            if (!body_is(w.body, w.body_len)) {
                uint32_t i = 0; while (i < w.body_len && iw_doc_byte(&g_ed.doc, i) == w.body[i]) i++;
                fprintf(stderr, "undo-corpus byte diff: %s deletion at %u, first diff %u, got %02x expected %02x, length %u/%u\n",
                        path, p, i, (unsigned)iw_doc_byte(&g_ed.doc, i), i < w.body_len ? w.body[i] : 0,
                        g_ed.doc.len, w.body_len);
            }
            CHECK(body_is(w.body, w.body_len), "undo-corpus", "Delete and Restore preserve real document bytes");
        }
        p += k;
    }
}
static void serial_undo(const char *path)
{
    FILE *f = fopen(path, "r"); char line[1200]; unsigned seen = 0; int last_abc = 0;
    CHECK(f != 0, "undo", "emulator capture exists"); if (!f) return;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "y=368 s=\"Undelete 1 Restore; 2 Previous Deletion: 0 ")) seen |= 1;
        if (strstr(line, "y=0 s=\"c\" fg=3 bg=0")) seen |= 2;
        if (strstr(line, "y=0 s=\"c ")) seen |= 4;
        if (strstr(line, "TENANT-GATE ax=0x0033 win=1 x=1 y=0 s=") && strstr(line, "fg=0 bg=3"))
            last_abc = strstr(line, "y=0 s=\"abc ") != 0;
    }
    fclose(f); CHECK(seen == 7, "undo", "emulator three-level selection preview and restore");
    CHECK(last_abc, "undo", "emulator Search cursor motion separates deletion groups");
}
/* PM-authorized Initech settings record, independently spelled bytes below.
 * Navigation/features: Reference pp. 42, 160, 574-575, 698-700. */
static void setup_test(void)
{
    char path[300]; uint8_t got[12]; FILE *f;
    m_read_limit = m_write_limit = 2; /* IBM DOS 3.30 6-137..6-140 */
    reset_face(); fkey(0x3B, SHIFT);
    CHECK(row_is(0, "Setup"), "setup", "Setup heading");
    tks("3"); CHECK(row_is(0, "Setup: Environment"), "setup", "Environment menu");
    tks("8"); CHECK(row_is(0, "Setup: Units of Measure"), "setup", "Units menu");
    tks("1c"); tks("2p"); fkey(0x41, 0); tks("1"); tks("2y");
    fkey(0x41, 0); fkey(0x41, 0); fkey(0x41, 0);
    snprintf(path, sizeof path, "%s/IWORD.SET", m_dir);
    f = fopen(path, "rb"); CHECK(f != 0, "setup", "settings file created on data volume");
    if (f) { size_t n = fread(got, 1, sizeof got, f); fclose(f);
        CHECK(n == 8 && !memcmp(got, "IWS1cp\1\0", 8), "setup", "locked settings bytes persisted");
        snprintf(path, sizeof path, "%s/IWORD.CAP", m_dir);
        f = fopen(path, "wb"); if (f) { fwrite(got, 1, n, f); fclose(f); }
        snprintf(path, sizeof path, "%s/IWORD.SET", m_dir);
    }
    CHECK(tbx_app_main() == 0, "setup", "relaunch");
    fkey(0x3B, SHIFT); tks("38");
    CHECK(row_is(4, "1 Display and Entry of Numbers for Margins, Tabs, etc.  Centimeters"), "setup", "units survive relaunch");
    CHECK(row_is(6, "2 Status Line Display  Points"), "setup", "status units survive relaunch");
    fkey(0x41, 0); tks("1");
    CHECK(row_is(6, "2 Original Document Backup  Yes"), "setup", "backup option survives relaunch");
    fkey(0x3B, 0); CHECK(g_mode == M_EDIT, "setup", "Cancel returns to document");
    f = fopen(path, "wb"); if (f) { fwrite("IWS1xx\1\0", 1, 8, f); fclose(f); }
    CHECK(tbx_app_main() == 0, "setup", "invalid settings relaunch");
    fkey(0x3B, SHIFT); tks("38");
    CHECK(row_is(4, "1 Display and Entry of Numbers for Margins, Tabs, etc.  Inches"), "setup", "invalid record uses defaults");
    m_read_limit = m_write_limit = 0;
}
static void serial_setup(const char *path)
{
    FILE *f = fopen(path, "r"); char line[1200]; unsigned seen = 0, launches = 0;
    CHECK(f != 0, "setup", "emulator capture exists"); if (!f) return;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "TENANT-REGISTER ok app=WORD")) launches++;
        if (launches < 2) continue;
        if (strstr(line, "y=0 s=\"Setup: Environment ")) seen |= 1;
        if (strstr(line, "y=0 s=\"Setup: Units of Measure ")) seen |= 2;
        if (strstr(line, "y=64 s=\"1 Display and Entry of Numbers for Margins, Tabs, etc.  Centimeters ")) seen |= 4;
        if (strstr(line, "y=96 s=\"2 Original Document Backup  Yes ")) seen |= 8;
    }
    fclose(f); CHECK(seen == 15, "setup", "emulator menus and persisted options after relaunch");
}
int main(int argc, char **argv)
{
    static int ctr[2]; g_ctr = ctr;
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "setup") && argc == 3) m_dir = argv[2];
    if (tbx_app_main()) return 2;
    if (!strcmp(argv[1], "replace")) replace_test();
    else if (!strcmp(argv[1], "replace-corpus") && argc == 3) replace_corpus(argv[2]);
    else if (!strcmp(argv[1], "replace-serial") && argc == 3) serial_replace(argv[2]);
    else if (!strcmp(argv[1], "undo")) undo_test();
    else if (!strcmp(argv[1], "undo-corpus") && argc == 3) undo_corpus(argv[2]);
    else if (!strcmp(argv[1], "undo-serial") && argc == 3) serial_undo(argv[2]);
    else if (!strcmp(argv[1], "setup") && argc == 3) setup_test();
    else if (!strcmp(argv[1], "setup-serial") && argc == 3) serial_setup(argv[2]);
    else return 2;
    printf("test-iword-%s: %d checks, %d failures\n", argv[1], ctr[0], ctr[1]);
    if (ctr[1]) fprintf(stderr, "RED-REASON %s: Reference P3 Part 2 contract\n", argv[1]);
    return ctr[1] ? 1 : 0;
}
