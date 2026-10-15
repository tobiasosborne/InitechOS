/*
 * harness/diff/wp5_diff/test_iword_reveal_codes.c -- THE REVEAL CODES DUMP
 * ORACLE of InitechWord (factory C; bead initech-jojk, IWORD P2 slice 1;
 * plan docs/plans/INITECH-WORD-plan.md Sec 6(d): "a diff oracle comparing a
 * serialized Reveal-Codes-mode screen dump (the code-glyph stream, not
 * pixels) against the P0 mint corpus's captures").
 *
 * THE REFERENCE is twelve Reveal Codes screens the real WP.EXE (5.1,
 * 03-09-92) drew under DOSBox-X, each pane row TRANSCRIBED BY HAND into the
 * table below from the shot file named beside it (../wordperfect51-decomp):
 *   goldens/minted/MINT01..05.shots/03_reveal.png      (mint session 001)
 *   mint/shots_wpMINT06/03_reveal.png, 05_reveal_top.png (bead initech-jojk)
 *   mint/shots_wpMINT07/03_reveal.png
 *   mint/shots_wpMINT08/03a_reveal_before.png, 03b_reveal_bs1.png,
 *                       03c_reveal_bs2.png, 03_reveal.png
 * Nothing here is read from spec/iword/reveal_codes.h (Law 2: an oracle that
 * shares its expected strings with the artifact agrees by construction).
 *
 * FOR EACH SCREEN the document is the one WP.EXE saved (MINT01..07: the
 * file, read by the C codec; MINT08's four screens: its keystrokes typed
 * into the editor, the screens being taken before the save), the cursor
 * where the shot shows it, and the pane is rendered at WordPerfect's own 80
 * columns and 11 rows (iw_reveal_pane). Graded:
 *   - the ANCHOR: the document line on the pane's first row (RED-REASON pane);
 *   - every ROW's text, character for character (RED-REASON label);
 *   - the CURSOR's highlight: row, column and length -- one cell at the end
 *     of the document, the whole label on a code (RED-REASON pane).
 * ONE ROW IS EXCLUDED, by name: MINT05 row 1 ("Footnote host.[Footnote:1;
 * [Note Num]A footnote.] Cafe-acute and a hard[ ]space.[HRt]") -- the
 * footnote label and the extended character are later slices (P4 footnotes;
 * the C0 character sets), shown "[?]" by this one; its other two rows are
 * graded.
 *
 * Mutants (Rule 6; test-iword-reveal-codes-mutant):
 *   IWORD_MUT_LABEL_ITALC  italic labelled [ITAL]/[ital] -> RED-REASON label
 *   IWORD_MUT_PANE_ANCHOR  two lines above the cursor   -> RED-REASON pane
 *
 * Usage: test_iword_reveal_codes <WORDPERFECT51_DECOMP>
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"
#include "iword.h"

TEST_HARNESS();

static const char *g_corpus;
static int g_label_fail, g_pane_fail;

#define CAP (256u * 1024u)
static uint8_t    g_in[CAP], g_add[CAP];
static iw_piece_t g_pc[8192];

#define WP_COLS 80u
#define WP_ROWS 11u
#define SKIP_ROW "\x01"            /* an excluded row (see the header) */

typedef struct screen {
    const char *shot;              /* the transcribed shot                   */
    const char *doc;               /* a golden's name, or 0 = typed MINT08   */
    int         typed_bs;          /* typed MINT08: Backspaces in Reveal     */
    int         cursor_top;        /* 1 = the cursor at 0, else at the end   */
    uint32_t    anchor;            /* the document line on pane row 0        */
    uint32_t    cur_row, cur_col, cur_len;
    const char *rows[WP_ROWS];     /* unlisted rows are empty                */
} screen_t;

