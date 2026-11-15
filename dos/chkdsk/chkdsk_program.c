/* Native flat .COM adapter. Ref: ADR-0003 App A (19h,3Fh,40h,47h,4Ch);
 * DEC-15 / spec/absdisk_int2526.json (25h/26h sectors), DEC-04a flat ABI.
 * Memory figures come from the kernel's OEM INT 21h AH=F9h arena summary
 * (kernel/README.md, kernel/include/dos_oem.h); CHKDSK no longer walks
 * the loader-bound MCB chain itself (initech-q9p80).
 */
#include "chkdsk.h"
#include "memory_map.h"
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
/* OEM AH=F9h AL=0 (kernel/README.md): CF=0 -> EAX total data bytes
 * (MCB headers excluded), EBX free data bytes. CF=1/AX=7 -> no arena. */
static int memory(uint32_t *total,uint32_t *free_bytes) {
    uint32_t a=0xf900,b=0; /* AH=F9h AL=0: DOS_OEM_ARENA in kernel/include/dos_oem.h,
     * unpublished in kernel/README.md interface records as of 2026-11-15;
     * include the header once the kernel owner exports it (initech-q9p80). */uint8_t cf;
    __asm__ volatile("int $0x21; setc %0":"=qm"(cf),"+a"(a),"+b"(b)::"memory","cc");
    if(cf)return -1;
    *total=a;*free_bytes=b;
    return 0;
}
/* AH=47h DL=drive+1 (IBM DOS 3.30 TR p6-188): the named drive's CWD. */
static int cwd_of(unsigned drive,char out[64]) {
    uint32_t a=0x4700,d=drive+1;uint8_t cf;out[0]=0;
    __asm__ volatile("int $0x21; setc %0":"=qm"(cf),"+a"(a),"+d"(d):"S"(out):"memory","cc");
    return cf?-1:0;
}
int chkdsk_entry(void) {
    char tail[128];uint32_t a=0x1900;
    volatile uint8_t *psp=(volatile uint8_t*)PROGRAM_BASE;
    unsigned n=psp[0x80];if(n>126)n=126;for(unsigned i=0;i<n;i++)tail[i]=(char)psp[0x81+i];tail[n]=0;
    __asm__ volatile("int $0x21":"+a"(a)::"memory","cc");
    io.drive=(uint8_t)a;
    io.cwd=cwd_of;io.disk.read_sectors=disk_read;io.disk.write_sectors=disk_write;io.put=put;io.get=get;
    io.memory=memory;
    return chkdsk_run(&io,tail);
}
