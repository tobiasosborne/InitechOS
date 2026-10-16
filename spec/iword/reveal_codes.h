/*
 * spec/iword/reveal_codes.h -- LOCKED spec-data: WordPerfect 5.1's Reveal
 * Codes screen (the code labels, the pane geometry, the ruler, the restore
 * message) and the Font menu lines, as InitechWord P2 slice 1 uses them
 * (CLAUDE.md Rule 8).
 *
 * bead: initech-jojk (IWORD P2, slice 1: attribute codes + Reveal Codes).
 *       Epic initech-fdxa. Plan: docs/plans/INITECH-WORD-plan.md Sec 6(d),
 *       Sec 7 P2.
 *
 * EVERY string below is TRANSCRIBED from a screen the real WP.EXE (5.1,
 * 03-09-92) drew under DOSBox-X (../wordperfect51-decomp):
 *   [R1..R5] goldens/minted/MINT01..05.shots/03_reveal.png (mint session 001);
 *   [R6]     mint/shots_wpMINT06/03_reveal.png, 05_reveal_top.png,
 *            01_font_menu.png, 01_appearance_menu.png (this bead's mint,
 *            2026-10-16, mint/wpMINT06.steps);
 *   [R7]     mint/shots_wpMINT07/03_reveal.png (mint/wpMINT07.steps);
 *   [R8]     mint/shots_wpMINT08/03a_reveal_before.png, 03b_reveal_bs1.png,
 *            03c_reveal_bs2.png, 03_reveal.png (mint/wpMINT08.steps).
 * The byte <-> label pairs are the corpus' table (specs/file-formats/wp5-
 * body-codes.md Sec 5) plus the three this bead's mints added ([ITALC]/
 * [italc], [SRt]). A code no shot labels gets IW_RV_UNKNOWN -- an AUTHORED
 * placeholder, never a guessed WordPerfect label (Law 1). A code the shots
 * show WITHOUT a label (0x83 end of center / flush right, R1/R3; the D4/00
 * group before a hard page, R3) gets the empty label.
 *
 * ASCII-clean (Rule 12).
 */
#ifndef INITECH_SPEC_IWORD_REVEAL_CODES_H
#define INITECH_SPEC_IWORD_REVEAL_CODES_H

/* ---- labels --------------------------------------------------------------- */
#define IW_RV_HRT        "[HRt]"        /* R1..R8                                 */
#define IW_RV_SRT        "[SRt]"        /* R6 03_reveal row 2 "e[SRt]" (MINT06 @0x1B5) */
#define IW_RV_HPG        "[HPg]"        /* R3                                     */
#define IW_RV_HSPACE     "[ ]"          /* R5 "hard[ ]space"                      */
#define IW_RV_CENTER     "[Center]"     /* R1, C1 sub 0xE0                        */
#define IW_RV_FLSH_RGT   "[Flsh Rgt]"   /* R3, C1 sub 0x60                        */
#define IW_RV_TAB        "[Tab]"        /* R1, C1 sub 0x02                        */
#define IW_RV_JUST_LEFT  "[Just:Left]"  /* R4, D0/06 new value 0                  */
/* P2 slice 3 (2026-10-16; ../wordperfect51-decomp mint/shots_wpMINT21..24,
 * re/mint-results-004.md): the other three justifications and the deletable
 * soft return. Full is the menu's word (MINT22 02_just_menu "4 Full") in the
 * label's form -- [inferred]: no Reveal shot shows a [Just:Full] code on the
 * pane (MINT22's scrolled off it). */
#define IW_RV_JUST_FULL   "[Just:Full]"
#define IW_RV_JUST_CENTER "[Just:Center]"  /* MINT22 05_reveal "[Just:Center]after" */
#define IW_RV_JUST_RIGHT  "[Just:Right]"   /* MINT24 02_reveal "[Just:Right]Right line." */
#define IW_RV_DSRT        "[DSRt]"         /* MINT22 05_reveal "Mid[DSRt]" (0x90)   */
#define IW_RV_LR_MAR     "[L/R Mar:"    /* R4 "[L/R Mar:2\",2\"]": D0/01, the NEW
                                         * left and right margins in inches       */
