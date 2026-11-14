/* Independent expectations from the legacy per-app fakes + DOS verb contract. */
#include "tbx_host.h"
#include "tbx_save.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x) do { checks++;if(!(x)){fprintf(stderr,"host:%d: %s\n",__LINE__,#x);exit(1);} } while(0)
static unsigned callbacks;
static void event(const tbx_event_t *e) { CHECK(e->message==(int32_t)++callbacks); }
int main(void)
{
    tbx_host_reset();CHECK(tbx_register()==0);CHECK(tbx_new_window(0,40,640,480,2)==1);
    CHECK(tbx_set_wtitle(1,"Document")==0);CHECK(!strcmp(tbx_host.windows[0].title,"Document"));
    CHECK(tbx_content_origin(1)==(62<<16|6));CHECK(tbx_content_size(1)==(397<<16|613));
    CHECK(tbx_content_size(7)==TBX_ERR_BADARG);CHECK(tbx_global_to_local(1,14,78)==(16<<16|8));
    CHECK(tbx_draw_text(1,7,12,"text",1,0)==0);CHECK(!strcmp(tbx_host.ops[tbx_host.nops-1].text,"text"));
    CHECK(tbx_fill_rect(1,0,0,20,20,2)==0);
    CHECK(tbx_draw_cells(1,1,0,"AB",TBX_TEXT_BOLD|1,3)==0);
    CHECK(!strncmp(tbx_host.scr[0],"AB",2)&&tbx_host.bg[0][0]=='N'&&tbx_host.face[0][0]==1);
    CHECK(tbx_draw_cells(1,1,0,"C",3,0)==0);CHECK(tbx_host.inv[0][0]&&tbx_host.face[0][0]==1);
    CHECK(tbx_draw_cells(1,2,0,"x",1,0)==TBX_ERR_BADARG);
    CHECK(tbx_set_mbar(0)==0&&tbx_draw_menubar()==0&&tbx_host.menubar_draws==1);
    CHECK(tbx_alert("Save?","Yes","Cancel","No")==0);CHECK(tbx_host.alerts==1&&!strcmp(tbx_host.alert_btn[2],"No"));
    CHECK(tbx_scrap_get(0,0)==TBX_ERR_NOTYPE);CHECK(tbx_scrap_put("hello",5)==0);char b[128]={0};
    CHECK(tbx_scrap_get(0,0)==5);CHECK(tbx_scrap_get(b,5)==5&&!memcmp(b,"hello",5));CHECK(tbx_scrap_get(b,3)==TBX_ERR_BADARG);
    CHECK(tbx_sf_getfile(b)==0&&tbx_host.sf_reply==b&&tbx_host.sf==1);
    CHECK(tbx_sf_putfile("Save","TEST",b)==0&&!strcmp(tbx_host.sf_name,"TEST"));
    CHECK(tbx_track_mouse()==0&&tbx_host.track==1);
    CHECK(tbx_set_scroll(1,1000,30)==30&&tbx_get_scroll(1)==30&&tbx_host.setscroll==1);
    CHECK(tbx_file_copy("Copy","A","B")==0&&!strcmp(tbx_host.copy_dst,"B"));
    cp_time_t clock;CHECK(tbx_get_clock(&clock)==0&&clock.year==1992);
    clock.hour=13;CHECK(tbx_clock(1,&clock)==0&&tbx_host.clock.hour==13);
    tbx_host.clock_rc=-42;CHECK(tbx_get_clock(&clock)==-42);tbx_host.clock_rc=0;
    uint8_t rec[16];CHECK(tbx_settings(0,rec)==0&&rec[14]==0xfa);rec[0]=0;CHECK(tbx_settings(1,rec)==TBX_ERR_BADARG);
    uint32_t mem[4];CHECK(tbx_memory(mem)==0&&mem[0]==8192);
    CHECK(tbx_beep()==0&&tbx_trap(TBX_BEEP,0,0)==0&&tbx_host.beeps==2);
    CHECK(tbx_trap(0xffff,0,0)==TBX_ERR_BADCODE);
    tbx_host.dialog_result=3;CHECK(tbx_dialog(1,0,0,1,2,0)==3&&tbx_host.dialog_cancel==2);
    int32_t h=tbx_file_create("c:\\test.dat");CHECK(h==5);CHECK(tbx_file_write(h,"hello",5)==5);
    CHECK(tbx_file_close(h)==0);CHECK(tbx_file_create_new("TEST.DAT")==TBX_ERR_DOS(80));
    h=tbx_file_open("test.dat",0);CHECK(h==5);CHECK(tbx_file_write(h,"x",1)==TBX_ERR_DOS(5));
    CHECK(tbx_file_read(h,b,8)==5&&!memcmp(b,"hello",5));CHECK(tbx_file_read(h,b,1)==0);
    CHECK(tbx_file_seek(h,-2,TBX_FILE_FROM_END)==3);CHECK(tbx_file_read(h,b,2)==2&&!memcmp(b,"lo",2));
    CHECK(tbx_file_close(h)==0&&tbx_file_close(h)==TBX_ERR_BADARG);
    CHECK(tbx_file_rename("test.dat","NEW.DAT")==0);CHECK(!tbx_host_file("test.dat")&&tbx_host_file("new.dat"));
    tbx_host.fail_create=1;CHECK(tbx_file_create("FAIL")==TBX_ERR_DOS(5));tbx_host.fail_create=0;
    CHECK(tbx_host_put_file("NEW.$$$","keep",4));
    tbx_host.fail_write=1;CHECK(tbx_file_save("NEW.DAT","lost",4)==0);CHECK(tbx_host_file("new.dat")->size==5);CHECK(tbx_host_file("NEW.$$$") && tbx_host_file("NEW.$$$")->size==4);tbx_host.fail_write=0;
    tbx_host.write_limit=2;CHECK(tbx_file_save("NEW.DAT","preserved",9)==9);CHECK(!memcmp(tbx_host_file("new.dat")->data,"preserved",9));
    tbx_host.file_error[TBX_FILE_RENAME]=5;CHECK(tbx_file_save("NEW.DAT","lost",4)==TBX_ERR_DOS(5));CHECK(tbx_host_file("NEW.DAT")->size==9);tbx_host.file_error[TBX_FILE_RENAME]=0;
    /* A committed save remains successful when only backup cleanup fails. */
    tbx_host.file_error[TBX_FILE_DELETE]=5;CHECK(tbx_file_save("NEW.DAT","new",3)==3);CHECK(!memcmp(tbx_host_file("NEW.DAT")->data,"new",3));CHECK(tbx_host_file("NEW.~00") && !memcmp(tbx_host_file("NEW.~00")->data,"preserved",9));tbx_host.file_error[TBX_FILE_DELETE]=0;
    CHECK(tbx_host_put_file("DIR\\FILE.WP5","wp",2));
    tbx_find_rec_t found[8];CHECK(tbx_file_find("DIR\\*.*",0x16,found,8)==1);CHECK(!strcmp(found[0].name,"FILE.WP5")&&found[0].size==2);
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
    printf("shared fake: %u checks, 0 failures\n",checks);return 0;
}
