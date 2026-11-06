/* Ref: dbase3-decomp/specs/file-formats/lbl.md Sections 2-6 and
 * specs/reports-labels/label-form-generator.md; Using III+ U5-66..69.
 * III+ labels: signature, 60-byte remarks, six LE words, 16x60, signature.
 */
#include "samir/kit.h"
#include "samir/rt.h"
#include "samir/workarea.h"
#include "samir/dbf.h"
#include "ui_apps.h"

#define LB_SIZE 1034
typedef struct {
    kit k;
    uint8_t raw[LB_SIZE];
    char name[128], remarks[61], line[16][61];
    int dim[6], current, dirty;
} lb_state;
static lb_state *lb;
#define L (*lb)
static int lb_dispose(int result)
{ samir_pal_t *p=L.k.pal;p->release(p,lb);lb=0;return result; }
static void lb_error(const char *message)
{kit_message(&L.k,24,message);(void)kit_key(&L.k);}
static void lb_copy(char *d,const char *s,int cap)
{int n=0;while(s[n] && n<cap){d[n]=s[n];n++;}d[n]=0;}
static int lb_u16(int off){return L.raw[off]|(L.raw[off+1]<<8);}
static void lb_set(int off,int n){L.raw[off]=(uint8_t)n;L.raw[off+1]=(uint8_t)(n>>8);}
static void lb_num(char *s,int n)
{
    char b[12];int i=0,j=0;
    do{b[i++]=(char)('0'+n%10);n/=10;}while(n);
    while(i)s[j++]=b[--i];
    s[j]=0;
}
static void lb_value(int row,int col,int n)
{char b[12];lb_num(b,n);kit_put(&L.k,row,col,b,-1);}
static int lb_name(const char *args)
{
    int n=0,dot=0;char quote=0;
    while(*args==' ')args++;
    if(*args=='\'' || *args=='"')quote=*args++;
    while(*args && (quote?*args!=quote:*args!=' ')) {
        char c=*args++;if(n>=122)return 0;
        if(c>='a' && c<='z')c-=32;
        L.name[n++]=c;
        if(c=='\\' || c==':')dot=0;
        else if(c=='.')dot=1;
    }
    if(quote && *args++!=quote)return 0;
    while(*args==' ')args++;
    if(*args)return 0;
    L.name[n]=0;
    if(!n || (n==1 && L.name[0]=='?'))return 2;
    if(!dot)lb_copy(L.name+n,".LBL",4);
    return 1;
}
static const char *const lb_sizes[]={
    "3 1/2 x 15/16 by 1","3 1/2 x 15/16 by 2","3 1/2 x 15/16 by 3",
    "4 x 1 7/16 by 1","3 2/10 x 11/12 by 3 (Cheshire)"};
