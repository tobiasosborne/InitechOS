/* loader.c -- InitechDOS flat program loader (lay out PSP + image, run, return).
 *
 * beads: initech-509.5 (MILTON loader keystone; advances f8v.4). The milestone
 *        where InitechDOS RUNS A PROGRAM for the first time.
 *
 * Ref:   docs/research/psp-loader-ground-truth.md Sec 3 (flat program model +
 *        memory layout), Sec 4 (THE control-transfer + return-to-loader
 *        mechanism), Sec 5 (the baked test program), Sec 7 (Risk 1 the return
 *        path -- the load-bearing, bug-prone part); spec/memory_map.h (LOCKED
 *        addresses); os/milton/psp.h (psp_build); os/milton/int21.h
 *        (int21_set_exit / int21_exit_fn). CLAUDE.md Law 1 (cite), Law 2
 *        (oracle), Law 3 (artifact = C), Rule 2 (fail loud), Rule 3 (root
 *        cause), Rule 8 (specs-as-data), Rule 11 (deterministic), Rule 12
 *        (ASCII).
 *
 * Split for testability (Law 2): loader_prepare() is the PURE, host-testable
 * input-validation + layout + psp_params computation. load_program() adds the
 * kernel-only image copy + PSP build + the asm context save / stack switch /
 * jump / non-returning return-to-loader. In a HOSTED build the asm path is
 * elided (the host oracle exercises loader_prepare directly).
 */

#include "loader.h"
#include "memory_map.h"   /* PROGRAM_BASE / PROGRAM_IMAGE / ENV_BLOCK / ... */
#include "int21.h"        /* int21_set_exit / int21_exit_fn (the repointed hook) */
#include "fat12.h"        /* FAT-sourced load (beads initech-saw): find + read file */
#include "psp.h"          /* psp_save_vectors / psp_load_vectors (beads 509.8)    */
#include "toolbox_gate.h" /* TBX_TENANT_PSP_BYTES -- the tenant block layout (tdnl.14) */
#include "transient.h"    /* TRANSIENT_MAGIC -- the COMMAND.COM overlay is not a .COM (13ig) */
/* InitechMZ parse + flat relocs (beads dtw.1; ADR-0003 DEC-08a). Pulled in via
 * the same-TU INCLUDE (not a separate object) so loader.c carries the mz_* code
 * directly: nothing in the tree links a standalone mz.o (the only other consumer,
 * test_mz.c, also #includes mz.c into its OWN TU), and the kernel object list
 * does not build mz.o. This keeps the existing Makefile recipes -- every kernel
 * image variant + the host loader oracles that link loader.c -- working with NO
 * Makefile edit (CLAUDE.md scope: do not touch the Makefile; Law 3 artifact = C).
 * mz.c is pure + freestanding (-ffreestanding, <stdint.h> only), so inlining it
 * here is identical to compiling it as a peer object. */
/* The complete object joins the kernel-only cold optimization set after the
 * full GPR/context audit (round 8 part 2). The final ELF transfer check and
 * the EXEC family grade the non-local transfer under the actual LTO link.
 * Ref: ADR-0003 DEC-08a.1-.5;
 * docs/design/kernel-size-policy-round2.md Sec 4 (loader_run_plan hazard).
 * The house macro defines this identically for every kernel that links us. */
#include "mz.c"

/* Host-visible bookkeeping used by the real transfer path; IBM DOS 3.30 TR
 * AH=4Bh/4Ch, pp. 6-196, 6-200: child termination resumes its invoking process. */
loader_status_t loader_nest_enter(loader_state_t *active, loader_state_t *saved,
                                  uintptr_t psp, void *return_frame)
{
    if (active->depth >= LOADER_EXEC_DEPTH) return LOADER_ERR_BUSY;
    *saved = *active;
    active->psp = psp;
    active->return_frame = return_frame;
    active->depth++;
    return LOADER_OK;
}

void loader_nest_leave(loader_state_t *active, const loader_state_t *saved)
{
#ifndef LOADER_MUT_NO_RESTORE
    active->psp = saved->psp;
#endif
    active->return_frame = saved->return_frame;
    active->depth = saved->depth;
}

/* ------------------------------------------------------------------------ *
 * Pure prep: validate + lay out + compute psp_params (HOST-TESTABLE).
 * ------------------------------------------------------------------------ */

/* loader_prepare_core -- the shared validation + layout + psp_params + arena
 * computation behind BOTH loader_prepare (baked path: image copied later) and
 * loader_prepare_in_place (FAT path: image already at PROGRAM_IMAGE; beads
 * initech-za4m). `in_place` selects whether an image pointer is expected:
 *
 *   in_place == 0 (baked):    `image` MUST be non-NULL; out->image_src = image.
 *                             load_program copies image -> PROGRAM_IMAGE.
 *   in_place == 1 (FAT/za4m):  `image` is the RESIDENT image (looked at, never
 *                              copied; NULL is a caller bug); out->image_src = NULL.
 *                             The bytes are ALREADY at PROGRAM_IMAGE -- no copy.
 *
 * EITHER way the ONLY size bound is PROGRAM_IMAGE_MAX (~188 KiB = ENV_BLOCK -
 * PROGRAM_IMAGE). The old FAT path also rejected image_len > LOAD_STAGING_MAX
 * (64 KiB), because it staged the .COM through [0x80000,0x90000) before copying
 * down; za4m reads straight into PROGRAM_IMAGE, so that 64 KiB cap is GONE and
 * a 77 KiB SAMIR.COM now loads (it used to fail LOADER_ERR_TOO_BIG). */
int loader_image_is_transient(const uint8_t *image, uint32_t image_len)
{
#ifndef LOADER_MUT_NO_TRANSIENT_GUARD
    uint32_t m = 0u;
    if (image == 0 || image_len < TRANSIENT_MAGIC_LEN) {
        return 0;
    }
    while (m < TRANSIENT_MAGIC_LEN && image[m] == (uint8_t)TRANSIENT_MAGIC[m]) {
        m++;
    }
    return m == TRANSIENT_MAGIC_LEN;
#else
    /* MUTANT (Rule 6; make test-flair-execguard-mutant / test-loader-mutant
     * only): the pre-13ig loader never recognises the overlay and runs its
     * header as code. NEVER in a real build. */
    (void)image; (void)image_len;
    return 0;
#endif
}

static loader_status_t loader_prepare_core(const uint8_t *image,
                                           uint32_t image_len,
                                           const char *cmd_tail,
                                           uint32_t cmd_tail_len, int in_place,
                                           loader_plan_t *out)
{
    /* Rule 2: fail loud on every bad input. The loader NEVER copies a NULL or
     * oversized image into the program region -- that would clobber the kernel
     * stack / framebuffer and silently corrupt the machine. */
    if (out == 0) {
        return LOADER_ERR_NULL_OUT;
    }
    /* The baked path needs source bytes to copy FROM; the in-place path needs
     * the resident bytes to LOOK AT (the content refusal below) -- a NULL in
     * either mode is a caller bug, never a program (initech-13ig). */
    if (image == 0) {
        return LOADER_ERR_NULL_IMAGE;
    }
    if (image_len == 0) {
        return LOADER_ERR_ZERO_LEN;
    }
    if (image_len > PROGRAM_IMAGE_MAX) {
        /* Would overrun the program region into the env/stack (Sec 3.2 gap
         * proof). This is now the SOLE size bound on the FAT path too (za4m:
         * the 64 KiB LOAD_STAGING_MAX cap is gone -- the .COM no longer transits
         * the 64 KiB staging buffer; it is read straight into PROGRAM_IMAGE). */
        return LOADER_ERR_TOO_BIG;
    }
#ifdef LOADER_MUTATE_REIMPOSE_STAGING_CAP
    /* MUTANT (CLAUDE.md Rule 6; make test-loader-big-mutant only): re-impose the
     * PRE-za4m 64 KiB LOAD_STAGING_MAX cap on the in-place path -- the EXACT bug
     * that wrongly rejected a 77 KiB SAMIR.COM. The big-.COM oracle's "64 KiB + 1
     * loads" case MUST go RED. NEVER define in a real build. */
    if (in_place && image_len > LOAD_STAGING_MAX) {
        return LOADER_ERR_TOO_BIG;
    }
#endif

    /* THE TRANSIENT IS NOT A PROGRAM (bead initech-13ig). This is the ONE
     * choke point every flat image passes before loader_run_plan's JMP --
     * baked (load_program), FAT top-level and nested (load_program_from_fat ->
     * load_program_in_place / loader_prepare_in_place) -- so the flagship's
     * COMMAND.COM overlay (os/milton/transient.h: a COMMAND3 header linked at
     * TRANSIENT_BASE, not a .COM), under its own name or any copy, is refused
     * here: DOS EXEC error 11 "Invalid format" (IBM DOS 3.30 TR p. 6-200) at
     * the EXEC seam, dBASE error 92 above it. The kernel's EXEC-by-name route
     * to the RESIDENT interpreter lives in kmain.c and never reaches here. */
    if (loader_image_is_transient(image, image_len)) {
        return LOADER_ERR_BAD_FORMAT;
    }

    /* Layout from the LOCKED spec (spec/memory_map.h; Sec 3.2). */
    out->psp_addr  = PROGRAM_BASE;
#ifdef LOADER_MUTATE_NO_OFFSET
    /* MUTANT (CLAUDE.md Rule 6; make test-loader-mutant only): load the image at
     * PROGRAM_BASE instead of PROGRAM_BASE+0x100 -- destroying the authentic
     * .COM offset. The layout oracle MUST go RED. NEVER define in a real build. */
    out->image_dst = PROGRAM_BASE;
    out->entry     = PROGRAM_BASE;
#else
    out->image_dst = PROGRAM_IMAGE;   /* PSP + 0x100 -- the authentic .COM offset */
    out->entry     = PROGRAM_IMAGE;   /* flat entry EIP == image_dst              */
#endif
    out->stack_top = PROGRAM_STACK_TOP;
    out->image_len = image_len;
    /* In-place (za4m): no source to copy FROM -- the bytes are already at
     * out->image_dst (PROGRAM_IMAGE). load_program_in_place skips the copy and
     * MUST see a NULL image_src so a stray copy can never be attempted. */
    out->image_src = in_place ? (const uint8_t *)0 : image;

    /* The psp_build inputs (Sec 2.2 / 2.5 / 2.7 / 2.11; psp.h). The vestigial
     * segment fields are passed as flat LINEAR addresses; psp_build stores each
     * as a fake paragraph (linear >> 4). */
    out->params.alloc_end_linear  = PROGRAM_ALLOC_END;  /* 0x80000 -> seg 0x8000 */
    out->params.env_linear        = ENV_BLOCK;          /* 0x6F000 -> seg 0x6F00 (2og) */
    out->params.parent_psp_linear = 0u;                 /* no parent PSP yet (Sec 2.5) */
    out->params.cmd_tail          = cmd_tail;           /* may be NULL (no args) */
    out->params.cmd_tail_len      = cmd_tail_len;
    /* JFT inheritance (beads initech-bsy.9): the PURE prep is parent-agnostic
     * (it has no live PSP), so default to the CON standard handles (NULL). The
     * kernel loader_run_plan overrides this from g_cur_psp->jft at EXEC time --
     * exactly the pattern loader_decide_env uses for env inheritance. */
    out->params.parent_jft        = 0;

    /* Compute the AH=48h heap-arena window DISJOINT from the loaded program (beads
     * initech-1q4u; ADR-0009 DEC-04). The arena starts paragraph-rounded ABOVE the
     * loaded image + a conservative BSS reserve, and ends at PROGRAM_ARENA_CEIL
     * (== ENV_BLOCK), so a 48h ALLOC can never return memory inside the program
     * image+BSS, the env block, or the 64 KiB stack. See spec/memory_map.h's ARENA
     * DISJOINTNESS INVARIANT for the proof. (Was: the arena overlaid the running
     * program -- the latent corruption SAMIR would have hit.) */
    {
#ifdef LOADER_MUTATE_ARENA_OVERLAP
        /* MUTANT (CLAUDE.md Rule 6; make test-arena-disjoint-mutant only): revert
         * the arena base to PROGRAM_BASE -- the EXACT pre-fix bug (the arena
         * overlays the PSP/image/stack). The disjointness oracle MUST go RED
         * (an allocation lands inside [PROGRAM_IMAGE, image_end) or the stack).
         * NEVER define in a real build. */
        uint32_t arena_base = PROGRAM_BASE;
#else
        /* roundup_paragraph(PROGRAM_IMAGE + image_len + PROGRAM_BSS_RESERVE). The
         * +0xF then mask-to-16 rounds UP to the next paragraph boundary so the DOS
         * segment math (base_linear >> 4) is exact. */
        uint32_t image_end  = PROGRAM_IMAGE + image_len;
        uint32_t arena_base = (image_end + PROGRAM_BSS_RESERVE + 0xFu) & ~0xFu;
#endif
        uint32_t arena_ceil = PROGRAM_ARENA_CEIL;   /* == ENV_BLOCK (exclusive) */

        if (arena_base < arena_ceil &&
            (arena_ceil - arena_base) >= 32u) {     /* >= 2 paragraphs (hdr+data) */
            out->arena_base    = arena_base;
            out->arena_ceil    = arena_ceil;
            out->arena_present = 1u;
        } else {
            /* Image too large to host a heap below the ceiling: no arena (fail
             * loud -- 48h reports insufficient memory, never a corrupting
             * overlap). The image itself already fit (PROGRAM_IMAGE_MAX check
             * above); this only denies the *heap*, not the run. */
            out->arena_base    = 0u;
            out->arena_ceil    = 0u;
            out->arena_present = 0u;
        }
    }

    return LOADER_OK;
}

