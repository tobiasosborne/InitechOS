/*
 * os/apps/itext/itext.c -- InitechText, the SimpleText-alike text editor
 * (THE ARTIFACT; bead initech-tdnl.16, GUI-remediation R4.2; absorbs the
 * initech-ww9c Scrap wiring).
 *
 * WHAT IT IS. A small disk application (TEXT.EXE in the data volume's APPS
 * folder, double-clicked in the Finder) that edits one plain-text document in
 * a Platinum document window -- the Mac OS 8 SimpleText this is the Initech
 * counterpart of. It owns NO editor core: the document IS a FLAIR TextEdit
 * record (os/flair/textedit.{c,h}, the shared engine: typing, Backspace,
 * arrows, click / shift-click / drag selection, word wrap, the capacity
 * probe), linked into the image. Everything else is the Toolbox Gate:
 * TEXTDRAW in Geneva 12 (spec Sec 7 FONT + SIZE), the shell's desk Scrap (Sec 12),
 * Standard File (Sec 13), mouse tracking (Sec 14), the vertical scroll bar +
 * GlobalToLocal (Sec 15), the file verbs (Sec 10) and the alert (Sec 11).
 *
 * GROUND TRUTH (Law 1). ../system7-decomp specs/sys8/simpletext-stdfile.md
 * (mint-results-007, Mac OS 8.1 SimpleText under Basilisk II) [MEAS]:
 *   - the window is "untitled", sized nearly to the screen, one vertical
 *     scroll bar; text starts 5 px right of / 4 px below the content corner;
 *   - the document face is Geneva 12 (Font menu Geneva, Size menu 12 Point),
 *     a 16-px line (ascent 12, descent 3, leading 1) -- InitechOS's Geneva 12
 *     strike carries the real Mac OS 8.1 NFNT 12112 metrics (spec/assets/
 *     geneva12.art; bead initech-uelk);
 *   - the caret is 1 px wide, black, one line tall, one pixel LEFT of the
 *     text position; a range is solid black with white ink, full line
 *     height, carried to the right edge where it continues past a line end;
 *   - File: New ^N, Open... ^O, -, Close ^W, Save ^S, Save As..., -,
 *     Page Setup..., Print... ^P, Print One Copy, -, Quit ^Q (all enabled);
 *   - Edit: Undo ^Z, -, Cut ^X, Copy ^C, Paste ^V, Clear, Select All ^A, ...
 *     Cut/Copy/Clear grayed without a selection, Paste grayed with an empty
 *     scrap.
 *   - Save changes alert: Save (default) / Cancel / Don't Save.
 * DEVIATIONS (stated, never hidden): the Geneva 12 glyph SHAPES are
 *   hand-authored clean-room (their metrics and ink extents are the real
 *   strike's; PRD Sec 6.4 / Sec 12); Undo, Page Setup, Print and Print One Copy are present but grayed
 *   (not built); the Find / page items and Show Clipboard are not in the menu;
 *   Close == Quit (the V1 gate has one window per tenant and no
 *   DISPOSEWINDOW: closing SimpleText's last window leaves it running, here it
 *   ends the application); the caret does not blink (no idle events reach a
 *   tenant); the window does not re-wrap when grown; files are saved with CR
 *   line ends (the Mac TEXT convention) and an opened file's CR LF / LF ends
 *   become CR.
 *
 * Mutants (Rule 6), -D on this file, each RED in its named gate leg:
 *   ITEXT_MUT_COPY_NOTHING   Copy puts an empty run on the scrap
 *                            (test-itext-route, test-flair-itext)
 *   ITEXT_MUT_SAVE_NO_CR     Save writes the text with the CRs dropped
 *                            (test-itext-route, test-flair-itext FAT leg)
 *   ITEXT_MUT_QUIT_NO_ASK    Quit over unsaved work exits without the alert
 *                            (test-itext-route)
 *   ITEXT_MUT_SEL_NOT_INVERSE  a range is drawn as plain text
 *                            (test-itext-route, test-flair-itext PIXEL leg)
 *   ITEXT_MUT_LF_PAIR_OUTPUT  Open's CR LF test reads the normalised output
 *                            (L005; test-itext-route "linefeed")
 *   ITEXT_MUT_FULL_SILENT    a refused edit at the limit raises no notice
 *                            (L014; test-itext-route "full")
 *   ITEXT_MUT_SCROLL_NEVER_ENABLED  reports a fitting document forever
 *                            (test-itext-route, test-flair-itext-scroll)
 *
 * Freestanding C (Law 3). ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include "tbx.h"
#include "tbx_save.h"   /* initech-mcep: the atomic document save */
#include "textedit.h"
#include "text.h"

