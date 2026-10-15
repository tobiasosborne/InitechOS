/*
 * harness/diff/wp5_diff/test_iword_wp5_roundtrip.c -- THE ORACLE of
 * InitechWord's .WP5 codec and document core against REAL WordPerfect 5.1
 * (factory C; bead initech-l4tj, IWORD P1; plan docs/plans/INITECH-WORD-
 * plan.md Sec 6(a)).
 *
 * Graded against two INDEPENDENT authorities (Law 2), never against itself:
 *   (1) the five documents the real WP.EXE (5.1, 03-09-92) wrote under
 *       DOSBox-X: $WORDPERFECT51_DECOMP/goldens/minted/MINT01..05.WP5, with
 *       the keystrokes that made them (mint/wp*.steps) and their skeletons
 *       transcribed in specs/file-formats/wp5-body-codes.md Sec 7;
 *   (2) the corpus' independent reader $WORDPERFECT51_DECOMP/tools/wp5_ref.py
 *       (it shares no constant with spec/iword/wp5_format.h).
 *
 * PARTS
 *   A  every golden: C read (header + every body unit validated) -> load
 *      into the piece table -> C write is BYTE-IDENTICAL to the file WP.EXE
 *      wrote; the C text equals wp5_ref.py's TEXT line for the golden, and
 *      wp5_ref.py reads the REWRITTEN file to its end with zero trailer
 *      errors. Opaque codes ride through untouched -- that is how the four
 *      formatted goldens round-trip before P2 (bead initech-jojk) gives
 *      their codes meaning.
 *   A2 WordPerfect's own wrap reproduced: re-wrapping every golden at the
 *      default 65 columns changes no byte (MINT01's SRt replaces the space
 *      after "real", 65 characters into the line, @0x188).
 *   B  TYPED documents: the keystrokes of mint/wpMINT02.steps (Return,
 *      "Plain text only.") typed into the editor and written with the
 *      default prefix are BYTE-IDENTICAL to MINT02.WP5 (all 344 bytes); the
 *      first paragraph of mint/wpmint01.steps (the 82-character sentence,
 *      Return, Return) typed into a new document gives exactly MINT01's
 *      first 84 body bytes, the SRt where WP put it.
 *   C  the plain-text subset: which goldens hold only text + HRt + SRt
 *      (interpreted fully in P1) and which carry codes P1 keeps opaque
 *      (interpreted in P2), stated from wp5-body-codes.md Sec 7.
 *   D  a synthetic document typed through the editor (three paragraphs,
 *      two of them wrapping) is read by wp5_ref.py: TEXT agrees, zero
 *      trailer errors, and the reader lists its [SRt] / [HRt] codes in order.
 *   E  refusals (Rule 2): bad magic, non-document type, wrong version,
 *      encryption, docstart out of range, an unverified code, a broken
 *      mirror, a truncated group -- each refused with its own code.
 *   F  (bead initech-jojk, P2 slice 1) ATTRIBUTE CODES TYPED: the keys of
 *      mint/wpMINT06.steps (italic through Ctrl-F8 2 4 twice, bold and
 *      underline overlapping through F6 / F8, a wrapped paragraph after
 *      them), wpMINT07.steps (bold through the Font menu) and wpMINT08.steps
 *      (an italic pair, then Backspace x3 in Reveal Codes) driven through
 *      the editor (iw_ed_attr / iw_ed_backspace with reveal on) and written
 *      with the default prefix are BYTE-IDENTICAL to the files the real WP.EXE
 *      saved -- every body byte and the prefix's byte 0x103, which records
 *      that the cursor was before the end at the save (MINT06 was saved
 *      after Home Home Up; spec/iword/wp5_format.h IW_WP5_PFX_CURSOR_OFF).
 *   G  (bead initech-jojk, P2 slice 2) the keys of mint/wpMINT09..19.steps
 *      -- tabs, indents and hard pages (MINT09), the tab WP recomputes when
 *      the text before it grows (MINT10), the "Delete [code]? No (Yes)"
 *      answers and Backspace over a tab / hard page / indent (MINT11, MINT12),
 *      Block, Move, Copy, Delete Block and retrieve into attributes (MINT12..
 *      15), a long line of pairs (MINT16), the cursor at the save (MINT17..
 *      19: prefix byte 0x103) -- driven through the editor key for
 *      key (iword_keys.h, iw_ed_fixup after each) and written with the
 *      default prefix are BYTE-IDENTICAL to the files WP.EXE saved; and
 *      iw_ed_fixup (WP's bookkeeping: the tab / indent / C6 / D4 payloads)
 *      run on every golden changes no byte (part A2).
 *
 * THE GOLDENS: MINT01..05 from goldens/minted (mint session 001); MINT06..16
 * minted for bead initech-jojk (2026-10-16) by tools/mint_drive.sh with
 * mint/wpMINT06..16.steps (ledgers re/mint-results-002.md, -003.md) -- read
 * from goldens/minted/MINTxx.WP5 once the
 * corpus files them there, until then from the harness's own output
 * mint/work/WP51/MINT0x (the file WP.EXE wrote, untouched). Absent from both
 * places is RED, never a skip.
 *
 * Mutants (Rule 6; test-iword-wp5-roundtrip-mutant):
 *   IWORD_MUT_DROP_CODE   the writer drops every soft-return code
 *                         -> RED-REASON golden-rewrite
 *   IWORD_MUT_WRAP_EARLY  the wrap ends a line one column short
 *                         -> RED-REASON typed
 *   IWORD_MUT_CURSOR_FLAG the writer never marks a save made with the cursor
 *                         before the end (0x103) -> RED-REASON typed
 *   IWORD_MUT_NO_D4       a hard page is written without its D4/00 group
 *                         -> RED-REASON typed
 *
 * Usage: test_iword_wp5_roundtrip <WORDPERFECT51_DECOMP> <scratch-dir>
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"
#include "oracle_policy.h"
#include "iword.h"
#include "iword/wp5_format.h"
#include "iword_keys.h"

TEST_HARNESS();

static const char *g_corpus, *g_scratch;
static char g_ref[1024];
static int g_rewrite_fail, g_ref_fail, g_wrap_fail, g_typed_fail, g_refuse_fail;

#define CAP (256u * 1024u)
static uint8_t    g_in[CAP], g_out[CAP], g_add[CAP];
static iw_piece_t g_pc[8192];
static char       g_text[CAP];

#define NMINT 19
static const char *const MINTS[NMINT] = { "MINT01", "MINT02", "MINT03", "MINT04", "MINT05",
                                          "MINT06", "MINT07", "MINT08", "MINT09", "MINT10",
                                          "MINT11", "MINT12", "MINT13", "MINT14", "MINT15",
                                          "MINT16", "MINT17", "MINT18", "MINT19" };

/* The golden's path (see THE GOLDENS above). */
static void golden_path(const char *name, char *out, size_t cap)
{
    FILE *f;
    snprintf(out, cap, "%s/goldens/minted/%s.WP5", g_corpus, name);
    if ((f = fopen(out, "rb")) != NULL) { fclose(f); return; }
    if (strcmp(name, "MINT06") >= 0)
        snprintf(out, cap, "%s/mint/work/WP51/%s", g_corpus, name);
}

