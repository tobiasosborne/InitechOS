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
 *   in_place == 1 (FAT/za4m):  `image` is ignored (NULL); out->image_src = NULL.
 *                             The bytes are ALREADY at PROGRAM_IMAGE -- no copy.
 *
 * EITHER way the ONLY size bound is PROGRAM_IMAGE_MAX (~188 KiB = ENV_BLOCK -
 * PROGRAM_IMAGE). The old FAT path also rejected image_len > LOAD_STAGING_MAX
 * (64 KiB), because it staged the .COM through [0x70000,0x80000) before copying
 * down; za4m reads straight into PROGRAM_IMAGE, so that 64 KiB cap is GONE and
 * a 77 KiB SAMIR.COM now loads (it used to fail LOADER_ERR_TOO_BIG). */
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
    /* The baked path needs source bytes to copy FROM; the in-place path does
     * not (the image is already resident at PROGRAM_IMAGE -- there is nothing to
     * copy, so NULL is correct, not an error). */
    if (!in_place && image == 0) {
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
    out->params.alloc_end_linear  = PROGRAM_ALLOC_END;  /* 0x70000 -> seg 0x7000 */
    out->params.env_linear        = ENV_BLOCK;          /* 0x5F000 -> seg 0x5F00 (2og) */
    out->params.parent_psp_linear = 0u;                 /* no parent PSP yet (Sec 2.5) */
    out->params.cmd_tail          = cmd_tail;           /* may be NULL (no args) */
    out->params.cmd_tail_len      = cmd_tail_len;

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

loader_status_t loader_prepare_in_place(uint32_t image_len, const char *cmd_tail,
                                        uint32_t cmd_tail_len, loader_plan_t *out)
{
    /* FAT path (beads initech-za4m): the .COM is ALREADY at PROGRAM_IMAGE
     * (load_program_from_fat read it there directly). No source image; the only
     * size bound is PROGRAM_IMAGE_MAX. */
    return loader_prepare_core(/*image=*/(const uint8_t *)0, image_len, cmd_tail,
                               cmd_tail_len, /*in_place=*/1, out);
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
                                      uint32_t cmd_tail_len,
                                      uint8_t *out_exit_code)
{
    loader_plan_t plan;
    loader_status_t st = loader_prepare_in_place(image_len, cmd_tail,
                                                 cmd_tail_len, &plan);
    (void)out_exit_code;
    return st;
}

/* Hosted stub: the FAT-sourced load is kernel-only (it pulls the volume + the
 * asm transfer). The host oracle exercises the AH=4Bh register/validation logic
 * through int21's MOCK EXEC backend (test_exec.c), never this path. */
void loader_bind_fat_volume(const struct fat12_volume *vol) { (void)vol; }

loader_status_t load_program_from_fat(const char *name83, uint16_t dir_start,
                                      const char *cmd_tail,
                                      uint32_t cmd_tail_len, uint8_t *out_rc)
{
    (void)name83; (void)dir_start; (void)cmd_tail; (void)cmd_tail_len;
    (void)out_rc;
    return LOADER_ERR_NO_VOLUME;
}

#else /* freestanding kernel build */

#include "idt.h"          /* idt_get_gate / idt_install_trap -- live IDT vectors */

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

/* The single in-flight loader context. Single program at a time this milestone
 * (Sec 6.2 -- no process table). The exit hook reaches it through this global
 * because the int21 hook signature is void(uint8_t) and cannot carry &ctx. */
static loader_context_t *g_loader_ctx = 0;

/* Single-level guard (Rule 2): set while a program is running through
 * load_program(). A re-entrant load (nested EXEC from inside a running program)
 * would clobber g_loader_ctx + the program region, so load_program_from_fat
 * REJECTS it (LOADER_ERR_BUSY) rather than corrupt the machine. KERNEL/shell
 * EXEC (the common case, g_load_active == 0) proceeds. Nested EXEC is deferred
 * (a follow-up bead). Note: load_program() itself does not set this -- the saw
 * entry point owns the guard, because the baked run_baked() path (kmain) calls
 * load_program directly and must keep working unchanged. */
static uint8_t g_load_active = 0;

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
    loader_context_t *ctx = g_loader_ctx;

    /* If no context is bound we cannot safely unwind -- fail loud rather than
     * jump through a NULL (Rule 2). This should never happen: the hook is only
     * bound while g_loader_ctx is set. */
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
 * IN-PLACE SAFETY PROOF (writing PROGRAM_IMAGE while the kernel/shell runs):
 *   The currently-running code is the KERNEL (the COMMAND.COM shell runs IN the
 *   kernel as a function, NOT as a separately-loaded program at PROGRAM_IMAGE --
 *   there is no second program region). EXEC is single-level (g_load_active guards
 *   re-entry in load_program_from_fat), so no OTHER program occupies
 *   [PROGRAM_IMAGE, ...] when the FAT reader writes there. The child loads at
 *   PROGRAM_IMAGE (0x30100); the parent (kernel) executes from its own link
 *   region [0x10000,0x30000) and its own stack at 0x80000+, both DISJOINT from
 *   [0x30100, PROGRAM_STACK_BOT). So reading the .COM straight into PROGRAM_IMAGE
 *   is exactly as safe as the OLD code that copied staging -> PROGRAM_IMAGE: both
 *   write the SAME region while the SAME kernel runs; za4m merely elides the
 *   intermediate 64 KiB staging buffer. (spec/memory_map.h gap proof; ADR-0009.) */
static loader_status_t loader_run_plan(loader_plan_t *plan_in, int do_copy,
                                       uint8_t *out_exit_code)
{
    loader_plan_t plan = *plan_in;

#ifdef LOADER_MUTATE_INPLACE_DOUBLE_HANDLE
    /* MUTANT (CLAUDE.md Rule 6; make test-loader-big-mutant only): force the copy
     * ON for the in-place path too -- re-imposing the pre-za4m double-handling
     * (the image, already at PROGRAM_IMAGE, would be copied FROM a NULL/garbage
     * src). The big-.COM oracle asserts the in-place plan carries image_src==NULL
     * and the run path must NOT copy; this mutant makes do_copy==1 unconditionally
     * so the oracle goes RED. NEVER define in a real build. */
    do_copy = 1;
#endif

    /* Copy the program image to PROGRAM_IMAGE (0x30100) ONLY on the baked path.
     * On the in-place path (za4m) the bytes are already there -- skip the copy
     * (plan.image_src is NULL; a copy would dereference NULL). Volatile-correct:
     * the destination is a fixed physical region the loader owns. */
    if (do_copy) {
        loader_memcpy((uint8_t *)(uintptr_t)plan.image_dst, plan.image_src,
                      plan.image_len);
    }

    /* Build the 2-byte empty environment block at ENV_BLOCK (Sec 2.7): the
     * double-NUL = a valid empty environment. */
    {
        uint8_t *env = (uint8_t *)(uintptr_t)ENV_BLOCK;
        env[0] = 0x00;
        env[1] = 0x00;
    }

    /* Build the PSP at PROGRAM_BASE (psp_build, beads initech-509.4). A clamp
     * (non-zero return) means the command tail was too long; that is a loud
     * caller bug for a baked program but not fatal -- the tail is clamped, never
     * overflowed. We ignore the count here (the baked program has no tail). */
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

    /* Bind the MCB heap arena to the window DISJOINT from this loaded program
     * (beads initech-1q4u; ADR-0009 DEC-04). The arena starts ABOVE the loaded
     * image+BSS (plan.arena_base) and ends at PROGRAM_ARENA_CEIL (plan.arena_ceil),
     * so a 48h ALLOC can never return memory inside the program image/env/stack --
     * the authentic DOS model (DOS hands the program the block AFTER its image).
     * This SUPERSEDES the old int21_mcb_reset() over [PROGRAM_BASE, PROGRAM_ALLOC_
     * END), which overlaid the running program (ADR-0009 Sec 1 latent corruption).
     * MUST run AFTER int21_set_psp above so the lone block is stamped with THIS
     * program's PSP as owner. If the image was too large to leave a positive arena
     * (plan.arena_present == 0) the arena is left UNBOUND -- a 48h ALLOC then
     * reports insufficient memory rather than overlapping (fail loud, Rule 2). */
    if (plan.arena_present) {
        (void)int21_mcb_bind_program(plan.arena_base, plan.arena_ceil);
    } else {
        int21_set_mcb_arena(0, 0u, 0u);   /* no heap -> 48h insufficient, no overlap */
    }

    /* Reset the current working directory to the root for the freshly loaded
     * program (beads initech-mzxa; ti8 Layer 2). Mirrors int21_mcb_reset above:
     * a launched program starts at the root (the simplest authentic ti8 model --
     * a child does not inherit the parent's CWD; with no CHDIR writer yet every
     * CWD is the root regardless). The kernel restores its own (saved) CWD after
     * the child returns, alongside the PSP/exit-hook restore (kmain). */
    int21_cwd_reset();

    /* --- The control transfer + return-to-loader (Sec 4; Risk 1). --- */
    loader_context_t ctx;
    ctx.exit_code  = 0;
    ctx.exited     = 0;
    ctx.saved_ebx  = 0;
    ctx.saved_ebp  = 0;
    ctx.saved_esp  = 0;
    ctx.return_eip = 0;

    /* Save the previous exit hook so kernel-context 4Ch (outside a load) keeps
     * its existing cli;hlt behavior afterward (Sec 4.5 / Risk 3). int21.h has no
     * getter, so we restore via the kernel's known binder after we return: the
     * caller (kmain) re-binds int21_exit_hook. We bind our hook + ctx here. */
    g_loader_ctx = &ctx;
    int21_set_exit(loader_exit_hook);

    /* Pin the three transfer targets into plain locals so the asm can reference
     * them as MEMORY operands -- i386 has too few registers to carry entry +
     * stack_top + psp_addr in registers alongside the GPR clobbers (the asm
     * would over-constrain). EAX is the only scratch register the asm needs. */
    const uint32_t entry_eip = plan.entry;
    const uint32_t stack_top = plan.stack_top;
    const uint32_t psp_ptr   = plan.psp_addr;

    /* The asm: capture ESP/EBP/EBX + a return label into ctx, switch to the
     * program stack, load EBX = PSP_ADDR (the program's pointer to its own PSP,
     * Sec 3.3), and JMP (not CALL -- Sec 4.3) to the program entry. When the
     * program terminates, loader_exit_hook restores ESP/EBP/EBX and jumps to the
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
     * reads/writes memory across the non-local flow. */
    __asm__ __volatile__(
        "mov %%esp, %0\n\t"          /* ctx.saved_esp = current kernel ESP        */
        "mov %%ebp, %1\n\t"          /* ctx.saved_ebp = current frame base        */
        "mov %%ebx, %2\n\t"          /* ctx.saved_ebx = current EBX               */
        "lea 1f, %%eax\n\t"          /* EAX = address of the return label (1:)    */
        "mov %%eax, %3\n\t"          /* ctx.return_eip = &1f                      */
        "mov %4, %%eax\n\t"          /* EAX = entry_eip, read BEFORE the switch    */
        "mov %5, %%ebx\n\t"          /* EBX = PSP_ADDR (program's PSP pointer)    */
        "mov %6, %%esp\n\t"          /* switch to the program stack top (LAST)    */
        "jmp *%%eax\n\t"             /* JMP via REGISTER (no post-switch mem read) */
        "1:\n\t"                     /* <- loader_exit_hook jumps back here       */
        : "=m"(ctx.saved_esp), "=m"(ctx.saved_ebp), "=m"(ctx.saved_ebx),
          "=m"(ctx.return_eip)
        : "m"(entry_eip), "m"(psp_ptr), "m"(stack_top)
        : "eax", "cc", "memory");

    /* Resumed here via loader_exit_hook's jmp. Unbind our hook + ctx so a later
     * stray 4Ch from kernel context does not jump through a stale context
     * (Sec 4.5). The caller (kmain) re-binds its own int21_exit_hook. */
    g_loader_ctx = 0;

    /* Restore the parent's INT 22h/23h/24h vectors into the live IDT (beads
     * initech-509.8; ADR-0003 DEC-10). If the child SETVECT'd its own 23h/24h
     * (INT 21h AH=25h) while running, the parent's handlers must be reinstated
     * now that the child has terminated (DOS-authentic). We restore from the
     * SAME PSP we saved into (plan.psp_addr == PROGRAM_BASE), which is still
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
    loader_plan_t plan;
    loader_status_t st = loader_prepare(image, image_len, cmd_tail,
                                        cmd_tail_len, &plan);
    if (st != LOADER_OK) {
        return st;   /* fail loud: the program is NOT run (Rule 2) */
    }
    return loader_run_plan(&plan, /*do_copy=*/1, out_exit_code);
}

