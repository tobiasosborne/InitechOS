/*
 * harness/diff/wp5_diff/test_iword_p4.c -- THE HOST ORACLE of InitechWord P4
 * (factory C; bead initech-8q6j): the page features -- the soft page,
 * headers / footers, footnotes / endnotes, newspaper columns, tables -- and
 * the Standard Printer stream, graded against what the REAL WordPerfect 5.1
 * (03-09-92) wrote and printed under DOSBox-X for this bead
 * (../wordperfect51-decomp mint/wpMINT33..44.steps, mint/work/WP51/MINT33..44,
 * the print streams mint/work/WP51/PRTMINTnn.TXT made by mint/print_one.sh;
 * ledger re/mint-results-005.md). Never against the artifact itself (Law 2):
 * every expected byte is a file WP.EXE wrote, every expected label a string
 * typed here from a WP screen (mint/shots_wpMINTnn/).
 *
 * PARTS (each its own RED-REASON):
 *   codec   MINT33..44 read (every unit verified), write back byte-identical,
 *           and iw_ed_fixup on each changes no byte (WP's own bookkeeping);
 *   typed   the keys of each mint driven through the core (iword_keys.h +
 *           the P4 keys below, iw_ed_fixup after every key) and written with
 *           the default prefix == the file WP.EXE saved. Two stated masks,
 *           both WP values this bead could not derive (spec/iword/
 *           wp5_page.h): the X u16 of every header / footer definition but
 *           the first of the document (W-20), and MINT41's second cell word
 *           in its one row and in its [Tbl Off] (W-24). MINT33 is not typed:
 *           its definitions are numbered from 1 after a leading [HRt] (W-20);
 *   reveal  the Reveal Codes text of lines of the mints == the panes in
 *           mint/shots_wpMINTnn/NN_reveal.png (transcribed below);
 *   print   iw_print on each minted document (read from its file) == the
 *           stream the real WP printed for it, byte for byte (14 streams);
 *   pages   the soft page as a property: N one-line paragraphs (N 1..130)
 *           typed -> an [HRt-SPg] after every 54th, its D4/00 at 200 x 53,
 *           nothing else (MINT39 rule, spec/iword/wp5_page.h);
 *   notes   renumbering as a property (Reference p. 210 "When you add or
 *           delete a note, the remaining notes in your document are
 *           renumbered automatically"): k footnotes typed, then one more put
 *           in front of note j -> the numbers run 1..k+1 in document order.
 *
 * Mutants (Rule 6; test-iword-p4-mutant, one per rule, each with the part it
 * turns RED):
 *   IWORD_MUT_HF_OCC     Odd and Even swapped in the definition   -> typed
 *   IWORD_MUT_HF_GAP     no blank line under a header             -> print
 *   IWORD_MUT_SPG_NONE   no [HRt-SPg] at a soft page end          -> pages
 *   IWORD_MUT_NOTE_RENUM notes keep the number they were typed with -> notes
 *   IWORD_MUT_NOTE_TOP   footnotes printed under the text, not at
 *                        the page's bottom                         -> print
 *   IWORD_MUT_COL_BREAK  a hard page in columns starts a new page  -> print
 *   IWORD_MUT_COL_BELOW  the text after [Col Off] starts below the
 *                        LAST column, not the longest               -> print
 *   IWORD_MUT_TBL_WIDTH  a table's columns one WPU off equal        -> typed
 *   IWORD_MUT_TBL_ROWH   every row the middle row's height         -> typed
 *   IWORD_MUT_PRT_PAGEEND no "\r" after a page's last line          -> print
 *
 * Usage: test_iword_p4 <WORDPERFECT51_DECOMP>
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
#include "iword/wp5_page.h"
#include "iword_keys.h"

TEST_HARNESS();

static const char *g_corpus;
static int g_codec_fail, g_typed_fail, g_reveal_fail, g_print_fail, g_pages_fail, g_notes_fail;

#define CAP (256u * 1024u)
static uint8_t    g_in[CAP], g_out[CAP], g_add[CAP], g_prt[CAP], g_want[CAP], g_scr[CAP];
static uint32_t   g_sink_n;
static int sink(void *ctx, const uint8_t *b, uint32_t n)
{
    (void)ctx;
    if (g_sink_n + n > CAP) return -1;
    memcpy(g_prt + g_sink_n, b, n);
    g_sink_n += n;
    return 0;
}
static iw_piece_t g_pc[8192];

static uint32_t slurp(const char *path, uint8_t *buf, uint32_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) oracle_missing("WP_GOLDENS", "%s absent\n", path);
    if (!f) return 0;
    n = fread(buf, 1, cap, f);
    fclose(f);
    return (uint32_t)n;
}

static void mint_path(const char *name, char *out, size_t cap)
{
    snprintf(out, cap, "%s/mint/work/WP51/%s", g_corpus, name);
}

static void doc_bind(iw_doc_t *d)
{
    iw_doc_init(d, g_add, CAP, g_pc, 8192, CAP);
}

/* ---- the P4 keys (the steps of mint/wpMINT33..44 by name) ------------------- *
 *   %HF=w,o,text     Shift-F8 2 3|4 1|2 o text F7 (w IW_HF_*, o 1/2/4, 0 =
 *                    Discontinue); in text "\n" is Enter, "^B" Ctrl-B
 *   %HFEDIT=w,text   Shift-F8 2 3|4 1|2 5: the definition Edit finds, its text
 *   %NOTE=k,text     Ctrl-F7 1|2 1 text F7 (k 0 footnote, 1 endnote)
 *   %COLDEF=n        Alt-F7 1 3 (the default n columns) F7
 *   %COLON %COLOFF   Alt-F7 1 1 / Alt-F7 1 2
 *   %TBL=c,r         Alt-F7 2 1 c Enter r Enter F7
 *   %CELL %PCELL     Tab / Shift-Tab in a table                              */