loader_status_t loader_prepare(const uint8_t *image, uint32_t image_len,
                               const char *cmd_tail, uint32_t cmd_tail_len,
                               loader_plan_t *out)
{
    /* Baked path: image copied to PROGRAM_IMAGE by load_program (in_place = 0). */
    return loader_prepare_core(image, image_len, cmd_tail, cmd_tail_len,
                               /*in_place=*/0, out);
}

loader_status_t loader_prepare_in_place(const uint8_t *resident, uint32_t image_len,
                                        const char *cmd_tail, uint32_t cmd_tail_len,
                                        loader_plan_t *out)
{
    /* FAT path (beads initech-za4m): the .COM is ALREADY resident (at
     * PROGRAM_IMAGE, or a nested child's PSP+100h). Nothing is copied; the
     * bytes are looked at only for the content refusal (initech-13ig); the only
     * size bound is PROGRAM_IMAGE_MAX. */
    return loader_prepare_core(resident, image_len, cmd_tail,
                               cmd_tail_len, /*in_place=*/1, out);
}

/* ------------------------------------------------------------------------ *
 * InitechMZ (.EXE) PROLOGUE -- the pure, host-testable content path (beads
 * initech-wczy / dtw.2; ADR-0003 DEC-08a). Mutates the in-place buffer (memmove
 * the load module over the header + apply flat relocs) and produces the SAME
 * loader_plan_t loader_run_plan consumes -- a prologue on the single transfer
 * path (Rule 3). See loader.h for the full contract.
 * ------------------------------------------------------------------------ */

/* Forward (low-address-first) byte move for the load-module down-shift. The MZ
 * load module is moved from file_at+header_bytes DOWN to file_at (dst < src), so
 * a forward copy can never clobber a not-yet-read source byte (dst[i] is written
 * before src[i+k] for any k>0 is read). Freestanding (loader.c pulls in no libc;
 * Rule 11 deterministic). NOT a general memmove -- it assumes dst <= src, which
 * the down-shift always satisfies. */
