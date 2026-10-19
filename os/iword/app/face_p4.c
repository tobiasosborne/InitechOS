/*
 * os/iword/app/face_p4.c -- WORD.EXE's P4 screens: Format: Page with the
 * headers and footers, the footnote / endnote menus and note screens,
 * Columns/Table, the Table Edit screen, and Print (THE ARTIFACT; bead
 * initech-8q6j, IWORD P4). Included by face_a.c (one translation unit: the
 * screen state is face_a.c's).
 *
 * Every screen, menu line and prompt is transcribed from the real WP 5.1
 * (03-09-92) in ../wordperfect51-decomp (spec/iword/wp5_page.h names each
 * shot); the behaviour is the WordPerfect 5.1 Reference's:
 *   Shift-F8 2 Format: Page   (MINT33 01_format_page; Headers and Footers
 *                              pp. 269-274): 3 Headers / 4 Footers, then
 *                              "1 Header A; 2 Header B: 0", then "1
 *                              Discontinue; 2 Every Page; 3 Odd Pages; 4 Even
 *                              Pages; 5 Edit: 0"; 2/3/4 open the header screen
 *                              ("Header A:  Press Exit when done", MINT33
 *                              05_header_typed), F7 puts the code in and comes
 *                              back to Format: Page (06_after_exit1 "HA Every
 *                              page"), 5 Edit finds the definition (p. 273)
 *                              and opens its text, 1 Discontinue puts that
 *                              code in at once (MINT35 04_after_discontinue).
 *   Ctrl-F7 Footnote          (MINT36 01..07; pp. 209-212): "1 Footnote; 2
 *                              Endnote; 3 Endnote Placement: 0", then
 *                              "Footnote: 1 Create; 2 Edit; 3 New Number; 4
 *                              Options: 0"; 1 Create opens the note screen
 *                              ("     1" + the text; "Footnote:  Press Exit
 *                              when done"), F7 puts the code in.
 *   Alt-F7 Columns/Table      (MINT37 01..05, MINT38 01..05; pp. 76-79,
 *                              640-643): "1 Columns; 2 Tables; 3 Math: 0";
 *                              Columns: "Columns: 1 On; 2 Off; 3 Define: 0",
 *                              3 the Text Column Definition screen (the
 *                              default two newspaper columns), F7 back to the
 *                              Columns menu with the [Col Def] in; Tables:
 *                              "Table: 1 Create; 2 Edit: 0", "Number of
 *                              Columns: 3", "Number of Rows: 1", then the Table
 *                              Edit screen; F7 to the document, the cursor in
 *                              cell A1. In a table Tab / Shift-Tab move to the
 *                              next / previous cell (p. 642).
 *   Shift-F7 Print            (shots_wpPRINT0, PRINT1; pp. 490-494, 505-509,
 *                              543): the Print screen; S Select Printer (the
 *                              Standard Printer, the one definition the tree
 *                              has), 3 Edit, 2 Port: "Port: 1 LPT 1; ... 8
 *                              Other: 0", 8 "Device or Filename:"; 1 Full
 *                              Document prints to that file. LPT1 is a device:
 *                              the Toolbox Gate refuses device names to a disk
 *                              application (spec/toolbox_gate.h Sec 10), so a
 *                              print to LPT1 fails -- "PC LOAD LETTER", the
 *                              InitechOS canon for a print-path failure (bead
 *                              initech-8q6j acceptance).
 * REDUCTIONS (stated): the header / note screens take characters, Enter,
 * Backspace (and Ctrl-B, the page number, in a header) at their end -- not the
 * full editor WP gives them; the other Format: Page, footnote, column, table
 * and print options are not live (their keys are ignored); the columns show
 * one after the other as with Setup's "Side-by-side Column Display: No"
 * (Reference p. 74), not side by side; a table cell holds one line.
 *
 * Freestanding C (no libc). Deterministic (Rule 11). ASCII-clean (Rule 12).
 */

typedef struct p4_row { uint8_t row, col; const char *text; } p4_row_t;
static const p4_row_t PG_MENU[IW_PG_MENU_ROWS] = IW_PG_MENU_INIT;
static const p4_row_t COLDEF_ROWS[IW_COLDEF_ROWS] = IW_COLDEF_INIT;
static const p4_row_t PRINT_ROWS[IW_PRINT_ROWS] = IW_PRINT_INIT;
static const p4_row_t PEDIT_ROWS[IW_PEDIT_ROWS] = IW_PEDIT_INIT;

