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

extern char __executable_start[],__bss_start[],_end[];
tbx_host_state_t tbx_host = {
    .cw=613,.ch=397,.origin_h=6,.origin_v=62,.scrap_len=-1,
    .clock={1992,3,9,12,0,0,0},.free_bytes=TBX_ERR_DOS(15),.write_budget=-1,
    .image_base=(uintptr_t)__executable_start,.image_end=(uintptr_t)__bss_start,
    .block_end=(uintptr_t)_end
};
#define S tbx_host
#include "tbx_validate.h"
static int32_t pack(int h,int v) { return (int32_t)(((uint32_t)(uint16_t)v<<16)|(uint16_t)h); }
static void copy(char *to,size_t n,const char *from) { snprintf(to,n,"%s",from?from:""); }
static void record(const char *name,int32_t a,int32_t b,int32_t c,int32_t d,int32_t e,int32_t f,const char *s)
{
    if(S.nops==TBX_HOST_OPS) { S.dropped_ops++; return; }
    tbx_host_op_t *o=&S.ops[S.nops++];
    *o=(tbx_host_op_t){.name=name,.a={a,b,c,d,e,f}};
    if(string_len(s,sizeof o->text,0)>=0)copy(o->text,sizeof o->text,s);
}
#define R(a,b,c,d,e,f,s) record(__func__,a,b,c,d,e,f,s)
void tbx_host_reset(void)
{
    /* ELF host equivalent of module + zero-filled allocation. Stack and heap
     * pointers are outside the tenant, exactly as on the live gate. */
    for(unsigned i=0;i<TBX_HOST_FILES;i++) free(S.files[i].data);
    memset(&S,0,sizeof S);
    tbx_host_arena(__executable_start,(size_t)(__bss_start-__executable_start),
                   (size_t)(_end-__executable_start));
    S.cw=613;S.ch=397;S.origin_h=6;S.origin_v=62;S.scrap_len=-1;S.write_budget=-1;
    S.clock=(cp_time_t){1992,3,9,12,0,0,0};S.free_bytes=TBX_ERR_DOS(15);
    S.memory[0]=8192;S.memory[1]=2048;S.memory[2]=6144;
    cp_defaults(&S.settings);
    for(int i=0;i<TBX_HOST_H;i++) { memset(S.scr[i],' ',TBX_HOST_W);memset(S.bg[i],' ',TBX_HOST_W); }
}
int32_t tbx_register(void) { R(0,0,0,0,0,0,0);if(S.exit_calls)return TBX_ERR_NOTREG;if(S.registrations)return TBX_ERR_BUSY;S.registrations++;return TBX_OK; }
int32_t tbx_new_window(int32_t l,int32_t t,int32_t r,int32_t b,int32_t g)
{
    R(l,t,r,b,g,0,0);
    LIVE();
    if(S.nwindows==TBX_TENANT_WINDOWS) return TBX_ERR_BUSY;
    uint32_t go=(uint32_t)g&255u,def=(uint32_t)g&~255u;
    if(l<0||r>640||t<20||b>480||(int64_t)r-l<96||(int64_t)b-t<64||
       go>2||(def&&def!=TBX_WDEF_RDOC))return TBX_ERR_BADARG;
    unsigned i=(unsigned)S.nwindows++;
    S.windows[i].l=l;S.windows[i].t=t;S.windows[i].r=r;S.windows[i].b=b;S.windows[i].goaway=g;
    return S.nwindows;
}
static int win(int32_t w) { return live() && w>0 && w<=S.nwindows; }
int32_t tbx_set_wtitle(int32_t w,const char *s)
{ R(w,0,0,0,0,0,s);if(!win(w)||string_len(s,64,1)<0)return TBX_ERR_BADARG;copy(S.windows[w-1].title,128,s);return TBX_OK; }
int32_t tbx_content_origin(int32_t w) { R(w,0,0,0,0,0,0);return win(w)?pack(S.origin_h,S.origin_v):TBX_ERR_BADARG; }
int32_t tbx_content_size(int32_t w) { R(w,0,0,0,0,0,0);return win(w)?pack(S.cw,S.ch):TBX_ERR_BADARG; }
int32_t tbx_draw_text(int32_t w,int32_t x,int32_t y,const char *s,uint32_t f,uint32_t b)
{ R(w,x,y,(int32_t)f,(int32_t)b,0,s);unsigned face=f>>8&255,font=f>>16&255,size=f>>24;
  return win(w)&&string_len(s,81,1)>=0&&(f&255)<4&&face<=7&&font<=1&&
    (!size||(size==12&&font==1))&&b<4?TBX_OK:TBX_ERR_BADARG; }
