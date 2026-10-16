/* R4.5, tdnl.19. Included by tbxgate.c: small privileged adapters, no UI.
 * Ref: spec/cpanels_record.h; int21.c AH=2A..2D; About's int21_mcb_stats;
 * event.c raw delta synthesis; finder_desktop.c click tracker; desktop.c.
 * Speaker: IBM PC/AT channel2 data42, control43, mode B6, port61 bits0/1;
 * Intel8254 mode3 as pit.c; duration in the existing 100Hz pump ticks.
 * PC speaker amplitude is fixed: the panel honestly offers silent/short/full.
 */
#include "desktop.h"
#include "menu.h"
#include "pit.h"
static cp_settings_t cp_live={0,1,1,0,2,1};
static uint32_t cp_total_k,cp_beep_until;
static int cp_dirty;
static void cp_apply_live(void) {
#ifndef CP_MUT_MOUSE_INERT
    flair_mouse_tracking=cp_live.tracking;
    flair_double_click_ticks=cp_live.double_click==0?40u:cp_live.double_click==1?20u:10u;
#endif
    desktop_set_pattern(cp_live.pattern);
    MenuClock_enable(cp_live.clock);

}
/* The boot path uses the same DOS handle adapter as tenants, with a temporary
 * kernel-owned PSP. It never creates/repairs a corrupt file silently. */
void tbx_preferences_boot(uint32_t total_k) {
    psp_t p;uint8_t b[17];uint32_t a[3]={(uint32_t)(uintptr_t)CP_PATH,0,0};
    cp_total_k=total_k;cp_defaults(&cp_live);tbxf_init(&p);
    int h=tbxf_call(&p,0,0xffffffffu,TBX_FILE_OPEN,a);
    if(h>=0) {
        a[0]=(uint32_t)h;a[1]=(uint32_t)(uintptr_t)b;a[2]=sizeof b;
        int n=tbxf_call(&p,0,0xffffffffu,TBX_FILE_READ,a);
        (void)tbxf_call(&p,0,0xffffffffu,TBX_FILE_CLOSE,a);
        if(n!=16 || !cp_decode(&cp_live,b)) {cp_defaults(&cp_live);tputs("CPANELS-CORRUPT defaults\n");}
        else {tputs("CPANELS-LOAD pattern=");tputu(cp_live.pattern);tputs(" tracking=");tputu(cp_live.tracking);tputs(" double=");tputu(cp_live.double_click);tputs("\n");}
    } else if(h!=TBX_ERR_DOS(2)) {tputs("CPANELS-READ-FAIL defaults\n");}
    cp_apply_live();cp_dirty=0;
}
int tbx_preferences_dirty(void) {int r=cp_dirty;cp_dirty=0;return r;}
void tbx_preferences_tick(void) {
    if(cp_beep_until && (int32_t)(flair_tick_count()-cp_beep_until)>=0) {
        outb(0x61,(uint8_t)(inb(0x61)&~3u));cp_beep_until=0;tputs("CPANELS-BEEP off\n");
    }
}
static int32_t cp_service(unsigned code,const uint32_t *a) {
    if(!g_slot.app || g_slot.exit_req)return TBX_ERR_NOTREG;
    if(code==TBX_SETTINGS) {
        if(a[0]>1 || !tbxf_span(img_lo(),img_len(),a[1],CP_RECORD_BYTES))return TBX_ERR_BADARG;
        uint8_t *b=(uint8_t *)(uintptr_t)a[1];
        if(!a[0])cp_encode(&cp_live,b);
        else {
            cp_settings_t next;
            if(!cp_decode(&next,b))return TBX_ERR_BADARG;
            cp_dirty|=next.pattern!=cp_live.pattern || next.clock!=cp_live.clock;
            cp_live=next;
            cp_apply_live();tputs("CPANELS-APPLY pattern=");tputu(cp_live.pattern);
            tputs(" tracking=");tputu(cp_live.tracking);tputs(" double=");tputu(cp_live.double_click);tputs(" ticks=");tputu(flair_double_click_ticks);tputs("\n");
        }return 0;
    }
    if(code==TBX_CLOCK) {
        if(a[0]>1 || !tbxf_span(img_lo(),img_len(),a[1],sizeof(cp_time_t)))return TBX_ERR_BADARG;
        cp_time_t *t=(cp_time_t *)(uintptr_t)a[1];int_frame_t f={0};
        if(!a[0]) {
            f.eax=0x2a00;int21_dispatch(&f);t->year=(uint16_t)f.ecx;t->month=(uint8_t)(f.edx>>8);t->day=(uint8_t)f.edx;
            f.eax=0x2c00;int21_dispatch(&f);t->hour=(uint8_t)(f.ecx>>8);t->minute=(uint8_t)f.ecx;t->second=(uint8_t)(f.edx>>8);t->pad=0;
        } else {
            f.eax=0x2b00;f.ecx=t->year;f.edx=((unsigned)t->month<<8)|t->day;int21_dispatch(&f);
            if((f.eax&255)!=0)return TBX_ERR_BADARG;
            f.eax=0x2d00;f.ecx=((unsigned)t->hour<<8)|t->minute;f.edx=(unsigned)t->second<<8;int21_dispatch(&f);
            if((f.eax&255)!=0)return TBX_ERR_BADARG;
            tputs("CPANELS-RTC hour=");tputu(t->hour);tputs(" minute=");tputu(t->minute);tputs("\n");
        }return 0;
    }
    if(code==TBX_MEMORY) {
        if(!tbxf_span(img_lo(),img_len(),a[0],16))return TBX_ERR_BADARG;
        uint32_t *m=(uint32_t *)(uintptr_t)a[0],total=0,free=0,largest=0;
        if(!int21_mcb_usage(&total,&free,&largest)){tputs("CPANELS-MEMORY unbound\n");}
        m[0]=cp_total_k;m[1]=cp_total_k-free/64;m[2]=largest/64;m[3]=0;
        tputs("CPANELS-MEMORY total=");tputu(m[0]);tputs(" system=");tputu(m[1]);tputs(" largest=");tputu(m[2]);tputs(" cache=0\n");return 0;
    }
    if(code==TBX_BEEP) {
        static const unsigned hz[]={880,660,1320};
        if(!cp_live.volume)return 0;
        unsigned div=PIT_INPUT_HZ/hz[cp_live.sound];outb(0x43,0xb6);outb(0x42,(uint8_t)div);outb(0x42,(uint8_t)(div>>8));
        outb(0x61,(uint8_t)(inb(0x61)|3));cp_beep_until=flair_tick_count()+(cp_live.volume==1?5u:15u);
        tputs("CPANELS-BEEP hz=");tputu(hz[cp_live.sound]);tputs("\n");return 0;
    }return TBX_ERR_BADCODE;
}
