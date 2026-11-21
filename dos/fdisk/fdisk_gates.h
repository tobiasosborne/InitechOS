#ifndef INITECH_FDISK_GATES_H
#define INITECH_FDISK_GATES_H

#include <stdint.h>

/* INT 13h returns -1 on carry, otherwise AH. CX and DX are in/out. */
int fdisk_int13(unsigned ax, uintptr_t bx, unsigned *cx, unsigned *dx);
unsigned fdisk_int21(unsigned ax, unsigned bx, unsigned cx, uintptr_t dx);
void fdisk_restart(void);

#endif
