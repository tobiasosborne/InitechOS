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
