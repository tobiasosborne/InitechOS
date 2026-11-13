/*
 * harness/proptest/test_itext_route.c -- the host ROUTING SUITE of
 * InitechText's verbs (factory C; bead initech-tdnl.16, R4.2).
 *
 * WHAT IS UNDER TEST. The real TEXT.EXE source, os/apps/itext/itext.c,
 * compiled on the host over the real FLAIR TextEdit (os/flair/textedit.c),
 * with the Toolbox Gate (os/apps/tbx/tbx.h) replaced by a RECORDING MOCK: the
 * TEXTDRAW runs and FILLRECTs of the last paint, a desk scrap that OUTLIVES a
 * launch (the shell's, spec/toolbox_gate.h Sec 12), an in-memory FAT for the
 * file verbs, and the asked-then-answered Standard File / alert / track
 * verbs the test answers by pushing the events the shell would push.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2 -- never from itext.c):
 *   - Mac OS 8.1 SimpleText as minted (../system7-decomp specs/sys8/
 *     simpletext-stdfile.md, mint-results-007): Cut/Copy/Clear grayed with no
 *     selection, Paste grayed with an empty scrap; the selection drawn black
 *     with white ink; the Save As prompt "Save this document as:"; the
 *     unsaved-changes alert's Save / Cancel / Don't Save;
 *   - Inside Macintosh's Scrap Manager (what one application cuts the next
 *     pastes; ZeroScrap + PutScrap; noTypeErr) and SFPutFile (an untitled
 *     document's Save asks for a name) -- textedit.md Sec 6, IM-I;
 *   - the Mac TEXT file is the bytes typed with CR line ends.
 * The bytes the EMULATOR writes are judged independently again by
 * test-flair-itext's mtools differential.
 *
 * Mutants (Rule 6; -D on the app build, test-itext-route-mutant), each RED for
 * its named reason (the first miss prints "RED-REASON <tag>"):
 *   ITEXT_MUT_COPY_NOTHING     -> copy
 *   ITEXT_MUT_SAVE_NO_CR       -> save
 *   ITEXT_MUT_QUIT_NO_ASK      -> quit
 *   ITEXT_MUT_SEL_NOT_INVERSE  -> inverse
 *   ITEXT_MUT_UNDO_NO_INVERSE  -> undo       (initech-4hkk)
 *   ITEXT_MUT_UNDO_PER_KEY     -> undorun    (initech-4hkk)
 *   ITEXT_MUT_UNDO_KEEPS_HISTORY -> undoreset (initech-4hkk)
 *   (and SCROLL_NEVER_ENABLED, LF_PAIR_OUTPUT, FULL_SILENT: Makefile list)
 * The mock volume keeps DOS 3.3 directory semantics -- one entry per name,
 * CREATE_NEW refuses a taken name, DELETE of a missing file is error 2,
 * RENAME moves the entry (name and data) and refuses an existing target --
 * and the test plants its own fixture files by name (mplant).
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../itext.c"   /* the application under test */

static long g_checks, g_fails;
static int  g_said[24];
static const char *const g_rn[] = { "copy", "save", "quit", "inverse", "route",
                                    "scrap", "file", "menu", "open", "edit", "scroll", "linefeed", "full",
                                    "undo", "undorun", "undoreset" };
enum { R_COPY, R_SAVE, R_QUIT, R_INV, R_ROUTE, R_SCRAP, R_FILE, R_MENU, R_OPEN, R_EDIT, R_SCROLL, R_LF, R_FULL,
       R_UNDO, R_UNDORUN, R_UNDORESET };
#define CHECK(c, r, ...) do { g_checks++; if (!(c)) { g_fails++; if (!g_said[r]) {    \
    g_said[r] = 1; printf("RED-REASON %s: ", g_rn[r]); printf(__VA_ARGS__);           \
    printf(" (line %d)\n", __LINE__); } else { printf("  FAIL line %d\n", __LINE__); } } } while (0)

