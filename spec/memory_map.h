/*
 * spec/memory_map.h -- InitechDOS flat (32-bit) physical memory map constants.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8): the program-loader layout is a contract,
 * not a magic number buried in a .c file. The loader (os/milton/loader.c) and
 * the baked test program (os/milton/test_program.asm, org PROGRAM_IMAGE) BOTH
 * read these; the host loader oracle (os/milton/test_loader.c) asserts the
 * derived params against them. Changing any of these is a deliberate act
 * (Rule 8): it ripples into test_program.asm's `org` and the loader's bounds.
 *
 * Source / Law 1 citations (all local):
 *   docs/research/psp-loader-ground-truth.md Sec 1 (the confirmed memory map +
 *     the gap proof), Sec 3.2 (the concrete addresses + the worked alloc/env
 *     fake-paragraph values), Sec 8 (Open Items 1-2: PROGRAM_BASE +
 *     PROGRAM_STACK_TOP belong in a spec header).
 *   os/milton/kernel.ld:18 (kernel linked at 0x00010000; _kernel_end);
 *   Makefile KERNEL_SECTORS=64 -> kernel end <= ~0x18000;
 *   os/milton/kstart.asm:25 (kernel ESP = 0x0008FFFC -- the kernel stack lives
 *     at 0x80000+, well above the program region).
 *
 * Gap proof (psp-loader-ground-truth.md Sec 3.2, re-verified here):
 *   kernel end (max)   = 0x18000   (kernel.ld + 64 sectors)
 *   PROGRAM_BASE       = 0x20000   -> 32 KiB headroom above the kernel .bss
 *   PROGRAM_IMAGE      = 0x20100   -> PSP + 0x100 (the authentic .COM offset)
 *   ENV_BLOCK          = 0x20200   -> 2-byte empty environment (00 00)
 *   PROGRAM_ALLOC_END  = 0x70000   -> ceiling of the program's allocation
 *   PROGRAM_STACK_TOP  = 0x6FFFC   -> initial program ESP (4-aligned, < ceiling)
 *   kernel stack       = 0x80000+  -> 64 KiB gap below it; no collision
 *
 * ASCII-clean (Rule 12). No nondeterminism (Rule 11).
 */
#ifndef INITECH_SPEC_MEMORY_MAP_H
#define INITECH_SPEC_MEMORY_MAP_H

/* Flat linear address of the PSP (256 bytes). The program's "segment" base in
 * the .COM model. Ref Sec 1 / Sec 3.2. */
#define PROGRAM_BASE        0x00020000u

/* Flat linear entry point of the loaded program image = PROGRAM_BASE + 0x100.
 * The 0x100 offset preserves the authentic DOS .COM layout (PSP then image);
 * test_program.asm is assembled with `org 0x00020100` so its absolute
 * references resolve at the load address. Ref Sec 3.1 / Sec 3.2 / Sec 5.3. */
#define PROGRAM_IMAGE       0x00020100u

/* Flat linear address of the minimal environment block (two bytes: 00 00 = the
 * empty-but-valid environment). env_seg in the PSP stores (this >> 4) = 0x2020.
 * Ref Sec 2.7 / Sec 3.2. */
#define ENV_BLOCK           0x00020200u

/* Two bytes: a double-NUL = an empty (but valid) DOS environment block. A
 * program walking env_seg:0 immediately finds the terminator. Ref Sec 2.7. */
#define ENV_BLOCK_LEN       2u

/* Flat linear address one past the program's allocation (the memory ceiling).
 * Stored in the PSP alloc_end_seg as (this >> 4) = 0x7000. This is the top of
 * the program stack region. Ref Sec 2.2 / Sec 3.2. */
#define PROGRAM_ALLOC_END   0x00070000u

/* Initial program ESP: top of the 64 KiB program stack, 4-byte aligned, below
 * PROGRAM_ALLOC_END and far below the kernel stack (0x80000+). Ref Sec 3.2. */
#define PROGRAM_STACK_TOP   0x0006FFFCu

/* Bottom of the program stack region (informational; the loader does not need
 * to touch it -- the stack descends from PROGRAM_STACK_TOP). Ref Sec 3.2. */
#define PROGRAM_STACK_BOT   0x00060000u

/* The largest program image that fits between PROGRAM_IMAGE and the bottom of
 * the program stack region. The loader rejects (fail loud, Rule 2) any image
 * larger than this. = PROGRAM_STACK_BOT - PROGRAM_IMAGE. */
#define PROGRAM_IMAGE_MAX   (PROGRAM_STACK_BOT - PROGRAM_IMAGE)

#endif /* INITECH_SPEC_MEMORY_MAP_H */
