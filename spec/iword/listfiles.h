/*
 * spec/iword/listfiles.h -- LOCKED spec-data (CLAUDE.md Rule 8): InitechWord's
 * List Files (F5) and Look screens. bead initech-qvgd (IWORD P3 remainder).
 *
 * SOURCE (Law 1): the REAL WordPerfect 5.1 (03-09-92) run under DOSBox-X on a
 * fixed FAT12 volume -- ../wordperfect51-decomp mint/work/LISTF/
 * mint-results-005.md, its cell-exact transcriptions grids/<session>_<shot>.txt
 * (every glyph matched exactly against DOSBox-X's own font atlas; md5s in
 * MD5SUMS). Each value below cites the grid it was read from. The Reference
 * (initech-manuals-ref/wordperfect51/WPDOS51Reference.pdf) PDF pp. 364-370
 * (List Files) and 381-384 (Look) supplied the keys the mint then exercised.
 *
 * THE WP SCREEN is 80 x 25. Every WP row below is COMPOSED AT ITS WP COLUMNS
 * (IW_LF_WP_COLS wide); the face then shows it in its 76 x 24 cells
 * (os/iword/app/face_a.c SCR_COLS / SCR_LINES) by DELETING the columns the
 * IW_LF_DEL_* lists name -- columns that are blank in every row of that kind
 * on the real screen, so no WP cell is lost -- and by dropping the list
 * area's last row (WP rows 3..22 -> face rows 3..21) and the Look body's last
 * row (WP 3..23 -> face 3..22); the menu / prompt rows 23..24 -> 22..23.
 * That adaptation is AUTHORED (stated, the tdnl.51 / jojk slice-3 pattern for
 * the 76-cell field), not a WordPerfect fact.
 * Colours (authored, the Toolbox has four tokens): WP's red bands (header rows
 * 0-1, the Look header, the selection bar) are BLACK on GRAY as the Reveal
 * Codes ruler and the Block band are; WP's bright white cells are drawn bold;
 * a List Files menu item InitechWord does not perform is drawn in GRAY (the
 * period-grayed convention of spec/iword/fkeys.json) and its key is ignored.
 *
 * ASCII-clean (Rule 12): the vertical bar is the CP437 byte 0xB3 written as
 * an escape, as WP draws it (grids LF11_06 col 40).
 */
#ifndef IWORD_LISTFILES_H
#define IWORD_LISTFILES_H

#define IW_LF_WP_COLS        80u

/* ---- the F5 prompt (grids LF11_01_dir_prompt_root, LF03_01_dir_prompt):
 * row 24 "Dir A:\*.*", 'Dir' and the path bright, the cursor on the path's
 * first cell; the hint grey at col 49 with '=' bright. */
#define IW_LF_DIR_PROMPT     "Dir "
#define IW_LF_PATH_COL       4u
#define IW_LF_HINT           "(Type = to change default Dir)"
#define IW_LF_HINT_COL       49u
#define IW_LF_HINT_BRIGHT    6u            /* '=' at 49 + 6 = 55 */
#define IW_LF_DRIVE          "A:\\"        /* the data volume (face_a.c) */
#define IW_LF_ALL            "*.*"

/* ---- header row 0 (red band; LF11_06 row 0, LF11_02 row 0) */
#define IW_LF_DATE_COL       0u            /* "03-01-92"  MM-DD-YY        */
#define IW_LF_TIME_COL       10u           /* "09:00a"    HH:MM a|p       */
#define IW_LF_DIR_COL        28u           /* "Directory A:\DOCS\*.*"     */
#define IW_LF_DIR_LABEL      "Directory "
/* ---- header row 1 (red band; LF11_06 row 1): labels at their columns,
 * values comma-grouped and right-aligned to end at the *_END column. */
#define IW_LF_DOCSIZE_LABEL  "Document size:"
#define IW_LF_DOCSIZE_COL    0u
#define IW_LF_DOCSIZE_END    22u
#define IW_LF_FREE_LABEL     "Free:"
#define IW_LF_FREE_COL       26u
#define IW_LF_FREE_END       41u
#define IW_LF_USED_LABEL     "Used:"
#define IW_LF_USED_COL       43u
#define IW_LF_USED_END       58u
#define IW_LF_FILES_LABEL    "Files:"
#define IW_LF_FILES_COL      64u
#define IW_LF_FILES_END      77u
/* "Document size:" is WP's in-memory figure, NOT the file size (ledger
 * "Document size -- NOT resolved"): 0 for an empty document (every list from a
 * fresh WP); a retrieved, unmodified document shows its file size + 193 in all
 * three minted cases (MEMO 495 -> 688, LETTER.WP5 483 -> 676, TABS.DOC 627 ->
 * 820; LF04_10, LF05_01, LF12_02). [inferred] InitechWord shows 0 for the
 * empty document and the bytes F10 would write + 193 otherwise; a typed, never
 * retrieved document differs in WP (LF05_03 264 for "abc") -- an open GAP. */
#define IW_LF_DOCSIZE_EXTRA  193u

/* ---- the list (LF11_06 rows 3..8): two entries per row, row-major; slot 0
 * ".  Current", slot 1 "..  Parent", then the directories alphabetically,
 * then the files alphabetically (ledger "Entries"; Reference PDF p. 365 Sort
 * List Files default). Hidden files are listed; Files counts files only; Used
 * is the sum of the listed files' sizes (137,424 = ... + HIDDEN.SYS 2). */
