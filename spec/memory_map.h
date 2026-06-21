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
 * DELIBERATE-ACT NOTE (Rule 8) -- beads initech-o0td (committee-ratified PATH 2,
 * 2026-06-20): whole-map shift +0x8000 of the conventional layout above
 * PROGRAM_BASE, lifting it 0x30000 -> 0x38000 for +32 KiB of kernel-window
 * headroom; the kernel stack (kstart.asm) moved up the same +0x8000. See
 * docs/worklog for the ratification. PROGRAM_BSS_RESERVE and every FLAIR_HEAP_*
 * constant are UNCHANGED.
 *
 * DELIBERATE-ACT NOTE (Rule 8) -- beads initech-re30.2 (FLAIR Phase-3 M3.0
 * headroom, 2026-06-21): the SECOND whole-map shift +0x8000 of the conventional
 * layout above PROGRAM_BASE, lifting it 0x38000 -> 0x40000 for ANOTHER +32 KiB
 * of kernel-window headroom (160 -> 192 KiB, 0x10000..0x40000) to make room for
 * the FLAIR Toolbox manager set (linked at M3.0, a SEPARATE later step). Exactly
 * mirrors the initech-o0td shift one step earlier (0x30000 -> 0x38000): every
 * program-region constant and the kernel stack (kstart.asm) move up the same
 * +0x8000, so all relative distances -- and the disjointness proofs -- are
 * preserved; only the absolute literals move. PROGRAM_BSS_RESERVE and every
 * FLAIR_HEAP_* constant are UNCHANGED. CRITICAL: this +0x8000 raise spends the
 * LAST of the conventional free gap (free gap = 0xA0000..0xA0000 = 0 KiB, since
 * the kernel stack now tops at 0x9FFFC, adjacent to the 0xA0000 VGA aperture):
 * this is the MAXIMUM PROGRAM_BASE raise possible under the conventional-memory
 * scheme -- any further kernel growth requires a high-half / extended-memory
 * relocation (filed as a follow-up). See docs/worklog for the ratification.
 *
 * Source / Law 1 citations (all local):
 *   docs/research/psp-loader-ground-truth.md Sec 1 (the confirmed memory map +
 *     the gap proof), Sec 3.2 (the concrete addresses + the worked alloc/env
 *     fake-paragraph values), Sec 8 (Open Items 1-2: PROGRAM_BASE +
 *     PROGRAM_STACK_TOP belong in a spec header).
 *   os/milton/kernel.ld:18 (kernel linked at 0x00010000; _kernel_end);
 *   Makefile KERNEL_SECTORS=96 -> kernel end ~0x1fd20 (kernel .bss grew:
 *     cfg_fat_buf et al. filled the old 64 KiB window);
 *   os/milton/kstart.asm:25 (kernel ESP = 0x0009FFFC -- the kernel stack lives
 *     at 0x90000+, well above the program region; raised +0x8000 by initech-o0td
 *     then ANOTHER +0x8000 by initech-re30.2 in lockstep with the whole-map
 *     shifts below).
 *
 * Gap proof (psp-loader-ground-truth.md Sec 3.2; re-verified, raised per
 * beads initech-5pe, then shifted up +0x8000 per beads initech-o0td, then a
 * SECOND +0x8000 per beads initech-re30.2):
 *   kernel window     = 0x10000..0x40000 = 192 KiB (kernel.ld + 96 sectors;
 *                       _kernel_end ~0x28890 (MEASURED 2026-06-21 via
 *                       `nm build/kernel.elf`; the earlier "~0x1fd20" figure was
 *                       stale by ~36 KiB), so ~0x17770 (~93 KiB) headroom under
 *                       PROGRAM_BASE -- was a 160 KiB window 0x10000..0x38000;
 *                       initech-re30.2 raised PROGRAM_BASE 0x38000 -> 0x40000 to
 *                       give the kernel +32 KiB MORE of growth window, analogous
 *                       to how initech-o0td raised it 0x30000 -> 0x38000 and
 *                       initech-5pe raised it 0x20000 -> 0x30000. NOTE
 *                       (initech-re30.1): the BOOTING shell kernel
 *                       (kernel_shell.elf) is TIGHTER -- _kernel_end=0x30920,
 *                       only ~61 KiB headroom under the new 0x40000 base -- and
 *                       linking the FLAIR Manager set (~51 KiB) at Phase-3 M3.0
 *                       is the reason this raise was taken NOW; kernel.ld now
 *                       ASSERTs _kernel_end<0x40000 (fail-loud at LINK,
 *                       mutation-proven).)
 *   PROGRAM_BASE       = 0x40000   -> top of the 192 KiB kernel window
 *   PROGRAM_IMAGE      = 0x40100   -> PSP + 0x100 (the authentic .COM offset)
 *   PROGRAM_IMAGE_MAX end = ENV_BLOCK -> image arena 0x40100..0x6F000 (~187 KiB)
 *   ENV_BLOCK          = 0x6F000   -> 2-byte empty env, OUTSIDE the image arena
 *                                     (was 0x20200 = image+0x100 -> corrupted any
 *                                      .COM > 256 B; beads initech-2og, Rule 8)
 *   PROGRAM_STACK_BOT  = 0x70000   -> env region 0x6F000..0x70000 below the stack
 *   PROGRAM_ALLOC_END  = 0x80000   -> ceiling of the program's allocation
 *   PROGRAM_STACK_TOP  = 0x7FFFC   -> initial program ESP (4-aligned, < ceiling)
 *   LOAD_STAGING       = 0x80000..0x90000 -> 64 KiB EXEC staging (above ceiling)
 *   kernel stack       = 0x90000..0xA0000 -> kstart.asm ESP 0x9FFFC; 0 KiB free
 *   free gap           = 0xA0000..0xA0000 = 0 KiB (this +0x8000 raise spent the
 *                        LAST of the conventional gap; the kernel stack now tops
 *                        at 0x9FFFC, adjacent to the 0xA0000 VGA / mode-0x13-LFB-
 *                        fallback aperture. This is the MAXIMUM PROGRAM_BASE raise
 *                        possible under the conventional-memory scheme -- further
 *                        kernel growth requires a high-half / extended-memory
 *                        relocation, filed as a follow-up.)
 *
 * THE WHOLE-MAP SHIFTS (beads initech-o0td 2026-06-20, then initech-re30.2
 * 2026-06-21; both committee-ratified; Rule 8 -- deliberate locked-data acts):
 * the entire conventional-memory layout above PROGRAM_BASE moved UP by delta =
 * 0x8000 (32 KiB) TWICE. o0td reclaimed half the old [0x90000,0xA0000) free gap
 * to lift PROGRAM_BASE 0x30000 -> 0x38000 (kernel window 128 -> 160 KiB);
 * re30.2 reclaimed the OTHER half to lift it 0x38000 -> 0x40000 (160 -> 192 KiB),
 * spending the last of the conventional gap. Every program-region constant
 * (PROGRAM_BASE/IMAGE, ENV_BLOCK, PROGRAM_STACK_BOT/TOP, PROGRAM_ALLOC_END,
 * LOAD_STAGING_BASE) and the kernel stack (kstart.asm) shifted by the same
 * +0x8000 each time, so all relative distances -- and thus the disjointness
 * proofs below -- are PRESERVED; only the absolute addresses move. See
 * docs/worklog for the committee ratifications. PROGRAM_BSS_RESERVE and every
 * FLAIR_HEAP_* constant are UNCHANGED.
 *
 * ASCII-clean (Rule 12). No nondeterminism (Rule 11).
 */