loader_status_t load_program_in_place(uint32_t image_len, const char *cmd_tail,
                                      uint32_t cmd_tail_len,
                                      uint8_t *out_exit_code)
{
    /* In-place path (beads initech-za4m; ADR-0009 companion to DEC-04): the .COM
     * is ALREADY resident at PROGRAM_IMAGE (load_program_from_fat read it there
     * directly off the FAT volume, lifting the old 64 KiB LOAD_STAGING_MAX cap to
     * PROGRAM_IMAGE_MAX). Validate + lay out via loader_prepare_in_place (no image
     * pointer; image_src comes back NULL), then run with NO copy (do_copy=0) --
     * the PSP build, env, vector save/restore, arena bind, stack switch, JMP and
     * return-to-loader are all the SAME shared path load_program uses. */
    loader_plan_t plan;
    loader_status_t st = loader_prepare_in_place(image_len, cmd_tail,
                                                 cmd_tail_len, &plan);
    if (st != LOADER_OK) {
        return st;   /* fail loud: the program is NOT run (Rule 2) */
    }
    return loader_run_plan(&plan, /*do_copy=*/0, out_exit_code);
}

/* ------------------------------------------------------------------------ *
 * FAT-sourced load (beads initech-saw; DIRECT-LOAD beads initech-za4m).
 *
 * loader.c reads the named .COM off the mounted volume DIRECTLY into the program
 * region (PROGRAM_IMAGE, 0x30100) and runs it IN PLACE via load_program_in_place
 * -- NO intermediate staging buffer, NO staging -> image copy. It caches the
 * volume + the whole FAT at bind time (fat12_read_file needs the FAT for the
 * cluster-chain walk). All scratch is kernel BSS -- NEVER a multi-KB buffer on
 * the kernel stack (spec/memory_map.h Risk 2).
 *
 * WHY DIRECT (beads initech-za4m; ADR-0009 companion to DEC-04): the OLD path
 * read the .COM into the 64 KiB LOAD_STAGING buffer [0x70000,0x80000) (it abuts
 * the kernel stack at 0x80000, so it CANNOT grow) and then copied DOWN to
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

/* The whole-FAT cache (1.44 MB FAT12: 9 sectors * 512 = 4608 bytes; round up so
 * a slightly larger FAT still fits). Filled once at bind by fat12_read_fat. */