#define SUB_CAP 1024u
static uint8_t  g_sub[SUB_CAP];            /* the header / note screen's text */
static uint32_t g_sub_n;
static uint32_t g_hf_which;                /* IW_HF_* the menus are about    */
static uint32_t g_hf_pos;                  /* Edit: the definition, else NONE */
static uint32_t g_note_kind;
static char     g_tfield[8];               /* Number of Columns / Rows field */
static uint32_t g_tflen, g_tcols;
static uint8_t  g_tfresh;
static char     g_port[TBX_FILE_PATH_MAX] TBX_IN_IMAGE = "LPT1:";
static uint8_t  g_port_file;               /* the port is a file name        */
static char     g_pfield[TBX_FILE_PATH_MAX];
static uint32_t g_pflen;
static int32_t  g_prn_h;                   /* the print file's handle        */
#define P4_NONE 0xFFFFFFFFu

static int p4_mode(void)
{
    return g_mode >= M_FMTPAGE && g_mode <= M_PRNPORTFILE;
}

/* The full-screen P4 screens (the others are status-line menus over the
 * document). */
static int p4_full(void)
{
    return g_mode == M_FMTPAGE || g_mode == M_HFMENU || g_mode == M_HFPLACE || g_mode == M_HFEDIT ||
           g_mode == M_NOTEEDIT || g_mode == M_COLDEF || g_mode == M_TBLEDIT || g_mode == M_PRINT ||
           g_mode == M_SELPRN || g_mode == M_PEDIT || g_mode == M_PRNPORT || g_mode == M_PRNPORTFILE;
}

/* ---- painting -------------------------------------------------------------- */
static void p4_rows(row_t *rows, const p4_row_t *t, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < SCR_LINES; i++) row_clear(&rows[i]);
    for (i = 0; i < n; i++) if (t[i].text && t[i].text[0]) put_at(&rows[t[i].row], t[i].col, t[i].text);
}

static void p4_bottom(row_t *rows, const char *t, int cursor_on_last)
{
    uint32_t n = slen(t);
    put_at(&rows[SCR_LINES - 1], 0, t);
    if (cursor_on_last && n) { rows[SCR_LINES - 1].inv_col = (uint8_t)(n - 1u); rows[SCR_LINES - 1].inv_len = 1; }
}

/* "HA Every page, HB Even pages" (MINT33 09_format_page_after) */
static void hf_value(row_t *r, const iw_hfstate_t *s, uint32_t first)
{
    static const char *const SHORT[4] = IW_HF_SHORT;
    char v[64];
    uint32_t n = 0, i, k;
    for (i = first; i < first + 2u; i++) {
        const char *occ;
        if (!s->occ[i]) continue;
        occ = s->occ[i] == IW_HF_ODD ? IW_HF_OCC_LABEL_ODD : s->occ[i] == IW_HF_EVEN ? IW_HF_OCC_LABEL_EVEN
                                                                                    : IW_HF_OCC_LABEL_EVERY;
        if (n) { v[n++] = ','; v[n++] = ' '; }
        for (k = 0; SHORT[i][k]; k++) v[n++] = SHORT[i][k];
        v[n++] = ' ';
        for (k = 0; occ[k]; k++) v[n++] = occ[k];
    }
    v[n] = 0;
    put_at(r, IW_PG_VALUE_COL, v);
}

/* The header / note screen: the text from row 0 (each [HRt] a new row, ^B as
 * "^B"), the cursor after it, the screen's own status line. */