#ifndef INITECH_SPEC_MEMORY_MAP_H
#define INITECH_SPEC_MEMORY_MAP_H

/* Flat linear address of the PSP (256 bytes). The program's "segment" base in
 * the .COM model. Ref Sec 1 / Sec 3.2. RAISED 0x20000 -> 0x30000 (beads
 * initech-5pe), then RAISED 0x30000 -> 0x38000 (beads initech-o0td), then RAISED
 * 0x38000 -> 0x40000 (beads initech-re30.2; LOCKED-spec change, Rule 8): the
 * whole conventional map shifted up +0x8000 a SECOND time to give the kernel a
 * 192 KiB window (0x10000..0x40000), +32 KiB of growth room over the prior
 * 160 KiB window -- the room the FLAIR Toolbox manager set needs at M3.0. */
#define PROGRAM_BASE        0x00040000u

/* Flat linear entry point of the loaded program image = PROGRAM_BASE + 0x100.
 * The 0x100 offset preserves the authentic DOS .COM layout (PSP then image);
 * the baked .asm programs are assembled with `org 0x00040100` so their absolute
 * references resolve at the load address. NASM cannot read this C header, so
 * each os/milton program .asm duplicates this constant -- they MUST move
 * together with this define (initech-5pe; shifted +0x8000 by initech-o0td, then
 * a SECOND +0x8000 by initech-re30.2). Ref Sec 3.1 / Sec 3.2 / Sec 5.3. */
