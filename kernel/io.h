/* io.h -- freestanding x86 port I/O primitives.
 *
 * Ref: Intel SDM Vol 1/2 (IN/OUT instructions); CLAUDE.md ADR-0002 + CDR-0001
 *      (freestanding C, gcc -m32 -ffreestanding -nostdlib). beads: initech-d00.
 *
 * ARTIFACT code: freestanding, no libc. Inline asm only; no host headers.
 */
#ifndef INITECH_IO_H
#define INITECH_IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

#endif /* INITECH_IO_H */
