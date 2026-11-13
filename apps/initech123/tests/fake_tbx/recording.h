/* Recording Toolbox and fork-based driver, extracted from the legacy quit oracle.
 * Included after face_a.c: tests inspect application state in the same TU. */
/* ---- shared counters (a fork()ed launch reports through these) ---------- */
static volatile int *g_ctr;            /* [0] checks, [1] failures */
#define CHECK(cond, id, msg) do { g_ctr[0]++; if (!(cond)) { g_ctr[1]++; \
    fprintf(stderr, "  FAIL %s %s (line %d)\n", (id), (msg), __LINE__); } } while (0)

/* ---- the recording mock ------------------------------------------------- */
#define SCR_W 76   /* 613-px content / 8 since bead initech-tdnl.51 (was 77) */
#define SCR_H 24
static char m_scr[SCR_H][SCR_W + 1];
/* B31 (initech-umfi): 'N' where the last cell draw was on NAVY (the pointer,
 * the borders, the mode indicator), ' ' elsewhere -- what test_i123_hit.c
 * reads the highlight from. */
static char m_bg[SCR_H][SCR_W + 1];
static int  m_exit_calls, m_exit_rc;
static int  m_alerts;
static unsigned m_beeps;              /* TBX_BEEP, also used by entry oracle */
static char m_alert_text[256], m_alert_btn[3][32];
static int32_t m_alert_rc = TBX_OK;
static int  m_fail_create;              /* CREATE answers ERR_DOS(5)          */
static int  m_fail_write;               /* WRITE writes nothing               */
static const char *m_dir;
static FILE *m_fh[8];
static char  m_fpath[8][300];
/* B08: injectable guest clock; the production gate requires module/BSS. */
static cp_time_t m_clock = {1992, 3, 9, 12, 0, 0, 0};
static int32_t m_clock_rc = TBX_OK;
static unsigned m_clock_reads;
int32_t tbx_get_clock(void *t)
{ m_clock_reads++; if (m_clock_rc == TBX_OK) *(cp_time_t *)t = m_clock; return m_clock_rc; }

int32_t tbx_register(void) { return TBX_OK; }
int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t g)
{ (void)l; (void)t; (void)r; (void)b; (void)g; return 1; }
int32_t tbx_set_wtitle(int32_t w, const char *s) { (void)w; (void)s; return TBX_OK; }
/* B31: the window's content origin, GLOBAL (spec/toolbox_gate.h Sec 3
 * CONTENTORIGIN = (top << 16) | left). The 123 frame (0,40)..(640,480) puts
 * its content at (6,62) (spec/flair_i123_traces.mk GEOMETRY, tdnl.51). */
#define M_ORG_H 6
#define M_ORG_V 62
int32_t tbx_content_origin(int32_t w) { (void)w; return (M_ORG_V << 16) | M_ORG_H; }
/* B12 (initech-89rc): the window's content SIZE (spec/toolbox_gate.h Sec 3
 * CONTENT_SIZE = (height << 16) | width). Default: the 640 x 440 frame's
 * 613 x 397 content (tdnl.51); test_i123_resize.c sets other sizes. The
 * vertical bar verb is recorded so an oracle can prove 123 never fakes a
 * scroll range (the bars stay the Toolbox's DISABLED ones, Sec 15). */