static uint32_t load(const char *path, uint8_t *b, uint32_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) return 0;
    n = fread(b, 1, cap, f);
    fclose(f);
    return (uint32_t)n;
}

static int save(const char *path, const uint8_t *b, uint32_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    if (fwrite(b, 1, n, f) != n) { fclose(f); return 0; }
    return fclose(f) == 0;
}

/* Python's repr() of a str of bytes 0x20..0x7F (what wp5_ref.py prints). */
static void py_repr(const char *s, uint32_t n, char *out, size_t cap)
{
    int has_sq = 0, has_dq = 0;
    char q;
    size_t o = 0;
    for (uint32_t i = 0; i < n; i++) { if (s[i] == '\'') has_sq = 1; if (s[i] == '"') has_dq = 1; }
    q = (has_sq && !has_dq) ? '"' : '\'';
    out[o++] = q;
    for (uint32_t i = 0; i < n && o + 6 < cap; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\\' || c == (unsigned char)q) { out[o++] = '\\'; out[o++] = (char)c; }
        else if (c < 0x20 || c >= 0x7F) o += (size_t)snprintf(out + o, cap - o, "\\x%02x", c);
        else out[o++] = (char)c;
    }
    out[o++] = q;
    out[o] = 0;
}

/* Run wp5_ref.py on `file`; the TEXT line must equal repr(text), the parse
 * must reach END == filesize with zero trailer errors. `codes_want`, when
 * given, must appear in the CODES line. */
