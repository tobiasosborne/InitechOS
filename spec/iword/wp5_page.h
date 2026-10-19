/*
 * spec/iword/wp5_page.h -- LOCKED spec-data: the page features of
 * WordPerfect 5.1 InitechWord P4 uses -- the soft page, headers / footers,
 * footnotes / endnotes, newspaper columns, tables -- and the page stream the
 * Standard Printer prints (CLAUDE.md Rule 8).
 *
 * bead: initech-8q6j (IWORD P4). Epic initech-fdxa. Plan: docs/plans/
 *       INITECH-WORD-plan.md Sec 6(e) (print-to-PRN byte goldens), Sec 7 P4.
 *
 * EVERY byte below was read from a document or a print stream the REAL
 * WP.EXE (5.1, 03-09-92) wrote under DOSBox-X for this bead (2026-10-17;
 * ../wordperfect51-decomp tools/mint_drive.sh, mint/wpMINT33..44.steps and
 * mint/print_one.sh; documents mint/work/WP51/MINT33..44, print streams
 * mint/work/WP51/PRTMINTnn.TXT; ledger re/mint-results-005.md). Menus,
 * prompts and labels are transcribed from the shots of those sessions
 * (mint/shots_wpMINTnn/, mint/shots_wpPRINT0..1/). The behaviour is the
 * WordPerfect 5.1 Reference's (initech-manuals-ref/wordperfect51/
 * WPDOS51Reference.pdf; printed page = PDF page - 17): Headers and Footers
 * pp. 269-274, Footnotes and Endnotes pp. 209-216, Columns, Newspaper pp.
 * 75-79 (Columns, Display p. 74), Table, Create pp. 640-644, Print pp.
 * 490-494, Printer, Edit / Select pp. 505-509, Printing to Disk p. 543.
 * A value marked [inferred] rests on fewer instances than its use; a value
 * marked [oracle-resolves] is carried as minted and its meaning is open
 * (corpus GAPS W-20..W-27).
 *
 * INDEPENDENCE: nothing here comes from tools/wp5_ref.py; the oracles grade
 * what this header lets the C core write against the minted bytes.
 * ASCII-clean (Rule 12). No timestamps (Rule 11).
 */
#ifndef INITECH_SPEC_IWORD_WP5_PAGE_H
#define INITECH_SPEC_IWORD_WP5_PAGE_H

/* ---- the page ------------------------------------------------------------- *
 * 11" paper, 1" top and bottom margins, 6 lines an inch: 66 lines, the text
 * on lines 7..60 -- 54 lines, 10800 WPU (PRTMINT39: L01 on print line 7, L54
 * on line 60, L55 on the next page's line 7 = 73; MINT39 @0x21E the D4/00 of
 * the soft page v = 10600 = 200 x 53). */
#define IW_PG_LINES            66u
#define IW_PG_TOP_LINES        6u      /* lines 1..6 are the top margin        */
#define IW_PG_TEXT_LINES       54u
#define IW_PG_TEXT_WPU         10800u  /* 0x2A30: "0*" in every D4/00          */
#define IW_PG_TOP_WPU          1200u   /* the 1" top margin                    */

/* [HRt-SPg] 0x8C: the hard return that ends a page's last line takes this
 * byte, a D4/00 group before it (MINT39 @0x21E: `D4 00 1C 00 | 00 16 | 68 29
 * | 01 00 | ...` then 0x8C; label "[HRt-SPg]", mint/shots_wpMINT39/
 * 04_reveal_break.png). The editing screen shows a soft page as a row of '-'
 * (01_typed.png), a hard page as '=' (MINT09). */
#define IW_WP5_HRT_SPG         0x8Cu
#define IW_RV_HRT_SPG          "[HRt-SPg]"
#define IW_PG_SOFT_BAR         '-'
/* [SPg] 0x0B: the SOFT return that ends a page's last line -- a paragraph
 * wrapping over the page end (MINT44 @0x29B: the D4/00 then 0x0B where the
 * SRt 0x0D would be; label "[SPg]", mint/shots_wpMINT44/03_reveal.png). Its
 * D4/00 has payload[10] = 2 (MINT44), as has the hard page that ends a
 * newspaper column (MINT37 @0x23E); 0 for a hard page or [HRt-SPg] (MINT09,
 * MINT39, MINT43) [oracle-resolves: what 2 means, W-21]. */