static uint32_t unescape(const char *s, uint8_t *out)
{
    uint32_t n = 0;
    while (*s) {
        if (s[0] == '^' && s[1] == 'B') { out[n++] = IW_HF_PAGE_NUM; s += 2; }
        else if (s[0] == '\n') { out[n++] = IW_WP5_HRT; s++; }
        else out[n++] = (uint8_t)*s++;
    }
    return n;
}

static int p4_key(iw_ed_t *e, const char *k, int *handled)
{
    uint8_t t[512];
    int rc = IW_OK;
    *handled = 1;
    if (!strncmp(k, "HF=", 3)) {
        unsigned w = 0, o = 0;
        const char *c1 = strchr(k + 3, ','), *c2 = c1 ? strchr(c1 + 1, ',') : 0;
        if (!c2 || sscanf(k + 3, "%u,%u", &w, &o) != 2) return 0;
        rc = iw_ed_hf(e, w, o, t, unescape(c2 + 1, t));
    } else if (!strncmp(k, "HFEDIT=", 7)) {
        unsigned w = 0;
        const char *c1 = strchr(k + 7, ',');
        uint32_t at;
        if (!c1 || sscanf(k + 7, "%u", &w) != 1) return 0;
        at = iw_hf_find(&e->doc, e->cur, w);
        if (at == 0xFFFFFFFFu) return 0;
        rc = iw_ed_hf_edit(e, at, t, unescape(c1 + 1, t));
    } else if (!strncmp(k, "NOTE=", 5)) {
        unsigned kind = 0;
        const char *c1 = strchr(k + 5, ',');
        if (!c1 || sscanf(k + 5, "%u", &kind) != 1) return 0;
        rc = iw_ed_note(e, kind, t, unescape(c1 + 1, t));
    } else if (!strncmp(k, "COLDEF=", 7)) rc = iw_ed_coldef(e, (uint32_t)atoi(k + 7));
    else if (!strcmp(k, "COLON")) rc = iw_ed_col_on(e);
    else if (!strcmp(k, "COLOFF")) rc = iw_ed_col_off(e);
    else if (!strncmp(k, "TBL=", 4)) {
        unsigned c = 0, r = 0;
        if (sscanf(k + 4, "%u,%u", &c, &r) != 2) return 0;
        rc = iw_ed_table(e, c, r);
    } else if (!strcmp(k, "CELL")) rc = iw_ed_cell(e, 1);
    else if (!strcmp(k, "PCELL")) rc = iw_ed_cell(e, -1);
    else { *handled = 0; return 1; }
    if (rc != IW_OK) return 0;
    return iw_ed_fixup(e) == IW_OK;
}

static int p4_run(iw_ed_t *e, const char *const *script)
{
    iwk_pending = iwk_ask = iwk_blkdel = 0;
    iwk_n = 0;
    for (; *script; script++) {
        const char *s = *script;
        if (s[0] != '%') {
            for (; *s; s++) if (iw_ed_type(e, (uint8_t)*s) != IW_OK || iw_ed_fixup(e) != IW_OK) return 0;
            continue;
        }
        {
            int handled;
            if (!p4_key(e, s + 1, &handled)) { fprintf(stderr, "  key %s refused\n", s); return 0; }
            if (handled) continue;
        }
        {
            char k[16];
            int times = 1;
            const char *star = strchr(s + 1, '*');
            size_t len = star ? (size_t)(star - (s + 1)) : strlen(s + 1);
            if (len >= sizeof k) return 0;
            memcpy(k, s + 1, len);
            k[len] = 0;
            if (star) times = atoi(star + 1);
            while (times-- > 0) if (!iwk_key(e, k)) { fprintf(stderr, "  key %s refused\n", s); return 0; }
        }
    }
    return 1;
}