static char g_reader_out[CAP];
static int ref_agrees(const char *file, const char *text, uint32_t tlen, const char *codes_want)
{
    char cmd[2048], want[8192], line[8192];
    FILE *p;
    int text_ok = 0, end_ok = 0, codes_ok = codes_want ? 0 : 1;
    size_t used = 0;
    snprintf(cmd, sizeof cmd, "python3 -I '%s' '%s'", g_ref, file);
    p = popen(cmd, "r");
    if (!p) return 0;
    py_repr(text, tlen, want, sizeof want);
    g_reader_out[0] = 0;
    while (fgets(line, sizeof line, p)) {
        size_t l = strlen(line);
        if (used + l + 1 < sizeof g_reader_out) { memcpy(g_reader_out + used, line, l + 1); used += l; }
        if (l && line[l - 1] == '\n') line[--l] = 0;
        if (strncmp(line, "  TEXT: ", 8) == 0 && strcmp(line + 8, want) == 0) text_ok = 1;
        if (strncmp(line, "  CODES: ", 9) == 0 && codes_want && strstr(line, codes_want)) codes_ok = 1;
        if (strncmp(line, "  END offset=", 13) == 0) {
            unsigned a = 0, b = 1, e = 1;
            if (sscanf(line, "  END offset=%u filesize=%u trailer-errors=%u", &a, &b, &e) == 3 && a == b && e == 0) end_ok = 1;
        }
    }
    if (pclose(p) != 0) return 0;
    return text_ok && end_ok && codes_ok;
}

static void doc_bind(iw_doc_t *d)
{
    iw_doc_init(d, g_add, CAP, g_pc, 8192u, CAP);
}

/* ---- PART A + A2: every golden ------------------------------------------- */
static void part_a(void)
{
    int files = 0, rewrites = 0, agreed = 0, wraps = 0;
    for (int i = 0; i < NMINT; i++) {
        char path[1024], outp[1024];
        uint32_t n, olen = 0, tl;
        iw_wp5_t w;
        iw_doc_t d;
        int rc;
        golden_path(MINTS[i], path, sizeof path);
        n = load(path, g_in, CAP);
        CHECK(n > 0u, "golden present (a missing golden is RED, never a skip)");
        if (!n) { fprintf(stderr, "  FAIL golden missing: %s\n", path); g_rewrite_fail++; continue; }
        files++;
        rc = iw_wp5_read(g_in, n, &w);
        if (rc != IW_OK) {
            fprintf(stderr, "  FAIL read %s rc=%d at 0x%x\n", MINTS[i], rc, w.err_off);
            CHECK(0, "C reads the golden"); g_rewrite_fail++; continue;
        }
        CHECK(w.prefix_len == IW_WP5_DEFAULT_DOCSTART, "docstart 0x147 (wp5-header-and-prefix.md Sec 1)");
        doc_bind(&d);
        iw_doc_load(&d, w.body, w.body_len);
        rc = iw_wp5_write(w.prefix, w.prefix_len, &d, IW_WP5_CUR_KEEP, g_out, CAP, &olen);
        if (rc == IW_OK && olen == n && memcmp(g_out, g_in, n) == 0) rewrites++;
        else {
            uint32_t k = 0;
            while (k < n && k < olen && g_out[k] == g_in[k]) k++;
            fprintf(stderr, "  FAIL %s rewrite not byte-identical (len %u vs %u, first diff @0x%x)\n", MINTS[i], olen, n, k);
            g_rewrite_fail++;
            CHECK(0, "golden rewrite byte-identical");
        }
        snprintf(outp, sizeof outp, "%s/iword_%s.WP5", g_scratch, MINTS[i]);
        CHECK(save(outp, g_out, olen), "write the rewritten golden");
        tl = iw_wp5_text(&d, g_text, CAP);
        if (ref_agrees(path, g_text, tl, 0) && ref_agrees(outp, g_text, tl, 0)) agreed++;
        else {
            fprintf(stderr, "  FAIL wp5_ref.py disagrees with the C text of %s (or the rewrite does not parse clean):\n%s", MINTS[i], g_reader_out);
            g_ref_fail++;
            CHECK(0, "wp5_ref.py TEXT == C text; the rewrite parses clean");
        }
        /* A2: WordPerfect's own wrap, and its bookkeeping (iw_ed_fixup: the
         * tab / indent / C6 / D4 payloads from the layout) */
        rc = iw_reflow_all(&d, IW_WP5_WRAP_COLS);
        if (rc == IW_OK) {
            static iw_ed_t fe;
            fe.doc = d;
            iw_ed_init(&fe, IW_WP5_WRAP_COLS);
            rc = iw_ed_fixup(&fe);
            d = fe.doc;
        }
        if (rc == IW_OK && iw_wp5_write(w.prefix, w.prefix_len, &d, IW_WP5_CUR_KEEP, g_out, CAP, &olen) == IW_OK &&
            olen == n && memcmp(g_out, g_in, n) == 0) wraps++;
        else {
            uint32_t k = 0;
            while (k < n && k < olen && g_out[k] == g_in[k]) k++;
            fprintf(stderr, "  FAIL %s: re-wrapping at %u columns + the bookkeeping changed byte @0x%x (0x%02x, WP wrote 0x%02x)\n",
                    MINTS[i], IW_WP5_WRAP_COLS, k, k < olen ? g_out[k] : 0, k < n ? g_in[k] : 0);
            g_wrap_fail++;
            CHECK(0, "re-wrap at 65 columns reproduces WP's soft returns");
        }
    }
    CHECK(files == NMINT, "all nineteen minted goldens graded");
    printf("  part A: %d goldens; %d rewrites byte-identical; %d agree with wp5_ref.py (golden + rewrite)\n", files, rewrites, agreed);
    printf("  part A2: %d goldens unchanged by re-wrapping at %u columns + iw_ed_fixup (WP's own soft returns, tab / indent / C6 / D4 payloads)\n", wraps, IW_WP5_WRAP_COLS);
}

