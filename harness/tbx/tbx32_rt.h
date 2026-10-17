/*
 * harness/tbx/tbx32_rt.h -- the freestanding i386 Linux run-time of the
 * binding oracles (factory C; beads initech-tdnl.22 / initech-tdnl.23).
 *
 * The dev box has no 32-bit libc (CDR-0001's freestanding -m32), so the
 * i386 oracle processes are built -ffreestanding -nostdlib -static and talk
 * to Linux through raw `int 0x80` system calls. What this run-time adds:
 *   - THE TRAP CATCHER. A tenant's `int 0x81` is a general-protection fault
 *     in a user process (the IDT entry is DPL 0), which Linux delivers as
 *     SIGSEGV with EIP on the CD 81 bytes and every register intact. The
 *     handler reads AX and ESP exactly where the kernel gate does (spec/
 *     toolbox_gate.h Sec 2: argument i is the dword at the interrupted ESP +
 *     4i), calls the program's hook, writes the result into EAX and resumes
 *     after the instruction. `out dx, al` (EE: a tenant's Writeln to COM1)
 *     is caught the same way and handed to the serial hook. Any other fault
 *     is a tenant CRASH: reported, and the process exits 3 (Rule 2).
 *   - tbx32_call: call tenant code the way os/milton/tbx_gate.asm
 *     tbx_tenant_call does (EBX = the PSP; EBX/ESI/EDI/EBP saved; DF = 0).
 *   - buffered output on fd 1 (tm_write), file reading, an anonymous fixed
 *     mapping, exit.
 * ASCII (Rule 12).
 */
#ifndef INITECH_HARNESS_TBX32_RT_H
#define INITECH_HARNESS_TBX32_RT_H

#include <stdint.h>

/* The program's hooks. */
typedef int32_t (*tbx32_trap_fn)(uint32_t ax, const uint32_t *args,
                                 uint32_t esp, uint32_t ebp);
typedef void (*tbx32_serial_fn)(uint8_t byte);
extern tbx32_trap_fn   tbx32_on_trap;
extern tbx32_serial_fn tbx32_on_serial;   /* 0: serial bytes are dropped */

int  tbx32_main(int argc, char **argv);   /* the program's main */

void tbx32_install(void);                 /* the SIGSEGV catcher */
uint32_t tbx32_call(uint32_t fn, uint32_t ebx);
void tbx32_flush(void);
void tbx32_exit(int rc) __attribute__((noreturn));
void tbx32_die(const char *msg) __attribute__((noreturn));   /* exit 2 */
int  tbx32_read_file(const char *path, char *buf, int cap);  /* -1 on error */
void *tbx32_map_fixed(uint32_t addr, uint32_t len);          /* 0 on error */

void *memcpy(void *d, const void *s, uint32_t n);
void *memset(void *d, int c, uint32_t n);
void *memmove(void *d, const void *s, uint32_t n);
int   memcmp(const void *a, const void *b, uint32_t n);

#endif /* INITECH_HARNESS_TBX32_RT_H */
