/* Guest test utility. DOS 3.3 AH=2Ch: CH hours, CL minutes, DH seconds,
 * DL hundredths. Authority: kernel/int21.c do_get_time documented ABI.
 * Print through AH=40h; no hardware/private kernel access. */
void _start(void)
{
    unsigned ax=0x2c00,cx,dx;
    __asm__ volatile("int $0x21" : "+a"(ax),"=c"(cx),"=d"(dx) : : "cc","memory");
    cx &= 65535; dx &= 65535;
    unsigned tick=(((cx>>8)*60+(cx&255))*60+(dx>>8))*100+(dx&255);
    char text[15]="TICK=00000000\r\n";
    for(int i=12;i>=5;i--){text[i]=(char)('0'+tick%10);tick/=10;}
    /* Positions 5..12 are exactly eight decimal digits. */
    ax=0x4000;unsigned bx=1;cx=15;dx=(unsigned)text;
    __asm__ volatile("int $0x21" : "+a"(ax),"+b"(bx),"+c"(cx),"+d"(dx) : : "cc","memory");
    ax=0x4c00;
    __asm__ volatile("int $0x21" : "+a"(ax) : : "cc","memory");
    for(;;){}
}