static const screen_t SCREENS[] = {
    { "goldens/minted/MINT01.shots/03_reveal.png", "MINT01", 0, 0, 3, 3, 0, 1, {
      "[BOLD]Bold[bold] and [UND]underlined[und] words.[HRt]",
      "[Center]Centered line[HRt]",
      "[Tab]Tabbed line[HRt]",
      "" } },
    { "goldens/minted/MINT02.shots/03_reveal.png", "MINT02", 0, 0, 0, 1, 16, 1, {
      "[HRt]",
      "Plain text only." } },
    { "goldens/minted/MINT03.shots/03_reveal.png", "MINT03", 0, 0, 3, 3, 0, 1, {
      "[Flsh Rgt]Flush right[HRt]",
      "[HPg]",
      "Second page.[HRt]",
      "" } },
    { "goldens/minted/MINT04.shots/03_reveal.png", "MINT04", 0, 0, 0, 3, 0, 1, {
      "[HRt]",
      "[L/R Mar:2\",2\"][Just:Left]Left justified with 2 inch margins.[HRt]",
      "[LARGE]Large text[large] then normal.[HRt]",
      "" } },
    { "goldens/minted/MINT05.shots/03_reveal.png", "MINT05", 0, 0, 0, 2, 0, 1, {
      "[HRt]",
      SKIP_ROW,
      "" } },
    { "mint/shots_wpMINT06/03_reveal.png", "MINT06", 0, 0, 1, 4, 0, 1, {
      "Italic:[ITALC]slanted words[italc] and plain.[HRt]",
      "[BOLD]Bold[UND]both[bold] under[und] plain. This sentence is long enough that th",
      "e[SRt]",
      "paragraph must wrap onto a second line here.[HRt]",
      "" } },
    { "mint/shots_wpMINT06/05_reveal_top.png", "MINT06", 0, 1, 0, 0, 0, 5, {
      "[HRt]",
      "Italic:[ITALC]slanted words[italc] and plain.[HRt]",
      "[BOLD]Bold[UND]both[bold] under[und] plain. This sentence is long enough that th",
      "e[SRt]",
      "paragraph must wrap onto a second line here.[HRt]",
      "" } },
    { "mint/shots_wpMINT07/03_reveal.png", "MINT07", 0, 0, 0, 2, 0, 1, {
      "[HRt]",
      "[BOLD]Menu bold[bold][HRt]",
      "" } },
    { "mint/shots_wpMINT08/03a_reveal_before.png", 0, 0, 0, 0, 1, 19, 1, {
      "[HRt]",
      "Kept[ITALC]x[italc]" } },
    { "mint/shots_wpMINT08/03b_reveal_bs1.png", 0, 1, 0, 0, 1, 5, 1, {
      "[HRt]",
      "Keptx" } },
    { "mint/shots_wpMINT08/03c_reveal_bs2.png", 0, 2, 0, 0, 1, 4, 1, {
      "[HRt]",
      "Kept" } },
    { "mint/shots_wpMINT08/03_reveal.png", 0, 3, 0, 0, 1, 3, 1, {
      "[HRt]",
      "Kep" } },
};
#define NSCREENS ((int)(sizeof SCREENS / sizeof SCREENS[0]))

static uint32_t load(const char *path, uint8_t *b, uint32_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) return 0;
    n = fread(b, 1, cap, f);
    fclose(f);
    return (uint32_t)n;
}

/* goldens/minted, or the mint harness's output for this bead's mints (the
 * test-iword-wp5-roundtrip rule, THE GOLDENS) */
static uint32_t load_golden(const char *name)
{
    char path[1024];
    uint32_t n;
    snprintf(path, sizeof path, "%s/goldens/minted/%s.WP5", g_corpus, name);
    n = load(path, g_in, CAP);
    if (!n && strcmp(name, "MINT06") >= 0) {
        snprintf(path, sizeof path, "%s/mint/work/WP51/%s", g_corpus, name);
        n = load(path, g_in, CAP);
    }
    if (!n) fprintf(stderr, "  FAIL golden missing: %s\n", name);
    return n;
}