const char tbx_app_name[] = "TEXT";

/* ---- geometry (GLOBAL frame; content-local everything else) ------------- */
#define WIN_L   4
#define WIN_T  44
#define WIN_R 588
#define WIN_B 476
/* the content rect of a document window: frame inset (6, 22, 21, 21) --
 * os/flair/window.c CalcDocContentRect (the left body rail since tdnl.51,
 * the title bar, the two scroll-bar gutters); measured on the screendump of
 * this window: content x 10..566, y 66..454 */
#define CONT_W ((WIN_R - WIN_L) - 27)
#define CONT_H ((WIN_B - WIN_T) - 43)
#define TEXT_X  5                         /* [MEAS] content left + 5 */
#define TEXT_Y  4                         /* [MEAS] content top + 4  */
#define WRAP_W (CONT_W - 2 * TEXT_X)

#define MENU_FILE 129
#define MENU_EDIT 130
enum { F_NEW = 1, F_OPEN, F_D1, F_CLOSE, F_SAVE, F_SAVEAS, F_D2, F_SETUP,
       F_PRINT, F_PRINT1, F_D3, F_QUIT };
enum { E_UNDO = 1, E_D1, E_CUT, E_COPY, E_PASTE, E_CLEAR, E_ALL };

/* The menu resource lives in the IMAGE (.data) so the enable bytes can change
 * (the Menu Manager reads them when a menu is pulled down). */
static tbx_menuitem_t file_items[] TBX_IN_IMAGE = {
    { "New",            0, 'N', 0, 1, 0, { 0, 0, 0 } },
    { "Open...",        0, 'O', 0, 1, 0, { 0, 0, 0 } },
    { "-",              0, 0,   0, 0, 1, { 0, 0, 0 } },
    { "Close",          0, 'W', 0, 1, 0, { 0, 0, 0 } },
    { "Save",           0, 'S', 0, 1, 0, { 0, 0, 0 } },
    { "Save As...",     0, 0,   0, 1, 0, { 0, 0, 0 } },
    { "-",              0, 0,   0, 0, 1, { 0, 0, 0 } },
    { "Page Setup...",  0, 0,   0, 0, 0, { 0, 0, 0 } },
    { "Print...",       0, 'P', 0, 0, 0, { 0, 0, 0 } },
    { "Print One Copy", 0, 0,   0, 0, 0, { 0, 0, 0 } },
    { "-",              0, 0,   0, 0, 1, { 0, 0, 0 } },
    { "Quit",           0, 'Q', 0, 1, 0, { 0, 0, 0 } },
};
static tbx_menuitem_t edit_items[] TBX_IN_IMAGE = {
    { "Undo",       0, 'Z', 0, 0, 0, { 0, 0, 0 } },
    { "-",          0, 0,   0, 0, 1, { 0, 0, 0 } },
    { "Cut",        0, 'X', 0, 0, 0, { 0, 0, 0 } },
    { "Copy",       0, 'C', 0, 0, 0, { 0, 0, 0 } },
    { "Paste",      0, 'V', 0, 0, 0, { 0, 0, 0 } },
    { "Clear",      0, 0,   0, 0, 0, { 0, 0, 0 } },
    { "Select All", 0, 'A', 0, 1, 0, { 0, 0, 0 } },
};
static const tbx_menuinfo_t menus[] = {
    { MENU_FILE, 0, "File", file_items, 12, 0 },
    { MENU_EDIT, 0, "Edit", edit_items, 7, 0 },
};
static const tbx_menubar_t mbar = { menus, 2, 1, 0 };