#define IW_LF_LIST_ROW       3u            /* the first list row            */
#define IW_LF_LEAD_LEFT      1u            /* an entry's first cell          */
#define IW_LF_LEAD_RIGHT     42u
#define IW_LF_BAR_COL        40u           /* only on rows with a right entry
                                            * (LF01-era row 8 / LF11_02 row 5) */
#define IW_LF_BAR_CHAR       '\xB3'
#define IW_LF_ENTRY_W        38u
#define IW_LF_E_DOT          8u            /* '.' after the 8-cell name      */
#define IW_LF_E_EXT          9u            /* the 3-cell extension            */
#define IW_LF_E_SIZE_END     20u           /* size right-aligned, 9 cells     */
#define IW_LF_E_DIR          16u           /* "<Dir>" in the size's last 5    */
#define IW_LF_E_DATE         23u           /* MM-DD-YY                        */
#define IW_LF_E_TIME         32u           /* HH:MMa; a zero DOS date word:
                                            * no date and no time (LF14_02
                                            * DESKTOP.DB / TRASH)             */
#define IW_LF_DIR_TAG        "<Dir>"
#define IW_LF_CURRENT        "Current"     /* '.' at +0, the word at +5       */
#define IW_LF_PARENT         "Parent"      /* '..' at +0, the word at +5      */
#define IW_LF_E_WORD         5u
/* the selection bar covers the entry's lead blank + its 38 cells
 * (LF11_08_letter_sel: WP cols 41..79; LF03_03_archive: 0..38) */
#define IW_LF_SEL_LEFT0      0u
#define IW_LF_SEL_RIGHT0     41u
#define IW_LF_SEL_W          39u
/* the find attribute: hidden + system + directory (DOS 3.3 PRM AH=4Eh) */
#define IW_LF_FIND_ATTR      0x16u
#define IW_LF_ATTR_DIR       0x10u

/* ---- the menu (LF11_06 rows 23-24); B marks a bright cell. */
#define IW_LF_MENU1          "1 Retrieve; 2 Delete; 3 Move/Rename; 4 Print; 5 Short/Long Display;"
#define IW_LF_MENU1_BRIGHT   "B B         B B       B B            B B      B B"
#define IW_LF_MENU2          "6 Look; 7 Other Directory; 8 Copy; 9 Find; N Name Search: 6"
#define IW_LF_MENU2_BRIGHT   "B B     B B                B B     B B     B B            B"
/* The period-gray table (Rule 8 note, initech-qvgd): {key, menu row 1|2,
 * first col, last col, live}. 1 Retrieve and 6 Look are performed; the rest
 * are drawn GRAY and their keys ignored. */
#define IW_LF_ITEMS { \
    { '1', 1u,  0u, 10u, 1u }, { '2', 1u, 12u, 20u, 0u }, { '3', 1u, 22u, 35u, 0u }, \
    { '4', 1u, 37u, 44u, 0u }, { '5', 1u, 46u, 66u, 0u }, { '6', 2u,  0u,  6u, 1u }, \
    { '7', 2u,  8u, 25u, 0u }, { '8', 2u, 27u, 33u, 0u }, { '9', 2u, 35u, 41u, 0u }, \
    { 'N', 2u, 43u, 56u, 0u } }
#define IW_LF_N_ITEMS        10u
/* Retrieve into a non-empty document (LF12_03_into_q): row 23 blank, this on
 * row 24; y retrieves at the document cursor, n returns to the list. */
#define IW_LF_INTO_Q         "Retrieve into current document? No (Yes)"
#define IW_LF_INTO_Q_BRIGHT  "BBBBBBBB BBBB BBBBBBB BBBBBBBBB B   B"   /* LF13_22 row 24 */

/* ---- Look (LF11_09_look_letter, LF04_03, LF04_06) */
#define IW_LF_LOOK_FILE      "File: "
#define IW_LF_LOOK_WP51      "WP5.1"
#define IW_LF_LOOK_WP51_COL  45u
#define IW_LF_LOOK_REVISED   "Revised: "   /* + date + ' ' + time, ending at col 79 */
#define IW_LF_LOOK_BODY_ROW  3u
#define IW_LF_LOOK_MENU      "Look: 1 Next Doc; 2 Prev Doc: 0"
#define IW_LF_LOOK_BRIGHT    "BBBBB B B         B B         B"

/* ---- the face's column deletes (AUTHORED adaptation, see above). Each list
 * names WP columns blank in every row of its kind on the real screen:
 *   header row 1  59, 60      two of the five blanks before "Files:"
 *   list rows      0, 22, 39, 63   the left lead blank, the second blank of
 *                              each entry's size/date gap, the blank before
 *                              the bar
 *   F5 prompt     46, 47, 48  blanks before the hint (paths <= 42 cells)
 *   Look header   50..53      blanks between "WP5.1" and "Revised:"
 * Header row 0, the menu rows and the Look body are shown as they are, cut at
 * the face's 76th cell. */
#define IW_LF_DEL_HDR1       { 59u, 60u }
#define IW_LF_DEL_LIST       { 0u, 22u, 39u, 63u }
#define IW_LF_DEL_PROMPT     { 46u, 47u, 48u }
#define IW_LF_DEL_LOOK       { 50u, 51u, 52u, 53u }
#define IW_LF_FACE_LIST_ROWS 19u           /* face rows 3..21                */
#define IW_LF_FACE_LOOK_ROWS 20u           /* face rows 3..22                */

#endif /* IWORD_LISTFILES_H */
