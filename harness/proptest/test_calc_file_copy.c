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
int32_t tbx_alert(const char *text, const char *def,
                  const char *cancel, const char *other)
{ (void)text; (void)def; (void)cancel; (void)other; alerts++; return TBX_OK; }
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
    printf("test_calc_file_copy: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
