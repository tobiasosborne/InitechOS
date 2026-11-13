/* Shared host Toolbox, derived from the per-app recording fakes.
 * Sources and deliberate model limits are in sdk/README.md. */
#include "tbx_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

tbx_host_state_t tbx_host = {
    .cw=613,.ch=397,.origin_h=6,.origin_v=62,.scrap_len=-1,
    .clock={1992,3,9,12,0,0,0},.free_bytes=TBX_ERR_DOS(15),.write_budget=-1
};
#define S tbx_host
static int32_t pack(int h,int v) { return (int32_t)(((uint32_t)(uint16_t)v<<16)|(uint16_t)h); }
static void copy(char *to,size_t n,const char *from) { snprintf(to,n,"%s",from?from:""); }
static void record(const char *name,int32_t a,int32_t b,int32_t c,int32_t d,int32_t e,int32_t f,const char *s)
{
    if(S.nops==TBX_HOST_OPS) { S.dropped_ops++; return; }
    tbx_host_op_t *o=&S.ops[S.nops++];
    *o=(tbx_host_op_t){.name=name,.a={a,b,c,d,e,f}};
    copy(o->text,sizeof o->text,s);
}
#define R(a,b,c,d,e,f,s) record(__func__,a,b,c,d,e,f,s)
void tbx_host_reset(void)
{
    for(unsigned i=0;i<TBX_HOST_FILES;i++) free(S.files[i].data);
    memset(&S,0,sizeof S);
    S.cw=613;S.ch=397;S.origin_h=6;S.origin_v=62;S.scrap_len=-1;S.write_budget=-1;
    S.clock=(cp_time_t){1992,3,9,12,0,0,0};S.free_bytes=TBX_ERR_DOS(15);
    S.memory[0]=8192;S.memory[1]=2048;S.memory[2]=6144;
    cp_defaults(&S.settings);
    for(int i=0;i<TBX_HOST_H;i++) { memset(S.scr[i],' ',TBX_HOST_W);memset(S.bg[i],' ',TBX_HOST_W); }
}
int32_t tbx_register(void) { R(0,0,0,0,0,0,0);S.registrations++;return TBX_OK; }
int32_t tbx_new_window(int32_t l,int32_t t,int32_t r,int32_t b,int32_t g)
{
    R(l,t,r,b,g,0,0);
    if(S.nwindows==TBX_TENANT_WINDOWS) return TBX_ERR_BUSY;
    unsigned i=(unsigned)S.nwindows++;
    S.windows[i].l=l;S.windows[i].t=t;S.windows[i].r=r;S.windows[i].b=b;S.windows[i].goaway=g;
    return S.nwindows;
}
static int win(int32_t w) { return w>0 && w<=S.nwindows; }
int32_t tbx_set_wtitle(int32_t w,const char *s)
{ R(w,0,0,0,0,0,s);if(!win(w))return TBX_ERR_BADARG;copy(S.windows[w-1].title,128,s);return TBX_OK; }
int32_t tbx_content_origin(int32_t w) { R(w,0,0,0,0,0,0);return win(w)?pack(S.origin_h,S.origin_v):TBX_ERR_BADARG; }
int32_t tbx_content_size(int32_t w) { R(w,0,0,0,0,0,0);return win(w)?pack(S.cw,S.ch):TBX_ERR_BADARG; }
int32_t tbx_draw_text(int32_t w,int32_t x,int32_t y,const char *s,uint32_t f,uint32_t b)
{ R(w,x,y,(int32_t)f,(int32_t)b,0,s);return win(w)?TBX_OK:TBX_ERR_BADARG; }
int32_t tbx_fill_rect(int32_t w,int32_t l,int32_t t,int32_t r,int32_t b,uint32_t c)
{ R(w,l,t,r,b,(int32_t)c,0);return win(w)?TBX_OK:TBX_ERR_BADARG; }
int32_t tbx_draw_cells(int32_t w,int32_t x,int32_t y,const char *s,uint32_t f,uint32_t b)
{
    R(w,x,y,(int32_t)f,(int32_t)b,0,s);
    int col=(x-1)/8,line=y/16;
    if(!win(w)||!s||x<1||(x-1)%8||y<0||y%16||line>=TBX_HOST_H||
       (f&255)>=TBX_COLOR_COUNT||(f>>16)||((f>>8)&~7u)||b>=TBX_COLOR_COUNT) return TBX_ERR_BADARG;
    for(int i=0;s[i]&&col+i<TBX_HOST_W;i++) {
        int k=col+i,inv=f==TBX_COLOR_NAVY&&b==TBX_COLOR_WHITE;
        S.scr[line][k]=s[i];S.bg[line][k]=b==TBX_COLOR_NAVY?'N':' ';S.inv[line][k]=(char)inv;
        if(!inv) { S.face[line][k]=(uint8_t)(f>>8);S.col[line][k]=(uint8_t)(((f&255)<<4)|b); }
    }
    return TBX_OK;
}
int32_t tbx_set_mbar(const tbx_menubar_t *b) { R(0,0,0,0,0,0,0);S.bar=b;return TBX_OK; }
int32_t tbx_draw_menubar(void) { R(0,0,0,0,0,0,0);S.menubar_draws++;return TBX_OK; }
int32_t tbx_exit(int32_t rc) { R(rc,0,0,0,0,0,0);S.exit_calls++;S.exit_rc=rc;return TBX_OK; }
int32_t tbx_alert(const char *t,const char *d,const char *c,const char *o)
{ R(0,0,0,0,0,0,t);S.alerts++;copy(S.alert_text,256,t);copy(S.alert_btn[0],32,d);copy(S.alert_btn[1],32,c);copy(S.alert_btn[2],32,o);return S.alert_rc; }
int32_t tbx_clock(uint32_t mode,cp_time_t *t)
{ R((int32_t)mode,0,0,0,0,0,0);S.clock_reads++;if(mode>1||!t)return TBX_ERR_BADARG;if(S.clock_rc)return S.clock_rc;if(mode)S.clock=*t;else *t=S.clock;return TBX_OK; }
int32_t tbx_get_clock(void *time) { return tbx_clock(0, time); }
int32_t tbx_settings(uint32_t mode,uint8_t rec[CP_RECORD_BYTES])
{ R((int32_t)mode,0,0,0,0,0,0);if(!rec||mode>1)return TBX_ERR_BADARG;if(mode)return cp_decode(&S.settings,rec)?TBX_OK:TBX_ERR_BADARG;cp_encode(&S.settings,rec);return TBX_OK; }
int32_t tbx_memory(uint32_t out[4]) { R(0,0,0,0,0,0,0);if(!out)return TBX_ERR_BADARG;memcpy(out,S.memory,sizeof S.memory);return TBX_OK; }
int32_t tbx_beep(void) { R(0,0,0,0,0,0,0);S.beeps++;return TBX_OK; }
int32_t tbx_dialog(int32_t w,const cp_item_t *items,uint32_t n,uint32_t def,uint32_t cancel,const tbx_event_t *e)
{ R(w,(int32_t)n,(int32_t)def,(int32_t)cancel,0,0,0);S.dialog_items=items;S.dialog_count=n;S.dialog_def=def;S.dialog_cancel=cancel;if(e)S.dialog_event=*e;return S.dialog_result; }
int32_t tbx_scrap_put(const void *b,uint32_t n)
{ R((int32_t)n,0,0,0,0,0,0);if(n>sizeof S.scrap||(!b&&n))return TBX_ERR_BADARG;if(n)memcpy(S.scrap,b,n);S.scrap_len=(int)n;S.scrap_puts++;return TBX_OK; }
int32_t tbx_scrap_get(void *b,uint32_t n)
{ R((int32_t)n,0,0,0,0,0,0);if(S.scrap_len<0)return TBX_ERR_NOTYPE;if(!b)return S.scrap_len;if(n<(unsigned)S.scrap_len)return TBX_ERR_BADARG;memcpy(b,S.scrap,(unsigned)S.scrap_len);return S.scrap_len; }
int32_t tbx_sf_getfile(char *r) { R(0,0,0,0,0,0,0);S.sf=1;S.sf_reply=r;return TBX_OK; }
int32_t tbx_sf_putfile(const char *p,const char *n,char *r) { R(0,0,0,0,0,0,p);S.sf=2;S.sf_reply=r;copy(S.sf_prompt,64,p);copy(S.sf_name,128,n);return TBX_OK; }
int32_t tbx_track_mouse(void) { R(0,0,0,0,0,0,0);S.track++;return TBX_OK; }
int32_t tbx_set_scroll(int32_t w,int32_t len,int32_t val)
{ R(w,len,val,0,0,0,0);S.setscroll++;if(!win(w))return TBX_ERR_BADARG;int max=len>S.ch?len-S.ch:0;if(val<0)val=0;if(val>max)val=max;S.windows[w-1].scroll_len=len;S.windows[w-1].scroll_val=val;return val; }
int32_t tbx_get_scroll(int32_t w) { R(w,0,0,0,0,0,0);return win(w)?S.windows[w-1].scroll_val:TBX_ERR_BADARG; }
int32_t tbx_global_to_local(int32_t w,int32_t h,int32_t v)
{ R(w,h,v,0,0,0,0);return win(w)?pack(h-S.origin_h,v-S.origin_v):TBX_ERR_BADARG; }
int32_t tbx_file_copy(const char *c,const char *s,const char *d)
{ R(0,0,0,0,0,0,c);copy(S.copy_caption,128,c);copy(S.copy_src,128,s);copy(S.copy_dst,128,d);return TBX_OK; }