#define PROGRAM_IMAGE       0x00040100u

/* Flat linear address of the minimal environment block (two bytes: 00 00 = the
 * empty-but-valid environment). env_seg in the PSP stores (this >> 4) = 0x6F00.
 *
 * RELOCATED 0x20200 -> 0x5F000 (beads initech-2og; LOCKED-spec change, Rule 8 --
 * deliberate, with that issue + worklog note), then SHIFTED 0x5F000 -> 0x67000
 * (beads initech-o0td; whole-map +0x8000), then SHIFTED 0x67000 -> 0x6F000 (beads
 * initech-re30.2; whole-map +0x8000 again). The OLD value 0x20200 = PROGRAM_
 * IMAGE (0x20100) + 0x100 sat INSIDE the program image: load_program() copies the
 * whole .COM to 0x20100 and THEN wrote the 2-byte env at 0x20200, silently zeroing
 * the program's own bytes at file offset 0x100 for ANY image > 256 bytes (a
 * deterministic 2-byte corruption -- reads as NUL). The env now lives in a
 * dedicated region (0x6F000..0x70000) just BELOW the program stack (PROGRAM_STACK_
 * BOT = 0x70000) and ABOVE the largest image (PROGRAM_IMAGE_MAX now ends at
 * ENV_BLOCK), so it can never overlap the image or the stack. Room (~4 KiB) for a
 * real PATH/COMSPEC environment later (beads atf). Ref Sec 2.7 / Sec 3.2. */
#define ENV_BLOCK           0x0006F000u

/* Two bytes: a double-NUL = an empty (but valid) DOS environment block. A
 * program walking env_seg:0 immediately finds the terminator. Ref Sec 2.7. */
#define ENV_BLOCK_LEN       2u

/* Total bytes available in the dedicated environment region for a POPULATED
 * environment block (beads initech-1i0x Tranche E inc 3 -- EXEC env inheritance).
 * The region runs from ENV_BLOCK (0x6F000) up to PROGRAM_STACK_BOT (0x70000), so
 * the shell may serialize up to ENV_BLOCK_CAP bytes of "NAME=VALUE\0..\0" there
 * before the AH=4Bh EXEC without ever reaching the program stack. = PROGRAM_STACK_
 * BOT - ENV_BLOCK = 0x1000 = 4096 bytes -- comfortably above env.h's serialize
 * ceiling (ENV_ARENA_MAX + 1 = 513 bytes), with headroom for the deferred DOS 3.0+
 * trailing word-count + program-path string (beads atf). The shell bounds the
 * serialize write to this capacity and fails loud (Rule 2) on overflow rather than
 * scribbling past 0x70000 into the program stack. DERIVED from the locked
 * constants -- never a magic literal -- so it tracks any Rule-8 map change. */
#define ENV_BLOCK_CAP       (PROGRAM_STACK_BOT - ENV_BLOCK)

/* Flat linear address one past the program's allocation (the memory ceiling).
 * Stored in the PSP alloc_end_seg as (this >> 4) = 0x8000. This is the top of
 * the program stack region. Ref Sec 2.2 / Sec 3.2. (Shifted 0x70000 -> 0x78000
 * by beads initech-o0td, then 0x78000 -> 0x80000 by beads initech-re30.2;
 * whole-map +0x8000 each.) */
#define PROGRAM_ALLOC_END   0x00080000u

/* Initial program ESP: top of the 64 KiB program stack, 4-byte aligned, below
 * PROGRAM_ALLOC_END and far below the kernel stack (0x90000+). Ref Sec 3.2.
 * (Shifted 0x6FFFC -> 0x77FFC by beads initech-o0td, then 0x77FFC -> 0x7FFFC by
 * beads initech-re30.2; whole-map +0x8000 each.) */
#define PROGRAM_STACK_TOP   0x0007FFFCu

/* Bottom of the program stack region (informational; the loader does not need
 * to touch it -- the stack descends from PROGRAM_STACK_TOP). Ref Sec 3.2.
 * (Shifted 0x60000 -> 0x68000 by beads initech-o0td, then 0x68000 -> 0x70000 by
 * beads initech-re30.2; whole-map +0x8000 each.)
 *
 * ALSO the AH=48h MCB ARENA CEILING (beads initech-1q4u; ADR-0009 DEC-04): the
 * loader binds the heap arena to end here, so a 48h ALLOC can never return memory
 * inside the 64 KiB program stack [PROGRAM_STACK_BOT, PROGRAM_STACK_TOP]. See the
 * ARENA DISJOINTNESS INVARIANT block below. */
