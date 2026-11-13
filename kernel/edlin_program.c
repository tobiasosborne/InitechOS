/* DOS adapter. Ref: ADR-0003 Appendix A, DEC-04a flat ABI, DEC-08;
 * spec/memory_map.h PSP:80h command tail. No host runtime in the artifact. */
#include "edlin.h"
#include "memory_map.h"
#include <stdint.h>
static int call(unsigned a,unsigned b,unsigned c,const void *d,const void *i){
    unsigned char cf;
    __asm__ volatile("int $0x21; setc %0":"=qm"(cf),"+a"(a):"b"(b),"c"(c),"d"(d),"D"(i):"memory","cc");
    return cf?-(int)(a&65535):(int)a;
}
static int get(void){return call(0x0700,0,0,0,0)&255;}
static int openfile(const char *p){return call(0x3d00,0,0,p,0);}
static int create(const char *p){return call(0x3c00,0,0,p,0);}
static int readfile(int h,void *p,unsigned n){return call(0x3f00,(unsigned)h,n,p,0);}
static int writefile(int h,const void *p,unsigned n){return call(0x4000,(unsigned)h,n,p,0);}
static int closefile(int h){return call(0x3e00,(unsigned)h,0,0,0);}
static int removefile(const char *p){return call(0x4100,0,0,p,0);}
static int renamefile(const char *a,const char *b){return call(0x5600,0,0,a,b);}
static int attr(const char *p){unsigned a=0x4300,c=0;unsigned char cf;
    __asm__ volatile("int $0x21; setc %0":"=qm"(cf),"+a"(a),"+c"(c):"d"(p):"memory","cc");
    return cf?-(int)(a&65535):(int)c;
}
int edlin_entry(void){
    static const edlin_io io={get,openfile,create,readfile,writefile,closefile,removefile,renamefile,attr};
    char tail[128];volatile unsigned char *psp=(volatile unsigned char*)PROGRAM_BASE;
    unsigned n=psp[0x80];if(n>126)n=126;
    for(unsigned i=0;i<n;i++)tail[i]=(char)psp[0x81+i];
    tail[n]=0;
    return edlin_run(&io,tail);
}