/* ---- the recording mock -------------------------------------------------- */
#include "tbx_host.h"
typedef tbx_host_file_t mfile_t;
typedef tbx_host_op_t op_t;
#define m_ops tbx_host.ops
#define m_nops tbx_host.nops
#define m_exits tbx_host.exit_calls
#define m_alerts tbx_host.alerts
#define m_track tbx_host.track
#define m_sf tbx_host.sf
#define m_alert_text tbx_host.alert_text
#define m_alert_btn tbx_host.alert_btn
#define m_sf_prompt tbx_host.sf_prompt
#define m_sf_name tbx_host.sf_name
#define m_sf_reply tbx_host.sf_reply
#define m_title tbx_host.windows[0].title
#define m_scroll_len tbx_host.windows[0].scroll_len
#define m_scroll_val tbx_host.windows[0].scroll_val
#define m_scrap tbx_host.scrap
#define m_scrap_len tbx_host.scrap_len
#define m_scrap_puts tbx_host.scrap_puts
static mfile_t *mfind(const char *p) { return (mfile_t *)tbx_host_file(p); }
static mfile_t *mplant(const char *name, const char *data, int len) {
    unsigned char zero[6000]={0};
    if (!tbx_host_put_file(name,data ? (const void *)data : zero,(uint32_t)len)) exit(2);
    return mfind(name);
}

/* ---- driving ---------------------------------------------------------------- */
static void hev(int what, int32_t msg, int32_t mods, int h, int v)
{
    tbx_event_t e = { what, msg, mods, 0, h, v };
    m_nops = 0;                              /* the ops are THIS event's draws */
    tbx_app_event(&e);
}
static void hkey(int ascii, int vk, int mods) { hev(keyDown, (vk << 8) | ascii, mods, 0, 0); }
static void type(const char *s) { for (; *s; s++) hkey(*s == '\r' ? 0x0A : *s, *s == '\r' ? 0x1C : 0, 0); }
static void menu_hit(int id, int item) { hev(TBX_EVT_MENU, TBX_MENU_WORD(id, item), 0, 0, 0); }
static void alert_answer(int item) { hev(TBX_EVT_ALERT, item, 0, 0, 0); }
static void sf_answer(const char *path)
{
    if (path) snprintf(m_sf_reply, TBX_SF_REPLY_BYTES, "%s", path);
    m_sf = 0;
    hev(TBX_EVT_SFREPLY, path ? 1 : 0, 0, 0, 0);
}
/* a click / drag in TEXT-local pixels (the mock content corner is (10,66)) */
static void press(int x, int y, int shift)
{ hev(mouseDown, 0, shift ? FLAIR_EVT_MOD_SHIFT_KEY : 0, 10 + TEXT_X + x, 66 + TEXT_Y + y); }
static void track(int x, int y, int up) { hev(TBX_EVT_TRACK, up, 0, 10 + TEXT_X + x, 66 + TEXT_Y + y); }

static int doc_is(const char *s)
{ return g_te.teLength == (int)strlen(s) && memcmp(g_te.text, s, strlen(s)) == 0; }

static void launch(void)
{
    tbx_host.nwindows=0;
    memset(&g_te, 0, sizeof g_te);
    g_win = 0; g_dirty = 0; g_active = 1; g_scroll = 0; g_anchor = 0;
    g_scrap_has = 0; g_after = 0; g_sf_kind = 0; g_path[0] = 0;
    g_undo_ok = 0; g_undo_run = 0;           /* a fresh launch's BSS */
    m_exits = m_alerts = m_track = m_sf = 0;
    CHECK(tbx_app_main() == 0, R_ROUTE, "launch");
    hev(updateEvt, 1, 0, 0, 0);
}

static const op_t *find_text(const char *s, uint32_t fg, uint32_t bg)
{
    for (unsigned i = 0; i < m_nops; i++)
        if (!strcmp(m_ops[i].name,"tbx_draw_text") && strcmp(m_ops[i].text, s) == 0 && (uint32_t)m_ops[i].a[3] == fg && (uint32_t)m_ops[i].a[4] == bg)
            return &m_ops[i];
    return NULL;
}