/* ---- the scripts: mint/wpMINTnn.steps up to each save ------------------------ */
static const char *const S34[] = { "%HF=0,1,Initech", "%HF=2,1,Page ^B", "One.", "%CRET", "Two.", 0 };
static const char *const S35[] = { "%HF=3,2,Odd foot", "%HF=0,1,Line one\nLine two", "P1.", "%CRET", "P2.", "%CRET",
    "%HF=0,0,", "P3.", "%TOP", "%HFEDIT=0,Line one edited\nLine two", "%BOT", 0 };
static const char *const S36[] = { "Alpha", "%NOTE=0,First note.", " beta", "%NOTE=0,Second note.", " gamma",
    "%NOTE=1,End note.", ".", 0 };
static const char *const S37[] = { "%COLDEF=2", "%COLON", "The quick brown fox jumps over the lazy dog.", "%CRET",
    "Second column.", "%COLOFF", "After.", 0 };
static const char *const S38[] = { "%TBL=3,2", "A1", "%CELL", "B1", "%CELL", "C1", "%CELL", "A2", "%CELL", "B2",
    "%CELL", "C2", "%BOT", "After.", 0 };
static const char *S39[2 * 58 + 1];
static const char *const S40[] = { "Top.", "%RET", "%TBL=2,3", "x", "%CELL", "yy", "%CELL", "%CELL", "Long cell text",
    "%CELL", "q", "%CELL", "%PCELL", "%BOT", "End.", 0 };
static const char *const S41[] = { "%TBL=4,1", "a", "%CELL", "b", "%CELL", "c", "%CELL", "d", "%BOT", "Z", 0 };
static const char *const S42[] = { "One", "%NOTE=0,Note A.", " two",
    "%NOTE=0,A second note that is long enough to wrap onto a second line of the note.", " three",
    "%NOTE=0,Note C.", ".", 0 };
static const char *S43[1 + 2 * 55 + 1];
/* mint/wpMINT46.steps: test-flair-iword's paragraph, as the real WP wraps it */
static const char *const S46[] = { "Initech word processing is here. The quick brown fox jumps over the lazy dog.", 0 };
/* mint/wpMINT45.steps: one 783-character paragraph (the justified print) */
static const char *const S45[] = { "meeting red of fax in Lumbergh yeah an to pieces a paper Monday paper Lumbergh the to "
    "stapler meeting memo Initech Swingline to red memo of the cover TPS fax basement TPS and red in an yeah TPS to we "
    "need fax of of jump Initech basement memo an printer of Swingline Monday Initech red to to memo flair talk in talk "
    "to of code an need we Monday TPS Lumbergh report cover a meeting of conclusions and a about jump meeting flair "
    "mumble sheet yeah meeting printer we office of an memo an fax pieces and cover Initech to in to stapler "
    "conclusions report in mumble to meeting to conclusions to report we an to TPS red office TPS jump mumble about "
    "Initech conclusions we need a red the flair of office about office office of of in jump about Monday TPS mumble "
    "we to of conclusions flair to.", 0 };
static const char *S44[2 * 52 + 4 + 1];
static char g_lnames[60][4];

static void build_line_scripts(void)
{
    int i, k;
    for (i = 0; i < 58; i++) { snprintf(g_lnames[i], 4, "L%02d", i + 1); }
    for (i = 0, k = 0; i < 58; i++) { S39[k++] = g_lnames[i]; S39[k++] = "%RET"; }
    S39[k] = 0;
    S43[0] = "%HF=0,1,Head";
    for (i = 0, k = 1; i < 55; i++) { S43[k++] = g_lnames[i]; S43[k++] = "%RET"; }
    S43[k] = 0;
    for (i = 0, k = 0; i < 52; i++) { S44[k++] = g_lnames[i]; S44[k++] = "%RET"; }
    S44[k++] = "A long paragraph that wraps over the page break so the soft return at the end of the page "
               "shows how it is stored by WordPerfect when it falls there.";
    S44[k++] = "%RET";
    S44[k++] = "End.";
    S44[k] = 0;
}

