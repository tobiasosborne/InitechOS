/* Ref: dbase3-decomp/specs/file-formats/frm.md Sections 2-7 and
 * specs/reports-labels/report-form-generator.md; Using III+ U5-73..81.
 * W4 experiment: the 1990-byte III+ form is shared with cmd/report.c.
 */
#include "samir/kit.h"
#include "samir/rt.h"
#include "samir/workarea.h"
#include "samir/dbf.h"
#include "ui_apps.h"

#define FR_SIZE 1990
#define FR_COLS 24
#define FR_TEXT 265
typedef struct {
    char expr[129], head[FR_TEXT];
    int width, dec;
    unsigned char total, active, scratch;
} fr_column;
typedef struct {
    kit *k;
    uint8_t raw[FR_SIZE];
    char name[128], title[FR_TEXT], gx[2][FR_TEXT], gh[2][FR_TEXT];
    fr_column col[FR_COLS];
    int current, ncol, dirty;
} fr_state;
static fr_state *fr;
#define F (*fr)
static int fr_dispose(int result)
{ samir_pal_t *p=F.k->pal;p->release(p,fr);fr=0;return result; }
static void fr_error(const char *message)
{ kit_message(F.k,24,message);(void)kit_key(F.k); }

static int fr_get(int off) { return F.raw[off] | (F.raw[off+1] << 8); }
static void fr_set(int off, int n)
{ F.raw[off]=(uint8_t)n; F.raw[off+1]=(uint8_t)(n>>8); }
static void fr_copy(char *d, const char *s, int max)
{ int n=0; while (s[n] && n<max) { d[n]=s[n]; n++; } d[n]=0; }
static void fr_number(char *s, int n)
{
    char t[12]; int i=0,j=0;
    do { t[i++]=(char)('0'+n%10); n/=10; } while(n);
    while(i) s[j++]=t[--i];
    s[j]=0;
}
static void fr_value(int row, int col, int n)
{ char b[12]; fr_number(b,n); kit_put(F.k,row,col,b,-1); }
static int fr_equal(const char *a,const char *b)
{
    for (;;) {
        char x=*a++,y=*b++;
        if(x>='a' && x<='z')x-=32;
        if(y>='a' && y<='z')y-=32;
        if(x!=y)return 0;
        if(!x)return 1;
    }
}
static const dbf_field_t *fr_field(const char *expr)
{
    wa_env *e=xb_interp_env(F.k->ip);
    dbf_table *t=wa_table(e,wa_selected(e));
    const dbf_field_t *f; int i;
    for(i=0;(f=dbf_field(t,i))!=0;i++)if(fr_equal(f->name,expr))return f;
    return 0;
}
static int fr_name(const char *args)
{
    int i=0,dot=0; char quote=0;
    while(*args==' ')args++;
    if(*args=='\'' || *args=='"')quote=*args++;
    while(*args && (quote ? *args!=quote : *args!=' ')) {
        char c=*args++;
        if(i>=122)return 0;
        if(c>='a' && c<='z')c-=32;
        F.name[i++]=c;
        if(c=='\\' || c==':')dot=0;
        else if(c=='.')dot=1;
    }
    if(quote && *args++!=quote)return 0;
    while(*args==' ')args++;
    if(*args)return 0;
    F.name[i]=0;
    if(!i || (i==1 && F.name[0]=='?'))return 2;
    if(!dot)fr_copy(F.name+i,".FRM",4);
    return 1;
}
static void fr_defaults(void)
{
    int i;
    rt_memset(F.raw,' ',sizeof F.raw);
    rt_memset(F.raw,0,0xE0);
    F.title[0]=F.gx[0][0]=F.gx[1][0]=F.gh[0][0]=F.gh[1][0]=0;
    rt_memset(F.col,0,sizeof F.col);
    for(i=0;i<FR_COLS;i++) {
        F.col[i].width=10; F.col[i].active=1;
        F.col[i].total='N'; F.col[i].scratch=' ';
    }
    fr_set(0,2); fr_set(0x7B6,80); fr_set(0x7B8,58);
    fr_set(0x7BA,8); fr_set(0x7BC,0); fr_set(0x7BE,0);
    F.raw[0x7C0]=F.raw[0x7C1]=F.raw[0x7C2]='N';
    F.raw[0x7C3]=0; fr_set(0x7C4,2);
    F.current=F.ncol=0; F.dirty=1;
}
static int fr_readstr(char *dst,int index,int total,int cap)
{
    int len=fr_get(4+index*2),off=index?fr_get(0x72+index*2):0;
    if(len<1 || len>cap || off+len>total || F.raw[0xE0+off+len-1])return 0;
    rt_memcpy(dst,F.raw+0xE0+off,(uint32_t)len);
    return 1;
}
static int fr_load(void)
{
    samir_pal_t *p=F.k->pal;
    pal_fd fd=p->open(p,F.name,PAL_RD);
    int n,i,strings=0,sum=0,total,groups;
    char extra;
    if(fd<0)return fd==-PAL_ENOENT?1:29;
    n=p->read(p,fd,F.raw,FR_SIZE);
    if(n==FR_SIZE && p->read(p,fd,&extra,1)!=0)n=-1;
    p->close(p,fd);
    total=fr_get(2); F.ncol=fr_get(0x7BE);
    if(n!=FR_SIZE || fr_get(0)!=2 || fr_get(0x7C4)!=2 || total>0x68C-0xE0 || F.ncol>FR_COLS || F.ncol<1)return 50;
    for(i=0;i<55;i++){int z=fr_get(4+i*2);sum+=z;if(z)strings++;}
    if(sum!=total || !(strings&1))return 50;
    groups=(strings-1)/2-F.ncol;
    if(groups<0 || groups>2 || !fr_readstr(F.title,0,total,sizeof F.title))return 50;
    for(i=0;i<groups;i++)
        if(!fr_readstr(F.gx[i],1+i*2,total,sizeof F.gx[i]) || !fr_readstr(F.gh[i],2+i*2,total,sizeof F.gh[i]))return 50;
    for(i=0;i<F.ncol;i++) {
        int o=0x68C+12*i,e=fr_get(o+8),h=fr_get(o+10);
        fr_column *c=&F.col[i];
        if(e<1 || h<1 || e>=strings || h>=strings || !fr_readstr(c->expr,e,total,sizeof c->expr) || !fr_readstr(c->head,h,total,sizeof c->head))return 50;
        c->width=fr_get(o); c->dec=fr_get(o+6); c->total=F.raw[o+5];
        c->active=F.raw[o+2]; c->scratch=F.raw[o+4];
        if(c->width<1 || c->width>250 || c->dec>15 || (c->total!='Y' && c->total!='N'))return 50;
    }
    F.dirty=0;
    return 0;
}
static int fr_heap(const char *s,int slot,int *used)
{
    int n=(int)rt_strlen(s)+1;
    if(*used+n>0x68C-0xE0 || slot>54)return 0;
    fr_set(4+2*slot,n);
    if(slot)fr_set(0x72+2*slot,*used);
    rt_memcpy(F.raw+0xE0+*used,s,(uint32_t)n); *used+=n;
    return 1;
}
static int fr_pack(void)
{
    int i,used=0,slot=0,ncol=0;
    /* Measure first: an oversized edit must leave the model retryable. */
    int needed=(int)rt_strlen(F.title)+1;
    if(F.gx[1][0] && !F.gx[0][0])return 0;
    for(i=0;i<2;i++)if(F.gx[i][0])needed+=(int)(rt_strlen(F.gx[i])+rt_strlen(F.gh[i]))+2;
    for(i=0;i<FR_COLS;i++)if(F.col[i].expr[0]) {
        needed+=(int)(rt_strlen(F.col[i].expr)+rt_strlen(F.col[i].head))+2; ncol++;
    }
    if(!ncol || needed>0x68C-0xE0)return 0;
    if(!F.dirty)return 1;
    rt_memset(F.raw+2,0,0xDE);
    if(!fr_heap(F.title,slot++,&used))return 0;
    for(i=0;i<2;i++)if(F.gx[i][0]) {
        if(!fr_heap(F.gx[i],slot++,&used) || !fr_heap(F.gh[i],slot++,&used))return 0;
    }
    ncol=0;
    for(i=0;i<FR_COLS;i++)if(F.col[i].expr[0]) {
        fr_column *c=&F.col[i]; int o=0x68C+12*ncol++;
        fr_set(o,c->width); fr_set(o+2,c->active); F.raw[o+4]=c->scratch;
        F.raw[o+5]=c->total; fr_set(o+6,c->dec);
        fr_set(o+8,slot); if(!fr_heap(c->expr,slot++,&used))return 0;
        fr_set(o+10,slot); if(!fr_heap(c->head,slot++,&used))return 0;
    }
    fr_set(2,used); fr_set(0x7BE,ncol);
    return 1;
}
static int fr_save(void)
{
    samir_pal_t *p=F.k->pal; pal_fd fd; int ok,closed;
    if(!fr_pack()) {
        fr_error("Define a column; keep expressions within 1452 bytes and subgroup under group.");
        return 0;
    }
    if(fr_get(0x7BA)+fr_get(0x7BC)>=fr_get(0x7B6)) {
        fr_error("Left and right margins must leave room on the page.");return 0;
    }
    fd=p->open(p,F.name,PAL_WR|PAL_CREATE|PAL_TRUNC);
    if(fd<0){fr_error("Unable to create report file.");return 0;}
    ok=p->write(p,fd,F.raw,FR_SIZE)==FR_SIZE; closed=p->close(p,fd);
    if(!ok || closed){fr_error("Report file write failed.");return 0;}
    return 1;
}
static void fr_draw(void)
{
    int i,x=2; kit_clear_rows(F.k,1,21);
    kit_fill(F.k,0,0,80,' ',KIT_ATTR_NORMAL);
    kit_put(F.k,0,1,"Options   Groups   Columns   Locate   Exit",-1);
    kit_put_attr(F.k,2,2,KIT_ATTR_BRIGHT,"Report form: ",-1);
    kit_put(F.k,2,15,F.name,-1);
    kit_put(F.k,4,2,"Page title:",-1);kit_put(F.k,4,15,F.title,-1);
    kit_put(F.k,6,2,"Page width:",-1);fr_value(6,15,fr_get(0x7B6));
    kit_put(F.k,6,25,"Left margin:",-1);fr_value(6,39,fr_get(0x7BA));
    kit_put(F.k,6,48,"Lines per page:",-1);fr_value(6,65,fr_get(0x7B8));
    kit_put(F.k,8,2,"Group:",-1);kit_put(F.k,8,10,F.gx[0],-1);
    kit_put(F.k,9,2,"Subgroup:",-1);kit_put(F.k,9,12,F.gx[1],-1);
    kit_put_attr(F.k,11,2,KIT_ATTR_BRIGHT,"Report Format (X = character, # = numeric total)",-1);
    for(i=0;i<FR_COLS && x<77;i++)if(F.col[i].expr[0]) {
        fr_column *c=&F.col[i];int w=c->width;
        if(w>77-x)w=77-x;
        kit_put(F.k,13,x,c->head,(int)rt_strlen(c->head)<w?(int)rt_strlen(c->head):w);
        kit_fill(F.k,15,x,w,c->total=='Y'?'#':'X',i==F.current?KIT_ATTR_REVERSE:KIT_ATTR_NORMAL);
        x+=w+1;
    }
    kit_put(F.k,18,2,"Current column:",-1);fr_value(18,18,F.current+1);
    kit_put(F.k,19,2,F.col[F.current].expr,-1);
    kit_status(F.k,"Report",F.name,0,0,1);
    kit_message(F.k,23,"Select - Enter   Previous/next column - PgUp/PgDn   Save - Ctrl-W");
}
/* Preserve a long imported expression unless the visible field is changed. */
static int fr_edit(char *s,int row,int col,int width,char type,int *changed)
{
    char b[66]; int n=(int)rt_strlen(s),key;
    if(n>width)n=width;
    rt_memset(b,' ',(uint32_t)width);rt_memcpy(b,s,(uint32_t)n);
    key=kit_edit_field(F.k,row,col,b,width,type,0,changed);
    if(*changed && key!=KIT_K_ESC && key!=KIT_K_ABORT) {
        n=width;while(n && b[n-1]==' ')n--;
        rt_memcpy(s,b,(uint32_t)n);s[n]=0;
    } else *changed=0;
    return key;
}
static int fr_numeric(const char *prompt,int value,int lo,int hi,int *out)
{
    char b[12];int n=0,i=0;
    fr_number(b,value);
    if(!kit_prompt(F.k,24,prompt,b,5))return 0;
    while(b[i]==' ')i++;
    if(!b[i])return 0;
    for(;b[i];i++){if(b[i]<'0' || b[i]>'9'){fr_error("Enter a whole number.");return 0;}n=n*10+b[i]-'0';}
    if(n<lo || n>hi){fr_error("Value outside the allowed range.");return 0;}
    *out=n;return 1;
}
static int fr_multiline(char *s,int width,const char *caption)
{
    char lines[4][66];int i,j=0,line=0,key,changed,any=0;
    rt_memset(lines,0,sizeof lines);
    for(i=0;s[i] && line<4;i++) {
        if(s[i]==';'){line++;j=0;}
        else if(j<width)lines[line][j++]=s[i];
    }
    kit_clear_rows(F.k,2,10);kit_put(F.k,3,2,caption,-1);
    kit_message(F.k,24,"Four lines; Enter - next line   Ctrl-End - finish   Esc - cancel");
    line=0;
    for(;;) {
        for(i=0;i<4;i++){kit_fill(F.k,5+i,5,width,' ',KIT_ATTR_NORMAL);kit_put(F.k,5+i,5,lines[i],-1);}
        key=fr_edit(lines[line],5+line,5,width,'C',&changed);any|=changed;
        if(key==KIT_K_ESC || key==KIT_K_ABORT || key==KIT_K_EOF)return 0;
        if(key==KIT_K_SAVE || key==KIT_K_CTRL_END)break;
        if(key==KIT_K_UP){if(line)line--;}
        else if(++line==4)break;
    }
    if(any) {
        int last=3,p=0;while(last>0 && !lines[last][0])last--;
        for(i=0;i<=last;i++) {
            if(i)s[p++]=';';
            for(j=0;lines[i][j];j++)s[p++]=lines[i][j];
        }
        s[p]=0;F.dirty=1;
    }
    return 1;
}
static void fr_auto(fr_column *c)
{
    const dbf_field_t *f=fr_field(c->expr);
    if(!c->head[0])fr_copy(c->head,c->expr,FR_TEXT-1);
    if(f){c->width=f->field_len>30?30:f->field_len;c->dec=f->dec_count;c->total=f->type=='N'?'Y':'N';}
}
static int fr_pickfield(char *s)
{
    const char *items[128];int i,n=0,pick;
    wa_env *e=xb_interp_env(F.k->ip);dbf_table *t=wa_table(e,wa_selected(e));
    const dbf_field_t *f;
    for(i=0;i<128 && (f=dbf_field(t,i))!=0;i++)items[n++]=f->name;
    pick=kit_pick(F.k,3,5,items,n,0);
    if(pick<0)return 0;
    fr_copy(s,items[pick],128);return 1;
}
static int fr_columns(int field)
{
    static const char *const labels[]={"Contents","Heading","Width","Decimal places","Total this column"};
    int key,changed,i,n;char b[12];
    for(;;) {
        fr_column *c=&F.col[F.current];fr_draw();kit_clear_rows(F.k,3,9);
        for(i=0;i<5;i++)kit_put(F.k,4+i,2,labels[i],-1);
        kit_put(F.k,4,23,c->expr,-1);kit_put(F.k,5,23,c->head,-1);
        fr_value(6,23,c->width);fr_value(7,23,c->dec);
        b[0]=(char)c->total;b[1]=0;kit_put(F.k,8,23,b,1);
        kit_message(F.k,24,"Enter/down - next option   F10 - fields   Esc - menu   Ctrl-U - delete column");
        changed=0;
        if(field<2)key=fr_edit(field?c->head:c->expr,4+field,23,55,'C',&changed);
        else {
            if(field==4){b[0]=(char)c->total;b[1]=0;}
            else fr_number(b,field==2?c->width:c->dec);
            key=fr_edit(b,4+field,23,field==4?1:3,field==4?'L':'N',&changed);
            if(changed) {
                n=0;
                if(field!=4) {
                    for(i=0;b[i]==' ';i++);
                    if(!b[i])n=-1;
                    for(;b[i];i++) {
                        if(b[i]<'0' || b[i]>'9'){n=-1;break;}
                        n=n*10+b[i]-'0';
                    }
                }
                if(field==2){if(n>=1 && n<=250)c->width=n;else fr_error("Column width must be 1 to 250.");}
                else if(field==3){if(n>=0 && n<=15)c->dec=n;else fr_error("Decimal places must be 0 to 15.");}
                else if(b[0]=='Y' || b[0]=='y' || b[0]=='T' || b[0]=='t')c->total='Y';
                else c->total='N';
            }
        }
        if(changed){F.dirty=1;if(field==0)fr_auto(c);}
        if(key==KIT_K_F10 && field==0){if(fr_pickfield(c->expr)){fr_auto(c);F.dirty=1;}continue;}
        if(key==KIT_K_SAVE || key==KIT_K_CTRL_END)return 1;
        if(key==KIT_K_ESC || key==KIT_K_ABORT || key==KIT_K_EOF)return 0;
        if(key==KIT_K_F1){kit_message(F.k,24,"PgUp/PgDn change column; Ctrl-N inserts, Ctrl-U deletes; Ctrl-W saves.");continue;}
        if(key==KIT_K_CTRL_N) {
            if(F.col[FR_COLS-1].expr[0])continue;
            for(i=FR_COLS-1;i>F.current;i--)F.col[i]=F.col[i-1];
            rt_memset(c,0,sizeof *c);c->width=10;c->total='N';c->active=1;c->scratch=' ';
            F.dirty=1;field=0;continue;
        }
        if(key==KIT_K_TOGGLE_DEL) {
            for(i=F.current;i<FR_COLS-1;i++)F.col[i]=F.col[i+1];
            rt_memset(&F.col[FR_COLS-1],0,sizeof F.col[0]);
            F.col[FR_COLS-1].width=10;F.col[FR_COLS-1].total='N';F.col[FR_COLS-1].active=1;F.col[FR_COLS-1].scratch=' ';
            F.dirty=1;continue;
        }
        if(key==KIT_K_PGDN){if(F.current<FR_COLS-1)F.current++;field=0;}
        else if(key==KIT_K_PGUP){if(F.current)F.current--;field=0;}
        else if(key==KIT_K_UP){if(field)field--;}
        else field=(field+1)%5;
    }
}
static void fr_options(char *buf)
{
    static const char *const names[]={"Page title","Page width","Left margin","Right margin","Lines per page","Double space","Page eject before","Page eject after","Plain page"};
    int i,pos=0,j,start,n;char value[12];
    for(i=0;i<9;i++) {
        start=pos;for(j=0;names[i][j];j++)buf[pos++]=names[i][j];
        while(pos-start<24)buf[pos++]=' ';
        if(!i)fr_copy(value,"(4 lines)",11);
        else if(i<5){n=fr_get(i==1?0x7B6:i==2?0x7BA:i==3?0x7BC:0x7B8);fr_number(value,n);}
        else {
            n=i==5?F.raw[0x7C0]=='Y':i==6?!(F.raw[0x7C3]&1):i==7?(F.raw[0x7C3]&2):(F.raw[0x7C3]&4);
            value[0]=n?'Y':'N';value[1]=0;
        }
        for(j=0;value[j];j++)buf[pos++]=value[j];
        buf[pos++]=0;
    }
    buf[pos]=0;
}
int ui_create_report(xb_interp *ip,int modify,const char *args,int *ec)
{
    static const char *const titles[]={"Options","Groups","Columns","Locate","Exit"};
    static const char *const items[]={
        "Page title\0Page width\0Left margin\0Right margin\0Lines per page\0Double space\0Page eject before\0Page eject after\0Plain page\0",
        "Group on expression\0Group heading\0Sub-group on expression\0Sub-group heading\0Summary report only\0Page eject after group\0",
        "Contents\0Heading\0Width\0Decimal places\0Total this column\0",
        "Select column\0","Save\0Abandon\0"};
    kit screen;char options[384];const char *menus[5];
    wa_env *env=xb_interp_env(ip);int choice,pad=0,item,n,rc,done=0;
    *ec=0;
    if(!wa_is_open(env,wa_selected(env))){*ec=52;return -INTERP_ERR_EVAL;}
    fr=xb_interp_pal(ip)->acquire(xb_interp_pal(ip),sizeof *fr);
    if(!fr){*ec=3;return -INTERP_ERR_NOMEM;}
    F.k=&screen;F.k->ip=ip;F.k->pal=xb_interp_pal(ip);fr_defaults();
    rc=fr_name(args);
    if(!rc){*ec=10;return fr_dispose(-INTERP_ERR_EVAL);}
    if(rc==1) {
        rc=fr_load();
        if(rc!=0 && (rc!=1 || modify)){*ec=rc;return fr_dispose(-INTERP_ERR_EVAL);}
    }
    kit_open(F.k,ip);
    if(!F.name[0] || F.name[0]=='?') {
        if(!kit_prompt(F.k,24,"Enter report file name: ",F.name,60)){kit_close(F.k);return fr_dispose(CMD_OK);}
        {char name[128];fr_copy(name,F.name,127);if(fr_name(name)!=1){kit_close(F.k);*ec=10;return fr_dispose(-INTERP_ERR_EVAL);}}
        rc=fr_load();if(rc && (rc!=1 || modify)){kit_close(F.k);*ec=rc;return fr_dispose(-INTERP_ERR_EVAL);}
    }
    while(!done) {
        int i;for(i=0;i<5;i++)menus[i]=items[i];
        fr_options(options);menus[0]=options;
        fr_draw();choice=kit_menu(F.k,titles,menus,5,pad,0);
        if(choice<0) {
            static const char *const abandon[]={"Resume designing","Abandon changes"};
            if(kit_pick(F.k,7,20,abandon,2,0)==1)break;
            continue;
        }
        pad=choice>>8;item=choice&255;
        if(pad==0) {
            if(item==0)fr_multiline(F.title,59,"Enter report title. Exit - Ctrl-End.");
            else if(item<5) {
                int off=item==1?0x7B6:item==2?0x7BA:item==3?0x7BC:0x7B8;
                if(fr_numeric("New value: ",fr_get(off),item==2 || item==3?0:1,500,&n)) {
                    int old=fr_get(off);fr_set(off,n);
                    if(fr_get(0x7BA)+fr_get(0x7BC)>=fr_get(0x7B6)) {
                        fr_set(off,old);fr_error("Margins must leave at least one character on the page.");
                    }
                }
            } else if(item==5)F.raw[0x7C0]=F.raw[0x7C0]=='Y'?'N':'Y';
            else F.raw[0x7C3]^=(uint8_t)(item==6?1:item==7?2:4);
        } else if(pad==1) {
            if(item<4) {
                char *s=(item&1)?F.gh[item/2]:F.gx[item/2];
                char text[61];fr_copy(text,s,60);
                if(kit_prompt(F.k,24,(item&1)?"Group heading: ":"Group expression: ",text,60)) {
                    fr_copy(s,text,60);F.dirty=1;
                }
            } else F.raw[item==4?0x7C1:0x7C2]=F.raw[item==4?0x7C1:0x7C2]=='Y'?'N':'Y';
        } else if(pad==2) {
            if(item==1)fr_multiline(F.col[F.current].head,65,"Enter column heading. Exit - Ctrl-End.");
            else if(fr_columns(item))done=fr_save();
        } else if(pad==3) {
            const char *list[FR_COLS];int i;
            for(i=0;i<FR_COLS;i++)list[i]=F.col[i].expr[0]?F.col[i].expr:"<new column>";
            n=kit_pick(F.k,2,3,list,FR_COLS,F.current);
            if(n>=0){F.current=n;pad=2;if(fr_columns(0))done=fr_save();}
        } else if(item==0)done=fr_save();
        else done=1;
    }
    kit_close(F.k);return fr_dispose(CMD_OK);
}