static void paint_sub(row_t *rows)
{
    static const char *const NAMES[4] = IW_HF_NAMES;
    uint32_t i, r = 0, c = 0;
    char st[SCR_COLS + 1], num[16];
    for (i = 0; i < SCR_LINES; i++) row_clear(&rows[i]);
    if (g_mode == M_NOTEEDIT) {
        /* "     1First note." / "1." (MINT36 04_note_typed, 07_endnote_edit):
         * the number of the note the code will be */
        uint32_t nn = 1, p = 0, d, k;
        while (p < g_ed.cur && p < g_ed.doc.len) {
            if (iw_unit_kind(&g_ed.doc, p) == IW_U_NOTE && (uint32_t)iw_doc_byte(&g_ed.doc, p + 1u) == g_note_kind) nn++;
            p = iw_unit_next(&g_ed.doc, p);
        }
        if (g_note_kind == IW_NOTE_FOOT) c = IW_NOTE_INDENT_COLS;
        d = iw__digits(nn);
        for (k = 0; k < d; k++) { num[d - 1u - k] = (char)('0' + nn % 10u); nn /= 10u; }
        num[d] = 0;
#ifdef IWORD_MUT_NOTE_NUMBER
        /* MUTANT (Rule 6; test-iword-p4-face-mutant): no number on the note
         * screen. NEVER real. */
        num[0] = ' '; num[1] = 0; d = 1;
#endif
        put_at(&rows[0], c, num);
        c += d;
        if (g_note_kind == IW_NOTE_END) put_at(&rows[0], c++, ".");
    }
    for (i = 0; i < g_sub_n; i++) {
        uint8_t b = g_sub[i];
        if (b == IW_WP5_HRT) { r++; c = 0; if (r >= SCR_LINES - 1u) break; continue; }
        if (b == IW_HF_PAGE_NUM) { put_at(&rows[r], c, "^B"); c += 2u; continue; }
        if (c < SCR_COLS) rows[r].t[c] = (char)b;
        c++;
    }
    if (r < SCR_LINES - 1u) { rows[r].inv_col = (uint8_t)(c < SCR_COLS ? c : SCR_COLS - 1u); rows[r].inv_len = 1; }
    for (i = 0; i < SCR_COLS; i++) st[i] = ' ';
    st[SCR_COLS] = 0;
    {
        char left[48];
        uint32_t n = 0, k;
        const char *t;
        if (g_mode == M_HFEDIT) {
#ifndef IWORD_MUT_HF_STATUS
            /* (IWORD_MUT_HF_STATUS, Rule 6, test-iword-p4-face-mutant: the
             * screen does not name the header. NEVER real.) */
            for (k = 0; NAMES[g_hf_which][k]; k++) left[n++] = NAMES[g_hf_which][k];
#else
            (void)NAMES;
#endif
            t = IW_HF_EDIT_SUFFIX;
        } else t = g_note_kind == IW_NOTE_FOOT ? IW_FOOT_EDIT : IW_END_EDIT;
        for (k = 0; t[k]; k++) left[n++] = t[k];
        left[n] = 0;
        for (k = 0; k < n; k++) st[k] = left[k];
    }
    {
        /* "Ln 1" Pos 2.6"" (MINT33 05_header_typed): the cursor's line and
         * column on the screen, the measures of the editing screen */
        char lnv[12], pos[12], grp[24];
        uint32_t k, n = 0, lk = iw_fmt_inches(iw_ln_hundredths(r + 1u), lnv), pk = iw_fmt_inches(iw_pos_hundredths(c), pos);
        grp[n++] = 'L'; grp[n++] = 'n'; grp[n++] = ' ';
        for (k = 0; k < lk; k++) grp[n++] = lnv[k];
        for (k = 0; k < n; k++) st[STATUS_DOC_END + 1 - n + k] = grp[k];
        st[STATUS_POS] = 'P'; st[STATUS_POS + 1] = 'o'; st[STATUS_POS + 2] = 's';
        for (k = 0; k < pk && STATUS_POS + 4u + k < SCR_COLS; k++) st[STATUS_POS + 4 + k] = pos[k];
    }
    put_at(&rows[SCR_LINES - 1], 0, st);
}

/* One cell rule row of the table at tdef (cols columns) between the left
 * margin and the right: kind 0 top, 1 between rows, 2 bottom (CP437 rules,
 * spec/iword/wp5_page.h "ON THE SCREEN"). */
static void tbl_rule(row_t *r, const iw_doc_t *d, uint32_t tdef, uint32_t cols, int kind)
{
    static const char L[3] = { (char)0xC9, (char)0xC7, (char)0xC8 }, M[3] = { (char)0xD1, (char)0xC5, (char)0xCF },
                      R[3] = { (char)0xBB, (char)0xB6, (char)0xBC }, H[3] = { (char)0xCD, (char)0xC4, (char)0xCD };
    uint32_t right = IW_SCREEN_COL(iw__cell_left(d, tdef, cols)) - 1u, i, c;
    for (c = 0; c <= right && c < SCR_COLS; c++) r->t[c] = H[kind];
    r->t[0] = L[kind];
    for (i = 1; i < cols; i++) {
        c = IW_SCREEN_COL(iw__cell_left(d, tdef, i));
        if (c < SCR_COLS) r->t[c] = M[kind];
    }
    if (right < SCR_COLS) r->t[right] = R[kind];
}

