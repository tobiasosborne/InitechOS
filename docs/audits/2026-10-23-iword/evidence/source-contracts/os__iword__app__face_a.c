1: /*
2:  * os/iword/app/face_a.c -- WORD.EXE, InitechWord's Face A: the WordPerfect
3:  * 5.1 editing screen, drawn natively in a FLAIR window by a disk-launched
4:  * application (THE ARTIFACT).
5:  *
6:  * bead: initech-l4tj (IWORD P1, Face A skeleton); initech-jojk (P2 slice 1:
7:  *       attribute codes + Reveal Codes; slice 2: tabs, indents, hard pages,
8:  *       Block / Move, the "Delete [code]?" question). Epic initech-fdxa.
9:  * Plan: docs/plans/INITECH-WORD-plan.md Sec 4 Face A (one native FlairApp
10:  *       window, the WP 5.1 surface rendered NATIVELY in fixed character
11:  *       cells; ADR-0013 Sec 4: a native tenant, never a text console) and
12:  *       Sec 7 P1. Packaging: the C disk-application path Initech 123 opened
13:  *       (os/apps/tbx, Makefile tenant-exe-rules: an InitechMZ flat-32 .EXE
14:  *       through the INT 81h Toolbox Gate; WL-0094/WL-0095).
15:  *
16:  * THE SCREEN, laid out from the real WordPerfect 5.1 (../wordperfect51-
17:  * decomp/goldens/minted/MINT0x.shots, an 80 x 25 text screen): the editing
18:  * area from line 0, text from column 0 (the 1" left margin is the screen's
19:  * left edge), and the STATUS LINE on the last line -- at its left the
20:  * document's name once it has one ("C:\WP51\MINT01", MINT01.shots/
21:  * 04_saved.png), at its right "Doc 1 Pg 1 Ln 1" Pos 1"": the "Doc .. Ln"
22:  * group RIGHT-aligned to end at column 67 and "Pos" at column 69 whatever
23:  * the widths (01_edit "Ln 1"" starts at column 52, MINT02 02_typed "Ln
24:  * 1.17"" at 49; "Pos" at 69 in both). Caps Lock shows "POS" (Reference
25:  * Manual, Caps Lock "Status Line", PDF 74). The window is 76 x 24 cells
26:  * (FLAIR_SCREEN 640 x 480 less the menu bands and the frame), so 23 lines
27:  * of text show where the DOS screen shows 24; the 65-column line (1"/1"
28:  * margins at 10 pitch, spec/iword/wp5_format.h) and the whole status line
29:  * fit. WordPerfect's blue field is the Toolbox's NAVY with WHITE text
30:  * (spec/toolbox_gate.h Sec 7 tokens); the cursor is the cell in reverse
31:  * video (WP's is the hardware underline; a cell API has no underline --
32:  * authored adaptation). A hard page shows as a row of '=' BELOW its line,
33:  * a screen row of its own that is no document line (MINT03.shots/
34:  * 02_typed.png: the empty hard-page line, then the bar; ../wordperfect51-
35:  * decomp mint/shots_wpMINT09/08_edit_top.png: "Page one." then the bar).
36:  *
37:  * KEYS. Typing, Enter (hard return), Backspace, Delete, the arrows, End (the
38:  * end of the line) and WP's HOME PREFIX (slice 3, below). Word wrap as
39:  * WordPerfect stores it (os/iword/core/edit.c).
40:  * ATTRIBUTE CODES (bead initech-jojk, P2 slice 1; os/iword/core/codes.c):
41:  *   F6 Bold, F8 Underline -- "both an on and off code are inserted, with the
42:  *                   cursor resting just before the off code"; the key again
43:  *                   moves past it (Reference Manual, Attributes p. 36-37).
44:  *   Ctrl-F8 Font    the Font menu on the status line, then 1 Size (all
45:  *                   seven), 2 Appearance (all nine) or 3 Normal; the menu
46:  *                   lines and the sixteen attribute numbers are the real WP
47:  *                   5.1's (spec/iword/reveal_codes.h; slice 3, MINT28). Base
48:  *                   Font and Print Color (their own screens) return to the
49:  *                   document.
50:  *   Alt-F3 Reveal   the split screen: the document, the status line, the
51:  *   Codes           ruler, the codes pane (every code by its label, the
52:  *                   cursor's unit highlighted; 10 rows, the 11th blank as
53:  *                   WP leaves it, MINT16) and "Press Reveal Codes to restore
54:  *                   screen". In Reveal Codes, Backspace / Delete on an
55:  *                   attribute code delete it and its partner (MINT08); on the
56:  *                   editing screen WP asks "Delete [BOLD]? No (Yes)" (the
57:  *                   code's label; MINT11 01_bs_prompt / 02_del_prompt): y
58:  *                   deletes the pair, any other key steps over the code.
59:  * SLICE 2 (bead initech-jojk; the real WP.EXE in ../wordperfect51-decomp
60:  * mint/wpMINT09..16.steps, re/mint-results-003.md; os/iword/core/block.c):
61:  *   Tab, F4         [Tab] / [->Indent]: to the next tab stop; an indent
62:  *                   holds the rest of its paragraph at that margin (MINT09).
63:  *   Ctrl-Enter      a hard page: the bar below its line (MINT09).
64:  *   Alt-F4 Block    on / off; "Block on" at the left of the status line
65:  *                   (MINT12 01_block_on; WP blinks it, drawn steady here),
66:  *                   the block from the anchor to the cursor highlighted
67:  *                   (WP's red band; the Toolbox has GRAY: BLACK on GRAY,
68:  *                   authored), the cursor cell outside it. While Block is on
69:  *                   only the cursor keys, Alt-F4, Ctrl-F4, Delete /
70:  *                   Backspace, Alt-F3 and F1 / Esc (Block off) act.
71:  *   Ctrl-F4 Move    with Block on: "Move: 1 Block; 2 Tabular Column; 3
72:  *                   Rectangle: 0", then 1: "1 Move; 2 Copy; 3 Delete; 4
73:  *                   Append: 0" (MINT12 03/04). 1 Move cuts and 2 Copy copies
74:  *                   the block, then "Move cursor; press Enter to retrieve."
75:  *                   (05_move_prompt, with Doc .. Pos at the right): the
76:  *                   cursor keys move, Enter puts it in front of the cursor;
77:  *                   3 Delete deletes it. Tabular Column, Rectangle and
78:  *                   Append are later (any other key: back).
79:  *   Delete / Bksp   with Block on: "Delete Block? No (Yes)" (09_delete_prompt):
80:  *                   y deletes it, any other key keeps it.
81:  * SLICE 3 (bead initech-jojk; the real WP.EXE in ../wordperfect51-decomp
82:  * mint/wpMINT21..24.steps, re/mint-results-004.md):
83:  *   Shift-F8 Format the Format screen (mint/shots_wpMINT21/01_format_menu),
84:  *                   1 Line: the Format: Line screen (02_format_line) with the
85:  *                   values at the cursor; 3 Justification "1 Left; 2 Center;
86:  *                   3 Right; 4 Full: 0" on the bottom row (MINT22
87:  *                   02_just_menu); 7 Margins: the Left then the Right field
88:  *                   take a measurement in inches (04_left_typed "1.3_",
89:  *                   05_right_field), Enter after each; F7 back to the
90:  *                   document, Enter / 0 back to Format, Enter / 0 there to
91:  *                   the document. The other selections are later slices
92:  *                   (the key is ignored). Each setting goes in as WP puts
93:  *                   it (os/iword/core/edit.c iw_ed_margins / iw_ed_just).
94:  *   The document sits on the paper as WP shows it: screen column 0 is 1"
95:  *                   whatever the margins (MINT21 text at column 3 under
96:  *                   1.3"), a centered / right-justified line where the
97:  *                   justification puts it (MINT22 04_typed), Pos the paper
98:  *                   position (MINT23 "Pos 1.25""); the Reveal Codes ruler
99:  *                   follows the margins at the cursor (spec/iword/
100:  *                   reveal_codes.h "the ruler").
101:  *   6 Line Spacing  a number, Enter (MINT25 01/02, MINT30): [Ln Spacing:]
102:  *                   at the cursor. A line takes its spacing rounded to the
103:  *                   nearest whole row, the extra rows blank below it, and the
104:  *                   screen scrolls by rows (MINT25 03_typed / 04_reveal,
105:  *                   MINT26, MINT30); Ln counts the spacing (MINT25 2.83").
106:  *   Home            a PREFIX (Reference Manual p. 103-104; MINT27): Home,
107:  *                   Left / Home, Home, Left the line's first character past
108:  *                   its codes, Home x3, Left before them, Home, Right the
109:  *                   line's end, Home, Up / Home, Down the screen's top /
110:  *                   bottom line, Home, Home, Up / Down the document's top
111:  *                   (after its formatting codes; x3 before them) / end.
112:  *   Block + F6, F8, Ctrl-F8 ("Attribute: 1 Size; 2 Appearance: 0")
113:  *                   the attribute given to the block (p. 37; MINT28).
114:  *   Ctrl-F4         with Block off: "Move: 1 Sentence; 2 Paragraph; 3 Page;
115:  *                   4 Retrieve: 0" -- 1..3 highlight the text and offer the
116:  *                   block's menu (MINT31), 4 "Retrieve: 1 Block; ..." puts
117:  *                   the last moved / copied text back (MINT29 14, MINT31 15).
118:  * The text between the codes is drawn in its face through DRAWCELLS (bold,
119:  * italic, underline synthesized by the Font Manager rules, spec/
120:  * toolbox_gate.h Sec 7a STYLE). WordPerfect itself shows attributes as
121:  * colours on a colour text screen (MINT01.shots/02_typed.png: bold bright
122:  * white, underline on grey; MINT06: italic yellow); the four Toolbox colour
123:  * tokens cannot, so Face A draws the face itself -- authored adaptation. The
124:  * ruler band is BLACK on GRAY where WP's is red (same reason).
125:  *   F10 Save        "Document to be Saved: " (Reference Manual, Exit p. 195,
126:  *                   PDF 213; Save p. 564, PDF 582): the current name is
127:  *                   offered, a character typed first replaces it (Save p.
128:  *                   565, PDF 583), Enter saves; an existing file asks
129:  *                   "Replace A:\NAME? No (Yes)" (the prompt form of Macros,
130:  *                   PDF 399, "Replace YOURMAC.WPM? No (Yes)"): y replaces,
131:  *                   any other key asks for a name again (Save p. 565).
132:  *   Shift-F10       "Document to be Retrieved: " (Directories, PDF 137):
133:  *   Retrieve        "The document appears on the screen at the cursor"
134:  *                   (Retrieve p. 553, PDF 571) -- onto a clear screen it
135:  *                   becomes the document (its own prefix kept, so it saves
136:  *                   back byte for byte); into a document, its text is
137:  *                   inserted at the cursor. A missing file: "ERROR: File Not
138:  *                   Found" (p. 554, PDF 572); not a 5.1 document: "ERROR:
139:  *                   Incompatible File Format" (Appendix E, PDF 767; the
140:  *                   "ERROR: " prefix as the File Not Found message shows it).
141:  *   F7 Exit         THE ONE EXIT GUARD (bead initech-fvnj pattern), see below.
142:  *   F1 Cancel       backs out of any prompt to the document (Exit p. 193,
143:  *                   PDF 211: "press Cancel (F1) at the Save or Exit prompts
144:  *                   to return to the document"); Esc does the same
145:  *                   (authored convenience).
146:  * Drive "A:\" in the names shown stands for the data volume (as 123.EXE
147:  * shows "A:\*.wk1"); WordPerfect adds no extension (MINT01 was saved as
148:  * "MINT01", 04_saved.png).
149:  *
150:  * THE EXIT GUARD. Every way out -- F7, File > Exit (Ctrl-Q) and the close
151:  * box (TBX_GOAWAY_ASK) -- comes to request_exit(). F7 always asks, as WP
152:  * does (Exit p. 193: "Whenever you exit a WordPerfect document, you are
153:  * always asked if you want to save"); the Macintosh routes end at once when
154:  * nothing is unsaved and otherwise ask the same questions. The questions are
155:  * WordPerfect's own, on the status line (A Brief Lesson p. 21, PDF 38):
156:  * "Save Document? Yes (No)" -- with "(Text was not modified)" at the right
157:  * end when nothing changed (Exit p. 193) -- y / Enter (the default, "the
158:  * option not in parentheses") saves through the F10 flow, n goes on; then
159:  * "Exit WP? No (Yes)": y ends InitechWord, n / Enter clears the screen for a
160:  * new document (Exit p. 193 step 4). F1 / Esc at either keeps the document.
161:  *
162:  * Messages not in the manual (the size refusals) are authored, no local
163:  * reference. Freestanding C, ASCII-clean (Rule 12), deterministic (Rule 11).
164:  */
165: #include "tbx.h"
166: #include "../../../spec/iword/fkeys.h"
167: #include "iword.h"
168: #include "iword/wp5_format.h"
169: #include "iword/reveal_codes.h"
170: #include "iword/settings.h"
171: #include "iword/listfiles.h"
172: #include "cpanels_record.h"           /* cp_time_t: the TBX_CLOCK record */
173: 
174: const char tbx_app_name[] = "WORD";
175: 
176: /* ---- geometry (the 123.EXE window, os/i123/app/face_a.c) ------------------ */
177: #define WIN_L        0
178: #define WIN_T        40
179: #define WIN_R        640
180: #define WIN_B        480
181: #define CW           8           /* TBX_CELL_W */
182: #define CH           16          /* TBX_CELL_H */
183: /* RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): 613-px content since
184:  * the content starts inside the left body rail (frame.left + 6), so 76 whole
185:  * cells (was 77, 618 px); X0 stays 1. */
186: #define SCR_COLS     76
187: #define SCR_LINES    24
188: #define X0           1
189: #define EDIT_ROWS    (SCR_LINES - 1)     /* 23 */
190: #define LINE_STATUS  (SCR_LINES - 1)
191: /* RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): one column left of the
192:  * measured WordPerfect columns (Doc..Ln ending at 67, "Pos" at 69) because the
193:  * field is 76 cells now, not 77: "Pos 2.3"" (8 cells) must still end inside
194:  * it. The group keeps its measured shape and spacing. */
195: #define STATUS_DOC_END 66                /* "... Ln 1"" ends at column 66 */
196: #define STATUS_POS   68                  /* "Pos" at column 68            */
197: /* REVEAL CODES (Alt-F3; spec/iword/reveal_codes.h). WordPerfect's 25-line
198:  * screen splits into the document (lines 0-10), the status line (11), the
199:  * ruler (12), the 11-row codes pane (13-23) and "Press Reveal Codes to
200:  * restore screen" (24). The window has 24 lines, so the document part is one
201:  * line shorter (10 rows) and the rest is WordPerfect's, row for row. */
202: #define RV_TOP_ROWS  10
203: #define RV_STATUS    10
204: #define RV_RULER     11
205: #define RV_PANE      12                  /* rows 12..22 */
206: #define RV_MESSAGE   23
207: 
208: #define FG           TBX_COLOR_WHITE
209: #define BG           TBX_COLOR_NAVY
210: 
211: #define MENU_FILE    129
212: #define ITEM_RETRIEVE 1
213: #define ITEM_SAVE    2
214: #define ITEM_EXIT    3
215: 
216: /* PS/2 set-1 make codes (event.c cooks message = vkey << 8 | ascii) */
217: #define VK_ESC       0x01
218: #define VK_BSPC      0x0E
219: #define VK_TAB       0x0F
220: #define VK_ENTER     0x1C
221: #define VK_F1        0x3B
222: #define VK_F2        0x3C
223: #define VK_F3        0x3D
224: #define VK_F4        0x3E
225: #define VK_F6        0x40
226: #define VK_F7        0x41
227: #define VK_F8        0x42
228: #define VK_F10       0x44
229: #define VK_HOME      0x47
230: #define VK_UP        0x48
231: #define VK_LEFT      0x4B
232: #define VK_RIGHT     0x4D
233: #define VK_END       0x4F
234: #define VK_DOWN      0x50
235: #define VK_DEL       0x53
236: 
237: /* ---- the menu bar (spec/toolbox_gate.h Sec 9): WP 5.1's File menu
238:  * (Reference Manual, "Select Save from the File menu" p. 564, "Select
239:  * Retrieve from the File menu" p. 553, "Select Exit from the File menu"
240:  * p. 193). ^Q on Exit is the FLAIR Quit chord every tenant answers. */
241: static const tbx_menuitem_t file_items[] = {
242:     { "Retrieve", 0, 0,   0, 1, 0, { 0, 0, 0 } },
243:     { "Save",     0, 0,   0, 1, 0, { 0, 0, 0 } },
244:     { "Exit",     0, 'Q', 0, 1, 0, { 0, 0, 0 } },
245: };
246: static const tbx_menuinfo_t menus[] = {
247:     { MENU_FILE, 0, "File", file_items, 3, 0 },
248: };
249: static const tbx_menubar_t mbar = { menus, 1, 1, 0 };
250: 
251: /* ---- the document --------------------------------------------------------
252:  * Two 64 KB banks: one holds the current document's ORIGINAL (prefix +
253:  * body, the piece table's read-only buffer), the other is scratch -- a file
254:  * is read into it, a save is flattened into it, and on success it becomes
255:  * the original (so a save also compacts the piece table). All BSS (the
256:  * e_minalloc area): file buffers may be BSS (spec/toolbox_gate.h Sec 10). */
257: #ifndef IWORD_SMALL_BUFFERS
258: #define BANK        65536u
259: #define ADD_CAP     65536u
260: #define PIECES      8192u
261: #define ROOM_PIECES 2048u        /* compact below this many free pieces   */
262: #define ROOM_ADD    8192u        /* ... or this many free add bytes       */
263: #else
264: /* The host face test only (test-iword-face): tiny buffers, so a few
265:  * thousand keys exercise compaction and the size refusal. NEVER shipped. */
266: #define BANK        4096u
267: #define ADD_CAP     1024u
268: #define PIECES      96u
269: #define ROOM_PIECES 32u
270: #define ROOM_ADD    256u
271: #endif
272: static uint8_t       g_bank[2][BANK];
273: static uint32_t      g_live;                /* the bank holding the original */
274: static uint8_t       g_add[ADD_CAP];
275: static iw_piece_t    g_pc[PIECES];
276: static iw_ed_t       g_ed;
277: static const uint8_t *g_prefix;
278: static uint32_t      g_prefix_len;
279: static uint32_t      g_top;                 /* first line on the screen */
280: static uint32_t      g_skip;                /* ... and how many of its rows are
281:                                              * above the screen (a spaced line's
282:                                              * blank rows; slice 3, MINT25
283:                                              * 04_reveal: WP scrolls by rows) */
284: 
285: /* ---- the screen ----------------------------------------------------------
286:  * A row is its text, the face of every cell (IW_A_* == the txFace bits the
287:  * Toolbox synthesizes: bold 1, italic 2, underline 4), its colours and one
288:  * reverse-video run (the cursor). A row is drawn only when it differs from
289:  * what the screen has: the whole row plain in one DRAWCELLS run, then every
290:  * styled stretch over it in its face (fg | face << 8, spec/toolbox_gate.h
291:  * Sec 7/7a), then the reverse run -- so a row with no codes draws exactly as
292:  * P1 drew it. */
293: typedef struct row {
294:     char    t[SCR_COLS + 1];
295:     uint8_t a[SCR_COLS];
296:     uint8_t fg, bg, inv_col, inv_len;
297:     uint8_t blk_col, blk_len;           /* the block's run (BLACK on GRAY) */
298: } row_t;
299: static int32_t  g_win TBX_IN_IMAGE = 0;
300: static char     g_line[SCR_COLS + 1] TBX_IN_IMAGE = { 0 };
301: static row_t    g_shadow[SCR_LINES];
302: static uint8_t  g_shadow_ok[SCR_LINES];
303: static uint8_t  g_caps;
304: static uint8_t  g_reveal;                   /* Alt-F3: Reveal Codes is on */
305: static uint8_t  g_clip[BANK];               /* the Move / Copy buffer        */
306: static uint32_t g_clip_n;
307: 
308: /* ---- prompts --------------------------------------------------------------- */
309: enum { M_EDIT, M_SAVE_NAME, M_GET_NAME, M_REPLACE, M_SAVEQ, M_EXITQ, M_ERROR, M_FONT, M_APPEAR,
310:        M_DELCODE, M_DELBLOCK, M_MOVE, M_MOVEBLK, M_RETRIEVE,
311:        M_FORMAT, M_FMTLINE, M_FMTJUST, M_FMTMARL, M_FMTMARR,
312:        M_SIZE, M_FONTBLK, M_MOVENB, M_RETMENU, M_FMTSP, M_GRAY, M_SEARCH,
313:        M_RCONF, M_RSEARCH, M_RTEXT, M_RASK, M_UNDO,
314:        M_SETUP, M_ENV, M_UNITS, M_UNITVALUE, M_BACKUP, M_BACKUPVALUE,
315:        M_LISTDIR, M_LIST, M_LISTSUB, M_LOOK, M_LISTRQ };
316: static int      g_mode = M_EDIT;
317: static int      g_err_return;               /* the prompt an ERROR returns to */
318: static const char *g_err;
319: /* WP 5.1 Reference pp. 963-966, Template pp. 5-6. Unavailable functions
320:  * show the manual's name in the period-grayed status line. The six unmapped
321:  * modified F11/F12 slots have no command name (p. 966 note 4). */
322: static const char *g_gray;
323: static uint8_t g_search_back;
324: /* Search strings are unit streams, distinct from the document's paired-code
325:  * insertion model. F6 twice gives [BOLD][bold] (Reference p. 572). */
326: #define QUERY_CAP 256u
327: static uint8_t g_query[QUERY_CAP];
328: static uint32_t g_query_n, g_query_cur;
329: static uint8_t g_query_fresh;
330: /* Replace: Reference pp. 551-553. Input streams keep complete code units.
331:  * Search and replacement are separate; inserted text is never searched again. */
332: static uint8_t g_find[QUERY_CAP], g_replacement[QUERY_CAP];
333: static uint32_t g_find_n, g_replacement_n, g_rs, g_re;
334: static uint8_t g_confirm;
335: /* Reference pp. 697-698: three deletion GROUPS, delimited by cursor motion.
336:  * Backspace/Delete paired attribute codes are deliberately not retained.
337:  * Block copies use the MINT12..15 self-contained unit stream. */
338: /* Fixed arenas respect the existing 512 KiB tenant BSS contract. A group
339:  * beyond this arena is refused before deletion, never silently discarded. */
340: #define UNDO_CAP (BANK / 2u)
341: static uint8_t g_undo[3][UNDO_CAP], g_deleted[UNDO_CAP];
342: #define g_preview (g_bank[1u - g_live])
343: static uint32_t g_undo_n[3], g_undo_count, g_undo_sel, g_undo_at;
344: static uint8_t g_undo_join;
345: static uint8_t g_settings[IW_SETTINGS_SIZE], g_settings_io[IW_SETTINGS_SIZE+1];
346: static uint8_t g_unit_target;
347: static uint32_t query_label(char *out, uint32_t cap);
348: static char     g_name[TBX_FILE_PATH_MAX];  /* the name being typed        */
349: static uint32_t g_nlen;
350: static uint8_t  g_fresh;                    /* the offered name: a key replaces it */
351: static char     g_path[TBX_FILE_PATH_MAX];  /* "NAME" for the file verbs     */
352: /* "A:\NAME" or, retrieved from List Files, "A:\DOCS\NAME" (mint 005 LF11_12) */
353: static char     g_current[TBX_FILE_PATH_MAX + 4]; /* once saved/retrieved   */
354: static char     g_shown[TBX_FILE_PATH_MAX + 4];   /* of the name being used */
355: static uint8_t  g_exiting;                  /* the save is part of F7 Exit  */
356: /* the Format: Line margin fields (Shift-F8 1 7): the text typed so far, the
357:  * left value taken (WPU), whether anything was typed */
358: static char     g_field[12];
359: static uint32_t g_flen;
360: static uint32_t g_fleft, g_fright;
361: static uint8_t  g_ftyped;
362: static uint32_t g_fsp;                      /* the spacing typed (8.8 fixed) */
363: static uint8_t  g_home;                     /* Home presses pending: WP's Home
364:                                              * is a prefix key (slice 3)     */
365: static uint8_t  g_sel;                      /* the block is Ctrl-F4 1/2/3's  */
366: static uint32_t g_hin;                      /* the Homes pressed before this key */
367: 
368: /* ---- small helpers --------------------------------------------------------- */
369: static uint32_t slen(const char *s) { uint32_t n = 0; while (s[n]) n++; return n; }
370: static void scopy(char *d, const char *s) { uint32_t i = 0; do { d[i] = s[i]; } while (s[i++]); }
371: static char upper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; }
372: 
373: static void put_cells(int line, int col, const char *s, uint32_t n, uint32_t fg, uint32_t bg)
374: {
375:     uint32_t i;
376:     if (n > SCR_COLS) n = SCR_COLS;
377:     for (i = 0; i < n; i++) g_line[i] = s[i];
378:     g_line[n] = '\0';
379:     (void)tbx_draw_cells(g_win, X0 + col * CW, line * CH, g_line, fg, bg);
380: }
381: 
382: static void row_clear(row_t *r)
383: {
384:     for (uint32_t i = 0; i < SCR_COLS; i++) { r->t[i] = ' '; r->a[i] = 0; }
385:     r->t[SCR_COLS] = 0;
386:     r->fg = (uint8_t)FG;
387:     r->bg = (uint8_t)BG;
388:     r->inv_col = r->inv_len = 0;
389:     r->blk_col = r->blk_len = 0;
390: }
391: 
392: /* Draw row `line` as `r` unless the screen has it. */
393: /* IW_A_GRAY (face-local, never a txFace bit): the cell is drawn GRAY -- a
394:  * List Files menu item InitechWord does not perform (spec/iword/listfiles.h
395:  * IW_LF_ITEMS). g_row_off moves a painter's rows down (the Look body,
396:  * WP rows 3.. of the Look screen). */
397: #define IW_A_GRAY   0x80u
398: static int g_row_off;
399: static void put_row(int line, const row_t *r)
400: {
401:     line += g_row_off;
402:     const row_t *o = &g_shadow[line];
403:     int same = g_shadow_ok[line] && o->fg == r->fg && o->bg == r->bg &&
404:                o->inv_col == r->inv_col && o->inv_len == r->inv_len &&
405:                o->blk_col == r->blk_col && o->blk_len == r->blk_len;
406:     uint32_t c;
407:     for (c = 0; same && c < SCR_COLS; c++) if (o->t[c] != r->t[c] || o->a[c] != r->a[c]) same = 0;
408:     if (same) return;
409:     g_shadow[line] = *r;
410:     g_shadow_ok[line] = 1;
411:     put_cells(line, 0, r->t, SCR_COLS, r->fg, r->bg);
412:     for (c = 0; c < SCR_COLS; ) {
413:         uint8_t f = r->a[c];
414:         uint32_t s0 = c;
415:         if (!f) { c++; continue; }
416:         while (c < SCR_COLS && r->a[c] == f) c++;
417:         put_cells(line, (int)s0, r->t + s0, c - s0,
418:                   (uint32_t)((f & IW_A_GRAY) ? TBX_COLOR_GRAY : r->fg) | TBX_TEXT_FACE(f & 0x07u), r->bg);
419:     }
420:     if (r->blk_len) put_cells(line, r->blk_col, r->t + r->blk_col, r->blk_len,
421:                              g_mode == M_UNDO ? BG : TBX_COLOR_BLACK,
422:                              g_mode == M_UNDO ? FG : TBX_COLOR_GRAY);
423:     if (r->inv_len) put_cells(line, r->inv_col, r->t + r->inv_col, r->inv_len, r->bg, r->fg);
424: }
425: 
426: /* A row of plain text (the status line, the messages). */
427: static void put_text_row(int line, const char *s)
428: {
429:     static row_t r;
430:     row_clear(&r);
431:     for (uint32_t i = 0; s[i] && i < SCR_COLS; i++) r.t[i] = s[i];
432:     put_row(line, &r);
433: }
434: 
435: /* ---- the document's bytes in a bank ---------------------------------------- */
436: static void doc_reset(void)
437: {
438:     iw_doc_init(&g_ed.doc, g_add, ADD_CAP, g_pc, PIECES, BANK - IW_WP5_DEFAULT_DOCSTART);
439:     iw_ed_init(&g_ed, IW_WP5_WRAP_COLS);
440:     g_prefix = iw_wp5_default_prefix(&g_prefix_len);
441:     g_top = 0;
442:     g_skip = 0;
443:     g_undo_count = 0; g_undo_join = 0;
444: }
445: 
446: /* Flatten prefix + body into the spare bank; *n = its length. */
447: static int flatten(uint32_t *n)
448: {
449:     /* the prefix records whether the cursor is before the end (0x103) */
450:     return iw_wp5_write(g_prefix, g_prefix_len, &g_ed.doc, g_ed.cur, g_bank[1u - g_live], BANK, n);
451: }
452: 
453: /* The spare bank (prefix + body, n bytes, already validated or flattened)
454:  * becomes the original. Positions do not move: the bytes are the same. */
455: static void adopt(uint32_t n, uint32_t prefix_len)
456: {
457:     uint32_t cur = g_ed.cur;
458:     uint8_t dirty = g_ed.dirty;
459:     uint32_t goal = g_ed.goal;
460:     g_live = 1u - g_live;
461:     g_prefix = g_bank[g_live];
462:     g_prefix_len = prefix_len;
463:     iw_doc_init(&g_ed.doc, g_add, ADD_CAP, g_pc, PIECES, BANK - prefix_len);
464:     iw_doc_load(&g_ed.doc, g_bank[g_live] + prefix_len, n - prefix_len);
465:     g_ed.cur = cur;
466:     g_ed.dirty = dirty;
467:     g_ed.goal = goal;
468: }
469: 
470: /* Compaction: the document's bytes become one original (one piece, an
471:  * empty add buffer). Always fits: the document never outgrows a bank. */
472: static void compact(void)
473: {
474:     uint32_t n;
475:     if (flatten(&n) == IW_OK) adopt(n, g_prefix_len);
476: }
477: 
478: /* Keep room for the next edit (the piece table grows with every edit). */
479: static void ensure_room(void)
480: {
481:     if (g_ed.doc.pc_cap - g_ed.doc.npc >= ROOM_PIECES && g_ed.doc.add_cap - g_ed.doc.add_len >= ROOM_ADD) return;
482:     compact();
483: }
484: 
485: /* One edit. IW_E_FULL either refused the edit (nothing changed: compact
486:  * and try once more) or stopped the re-wrap after it (the length changed:
487:  * compact and finish the wrap -- reflow is idempotent). */
488: enum { ED_TYPE, ED_ENTER, ED_BACKSPACE, ED_DELETE, ED_ATTR, ED_NORMAL, ED_TAB, ED_INDENT,
489:        ED_HPAGE, ED_ANSWER, ED_CUT, ED_COPY, ED_RETRIEVE, ED_MARGINS, ED_JUST, ED_SPACING };
490: static int edit_once(int op, uint8_t ch)
491: {
492:     switch (op) {
493:     case ED_TYPE:      return iw_ed_type(&g_ed, ch);
494:     case ED_ENTER:     return iw_ed_enter(&g_ed);
495:     case ED_BACKSPACE: return iw_ed_backspace(&g_ed);
496:     case ED_ATTR:      return iw_ed_block_attr(&g_ed, ch); /* ch = attribute number;
497:                                                           * Block on: given to it */
498:     case ED_NORMAL:    return iw_ed_normal(&g_ed);
499:     case ED_TAB:       return iw_ed_tab(&g_ed);
500:     case ED_INDENT:    return iw_ed_indent(&g_ed);
501:     case ED_HPAGE:     return iw_ed_hpage(&g_ed);
502:     case ED_ANSWER:    return iw_ed_answer(&g_ed, ch);   /* ch = yes */
503:     case ED_CUT:       return iw_ed_block_cut(&g_ed, ch ? g_clip : 0, sizeof g_clip, &g_clip_n);
504:     case ED_COPY:      return iw_ed_block_copy(&g_ed, g_clip, sizeof g_clip, &g_clip_n);
505:     case ED_RETRIEVE:  return iw_ed_retrieve(&g_ed, g_clip, g_clip_n);
506:     case ED_MARGINS:   return iw_ed_margins(&g_ed, g_fleft, g_fright);
507:     case ED_JUST:      return iw_ed_just(&g_ed, ch);      /* ch = IW_WP5_JUST_* */
508:     case ED_SPACING:   return iw_ed_spacing(&g_ed, g_fsp);
509:     default:           return iw_ed_delete(&g_ed);
510:     }
511: }
512: static int edit(int op, uint8_t ch)
513: {
514:     uint32_t before = g_ed.doc.len;
515:     uint32_t ds = g_ed.cur, dt = ds, dn = 0;
516:     int rc, fx;
517:     ensure_room();
518:     if (op == ED_BACKSPACE && ds) ds = iw_unit_prev(&g_ed.doc, ds);
519:     else if (op == ED_DELETE && dt < before) dt = iw_unit_next(&g_ed.doc, dt);
520:     else if (op == ED_ANSWER && ch) { ds = g_ed.ask; dt = iw_unit_next(&g_ed.doc, ds); }
521:     else if (op == ED_CUT && !ch && g_ed.blk != IW_NO_BLOCK) {
522:         uint32_t anchor = g_ed.blk;
523:         iw_ed_block_span(&g_ed, &ds, &dt);
524:         int copied = iw_ed_block_copy(&g_ed, g_deleted, UNDO_CAP, &dn);
525: #ifndef IWORD_MUT_UNDO_BLOCK
526:         g_ed.blk = anchor;
527: #else
528:         (void)anchor;
529: #endif
530:         if (copied != IW_OK) return IW_E_FULL;
531:     }
532:     if (dt > ds && !dn) {
533:         if (dt-ds > UNDO_CAP) return IW_E_FULL;
534:         /* Paired attributes removed with Backspace/Delete are not restored
535:          * (p. 697). Other complete units retain their P2 byte identity. */
536:         for (uint32_t p = ds; p < dt; p = iw_unit_next(&g_ed.doc, p)) {
537:             uint32_t n = iw_unit_next(&g_ed.doc, p)-p;
538:             int c = iw_doc_byte(&g_ed.doc, p);
539:             if (c == 0xC3 || c == 0xC4) continue;
540:             dn += iw_doc_read(&g_ed.doc, p, g_deleted+dn, n);
541:         }
542:     }
543:     if (dn && g_undo_join && g_undo_count && (ds == g_undo_at || dt == g_undo_at) &&
544:         g_undo_n[0] + dn > UNDO_CAP) return IW_E_FULL;
545:     rc = edit_once(op, ch);
546:     if (rc == IW_E_FULL) {
547:         compact();
548:         if (g_ed.doc.len == before) rc = edit_once(op, ch);
549:         else {
550:             rc = IW_OK;
551:             if (g_ed.cur > 0u) rc = iw_reflow_para(&g_ed.doc, g_ed.cur - 1u, g_ed.width);
552:             if (rc == IW_OK || rc == IW_E_FULL) rc = iw_reflow_para(&g_ed.doc, g_ed.cur, g_ed.width);
553:         }
554:     }
555:     /* WordPerfect's bookkeeping after every edit (iw_ed_fixup: the tab /
556:      * indent / hard-page geometry, C6, D4) -- idempotent, so a refusal for
557:      * room is compacted away and run again */
558:     fx = iw_ed_fixup(&g_ed);
559:     if (fx == IW_E_FULL) { compact(); fx = iw_ed_fixup(&g_ed); }
560:     if (rc == IW_OK && dn && g_ed.doc.len < before) {
561:         uint32_t limit = 3;
562: #ifdef IWORD_MUT_UNDO_LEVEL
563:         limit = 2;
564: #endif
565:         int join = g_undo_join && g_undo_count && (ds == g_undo_at || dt == g_undo_at);
566:         if (!join) {
567:             if (g_undo_count < limit) g_undo_count++;
568:             for (uint32_t j = g_undo_count-1; j; j--) {
569:                 g_undo_n[j] = g_undo_n[j-1];
570:                 for (uint32_t i = 0; i < g_undo_n[j]; i++) g_undo[j][i] = g_undo[j-1][i];
571:             }
572:             g_undo_n[0] = 0;
573:         }
574:         if (dn + g_undo_n[0] <= UNDO_CAP) {
575:             int prepend = join && dt == g_undo_at && ds < dt;
576: #ifdef IWORD_MUT_UNDO_ORDER
577:             prepend = 0;
578: #endif
579:             if (prepend) {
580:                 for (uint32_t i = g_undo_n[0]; i; i--) g_undo[0][i+dn-1] = g_undo[0][i-1];
581:                 for (uint32_t i = 0; i < dn; i++) g_undo[0][i] = g_deleted[i];
582:             } else for (uint32_t i = 0; i < dn; i++) g_undo[0][g_undo_n[0]+i] = g_deleted[i];
583:             g_undo_n[0] += dn; g_undo_at = ds; g_undo_join = 1;
584:         }
585:     }
586:     return rc == IW_OK ? fx : rc;
587: }
588: 
589: /* ---- painting ------------------------------------------------------------------ */
590: /* Line `ln` into row `r`; `*state` is the attribute state at ln->start and
591:  * leaves as the state at ln->end (the codes are walked, zero width). */
592: /* Line `ln` into row `r`, every unit at its column (a tab or an indent is
593:  * blank up to its stop; the line starts at its margin, ln->col0); `*state`
594:  * is the attribute state at ln->start and leaves as the state at ln->end
595:  * (the codes are walked, zero width). With Block on, the cells of the units
596:  * in [bs, bt) are the block's run. */
597: /* The screen column of relative column c on line ln (slice 3: the paper
598:  * position, screen column 0 at 1"). */
599: static uint32_t scol(const iw_line_t *ln, uint32_t c)
600: {
601:     return IW_SCREEN_COL(IW_LINE_X(ln, c));
602: }
603: 
604: static void row_text(const iw_line_t *ln, uint32_t *state, row_t *r, uint32_t bs, uint32_t bt)
605: {
606:     uint32_t p = ln->start, c = ln->col0, b0 = SCR_COLS, b1 = 0;
607:     row_clear(r);
608:     while (p < ln->end) {
609:         int k = iw_unit_kind(&g_ed.doc, p);
610:         uint32_t q = iw_unit_next(&g_ed.doc, p), c1 = c;
611:         if (p < ln->text_end) {
612:             uint32_t sc = scol(ln, c);
613:             if (k == IW_U_TEXT || k == IW_U_HSPACE) {
614:                 if (sc < SCR_COLS) {
615:                     r->t[sc] = k == IW_U_TEXT ? (char)iw_doc_byte(&g_ed.doc, p) : ' ';
616:                     r->a[sc] = (uint8_t)iw_attr_faces(*state);
617:                 }
618:                 c1 = c + 1u;
619:             } else if (k == IW_U_TAB || k == IW_U_INDENT) {
620:                 c1 = iw_line_col(&g_ed.doc, ln, q);      /* blank, plain */
621:             }
622:             if (p >= bs && p < bt && c1 > c) {
623:                 if (sc < b0) b0 = sc;
624:                 if (scol(ln, c1) > b1) b1 = scol(ln, c1);
625:             }
626:         }
627:         if (k == IW_U_CODE) *state = iw_attr_step(&g_ed.doc, p, *state);
628:         c = c1;
629:         p = q;
630:     }
631:     if (b1 > SCR_COLS) b1 = SCR_COLS;
632:     if (b0 < b1) { r->blk_col = (uint8_t)b0; r->blk_len = (uint8_t)(b1 - b0); }
633: }
634: 
635: /* The screen rows line `ln` takes: its own, the blank rows of its line
636:  * spacing below it -- the spacing rounded to the nearest whole row (spec/
637:  * iword/wp5_format.h "line spacing" SCREEN: MINT25 1.5 -> 2, MINT26 1.33 ->
638:  * 1, 3 -> 3; MINT30 2.25 -> 2) -- and a hard page's bar (MINT09 08_edit_top;
639:  * the bar after the blank rows is inferred, W-17). `last` = the document's
640:  * last line, which has nothing below it. */
641: static uint32_t line_rows(const iw_line_t *ln, int last, uint32_t *bar)
642: {
643:     uint32_t n = 1u;
644:     *bar = 0;
645:     if (last) return n;
646:     n += iw_spacing_rows(ln->sp) - 1u;
647:     if (ln->term == IW_U_HPG) { *bar = 1u; n++; }
648:     return n;
649: }
650: 
651: /* Keep the cursor's line among the `rows` screen rows, scrolling by rows as
652:  * WordPerfect does (MINT25 04_reveal: the window may start on a spaced
653:  * line's blank row; never on a hard page's bar -- a line and its bar go
654:  * together, as before slice 3). */
655: static void follow(const iw_where_t *w, uint32_t rows)
656: {
657:     static iw_line_t lines[EDIT_ROWS + 1];
658:     if (w->line < g_top || (w->line == g_top && g_skip)) { g_top = w->line; g_skip = 0; }
659:     if (w->line >= g_top + rows) { g_top = w->line - (rows - 1u); g_skip = 0; }
660:     for (;;) {
661:         uint32_t n = w->line - g_top, got, k, r = 0, bar;
662:         int32_t d;
663:         if (n > EDIT_ROWS) n = EDIT_ROWS;
664:         got = iw_ed_lines(&g_ed, g_top, n, lines);
665:         for (k = 0; k < got && k < n; k++) r += line_rows(&lines[k], 0, &bar);
666:         r -= g_skip;
667:         if (r < rows) return;
668:         d = (int32_t)(r - (rows - 1u));
669:         /* the top moves down d rows */
670:         while (d > 0) {
671:             uint32_t nr;
672:             (void)iw_ed_lines(&g_ed, g_top, 1u, lines);
673:             nr = line_rows(&lines[0], 0, &bar);
674: #ifdef IWORD_MUT_LINE_SCROLL
675:             /* MUTANT (Rule 6; test-iword-face-mutant): the window always
676:              * starts on a line, never on a blank row. NEVER real. */
677:             bar = nr - 1u;
678: #endif
679:             if (g_skip + (uint32_t)d <= nr - bar - 1u) { g_skip += (uint32_t)d; d = 0; }
680:             else { d -= (int32_t)(nr - g_skip); g_top++; g_skip = 0; }
681:         }
682:     }
683: }
684: 
685: /* The first and last lines whose text row is on the screen (Home, Up /
686:  * Home, Down). */
687: static void screen_lines(uint32_t rows, uint32_t *first, uint32_t *last)
688: {
689:     static iw_line_t lines[EDIT_ROWS + 1];
690:     uint32_t got = iw_ed_lines(&g_ed, g_top, rows + 1u, lines), k, bar;
691:     int32_t r = -(int32_t)g_skip;
692:     int any = 0;
693:     *first = *last = g_top;
694:     for (k = 0; k < got && r < (int32_t)rows; k++) {
695:         if (r >= 0) {
696:             if (!any) *first = g_top + k;
697:             *last = g_top + k;
698:             any = 1;
699:         }
700:         r += (int32_t)line_rows(&lines[k], k + 1u == got && got < rows + 1u, &bar);
701:     }
702: }
703: 
704: static void paint_status(const iw_where_t *w, int line)
705: {
706:     char s[SCR_COLS + 1], num[12], doc[32];
707:     uint32_t n, i, k, query_cursor = 0;
708:     for (i = 0; i < SCR_COLS; i++) s[i] = ' ';
709:     s[SCR_COLS] = 0;
710:     if (g_mode == M_EDIT || g_mode == M_RETRIEVE) {
711:         /* left: the document's name; "Block on" while Block is on (MINT12
712:          * 01_block_on); the Move prompt while a block waits (05_move_prompt) */
713:         const char *left = g_mode == M_RETRIEVE ? IW_MOVE_RETRIEVE :
714:                            g_ed.blk != IW_NO_BLOCK ? IW_BLOCK_ON : g_current;
715: #ifdef IWORD_MUT_NO_BLOCK_ON
716:         /* MUTANT (Rule 6; test-iword-face-mutant): Block on is not shown --
717:          * the document's name stays. NEVER real. */
718:         if (g_mode == M_EDIT) left = g_current;
719: #endif
720:         for (i = 0; left[i] && i < 40u; i++) s[i] = left[i];
721:         /* "Doc 1 Pg <p> Ln <ln>" ending at column STATUS_DOC_END */
722:         scopy(doc, "Doc 1 Pg ");
723:         n = slen(doc);
724:         {
725:             uint32_t pg = w->page, t = 0;
726:             char tmp[12];
727:             do { tmp[t++] = (char)('0' + pg % 10u); pg /= 10u; } while (pg);
728:             while (t) doc[n++] = tmp[--t];
729:         }
730:         doc[n++] = ' '; doc[n++] = 'L'; doc[n++] = 'n'; doc[n++] = ' ';
731:         /* Ln from the line's height down the page (slice 3: the spacing,
732:          * MINT25 "Ln 2.83""; single spacing gives P1's 1" + lines / 6) */
733:         k = iw_fmt_inches(iw_ln_y_hundredths(w->y), num);
734:         for (i = 0; i < k; i++) doc[n++] = num[i];
735:         {
736:             /* Pos: the paper position (slice 3, MINT23 "Pos 1.25""; at 1"/1"
737:              * it is 1" + column / 10 as before). RE-KEYED 2026-10-16 (bead
738:              * initech-jojk slice 3, stated): a value with two decimals
739:              * ("Pos 3.85"", MINT32 03_typed) is one cell longer than the
740:              * 76-cell field leaves after column 68 -- the whole group (Doc ..
741:              * Ln and Pos) moves left by the overflow so the inch mark stays,
742:              * its shape and spacing kept (authored adaptation of WP's 80
743:              * columns, as tdnl.51's one-column move) */
744:             char pos[12];
745:             uint32_t pk = iw_fmt_inches(w->x / 12u, pos);
746:             uint32_t sh = STATUS_POS + 4u + pk > SCR_COLS ? STATUS_POS + 4u + pk - SCR_COLS : 0u;
747:             for (i = 0; i < n; i++) s[STATUS_DOC_END + 1 - sh - n + i] = doc[i];
748:             s[STATUS_POS - sh] = 'P';
749:             s[STATUS_POS - sh + 1] = g_caps ? 'O' : 'o';
750:             s[STATUS_POS - sh + 2] = g_caps ? 'S' : 's';
751:             for (i = 0; i < pk; i++) s[STATUS_POS - sh + 4 + i] = pos[i];
752:         }
753:         if (g_mode == M_RETRIEVE) {
754:             /* "Enter" bright in WP's prompt: drawn bold here */
755:             static row_t r;
756:             row_clear(&r);
757:             for (i = 0; i < SCR_COLS; i++) r.t[i] = s[i];
758:             for (i = 0; i < 5u; i++) r.a[IW_MOVE_ENTER_COL + i] = IW_A_BOLD;
759:             put_row(line, &r);
760:             return;
761:         }
762:     } else {
763:         const char *t = "";
764:         char replace[48];
765:         if (g_mode == M_SAVE_NAME) t = "Document to be Saved: ";
766:         else if (g_mode == M_GET_NAME) t = "Document to be Retrieved: ";
767:         else if (g_mode == M_SAVEQ) t = "Save Document? Yes (No)";
768:         else if (g_mode == M_EXITQ) t = "Exit WP? No (Yes)";
769:         else if (g_mode == M_ERROR) t = g_err;
770:         else if (g_mode == M_GRAY) t = g_gray;
771:         else if (g_mode == M_RCONF) {
772:             t = "w/Confirm? No (Yes)";
773: #ifdef IWORD_MUT_REPLACE_PROMPT
774:             t = "Confirm?";
775: #endif
776:         }
777:         else if (g_mode == M_RASK) t = "Replace? Yes (No)";
778:         else if (g_mode == M_UNDO) t = "Undelete 1 Restore; 2 Previous Deletion: 0";
779:         else if (g_mode == M_RTEXT) t = "Replace with:";
780:         else if (g_mode == M_SEARCH || g_mode == M_RSEARCH) {
781:             t = g_search_back ? "<- Srch:" : "-> Srch:";
782: #ifdef IWORD_MUT_SEARCH_PROMPT
783:             t = "Search:";
784: #endif
785:         }
786:         else if (g_mode == M_FONT) t = IW_FONT_MENU;
787:         else if (g_mode == M_FONTBLK) t = IW_FONT_BLK_MENU;   /* MINT28 07 */
788:         else if (g_mode == M_SIZE) t = IW_SIZE_MENU;           /* MINT28 11 / 18 */
789:         else if (g_mode == M_APPEAR) t = IW_APPEAR_MENU;
790:         else if (g_mode == M_MOVE) t = IW_MOVE_MENU;
791:         else if (g_mode == M_MOVENB) t = IW_MOVE_NB_MENU;      /* MINT31 02 */
792:         else if (g_mode == M_RETMENU) t = IW_RETRIEVE_MENU;    /* MINT29 14 */
793:         else if (g_mode == M_MOVEBLK) t = IW_MOVE_BLK_MENU;
794:         else if (g_mode == M_DELBLOCK) t = IW_DEL_BLOCK;
795:         else if (g_mode == M_DELCODE) {
796:             /* "Delete [BOLD]? No (Yes)": the code's own Reveal label */
797:             char lab[24];
798:             scopy(replace, IW_DEL_CODE_PRE);
799:             n = slen(replace);
800:             (void)iw_code_label(&g_ed.doc, g_ed.ask, lab, sizeof lab);
801:             for (i = 0; lab[i] && n < 40u; i++) replace[n++] = lab[i];
802:             scopy(replace + n, IW_DEL_CODE_POST);
803:             t = replace;
804:         }
805:         else if (g_mode == M_REPLACE) {
806:             n = 0;
807:             scopy(replace, "Replace ");
808:             n = slen(replace);
809:             for (i = 0; g_shown[i] && n < 40u; i++) replace[n++] = g_shown[i];
810:             scopy(replace + n, "? No (Yes)");
811:             t = replace;
812:         }
813:         n = 0;
814:         for (i = 0; t[i] && n < SCR_COLS; i++) s[n++] = t[i];
815:         if (g_mode == M_SEARCH || g_mode == M_RSEARCH || g_mode == M_RTEXT)
816:             query_cursor = n + query_label(s + n, SCR_COLS - n + 1);
817:         if (g_mode == M_SAVE_NAME || g_mode == M_GET_NAME)
818:             for (i = 0; i < g_nlen && n < SCR_COLS; i++) s[n++] = g_name[i];
819:         if (g_mode == M_SAVEQ && !g_ed.dirty) {
820:             const char *nm = "(Text was not modified)";
821:             k = slen(nm);
822:             for (i = 0; i < k; i++) s[SCR_COLS - k + i] = nm[i];
823:         }
824:     }
825:     if (g_mode == M_GRAY || g_mode == M_SEARCH || g_mode == M_RSEARCH || g_mode == M_RTEXT) {
826:         row_t r;
827:         row_clear(&r);
828:         for (i = 0; i < SCR_COLS; i++) r.t[i] = s[i];
829:         if (g_mode == M_GRAY) r.fg = TBX_COLOR_GRAY;
830:         else {
831:             r.inv_col = (uint8_t)query_cursor; r.inv_len = 1;
832: #ifdef IWORD_MUT_SEARCH_CURSOR
833:             r.inv_len = 0;
834: #endif
835:         }
836:         put_row(line, &r);
837:     } else put_text_row(line, s);
838: }
839: 
840: /* The document rows 0 .. rows-1, the cursor in reverse video, a spaced
841:  * line's blank rows and a hard page's bar below its line, the block (if on)
842:  * in its run. */
843: static void paint_doc(const iw_where_t *w, uint32_t rows)
844: {
845:     static iw_line_t lines[EDIT_ROWS + 1];
846:     static row_t r;
847:     uint32_t got, li, i = 0, state = 0, bs = 1u, bt = 0, skip;
848:     int ccol;
849:     follow(w, rows);
850:     skip = g_skip;
851:     got = iw_ed_lines(&g_ed, g_top, rows + 1u, lines);
852:     if (got) state = iw_attr_state(&g_ed.doc, lines[0].start);
853:     if (g_ed.blk != IW_NO_BLOCK) iw_ed_block_span(&g_ed, &bs, &bt);
854:     ccol = (int)(IW_SCREEN_COL(w->x) < SCR_COLS ? IW_SCREEN_COL(w->x) : SCR_COLS - 1u);
855:     for (li = 0; li < got && i < rows; li++) {
856:         uint32_t bar, n = line_rows(&lines[li], li + 1u == got && got < rows + 1u, &bar), k;
857:         row_text(&lines[li], &state, &r, bs, bt);
858:         if (g_top + li == w->line) {
859:             r.inv_col = (uint8_t)ccol;
860:             r.inv_len = 1;
861:             /* the cursor cell is never in the block's band (MINT12 02/12a) */
862:             if (r.blk_len && ccol >= r.blk_col && ccol < r.blk_col + r.blk_len) {
863:                 if (ccol == r.blk_col) { r.blk_col++; r.blk_len--; }
864:                 else if (ccol == r.blk_col + r.blk_len - 1) r.blk_len--;
865:             }
866:         }
867:         for (k = 0; k < n && i < rows; k++) {
868:             if (skip) { skip--; continue; }
869:             if (k == 0) { put_row((int)i++, &r); continue; }
870:             row_clear(&r);
871:             if (bar && k == n - 1u)
872:                 for (uint32_t c = 0; c < SCR_COLS; c++) r.t[c] = '=';
873:             put_row((int)i++, &r);
874:         }
875:     }
876:     while (i < rows) { row_clear(&r); put_row((int)i++, &r); }
877: }
878: 
879: /* Reveal Codes: the ruler, the codes pane and the restore message. */
880: static char g_pane[IW_RV_PANE_ROWS][SCR_COLS + 1];
881: static void paint_reveal(const iw_where_t *w)
882: {
883:     static row_t r;
884:     iw_pane_t pane;
885:     uint32_t i, x;
886:     row_clear(&r);
887:     r.fg = (uint8_t)TBX_COLOR_BLACK;           /* authored: WP's band is red */
888:     r.bg = (uint8_t)TBX_COLOR_GRAY;
889:     /* the tab marks every 0.5" from the left margin - 1" (relative tabs),
890:      * the margins as braces (on a stop) or brackets (spec/iword/
891:      * reveal_codes.h "the ruler"; MINT21 09_reveal, MINT23 03_reveal_mid) */
892:     x = w->lm >= 2u * IW_WP5_TAB_STEP_WPU ? w->lm - 2u * IW_WP5_TAB_STEP_WPU : w->lm % IW_WP5_TAB_STEP_WPU;
893:     for (; IW_SCREEN_COL(x) < SCR_COLS; x += IW_WP5_TAB_STEP_WPU)
894:         if (x >= IW_WP5_LEFT_WPU) r.t[IW_SCREEN_COL(x)] = (char)IW_RV_RULER_TAB;
895:     if (IW_SCREEN_COL(w->lm) < SCR_COLS) r.t[IW_SCREEN_COL(w->lm)] = IW_RV_RULER_LEFT;
896:     if (IW_SCREEN_COL(w->rm) < SCR_COLS)
897:         r.t[IW_SCREEN_COL(w->rm)] = (w->rm >= w->lm && (w->rm - w->lm) % IW_WP5_TAB_STEP_WPU == 0u)
898:                                     ? IW_RV_RULER_RIGHT : IW_RV_RULER_RBRACK;
899:     put_row(RV_RULER, &r);
900:     iw_reveal_pane(&g_ed, SCR_COLS, IW_RV_PANE_ROWS, &g_pane[0][0], SCR_COLS + 1u, &pane);
901:     for (i = 0; i < IW_RV_PANE_ROWS; i++) {
902:         row_clear(&r);
903:         for (uint32_t c = 0; g_pane[i][c] && c < SCR_COLS; c++) r.t[c] = g_pane[i][c];
904:         if (pane.cur_len && pane.cur_row == i) { r.inv_col = (uint8_t)pane.cur_col; r.inv_len = (uint8_t)pane.cur_len; }
905:         put_row(RV_PANE + (int)i, &r);
906:     }
907:     for (; RV_PANE + (int)i < RV_MESSAGE; i++) put_text_row(RV_PANE + (int)i, "");   /* WP's blank line 23 */
908:     put_text_row(RV_MESSAGE, IW_RV_MESSAGE);
909: }
910: 
911: /* ---- the Format screens (Shift-F8; slice 3) ------------------------------------ */
912: typedef struct fmt_row { uint8_t row, col; const char *text; } fmt_row_t;
913: static const fmt_row_t FMT_MAIN[] = IW_FMT_INIT;
914: _Static_assert(sizeof FMT_MAIN / sizeof FMT_MAIN[0] == IW_FMT_ROWS, "IW_FMT_ROWS == the rows of IW_FMT_INIT");
915: static const fmt_row_t FMT_LINE[IW_FMT_LINE_ROWS] = IW_FMT_LINE_INIT;
916: 
917: static void put_at(row_t *r, uint32_t col, const char *t)
918: {
919:     for (uint32_t i = 0; t[i] && col + i < SCR_COLS; i++) r->t[col + i] = t[i];
920: }
921: 
922: /* A margin as the Format: Line screen shows it ("1"", "1.3"") */
923: static void put_inches(row_t *r, uint32_t col, uint32_t wpu)
924: {
925:     char num[12];
926:     (void)iw_fmt_inches(wpu / 12u, num);
927:     put_at(r, col, num);
928: }
929: 
930: static void paint_format(const iw_where_t *w)
931: {
932:     static row_t rows[SCR_LINES];
933:     static const char *const JN[4] = IW_FMT_JUST_NAMES;
934:     uint32_t i, n = g_mode == M_FORMAT ? IW_FMT_ROWS : IW_FMT_LINE_ROWS;
935:     const fmt_row_t *t = g_mode == M_FORMAT ? FMT_MAIN : FMT_LINE;
936:     char sel[16];
937:     for (i = 0; i < SCR_LINES; i++) row_clear(&rows[i]);
938:     for (i = 0; i < n; i++) {
939:         /* WP's 25 rows in 24: the Format screen drops its blank row 1 */
940:         uint32_t r = t[i].row;
941:         if (!t[i].text || !t[i].text[0]) continue;
942:         if (g_mode == M_FORMAT && r > 0u) r--;
943:         put_at(&rows[r], t[i].col, t[i].text);
944:     }
945:     if (g_mode != M_FORMAT) {
946:         put_at(&rows[2], IW_FMT_VALUE_COL, IW_FMT_V_HYPH);
947:         put_at(&rows[4], IW_FMT_VALUE_COL, IW_FMT_V_ZONE_L);
948:         put_at(&rows[5], IW_FMT_VALUE_COL, IW_FMT_V_ZONE_R);
949:         put_at(&rows[7], IW_FMT_VALUE_COL, JN[w->just < 4u ? w->just : 1u]);
950:         put_at(&rows[9], IW_FMT_VALUE_COL, IW_FMT_V_HEIGHT);
951:         put_at(&rows[11], IW_FMT_VALUE_COL, IW_FMT_V_NUMBER);
952:         {
953:             /* the spacing at the cursor (slice 3; MINT25 02_spacing_set "2",
954:              * MINT30 02_spacing_225 "2.25"): hundredths, trailing zeros
955:              * dropped (IW_FMT_V_SPACING "1" at rest) */
956:             char num[12];
957:             uint32_t k = iw_fmt_inches((w->sp * 100u + IW_WP5_SPACING_ONE / 2u) / IW_WP5_SPACING_ONE, num);
958:             num[k - 1u] = 0;
959:             put_at(&rows[13], IW_FMT_VALUE_COL, num);
960:         }
961:         put_inches(&rows[15], IW_FMT_VALUE_COL, w->lm);
962:         put_inches(&rows[16], IW_FMT_VALUE_COL, IW_WP5_PAPER_WPU - w->rm);
963:         put_at(&rows[18], IW_FMT_VALUE_COL, IW_FMT_V_TABS);
964:         put_at(&rows[20], IW_FMT_VALUE_COL, IW_FMT_V_WIDOW);
965:         if (g_mode == M_FMTMARL || g_mode == M_FMTMARR || g_mode == M_FMTSP) {
966:             /* the field being typed: the value until a key replaces it, then
967:              * the text typed and the cursor after it (04_left_typed "1.3_";
968:              * the spacing field likewise, MINT25 01_spacing_field) */
969:             row_t *fr = &rows[g_mode == M_FMTMARL ? 15 : g_mode == M_FMTMARR ? 16 : 13];
970:             if (g_mode == M_FMTMARR) put_inches(&rows[15], IW_FMT_VALUE_COL, g_fleft);
971:             if (g_ftyped) {
972:                 for (i = IW_FMT_VALUE_COL; i < SCR_COLS; i++) fr->t[i] = ' ';
973:                 for (i = 0; i < g_flen; i++) fr->t[IW_FMT_VALUE_COL + i] = g_field[i];
974:                 fr->inv_col = (uint8_t)(IW_FMT_VALUE_COL + g_flen);
975:             } else {
976:                 fr->inv_col = (uint8_t)IW_FMT_VALUE_COL;
977:             }
978:             fr->inv_len = 1;
979:         }
980:     }
981:     if (g_mode == M_FMTJUST) {
982:         put_at(&rows[SCR_LINES - 1], 0, IW_FMT_JUST_PROMPT);
983:     } else {
984:         scopy(sel, IW_FMT_SELECTION);
985:         i = slen(sel);
986:         sel[i] = (g_mode == M_FMTMARL || g_mode == M_FMTMARR) ? '7' : g_mode == M_FMTSP ? '6' : '0';
987:         sel[i + 1] = 0;
988:         put_at(&rows[SCR_LINES - 1], 0, sel);
989:         if (g_mode == M_FORMAT || g_mode == M_FMTLINE) {
990:             rows[SCR_LINES - 1].inv_col = (uint8_t)i;
991:             rows[SCR_LINES - 1].inv_len = 1;
992:         }
993:     }
994:     for (i = 0; i < SCR_LINES; i++) put_row((int)i, &rows[i]);
995: }
996: 
997: static int format_mode(void)
998: {
999:     return g_mode == M_FORMAT || g_mode == M_FMTLINE || g_mode == M_FMTJUST || g_mode == M_FMTMARL || g_mode == M_FMTMARR ||
1000:            g_mode == M_FMTSP;
1001: }
1002: 
1003: /* Minimal Setup, Reference pp. 574-575; Environment 3 / Backup 1 (p. 42),
1004:  * Units 8 and its two independent selections (pp. 698-700). Unavailable
1005:  * entries retain their printed names in gray. Layout fits the native face. */
1006: static int setup_mode(void) { return g_mode >= M_SETUP && g_mode <= M_BACKUPVALUE; }
1007: static const char *unit_name(uint8_t u)
1008: {
1009:     switch (u) {
1010:     case 'i': return "Inches"; case 'c': return "Centimeters"; case 'p': return "Points";
1011:     case 'w': return "1200ths of an inch"; case 'u': return "WordPerfect 4.2 units";
1012:     default: return "?";
1013:     }
1014: }
1015: static void setup_row(int line, const char *text, int gray)
1016: {
1017:     row_t r; row_clear(&r); put_at(&r, 0, text);
1018:     if (gray) r.fg = TBX_COLOR_GRAY;
1019:     put_row(line, &r);
1020: }
1021: static void setup_value(int line, const char *text, const char *value)
1022: {
1023:     row_t r; row_clear(&r); put_at(&r, 0, text); put_at(&r, slen(text)+2, value); put_row(line, &r);
1024: }
1025: static void paint_setup(void)
1026: {
1027:     for (int i = 0; i < SCR_LINES; i++) put_text_row(i, "");
1028:     if (g_mode == M_SETUP) {
1029:         setup_row(0, "Setup", 0);
1030:         setup_row(4, "2 Display", 1); setup_row(6, "3 Environment", 0);
1031:         setup_row(8, "4 Initial Settings", 1); setup_row(10, "5 Keyboard Layout", 1);
1032:         setup_row(12, "6 Location of Files", 1);
1033:     } else if (g_mode == M_ENV) {
1034:         setup_row(0, "Setup: Environment", 0);
1035:         setup_row(4, "1 Backup Options", 0); setup_row(6, "8 Units of Measure", 0);
1036:     } else if (g_mode == M_UNITS || g_mode == M_UNITVALUE) {
1037:         setup_row(0, "Setup: Units of Measure", 0);
1038:         setup_value(4, "1 Display and Entry of Numbers for Margins, Tabs, etc.", unit_name(g_settings[4]));
1039:         setup_value(6, "2 Status Line Display", unit_name(g_settings[5]));
1040:         setup_row(10, "i Inches; c Centimeters; p Points", 0);
1041:         setup_row(12, "w 1200ths of an inch; u WordPerfect 4.2 units", 0);
1042:     } else {
1043:         setup_row(0, "Setup: Backup", 0);
1044:         setup_row(4, "1 Timed Document Backup", 1);
1045:         setup_value(6, "2 Original Document Backup", g_settings[6] ? "Yes" : "No");
1046:     }
1047:     if (g_mode == M_UNITVALUE) setup_row(23, "Units of Measure:", 0);
1048:     else if (g_mode == M_BACKUPVALUE) setup_row(23, "Original Document Backup? No (Yes)", 0);
1049:     else setup_row(23, "Selection: 0", 0);
1050: }
1051: 
1052: static int lf_mode(void);
1053: static void paint_lf(void);
1054: static void paint(void)
1055: {
1056:     iw_where_t w;
1057:     if (setup_mode()) { paint_setup(); return; }
1058:     if (lf_mode()) { paint_lf(); return; }
1059:     if (g_mode == M_UNDO) {
1060:         /* Preview is a borrowed temporary document. The live piece table,
1061:          * add arena, original bytes, cursor and dirty flag never change. */
1062:         iw_ed_t saved = g_ed;
1063:         iw_piece_t pc;
1064:         uint32_t n = g_undo_count ? g_undo_n[g_undo_sel] : 0, at = g_ed.cur;
1065:         if (g_ed.doc.len+n <= BANK) {
1066:             (void)iw_doc_read(&g_ed.doc, 0, g_preview, at);
1067:             for (uint32_t i = 0; i < n; i++) g_preview[at+i] = g_undo[g_undo_sel][i];
1068:             (void)iw_doc_read(&g_ed.doc, at, g_preview+at+n, g_ed.doc.len-at);
1069:             iw_doc_init(&g_ed.doc, 0, 0, &pc, 1, BANK);
1070:             iw_doc_load(&g_ed.doc, g_preview, saved.doc.len+n);
1071:             g_ed.blk = at; g_ed.cur = at+n;
1072: #ifdef IWORD_MUT_UNDO_PREVIEW
1073:             g_ed.blk = IW_NO_BLOCK;
1074: #endif
1075:         }
1076:         iw_ed_where(&g_ed, &w); paint_doc(&w, EDIT_ROWS); paint_status(&w, LINE_STATUS);
1077:         g_ed = saved;
1078:         return;
1079:     }
1080:     iw_ed_where(&g_ed, &w);
1081:     if (format_mode()) {
1082:         paint_format(&w);
1083:         return;
1084:     }
1085:     if (!g_reveal) {
1086:         paint_doc(&w, EDIT_ROWS);
1087:         paint_status(&w, LINE_STATUS);
1088:         return;
1089:     }
1090:     paint_doc(&w, RV_TOP_ROWS);
1091:     paint_status(&w, RV_STATUS);
1092:     paint_reveal(&w);
1093: }
1094: 
1095: static void paint_all(void)
1096: {
1097:     for (int i = 0; i < SCR_LINES; i++) g_shadow_ok[i] = 0;
1098:     (void)tbx_fill_rect(g_win, 0, 0, WIN_R, WIN_B, BG);
1099:     paint();
1100: }
1101: 
1102: /* ---- files ------------------------------------------------------------------------ */
1103: static void error(const char *msg, int back)
1104: {
1105:     g_err = msg;
1106:     g_err_return = back;
1107:     g_mode = M_ERROR;
1108: }
1109: 
1110: static void settings_defaults(void)
1111: {
1112:     for (uint32_t i = 0; i < 4; i++) g_settings[i] = IW_SETTINGS_MAGIC[i];
1113:     g_settings[4] = g_settings[5] = IW_SETTINGS_DEFAULT_UNIT;
1114:     g_settings[6] = g_settings[7] = 0;
1115: }
1116: static void settings_load(void)
1117: {
1118:     int32_t h, n, cl;
1119:     uint32_t pos = 0;
1120:     settings_defaults();
1121:     h = tbx_file_open(IW_SETTINGS_PATH, TBX_FILE_READ_ONLY);
1122:     if (h < 0) return;
1123:     do {
1124:         n = tbx_file_read(h, g_settings_io+pos, sizeof g_settings_io-pos);
1125:         if (n <= 0 || (uint32_t)n > sizeof g_settings_io-pos) break;
1126:         pos += (uint32_t)n;
1127:     } while (pos < sizeof g_settings_io);
1128:     cl = tbx_file_close(h);
1129:     if (n < 0 || pos != IW_SETTINGS_SIZE || cl < 0) return;
1130:     for (uint32_t i = 0; i < 4; i++) if (g_settings_io[i] != IW_SETTINGS_MAGIC[i]) return;
1131: #ifndef IWORD_MUT_SETUP_VALIDATE
1132:     if (g_settings_io[6] > 1 || g_settings_io[7] || unit_name(g_settings_io[4])[0] == '?' ||
1133:         unit_name(g_settings_io[5])[0] == '?') return;
1134: #endif
1135: #ifndef IWORD_MUT_SETUP_LOAD
1136:     for (uint32_t i = 0; i < IW_SETTINGS_SIZE; i++) g_settings[i] = g_settings_io[i];
1137: #endif
1138: }
1139: static int settings_save(void)
1140: {
1141:     int32_t h, n, cl;
1142:     uint32_t pos = 0;
1143:     for (uint32_t i = 0; i < IW_SETTINGS_SIZE; i++) g_settings_io[i] = g_settings[i];
1144: #ifdef IWORD_MUT_SETUP_SAVE
1145:     g_settings_io[4] = 'i';
1146: #endif
1147:     h = tbx_file_create(IW_SETTINGS_PATH);
1148:     if (h < 0) return IW_E_FULL;
1149:     while (pos < IW_SETTINGS_SIZE) {
1150:         n = tbx_file_write(h, g_settings_io+pos, IW_SETTINGS_SIZE-pos);
1151:         if (n <= 0 || (uint32_t)n > IW_SETTINGS_SIZE-pos) break;
1152:         pos += (uint32_t)n;
1153: #ifdef IWORD_MUT_SETUP_SHORT
1154:         break;
1155: #endif
1156:     }
1157:     cl = tbx_file_close(h);
1158:     return pos == IW_SETTINGS_SIZE && cl >= 0 ? IW_OK : IW_E_FULL;
1159: }
1160: static void setup_key(char ch, uint8_t vk)
1161: {
1162:     if (vk == VK_F1 || vk == VK_ESC) { g_mode = M_EDIT; paint_all(); return; }
1163:     if (vk == VK_F7) {
1164:         g_mode = g_mode == M_SETUP ? M_EDIT : g_mode == M_ENV ? M_SETUP : M_ENV;
1165:         paint_all(); return;
1166:     }
1167:     if (g_mode == M_SETUP && ch == '3') g_mode = M_ENV;
1168:     else if (g_mode == M_ENV && ch == '8') g_mode = M_UNITS;
1169:     else if (g_mode == M_ENV && ch == '1') g_mode = M_BACKUP;
1170:     else if (g_mode == M_UNITS && (ch == '1' || ch == '2')) { g_unit_target = ch == '1' ? 4 : 5; g_mode = M_UNITVALUE; }
1171:     else if (g_mode == M_BACKUP && ch == '2') g_mode = M_BACKUPVALUE;
1172:     else if (g_mode == M_UNITVALUE || g_mode == M_BACKUPVALUE) {
1173:         uint8_t at = g_mode == M_UNITVALUE ? g_unit_target : 6, old = g_settings[at], value;
1174:         if (g_mode == M_UNITVALUE) {
1175:             value = (uint8_t)ch;
1176:             if (unit_name(value)[0] == '?') return;
1177:         } else {
1178:             if (ch != 'y' && ch != 'Y' && ch != 'n' && ch != 'N') return;
1179:             value = ch == 'y' || ch == 'Y';
1180:         }
1181:         g_settings[at] = value;
1182:         if (settings_save() != IW_OK) {
1183:             g_settings[at] = old; error("ERROR: File I/O error", M_EDIT); return;
1184:         }
1185:         g_mode = at == 6 ? M_BACKUP : M_UNITS;
1186:     }
1187: }
1188: 
1189: static void offer_name(void)
1190: {
1191:     /* the current name is offered; the first character typed replaces it */
1192:     uint32_t i = 0;
1193:     const char *src = g_current[0] ? g_current + 3 : "";   /* skip "A:\" */
1194:     for (; src[i] && i < sizeof g_name - 1u; i++) g_name[i] = src[i];
1195:     g_name[i] = 0;
1196:     g_nlen = i;
1197:     g_fresh = 1;
1198: }
1199: 
1200: /* g_name -> g_path ("NAME", or "DIR\NAME" for a document retrieved from
1201:  * List Files, mint 005 LF11_12) and the shown "A:\NAME"; 0 = refused. */
1202: static int take_name(void)
1203: {
1204:     uint32_t i, dot = 0, base = 0, ext = 0;
1205:     if (g_nlen == 0u) return 0;
1206:     for (i = 0; i < g_nlen; i++) {
1207:         char c = upper(g_name[i]);
1208:         int ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
1209:         if (c == '\\') {
1210:             /* a directory component: 8.3 itself, never empty */
1211:             if (base == 0u || base > 8u || ext > 3u || i + 1u == g_nlen) return 0;
1212:             dot = base = ext = 0; continue;
1213:         }
1214:         if (!ok) return 0;
1215:         if (c == '.') { if (dot) return 0; dot = 1; continue; }
1216:         if (dot) ext++; else base++;
1217:     }
1218:     /* DOS 8.3 (Save p. 564, PDF 582: "up to 8 characters followed by an
1219:      * optional period and up to 3 more characters") */
1220:     if (base == 0u || base > 8u || ext > 3u) return 0;
1221:     for (i = 0; i < g_nlen; i++) g_path[i] = upper(g_name[i]);
1222:     g_path[i] = 0;
1223:     g_shown[0] = 'A'; g_shown[1] = ':'; g_shown[2] = '\\';
1224:     for (i = 0; i <= g_nlen; i++) g_shown[3 + i] = g_path[i];
1225:     return 1;
1226: }
1227: 
1228: static void after_save(void)
1229: {
1230:     scopy(g_current, g_shown);
1231:     g_ed.dirty = 0;
1232:     if (g_exiting) { g_mode = M_EXITQ; return; }
1233:     g_mode = M_EDIT;
1234: }
1235: 
1236: static void write_file(void)
1237: {
1238:     uint32_t n = 0, wn, pos = 0;
1239:     int32_t h, rc = 0, closed;
1240:     /* Flatten BEFORE create can truncate an existing file (the 123.EXE
1241:      * preflight, initech-tdnl.91). */
1242:     if (flatten(&n) != IW_OK) { error("ERROR: Document too large", M_EDIT); return; }
1243:     wn = n;
1244: #ifdef IWORD_MUT_SAVE_NOTHING
1245:     /* MUTANT (Rule 6; test-flair-iword-mutant): the save writes no byte --
1246:      * an empty file is left behind. NEVER real. */
1247:     wn = 0;
1248: #endif
1249:     h = tbx_file_create(g_path);
1250:     if (h < 0) { error("ERROR: File I/O error", M_SAVE_NAME); return; }
1251:     while (pos < wn) {
1252:         rc = tbx_file_write(h, g_bank[1u - g_live] + pos, wn - pos);
1253:         if (rc <= 0 || (uint32_t)rc > wn - pos) break;
1254:         pos += (uint32_t)rc;
1255:     }
1256:     closed = tbx_file_close(h);
1257:     if (pos != wn || closed < 0) {
1258:         /* Appendix E, PDF 763 */
1259:         error("ERROR: Disk Full--Press Any Key to Continue", M_SAVE_NAME);
1260:         return;
1261:     }
1262:     adopt(n, g_prefix_len);          /* the saved bytes are the new original */
1263:     after_save();
1264: }
1265: 
1266: static void save_named(void)
1267: {
1268:     int32_t h;
1269:     if (!take_name()) { error("ERROR: Invalid file name", M_SAVE_NAME); return; }
1270:     h = tbx_file_open(g_path, TBX_FILE_READ_ONLY);
1271:     if (h >= 0) {
1272:         (void)tbx_file_close(h);
1273:         g_mode = M_REPLACE;
1274:         return;
1275:     }
1276:     if (h != TBX_ERR_DOS(2)) { error("ERROR: File I/O error", M_SAVE_NAME); return; }
1277:     write_file();
1278: }
1279: 
1280: /* g_path / g_shown into the document; an error returns to `back` (the
1281:  * Retrieve prompt, or the List Files screen). */
1282: static void retrieve_file(int back)
1283: {
1284:     uint8_t *buf;
1285:     int32_t h, size, rc = 0, closed;
1286:     uint32_t pos = 0;
1287:     iw_wp5_t w;
1288:     /* Into a document the file's text is copied into the add buffer: make
1289:      * the room FIRST, while the spare bank is still free to compact into
1290:      * (the file is read into that bank next). */
1291:     if (g_ed.doc.len != 0u) compact();
1292:     buf = g_bank[1u - g_live];
1293:     h = tbx_file_open(g_path, TBX_FILE_READ_ONLY);
1294:     if (h < 0) { error("ERROR: File Not Found", back); return; }
1295:     size = tbx_file_seek(h, 0, TBX_FILE_FROM_END);
1296:     if (size >= 0) rc = tbx_file_seek(h, 0, TBX_FILE_FROM_START);
1297:     if (size >= 0 && rc >= 0 && (uint32_t)size <= BANK) {
1298:         while (pos < (uint32_t)size) {
1299:             rc = tbx_file_read(h, buf + pos, (uint32_t)size - pos);
1300:             if (rc <= 0 || (uint32_t)rc > (uint32_t)size - pos) break;
1301:             pos += (uint32_t)rc;
1302:         }
1303:     }
1304:     closed = tbx_file_close(h);
1305:     if (size < 0 || (uint32_t)size > BANK) { error("ERROR: Document too large", back); return; }
1306:     if (pos != (uint32_t)size || closed < 0) { error("ERROR: File Not Found", back); return; }
1307:     if (iw_wp5_read(buf, pos, &w) != IW_OK) { error("ERROR: Incompatible File Format", back); return; }
1308:     if (g_ed.doc.len == 0u) {
1309:         /* onto a clear screen: the file IS the document now */
1310:         g_ed.cur = 0;
1311:         g_ed.goal = 0xFFFFFFFFu;
1312:         g_ed.dirty = 0;
1313:         adopt(pos, w.prefix_len);
1314:         g_ed.dirty = 0;
1315:         g_top = 0;
1316:     g_skip = 0;
1317:         scopy(g_current, g_shown);
1318:     } else {
1319:         /* into the document, at the cursor (Retrieve p. 553); the
1320:          * on-screen document keeps its own name and prefix (p. 555) */
1321:         uint32_t at = g_ed.cur;
1322:         if (iw_doc_insert(&g_ed.doc, at, w.body, w.body_len) != IW_OK) {
1323:             error("ERROR: Document too large", back == M_GET_NAME ? M_EDIT : back);
1324:             return;
1325:         }
1326:         (void)iw_reflow_para(&g_ed.doc, at + w.body_len, g_ed.width);
1327:         (void)iw_reflow_para(&g_ed.doc, at, g_ed.width);
1328:         g_ed.dirty = 1;
1329:     }
1330:     g_mode = M_EDIT;
1331: }
1332: 
1333: static void retrieve_named(void)
1334: {
1335:     if (!take_name()) { error("ERROR: Invalid file name", M_GET_NAME); return; }
1336:     retrieve_file(M_GET_NAME);
1337: }
1338: 
1339: /* ---- List Files (F5) and Look -------------------------------------------------
1340:  * bead initech-qvgd (IWORD P3 remainder). Every screen as the REAL WordPerfect
1341:  * 5.1 drew it on a fixed FAT12 volume (../wordperfect51-decomp mint/work/
1342:  * LISTF/mint-results-005.md; the rows composed by os/iword/core/listfiles.c
1343:  * from spec/iword/listfiles.h, then fitted to the 76 cells):
1344:  *   F5            "Dir A:\*.*" + the hint on the status line (LF11_01); the
1345:  *                 default directory is the data volume's root (InitechWord
1346:  *                 has no other). Enter lists it; F1 / Esc back (authored:
1347:  *                 the mint did not press them here). Typing a path and "="
1348:  *                 are not performed (the keys are ignored).
1349:  *   the list      header rows, two entries per row, the menu (LF11_06);
1350:  *                 arrows as WP moves (Right on the right entry goes to the
1351:  *                 next row's left one, LF10 07..11); 6 / Enter on a
1352:  *                 directory, "." or ".." puts "Dir <path>" under the list
1353:  *                 (LF11_05), Enter lists it with ".. Parent" selected
1354:  *                 (LF11_06), F1 returns (LF03_19); 6 / Enter on a file: Look;
1355:  *                 1 on a file: Retrieve (onto an empty document at once,
1356:  *                 LF11_12; else "Retrieve into current document? No (Yes)",
1357:  *                 LF12_03); F7 back to the document (LF03_17); Esc does
1358:  *                 nothing (LF03_16); F1 back to the document (authored).
1359:  *                 2 3 4 5 7 8 9 N: drawn GRAY, ignored (IW_LF_ITEMS).
1360:  *   Look          "File: <path>  WP5.1  Revised: <stamp>" band, the
1361:  *                 document from row 3 as the editing screen wraps it, "Look:
1362:  *                 1 Next Doc; 2 Prev Doc: 0" (LF11_09); Esc and Space back
1363:  *                 to the list (LF11_10, LF04_07), F7 too (Reference PDF p.
1364:  *                 381); Up / Down move the cursor (p. 381 step 3). 1 / 2 (Next
1365:  *                 / Previous Doc) are not performed. A file that is not a 5.1
1366:  *                 document shows its text lines (LF04_06 README.TXT).
1367:  * Hidden entries are listed (LF11_06 HIDDEN.SYS); the find attribute is
1368:  * hidden + system + directory. */
1369: #define LF_MAX 256u
1370: static uint8_t     g_lf_recs[LF_MAX * TBX_FIND_REC_BYTES];
1371: static iw_lf_ent_t g_lf_ent[LF_MAX];
1372: static uint32_t    g_lf_n, g_lf_sel, g_lf_top, g_lf_used, g_lf_files, g_lf_doc, g_lf_free;
1373: static char        g_lf_dir[TBX_FILE_PATH_MAX];    /* "" = the root, or "DOCS\"   */
1374: static char        g_lf_sub[TBX_FILE_PATH_MAX];    /* the directory offered       */
1375: static cp_time_t   g_lf_clock;
1376: static iw_lf_clock_t g_lf_now;
1377: static uint32_t    g_look_n, g_look_cur;
1378: static uint8_t     g_look_wp;
1379: static iw_wp5_t    g_look_w;
1380: static uint16_t    g_look_date, g_look_time;
1381: static char        g_look_path[TBX_FILE_PATH_MAX + 4];
1382: static const uint8_t LF_DEL_HDR1[] = IW_LF_DEL_HDR1;
1383: static const uint8_t LF_DEL_LIST[] = IW_LF_DEL_LIST;
1384: static const uint8_t LF_DEL_PROMPT[] = IW_LF_DEL_PROMPT;
1385: static const uint8_t LF_DEL_LOOK[] = IW_LF_DEL_LOOK;
1386: typedef struct lf_item { char key; uint8_t row, c0, c1, live; } lf_item_t;
1387: static const lf_item_t LF_ITEMS[IW_LF_N_ITEMS] = IW_LF_ITEMS;
1388: 
1389: static int lf_mode(void) { return g_mode >= M_LISTDIR && g_mode <= M_LISTRQ; }
1390: static uint32_t lf_slots(void) { return g_lf_n + 2u; }
1391: static int lf_is_file(uint32_t slot)
1392: {
1393:     return slot >= 2u && !(g_lf_ent[slot - 2u].attr & IW_LF_ATTR_DIR);
1394: }
1395: 
1396: /* scopy of a + b into d (cap TBX_FILE_PATH_MAX); 0 when it does not fit */
1397: static int lf_cat(char *d, const char *a, const char *b)
1398: {
1399:     uint32_t n = slen(a), m = slen(b);
1400:     if (n + m + 1u > TBX_FILE_PATH_MAX) return 0;
1401:     scopy(d, a); scopy(d + n, b);
1402:     return 1;
1403: }
1404: 
1405: /* List `dir` ("" or "DOCS\"), the slot `sel` selected. */
1406: static void lf_open(const char *dir, uint32_t sel)
1407: {
1408:     /* static: the gate refuses automatic buffers (tbx.h, spec Sec 10) */
1409:     static char pat[TBX_FILE_PATH_MAX];
1410:     int32_t n, fr;
1411:     if (!lf_cat(pat, dir, IW_LF_ALL)) { error("ERROR: File Not Found", M_EDIT); return; }
1412:     n = tbx_file_find(pat, IW_LF_FIND_ATTR, g_lf_recs, LF_MAX);
1413:     /* authored: WP's message for a bad directory was not minted */
1414:     if (n < 0) { error("ERROR: File Not Found", M_EDIT); return; }
1415:     g_lf_n = iw_lf_load(g_lf_recs, (uint32_t)n, g_lf_ent, LF_MAX, &g_lf_used, &g_lf_files);
1416:     fr = tbx_disk_free();
1417:     g_lf_free = fr > 0 ? (uint32_t)fr : 0u;
1418:     if (tbx_get_clock(&g_lf_clock) != TBX_OK) { g_lf_clock.year = 1980; g_lf_clock.month = 1; g_lf_clock.day = 1;
1419:                                                g_lf_clock.hour = 0; g_lf_clock.minute = 0; }
1420:     g_lf_now.year = g_lf_clock.year; g_lf_now.month = g_lf_clock.month; g_lf_now.day = g_lf_clock.day;
1421:     g_lf_now.hour = g_lf_clock.hour; g_lf_now.minute = g_lf_clock.minute;
1422:     /* spec IW_LF_DOCSIZE_EXTRA: 0 for an empty document [mint], else the
1423:      * bytes F10 would write + 193 [inferred, three minted points] */
1424:     g_lf_doc = 0;
1425:     if (g_ed.doc.len) { uint32_t fn; if (flatten(&fn) == IW_OK) g_lf_doc = fn + IW_LF_DOCSIZE_EXTRA; }
1426:     scopy(g_lf_dir, dir);
1427:     g_lf_sel = sel < lf_slots() ? sel : 0u;
1428:     g_lf_top = 0;
1429:     g_mode = M_LIST;
1430: }
1431: 
1432: /* the directory slot `slot` names: "." the listed one, ".." its parent */
1433: static int lf_slot_dir(uint32_t slot, char *out)
1434: {
1435:     if (slot == 0u) { scopy(out, g_lf_dir); return 1; }
1436:     if (slot == 1u) {
1437:         uint32_t n = slen(g_lf_dir);
1438:         scopy(out, g_lf_dir);
1439:         if (n) { n--; while (n && out[n - 1u] != '\\') n--; out[n] = 0; }
1440:         return 1;
1441:     }
1442:     return lf_cat(out, g_lf_dir, g_lf_ent[slot - 2u].name) && lf_cat(out, out, "\\");
1443: }
1444: 
1445: /* the selected file's path "DIR\NAME" into g_path and "A:\DIR\NAME" into s */
1446: static int lf_file_path(char *shown)
1447: {
1448:     if (!lf_is_file(g_lf_sel) || !lf_cat(g_path, g_lf_dir, g_lf_ent[g_lf_sel - 2u].name)) return 0;
1449:     scopy(shown, IW_LF_DRIVE);
1450:     scopy(shown + 3, g_path);
1451:     return 1;
1452: }
1453: 
1454: static void lf_retrieve(void)
1455: {
1456:     if (!lf_file_path(g_shown)) { g_mode = M_LIST; return; }
1457:     retrieve_file(M_LIST);
1458: }
1459: 
1460: /* Look: the file into the spare bank (at most a bank; a longer text file
1461:  * shows its first BANK bytes) */
1462: static void lf_look(void)
1463: {
1464:     uint8_t *buf = g_bank[1u - g_live];
1465:     int32_t h, rc = 1;
1466:     uint32_t pos = 0;
1467:     if (!lf_file_path(g_look_path)) return;
1468:     h = tbx_file_open(g_path, TBX_FILE_READ_ONLY);
1469:     if (h < 0) { error("ERROR: File Not Found", M_LIST); return; }
1470:     while (pos < BANK && rc > 0) {
1471:         rc = tbx_file_read(h, buf + pos, BANK - pos);
1472:         if (rc > 0 && (uint32_t)rc <= BANK - pos) pos += (uint32_t)rc; else break;
1473:     }
1474:     (void)tbx_file_close(h);
1475:     g_look_n = pos;
1476:     g_look_wp = iw_wp5_read(buf, pos, &g_look_w) == IW_OK;
1477:     g_look_date = g_lf_ent[g_lf_sel - 2u].date;
1478:     g_look_time = g_lf_ent[g_lf_sel - 2u].time;
1479:     g_look_cur = 0xFFFFFFFFu;                    /* placed at the first paint */
1480:     g_mode = M_LOOK;
1481: }
1482: 
1483: static void lf_key(char ch, uint8_t vk)
1484: {
1485:     uint32_t n = lf_slots();
1486:     char up = upper(ch);
1487:     switch (g_mode) {
1488:     case M_LISTDIR:
1489:         if (vk == VK_ENTER || ch == '\r') { g_lf_dir[0] = 0; lf_open("", 0u); }
1490:         else if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) g_mode = M_EDIT;
1491:         return;
1492:     case M_LISTSUB:
1493:         if (vk == VK_ENTER || ch == '\r') {
1494:             char d[TBX_FILE_PATH_MAX];
1495:             scopy(d, g_lf_sub);
1496:             lf_open(d, 1u);                      /* ".. Parent" selected, LF11_06 */
1497:         } else if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) g_mode = M_LIST;
1498:         return;
1499:     case M_LISTRQ:
1500:         if (up == 'Y') lf_retrieve(); else g_mode = M_LIST;
1501:         return;
1502:     case M_LOOK:
1503:         if (vk == VK_ESC || ch == 0x1B || ch == ' ' || vk == VK_F7) g_mode = M_LIST;
1504:         else if (g_look_wp && (vk == VK_DOWN || vk == VK_UP)) {
1505:             iw_line_t ln[2];
1506:             iw_ed_t saved = g_ed;
1507:             iw_piece_t pc;
1508:             iw_where_t w;
1509:             iw_doc_init(&g_ed.doc, 0, 0, &pc, 1, BANK);
1510:             iw_doc_load(&g_ed.doc, g_look_w.body, g_look_w.body_len);
1511:             g_ed.cur = g_look_cur <= g_look_w.body_len ? g_look_cur : g_look_w.body_len;
1512:             iw_ed_where(&g_ed, &w);
1513:             if (vk == VK_DOWN && iw_ed_lines(&g_ed, w.line + 1u, 1u, ln)) g_look_cur = ln[0].start;
1514:             if (vk == VK_UP && w.line && iw_ed_lines(&g_ed, w.line - 1u, 1u, ln)) g_look_cur = ln[0].start;
1515:             g_ed = saved;
1516:         }
1517:         return;
1518:     default:
1519:         break;
1520:     }
1521:     /* M_LIST */
1522:     if (vk == VK_DOWN) { if (g_lf_sel + 2u < n) g_lf_sel += 2u; }
1523:     else if (vk == VK_UP) { if (g_lf_sel >= 2u) g_lf_sel -= 2u; }
1524:     else if (vk == VK_RIGHT) { if (g_lf_sel + 1u < n) g_lf_sel++; }
1525:     else if (vk == VK_LEFT) { if (g_lf_sel) g_lf_sel--; }
1526:     else if (vk == VK_F7 || vk == VK_F1) { g_mode = M_EDIT; return; }
1527:     else if (ch == '6' || vk == VK_ENTER || ch == '\r') {
1528:         if (lf_is_file(g_lf_sel)) lf_look();
1529:         else if (lf_slot_dir(g_lf_sel, g_lf_sub)) g_mode = M_LISTSUB;
1530:     } else if (ch == '1') {
1531:         if (lf_is_file(g_lf_sel)) { if (g_ed.doc.len) g_mode = M_LISTRQ; else lf_retrieve(); }
1532:     }
1533:     /* keep the selection's row on the screen */
1534:     if (g_lf_sel / 2u < g_lf_top) g_lf_top = g_lf_sel / 2u;
1535:     if (g_lf_sel / 2u >= g_lf_top + IW_LF_FACE_LIST_ROWS) g_lf_top = g_lf_sel / 2u - (IW_LF_FACE_LIST_ROWS - 1u);
1536: }
1537: 
1538: /* a WP row fitted to the cells (`del` of `nd` columns), its bright cells
1539:  * (`bright`: 'B' marks, at WP columns) bold */
1540: static void lf_row(row_t *r, const char *row80, const uint8_t *del, uint32_t nd, const char *bright, uint32_t bcol)
1541: {
1542:     char fit[SCR_COLS + 1];
1543:     row_clear(r);
1544:     (void)iw_lf_fit(row80, del, nd, fit, SCR_COLS);
1545:     for (uint32_t i = 0; i < SCR_COLS; i++) r->t[i] = fit[i];
1546:     for (uint32_t i = 0; bright && bright[i]; i++) {
1547:         uint32_t c;
1548:         if (bright[i] != 'B') continue;
1549:         c = iw_lf_face_col(bcol + i, del, nd);
1550:         if (c < SCR_COLS) r->a[c] = (uint8_t)IW_A_BOLD;
1551:     }
1552: }
1553: static void lf_band(row_t *r) { r->fg = (uint8_t)TBX_COLOR_BLACK; r->bg = (uint8_t)TBX_COLOR_GRAY; }
1554: 
1555: /* the F5 prompt / the Dir prompt row: 'Dir' and the path bright, the cursor
1556:  * on the path's first cell (LF11_01, LF11_05) */
1557: static void lf_prompt_row(row_t *r, const char *dir, int hint)
1558: {
1559:     char row80[IW_LF_WP_COLS + 1], path[TBX_FILE_PATH_MAX + 8];
1560:     uint32_t i, n;
1561:     scopy(path, IW_LF_DRIVE);
1562:     n = slen(path);
1563:     for (i = 0; dir[i] && n + 4u < sizeof path; i++) path[n++] = dir[i];
1564:     scopy(path + n, IW_LF_ALL);
1565:     iw_lf_row_prompt(row80, path, hint);
1566:     lf_row(r, row80, LF_DEL_PROMPT, sizeof LF_DEL_PROMPT, "BBB", 0u);
1567:     for (i = 0; path[i] && IW_LF_PATH_COL + i < SCR_COLS; i++) r->a[IW_LF_PATH_COL + i] = (uint8_t)IW_A_BOLD;
1568:     if (hint) {
1569:         uint32_t c = iw_lf_face_col(IW_LF_HINT_COL + IW_LF_HINT_BRIGHT, LF_DEL_PROMPT, sizeof LF_DEL_PROMPT);
1570:         if (c < SCR_COLS) r->a[c] = (uint8_t)IW_A_BOLD;
1571:     }
1572:     r->inv_col = (uint8_t)IW_LF_PATH_COL; r->inv_len = 1;
1573: }
1574: 
1575: static void paint_look(void)
1576: {
1577:     static row_t r;
1578:     char row80[IW_LF_WP_COLS + 1];
1579:     uint32_t i;
1580:     iw_lf_row_look(row80, g_look_path, g_look_wp, g_look_date, g_look_time);
1581:     lf_row(&r, row80, LF_DEL_LOOK, sizeof LF_DEL_LOOK, 0, 0u);
1582:     lf_band(&r);
1583:     put_row(0, &r);
1584:     put_text_row(1, "");
1585:     put_text_row(2, "");
1586:     if (g_look_wp) {
1587:         /* the document as the editing screen wraps it, from row 3, through
1588:          * a borrowed read-only editor (the M_UNDO preview pattern) */
1589:         iw_ed_t saved = g_ed;
1590:         uint32_t top = g_top, skip = g_skip;
1591:         iw_piece_t pc;
1592:         iw_where_t w;
1593:         static iw_line_t lines[IW_LF_FACE_LOOK_ROWS + 1];
1594:         iw_doc_init(&g_ed.doc, 0, 0, &pc, 1, BANK);
1595:         iw_doc_load(&g_ed.doc, g_look_w.body, g_look_w.body_len);
1596:         g_ed.blk = IW_NO_BLOCK;
1597:         g_ed.cur = g_look_w.body_len;
1598:         if (g_look_cur == 0xFFFFFFFFu) {
1599:             /* "the cursor is at the bottom of the screen" (PDF p. 384): the
1600:              * document's end, or the last row's line of a longer one */
1601:             if (iw_ed_lines(&g_ed, 0, IW_LF_FACE_LOOK_ROWS + 1u, lines) > IW_LF_FACE_LOOK_ROWS)
1602:                 g_ed.cur = lines[IW_LF_FACE_LOOK_ROWS - 1u].start;
1603:             g_look_cur = g_ed.cur;
1604:         } else g_ed.cur = g_look_cur;
1605:         g_top = 0; g_skip = 0;
1606:         iw_ed_where(&g_ed, &w);
1607:         g_row_off = (int)IW_LF_LOOK_BODY_ROW;
1608:         paint_doc(&w, IW_LF_FACE_LOOK_ROWS);
1609:         g_row_off = 0;
1610:         g_ed = saved; g_top = top; g_skip = skip;
1611:     } else {
1612:         /* a text file's lines (LF04_06), CR LF ends a line, cut at 76 */
1613:         const uint8_t *b = g_bank[1u - g_live];
1614:         uint32_t p = 0;
1615:         for (i = 0; i < IW_LF_FACE_LOOK_ROWS; i++) {
1616:             row_clear(&r);
1617:             for (uint32_t c = 0; p < g_look_n && b[p] != '\n'; p++) {
1618:                 if (b[p] == '\r') continue;
1619:                 if (c < SCR_COLS) r.t[c++] = (b[p] >= ' ' && b[p] <= '~') ? (char)b[p] : ' ';
1620:             }
1621:             if (p < g_look_n) p++;
1622:             put_row((int)(IW_LF_LOOK_BODY_ROW + i), &r);
1623:         }
1624:     }
1625:     {
1626:         char m[IW_LF_WP_COLS + 1];
1627:         iw_lf_blank(m);
1628:         for (i = 0; IW_LF_LOOK_MENU[i]; i++) m[i] = IW_LF_LOOK_MENU[i];
1629:         lf_row(&r, m, 0, 0, IW_LF_LOOK_BRIGHT, 0u);
1630:         put_row(LINE_STATUS, &r);
1631:     }
1632: }
1633: 
1634: static void paint_lf(void)
1635: {
1636:     static row_t r;
1637:     char row80[IW_LF_WP_COLS + 1], pat[TBX_FILE_PATH_MAX + 8];
1638:     uint32_t i;
1639:     if (g_mode == M_LISTDIR) {
1640:         iw_where_t w;
1641:         iw_ed_where(&g_ed, &w);
1642:         paint_doc(&w, EDIT_ROWS);
1643:         lf_prompt_row(&r, "", 1);
1644:         put_row(LINE_STATUS, &r);
1645:         return;
1646:     }
1647:     if (g_mode == M_LOOK) { paint_look(); return; }
1648:     scopy(pat, IW_LF_DRIVE);
1649:     (void)lf_cat(pat + 3, g_lf_dir, IW_LF_ALL);
1650:     iw_lf_row_header0(row80, &g_lf_now, pat);
1651:     lf_row(&r, row80, 0, 0, 0, 0u); lf_band(&r); put_row(0, &r);
1652:     iw_lf_row_header1(row80, g_lf_doc, g_lf_free, g_lf_used, g_lf_files);
1653:     lf_row(&r, row80, LF_DEL_HDR1, sizeof LF_DEL_HDR1, 0, 0u); lf_band(&r); put_row(1, &r);
1654:     put_text_row(2, "");
1655:     for (i = 0; i < IW_LF_FACE_LIST_ROWS; i++) {
1656:         uint32_t lr = g_lf_top + i;
1657:         iw_lf_row_list(row80, g_lf_ent, g_lf_n, lr);
1658:         lf_row(&r, row80, LF_DEL_LIST, sizeof LF_DEL_LIST, 0, 0u);
1659:         if (g_lf_sel / 2u == lr) {
1660:             /* the bar over the entry's lead blank + 38 cells (LF11_08) */
1661:             uint32_t w0 = (g_lf_sel & 1u) ? IW_LF_SEL_RIGHT0 : IW_LF_SEL_LEFT0, c0 = SCR_COLS, c1 = 0;
1662:             for (uint32_t c = w0; c < w0 + IW_LF_SEL_W; c++) {
1663:                 uint32_t k;
1664:                 for (k = 0; k < sizeof LF_DEL_LIST && LF_DEL_LIST[k] != c; k++) {}
1665:                 if (k < sizeof LF_DEL_LIST) continue;
1666:                 k = iw_lf_face_col(c, LF_DEL_LIST, sizeof LF_DEL_LIST);
1667:                 if (k < c0) c0 = k;
1668:                 if (k + 1u > c1) c1 = k + 1u;
1669:             }
1670:             if (c1 > SCR_COLS) c1 = SCR_COLS;
1671:             if (c0 < c1) { r.blk_col = (uint8_t)c0; r.blk_len = (uint8_t)(c1 - c0); }
1672:         }
1673:         put_row((int)(IW_LF_LIST_ROW + i), &r);
1674:     }
1675:     if (g_mode == M_LISTSUB || g_mode == M_LISTRQ) {
1676:         put_text_row(LINE_STATUS - 1, "");
1677:         if (g_mode == M_LISTSUB) { lf_prompt_row(&r, g_lf_sub, 0); put_row(LINE_STATUS, &r); }
1678:         else {
1679:             char q[IW_LF_WP_COLS + 1];
1680:             iw_lf_blank(q);
1681:             for (i = 0; IW_LF_INTO_Q[i]; i++) q[i] = IW_LF_INTO_Q[i];
1682:             lf_row(&r, q, 0, 0, IW_LF_INTO_Q_BRIGHT, 0u);
1683:             put_row(LINE_STATUS, &r);
1684:         }
1685:         return;
1686:     }
1687:     for (int m = 1; m <= 2; m++) {
1688:         char row[IW_LF_WP_COLS + 1];
1689:         const char *t = m == 1 ? IW_LF_MENU1 : IW_LF_MENU2;
1690:         iw_lf_blank(row);
1691:         for (i = 0; t[i]; i++) row[i] = t[i];
1692:         lf_row(&r, row, 0, 0, m == 1 ? IW_LF_MENU1_BRIGHT : IW_LF_MENU2_BRIGHT, 0u);
1693:         for (i = 0; i < IW_LF_N_ITEMS; i++)
1694:             if (LF_ITEMS[i].row == (uint8_t)m && !LF_ITEMS[i].live)
1695:                 for (uint32_t c = LF_ITEMS[i].c0; c <= LF_ITEMS[i].c1 && c < SCR_COLS; c++)
1696:                     r.a[c] = (uint8_t)IW_A_GRAY;
1697:         put_row(LINE_STATUS - 2 + m, &r);
1698:     }
1699: }
1700: 
1701: /* ---- the exit guard ------------------------------------------------------------- */
1702: enum { FROM_F7, FROM_MAC };
1703: static void request_exit(int from)
1704: {
1705:     if (g_mode != M_EDIT) return;                 /* one question at a time */
1706:     if (from == FROM_MAC && !g_ed.dirty) { (void)tbx_exit(0); return; }
1707:     g_mode = M_SAVEQ;
1708: }
1709: 
1710: static void clear_document(void)
1711: {
1712:     doc_reset();
1713:     g_current[0] = 0;
1714:     g_mode = M_EDIT;
1715: }
1716: 
1717: /* ---- keys ----------------------------------------------------------------------------- */
1718: static void name_key(char ch, uint8_t vk)
1719: {
1720:     if (vk == VK_ENTER || ch == '\r') {
1721:         if (g_mode == M_SAVE_NAME) save_named(); else retrieve_named();
1722:         return;
1723:     }
1724:     if (vk == VK_BSPC || ch == 8) { g_fresh = 0; if (g_nlen) g_name[--g_nlen] = 0; return; }
1725:     if (ch > ' ' && ch <= '~') {
1726:         if (g_fresh) { g_nlen = 0; g_fresh = 0; }
1727:         if (g_nlen < sizeof g_name - 1u) { g_name[g_nlen++] = ch; g_name[g_nlen] = 0; }
1728:     }
1729: }
1730: 
1731: static void prompt_key(char ch, uint8_t vk)
1732: {
1733:     int font_rc = IW_OK;
1734:     if (g_mode == M_ERROR) { g_mode = g_err_return; return; }
1735:     if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) {   /* Cancel */
1736:         g_exiting = 0;
1737:         g_ed.ask = 0xFFFFFFFFu;                         /* a question is dropped */
1738:         /* Ctrl-F4 1/2/3's highlight goes, the cursor stays at its end
1739:          * (MINT31 13_page_cancel) */
1740:         if (g_sel) { g_ed.blk = IW_NO_BLOCK; g_sel = 0; }
1741:         g_mode = M_EDIT;
1742:         return;
1743:     }
1744:     switch (g_mode) {
1745:     case M_SAVE_NAME:
1746:     case M_GET_NAME:
1747:         name_key(ch, vk);
1748:         break;
1749:     case M_REPLACE:
1750:         if (ch == 'y' || ch == 'Y') write_file();
1751:         else g_mode = M_SAVE_NAME;              /* "prompted for a new filename" */
1752:         break;
1753:     case M_SAVEQ:
1754:         if (ch == 'y' || ch == 'Y' || vk == VK_ENTER || ch == '\r') {
1755:             g_exiting = 1;
1756:             offer_name();
1757:             g_mode = M_SAVE_NAME;
1758:         } else if (ch == 'n' || ch == 'N') {
1759:             g_mode = M_EXITQ;
1760:         }
1761:         break;
1762:     case M_EXITQ:
1763:         if (ch == 'y' || ch == 'Y') { (void)tbx_exit(0); return; }
1764:         if (ch == 'n' || ch == 'N' || vk == VK_ENTER || ch == '\r') { g_exiting = 0; clear_document(); }
1765:         break;
1766:     case M_FONT:
1767:         /* IW_FONT_MENU: 1 Size, 2 Appearance, 3 Normal. Base Font and Print
1768:          * Color (MINT28 25 / 27: their own screens) are a later slice: the
1769:          * key returns to the document, as Cancel does. */
1770:         g_mode = M_EDIT;
1771:         if (ch == '1') g_mode = M_SIZE;
1772:         else if (ch == '2') g_mode = M_APPEAR;
1773:         else if (ch == '3') font_rc = edit(ED_NORMAL, 0);
1774:         break;
1775:     case M_FONTBLK:
1776:         /* Block on: "Attribute: 1 Size; 2 Appearance: 0" (MINT28 07) */
1777:         g_mode = M_EDIT;
1778:         if (ch == '1') g_mode = M_SIZE;
1779:         else if (ch == '2') g_mode = M_APPEAR;
1780:         break;
1781:     case M_SIZE:
1782:     case M_APPEAR: {
1783:         /* every Size / Appearance attribute, its number from the real WP's
1784:          * bytes (MINT28; spec/iword/reveal_codes.h IW_SIZE_ATTRS /
1785:          * IW_APPEAR_ATTRS); with Block on it is given to the block (p. 37) */
1786:         static const uint8_t SZ[7] = IW_SIZE_ATTRS;
1787:         static const uint8_t AP[9] = IW_APPEAR_ATTRS;
1788:         int last = g_mode == M_SIZE ? 7 : 9;
1789:         g_mode = M_EDIT;
1790:         if (ch >= '1' && ch <= (char)('0' + last))
1791:             font_rc = edit(ED_ATTR, last == 7 ? SZ[ch - '1'] : AP[ch - '1']);
1792:         break;
1793:     }
1794:     case M_DELCODE:
1795:         /* "Delete [code]? No (Yes)": y deletes the pair; any other key is No
1796:          * and steps over the code (MINT11; the option not in parentheses is
1797:          * the default, A Brief Lesson p. 21) */
1798:         g_mode = M_EDIT;
1799:         font_rc = edit(ED_ANSWER, (uint8_t)(ch == 'y' || ch == 'Y'));
1800:         break;
1801:     case M_DELBLOCK:
1802:         /* "Delete Block? No (Yes)": y deletes it (MINT12 09/10); No keeps
1803:          * Block on [authored: the shot shows y only] */
1804:         g_mode = M_EDIT;
1805:         if (ch == 'y' || ch == 'Y') font_rc = edit(ED_CUT, 0);
1806:         break;
1807:     case M_MOVE:
1808:         /* 1 Block; Tabular Column and Rectangle are not in this slice */
1809:         g_mode = ch == '1' ? M_MOVEBLK : M_EDIT;
1810:         break;
1811:     case M_MOVENB:
1812:         /* Block off: "Move: 1 Sentence; 2 Paragraph; 3 Page; 4 Retrieve: 0"
1813:          * (MINT31 02): 1..3 highlight the text (iw_ed_select) and offer the
1814:          * block's menu; 4 the Retrieve menu */
1815:         g_mode = M_EDIT;
1816:         if (ch >= '1' && ch <= '3') {
1817:             if (iw_ed_select(&g_ed, ch - '0') == IW_OK) { g_sel = 1; g_mode = M_MOVEBLK; }
1818:         } else if (ch == '4') {
1819:             g_mode = M_RETMENU;
1820:         }
1821:         break;
1822:     case M_RETMENU:
1823:         /* "Retrieve: 1 Block; ..." (MINT29 14): 1 puts the last moved /
1824:          * copied text at the cursor again, the cursor in front (MINT31 15);
1825:          * Tabular Column and Rectangle are not in this slice */
1826:         g_mode = M_EDIT;
1827:         if (ch == '1' && g_clip_n) font_rc = edit(ED_RETRIEVE, 0);
1828:         break;
1829:     case M_MOVEBLK:
1830:         /* 1 Move, 2 Copy: the block goes to the buffer, then the Move prompt;
1831:          * 3 Delete; 4 Append (to a file) is not in this slice */
1832:         g_mode = M_EDIT;
1833:         if (ch == '1') { font_rc = edit(ED_CUT, 1); if (font_rc == IW_OK) g_mode = M_RETRIEVE; }
1834:         else if (ch == '2') { font_rc = edit(ED_COPY, 0); if (font_rc == IW_OK) g_mode = M_RETRIEVE; }
1835:         else if (ch == '3') font_rc = edit(ED_CUT, 0);
1836:         else if (g_sel) g_ed.blk = IW_NO_BLOCK;   /* a selection does not stay */
1837:         g_sel = 0;
1838:         break;
1839:     default:
1840:         g_mode = M_EDIT;
1841:         break;
1842:     }
1843:     if (font_rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
1844: }
1845: 
1846: /* A measurement typed in inches ("2", "1.3", "1.25", an optional '"') ->
1847:  * WPU; 0 when it is not one [authored: fractions like "1 1/2" (Reference
1848:  * Manual p. 394) are not taken]. Hundredths of an inch, 12 WPU each. */
1849: static uint32_t parse_inches(void)
1850: {
1851:     uint32_t i = 0, ip = 0, fr = 0, nf = 0, digits = 0;
1852:     while (i < g_flen && g_field[i] >= '0' && g_field[i] <= '9') { ip = ip * 10u + (uint32_t)(g_field[i] - '0'); i++; digits++; }
1853:     if (i < g_flen && g_field[i] == '.') {
1854:         i++;
1855:         while (i < g_flen && g_field[i] >= '0' && g_field[i] <= '9') {
1856:             if (nf < 2u) { fr = fr * 10u + (uint32_t)(g_field[i] - '0'); nf++; }
1857:             i++; digits++;
1858:         }
1859:     }
1860:     if (i < g_flen && g_field[i] == '"') i++;
1861:     if (i != g_flen || digits == 0u || ip > 8u) return 0;
1862:     if (nf == 1u) fr *= 10u;
1863:     return (ip * 100u + fr) * 12u;
1864: }
1865: 
1866: static void field_start(void)
1867: {
1868:     g_flen = 0;
1869:     g_field[0] = 0;
1870:     g_ftyped = 0;
1871: }
1872: 
1873: /* Shift-F8: the Format screen, Format: Line, its prompt and fields. */
1874: static void format_key(char ch, uint8_t vk)
1875: {
1876:     int rc = IW_OK;
1877:     int cancel = vk == VK_F1 || vk == VK_ESC || ch == 0x1B;
1878:     int enter = vk == VK_ENTER || ch == '\r';
1879:     iw_where_t w;
1880:     switch (g_mode) {
1881:     case M_FORMAT:
1882:         if (ch == '1') g_mode = M_FMTLINE;
1883:         else if (cancel || enter || ch == '0' || vk == VK_F7) g_mode = M_EDIT;
1884:         break;
1885:     case M_FMTLINE:
1886:         if (vk == VK_F7) g_mode = M_EDIT;                    /* Exit (F7) */
1887:         else if (cancel || enter || ch == '0') g_mode = M_FORMAT;
1888:         else if (ch == '3') g_mode = M_FMTJUST;
1889:         else if (ch == '7') { iw_ed_where(&g_ed, &w); g_fleft = w.lm; g_fright = IW_WP5_PAPER_WPU - w.rm; g_mode = M_FMTMARL; field_start(); }
1890:         else if (ch == '6') { g_mode = M_FMTSP; field_start(); }     /* MINT25 01_spacing_field */
1891:         break;
1892:     case M_FMTJUST:
1893:         /* "1 Left; 2 Center; 3 Right; 4 Full" (MINT22 02_just_menu) */
1894:         g_mode = M_FMTLINE;
1895:         if (ch == '1') rc = edit(ED_JUST, IW_WP5_JUST_LEFT);
1896:         else if (ch == '2') rc = edit(ED_JUST, IW_WP5_JUST_CENTER);
1897:         else if (ch == '3') rc = edit(ED_JUST, IW_WP5_JUST_RIGHT);
1898:         else if (ch == '4') rc = edit(ED_JUST, IW_WP5_JUST_FULL);
1899:         break;
1900:     case M_FMTSP:
1901:         /* Line Spacing: a number, Enter (MINT25: "2", "1.5", "1"; MINT30
1902:          * "1.33", "2.25"); the code goes in when one was typed */
1903:         if (cancel) { g_mode = M_FMTLINE; break; }
1904:         if (enter) {
1905:             uint32_t h = g_ftyped ? parse_inches() / 12u : 0u;    /* hundredths */
1906:             g_mode = M_FMTLINE;
1907:             if (h) { g_fsp = iw_spacing_from_hundredths(h); rc = edit(ED_SPACING, 0); }
1908:             break;
1909:         }
1910:         if (vk == VK_BSPC || ch == 8) { if (g_flen) g_field[--g_flen] = 0; break; }
1911:         if (((ch >= '0' && ch <= '9') || ch == '.') && g_flen < 10u) {
1912:             g_field[g_flen++] = ch;
1913:             g_field[g_flen] = 0;
1914:             g_ftyped = 1;
1915:         }
1916:         break;
1917:     case M_FMTMARL:
1918:     case M_FMTMARR:
1919:         if (cancel) { g_mode = M_FMTLINE; break; }
1920:         if (enter) {
1921:             uint32_t v = g_ftyped ? parse_inches() : 0u;
1922:             static uint8_t any;
1923:             if (g_mode == M_FMTMARL) any = 0;
1924:             if (v) { if (g_mode == M_FMTMARL) g_fleft = v; else g_fright = v; }
1925:             if (g_ftyped) any = 1;
1926:             if (g_mode == M_FMTMARL) { g_mode = M_FMTMARR; field_start(); break; }
1927:             /* both entered: the code goes in (when a value was typed) */
1928:             g_mode = M_FMTLINE;
1929:             if (any) rc = edit(ED_MARGINS, 0);
1930:             break;
1931:         }
1932:         if (vk == VK_BSPC || ch == 8) { if (g_flen) g_field[--g_flen] = 0; break; }
1933:         /* "WordPerfect allows ten input characters for margins" (p. 394) */
1934:         if (((ch >= '0' && ch <= '9') || ch == '.' || ch == '"') && g_flen < 10u) {
1935:             g_field[g_flen++] = ch;
1936:             g_field[g_flen] = 0;
1937:             g_ftyped = 1;
1938:         }
1939:         break;
1940:     default:
1941:         g_mode = M_EDIT;
1942:         break;
1943:     }
1944:     if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
1945:     if (g_mode == M_EDIT) paint_all();
1946: }
1947: 
1948: /* The cursor keys (the editing screen, Block, the Move prompt). HOME IS A
1949:  * PREFIX KEY (slice 3; Reference Manual, Cursor Movement p. 103-104; the
1950:  * real WP.EXE in mint/wpMINT27.steps): the arrow after one, two or three
1951:  * Homes says where to go. `homes` = the Homes pressed just before. */
1952: static int cursor_key(uint8_t vk, uint32_t homes)
1953: {
1954:     uint32_t rows = g_reveal ? RV_TOP_ROWS : EDIT_ROWS, first, last;
1955:     iw_where_t w;
1956:     if (vk == VK_HOME) { g_home = (uint8_t)(homes < 3u ? homes + 1u : 3u); return 1; }
1957:     if (homes && (vk == VK_UP || vk == VK_DOWN)) {
1958:         if (homes >= 2u) {
1959:             /* Home, Home, Up: the top after the formatting codes; Home x3,
1960:              * Up before them (MINT27 10 / 12); Home, Home, Down: the end */
1961:             if (vk == VK_UP) iw_ed_doc_top(&g_ed, homes >= 3u);
1962:             else iw_ed_doc_end(&g_ed);
1963:             return 1;
1964:         }
1965:         /* Home, Up / Home, Down: the top / bottom line on the screen, the
1966:          * column kept (MINT27 15 / 16); already there, one screen further
1967:          * [inferred, W-18] */
1968:         iw_ed_where(&g_ed, &w);
1969:         screen_lines(rows, &first, &last);
1970:         if (vk == VK_UP) iw_ed_to_line(&g_ed, w.line != first ? first : (first > rows - 1u ? first - (rows - 1u) : 0u));
1971:         else iw_ed_to_line(&g_ed, w.line != last ? last : last + (rows - 1u));
1972:         return 1;
1973:     }
1974:     if (homes && vk == VK_LEFT) {
1975:         /* Home, Left and Home, Home, Left: past the codes in front of the
1976:          * line's first character; Home x3, Left before them (MINT27 03 / 06
1977:          * / 08) */
1978:         if (homes >= 3u) iw_ed_line_start(&g_ed); else iw_ed_line_home(&g_ed);
1979:         return 1;
1980:     }
1981:     switch (vk) {
1982:     case VK_UP:    iw_ed_up(&g_ed); return 1;
1983:     case VK_DOWN:  iw_ed_down(&g_ed); return 1;
1984:     case VK_LEFT:  iw_ed_left(&g_ed); return 1;
1985:     case VK_RIGHT: if (homes) iw_ed_line_end(&g_ed); else iw_ed_right(&g_ed); return 1;   /* Home, Right: MINT27 04 */
1986:     case VK_END:   iw_ed_line_end(&g_ed); return 1;     /* End = Home, Home, Right */
1987:     default:       return 0;
1988:     }
1989: }
1990: 
1991: static void toggle_reveal(void)
1992: {
1993:     g_reveal = (uint8_t)!g_reveal;
1994:     g_ed.reveal = g_reveal;
1995:     paint_all();
1996: }
1997: 
1998: /* Block on: the cursor keys stretch it; Alt-F4 / F1 / Esc turn it off;
1999:  * Ctrl-F4 the Move menu; Delete / Backspace "Delete Block?"; Alt-F3. */
2000: static void block_key(char ch, uint8_t vk, int32_t mods)
2001: {
2002:     int ctrl = (mods & (int32_t)FLAIR_EVT_MOD_CONTROL_KEY) != 0;
2003:     int alt  = (mods & (int32_t)FLAIR_EVT_MOD_OPTION_KEY) != 0;
2004:     int shift = (mods & (int32_t)FLAIR_EVT_MOD_SHIFT_KEY) != 0;
2005:     int rc = IW_OK;
2006:     if (ch == 0 && cursor_key(vk, g_hin)) return;
2007:     if (vk == VK_F4 && alt) { iw_ed_block(&g_ed); return; }
2008:     if (vk == VK_F4 && ctrl) { g_mode = M_MOVE; return; }
2009:     /* an attribute given to the block (slice 3; p. 37; MINT28): F6, F8,
2010:      * Ctrl-F8 "Attribute: 1 Size; 2 Appearance: 0" */
2011:     if (vk == VK_F6 && !ctrl && !alt && !shift) rc = edit(ED_ATTR, IW_WP5_ATTR_BOLD);
2012:     else if (vk == VK_F8 && !ctrl && !alt && !shift) rc = edit(ED_ATTR, IW_WP5_ATTR_UNDERLINE);
2013:     else if (vk == VK_F8 && ctrl && !alt && !shift) { g_mode = M_FONTBLK; return; }
2014:     if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
2015:     if (vk == VK_F6 || vk == VK_F8) return;
2016:     if (vk == VK_F3 && alt) { toggle_reveal(); return; }
2017:     if (vk == VK_DEL || vk == VK_BSPC || ch == 8) { g_mode = M_DELBLOCK; return; }
2018:     if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) { iw_ed_block(&g_ed); return; }
2019: }
2020: 
2021: /* "Move cursor; press Enter to retrieve.": the cursor keys, Enter; F1 /
2022:  * Esc leave the text unretrieved [authored]. */
2023: static void retrieve_key(char ch, uint8_t vk, int32_t mods)
2024: {
2025:     int alt = (mods & (int32_t)FLAIR_EVT_MOD_OPTION_KEY) != 0;
2026:     int rc;
2027:     if (ch == 0 && cursor_key(vk, g_hin)) return;
2028:     if (vk == VK_F3 && alt) { toggle_reveal(); return; }
2029:     if (vk == VK_ENTER || ch == '\r') {
2030:         g_mode = M_EDIT;
2031:         rc = edit(ED_RETRIEVE, 0);
2032:         if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
2033:         return;
2034:     }
2035:     if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) g_mode = M_EDIT;
2036: }
2037: 
2038: /* All 48 slots enter here; the table controls dispatch, including aliases.
2039:  * Ref: spec/iword/fkeys.json's local Reference citations. No modified F7
2040:  * accidentally falls through to Exit, nor Ctrl/Alt-F10 to Save. */
2041: static int function_key(uint8_t vk, int32_t mods)
2042: {
2043:     uint32_t k, m = 0, count = 0;
2044:     int rc = IW_OK;
2045:     const iw_fkey_t *f;
2046:     if (vk >= VK_F1 && vk <= VK_F10) k = vk - VK_F1;
2047:     else if (vk == 0x57 || vk == 0x58) k = vk - 0x57u + 10u;
2048:     else return 0;
2049:     if (mods & FLAIR_EVT_MOD_SHIFT_KEY) { m = 1; count++; }
2050:     if (mods & FLAIR_EVT_MOD_OPTION_KEY) { m = 2; count++; }
2051:     if (mods & FLAIR_EVT_MOD_CONTROL_KEY) { m = 3; count++; }
2052:     if (count > 1) return 1;   /* outside the four rows of the WP template */
2053: #ifdef IWORD_MUT_FKEY_DROP
2054:     if (m == 0 && k == 8) return 1; /* named mutant: drop plain F9 */
2055: #endif
2056:     f = &IW_FKEYS[m][k];
2057:     switch (f->action) {
2058:     case IW_FK_setup: g_mode = M_SETUP; break;
2059:     case IW_FK_undelete:
2060:         g_undo_sel = 0; g_undo_join = 0; g_mode = M_UNDO; break;
2061:     case IW_FK_replace:
2062:         g_search_back = 0; g_confirm = 0; g_mode = M_RCONF; break;
2063:     case IW_FK_search_forward: case IW_FK_search_backward:
2064:         g_search_back = f->action == IW_FK_search_backward;
2065:         g_query_cur = g_query_n; g_query_fresh = 1; g_mode = M_SEARCH; break;
2066:     case IW_FK_indent:    rc = edit(ED_INDENT, 0); break;
2067:     case IW_FK_bold:      rc = edit(ED_ATTR, IW_WP5_ATTR_BOLD); break;
2068:     case IW_FK_underline: rc = edit(ED_ATTR, IW_WP5_ATTR_UNDERLINE); break;
2069:     case IW_FK_exit:      request_exit(FROM_F7); break;
2070:     case IW_FK_save:      g_exiting = 0; offer_name(); g_mode = M_SAVE_NAME; break;
2071:     case IW_FK_retrieve:  g_exiting = 0; g_nlen = 0; g_name[0] = 0; g_fresh = 0; g_mode = M_GET_NAME; break;
2072:     case IW_FK_reveal:    toggle_reveal(); break;
2073:     case IW_FK_block:     iw_ed_block(&g_ed); break;
2074:     case IW_FK_format:    g_mode = M_FORMAT; break;
2075:     case IW_FK_move:      g_mode = M_MOVENB; break;
2076:     case IW_FK_font:      g_mode = M_FONT; break;
2077:     case IW_FK_list:      g_mode = M_LISTDIR; break;   /* List Files (F5), mint 005 */
2078:     case IW_FK_unmapped:
2079:         /* Reference p. 966 note 4: no command is mapped to these keys.
2080:          * Keep the editing screen/status, rather than inventing a blank
2081:          * disabled command. */
2082: #ifdef IWORD_MUT_FKEY_UNMAPPED
2083:         g_gray = f->name; g_mode = M_GRAY;
2084: #endif
2085:         break;
2086:     default:             g_gray = f->name; g_mode = M_GRAY; break;
2087:     }
2088:     if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
2089:     return 1;
2090: }
2091: 
2092: static void edit_key(char ch, uint8_t vk, int32_t mods)
2093: {
2094:     int ctrl  = (mods & (int32_t)FLAIR_EVT_MOD_CONTROL_KEY) != 0;
2095:     int rc = IW_OK;
2096:     if (g_ed.blk != IW_NO_BLOCK) {
2097:         /* p. 966 note 3: the aliases work in Block as well. */
2098:         int chord = mods & (FLAIR_EVT_MOD_SHIFT_KEY | FLAIR_EVT_MOD_CONTROL_KEY | FLAIR_EVT_MOD_OPTION_KEY);
2099:         if (vk == 0x57 && !chord) { vk = VK_F3; mods |= FLAIR_EVT_MOD_OPTION_KEY; }
2100:         if (vk == 0x58 && !chord) { vk = VK_F4; mods |= FLAIR_EVT_MOD_OPTION_KEY; }
2101:         block_key(ch, vk, mods); return;
2102:     }
2103:     if (!ch && function_key(vk, mods)) return;
2104:     if (ch == '\t' || vk == VK_TAB) {   /* Tab: [Tab] (MINT09) */
2105:         rc = edit(ED_TAB, 0);
2106:     } else if (ch == 0) {                /* function and cursor keys */
2107:         switch (vk) {
2108:         case VK_DEL:   rc = edit(ED_DELETE, 0); break;
2109:         default:       (void)cursor_key(vk, g_hin); break;
2110:         }
2111:     } else if (ch == '\r' || ch == '\n' || vk == VK_ENTER) {
2112:         rc = edit(ctrl ? ED_HPAGE : ED_ENTER, 0);      /* Ctrl-Enter: hard page */
2113:     } else if (ch == 8 || vk == VK_BSPC) {
2114:         rc = edit(ED_BACKSPACE, 0);
2115:     } else if (ch >= ' ' && ch <= '~') {
2116:         rc = edit(ED_TYPE, (uint8_t)ch);
2117:     }
2118:     if (rc == IW_E_ASK) g_mode = M_DELCODE;             /* "Delete [code]?" */
2119:     if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
2120: }
2121: 
2122: /* A borrowed read-only view over the query. Never uses the document's add
2123:  * buffer, pairing, fixup, dirty flag or reflow. Reference pp. 572-573. */
2124: static void query_view(iw_doc_t *q, iw_piece_t *pc)
2125: {
2126:     iw_doc_init(q, 0, 0, pc, 1, QUERY_CAP);
2127:     iw_doc_load(q, g_query, g_query_n);
2128: }
2129: 
2130: static uint32_t query_label(char *out, uint32_t cap)
2131: {
2132:     iw_doc_t q; iw_piece_t pc;
2133:     /* Native 76-column adaptation of the Reference's editable Search field
2134:      * (p. 572): scroll the query to keep the insertion cell in view. Labels
2135:      * and cursor use complete units; no code's bytes become text. */
2136:     static char text[QUERY_CAP * 16];
2137:     uint32_t p = 0, n = 0, cur = 0, left;
2138:     query_view(&q, &pc);
2139:     while (p < q.len && n + 48 < sizeof text) {
2140:         if (p == g_query_cur) cur = n;
2141:         if (iw_unit_kind(&q, p) == IW_U_TEXT) text[n++] = (char)iw_doc_byte(&q, p);
2142:         else {
2143:             char lab[48];
2144:             (void)iw_code_label(&q, p, lab, sizeof lab);
2145:             for (uint32_t i = 0; lab[i]; i++) text[n++] = lab[i];
2146:         }
2147:         p = iw_unit_next(&q, p);
2148:     }
2149:     if (g_query_cur == q.len) cur = n;
2150:     left = cur >= cap - 1 ? cur - (cap - 2) : 0;
2151:     for (uint32_t i = left; i < n && i - left + 1 < cap; i++) out[i-left] = text[i];
2152:     return cur - left;
2153: }
2154: 
2155: static void query_insert(const uint8_t *s, uint32_t n)
2156: {
2157:     if (g_query_fresh) { g_query_n = g_query_cur = 0; g_query_fresh = 0; }
2158:     if (g_query_n + n > QUERY_CAP) return;
2159:     for (uint32_t i = g_query_n; i > g_query_cur; i--) g_query[i + n - 1] = g_query[i - 1];
2160:     for (uint32_t i = 0; i < n; i++) g_query[g_query_cur + i] = s[i];
2161:     g_query_cur += n; g_query_n += n;
2162: }
2163: 
2164: /* One complete span. The old P2 model remains the sole byte-format writer:
2165:  * reflow and fixup recompute position-bearing codes (MINT09..32). Lowercase
2166:  * replacement preserves the found word's case (Reference p. 552). */
2167: static int replace_match(void)
2168: {
2169:     uint8_t r[QUERY_CAP];
2170:     uint32_t letters = 0, capitals = 0, first = 0, lower = 1;
2171:     for (uint32_t i = 0; i < g_find_n; i++)
2172:         if (g_find[i] >= 'A' && g_find[i] <= 'Z') lower = 0;
2173:     for (uint32_t i = 0; i < g_replacement_n; i++) {
2174:         r[i] = g_replacement[i];
2175:         if (r[i] >= 'A' && r[i] <= 'Z') lower = 0;
2176:     }
2177:     for (uint32_t p = g_rs; p < g_re; p = iw_unit_next(&g_ed.doc, p)) {
2178:         int c = iw_doc_byte(&g_ed.doc, p);
2179:         if (iw_unit_kind(&g_ed.doc, p) == IW_U_TEXT &&
2180:             ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
2181:             if (!letters) first = c >= 'A' && c <= 'Z';
2182:             letters++; capitals += c >= 'A' && c <= 'Z';
2183:         }
2184:     }
2185: #ifndef IWORD_MUT_REPLACE_CASE
2186:     if (lower && letters) {
2187:         int initial = 1;
2188:         for (uint32_t p = 0; p < g_replacement_n; ) {
2189:             uint32_t n = iw_unit_len(r, g_replacement_n, p);
2190:             if (n == 1 && r[p] >= 'a' && r[p] <= 'z') {
2191:                 if (capitals == letters || (initial && first)) r[p] -= 32;
2192:                 initial = 0;
2193:             }
2194:             p += n;
2195:         }
2196:     }
2197: #else
2198:     (void)lower; (void)letters; (void)capitals; (void)first;
2199: #endif
2200:     ensure_room();
2201:     /* Refuse BEFORE the delete if an insert cannot fit. The small fixed
2202:      * query cap and P2's room reserve also leave room for code fixup. */
2203:     if (g_ed.doc.len - (g_re-g_rs) + g_replacement_n > g_ed.doc.max_len ||
2204:         g_ed.doc.add_cap - g_ed.doc.add_len < g_replacement_n ||
2205:         g_ed.doc.pc_cap - g_ed.doc.npc < 4u) return IW_E_FULL;
2206: #ifndef IWORD_MUT_REPLACE_CODE
2207:     (void)iw_doc_delete(&g_ed.doc, g_rs, g_re-g_rs);
2208: #else
2209:     if (g_find[0] >= 0xC0) { g_ed.cur = g_re; return IW_OK; }
2210:     (void)iw_doc_delete(&g_ed.doc, g_rs, g_re-g_rs);
2211: #endif
2212:     if (g_replacement_n && iw_doc_insert(&g_ed.doc, g_rs, r, g_replacement_n) != IW_OK) return IW_E_FULL;
2213:     g_ed.cur = g_search_back ? g_rs : g_rs + g_replacement_n;
2214: #ifdef IWORD_MUT_REPLACE_SKIP
2215:     if (!g_search_back && g_ed.cur < g_ed.doc.len) g_ed.cur = iw_unit_next(&g_ed.doc, g_ed.cur);
2216: #endif
2217:     g_ed.blk = IW_NO_BLOCK; g_ed.dirty = 1; g_ed.goal = 0xFFFFFFFFu;
2218:     (void)iw_reflow_all(&g_ed.doc, g_ed.width);
2219:     return iw_ed_fixup(&g_ed);
2220: }
2221: 
2222: static void replace_next(void)
2223: {
2224:     iw_doc_t q; iw_piece_t pc;
2225:     iw_doc_init(&q, 0, 0, &pc, 1, QUERY_CAP); iw_doc_load(&q, g_find, g_find_n);
2226:     g_ed.blk = IW_NO_BLOCK;
2227:     while (iw_search(&g_ed.doc, &q, g_ed.cur, g_search_back, &g_rs, &g_re) == IW_OK) {
2228:         if (g_confirm) { g_ed.blk = g_rs; g_ed.cur = g_re; g_mode = M_RASK; return; }
2229:         if (replace_match() != IW_OK) { error("ERROR: Document too large", M_EDIT); return; }
2230:     }
2231:     error("* Not found *", M_EDIT);
2232: }
2233: 
2234: static void search_key(char ch, uint8_t vk, int32_t mods)
2235: {
2236:     iw_doc_t q; iw_piece_t pc;
2237:     uint32_t p, n, s, e;
2238:     int plain = !(mods & (FLAIR_EVT_MOD_SHIFT_KEY | FLAIR_EVT_MOD_OPTION_KEY | FLAIR_EVT_MOD_CONTROL_KEY));
2239:     query_view(&q, &pc);
2240:     if (vk == VK_F1) {
2241: #ifdef IWORD_MUT_SEARCH_CANCEL
2242:         g_ed.cur = 0;
2243: #endif
2244:         g_mode = M_EDIT; return;
2245:     }
2246:     if (vk == VK_F2 || vk == VK_ESC || ch == 0x1B) {
2247:         if (g_mode == M_RSEARCH) {
2248:             if (!g_query_n) { g_mode = M_EDIT; return; }
2249:             g_find_n = g_query_n;
2250:             for (uint32_t i = 0; i < g_find_n; i++) g_find[i] = g_query[i];
2251:             g_query_n = g_query_cur = 0; g_query_fresh = 0; g_mode = M_RTEXT; return;
2252:         }
2253:         if (g_mode == M_RTEXT) {
2254:             g_replacement_n = g_query_n;
2255:             for (uint32_t i = 0; i < g_replacement_n; i++) g_replacement[i] = g_query[i];
2256:             /* Retain the search string for F2/F2, never the replacement. */
2257:             g_query_n = g_find_n; g_query_cur = g_query_n;
2258:             for (uint32_t i = 0; i < g_find_n; i++) g_query[i] = g_find[i];
2259:             replace_next(); return;
2260:         }
2261:         if (iw_search(&g_ed.doc, &q, g_ed.cur, g_search_back, &s, &e) == IW_OK) {
2262:             g_ed.cur = g_search_back ? s : e; g_ed.goal = 0xFFFFFFFFu; g_mode = M_EDIT;
2263:         } else {
2264: #ifdef IWORD_MUT_SEARCH_NOTFOUND
2265:             error("Not found", M_EDIT);
2266: #else
2267:             error("* Not found *", M_EDIT);
2268: #endif
2269:         }
2270:         return;
2271:     }
2272:     if ((vk == VK_UP || vk == VK_DOWN) && g_mode != M_RTEXT) { g_search_back = vk == VK_UP; return; }
2273:     if (vk == VK_LEFT || vk == VK_RIGHT) {
2274:         g_query_fresh = 0;
2275:         if (vk == VK_LEFT && g_query_cur) g_query_cur = iw_unit_prev(&q, g_query_cur);
2276:         if (vk == VK_RIGHT && g_query_cur < q.len) g_query_cur = iw_unit_next(&q, g_query_cur);
2277:         return;
2278:     }
2279:     if (vk == VK_BSPC || ch == 8 || vk == VK_DEL) {
2280:         g_query_fresh = 0;
2281:         p = g_query_cur;
2282:         if (vk != VK_DEL) { if (!p) return; p = iw_unit_prev(&q, p); }
2283:         if (p == q.len) return;
2284:         n = iw_unit_next(&q, p) - p;
2285:         for (uint32_t i = p; i + n < g_query_n; i++) g_query[i] = g_query[i + n];
2286:         g_query_n -= n; g_query_cur = p; return;
2287:     }
2288:     /* Reference p. 553 allows Tab, Indent and Hard Page in replacement.
2289:      * Their position-bearing payloads are the existing MINT09/P2 codes;
2290:      * replace_match's fixup fills the coordinates at the destination. */
2291:     if (vk == VK_TAB || ch == '\t') {
2292:         static const uint8_t tab[9] = {0xC1,2,0,0,0,0,0,0,0xC1};
2293:         query_insert(tab, sizeof tab); return;
2294:     }
2295:     if (vk == VK_F4 && plain) {
2296:         static const uint8_t indent[11] = {0xC2,0,0x58,2,0,0,0,0,0,0,0xC2};
2297:         query_insert(indent, sizeof indent); return;
2298:     }
2299:     if (vk == VK_ENTER && (mods & FLAIR_EVT_MOD_CONTROL_KEY)) {
2300:         uint8_t pg = IW_WP5_HPG; query_insert(&pg, 1); return;
2301:     }
2302:     /* Appearance in replacement text needs WP's Alt-R macro (p. 553),
2303:      * which P3 does not claim. Codes ARE searchable/deletable as tokens. */
2304:     if (plain && (vk == VK_F6 || vk == VK_F8) && g_mode != M_RTEXT) {
2305:         uint8_t attr = vk == VK_F6 ? IW_WP5_ATTR_BOLD : IW_WP5_ATTR_UNDERLINE;
2306:         uint8_t code[3];
2307:         uint32_t state = g_query_fresh ? 0 : iw_attr_state(&q, g_query_cur);
2308:         code[0] = code[2] = (state & (1u << attr)) ? 0xC4 : 0xC3; code[1] = attr;
2309:         query_insert(code, 3); return;
2310:     }
2311:     if (ch >= ' ' && ch <= '~') { uint8_t c = (uint8_t)ch; query_insert(&c, 1); }
2312:     else if (vk == VK_ENTER) { uint8_t c = 0x0A; query_insert(&c, 1); }
2313:     else if (ch == 13 && (mods & FLAIR_EVT_MOD_CONTROL_KEY)) { uint8_t c = 0x0D; query_insert(&c, 1); }
2314: }
2315: 
2316: static void replace_key(char ch, uint8_t vk)
2317: {
2318:     if (vk == VK_F1) { g_ed.blk = IW_NO_BLOCK; g_mode = M_EDIT; return; }
2319:     if (g_mode == M_RCONF) {
2320:         if (ch != 'y' && ch != 'Y' && ch != 'n' && ch != 'N' && vk != VK_ENTER) return;
2321:         g_confirm = ch == 'y' || ch == 'Y';
2322:         g_query_n = g_query_cur = 0; g_query_fresh = 0; g_mode = M_RSEARCH; return;
2323:     }
2324:     if (ch == 'y' || ch == 'Y' || ch == 'n' || ch == 'N' || vk == VK_ENTER) {
2325:         int yes = ch != 'n' && ch != 'N';
2326: #ifdef IWORD_MUT_REPLACE_CONFIRM
2327:         yes = 1;
2328: #endif
2329:         g_ed.blk = IW_NO_BLOCK;
2330:         if (yes) {
2331:             if (replace_match() != IW_OK) { error("ERROR: Document too large", M_EDIT); return; }
2332:         } else g_ed.cur = g_search_back ? g_rs : g_re;
2333:         replace_next();
2334:     }
2335: }
2336: 
2337: static void undo_key(char ch, uint8_t vk)
2338: {
2339:     int restore = ch == '1';
2340:     if (vk == VK_F1 || vk == VK_ESC) {
2341: #ifdef IWORD_MUT_UNDO_CANCEL
2342:         restore = 1;
2343: #else
2344:         g_mode = M_EDIT; return;
2345: #endif
2346:     }
2347:     if (restore) {
2348:         g_mode = M_EDIT;
2349:         if (g_undo_count) {
2350:             ensure_room();
2351:             int rc = iw_ed_retrieve(&g_ed, g_undo[g_undo_sel], g_undo_n[g_undo_sel]);
2352: #ifndef IWORD_MUT_UNDO_FIXUP
2353:             if (rc == IW_OK) rc = iw_ed_fixup(&g_ed);
2354: #endif
2355:             if (rc != IW_OK) error("ERROR: Document too large", M_EDIT);
2356:         }
2357:     } else if ((ch == '2' || vk == VK_UP) && g_undo_sel+1 < g_undo_count) g_undo_sel++;
2358:     else if (vk == VK_DOWN && g_undo_sel) g_undo_sel--;
2359: }
2360: 
2361: static void key(int32_t message, int32_t mods)
2362: {
2363:     char ch = (char)(message & 0xFF);
2364:     uint8_t vk = (uint8_t)((message >> 8) & 0xFF);
2365:     uint32_t cur_before = g_ed.cur;
2366:     int mode_before = g_mode;
2367:     g_caps = (mods & (int32_t)FLAIR_EVT_MOD_ALPHA_LOCK) != 0;
2368:     g_ed.reveal = g_reveal;                       /* a new document resets it */
2369:     g_hin = g_home;                               /* Home is a prefix: it waits */
2370:     g_home = 0;                                   /* for the next key (slice 3) */
2371:     if (vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN || vk == VK_HOME || vk == VK_END)
2372:         g_undo_join = 0;
2373:     if (g_mode == M_GRAY) g_mode = M_EDIT;
2374:     if (g_mode == M_EDIT) edit_key(ch, vk, mods);
2375:     else if (g_mode == M_SEARCH || g_mode == M_RSEARCH || g_mode == M_RTEXT) search_key(ch, vk, mods);
2376:     else if (g_mode == M_RCONF || g_mode == M_RASK) replace_key(ch, vk);
2377:     else if (g_mode == M_UNDO) undo_key(ch, vk);
2378:     else if (setup_mode()) setup_key(ch, vk);
2379:     else if (format_mode()) format_key(ch, vk);
2380:     else if (g_mode == M_RETRIEVE) retrieve_key(ch, vk, mods);
2381:     else if (lf_mode()) lf_key(ch, vk);
2382:     else prompt_key(ch, vk);
2383:     /* Reference p. 697: ANY cursor motion ends a deletion group, including
2384:      * Search. The motion caused by the deletion itself remains in its group. */
2385: #ifndef IWORD_MUT_UNDO_MOVEMENT
2386:     if (g_ed.cur != cur_before &&
2387:         !((mode_before == M_EDIT || mode_before == M_GRAY) && (vk == VK_DEL || vk == VK_BSPC || ch == 8)) &&
2388:         !((mode_before == M_DELCODE || mode_before == M_DELBLOCK) && (ch == 'y' || ch == 'Y')) &&
2389:         !(mode_before == M_MOVEBLK && ch == '3')) g_undo_join = 0;
2390: #else
2391:     (void)cur_before; (void)mode_before;
2392: #endif
2393:     paint();
2394: }
2395: 
2396: /* ---- the application ---------------------------------------------------------------- */
2397: int tbx_app_main(void)
2398: {
2399:     if (tbx_register() != TBX_OK) return 1;
2400:     doc_reset();
2401:     settings_load();
2402:     (void)tbx_set_mbar(&mbar);
2403:     g_win = tbx_new_window(WIN_L, WIN_T, WIN_R, WIN_B, (int32_t)TBX_GOAWAY_ASK);
2404:     if (g_win <= 0) {
2405:         (void)tbx_exit(1);
2406:         return 1;
2407:     }
2408:     (void)tbx_set_wtitle(g_win, "InitechWord");
2409:     return 0;
2410: }
2411: 
2412: void tbx_app_event(const tbx_event_t *ev)
2413: {
2414:     switch (ev->what) {
2415:     case updateEvt:
2416:         paint_all();
2417:         break;
2418:     case keyDown:
2419:     case autoKey:
2420:         key(ev->message, ev->modifiers);
2421:         break;
2422:     case TBX_EVT_MENU:
2423:         if (g_mode == M_EDIT) {
2424:             if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_EXIT)) request_exit(FROM_MAC);
2425:             else if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_SAVE)) { offer_name(); g_exiting = 0; g_mode = M_SAVE_NAME; }
2426:             else if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_RETRIEVE)) { g_nlen = 0; g_name[0] = 0; g_mode = M_GET_NAME; }
2427:             paint();
2428:         }
2429:         break;
2430:     case TBX_EVT_GOAWAY:
2431:         request_exit(FROM_MAC);
2432:         paint();
2433:         break;
2434:     default:
2435:         break;
2436:     }
2437: }