/* ---- state ---------------------------------------------------------------- */
static FlairTE g_te;                       /* the document (BSS)            */
static int32_t g_win;
static int     g_dirty, g_active = 1, g_scroll, g_anchor, g_scrap_has;
static int     g_after;                    /* what follows a save / Don't Save */
static int     g_sf_kind;                  /* the pending Standard File ask  */
static char    g_path[TBX_SF_REPLY_BYTES]; /* the document's DOS path, or "" */
static char    g_reply[TBX_SF_REPLY_BYTES];
static char    g_io[FLAIR_TE_TEXT_MAX + 1];
static char    g_clip[8192];
static char    g_run[84] TBX_IN_IMAGE;     /* a TEXTDRAW run (image-resident) */
static char    g_title[16] TBX_IN_IMAGE;
static char    g_alert[96] TBX_IN_IMAGE;

enum { ACT_NONE, ACT_QUIT, ACT_NEW, ACT_OPEN };
enum { SF_NONE, SF_OPEN, SF_SAVEAS };

#define FG_TEXT (TBX_COLOR_BLACK | TBX_TEXT_GENEVA12)
#define FG_HIL  (TBX_COLOR_WHITE | TBX_TEXT_GENEVA12)

static int lh(void) { return FlairTELineHeight(&g_te); }

/* ---- drawing -------------------------------------------------------------- */
static int adv(char c) { return text_face_advance(FONT_GENEVA12, 0u, (int)(unsigned char)c); }

/* Draw [a, b) of line L starting at content-local x, y; returns the x after. */
static int draw_run(int a, int b, int x, int y, int hil)
{
    while (a < b) {
        int n = 0, w = 0;
        while (a + n < b && n < 80) { g_run[n] = g_te.text[a + n]; w += adv(g_run[n]); n++; }
        g_run[n] = '\0';
#ifdef ITEXT_MUT_SEL_NOT_INVERSE
        /* MUTANT (Rule 6): a range is drawn as plain text. NEVER real. */
        hil = 0;
#endif
        (void)tbx_draw_text(g_win, x, y, g_run, hil ? FG_HIL : FG_TEXT,
                            hil ? TBX_COLOR_BLACK : TBX_COLOR_WHITE);
        x += w;
        a += n;
    }
    return x;
}

/* The paint. `full` (an updateEvt, a scroll, an activate) erases the whole
 * content; an edit repaints each line's own row and erases only below the
 * text -- a keystroke costs one row per line, not a 557 x 389 fill (the pump
 * presents after every tenant return, and a slow keystroke lets the next
 * input wait in the ring). */
static int g_full = 1, g_last_bottom, g_ps;

/* An edit or a selection move changes nothing BEFORE the lower of the old and
 * the new selection start (TextEdit edits at the selection; a greedy wrap that
 * pulls a word up changes the earlier line's end, which then reaches the
 * edit), so a partial paint starts at the first line ending at or after it:
 * typing at the end of a document repaints one line. */
static void paint(void)
{
    int L, h = lh(), s = g_te.selStart, e = g_te.selEnd, bottom;
    int lo = (g_ps < s) ? g_ps : s;
    if (g_full) (void)tbx_fill_rect(g_win, 0, 0, CONT_W, CONT_H, TBX_COLOR_WHITE);
    for (L = 0; L < g_te.nLines; L++) {
        int y = TEXT_Y + L * h - g_scroll;
        int ls = g_te.lineStarts[L];
        int ns = (L + 1 < g_te.nLines) ? g_te.lineStarts[L + 1] : g_te.teLength;
        int le = (ns > ls && g_te.text[ns - 1] == '\r') ? ns - 1 : ns;
        int x = TEXT_X, a, b;
        if (y + h <= 0) continue;
        if (y >= CONT_H) break;
        if (!g_full && ns < lo) continue;            /* untouched line */
        if (!g_full)                                /* this line's own row */
            (void)tbx_fill_rect(g_win, 0, y < 0 ? 0 : y, CONT_W, y + h, TBX_COLOR_WHITE);
        a = (s > ls) ? (s < le ? s : le) : ls;     /* the selected part */
        b = (e > ls) ? (e < le ? e : le) : ls;
        if (s >= e) { a = b = le; }
        x = draw_run(ls, a, x, y, 0);
        x = draw_run(a, b, x, y, 1);
        x = draw_run(b, le, x, y, 0);
        /* a range that continues past this line's end runs to the edge */
        if (s < e && s <= le && e > le && x < TEXT_X + WRAP_W)
#ifndef ITEXT_MUT_SEL_NOT_INVERSE
            (void)tbx_fill_rect(g_win, x, y, TEXT_X + WRAP_W, y + h, TBX_COLOR_BLACK);
#else
            { }
#endif
    }
    bottom = TEXT_Y + g_te.nLines * h - g_scroll;   /* below the text */
    if (bottom < 0) bottom = 0;
    if (!g_full && g_last_bottom > bottom && bottom < CONT_H)
        (void)tbx_fill_rect(g_win, 0, bottom, CONT_W, g_last_bottom, TBX_COLOR_WHITE);
    g_last_bottom = bottom;
    g_full = 0;
    g_ps = s;
    if (s == e && g_active) {                       /* [MEAS] 1 px, x - 1 */
        int16_t ph, pv;
        FlairTEGetPoint(&g_te, (int16_t)s, &ph, &pv);
        (void)tbx_fill_rect(g_win, TEXT_X + ph - 1, TEXT_Y + pv - g_scroll,
                            TEXT_X + ph, TEXT_Y + pv - g_scroll + h, TBX_COLOR_BLACK);
    }
}

