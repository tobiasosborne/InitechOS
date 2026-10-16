/* Freestanding flat COM adapter. Ref: ADR-0003 Appendix A, DEC-04a/08/15;
 * spec/find_data.h; spec/memory_map.h PROGRAM_IMAGE=0x40100. */
#include "dosutil.h"
#include "find_data.h"
static find_data_t dta;
static unsigned len(const char *s) { unsigned n=0; while (s[n]) n++; return n; }
static int call21(unsigned ax, unsigned bx, unsigned cx, unsigned dx)
{
    unsigned char cf;
    __asm__ volatile("int $0x21" : "=@ccc"(cf), "+a"(ax),
        "+b"(bx), "+c"(cx), "+d"(dx) : : "memory","cc");
    return cf ? -(int)(ax&65535u) : (int)ax;
}
int du_drive(void) { return call21(0x1900,0,0,0)&255; }
int du_cwd(char out[64])
{
    unsigned ax=0x4700, dx=0, si=(unsigned)out;
    unsigned char cf;
    __asm__ volatile("int $0x21" : "=@ccc"(cf), "+a"(ax),
        "+d"(dx), "+S"(si) : : "memory","cc");
    return cf ? -1 : 0;
}
int du_list(const char *dir, unsigned index, du_entry *out)
{
    char spec[80];
    unsigned n=len(dir);
    if (n+5>=sizeof(spec)) return -1;
    for (unsigned i=0;i<n;i++) spec[i]=dir[i];
    if (!n || spec[n-1]!='\\') spec[n++]='\\';
    spec[n++]='*'; spec[n++]='.'; spec[n++]='*'; spec[n]=0;
    call21(0x1a00,0,0,(unsigned)&dta);
    int rc=call21(0x4e00,0,0x16,(unsigned)spec);
    while (rc>=0 && index--) rc=call21(0x4f00,0,0,0);
    if (rc<0) return rc==-18 || rc==-2 ? 0 : -1;
    for (unsigned i=0;i<13;i++) out->name[i]=dta.fname[i];
    out->attr=dta.attr;
    return 1;
}
int du_attr(const char *path, int set, uint8_t *attr)
{
    unsigned ax=0x4300u+(set!=0), cx=*attr, dx=(unsigned)path;
    unsigned char cf;
    __asm__ volatile("int $0x21" : "=@ccc"(cf), "+a"(ax),
        "+c"(cx), "+d"(dx) : : "memory","cc");
    if (!set) *attr=(uint8_t)cx;
    return cf ? -1 : 0;
}
int du_sector(int write, unsigned drive, unsigned lba, uint8_t buf[512])
{
    unsigned ax=drive,bx=(unsigned)buf,cx=1,dx=lba;
    unsigned char cf;
    if (write) __asm__ volatile("int $0x26" : "=@ccc"(cf),
        "+a"(ax), "+b"(bx), "+c"(cx), "+d"(dx) : : "memory","cc");
    else __asm__ volatile("int $0x25" : "=@ccc"(cf),
        "+a"(ax), "+b"(bx), "+c"(cx), "+d"(dx) : : "memory","cc");
    return cf ? -1 : 0;
}
void du_puts(const char *s) { call21(0x4000,1,len(s),(unsigned)s); }
int du_line(char out[128])
{
    /* Handle reads honor redirected stdin too. CON's line discipline supplies
     * echoed input; redirected files deliver literal CR/LF bytes. */
    int n=0;
    char c;
    for (;;) {
        int rc=call21(0x3f00,0,1,(unsigned)&c);
        if (rc<=0) return -1;
        if (c=='\r') continue;
        if (c=='\n') break;
        if (n<127) out[n++]=c;
    }
    out[n]=0; return n;
}
extern unsigned char __bss_start[], __bss_end[];
__attribute__((section(".start"),noreturn)) void _start(void)
{
    for (unsigned char *p=__bss_start;p<__bss_end;p++) *p=0;
    char tail[128];
    unsigned n=*(volatile unsigned char *)0x40080;
    if (n>127) n=127;
    for (unsigned i=0;i<n;i++) tail[i]=((volatile char *)0x40081)[i];
    tail[n]=0;
#ifdef DOSUTIL_ATTRIB
    int rc=attrib_main(tail);
#else
    int rc=label_main(tail);
#endif
    call21(0x4c00u|(unsigned)rc,0,0,0);
    for (;;) __asm__ volatile("hlt");
}