#define IW_RV_UNKNOWN    "[?]"          /* authored: no shot labels the code      */
/* P2 slice 2 (this bead; ../wordperfect51-decomp mint/shots_wpMINT09..15,
 * re/mint-results-003.md):                                                   */
#define IW_RV_INDENT     "[\x1A" "Indent]"  /* C2 (F4): "[->Indent]" with the CP437
                                         * right arrow 0x1A (MINT09 05_reveal_top
                                         * row 5 "[->Indent]Indented once ...")   */
#define IW_RV_BLOCK      "[Block]"      /* the block's anchor while Block is on,
                                         * not a code in the file (MINT13
                                         * 01b_block_reveal "word[Block]s[bold]") */
/* C6 (end of indent) and D4/00 (the hard page's bookkeeping) show NO label
 * (MINT09 05_reveal_top "the indent here.[HRt]", 07_reveal_pages
 * "Page one.[HPg]"). */

/* Attribute labels: upper case = the ON code, lower case = the OFF code
 * (R1 [BOLD]/[bold] [UND]/[und]; R4 [LARGE]/[large]; R6 [ITALC]/[italc]). */
#define IW_RV_ATTR_ROWS 4
#define IW_RV_ATTR_INIT { \
    { 0x02u, "[LARGE]", "[large]" }, \
    { 0x08u, "[ITALC]", "[italc]" }, \
    { 0x0Cu, "[BOLD]",  "[bold]"  }, \
    { 0x0Eu, "[UND]",   "[und]"   }  \
}

/* ---- the pane -------------------------------------------------------------- *
 * WordPerfect's 25-line screen in Reveal Codes (R1..R8): the document on lines
 * 0..10, the status line on line 11, the ruler on line 12, the CODES PANE on
 * lines 13..22 (10 rows; line 23 stays blank) and the restore message on line
 * 24. CORRECTED in P2 slice 2 (Rule 8, deliberate; bead initech-jojk): slice 1
 * had 11 rows, inferred from screens whose 11th row was empty anyway. The
 * first screens with more to show keep line 23 blank: mint/shots_wpMINT09/
 * 05_reveal_top.png (lines 0..9 shown, "Page three.[HRt]" -- line 10 -- not on
 * line 23), mint/shots_wpMINT16/01_reveal_top.png (10 rows from line 0, one
 * line taking two rows, "L9[HRt]" not shown) and 02_reveal_line6.png (L3..L12).
 * PANE ROWS. Each document line's labels run on from the row before it ends;
 * a line longer than the screen is cut at column 80 wherever that falls, even
 * inside a label (R6 03_reveal: "...enough that th" | "e[SRt]").
 * ANCHOR. The pane's first row is the start of the document line 3 lines above
 * the cursor's line, or the first line when fewer precede it (cursor line 6 ->
 * pane from line 3: R1, R3; line 4 -> line 1: R6 03_reveal; lines 0..3 -> line
 * 0: R2, R4, R5, R6 05_reveal_top, R7, R8) [inferred from those 10 screens, all
 * taken on entering Reveal Codes or right after a move/delete in it].
 * THE CURSOR in the pane is the reverse-video run over the whole label of the
 * unit at the cursor ([HRt] in R6 05_reveal_top), over the character at it, or
 * one cell after the last label when the cursor is at the end of the document
 * (R2 "Plain text only." + the cell at column 16; R8 03a column 19). */
#define IW_RV_WP_COLS     80u
#define IW_RV_PANE_ROWS   10u
#define IW_RV_PANE_ABOVE  3u
#define IW_RV_MESSAGE     "Press Reveal Codes to restore screen"   /* R1..R8 line 24 */