#define IW_WP5_SRT_SPG         0x0Bu
#define IW_RV_SRT_SPG          "[SPg]"
#define IW_PG_GRP_MID_OFF      10u
#define IW_PG_GRP_MID          2u

/* The D4/00 group's page fields (payload offsets; P2's IW_WP5_PAGE_GRP_INIT
 * is the no-header case). PAYLOAD[13] is 01 in MINT33's first group (its page
 * holds header codes after its first line) and in MINT37's [Col Off] group,
 * 00 elsewhere [oracle-resolves, W-21]. [14] the page's text height 10800;
 * [16] / [18] the text height of an ODD / EVEN page under the headers and
 * footers in effect, [20] / [22] where the text starts on an odd / even page
 * (WPU from the paper's top): MINT34 (Header A + Footer A every page) 10000
 * 10000 1600 1600; MINT35 (2-line Header A every page, Footer B odd pages)
 * 9800 10200 1800 1800; MINT33 second group (A every, B even, footers A
 * every, B odd) 10000 10000 1600 1600 [verified: three documents]. */
#define IW_PG_GRP_FLAG_OFF     13u
#define IW_PG_GRP_BASE_OFF     14u
#define IW_PG_GRP_ODD_H_OFF    16u
#define IW_PG_GRP_EVEN_H_OFF   18u
#define IW_PG_GRP_ODD_TOP_OFF  20u
#define IW_PG_GRP_EVEN_TOP_OFF 22u

/* ---- headers and footers (Shift-F8 2 3 / 4) -------------------------------- *
 * `D5 <which> <u16 size> | old 7 | occ | h u16 | seq | 5 x 00 | X u16 | text |
 * <u16 size> <which> D5`:
 *   which  0 Header A, 1 Header B, 2 Footer A, 3 Footer B (MINT33 @0x148,
 *          0x172, 0x192, 0x1B2);
 *   old    the definition of the same letter before this one -- its occ, h,
 *          seq and 3 zeros -- or 7 zeros (MINT35 @0x1E3: the Discontinue
 *          after a 2-line Header A every page carries `01 90 01 01 00 00 00`);
 *   occ    1 Every page, 2 Odd pages, 4 Even pages (MINT33);
 *   h      the header's height, 200 a line (MINT33/34 one line `c8 00`,
 *          MINT35 two lines `90 01`);
 *   seq    the definitions before it in the document, 0-based (MINT34 0, 1;
 *          MINT35 0, 1) [inferred: MINT33's four run 1..4 after a leading
 *          [HRt] -- W-20];
 *   X      `81 C8` on the first definition of every minted session (MINT33,
 *          34, 35: 3 of 3); later ones vary (7E 37, C7 95, 81 DA) -- WP's own
 *          value [oracle-resolves, W-20]: written as 0xC881;
 *   text   the typed text: characters, 0x0A [HRt], 0x02 the page number
 *          (Ctrl-B, shown "^B": MINT34 08_footer_typed / 06_reveal).
 * DISCONTINUE (1 at the A/B menu): `D5 <which> 12 00 | old 7 | 00 00 00 12 00
 *          12 00 | 12 00 <which> D5` (MINT35 @0x1E3) [single instance].
 * Labels (MINT33 13_reveal, MINT35 10_reveal): "[Header A:Every page;<text>]",
 * "[Header B:Even pages;...]", "[Footer B:Odd pages;...]", "[Header A:
 * Discontinue]"; the text to its first 50 characters, then "..." (Reference
 * p. 271). A page uses the definitions that stand before its first line's
 * text; one placed after it starts on the next page (p. 271; MINT33 page 1
 * prints none: the codes follow its [HRt]). */
