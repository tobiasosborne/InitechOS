/* initech-tdnl.19: independent routing/transaction oracle. Ref: user R4.5
 * acceptance, period-gui-suite.md sec 5; real DialogHandleEvent, not hit stubs.
 * Expected item numbers/record bytes are authored independently of the view. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dialog.h"
#include "cpanels.h"
#include "cp_dialog.h"
static unsigned failures, writes, applies, clocks, beeps;
static cp_settings_t saved, applied;
#define CHECK(rule,c) do { if (!(c)) { printf("FAIL %s line=%d\n",rule,__LINE__); failures++; } } while (0)
static int save(void *u,const cp_settings_t *s) { (void)u; writes++; saved=*s; return 1; }
static int apply(void *u,const cp_settings_t *s) { (void)u; applies++; applied=*s; return 1; }
static int clockset(void *u,const cp_time_t *t) { (void)u; clocks++; return t->hour==13; }
static void beep(void *u) { (void)u; beeps++; }
static cp_ops_t ops={save,apply,clockset,beep,0};
static unsigned route(cp_state_t *s,unsigned item) {
    cp_item_t flat[CP_MAX_ITEMS]={0}; DialogRecord d={0}; DialogItem items[CP_MAX_ITEMS];
    ControlRecord controls[CP_MAX_ITEMS]; EventRecord e={0};
    unsigned n=cp_items(s,flat); uint16_t hit=0;
    cp_dialog_items(&d,items,controls,flat,n,0,0);
    d.defaultItem=1; d.cancelItem=2;
    if (item) { e.what=mouseDown; e.where.h=(flat[item-1].left+flat[item-1].right)/2;
        e.where.v=(flat[item-1].top+flat[item-1].bottom)/2; }
    else { e.what=keyDown; e.message=13; }
    (void)DialogHandleEvent(&d,&e,0,&hit);
    if (hit) cp_hit(s,hit,&ops);
    return hit;
}
int main(int argc,char **argv) {
    cp_state_t s; cp_settings_t pref; cp_defaults(&pref);
    CHECK("DEFAULTS",pref.pattern==0 && pref.tracking==1 && pref.double_click==1 && pref.clock==1);
    for (unsigned p=1;p<=5;p++) {
        writes=applies=clocks=beeps=0; cp_init(&s,&pref); s.time.hour=12;
        CHECK("CHOOSER",route(&s,p+2)==p+2 && s.panel==p);
        if (p==1) { CHECK("DATE",route(&s,4)==4 && s.time.hour==13); }
        if (p==2) { CHECK("PATTERN",route(&s,6)==6 && s.edit.pattern==3); }
        if (p==3) { CHECK("MOUSE",route(&s,5)==5 && s.edit.tracking==2); CHECK("MOUSE",route(&s,6)==6 && s.edit.double_click==0); }
        if (p==4) { CHECK("SOUND",route(&s,4)==4 && s.edit.sound==1 && beeps==1); }
        if (p==5) { CHECK("MEMORY",route(&s,3)==0); }
        CHECK("STAGED",writes==0 && applies==0 && clocks==0);
        route(&s,2); CHECK("CANCEL_WRITES",writes==0 && applies==0 && clocks==0 && s.panel==0);
        route(&s,p+2); if(p==1) route(&s,4); if(p==2) route(&s,6);
        if(p==3) {route(&s,5);route(&s,6);} if(p==4) route(&s,4);
        route(&s,0);
        CHECK("OK_WRITES",writes==(p==5?0u:1u));
        CHECK("APPLIED",applies==(p==5?0u:1u));
        CHECK("CLOCK_APPLIED",clocks==(p==1?1u:0u));
        if(p==2) CHECK("PATTERN",saved.pattern==3 && applied.pattern==3);
        if(p==3) CHECK("MOUSE",applied.tracking==2 && applied.double_click==0);
        if(p==4) CHECK("SOUND",applied.sound==1);
        CHECK("RETURN_CHOOSER",s.panel==0);
    }
    cp_init(&s,&pref);s.time=(cp_time_t){1992,2,28,12,0,0,0};
    route(&s,3);route(&s,9);CHECK("DATE_CALENDAR",s.time.day==29);
    route(&s,13);CHECK("DATE_CALENDAR",s.time.year==1993 && s.time.day==28);
    CHECK("CLOCK_TAB",route(&s,15)==15 && s.clock_tab==1);
    route(&s,7);CHECK("CLOCK_TAB",s.edit.clock==0);route(&s,2);
    CHECK("CLOCK_CANCEL",s.live.clock==1 && s.time.year==1992 && s.time.day==28);
    cp_init(&s,&pref);s.memory[0]=8192;s.memory[1]=2048;s.memory[2]=6144;s.memory[3]=0;
    route(&s,7);cp_item_t view[CP_MAX_ITEMS];cp_items(&s,view);
    CHECK("MEMORY_LABELS",!strcmp(view[2].text,"Built-in Memory: 8192 K") &&
          !strcmp(view[3].text,"System: 2048 K") &&
          !strcmp(view[4].text,"Largest Unused Block: 6144 K") &&
          !strcmp(view[5].text,"Disk Cache: 0 K") && !strcmp(view[6].text,"Virtual Memory: Off"));
    unsigned char rec[CP_RECORD_BYTES]; cp_encode(&pref,rec);
    const unsigned char expected[16]={'C','P','0','1',1,0,1,1,0,2,1,0,0,0,0xfa,0};
    CHECK("RECORD",memcmp(rec,expected,16)==0);
    for (unsigned i=0;i<16;i++) { rec[i]^=0x80; CHECK("CORRUPT",!cp_decode(&pref,rec)); rec[i]^=0x80; }
    CHECK("RECORD",cp_decode(&pref,rec));
    if(argc!=2) { puts("FAIL PAT_GOLDEN missing corpus root"); return 1; }
    char path[1024]; snprintf(path,sizeof path,"%s/goldens/resources/PAT#_0.bin",argv[1]);
    FILE *f=fopen(path,"rb"); unsigned char pat[66];
    CHECK("PAT_GOLDEN",f && fread(pat,1,66,f)==66); if(f) fclose(f);
    for(unsigned p=1;p<=8;p++) CHECK("PAT_GOLDEN",memcmp(cp_patterns[p-1],pat+2+8*(p-1),8)==0);
    if(failures) {printf("cpanels: %u failures\n",failures);return 1;}
    puts("cpanels: GREEN (five Dialog Manager routes, OK/Cancel transactions, RTC, binary corruption, eight independent PAT# tiles)");return 0;
}