static void loader_move_down(uint8_t *dst, const uint8_t *src, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

loader_status_t loader_prepare_mz(uint8_t *file_at, uint32_t file_len,
                                  const char *cmd_tail, uint32_t cmd_tail_len,
                                  loader_plan_t *out)
{
    mz_image_t img;
    int ps;

    if (out == 0) {
        return LOADER_ERR_NULL_OUT;
    }
    if (file_at == 0 || file_len == 0u) {
        return LOADER_ERR_BAD_FORMAT;   /* nothing to parse -- fail loud (Rule 2) */
    }

    /* 1. Parse + validate the MZ container (DEC-08a.1/.5). The pure prep NEVER
     *    panics: it maps MZ_ERR_FOREIGN to a DISTINCT status the KERNEL call site
     *    panics on (DEC-08a.5), and every other parse error to bad-format. This
     *    keeps the panic at the kernel seam (host-observable here, Law 2). */
    ps = mz_parse_header(file_at, file_len, &img);
    if (ps == MZ_ERR_FOREIGN) {
        /* DEC-08a.5: a genuine untagged 16-bit DOS MZ. The kernel will PANIC
         * (loader_panic_foreign_mz) rather than relocate-and-misexecute 16-bit
         * code in 32-bit flat mode. NEVER produce a runnable plan. */
        return LOADER_ERR_FOREIGN_MZ;
    }
    if (ps != MZ_OK) {
        return LOADER_ERR_BAD_FORMAT;   /* truncated / bad header / OOB reloc */
    }

    /* 2. Bound-check the LOAD MODULE against the program region (DEC-08a.2). The
     *    module lands at PROGRAM_IMAGE; it must fit below the env/stack exactly
     *    like a flat .COM. (load_module_len, not file_len: the header + reloc
     *    table are dropped, so a file slightly over the cap may still fit.) */
    if (img.load_module_len == 0u || img.load_module_len > PROGRAM_IMAGE_MAX) {
        return LOADER_ERR_BAD_FORMAT;
    }
    /* The load module must lie wholly within the resident file (mz_parse_header
     * already proved image_bytes <= file_len, so this is belt-and-braces, Rule 2). */
    if (img.load_module_off + img.load_module_len < img.load_module_off ||
        img.load_module_off + img.load_module_len > file_len) {
        return LOADER_ERR_BAD_FORMAT;
    }

    /* 3. Apply flat-32 relocations to the module IN ITS ORIGINAL FILE POSITION
     *    (at file_at + load_module_off), reading the reloc table from its original
     *    file offset -- THEN move. Relocating BEFORE the move makes the order
     *    robust to ANY reloc-table placement: a real MZ commonly puts the reloc
     *    table in the header gap (reloc_table_off < load_module_off), which the
     *    down-move's DESTINATION window [0, load_module_len) would overwrite if we
     *    relocated after moving. Applying first -- while header, reloc table, and
     *    module all sit untouched at their file offsets -- can never read a
     *    clobbered fixup (Rule 3: fix the root cause, not a placement-specific
     *    band-aid). The reloc offsets are MODULE-RELATIVE, so they index the module
     *    at load_module_off here exactly as they will at the load base after the
     *    move; mz_apply_relocs adds PROGRAM_IMAGE (the flat load base, DEC-08a.1),
     *    not the staging address, so the dwords are correct for the run location. */
#ifdef LOADER_MUTATE_MZ_NO_RELOC
    /* MUTANT (CLAUDE.md Rule 6; make test-mzload-mutant only): SKIP the flat-32
     * relocation entirely. The load module's absolute references keep their
     * unrelocated (link-time) value, so the relocated-dword assertion in
     * test_mzload.c (dword == original + PROGRAM_IMAGE) MUST go RED. This is the
     * EXACT bug -- a relocatable MZ run without its fixups -- the oracle exists to
     * catch. NEVER define in a real build. */
    (void)0;
#else
    if (img.reloc_count > 0u) {
        int rs = mz_apply_relocs(file_at + img.load_module_off,
                                 img.load_module_len,
                                 (uint32_t)PROGRAM_IMAGE,
                                 file_at + img.reloc_table_off, img.reloc_count);
        if (rs != MZ_OK) {
            return LOADER_ERR_BAD_FORMAT;   /* OOB reloc -- fail loud (Rule 2) */
        }
    }
#endif

    /* 4. Move the (now-relocated) LOAD MODULE down over the header to the flat
     *    load base (PROGRAM_IMAGE == file_at). DEC-08a.2: the MZ header is NEVER
     *    part of the loaded image. dst (file_at) <= src (file_at+load_module_off),
     *    so the forward loader_move_down can never clobber an unread source byte.
     *    A zero header (load_module_off == 0; degenerate) needs no move. */
    if (img.load_module_off != 0u) {
        loader_move_down(file_at, file_at + img.load_module_off,
                         img.load_module_len);
    }

    /* 5. Lay out the plan exactly like the in-place .COM path (PSP @ PROGRAM_BASE,
     *    image @ PROGRAM_IMAGE, the heap arena DISJOINT above the load module).
     *    image_len is the LOAD MODULE length, so the arena base sits above the
     *    relocated module, not the whole file (DEC-08a.3). */
    {
        /* The relocated load module is RESIDENT at file_at (== PROGRAM_IMAGE)
         * after step 4, so it is the in-place image the core looks at for the
         * content refusal (initech-13ig); the core still records image_src ==
         * NULL for every in-place plan. A NULL here has been LOADER_ERR_NULL_IMAGE
         * since 7850d4d2 and refused EVERY InitechMZ (initech-zxzi). */
        loader_status_t cs = loader_prepare_core(file_at,
                                                 img.load_module_len, cmd_tail,
                                                 cmd_tail_len, /*in_place=*/1, out);
        if (cs != LOADER_OK) {
            return cs;
        }
    }

    /* 6. Flat MZ entry (DEC-08a.2): entry = PROGRAM_IMAGE + (e_cs*16 + e_ip). The
     *    canonical InitechMZ emits cs=ip=0 (entry == load base, identical to a
     *    .COM), but a non-zero entry_off is honored. ESP stays PROGRAM_STACK_TOP
     *    (loader_prepare_core set it); e_ss:e_sp are advisory this release. */
#ifdef LOADER_MUTATE_MZ_ENTRY_EQUAL
    if (img.entry_off > img.load_module_len) {
#else
    if (img.entry_off >= img.load_module_len) {
#endif
        /* The entry must point INTO the loaded module (Rule 2: never JMP past it). */
        return LOADER_ERR_BAD_FORMAT;
    }
    out->entry = (uint32_t)PROGRAM_IMAGE + img.entry_off;

    /* 7. e_minalloc TEETH (DEC-08a.3): the program declares it needs at least
     *    min_alloc_paras paragraphs of heap PAST its image. If the disjoint arena
     *    cannot satisfy that minimum, fail loud (DOS 08h insufficient memory) --
     *    never run a heap-starved program. e_maxalloc (typically 0xFFFF) is
     *    naturally clamped: the arena already ends at PROGRAM_ARENA_CEIL, so a
     *    max-hungry request can never reach past the ceiling. */
    {
        uint32_t need_bytes = (uint32_t)img.min_alloc_paras * 16u;
        uint32_t have_bytes = out->arena_present
                                  ? (out->arena_ceil - out->arena_base)
                                  : 0u;
        if (need_bytes > have_bytes) {
            return LOADER_ERR_BAD_FORMAT;   /* maps to DOS 08h at the EXEC seam */
        }
    }

    return LOADER_OK;
}

/* loader_decide_env -- the PURE, host-testable EXEC env-inheritance decision
 * (beads initech-1i0x Tranche E inc 3). Given the caller's env_block (the flat
 * linear address of the env block the child inherits, 0 = inherit-empty), compute:
 *   out->env_linear  = the flat addr to store in the child PSP env_seg, and
 *   out->write_empty = 1 iff the loader must synthesize the 2-byte empty block at
 *                      ENV_BLOCK (the legacy / inherit-empty path).
 *
 * Two cases:
 *   env_block == 0 (inherit-empty / baked path): env_linear = ENV_BLOCK,
 *       write_empty = 1 -- byte-identical legacy: the loader writes the 2-byte
 *       double-NUL empty block and the child PSP env_seg points at it.
 *   env_block == ENV_BLOCK (the shell populated the region): env_linear =
 *       env_block, write_empty = 0 -- the loader must NOT overwrite the shell's
 *       serialized env; the child PSP env_seg points at the populated block.
 * A non-zero env_block that is NOT ENV_BLOCK is a caller bug (the loader never
 * wrote that region) -> LOADER_ERR_BAD_ENV (Rule 2 fail loud).
 *
 * Split out as a pure function (NO asm, NO absolute-address writes) so the host
 * oracle drives the inheritance decision directly (loader_run_plan -- the asm
 * stack-switch path -- is kernel-only and cannot run hosted). loader_run_plan
 * calls THIS to choose, so the kernel and the oracle agree by construction (Law 2). */
loader_status_t loader_decide_env(uint32_t env_block, loader_env_decision_t *out)
{
    if (out == 0) {
        return LOADER_ERR_NULL_OUT;
    }
    if (env_block != 0u && env_block != (uint32_t)ENV_BLOCK) {
        return LOADER_ERR_BAD_ENV;   /* an env at an address the loader never wrote */
    }
#ifdef LOADER_MUTATE_FORCE_EMPTY_ENV
    /* MUTANT (CLAUDE.md Rule 6; make test-exec-env-mutant only): IGNORE the provided
     * env_block and ALWAYS choose the empty-block path -- the exact pre-inc-3 bug
     * (every child gets an empty env, the shell's master env never inherited). The
     * inheritance oracle (a populated env_block must yield write_empty=0 and
     * env_linear==env_block) MUST go RED. NEVER define in a real build. */
    out->env_linear  = (uint32_t)ENV_BLOCK;
    out->write_empty = 1;
#else
    if (env_block == 0u) {
        out->env_linear  = (uint32_t)ENV_BLOCK;   /* inherit-empty: PSP -> ENV_BLOCK */
        out->write_empty = 1;                     /* synthesize the 2-byte empty block */
    } else {
        out->env_linear  = env_block;             /* inherit the populated block      */
        out->write_empty = 0;                     /* do NOT overwrite the shell's env  */
    }
#endif
    return LOADER_OK;
}

/* ------------------------------------------------------------------------ *
 * loader_prepare_tenant -- the DEC-08a PARAMETERIZED-BASE extension for disk-
 * launched FLAIR tenants (bead initech-tdnl.14; reconciliation Part B
 * DEC-AC3-1 + Part A.1/A.4 + Part C item 3). PURE: no I/O, no absolute-address
 * writes outside the caller's block, host-testable (test_tenant_load.c).
 *
 * Same five DEC-08a steps as loader_prepare_mz, with the two hardcoded
 * placements replaced by the explicit pair (block_linear, block_linear +
 * TBX_TENANT_PSP_BYTES) -- the EXEC path above is not touched:
 *   1. parse (foreign -> LOADER_ERR_FOREIGN_MZ; the kernel panics, DEC-08a.5);
 *   2. bound the module against the BLOCK (not PROGRAM_IMAGE_MAX);
 *   3. relocate IN FILE POSITION against load_base (mz_apply_relocs already
 *      takes load_base as a free parameter -- zero new reloc arithmetic);
 *   4. move the module down over the header to load_base;
 *   5. entry = load_base + entry_off, bounded inside the module;
 * plus the two tenant-specific facts: the e_minalloc BSS must fit the block
 * (refused, never overrun) and is ZEROED (a carved heap block is not); and the
 * PSP is built at `block` with params derived only from the caller's numbers
 * -- psp_build writes no address-derived field, which the host fixture proves
 * by building the same params at two addresses and comparing bytes.
 * ------------------------------------------------------------------------ */
loader_status_t loader_prepare_tenant(uint8_t *block, uint32_t block_len,
                                      uint32_t file_len, uint32_t block_linear,
                                      tenant_plan_t *out)
{
    mz_image_t img;
    int ps;
    uint8_t *file_at;
    uint32_t room;
    uint32_t bss;

    if (out == 0) {
        return LOADER_ERR_NULL_OUT;
    }
    if (block == 0 || block_len <= (uint32_t)TBX_TENANT_PSP_BYTES ||
        file_len == 0u ||
        file_len > block_len - (uint32_t)TBX_TENANT_PSP_BYTES) {
        return LOADER_ERR_BAD_FORMAT;
    }
    file_at = block + TBX_TENANT_PSP_BYTES;
    room    = block_len - (uint32_t)TBX_TENANT_PSP_BYTES;

    /* V1 disk GUI tenants are InitechMZ ONLY (D1-1): a flat .COM is position-
     * dependent and cannot be placed at a heap-chosen base. */
    if (!mz_is_mz(file_at, file_len)) {
        return LOADER_ERR_BAD_FORMAT;
    }

    /* 1. Parse (DEC-08a.1/.5). */
    ps = mz_parse_header(file_at, file_len, &img);
    if (ps == MZ_ERR_FOREIGN) {
        return LOADER_ERR_FOREIGN_MZ;
    }
    if (ps != MZ_OK) {
        return LOADER_ERR_BAD_FORMAT;
    }

    /* 2. Bound the module against the BLOCK. */
    if (img.load_module_len == 0u || img.load_module_len > room ||
        img.load_module_off + img.load_module_len < img.load_module_off ||
        img.load_module_off + img.load_module_len > file_len) {
        return LOADER_ERR_BAD_FORMAT;
    }
    bss = (uint32_t)img.min_alloc_paras * 16u;
    if (bss > room - img.load_module_len) {
        return LOADER_ERR_BAD_FORMAT;   /* e_minalloc does not fit: refuse */
    }
    if (img.entry_off >= img.load_module_len) {
        return LOADER_ERR_BAD_FORMAT;   /* never JMP past the module (Rule 2) */
    }

    /* 3. Relocate in file position against the PARAMETERIZED load base. */
#ifdef LOADER_MUTATE_TENANT_PROGRAM_IMAGE_BASE
    /* MUTANT (Rule 6; test-tenant-load-mutant only): relocate against the
     * EXEC path's PROGRAM_IMAGE instead of the block -- the exact "reinterpret
     * DEC-08a instead of extending it" bug. The relocated-dword assertions of
     * test_tenant_load.c MUST go RED. NEVER define in a real build. */
    if (img.reloc_count > 0u) {
        if (mz_apply_relocs(file_at + img.load_module_off, img.load_module_len,
                            (uint32_t)PROGRAM_IMAGE,
                            file_at + img.reloc_table_off,
                            img.reloc_count) != MZ_OK) {
            return LOADER_ERR_BAD_FORMAT;
        }
    }
#else
    if (img.reloc_count > 0u) {
        if (mz_apply_relocs(file_at + img.load_module_off, img.load_module_len,
                            block_linear + (uint32_t)TBX_TENANT_PSP_BYTES,
                            file_at + img.reloc_table_off,
                            img.reloc_count) != MZ_OK) {
            return LOADER_ERR_BAD_FORMAT;   /* OOB reloc -- fail loud */
        }
    }
#endif

    /* 4. Move the module down over the header to the load base. */
    if (img.load_module_off != 0u) {
        loader_move_down(file_at, file_at + img.load_module_off,
                         img.load_module_len);
    }
    /* Zero everything past the module inside the block: the e_minalloc BSS and
     * the dropped header/reloc-table bytes (a heap block is not zeroed). */
    for (uint32_t i = img.load_module_len; i < room; i++) {
        file_at[i] = 0u;
    }

    /* 5. The plan + the PSP at the EXPLICIT address. */
    out->psp_addr   = block_linear;
    out->load_base  = block_linear + (uint32_t)TBX_TENANT_PSP_BYTES;
    out->module_len = img.load_module_len;
    out->entry      = out->load_base + img.entry_off;
    out->block_len  = block_len;
    out->params.alloc_end_linear  = block_linear + block_len;
    out->params.env_linear        = ENV_BLOCK;   /* inherit-empty (S4 note) */
    out->params.parent_psp_linear = 0u;          /* the kernel is the parent */
    out->params.cmd_tail          = 0;
    out->params.cmd_tail_len      = 0u;
    out->params.parent_jft        = 0;           /* the CON standard handles */
    (void)psp_build((psp_t *)(void *)block, &out->params);
    return LOADER_OK;
}

/* ------------------------------------------------------------------------ *
 * AH=4Bh AL=03h LOAD OVERLAY core (ADR-0003 Amendment DEC-19; see loader.h).
 * Pure: every file byte comes through `rd`, every ownership question through
 * `owns`, so the host oracle drives it over an in-memory file and a fake
 * window. PM ruling on cb3c7910 (initech-f9t): prove every validation before
 * writing, then stream the copy/fixups with at most the existing sector
 * buffers. Only a device read error can return after copying begins.
 * Ref: DEC-19.3/.4, TR p. 11-5 (the caller's allocated footprint).
 * ------------------------------------------------------------------------ */
#define LOADER_OVL_CHUNK (LOADER_OVL_SCRATCH / 4u)

/* First pass validates every offset; second pass applies the SAME table.
 * The reader contract requires stable file bytes/FAT throughout the call.
 * An inconsistent second pass is an invariant violation, never a normal
 * validation refusal after modifying the caller's memory (Rule 2). */
static loader_status_t loader_ovl_relocs(loader_ovl_read_fn rd, void *ctx,
                                         const mz_image_t *img, uint8_t *dest,
                                         uint32_t reloc_factor,
                                         uint8_t *scratch, int apply)
{
    uint32_t done = 0u;
    while (done < img->reloc_count) {
        uint32_t n = img->reloc_count - done;
        if (n > LOADER_OVL_CHUNK) n = LOADER_OVL_CHUNK;
        if (rd(ctx, img->reloc_table_off + done * 4u, n * 4u, scratch) != 0) {
            return LOADER_ERR_READ;
        }
        if (apply) {
#ifndef LOADER_MUTATE_OVL_NO_FACTOR
            if (mz_apply_relocs(dest, img->load_module_len, reloc_factor,
                                scratch, (uint16_t)n) != MZ_OK) {
                __builtin_trap(); /* source changed after validation */
            }
#endif
        } else {
            for (uint32_t i = 0u; i < n; i++) {
                uint32_t off = mz_rd32(scratch + i * 4u);
                if (off > img->load_module_len || img->load_module_len - off < 4u)
                    return LOADER_ERR_BAD_FORMAT;
            }
        }
#ifdef LOADER_MUTATE_OVL_NO_FACTOR
        (void)dest; (void)reloc_factor;
#endif
        done += n;
    }
    return LOADER_OK;
}

loader_status_t loader_overlay_core(loader_ovl_read_fn rd, void *ctx,
                                    uint32_t file_len, uint8_t *dest,
                                    uint32_t load_linear, uint32_t reloc_factor,
                                    loader_ovl_owns_fn owns,
                                    loader_ovl_check_fn check, uint8_t *scratch,
                                    uint32_t *out_len)
{
    mz_image_t img;
    uint32_t len = file_len, src = 0u;
    uint32_t head = file_len < MZ_MIN_HDR_READ ? file_len : MZ_MIN_HDR_READ;
    int is_mz = 0;
    loader_status_t st;

    if (rd == 0 || owns == 0 || dest == 0 || check == 0 ||
        scratch == 0 || out_len == 0) {
        return LOADER_ERR_NULL_OUT;
    }
    if (head != 0u && rd(ctx, 0u, head, scratch) != 0) {
        return LOADER_ERR_READ;
    }
    if (mz_is_mz(scratch, head)) {
        /* mz_parse_header reads only bytes [0, MZ_MIN_HDR_READ) of its buffer
         * and checks every extent against file_len (mz.c steps 2-9), so the
         * header-only scratch copy is a sufficient view of the whole file. */
        int ps = mz_parse_header(scratch, file_len, &img);
        if (ps == MZ_OK) {
            is_mz = 1;
            src   = img.load_module_off;
            len   = img.load_module_len;
        } else if (ps == MZ_ERR_FOREIGN) {
#ifndef LOADER_MUTATE_OVL_ACCEPT_FOREIGN
            return LOADER_ERR_FOREIGN_MZ;   /* DEC-08a.5: the kernel panics */
#endif
            /* MUTANT (Rule 6): the untagged 16-bit MZ falls through and is
             * copied as a raw file. NEVER define in a real build. */
        } else {
            return LOADER_ERR_BAD_FORMAT;
        }
    }
    if (!owns(load_linear, len)) {
        return LOADER_ERR_NOT_OWNED;        /* header read; destination untouched */
    }
    if (check(ctx) != 0) return LOADER_ERR_READ; /* the WHOLE declared chain */
#ifdef LOADER_MUTATE_OVL_COPY_FIRST
    /* Rule 6: copy before the relocation validation, violating PM's boundary. */
    if (len != 0u && rd(ctx, src, len, dest) != 0) return LOADER_ERR_READ;
#endif
    if (is_mz) {
        st = loader_ovl_relocs(rd, ctx, &img, dest, reloc_factor, scratch, 0);
        if (st != LOADER_OK) {
            return st;
        }
    }
    /* Last validation before ANY write. The source and ownership stay stable
     * during this non-reentrant DOS call; the cached FAT is read-only here. */
    if (!owns(load_linear, len)) return LOADER_ERR_NOT_OWNED;
    if (len != 0u && rd(ctx, src, len, dest) != 0) return LOADER_ERR_READ;
    if (is_mz) {
        st = loader_ovl_relocs(rd, ctx, &img, dest, reloc_factor, scratch, 1);
        if (st != LOADER_OK) return st; /* only a device read error */
    }
    *out_len = len;
    return LOADER_OK;
}

/* ------------------------------------------------------------------------ *
 * Kernel-only: image copy + PSP build + control transfer + return-to-loader.
 *
 * Compiled into the kernel (freestanding). In a HOSTED build (__STDC_HOSTED__)
 * the body is a stub that only runs loader_prepare so the test TU links; the
 * oracle never calls the asm path.
 * ------------------------------------------------------------------------ */

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__

/* Hosted stub: validate only; do NOT touch the (host's) absolute addresses or
 * attempt the asm jump. The host oracle drives loader_prepare directly. */
loader_status_t load_program(const uint8_t *image, uint32_t image_len,
                             const char *cmd_tail, uint32_t cmd_tail_len,
                             uint8_t *out_exit_code)
{
    loader_plan_t plan;
    loader_status_t st = loader_prepare(image, image_len, cmd_tail,
                                        cmd_tail_len, &plan);
    (void)out_exit_code;
    return st;
}

/* Hosted stub: validate the in-place layout only (beads initech-za4m). The host
 * big-.COM oracle (test_loader_big.c) drives loader_prepare_in_place directly to
 * prove the PROGRAM_IMAGE_MAX bound; it never reaches the asm jump. */
loader_status_t load_program_in_place(uint32_t image_len, const char *cmd_tail,
                                      uint32_t cmd_tail_len, uint32_t env_block,
                                      uint8_t *out_exit_code)
{
    loader_plan_t plan;
    /* The host has no PROGRAM_IMAGE to look at: the resident bytes are NULL,
     * so this stub reports LOADER_ERR_NULL_IMAGE (the content refusal is
     * host-tested through loader_prepare_in_place directly, test_loader.c). */
    loader_status_t st = loader_prepare_in_place((const uint8_t *)0, image_len,
                                                 cmd_tail, cmd_tail_len, &plan);
    (void)env_block;
    (void)out_exit_code;
    return st;
}

/* Hosted stub: the FAT-sourced load is kernel-only (it pulls the volume + the
 * asm transfer). The host oracle exercises the AH=4Bh register/validation logic
 * through int21's MOCK EXEC backend (test_exec.c), never this path. The shared-
 * FAT-cache signature (fat/fat_len) mirrors the kernel definition (beads
 * y206/headroom): the loader aliases the file backend's FAT instead of a second
 * 6 KiB copy. */
void loader_bind_fat_volume(const struct fat12_volume *vol,
                            const uint8_t *fat, uint32_t fat_len)
{ (void)vol; (void)fat; (void)fat_len; }

loader_status_t load_program_from_fat(const char *name83, uint16_t dir_start,
                                      const char *cmd_tail, uint32_t cmd_tail_len,
                                      uint32_t env_block, uint8_t *out_rc)
{
    (void)name83; (void)dir_start; (void)cmd_tail; (void)cmd_tail_len;
    (void)env_block; (void)out_rc;
    return LOADER_ERR_NO_VOLUME;
}

/* Hosted stub (bead initech-tdnl.14): the FAT read + carve are kernel-only; the
 * host oracle drives the pure loader_prepare_tenant directly. */
loader_status_t loader_load_tenant(const char *name83, uint16_t dir_start,
                                   loader_carve_fn carve, void *user,
                                   void **out_block, tenant_plan_t *out)
{
    (void)name83; (void)dir_start; (void)carve; (void)user; (void)out;
    if (out_block) *out_block = 0;
    return LOADER_ERR_NO_VOLUME;
}

/* Hosted stub (ADR-0003 Amendment DEC-19): the FAT find is kernel-only; the
 * host oracle (test_overlay.c) drives the pure loader_overlay_core directly. */
loader_status_t loader_load_overlay(const char *name83, uint16_t dir_start,
                                    uint32_t load_linear, uint32_t reloc_factor,
                                    loader_ovl_owns_fn owns)
{
    (void)name83; (void)dir_start; (void)load_linear; (void)reloc_factor;
    (void)owns;
    return LOADER_ERR_NO_VOLUME;
}

#else /* freestanding kernel build */

#include "idt.h"          /* idt_get_gate / idt_install_trap -- live IDT vectors */
#include "io.h"           /* inb / outb -- the foreign-MZ panic serial dump (dtw.2) */
#include "panic.h"        /* typed physical-screen refusal (DEC-08a.5) */
#include "sft.h"          /* sft_inherit -- bump refcount for inherited handles (bsy.9) */

/* ------------------------------------------------------------------------ *
 * Live IDT vector read/write for INT 22h/23h/24h (beads initech-509.8).
 *
 * The kernel installs trap gates at 0x22/0x23/0x24 in sysinit (int22/23/24_entry)
 * -- the PARENT's (kernel's) termination / control-break / critical-error
 * handlers. On EXEC the loader snapshots these three live vectors into the child
 * PSP (psp_save_vectors); on EXIT it restores them into the IDT (the DOS-
 * authentic behavior: if the child SETVECT'd its own 23h/24h via INT 21h AH=25h,
 * the parent's handlers are reinstated when the child terminates).
 *
 * read_live_vector reassembles the flat handler offset from the gate's split
 * lo/hi fields -- the SAME reassembly the AH=35h GETVECT seam uses (kmain.c
 * int21_getvect_idt). install_live_vector writes a 0x8F TRAP gate (the gate type
 * the DOS handlers use), mirroring AH=25h SETVECT (kmain.c int21_setvect_idt).
 * Ref: idt.h (idt_get_gate / idt_install_trap); Sec 2.4. */
static uint32_t read_live_vector(uint8_t vec)
{
    idt_gate_t g = idt_get_gate(vec);
    return (uint32_t)g.offset_lo | ((uint32_t)g.offset_hi << 16);
}

static void install_live_vector(uint8_t vec, uint32_t handler)
{
    idt_install_trap(vec, (void *)(uintptr_t)handler);
}

/* loader_context_t -- the saved kernel state for the non-returning return jump
 * (Sec 4.2). saved_esp + return_eip are captured at the point of program entry;
 * the exit hook restores them to unwind the program run + the INT 21h trap
 * frame in one jump. exit_code/exited are filled by the hook. */
typedef struct loader_context {
    uint32_t saved_esp;    /* kernel ESP at the instant of program entry        */
    uint32_t return_eip;   /* address in load_program() to resume after exit    */
    uint32_t saved_ebx;    /* loader's EBX (we clobber EBX to pass the PSP ptr)  */
    uint32_t saved_ebp;    /* loader's EBP (the frame base we return into)       */
    uint32_t saved_esi, saved_edi; /* callee-saved registers across child code */
    uint8_t  exit_code;    /* filled by loader_exit_hook when 4Ch/INT20 fires   */
    uint8_t  exited;       /* 1 once exit_code is valid                          */
} loader_context_t;

/* The loader_exit_hook restore asm hardcodes these byte offsets (it reaches the
 * ctx fields as memory operands off EAX). Lock them so a field reorder can never
 * silently mis-restore the stack (Rule 2: fail loud at COMPILE time). */
_Static_assert(__builtin_offsetof(loader_context_t, saved_esp)  == 0,
               "loader asm assumes saved_esp at offset 0");
_Static_assert(__builtin_offsetof(loader_context_t, return_eip) == 4,
               "loader asm assumes return_eip at offset 4");
_Static_assert(__builtin_offsetof(loader_context_t, saved_ebx)  == 8,
               "loader asm assumes saved_ebx at offset 8");
_Static_assert(__builtin_offsetof(loader_context_t, saved_ebp)  == 12,
               "loader asm assumes saved_ebp at offset 12");
_Static_assert(__builtin_offsetof(loader_context_t, saved_esi) == 16,
               "loader asm assumes saved_esi at offset 16");
_Static_assert(__builtin_offsetof(loader_context_t, saved_edi) == 20,
               "loader asm assumes saved_edi at offset 20");

/* The active frame links to its caller through the saved state on that caller's
 * stack. The exit hook signature has no context argument. Ref: IBM DOS 3.30
 * TR pp. 6-196/200; PRD Sec 2 (no scheduler or isolation). */
static loader_state_t g_load;

/* Tiny freestanding memcpy (loader.c pulls in no libc; Rule 11 deterministic). */
static void loader_memcpy(uint8_t *dst, const uint8_t *src, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

/* The repointed INT 21h exit hook. Fires from inside int21_dispatch ->
 * do_terminate (which ran on the INT 21h trap frame, on the kernel stack). It
 * must NOT return (that would iretd back into the terminated program -- Sec 7
 * Risk 1). Instead: record the code, then restore the loader's ESP/EBP/EBX and
 * JMP to the saved return point, discarding the trap frame + everything the
 * program pushed. Marked noreturn so the compiler does not emit a tail it can
 * never reach. Ref Sec 4.2 / Sec 4.4 (INT 20h routes here too). */
__attribute__((noreturn))
static void loader_exit_hook(uint8_t code)
{
    loader_context_t *ctx = (loader_context_t *)g_load.return_frame;

    /* If no context is bound we cannot safely unwind -- fail loud rather than
     * jump through a NULL (Rule 2). This should never happen: the hook is only
     * bound while g_load.return_frame is set. */
    if (ctx == 0) {
        for (;;) {
            __asm__ __volatile__("cli; hlt");
        }
    }

    ctx->exit_code = code;
    ctx->exited    = 1;

    /* Non-returning restore (Sec 4.2 / Risk 1). Restore the loader's EBX/EBP,
     * then switch ESP back to the loader's kernel stack, then JMP to the saved
     * return EIP -- which resumes load_program() right after the entry jmp. We
     * reference the ctx fields as MEMORY operands (ctx held in a register, %0):
     * restoring EBX/EBP FIRST and ESP LAST means no register the jmp depends on
     * is clobbered (the classic bug: restoring EBX into the same register the
     * jump target lives in). The final `jmp *0(%%eax)` reads return_eip from
     * memory, so no GPR carries it. EAX is the only scratch the asm needs. */
    __asm__ __volatile__(
        "mov 8(%0), %%ebx\n\t"    /* EBX = ctx->saved_ebx (offset 8)             */
        "mov 12(%0), %%ebp\n\t"   /* EBP = ctx->saved_ebp (offset 12)            */
        "mov 16(%0), %%esi\n\t"
        "mov 20(%0), %%edi\n\t"
        "mov 0(%0), %%esp\n\t"    /* ESP = ctx->saved_esp (offset 0) -- LAST     */
        "jmp *4(%0)\n\t"          /* jmp ctx->return_eip (offset 4) via memory   */
        :
        : "a"(ctx)                /* %0 = ctx in EAX (the one scratch register)  */
        : "memory");

    /* Not reached. */
    __builtin_unreachable();
}

/* loader_run_plan -- the shared kernel run path behind BOTH load_program (baked,
 * do_copy=1) and load_program_in_place (FAT/za4m, do_copy=0). Everything from the
 * image copy through the control transfer + return-to-loader + vector restore is
 * IDENTICAL; the ONLY difference is whether the image is copied to PROGRAM_IMAGE
 * first. Splitting it here keeps the load-bearing, bug-prone asm transfer (Sec 7
 * Risk 1) in ONE place -- the in-place path inherits it byte-for-byte (Rule 3,
 * root-cause: no duplicated transfer to drift).
 *
 * do_copy == 1: the image lives at plan.image_src (a kernel-owned buffer); copy
 *               plan.image_len bytes to plan.image_dst (PROGRAM_IMAGE).
 * do_copy == 0: the image is ALREADY at plan.image_dst (PROGRAM_IMAGE); the FAT
 *               reader put it there directly (beads initech-za4m). plan.image_src
 *               is NULL -- DO NOT copy (a NULL-src copy would fault / corrupt).
 *
 * env_block (beads initech-1i0x Tranche E inc 3 -- EXEC env inheritance): the
 * FLAT linear address of the environment block the CHILD should inherit, or 0 for
 * "inherit-empty" (the legacy / baked-demo path). The contract matches the AH=4Bh
 * exec_param_block_t.env_block field (dos_structs.h: a flat ptr, 0 = inherit) and
 * psp.c's flat_to_fake_paragraph (which stores a flat linear addr as env_seg).
 *   env_block == 0 (inherit-empty): synthesize the 2-byte empty block at ENV_BLOCK
 *               exactly as before (byte-identical legacy); plan.params.env_linear
 *               stays ENV_BLOCK (set by loader_prepare), so the child PSP env_seg
 *               points at the empty block. The baked demos take THIS path.
 *   env_block != 0 (inherit a populated env): the SHELL (command.c) has ALREADY
 *               serialized its master env into [ENV_BLOCK, ...) and passes env_block
 *               == ENV_BLOCK. DO NOT overwrite it with the empty stub; thread
 *               env_block into plan.params.env_linear so the child PSP env_seg
 *               points at the populated block. ENV_BLOCK is the ONLY env region in
 *               the locked map (spec/memory_map.h), so a non-zero env_block MUST be
 *               ENV_BLOCK -- any other value is a caller bug and we fail loud
 *               (Rule 2) rather than build a PSP whose env_seg points at garbage.
 *
 * IN-PLACE SAFETY PROOF (writing PROGRAM_IMAGE while the kernel/shell runs):
 *   The currently-running code is the KERNEL (the COMMAND.COM shell runs IN the
 *   kernel as a function, NOT as a separately-loaded program at PROGRAM_IMAGE --
 *   there is no resident program at depth zero). A nested FAT load -- or a
 *   top-level load while an AH=31h resident holds [PROGRAM_BASE, int21_tsr_top())
 *   (bead initech-6fdt) -- instead allocates an MCB ABOVE the resident image and
 *   never writes PROGRAM_IMAGE.
 *   At depth zero the child loads at PROGRAM_IMAGE (0x40100); the parent
 *   (kernel) executes from its high runway image [KERNEL_BASE, KERNEL_IMAGE_CEIL)
 *   and its own stack [KERNEL_STACK_BOT, KERNEL_STACK_TOP] (beads
 *   initech-tdnl.29, initech-xu97), both DISJOINT from [0x40100,
 *   PROGRAM_STACK_BOT) (spec/memory_map.h WINDOW DISJOINTNESS). So reading the
 *   .COM straight into PROGRAM_IMAGE
 *   is exactly as safe as the OLD code that copied staging -> PROGRAM_IMAGE: both
 *   write the SAME region while the SAME kernel runs; za4m merely elides the
 *   intermediate 64 KiB staging buffer. (spec/memory_map.h gap proof; ADR-0009.) */
static loader_status_t loader_run_plan(loader_plan_t *plan_in, int do_copy,
                                       uint32_t env_block, uint8_t *out_exit_code)
{
    loader_plan_t plan = *plan_in;

    /* The return frame stays on the invoking stack, not in a per-depth image
     * buffer. IBM DOS 3.30 TR pp. 6-196/200; PRD Sec 2 (cooperative, shared).
     * Save the actual bound PSP even for a kernel/shell entry. */
    loader_context_t ctx;
    loader_state_t saved;
    g_load.psp = (uintptr_t)int21_get_psp();
    loader_status_t ns = loader_nest_enter(&g_load, &saved, plan.psp_addr, &ctx);
    if (ns != LOADER_OK) return ns;
    int21_exit_fn parent_exit = int21_get_exit();
    int21_cwd_snapshot_t parent_cwd = int21_cwd_save();
    uint32_t parent_dta = int21_swap_dta(0u);

    /* EXEC env inheritance (beads initech-1i0x Tranche E inc 3): resolve the
     * env-block decision through the pure, host-tested loader_decide_env (so the
     * kernel and the oracle agree by construction). A bad env_block fails loud
     * (Rule 2: LOADER_ERR_BAD_ENV -- the program is NOT run). */
    loader_env_decision_t env_dec;
    {
        loader_status_t es = loader_decide_env(env_block, &env_dec);
        if (es != LOADER_OK) {
            loader_nest_leave(&g_load, &saved);
            (void)int21_swap_dta(parent_dta);
            return es;
        }
    }

#ifdef LOADER_MUTATE_INPLACE_DOUBLE_HANDLE
    /* MUTANT (CLAUDE.md Rule 6; make test-loader-big-mutant only): force the copy
     * ON for the in-place path too -- re-imposing the pre-za4m double-handling
     * (the image, already at PROGRAM_IMAGE, would be copied FROM a NULL/garbage
     * src). The big-.COM oracle asserts the in-place plan carries image_src==NULL
     * and the run path must NOT copy; this mutant makes do_copy==1 unconditionally
     * so the oracle goes RED. NEVER define in a real build. */
    do_copy = 1;
#endif

    /* Copy the program image to PROGRAM_IMAGE (0x40100) ONLY on the baked path.
     * On the in-place path (za4m) the bytes are already there -- skip the copy
     * (plan.image_src is NULL; a copy would dereference NULL). Volatile-correct:
     * the destination is a fixed physical region the loader owns. */
    if (do_copy) {
        loader_memcpy((uint8_t *)(uintptr_t)plan.image_dst, plan.image_src,
                      plan.image_len);
    }

    /* Environment block at ENV_BLOCK (Sec 2.7; beads initech-1i0x Tranche E inc 3).
     * loader_decide_env (above) chose:
     *   - write_empty == 1 (inherit-empty / the baked-demo path): synthesize the
     *     2-byte empty block (double-NUL = a valid empty environment), EXACTLY as
     *     before. Byte-identical legacy behavior the baked programs + every existing
     *     boot oracle depend on.
     *   - write_empty == 0 (the shell populated [ENV_BLOCK, ...) before EXEC): do
     *     NOT write the empty stub -- that would zero the first two bytes of the
     *     shell's serialized env. env_dec.env_linear already carries the block the
     *     child PSP env_seg must point at.
     * Either way thread env_dec.env_linear into the PSP params so env_seg =
     * flat_to_fake_paragraph(env_linear) is honest about which block we inherit. */
    if (env_dec.write_empty && saved.depth == 0u) {
        uint8_t *env = (uint8_t *)(uintptr_t)ENV_BLOCK;
        env[0] = 0x00;
        env[1] = 0x00;
    }
    plan.params.env_linear = env_dec.env_linear;

    /* JFT inheritance (beads initech-bsy.9; MS-DOS 3.3 AH=4Bh; ADR-0003 DEC-06):
     * the EXEC child inherits a COPY of the PARENT process's Job File Table. The
     * parent is the CURRENTLY-bound PSP (g_cur_psp) -- at a shell EXEC that is the
     * shell's PSP, whose jft[1] a `>`-DUP2 has repointed at the redirect file. We
     * thread its jft into the PSP params so psp_build copies all 20 handles; a
     * NULL parent (nothing bound) falls back to the CON defaults. This MUST be
     * read BEFORE int21_set_psp(plan.psp_addr) below rebinds g_cur_psp to the
     * child. Without it the child's handle 1 is reset to CON and external-command
     * output redirect (GREET.COM > OUT.TXT) is lost. */
    {
        psp_t *parent = (psp_t *)int21_get_psp();
        plan.params.parent_psp_linear = (uint32_t)(uintptr_t)parent;
        plan.params.parent_jft = parent ? parent->jft : (const uint8_t *)0;
    }

    /* Build the PSP at PROGRAM_BASE (psp_build, beads initech-509.4). A clamp
     * (non-zero return) means the command tail was too long; that is a loud
     * caller bug for a baked program but not fatal -- the tail is clamped, never
     * overflowed. We ignore the count here (the baked program has no tail).
     * psp_build now ALSO copies the inherited parent JFT (bsy.9) when parent_jft
     * is non-NULL, else it lays the CON defaults. */
    (void)psp_build((psp_t *)(uintptr_t)plan.psp_addr, &plan.params);

    /* Save the PARENT's (kernel's) live INT 22h/23h/24h vectors into the child
     * PSP (beads initech-509.8; ADR-0003 DEC-10; ground-truth Sec 2.4). psp_build
     * left saved_vectors zero by design; we fill it HERE from the live IDT so the
     * parent's handlers can be restored on the child's EXIT below. Read all three
     * BEFORE binding/JMP so we capture exactly the state the child inherits. */
    {
        uint32_t v22 = read_live_vector(0x22u);
        uint32_t v23 = read_live_vector(0x23u);
        uint32_t v24 = read_live_vector(0x24u);
        psp_save_vectors((psp_t *)(uintptr_t)plan.psp_addr, v22, v23, v24);
    }

    /* Bind the loaded program's PSP as the current process (beads initech-509.3)
     * so its INT 21h handle functions (40h WRITE, 45h DUP, 46h DUP2, file ops)
     * resolve through ITS Job File Table. The kernel re-binds its own PSP after
     * we return (kmain, alongside the exit-hook restore). The program's JFT is
     * the standard predefined set psp_build just laid down. */
    int21_set_psp((struct psp *)(uintptr_t)plan.psp_addr);

    /* Bump the SFT refcount for each handle the child just INHERITED from the
     * parent JFT (beads initech-bsy.9). psp_build copied the parent's table into
     * the child, so the child now holds an ADDITIONAL reference to every open
     * FILE/open-device slot the parent had (e.g. the `>`-redirect file). The
     * child's exit (do_terminate -> sft_close_process) will RELEASE exactly those,
     * so this bump keeps the count balanced: without it the child's exit would
     * over-free the slot out from under the parent, and the shell's post-EXEC
     * dos_dup2(saved,1)/dos_close would then fail loud on a freed SFT entry.
     * Resident CON/AUX/PRN slots 0..3 are shared/refcount-fixed and are NOT bumped
     * (sft_inherit skips them), mirroring sft_close_process. */
    sft_inherit((psp_t *)(uintptr_t)plan.psp_addr);

    /* Bind the MCB heap arena to the window DISJOINT from this loaded program
     * (beads initech-1q4u; ADR-0009 DEC-04). The arena starts ABOVE the loaded
     * image+BSS (plan.arena_base) and ends at PROGRAM_ARENA_CEIL (plan.arena_ceil),
     * so a 48h ALLOC can never return memory inside the program image/env/stack --
     * the authentic DOS model (DOS hands the program a FREE block AFTER its image).
     * This SUPERSEDES the old int21_mcb_reset() over [PROGRAM_BASE, PROGRAM_ALLOC_
     * END), which overlaid the running program (ADR-0009 Sec 1 latent corruption).
     * The disjoint arena is bound as ONE FREE block (NOT owned by the PSP): unlike
     * the program's image block, it is not the program's "single big block", so
     * there is nothing to AH=4Ah SETBLOCK-shrink -- the program's first AH=48h
     * carves its heap straight from the free region (bead hdlb, S8.2: handing the
     * whole arena to the PSP up front left zero free space and made a no-SETBLOCK
     * heap user -- SAMIR -- get 48h insufficient-memory and panic). If the image
     * was too large to leave a positive arena (plan.arena_present == 0) the arena
     * is left UNBOUND -- a 48h ALLOC then reports insufficient memory rather than
     * overlapping (fail loud, Rule 2). */
    if (plan.psp_addr != PROGRAM_BASE) {
        /* Child PSP/image/stack already own an MCB in the caller's chain --
         * a nested EXEC, or a top-level EXEC placed above a resident (bead
         * initech-6fdt). Reinitializing that chain would destroy the caller's
         * live heap or a resident's blocks. IBM DOS 3.30 TR pp. 11-4/5;
         * ADR-0003 DEC-03. */
    } else if (plan.arena_present) {
        (void)int21_mcb_bind_program(plan.arena_base, plan.arena_ceil);
    } else {
        int21_set_mcb_arena(0, 0u, 0u);   /* no heap -> 48h insufficient, no overlap */
    }

    /* THE CHILD RUNS IN THE DRIVE'S CURRENT DIRECTORY (bead initech-3p1a;
     * supersedes the initech-mzxa reset-to-root, which that bead filed as an
     * OPEN DECISION "(b) CWD inheritance ... simplest model is reset-to-root
     * on launch -- confirm acceptable", made before the CHDIR writer existed).
     * In DOS the current directory belongs to the DRIVE, not to the process:
     * AH=47h "Get Current Directory ... for the specified drive" and AH=3Bh
     * set it (80X0945 DOS 3.30 Technical Reference pp. 6-121, 6-188); EXEC
     * hands the child the parent's open files and environment (pp. 6-197/198)
     * and changes no directory; "FCB function calls ... only in the current
     * directory of the specified drive" (p. 4-4); and a program's data files
     * are found in the user's current directory ("MS-DOS first searches the
     * current directory for data files", MS-DOS_3.3_Users_Guide_198707.pdf
     * PDF p. 114, APPEND). So `CD APPS` then `TPSREP TPS.DBF` must open
     * APPS\TPSPARM.DAT -- with the old reset it opened \TPSPARM.DAT and
     * failed TPS-001. Nothing is reset here; loader_run_plan restores its saved
     * snapshot after the child returns, so a
     * child's CHDIR does not outlive it (a stated deviation, unchanged).
     * MUTANT LOADER_MUT_CWD_RESET (Rule 6; test-flair-tps-journey-mutant
     * CWDRESET): the pre-3p1a reset-to-root. NEVER in a real build. */
#if defined(LOADER_MUT_CWD_RESET)
    int21_cwd_reset();
#endif

    /* --- The control transfer + return-to-loader (Sec 4; Risk 1). --- */
    ctx.exit_code  = 0;
    ctx.exited     = 0;
    ctx.saved_ebx  = 0;
    ctx.saved_ebp  = 0;
    ctx.saved_esp  = 0;
    ctx.return_eip = 0;

    /* The active frame is bound above; replace the hook only for this run.
     * parent_exit is restored on return (IBM DOS 3.30 TR pp. 6-196/200). */
    int21_set_exit(loader_exit_hook);

    /* Keep entry and stack_top in memory; constrain the PSP pointer to EBX.
     * GCC then preserves the caller's EBX and cannot use it as an unrelated
     * address base that an undeclared in-asm EBX write would invalidate. */
    const uint32_t entry_eip = plan.entry;
    const uint32_t stack_top = plan.stack_top;
    const uint32_t psp_ptr   = plan.psp_addr;

    /* The asm: capture ESP and all callee-saved GPRs plus a return label, then
     * switch to the
     * program stack, with EBX = PSP_ADDR (the program's pointer to its own PSP,
     * Sec 3.3), and JMP (not CALL -- Sec 4.3) to the program entry. When the
     * program terminates, loader_exit_hook restores ESP/EBP/EBX/ESI/EDI and jumps to the
     * 1: label, landing us right after this block with ctx.exit_code set.
     *
     * Order matters (bcg.10): every MEMORY operand must be read while ESP still
     * points at the KERNEL stack, because at higher optimization (or
     * -fomit-frame-pointer) the compiler addresses these locals ESP-relative.
     * So the jump target (entry_eip) is loaded into EAX BEFORE the ESP switch;
     * the jmp then goes through the REGISTER, never a memory operand resolved
     * against the just-switched program stack. EAX is free to clobber at entry
     * (the program does not rely on it; it previously held &1f anyway). The very
     * last instruction before the jmp is the ESP switch, whose source (stack_top)
     * is read with the kernel ESP still live. "memory" tells GCC the asm
     * reads/writes memory across the non-local flow. ESI/EDI are preserved by
     * the context (offsets asserted above); the child may destroy ECX/EDX as
     * well as EAX, so all three caller-saved registers must be clobbers even
     * though this entry sequence itself only uses EAX. Ref: round-2 Sec 4. */
    __asm__ __volatile__(
        "mov %%esp, %0\n\t"          /* ctx.saved_esp = current kernel ESP        */
        "mov %%ebp, %1\n\t"          /* ctx.saved_ebp = current frame base        */
        "mov %%ebx, %2\n\t"          /* save the constrained PSP input in EBX     */
        "lea 1f, %%eax\n\t"          /* EAX = address of the return label (1:)    */
        "mov %%eax, %3\n\t"          /* ctx.return_eip = &1f                      */
        "mov %%esi, %4\n\t"
        "mov %%edi, %5\n\t"
        "mov %6, %%eax\n\t"          /* EAX = entry_eip, read BEFORE the switch    */
        "mov %8, %%esp\n\t"          /* switch to the program stack top (LAST)    */
        "jmp *%%eax\n\t"             /* JMP via REGISTER (no post-switch mem read) */
        "1:\n\t"                     /* <- loader_exit_hook jumps back here       */
        /* Intel SDM Vol. 1 "DF Flag" / 8086 convention: a non-local return
         * from the child's INT 21h bypasses IRET; C requires forward strings. */
        "cld\n\t"
        : "=m"(ctx.saved_esp), "=m"(ctx.saved_ebp), "=m"(ctx.saved_ebx),
          "=m"(ctx.return_eip), "=m"(ctx.saved_esi), "=m"(ctx.saved_edi)
        : "m"(entry_eip), "b"(psp_ptr), "m"(stack_top)
        : "eax", "ecx", "edx", "cc", "memory");

    /* Resume the immediate caller's frame, PSP, hook and directory state.
     * Neither nested termination path may force a kernel-context rebind. */
    loader_nest_leave(&g_load, &saved);
    int21_set_psp((struct psp *)g_load.psp);
    int21_set_exit(parent_exit);
    int21_cwd_restore(&parent_cwd);
    (void)int21_swap_dta(parent_dta);

    /* Restore the parent's INT 22h/23h/24h vectors into the live IDT (beads
     * initech-509.8; ADR-0003 DEC-10). If the child SETVECT'd its own 23h/24h
     * (INT 21h AH=25h) while running, the parent's handlers must be reinstated
     * now that the child has terminated (DOS-authentic). We restore from the
     * SAME PSP we saved into (plan.psp_addr), which is still
     * intact: the child can no longer run (we resumed past its terminating
     * syscall), and do_terminate frees handles only -- it never scribbles the
     * PSP bytes. Runs on EVERY exit path (4Ch and INT 20h both route through
     * do_terminate -> loader_exit_hook -> the jmp that landed us here). */
    {
        uint32_t v22 = 0, v23 = 0, v24 = 0;
        psp_load_vectors((const psp_t *)(uintptr_t)plan.psp_addr,
                         &v22, &v23, &v24);
        install_live_vector(0x22u, v22);
        install_live_vector(0x23u, v23);
        install_live_vector(0x24u, v24);
    }

    if (out_exit_code) {
        *out_exit_code = ctx.exit_code;
    }
    return LOADER_OK;
}

loader_status_t load_program(const uint8_t *image, uint32_t image_len,
                             const char *cmd_tail, uint32_t cmd_tail_len,
                             uint8_t *out_exit_code)
{
    /* Baked path: validate + lay out, then run WITH the image copy (do_copy=1).
     * The image source bytes live in a kernel-owned buffer (e.g. the baked demo
     * program in run_baked); loader_run_plan copies them DOWN to PROGRAM_IMAGE.
     * UNCHANGED by za4m -- only the FAT path goes direct/in-place. */
    /* The baked image is copied to PROGRAM_IMAGE: refuse while a resident
     * occupies the window (bead initech-6fdt) rather than overwrite it. */
    if (g_load.depth || int21_tsr_top()) return LOADER_ERR_BUSY;
    loader_plan_t plan;
    loader_status_t st = loader_prepare(image, image_len, cmd_tail,
                                        cmd_tail_len, &plan);
    if (st != LOADER_OK) {
        return st;   /* fail loud: the program is NOT run (Rule 2) */
    }
    /* Baked path inherits an EMPTY env (env_block=0): the baked demos carry no
     * environment, so loader_run_plan synthesizes the 2-byte empty block exactly
     * as before (byte-identical; beads initech-1i0x inc 3). */
    st = loader_run_plan(&plan, /*do_copy=*/1, /*env_block=*/0u, out_exit_code);
    return st;
}

loader_status_t load_program_in_place(uint32_t image_len, const char *cmd_tail,
                                      uint32_t cmd_tail_len, uint32_t env_block,
                                      uint8_t *out_exit_code)
{
    /* In-place path (beads initech-za4m; ADR-0009 companion to DEC-04): the .COM
     * is ALREADY resident at PROGRAM_IMAGE (load_program_from_fat read it there
     * directly off the FAT volume, lifting the old 64 KiB LOAD_STAGING_MAX cap to
     * PROGRAM_IMAGE_MAX). Validate + lay out via loader_prepare_in_place (no image
     * pointer; image_src comes back NULL), then run with NO copy (do_copy=0) --
     * the PSP build, env, vector save/restore, arena bind, stack switch, JMP and
     * return-to-loader are all the SAME shared path load_program uses. env_block
     * (beads initech-1i0x inc 3) selects inherit-empty (0) vs inherit the shell's
     * populated env block (ENV_BLOCK); it threads to loader_run_plan unchanged. */
    loader_plan_t plan;
    loader_status_t st = loader_prepare_in_place((const uint8_t *)(uintptr_t)PROGRAM_IMAGE,
                                                 image_len, cmd_tail,
                                                 cmd_tail_len, &plan);
    if (st != LOADER_OK) {
        return st;   /* fail loud: the program is NOT run (Rule 2) */
    }
    return loader_run_plan(&plan, /*do_copy=*/0, env_block, out_exit_code);
}

/* ------------------------------------------------------------------------ *
 * InitechMZ (.EXE) fail-loud foreign-MZ panic + in-place runner (beads
 * initech-wczy / dtw.2; ADR-0003 DEC-08a).
 * ------------------------------------------------------------------------ */

/* loader_panic_foreign_mz -- the DEC-08a.5 honesty gate, fail-loud (Rule 2).
 *
 * Reached ONLY from the kernel EXEC path when loader_prepare_mz reports
 * LOADER_ERR_FOREIGN_MZ: a genuine UNTAGGED 16-bit DOS MZ (e_res[0] != the
 * InitechMZ tag) whose relocations are paragraph fixups over 16-bit code. The
 * flat 32-bit CPU (ADR-0001: no v8086, no 16-bit segments) cannot decode that
 * instruction stream; relocating and jumping to it would silently misexecute
 * (DEC-08a.5). So we PANIC instead of run -- never relocate-and-misexecute.
 *
 * Self-contained serial dump (mirrors panic.c's bounded 16550 poll so a
 * stuck/absent UART can never hang the panic itself). The heap/console-free
 * physical panic painter shows the typed refusal and program name (Rule 2;
 * audit DOS L030) before HALTED. Marked noreturn: a foreign MZ is
 * terminal -- control NEVER returns to the EXEC caller. */
#define LDR_COM1_THR  0x3F8u
#define LDR_COM1_LSR  0x3FDu
#define LDR_LSR_THRE  0x20u
#define LDR_SERIAL_SPIN_MAX 100000u

static void ldr_serial_putc(char c)
{
    uint32_t spins = 0u;
    while ((inb(LDR_COM1_LSR) & LDR_LSR_THRE) == 0u) {
        if (++spins >= LDR_SERIAL_SPIN_MAX) {
            return;   /* UART not draining -> drop the byte, never hang (Rule 2) */
        }
    }
    outb(LDR_COM1_THR, (uint8_t)c);
}

static void ldr_serial_puts(const char *s)
{
    while (*s) {
        ldr_serial_putc(*s++);
    }
}

__attribute__((noreturn))
static void loader_panic_foreign_mz(const char *name83)
{
    /* Grep-able marker FIRST (the oracle keys on "PANIC"). */
    ldr_serial_puts("PANIC foreign-mz: untagged 16-bit DOS .EXE -- cannot run in flat mode\n");
    if (name83) {
        ldr_serial_puts("  name=");
        ldr_serial_puts(name83);
        ldr_serial_putc('\n');
    }
    ldr_serial_puts("  DEC-08a.5: refusing to relocate-and-misexecute 16-bit code\n");
    panic_show_reason(PANIC_REASON_FOREIGN_MZ, name83);
    ldr_serial_puts("HALTED\n");
    /* Terminal: never proceed (Rule 2 / DEC-08a.5). */
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

/* load_program_mz_in_place -- the InitechMZ sibling of load_program_in_place
 * (DEC-08a). The WHOLE MZ FILE is already resident at PROGRAM_IMAGE (the FAT
 * reader put it there); loader_prepare_mz parses it, moves the load module down
 * over the header, applies the flat relocations IN PLACE, and produces the same
 * loader_plan_t -- then we run it through the ONE shared transfer (loader_run_plan,
 * do_copy=0: the relocated module is already at PROGRAM_IMAGE). A foreign MZ
 * PANICS here (DEC-08a.5); any other malformed MZ fails loud with the prep status.
 *
 * `file_len` is the on-disk MZ file byte count at PROGRAM_IMAGE. `name83` is the
 * program name, for the panic diagnostic only. */
static loader_status_t load_program_mz_in_place(uint32_t file_len,
                                                const char *name83,
                                                const char *cmd_tail,
                                                uint32_t cmd_tail_len,
                                                uint32_t env_block,
                                                uint8_t *out_exit_code)
{
    loader_plan_t plan;
    loader_status_t st = loader_prepare_mz((uint8_t *)(uintptr_t)PROGRAM_IMAGE,
                                           file_len, cmd_tail, cmd_tail_len,
                                           &plan);
    if (st == LOADER_ERR_FOREIGN_MZ) {
        /* DEC-08a.5 fail-loud honesty gate -- never returns. */
        loader_panic_foreign_mz(name83);
    }
    if (st != LOADER_OK) {
        return st;   /* fail loud: the program is NOT run (Rule 2) -- bad format */
    }
    return loader_run_plan(&plan, /*do_copy=*/0, env_block, out_exit_code);
}

/* ------------------------------------------------------------------------ *
 * FAT-sourced load (beads initech-saw; DIRECT-LOAD beads initech-za4m).
 *
 * loader.c reads the named .COM off the mounted volume DIRECTLY into the program
 * region (PROGRAM_IMAGE, 0x40100) and runs it IN PLACE via load_program_in_place
 * -- NO intermediate staging buffer, NO staging -> image copy. It caches the
 * volume + the whole FAT at bind time (fat12_read_file needs the FAT for the
 * cluster-chain walk). All scratch is kernel BSS -- NEVER a multi-KB buffer on
 * the kernel stack (spec/memory_map.h Risk 2).
 *
 * WHY DIRECT (beads initech-za4m; ADR-0009 companion to DEC-04): the OLD path
 * read the .COM into the 64 KiB LOAD_STAGING buffer [0x80000,0x90000) (it abuts
 * the kernel stack at 0x90000, so it CANNOT grow) and then copied DOWN to
 * PROGRAM_IMAGE. That capped a FAT .COM at 64 KiB (LOAD_STAGING_MAX) even though
 * the program arena allows ~188 KiB (PROGRAM_IMAGE_MAX = ENV_BLOCK -
 * PROGRAM_IMAGE). A 77 KiB SAMIR.COM was wrongly rejected LOADER_ERR_TOO_BIG.
 * Reading straight into PROGRAM_IMAGE makes PROGRAM_IMAGE_MAX the SOLE bound and
 * removes a whole 77+ KiB copy per EXEC. The LOAD_STAGING region is now UNUSED by
 * this path (its #defines remain in spec/memory_map.h, but no code writes it).
 *
 * Ref: fat12.h (fat12_find / fat12_read_file); spec/memory_map.h; ADR-0009.
 * ------------------------------------------------------------------------ */

/* The mounted volume the loader reads .COMs from (caller-owned; bound by
 * loader_bind_fat_volume). NULL -> load_program_from_fat returns NO_VOLUME. */
static const fat12_volume_t *g_load_vol = 0;

/* The whole-FAT cache: a POINTER that ALIASES the int21 file backend's FAT
 * buffer (fileio_fat's g_fat -- the SAME mounted volume's FAT), passed in at
 * loader_bind_fat_volume. This module used to keep its OWN 6 KiB copy in kernel
 * .bss (the recurring kernel-window pressure); sharing the backend's already-
 * loaded FAT removes that duplicate. Safe because the loader reads the FAT only
 * during load_program_from_fat (the load phase), never while a program runs.
 * NULL until a successful bind. */
static const uint8_t *g_load_fat = 0;
static uint32_t       g_load_fat_len = 0;

/* Sector scratch for fat12_find; cluster scratch for fat12_read_file (one
 * sector each on the 1.44 MB geometry). Kernel BSS. */
static uint8_t  g_load_sector[BLOCKDEV_SECTOR_SIZE];
static uint8_t  g_load_cluster[BLOCKDEV_SECTOR_SIZE];

static loader_status_t loader_find_file(const char *name83, uint16_t dir_start,
                                        dir_entry_t *de);

void loader_bind_fat_volume(const struct fat12_volume *vol,
                            const uint8_t *fat, uint32_t fat_len)
{
    /* A missing volume OR a missing whole-FAT cache (NULL/zero -- e.g. a windowed
     * FAT16 volume) leaves the loader UNBOUND (fail loud, Rule 2 -- never half-
     * initialised; load_program_from_fat then returns NO_VOLUME). */
    if (vol == 0 || fat == 0 || fat_len == 0u) {
        g_load_vol     = 0;
        g_load_fat     = 0;
        g_load_fat_len = 0;
        return;
    }
    /* Alias the int21 file backend's already-loaded FAT (the SAME volume's FAT).
     * No second 6 KiB buffer, no second FAT read. */
    g_load_fat     = fat;
    g_load_fat_len = fat_len;
    g_load_vol     = vol;
}

loader_status_t load_program_from_fat(const char *name83, uint16_t dir_start,
                                      const char *cmd_tail, uint32_t cmd_tail_len,
                                      uint32_t env_block, uint8_t *out_rc)
{
    if (g_load_vol == 0) {
        return LOADER_ERR_NO_VOLUME;
    }
    /* Fail before allocation or writes when the bounded return chain is full.
     * Ref: LOADER_EXEC_DEPTH, IBM DOS 3.30 TR pp. 6-199, 11-4/5. */
    if (g_load.depth >= LOADER_EXEC_DEPTH) {
        return LOADER_ERR_BUSY;
    }

#ifdef LOADER_MUTATE_EXEC_ROOTONLY
    /* MUTANT (CLAUDE.md Rule 6; make test-zs24-exec-mutant only): IGNORE the
     * threaded containing-directory cluster and look ONLY in the root, the pre-
     * zs24 root-only behavior. A subdir EXEC of a program that exists ONLY inside
     * ::SUB then can't be located -> LOADER_ERR_NOT_FOUND, the program never runs,
     * its serial marker is ABSENT -> the subdir-EXEC oracle goes RED. NEVER define
     * in a real build. */
    dir_start = 0u;
#endif

    /* Locate the .COM in its CONTAINING directory (beads initech-zs24). The ROOT
     * case (dir_start==0) takes the BYTE-IDENTICAL historical fat12_find so root
     * EXEC (test-shell / test-exec / the baked demos) is provably unchanged
     * (Rule 3). A subdir (dir_start!=0) uses the parent-aware fat12_find_slot_in
     * over the cached whole-FAT to walk the subdir cluster chain; the located
     * entry's OWN start_cluster then feeds fat12_read_file exactly as the root
     * case. We discard the returned slot (EXEC is read-only -- no dir write-back). */
    dir_entry_t de;
    int rc;
    {
        loader_status_t fst = loader_find_file(name83, dir_start, &de);
        if (fst != LOADER_OK) {
            return fst;
        }
    }

    /* A leaf that resolves to a DIRECTORY is not a runnable program: refuse it
     * (Rule 2) rather than read the directory's cluster bytes into PROGRAM_IMAGE
     * as if they were a .COM. fat12_find / fat12_find_slot_in match by NAME only
     * (no attribute filter -- they back FINDFIRST too), so EXEC owns this guard.
     * Maps to 0x0002 (file not found) at the do_exec seam -- the DOS-authentic
     * "no runnable program here". The root case can only reach this for a root
     * directory entry, which no prior root-EXEC golden exercises (a .COM carries
     * attr 0x00/0x20, never DIR_ATTR_DIRECTORY), so root EXEC stays byte-identical.
     * (The guard lives in loader_find_file, shared with the tenant and overlay
     * loads -- ADR-0003 Amendment DEC-19.) */

    /* Reject before reading anything that cannot fit the program region. The SOLE
     * size bound is PROGRAM_IMAGE_MAX (~188 KiB = ENV_BLOCK - PROGRAM_IMAGE) --
     * the .COM is read DIRECTLY into PROGRAM_IMAGE (beads initech-za4m), so the old
     * 64 KiB LOAD_STAGING_MAX cap is GONE (it bound the now-deleted staging copy;
     * the stale comment that claimed "staging cap >= PROGRAM_IMAGE_MAX" was FALSE
     * -- staging was 64 KiB, the binding constraint). load_program_in_place
     * re-checks via loader_prepare_in_place (Rule 2: belt and braces). */
    if (de.file_size > PROGRAM_IMAGE_MAX) {
        return LOADER_ERR_TOO_BIG;
    }
    if (de.file_size == 0u) {
        /* An empty file is not a runnable program (load_program_in_place rejects
         * zero length too, but fail loud here with the FAT-specific bad-format-ish
         * code rather than read 0 bytes and run garbage). */
        return LOADER_ERR_TOO_BIG;   /* reuse: nothing runnable (size 0) */
    }

    /* Read the whole .COM DIRECTLY into PROGRAM_IMAGE (0x40100) -- no staging
     * buffer, no later copy (beads initech-za4m). dst_cap is PROGRAM_IMAGE_MAX so
     * fat12_read_file fails loud (FAT12_ERR_BUFFER) before overrunning the program
     * arena -- redundant with the file_size guard above but a hard backstop (Rule
     * 2). Safety (writing PROGRAM_IMAGE while the kernel runs): see the IN-PLACE
     * SAFETY PROOF on loader_run_plan -- the parent is the KERNEL (image and
     * stack in the high runway, both disjoint from [0x40100,...); spec/
     * memory_map.h WINDOW DISJOINTNESS, bead initech-xu97). At depth zero
     * nothing else occupies PROGRAM_IMAGE; nested loads use an MCB.
     * This is exactly as safe as the OLD copy to 0x40100 -- same region, same
     * running kernel, minus the intermediate staging hop. */
    /* DOS EXEC allocates the largest free block for the child's PSP and stack
     * (IBM DOS 3.30 TR pp. 11-4/5). The existing top-level disjoint arena is
     * ABOVE the resident image+BSS. Keep its chain, including caller-owned
     * blocks. Native flat .COM code loaded here must be position independent;
     * unlike InitechMZ it has no relocation records (DEC-08/DEC-08a.1). */
    uint32_t child_psp = 0u, child_end = 0u;
    /* A resident (AH=31h / INT 27h) keeps [PROGRAM_BASE, int21_tsr_top()), so
     * a top-level EXEC is placed exactly like a nested one: the arena is
     * re-bound above every resident and EXEC takes its largest block (TR
     * pp. 11-4/5; bead initech-6fdt). The same position-independence rule as
     * nested EXEC applies (DEC-08: a flat .COM carries no relocations). */
    uint32_t above = g_load.depth ? 0u : int21_tsr_top();
    if (g_load.depth || above) {
        if (above && !int21_mcb_bind_above_residents(PROGRAM_ARENA_CEIL))
            return LOADER_ERR_NOMEM;
        if (int21_mcb_exec_alloc(de.file_size, &child_psp, &child_end) != 0u)
            return LOADER_ERR_NOMEM;
    }
    uint8_t *image = (uint8_t *)(uintptr_t)(child_psp ? child_psp + 0x100u : PROGRAM_IMAGE);
    uint32_t got = 0;
    rc = fat12_read_file(g_load_vol, g_load_fat, g_load_fat_len, &de,
                         image, child_psp ? child_end - child_psp - 0x100u : PROGRAM_IMAGE_MAX,
                         g_load_cluster, &got);
    if (rc != FAT12_OK) {
        if (child_psp) int21_mcb_exec_free(child_psp);
        return LOADER_ERR_READ;
    }

    /* The overlay by name on serial (bead initech-13ig): the REFUSAL itself is
     * loader_prepare_core's (the one choke point every entry below passes);
     * this only names the file for the emu oracle. With the loader mutant the
     * recogniser is inert, so the line vanishes with the refusal. */
    if (loader_image_is_transient(image, got)) {
        ldr_serial_puts("LOADER-REFUSE-TRANSIENT name=");
        ldr_serial_puts(name83);
        ldr_serial_puts("\n");
    }

    /* DISPATCH BY CONTENT (DEC-08a.4 -- DOS-authentic: format from the first two
     * image bytes, NOT the extension). 'MZ'/'ZM' routes to the InitechMZ (.EXE)
     * path; anything else is the flat .COM-equivalent path (DEC-08, UNCHANGED --
     * byte-identical to before dtw.2). The .COM default is the common case and is
     * not perturbed: the mz_is_mz probe is a 2-byte read, then the SAME
     * load_program_in_place runs as always.
     *
     * Run it IN PLACE (beads initech-za4m). The image is already at its assigned
     * base, so the in-place runner
     * does everything EXCEPT the copy -- builds the PSP, env, saves/restores the
     * INT 22/23/24 vectors, binds the disjoint MCB arena, switches the stack, JMPs
     * to PROGRAM_IMAGE, and handles return-to-loader -- via the SAME shared path
     * (loader_run_plan, do_copy=0). For an MZ the runner adds the DEC-08a prologue
     * (parse + move load module over header + apply flat relocs) BEFORE that one
     * transfer; a foreign untagged MZ PANICS fail-loud (DEC-08a.5). */
    uint8_t rcv = 0;
    loader_status_t st;
    if (child_psp) {
        loader_plan_t plan;
        st = loader_prepare_in_place(image, got, cmd_tail, cmd_tail_len, &plan);
        if (mz_is_mz(image, got)) {
            mz_image_t info;
            if (mz_parse_header(image, got, &info) == MZ_ERR_FOREIGN)
                loader_panic_foreign_mz(name83);
            /* This seam admits native position-independent flat .COM children.
             * Nested InitechMZ relocation is a separate format extension. */
            st = LOADER_ERR_BAD_FORMAT;
        }
        if (st == LOADER_OK) {
            plan.psp_addr = child_psp;
            plan.image_dst = plan.entry = child_psp + 0x100u;
            plan.stack_top = child_end - 4u;
            plan.params.alloc_end_linear = child_end;
            st = loader_run_plan(&plan, 0, env_block, &rcv);
        }
        /* Normal termination already frees the owner. Also reclaim a failed
         * preparation; this is idempotent and does not touch caller blocks. */
        int21_mcb_exec_free(child_psp);
    } else if (mz_is_mz(image, got)) {
        st = load_program_mz_in_place(got, name83, cmd_tail, cmd_tail_len,
                                      env_block, &rcv);
    } else {
        st = load_program_in_place(got, cmd_tail, cmd_tail_len, env_block, &rcv);
    }

    if (st != LOADER_OK) {
        return st;   /* propagate the in-place loader's fail-loud status (TOO_BIG) */
    }
    if (out_rc) {
        *out_rc = rcv;
    }
    return LOADER_OK;
}

/* ------------------------------------------------------------------------ *
 * loader_load_tenant -- the disk-tenant sibling of load_program_from_fat (bead
 * initech-tdnl.14; DEC-AC3-1). Locate + carve + read + loader_prepare_tenant.
 * It does NOT run anything and does NOT take an EXEC frame: a GUI tenant is not
 * an EXEC, never touches PROGRAM_BASE, and must be able to coexist with a
 * later class-2 text-tenant EXEC (ADR-0013 Sec 3.2). The carve goes through the
 * caller's callback so this object never learns about the FLAIR heap.
 * ------------------------------------------------------------------------ */
/* Locate a plain file `name83` in the directory whose first cluster is
 * `dir_start` (0 == root) on the bound volume: the find shared by the tenant
 * and overlay loads (the same two fat12 calls load_program_from_fat makes).
 * A directory entry is not a loadable file (LOADER_ERR_NOT_FOUND). */
static loader_status_t loader_find_file(const char *name83, uint16_t dir_start,
                                        dir_entry_t *de)
{
    int rc;
    if (g_load_vol == 0) {
        return LOADER_ERR_NO_VOLUME;
    }
    if (dir_start == 0u) {
        rc = fat12_find(g_load_vol, g_load_sector, name83, de);
    } else {
        uint32_t slot = 0u;
        rc = fat12_find_slot_in(g_load_vol, g_load_fat, g_load_fat_len,
                                dir_start, g_load_sector, name83, de, &slot);
    }
    if (rc == FAT12_ERR_NOT_FOUND) {
        return LOADER_ERR_NOT_FOUND;
    }
    if (rc != FAT12_OK) {
        return LOADER_ERR_READ;
    }
    if ((de->attribute & DIR_ATTR_DIRECTORY) != 0u) {
        return LOADER_ERR_NOT_FOUND;   /* a folder is not a loadable file */
    }
    return LOADER_OK;
}

/* loader_overlay_core's reader on the kernel: a positioned read of the
 * located entry (ctx) through the shared FAT cache; a short read fails. */
typedef struct loader_ovl_source {
    dir_entry_t entry;
    int checked;
} loader_ovl_source_t;

static int loader_ovl_fat_check(void *ctx)
{
    loader_ovl_source_t *source = ctx;
    int rc = fat12_validate_file_chain(g_load_vol, g_load_fat, g_load_fat_len,
                                       &source->entry);
    source->checked = rc == FAT12_OK;
    return rc;
}

static int loader_ovl_fat_read(void *ctx, uint32_t off, uint32_t len,
                               uint8_t *dst)
{
    const loader_ovl_source_t *source = ctx;
    uint32_t got = 0u;
    int rc = fat12_read_partial(g_load_vol, g_load_fat, g_load_fat_len,
                                &source->entry, off, len, dst, g_load_cluster, &got);
    /* After preflight, a structural error/short read contradicts the stable
     * cached-chain proof. Never mislabel it as a genuine device read error. */
    if (source->checked && ((rc != FAT12_OK && rc != FAT12_ERR_READ) ||
                            (rc == FAT12_OK && got != len))) __builtin_trap();
    return rc != FAT12_OK || got != len;
}

/* AH=4Bh AL=03h LOAD OVERLAY on the kernel (ADR-0003 Amendment DEC-19; see
 * loader.h). The find's sector scratch is free once the entry is located, so
 * it doubles as the core's header/relocation scratch (no new BSS). */
_Static_assert(BLOCKDEV_SECTOR_SIZE >= LOADER_OVL_SCRATCH,
               "g_load_sector must hold the overlay core's scratch");
loader_status_t loader_load_overlay(const char *name83, uint16_t dir_start,
                                    uint32_t load_linear, uint32_t reloc_factor,
                                    loader_ovl_owns_fn owns)
{
    loader_ovl_source_t source;
    uint32_t len = 0u;
    loader_status_t st = loader_find_file(name83, dir_start, &source.entry);
    if (st != LOADER_OK) {
        return st;
    }
    source.checked = 0;
    st = loader_overlay_core(loader_ovl_fat_read, &source, source.entry.file_size,
                             (uint8_t *)(uintptr_t)load_linear, load_linear,
                             reloc_factor, owns, loader_ovl_fat_check,
                             g_load_sector, &len);
    if (st == LOADER_ERR_FOREIGN_MZ) {
        loader_panic_foreign_mz(name83);   /* DEC-08a.5 verbatim -- noreturn */
    }
    return st;
}

loader_status_t loader_load_tenant(const char *name83, uint16_t dir_start,
                                   loader_carve_fn carve, void *user,
                                   void **out_block, tenant_plan_t *out)
{
    dir_entry_t de;
    int rc;
    uint32_t block_len;
    uint32_t got = 0u;
    uint8_t *block;
    loader_status_t st;

    if (out_block == 0 || out == 0 || carve == 0 || name83 == 0) {
        return LOADER_ERR_NULL_OUT;
    }
    *out_block = 0;
    st = loader_find_file(name83, dir_start, &de);
    if (st != LOADER_OK) {
        return st;                     /* no volume / absent / a folder / read */
    }
    if (de.file_size == 0u || de.file_size > (uint32_t)TBX_TENANT_IMAGE_MAX) {
        return LOADER_ERR_TOO_BIG;
    }

    /* Constant for a given file (the O-3 avail-stable invariant needs the code
     * block to be the same size every launch, so its GENERAL free-list slot is
     * reused exactly -- DEC-AC3-1's carve/free discipline). */
    block_len = (uint32_t)TBX_TENANT_PSP_BYTES + ((de.file_size + 15u) & ~15u);
    {
        /* spec/toolbox_gate.h Sec 8 (bead initech-w96l): the BSS a compiled
         * tenant declares as e_minalloc (MZ header word @0x0A, paragraphs) is
         * carved WITH the image -- read from the header before the carve,
         * bounded, never partial. A non-MZ file carves nothing extra and is
         * refused by loader_prepare_tenant as before. */
        uint8_t hdr[0x0C];
        uint32_t got_hdr = 0u, bss;
        rc = fat12_read_partial(g_load_vol, g_load_fat, g_load_fat_len, &de, 0u,
                                (uint32_t)sizeof hdr, hdr, g_load_cluster, &got_hdr);
        if (rc != FAT12_OK) {
            return LOADER_ERR_READ;
        }
        if (got_hdr == (uint32_t)sizeof hdr && hdr[0] == 'M' && hdr[1] == 'Z') {
            bss = ((uint32_t)hdr[0x0A] | ((uint32_t)hdr[0x0B] << 8)) * 16u;
            if (bss > (uint32_t)TBX_TENANT_BSS_MAX) {
                return LOADER_ERR_TOO_BIG;
            }
            block_len += bss;
        }
    }
    block = (uint8_t *)carve(block_len, user);
    if (block == 0) {
        return LOADER_ERR_NOMEM;
    }
    *out_block = block;

    rc = fat12_read_file(g_load_vol, g_load_fat, g_load_fat_len, &de,
                         block + TBX_TENANT_PSP_BYTES,
                         block_len - (uint32_t)TBX_TENANT_PSP_BYTES,
                         g_load_cluster, &got);
    if (rc != FAT12_OK) {
        return LOADER_ERR_READ;
    }
    st = loader_prepare_tenant(block, block_len, got,
                               (uint32_t)(uintptr_t)block, out);
    if (st == LOADER_ERR_FOREIGN_MZ) {
        loader_panic_foreign_mz(name83);   /* DEC-08a.5 verbatim -- noreturn */
    }
    return st;
}

#endif /* freestanding vs hosted */