/* The row's verticals over its text row */
static void tbl_verticals(row_t *r, const iw_doc_t *d, uint32_t tdef, uint32_t cols)
{
    uint32_t right = IW_SCREEN_COL(iw__cell_left(d, tdef, cols)) - 1u, i, c;
    r->t[0] = (char)0xBA;
    for (i = 1; i < cols; i++) {
        c = IW_SCREEN_COL(iw__cell_left(d, tdef, i));
        if (c < SCR_COLS) r->t[c] = (char)0xB3;
    }
    if (right < SCR_COLS) r->t[right] = (char)0xBA;
}

static void put_cell_name(char *out, uint32_t *n)
{
    /* "Cell A1" (shots_wpMINT38/05_table_normal): the cursor's column letter
     * and row number */
    iw_where_t w;
    uint32_t p = 0, row = 0, t = P4_NONE;
    iw_ed_where(&g_ed, &w);
    while (p < g_ed.cur && p < g_ed.doc.len) {
        int b = iw_doc_byte(&g_ed.doc, p);
        if (b == (int)IW_WP5_COLDEF_GRP && iw_doc_byte(&g_ed.doc, p + 1u) == (int)IW_WP5_TBL_SUB) { t = p; row = 0; }
        else if (t != P4_NONE && b == (int)IW_WP5_TROW_GRP && iw_doc_byte(&g_ed.doc, p + 1u) == (int)IW_WP5_TROW_SUB) row++;
        p = iw_unit_next(&g_ed.doc, p);
    }
    out[(*n)++] = 'C'; out[(*n)++] = 'e'; out[(*n)++] = 'l'; out[(*n)++] = 'l'; out[(*n)++] = ' ';
    out[(*n)++] = (char)('A' + (w.cell ? w.cell - 1u : 0u));
    if (row >= 10u) out[(*n)++] = (char)('0' + (row / 10u) % 10u);
    out[(*n)++] = (char)('0' + row % 10u);
    out[(*n)++] = ' ';
}