int32_t tbx_fill_rect(int32_t w,int32_t l,int32_t t,int32_t r,int32_t b,uint32_t c)
{ R(w,l,t,r,b,(int32_t)c,0);return win(w)&&c<4&&l<r&&t<b?TBX_OK:TBX_ERR_BADARG; }
int32_t tbx_draw_cells(int32_t w,int32_t x,int32_t y,const char *s,uint32_t f,uint32_t b)
{
    R(w,x,y,(int32_t)f,(int32_t)b,0,s);
    if(!win(w)||string_len(s,TBX_CELLS_MAX_RUN+1,1)<0||
       (f&255)>=TBX_COLOR_COUNT||(f>>16)||((f>>8)&~7u)||b>=TBX_COLOR_COUNT) return TBX_ERR_BADARG;
    /* Off-grid draws are valid; the cell recorder only represents its grid. */
    if(x<1||(x-1)%8||y<0||y%16||y/16>=TBX_HOST_H)return TBX_OK;
    int col=(x-1)/8,line=y/16;
    for(int i=0;s[i]&&col+i<TBX_HOST_W;i++) {
        int k=col+i,inv=f==TBX_COLOR_NAVY&&b==TBX_COLOR_WHITE;
        S.scr[line][k]=s[i];S.bg[line][k]=b==TBX_COLOR_NAVY?'N':' ';S.inv[line][k]=(char)inv;
        if(!inv) { S.face[line][k]=(uint8_t)(f>>8);S.col[line][k]=(uint8_t)(((f&255)<<4)|b); }
    }
    return TBX_OK;
}
int32_t tbx_set_mbar(const tbx_menubar_t *b) { R(0,0,0,0,0,0,0);LIVE();if(!menu_valid(b))return TBX_ERR_BADARG;S.bar=b;return TBX_OK; }
int32_t tbx_draw_menubar(void) { R(0,0,0,0,0,0,0);LIVE();S.menubar_draws++;return TBX_OK; }
int32_t tbx_exit(int32_t rc) { R(rc,0,0,0,0,0,0);LIVE();S.exit_calls++;S.exit_rc=rc;return TBX_OK; }
int32_t tbx_alert(const char *t,const char *d,const char *c,const char *o)
{ R(0,0,0,0,0,0,t);LIVE();if(S.alert_pending)return TBX_ERR_BUSY;if(!alert_valid(t,d,c,o))return TBX_ERR_BADARG;if(S.alert_rc)return S.alert_rc;S.alert_pending=1;S.alerts++;copy(S.alert_text,256,t);copy(S.alert_btn[0],32,d);copy(S.alert_btn[1],32,c);copy(S.alert_btn[2],32,o);return TBX_OK; }
int32_t tbx_clock(uint32_t mode,cp_time_t *t)
{ R((int32_t)mode,0,0,0,0,0,0);LIVE();S.clock_reads++;if(mode>1||!span(t,sizeof *t,0))return TBX_ERR_BADARG;if(S.clock_rc)return S.clock_rc;
  if(mode){unsigned y=t->year,m=t->month,d=t->day;static const unsigned days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(y<1980||y>2099||m<1||m>12||d<1||d>days[m-1]+(m==2&&!(y%4)))return TBX_ERR_BADARG;
    S.clock.year=t->year;S.clock.month=t->month;S.clock.day=t->day;
    if(t->hour>23||t->minute>59||t->second>59)return TBX_ERR_BADARG;
    S.clock=*t;
  }else *t=S.clock;return TBX_OK; }
