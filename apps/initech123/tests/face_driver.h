/* Recording Toolbox and fork-based driver, extracted from the legacy quit oracle.
 * Included after face_a.c: tests inspect application state in the same TU. */
/* ---- shared counters (a fork()ed launch reports through these) ---------- */
static volatile int *g_ctr;            /* [0] checks, [1] failures */
#define CHECK(cond, id, msg) do { g_ctr[0]++; if (!(cond)) { g_ctr[1]++; \
    fprintf(stderr, "  FAIL %s %s (line %d)\n", (id), (msg), __LINE__); } } while (0)

/* The shared fake owns all Toolbox behavior; this driver owns app state. */
#include "tbx_host.h"
#define SCR_W TBX_HOST_W
#define SCR_H TBX_HOST_H
#define M_ORG_H 6
#define M_ORG_V 62
static const char *m_dir;
#define m_scr (tbx_host.scr)
#define m_bg (tbx_host.bg)
#define m_exit_calls (tbx_host.exit_calls)
#define m_exit_rc (tbx_host.exit_rc)
#define m_alerts (tbx_host.alerts)
#define m_beeps (tbx_host.beeps)
#define m_alert_text (tbx_host.alert_text)
#define m_alert_btn (tbx_host.alert_btn)
#define m_alert_rc (tbx_host.alert_rc)
#define m_fail_create (tbx_host.fail_create)
#define m_fail_write (tbx_host.fail_write)
#define m_clock (tbx_host.clock)
#define m_clock_rc (tbx_host.clock_rc)
#define m_clock_reads (tbx_host.clock_reads)
#define m_cw (tbx_host.cw)
#define m_ch (tbx_host.ch)
#define m_setscroll (tbx_host.setscroll)
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
static int file_exists(const char *name) { return tbx_host_file(name) != 0; }

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
        tbx_host_reset();
        if (!tbx_host_import(m_dir)) _exit(2);
        for (int i = 0; i < SCR_H; i++) {
            memset(m_scr[i], ' ', SCR_W); m_scr[i][SCR_W] = 0;
            memset(m_bg[i], ' ', SCR_W); m_bg[i][SCR_W] = 0;
        }
        if (tbx_app_main() != 0) { CHECK(0, "Q0", "tbx_app_main failed"); _exit(0); }
        ev(updateEvt, 0);
        script();
        if (!tbx_host_export(m_dir)) _exit(2);
        _exit(0);
    }
    int st = 0;
    waitpid(p, &st, 0);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) { g_ctr[0]++; g_ctr[1]++; fprintf(stderr, "  FAIL %s: the launch died (status 0x%x)\n", name, st); }
}
