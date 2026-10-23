/* FILE COPY routing, initech-bf2. Factory C, host. The REAL Calc tenant over
 * the real i123 core, with the Toolbox calls recorded.
 * Ref: PRD 6.3/6.5 and Appendix A (File > Save opens the Dialog Manager's
 * FILE COPY box captioned "Saving tables to disk..."); spec/toolbox_gate.h
 * Sec 12 (TBX_FILECOPY asked by the trap, answered by TBX_EVT_FILECOPY).
 * Expectations are PRD literals typed here, independent of calc_ledger.h.
 * Pixels, the rate and the copied bytes are graded in the emulator
 * (test-flair-file-copy: ppm_flair_file_copy_check + the FAT differential). */
#include <stdio.h>
#include <string.h>
#include "../../os/apps/calc/calc.c"

static unsigned checks, failures, windows, alerts, exits, copies;
static const tbx_menubar_t *bar;
static char caption[160], src[160], dst[160];
#define CHECK(c, s) do { checks++; if (!(c)) { failures++; \
    fprintf(stderr, "FAIL file-copy routing: %s\n", s); } } while (0)

int32_t tbx_register(void) { return TBX_OK; }
int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t g)
{ (void)l; (void)t; (void)r; (void)b; (void)g; return (int32_t)++windows; }
int32_t tbx_set_wtitle(int32_t w, const char *s)
{ (void)w; (void)s; return TBX_OK; }
int32_t tbx_set_mbar(const tbx_menubar_t *b) { bar = b; return TBX_OK; }
int32_t tbx_draw_menubar(void) { return TBX_OK; }
int32_t tbx_draw_text(int32_t w, int32_t x, int32_t y, const char *s,
                      uint32_t f, uint32_t b)
{ (void)w; (void)x; (void)y; (void)s; (void)f; (void)b; return TBX_OK; }
int32_t tbx_fill_rect(int32_t w, int32_t l, int32_t t, int32_t r, int32_t b,
                      uint32_t c)
{ (void)w; (void)l; (void)t; (void)r; (void)b; (void)c; return TBX_OK; }
int32_t tbx_exit(int32_t rc) { (void)rc; exits++; return TBX_OK; }
static char alert_text[200];
static unsigned alert_buttons;
int32_t tbx_alert(const char *text, const char *def,
                  const char *cancel, const char *other)
{
    (void)def; alerts++; alert_buttons = 1u + (cancel != 0) + (other != 0);
    snprintf(alert_text, sizeof alert_text, "%s", text);
    return TBX_OK;
}
/* initech-qxft: a one-file data volume. tables_exists models APPS\TABLES.TXT. */
static int tables_exists, stage_open;
static char stage_name[80], stage_data[1024];
static unsigned stage_len, stage_deletes;
int32_t tbx_file_open(const char *p, uint32_t m)
{ (void)m; return (tables_exists && !strcmp(p, "APPS\\TABLES.TXT")) ? 7 : -1; }
int32_t tbx_file_create(const char *p)
{ snprintf(stage_name, sizeof stage_name, "%s", p); stage_len = 0; stage_open = 1; return 9; }
int32_t tbx_file_write(int32_t h, const void *b, uint32_t n)
{
    if (h != 9 || stage_len + n >= sizeof stage_data) return -1;
    memcpy(stage_data + stage_len, b, n); stage_len += n; stage_data[stage_len] = 0; return (int32_t)n;
}
int32_t tbx_file_close(int32_t h) { (void)h; stage_open = 0; return 0; }
int32_t tbx_file_delete(const char *p) { if (!strcmp(p, stage_name)) stage_deletes++; return 0; }
int32_t tbx_file_copy(const char *c, const char *s, const char *d)
{
    copies++;
    snprintf(caption, sizeof caption, "%s", c ? c : "");
    snprintf(src, sizeof src, "%s", s ? s : "");
    snprintf(dst, sizeof dst, "%s", d ? d : "");
    return TBX_OK;
}