int32_t tbx_get_clock(void *time) { return tbx_clock(0, time); }
int32_t tbx_settings(uint32_t mode,uint8_t rec[CP_RECORD_BYTES])
{ R((int32_t)mode,0,0,0,0,0,0);LIVE();if(!span(rec,CP_RECORD_BYTES,0)||mode>1)return TBX_ERR_BADARG;if(mode)return cp_decode(&S.settings,rec)?TBX_OK:TBX_ERR_BADARG;cp_encode(&S.settings,rec);return TBX_OK; }
int32_t tbx_memory(uint32_t out[4]) { R(0,0,0,0,0,0,0);LIVE();if(!span(out,16,0))return TBX_ERR_BADARG;memcpy(out,S.memory,sizeof S.memory);return TBX_OK; }
int32_t tbx_beep(void) { R(0,0,0,0,0,0,0);LIVE();S.beeps++;return TBX_OK; }
int32_t tbx_dialog(int32_t w,const cp_item_t *items,uint32_t n,uint32_t def,uint32_t cancel,const tbx_event_t *e)
{ R(w,(int32_t)n,(int32_t)def,(int32_t)cancel,0,0,0);
  if(!win(w)||!n||n>CP_MAX_ITEMS||def>n||cancel>n||!span(items,n*sizeof *items,1)||
    (e&&!span(e,sizeof *e,1)))return TBX_ERR_BADARG;
  /* CalcDocContentRect / CalcRDocContentRect, flair/window.c. */
  int rdoc=(S.windows[w-1].goaway&~255)==TBX_WDEF_RDOC;
  int dw=S.windows[w-1].r-S.windows[w-1].l-(rdoc?4:27);
  int dh=S.windows[w-1].b-S.windows[w-1].t-(rdoc?22:43);
  for(unsigned i=0;i<n;i++)if(items[i].type>3||items[i].left<0||items[i].top<0||
    items[i].right<=items[i].left||items[i].bottom<=items[i].top||items[i].right>dw||items[i].bottom>dh||
    string_len(items[i].text,64,1)<0)return TBX_ERR_BADARG;
  if(e&&e->what!=mouseDown&&e->what!=keyDown&&e->what!=autoKey)return TBX_ERR_BADARG;
  S.dialog_items=items;S.dialog_count=n;S.dialog_def=def;S.dialog_cancel=cancel;if(e)S.dialog_event=*e;return S.dialog_result; }