#define IW_WP5_HF_GRP          0xD5u
#define IW_HF_HEADER_A         0u
#define IW_HF_HEADER_B         1u
#define IW_HF_FOOTER_A         2u
#define IW_HF_FOOTER_B         3u
#define IW_HF_EVERY            1u
#define IW_HF_ODD              2u
#define IW_HF_EVEN             4u
#define IW_HF_OLD_LEN          7u
#define IW_HF_HEAD_LEN         18u     /* old 7 + occ + h 2 + seq + 5 + X 2 */
#define IW_HF_X                0xC881u
#define IW_HF_PAGE_NUM         0x02u   /* ^B in the text                     */
#define IW_HF_DISC_SIZE        0x12u   /* a Discontinue group's size field    */
#define IW_HF_DISC_TAIL_INIT   { 0x00, 0x00, 0x00, 0x12, 0x00, 0x12, 0x00 }
#define IW_HF_LABEL_MAX        50u
/* The page under them (PRTMINT33/34/35): a header's lines from the first
 * text line, then one blank line, then the text; a footer's lines end on the
 * last text line, one blank line above them; Header A and B on the same page
 * print over each other on the same lines (PRTMINT33 line 73 "Initech
 * Header A\r          Even B"; Reference p. 274 "Header A will print on top
 * of Header B"). The space a header takes: 200 x (lines + 1). */
#define IW_HF_GAP_WPU          200u

/* The menus (shots_wpMINT33/34/35): */
#define IW_PG_MENU_TITLE       "Format: Page"
#define IW_PG_MENU_ROWS 12
#define IW_PG_MENU_INIT { \
    {  0,  0, "Format: Page" }, \
    {  2,  5, "1 - Center Page (top to bottom)" }, \
    {  4,  5, "2 - Force Odd/Even Page" }, \
    {  6,  5, "3 - Headers" }, \
    {  8,  5, "4 - Footers" }, \
    { 10,  5, "5 - Margins - Top" }, \
    { 11, 19, "Bottom" }, \
    { 13,  5, "6 - Page Numbering" }, \
    { 15,  5, "7 - Paper Size" }, \
    { 16, 15, "Type" }, \
    { 17, 15, "Labels" }, \
    { 19,  5, "8 - Suppress (this page only)" } \
}
#define IW_PG_VALUE_COL        40u
#define IW_PG_V_CENTER         "No"
#define IW_PG_V_MARGIN         "1\""
#define IW_PG_V_PAPER          "8.5\" x 11\""
#define IW_PG_V_TYPE           "Standard"
#define IW_HF_MENU_HEADERS     "1 Header A; 2 Header B: 0"
#define IW_HF_MENU_FOOTERS     "1 Footer A; 2 Footer B: 0"
#define IW_HF_MENU_PLACE       "1 Discontinue; 2 Every Page; 3 Odd Pages; 4 Even Pages; 5 Edit: 0"
#define IW_HF_EDIT_SUFFIX      ":  Press Exit when done"   /* "Header A:  Press ..." */
#define IW_HF_NAMES            { "Header A", "Header B", "Footer A", "Footer B" }
#define IW_HF_SHORT            { "HA", "HB", "FA", "FB" }
#define IW_HF_OCC_LABEL_EVERY  "Every page"     /* label and Format: Page value */
#define IW_HF_OCC_LABEL_ODD    "Odd pages"
#define IW_HF_OCC_LABEL_EVEN   "Even pages"
#define IW_HF_DISC_LABEL       "Discontinue"

/* ---- footnotes and endnotes (Ctrl-F7) ---------------------------------------- *
 * FOOTNOTE `D6 00 <u16 size> | 00 | n u16 | 00 | h u16 | y u16 | off u16 |
 *          00 00 00 | X u16 | 8D | text | <u16 size> 00 D6` (MINT36 @0x14C,
 *          @0x174): n the note number (WP renumbers: Reference p. 210), h the
 *          note's height 200 a line, y where the line holding it is below the
 *          page's text top (MINT05 @0x156: 200, its line 2; 0 in MINT36/42),
 *          off where the note starts in the page's
 *          note area (0, then 600 for a second one-line note -- see
 *          IW_NOTE_OFF_* below), X as headers (81 C8 in all minted notes);
 *          0x8D is [Note Num].
 * ENDNOTE  `D6 01 <u16 size> | 00 | n u16 | 00 00 | X u16 | 8D | text |
 *          <u16 size> 01 D6` (MINT36 @0x19E).
 * Labels (MINT36 10_reveal): "[Footnote:1;[Note Num]First note.]",
 * "[Endnote:1;[Note Num]End note.]". On the editing screen the code shows as
 * its number in the text (05_doc "Alpha1"); on paper too ("Alpha1 beta2
 * gamma1." PRTMINT36 line 7).
 * THE PRINTED NOTES (PRTMINT36): footnotes at the bottom of their page --
 * the 2" separator line, a blank line, then each note with a blank line
 * before the next, the last one ending on the page's last text line; a note
 * prints "     " + its number + its text (the 0.5" tab of the default style,
 * shots_wpMINT36/04_note_typed "     1First note."); endnotes on a page of
 * their own after the document's last page, each "<n>." + its text
 * (07_endnote_edit "1.", PRTMINT36 line 73). The separator line is printed
 * by the Standard Printer as spaces to its right end, CR, spaces to its left
 * end and twenty '_' (PRTMINT36 line 56). */