#define PROGRAM_STACK_BOT   0x00070000u

/* The largest program image that fits between PROGRAM_IMAGE and the dedicated
 * environment region (ENV_BLOCK, just below the program stack). The loader
 * rejects (fail loud, Rule 2) any image larger than this, so the image can never
 * reach the env block or the stack. = ENV_BLOCK - PROGRAM_IMAGE (beads
 * initech-2og: was PROGRAM_STACK_BOT - PROGRAM_IMAGE, before the env moved out of
 * the image arena). The env region 0x6F000..0x70000 sits between the max image
 * and PROGRAM_STACK_BOT (0x70000). Value is invariant under both whole-map
 * +0x8000 shifts (both ENV_BLOCK and PROGRAM_IMAGE moved up together each time):
 * 0x6F000 - 0x40100 = 0x2EF00 (~187 KiB), same as the o0td 0x67000 - 0x38100 and
 * the original 0x5F000 - 0x30100. */
#define PROGRAM_IMAGE_MAX   (ENV_BLOCK - PROGRAM_IMAGE)

/* ------------------------------------------------------------------------ *
 * AH=48h MCB ARENA DISJOINTNESS INVARIANT (beads initech-1q4u; ADR-0009 DEC-04,
 * RATIFIED 2026-06-19; Rule 8 -- a deliberate locked-data act).
 *
 * THE BUG THIS FIXES (ADR-0009 Sec 1 / DEC-04): the AH=48h/49h/4Ah MCB heap arena
 * was bound over the WHOLE program window [PROGRAM_BASE, PROGRAM_ALLOC_END) with
 * flat base == PROGRAM_BASE (sysinit.c). That window IS the running program: its
 * PSP (0x40000), its image+BSS (0x40100+), its env (0x6F000), and its 64 KiB
 * stack (top 0x7FFFC) all live inside it. So a 48h ALLOC (after the authentic
 * 4Ah shrink) would hand the program a block OVERLAPPING its own code/BSS/stack.
 * Latent before SAMIR (toy .COMs never call 48h; the emulator zero-inits RAM),
 * but SAMIR -- the first heap-using app -- would corrupt itself nondeterministically.
 *
 * THE FIX (DEC-04; the authentic DOS model -- DOS hands the program the block
 * AFTER its image, not the block it is running in): the loader binds the arena to
 * a COMPUTED window DISJOINT from the loaded program, NOT to [PROGRAM_BASE, ...).
 * The window is
 *
 *   arena_base    = roundup_paragraph(PROGRAM_IMAGE + image_len + PROGRAM_BSS_RESERVE)
 *   arena_ceiling = PROGRAM_ARENA_CEIL              ( == ENV_BLOCK, see below )
 *   arena         = [arena_base, arena_ceiling)
 *
 * so EVERY address a 48h ALLOC can return satisfies
 *   arena_base >= PROGRAM_IMAGE + image_len   (above the loaded image+BSS), AND
 *   alloc + size <= PROGRAM_ARENA_CEIL         (below env + stack).
 *
 * DISJOINTNESS PROOF (the regions a heap block must NEVER touch):
 *   1. PSP + image + BSS  : [PROGRAM_BASE, image_end), image_end = PROGRAM_IMAGE
 *      + image_len. arena_base is paragraph-rounded ABOVE image_end +
 *      PROGRAM_BSS_RESERVE, so arena_base > image_end: the arena never overlaps
 *      the PSP/image/BSS. (PROGRAM_BSS_RESERVE covers the .bss a flat .COM does
 *      NOT carry in its file -- the loader knows only image_len, not the BSS
 *      extent; ADR-0009 DEC-05's crt0 stub zeroes BSS within this reserve.)
 *   2. Env block          : [ENV_BLOCK, ENV_BLOCK+ENV_BLOCK_LEN). The ceiling is
 *      PROGRAM_ARENA_CEIL == ENV_BLOCK (exclusive), so no allocated paragraph
 *      reaches the env the PSP env_seg points at.
 *   3. Program stack      : [PROGRAM_STACK_BOT, PROGRAM_STACK_TOP]. Since
 *      PROGRAM_ARENA_CEIL (== ENV_BLOCK, 0x6F000) < PROGRAM_STACK_BOT (0x70000),
 *      the ceiling is BELOW the stack: no allocated paragraph reaches the stack.
 * The single invariant a caller relies on: a 48h block is ALWAYS within
 * [arena_base, PROGRAM_ARENA_CEIL) with arena_base > image_end -- provably
 * disjoint from the image, the env, and the stack.
 *
 * WORKED NUMBERS at the initech-re30.2 map, for SAMIR (image_len = 0x13220,
 * .bss = 0xBD20, both measured 2026-06-20; addresses shifted +0x8000 from the
 * o0td worked example, distances unchanged):
 *   image_end  = PROGRAM_IMAGE + image_len     = 0x40100 + 0x13220 = 0x53320
 *   .bss end   = image_end + .bss              = 0x53320 + 0xBD20  = 0x5F040
 *   arena_base = roundup_para(0x40100 + 0x13220 + 0x10000)         = 0x63320
 *   arena      = [0x63320, 0x6F000)            = 0xBCE0 = 47.2 KiB usable heap
 *   margin     = arena_base - .bss end = 0x63320 - 0x5F040 = 0x42E0 = 16.7 KiB
 *               -> arena_base (0x63320) >= .bss end (0x5F040): NO heap/BSS overlap.
 *   env  [0x6F000, 0x6F002) sits AT the exclusive ceiling (PROGRAM_ARENA_CEIL);
 *   stack [0x70000, 0x7FFFC] sits ABOVE the ceiling -> arena, env, stack all
 *   pairwise disjoint. (This is exactly the disjointness the +0x8000 shift
 *   preserves: every distance -- usable heap 0xBCE0, margin 0x42E0 -- is
 *   byte-identical to the o0td and 5pe maps; only the absolute literals moved.)
 *
 * The loader fails loud (Rule 2) if image_len + PROGRAM_BSS_RESERVE leaves no
 * positive-size window below PROGRAM_ARENA_CEIL (an image too big to heap-host);
 * the program then runs with NO arena bound and a 48h ALLOC reports insufficient
 * memory rather than a corrupting overlap. See loader.c int21_mcb_bind_program.
 *
 * (HISTORICAL NOTE: when this invariant was ADDED -- ADR-0009 DEC-04 -- it changed
 * no numeric constant; the arena BASE became a COMPUTED value, was the literal
 * PROGRAM_BASE in sysinit.c, and only this commentary + PROGRAM_BSS_RESERVE were
 * added. The LATER beads initech-o0td whole-map +0x8000 shift, and then the
 * beads initech-re30.2 SECOND +0x8000 shift, DO move the numeric constants
 * (PROGRAM_BASE / PROGRAM_IMAGE / ENV_BLOCK / PROGRAM_STACK_BOT / ...) and the
 * .asm `org`s up by 0x8000 each, but the proof STRUCTURE is invariant: every
 * region shifted by the same delta, so all the >= / < relations and the
 * PROGRAM_IMAGE_MAX bound hold byte-for-byte at the new addresses -- only the
 * absolute literals in the worked SAMIR numbers above move.)
 * Ref: ADR-0009 DEC-04/DEC-05; DOS 3.3 PRM AH=48h; beads initech-o0td,
 * initech-re30.2. */