/* ---- PART B: typed documents ---------------------------------------------- */
static void type_str(iw_ed_t *e, const char *s)
{
    for (; *s; s++) CHECK(iw_ed_type(e, (uint8_t)*s) == IW_OK, "type");
}

static void part_b(void)
{
    static iw_ed_t e;
    uint32_t n, olen = 0, plen;
    const uint8_t *prefix = iw_wp5_default_prefix(&plen);
    char path[1024];
    iw_wp5_t w;
    int ok2 = 0, ok1 = 0;
    /* MINT02: "key Return" then "type Plain text only." (mint/wpMINT02.steps) */
    doc_bind(&e.doc);
    iw_ed_init(&e, IW_WP5_WRAP_COLS);
    CHECK(iw_ed_enter(&e) == IW_OK, "Return");
    type_str(&e, "Plain text only.");
    snprintf(path, sizeof path, "%s/goldens/minted/MINT02.WP5", g_corpus);
    n = load(path, g_in, CAP);
    CHECK(plen == IW_WP5_DEFAULT_DOCSTART && n > plen && memcmp(prefix, g_in, plen) == 0,
          "the default prefix == the 327 bytes WP.EXE wrote");
    if (iw_wp5_write(prefix, plen, &e.doc, e.cur, g_out, CAP, &olen) == IW_OK && olen == n && memcmp(g_out, g_in, n) == 0) ok2 = 1;
    else { fprintf(stderr, "  FAIL typed MINT02 != MINT02.WP5 (%u vs %u bytes)\n", olen, n); g_typed_fail++; }
    CHECK(ok2, "typed MINT02 is byte-identical to the real MINT02.WP5");
    /* MINT01's first paragraph: the sentence, Return, Return
     * (mint/wpmint01.steps lines 5-7; the first "key Return" of that run
     * was not stored, wp5-body-codes.md Sec 1 / GAPS W-6). */
    doc_bind(&e.doc);
    iw_ed_init(&e, IW_WP5_WRAP_COLS);
    type_str(&e, "Hello from InitechWord. This is a plain paragraph typed into real WordPerfect 5.1.");
    CHECK(iw_ed_enter(&e) == IW_OK && iw_ed_enter(&e) == IW_OK, "Return Return");
    snprintf(path, sizeof path, "%s/goldens/minted/MINT01.WP5", g_corpus);
    n = load(path, g_in, CAP);
    if (n && iw_wp5_read(g_in, n, &w) == IW_OK) {
        uint32_t k = e.doc.len;
        iw_doc_read(&e.doc, 0, g_out, CAP);
        if (k == 84u && w.body_len >= k && memcmp(g_out, w.body, k) == 0) ok1 = 1;
        else {
            uint32_t j = 0;
            while (j < k && j < w.body_len && g_out[j] == w.body[j]) j++;
            fprintf(stderr, "  FAIL typed MINT01 paragraph != MINT01 body (len %u; first diff @%u: 0x%02x vs WP 0x%02x)\n",
                    k, j, j < k ? g_out[j] : 0, j < w.body_len ? w.body[j] : 0);
            g_typed_fail++;
        }
    }
    CHECK(ok1, "typed MINT01 paragraph == the first 84 body bytes WP.EXE wrote (SRt at column 65)");
    printf("  part B: typed MINT02 %s byte-identical (344 bytes); typed MINT01 paragraph %s (SRt after \"real\")\n",
           ok2 ? "is" : "is NOT", ok1 ? "matches" : "does NOT match");
}

