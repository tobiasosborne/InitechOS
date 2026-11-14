/* initech-0xs6: real Calculator tenant over a recording Toolbox mock.
 * Mac guide printed p.125: Close ends an accessory; IM VI PDF p.345:
 * shutdown asks the application's own quit routine. Our existing gate asks
 * an enabled literal Quit item. Close W must remain available as well.
 * No clipboard syntax is invented here (B13 still needs a real-Mac mint). */
#include <stdio.h>
#include <string.h>
#include "tbx.h"
static int fail_scrap;
static int32_t put_scrap(const void *p, uint32_t n)
{ return fail_scrap ? TBX_ERR_BADARG : tbx_scrap_put(p,n); }
#define tbx_scrap_put put_scrap
#include "../calcda.c"
#undef tbx_scrap_put
#include "tbx_host.h"

static int fails, checks;
#define installed tbx_host.bar
#define exits tbx_host.exit_calls
#define CHECK(c, why) do { checks++; if (!(c)) { puts("FAIL " why); fails++; } } while (0)

static void menu(int32_t word)
{
    tbx_event_t e={0};e.what=TBX_EVT_MENU;e.message=word;tbx_app_event(&e);
}
static void key(int c)
{ tbx_event_t e={0};e.what=keyDown;e.message=c;tbx_app_event(&e); }
static void type(const char *s) { for(;*s;s++)key(*s); }
static void sweep(void)
{
    (void)put_scrap; /* baseline has no Copy call yet */
    /* Reviewed shipping manifest, independent of the product arrays. Native
     * shortcuts belong to the shell: validate their mapping, then dispatch
     * the same menu word. No contextual/popup menu exists in this tenant. */
    const char *names[]={"Close","Quit","Copy","Clear"};
    const char keys[]={'W','Q',0,0};
    unsigned n=0;
    CHECK(installed->n_menus==2,"sweep exact menu count");
    for(unsigned m=0;m<installed->n_menus;m++)
        CHECK(m<2 && installed->menus[m].menu_id==129+(int)m &&
              !strcmp(installed->menus[m].title,m?"Edit":"File"),"sweep menu dispatch identity");
    for(unsigned m=0;m<installed->n_menus;m++)
        for(unsigned i=0;i<installed->menus[m].n_items;i++) {
            const tbx_menuitem_t *it=&installed->menus[m].items[i];
            CHECK(n<4 && !it->is_divider && !strcmp(it->text,names[n]) &&
                  it->cmd_char==keys[n] && it->enabled,"sweep exact live manifest");
            n++;
        }
    CHECK(n==4,"sweep no missing or extra commands");
    const char *seq[]={"2+3*4=","1.5","1/1000=","1/0="};
    const char *want[]={"20","1.5","1E-3","infinity"};
    for(unsigned i=0;i<4;i++) {
        key('c');type(seq[i]);
        calc_t before=g_calc;
        menu(TBX_MENU_WORD(130,1));
        CHECK(tbx_host.scrap_len==(int)strlen(want[i]) &&
              !memcmp(tbx_host.scrap,want[i],strlen(want[i])),"sweep Copy exact TEXT");
        CHECK(!memcmp(&before,&g_calc,sizeof before),"Copy preserves arithmetic");
    }
    fail_scrap=1;int alerts=tbx_host.alerts;menu(TBX_MENU_WORD(130,1));
    CHECK(tbx_host.alerts==alerts+1,"Copy transport failure is reported");fail_scrap=0;
    key('c');type("2+3");menu(TBX_MENU_WORD(130,2));type("4=");
    CHECK(!strcmp(calc_display(&g_calc),"4"),"menu Clear resets pending arithmetic");
    CHECK(tbx_host.scrap_len==8 && !memcmp(tbx_host.scrap,"infinity",8),"Clear preserves scrap");
    /* Every painted key: literal initial state / result, driven separately by
     * pointer and keyboard. Calc vectors p00/p01/r05/p03 supply the oracles. */
    const char labels[]="0123456789.+-*/=c";
    const char *results[]={"0","1","2","3","4","5","6","7","8","9","0.","7","3","10","2.5","2","0"};
    for(int route=0;route<2;route++) for(int i=0;i<17;i++) {
        key('c');
        if(i>=11 && i<=14) type("5");
        if(i==15) type("1+1");
        if(i==16) type("2+3");
        if(!route) key(labels[i]);
        else {
            int l,t,r,b;
            calc_view_key_rect(calc_view_key_of_cmd((calc_cmd_t)i),&l,&t,&r,&b);
            tbx_event_t e={0};e.what=mouseDown;
            e.whereh=tbx_host.origin_h+(l+r)/2;e.wherev=tbx_host.origin_v+(t+b)/2;
            tbx_app_event(&e);
        }
        if(i>=11 && i<=14) type("2=");
        CHECK(!strcmp(calc_display(&g_calc),results[i]),"sweep key/button effect");
    }
    /* The tall bottom '=' is a separate visible button from the top '='.
     * Its literal locked reading-order slot is 15; do not collapse aliases. */
    key('c');type("2+3");
    int l,t,r,b;calc_view_key_rect(15,&l,&t,&r,&b);
    tbx_event_t click={0};click.what=mouseDown;
    click.whereh=tbx_host.origin_h+(l+r)/2;click.wherev=tbx_host.origin_v+(t+b)/2;
    tbx_app_event(&click);
    CHECK(!strcmp(calc_display(&g_calc),"5"),"sweep separate tall equals button");
    key('c');click.what=autoKey;click.message='7';tbx_app_event(&click);
    CHECK(!strcmp(calc_display(&g_calc),"7"),"sweep autoKey entry route");
    key('c');type("12");
    for(const char *s="zxv";*s;s++) {
        tbx_event_t e={0};e.what=keyDown;e.message=*s;e.modifiers=FLAIR_EVT_MOD_CONTROL_KEY;
        tbx_app_event(&e);
    }
    CHECK(!strcmp(calc_display(&g_calc),"12"),"removed shortcuts are unmapped");
}
/* Independent NPAD reader for Copy -> Note Pad on the booted volume. This
 * uses only the authored byte contract, no Note Pad serializer or headers. */