/* Conservative .bss reserve the loader adds PAST the loaded image before the heap
 * arena begins (beads initech-1q4u; ADR-0009 DEC-04/DEC-05). A flat .COM does NOT
 * carry its .bss in the file, so the loader -- which knows only image_len (the
 * on-disk bytes) -- cannot compute the BSS end. This reserve bounds it.
 *
 * MEASURED SIZING (beads initech-o0td, 2026-06-20, Law 1 -- this corrects a stale
 * claim): build/samir_com/SAMIR.elf was measured today as ~76.5 KiB on-disk image
 * (text+rodata, image_len = 0x13220) with .bss = 0xBD20 = 48,416 bytes = 47.3 KiB.
 * (The earlier comment here said "~81 KiB text + ~26 KiB BSS"; the 26 KiB BSS
 * figure was WRONG and stale -- the real .bss is 47.3 KiB.) The 64 KiB (0x10000)
 * reserve covers the measured 47.3 KiB .bss the crt0 stub (DEC-05) zeroes past the
 * text, with 0x10000 - 0xBD20 = 0x42E0 = 16.7 KiB of slack -- so the heap
 * arena_base (= roundup_para(PROGRAM_IMAGE + image_len + PROGRAM_BSS_RESERVE))
 * sits ABOVE SAMIR's real .bss end, never overlapping it (worked below).
 *
 * THIS MEASUREMENT IS DISPOSITIVE: it killed the rejected "shrink the reserve to
 * 32 KiB" plan, which would have put arena_base BELOW SAMIR's 47.3 KiB .bss end
 * (32 KiB < 47.3 KiB) and re-created the y206 heap-over-BSS corruption. Hence the
 * committee-ratified PATH 2 instead WIDENS the kernel window (whole-map +0x8000)
 * and KEEPS this reserve at 64 KiB.
 *
 * It is intentionally generous: a too-small reserve risks a heap block overlapping
 * uninitialised BSS; a too-large one only shrinks the heap window (a ~187 KiB image
 * arena minus a 64 KiB reserve still leaves a usable heap below PROGRAM_STACK_BOT
 * for any realistic image). The loader fails loud (Rule 2) if image_len + this
 * reserve leaves no positive arena below the ceiling. UNCHANGED by initech-o0td
 * AND by initech-re30.2 (both whole-map shifts move only absolute addresses; the
 * reserve is a distance, so it is invariant). */