int32_t tbx_scrap_put(const void *b,uint32_t n)
{ R((int32_t)n,0,0,0,0,0,0);LIVE();if(n>sizeof S.scrap||!span(b,n,0))return TBX_ERR_BADARG;if(n)memcpy(S.scrap,b,n);S.scrap_len=(int)n;S.scrap_puts++;return TBX_OK; }
int32_t tbx_scrap_get(void *b,uint32_t n)
{ R((int32_t)n,0,0,0,0,0,0);LIVE();if(S.scrap_len<0)return TBX_ERR_NOTYPE;if(!b)return S.scrap_len;if(n<(unsigned)S.scrap_len||!span(b,(unsigned)S.scrap_len,0))return TBX_ERR_BADARG;memcpy(b,S.scrap,(unsigned)S.scrap_len);return S.scrap_len; }
int32_t tbx_sf_getfile(char *r) { R(0,0,0,0,0,0,0);LIVE();if(S.sf||S.alert_pending)return TBX_ERR_BUSY;if(!span(r,128,0))return TBX_ERR_BADARG;S.sf=1;S.sf_reply=r;return TBX_OK; }
int32_t tbx_sf_putfile(const char *p,const char *n,char *r) { R(0,0,0,0,0,0,p);LIVE();if(S.sf||S.alert_pending)return TBX_ERR_BUSY;if(!span(r,128,0)||string_len(p,40,1)<1||string_len(n,13,1)<0)return TBX_ERR_BADARG;S.sf=2;S.sf_reply=r;copy(S.sf_prompt,64,p);copy(S.sf_name,128,n);return TBX_OK; }
int32_t tbx_track_mouse(void) { R(0,0,0,0,0,0,0);LIVE();if(!S.in_mousedown)return TBX_ERR_BADARG;S.track=1;return TBX_OK; }
int32_t tbx_set_scroll(int32_t w,int32_t len,int32_t val)
{ R(w,len,val,0,0,0,0);if(!win(w)||(uint32_t)len>32767||(uint32_t)val>32767)return TBX_ERR_BADARG;S.setscroll++;int max=len>S.ch?len-S.ch:0;if(val>max)val=max;S.scroll_len=len;S.scroll_val=val;S.windows[w-1].scroll_len=len;S.windows[w-1].scroll_val=val;return val; }
int32_t tbx_get_scroll(int32_t w) { R(w,0,0,0,0,0,0);return win(w)?S.scroll_val:TBX_ERR_BADARG; }
int32_t tbx_global_to_local(int32_t w,int32_t h,int32_t v)
{ R(w,h,v,0,0,0,0);return win(w)&&h>=-32768&&h<=32767&&v>=-32768&&v<=32767?pack(h-S.origin_h,v-S.origin_v):TBX_ERR_BADARG; }
int32_t tbx_file_copy(const char *c,const char *s,const char *d)
{ R(0,0,0,0,0,0,c);LIVE();if(S.alert_pending||S.copy_pending)return TBX_ERR_BUSY;int n=string_len(c,48,1);
  if(n<1||width(c,(unsigned)n)>332||strchr(c,'\r')||string_len(s,128,1)<1||string_len(d,128,1)<1)return TBX_ERR_BADARG;
  S.copy_pending=1;copy(S.copy_caption,128,c);copy(S.copy_src,128,s);copy(S.copy_dst,128,d);return TBX_OK; }

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
    LIVE();if(!path_valid(p)||device(p))return TBX_ERR_BADARG;
    if(volume_error(p))return volume_error(p);
    int32_t err=error(exclusive?TBX_FILE_CREATE_NEW:TBX_FILE_CREATE);if(err)return err;
    if(S.fail_create)return TBX_ERR_DOS(5);
    int i=find(p);if(i>=0&&exclusive)return TBX_ERR_DOS(80);
    /* No truncation or allocation when handles are exhausted. */
    int h;for(h=0;h<TBX_HOST_HANDLES&&S.handles[h].used;h++);if(h==TBX_HOST_HANDLES)return TBX_ERR_DOS(4);
    if(i>=0&&(S.files[i].attr&0x19))return TBX_ERR_DOS(5);
    if(i<0)i=allocfile(p);
    if(i<0)return TBX_ERR_DOS(5);
    S.files[i].size=0;return openfile(i,TBX_FILE_READ_WRITE);
}
int32_t tbx_file_create(const char *p) { R(0,0,0,0,0,0,p);return create(p,0); }
int32_t tbx_file_create_new(const char *p) { R(0,0,0,0,0,0,p);return create(p,1); }
int32_t tbx_file_open(const char *p,uint32_t mode)
{ R((int32_t)mode,0,0,0,0,0,p);LIVE();if(!path_valid(p)||mode>2)return TBX_ERR_BADARG;if(volume_error(p))return volume_error(p);int32_t e=error(TBX_FILE_OPEN);if(e)return e;if(device(p))return TBX_ERR_BADARG;int i=find(p);if(i<0)return TBX_ERR_DOS(2);if((S.files[i].attr&0x18)||(mode&&(S.files[i].attr&1)))return TBX_ERR_DOS(5);return openfile(i,mode); }
int32_t tbx_file_read(int32_t h,void *b,uint32_t n)
{
    R(h,(int32_t)n,0,0,0,0,0);LIVE();
    if(!handle(h)||n>INT32_MAX||!span(b,n,0))return TBX_ERR_BADARG;
    unsigned k=(unsigned)(h-5);if(S.handles[k].mode==TBX_FILE_WRITE_ONLY)return TBX_ERR_BADARG;
    int32_t e=error(TBX_FILE_READ);if(e)return e;
    tbx_host_file_t *f=&S.files[S.handles[k].file];uint32_t pos=S.handles[k].pos;
    uint32_t avail=pos<f->size?f->size-pos:0;if(n>avail)n=avail;
    if(S.read_limit&&n>S.read_limit)n=S.read_limit;
    if(n)memcpy(b,f->data+pos,n);
    S.handles[k].pos+=n;return (int32_t)n;
}
int32_t tbx_file_write(int32_t h,const void *b,uint32_t n)
{
    R(h,(int32_t)n,0,0,0,0,0);LIVE();
    if(!handle(h)||n>INT32_MAX||!span(b,n,0))return TBX_ERR_BADARG;
    unsigned k=(unsigned)(h-5);if(S.handles[k].mode==TBX_FILE_READ_ONLY)return TBX_ERR_BADARG;
    int32_t e=error(TBX_FILE_WRITE);if(e)return e;
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
    R(h,d,(int32_t)o,0,0,0,0);LIVE();int32_t e=error(TBX_FILE_SEEK);if(e)return e;
    if(!handle(h)||o>2)return TBX_ERR_BADARG;
    unsigned k=(unsigned)(h-5);int64_t base=o==0?0:o==1?S.handles[k].pos:S.files[S.handles[k].file].size;
    int64_t p=base+d;if(p<0||p>INT32_MAX)return TBX_ERR_BADARG;S.handles[k].pos=(uint32_t)p;return (int32_t)p;
}
int32_t tbx_file_close(int32_t h)
{ R(h,0,0,0,0,0,0);LIVE();if(!handle(h))return TBX_ERR_BADARG;S.handles[h-5].used=0;if(S.fail_close&&(!S.fail_close_handle||h==S.fail_close_handle)){S.fail_close=0;return TBX_ERR_DOS(5);}return error(TBX_FILE_CLOSE); }
int32_t tbx_file_delete(const char *p)
{ R(0,0,0,0,0,0,p);LIVE();if(!path_valid(p))return TBX_ERR_BADARG;if(volume_error(p))return volume_error(p);int32_t e=error(TBX_FILE_DELETE);if(e)return e;int i=find(p);if(i<0)return TBX_ERR_DOS(2);if(S.files[i].attr&0x19)return TBX_ERR_DOS(5);if(!remember_deleted(S.files[i].name))return TBX_ERR_DOS(5);S.files[i].used=0;return TBX_OK; }
int32_t tbx_file_rename(const char *p,const char *q)
{ R(0,0,0,0,0,0,p);LIVE();if(!path_valid(p)||!path_valid(q))return TBX_ERR_BADARG;
  if(volume_error(p)||volume_error(q))return TBX_ERR_DOS(15);
  int32_t e=error(TBX_FILE_RENAME);if(e)return e;char name[128],old[128];if(!norm(q,name)||!norm(p,old))return TBX_ERR_BADARG;
  char *a=strrchr(old,'\\'),*b=strrchr(name,'\\');size_t an=a?(size_t)(a-old):0,bn=b?(size_t)(b-name):0;
  if(an!=bn||memcmp(old,name,an))return TBX_ERR_DOS(17);
  if(find(q)>=0)return TBX_ERR_DOS(5);
  int i=find(p);if(i<0)return TBX_ERR_DOS(2);if(S.files[i].attr&0x19)return TBX_ERR_DOS(5);if(!remember_deleted(S.files[i].name))return TBX_ERR_DOS(5);copy(S.files[i].name,sizeof S.files[i].name,name);return TBX_OK; }