/* Keep the caret (the selection's moving end) in view; give the bar the
 * document height. */
static void show_caret(void)
{
    int16_t ph, pv;
    int top, bot, doc = TEXT_Y * 2 + g_te.nLines * lh();
    FlairTEGetPoint(&g_te, g_te.selEnd, &ph, &pv);
    top = TEXT_Y + pv;
    bot = top + lh();
    if (top - TEXT_Y < g_scroll) g_scroll = top - TEXT_Y;
    if (bot + TEXT_Y > g_scroll + CONT_H) g_scroll = bot + TEXT_Y - CONT_H;
    if (g_scroll < 0) g_scroll = 0;
#ifdef ITEXT_MUT_SCROLL_NEVER_ENABLED
    /* Rule 6 (ehq4): the shared bar is never given an overflowing extent. */
    doc = CONT_H;
#endif
    {
        int was = g_scroll;
        g_scroll = tbx_set_scroll(g_win, doc, g_scroll);
        if (g_scroll < 0) g_scroll = 0;
        if (g_scroll != was) g_full = 1;
    }
}

static void menus_sync(void)
{
    int range = g_te.selStart < g_te.selEnd;
    edit_items[E_CUT - 1].enabled = (uint8_t)range;
    edit_items[E_COPY - 1].enabled = (uint8_t)range;
    edit_items[E_CLEAR - 1].enabled = (uint8_t)range;
    edit_items[E_PASTE - 1].enabled = (uint8_t)(g_scrap_has > 0);
}

static void refresh(void)
{
    show_caret();
    menus_sync();
    paint();
}

static void query_scrap(void)
{
    int32_t n = tbx_scrap_get((void *)0, 0u);
    g_scrap_has = n > 0;
}

/* ---- the document --------------------------------------------------------- */
static void set_title(void)
{
    int i = 0, k = 0;
    const char *t = "untitled";
    for (; g_path[i] != '\0'; i++) if (g_path[i] == '\\') k = i + 1;
    if (g_path[0] != '\0') t = g_path + k;
    for (i = 0; t[i] != '\0' && i < 15; i++) g_title[i] = t[i];
    g_title[i] = '\0';
    (void)tbx_set_wtitle(g_win, g_title);
}

static void doc_new(void)
{
    FlairTESetText(&g_te, "", 0);
    FlairTESetSelect(&g_te, 0, 0);
    g_path[0] = '\0';
    g_dirty = 0;
    g_scroll = 0;
    g_full = 1;
    set_title();
}

static void say(const char *l1, const char *l2)
{
    int i = 0, k;
    for (k = 0; l1[k] != '\0' && i < 90; k++) g_alert[i++] = l1[k];
    if (l2 != (const char *)0) {
        g_alert[i++] = '\r';
        for (k = 0; l2[k] != '\0' && i < 94; k++) g_alert[i++] = l2[k];
    }
    g_alert[i] = '\0';
    (void)tbx_alert(g_alert, "OK", (const char *)0, (const char *)0);
}