#define PROGRAM_BSS_RESERVE 0x00010000u

/* The AH=48h heap-arena CEILING (exclusive): no allocated paragraph reaches at
 * or above this address (beads initech-1q4u; ADR-0009 DEC-04). == ENV_BLOCK so
 * the heap stays BELOW both the env block (at ENV_BLOCK) and -- since ENV_BLOCK
 * (0x6F000) < PROGRAM_STACK_BOT (0x70000) -- the 64 KiB program stack. The loader
 * binds [arena_base, PROGRAM_ARENA_CEIL); see the disjointness invariant above. */
#define PROGRAM_ARENA_CEIL  ENV_BLOCK

/* ------------------------------------------------------------------------ *
 * FAT-sourced program load staging (beads initech-saw; multi-tenant file I/O
 * decouple, beads initech-0qh).
 *
 * AH=4Bh EXEC / load_program_from_fat (os/milton/loader.c) reads the named .COM
 * off the mounted FAT12 volume into a STAGING buffer, then load_program() copies
 * it DOWN to PROGRAM_IMAGE (0x40100) and JMPs in. The staging buffer must NOT
 * collide with the program region (0x40100..PROGRAM_STACK_BOT) -- the copy reads
 * staging and writes the program region, so they must be disjoint.
 *
 * Placement: the 64 KiB gap between PROGRAM_ALLOC_END (0x80000, the program's
 * memory ceiling) and the kernel stack (0x90000+; os/milton/kstart.asm:25 ESP =
 * 0x0009FFFC -- the kernel stack moved up +0x8000 with the rest of the map under
 * beads initech-o0td, then a SECOND +0x8000 under beads initech-re30.2). This
 * region is unused by the program model (it is ABOVE the program's alloc ceiling)
 * and below the kernel stack, so it is disjoint from PROGRAM_IMAGE -- a clean,
 * kernel-owned staging home (NOT the kernel stack; Risk 2 /
 * fs-mount-sft-ground-truth.md). The old single open-file buffer that used to
 * share this region is GONE: file OPEN/READ/WRITE are now positioned per-handle
 * over the cluster chain (no whole-file buffer; beads initech-0qh), so this
 * region is staging-only.
 *
 * Disjoint-from-PROGRAM_IMAGE proof: LOAD_STAGING_BASE (0x80000) >=
 * PROGRAM_ALLOC_END (0x80000) > PROGRAM_STACK_BOT (0x70000) > PROGRAM_IMAGE
 * (0x40100); the staging window [0x80000,0x90000) never overlaps the program
 * region [0x40100,0x70000). Disjoint-from-kernel-stack proof: the staging top
 * (0x90000) == the kernel-stack bottom (0x90000) -- adjacent, so disjoint -- and
 * the kernel-stack top (kstart.asm ESP 0x9FFFC) is adjacent to (one dword below)
 * the 0xA0000 VGA/BIOS aperture, so the kernel stack [0x90000,0xA0000) stays
 * BELOW VGA: 0x9FFFC < 0xA0000. NOTE (initech-re30.2): this SECOND +0x8000 shift
 * spent the LAST of the conventional free gap -- the kernel stack now butts
 * directly against the 0xA0000 VGA aperture with no slack, so this is the maximum
 * raise possible under the conventional-memory scheme (further growth needs a
 * high-half / extended-memory relocation). SINGLE-USE LIMIT (documented): EXEC
 * runs from kernel/shell context and is single-level (loader g_load_active
 * guard), so one staging region suffices; concurrent EXEC is a follow-up bead.
 * The image-size cap is the smaller of LOAD_STAGING_MAX and PROGRAM_IMAGE_MAX
 * (load_program rejects anything bigger). */
#define LOAD_STAGING_BASE   0x00080000u
#define LOAD_STAGING_MAX    0x00010000u   /* 64 KiB staging (0x80000..0x90000)   */