/* ---- PART C: the plain-text subset ---------------------------------------- */
static void part_c(void)
{
    /* wp5-body-codes.md Sec 7: MINT02 = HRt + text; every other golden
     * carries at least one code beyond text/HRt/SRt (bold/underline/center/
     * tab; indent/flush right/D4/HPg; margins/justification/size; footnote/
     * extended char/hard space). */
    /* MINT06 / MINT07 carry attribute pairs; MINT08's pair was deleted
     * before the save ("[HRt] Kep"). */
    /* MINT12 (block ops on plain text) is plain; MINT09/10 carry tabs,
     * indents, hard pages; MINT11, MINT13..16 attribute pairs. */
    static const int want_plain[NMINT] = { 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1 };
    int ok = 1;
    printf("  part C: plain-text subset (text + HRt + SRt only, P1 interprets fully):");
    for (int i = 0; i < NMINT; i++) {
        char path[1024];
        iw_wp5_t w;
        iw_doc_t d;
        uint32_t n, p = 0, plain = 1;
        golden_path(MINTS[i], path, sizeof path);
        n = load(path, g_in, CAP);
        if (!n || iw_wp5_read(g_in, n, &w) != IW_OK) { ok = 0; continue; }
        doc_bind(&d);
        iw_doc_load(&d, w.body, w.body_len);
        while (p < d.len) {
            int k = iw_unit_kind(&d, p);
            if (k != IW_U_TEXT && k != IW_U_HRT && k != IW_U_SRT) plain = 0;
            p = iw_unit_next(&d, p);
        }
        if ((int)plain != want_plain[i]) ok = 0;
        if (plain) printf(" %s", MINTS[i]);
    }
    printf("; every other golden carries codes (attribute pairs interpreted since initech-jojk; the rest opaque, byte-exact)\n");
    CHECK(ok, "the plain-text subset is exactly {MINT02, MINT08, MINT12, MINT17..19}");
}

/* ---- PART D: a synthetic typed document through wp5_ref.py ---------------- */
static void part_d(void)
{
    static iw_ed_t e;
    uint32_t plen, olen = 0, tl;
    const uint8_t *prefix = iw_wp5_default_prefix(&plen);
    char path[1024];
    doc_bind(&e.doc);
    iw_ed_init(&e, IW_WP5_WRAP_COLS);
    type_str(&e, "Initech word processing is here. The quick brown fox jumps over the lazy dog.");
    CHECK(iw_ed_enter(&e) == IW_OK, "Return");
    type_str(&e, "Short line.");
    CHECK(iw_ed_enter(&e) == IW_OK, "Return");
    type_str(&e, "Yeah, if you could just go ahead and make sure you do that from now on, that will be great. Mmm-kay?");
    CHECK(iw_wp5_write(prefix, plen, &e.doc, e.cur, g_out, CAP, &olen) == IW_OK, "write");
    snprintf(path, sizeof path, "%s/iword_synthetic.WP5", g_scratch);
    CHECK(save(path, g_out, olen), "save the synthetic document");
    tl = iw_wp5_text(&e.doc, g_text, CAP);
    {
        int ok = ref_agrees(path, g_text, tl,
            "T'Initech word processing is here. The quick brown fox jumps over' [SRt] T'the lazy dog.' [HRt] T'Short line.' [HRt]");
        if (!ok) { fprintf(stderr, "  FAIL wp5_ref.py on the synthetic document:\n%s", g_reader_out); g_ref_fail++; }
        CHECK(ok, "wp5_ref.py reads the synthetic document: TEXT, codes, zero trailer errors");
        printf("  part D %s: synthetic 3-paragraph document (%u bytes) read by wp5_ref.py, SRt/HRt in place\n", ok ? "green" : "RED", olen);
    }
}


/* ---- PART F: attribute codes typed (bead initech-jojk) -------------------- */
static int typed_equals(const char *name, iw_ed_t *e)
{
    char path[1024];
    uint32_t n, plen, olen = 0;
    const uint8_t *prefix = iw_wp5_default_prefix(&plen);
    golden_path(name, path, sizeof path);
    n = load(path, g_in, CAP);
    if (!n) { fprintf(stderr, "  FAIL golden missing: %s\n", path); g_typed_fail++; return 0; }
    if (iw_wp5_write(prefix, plen, &e->doc, e->cur, g_out, CAP, &olen) == IW_OK && olen == n && memcmp(g_out, g_in, n) == 0)
        return 1;
    {
        uint32_t k = 0;
        while (k < n && k < olen && g_out[k] == g_in[k]) k++;
        fprintf(stderr, "  FAIL typed %s != the real file (%u vs %u bytes; first diff @0x%x: 0x%02x, WP wrote 0x%02x)\n",
                name, olen, n, k, k < olen ? g_out[k] : 0, k < n ? g_in[k] : 0);
    }
    g_typed_fail++;
    return 0;
}

