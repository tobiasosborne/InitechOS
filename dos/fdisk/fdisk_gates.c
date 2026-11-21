#include "fdisk_gates.h"

int fdisk_int13(unsigned ax, uintptr_t bx, unsigned *cx, unsigned *dx)
{
    unsigned c = *cx, d = *dx;
    unsigned char carry;
    __asm__ volatile("int $0x13"
                     : "=@ccc"(carry), "+a"(ax), "+b"(bx), "+c"(c), "+d"(d)
                     :: "memory", "cc");
    *cx = c;
    *dx = d;
    return carry ? -1 : (int)((ax >> 8) & 255u);
}

unsigned fdisk_int21(unsigned ax, unsigned bx, unsigned cx, uintptr_t dx)
{
    __asm__ volatile("int $0x21"
                     : "+a"(ax), "+b"(bx), "+c"(cx), "+d"(dx)
                     :: "memory", "cc");
    return ax;
}

void fdisk_restart(void)
{
    /* OEM INT 21h AH=FAh AL=0 never returns (kernel/README.md, initech-q9p80).
     * If an older kernel returns, halt with the partition state still stale. */
    fdisk_int21(0xfa00,0,0,0);
    for(;;)__asm__ volatile("cli; hlt");
}
