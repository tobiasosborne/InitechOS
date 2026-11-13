/* R4.5, tdnl.19. Panel state/transactions; no hardware or file I/O here.
 * Ref: period-gui-suite.md sec5, lane acceptance. Native layouts unavailable:
 * composed from dialog_record.h/control_record.h and chrome_metrics.h.
 * All item geometry authored product layout, NOT a reconstructed Mac panel. */
#include "cpanels.h"
static const char *const names[]={"Control Panels","Date & Time","Desktop Patterns","Mouse","Sound","Memory"};
void cp_init(cp_state_t *s,const cp_settings_t *p) {
    *s=(cp_state_t){0}; s->live=*p; s->edit=*p;
}
static void line(char *d,const char *label,uint32_t v,const char *suffix) {
    char b[11];unsigned n=0,i=0;while(*label)d[i++]=*label++;
    do{b[n++]=(char)('0'+v%10);v/=10;}while(v);while(n)d[i++]=b[--n];
    while(*suffix)d[i++]=*suffix++;
    d[i]=0;
}
/* Gregorian month length, same valid calendar as the DOS RTC seam. */
static int days(const cp_time_t *t) {
    static const unsigned char count[]={31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned m=t->month?t->month:1;
    return count[m-1]+(m==2 && t->year%4==0 && (t->year%100!=0 || t->year%400==0));
}
static void add(cp_item_t *p,unsigned *n,int x,int y,int w,unsigned type,int value,const char *text) {
    p[(*n)++]=(cp_item_t){(int16_t)x,(int16_t)y,(int16_t)(x+w),(int16_t)(y+20),(uint16_t)type,(int16_t)value,text};
}
unsigned cp_items(cp_state_t *s,cp_item_t *p) {
    unsigned n=0;
    add(p,&n,309,220,59,1,0,s->panel?"OK":"Close");
    add(p,&n,237,220,59,1,0,"Cancel");
    if(!s->panel) {
        for(unsigned i=1;i<=5;i++)add(p,&n,20,22+28*(int)i,200,1,0,names[i]);
    } else if(s->panel==1) {
        add(p,&n,24,50,59,1,0,"Hour -");add(p,&n,210,50,59,1,0,"Hour +");
        add(p,&n,24,78,59,1,0,"Min -");add(p,&n,210,78,59,1,0,"Min +");
        add(p,&n,24,174,240,2,s->edit.clock,"Menu Bar Clock");
        add(p,&n,24,106,59,1,0,"Day -");add(p,&n,210,106,59,1,0,"Day +");
        add(p,&n,24,134,59,1,0,"Mon -");add(p,&n,210,134,59,1,0,"Mon +");
        add(p,&n,90,134,59,1,0,"Year -");add(p,&n,280,134,59,1,0,"Year +");
        add(p,&n,24,4,140,1,0,"Date & Time");
        add(p,&n,180,4,188,1,0,"Menu Bar Clock");
        line(s->text[0],"Time: ",s->time.hour,":");
        unsigned j=0;while(s->text[0][j])j++;
        s->text[0][j++]=(char)('0'+s->time.minute/10);s->text[0][j++]=(char)('0'+s->time.minute%10);s->text[0][j]=0;
        line(s->text[1],"Year: ",s->time.year,"");line(s->text[2],"Month: ",s->time.month,"");line(s->text[3],"Day: ",s->time.day,"");
        add(p,&n,24,26,330,0,0,s->text[0]);add(p,&n,24,174,110,0,0,s->text[1]);
        add(p,&n,140,174,100,0,0,s->text[2]);add(p,&n,260,174,90,0,0,s->text[3]);
        if(s->clock_tab) {
            for(unsigned i=2;i<13;i++)if(i!=6){p[i].type=0;p[i].text="";}
            for(unsigned i=15;i<n;i++)p[i].text="";
        } else {p[6].type=0;p[6].text="";}
    } else if(s->panel==2) {
        static const char *const labels[]={"Teal solid","Black","Dark stipple","Stipple","Gray","Rows","Columns","Diagonal","Lines"};
        for(unsigned i=0;i<9;i++)add(p,&n,24+120*(int)(i%3),50+40*(int)(i/3),108,3,s->edit.pattern==i,labels[i]);
    } else if(s->panel==3 || s->panel==4) {
        static const char *const speeds[]={"Slow","Normal","Fast"};
        static const char *const sounds[]={"Beep","Bell","Click"};
        static const char *const volumes[]={"Silent","Short","Full"};
        for(unsigned i=0;i<3;i++)add(p,&n,24,50+24*(int)i,160,3,(s->panel==3?s->edit.tracking:s->edit.sound)==i,s->panel==3?speeds[i]:sounds[i]);
        for(unsigned i=0;i<3;i++)add(p,&n,210,50+24*(int)i,160,3,(s->panel==3?s->edit.double_click:s->edit.volume)==i,s->panel==3?speeds[i]:volumes[i]);
        if(s->panel==4)add(p,&n,24,140,59,1,0,"Test");
        add(p,&n,24,26,170,0,0,s->panel==3?"Tracking Speed":"Alert Sound");
        add(p,&n,210,26,160,0,0,s->panel==3?"Double-Click Speed":"Beep Length");
    } else {
        static const char *const labels[]={"Built-in Memory: ","System: ","Largest Unused Block: ","Disk Cache: "};
        for(unsigned i=0;i<4;i++){line(s->text[i],labels[i],s->memory[i]," K");add(p,&n,24,50+28*(int)i,340,0,0,s->text[i]);}
        add(p,&n,24,166,340,0,0,"Virtual Memory: Off");
    }
    add(p,&n,24,s->panel==1?198:4,340,0,0,s->error?"Settings could not be saved.":names[s->panel]);
    return n;
}
void cp_hit(cp_state_t *s,unsigned item,const cp_ops_t *o) {
    if(!s->panel) {if(item<=2)s->closed=1;else if(item<=7){s->panel=item-2;s->edit=s->live;s->original_time=s->time;s->error=0;s->clock_tab=0;}return;}
    if(item==2) {
#ifdef CP_MUT_CANCEL_WRITES
        (void)o->save(o->user,&s->edit);
#endif
        s->edit=s->live;s->time=s->original_time;s->panel=0;s->error=0;return;
    }
    if(item==1) {
        if(s->panel!=5) {
#ifndef CP_MUT_OK_NOTHING
            if(!o->save(o->user,&s->edit)){s->error=1;return;}
#endif
#ifndef CP_MUT_CLOCK_NOT_APPLIED
            if(s->panel==1 && !o->clock(o->user,&s->time)){s->error=1;return;}
#endif
            if(!o->apply(o->user,&s->edit)){s->error=1;return;}
            s->live=s->edit;
        }
        s->panel=0;return;
    }
    if(s->panel==1) {
        switch(item) {
        case 3:s->time.hour=(s->time.hour+23)%24;break;case 4:s->time.hour=(s->time.hour+1)%24;break;
        case 5:s->time.minute=(s->time.minute+59)%60;break;case 6:s->time.minute=(s->time.minute+1)%60;break;
        case 7:s->edit.clock^=1;break;
        case 8:s->time.day=s->time.day>1?s->time.day-1:days(&s->time);break;case 9:s->time.day=s->time.day<days(&s->time)?s->time.day+1:1;break;
        case 10:s->time.month=s->time.month>1?s->time.month-1:12;break;case 11:s->time.month=s->time.month<12?s->time.month+1:1;break;
        case 12:if(s->time.year>1980)s->time.year--;break;case 13:if(s->time.year<2099)s->time.year++;break;
        case 14:s->clock_tab=0;break;case 15:s->clock_tab=1;break;
        default:break;
        }
        if(s->time.day>days(&s->time))s->time.day=(uint8_t)days(&s->time);
    } else if(s->panel==2 && item>=3 && item<=11)s->edit.pattern=(uint8_t)(item-3);
    else if(s->panel==3) {
        if(item>=3&&item<=5)s->edit.tracking=(uint8_t)(item-3);
        if(item>=6&&item<=8)s->edit.double_click=(uint8_t)(item-6);
    } else if(s->panel==4) {
        if(item>=3&&item<=5)s->edit.sound=(uint8_t)(item-3);
        if(item>=6&&item<=8)s->edit.volume=(uint8_t)(item-6);
        if(item>=3&&item<=9)o->beep(o->user);
    }
}
