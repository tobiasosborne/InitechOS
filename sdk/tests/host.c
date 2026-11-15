/* Independent expectations from the legacy per-app fakes + DOS verb contract. */
#include "tbx_host.h"
#include "tbx_save.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x) do { checks++;if(!(x)){fprintf(stderr,"  FAIL host:%d: %s\n",__LINE__,#x);printf("INITECH_CHECKS %u failures=1\n",checks);exit(1);} } while(0)
static unsigned callbacks;
static void event(const tbx_event_t *e) { CHECK(e->message==(int32_t)++callbacks); }
/* Model the shell's delivery, not a direct tenant callback: replies complete
 * modal services and mouseDown establishes TRACKMOUSE's dynamic context. */
static int32_t control_what,control_message;
static void control_event(const tbx_event_t *e)
{
    CHECK(e->what==control_what && e->message==control_message);
    if(e->what==mouseDown) CHECK(tbx_track_mouse()==TBX_OK);
}
static void deliver(int32_t what,int32_t message)
{
    tbx_event_t e={.what=what,.message=message};
    control_what=what;control_message=message;
    CHECK(tbx_host_push_event(&e));CHECK(tbx_host_dispatch(control_event)==1);
}
static const tbx_menuitem_t menu_items[]={{.text="Open",.enabled=1}};
static const tbx_menuinfo_t menus[]={{.menu_id=1,.title="File",.items=menu_items,.n_items=1}};
static const tbx_menubar_t bar={.menus=menus,.n_menus=1};
static const cp_item_t dialog_items[]={
    {0,0,60,20,1,0,"One"},{70,0,130,20,1,0,"Cancel"},{140,0,200,20,1,0,"Other"}
};
int main(void)
{
    /* QUIT_REPLY (toolbox_gate.h, initech-qv7t): NOTREG before registration,
     * then BADARG for a state other than CANCEL/PENDING; 0 records it. */
    tbx_host_reset();CHECK(tbx_quit_reply(TBX_QUIT_PENDING)==TBX_ERR_NOTREG&&tbx_host.quit_replies==0);
    CHECK(tbx_register()==0);
    CHECK(tbx_quit_reply(2)==TBX_ERR_BADARG&&tbx_host.quit_replies==0);
    CHECK(tbx_quit_reply(TBX_QUIT_PENDING)==0&&tbx_host.quit_replies==1&&tbx_host.quit_reply_state==1);
    {uint32_t q[1]={TBX_QUIT_CANCEL};CHECK(tbx_trap(TBX_QUIT_REPLY,1,q)==0&&tbx_host.quit_replies==2&&tbx_host.quit_reply_state==0);}
    CHECK(tbx_exit(0)==0&&tbx_quit_reply(TBX_QUIT_CANCEL)==TBX_ERR_NOTREG&&tbx_host.quit_replies==2);
    tbx_host_reset();CHECK(tbx_register()==0);CHECK(tbx_new_window(0,40,640,480,2)==1);
    CHECK(tbx_set_wtitle(1,"Document")==0);CHECK(!strcmp(tbx_host.windows[0].title,"Document"));
    CHECK(tbx_content_origin(1)==(62<<16|6));CHECK(tbx_content_size(1)==(397<<16|613));
    CHECK(tbx_content_size(7)==TBX_ERR_BADARG);CHECK(tbx_global_to_local(1,14,78)==(16<<16|8));
    CHECK(tbx_draw_text(1,7,12,"text",1,0)==0);CHECK(!strcmp(tbx_host.ops[tbx_host.nops-1].text,"text"));
    CHECK(tbx_fill_rect(1,0,0,20,20,2)==0);
    CHECK(tbx_draw_cells(1,1,0,"AB",TBX_TEXT_BOLD|1,3)==0);
    CHECK(!strncmp(tbx_host.scr[0],"AB",2)&&tbx_host.bg[0][0]=='N'&&tbx_host.face[0][0]==1);
    CHECK(tbx_draw_cells(1,1,0,"C",3,0)==0);CHECK(tbx_host.inv[0][0]&&tbx_host.face[0][0]==1);
    /* Off-grid coordinates are valid on the real gate; use an invalid colour
     * for the common negative case, retaining the grid's AB/C assertions. */
    CHECK(tbx_draw_cells(1,1,0,"x",TBX_COLOR_COUNT,0)==TBX_ERR_BADARG);
    CHECK(tbx_set_mbar(&bar)==0&&tbx_draw_menubar()==0&&tbx_host.menubar_draws==1);
    CHECK(tbx_alert("Save?","Yes","Cancel","No")==0);CHECK(tbx_host.alerts==1&&!strcmp(tbx_host.alert_btn[2],"No"));
    deliver(TBX_EVT_ALERT,1);
    CHECK(tbx_scrap_get(0,0)==TBX_ERR_NOTYPE);CHECK(tbx_scrap_put("hello",5)==0);static char b[128];
    CHECK(tbx_scrap_get(0,0)==5);CHECK(tbx_scrap_get(b,5)==5&&!memcmp(b,"hello",5));CHECK(tbx_scrap_get(b,3)==TBX_ERR_BADARG);
    CHECK(tbx_sf_getfile(b)==0&&tbx_host.sf_reply==b&&tbx_host.sf==1);
    deliver(TBX_EVT_SFREPLY,0);
    CHECK(tbx_sf_putfile("Save","TEST",b)==0&&!strcmp(tbx_host.sf_name,"TEST"));
    deliver(TBX_EVT_SFREPLY,0);
    deliver(mouseDown,0);CHECK(tbx_host.track==1);
    CHECK(tbx_set_scroll(1,1000,30)==30&&tbx_get_scroll(1)==30&&tbx_host.setscroll==1);
    CHECK(tbx_file_copy("Copy","A","B")==0&&!strcmp(tbx_host.copy_dst,"B"));
    deliver(TBX_EVT_FILECOPY,TBX_ERR_DOS(2));
    /* The live FILE COPY caption is 314 px (FileCopyDialog statText, flair
     * dialog.h FLAIR_LIVECOPY_TEXT_W). Chicago 12 '0' advances 8 px (NFNT
     * 5478 owTable): 39 zeros are 312 px, 40 are 320 px. */
    CHECK(tbx_file_copy("0000000000000000000000000000000000000000","A","B")==TBX_ERR_BADARG);
    CHECK(tbx_file_copy("000000000000000000000000000000000000000","A","B")==0);
    deliver(TBX_EVT_FILECOPY,0);
    static cp_time_t clock;CHECK(tbx_get_clock(&clock)==0&&clock.year==1992);
    clock.hour=13;CHECK(tbx_clock(1,&clock)==0&&tbx_host.clock.hour==13);
    tbx_host.clock_rc=-42;CHECK(tbx_get_clock(&clock)==-42);tbx_host.clock_rc=0;
    static uint8_t rec[16];CHECK(tbx_settings(0,rec)==0&&rec[14]==0xfa);rec[0]=0;CHECK(tbx_settings(1,rec)==TBX_ERR_BADARG);
    static uint32_t mem[4];CHECK(tbx_memory(mem)==0&&mem[0]==8192);
    CHECK(tbx_beep()==0&&tbx_trap(TBX_BEEP,0,0)==0&&tbx_host.beeps==2);
    CHECK(tbx_trap(0xffff,0,0)==TBX_ERR_BADCODE);
    tbx_host.dialog_result=3;CHECK(tbx_dialog(1,dialog_items,3,1,2,0)==3&&tbx_host.dialog_cancel==2);
    /* noGrowDocProc (frame owner W1): content is frame + (6,22) .. frame - 6,
     * flair/window.c CalcPlainDocContentRect, so a 200 x 150 frame holds
     * items up to 188 x 122; the zoomDocProc frame holds only 173 x 107. */
    CHECK(tbx_new_window(100,100,300,250,TBX_WDEF_NOGROW|TBX_WDEF_RDOC)==TBX_ERR_BADARG);
    CHECK(tbx_new_window(100,100,300,250,1|TBX_WDEF_NOGROW)==2);
    {
        static const cp_item_t edge[]={{0,0,188,122,1,0,"Edge"}};
        static const cp_item_t wide[]={{0,0,189,122,1,0,"Wide"}};
        static const cp_item_t tall[]={{0,0,188,123,1,0,"Tall"}};
        CHECK(tbx_dialog(2,edge,1,1,1,0)==3);
        CHECK(tbx_dialog(2,wide,1,1,1,0)==TBX_ERR_BADARG);
        CHECK(tbx_dialog(2,tall,1,1,1,0)==TBX_ERR_BADARG);
    }
    int32_t h=tbx_file_create("a:\\test.dat");CHECK(h==5);CHECK(tbx_file_write(h,"hello",5)==5);
    CHECK(tbx_file_close(h)==0);CHECK(tbx_file_create_new("TEST.DAT")==TBX_ERR_DOS(80));
    h=tbx_file_open("test.dat",0);CHECK(h==5);
    int32_t denied=tbx_file_write(h,"x",1);
    /* The legacy recording fake returns DOS access denied; the real gate and
     * stricter fake return BADARG. Neither may write or advance the handle. */
    CHECK(denied==TBX_ERR_BADARG || denied==TBX_ERR_DOS(5));
    CHECK(tbx_host_file("test.dat")->size==5 && !memcmp(tbx_host_file("test.dat")->data,"hello",5));
    CHECK(tbx_file_read(h,b,8)==5&&!memcmp(b,"hello",5));CHECK(tbx_file_read(h,b,1)==0);
    CHECK(tbx_file_seek(h,-2,TBX_FILE_FROM_END)==3);CHECK(tbx_file_read(h,b,2)==2&&!memcmp(b,"lo",2));
    CHECK(tbx_file_close(h)==0&&tbx_file_close(h)==TBX_ERR_BADARG);
    CHECK(tbx_file_rename("test.dat","NEW.DAT")==0);CHECK(!tbx_host_file("test.dat")&&tbx_host_file("new.dat"));
    tbx_host.fail_create=1;CHECK(tbx_file_create("FAIL")==TBX_ERR_DOS(5));tbx_host.fail_create=0;
    CHECK(tbx_host_put_file("NEW.$$$","keep",4));
    tbx_host.fail_write=1;CHECK(tbx_file_save("NEW.DAT","lost",4)==0);CHECK(tbx_host_file("new.dat")->size==5);CHECK(tbx_host_file("NEW.$$$") && tbx_host_file("NEW.$$$")->size==4);tbx_host.fail_write=0;
    tbx_host.write_limit=2;CHECK(tbx_file_save("NEW.DAT","preserved",9)==9);CHECK(!memcmp(tbx_host_file("new.dat")->data,"preserved",9));
    tbx_host.file_error[TBX_FILE_REPLACE]=5;CHECK(tbx_file_save("NEW.DAT","lost",4)==TBX_ERR_DOS(5));CHECK(tbx_host_file("NEW.DAT")->size==9);tbx_host.file_error[TBX_FILE_REPLACE]=0;
    /* A committed replacement never unlinks its adopted stage. */
    tbx_host.file_error[TBX_FILE_DELETE]=5;CHECK(tbx_file_save("NEW.DAT","new",3)==3);CHECK(!memcmp(tbx_host_file("NEW.DAT")->data,"new",3));CHECK(!tbx_host_file("NEW.~00"));tbx_host.file_error[TBX_FILE_DELETE]=0;
    CHECK(tbx_host_put_file("DIR\\FILE.WP5","wp",2));
    static tbx_find_rec_t found[8];CHECK(tbx_file_find("DIR\\*.*",0x16,found,8)==1);CHECK(!strcmp(found[0].name,"FILE.WP5")&&found[0].size==2);
    tbx_host.free_bytes=12345;CHECK(tbx_disk_free()==12345);
    h=tbx_file_open("NEW.DAT",0);tbx_host.read_limit=2;CHECK(tbx_file_read(h,b,9)==2);
    tbx_host.file_error[TBX_FILE_CLOSE]=5;CHECK(tbx_file_close(h)==TBX_ERR_DOS(5));CHECK(tbx_file_read(h,b,1)==TBX_ERR_BADARG);
    CHECK(tbx_file_delete("absent")==TBX_ERR_DOS(2));
    tbx_host.file_error[TBX_FILE_CLOSE]=0;tbx_host.write_limit=0;
    h=tbx_file_create("budget");tbx_host.write_budget=3;
    CHECK(tbx_file_write(h,"abcdef",6)==3&&tbx_file_write(h,"x",1)==0);
    CHECK(tbx_file_seek(h,0,0)==0&&tbx_file_write(h,"x",1)==0);
    CHECK(tbx_host_file("budget")->size==3);
    tbx_host.fail_close=1;tbx_host.fail_close_handle=h;CHECK(tbx_file_close(h)==TBX_ERR_DOS(5));
    CHECK(!tbx_host.fail_close);
    for(int i=1;i<=3;i++){tbx_event_t e={.what=keyDown,.message=i};CHECK(tbx_host_push_event(&e));}
    CHECK(tbx_host_dispatch(event)==3&&callbacks==3&&tbx_host.event_count==0);
    CHECK(tbx_exit(7)==0&&tbx_host.exit_calls==1&&tbx_host.exit_rc==7);
    tbx_host_reset();CHECK(tbx_host_file("new.dat")==0&&tbx_host.exit_calls==0&&tbx_host.nops==0);
    /* initech-ui9dg: a stage whose cleanup DELETE was refused is named to the
     * user and is theirs from then on. The user replaces it with an unrelated
     * file between saves; the next save must not delete it, and still saves
     * through another exclusively created stage. Expected bytes are literals. */
    {
        static const char foreign[]="Unrelated user file\000\377\r\n";
        CHECK(tbx_register()==0&&tbx_host_put_file("DOC.WP5","old document",12));
        tbx_host.file_error[TBX_FILE_REPLACE]=5;tbx_host.file_error[TBX_FILE_DELETE]=5;
        CHECK(tbx_file_save("DOC.WP5","new document",12)==TBX_ERR_DOS(5));
        CHECK(tbx_host_file("DOC.$$$")&&tbx_host.alerts==1&&strstr(tbx_host.alert_text,"DOC.$$$"));
        tbx_host.file_error[TBX_FILE_REPLACE]=0;tbx_host.file_error[TBX_FILE_DELETE]=0;
        deliver(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
        CHECK(tbx_file_delete("DOC.$$$")==0&&tbx_host_put_file("DOC.$$$",foreign,sizeof foreign-1));
        CHECK(tbx_file_save("DOC.WP5","new document",12)==12);
        CHECK(tbx_host_file("DOC.WP5")->size==12&&!memcmp(tbx_host_file("DOC.WP5")->data,"new document",12));
        const tbx_host_file_t *f=tbx_host_file("DOC.$$$");
        CHECK(f&&f->size==sizeof foreign-1&&!memcmp(f->data,foreign,sizeof foreign-1));
        tbx_host_reset();
    }
    /* Eleven saves whose REPLACE and cleanup DELETE are refused leave eleven
     * named stages (.$$$, .$$0-.$$9) that are the user's now. A twelfth save,
     * faults cleared, must still succeed through a stage of its own and leave
     * all eleven untouched (review xreview-frame-factory; Calc's 40kpb). */
    {
        static const char stages[11][8]={"$$$","$$0","$$1","$$2","$$3","$$4","$$5","$$6","$$7","$$8","$$9"};
        CHECK(tbx_register()==0&&tbx_host_put_file("DOC.WP5","old document",12));
        tbx_host.file_error[TBX_FILE_REPLACE]=5;tbx_host.file_error[TBX_FILE_DELETE]=5;
        for(unsigned i=0;i<11;i++) {
            CHECK(tbx_file_save("DOC.WP5","new document",12)==TBX_ERR_DOS(5));
            deliver(TBX_EVT_ALERT,TBX_ALERT_DEFAULT);
        }
        tbx_host.file_error[TBX_FILE_REPLACE]=0;tbx_host.file_error[TBX_FILE_DELETE]=0;
        for(unsigned i=0;i<11;i++) {
            char name[16]="DOC.";memcpy(name+4,stages[i],4);
            const tbx_host_file_t *f=tbx_host_file(name);
            CHECK(f&&f->size==12&&!memcmp(f->data,"new document",12));
        }
        CHECK(tbx_file_save("DOC.WP5","new document",12)==12);
        CHECK(!memcmp(tbx_host_file("DOC.WP5")->data,"new document",12));
        for(unsigned i=0;i<11;i++) {
            char name[16]="DOC.";memcpy(name+4,stages[i],4);
            CHECK(tbx_host_file(name)!=0);
        }
        tbx_host_reset();
    }
    /* Delivery's C: system disk (initech-tlhqy): bound C:, optional A: data.
     * Root-relative paths live on C:, explicit A: paths on A:, and the two
     * never alias; an unmounted drive is DOS 15, a cross-drive rename or
     * replace DOS 17 (INT 21h 56h, not same device). */
    {
        CHECK(!tbx_host_bind_system('C'));                 /* not mounted yet */
        CHECK(tbx_host_mount('C',1)&&tbx_host_bind_system('C')&&!tbx_host_mount('C',0));
        CHECK(tbx_register()==0);
        int32_t h=tbx_file_create("APPS\\TABLES.TXT");CHECK(h>=5&&tbx_file_write(h,"on C",4)==4&&tbx_file_close(h)==0);
        CHECK(tbx_host_file("C:\\APPS\\TABLES.TXT")&&!tbx_host_file("A:\\APPS\\TABLES.TXT"));
        h=tbx_file_create("A:\\LEDGER.TXT");CHECK(h>=5&&tbx_file_write(h,"on A",4)==4&&tbx_file_close(h)==0);
        CHECK(tbx_host_file("A:LEDGER.TXT")&&!tbx_host_file("LEDGER.TXT")&&!tbx_host_file("C:LEDGER.TXT"));
        CHECK(tbx_host_put_file("C:\\LEDGER.TXT","on C too",8));
        CHECK(tbx_host_file("LEDGER.TXT")->size==8&&tbx_host_file("A:LEDGER.TXT")->size==4);
        CHECK(tbx_file_open("D:\\X.TXT",0)==TBX_ERR_DOS(15)&&tbx_file_create("D:X")==TBX_ERR_DOS(15));
        CHECK(tbx_file_rename("A:\\LEDGER.TXT","C:\\L2.TXT")==TBX_ERR_DOS(17));
        CHECK(tbx_file_replace("A:\\LEDGER.TXT","C:\\LEDGER.TXT")==TBX_ERR_DOS(17));
        static tbx_find_rec_t found[8];
        CHECK(tbx_file_find("*.*",0x16,found,8)==1&&found[0].size==8);  /* C: root: its own LEDGER.TXT */
        CHECK(tbx_file_find("A:*.*",0x16,found,8)==1&&!strcmp(found[0].name,"LEDGER.TXT")&&found[0].size==4);
        CHECK(tbx_file_save("APPS\\TABLES.TXT","saved on C",10)==10);
        CHECK(tbx_host_file("C:APPS\\TABLES.TXT")->size==10&&!tbx_host_file("A:APPS\\TABLES.$$$"));
        CHECK(tbx_host_mount('A',0)&&tbx_file_open("A:LEDGER.TXT",0)==TBX_ERR_DOS(15)&&tbx_host_mount('A',1));
        tbx_host_reset();
        CHECK(tbx_register()==0&&tbx_file_create("C:\\X")==TBX_ERR_DOS(15));  /* legacy default: A: only */
        tbx_host_reset();
    }
    printf("shared fake: %u checks, 0 failures\n",checks);return 0;
}