/* ------------------------------------------------------------------------ *
 * FLAIR TOOLBOX HEAP -- dedicated EXTENDED-MEMORY region above 1 MiB
 * (beads initech-k8o5.5; ADR-0004 DEC-03 / FO-5 / AM-5, RATIFIED 2026-06-19;
 * Rule 8 -- a deliberate locked-data act).
 *
 * THE RULING (DEC-03, verbatim intent): the FLAIR Toolbox allocates its
 * WindowRecords / MenuInfo / ControlRecords / region pools / indexed-8 offscreen
 * bitmaps / NFNT strikes from a DEDICATED high region in extended memory --
 * NOT the per-program DOS MCB arena. The window is FIXED and spec-locked, NOT a
 * runtime-computed base, so the layout is identical every boot (Rule 11
 * determinism; the self-host fixpoint K2==K3 is unaffected). stage2 PROBES
 * installed RAM (INT 15h E820/E801/88h -> boot_info_t.ext_mem_kb) and the kernel
 * PANICS LOUD ("PC LOAD LETTER", Rule 2) below FLAIR_HEAP_MIN -- the probe GATES
 * boot but NEVER alters this map.
 *
 *   FLAIR_HEAP_BASE = 0x00100000   (first byte above the 1 MiB line)
 *   FLAIR_HEAP_SIZE = 0x00400000   (4 MiB)
 *   window          = [0x00100000, 0x00500000)
 *   FLAIR_HEAP_MIN  = 0x00400000   (== FLAIR_HEAP_SIZE: required extended RAM)
 *
 * Law 1 source citations (all local):
 *
 *  (a) ADR-0004 DEC-03 full ruling (Sec 8.1) -- base/size/probe/allocator and
 *      the "FIXED + spec-locked window" mandate; AM-5 (Sec 8.2) -- the ONE
 *      Rule 8 act scoping; FO-5 (Sec 5.2) / FO-G (Sec 8.4) -- the fail-loud
 *      probe gate.
 *
 *  (b) ADR-0001 (386+, 32-bit flat) -- extended memory above 1 MiB is DIRECTLY
 *      addressable in flat 32-bit protected mode. No A20-segment-wraparound
 *      games, no HIMEM/XMS handle gymnastics: the kernel runs with one flat
 *      4 GiB data segment (os/boot/stage2.asm GDT: base 0, limit 0xFFFFF, G=1),
 *      so 0x00100000 is just a pointer. (A20 is already enabled by stage2's
 *      port-0x92 fast-A20 before the PM switch, so the 1 MiB line is not a
 *      wraparound boundary.)
 *
 *  (c) NO-LFB-COLLISION proof (verified by reading os/boot/stage2.asm): the
 *      framebuffer base handed to the kernel (boot_info_t.lfb_addr) is EITHER
 *        - the VBE PhysBasePtr at ModeInfoBlock offset 0x28 (stage2.asm:380),
 *          a PCI aperture FAR above 0xE0000000 on QEMU/SeaBIOS; OR
 *        - 0x000A0000, the standard-VGA mode-0x13 fallback aperture
 *          (stage2.asm:436, the Bochs leg).
 *      Neither lands in [0x00100000, 0x00500000): 0xA0000 < 0x100000, and any
 *      PCI aperture >= 0xE0000000 > 0x500000. The FLAIR window is disjoint from
 *      the LFB on BOTH the VBE and the mode-0x13 paths.
 *
 *  (d) REBOUND-MCB-ARENA DISQUALIFIER: the per-program MCB arena cannot host
 *      GUI state because loader.c int21_mcb_bind_program(arena_base,
 *      arena_ceil) REBINDS the arena to the freshly-loaded program on EVERY
 *      AH=4Bh EXEC (see the ARENA DISJOINTNESS INVARIANT block above). GUI
 *      state (WindowRecords, z-order, visRgn/clipRgn, save-unders, the
 *      EventRecord queue) MUST persist across app launches; coupling it to the
 *      DOS arena would destroy it on the next EXEC -- a deep correctness bug
 *      (Rule 3), not a trade-off. The arena ceiling is also tight
 *      (PROGRAM_ARENA_CEIL == ENV_BLOCK == 0x6F000, already snug for SAMIR),
 *      and the conventional gap is now GONE (0xA0000..0xA0000 = 0 KiB, after
 *      initech-o0td then initech-re30.2 spent BOTH halves of the old 64 KiB gap
 *      on the kernel window) -- it cannot hold even one 307,200-byte indexed-8
 *      640x480 offscreen. Extended memory is the only viable home.
 *
 * DISJOINTNESS FROM THE CONVENTIONAL MAP: the entire conventional layout above
 * lives at or below the kernel stack (0x90000..0xA0000, post initech-re30.2) and
 * the staging window (0x80000..0x90000); the highest conventional address in use
 * is < 0x100000 (kernel-stack top 0x9FFFC < the 1 MiB line).
 * FLAIR_HEAP_BASE == 0x100000 begins ABOVE the 1 MiB line, so [0x100000,
 * 0x500000) is disjoint from PROGRAM_BASE / PROGRAM_IMAGE / ENV_BLOCK / the
 * PROGRAM_STACK region / PROGRAM_ARENA_CEIL / the LOAD_STAGING window by
 * construction. NO existing constant changes (Rule 8 / AM-5).
 *
 * ALLOCATOR (DEFERRED to a later bead, NOT written here): one FLAIR-owned flat
 * arena over [FLAIR_HEAP_BASE, FLAIR_HEAP_BASE+FLAIR_HEAP_SIZE) -- bump-pointer
 * + typed free-list per class, no per-row malloc, fail-loud on exhaustion
 * (PRD Sec 5). This locked-data act defines ONLY the window + the boot probe +
 * the fail-loud gate; no allocator .c may land before its own bead is open. */