/* ---- the masks (stated in the header) ------------------------------------------- */
static void mask_hf_x(uint8_t *b, uint32_t n, uint32_t from)
{
    uint32_t p = from, seen = 0;
    while (p + 4u <= n) {
        uint32_t L = iw_unit_len(b, n, p);
        if (!L) return;
        if (b[p] == IW_WP5_HF_GRP && L > 4u + IW_HF_HEAD_LEN && b[p + 3] == 0x00 && b[p + 2] != 0x12) {
            if (seen++) { b[p + 4 + 16] = 0; b[p + 4 + 17] = 0; }
        }
        p += L;
    }
}

static void mask_w24(uint8_t *b, uint32_t n, uint32_t from)
{
    uint32_t p = from;
    while (p + 4u <= n) {
        uint32_t L = iw_unit_len(b, n, p);
        if (!L) return;
        /* [Row]: prev info at +4, info at +4+12; [Tbl Off]: info at +4.
         * the second cell word of a 4-column row info is at info + 6 */
        if (b[p] == IW_WP5_TROW_GRP && b[p + 1] == IW_WP5_TROW_SUB) {
            b[p + 4 + 6] = b[p + 4 + 7] = 0;
            b[p + 4 + 12 + 6] = b[p + 4 + 12 + 7] = 0;
        } else if (b[p] == IW_WP5_TROW_GRP && b[p + 1] == IW_WP5_TOFF_SUB) {
            b[p + 4 + 6] = b[p + 4 + 7] = 0;
        }
        p += L;
    }
}

/* ---- part codec ----------------------------------------------------------------- */
static const char *const P4_MINTS[] = { "MINT33", "MINT34", "MINT35", "MINT36", "MINT37", "MINT38", "MINT39",
                                        "MINT40", "MINT41", "MINT42", "MINT43", "MINT44", "MINT45", "MINT46" };
#define NP4 ((int)(sizeof P4_MINTS / sizeof P4_MINTS[0]))

static void part_codec(void)
{
    char path[1024];
    int good = 0;
    printf("  part codec:");
    for (int i = 0; i < NP4; i++) {
        iw_wp5_t w;
        iw_ed_t e;
        uint32_t n, outn = 0;
        int ok;
        mint_path(P4_MINTS[i], path, sizeof path);
        n = slurp(path, g_in, CAP);
        ok = n && iw_wp5_read(g_in, n, &w) == IW_OK;
        if (ok) {
            doc_bind(&e.doc);
            iw_doc_load(&e.doc, w.body, w.body_len);
            iw_ed_init(&e, IW_WP5_WRAP_COLS);
            e.cur = e.doc.len;
            ok = iw_wp5_write(w.prefix, w.prefix_len, &e.doc, IW_WP5_CUR_KEEP, g_out, CAP, &outn) == IW_OK &&
                 outn == n && !memcmp(g_out, g_in, n);
            if (ok) {
                /* MINT41: the stated W-24 mask (the one minted divergence) */
                int w24 = !strcmp(P4_MINTS[i], "MINT41");
                ok = iw_ed_fixup(&e) == IW_OK &&
                     iw_wp5_write(w.prefix, w.prefix_len, &e.doc, IW_WP5_CUR_KEEP, g_out, CAP, &outn) == IW_OK &&
                     outn == n;
                if (ok && w24) { mask_w24(g_out, outn, w.prefix_len); mask_w24(g_in, n, w.prefix_len); }
                ok = ok && !memcmp(g_out, g_in, n);
                if (!ok) {
                    uint32_t j = 0;
                    while (j < n && j < outn && g_out[j] == g_in[j]) j++;
                    fprintf(stderr, "  FAIL codec %s: iw_ed_fixup changed WP's bytes (first at 0x%X, %u vs %u bytes)\n",
                            P4_MINTS[i], j, outn, n);
                }
            } else fprintf(stderr, "  FAIL codec %s: write-back differs\n", P4_MINTS[i]);
        } else fprintf(stderr, "  FAIL codec %s: refused at 0x%X\n", P4_MINTS[i], n ? w.err_off : 0u);
        CHECK(ok, "every P4 mint reads, writes back byte-identical, and WP's bookkeeping is a fixed point");
        if (!ok) g_codec_fail++;
        good += ok;
        printf(" %s %s", P4_MINTS[i], ok ? "==" : "!=");
    }
    printf(" (%d of %d)\n", good, NP4);
}