static int match(const char *p,const char *s)
{
    /* Genuine DOS 3.30 search33 captures: A0.* includes extensionless A0.
     * Keep the existing host glob subset; a terminal .* may match no extension. */
    if(!strcmp(p,"*.*"))return !strchr(s,'\\');
    if(!*s&&!strcmp(p,".*"))return 1;
    if(!*p)return !*s;
    if(*p=='*')return match(p+1,s)||(*s&&*s!='\\'&&match(p,s+1));
    if(*p=='?')return *s&&*s!='\\'&&match(p+1,s+1);
    return *p==*s&&match(p+1,s+1);
}
int32_t tbx_file_find(const char *p,uint32_t attr,void *buf,uint32_t max)
{
    R((int32_t)attr,(int32_t)max,0,0,0,0,p);LIVE();S.find_calls++;
    int32_t e=error(TBX_FILE_FIND);if(e)return e;
    char pat[TBX_FILE_PATH_MAX];if(!path_valid(p)||!norm(p,pat)||attr>255||max>TBX_FIND_MAX_RECS||!span(buf,max*TBX_FIND_REC_BYTES,0))return TBX_ERR_BADARG;
    if(volume_error(p))return volume_error(p);
    uint32_t n=0;for(int i=0;i<TBX_HOST_FILES&&n<max;i++) {
        tbx_host_file_t *f=&S.files[i];
        if(!f->used||!match(pat,f->name)||((f->attr&0x1e)&~attr)||
           ((attr&0x1e)==8&&!(f->attr&8)))continue;
        const char *name=strrchr(f->name,'\\');name=name?name+1:f->name;
        tbx_find_rec_t rec={.attr=f->attr,.time=f->time,.date=f->date,.size=f->size};copy(rec.name,sizeof rec.name,name);
        memcpy((uint8_t *)buf+n*TBX_FIND_REC_BYTES,&rec,sizeof rec);n++;
    }
    return (int32_t)n;
}
int32_t tbx_disk_free(void) { R(0,0,0,0,0,0,0);LIVE();int32_t e=error(TBX_DISK_FREE);return e?e:S.free_bytes; }
int tbx_host_push_event(const tbx_event_t *e)
{ if(!e||S.event_count==TBX_HOST_EVENTS)return 0;S.events[(S.event_head+S.event_count)%TBX_HOST_EVENTS]=*e;S.event_count++;return 1; }
unsigned tbx_host_dispatch(void (*callback)(const tbx_event_t *))
{ unsigned n=0;while(S.event_count&&!S.exit_calls){tbx_event_t e=S.events[S.event_head];S.event_head=(S.event_head+1)%TBX_HOST_EVENTS;S.event_count--;
  if(e.what==TBX_EVT_ALERT)S.alert_pending=0;
  if(e.what==TBX_EVT_SFREPLY)S.sf=0;
  if(e.what==TBX_EVT_FILECOPY)S.copy_pending=0;
  S.in_mousedown=e.what==mouseDown;callback(&e);S.in_mousedown=0;n++;}return n; }
