/*
 * harness/diff/wp5_diff/test_iword_reveal_codes.c -- THE REVEAL CODES DUMP
 * ORACLE of InitechWord (factory C; bead initech-jojk, IWORD P2 slices 1 and
 * 2; plan docs/plans/INITECH-WORD-plan.md Sec 6(d): "a diff oracle comparing
 * a serialized Reveal-Codes-mode screen dump (the code-glyph stream, not
 * pixels) against the P0 mint corpus's captures").
 *
 * THE REFERENCE is the Reveal Codes screens the real WP.EXE (5.1, 03-09-92)
 * drew under DOSBox-X, each pane row TRANSCRIBED BY HAND into the table below
 * from the shot file named beside it (../wordperfect51-decomp):
 *   goldens/minted/MINT01..05.shots/03_reveal.png      (mint session 001)
 *   mint/shots_wpMINT06..08                             (slice 1, ledger 002)
 *   mint/shots_wpMINT09..16                             (slice 2, ledger 003:
 *     tabs, indents, hard pages; the "Delete [code]?" answers; Block, Move,
 *     Copy, Delete Block, retrieve into attributes; the pane's row count)
 *   mint/shots_wpMINT21..24                             (slice 3, ledger 004:
 *     [L/R Mar:..] with the [HRt] WP puts before one typed mid-line,
 *     [Just:Left/Center/Right], [DSRt], the codes Down lands after, the
 *     answers to "Delete [L/R Mar:..]?")
 * Nothing here is read from spec/iword/reveal_codes.h (Law 2: an oracle that
 * shares its expected strings with the artifact agrees by construction). The
 * right arrow of "[->Indent]" is CP437 0x1A as the screen shows it.
 *
 * FOR EACH SCREEN the document is the one WP.EXE saved (read by the C codec)
 * or the mint's own keystrokes typed into the editor up to the shot
 * (iword_keys.h: the step file, key for key), then the cursor keys the step
 * file pressed before the shot; the pane is rendered for WordPerfect's 80
 * columns and the 11 screen lines 13..23 (iw_reveal_pane). Graded:
 *   - the ANCHOR: the document line on the pane's first row (RED-REASON pane);
 *   - every ROW's text, character for character (RED-REASON label) -- the
 *     11th row (screen line 23) blank as WP leaves it (MINT09 05, MINT16 01);
 *   - the CURSOR's highlight: row, column and length (RED-REASON pane).
 * ONE ROW IS EXCLUDED, by name: MINT05 row 1 ("Footnote host.[Footnote:1;
 * [Note Num]A footnote.] Cafe-acute and a hard[ ]space.[HRt]") -- footnotes
 * and the extended characters are later slices; its other rows are graded.
 *
 * Mutants (Rule 6; test-iword-reveal-codes-mutant):
 *   IWORD_MUT_LABEL_ITALC  italic labelled [ITAL]/[ital] -> RED-REASON label
 *   IWORD_MUT_PANE_ANCHOR  two lines above the cursor   -> RED-REASON pane
 *   IWORD_MUT_BLOCK_LABEL  no [Block] at the anchor     -> RED-REASON label
 *   IWORD_MUT_LABEL_JUST   every [Just:] shows as Left   -> RED-REASON label
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
#include "iword_keys.h"

TEST_HARNESS();

static const char *g_corpus;
static int g_label_fail, g_pane_fail;

#define CAP (256u * 1024u)
static uint8_t    g_in[CAP], g_add[CAP];
static iw_piece_t g_pc[8192];

#define WP_COLS 80u
#define WP_ROWS 11u
#define SKIP_ROW "\x01"            /* an excluded row (see the header) */
#define IND "[\x1A" "Indent]"

typedef struct screen {
    const char *shot;              /* the transcribed shot                    */
    const char *doc;               /* a golden's name, or 0 = typed (script)  */
    const char *const *script;     /* keys (iword_keys.h); 0 = cursor at end  */
    uint32_t    anchor;            /* the document line on pane row 0         */
    uint32_t    cur_row, cur_col, cur_len;
    const char *rows[WP_ROWS];     /* unlisted rows are empty                 */
} screen_t;

/* ---- the keys before each shot (the step files, key for key) ------------- */
static const char *const TOP[] = { "%TOP", 0 };
/* mint/wpMINT08.steps: Return; "Kept"; Ctrl-F8 2 4; "x"; Ctrl-F8 2 4; Alt-F3;
 * then Backspace x0..3 */