static void part_f(void)
{
    static iw_ed_t e;
    int ok6, ok7, ok8;
    /* mint/wpMINT06.steps: Return; "Italic:"; Ctrl-F8 2 4; "slanted words";
     * Ctrl-F8 2 4; " and plain."; Return; F6; "Bold"; F8; "both"; F6;
     * " under"; F8; " plain. This sentence ... here."; Return. */
    doc_bind(&e.doc);
    iw_ed_init(&e, IW_WP5_WRAP_COLS);
    CHECK(iw_ed_enter(&e) == IW_OK, "Return");
    type_str(&e, "Italic:");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_ITALIC) == IW_OK, "Ctrl-F8 2 4");
    type_str(&e, "slanted words");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_ITALIC) == IW_OK, "Ctrl-F8 2 4 again");
    type_str(&e, " and plain.");
    CHECK(iw_ed_enter(&e) == IW_OK, "Return");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_BOLD) == IW_OK, "F6");
    type_str(&e, "Bold");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_UNDERLINE) == IW_OK, "F8");
    type_str(&e, "both");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_BOLD) == IW_OK, "F6 (moves [bold] to the cursor)");
    type_str(&e, " under");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_UNDERLINE) == IW_OK, "F8 (steps past [und])");
    type_str(&e, " plain. This sentence is long enough that the paragraph must wrap onto a second line here.");
    CHECK(iw_ed_enter(&e) == IW_OK, "Return");
    e.cur = 0;                 /* Home Home Up (05_reveal_top) before the save */
    ok6 = typed_equals("MINT06", &e);
    CHECK(ok6, "typed MINT06 is byte-identical to the file WP.EXE saved (483 bytes, 0x08 at 0x103)");
    /* mint/wpMINT07.steps: Return; Ctrl-F8 2 1; "Menu bold"; Ctrl-F8 2 1; Return */
    doc_bind(&e.doc);
    iw_ed_init(&e, IW_WP5_WRAP_COLS);
    CHECK(iw_ed_enter(&e) == IW_OK, "Return");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_BOLD) == IW_OK, "Ctrl-F8 2 1");
    type_str(&e, "Menu bold");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_BOLD) == IW_OK, "Ctrl-F8 2 1 again");
    CHECK(iw_ed_enter(&e) == IW_OK, "Return");
    ok7 = typed_equals("MINT07", &e);
    CHECK(ok7, "typed MINT07 is byte-identical to the file WP.EXE saved (344 bytes)");
    /* mint/wpMINT08.steps: Return; "Kept"; Ctrl-F8 2 4; "x"; Ctrl-F8 2 4;
     * Alt-F3; Backspace x3 */
    doc_bind(&e.doc);
    iw_ed_init(&e, IW_WP5_WRAP_COLS);
    CHECK(iw_ed_enter(&e) == IW_OK, "Return");
    type_str(&e, "Kept");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_ITALIC) == IW_OK, "Ctrl-F8 2 4");
    type_str(&e, "x");
    CHECK(iw_ed_attr(&e, IW_WP5_ATTR_ITALIC) == IW_OK, "Ctrl-F8 2 4 again");
    e.reveal = 1;
    CHECK(iw_ed_backspace(&e) == IW_OK && iw_ed_backspace(&e) == IW_OK && iw_ed_backspace(&e) == IW_OK,
          "Backspace x3 in Reveal Codes");
    ok8 = typed_equals("MINT08", &e);
    CHECK(ok8, "typed MINT08 is byte-identical to the file WP.EXE saved (331 bytes: the pair went with [italc])");
    printf("  part F %s: typed MINT06 %s, MINT07 %s, MINT08 %s (attribute pairs, the cursor mark at 0x103, Reveal Codes delete)\n",
           ok6 && ok7 && ok8 ? "green" : "RED", ok6 ? "==" : "!=", ok7 ? "==" : "!=", ok8 ? "==" : "!=");
}

/* ---- PART G: the slice 2 mints typed key for key (bead initech-jojk) --------- */
/* Each script is its step file (../wordperfect51-decomp mint/wpMINTnn.steps)
 * key for key up to the save; Alt-F3 pairs and the cursor keys between the
 * shots are kept where they come before an edit. */
static const char *const S09[] = { "%RET", "%TAB", "Tab at zero", "%RET", "abc", "%TAB", "Tab at three", "%RET",
    "abcde", "%TAB", "Tab at five", "%RET", "%F4",
    "Indented once and long enough that the paragraph wraps under the indent here.", "%RET",
    "%F4", "%F4", "Twice.", "%RET", "Page one.", "%CRET", "Page two.", "%RET", "%CRET", "Page three.", "%RET",
    "%TOP", 0 };
static const char *const S10[] = { "%RET", "ab", "%TAB", "x", "%RET", "abc", "%F4", "Indent from three.", "%RET",
    "%U", "%U", "%HOME", "1234", "%D", "%D", 0 };
