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
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../os/apps/itext/itext.c"   /* the application under test */

static long g_checks, g_fails;
static int  g_said[16];
static const char *const g_rn[] = { "copy", "save", "quit", "inverse", "route",
                                    "scrap", "file", "menu", "open", "edit" };
enum { R_COPY, R_SAVE, R_QUIT, R_INV, R_ROUTE, R_SCRAP, R_FILE, R_MENU, R_OPEN, R_EDIT };
#define CHECK(c, r, ...) do { g_checks++; if (!(c)) { g_fails++; if (!g_said[r]) {    \
    g_said[r] = 1; printf("RED-REASON %s: ", g_rn[r]); printf(__VA_ARGS__);           \
    printf(" (line %d)\n", __LINE__); } else { printf("  FAIL line %d\n", __LINE__); } } } while (0)

/* ---- the recording mock -------------------------------------------------- */
#define MAXOPS 512
typedef struct { int kind, x, y, r, b; uint32_t fg, bg; char s[96]; } op_t; /* 0 text 1 fill */
static op_t   m_ops[MAXOPS];
static int    m_nops;
static int    m_exits, m_alerts, m_track, m_sf;          /* sf: 1 get, 2 put */
static char   m_alert_text[128], m_alert_btn[3][24];
static char   m_sf_prompt[64], m_sf_name[24];
static char  *m_sf_reply;
static char   m_title[32];
static int    m_scroll_len, m_scroll_val;
/* the shell's desk scrap: survives every launch */
static char   m_scrap[8192];
static int    m_scrap_len = -1;
static int    m_scrap_puts;
/* the volume */
typedef struct { char name[64]; char data[6000]; int len; int used; } mfile_t;
static mfile_t m_files[8];
static struct { int f; int pos; int wr; } m_h[8];

