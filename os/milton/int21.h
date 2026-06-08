/* int21.h -- InitechDOS INT 21h dispatcher (the `int 0x21` syscall spine).
 *
 * beads: initech-509.5 ("INT 21h dispatcher: full controlled register").
 *        Gate ratification: initech-1f9. CONSOLE subset only (no filesystem);
 *        file-handle/SFT functions (3Dh/3Eh/3Fh, find-first/next) are deferred
 *        to initech-509.3.
 * Ref:   docs/research/internals-int21h-ground-truth.md Sec 5 (flat 32-bit
 *        calling convention: AH-dispatch, EDX flat ptr, ECX count, EBX handle,
 *        EAX return, CF in saved EFLAGS), Sec 5.4 (the register frame + the
 *        carry-flag return mechanism), Sec 6 (the console-output first
 *        functions); spec/int21h_calling_convention.json (the LOCKED ABI + the
 *        per-function table); spec/int21h_register.json (the controlled scope --
 *        NO unlisted functions; ADR-0003 DEC-04 / DEC-13); spec/dos_messages.json
 *        (the controlled diagnostic catalogue). CLAUDE.md Law 1 (cite source),
 *        Law 2 (oracle is truth), Law 3 (artifact = C), Rule 2 (fail loud +
 *        controlled scope), Rule 8 (specs-as-data), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. The
 * SAME translation unit (int21.c) ALSO compiles HOSTED for the factory dispatch
 * oracle (os/milton/test_int21.c, reuses seed/test_assert.h). So the CON output
 * and the terminate action are routed through a SINK abstraction (a function
 * pointer the host test overrides) -- never a direct console/serial call from
 * the dispatch logic. The kernel binds the real sink (console + serial) once at
 * boot via int21_set_sink().
 */
#ifndef INITECH_INT21_H
#define INITECH_INT21_H

#include <stdint.h>
#include "idt.h"   /* int_frame_t -- the trap stub builds the SAME layout */

/* DOS error codes used by this subset (DOS 3.3 INT 21h error returns). */
#define INT21_ERR_INVALID_FUNCTION  0x0001u  /* unlisted/not-yet-impl AH      */
#define INT21_ERR_INVALID_HANDLE    0x0006u  /* WRITE to a non-CON handle     */

/* Predefined device handles (no SFT yet -- the only handles this subset honors;
 * real JFT/SFT file handles arrive with beads initech-509.3). */
#define INT21_HANDLE_STDOUT  1u
#define INT21_HANDLE_STDERR  2u

/* InitechDOS version (3.30; ADR-0003 DEC-12 / spec/dos_banner.txt). GETVER
 * returns AL=major, AH=minor. 3.30 -> minor 30 = 0x1E. */
#define INT21_VER_MAJOR  3u
#define INT21_VER_MINOR  30u   /* 0x1E */

/* The CON SINK: every byte the dispatcher would "display" goes here. The kernel
 * binds a sink that fans out to the LFB console + COM1 serial; the host oracle
 * binds a sink that captures into a buffer. NULL (the default) discards bytes
 * (so the dispatch logic never faults if the sink is unbound). */
typedef void (*int21_sink_fn)(char c);

/* The TERMINATE hook: AH=4Ch / AH=00h call this with the return code. The kernel
 * binds a hook that emits the exit line then cli;hlt (terminate == stop, no
 * process model yet); the host oracle binds a hook that records the code +
 * returns (so the test can observe it without halting). NULL -> the dispatcher
 * still emits the diagnostic but simply returns (host-safe default). */
typedef void (*int21_exit_fn)(uint8_t code);

/* Bind the CON sink (NULL clears it). Called once by the kernel at boot. */
void int21_set_sink(int21_sink_fn sink);

/* Bind the terminate hook (NULL clears it). Called once by the kernel at boot. */
void int21_set_exit(int21_exit_fn fn);

/* The C dispatch routine the asm trap stub (int21_entry, isr.asm) invokes with a
 * pointer to the on-stack int_frame_t. Reads AH = (frame->eax >> 8) & 0xFF and
 * switches per spec/int21h_register.json. Writes the return value into
 * frame->eax and sets/clears CF (bit 0 of frame->eflags) before returning; the
 * stub's iretd then restores the modified EFLAGS so the caller sees CF. An AH
 * not in the locked register emits a diagnostic and returns CF=1/AX=0x0001 --
 * NEVER a silent no-op (Rule 2 / DEC-13). */
void int21_dispatch(int_frame_t *frame);

/* The INT 20h legacy-terminate handler the asm trap stub (int20_entry, isr.asm)
 * invokes. Routes to the SAME terminate path as INT 21h AH=4Ch with exit code 0
 * (DOS legacy termination; ADR-0003 DEC-10, beads initech-509.5). Ref:
 * docs/research/psp-loader-ground-truth.md Sec 2.1 / Sec 4.4. */
void int20_dispatch(int_frame_t *frame);

#endif /* INITECH_INT21_H */
