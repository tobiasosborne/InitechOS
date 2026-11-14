/* Same calls on the host and a disk tenant. Expected results are captured from
 * the booted product, never calculated by the recording fake. */
static int32_t results[256];
static unsigned nr;
static char buf[256];
static char longtext[160] TBX_IN_IMAGE = "x";
static tbx_menubar_t bar TBX_IN_IMAGE;
static tbx_menuinfo_t menu TBX_IN_IMAGE;
static tbx_menuitem_t item TBX_IN_IMAGE;
static cp_item_t dialog_item TBX_IN_IMAGE = {1,2,45,23,1,0,"OK"};
static tbx_event_t dialog_event TBX_IN_IMAGE;
static cp_time_t timebuf;
static uint8_t settings[16];
#define TAKE(call) do { results[nr++]=(call); } while (0)
static void cases(void)
{
    char stack[128]="SDK.DAT";
    TAKE(tbx_new_window(12,40,250,200,1));
    TAKE(tbx_set_mbar(&bar)); TAKE(tbx_draw_menubar()); TAKE(tbx_beep());
    TAKE(tbx_file_open("SDK.DAT",0)); TAKE(tbx_track_mouse());
    TAKE(tbx_set_wtitle(1,"x")); TAKE(tbx_fill_rect(1,1,2,30,40,1));
    TAKE(tbx_register()); TAKE(tbx_register());
    TAKE(tbx_new_window(-1,40,250,200,1));
    TAKE(tbx_new_window(12,19,250,200,1));
    TAKE(tbx_new_window(12,40,641,200,1));
    TAKE(tbx_new_window(12,40,250,481,1));
    TAKE(tbx_new_window(12,40,107,200,1));
    TAKE(tbx_new_window(12,40,250,103,1));
    TAKE(tbx_new_window(12,40,250,200,3));
    TAKE(tbx_new_window(12,40,250,200,512));
    TAKE(tbx_new_window(12,40,250,200,2));
    TAKE(tbx_set_wtitle(1,0)); TAKE(tbx_set_wtitle(1,"Asymmetric title"));
    TAKE(tbx_set_wtitle(1,buf)); TAKE(tbx_set_wtitle(1,stack));
    TAKE(tbx_draw_text(1,7,13,buf,1,2)); TAKE(tbx_draw_cells(1,1,0,buf,1,2));
    for(unsigned i=0;i<sizeof longtext-1;i++)longtext[i]='W';
    TAKE(tbx_set_wtitle(1,longtext));
    TAKE(tbx_draw_text(1,7,13,"x",4,0));
    TAKE(tbx_draw_text(1,7,13,"x",0x800,0));
    TAKE(tbx_draw_text(1,7,13,"x",0x20000,0));
    TAKE(tbx_draw_text(1,7,13,"x",0x0c000000,0));
    TAKE(tbx_draw_text(1,7,13,"x",0x0c010100,2));
    TAKE(tbx_draw_text(1,-8,-19,"x",1,2));
    TAKE(tbx_draw_text(1,7,13,longtext,1,2));
    TAKE(tbx_draw_text(1,7,13,0,1,2));
    TAKE(tbx_fill_rect(1,1,2,1,30,1));
    TAKE(tbx_fill_rect(1,1,30,20,2,1));
    TAKE(tbx_fill_rect(1,1,2,20,30,4));
    TAKE(tbx_draw_cells(1,2,3,"ok",1,2));
    TAKE(tbx_draw_cells(1,-7,-9,"ok",1,2));
    TAKE(tbx_draw_cells(1,900,800,"ok",1,2));
    TAKE(tbx_draw_cells(1,1,0,longtext,1,2));
    TAKE(tbx_draw_cells(1,1,0,"x",0x10000,0));
    TAKE(tbx_set_mbar(0));
    bar.n_menus=9; TAKE(tbx_set_mbar(&bar));
    bar.n_menus=1;bar.menus=&menu;menu.menu_id=0;menu.title="File";
    TAKE(tbx_set_mbar(&bar)); menu.menu_id=17;menu.title="";
    TAKE(tbx_set_mbar(&bar));menu.title="File";menu.n_items=17;
    TAKE(tbx_set_mbar(&bar));menu.n_items=1;menu.items=&item;item.text=0;
    TAKE(tbx_set_mbar(&bar));item.text="Open";TAKE(tbx_set_mbar(&bar));
    TAKE(tbx_track_mouse());
    TAKE(tbx_set_scroll(1,-1,0)); TAKE(tbx_set_scroll(1,32768,0));
    TAKE(tbx_set_scroll(1,1000,-1)); TAKE(tbx_set_scroll(1,1000,32768));
    TAKE(tbx_global_to_local(1,32768,5)); TAKE(tbx_global_to_local(1,5,-32769));
    TAKE(tbx_dialog(1,&dialog_item,0,0,0,0));
    TAKE(tbx_dialog(1,&dialog_item,1,2,0,0));
    dialog_item.type=4;TAKE(tbx_dialog(1,&dialog_item,1,1,0,0));
    dialog_item.type=1;dialog_item.left=-1;TAKE(tbx_dialog(1,&dialog_item,1,1,0,0));
    dialog_item.left=1;dialog_event.what=0;
    TAKE(tbx_dialog(1,&dialog_item,1,1,0,&dialog_event));
    TAKE(tbx_settings(2,settings)); TAKE(tbx_clock(2,&timebuf)); TAKE(tbx_memory(0));
    TAKE(tbx_settings(0,(uint8_t *)stack)); TAKE(tbx_clock(0,(cp_time_t *)stack));
    TAKE(tbx_memory((uint32_t *)stack)); TAKE(tbx_sf_getfile(stack));
    TAKE(tbx_scrap_put(stack,1)); TAKE(tbx_scrap_put(stack,0));
    TAKE(tbx_scrap_put(0,1)); TAKE(tbx_scrap_put(buf,8193));
    TAKE(tbx_scrap_put("abc",3)); TAKE(tbx_scrap_get(buf,2));
    TAKE(tbx_file_create(0)); TAKE(tbx_file_create("")); TAKE(tbx_file_create("CON"));
    TAKE(tbx_file_create(stack));
    TAKE(tbx_file_open("SDK.DAT",3));
    TAKE(tbx_file_open("Z:\\SDK.DAT",0));
    int h=tbx_file_open("SDK.DAT",0); TAKE(h);
    TAKE(tbx_file_read(h,stack,1)); TAKE(tbx_file_read(h,0,0));
    TAKE(tbx_file_write(h,"x",1)); TAKE(tbx_file_read(h,buf,0x80000000u));
    TAKE(tbx_file_seek(h,-1,0)); TAKE(tbx_file_seek(h,0,3));
    TAKE(tbx_file_close(h));
    h=tbx_file_open("SDK.DAT",1);TAKE(h); TAKE(tbx_file_read(h,buf,1));TAKE(tbx_file_close(h));
    TAKE(tbx_file_find("*.*",256,buf,1));TAKE(tbx_file_find("*.*",0,buf,513));
    TAKE(tbx_file_rename("SDK.DAT","SUB\\SDK.DAT"));
    for(unsigned i=0;i<16;i++)TAKE(tbx_file_open("SDK.DAT",0));
    for(int i=5;i<20;i++)(void)tbx_file_close(i);
    TAKE(tbx_alert("","OK",0,0));TAKE(tbx_alert("x",0,0,0));
    TAKE(tbx_alert("x","OK",0,"No"));TAKE(tbx_alert("x","WWWWWW",0,0));
    TAKE(tbx_alert("x\r","OK",0,0));TAKE(tbx_alert("a\rb\rc\rd","OK",0,0));
    TAKE(tbx_file_copy("x\ry","SDK.DAT","COPY.DAT"));
    TAKE(tbx_file_copy("Copy","","COPY.DAT"));
    TAKE(tbx_sf_putfile("","X",buf));TAKE(tbx_sf_putfile("Save","TOOLONGNAME.TXT",buf));
    TAKE(tbx_sf_getfile(0));
    TAKE(tbx_alert("Question","OK",0,0)); TAKE(tbx_alert("Question","OK",0,0));
    TAKE(tbx_sf_getfile(buf)); TAKE(tbx_file_copy("Copy","SDK.DAT","COPY.DAT"));
    TAKE(tbx_new_window(20,50,300,250,1));
    TAKE(tbx_new_window(-1,0,1,1,9));
}
/* These results cannot be written through a closed tenant's file gate.
 * A wrong return therefore crashes the test tenant, making smoke red. */
static int postexit(void)
{
    if(tbx_exit(0)!=0)return 0;
    return tbx_register()==TBX_ERR_NOTREG&&
        tbx_new_window(12,40,250,200,1)==TBX_ERR_NOTREG&&
        tbx_set_mbar(&bar)==TBX_ERR_NOTREG&&tbx_beep()==TBX_ERR_NOTREG&&
        tbx_track_mouse()==TBX_ERR_NOTREG&&
        tbx_file_open("SDK.DAT",0)==TBX_ERR_NOTREG&&
        tbx_set_wtitle(1,"x")==TBX_ERR_BADARG&&
        tbx_scrap_get(buf,sizeof buf)==TBX_ERR_NOTREG;
}