/* Raw calls keep the i386 uint32 pointer convention. Native 64-bit fixtures
 * use typed bindings; hosted i386 can exercise the same raw transports. */
TBX_TRAP_ABI int32_t tbx_trap(uint32_t c,uint32_t n,const uint32_t *a)
{
    R((int32_t)c,(int32_t)n,0,0,0,0,0);
#define ARGS(k) do { if(n<(k)||!a)return TBX_ERR_BADARG; } while(0)
#define PTR(t,i) ((t *)(uintptr_t)a[i])
    switch(c&65535u) {
    case TBX_REGISTER: {
        ARGS(1);if(S.exit_calls)return TBX_ERR_NOTREG;if(S.registrations)return TBX_ERR_BUSY;
        const tbx_rec_t *r=PTR(tbx_rec_t,0);
        if(!span(r,sizeof *r,1)||r->tag!=FLAIR_TENANT_REC_TAG||
           (uintptr_t)r->image_base!=S.image_base||(uintptr_t)r->image_len!=S.image_end-S.image_base||
           string_len(r->name,FLAIR_TENANT_NAME_MAX,1)<=0||!span(r->evbuf,sizeof *r->evbuf,1)||
           !span((const void *)(uintptr_t)r->proc,1,1))return TBX_ERR_BADREC;
        return tbx_register();
    }
    case TBX_NEWWINDOW: ARGS(5);return tbx_new_window(a[0],a[1],a[2],a[3],a[4]);
    case TBX_SETWTITLE: ARGS(2);return tbx_set_wtitle(a[0],PTR(char,1));
    case TBX_CONTENTORIGIN: ARGS(1);return tbx_content_origin(a[0]);
    case TBX_CONTENT_SIZE: ARGS(1);return tbx_content_size(a[0]);
    case TBX_TEXTDRAW: ARGS(6);return tbx_draw_text(a[0],a[1],a[2],PTR(char,3),a[4],a[5]);
    case TBX_FILLRECT: ARGS(6);return tbx_fill_rect(a[0],a[1],a[2],a[3],a[4],a[5]);
    case TBX_DRAWCELLS: ARGS(6);return tbx_draw_cells(a[0],a[1],a[2],PTR(char,3),a[4],a[5]);
    case TBX_SETMBAR: ARGS(1);return tbx_set_mbar(PTR(tbx_menubar_t,0));
    case TBX_DRAWMENUBAR:return tbx_draw_menubar();
    case TBX_EXIT: ARGS(1);return tbx_exit(a[0]);
    case TBX_FILE_CREATE: ARGS(1);return tbx_file_create(PTR(char,0));
    case TBX_FILE_CREATE_NEW: ARGS(1);return tbx_file_create_new(PTR(char,0));
    case TBX_FILE_OPEN: ARGS(2);return tbx_file_open(PTR(char,0),a[1]);
    case TBX_FILE_READ: ARGS(3);return tbx_file_read(a[0],PTR(void,1),a[2]);
    case TBX_FILE_WRITE: ARGS(3);return tbx_file_write(a[0],PTR(void,1),a[2]);
    case TBX_FILE_SEEK: ARGS(3);return tbx_file_seek(a[0],a[1],a[2]);
    case TBX_FILE_CLOSE: ARGS(1);return tbx_file_close(a[0]);
    case TBX_FILE_DELETE: ARGS(1);return tbx_file_delete(PTR(char,0));
    case TBX_FILE_RENAME: ARGS(2);return tbx_file_rename(PTR(char,0),PTR(char,1));
    case TBX_FILE_FIND: ARGS(4);return tbx_file_find(PTR(char,0),a[1],PTR(void,2),a[3]);
    case TBX_DISK_FREE:return tbx_disk_free();
    case TBX_ALERT: ARGS(4);return tbx_alert(PTR(char,0),PTR(char,1),PTR(char,2),PTR(char,3));
    case TBX_FILECOPY: ARGS(3);return tbx_file_copy(PTR(char,0),PTR(char,1),PTR(char,2));
    case TBX_DIALOG: ARGS(6);return tbx_dialog(a[0],PTR(cp_item_t,1),a[2],a[3],a[4],PTR(tbx_event_t,5));
    case TBX_SETTINGS: ARGS(2);return tbx_settings(a[0],PTR(uint8_t,1));
    case TBX_CLOCK: ARGS(2);return tbx_clock(a[0],PTR(cp_time_t,1));
    case TBX_MEMORY: ARGS(1);return tbx_memory(PTR(uint32_t,0));
    case TBX_BEEP:return tbx_beep();
    case TBX_SCRAP_PUT: ARGS(2);return tbx_scrap_put(PTR(void,0),a[1]);
    case TBX_SCRAP_GET: ARGS(2);return tbx_scrap_get(PTR(void,0),a[1]);
    case TBX_SFGETFILE: ARGS(1);return tbx_sf_getfile(PTR(char,0));
    case TBX_SFPUTFILE: ARGS(3);return tbx_sf_putfile(PTR(char,0),PTR(char,1),PTR(char,2));
    case TBX_TRACKMOUSE:return tbx_track_mouse();
    case TBX_SETSCROLL: ARGS(3);return tbx_set_scroll(a[0],a[1],a[2]);
    case TBX_GETSCROLL: ARGS(1);return tbx_get_scroll(a[0]);
    case TBX_GLOBALTOLOCAL: ARGS(3);return tbx_global_to_local(a[0],a[1],a[2]);
    default:return TBX_ERR_BADCODE;
    }
#undef PTR
#undef ARGS
}

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