static void lb_stock(int n)
{
    /* First three dimensions match the independently supplied test.lbl.
     * The last two stocks use 10 cpi and 6 lpi, rounded down to whole lines. */
    L.dim[0]=n==3?8:5;L.dim[1]=n==3?40:n==4?32:35;
    L.dim[2]=0;L.dim[3]=1;L.dim[4]=n==4?0:2;
    L.dim[5]=n<3?n+1:n==3?1:3;
    lb_copy(L.remarks,lb_sizes[n],60);L.dirty=1;
}
static void lb_defaults(void)
{
    rt_memset(L.raw,' ',sizeof L.raw);L.raw[0]=L.raw[1033]=2;
    rt_memset(L.line,0,sizeof L.line);L.current=0;
    lb_stock(0);
}
static int lb_valid(void)
{
    return L.dim[0]>=1 && L.dim[0]<=16 && L.dim[1]>=1 && L.dim[1]<=120 &&
        L.dim[2]>=0 && L.dim[2]<=120 && L.dim[3]>=0 && L.dim[3]<=16 &&
        L.dim[4]>=0 && L.dim[4]<=120 && L.dim[5]>=1 && L.dim[5]<=5 &&
        L.dim[2]+L.dim[1]*L.dim[5]+L.dim[4]*(L.dim[5]-1)<=250;
}
static void lb_decode(char *s,int off)
{
    int n=60;rt_memcpy(s,L.raw+off,60);
    while(n && s[n-1]==' ')n--;
    s[n]=0;
}
static int lb_load(void)
{
    samir_pal_t *p=L.k.pal;pal_fd fd=p->open(p,L.name,PAL_RD);
    int n,i;char extra;
    if(fd<0)return fd==-PAL_ENOENT?1:29;
    n=p->read(p,fd,L.raw,LB_SIZE);
    if(n==LB_SIZE && p->read(p,fd,&extra,1)!=0)n=-1;
    p->close(p,fd);
    if(n!=LB_SIZE || L.raw[0]!=2 || L.raw[1033]!=2)return 54;
    for(i=0;i<6;i++)L.dim[i]=lb_u16(61+2*i);
    if(!lb_valid())return 54;
    lb_decode(L.remarks,1);
    for(i=0;i<16;i++)lb_decode(L.line[i],73+i*60);
    L.dirty=0;return 0;
}
static int lb_save(void)
{
    samir_pal_t *p=L.k.pal;pal_fd fd;int i,ok,closed;
    if(!lb_valid()) {
        lb_error("Invalid dimensions: all labels plus gaps and margin must fit 250 characters.");return 0;
    }
    if(L.dirty) {
        rt_memset(L.raw,' ',sizeof L.raw);L.raw[0]=L.raw[1033]=2;
        rt_memcpy(L.raw+1,L.remarks,rt_strlen(L.remarks));
        for(i=0;i<6;i++)lb_set(61+2*i,L.dim[i]);
        for(i=0;i<16;i++)rt_memcpy(L.raw+73+i*60,L.line[i],rt_strlen(L.line[i]));
    }
    fd=p->open(p,L.name,PAL_WR|PAL_CREATE|PAL_TRUNC);
    if(fd<0){lb_error("Unable to create label file.");return 0;}
    ok=p->write(p,fd,L.raw,LB_SIZE)==LB_SIZE;closed=p->close(p,fd);
    if(!ok || closed){lb_error("Label file write failed.");return 0;}
    return 1;
}
static void lb_draw(void)
{
    int i;kit_clear_rows(&L.k,1,21);
    kit_fill(&L.k,0,0,80,' ',KIT_ATTR_NORMAL);
    kit_put(&L.k,0,1,"Options   Contents   Exit",-1);
    kit_put_attr(&L.k,1,2,KIT_ATTR_BRIGHT,"Label form: ",-1);kit_put(&L.k,1,15,L.name,-1);
    kit_put(&L.k,2,2,"Width:",-1);lb_value(2,9,L.dim[1]);
    kit_put(&L.k,2,16,"Height:",-1);lb_value(2,24,L.dim[0]);
    kit_put(&L.k,2,31,"Left margin:",-1);lb_value(2,44,L.dim[2]);
    kit_put(&L.k,2,52,"Labels across:",-1);lb_value(2,67,L.dim[5]);
    kit_put(&L.k,3,2,"Lines between:",-1);lb_value(3,17,L.dim[3]);
    kit_put(&L.k,3,25,"Spaces between:",-1);lb_value(3,41,L.dim[4]);
    kit_put(&L.k,4,2,L.remarks,-1);
    for(i=0;i<L.dim[0];i++) {
        lb_value(5+i,2,i+1);kit_put(&L.k,5+i,5,":",1);
        kit_fill(&L.k,5+i,7,60,' ',i==L.current?KIT_ATTR_REVERSE:KIT_ATTR_NORMAL);
        kit_put_attr(&L.k,5+i,7,i==L.current?KIT_ATTR_REVERSE:KIT_ATTR_NORMAL,L.line[i],-1);
    }
    kit_status(&L.k,"Label",L.name,0,0,1);
    kit_message(&L.k,23,"Enter - select   F10 - field names   Ctrl-W - save   Esc - menu");
}
static int lb_field(void)
{
    const char *list[128];int i,n=0,pick;
    const dbf_field_t *f;wa_env *e=xb_interp_env(L.k.ip);
    dbf_table *t=wa_table(e,wa_selected(e));
    for(i=0;i<128 && (f=dbf_field(t,i))!=0;i++)list[n++]=f->name;
    pick=kit_pick(&L.k,3,5,list,n,0);
    if(pick<0)return 0;
    i=(int)rt_strlen(L.line[L.current]);
    if(i+(i!=0)+(int)rt_strlen(list[pick])>60){lb_error("The label expression cannot exceed 60 characters.");return 0;}
    if(i && i<60)L.line[L.current][i++]=',';
    lb_copy(L.line[L.current]+i,list[pick],60-i);L.dirty=1;return 1;
}
static int lb_contents(void)
{
    char b[60];int key,n,changed;
    for(;;) {
        lb_draw();kit_message(&L.k,24,"Enter/down - next line   Up - previous   F10 - insert a field name");
        n=(int)rt_strlen(L.line[L.current]);rt_memset(b,' ',sizeof b);
        rt_memcpy(b,L.line[L.current],(uint32_t)n);
        key=kit_edit_field(&L.k,5+L.current,7,b,60,'C',0,&changed);
        if(changed && key!=KIT_K_ESC && key!=KIT_K_ABORT) {
            n=60;while(n && b[n-1]==' ')n--;
            rt_memcpy(L.line[L.current],b,(uint32_t)n);L.line[L.current][n]=0;L.dirty=1;
        }
        if(key==KIT_K_F10){lb_field();continue;}
        if(key==KIT_K_F1){kit_message(&L.k,24,"Use dBASE character expressions. Commas trim fields and insert a space.");continue;}
        if(key==KIT_K_SAVE || key==KIT_K_CTRL_END)return 1;
        if(key==KIT_K_ESC || key==KIT_K_ABORT || key==KIT_K_EOF)return 0;
        if(key==KIT_K_UP || key==KIT_K_PGUP){if(L.current)L.current--;}
        else if(L.current<L.dim[0]-1)L.current++;
        else L.current=0;
    }
}
static void lb_option(int item)
{
    static const int map[]={0,1,0,2,3,4,5};
    static const int lo[]={1,1,0,0,0,1},hi[]={16,120,120,16,120,5};
    int n,i,index,old;char b[12];
    if(item==0) {
        n=kit_pick(&L.k,3,4,lb_sizes,5,0);if(n>=0)lb_stock(n);
        if(L.current>=L.dim[0])L.current=L.dim[0]-1;
        return;
    }
    if(item==7){
        char text[61];lb_copy(text,L.remarks,60);
        if(kit_prompt(&L.k,24,"Remarks: ",text,60)){lb_copy(L.remarks,text,60);L.dirty=1;}
        return;
    }
    index=map[item];lb_num(b,L.dim[index]);
    if(!kit_prompt(&L.k,24,"New value: ",b,4))return;
    n=0;i=0;while(b[i]==' ')i++;
    if(!b[i])return;
    for(;b[i];i++){if(b[i]<'0' || b[i]>'9'){lb_error("Enter a whole number.");return;}n=n*10+b[i]-'0';}
    if(n<lo[index] || n>hi[index]){lb_error("Value outside the allowed range.");return;}
    old=L.dim[index];L.dim[index]=n;
    if(!lb_valid()){L.dim[index]=old;lb_error("Labels and spaces exceed the 250 character page width.");return;}
    if(L.current>=L.dim[0])L.current=L.dim[0]-1;
    L.dirty=1;
}
static void lb_options(char *buf)
{
    static const char *const names[]={"Predefined size","Label width","Label height","Left margin","Lines between labels","Spaces between labels","Labels across page","Remarks"};
    static const int map[]={0,1,0,2,3,4,5};
    int i,j,pos=0,start;char value[12];
    for(i=0;i<8;i++) {
        start=pos;for(j=0;names[i][j];j++)buf[pos++]=names[i][j];
        if(i>0 && i<7) {
            while(pos-start<25)buf[pos++]=' ';
            lb_num(value,L.dim[map[i]]);for(j=0;value[j];j++)buf[pos++]=value[j];
        }
        buf[pos++]=0;
    }
    buf[pos]=0;
}
int ui_create_label(xb_interp *ip,int modify,const char *args,int *ec)
{
    static const char *const titles[]={"Options","Contents","Exit"};
    static const char *const items[]={
        "Predefined size\0Label width\0Label height\0Left margin\0Lines between labels\0Spaces between labels\0Labels across page\0Remarks\0",
        "Edit label expressions\0","Save\0Abandon\0"};
    char options[320];const char *menus[3];
    wa_env *e=xb_interp_env(ip);int rc,choice,pad=0,done=0;
    *ec=0;
    if(!wa_is_open(e,wa_selected(e))){*ec=52;return -INTERP_ERR_EVAL;}
    lb=xb_interp_pal(ip)->acquire(xb_interp_pal(ip),sizeof *lb);
    if(!lb){*ec=3;return -INTERP_ERR_NOMEM;}
    L.k.ip=ip;L.k.pal=xb_interp_pal(ip);lb_defaults();rc=lb_name(args);
    if(!rc){*ec=10;return lb_dispose(-INTERP_ERR_EVAL);}
    if(rc==1){rc=lb_load();if(rc && (rc!=1 || modify)){*ec=rc;return lb_dispose(-INTERP_ERR_EVAL);}}
    kit_open(&L.k,ip);
    if(!L.name[0] || L.name[0]=='?') {
        char name[128];
        if(!kit_prompt(&L.k,24,"Enter label file name: ",L.name,60)){kit_close(&L.k);return lb_dispose(CMD_OK);}
        lb_copy(name,L.name,127);if(lb_name(name)!=1){kit_close(&L.k);*ec=10;return lb_dispose(-INTERP_ERR_EVAL);}
        rc=lb_load();if(rc && (rc!=1 || modify)){kit_close(&L.k);*ec=rc;return lb_dispose(-INTERP_ERR_EVAL);}
    }
    while(!done) {
        lb_options(options);menus[0]=options;menus[1]=items[1];menus[2]=items[2];
        lb_draw();choice=kit_menu(&L.k,titles,menus,3,pad,0);
        if(choice<0) {
            static const char *const abandon[]={"Resume designing","Abandon changes"};
            if(kit_pick(&L.k,7,20,abandon,2,0)==1)break;
            continue;
        }
        pad=choice>>8;
        if(pad==0)lb_option(choice&255);
        else if(pad==1){if(lb_contents())done=lb_save();}
        else if((choice&255)==0)done=lb_save();
        else done=1;
    }
    kit_close(&L.k);return lb_dispose(CMD_OK);
}