/* ---- the ruler (R1..R8 line 12): '{' at the left margin, a tab mark every
 * 5 columns (the default tabs every 0.5" at 10 pitch), '}' at the right
 * margin -- at the default 1"/1" margins '{' sits in column 0 and '}' in
 * column 65 (R1: x 2 and x 588 on the 9-pixel cells), tab marks at 5..60 and
 * 70, 75. The mark is the up triangle, code page 437 0x1E. */
#define IW_RV_RULER_LEFT   '{'
#define IW_RV_RULER_RIGHT  '}'
#define IW_RV_RULER_TAB    0x1E
#define IW_RV_RULER_EVERY  5u
/* Slice 3 (2026-10-16, RULE 8 stated: the ruler follows the margins at the
 * cursor). Reference Manual, Margins Left and Right p. 394: "the ruler ...
 * displays the left margin as a left bracket ([) and the right margin as a
 * right bracket (]). If a margin falls on a tab setting, it is displayed as a
 * left brace ({) or a right brace (})". The tabs are relative to the left
 * margin, every 0.5" from -1" (MINT21 02_format_line "Rel; -1", every
 * 0.5""), so the left margin is always on one: MINT21 09_reveal '{' at
 * column 3 under 1.3", '}' at 58; MINT23 03_reveal_mid '{' at 2 under 1.25",
 * marks at 7, 12, .., 62, ']' at 63 (8820 WPU is no stop), marks on to the
 * right edge; MINT04 '{' at 10, marks at 0 and 5 left of it. A mark sits at
 * column (L + 600 k - 1200) / 120 rounded down, k >= -2. */
#define IW_RV_RULER_LBRACK '['
#define IW_RV_RULER_RBRACK ']'

/* ---- the Format menus (Shift-F8; slice 3) -----------------------------------
 * Transcribed from mint/shots_wpMINT21/01_format_menu.png and
 * 02_format_line.png (80 x 25 screens: each row is (screen row, column,
 * text); the 9-pixel cells give the columns). WP's screen has 25 rows,
 * Face A's window 24 (os/iword/app/face_a.c): the Format screen drops its
 * blank row 1 and Format: Line its blank row 22 [authored adaptation; no
 * text is lost]. "Selection: 0" is on WP's last row. */
#define IW_FMT_ROWS 34
#define IW_FMT_INIT { \
    {  0,  0, "Format" }, \
    {  2,  5, "1 - Line" }, \
    {  3, 14, "Hyphenation" },              {  3, 45, "Line Spacing" }, \
    {  4, 14, "Justification" },            {  4, 45, "Margins Left/Right" }, \
    {  5, 14, "Line Height" },              {  5, 45, "Tab Set" }, \
    {  6, 14, "Line Numbering" },           {  6, 45, "Widow/Orphan Protection" }, \
    {  8,  5, "2 - Page" }, \
    {  9, 14, "Center Page (top to bottom)" }, {  9, 45, "Page Numbering" }, \
    { 10, 14, "Force Odd/Even Page" },      { 10, 45, "Paper Size/Type/Labels" }, \
    { 11, 14, "Headers and Footers" },      { 11, 45, "Suppress" }, \
    { 12, 14, "Margins Top/Bottom" }, \
    { 14,  5, "3 - Document" }, \
    { 15, 14, "Display Pitch" },            { 15, 45, "Redline Method" }, \
    { 16, 14, "Initial Codes/Font" },       { 16, 45, "Summary" }, \
    { 18,  5, "4 - Other" }, \
    { 19, 14, "Advance" },                  { 19, 45, "Printer Functions" }, \
    { 20, 14, "Conditional End of Page" },  { 20, 45, "Underline Spaces/Tabs" }, \
    { 21, 14, "Decimal Characters" },       { 21, 45, "Border Options" }, \
    { 22, 14, "Language" },                 { 22, 45, "End Centering/Alignment" }, \
    { 23, 14, "Overstrike" } \
}
#define IW_FMT_LINE_ROWS 14
#define IW_FMT_LINE_INIT { \
    {  0,  0, "Format: Line" }, \
    {  2,  5, "1 - Hyphenation" }, \
    {  4,  5, "2 - Hyphenation Zone -  Left" }, \
    {  5, 28, "Right" }, \
    {  7,  5, "3 - Justification" }, \
    {  9,  5, "4 - Line Height" }, \
    { 11,  5, "5 - Line Numbering" }, \
    { 13,  5, "6 - Line Spacing" }, \
    { 15,  5, "7 - Margins - Left" }, \
    { 16, 19, "Right" }, \
    { 18,  5, "8 - Tab Set" }, \
    { 20,  5, "9 - Widow/Orphan Protection" }, \
    {  0,  0, "" }, \
    {  0,  0, "" } \
}
/* The values, at column 40 (02_format_line): the defaults of the fresh
 * install; Justification, Margins and (P2: unchanged) Line Spacing follow the
 * format at the cursor (06_margins_set "1.3"" / "1.7""). */
