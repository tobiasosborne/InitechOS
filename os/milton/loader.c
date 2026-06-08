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

/* ------------------------------------------------------------------------ *
 * Pure prep: validate + lay out + compute psp_params (HOST-TESTABLE).
 * ------------------------------------------------------------------------ */

loader_status_t loader_prepare(const uint8_t *image, uint32_t image_len,
                               const char *cmd_tail, uint32_t cmd_tail_len,
                               loader_plan_t *out)
{
    /* Rule 2: fail loud on every bad input. The loader NEVER copies a NULL or
     * oversized image into the program region -- that would clobber the kernel
     * stack / framebuffer and silently corrupt the machine. */
    if (out == 0) {
        return LOADER_ERR_NULL_OUT;
    }
    if (image == 0) {
        return LOADER_ERR_NULL_IMAGE;
    }
    if (image_len == 0) {
        return LOADER_ERR_ZERO_LEN;
    }
    if (image_len > PROGRAM_IMAGE_MAX) {
        /* Would overrun the program region into the stack (Sec 3.2 gap proof). */
        return LOADER_ERR_TOO_BIG;
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
    out->image_src = image;

    /* The psp_build inputs (Sec 2.2 / 2.5 / 2.7 / 2.11; psp.h). The vestigial
     * segment fields are passed as flat LINEAR addresses; psp_build stores each
     * as a fake paragraph (linear >> 4). */
    out->params.alloc_end_linear  = PROGRAM_ALLOC_END;  /* 0x70000 -> seg 0x7000 */
    out->params.env_linear        = ENV_BLOCK;          /* 0x20200 -> seg 0x2020 */
    out->params.parent_psp_linear = 0u;                 /* no parent PSP yet (Sec 2.5) */
    out->params.cmd_tail          = cmd_tail;           /* may be NULL (no args) */
    out->params.cmd_tail_len      = cmd_tail_len;

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

#else /* freestanding kernel build */

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

loader_status_t load_program(const uint8_t *image, uint32_t image_len,
                             const char *cmd_tail, uint32_t cmd_tail_len,
                             uint8_t *out_exit_code)
{
    loader_plan_t plan;
    loader_status_t st = loader_prepare(image, image_len, cmd_tail,
                                        cmd_tail_len, &plan);
    if (st != LOADER_OK) {
        return st;   /* fail loud: the program is NOT run (Rule 2) */
    }

    /* Copy the program image to PROGRAM_IMAGE (0x20100). Volatile-correct: the
     * destination is a fixed physical region the loader owns. */
    loader_memcpy((uint8_t *)(uintptr_t)plan.image_dst, plan.image_src,
                  plan.image_len);

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
     * Order matters: read everything we need into EAX/ESP/EBX from MEMORY
     * operands; the final two writes (ESP, EBX) happen LAST, immediately before
     * the jmp, so nothing the compiler emits depends on the now-switched stack.
     * "memory" tells GCC the asm reads/writes memory across the non-local flow. */
    __asm__ __volatile__(
        "mov %%esp, %0\n\t"          /* ctx.saved_esp = current kernel ESP        */
        "mov %%ebp, %1\n\t"          /* ctx.saved_ebp = current frame base        */
        "mov %%ebx, %2\n\t"          /* ctx.saved_ebx = current EBX               */
        "lea 1f, %%eax\n\t"          /* EAX = address of the return label (1:)    */
        "mov %%eax, %3\n\t"          /* ctx.return_eip = &1f                      */
        "mov %5, %%ebx\n\t"          /* EBX = PSP_ADDR (program's PSP pointer)    */
        "mov %6, %%esp\n\t"          /* switch to the program stack top (LAST)    */
        "jmp *%4\n\t"                /* JMP to the program entry (no return addr) */
        "1:\n\t"                     /* <- loader_exit_hook jumps back here       */
        : "=m"(ctx.saved_esp), "=m"(ctx.saved_ebp), "=m"(ctx.saved_ebx),
          "=m"(ctx.return_eip)
        : "m"(entry_eip), "m"(psp_ptr), "m"(stack_top)
        : "eax", "cc", "memory");

    /* Resumed here via loader_exit_hook's jmp. Unbind our hook + ctx so a later
     * stray 4Ch from kernel context does not jump through a stale context
     * (Sec 4.5). The caller (kmain) re-binds its own int21_exit_hook. */
    g_loader_ctx = 0;

    if (out_exit_code) {
        *out_exit_code = ctx.exit_code;
    }
    return LOADER_OK;
}

#endif /* freestanding vs hosted */