static void p4_paint_full(const iw_where_t *w)
{
    static row_t rows[SCR_LINES];
    uint32_t i;
    switch (g_mode) {
    case M_FMTPAGE: case M_HFMENU: case M_HFPLACE: {
        iw_hfstate_t s;
        p4_rows(rows, PG_MENU, IW_PG_MENU_ROWS);
        iw_hf_at(&g_ed.doc, g_ed.cur, &s);
        put_at(&rows[2], IW_PG_VALUE_COL, IW_PG_V_CENTER);
        hf_value(&rows[6], &s, IW_HF_HEADER_A);
        hf_value(&rows[8], &s, IW_HF_FOOTER_A);
        put_at(&rows[10], IW_PG_VALUE_COL, IW_PG_V_MARGIN);
        put_at(&rows[11], IW_PG_VALUE_COL, IW_PG_V_MARGIN);
        put_at(&rows[15], IW_PG_VALUE_COL, IW_PG_V_PAPER);
        put_at(&rows[16], IW_PG_VALUE_COL, IW_PG_V_TYPE);
        if (g_mode == M_FMTPAGE) p4_bottom(rows, IW_FMT_SELECTION "0", 1);
        else if (g_mode == M_HFMENU) p4_bottom(rows, g_hf_which >= IW_HF_FOOTER_A ? IW_HF_MENU_FOOTERS : IW_HF_MENU_HEADERS, 1);
        else p4_bottom(rows, IW_HF_MENU_PLACE, 1);
        break; }
    case M_HFEDIT: case M_NOTEEDIT:
        paint_sub(rows);
        break;
    case M_COLDEF: {
        /* shots_wpMINT37/03_col_define: the default two columns 1"-4",
         * 4.5"-7.5" */
        static const char *const NUMS[24] = { " 1:", " 2:", " 3:", " 4:", " 5:", " 6:", " 7:", " 8:", " 9:", "10:", "11:",
            "12:", "13:", "14:", "15:", "16:", "17:", "18:", "19:", "20:", "21:", "22:", "23:", "24:" };
        p4_rows(rows, COLDEF_ROWS, IW_COLDEF_ROWS);
        put_at(&rows[10], 37, "Column");
        put_at(&rows[10], 47, "Left");
        put_at(&rows[10], 58, "Right");
        put_at(&rows[2], 45, IW_COLDEF_TYPE);
        put_at(&rows[4], 45, "2");
        for (i = 0; i < 12u; i++) {
            put_at(&rows[11 + i], 5, NUMS[i]);          /* right-aligned to column 7 */
            put_at(&rows[11 + i], 37, NUMS[12 + i]);
        }
        put_at(&rows[11], 15, "1\""); put_at(&rows[11], 26, "4\"");
        put_at(&rows[12], 15, "4.5\""); put_at(&rows[12], 26, "7.5\"");
        p4_bottom(rows, IW_FMT_SELECTION "0", 1);
        break; }
    case M_TBLEDIT: {
        /* shots_wpMINT38/04_table_edit: the document above, the Table Edit
         * status line, a rule, the two menu lines */
        uint32_t k;
        int save = g_mode;
        paint_doc(w, SCR_LINES - 4u);
        for (i = 0; i < 4u; i++) row_clear(&rows[i]);
        /* the editing screen's status line ("Cell A1 Doc 1 Pg 1 Ln 1.14" Pos
         * 1.12"" at the right), "Table Edit:  Press Exit when done" at the
         * left */
        g_mode = M_EDIT;
        paint_status(w, (int)(SCR_LINES - 4u));
        g_mode = save;
        rows[0] = g_shadow[SCR_LINES - 4u];
        for (k = 0; IW_TBL_EDIT_STATUS[k]; k++) rows[0].t[k] = IW_TBL_EDIT_STATUS[k];
        for (k = 0; k < SCR_COLS; k++) rows[1].t[k] = (char)0xCD;
        put_at(&rows[2], 0, IW_TBL_EDIT_MENU1);
        put_at(&rows[3], 0, IW_TBL_EDIT_MENU2);
        for (i = 0; i < 4u; i++) put_row((int)(SCR_LINES - 4u + i), &rows[i]);
        return; }
    case M_PRINT:
        p4_rows(rows, PRINT_ROWS, IW_PRINT_ROWS);
        put_at(&rows[13], IW_PRINT_VALUE_COL, IW_PRINT_V_PRINTER);
        put_at(&rows[14], IW_PRINT_VALUE_COL, IW_PRINT_V_BINDING);
        put_at(&rows[15], IW_PRINT_VALUE_COL, IW_PRINT_V_COPIES);
        put_at(&rows[16], IW_PRINT_VALUE_COL, IW_PRINT_V_GENERATED);
        put_at(&rows[17], IW_PRINT_VALUE_COL, IW_PRINT_V_GRAPHICS);
        put_at(&rows[18], IW_PRINT_VALUE_COL, IW_PRINT_V_TEXT);
        p4_bottom(rows, IW_FMT_SELECTION "0", 1);
        break;
    case M_SELPRN:
        for (i = 0; i < SCR_LINES; i++) row_clear(&rows[i]);
        put_at(&rows[0], 0, IW_SELPRN_TITLE);
        put_at(&rows[2], 0, "* " IW_PRINT_V_PRINTER);
        rows[2].blk_col = 0; rows[2].blk_len = 38;    /* the selection band (PRINT0 07) */
        p4_bottom(rows, IW_SELPRN_MENU, 1);
        break;
    case M_PEDIT: case M_PRNPORT: case M_PRNPORTFILE:
        p4_rows(rows, PEDIT_ROWS, IW_PEDIT_ROWS);
        put_at(&rows[2], IW_PRINT_VALUE_COL, IW_PEDIT_V_FILE);
        put_at(&rows[4], IW_PRINT_VALUE_COL, IW_PRINT_V_PRINTER);
        put_at(&rows[6], IW_PRINT_VALUE_COL, g_port);
        put_at(&rows[8], IW_PRINT_VALUE_COL, IW_PEDIT_V_FEEDER);
        put_at(&rows[12], IW_PRINT_VALUE_COL, IW_PEDIT_V_FONT);
        /* a file port leaves "Print to Hardware Port" blank (PRINT1 04) */
        if (!g_port_file) put_at(&rows[18], IW_PRINT_VALUE_COL, IW_PEDIT_V_HWPORT);
        if (g_mode == M_PRNPORT) p4_bottom(rows, IW_PORT_MENU, 1);
        else if (g_mode == M_PRNPORTFILE) {
            char t[TBX_FILE_PATH_MAX + 24];
            uint32_t n = 0, k;
            for (k = 0; IW_PORT_FILE_PROMPT[k]; k++) t[n++] = IW_PORT_FILE_PROMPT[k];
            for (k = 0; k < g_pflen && n + 1u < sizeof t; k++) t[n++] = g_pfield[k];
            t[n] = 0;
            put_at(&rows[SCR_LINES - 1], 0, t);
            rows[SCR_LINES - 1].inv_col = (uint8_t)(n < SCR_COLS ? n : SCR_COLS - 1u);
            rows[SCR_LINES - 1].inv_len = 1;
        } else p4_bottom(rows, IW_FMT_SELECTION "0", 1);
        break;
    default:
        break;
    }
    for (i = 0; i < SCR_LINES; i++) put_row((int)i, &rows[i]);
}