/* ---- part typed ------------------------------------------------------------------- */
static int typed_case(const char *name, const char *const *script, int masks)
{
    char path[1024];
    static iw_ed_t e;
    uint32_t plen, n, outn = 0;
    const uint8_t *prefix = iw_wp5_default_prefix(&plen);
    int ok;
    doc_bind(&e.doc);
    iw_ed_init(&e, IW_WP5_WRAP_COLS);
    if (!p4_run(&e, script)) {
        fprintf(stderr, "  FAIL typed %s: a key was refused\n", name);
        return 0;
    }
    mint_path(name, path, sizeof path);
    n = slurp(path, g_want, CAP);
    if (!n || iw_wp5_write(prefix, plen, &e.doc, e.cur, g_out, CAP, &outn) != IW_OK) return 0;
    if (masks & 1) { mask_hf_x(g_out, outn, plen); mask_hf_x(g_want, n, plen); }
    if (masks & 2) { mask_w24(g_out, outn, plen); mask_w24(g_want, n, plen); }
    ok = outn == n && !memcmp(g_out, g_want, n);
    if (!ok) {
        uint32_t j = 0;
        while (j < n && j < outn && g_out[j] == g_want[j]) j++;
        fprintf(stderr, "  FAIL typed %s: first difference at 0x%X (got %02X want %02X), %u vs %u bytes\n",
                name, j, j < outn ? g_out[j] : 0u, j < n ? g_want[j] : 0u, outn, n);
    }
    return ok;
}

static void part_typed(void)
{
    static const struct { const char *name; const char *const *script; int masks; } T[] = {
        { "MINT34", S34, 1 }, { "MINT35", S35, 1 }, { "MINT36", S36, 0 }, { "MINT37", S37, 0 },
        { "MINT38", S38, 0 }, { "MINT39", S39, 0 }, { "MINT40", S40, 0 }, { "MINT41", S41, 2 },
        { "MINT42", S42, 0 }, { "MINT43", S43, 0 }, { "MINT44", S44, 0 }, { "MINT45", S45, 0 },
        { "MINT46", S46, 0 },
    };
    int good = 0, n = (int)(sizeof T / sizeof T[0]);
    printf("  part typed:");
    for (int i = 0; i < n; i++) {
        int ok = typed_case(T[i].name, T[i].script, T[i].masks);
        CHECK(ok, "the P4 mint typed key for key is byte-identical to the file WP.EXE saved");
        if (!ok) g_typed_fail++;
        good += ok;
        printf(" %s %s", T[i].name, ok ? "==" : "!=");
    }
    printf(" (%d of %d; masks: W-20 X of later definitions, W-24 MINT41's second cell word)\n", good, n);
}