#define IW_WP5_NOTE_GRP        0xD6u
#define IW_NOTE_FOOT           0u
#define IW_NOTE_END            1u
#define IW_WP5_NOTE_NUM        0x8Du
#define IW_RV_NOTE_NUM         "[Note Num]"
#define IW_FOOT_HEAD_LEN       15u     /* 00 n 00 h 00 00 off 00 00 00 X    */
#define IW_END_HEAD_LEN        7u      /* 00 n 00 00 X                       */
#define IW_NOTE_SEP_COLS       20u     /* the separator: 2" at 10 pitch      */
#define IW_NOTE_INDENT_COLS    5u
#define IW_NOTE_MENU           "1 Footnote; 2 Endnote; 3 Endnote Placement: 0"
#define IW_FOOT_MENU           "Footnote: 1 Create; 2 Edit; 3 New Number; 4 Options: 0"
#define IW_END_MENU            "Endnote: 1 Create; 2 Edit; 3 New Number; 4 Options: 0"
#define IW_FOOT_EDIT           "Footnote:  Press Exit when done"
#define IW_END_EDIT            "Endnote:  Press Exit when done"
#define IW_RV_FOOTNOTE         "[Footnote:"
#define IW_RV_ENDNOTE          "[Endnote:"

/* ---- newspaper columns (Alt-F7 1) ------------------------------------------- *
 * [Col Def] `D2 01 C6 00 | old 97 | new 97 | C6 00 01 D2` (MINT37 @0x147): a
 *          definition is the count of columns then 24 (left u16, right u16)
 *          pairs, zero past the count; old = the definition before it (all
 *          zero: none). The default for 1"/1" margins: 2 columns, 1"-4" and
 *          4.5"-7.5" (shots_wpMINT37/03_col_define: Type Newspaper, Number of
 *          Columns 2, the Margins table; Reference p. 77 "approximately
 *          one-half inch" between).
 * [Col On] 0x87 (MINT37 @0x23E-1).
 * A HARD PAGE between [Col On] and [Col Off] ends the column (Reference p.
 *          79): its D4/00 has payload[10] = the column count (MINT37 @0x23E
 *          `.. 02 00 ..`).
 * [Col Off] 0xAF, a 56-byte D4/00 before it (MINT37 @0x26D): the 28-byte
 *          group's fields for the column's line, payload[13] = 01, then 24
 *          bytes whose u16 at [26] is the longest column's height (400: two
 *          lines) and the rest constant [single instance; W-23].
 * Labels (MINT37 10_reveal): "[Col Def:Newspaper;2;1",4";4.5",7.5"]",
 * "[Col On]", "[Col Off]". The text after [Col Off] starts right below the
 * longest column (08_after_off "Ln 1.33""; PRTMINT37 "After." on line 9):
 * WordPerfect 5.1 has no column balancing (Reference p. 79 "When the text in
 * a newspaper column reaches the bottom of a page ... the text moves to the
 * top of the next column"). On paper the columns stand side by side
 * (PRTMINT37 line 7: column 2 at 4.5" = print column 45). */