static const char *const S11[] = { "%RET", "Alpha", "%F6", "%BS", "%Y", " one", "%RET", "Beta", "%F6", "%DEL", "%N",
    "%REV", "%REV", "x", "%END", "%RET", "Gamma", "%F6", "%BS", "%N", "%REV", "%REV", "y", "%END", "%RET",
    "Delta", "%F6", "%DEL", "%Y", "z", "%RET", "q", "%TAB", "%BS", "r", "%RET", "Page", "%CRET", "%BS", "s", "%RET",
    "%TOP", 0 };
static const char *const S12[] = { "%RET", "Alpha Beta Gamma.", "%L*11", "%BLK", "%R*5", "%MOVE", "%END", "%RET",
    "%RET", "Copy me.", "%HOME", "%BLK", "%END", "%COPY", "%END", "%RET",
    "%RET", "Delete this. Keep this.", "%HOME", "%BLK", "%R*13", "%DEL", "%Y", "%END",
    "%RET", "Cut via menu. Stay.", "%HOME", "%BLK", "%R*14", "%CUT", "%END",
    "%RET", "Toggle", "%BLK", "%L*2", "%BLK", "%END", "%RET", "%F4", "%BS", "Indent gone.", "%RET", "%TOP", 0 };
static const char *const S13[] = { "%RET", "Plain", "%F6", "bold words", "%F6", " after.", "%END", "%L*9", "%BLK", "%R*4",
    "%REV", "%REV", "%MOVE", "%END", "%RET", "%REV", "%REV", "%END",
    "%RET", "Keep", "%F8", "under words", "%F8", " tail.", "%HOME", "%R*2", "%BLK", "%R*4",
    "%REV", "%REV", "%COPY", "%END", "%RET", "%REV", "%REV", "%END",
    "%RET", "Drop", "%F6", "bold here", "%F6", " end.", "%HOME", "%R*2", "%BLK", "%R*4",
    "%REV", "%REV", "%DEL", "%Y", "%REV", "%REV", "%END",
    "%RET", "Cut", "%F8", "under here", "%F8", " end.", "%END", "%L*7", "%BLK", "%R*4",
    "%REV", "%REV", "%DEL", "%Y", "%REV", "%REV", "%END", "%RET", "%TOP", 0 };
static const char *const S14[] = { "%RET", "ab", "%F6", "CDEF", "%F6", "gh", "%RET", "xy", "%HOME", "%BLK", "%END", "%COPY",
    "%U", "%HOME", "%R*4", "%RET", "%REV", "%REV", "%D", "%END",
    "%RET", "pq", "%F6", "RS", "%F6", "tu", "%HOME", "%R*3", "%BLK", "%R*2", "%COPY", "%HOME", "%R", "%RET",
    "%REV", "%REV", "%END",
    "%RET", "%F6", "vwxyz", "%F6", "%HOME", "%R*2", "%BLK", "%R", "%COPY", "%R*2", "%RET",
    "%REV", "%REV", "%END", "%RET", "%TOP", 0 };
static const char *const S15[] = { "%RET", "%F6", "vwxyz", "%F6", "%HOME", "%REV", "%REV", "%R", "%BLK", "%R", "%COPY",
    "%R", "%RET", "%REV", "%REV", "%END", "%RET", 0 };
static const char *const S16[] = { "%RET", "a", "%F6", "b", "%F6", "%F6", "b", "%F6", "%F6", "b", "%F6", "%F6", "b", "%F6",
    "%F6", "b", "%F6", "%F6", "b", "%F6", "%F6", "b", "%F6", "%RET",
    "L2", "%RET", "L3", "%RET", "L4", "%RET", "L5", "%RET", "L6", "%RET", "L7", "%RET", "L8", "%RET",
    "L9", "%RET", "L10", "%RET", "L11", "%RET", "L12", "%RET", "%TOP", "%D*6", 0 };
/* the prefix byte 0x103: the cursor before the end (MINT17, MINT19) or back
 * at the end after Home Home Up (MINT18) at the save */
static const char *const S17[] = { "%RET", "One", "%RET", "Two", "%RET", "Three", "%RET", "%U", "%U", 0 };
static const char *const S18[] = { "%RET", "One", "%RET", "Two", "%RET", "Three", "%RET", "%TOP", "%BOT", 0 };
static const char *const S19[] = { "%RET", "One", "%RET", "Two", "%L", 0 };