/* ---- part reveal ------------------------------------------------------------------- */
static void part_reveal(void)
{
    /* { mint, document line, the pane's text of that line } from the shots */
    static const struct { const char *name; uint32_t line; const char *want; } R[] = {
        /* mint/shots_wpMINT33/13_reveal.png rows 1-2 (one line, cut at 80) */
        { "MINT33", 1, "[Header A:Every page;Initech Header A][Header B:Even pages;Even B][Footer A:Every page;"
                       "Page ^B][Footer B:Odd pages;Odd B]Body one.[HPg]" },
        /* mint/shots_wpMINT34/06_reveal.png */
        { "MINT34", 0, "[Header A:Every page;Initech][Footer A:Every page;Page ^B]One.[HPg]" },
        /* mint/shots_wpMINT35/10_reveal.png row 4 -- document line 2: WP's
         * pane also starts a row after the [HRt] INSIDE Header A's label
         * (rows 1-2), which Face A's pane does not [reduction, stated] */
        { "MINT35", 2, "[Header A:Discontinue]P3." },
        { "MINT35", 0, "[Footer B:Odd pages;Odd foot][Header A:Every page;Line one edited[HRt]Line two]P1.[HPg]" },
        /* mint/shots_wpMINT36/10_reveal.png rows 1-2 */
        { "MINT36", 0, "Alpha[Footnote:1;[Note Num]First note.] beta[Footnote:2;[Note Num]Second note.] "
                       "gamma[Endnote:1;[Note Num]End note.]." },
        /* mint/shots_wpMINT37/10_reveal.png */
        { "MINT37", 0, "[Col Def:Newspaper;2;1\",4\";4.5\",7.5\"][Col On]The quick brown fox jumps over[SRt]" },
        { "MINT37", 1, "the lazy dog.[HPg]" },
        { "MINT37", 2, "Second column.[Col Off]" },
        { "MINT37", 3, "After." },
        /* mint/shots_wpMINT38/09_reveal.png */
        { "MINT38", 0, "[Tbl Def:I;3,2.17\",2.17\",2.17\"]" },
        { "MINT38", 1, "[Row][Cell]A1[Cell]B1[Cell]C1" },
        { "MINT38", 2, "[Row][Cell]A2[Cell]B2[Cell]C2" },
        { "MINT38", 3, "[Tbl Off]After." },
        /* mint/shots_wpMINT39/04_reveal_break.png */
        { "MINT39", 53, "L54[HRt-SPg]" },
        { "MINT39", 54, "L55[HRt]" },
        /* mint/shots_wpMINT41/05_reveal.png */
        { "MINT41", 0, "[Tbl Def:I;4,1.63\",1.63\",1.63\",1.63\"]" },
        { "MINT41", 1, "[Row][Cell]a[Cell]b[Cell]c[Cell]d" },
        { "MINT41", 2, "[Tbl Off]Z" },
    };
    char path[1024], buf[IW_REVEAL_LINE_MAX];
    int good = 0, n = (int)(sizeof R / sizeof R[0]);
    printf("  part reveal:");
    for (int i = 0; i < n; i++) {
        static iw_ed_t e;
        iw_wp5_t w;
        iw_line_t ln;
        uint32_t len, off, cl;
        int ok = 0;
        mint_path(R[i].name, path, sizeof path);
        len = slurp(path, g_in, CAP);
        if (len && iw_wp5_read(g_in, len, &w) == IW_OK) {
            doc_bind(&e.doc);
            iw_doc_load(&e.doc, w.body, w.body_len);
            iw_ed_init(&e, IW_WP5_WRAP_COLS);
            if (iw_ed_lines(&e, R[i].line, 1, &ln) != 1)
                fprintf(stderr, "  FAIL reveal %s: no line %u\n", R[i].name, R[i].line);
            else {
                (void)iw_reveal_line(&e.doc, &ln, 0xFFFFFFFFu, IW_NO_BLOCK, buf, sizeof buf, &off, &cl);
                ok = !strcmp(buf, R[i].want);
                if (!ok) fprintf(stderr, "  FAIL reveal %s line %u:\n    got  %s\n    want %s\n", R[i].name, R[i].line, buf, R[i].want);
            }
        }
        CHECK(ok, "the Reveal Codes text of the P4 codes == the real WP pane");
        if (!ok) g_reveal_fail++;
        good += ok;
    }
    printf(" %d of %d lines == the WP panes\n", good, n);
}

/* ---- part print -------------------------------------------------------------------- *
 * W-27 (stated; spec/iword/wp5_page.h): where WP puts the whole extra spaces
 * of a fully justified printed line is not derivable from the 14 justified
 * lines minted (PRTMINT42 line 57, PRTMINT44 59-60, PRTMINT45 7-18: no one
 * rule fits them -- the phase varies from line to line). A stream that is
 * not byte-identical is accepted ONLY when every line still agrees in its
 * start column, its words, in order, and its end column (so the line breaks,
 * the spare width and its spreading over the same gaps' total are graded),
 * and only lines whose golden holds a doubled space may differ at all. */
static int lines_justify_equal(const uint8_t *a, uint32_t an, const uint8_t *b, uint32_t bn, int *masked)
{
    uint32_t i = 0, j = 0;
    *masked = 0;
    while (i < an || j < bn) {
        uint32_t ie = i, je = j;
        while (ie < an && a[ie] != '\n') ie++;
        while (je < bn && b[je] != '\n') je++;
        if (ie - i != je - j || memcmp(a + i, b + j, ie - i)) {
            /* the golden's line (b) must be justified: a doubled space */
            uint32_t k, wa = i, wb = j, dbl = 0;
            for (k = j; k + 1u < je; k++) if (b[k] == ' ' && b[k + 1u] == ' ' && k > j && b[k - 1u] != ' ') dbl = 1;
            if (!dbl || ie - i != je - j) return 0;              /* same end column */
            while (wa < ie && a[wa] == ' ') wa++;
            while (wb < je && b[wb] == ' ') wb++;
            if (wa - i != wb - j) return 0;                      /* same start column */
            for (;;) {                                           /* same words */
                uint32_t ea = wa, eb = wb;
                while (ea < ie && a[ea] != ' ') ea++;
                while (eb < je && b[eb] != ' ') eb++;
                if (ea - wa != eb - wb || memcmp(a + wa, b + wb, ea - wa)) return 0;
                while (ea < ie && a[ea] == ' ') ea++;
                while (eb < je && b[eb] == ' ') eb++;
                if ((ea >= ie) != (eb >= je)) return 0;
                if (ea >= ie) break;
                wa = ea; wb = eb;
            }
            (*masked)++;
        }
        i = ie + (ie < an); j = je + (je < bn);
    }
    return 1;
}