#define FLAIR_HEAP_BASE     0x00100000u
#define FLAIR_HEAP_SIZE     0x00400000u
#define FLAIR_HEAP_MIN      0x00400000u

/* Required installed extended memory, in KiB, for the FLAIR heap window to be
 * fully backed by RAM (ADR-0004 DEC-03 / FO-G). Extended memory is reported by
 * stage2 as KiB ABOVE the 1 MiB line (boot_info_t.ext_mem_kb); the top of the
 * FLAIR window is FLAIR_HEAP_BASE + FLAIR_HEAP_SIZE. So the heap is fully backed
 * iff
 *     ext_mem_kb * 1024 + 0x100000  >=  FLAIR_HEAP_BASE + FLAIR_HEAP_SIZE
 * i.e. ext_mem_kb >= (FLAIR_HEAP_BASE + FLAIR_HEAP_SIZE - 0x100000) / 1024.
 * DERIVED from the constants -- NOT hardcoded -- so it tracks any future
 * Rule-8 change to base/size. For the locked window this evaluates to
 *   (0x500000 - 0x100000) / 1024 = 0x400000 / 1024 = 4096 KiB
 * (== FLAIR_HEAP_MIN / 1024). The "- 0x100000" is the 1 MiB line that
 * ext_mem_kb is measured ABOVE, not a hardcoded literal: FLAIR_HEAP_BASE is
 * itself 0x100000, so the window's footprint above 1 MiB is exactly
 * FLAIR_HEAP_BASE - 0x100000 (== 0) + FLAIR_HEAP_SIZE == FLAIR_HEAP_SIZE. */
#define FLAIR_HEAP_REQUIRED_EXT_KB \
    (((FLAIR_HEAP_BASE + FLAIR_HEAP_SIZE) - 0x00100000u) / 1024u)

/* flair_heap_ram_ok -- the PURE RAM-sufficiency decision (ADR-0004 FO-G /
 * DEC-03). Returns 1 (PASS) iff the probed extended memory `ext_mem_kb` is at
 * or above the derived minimum; 0 (FAIL) below it. This is the single point of
 * truth the kernel boot gate and the host oracle BOTH call -- so the mechanical
 * test (harness) and the artifact agree by construction (Law 2). Threshold is
 * DERIVED from the FLAIR constants (FLAIR_HEAP_REQUIRED_EXT_KB), never a magic
 * literal, so it can never drift from the locked window. Pure, side-effect-free,
 * freestanding-safe (no libc, no includes): callable from the kernel and the
 * host test alike. NOT compiled into NASM/ld -- those only cite this header in
 * comments (the constants are duplicated in asm), so a C function here is safe.
 *
 * The comparison is `>=` (at-min PASSES): a machine with EXACTLY
 * FLAIR_HEAP_REQUIRED_EXT_KB of extended memory backs the whole window to its
 * last byte, so it must boot. */
static inline int flair_heap_ram_ok(unsigned long ext_mem_kb)
{
    return (ext_mem_kb >= (unsigned long)FLAIR_HEAP_REQUIRED_EXT_KB) ? 1 : 0;
}

#endif /* INITECH_SPEC_MEMORY_MAP_H */