static int doc_load(const char *path)
{
    int32_t h = tbx_file_open(path, TBX_FILE_READ_ONLY), n, i, k = 0;
    if (h < 0) { say("The document could not be opened.", (const char *)0); return 0; }
    n = tbx_file_read(h, g_io, (uint32_t)sizeof g_io);
    (void)tbx_file_close(h);
    if (n < 0) { say("The document could not be opened.", (const char *)0); return 0; }
    /* CR LF -> one CR, a lone LF -> one CR, a lone CR stays. The pair test
     * remembers the previous INPUT byte (`prev`), never the normalised
     * output (which holds a CR for every LF): reading the output made the
     * second of two LF-only line ends look like the half of a CR LF (audit
     * 2026-10-23-das L005: blank lines vanished and were lost on Save). */
    {
        char prev = 0;
        for (i = 0; i < n; i++) {
            char c = g_io[i];
            int skip = (c == '\n' && prev == '\r');
#ifdef ITEXT_MUT_LF_PAIR_OUTPUT
            /* MUTANT (Rule 6): the pair test reads the normalised output (the
             * L005 defect). NEVER real. */
            skip = (c == '\n' && k > 0 && g_io[k - 1] == '\r');
#endif
            prev = c;
            if (!skip) g_io[k++] = (c == '\n') ? '\r' : c;
        }
    }
    if (n > FLAIR_TE_TEXT_MAX ||
        !FlairTECanReplace(&g_te, 0, g_te.teLength, g_io, (int16_t)k)) {
        say("This document is too large to be", "opened by InitechText.");
        return 0;
    }
    FlairTESetText(&g_te, g_io, (int16_t)k);
    FlairTESetSelect(&g_te, 0, 0);
    for (i = 0; path[i] != '\0' && i < (int32_t)sizeof g_path - 1; i++) g_path[i] = path[i];
    g_path[i] = '\0';
    g_dirty = 0;
    g_scroll = 0;
    g_full = 1;
    set_title();
    return 1;
}

static int doc_write(void)
{
    int32_t n = g_te.teLength, i, k = 0;
    for (i = 0; i < n; i++) {
#ifdef ITEXT_MUT_SAVE_NO_CR
        /* MUTANT (Rule 6): the line ends are dropped on the way out. */
        if (g_te.text[i] == '\r') continue;
#endif
        g_io[k++] = g_te.text[i];
    }
    /* initech-mcep (audit 2026-10-23-das L003): the atomic save -- a write
     * that does not fit leaves the old file exactly as it was. */
    if (tbx_file_save(g_path, g_io, (uint32_t)k) != k) {
        say("The document could not be saved.", (const char *)0);
        return 0;
    }
    g_dirty = 0;
    return 1;
}

static void run_after(void);

static void save_as(int after)
{
    const char *nm = "UNTITLED";
    int i = 0, k = 0;
    for (; g_path[i] != '\0'; i++) if (g_path[i] == '\\') k = i + 1;
    if (g_path[0] != '\0') nm = g_path + k;
    for (i = 0; nm[i] != '\0' && i < 12; i++) g_title[i] = nm[i];
    g_title[i] = '\0';
    g_after = after;
    g_sf_kind = SF_SAVEAS;
    if (tbx_sf_putfile("Save this document as:", g_title, g_reply) != TBX_OK) {
        g_sf_kind = SF_NONE;
        set_title();
    }
}

/* File > Save: write, or ask for a name first; then whatever was waiting. */
static void save(int after)
{
    if (g_path[0] == '\0') { save_as(after); return; }
    if (doc_write()) { g_after = after; run_after(); }
    else g_after = ACT_NONE;
}

static void open_ask(void)
{
    g_sf_kind = SF_OPEN;
    if (tbx_sf_getfile(g_reply) != TBX_OK) g_sf_kind = SF_NONE;
}

static void run_after(void)
{
    int a = g_after;
    g_after = ACT_NONE;
    if (a == ACT_QUIT) (void)tbx_exit(0);
    else if (a == ACT_NEW) doc_new();
    else if (a == ACT_OPEN) open_ask();
}