static const char *const M08_0[] = { "%RET", "Kept", "%ITAL", "x", "%ITAL", "%REV", 0 };
static const char *const M08_1[] = { "%RET", "Kept", "%ITAL", "x", "%ITAL", "%REV", "%BS", 0 };
static const char *const M08_2[] = { "%RET", "Kept", "%ITAL", "x", "%ITAL", "%REV", "%BS*2", 0 };
static const char *const M08_3[] = { "%RET", "Kept", "%ITAL", "x", "%ITAL", "%REV", "%BS*3", 0 };
/* mint/wpMINT09.steps: Home Home Up; Down x4; Down x3 */
static const char *const M09_IND[] = { "%TOP", "%D*4", 0 };
static const char *const M09_PG[]  = { "%TOP", "%D*7", 0 };
/* mint/wpMINT11.steps, to 02c_del_no_reveal and 03_bs_no_reveal */
static const char *const M11_02C[] = { "%RET", "Alpha", "%F6", "%BS", "%Y", " one", "%RET", "Beta", "%F6", "%DEL", "%N", "%REV", 0 };
static const char *const M11_03[]  = { "%RET", "Alpha", "%F6", "%BS", "%Y", " one", "%RET", "Beta", "%F6", "%DEL", "%N", "%REV",
                                       "%REV", "x", "%END", "%RET", "Gamma", "%F6", "%BS", "%N", "%REV", 0 };
/* mint/wpMINT13.steps, to 01b, 01c, 02a, 02b, 03a, 03c, 04a, 04b */
#define M13_L1 "%RET", "Plain", "%F6", "bold words", "%F6", " after.", "%END", "%L*9", "%BLK", "%R*4"
#define M13_L1M M13_L1, "%MOVE", "%END", "%RET"
#define M13_L2 M13_L1M, "%END", "%RET", "Keep", "%F8", "under words", "%F8", " tail.", "%HOME", "%R*2", "%BLK", "%R*4"
#define M13_L2C M13_L2, "%COPY", "%END", "%RET"
#define M13_L3 M13_L2C, "%END", "%RET", "Drop", "%F6", "bold here", "%F6", " end.", "%HOME", "%R*2", "%BLK", "%R*4"
#define M13_L3D M13_L3, "%DEL", "%Y"
#define M13_L4 M13_L3D, "%END", "%RET", "Cut", "%F8", "under here", "%F8", " end.", "%END", "%L*7", "%BLK", "%R*4"
static const char *const M13_01B[] = { M13_L1, 0 };
static const char *const M13_01C[] = { M13_L1M, 0 };
static const char *const M13_02A[] = { M13_L2, 0 };
static const char *const M13_02B[] = { M13_L2C, 0 };
static const char *const M13_03A[] = { M13_L3, 0 };
static const char *const M13_03C[] = { M13_L3D, 0 };
static const char *const M13_04A[] = { M13_L4, 0 };
static const char *const M13_04B[] = { M13_L4, "%DEL", "%Y", 0 };
/* mint/wpMINT14.steps, to 01, 02, 03 */
#define M14_L1 "%RET", "ab", "%F6", "CDEF", "%F6", "gh", "%RET", "xy", "%HOME", "%BLK", "%END", "%COPY", "%U", "%HOME", "%R*4", "%RET"
#define M14_L3 M14_L1, "%D", "%END", "%RET", "pq", "%F6", "RS", "%F6", "tu", "%HOME", "%R*3", "%BLK", "%R*2", "%COPY", "%HOME", "%R", "%RET"
#define M14_L4 M14_L3, "%END", "%RET", "%F6", "vwxyz", "%F6", "%HOME", "%R*2", "%BLK", "%R", "%COPY", "%R*2", "%RET"
static const char *const M14_01[] = { M14_L1, 0 };
static const char *const M14_02[] = { M14_L3, 0 };
static const char *const M14_03[] = { M14_L4, 0 };
/* mint/wpMINT15.steps, to 01 and 02 */
static const char *const M15_01[] = { "%RET", "%F6", "vwxyz", "%F6", "%HOME", 0 };
static const char *const M15_02[] = { "%RET", "%F6", "vwxyz", "%F6", "%HOME", "%R", "%BLK", "%R", "%COPY", "%R", "%RET", 0 };
/* mint/wpMINT16.steps: Home Home Up; Down x6 */
static const char *const M16_02[] = { "%TOP", "%D*6", 0 };
/* slice 3: mint/wpMINT21..24.steps up to each shot ("%MAR=L,R" WPU, "%Jx") */
#define M21_A "%RET", "%MAR=1560,2040", \
    "Margins of one point three and one point seven inches make this paragraph wrap at fifty five columns, which it does.", \
    "%RET", "%TAB", "tab", "%RET", "ab", "%TAB", "x", "%RET", "%F4", \
    "An indented paragraph under the new margins that is long enough to wrap twice over the lines.", "%RET"