int32_t tbx_register(void) { return TBX_OK; }
int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t g)
{ (void)l; (void)t; (void)r; (void)b; (void)g; return 1; }
int32_t tbx_set_wtitle(int32_t w, const char *s) { (void)w; snprintf(m_title, sizeof m_title, "%s", s); return TBX_OK; }
int32_t tbx_draw_text(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{
    (void)w;
    if (m_nops < MAXOPS) { op_t *o = &m_ops[m_nops++]; o->kind = 0; o->x = x; o->y = y;
        o->fg = f; o->bg = b; snprintf(o->s, sizeof o->s, "%s", s); }
    return TBX_OK;
}
int32_t tbx_fill_rect(int32_t w, int32_t l, int32_t t, int32_t r, int32_t b, uint32_t c)
{
    (void)w;
    if (m_nops < MAXOPS) { op_t *o = &m_ops[m_nops++]; o->kind = 1; o->x = l; o->y = t;
        o->r = r; o->b = b; o->fg = c; o->s[0] = 0; }
    return TBX_OK;
}
int32_t tbx_draw_cells(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{ (void)w; (void)x; (void)y; (void)s; (void)f; (void)b; return TBX_OK; }
int32_t tbx_set_mbar(const tbx_menubar_t *b) { (void)b; return TBX_OK; }
int32_t tbx_draw_menubar(void) { return TBX_OK; }
int32_t tbx_exit(int32_t rc) { (void)rc; m_exits++; return TBX_OK; }
int32_t tbx_trap(uint32_t c, uint32_t n, const uint32_t *a) { (void)c; (void)n; (void)a; return TBX_ERR_BADCODE; }
int32_t tbx_alert(const char *t, const char *d, const char *c, const char *o)
{
    m_alerts++;
    snprintf(m_alert_text, sizeof m_alert_text, "%s", t);
    snprintf(m_alert_btn[0], 24, "%s", d ? d : "");
    snprintf(m_alert_btn[1], 24, "%s", c ? c : "");
    snprintf(m_alert_btn[2], 24, "%s", o ? o : "");
    return TBX_OK;
}
int32_t tbx_scrap_put(const void *buf, uint32_t len)
{
    if (len > sizeof m_scrap) return TBX_ERR_BADARG;
    memcpy(m_scrap, buf, len); m_scrap_len = (int)len; m_scrap_puts++;
    return TBX_OK;
}
int32_t tbx_scrap_get(void *buf, uint32_t bufsz)
{
    if (m_scrap_len < 0) return TBX_ERR_NOTYPE;
    if (buf == (void *)0) return m_scrap_len;
    if (bufsz < (uint32_t)m_scrap_len) return TBX_ERR_BADARG;
    memcpy(buf, m_scrap, (size_t)m_scrap_len);
    return m_scrap_len;
}
int32_t tbx_sf_getfile(char *reply) { m_sf = 1; m_sf_reply = reply; return TBX_OK; }
int32_t tbx_sf_putfile(const char *p, const char *n, char *reply)
{ m_sf = 2; m_sf_reply = reply; snprintf(m_sf_prompt, 64, "%s", p); snprintf(m_sf_name, 24, "%s", n); return TBX_OK; }
int32_t tbx_track_mouse(void) { m_track++; return TBX_OK; }
int32_t tbx_set_scroll(int32_t w, int32_t len, int32_t v)
{
    int max = len - CONT_H;
    (void)w;
    if (max < 0) max = 0;
    if (v > max) v = max;
    if (v < 0) v = 0;
    m_scroll_len = len; m_scroll_val = v;
    return v;
}
int32_t tbx_get_scroll(int32_t w) { (void)w; return m_scroll_val; }
int32_t tbx_global_to_local(int32_t w, int32_t h, int32_t v)
{ (void)w; return (int32_t)(((uint32_t)(uint16_t)(v - 66) << 16) | (uint16_t)(h - 10)); }

static mfile_t *mfind(const char *p)
{
    for (int i = 0; i < 8; i++) if (m_files[i].used && strcmp(m_files[i].name, p) == 0) return &m_files[i];
    return NULL;
}
int32_t tbx_file_create(const char *path)
{
    mfile_t *f = mfind(path);
    if (!f) for (int i = 0; i < 8 && !f; i++) if (!m_files[i].used) { f = &m_files[i]; f->used = 1; snprintf(f->name, 64, "%s", path); }
    f->len = 0;
    for (int h = 0; h < 8; h++) if (!m_h[h].f) { m_h[h].f = (int)(f - m_files) + 1; m_h[h].pos = 0; m_h[h].wr = 1; return 5 + h; }
    return TBX_ERR_BUSY;
}
int32_t tbx_file_open(const char *path, uint32_t mode)
{
    mfile_t *f = mfind(path);
    if (!f) return TBX_ERR_DOS(2);
    for (int h = 0; h < 8; h++) if (!m_h[h].f) { m_h[h].f = (int)(f - m_files) + 1; m_h[h].pos = 0; m_h[h].wr = mode != 0; return 5 + h; }
    return TBX_ERR_BUSY;
}
int32_t tbx_file_read(int32_t h, void *buf, uint32_t n)
{
    mfile_t *f = &m_files[m_h[h - 5].f - 1];
    int k = f->len - m_h[h - 5].pos;
    if (k > (int)n) k = (int)n;
    memcpy(buf, f->data + m_h[h - 5].pos, (size_t)k); m_h[h - 5].pos += k;
    return k;
}
int32_t tbx_file_write(int32_t h, const void *buf, uint32_t n)
{
    mfile_t *f = &m_files[m_h[h - 5].f - 1];
    memcpy(f->data + m_h[h - 5].pos, buf, n); m_h[h - 5].pos += (int)n;
    if (m_h[h - 5].pos > f->len) f->len = m_h[h - 5].pos;
    return (int32_t)n;
}
int32_t tbx_file_seek(int32_t h, int32_t d, uint32_t o) { (void)h; (void)d; (void)o; return 0; }
int32_t tbx_file_close(int32_t h) { m_h[h - 5].f = 0; return TBX_OK; }
int32_t tbx_file_delete(const char *p) { mfile_t *f = mfind(p); if (f) f->used = 0; return TBX_OK; }

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
    memset(&g_te, 0, sizeof g_te);
    g_win = 0; g_dirty = 0; g_active = 1; g_scroll = 0; g_anchor = 0;
    g_scrap_has = 0; g_after = 0; g_sf_kind = 0; g_path[0] = 0;
    m_exits = m_alerts = m_track = m_sf = 0;
    CHECK(tbx_app_main() == 0, R_ROUTE, "launch");
    hev(updateEvt, 1, 0, 0, 0);
}

static const op_t *find_text(const char *s, uint32_t fg, uint32_t bg)
{
    for (int i = 0; i < m_nops; i++)
        if (m_ops[i].kind == 0 && strcmp(m_ops[i].s, s) == 0 && m_ops[i].fg == fg && m_ops[i].bg == bg)
            return &m_ops[i];
    return NULL;
}

int main(void)
{
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
        CHECK(c->kind == 1 && c->r - c->x == 1 && c->b - c->y == 11 && c->fg == TBX_COLOR_BLACK,
              R_INV, "caret rect %d,%d,%d,%d", c->x, c->y, c->r, c->b);
    }
    hkey(0, 0, FLAIR_EVT_MOD_CONTROL_KEY);           /* an unbound chord: nothing */
    hkey('q', 0x10, FLAIR_EVT_MOD_CONTROL_KEY);
    CHECK(doc_is("Saving tables to disk\rPC LOAD LETTER"), R_EDIT, "a Ctrl chord types nothing");

    /* -- drag-select "tables" (Geneva 9: "Saving " 38 px, "tables" ends at 69) */
    press(38, 5, 0);
    CHECK(m_track == 1, R_ROUTE, "a press asks the shell to track");
    track(60, 5, 0);
    track(69, 5, 1);
    CHECK(g_te.selStart == 7 && g_te.selEnd == 13, R_EDIT, "drag selection [%d,%d)", g_te.selStart, g_te.selEnd);
    CHECK(find_text("tables", TBX_COLOR_WHITE | TBX_TEXT_GENEVA9, TBX_COLOR_BLACK) != NULL &&
          find_text("Saving ", TBX_COLOR_BLACK | TBX_TEXT_GENEVA9, TBX_COLOR_WHITE) != NULL,
          R_INV, "the range is black with white ink, the rest black on white");
    CHECK(edit_items[E_CUT - 1].enabled && edit_items[E_COPY - 1].enabled &&
          edit_items[E_CLEAR - 1].enabled, R_MENU, "a range enables Cut/Copy/Clear");

    /* -- Copy: the desk scrap holds exactly "tables"; Paste comes alive */
    menu_hit(MENU_EDIT, E_COPY);
    CHECK(m_scrap_len == 6 && memcmp(m_scrap, "tables", 6) == 0, R_COPY,
          "scrap after Copy has %d bytes", m_scrap_len);
    CHECK(edit_items[E_PASTE - 1].enabled, R_COPY, "Paste enabled after Copy");

    /* -- click past the end of line 1, Return, Paste */
    press(300, 16, 0);
    track(300, 16, 1);
    CHECK(g_te.selStart == g_te.teLength && g_te.selEnd == g_te.teLength, R_EDIT, "caret at the end");
    type("\r");
    menu_hit(MENU_EDIT, E_PASTE);
    CHECK(doc_is("Saving tables to disk\rPC LOAD LETTER\rtables"), R_COPY, "pasted text");

    /* -- shift-click extends from the caret back to "LOAD" (line 1, x 17) */
    press(17, 16, 1);
    CHECK(g_te.selStart == 25 && g_te.selEnd == g_te.teLength, R_EDIT, "shift-click [%d,%d)", g_te.selStart, g_te.selEnd);
    track(17, 16, 1);

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
        CHECK(f && f->len == 18 && memcmp(f->data, "LOAD LETTER\rtables", 18) == 0, R_SAVE,
              "MEMO.TXT bytes (len %d)", f ? f->len : -1);
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
    { mfile_t *f = &m_files[7]; f->used = 1; snprintf(f->name, 64, "APPS\\NOTE.TXT"); memcpy(f->data, "x\r\ny\nz", 6); f->len = 6; }
    menu_hit(MENU_FILE, F_OPEN);
    CHECK(m_sf == 1, R_OPEN, "Open asks SFGetFile");
    sf_answer("APPS\\NOTE.TXT");
    CHECK(doc_is("x\ry\rz") && strcmp(m_title, "NOTE.TXT") == 0 && !g_dirty, R_OPEN, "opened");
    /* a too-large file is refused with a notice; the document stays */
    { mfile_t *f = &m_files[6]; f->used = 1; snprintf(f->name, 64, "BIG.TXT"); memset(f->data, 'a', 5000); f->len = 5000; }
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
        CHECK(f && f->len == 6 && memcmp(f->data, "!x\ry\rz", 6) == 0, R_SAVE, "Save from the alert (the caret opens at 0)");
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

    printf("test_itext_route: %ld checks, %ld failures\n", g_checks, g_fails);
    return g_fails == 0 ? 0 : 1;
}