static void part_g(void)
{
    static const struct { const char *name; const char *const *script; } T[] = {
        { "MINT09", S09 }, { "MINT10", S10 }, { "MINT11", S11 }, { "MINT12", S12 },
        { "MINT13", S13 }, { "MINT14", S14 }, { "MINT15", S15 }, { "MINT16", S16 },
        { "MINT17", S17 }, { "MINT18", S18 }, { "MINT19", S19 },
    };
    static iw_ed_t e;
    int good = 0, n = (int)(sizeof T / sizeof T[0]);
    printf("  part G:");
    for (int i = 0; i < n; i++) {
        int ok;
        doc_bind(&e.doc);
        iw_ed_init(&e, IW_WP5_WRAP_COLS);
        ok = iwk_run(&e, T[i].script);
        if (!ok) { fprintf(stderr, "  FAIL typed %s: a key was refused or a question was not asked where WP asked\n", T[i].name); g_typed_fail++; }
        else ok = typed_equals(T[i].name, &e);
        CHECK(ok, "the slice 2 mint typed key for key is byte-identical to the file WP.EXE saved");
        good += ok;
        printf(" %s %s", T[i].name, ok ? "==" : "!=");
    }
    printf(" (%d of %d byte-identical: tabs, indents, hard pages, the questions, Block / Move / Copy / Delete)\n", good, n);
}

/* ---- PART E: refusals ------------------------------------------------------- */
static void part_e(void)
{
    uint32_t plen, n;
    const uint8_t *prefix = iw_wp5_default_prefix(&plen);
    iw_wp5_t w;
    int bad = 0;
#define REFUSE(mutate, want, what) do { \
        memcpy(g_in, prefix, plen); memcpy(g_in + plen, "Hi", 2); n = plen + 2u; \
        mutate; \
        if (iw_wp5_read(g_in, n, &w) != (want)) { fprintf(stderr, "  FAIL refusal: %s\n", what); bad = 1; } \
    } while (0)
    REFUSE((void)0, IW_OK, "the control document reads");
    REFUSE(g_in[1] = 'X', IW_E_FORMAT, "bad magic");
    REFUSE(g_in[9] = 0x0B, IW_E_FORMAT, "not a document (file type)");
    REFUSE(g_in[11] = 0x00, IW_E_FORMAT, "version 5.0 is not 5.1");
    REFUSE(g_in[12] = 0x01, IW_E_FORMAT, "encrypted");
    REFUSE(g_in[4] = 0x00; g_in[5] = 0x10, IW_E_FORMAT, "docstart past the end");
    REFUSE(g_in[4] = 0x08; g_in[5] = 0x00, IW_E_FORMAT, "docstart inside the header");
    REFUSE(n = 10u, IW_E_FORMAT, "shorter than a header");
    REFUSE(g_in[plen] = 0xC5, IW_E_CODE, "unverified fixed code C5");
    REFUSE(g_in[plen] = 0x09, IW_E_CODE, "unseen single byte 0x09");
    REFUSE(memcpy(g_in + plen, "\xD0\x06\x06\x00\x01\x00\x06\x00\x07\xD0", 10); n = plen + 10u, IW_E_CODE, "variable group with a broken mirror");
    REFUSE(memcpy(g_in + plen, "\xD0\x06\x06\x00\x01\x00\x06\x00\x06\xD0", 10); n = plen + 10u, IW_OK, "a mirrored D0/06 group reads");
    REFUSE(memcpy(g_in + plen, "\xC3\x0C", 2); n = plen + 2u, IW_E_CODE, "truncated fixed group");
#undef REFUSE
    CHECK(!bad, "malformed documents refused fail-loud with their own codes");
    if (bad) g_refuse_fail++;
    printf("  part E %s: 11 malformed headers/bodies refused (IW_E_FORMAT / IW_E_CODE), 2 controls read\n", bad ? "RED" : "green");
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <WORDPERFECT51_DECOMP> <scratch-dir>\n", argv[0]);
        return 2;
    }
    g_corpus = argv[1];
    g_scratch = argv[2];
    snprintf(g_ref, sizeof g_ref, "%s/tools/wp5_ref.py", g_corpus);
    if (!oracle_reader_available(g_ref)) return TEST_SUMMARY("test-iword-wp5-roundtrip");
    part_a();
    part_b();
    part_c();
    part_d();
    part_e();
    part_f();
    part_g();
    if (g_rewrite_fail) fprintf(stderr, "  RED-REASON golden-rewrite: %d goldens not byte-identical\n", g_rewrite_fail);
    if (g_ref_fail)     fprintf(stderr, "  RED-REASON wp5-ref-disagrees: %d readings differ from wp5_ref.py\n", g_ref_fail);
    if (g_wrap_fail)    fprintf(stderr, "  RED-REASON wrap: %d goldens re-wrapped differently from WP\n", g_wrap_fail);
    if (g_typed_fail)   fprintf(stderr, "  RED-REASON typed: %d typed documents differ from what WP.EXE wrote\n", g_typed_fail);
    if (g_refuse_fail)  fprintf(stderr, "  RED-REASON refusal: a malformed document was not refused\n");
    return TEST_SUMMARY("test-iword-wp5-roundtrip");
}