static void part_print(void)
{
    static const char *const P[] = { "MINT02", "MINT09", "MINT33", "MINT34", "MINT35", "MINT36", "MINT37",
                                     "MINT38", "MINT39", "MINT40", "MINT41", "MINT42", "MINT43", "MINT44", "MINT45" };
    char path[1024], pname[64];
    int good = 0, exact = 0, n = (int)(sizeof P / sizeof P[0]);
    printf("  part print:");
    for (int i = 0; i < n; i++) {
        static iw_ed_t e;
        iw_wp5_t w;
        uint32_t len, want, got = 0;
        int ok = 0;
        mint_path(P[i], path, sizeof path);
        len = slurp(path, g_in, CAP);
        snprintf(pname, sizeof pname, "PRT%s.TXT", P[i]);
        mint_path(pname, path, sizeof path);
        want = slurp(path, g_want, CAP);
        if (len && want && iw_wp5_read(g_in, len, &w) == IW_OK) {
            doc_bind(&e.doc);
            iw_doc_load(&e.doc, w.body, w.body_len);
            iw_ed_init(&e, IW_WP5_WRAP_COLS);
            g_sink_n = 0;
            if (iw_print(&e, g_scr, CAP, sink, 0) == IW_OK) {
                got = g_sink_n;
                ok = got == want && !memcmp(g_prt, g_want, want);
                exact += ok;
                if (!ok) {
                    int masked = 0;
                    if (lines_justify_equal(g_prt, got, g_want, want, &masked) && masked) {
                        ok = 1;
                        printf(" [%s: %d justified line(s) W-27]", P[i], masked);
                    }
                }
                if (!ok) {
                    uint32_t j = 0, line = 1;
                    while (j < want && j < got && g_prt[j] == g_want[j]) { if (g_want[j] == '\n') line++; j++; }
                    fprintf(stderr, "  FAIL print %s: first difference at byte %u (print line %u), %u vs %u bytes\n",
                            P[i], j, line, got, want);
                }
            } else fprintf(stderr, "  FAIL print %s: iw_print refused\n", P[i]);
            /* the document itself is untouched by printing */
            {
                uint32_t outn = 0, plen;
                const uint8_t *pre = iw_wp5_default_prefix(&plen);
                (void)pre;
                if (iw_wp5_write(w.prefix, w.prefix_len, &e.doc, IW_WP5_CUR_KEEP, g_out, CAP, &outn) != IW_OK ||
                    outn != len || memcmp(g_out, g_in, len)) {
                    fprintf(stderr, "  FAIL print %s: printing changed the document\n", P[i]);
                    ok = 0;
                }
            }
        }
        CHECK(ok, "the page stream == the real WP 5.1 Standard Printer's print-to-file");
        if (!ok) g_print_fail++;
        good += ok;
        printf(" %s %s", P[i], ok ? "==" : "!=");
    }
    printf(" (%d of %d: %d byte-identical, %d with W-27 justified lines)\n", good, n, exact, good - exact);
}

/* ---- part pages ------------------------------------------------------------------- */
static void part_pages(void)
{
    int bad = 0;
    for (int N = 1; N <= 130 && !bad; N++) {
        static iw_ed_t e;
        char t[8];
        uint32_t p = 0, line = 0, spg = 0;
        doc_bind(&e.doc);
        iw_ed_init(&e, IW_WP5_WRAP_COLS);
        for (int i = 0; i < N && !bad; i++) {
            snprintf(t, sizeof t, "P%d", i);
            for (char *c = t; *c; c++) if (iw_ed_type(&e, (uint8_t)*c) != IW_OK || iw_ed_fixup(&e) != IW_OK) bad = 1;
            if (iw_ed_enter(&e) != IW_OK || iw_ed_fixup(&e) != IW_OK) bad = 1;
        }
        /* every 54th line's return is the [HRt-SPg] with its D4/00 at v =
         * 200 x 53 and the page number; every other one a plain [HRt] */
        while (p < e.doc.len && !bad) {
            int b = iw_doc_byte(&e.doc, p);
            uint32_t q = iw_unit_next(&e.doc, p);
            if (b == IW_WP5_HRT || b == IW_WP5_HRT_SPG) {
                line++;
                if ((line % IW_PG_TEXT_LINES == 0u) != (b == IW_WP5_HRT_SPG)) bad = 2;
                if (b == IW_WP5_HRT_SPG) {
                    uint8_t g[IW_WP5_PAGE_GRP_LEN];
                    uint32_t gp = iw_unit_prev(&e.doc, p);
                    spg++;
                    if (iw_doc_read(&e.doc, gp, g, sizeof g) != sizeof g || g[0] != IW_WP5_PAGE_GRP ||
                        g[1] != IW_WP5_PAGE_SUB || (g[6] | g[7] << 8) != 200u * 53u || (uint32_t)(g[8] | g[9] << 8) != spg)
                        bad = 3;
                }
            }
            p = q;
        }
        if (bad) fprintf(stderr, "  FAIL pages: %d one-line paragraphs: %s\n", N,
                         bad == 1 ? "typing refused" : bad == 2 ? "an [HRt-SPg] where the page does not end, or none where it does"
                                                     : "the soft page's D4/00 is wrong");
    }
    CHECK(!bad, "a soft page ends every 54 one-line paragraphs, its D4/00 at 200 x 53 (MINT39)");
    if (bad) g_pages_fail++;
    printf("  part pages %s: 1..130 one-line paragraphs, an [HRt-SPg] after every 54th\n", bad ? "RED" : "green");
}