/* Unsaved work asks first (Save / Cancel / Don't Save), else acts now. */
static void guarded(int act, const char *why)
{
#ifdef ITEXT_MUT_QUIT_NO_ASK
    /* MUTANT (Rule 6): unsaved work is never asked about. NEVER real. */
    if (act == ACT_QUIT) g_dirty = 0;
#endif
    if (!g_dirty) { g_after = act; run_after(); return; }
    {
        int i = 0, k;
        const char *a = "Save changes to the document";
        for (k = 0; a[k] != '\0'; k++) g_alert[i++] = a[k];
        g_alert[i++] = '\r';
        g_alert[i++] = '"';
        for (k = 0; g_title[k] != '\0'; k++) g_alert[i++] = g_title[k];
        g_alert[i++] = '"';
        for (k = 0; why[k] != '\0'; k++) g_alert[i++] = why[k];
        g_alert[i] = '\0';
    }
    g_after = act;
    if (tbx_alert(g_alert, "Save", "Cancel", "Don't Save") != TBX_OK) g_after = ACT_NONE;
}

/* ---- editing -------------------------------------------------------------- */
static void replace_sel(const char *s, int n)
{
    if (!FlairTECanReplace(&g_te, g_te.selStart, g_te.selEnd, s, (int16_t)n)) {
        /* The edit cannot fit: say so. This is AUTHORED capacity feedback
         * grounded in audit 2026-10-23-das L014 (a refused edit must be
         * discoverable); no local TeachText manual page documents the exact
         * wording, which is from memory of TeachText's full-document notice
         * (unverified). The Toolbox Gate has no beep verb, so the notice is
         * the whole signal. */
#ifndef ITEXT_MUT_FULL_SILENT      /* MUTANT (Rule 6): the refusal is silent (L014). NEVER real. */
        say("This document is full.", "The edit could not be made.");
#endif
        return;
    }
    FlairTEInsert(&g_te, s, (int16_t)n);
    g_dirty = 1;
}

static void copy_sel(void)
{
    int n = g_te.selEnd - g_te.selStart;
#ifdef ITEXT_MUT_COPY_NOTHING
    /* MUTANT (Rule 6): an empty run goes to the scrap. NEVER real. */
    n = 0;
#endif
    if (tbx_scrap_put(g_te.text + g_te.selStart, (uint32_t)n) == TBX_OK)
        g_scrap_has = n > 0;
}

static void paste(void)
{
    int32_t n = tbx_scrap_get(g_clip, (uint32_t)sizeof g_clip);
    if (n < 0) return;
    replace_sel(g_clip, (int)n);
}

static void key(int32_t msg, int32_t mods)
{
    int ch = (int)(msg & 0xFF), vk = (int)((msg >> 8) & 0xFF);
    if (mods & FLAIR_EVT_MOD_CONTROL_KEY) return;   /* an unbound chord */
    if (ch == 0) {
        char a = 0;
        if (vk == 0x4B) a = FLAIR_TE_CH_LEFT;
        else if (vk == 0x4D) a = FLAIR_TE_CH_RIGHT;
        else if (vk == 0x48) a = FLAIR_TE_CH_UP;
        else if (vk == 0x50) a = FLAIR_TE_CH_DOWN;
        if (a != 0) FlairTEKey(&g_te, a);
    } else if (ch == 0x08) {
        if (g_te.selStart < g_te.selEnd || g_te.selStart > 0) {
            FlairTEKey(&g_te, 0x08);
            g_dirty = 1;
        }
    } else if (ch == 0x0D || ch == 0x0A || (ch >= 0x20 && ch < 0x7F)) {
        /* Return cooks to LF on the PS/2 table (os/flair/event.c, the
         * Dialog Manager accepts both); the document's line end is CR. */
        char c = (ch == 0x0A) ? '\r' : (char)ch;
        replace_sel(&c, 1);
    } else {
        return;
    }
    refresh();
}

static void local(const tbx_event_t *ev, int *h, int *v)
{
    int32_t p = tbx_global_to_local(g_win, ev->whereh, ev->wherev);
    *h = (int)(int16_t)(p & 0xFFFF) - TEXT_X;
    *v = (int)(int16_t)((uint32_t)p >> 16) - TEXT_Y + g_scroll;
}

