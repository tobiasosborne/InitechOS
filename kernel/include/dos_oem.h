/* InitechDOS OEM functions. Microsoft's MS-DOS2 source/SYSCALL.TXT reserves
 * AH=F8h..FFh for OEM extensions; F8h itself is left unused here.
 * https://github.com/microsoft/MS-DOS/blob/main/v2.0/source/SYSCALL.txt
 * Contracts are documented in kernel/README.md. */
#ifndef INITECH_DOS_OEM_H
#define INITECH_DOS_OEM_H
#define DOS_OEM_ARENA   0xf9u
#define DOS_OEM_RESTART 0xfau
#define DOS_OEM_PAGE    0xfbu
#endif
