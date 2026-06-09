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
 *   Makefile KERNEL_SECTORS=96 -> kernel end ~0x1fd20 (kernel .bss grew:
 *     cfg_fat_buf et al. filled the old 64 KiB window);
 *   os/milton/kstart.asm:25 (kernel ESP = 0x0008FFFC -- the kernel stack lives
 *     at 0x80000+, well above the program region).
 *
 * Gap proof (psp-loader-ground-truth.md Sec 3.2; re-verified, raised per
 * beads initech-5pe):
 *   kernel window     = 0x10000..0x30000 = 128 KiB (kernel.ld + 96 sectors;
 *                       _kernel_end ~0x1fd20, so ~0x102E0 headroom under
 *                       PROGRAM_BASE -- was a 64 KiB window 0x10000..0x20000
 *                       with the kernel reaching 0x1fd20, ~480 B short of the
 *                       u0a guard; initech-5pe raised PROGRAM_BASE 0x20000 ->
 *                       0x30000 to restore room for further kernel growth)
 *   PROGRAM_BASE       = 0x30000   -> top of the 128 KiB kernel window
 *   PROGRAM_IMAGE      = 0x30100   -> PSP + 0x100 (the authentic .COM offset)
 *   PROGRAM_IMAGE_MAX end = ENV_BLOCK -> image arena 0x30100..0x5F000 (~188 KiB)
 *   ENV_BLOCK          = 0x5F000   -> 2-byte empty env, OUTSIDE the image arena
 *                                     (was 0x20200 = image+0x100 -> corrupted any
 *                                      .COM > 256 B; beads initech-2og, Rule 8)
 *   PROGRAM_STACK_BOT  = 0x60000   -> env region 0x5F000..0x60000 below the stack
 *   PROGRAM_ALLOC_END  = 0x70000   -> ceiling of the program's allocation
 *   PROGRAM_STACK_TOP  = 0x6FFFC   -> initial program ESP (4-aligned, < ceiling)
 *   kernel stack       = 0x80000+  -> 64 KiB gap below it; no collision
 *
 * ASCII-clean (Rule 12). No nondeterminism (Rule 11).
 */
#ifndef INITECH_SPEC_MEMORY_MAP_H
#define INITECH_SPEC_MEMORY_MAP_H

/* Flat linear address of the PSP (256 bytes). The program's "segment" base in
 * the .COM model. Ref Sec 1 / Sec 3.2. RAISED 0x20000 -> 0x30000 (beads
 * initech-5pe; LOCKED-spec change, Rule 8): the kernel outgrew its 64 KiB
 * window (_kernel_end ~0x1fd20), so the program window was lifted to give the
 * kernel a 128 KiB window (0x10000..0x30000). */
#define PROGRAM_BASE        0x00030000u

/* Flat linear entry point of the loaded program image = PROGRAM_BASE + 0x100.
 * The 0x100 offset preserves the authentic DOS .COM layout (PSP then image);
 * the baked .asm programs are assembled with `org 0x00030100` so their absolute
 * references resolve at the load address. NASM cannot read this C header, so
 * each os/milton program .asm duplicates this constant -- they MUST move
 * together with this define (initech-5pe). Ref Sec 3.1 / Sec 3.2 / Sec 5.3. */
#define PROGRAM_IMAGE       0x00030100u

/* Flat linear address of the minimal environment block (two bytes: 00 00 = the
 * empty-but-valid environment). env_seg in the PSP stores (this >> 4) = 0x5F00.
 *
 * RELOCATED 0x20200 -> 0x5F000 (beads initech-2og; LOCKED-spec change, Rule 8 --
 * deliberate, with that issue + worklog note). The OLD value 0x20200 = PROGRAM_
 * IMAGE (0x20100) + 0x100 sat INSIDE the program image: load_program() copies the
 * whole .COM to 0x20100 and THEN wrote the 2-byte env at 0x20200, silently zeroing
 * the program's own bytes at file offset 0x100 for ANY image > 256 bytes (a
 * deterministic 2-byte corruption -- reads as NUL). The env now lives in a
 * dedicated region (0x5F000..0x60000) just BELOW the program stack (PROGRAM_STACK_
 * BOT = 0x60000) and ABOVE the largest image (PROGRAM_IMAGE_MAX now ends at
 * ENV_BLOCK), so it can never overlap the image or the stack. Room (~4 KiB) for a
 * real PATH/COMSPEC environment later (beads atf). Ref Sec 2.7 / Sec 3.2. */
#define ENV_BLOCK           0x0005F000u

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

/* The largest program image that fits between PROGRAM_IMAGE and the dedicated
 * environment region (ENV_BLOCK, just below the program stack). The loader
 * rejects (fail loud, Rule 2) any image larger than this, so the image can never
 * reach the env block or the stack. = ENV_BLOCK - PROGRAM_IMAGE (beads
 * initech-2og: was PROGRAM_STACK_BOT - PROGRAM_IMAGE, before the env moved out of
 * the image arena). The env region 0x5F000..0x60000 sits between the max image
 * and PROGRAM_STACK_BOT (0x60000). */
#define PROGRAM_IMAGE_MAX   (ENV_BLOCK - PROGRAM_IMAGE)

/* ------------------------------------------------------------------------ *
 * FAT-sourced program load staging (beads initech-saw; multi-tenant file I/O
 * decouple, beads initech-0qh).
 *
 * AH=4Bh EXEC / load_program_from_fat (os/milton/loader.c) reads the named .COM
 * off the mounted FAT12 volume into a STAGING buffer, then load_program() copies
 * it DOWN to PROGRAM_IMAGE (0x30100) and JMPs in. The staging buffer must NOT
 * collide with the program region (0x30100..PROGRAM_STACK_BOT) -- the copy reads
 * staging and writes the program region, so they must be disjoint.
 *
 * Placement: the 64 KiB gap between PROGRAM_ALLOC_END (0x70000, the program's
 * memory ceiling) and the kernel stack (0x80000+; os/milton/kstart.asm:25 ESP =
 * 0x0008FFFC). This region is unused by the program model (it is ABOVE the
 * program's alloc ceiling) and below the kernel stack, so it is disjoint from
 * PROGRAM_IMAGE -- a clean, kernel-owned staging home (NOT the kernel stack;
 * Risk 2 / fs-mount-sft-ground-truth.md). The old single open-file buffer that
 * used to share this region is GONE: file OPEN/READ/WRITE are now positioned
 * per-handle over the cluster chain (no whole-file buffer; beads initech-0qh),
 * so this region is staging-only.
 *
 * Disjoint-from-PROGRAM_IMAGE proof: LOAD_STAGING_BASE (0x70000) >=
 * PROGRAM_ALLOC_END (0x70000) > PROGRAM_STACK_BOT (0x60000) > PROGRAM_IMAGE
 * (0x30100); the staging window [0x70000,0x80000) never overlaps the program
 * region [0x30100,0x60000). SINGLE-USE LIMIT (documented): EXEC runs from
 * kernel/shell context and is single-level (loader g_load_active guard), so one
 * staging region suffices; concurrent EXEC is a follow-up bead. The image-size
 * cap is the smaller of LOAD_STAGING_MAX and PROGRAM_IMAGE_MAX (load_program
 * rejects anything bigger). */
#define LOAD_STAGING_BASE   0x00070000u
#define LOAD_STAGING_MAX    0x00010000u   /* 64 KiB staging (0x70000..0x80000)   */

#endif /* INITECH_SPEC_MEMORY_MAP_H */
