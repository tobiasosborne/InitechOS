/* Literal, asymmetric calls. The independent table abi.tsv is checked AFTER
 * the actual dispatcher has copied interrupted-stack arguments. */
#include "tbx.h"
const char tbx_app_name[]="SDKCHECK";
volatile unsigned sdk_phase TBX_IN_IMAGE;
volatile int sdk_exit_return TBX_IN_IMAGE;
static unsigned failed;
static const tbx_menuitem_t items[]={{"Check",0,'K',0,1,0,{0,0,0}}};
static const tbx_menuinfo_t menus[]={{0x1234,0,"Test",items,1,0}};
static const tbx_menubar_t bar={menus,1,0,0};
#define P(type,n) ((type *)(uintptr_t)(n))
#define EXPECT(call,want) do { if((call)!=(want))failed++; } while(0)
#define BAD(call) EXPECT(call,TBX_ERR_BADARG)
int tbx_app_main(void)
{
    tbx_register();tbx_set_mbar(&bar);tbx_new_window(21,43,311,239,1);
    return 0;
}
void tbx_app_event(const tbx_event_t *e)
{
    if(e->what!=TBX_EVT_MENU)return;
    if(e->message!=0x12340001){tbx_exit(99);return;}
    static tbx_event_t saved;
    saved=*e;
    int h=tbx_file_create("SDKEVT.BIN");
    if(h<5||tbx_file_write(h,&saved,sizeof saved)!=sizeof saved||tbx_file_close(h)!=0){tbx_exit(98);return;}
    sdk_phase=1;
    EXPECT(tbx_register(),TBX_ERR_BUSY);
    BAD(tbx_new_window(11,37,213,169,0x513));
    BAD(tbx_set_wtitle(0x6101,P(char,0x6202)));
    BAD(tbx_content_origin(0x6103));BAD(tbx_content_size(0x6104));
    BAD(tbx_draw_text(0x6105,0x6206,0x6307,P(char,0x6408),0x6509,0x660a));
    BAD(tbx_fill_rect(0x6111,0x6212,0x6313,0x6414,0x6515,0x6616));
    BAD(tbx_draw_cells(0x6121,0x6222,0x6323,P(char,0x6424),0x6525,0x6626));
    BAD(tbx_set_mbar(P(tbx_menubar_t,0x6131)));EXPECT(tbx_draw_menubar(),0);
    BAD(tbx_file_create(P(char,0x6141)));
    BAD(tbx_file_open(P(char,0x6142),0x6243));
    BAD(tbx_file_read(0x6151,P(void,0x6252),0x6353));
    BAD(tbx_file_write(0x6161,P(void,0x6262),0x6363));
    BAD(tbx_file_seek(0x6171,-0x6272,0x6373));BAD(tbx_file_close(0x6181));
    BAD(tbx_file_delete(P(char,0x6191)));
    BAD(tbx_file_find(P(char,0x61a1),0x62a2,P(void,0x63a3),0x64a4));if(tbx_disk_free()<0)failed++;
    BAD(tbx_file_rename(P(char,0x61b1),P(char,0x62b2)));
    BAD(tbx_file_create_new(P(char,0x61c1)));
    BAD(tbx_alert(P(char,0x61d1),P(char,0x62d2),P(char,0x63d3),P(char,0x64d4)));
    BAD(tbx_file_copy(P(char,0x61e1),P(char,0x62e2),P(char,0x63e3)));
    BAD(tbx_dialog(0x61f1,P(cp_item_t,0x62f2),0x63f3,0x64f4,0x65f5,P(tbx_event_t,0x66f6)));
    BAD(tbx_settings(0x7101,P(uint8_t,0x7202)));BAD(tbx_clock(0x7111,P(cp_time_t,0x7212)));
    BAD(tbx_memory(P(uint32_t,0x7121)));EXPECT(tbx_beep(),0);
    BAD(tbx_scrap_put(P(void,0x7131),0x7232));EXPECT(tbx_scrap_get(P(void,0x7141),0x7242),TBX_ERR_NOTYPE);
    BAD(tbx_sf_getfile(P(char,0x7151)));
    BAD(tbx_sf_putfile(P(char,0x7161),P(char,0x7262),P(char,0x7363)));
    BAD(tbx_track_mouse());BAD(tbx_set_scroll(0x7171,0x7272,0x7373));BAD(tbx_get_scroll(0x7181));
    BAD(tbx_global_to_local(0x7191,-0x7292,0x7393));
    sdk_exit_return=tbx_exit(failed?97:0x13579);
}
