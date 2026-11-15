/* Independent reader of the DOS 3.30 PSP environment and command tail. */
static unsigned char evidence[4352] = {1};
static const char name[] = "RAWPSP.BIN";
void _start(void)
{
    unsigned ax=0x6200, bx=0;
    __asm__ volatile("int $0x21" : "+a"(ax), "+b"(bx) : : "memory", "cc");
    const volatile unsigned char *psp=(void *)((bx & 65535)<<4);
    unsigned env=((unsigned)psp[0x2c] | (unsigned)psp[0x2d]<<8)<<4;
    for (unsigned i=0;i<256;i++) evidence[i]=psp[i];
    for (unsigned i=0;i<4096;i++) evidence[256+i]=((volatile unsigned char *)env)[i];
    ax=0x3c00; unsigned dx=(unsigned)name, cx=0, cf;
    __asm__ volatile("int $0x21; sbb %3,%3" : "+a"(ax), "+d"(dx), "+c"(cx), "=r"(cf) : : "memory", "cc");
    unsigned bad=cf; bx=ax;
    if (!bad) {
        ax=0x4000; dx=(unsigned)evidence; cx=sizeof evidence;
        __asm__ volatile("int $0x21; sbb %4,%4" : "+a"(ax), "+b"(bx), "+d"(dx), "+c"(cx), "=r"(cf) : : "memory", "cc");
        bad=cf || ax!=sizeof evidence; ax=0x3e00;
        __asm__ volatile("int $0x21; sbb %2,%2" : "+a"(ax), "+b"(bx), "=r"(cf) : : "memory", "cc");
        bad |= cf;
    }
    ax=0x4c00 | !!bad;
    __asm__ volatile("int $0x21" : "+a"(ax) : : "memory", "cc");
    for (;;) {}
}