static int copied_store(const char *path)
{
    unsigned char b[8225];FILE *f=fopen(path,"rb");
    if(!f) return 1;
    size_t n=fread(b,1,sizeof b,f);fclose(f);
    if(n!=8224 || memcmp(b,"NPAD\1\0\10\0",8) || b[10]!=8 || b[11]) return 1;
    uint32_t hash=2166136261u,want=0;
    for(unsigned i=0;i<8224;i++) hash=(hash^(i>=12 && i<16?0:b[i]))*16777619u;
    for(unsigned i=0;i<4;i++) want|=(uint32_t)b[12+i]<<(8*i);
    if(hash!=want || memcmp(b+1042,"\2\00020",4)) return 1;
    puts("Calculator Copy: independent NPAD checksum and page 2 TEXT = 20");return 0;
}
int main(int argc,char **argv)
{
    if(argc==2) return copied_store(argv[1]);
    if(argc!=1) return 2;
    tbx_host_reset();
    int32_t quit=0;
    CHECK(tbx_app_main()==0 && installed,"launch");
    CHECK(!strcmp(calc_display(&g_calc),"0"),"initial readout");
    sweep();
    CHECK(installed && installed->menus[0].items[0].enabled &&
          installed->menus[0].items[0].cmd_char=='W' &&
          !strcmp(installed->menus[0].items[0].text,"Close"),"Close W retained");
    /* The supported shutdown contract, as documented by tbxgate.h:
     * explicitly registered Quit; never infer consent from a close box. */
    if(installed) for(unsigned m=0;m<installed->n_menus;m++) {
        const tbx_menuinfo_t *mi=&installed->menus[m];
        for(unsigned k=0;k<mi->n_items;k++) {
            const tbx_menuitem_t *it=&mi->items[k];
            if(it->enabled && !it->is_divider && !strcmp(it->text,"Quit"))
                quit=TBX_MENU_WORD(mi->menu_id,k+1);
        }
    }
    CHECK(quit==0x00810002,"shutdown has an explicit enabled Quit route");
    menu(quit);
    CHECK(exits==1 && tbx_host.exit_rc==0,"shutdown quit closes the documentless Calculator");
    exits=0;menu(0x00810001);
    CHECK(exits==1 && tbx_host.exit_rc==0,"Close W closes Calculator");
    exits=0;menu(0x00810003);
    CHECK(exits==0,"unrelated menu does not close Calculator");
    printf("test_calcda_route: %d checks, %d failures\n",checks,fails);
    return fails?1:0;
}
