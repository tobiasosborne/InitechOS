/* FILE COPY routing, initech-bf2. Factory C, host. The REAL Calc tenant over
 * the real i123 core, with the Toolbox calls recorded.
 * Ref: PRD 6.3/6.5 and Appendix A (File > Save opens the Dialog Manager's
 * FILE COPY box captioned "Saving tables to disk..."); spec/toolbox_gate.h
 * Sec 12 (TBX_FILECOPY asked by the trap, answered by TBX_EVT_FILECOPY).
 * Expectations are PRD literals typed here, independent of calc_ledger.h.
 * Pixels, the rate and the copied bytes are graded in the emulator
 * (test-flair-file-copy: ppm_flair_file_copy_check + the FAT differential). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../calc.c"
#include "tbx_host.h"

static unsigned checks, failures;
#define windows tbx_host.nwindows
#define alerts tbx_host.alerts
#define exits tbx_host.exit_calls
#define bar tbx_host.bar
#define caption tbx_host.copy_caption
#define src tbx_host.copy_src
#define dst tbx_host.copy_dst
#define alert_text tbx_host.alert_text
#define alert_buttons (1+!!tbx_host.alert_btn[1][0]+!!tbx_host.alert_btn[2][0])
#define CHECK(c, s) do { checks++; if (!(c)) { failures++; fprintf(stderr,"FAIL file-copy routing: %s\n",s); } } while(0)
static unsigned calls(const char *name) { unsigned n=0;for(unsigned i=0;i<tbx_host.nops;i++)if(!strcmp(tbx_host.ops[i].name,name))n++;return n; }
#define copies calls("tbx_file_copy")
#define stage_deletes calls("tbx_file_delete")
static char stage_name[128],stage_data[2048];
static int stranger_changed(void) {
 const tbx_host_file_t *f=tbx_host_file("APPS\\TABLES.TMP");
 return !f || f->size!=8 || memcmp(f->data,"stranger",8);
}
#define tmp_clobbered stranger_changed()
/* Explicit completion fixture: OS copies bytes before sending FILECOPY done. */
static void deliver(const tbx_event_t *e) {
 if(e->what==TBX_EVT_FILECOPY && e->message>=0) {
  const tbx_host_file_t *f=tbx_host_file(src);
  if(f) tbx_host_put_file(dst,f->data,f->size);
 }
 if(!tbx_host_push_event(e))exit(2);
 (void)tbx_host_dispatch(tbx_app_event);
 if(src[0] && strcmp(src,"APPS\\LEDGER.TXT")) {
  const tbx_host_file_t *f=tbx_host_file(src);
  if(f && f->size<sizeof stage_data) {
   snprintf(stage_name,sizeof stage_name,"%s",src);
   memcpy(stage_data,f->data,f->size);stage_data[f->size]=0;
  }
 }
}
#define tbx_app_event deliver

int main(void)
{
    tbx_host_reset();
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
        { tbx_event_t done = { (int32_t)TBX_EVT_FILECOPY, 50, 0, 0, 0, 0 }; tbx_app_event(&done); /* completion installed TABLES.TXT */ }
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
        /* Save the second edit while the tenant is live; repeat-save checks
         * below must run before the final Quit, never by clearing exit_calls. */
        k = (tbx_event_t){ keyDown, '9', 0, 0, 0, 0 }; tbx_app_event(&k);
        k.message = '\r'; tbx_app_event(&k);
        { tbx_event_t sv = { TBX_EVT_MENU, TBX_MENU_WORD(menu_id,item), 0, 0, 0, 0 }; tbx_app_event(&sv); }
        { tbx_event_t a = { (int32_t)TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0, 0 }; tbx_app_event(&a); }
        { tbx_event_t done = { (int32_t)TBX_EVT_FILECOPY, 50, 0, 0, 0, 0 }; tbx_app_event(&done); }
    }
    /* The model is saved EVERY time once edited. Use the live tenant: no new edit,
     * Save again (TABLES.TXT exists -> Yes): the source is still the stage
     * holding the edited model, never the shipped LEDGER.TXT. */
    {
        unsigned c0 = copies;
        tbx_event_t sv = { TBX_EVT_MENU, TBX_MENU_WORD(menu_id, item), 0, 0, 0, 0 };
        tbx_event_t yes = { (int32_t)TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0, 0 };
        tbx_event_t done = { (int32_t)TBX_EVT_FILECOPY, 50, 0, 0, 0, 0 };
        tbx_app_event(&sv); tbx_app_event(&yes);
        CHECK(copies == c0 + 1 && strcmp(src, "APPS\\LEDGER.TXT") != 0 && strstr(stage_data, "\r\n9 ") != NULL,
              "SAVE_REPEAT a second Save with no new edit still saves the edited model, not the stale ledger");
        tbx_app_event(&done);
        /* A stranger's APPS\TABLES.TMP must survive. */
        tbx_host_put_file("APPS\\TABLES.TMP","stranger",8); c0 = copies; stage_name[0] = 0;
        tbx_app_event(&sv); tbx_app_event(&yes);
        CHECK(copies == c0 + 1 && !tmp_clobbered && strcmp(src, "APPS\\TABLES.TMP") != 0,
              "STAGE_CLOBBER an existing APPS\\TABLES.TMP is neither truncated nor used as the stage");
        tbx_app_event(&done);
        CHECK(!tmp_clobbered, "STAGE_CLOBBER completion deletes only the stage this Save created");
    }
    /* L007 + L006: make another edit, then Quit > Save. This is the last
     * scenario in the tenant; no callback may be sent after its final EXIT. */
    {
        tbx_event_t k={keyDown,'9',0,0,0,0};tbx_app_event(&k);
        k.message='\r';tbx_app_event(&k);
        alerts=0;unsigned c0=copies;
        { tbx_event_t q={TBX_EVT_MENU,TBX_MENU_WORD(menu_id,2),0,0,0,0};tbx_app_event(&q); }
        CHECK(alerts == 1 && alert_buttons == 3 && exits == 0, "QUITSAVE Quit on an edited sheet asks first");
        { tbx_event_t a={TBX_EVT_ALERT,TBX_ALERT_DEFAULT,0,0,0,0};tbx_app_event(&a); }
        CHECK(alerts == 2 && exits == 0, "QUITSAVE Save over an existing TABLES.TXT asks the replace question");
        { tbx_event_t a={TBX_EVT_ALERT,TBX_ALERT_DEFAULT,0,0,0,0};tbx_app_event(&a); }
        CHECK(copies == c0 + 1 && exits == 0, "QUITSAVE Replace copies, and the session is still open until the copy lands");
        CHECK(strstr(stage_data, "\r\n9 ") != NULL, "QUITSAVE the saved file holds the second edit (9)");
        { tbx_event_t done={TBX_EVT_FILECOPY,50,0,0,0,0};tbx_app_event(&done); }
        CHECK(exits == 1, "QUITSAVE a successful Save ends the session");
    }
    printf("test_calc_file_copy: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