#define IW_FMT_VALUE_COL   40u
#define IW_FMT_V_HYPH      "No"
#define IW_FMT_V_ZONE_L    "10%"
#define IW_FMT_V_ZONE_R    "4%"
#define IW_FMT_V_HEIGHT    "Auto"
#define IW_FMT_V_NUMBER    "No"
#define IW_FMT_V_SPACING   "1"
#define IW_FMT_V_TABS      "Rel; -1\", every 0.5\""
#define IW_FMT_V_WIDOW     "No"
#define IW_FMT_JUST_NAMES  { "Left", "Full", "Center", "Right" }
#define IW_FMT_SELECTION   "Selection: "            /* + the digit, "0" at rest */
#define IW_FMT_JUST_PROMPT "Justification: 1 Left; 2 Center; 3 Right; 4 Full: 0"
                                                    /* MINT22 02_just_menu      */

/* ---- the Font menu (Ctrl-F8), on the status line (R6) ---------------------- */
#define IW_FONT_MENU    "1 Size; 2 Appearance; 3 Normal; 4 Base Font; 5 Print Color: 0"
#define IW_APPEAR_MENU  "1 Bold 2 Undln 3 Dbl Und 4 Italc 5 Outln 6 Shadw 7 Sm Cap 8 Redln 9 Stkout: 0"

/* ---- block and code prompts, on the status line (P2 slice 2) -------------
 * Each transcribed from a real WP 5.1 screen (mint/shots_wpMINT11..13):    */
#define IW_BLOCK_ON      "Block on"     /* MINT12 01_block_on: left of the status
                                         * line while Block is on (WP blinks it) */
#define IW_MOVE_MENU     "Move: 1 Block; 2 Tabular Column; 3 Rectangle: 0"
                                        /* MINT12 03_move_menu (Ctrl-F4, Block on) */
#define IW_MOVE_BLK_MENU "1 Move; 2 Copy; 3 Delete; 4 Append: 0"
                                        /* MINT12 04_block_menu (then 1 Block)  */
#define IW_MOVE_RETRIEVE "Move cursor; press Enter to retrieve."
                                        /* MINT12 05_move_prompt, 07_copy_prompt:
                                         * left of the status line, Doc .. Pos
                                         * kept at its right; "Enter" bright      */
#define IW_MOVE_ENTER_COL 19u           /* "Enter" starts at column 19          */
#define IW_DEL_BLOCK     "Delete Block? No (Yes)"   /* MINT12 09_delete_prompt  */
#define IW_DEL_CODE_PRE  "Delete "      /* MINT11 01_bs_prompt "Delete [BOLD]? No */
#define IW_DEL_CODE_POST "? No (Yes)"   /* (Yes)", 02_del_prompt "Delete [bold]? */
                                        /* No (Yes)": the code's Reveal label   */

#endif /* INITECH_SPEC_IWORD_REVEAL_CODES_H */
