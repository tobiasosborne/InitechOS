/* Native flat .COM adapter. Ref: ADR-0003 App A (19h,3Fh,40h,47h,4Ch);
 * DEC-15 / spec/absdisk_int2526.json (25h/26h sectors), DEC-04a flat ABI.
 * The caller's actual loader-bound MCB chain is at roundup(__load_end +
 * PROGRAM_BSS_RESERVE), per loader.c load_plan, not a fabricated DOS constant.
 */
#include "chkdsk.h"
#include "memory_map.h"
extern char __load_end[];
static chkdsk_io_t io;
static int disk_read(void*ctx,uint32_t l,uint32_t n,void*p) {
    (void)ctx;uint32_t a=io.drive;uint8_t cf;
    __asm__ volatile("int $0x25; setc %0":"=qm"(cf),"+a"(a):"b"(p),"c"(n),"d"(l):"memory","cc");
    return cf?-1:0;
}
static int disk_write(void*ctx,uint32_t l,uint32_t n,const void*p) {
    (void)ctx;uint32_t a=io.drive;uint8_t cf;
    __asm__ volatile("int $0x26; setc %0":"=qm"(cf),"+a"(a):"b"(p),"c"(n),"d"(l):"memory","cc");
    return cf?-1:0;
}
static void put(void*ctx,const char*p) {
    (void)ctx;uint32_t n=0;while(p[n])n++;
    uint32_t a=0x4000;
    __asm__ volatile("int $0x21":"+a"(a):"b"(1),"c"(n),"d"(p):"memory","cc");
}
static int get(void*ctx) {
    (void)ctx;char c;uint32_t a=0x3f00;uint8_t cf;
    __asm__ volatile("int $0x21; setc %0":"=qm"(cf),"+a"(a):"b"(0),"c"(1),"d"(&c):"memory","cc");
    return cf||!a?-1:(unsigned char)c;
}
int chkdsk_entry(void) {
    char tail[128],cwd[128];uint32_t a=0x1900;
    volatile uint8_t *psp=(volatile uint8_t*)PROGRAM_BASE;
    unsigned n=psp[0x80];if(n>126)n=126;for(unsigned i=0;i<n;i++)tail[i]=(char)psp[0x81+i];tail[n]=0;
    __asm__ volatile("int $0x21":"+a"(a)::"memory","cc");
    io.drive=(uint8_t)a;
    cwd[0]='\\';cwd[1]=0;a=0x4700;uint32_t d=0;
    __asm__ volatile("int $0x21":"+a"(a),"+d"(d):"S"(cwd+1):"memory","cc");
    io.cwd=cwd;io.disk.read_sectors=disk_read;io.disk.write_sectors=disk_write;io.put=put;io.get=get;
    uintptr_t base=((uintptr_t)__load_end+PROGRAM_BSS_RESERVE+15u)&~(uintptr_t)15u;
    io.arena.base=(uint8_t*)base;io.arena.total_paras=(PROGRAM_ARENA_CEIL-base)/16;
    return chkdsk_run(&io,tail);
}