/* ---- part notes ------------------------------------------------------------------- */
static void part_notes(void)
{
    int bad = 0;
    for (uint32_t k = 1; k <= 6 && !bad; k++) {
        for (uint32_t j = 0; j <= k && !bad; j++) {
            static iw_ed_t e;
            uint32_t p, want = 1, at[8], m = 0;
            doc_bind(&e.doc);
            iw_ed_init(&e, IW_WP5_WRAP_COLS);
            for (uint32_t i = 0; i < k; i++) {
                at[m++] = e.cur;
                if (iw_ed_type(&e, 'w') != IW_OK || iw_ed_note(&e, 0, (const uint8_t *)"n", 1) != IW_OK ||
                    iw_ed_fixup(&e) != IW_OK) bad = 1;
            }
            /* a new note in front of note j (at the end when j == k) */
            e.cur = j < k ? at[j] : e.doc.len;
            if (iw_ed_note(&e, 0, (const uint8_t *)"x", 1) != IW_OK || iw_ed_fixup(&e) != IW_OK) bad = 1;
            for (p = 0; p < e.doc.len && !bad; p = iw_unit_next(&e.doc, p))
                if (iw_unit_kind(&e.doc, p) == IW_U_NOTE && iw_note_number(&e.doc, p) != want++) bad = 2;
            if (!bad && want != k + 2u) bad = 3;
            if (bad) fprintf(stderr, "  FAIL notes: %u notes, one more before note %u: %s\n", k, j,
                             bad == 1 ? "refused" : "not numbered 1.. in document order");
        }
    }
    CHECK(!bad, "notes are renumbered in document order when one is added (Reference p. 210)");
    if (bad) g_notes_fail++;
    printf("  part notes %s: 1..6 footnotes, one more added before each, numbered in order\n", bad ? "RED" : "green");
}

int main(int argc, char **argv)
{
    char ref[1024];
    if (argc != 2) {
        fprintf(stderr, "usage: %s <WORDPERFECT51_DECOMP>\n", argv[0]);
        return 2;
    }
    g_corpus = argv[1];
    snprintf(ref, sizeof ref, "%s/tools/wp5_ref.py", g_corpus);
    if (!oracle_reader_available(ref)) return TEST_SUMMARY("test-iword-p4");
    build_line_scripts();
    part_codec();
    part_typed();
    part_reveal();
    part_print();
    part_pages();
    part_notes();
    (void)iwk_run;
    if (g_notes_fail)  fprintf(stderr, "  RED-REASON notes: notes are not renumbered\n");
    if (g_codec_fail)  fprintf(stderr, "  RED-REASON codec: %d mints not read/written/fixed-point byte-identical\n", g_codec_fail);
    if (g_typed_fail)  fprintf(stderr, "  RED-REASON typed: %d typed documents differ from what WP.EXE wrote\n", g_typed_fail);
    if (g_reveal_fail) fprintf(stderr, "  RED-REASON reveal: %d Reveal Codes lines differ from the WP panes\n", g_reveal_fail);
    if (g_print_fail)  fprintf(stderr, "  RED-REASON print: %d page streams differ from what WP printed\n", g_print_fail);
    if (g_pages_fail)  fprintf(stderr, "  RED-REASON pages: the soft page rule is broken\n");
    return TEST_SUMMARY("test-iword-p4");
}