#define IW_WP5_COLDEF_SUB      0x01u   /* D2/01 */
#define IW_WP5_COLDEF_GRP      0xD2u
#define IW_COLDEF_PAYLOAD      194u
#define IW_COLDEF_DEF_LEN      97u
#define IW_COLDEF_MAX          24u
#define IW_WP5_COL_ON          0x87u
#define IW_WP5_COL_OFF         0xAFu
#define IW_COLOFF_GRP_LEN      60u     /* the D4/00 before [Col Off]        */
#define IW_COLOFF_TAIL_INIT { 0x01, 0x1A, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, \
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x30, 0x2A, 0x30, 0x2A, 0xB0, 0x04, 0xB0, 0x04 }
#define IW_COLOFF_HEIGHT_OFF   26u     /* u16 in the payload                 */
#define IW_COL_GAP_WPU         600u    /* 4" -> 4.5" in the default          */
#define IW_COL_MENU            "Columns: 1 On; 2 Off; 3 Define: 0"
#define IW_COLTAB_MENU         "1 Columns; 2 Tables; 3 Math: 0"
#define IW_COLDEF_TITLE        "Text Column Definition"
#define IW_COLDEF_ROWS 8
#define IW_COLDEF_INIT { \
    {  0,  0, "Text Column Definition" }, \
    {  2,  5, "1 - Type" }, \
    {  4,  5, "2 - Number of Columns" }, \
    {  6,  5, "3 - Distance Between Columns" }, \
    {  8,  5, "4 - Margins" }, \
    { 10,  5, "Column" }, { 10, 15, "Left" }, { 10, 26, "Right" } \
}
#define IW_COLDEF_TYPE         "Newspaper"
#define IW_RV_COL_ON           "[Col On]"
#define IW_RV_COL_OFF          "[Col Off]"
#define IW_RV_COL_DEF          "[Col Def:Newspaper;"

/* ---- tables (Alt-F7 2) --------------------------------------------------------- *
 * [Tbl Def] `D2 0B <u16 size> | old 40 | 0A n 00 21 00 | B0 04 | 64 00 64 00
 *          78 00 | 4 x 00 | top u16 | 01 00 00 00 | n x width u16 | 2n x 00 |
 *          n x 20 | 00 00 | <u16 size> 0B D2` (MINT38 n 3 at the top, MINT40 n
 *          2 under a line: top = 200, MINT41 n 4); old = `00 0A 03 00 01 00
 *          00 00 64 00 64 00 78 00` + 26 x 00 in all three; width = (9000 -
 *          1200) / n (2600, 3900, 1950).
 * [Row]    `DC 01 <u16 size> | prev row info | row info | y u16 |
 *          <u16 size> 01 DC`; row info = `03 | h u16 | 80 + 2n | n cell
 *          words`; the first [Row]'s prev is its own info; y = the table's
 *          top for the first row, else the previous row's top (WPU from the
 *          top margin). A cell word (u16) packs its lines: bits 0-2 top,
 *          3-5 left, 8-10 bottom, 11-13 right; 1 single, 2 double, 0 none --
 *          the outside double, inside single, an inside bottom / right none
 *          (MINT38 / MINT40 every word) [verified: two documents, five rows;
 *          MINT41's one-row table has 0x8002 where the rule gives 0x020A in
 *          its second word -- W-24, the one minted divergence].
 *          h: 336 + 32 when the row's top is double (the first) + 48 when
 *          its bottom is (the last): 368 / 336 / 384, 416 for one row alone
 *          (MINT41).
 * [Cell]   `DC 00 0F 00 | 00 | col | 01 01 | y u16 | 4 x 00 | 20 | 0F 00 00 DC`;
 *          y = 0 in the first column, else the bottom of the cell's row (WPU
 *          from the top margin: MINT38 368 / 752, MINT40 568 / 904 / 1288).
 * [Tbl Off] `DC 02 <u16 size> | last row info | rows - 1 u16 | last row top
 *          u16 | 00 00 | n | <u16 size> 02 DC`.
 * Labels (MINT38 09_reveal): "[Tbl Def:I;3,2.17",2.17",2.17"]", "[Row]",
 * "[Cell]", "[Tbl Off]"; each [Row] starts a line of the pane.
 * CREATE: "Number of Columns: 3", "Number of Rows: 1" (shots_wpMINT38/02,
 * 03), then the Table Edit screen (04_table_edit), F7 to the document; Tab /
 * Shift-Tab to the next / previous cell (Reference p. 642).
 * ON PAPER (PRTMINT38/40/41, the Standard Printer prints no rules): see
 * "printing" below. ON THE SCREEN (shots_wpMINT38/04, 07; MINT40 03_after;
 * MINT41 01): the [Tbl Def] line is the top rule, each row's text one row
 * with a rule row under it (the last the bottom rule) -- double outside,
 * single inside (CP437 0xC9 0xCD 0xD1 0xBB / 0xBA 0xB3 / 0xC7 0xC4 0xC5 0xB6 /
 * 0xC8 0xCF 0xBC); a rule at screen column (x - 1200) / 120 of its edge x
 * (the right edge one column left); a cell's text from the column of
 * x = its left edge + 144 (Table Edit "Cell A1 ... Pos 1.12"", A1 at column
 * 1, B1 at 22, C1 at 44 in MINT38; yy at 33 in MINT40). The text after the
 * table's last row starts its height below its top (MINT40 03_after "End."
 * at Ln 2.07" = 1" + 200 + 1088 WPU). */
