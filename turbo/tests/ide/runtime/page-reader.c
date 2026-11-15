/* Independent DOS AH=FBh reader. It never paints or uses Turbo's runtime.
 * Packet: 4000 cell bytes, cursor row, cursor column (dos_oem.h). */
static unsigned char packet[4003] = {1};
static const char name[] = "PAGE.BIN";
static void stop(unsigned code)
{
    unsigned ax = 0x4c00 | code;
    __asm__ volatile("int $0x21" : "+a"(ax) : : "memory", "cc");
    for (;;) {}
}
void _start(void)
{
    unsigned ax=0xfb00, dx=(unsigned)packet, cf;
    __asm__ volatile("int $0x21; sbb %2,%2" : "+a"(ax), "+d"(dx), "=r"(cf) : : "memory", "cc");
    if (cf & 255) stop(1);
    unsigned row=packet[4000], col=packet[4001];
    packet[4000]=(unsigned char)col; packet[4001]=(unsigned char)row;
    packet[4002]=(unsigned char)(row<25 && col<80);
    ax=0x3c00; dx=(unsigned)name; unsigned cx=0;
    __asm__ volatile("int $0x21; sbb %3,%3" : "+a"(ax), "+d"(dx), "+c"(cx), "=r"(cf) : : "memory", "cc");
    if (cf & 255) stop(2);
    unsigned bx=ax; ax=0x4000; dx=(unsigned)packet; cx=sizeof packet;
    __asm__ volatile("int $0x21; sbb %4,%4" : "+a"(ax), "+b"(bx), "+d"(dx), "+c"(cx), "=r"(cf) : : "memory", "cc");
    unsigned bad=(cf & 255) || ax!=sizeof packet;
    ax=0x3e00;
    __asm__ volatile("int $0x21; sbb %2,%2" : "+a"(ax), "+b"(bx), "=r"(cf) : : "memory", "cc");
    stop(bad || (cf & 255) ? 3 : 0);
}