/* Paths are DOS case-insensitive, rooted on one data volume. */
static int norm(const char *p,char out[TBX_FILE_PATH_MAX])
{
    unsigned n=0;if(!p)return 0;
    if(p[0]&&p[1]==':')p+=2;
    while(*p=='\\'||*p=='/')p++;
    for(;*p;p++) {
        if(n+1>=TBX_FILE_PATH_MAX)return 0;
        char c=*p=='/'?'\\':*p;
        out[n++]=(c>='a'&&c<='z')?(char)(c-32):c;
    }
    out[n]=0;return n!=0;
}
static int find(const char *p)
{ char name[TBX_FILE_PATH_MAX];if(!norm(p,name))return -1;for(int i=0;i<TBX_HOST_FILES;i++)if(S.files[i].used&&!strcmp(S.files[i].name,name))return i;return -1; }
const tbx_host_file_t *tbx_host_file(const char *p) { int i=find(p);return i<0?0:&S.files[i]; }
static int allocfile(const char *p)
{
    char name[TBX_FILE_PATH_MAX];if(!norm(p,name))return -1;
    for(int i=0;i<TBX_HOST_FILES;i++)if(!S.files[i].used) { free(S.files[i].data);memset(&S.files[i],0,sizeof S.files[i]);S.files[i].used=1;copy(S.files[i].name,sizeof S.files[i].name,name);return i; }
    return -1;
}
static int grow(tbx_host_file_t *f,uint32_t n)
{ if(n<=f->capacity)return 1;uint8_t *b=realloc(f->data,n);if(!b)return 0;memset(b+f->capacity,0,n-f->capacity);f->data=b;f->capacity=n;return 1; }
int tbx_host_put_file(const char *p,const void *b,uint32_t n)
{ int i=find(p);if(i<0)i=allocfile(p);if(i<0||!grow(&S.files[i],n))return 0;if(n)memcpy(S.files[i].data,b,n);S.files[i].size=n;return 1; }
static int handle(int h) { return h>=5&&h<5+TBX_HOST_HANDLES&&S.handles[h-5].used; }
static int remember_deleted(const char *p)
{ if(S.ndeleted==TBX_HOST_FILES*2)return 0;copy(S.deleted[S.ndeleted++],TBX_FILE_PATH_MAX,p);return 1; }
static int32_t error(unsigned op) { return S.file_error[op]?TBX_ERR_DOS(S.file_error[op]):0; }
static int32_t openfile(int i,uint32_t mode)
{ for(int h=0;h<TBX_HOST_HANDLES;h++)if(!S.handles[h].used){S.handles[h].used=1;S.handles[h].file=i;S.handles[h].pos=0;S.handles[h].mode=mode;return h+5;}return TBX_ERR_DOS(4); }
static int32_t create(const char *p,int exclusive)
{
    int32_t err=error(exclusive?TBX_FILE_CREATE_NEW:TBX_FILE_CREATE);if(err)return err;
    if(S.fail_create)return TBX_ERR_DOS(5);
    int i=find(p);if(i>=0&&exclusive)return TBX_ERR_DOS(80);
    /* No truncation or allocation when handles are exhausted. */
    int h;for(h=0;h<TBX_HOST_HANDLES&&S.handles[h].used;h++);if(h==TBX_HOST_HANDLES)return TBX_ERR_DOS(4);
    if(i>=0&&(S.files[i].attr&1))return TBX_ERR_DOS(5);
    if(i<0)i=allocfile(p);
    if(i<0)return TBX_ERR_DOS(5);
    S.files[i].size=0;return openfile(i,TBX_FILE_READ_WRITE);
}
int32_t tbx_file_create(const char *p) { R(0,0,0,0,0,0,p);return create(p,0); }
int32_t tbx_file_create_new(const char *p) { R(0,0,0,0,0,0,p);return create(p,1); }
int32_t tbx_file_open(const char *p,uint32_t mode)
{ R((int32_t)mode,0,0,0,0,0,p);int32_t e=error(TBX_FILE_OPEN);if(e)return e;int i=find(p);if(i<0)return TBX_ERR_DOS(2);if(mode>2)return TBX_ERR_BADARG;if(mode&&(S.files[i].attr&1))return TBX_ERR_DOS(5);return openfile(i,mode); }
int32_t tbx_file_read(int32_t h,void *b,uint32_t n)
{
    R(h,(int32_t)n,0,0,0,0,0);int32_t e=error(TBX_FILE_READ);if(e)return e;
    if(!handle(h)||(!b&&n))return TBX_ERR_BADARG;
    unsigned k=(unsigned)(h-5);if(S.handles[k].mode==TBX_FILE_WRITE_ONLY)return TBX_ERR_DOS(5);
    tbx_host_file_t *f=&S.files[S.handles[k].file];uint32_t pos=S.handles[k].pos;
    uint32_t avail=pos<f->size?f->size-pos:0;if(n>avail)n=avail;
    if(S.read_limit&&n>S.read_limit)n=S.read_limit;
    if(n)memcpy(b,f->data+pos,n);
    S.handles[k].pos+=n;return (int32_t)n;
}
int32_t tbx_file_write(int32_t h,const void *b,uint32_t n)
{
    R(h,(int32_t)n,0,0,0,0,0);int32_t e=error(TBX_FILE_WRITE);if(e)return e;
    if(!handle(h)||(!b&&n))return TBX_ERR_BADARG;
    unsigned k=(unsigned)(h-5);if(S.handles[k].mode==TBX_FILE_READ_ONLY)return TBX_ERR_DOS(5);
    if(S.fail_write)return 0;
    int truncate=n==0;
    if(S.write_limit&&n>S.write_limit)n=S.write_limit;
    if(S.write_budget>=0&&n>(uint64_t)S.write_budget)n=(uint32_t)S.write_budget;
    if(!n&&!truncate)return 0;
    tbx_host_file_t *f=&S.files[S.handles[k].file];uint32_t pos=S.handles[k].pos;
    if(n>INT32_MAX||pos>INT32_MAX-n||!grow(f,pos+n))return TBX_ERR_DOS(39);
    if(pos>f->size)memset(f->data+f->size,0,pos-f->size);
    if(n)memcpy(f->data+pos,b,n);
    if(S.write_budget>=0)S.write_budget-=n;
    S.handles[k].pos=pos+n;
    if(!n||pos+n>f->size)f->size=pos+n;
    return (int32_t)n;
}
int32_t tbx_file_seek(int32_t h,int32_t d,uint32_t o)
{
    R(h,d,(int32_t)o,0,0,0,0);int32_t e=error(TBX_FILE_SEEK);if(e)return e;
    if(!handle(h)||o>2)return TBX_ERR_BADARG;
    unsigned k=(unsigned)(h-5);int64_t base=o==0?0:o==1?S.handles[k].pos:S.files[S.handles[k].file].size;
    int64_t p=base+d;if(p<0||p>INT32_MAX)return TBX_ERR_BADARG;S.handles[k].pos=(uint32_t)p;return (int32_t)p;
}
int32_t tbx_file_close(int32_t h)
{ R(h,0,0,0,0,0,0);if(!handle(h))return TBX_ERR_BADARG;S.handles[h-5].used=0;if(S.fail_close&&(!S.fail_close_handle||h==S.fail_close_handle)){S.fail_close=0;return TBX_ERR_DOS(5);}return error(TBX_FILE_CLOSE); }
int32_t tbx_file_delete(const char *p)
{ R(0,0,0,0,0,0,p);int32_t e=error(TBX_FILE_DELETE);if(e)return e;int i=find(p);if(i<0)return TBX_ERR_DOS(2);if(S.files[i].attr&1)return TBX_ERR_DOS(5);if(!remember_deleted(S.files[i].name))return TBX_ERR_DOS(5);S.files[i].used=0;return TBX_OK; }
int32_t tbx_file_rename(const char *p,const char *q)
{ R(0,0,0,0,0,0,p);int32_t e=error(TBX_FILE_RENAME);if(e)return e;char name[TBX_FILE_PATH_MAX];if(!norm(q,name))return TBX_ERR_BADARG;if(find(q)>=0)return TBX_ERR_DOS(5);int i=find(p);if(i<0)return TBX_ERR_DOS(2);if(S.files[i].attr&1)return TBX_ERR_DOS(5);if(!remember_deleted(S.files[i].name))return TBX_ERR_DOS(5);copy(S.files[i].name,sizeof S.files[i].name,name);return TBX_OK; }
static int match(const char *p,const char *s)
{ if(!strcmp(p,"*.*"))return !strchr(s,'\\');if(!*p)return !*s;if(*p=='*')return match(p+1,s)||(*s&&*s!='\\'&&match(p,s+1));if(*p=='?')return *s&&*s!='\\'&&match(p+1,s+1);return *p==*s&&match(p+1,s+1); }
int32_t tbx_file_find(const char *p,uint32_t attr,void *buf,uint32_t max)
{
    R((int32_t)attr,(int32_t)max,0,0,0,0,p);S.find_calls++;
    int32_t e=error(TBX_FILE_FIND);if(e)return e;
    char pat[TBX_FILE_PATH_MAX];if(!norm(p,pat)||(!buf&&max)||max>TBX_FIND_MAX_RECS)return TBX_ERR_BADARG;
    uint32_t n=0;for(int i=0;i<TBX_HOST_FILES&&n<max;i++) {
        tbx_host_file_t *f=&S.files[i];if(!f->used||!match(pat,f->name)||((f->attr&0x16)&~attr))continue;
        const char *name=strrchr(f->name,'\\');name=name?name+1:f->name;
        tbx_find_rec_t rec={.attr=f->attr,.time=f->time,.date=f->date,.size=f->size};copy(rec.name,sizeof rec.name,name);
        memcpy((uint8_t *)buf+n*TBX_FIND_REC_BYTES,&rec,sizeof rec);n++;
    }
    return (int32_t)n;
}
int32_t tbx_disk_free(void) { R(0,0,0,0,0,0,0);int32_t e=error(TBX_DISK_FREE);return e?e:S.free_bytes; }
int tbx_host_push_event(const tbx_event_t *e)
{ if(!e||S.event_count==TBX_HOST_EVENTS)return 0;S.events[(S.event_head+S.event_count)%TBX_HOST_EVENTS]=*e;S.event_count++;return 1; }
unsigned tbx_host_dispatch(void (*callback)(const tbx_event_t *))
{ unsigned n=0;while(S.event_count&&!S.exit_calls){tbx_event_t e=S.events[S.event_head];S.event_head=(S.event_head+1)%TBX_HOST_EVENTS;S.event_count--;callback(&e);n++;}return n; }
/* Raw calls keep the i386 uint32 pointer convention. On a 64-bit host use
 * typed bindings for pointers; zero-argument beep is used by existing apps. */