#define IW_WP5_TBL_SUB         0x0Bu   /* D2/0B */
#define IW_WP5_TROW_GRP        0xDCu
#define IW_WP5_TCELL_SUB       0x00u
#define IW_WP5_TROW_SUB        0x01u
#define IW_WP5_TOFF_SUB        0x02u
#define IW_TBL_OLD_INIT { 0x00, 0x0A, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x78, 0x00 }
#define IW_TBL_OLD_LEN         40u
#define IW_TBL_MAX_COLS        32u     /* Reference p. 641 "maximum of 32"   */
#define IW_TBL_ROW_H_BASE      336u
#define IW_TBL_ROW_H_TOP2      32u
#define IW_TBL_ROW_H_BOT2      48u
#define IW_TBL_SCREEN_DX       144u
#define IW_TBL_TEXT_DY         120u
#define IW_TBL_MENU            "Table: 1 Create; 2 Edit: 0"
#define IW_TBL_COLS_PROMPT     "Number of Columns: "
#define IW_TBL_ROWS_PROMPT     "Number of Rows: "
#define IW_TBL_EDIT_STATUS     "Table Edit:  Press Exit when done"
#define IW_TBL_EDIT_MENU1      "Ctrl-Arrows Column Widths; Ins Insert; Del Delete; Move Move/Copy;"
#define IW_TBL_EDIT_MENU2      "1 Size; 2 Format; 3 Lines; 4 Header; 5 Math; 6 Options; 7 Join; 8 Split: 0"
#define IW_RV_TBL_DEF          "[Tbl Def:I;"
#define IW_RV_ROW              "[Row]"
#define IW_RV_CELL             "[Cell]"
#define IW_RV_TBL_OFF          "[Tbl Off]"

/* ---- printing (Shift-F7) -------------------------------------------------------- *
 * THE STREAM of the Standard Printer (STANDARD.PRS, the one printer
 * definition the installed tree has; selected from List Printer Files,
 * shots_wpPRINT0/04..07) printed to a file (Port 8 Other, "Device or
 * Filename:", shots_wpPRINT1/01..04; Reference p. 543 "Printing to Disk"):
 * every page is "\r", then for each printed line L of the page (1..66) as
 * many "\r\n" as take the head from the line it is on to L, then the line --
 * spaces to the text's column (10 a inch from the paper's edge) and the
 * text; several texts on one line (Header A over Header B) joined by "\r"
 * and spaces; after the page's last printed line "\r" and "\r\n" down to
 * line 67, the next page's first. No form feed; nothing after the last page
 * (PRTMINT34: 362 bytes, 132 line feeds for two pages; PRTMINT39 page 1 the
 * 54 lines L01..L54 on lines 7..60). The page number of ^B is printed in
 * decimal (PRTMINT34 "Page 1", "Page 2"). */
/* WHERE A TEXT PRINTS: at print column floor((x + 72) / 120) of its paper
 * position x (WPU) -- exact for text on the 0.1" grid; fitted [inferred,
 * W-26] where WP places text off the grid: the cells of a table (x = the
 * cell's left edge + 100, the `64 00` cell margin: PRTMINT38/40/41, nine
 * cells) and the words of a FULLY JUSTIFIED line (the default
 * justification, Reference p. 307: the line's spare width spread evenly over
 * its spaces, a word at x + k x spare / spaces: PRTMINT42 line 57, eleven
 * words). A line is justified when it ends in a soft return and its
 * justification is Full; the spare width is the right margin position
 * (10200 - R) less where its last character ends, never below 0 (a 66-
 * character line ends 0.1" past it: PRTMINT44 line 59 printed as stored).
 * A table row prints its cells' text on the line round((row top + 120) /
 * 200) of the table (the `78 00` 0.1" margin), the text after the table on
 * round(height / 200) (PRTMINT38 "After." line 11, PRTMINT40 "End." 13).
 * The body prints the lines the editing screen shows (the stored soft
 * returns: PRTMINT44's 66-character line); a NOTE's text is wrapped again on
 * paper, a word fitting when it ENDS by the right margin position
 * (PRTMINT42 lines 57-58 break before "line", which the note screen kept on
 * the first line, 01_note_wrap.png) [inferred: W-25, one note]. */