/* The status-line menus over the document (paint_status) */
static const char *p4_status_text(char *buf, uint32_t cap)
{
    uint32_t n = 0, k;
    const char *t = 0;
    switch (g_mode) {
    case M_NOTEMENU: return IW_NOTE_MENU;
    case M_NOTESUB:  return g_note_kind == IW_NOTE_FOOT ? IW_FOOT_MENU : IW_END_MENU;
    case M_COLTAB:   return IW_COLTAB_MENU;
    case M_COLMENU:  return IW_COL_MENU;
    case M_TBLMENU:  return IW_TBL_MENU;
    case M_TBLCOLS:  t = IW_TBL_COLS_PROMPT; break;
    case M_TBLROWS:  t = IW_TBL_ROWS_PROMPT; break;
    case M_PRNERR:   return IW_PRINT_FAIL;
    default:         return 0;
    }
    for (k = 0; t[k] && n + 1u < cap; k++) buf[n++] = t[k];
    for (k = 0; k < g_tflen && n + 1u < cap; k++) buf[n++] = g_tfield[k];
    buf[n] = 0;
    return buf;
}

/* ---- printing: the stream into the port's file ---------------------------------------- */
static int print_sink(void *ctx, const uint8_t *b, uint32_t n)
{
    uint32_t done = 0;
    (void)ctx;
    while (done < n) {
        int32_t w = tbx_file_write(g_prn_h, b + done, n - done);
        if (w <= 0) return -1;
        done += (uint32_t)w;
    }
    return 0;
}

/* Full Document (p. 493): the stream to the port. A file name: created on
 * the data volume and written. LPT1: the device -- a disk application gets
 * no device from the Toolbox Gate (CREATE refuses DOS device names), so the
 * print fails visibly: PC LOAD LETTER. */
static void print_document(void)
{
    const char *name = g_port_file ? g_port : "PRN";
    int rc;
#ifdef IWORD_MUT_PRINT_NOFAIL
    /* MUTANT (Rule 6; test-flair-iword-print-mutant): a refused port is
     * silently ignored. NEVER real. */
    if (!g_port_file) { g_mode = M_EDIT; return; }
#endif
    g_prn_h = tbx_file_create(name);
    if (g_prn_h < 0) { g_mode = M_PRNERR; return; }
    rc = iw_print(&g_ed, g_bank[1u - g_live], BANK, print_sink, 0);
    if (tbx_file_close(g_prn_h) != TBX_OK || rc != IW_OK) { g_mode = M_PRNERR; return; }
    g_mode = M_EDIT;
}

/* ---- keys ----------------------------------------------------------------------- */
static void sub_start(const uint8_t *t, uint32_t n)
{
    uint32_t i;
    g_sub_n = 0;
    for (i = 0; i < n && i < SUB_CAP; i++) g_sub[g_sub_n++] = t[i];
}