static const char *const M21_09[] = { M21_A, 0 };
static const char *const M21_11[] = { M21_A, "Mid", "%MAR=2400,2400", "dle of a line.", "%RET", "%TAB", "t2", "%RET", 0 };
static const char *const M22_05[] = { "%RET",
    "wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz abcdef next words.", "%RET",
    "wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz wxyz abcde next words.", "%RET",
    "%JC", "Centered justification line.", "%RET", "%JR", "Right justified.", "%RET",
    "%JF", "Full again.", "%RET", "%JL", "Left.", "%RET", "Mid", "%JC", "after", "%RET", 0 };
#define M23_A "%RET", "%JL", "%MAR=1800,1200", "Same trip.", "%RET", "%CRET", "Page two.", "%RET", \
    "Mid2", "%L", "%L", "%MAR=1500,1380"
#define M23_B M23_A, "%END", "%RET", "x", "%TAB", "y", "%RET", \
    "The quarter inch margins wrap this paragraph somewhere near the right edge of the page, see where.", "%RET"
static const char *const M23_03[] = { M23_A, 0 };
static const char *const M23_05[] = { M23_B, "%TOP", "%D", 0 };
static const char *const M23_09[] = { M23_B, "%TOP", "%D", "%BS", "%N", "%DEL", "%N", 0 };
static const char *const M24_02[] = { "%RET", "%JC",
    "A centered paragraph long enough to wrap onto a second line and then a third line of text, to see each line.",
    "%RET", "ab", "%RET", "xyz", "%L", "%L", "Q", "%END", "%RET", "%JR", "Right line.", "%RET", "Plain", 0 };
#define M23R1 "[Just:Left][L/R Mar:1.5\",1\"]Same trip.[HRt]"

#define M13R1 "Plain[BOLD]bold word[bold]fter.[BOLD]s[bold] a[HRt]"
#define M13R2 "Keep[UND]under words[und] tail.ep[UND]u[und][HRt]"
#define M14R1 "ab[BOLD]CxyDEF[bold]gh[HRt]"
#define M14R3 "p[BOLD]RS[bold]q[BOLD]RS[bold]tu[HRt]"