TBX_TRAP_ABI int32_t tbx_trap(uint32_t c,uint32_t n,const uint32_t *a)
{ R((int32_t)c,(int32_t)n,0,0,0,0,0);(void)a;if(c==TBX_BEEP&&n==0)return tbx_beep();return TBX_ERR_BADCODE; }

static int hostpath(char path[1024],const char *dir,const char *name)
{
    if(snprintf(path,1024,"%s/%s",dir,name)>=1024)return 0;
    for(char *p=path+strlen(dir)+1;*p;p++)if(*p=='\\')*p='/';
    return 1;
}
static int import_tree(const char *dir,const char *rel)
{
    char path[1024];if(!hostpath(path,dir,rel))return 0;
    struct dirent **entries;int n=scandir(path,&entries,0,alphasort);
    if(n<0)return 0;
    int ok=1;
    for(int i=0;i<n;i++) {
        const char *name=entries[i]->d_name;
        if(ok&&strcmp(name,".")&&strcmp(name,"..")) {
            char key[TBX_FILE_PATH_MAX];
            if(snprintf(key,sizeof key,"%s%s%s",rel,*rel?"\\":"",name)>=(int)sizeof key||!hostpath(path,dir,key))ok=0;
            struct stat st;if(ok&&lstat(path,&st))ok=0;
            if(ok&&S_ISDIR(st.st_mode)) {
                ok=tbx_host_put_file(key,0,0);
                if(ok) { S.files[find(key)].attr=16;ok=import_tree(dir,key); }
            } else if(ok&&S_ISREG(st.st_mode)) {
                if(st.st_size<0||st.st_size>INT32_MAX)ok=0;
                FILE *f=ok?fopen(path,"rb"):0;
                if(!f)ok=0;
                if(ok) {
                    uint32_t size=(uint32_t)st.st_size;uint8_t *b=malloc(size?size:1);
                    if(!b)ok=0;
                    if(ok&&fread(b,1,size,f)!=size)ok=0;
                    if(fclose(f))ok=0;
                    if(ok)ok=tbx_host_put_file(key,b,size);
                    free(b);
                }
            }
        }
        free(entries[i]);
    }
    free(entries);return ok;
}
int tbx_host_import(const char *d) { return import_tree(d,""); }
int tbx_host_export(const char *dir)
{
    char path[1024];
    for(unsigned i=0;i<S.ndeleted;i++)if(find(S.deleted[i])<0) {
        if(!hostpath(path,dir,S.deleted[i]))return 0;
        if(remove(path)&&errno!=ENOENT)return 0;
    }
    for(int i=0;i<TBX_HOST_FILES;i++)if(S.files[i].used) {
        tbx_host_file_t *f=&S.files[i];
        if(!hostpath(path,dir,f->name))return 0;
        for(char *p=path+strlen(dir)+1;*p;p++)if(*p=='/') {
            *p=0;int rc=mkdir(path,0700);*p='/';if(rc&&errno!=EEXIST)return 0;
        }
        if(f->attr&16) { if(mkdir(path,0700)&&errno!=EEXIST)return 0;continue; }
        FILE *o=fopen(path,"wb");if(!o)return 0;
        int ok=fwrite(f->data,1,f->size,o)==f->size;if(fclose(o))ok=0;if(!ok)return 0;
    }
    return 1;
}