static void sub_key(char ch, uint8_t vk, int32_t mods)
{
    int ctrl = (mods & (int32_t)FLAIR_EVT_MOD_CONTROL_KEY) != 0;
    int rc = IW_OK;
    if (vk == VK_F7) {
        /* Exit: the code goes in (headers p. 270 step 7, notes p. 210 step 5) */
        ensure_room();
        if (g_mode == M_HFEDIT) {
            if (g_hf_pos != P4_NONE) rc = iw_ed_hf_edit(&g_ed, g_hf_pos, g_sub, g_sub_n);
            else rc = iw_ed_hf(&g_ed, g_hf_which, g_hf_occ, g_sub, g_sub_n);
            g_mode = M_FMTPAGE;
        } else {
            rc = iw_ed_note(&g_ed, g_note_kind, g_sub, g_sub_n);
            g_mode = M_EDIT;
        }
        if (rc == IW_OK) rc = iw_ed_fixup(&g_ed);
        if (rc != IW_OK) error("ERROR: Document too large", M_EDIT);
        if (g_mode == M_EDIT) paint_all();
        return;
    }
    if (vk == VK_ENTER || ch == '\r') { if (g_sub_n < SUB_CAP) g_sub[g_sub_n++] = IW_WP5_HRT; return; }
    if (vk == VK_BSPC || ch == 8) { if (g_sub_n) g_sub_n--; return; }
    if (ctrl && (ch == 'b' || ch == 'B' || ch == 2) && g_mode == M_HFEDIT) {
        if (g_sub_n < SUB_CAP) g_sub[g_sub_n++] = IW_HF_PAGE_NUM;      /* ^B, p. 273 */
        return;
    }
    if (!ctrl && ch >= ' ' && ch <= '~' && g_sub_n < SUB_CAP) g_sub[g_sub_n++] = (uint8_t)ch;
}