static int build(const screen_t *sc, iw_ed_t *e)
{
    iw_doc_init(&e->doc, g_add, CAP, g_pc, 8192u, CAP);
    iw_ed_init(e, 65u);
    if (sc->doc) {
        iw_wp5_t w;
        uint32_t n = load_golden(sc->doc);
        if (!n || iw_wp5_read(g_in, n, &w) != IW_OK) return 0;
        iw_doc_load(&e->doc, w.body, w.body_len);
        e->cur = sc->cursor_top ? 0u : e->doc.len;
        return 1;
    }
    /* mint/wpMINT08.steps: Return; "Kept"; Ctrl-F8 2 4; "x"; Ctrl-F8 2 4;
     * Alt-F3; then Backspace x typed_bs */
    iw_doc_load(&e->doc, g_in, 0);
    if (iw_ed_enter(e) != IW_OK) return 0;
    for (const char *s = "Kept"; *s; s++) if (iw_ed_type(e, (uint8_t)*s) != IW_OK) return 0;
    if (iw_ed_attr(e, 0x08) != IW_OK || iw_ed_type(e, 'x') != IW_OK || iw_ed_attr(e, 0x08) != IW_OK) return 0;
    e->reveal = 1;
    for (int i = 0; i < sc->typed_bs; i++) if (iw_ed_backspace(e) != IW_OK) return 0;
    return 1;
}

int main(int argc, char **argv)
{
    static iw_ed_t e;
    static char rows[WP_ROWS][WP_COLS + 1];
    int graded_rows = 0, skipped = 0;
    if (argc != 2) { fprintf(stderr, "usage: %s <WORDPERFECT51_DECOMP>\n", argv[0]); return 2; }
    g_corpus = argv[1];
    for (int i = 0; i < NSCREENS; i++) {
        const screen_t *sc = &SCREENS[i];
        iw_pane_t pane;
        int bad_label = 0, bad_pane = 0;
        if (!build(sc, &e)) {
            fprintf(stderr, "  FAIL %s: the document could not be built\n", sc->shot);
            g_label_fail++;
            CHECK(0, "document built");
            continue;
        }
        iw_reveal_pane(&e, WP_COLS, WP_ROWS, &rows[0][0], WP_COLS + 1u, &pane);
        if (pane.first_line != sc->anchor) {
            fprintf(stderr, "  FAIL %s: the pane starts at document line %u, WP shows line %u\n", sc->shot, pane.first_line, sc->anchor);
            bad_pane = 1;
        } else {
            for (uint32_t r = 0; r < WP_ROWS; r++) {
                const char *want = sc->rows[r] ? sc->rows[r] : "";
                if (strcmp(want, SKIP_ROW) == 0) { skipped++; continue; }
                graded_rows++;
                if (strcmp(rows[r], want) != 0) {
                    fprintf(stderr, "  FAIL %s row %u:\n    have [%s]\n    WP   [%s]\n", sc->shot, r, rows[r], want);
                    bad_label = 1;
                }
            }
            if (pane.cur_row != sc->cur_row || pane.cur_col != sc->cur_col || pane.cur_len != sc->cur_len) {
                fprintf(stderr, "  FAIL %s: the cursor highlight is row %u col %u len %u, WP shows row %u col %u len %u\n",
                        sc->shot, pane.cur_row, pane.cur_col, pane.cur_len, sc->cur_row, sc->cur_col, sc->cur_len);
                bad_pane = 1;
            }
        }
        CHECK(!bad_label && !bad_pane, "the pane equals the real WP 5.1 Reveal Codes screen");
        g_label_fail += bad_label;
        g_pane_fail += bad_pane;
        printf("  %-46s %s\n", sc->shot, bad_label || bad_pane ? "RED" : "green");
    }
    printf("  %d screens, %d pane rows diffed at 80 columns (%d excluded by name: MINT05 row 1, footnote + extended character)\n",
           NSCREENS, graded_rows, skipped);
    if (g_label_fail) fprintf(stderr, "  RED-REASON label: %d screens show a row WordPerfect does not\n", g_label_fail);
    if (g_pane_fail)  fprintf(stderr, "  RED-REASON pane: %d screens anchor or highlight differently from WordPerfect\n", g_pane_fail);
    return TEST_SUMMARY("test-iword-reveal-codes");
}