static uint8_t  g_load_fat[12u * 512u];
static uint32_t g_load_fat_len = 0;

/* Sector scratch for fat12_find; cluster scratch for fat12_read_file (one
 * sector each on the 1.44 MB geometry). Kernel BSS. */
static uint8_t  g_load_sector[BLOCKDEV_SECTOR_SIZE];
static uint8_t  g_load_cluster[BLOCKDEV_SECTOR_SIZE];

void loader_bind_fat_volume(const struct fat12_volume *vol)
{
    if (vol == 0) {
        g_load_vol     = 0;
        g_load_fat_len = 0;
        return;
    }
    /* Cache the whole FAT (read-only this milestone). A read error leaves the
     * loader UNBOUND (fail loud, Rule 2 -- never half-initialised). */
    int rc = fat12_read_fat(vol, g_load_fat, (uint32_t)sizeof(g_load_fat));
    if (rc != FAT12_OK) {
        g_load_vol     = 0;
        g_load_fat_len = 0;
        return;
    }
    g_load_fat_len = (uint32_t)vol->bpb.sectors_per_fat *
                     (uint32_t)vol->bpb.bytes_per_sector;
    g_load_vol     = vol;
}

loader_status_t load_program_from_fat(const char *name83, uint16_t dir_start,
                                      const char *cmd_tail,
                                      uint32_t cmd_tail_len, uint8_t *out_rc)
{
    if (g_load_vol == 0) {
        return LOADER_ERR_NO_VOLUME;
    }
    /* Single-level guard (Rule 2 / stop condition): refuse a nested load rather
     * than clobber g_loader_ctx + the program region. */
    if (g_load_active) {
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
    if (dir_start == 0u) {
        rc = fat12_find(g_load_vol, g_load_sector, name83, &de);
    } else {
        uint32_t slot = 0u;
        rc = fat12_find_slot_in(g_load_vol, g_load_fat, g_load_fat_len,
                                dir_start, g_load_sector, name83, &de, &slot);
    }
    if (rc == FAT12_ERR_NOT_FOUND) {
        return LOADER_ERR_NOT_FOUND;
    }
    if (rc != FAT12_OK) {
        return LOADER_ERR_READ;
    }

    /* A leaf that resolves to a DIRECTORY is not a runnable program: refuse it
     * (Rule 2) rather than read the directory's cluster bytes into PROGRAM_IMAGE
     * as if they were a .COM. fat12_find / fat12_find_slot_in match by NAME only
     * (no attribute filter -- they back FINDFIRST too), so EXEC owns this guard.
     * Maps to 0x0002 (file not found) at the do_exec seam -- the DOS-authentic
     * "no runnable program here". The root case can only reach this for a root
     * directory entry, which no prior root-EXEC golden exercises (a .COM carries
     * attr 0x00/0x20, never DIR_ATTR_DIRECTORY), so root EXEC stays byte-identical. */
    if ((de.attribute & DIR_ATTR_DIRECTORY) != 0u) {
        return LOADER_ERR_NOT_FOUND;
    }

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

    /* Read the whole .COM DIRECTLY into PROGRAM_IMAGE (0x30100) -- no staging
     * buffer, no later copy (beads initech-za4m). dst_cap is PROGRAM_IMAGE_MAX so
     * fat12_read_file fails loud (FAT12_ERR_BUFFER) before overrunning the program
     * arena -- redundant with the file_size guard above but a hard backstop (Rule
     * 2). Safety (writing PROGRAM_IMAGE while the kernel runs): see the IN-PLACE
     * SAFETY PROOF on loader_run_plan -- the parent is the KERNEL (links at
     * [0x10000,0x30000), stack at 0x80000+, both disjoint from [0x30100,...)),
     * EXEC is single-level (g_load_active), so nothing else occupies PROGRAM_IMAGE.
     * This is exactly as safe as the OLD copy to 0x30100 -- same region, same
     * running kernel, minus the intermediate staging hop. */
    uint8_t *image = (uint8_t *)(uintptr_t)PROGRAM_IMAGE;
    uint32_t got = 0;
    rc = fat12_read_file(g_load_vol, g_load_fat, g_load_fat_len, &de,
                         image, PROGRAM_IMAGE_MAX, g_load_cluster, &got);
    if (rc != FAT12_OK) {
        return LOADER_ERR_READ;
    }

    /* Run it IN PLACE (beads initech-za4m). Mark the load active across the run so
     * a nested EXEC issued by the child is rejected (the guard above). The image
     * is ALREADY at PROGRAM_IMAGE (we just read it there), so load_program_in_place
     * does everything EXCEPT the copy -- builds the PSP, env, saves/restores the
     * INT 22/23/24 vectors, binds the disjoint MCB arena, switches the stack, JMPs
     * to PROGRAM_IMAGE, and handles return-to-loader -- via the SAME shared path
     * load_program uses for the baked demo. */
    g_load_active = 1;
    uint8_t rcv = 0;
    loader_status_t st = load_program_in_place(got, cmd_tail, cmd_tail_len, &rcv);
    g_load_active = 0;

    if (st != LOADER_OK) {
        return st;   /* propagate the in-place loader's fail-loud status (TOO_BIG) */
    }
    if (out_rc) {
        *out_rc = rcv;
    }
    return LOADER_OK;
}

#endif /* freestanding vs hosted */