static void p4_key(char ch, uint8_t vk, int32_t mods)
{
    int cancel = vk == VK_F1 || vk == VK_ESC || ch == 0x1B;
    int enter = vk == VK_ENTER || ch == '\r';
    int rc = IW_OK;
    switch (g_mode) {
    case M_FMTPAGE:
        if (ch == '3') { g_hf_which = IW_HF_HEADER_A; g_mode = M_HFMENU; }
        else if (ch == '4') { g_hf_which = IW_HF_FOOTER_A; g_mode = M_HFMENU; }
        else if (vk == VK_F7) g_mode = M_EDIT;
        else if (cancel || enter || ch == '0') g_mode = M_FORMAT;
        break;
    case M_HFMENU:
        if (ch == '1' || ch == '2') { g_hf_which += (uint32_t)(ch - '1'); g_mode = M_HFPLACE; }
        else g_mode = M_FMTPAGE;
        break;
    case M_HFPLACE:
        if (ch == '1') {
            /* Discontinue: the code at once (MINT35 04_after_discontinue) */
            ensure_room();
            rc = iw_ed_hf(&g_ed, g_hf_which, 0, 0, 0);
            if (rc == IW_OK) rc = iw_ed_fixup(&g_ed);
            g_mode = M_FMTPAGE;
        } else if (ch >= '2' && ch <= '4') {
            static const uint8_t OCC[3] = { IW_HF_EVERY, IW_HF_ODD, IW_HF_EVEN };
            g_hf_occ = OCC[ch - '2'];
            g_hf_pos = P4_NONE;
            g_sub_n = 0;
            g_mode = M_HFEDIT;
        } else if (ch == '5') {
            /* Edit (p. 273): the definition before the cursor, else after */
            uint32_t at = iw_hf_find(&g_ed.doc, g_ed.cur, g_hf_which);
            if (at == P4_NONE) g_mode = M_FMTPAGE;
            else {
                static uint8_t t[SUB_CAP];
                g_hf_pos = at;
                sub_start(t, iw_hf_text(&g_ed.doc, at, t, SUB_CAP));
                g_mode = M_HFEDIT;
            }
        } else g_mode = M_FMTPAGE;
        break;
    case M_HFEDIT: case M_NOTEEDIT:
        sub_key(ch, vk, mods);
        return;
    case M_NOTEMENU:
        if (ch == '1' || ch == '2') { g_note_kind = ch == '1' ? IW_NOTE_FOOT : IW_NOTE_END; g_mode = M_NOTESUB; }
        else g_mode = M_EDIT;
        break;
    case M_NOTESUB:
        if (ch == '1') { g_sub_n = 0; g_mode = M_NOTEEDIT; }
        else g_mode = M_EDIT;
        break;
    case M_COLTAB:
        if (ch == '1') g_mode = M_COLMENU;
        else if (ch == '2') g_mode = M_TBLMENU;
        else g_mode = M_EDIT;
        break;
    case M_COLMENU:
        ensure_room();
        if (ch == '1') { rc = iw_ed_col_on(&g_ed); g_mode = M_EDIT; }
        else if (ch == '2') { rc = iw_ed_col_off(&g_ed); g_mode = M_EDIT; }
        else if (ch == '3') g_mode = M_COLDEF;
        else g_mode = M_EDIT;
        if (rc == IW_OK && ch >= '1' && ch <= '2') rc = iw_ed_fixup(&g_ed);
        if (rc == IW_E_RANGE) rc = IW_OK;                 /* nothing to turn on / off */
        break;
    case M_COLDEF:
        if (vk == VK_F7 || enter) {
            /* the default definition (03_col_define), back to the Columns
             * menu (04_after_define) */
            ensure_room();
            rc = iw_ed_coldef(&g_ed, 2u);
            if (rc == IW_OK) rc = iw_ed_fixup(&g_ed);
            if (rc == IW_E_RANGE) rc = IW_OK;
            g_mode = M_COLMENU;
        } else if (cancel) g_mode = M_COLMENU;
        break;
    case M_TBLMENU:
        if (ch == '1') { g_tfield[0] = '3'; g_tflen = 1; g_tfresh = 1; g_mode = M_TBLCOLS; }
        else g_mode = M_EDIT;
        break;
    case M_TBLCOLS: case M_TBLROWS:
        if (cancel) { g_mode = M_EDIT; break; }
        if (enter) {
            uint32_t v = 0, k;
            for (k = 0; k < g_tflen; k++) v = v * 10u + (uint32_t)(g_tfield[k] - '0');
            if (g_mode == M_TBLCOLS) {
                g_tcols = v;
                g_tfield[0] = '1'; g_tflen = 1; g_tfresh = 1;
                g_mode = M_TBLROWS;
            } else {
                ensure_room();
                rc = iw_ed_table(&g_ed, g_tcols, v);
                if (rc == IW_OK) rc = iw_ed_fixup(&g_ed);
                if (rc == IW_E_RANGE) { rc = IW_OK; g_mode = M_EDIT; }
                else g_mode = M_TBLEDIT;
            }
            break;
        }
        if (vk == VK_BSPC || ch == 8) { if (g_tflen) g_tflen--; g_tfresh = 0; break; }
        if (ch >= '0' && ch <= '9') {
            if (g_tfresh) { g_tflen = 0; g_tfresh = 0; }
            if (g_tflen < 5u) g_tfield[g_tflen++] = ch;
        }
        break;
    case M_TBLEDIT:
        /* F7 / Alt-F7: the normal editing mode (p. 642); the structure
         * options are not live */
        if (vk == VK_F7 || (vk == 0x41 && (mods & (int32_t)FLAIR_EVT_MOD_OPTION_KEY))) g_mode = M_EDIT;
        break;
    case M_PRINT:
        if (ch == '1') print_document();
        else if (ch == 's' || ch == 'S') g_mode = M_SELPRN;
        else if (vk == VK_F7 || cancel || enter || ch == '0') g_mode = M_EDIT;
        break;
    case M_SELPRN:
        if (ch == '3') g_mode = M_PEDIT;
        else if (vk == VK_F7 || cancel || ch == '1' || enter) g_mode = M_PRINT;
        break;
    case M_PEDIT:
        if (ch == '2') g_mode = M_PRNPORT;
        else if (vk == VK_F7 || cancel || enter || ch == '0') g_mode = M_SELPRN;
        break;
    case M_PRNPORT:
        if (ch == '1') { scopy(g_port, IW_PEDIT_V_LPT1); g_port_file = 0; g_mode = M_PEDIT; }
        else if (ch == '8') { g_pflen = 0; g_mode = M_PRNPORTFILE; }
        else g_mode = M_PEDIT;
        break;
    case M_PRNPORTFILE:
        if (cancel) { g_mode = M_PEDIT; break; }
        if (enter) {
            uint32_t k;
            if (g_pflen) {
                for (k = 0; k < g_pflen; k++) g_port[k] = upper(g_pfield[k]);
                g_port[g_pflen] = 0;
                g_port_file = 1;
            }
            g_mode = M_PEDIT;
            break;
        }
        if (vk == VK_BSPC || ch == 8) { if (g_pflen) g_pflen--; break; }
        if (ch > ' ' && ch <= '~' && g_pflen + 1u < sizeof g_pfield) g_pfield[g_pflen++] = ch;
        break;
    case M_PRNERR:
        g_mode = M_EDIT;
        break;
    default:
        g_mode = M_EDIT;
        break;
    }
    if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
    if (g_mode == M_EDIT || g_mode == M_PRNERR || g_mode == M_TBLEDIT || g_mode == M_COLMENU) paint_all();
}