int main(void)
{
    tbx_host_reset(); tbx_host.origin_h=10; tbx_host.origin_v=66; tbx_host.ch=389;
    /* -- launch 1: an empty scrap grays Paste; nothing selected grays Cut/Copy/Clear */
    launch();
    CHECK(strcmp(m_title, "untitled") == 0, R_ROUTE, "title %s", m_title);
    CHECK(!edit_items[E_PASTE - 1].enabled && !edit_items[E_CUT - 1].enabled &&
          !edit_items[E_COPY - 1].enabled && !edit_items[E_CLEAR - 1].enabled &&
          edit_items[E_ALL - 1].enabled, R_MENU, "empty-doc Edit enables");

    /* -- typing: "Saving tables to disk" Return "PC LOAD LETTER" */
    type("Saving tables to disk\rPC LOAD LETTER");
    CHECK(doc_is("Saving tables to disk\rPC LOAD LETTER"), R_EDIT, "typed text");
    CHECK(g_dirty, R_EDIT, "typing dirties");
    {   /* the caret: a 1-px black bar one line tall, one px left of the end */
        const op_t *c = &m_ops[m_nops - 1];
        /* 16 = the SimpleText line, simpletext-stdfile.md Sec 1.3 [MEAS] */
        CHECK(!strcmp(c->name,"tbx_fill_rect") && c->a[3] - c->a[1] == 1 && c->a[4] - c->a[2] == 16 && c->a[5] == TBX_COLOR_BLACK,
              R_INV, "caret rect %d,%d,%d,%d", c->a[1], c->a[2], c->a[3], c->a[4]);
    }
    hkey(0, 0, FLAIR_EVT_MOD_CONTROL_KEY);           /* an unbound chord: nothing */
    hkey('q', 0x10, FLAIR_EVT_MOD_CONTROL_KEY);
    CHECK(doc_is("Saving tables to disk\rPC LOAD LETTER"), R_EDIT, "a Ctrl chord types nothing");

    /* -- drag-select "tables" (Geneva 12, simpletext-stdfile.md Sec 1.2/1.4
     *    [MEAS]: "Saving " 45 px, "tables" 39 px -> ends at 84) */
    press(45, 5, 0);
    CHECK(m_track == 1, R_ROUTE, "a press asks the shell to track");
    track(60, 5, 0);
    track(84, 5, 1);
    CHECK(g_te.selStart == 7 && g_te.selEnd == 13, R_EDIT, "drag selection [%d,%d)", g_te.selStart, g_te.selEnd);
    CHECK(find_text("tables", TBX_COLOR_WHITE | TBX_TEXT_GENEVA12, TBX_COLOR_BLACK) != NULL &&
          find_text("Saving ", TBX_COLOR_BLACK | TBX_TEXT_GENEVA12, TBX_COLOR_WHITE) != NULL,
          R_INV, "the range is black with white ink, the rest black on white");
    CHECK(edit_items[E_CUT - 1].enabled && edit_items[E_COPY - 1].enabled &&
          edit_items[E_CLEAR - 1].enabled, R_MENU, "a range enables Cut/Copy/Clear");

    /* -- Copy: the desk scrap holds exactly "tables"; Paste comes alive */
    menu_hit(MENU_EDIT, E_COPY);
    CHECK(m_scrap_len == 6 && memcmp(m_scrap, "tables", 6) == 0, R_COPY,
          "scrap after Copy has %d bytes", m_scrap_len);
    CHECK(edit_items[E_PASTE - 1].enabled, R_COPY, "Paste enabled after Copy");

    /* -- click past the end of line 1, Return, Paste */
    press(300, 20, 0);
    track(300, 20, 1);
    CHECK(g_te.selStart == g_te.teLength && g_te.selEnd == g_te.teLength, R_EDIT, "caret at the end");
    type("\r");
    menu_hit(MENU_EDIT, E_PASTE);
    CHECK(doc_is("Saving tables to disk\rPC LOAD LETTER\rtables"), R_COPY, "pasted text");

    /* -- shift-click extends from the caret back to "LOAD" (line 1; Geneva 12
     *    "PC " = 8 + 8 + 4 = 20, the NFNT 12112 advances) */
    press(20, 20, 1);
    CHECK(g_te.selStart == 25 && g_te.selEnd == g_te.teLength, R_EDIT, "shift-click [%d,%d)", g_te.selStart, g_te.selEnd);
    track(20, 20, 1);

    /* -- Cut then Clear */
    menu_hit(MENU_EDIT, E_CUT);
    CHECK(doc_is("Saving tables to disk\rPC "), R_EDIT, "after Cut");
    CHECK(m_scrap_len == 18, R_SCRAP, "Cut put %d bytes", m_scrap_len);
    menu_hit(MENU_EDIT, E_ALL);
    CHECK(g_te.selStart == 0 && g_te.selEnd == g_te.teLength, R_EDIT, "Select All");
    menu_hit(MENU_EDIT, E_PASTE);
    CHECK(doc_is("LOAD LETTER\rtables"), R_EDIT, "Paste over Select All");

    /* -- Save on an untitled document asks SFPutFile; the reply is written */
    menu_hit(MENU_FILE, F_SAVE);
    CHECK(m_sf == 2 && strcmp(m_sf_prompt, "Save this document as:") == 0 &&
          strcmp(m_sf_name, "UNTITLED") == 0, R_SAVE, "Save asks SFPutFile (sf=%d prompt=%s)", m_sf, m_sf_prompt);
    sf_answer("MEMO.TXT");
    {
        mfile_t *f = mfind("MEMO.TXT");
        CHECK(f && f->size == 18 && memcmp(f->data, "LOAD LETTER\rtables", 18) == 0, R_SAVE,
              "MEMO.TXT bytes (len %d)", f ? (int)f->size : -1);
    }
    CHECK(!g_dirty && strcmp(m_title, "MEMO.TXT") == 0, R_SAVE, "saved: clean, titled");

    /* -- a clean Quit exits at once */
    menu_hit(MENU_FILE, F_QUIT);
    CHECK(m_exits == 1 && m_alerts == 0, R_QUIT, "clean quit");

    /* -- launch 2: the scrap OUTLIVED launch 1 (the shell's) */
    launch();
    CHECK(edit_items[E_PASTE - 1].enabled, R_SCRAP, "Paste enabled at launch 2");
    menu_hit(MENU_EDIT, E_PASTE);
    CHECK(doc_is("LOAD LETTER\rtables"), R_SCRAP, "launch 2 pastes launch 1's cut");

    /* -- dirty Quit asks; Cancel keeps; Don't Save exits */
    menu_hit(MENU_FILE, F_QUIT);
    CHECK(m_alerts == 1 && m_exits == 0 && strstr(m_alert_text, "Save changes") != NULL &&
          strcmp(m_alert_btn[0], "Save") == 0 && strcmp(m_alert_btn[1], "Cancel") == 0 &&
          strcmp(m_alert_btn[2], "Don't Save") == 0, R_QUIT, "dirty quit asks (alerts %d exits %d)", m_alerts, m_exits);
    alert_answer(TBX_ALERT_CANCEL);
    CHECK(m_exits == 0 && doc_is("LOAD LETTER\rtables"), R_QUIT, "Cancel keeps the document");
    hev(TBX_EVT_GOAWAY, 1, 0, 0, 0);                /* the close box asks too */
    CHECK(m_alerts == 2 && m_exits == 0, R_QUIT, "close box asks");
    alert_answer(TBX_ALERT_OTHER);
    CHECK(m_exits == 1, R_QUIT, "Don't Save exits");

    /* -- launch 3: Open reads a DOS file (CR LF / LF -> CR), titles it */
    launch();
    (void)mplant("APPS\\NOTE.TXT", "x\r\ny\nz", 6);
    menu_hit(MENU_FILE, F_OPEN);
    CHECK(m_sf == 1, R_OPEN, "Open asks SFGetFile");
    sf_answer("APPS\\NOTE.TXT");
    CHECK(doc_is("x\ry\rz") && strcmp(m_title, "NOTE.TXT") == 0 && !g_dirty, R_OPEN, "opened");
    /* a too-large file is refused with a notice; the document stays */
    { mfile_t *f = mplant("BIG.TXT", NULL, 5000); memset(f->data, 'a', 5000); }
    menu_hit(MENU_FILE, F_OPEN);
    sf_answer("BIG.TXT");
    CHECK(m_alerts == 1 && doc_is("x\ry\rz"), R_OPEN, "too large refused");
    alert_answer(TBX_ALERT_DEFAULT);
    CHECK(m_exits == 0, R_OPEN, "OK on the notice does nothing more");
    /* dirty + Save from the quit alert: written to the open path, then exits */
    type("!");
    menu_hit(MENU_FILE, F_QUIT);
    alert_answer(TBX_ALERT_DEFAULT);
    {
        mfile_t *f = mfind("APPS\\NOTE.TXT");
        CHECK(f && f->size == 6 && memcmp(f->data, "!x\ry\rz", 6) == 0, R_SAVE, "Save from the alert (the caret opens at 0)");
    }
    CHECK(m_exits == 1, R_QUIT, "then quits");
    /* Cancel in Save As keeps the document open */
    launch();
    type("w");
    menu_hit(MENU_FILE, F_QUIT);
    alert_answer(TBX_ALERT_DEFAULT);
    CHECK(m_sf == 2, R_SAVE, "untitled Save from the alert asks a name");
    sf_answer(NULL);
    CHECK(m_exits == 0 && g_dirty, R_QUIT, "Cancel in Save As keeps it open and dirty");

    /* ehq4: the exact emu history, forty Returns from an empty document.
     * Expected extent is 41 SimpleText lines x 16 (Geneva 12, simpletext-
     * stdfile.md Sec 1.2 [MEAS] line pitch) + two SimpleText 4px insets =
     * 664; view389 is the locked frame's content -> range/value 275. IM
     * TextEdit keeps the insertion point visible; SimpleText's one-pixel,
     * 16-px caret is visible in independent goldens/captures/
     * s8_simpletext_doc_caret.png (system7-decomp): line 40's top 4 + 640 -
     * 275 = 369. No expected value reads the app's geometry or TE record. */
    launch();
    for (int i = 0; i < 40; i++) type("\r");
    CHECK(m_scroll_len == 664 && m_scroll_val == 275, R_SCROLL,
          "40 Returns must enable range275: len=%d value=%d", m_scroll_len, m_scroll_val);
    {
        const op_t *c = &m_ops[m_nops - 1];
        CHECK(!strcmp(c->name,"tbx_fill_rect") && c->a[1] == 4 && c->a[3] == 5 && c->a[2] == 369 && c->a[4] == 385 &&
              c->a[5] == TBX_COLOR_BLACK, R_SCROLL, "caret stays in view at (4,369)..(5,385)");
    }

    /* L005 (audit 2026-10-23-das): LF-only blank lines must survive Open.
     * Normalising rule (itext.c header): CR LF -> one CR, a lone LF -> one CR,
     * a lone CR stays; every line END read is kept, so the line COUNT is
     * preserved. Expected strings are written out, not derived from itext.c. */
    {
        static const struct { const char *in; int n; const char *want; } lf[] = {
            { "ONE\n\nTHREE\n",      11, "ONE\r\rTHREE\r" },       /* the audit file */
            { "ONE\r\n\r\nTHREE\r\n", 14, "ONE\r\rTHREE\r" },    /* CRLF control */
            { "a\n\n\nb",            5, "a\r\r\rb" },
            { "a\r\rb",              4, "a\r\rb" },                 /* lone CRs stay */
            { "a\r\n\nb",           5, "a\r\rb" },                 /* CRLF then LF */
        };
        for (int i = 0; i < 5; i++) {
            mfile_t *f = mplant("LF.TXT", lf[i].in, lf[i].n);
            launch();
            menu_hit(MENU_FILE, F_OPEN);
            sf_answer("LF.TXT");
            CHECK(doc_is(lf[i].want), R_LF, "case %d: LF/CRLF open gave %d bytes", i, g_te.teLength);
            menu_hit(MENU_FILE, F_SAVE);
            f = mfind("LF.TXT");
            CHECK(f && f->size == strlen(lf[i].want) && memcmp(f->data, lf[i].want, (size_t)f->size) == 0,
                  R_LF, "case %d: Save of an opened LF file lost line ends", i);
        }
    }

    /* L014: typing at the 4096-byte limit must say so (TeachText reports a
     * full document with a notice; the Toolbox Gate has no beep verb). The
     * edit is refused and the document is unchanged. */
    {
        mfile_t *f = mplant("K4.TXT", NULL, 4096);
        memset(f->data, 'k', 4096);
        launch();
        menu_hit(MENU_FILE, F_OPEN);
        sf_answer("K4.TXT");
        CHECK(g_te.teLength == 4096 && m_alerts == 0, R_FULL, "4096-byte file opens silently");
        type("!");
        CHECK(g_te.teLength == 4096 && g_te.text[0] == 'k' && !g_dirty, R_FULL, "full document refuses the edit");
        CHECK(m_alerts == 1 && strstr(m_alert_text, "full") != NULL, R_FULL,
              "typing at the limit raised %d alerts (%s)", m_alerts, m_alert_text);
        alert_answer(TBX_ALERT_DEFAULT);
        CHECK(m_exits == 0 && g_te.teLength == 4096, R_FULL, "OK on the full notice does nothing more");
    }

    /* initech-4hkk (audit 2026-10-23-das L011): one-level Undo.
     * GROUND TRUTH (Law 1): Macintosh System Software User's Guide v6.0
     * p.118 (initech-manuals-ref/macos): "Undo. This command undoes your
     * last text-editing action ... type something wrong ... choose Undo to
     * erase the typing and start fresh" -- ONE level, and a typing run is
     * undone as a unit; ../system7-decomp goldens/captures/
     * s8_simpletext_editmenu{,_nosel}.png: after typing, SimpleText's Edit
     * menu shows "Undo ^Z" ENABLED. Every expected string below is written
     * out by hand, never read back from itext.c. */
    {
        static const char doc[] = "Saving tables to disk\rPC LOAD LETTER";   /* 36 bytes */
        launch();
        CHECK(!edit_items[E_UNDO - 1].enabled, R_UNDO, "Undo grayed at launch (nothing to undo)");
        menu_hit(MENU_EDIT, E_UNDO);                  /* a no-op, not a crash */
        CHECK(g_te.teLength == 0 && !g_dirty, R_UNDO, "Undo with no history changes nothing");
        type(doc);
        CHECK(edit_items[E_UNDO - 1].enabled, R_UNDO, "typing enables Undo (s8_simpletext_editmenu.png)");

        /* Cut -> Undo: the exact bytes, the cut range reselected, still dirty */
        menu_hit(MENU_EDIT, E_ALL);
        menu_hit(MENU_EDIT, E_CUT);
        CHECK(g_te.teLength == 0 && m_scrap_len == 36, R_UNDO, "Select All + Cut empties (len %d)", g_te.teLength);
        CHECK(edit_items[E_UNDO - 1].enabled, R_UNDO, "Cut leaves Undo enabled");
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(doc_is(doc), R_UNDO, "Cut -> Undo restores the 36 bytes (got %d)", g_te.teLength);
        CHECK(g_te.selStart == 0 && g_te.selEnd == 36, R_UNDO, "Undo reselects the cut range [%d,%d)", g_te.selStart, g_te.selEnd);
        CHECK(g_dirty, R_UNDO, "the restored untitled document is still unsaved");
        CHECK(find_text("Saving tables to disk", TBX_COLOR_WHITE | TBX_TEXT_GENEVA12, TBX_COLOR_BLACK) != NULL &&
              find_text("PC LOAD LETTER", TBX_COLOR_WHITE | TBX_TEXT_GENEVA12, TBX_COLOR_BLACK) != NULL,
              R_UNDO, "the restored text is PAINTED, selected (white ink on black)");
        /* the restored document saves as the restored bytes */
        menu_hit(MENU_FILE, F_SAVE);
        sf_answer("UNDO.TXT");
        {
            mfile_t *f = mfind("UNDO.TXT");
            CHECK(f && f->size == 36 && memcmp(f->data, doc, 36) == 0, R_UNDO, "UNDO.TXT bytes (len %d)", f ? (int)f->size : -1);
        }
        CHECK(!g_dirty, R_SAVE, "saved clean");

        /* Clear -> Undo over a SAVED document: the dirty state comes back too */
        press(45, 5, 0); track(84, 5, 1);             /* "tables" [7,13) */
        CHECK(g_te.selStart == 7 && g_te.selEnd == 13, R_EDIT, "drag [%d,%d)", g_te.selStart, g_te.selEnd);
        menu_hit(MENU_EDIT, E_CLEAR);
        CHECK(doc_is("Saving  to disk\rPC LOAD LETTER") && g_dirty, R_UNDO, "Clear");
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(doc_is(doc) && g_te.selStart == 7 && g_te.selEnd == 13, R_UNDO, "Clear -> Undo restores text + [7,13)");
        CHECK(!g_dirty, R_UNDO, "Undo back to the saved bytes is clean again");
        /* one level: Undo again undoes the Undo (the classic one-level swap) */
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(doc_is("Saving  to disk\rPC LOAD LETTER") && g_dirty, R_UNDO, "Undo of the Undo re-clears, dirty");
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(doc_is(doc) && !g_dirty, R_UNDO, "and back again, clean");

        /* Paste replacing a range -> Undo (the scrap holds the 36-byte cut) */
        menu_hit(MENU_EDIT, E_PASTE);
        CHECK(doc_is("Saving Saving tables to disk\rPC LOAD LETTER to disk\rPC LOAD LETTER"), R_UNDO, "paste over [7,13)");
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(doc_is(doc) && g_te.selStart == 7 && g_te.selEnd == 13 && !g_dirty, R_UNDO,
              "Paste -> Undo restores \"tables\" selected, clean");

        /* a typing run -- keys and Backspace with no click between -- undoes as ONE */
        press(300, 20, 0); track(300, 20, 1);         /* caret at the end, 36 */
        type("!!x");
        hkey(0x08, 0x0E, 0);                          /* Backspace: "!!" */
        type("?");
        CHECK(doc_is("Saving tables to disk\rPC LOAD LETTER!!?"), R_UNDORUN, "typed run");
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(doc_is(doc), R_UNDORUN, "Undo erases the whole typing run (len %d)", g_te.teLength);
        CHECK(g_te.selStart == 36 && g_te.selEnd == 36 && !g_dirty, R_UNDORUN, "caret back at 36, clean");
        /* a click ends the run: only the second run is undone */
        type("a");
        press(0, 5, 0); track(0, 5, 1);               /* caret at 0 */
        type("b");
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(doc_is("Saving tables to disk\rPC LOAD LETTERa") && g_te.selStart == 0 && g_te.selEnd == 0,
              R_UNDORUN, "a click starts a new run; Undo takes back only \"b\"");

        /* New resets the history (Don't Save the edit first) */
        menu_hit(MENU_FILE, F_NEW);
        alert_answer(TBX_ALERT_OTHER);
        CHECK(g_te.teLength == 0, R_ROUTE, "New empties");
        CHECK(!edit_items[E_UNDO - 1].enabled, R_UNDORESET, "New grays Undo");
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(g_te.teLength == 0 && !g_dirty, R_UNDORESET, "Undo after New brings nothing back (len %d)", g_te.teLength);
        type("z");
        /* Open resets it too */
        menu_hit(MENU_FILE, F_OPEN);
        alert_answer(TBX_ALERT_OTHER);
        sf_answer("UNDO.TXT");
        CHECK(doc_is(doc) && !edit_items[E_UNDO - 1].enabled, R_UNDORESET, "Open grays Undo");
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(doc_is(doc) && !g_dirty, R_UNDORESET, "Undo after Open keeps the opened bytes");
    }
    /* capacity: a refused edit records nothing; the last real edit stays undoable */
    {
        mfile_t *f = mplant("K4.TXT", NULL, 4096);
        memset(f->data, 'k', 4096);
        launch();
        menu_hit(MENU_FILE, F_OPEN);
        sf_answer("K4.TXT");
        menu_hit(MENU_EDIT, E_ALL);
        menu_hit(MENU_EDIT, E_CUT);                   /* scrap = 4096 'k' */
        menu_hit(MENU_EDIT, E_PASTE);                 /* the 4096 come back */
        CHECK(g_te.teLength == 4096, R_UNDO, "paste 4096 (len %d)", g_te.teLength);
        menu_hit(MENU_EDIT, E_PASTE);                 /* refused: full */
        CHECK(m_alerts == 1 && g_te.teLength == 4096, R_FULL, "second paste refused");
        alert_answer(TBX_ALERT_DEFAULT);
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(g_te.teLength == 0 && g_dirty, R_UNDO, "Undo after a refused paste undoes the PASTE that fit (len %d)", g_te.teLength);
        menu_hit(MENU_EDIT, E_UNDO);
        CHECK(g_te.teLength == 4096 && g_te.text[4095] == 'k', R_UNDO, "and redoes it");
    }

    printf("test_itext_route: %ld checks, %ld failures\n", g_checks, g_fails);
    return g_fails == 0 ? 0 : 1;
}