static const screen_t SCREENS[] = {
    /* ---- slice 1 ---------------------------------------------------------- */
    { "goldens/minted/MINT01.shots/03_reveal.png", "MINT01", 0, 3, 3, 0, 1, {
      "[BOLD]Bold[bold] and [UND]underlined[und] words.[HRt]",
      "[Center]Centered line[HRt]",
      "[Tab]Tabbed line[HRt]",
      "" } },
    { "goldens/minted/MINT02.shots/03_reveal.png", "MINT02", 0, 0, 1, 16, 1, {
      "[HRt]",
      "Plain text only." } },
    { "goldens/minted/MINT03.shots/03_reveal.png", "MINT03", 0, 3, 3, 0, 1, {
      "[Flsh Rgt]Flush right[HRt]",
      "[HPg]",
      "Second page.[HRt]",
      "" } },
    { "goldens/minted/MINT04.shots/03_reveal.png", "MINT04", 0, 0, 3, 0, 1, {
      "[HRt]",
      "[L/R Mar:2\",2\"][Just:Left]Left justified with 2 inch margins.[HRt]",
      "[LARGE]Large text[large] then normal.[HRt]",
      "" } },
    { "goldens/minted/MINT05.shots/03_reveal.png", "MINT05", 0, 0, 2, 0, 1, {
      "[HRt]",
      SKIP_ROW,
      "" } },
    { "mint/shots_wpMINT06/03_reveal.png", "MINT06", 0, 1, 4, 0, 1, {
      "Italic:[ITALC]slanted words[italc] and plain.[HRt]",
      "[BOLD]Bold[UND]both[bold] under[und] plain. This sentence is long enough that th",
      "e[SRt]",
      "paragraph must wrap onto a second line here.[HRt]",
      "" } },
    { "mint/shots_wpMINT06/05_reveal_top.png", "MINT06", TOP, 0, 0, 0, 5, {
      "[HRt]",
      "Italic:[ITALC]slanted words[italc] and plain.[HRt]",
      "[BOLD]Bold[UND]both[bold] under[und] plain. This sentence is long enough that th",
      "e[SRt]",
      "paragraph must wrap onto a second line here.[HRt]",
      "" } },
    { "mint/shots_wpMINT07/03_reveal.png", "MINT07", 0, 0, 2, 0, 1, {
      "[HRt]",
      "[BOLD]Menu bold[bold][HRt]",
      "" } },
    { "mint/shots_wpMINT08/03a_reveal_before.png", 0, M08_0, 0, 1, 19, 1, {
      "[HRt]",
      "Kept[ITALC]x[italc]" } },
    { "mint/shots_wpMINT08/03b_reveal_bs1.png", 0, M08_1, 0, 1, 5, 1, {
      "[HRt]",
      "Keptx" } },
    { "mint/shots_wpMINT08/03c_reveal_bs2.png", 0, M08_2, 0, 1, 4, 1, {
      "[HRt]",
      "Kept" } },
    { "mint/shots_wpMINT08/03_reveal.png", 0, M08_3, 0, 1, 3, 1, {
      "[HRt]",
      "Kep" } },
    /* ---- slice 2: tabs, indents, hard pages ------------------------------- */
    { "mint/shots_wpMINT09/05_reveal_top.png", "MINT09", TOP, 0, 0, 0, 5, {
      "[HRt]",
      "[Tab]Tab at zero[HRt]",
      "abc[Tab]Tab at three[HRt]",
      "abcde[Tab]Tab at five[HRt]",
      IND "Indented once and long enough that the paragraph wraps under[SRt]",
      "the indent here.[HRt]",
      IND IND "Twice.[HRt]",
      "Page one.[HPg]",
      "Page two.[HRt]",
      "[HPg]",
      "" } },
    { "mint/shots_wpMINT09/06_reveal_indent.png", "MINT09", M09_IND, 1, 3, 0, 9, {
      "[Tab]Tab at zero[HRt]",
      "abc[Tab]Tab at three[HRt]",
      "abcde[Tab]Tab at five[HRt]",
      IND "Indented once and long enough that the paragraph wraps under[SRt]",
      "the indent here.[HRt]",
      IND IND "Twice.[HRt]",
      "Page one.[HPg]",
      "Page two.[HRt]",
      "[HPg]",
      "Page three.[HRt]",
      "" } },
    { "mint/shots_wpMINT09/07_reveal_pages.png", "MINT09", M09_PG, 4, 3, 0, 1, {
      IND "Indented once and long enough that the paragraph wraps under[SRt]",
      "the indent here.[HRt]",
      IND IND "Twice.[HRt]",
      "Page one.[HPg]",
      "Page two.[HRt]",
      "[HPg]",
      "Page three.[HRt]",
      "" } },
    { "mint/shots_wpMINT09/03_reveal.png", "MINT09", 0, 8, 3, 0, 1, {
      "Page two.[HRt]",
      "[HPg]",
      "Page three.[HRt]",
      "" } },
    { "mint/shots_wpMINT10/03_reveal.png", "MINT10", 0, 0, 3, 0, 1, {
      "[HRt]",
      "1234ab[Tab]x[HRt]",
      "abc" IND "Indent from three.[HRt]",
      "" } },
    /* ---- slice 2: the "Delete [code]?" answers ---------------------------- */
    { "mint/shots_wpMINT11/02c_del_no_reveal.png", 0, M11_02C, 0, 2, 16, 1, {
      "[HRt]",
      "Alpha one[HRt]",
      "Beta[BOLD][bold]" } },
    { "mint/shots_wpMINT11/03_bs_no_reveal.png", 0, M11_03, 0, 3, 5, 6, {
      "[HRt]",
      "Alpha one[HRt]",
      "Beta[BOLD][bold]x[HRt]",
      "Gamma[BOLD][bold]" } },
    { "mint/shots_wpMINT11/09_reveal_top.png", "MINT11", TOP, 0, 0, 0, 5, {
      "[HRt]",
      "Alpha one[HRt]",
      "Beta[BOLD][bold]x[HRt]",
      "Gammay[BOLD][bold][HRt]",
      "Deltaz[HRt]",
      "qr[HRt]",
      "Pages[HRt]",
      "" } },
    /* ---- slice 2: Block, Move, Copy, Delete Block ------------------------- */
    { "mint/shots_wpMINT12/16_reveal_top.png", "MINT12", TOP, 0, 0, 0, 5, {
      "[HRt]",
      "Alpha Gamma.[HRt]",
      "Copy me.Beta [HRt]",
      "Keep this.Copy me.Beta [HRt]",
      "Stay.[HRt]",
      "Toggle[HRt]",
      "Indent gone.[HRt]",
      "" } },
    { "mint/shots_wpMINT13/01b_block_reveal.png", 0, M13_01B, 0, 1, 36, 1, {
      "[HRt]",
      "Plain[BOLD]bold word[Block]s[bold] after." } },
    { "mint/shots_wpMINT13/01c_moved_reveal.png", 0, M13_01C, 0, 1, 31, 6, {
      "[HRt]",
      "Plain[BOLD]bold word[bold]fter.[BOLD]s[bold] a" } },
    { "mint/shots_wpMINT13/02a_block_reveal.png", 0, M13_02A, 0, 2, 17, 1, {
      "[HRt]",
      M13R1,
      "Ke[Block]ep[UND]under words[und] tail." } },
    { "mint/shots_wpMINT13/02b_copied_reveal.png", 0, M13_02B, 0, 2, 31, 1, {
      "[HRt]",
      M13R1,
      "Keep[UND]under words[und] tail.ep[UND]u[und]" } },
    { "mint/shots_wpMINT13/03a_block_reveal.png", 0, M13_03A, 0, 3, 18, 1, {
      "[HRt]",
      M13R1,
      M13R2,
      "Dr[Block]op[BOLD]bold here[bold] end." } },
    { "mint/shots_wpMINT13/03c_deleted_reveal.png", 0, M13_03C, 0, 3, 8, 1, {
      "[HRt]",
      M13R1,
      M13R2,
      "Dr[BOLD]old here[bold] end." } },
    { "mint/shots_wpMINT13/04a_block_reveal.png", 0, M13_04A, 1, 3, 32, 1, {
      M13R1,
      M13R2,
      "Dr[BOLD]old here[bold] end.[HRt]",
      "Cut[UND]under her[Block]e[und] end." } },
    { "mint/shots_wpMINT13/04b_deleted_reveal.png", 0, M13_04B, 1, 3, 22, 1, {
      M13R1,
      M13R2,
      "Dr[BOLD]old here[bold] end.[HRt]",
      "Cut[UND]under her[und]nd." } },
    { "mint/shots_wpMINT13/06_reveal_top.png", "MINT13", TOP, 0, 0, 0, 5, {
      "[HRt]",
      M13R1,
      M13R2,
      "Dr[BOLD]old here[bold] end.[HRt]",
      "Cut[UND]under her[und]nd.[HRt]",
      "" } },
    { "mint/shots_wpMINT14/01_plain_into_bold_reveal.png", 0, M14_01, 0, 1, 9, 1, {
      "[HRt]",
      "ab[BOLD]CxyDEF[bold]gh[HRt]",
      "xy" } },
    { "mint/shots_wpMINT14/02_bold_into_plain_reveal.png", 0, M14_02, 0, 3, 1, 6, {
      "[HRt]",
      M14R1,
      "xy[HRt]",
      "p[BOLD]RS[bold]q[BOLD]RS[bold]tu" } },
    { "mint/shots_wpMINT14/03_bold_into_bold_reveal.png", 0, M14_03, 1, 3, 11, 1, {
      M14R1,
      "xy[HRt]",
      M14R3,
      "[BOLD]vwxyzx[bold]" } },
    { "mint/shots_wpMINT14/05_reveal_top.png", "MINT14", TOP, 0, 0, 0, 5, {
      "[HRt]",
      M14R1,
      "xy[HRt]",
      M14R3,
      "[BOLD]vwxyzx[bold][HRt]",
      "" } },
    { "mint/shots_wpMINT15/01_home_left_reveal.png", 0, M15_01, 0, 1, 6, 1, {
      "[HRt]",
      "[BOLD]vwxyz[bold]" } },
    { "mint/shots_wpMINT15/02_bold_into_bold_mid_reveal.png", 0, M15_02, 0, 1, 9, 1, {
      "[HRt]",
      "[BOLD]vwxw[bold]yz" } },
    /* ---- slice 2: the pane fills 10 rows, the 11th stays blank ------------- */
    { "mint/shots_wpMINT16/01_reveal_top.png", "MINT16", TOP, 0, 0, 0, 5, {
      "[HRt]",
      "a[BOLD]b[bold][BOLD]b[bold][BOLD]b[bold][BOLD]b[bold][BOLD]b[bold][BOLD]b[bold][",
      "BOLD]b[bold][HRt]",
      "L2[HRt]", "L3[HRt]", "L4[HRt]", "L5[HRt]", "L6[HRt]", "L7[HRt]", "L8[HRt]",
      "" } },
    { "mint/shots_wpMINT16/02_reveal_line6.png", "MINT16", M16_02, 3, 3, 0, 1, {
      "L3[HRt]", "L4[HRt]", "L5[HRt]", "L6[HRt]", "L7[HRt]", "L8[HRt]", "L9[HRt]",
      "L10[HRt]", "L11[HRt]", "L12[HRt]",
      "" } },
    /* ---- slice 3: margins and justification ----------------------------------- */
    { "mint/shots_wpMINT21/09_reveal.png", 0, M21_09, 5, 3, 0, 1, {
      "ab[Tab]x[HRt]",
      IND "An indented paragraph under the new margins that is[SRt]",
      "long enough to wrap twice over the lines.[HRt]",
      "" } },
    { "mint/shots_wpMINT21/11_reveal_second.png", 0, M21_11, 8, 3, 0, 1, {
      "Mid[HRt]",
      "[L/R Mar:2\",2\"]dle of a line.[HRt]",
      "[Tab]t2[HRt]",
      "" } },
    { "mint/shots_wpMINT22/05_reveal.png", 0, M22_05, 8, 3, 0, 1, {
      "[Just:Left]Left.[HRt]",
      "Mid[DSRt]",
      "[Just:Center]after[HRt]",
      "" } },
    { "mint/shots_wpMINT23/03_reveal_mid.png", 0, M23_03, 2, 3, 21, 1, {
      "[HPg]",
      "Page two.[HRt]",
      "Mi[HRt]",
      "[L/R Mar:1.25\",1.15\"]d2" } },
    { "mint/shots_wpMINT23/05_reveal_line2.png", 0, M23_05, 0, 1, 28, 1, {
      "[HRt]",
      M23R1,
      "[HPg]",
      "Page two.[HRt]",
      "Mi[HRt]",
      "[L/R Mar:1.25\",1.15\"]d2[HRt]",
      "x[Tab]y[HRt]",
      "The quarter inch margins wrap this paragraph somewhere near[SRt]",
      "the right edge of the page, see where.[HRt]",
      "" } },
    { "mint/shots_wpMINT23/09_after_del_no.png", 0, M23_09, 0, 1, 28, 1, {
      "[HRt]",
      M23R1,
      "[HPg]",
      "Page two.[HRt]",
      "Mi[HRt]",
      "[L/R Mar:1.25\",1.15\"]d2[HRt]",
      "x[Tab]y[HRt]",
      "The quarter inch margins wrap this paragraph somewhere near[SRt]",
      "the right edge of the page, see where.[HRt]",
      "" } },
    { "mint/shots_wpMINT24/02_reveal.png", 0, M24_02, 3, 3, 5, 1, {
      "ab[HRt]",
      "xQyz[HRt]",
      "[Just:Right]Right line.[HRt]",
      "Plain" } },
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
    /* RE-KEYED 2026-10-16 (slice 3, stated): WP's line holds 66 characters
     * at 1"/1" (MINT22; spec/iword/wp5_format.h WRAP), was 65 */
    iw_ed_init(e, 66u);
    if (sc->doc) {
        iw_wp5_t w;
        uint32_t n = load_golden(sc->doc);
        if (!n || iw_wp5_read(g_in, n, &w) != IW_OK) return 0;
        iw_doc_load(&e->doc, w.body, w.body_len);
        e->cur = e->doc.len;
        return sc->script ? iwk_run(e, sc->script) : 1;
    }
    iw_doc_load(&e->doc, g_in, 0);
    return iwk_run(e, sc->script);
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
        printf("  %-52s %s\n", sc->shot, bad_label || bad_pane ? "RED" : "green");
    }
    printf("  %d screens, %d pane rows diffed at 80 columns x 11 lines (%d excluded by name: MINT05 row 1, footnote + extended character)\n",
           NSCREENS, graded_rows, skipped);
    if (g_label_fail) fprintf(stderr, "  RED-REASON label: %d screens show a row WordPerfect does not\n", g_label_fail);
    if (g_pane_fail)  fprintf(stderr, "  RED-REASON pane: %d screens anchor or highlight differently from WordPerfect\n", g_pane_fail);
    return TEST_SUMMARY("test-iword-reveal-codes");
}