int main(void)
{
    const tbx_menuitem_t *save = NULL;
    int16_t menu_id = 0;
    uint16_t item = 0;
    CHECK(tbx_app_main() == 0, "Calc launches");
    CHECK(windows == 2, "ledger and chart coexist");
    if (bar) for (uint16_t m = 0; m < bar->n_menus; m++) {
        const tbx_menuinfo_t *menu = &bar->menus[m];
        if (strcmp(menu->title, "File")) continue;
        for (uint16_t i = 0; i < menu->n_items; i++) {
            if (strcmp(menu->items[i].text, "Save")) continue;
            save = &menu->items[i]; menu_id = menu->menu_id; item = i + 1;
        }
    }
    CHECK(save != NULL, "File > Save exists");
    CHECK(save && save->enabled, "SAVE_ENABLED File > Save is enabled");
    CHECK(save && save->cmd_char == 'S', "SAVE_KEY File > Save answers Cmd/Ctrl-S");
    if (save) {
        tbx_event_t ev = { TBX_EVT_MENU, TBX_MENU_WORD(menu_id, item), 0, 0, 0, 0 };
        tbx_app_event(&ev);
    }
    CHECK(copies == 1 && alerts == 0,
          "SAVE_MODAL Save asks the Dialog Manager's FILE COPY box exactly once");
    CHECK(!strcmp(caption, "Saving tables to disk..."),
          "SAVE_CAPTION Saving tables to disk...");
    CHECK(src[0] && dst[0] && strcmp(src, dst) != 0,
          "SAVE_PATHS a real copy: a source file to a DIFFERENT new file");
    CHECK(!strcmp(src, "APPS\\LEDGER.TXT") && !strcmp(dst, "APPS\\TABLES.TXT"),
          "SAVE_PATHS the shipped ledger -> APPS\\TABLES.TXT (8.3 names)");
    {   /* The answer: bytes copied. The tenant survives it and can save again. */
        tbx_event_t done = { (int32_t)TBX_EVT_FILECOPY, 123, 0, 0, 0, 0 };
        tbx_app_event(&done);
        tbx_event_t again = { TBX_EVT_MENU, TBX_MENU_WORD(menu_id, item), 0, 0, 0, 0 };
        tbx_app_event(&again);
    }
    CHECK(copies == 2, "SAVE_AGAIN a second Save asks again after the answer");
    CHECK(exits == 0 && windows == 2, "Save retains both Calc windows");
    /* ---- initech-qxft ---- */
    {
        tbx_event_t k;
        unsigned c0;
        { tbx_event_t done = { (int32_t)TBX_EVT_FILECOPY, 123, 0, 0, 0, 0 }; tbx_app_event(&done); } /* the 2nd clean Save lands */
        CHECK(stage_name[0] == 0, "CLEAN_NO_STAGE a clean Save copies the shipped ledger, stages nothing");
        /* L006: an edit reaches the saved file. A1 := 73. */
        k = (tbx_event_t){ keyDown, '7', 0, 0, 0, 0 }; tbx_app_event(&k);
        k.message = '3'; tbx_app_event(&k);
        k.message = '\r'; tbx_app_event(&k);
        c0 = copies;
        { tbx_event_t sv = { TBX_EVT_MENU, TBX_MENU_WORD(menu_id, item), 0, 0, 0, 0 }; tbx_app_event(&sv); }
        CHECK(copies == c0 + 1, "SAVE_EDIT a dirty Save asks the FILE COPY box");
        CHECK(!strcmp(src, "APPS\\TABLES.TMP") && !strcmp(dst, "APPS\\TABLES.TXT"),
              "SAVE_EDIT the edited model is staged, then copied to APPS\\TABLES.TXT");
        CHECK(strstr(stage_data, "\r\n73               TODO_FRAME\r\n") != NULL,
              "SAVE_EDIT the staged file holds the edited A1 (73), not Film Costs");
        CHECK(strstr(stage_data, "Film Costs") == NULL, "SAVE_EDIT the replaced label is gone from the saved file");
        CHECK(strstr(stage_data, "570- format      570-\r\n") && strstr(stage_data, "TOTAL            $112,276.87\r\n"),
              "SAVE_EDIT the untouched PRD rows (570-, the 116% total line) are saved");
        { tbx_event_t done = { (int32_t)TBX_EVT_FILECOPY, 50, 0, 0, 0, 0 }; tbx_app_event(&done); tables_exists = 1; }
        CHECK(stage_deletes == 1, "STAGE_CLEANED the stage file is deleted after the copy");
        /* L009: a second Save onto the existing TABLES.TXT asks before replacing. */
        alerts = 0; c0 = copies;
        { tbx_event_t sv = { TBX_EVT_MENU, TBX_MENU_WORD(menu_id, item), 0, 0, 0, 0 }; tbx_app_event(&sv); }
        CHECK(alerts == 1 && copies == c0 && alert_buttons == 2, "REPLACE_ASKS Save onto an existing TABLES.TXT asks (Yes / No) and copies nothing yet");
        CHECK(strstr(alert_text, "TABLES.TXT") != NULL && strstr(alert_text, "replace") != NULL, "REPLACE_NAMES the alert names TABLES.TXT and says replace");
        { tbx_event_t a = { (int32_t)TBX_EVT_ALERT, TBX_ALERT_CANCEL, 0, 0, 0, 0 }; tbx_app_event(&a); }
        CHECK(copies == c0, "REPLACE_CANCEL Cancel leaves the existing file alone");
        { tbx_event_t sv = { TBX_EVT_MENU, TBX_MENU_WORD(menu_id, item), 0, 0, 0, 0 }; tbx_app_event(&sv); }
        { tbx_event_t a = { (int32_t)TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0, 0 }; tbx_app_event(&a); }
        CHECK(copies == c0 + 1, "REPLACE_YES Replace performs the copy");
        { tbx_event_t done = { (int32_t)TBX_EVT_FILECOPY, 50, 0, 0, 0, 0 }; tbx_app_event(&done); }
        /* L007 + L006: Quit on a dirty sheet, answer Save: saved (after the replace question), then the session ends. */
        k = (tbx_event_t){ keyDown, '9', 0, 0, 0, 0 }; tbx_app_event(&k);
        k.message = '\r'; tbx_app_event(&k);
        alerts = 0; c0 = copies;
        { tbx_event_t q = { TBX_EVT_MENU, TBX_MENU_WORD(menu_id, 2), 0, 0, 0, 0 }; tbx_app_event(&q); }
        CHECK(alerts == 1 && alert_buttons == 3 && exits == 0, "QUITSAVE Quit on an edited sheet asks first");
        { tbx_event_t a = { (int32_t)TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0, 0 }; tbx_app_event(&a); }
        CHECK(alerts == 2 && exits == 0, "QUITSAVE Save over an existing TABLES.TXT asks the replace question");
        { tbx_event_t a = { (int32_t)TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0, 0 }; tbx_app_event(&a); }
        CHECK(copies == c0 + 1 && exits == 0, "QUITSAVE Replace copies, and the session is still open until the copy lands");
        CHECK(strstr(stage_data, "\r\n9 ") != NULL, "QUITSAVE the saved file holds the second edit (9)");
        { tbx_event_t done = { (int32_t)TBX_EVT_FILECOPY, 50, 0, 0, 0, 0 }; tbx_app_event(&done); }
        CHECK(exits == 1, "QUITSAVE a successful Save ends the session");
    }
    printf("test_calc_file_copy: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