static void menu(int32_t word)
{
    int id = (int)((uint32_t)word >> 16), item = (int)(word & 0xFFFF);
    if (id == MENU_FILE) {
        switch (item) {
        case F_NEW:    guarded(ACT_NEW, " before closing?"); break;
        case F_OPEN:   guarded(ACT_OPEN, " before closing?"); break;
        case F_CLOSE:
        case F_QUIT:   guarded(ACT_QUIT, " before quitting?"); break;
        case F_SAVE:   save(ACT_NONE); break;
        case F_SAVEAS: save_as(ACT_NONE); break;
        default: break;
        }
    } else if (id == MENU_EDIT) {
        switch (item) {
        case E_CUT:   copy_sel(); replace_sel("", 0); break;
        case E_COPY:  copy_sel(); break;
        case E_PASTE: paste(); break;
        case E_CLEAR: replace_sel("", 0); break;
        case E_ALL:   FlairTESetSelect(&g_te, 0, g_te.teLength); break;
        default: break;
        }
    }
}

/* ---- the application ------------------------------------------------------ */
int tbx_app_main(void)
{
    if (tbx_register() != TBX_OK) return 1;
    FlairTE_init(&g_te, (int16_t)WRAP_W, FLAIR_TE_JUST_LEFT);
    g_te.txFont = 1;                                /* applFont: Geneva ... */
    g_te.txSize = 12;                               /* ... 12 (SimpleText) */
    FlairTESetText(&g_te, "", 0);
    (void)tbx_set_mbar(&mbar);
    g_win = tbx_new_window(WIN_L, WIN_T, WIN_R, WIN_B, (int32_t)TBX_GOAWAY_ASK);
    if (g_win <= 0) {
        (void)tbx_exit(1);
        return 1;
    }
    doc_new();
    query_scrap();
    menus_sync();
    return 0;
}

void tbx_app_event(const tbx_event_t *ev)
{
    int h, v;
    switch (ev->what) {
    case updateEvt: {
        int32_t sv = tbx_get_scroll(g_win);
        if (sv >= 0) g_scroll = (int)sv;
        g_full = 1;
        paint();
        break;
    }
    case activateEvt:
        g_active = (ev->modifiers & FLAIR_EVT_MOD_ACTIVE_FLAG) != 0;
        if (g_active) query_scrap();
        menus_sync();
        g_full = 1;
        paint();
        break;
    case mouseDown:
        local(ev, &h, &v);
        g_anchor = FlairTEClick(&g_te, (int16_t)h, (int16_t)v,
                                (ev->modifiers & FLAIR_EVT_MOD_SHIFT_KEY) != 0);
        (void)tbx_track_mouse();
        menus_sync();
        paint();
        break;
    case TBX_EVT_TRACK: {
        int s0 = g_te.selStart, e0 = g_te.selEnd;
        local(ev, &h, &v);
        FlairTESetSelect(&g_te, (int16_t)g_anchor,
                         FlairTEGetOffset(&g_te, (int16_t)h, (int16_t)v));
        if (g_te.selStart != s0 || g_te.selEnd != e0 || ev->message == 1) {
            menus_sync();
            paint();
        }
        break;
    }
    case keyDown:
    case autoKey:
        key(ev->message, ev->modifiers);
        break;
    case TBX_EVT_MENU:
        menu(ev->message);
        refresh();
        break;
    case TBX_EVT_GOAWAY:
        guarded(ACT_QUIT, " before closing?");
        break;
    case TBX_EVT_ALERT: {
        int a = g_after;
        g_after = ACT_NONE;
        if (a == ACT_NONE) break;                    /* a notice: OK */
        if (ev->message == TBX_ALERT_DEFAULT) save(a);          /* Save */
        else if (ev->message == TBX_ALERT_OTHER) { g_after = a; run_after(); }
        refresh();                                   /* Cancel: nothing */
        break;
    }
    case TBX_EVT_SFREPLY: {
        int kind = g_sf_kind;
        g_sf_kind = SF_NONE;
        if (ev->message != 1) { g_after = ACT_NONE; set_title(); break; }
        if (kind == SF_OPEN) {
            (void)doc_load(g_reply);
        } else if (kind == SF_SAVEAS) {
            int i;
            for (i = 0; g_reply[i] != '\0'; i++) g_path[i] = g_reply[i];
            g_path[i] = '\0';
            set_title();
            if (doc_write()) run_after();
            else g_after = ACT_NONE;
        }
        refresh();
        break;
    }
    default:
        break;
    }
}
