/* Independent manual vectors drive the real INT 21h editor, not a model of it.
 * Ref: ADR-0003 Appendix A; MS-DOS 3.3 User's Guide printed pp.154-159,
 * 165-169 (PDF239-244,250-254). No keyboard oracle exists in mtools;
 * conkeys.sh compares the emulator's saved lines on identical FAT images.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "int21.h"
static const unsigned char *keys;
static unsigned left, gets;
static int poll_input;
static char echo[4096],printer[4096],trace[4096];
static unsigned ne,np,nt;
static int failures;
static int get(void) { if(!left){fprintf(stderr,"RED CONKEYS input-underflow\n");exit(2);} gets++;left--;return *keys++; }
static int poll(void) { return poll_input&&left ? (left--,*keys++) : -1; }
static void sink(char c) { if(ne<sizeof echo-1)echo[ne++]=c;echo[ne]=0; }
static void debug(char c) { if(nt<sizeof trace-1)trace[nt++]=c;trace[nt]=0; }
static int prn(const uint8_t *s,int n,void *ctx) { (void)ctx;for(int i=0;i<n;i++)printer[np++]=(char)s[i];printer[np]=0;return n; }
static void check(int ok,const char *name) { if(!ok){fprintf(stderr,"RED CONKEYS %s\n",name);failures++;} }
static uint8_t *buf;
static void vector(const char *name,const char *tmpl,const unsigned char *input,unsigned size,const char *want,const char *display) {
    memset(buf,0,260);buf[0]=255;buf[1]=(uint8_t)strlen(tmpl);memcpy(buf+2,tmpl,strlen(tmpl));
    keys=input;left=size;ne=np=nt=gets=0;echo[0]=printer[0]=trace[0]=0;
    int_frame_t f={0};f.eax=0x0a00;f.edx=(uint32_t)(uintptr_t)buf;int21_dispatch(&f);
    check(buf[1]==strlen(want)&&memcmp(buf+2,want,strlen(want))==0&&buf[2+strlen(want)]==13,name);
    check(!left,"all keys consumed");
    if(display)check(strcmp(echo,display)==0,"display transcript");
}
#define V(name,t,k,w,e) vector(name,t,(const unsigned char *)(k),sizeof(k)-1,w,e)
int main(void) {
    buf=mmap(0,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    if(buf==MAP_FAILED || (uintptr_t)buf>UINT32_MAX)return 2;
    int21_set_conin(get,poll);int21_set_sink(sink);int21_set_break_trace(debug);
    devices_io_t io={0};io.prn_write=prn;devices_init();int21_set_devices(devices_head(),&io);
    V("F1","Sharpe Office Supplies.","\0\073\0\073\r","Sh","Sh\r\n");
    V("F2 excludes target","dir invest.mnt","\0\074mrpt\r","dir invest.rpt","dir invest.rpt\r\n");
    V("F2 missing target","abc","\0\074x\0\075\r","abc",0);
    V("F3 rest","Sharpe Office Supplies.","\0\075\r","Sharpe Office Supplies.",0);
    V("F4 excludes target","Sharpe Office Supplies.","\0\076c\0\075\r","ce Supplies.",0);
    V("F4 missing target","abc","\0\076x\0\075\r","abc",0);
    V("Del skips template","Sharpe Office Supplies.","\0\123\0\075\r","harpe Office Supplies.",0);
    V("Ins toggles","dir invest.rpt","type\0\122 \0\122\0\075\r","type invest.rpt",0);
    V("manual correction","pyte invest.rpt","\0\123\0\123\0\073\0\122yp\0\075\r","type invest.rpt",0);
    V("Esc retains template","original","bad\033\0\075\r","original","bad\\\r\noriginal\r\n");
    V("F5 replaces template","old","new\0\077\0\075\r","new","new@\r\nnew\r\n");
    V("F1 ends insert","abcd","\0\122x\0\073y\0\075\r","xaycd",0);
    V("F2 ends insert","abcd","\0\122x\0\074cy\0\075\r","xabyd",0);
    V("F3 ends insert","ab","\0\122x\0\075y\r","xaby",0);
    V("Esc ends insert","abcd","\0\122x\033y\0\075\r","ybcd",0);
    V("F5 ends insert","old","\0\122new\0\077x\0\075\r","xew",0);
    V("Backspace rewinds template","abc","\0\073\b\0\075\r","abc",0);
    V("Ctrl-S consumes resume","","a\023zb\r","ab","ab\r\n");
    check(strstr(trace,"CONKEYS-PAUSE")&&strstr(trace,"CONKEYS-RESUME"),"Ctrl-S pause/resume debug seam");
    V("Ctrl-P/Ctrl-N","","\020p\016n\r","pn","pn\r\n");
    check(strcmp(printer,"p")==0,"Ctrl-P echoes PRN; Ctrl-N ends echo");
    V("Ctrl-P toggles twice","","\020p\020n\r","pn",0);
    check(strcmp(printer,"p")==0,"Ctrl-P toggle off");
    /* Display output, not just input echo: pause must read the resume key
     * before the pending character reaches the sink. Normal typeahead stays
     * available to the next input call. */
    static const unsigned char flow[]={19,'z'};
    keys=flow;left=sizeof flow;ne=np=nt=gets=0;poll_input=1;
    int_frame_t f={0};f.eax=0x0200;f.edx='!';int21_dispatch(&f);
    check(gets==1&&left==0&&strcmp(echo,"!")==0,"Ctrl-S output blocks until resume");
    static const unsigned char typeahead[]={'k'};
    keys=typeahead;left=1;f.eax=0x0200;f.edx='?';int21_dispatch(&f);
    f.eax=0x0700;int21_dispatch(&f);
    check((f.eax&255)=='k',"output preserves ordinary typeahead");
    static const unsigned char on[]={16};
    keys=on;left=1;f.eax=0x0200;f.edx='P';int21_dispatch(&f);
    static const unsigned char off[]={14};
    keys=off;left=1;f.eax=0x0200;f.edx='N';int21_dispatch(&f);
    check(strcmp(printer,"P")==0,"Ctrl-P output polling routes PRN; Ctrl-N disables");
    poll_input=0;
    /* DOS maximum-size CR and recall bounds: full template cannot overrun. */
    char full[255];memset(full,'x',254);full[254]=0;
    V("F3 maximum",full,"\0\075\r",full,0);
    printf("%s CONKEYS host manual vectors (%d failures)\n",failures?"RED":"GREEN",failures);
    return failures?1:0;
}