#define IW_PRT_X_ROUND         72u
#define IW_TBL_CELL_MARGIN     100u
#define IW_PRT_CR              0x0Du
#define IW_PRT_LF              0x0Au
#define IW_PRT_COL_WPU         120u
#define IW_PRT_LINE_WPU        200u
#define IW_PRINT_TITLE         "Print"
#define IW_PRINT_ROWS 16
#define IW_PRINT_INIT { \
    {  0,  0, "Print" }, \
    {  2,  5, "1 - Full Document" }, \
    {  3,  5, "2 - Page" }, \
    {  4,  5, "3 - Document on Disk" }, \
    {  5,  5, "4 - Control Printer" }, \
    {  6,  5, "5 - Multiple Pages" }, \
    {  7,  5, "6 - View Document" }, \
    {  8,  5, "7 - Initialize Printer" }, \
    { 11,  0, "Options" }, \
    { 13,  5, "S - Select Printer" }, \
    { 14,  5, "B - Binding Offset" }, \
    { 15,  5, "N - Number of Copies" }, \
    { 16,  5, "U - Multiple Copies Generated by" }, \
    { 17,  5, "G - Graphics Quality" }, \
    { 18,  5, "T - Text Quality" }, \
    {  0,  0, "" } \
}
#define IW_PRINT_V_PRINTER     "Standard Printer"
#define IW_PRINT_V_BINDING     "0\""
#define IW_PRINT_V_COPIES      "1"
#define IW_PRINT_V_GENERATED   "WordPerfect"
#define IW_PRINT_V_GRAPHICS    "Medium"
#define IW_PRINT_V_TEXT        "High"
#define IW_PRINT_VALUE_COL     40u
/* Select Printer: Edit (shots_wpPRINT0/06_after_enter, PRINT1/01..04) */
#define IW_PEDIT_ROWS 12
#define IW_PEDIT_INIT { \
    {  0,  0, "Select Printer: Edit" }, \
    {  2,  9, "Filename" }, \
    {  4,  5, "1 - Name" }, \
    {  6,  5, "2 - Port" }, \
    {  8,  5, "3 - Sheet Feeder" }, \
    { 10,  5, "4 - Cartridges/Fonts/Print Wheels" }, \
    { 12,  5, "5 - Initial Base Font" }, \
    { 14,  5, "6 - Path for Downloadable" }, \
    { 15, 11, "Fonts and Printer" }, \
    { 16, 11, "Command Files" }, \
    { 18,  5, "7 - Print to Hardware Port" }, \
    {  0,  0, "" } \
}
#define IW_PEDIT_V_FILE        "STANDARD.PRS"
#define IW_PEDIT_V_FEEDER      "None"
#define IW_PEDIT_V_FONT        "Courier 10 Pitch"
#define IW_PEDIT_V_HWPORT      "No"
#define IW_PEDIT_V_LPT1        "LPT1:"
#define IW_PORT_MENU           "Port: 1 LPT 1; 2 LPT 2; 3 LPT 3; 4 COM 1; 5 COM 2; 6 COM 3; 7 COM 4; 8 Other: 0"
#define IW_PORT_FILE_PROMPT    "Device or Filename: "
#define IW_SELPRN_TITLE        "Print: Select Printer"
#define IW_SELPRN_MENU         "1 Select; 2 Additional Printers; 3 Edit; 4 Copy; 5 Delete; 6 Help; 7 Update: 1"
/* PC LOAD LETTER: InitechOS canon for a print-path failure (bead initech-
 * 8q6j acceptance; plan Sec 5 "Print" row, Sec 7 P4) -- authored, no WP
 * screen: a disk-launched FLAIR application has no device path to LPT1 (the
 * Toolbox Gate refuses DOS device names, spec/toolbox_gate.h Sec 10). */
#define IW_PRINT_FAIL          "PC LOAD LETTER"

#endif /* INITECH_SPEC_IWORD_WP5_PAGE_H */