static int m_cw = 613, m_ch = 397, m_setscroll;
int32_t tbx_content_size(int32_t w) { (void)w; return (m_ch << 16) | m_cw; }
int32_t tbx_set_scroll(int32_t w, int32_t len, int32_t v) { (void)w; (void)len; m_setscroll++; return v; }
int32_t tbx_draw_text(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{ (void)w; (void)x; (void)y; (void)s; (void)f; (void)b; return TBX_OK; }
int32_t tbx_fill_rect(int32_t w, int32_t l, int32_t t, int32_t r, int32_t b, uint32_t c)
{ (void)w; (void)l; (void)t; (void)r; (void)b; (void)c; return TBX_OK; }
int32_t tbx_set_mbar(const tbx_menubar_t *b) { (void)b; return TBX_OK; }
int32_t tbx_draw_menubar(void) { return TBX_OK; }
int32_t tbx_trap(uint32_t c, uint32_t n, const uint32_t *a)
{ (void)a; if (c == TBX_BEEP && n == 0) { m_beeps++; return TBX_OK; } return TBX_ERR_BADCODE; }

int32_t tbx_draw_cells(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{
    int col = (x - 1) / 8, line = y / 16;
    (void)w; (void)f; (void)b;
    if (line < 0 || line >= SCR_H || col < 0) return TBX_ERR_BADARG;
    for (int i = 0; s[i] && col + i < SCR_W; i++) {
        m_scr[line][col + i] = s[i];
        m_bg[line][col + i] = b == TBX_COLOR_NAVY ? 'N' : ' ';
    }
    return TBX_OK;
}

int32_t tbx_exit(int32_t rc) { m_exit_calls++; m_exit_rc = rc; return TBX_OK; }

int32_t tbx_alert(const char *text, const char *def, const char *cancel, const char *other)
{
    m_alerts++;
    snprintf(m_alert_text, sizeof m_alert_text, "%s", text ? text : "");
    snprintf(m_alert_btn[0], 32, "%s", def ? def : "");
    snprintf(m_alert_btn[1], 32, "%s", cancel ? cancel : "");
    snprintf(m_alert_btn[2], 32, "%s", other ? other : "");
    return m_alert_rc;
}

/* Files: DOS names are case-insensitive -- the host file is the UPPER name. */
static void host_name(const char *path, char *out)
{
    const char *p = path;
    char *o;
    if (p[0] && p[1] == ':') p += 2;
    if (*p == '\\') p++;
    snprintf(out, 300, "%s/", m_dir);
    o = out + strlen(out);
    for (; *p; p++) *o++ = (char)((*p >= 'a' && *p <= 'z') ? *p - 32 : *p);
    *o = 0;
}
static int32_t new_handle(FILE *f, const char *hp)
{
    for (int i = 0; i < 8; i++) if (!m_fh[i]) { m_fh[i] = f; snprintf(m_fpath[i], 300, "%s", hp); return 5 + i; }
    fclose(f); return TBX_ERR_DOS(4);
}
static FILE *fh(int32_t h) { return (h >= 5 && h < 13) ? m_fh[h - 5] : 0; }
int32_t tbx_file_create(const char *path)
{
    char hp[300]; FILE *f;
    if (m_fail_create) return TBX_ERR_DOS(5);
    host_name(path, hp);
    f = fopen(hp, "w+b");
    return f ? new_handle(f, hp) : TBX_ERR_DOS(5);
}
int32_t tbx_file_open(const char *path, uint32_t mode)
{
    char hp[300]; FILE *f;
    (void)mode;
    host_name(path, hp);
    f = fopen(hp, "r+b");
    return f ? new_handle(f, hp) : TBX_ERR_DOS(2);
}
int32_t tbx_file_read(int32_t h, void *b, uint32_t n)
{ FILE *f = fh(h); return f ? (int32_t)fread(b, 1, n, f) : TBX_ERR_BADARG; }
int32_t tbx_file_write(int32_t h, const void *b, uint32_t n)
{
    FILE *f = fh(h);
    if (!f) return TBX_ERR_BADARG;
    if (m_fail_write) return 0;         /* a full disk: nothing written */
    return (int32_t)fwrite(b, 1, n, f);
}
int32_t tbx_file_seek(int32_t h, int32_t d, uint32_t o)
{
    FILE *f = fh(h);
    if (!f || fseek(f, d, o == TBX_FILE_FROM_END ? SEEK_END : o == TBX_FILE_FROM_HERE ? SEEK_CUR : SEEK_SET)) return TBX_ERR_BADARG;
    return (int32_t)ftell(f);
}
int32_t tbx_file_close(int32_t h)
{ FILE *f = fh(h); if (!f) return TBX_ERR_BADARG; fclose(f); m_fh[h - 5] = 0; return TBX_OK; }
int32_t tbx_file_delete(const char *path)
{ char hp[300]; host_name(path, hp); return remove(hp) == 0 ? TBX_OK : TBX_ERR_DOS(2); }
/* Sec 10b CREATE_NEW (initech-mcep): DOS 3.3 AH=5Bh -- never truncates. */
int32_t tbx_file_create_new(const char *path)
{
    char hp[300]; FILE *f;
    host_name(path, hp);
    if ((f = fopen(hp, "rb")) != NULL) { fclose(f); return TBX_ERR_DOS(0x50); }
    return tbx_file_create(path);
}
/* Sec 10b RENAME (initech-mcep): DOS 3.3 AH=56h -- an existing target is
 * access denied, a missing source is file not found. */
int32_t tbx_file_rename(const char *o, const char *n)
{
    char ho[300], hn[300]; FILE *f;
    host_name(o, ho); host_name(n, hn);
    if ((f = fopen(hn, "rb")) != NULL) { fclose(f); return TBX_ERR_DOS(5); }
    return rename(ho, hn) == 0 ? TBX_OK : TBX_ERR_DOS(2);
}

/* ---- driving the application ------------------------------------------- */
static void ev(int32_t what, int32_t msg)
{
    tbx_event_t e;
    memset(&e, 0, sizeof e);
    e.what = what; e.message = msg;
    tbx_app_event(&e);
}
static void tk(char c)
{
    if (c == '\r')      ev(keyDown, (0x1C << 8) | '\r');
    else if (c == 27)   ev(keyDown, (0x01 << 8) | 27);
    else                ev(keyDown, (uint8_t)c);
}
static void tks(const char *s) { while (*s) tk(*s++); }
static void right(void) { ev(keyDown, 0x4D << 8); }
static void menu_quit(void) { ev(TBX_EVT_MENU, TBX_MENU_WORD(129, 1)); }   /* File > Quit / Ctrl-Q */
static void close_box(void) { ev(TBX_EVT_GOAWAY, 1); }
static void answer(int item) { ev(TBX_EVT_ALERT, item); ev(updateEvt, 0); }

static int line_has(int line, const char *want) { return strstr(m_scr[line], want) != 0; }
static void cell(uint16_t c, uint16_t r, char *out, uint32_t n)
{ i123_cell_content(&g_sheet, i123_find(&g_sheet, c, r), out, n); }
static int file_exists(const char *name)
{ char hp[300]; FILE *f; host_name(name, hp); f = fopen(hp, "rb"); if (f) fclose(f); return f != 0; }

/* The MINT01 first row (mint/mint01.steps; what test-flair-i123 types). */
static void type_row(void) { tks("Hello"); right(); tks("123"); right(); tks("1.5"); right(); tks("-7\r"); }

#define ALERT_LINE1 "WORKSHEET CHANGES NOT SAVED!"
static int asked_properly(void)
{
    return m_alerts > 0 && strncmp(m_alert_text, ALERT_LINE1, strlen(ALERT_LINE1)) == 0 &&
           strchr(m_alert_text, '\r') != 0 &&
           strcmp(m_alert_btn[0], "Save") == 0 && strcmp(m_alert_btn[1], "Cancel") == 0 &&
           strcmp(m_alert_btn[2], "Don't Save") == 0;
}

static void launch(const char *name, void (*script)(void))
{
    pid_t p;
    fflush(stdout); fflush(stderr);
    p = fork();
    if (p < 0) { perror("fork"); exit(2); }
    if (p == 0) {
        for (int i = 0; i < SCR_H; i++) {
            memset(m_scr[i], ' ', SCR_W); m_scr[i][SCR_W] = 0;
            memset(m_bg[i], ' ', SCR_W); m_bg[i][SCR_W] = 0;
        }
        if (tbx_app_main() != 0) { CHECK(0, "Q0", "tbx_app_main failed"); _exit(0); }
        ev(updateEvt, 0);
        script();
        _exit(0);
    }
    int st = 0;
    waitpid(p, &st, 0);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) { g_ctr[0]++; g_ctr[1]++; fprintf(stderr, "  FAIL %s: the launch died (status 0x%x)\n", name, st); }
}
