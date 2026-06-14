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
    out->params.env_linear        = ENV_BLOCK;          /* 0x5F000 -> seg 0x5F00 (2og) */
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

/* Hosted stub: the FAT-sourced load is kernel-only (it pulls the volume + the
 * asm transfer). The host oracle exercises the AH=4Bh register/validation logic
 * through int21's MOCK EXEC backend (test_exec.c), never this path. */
void loader_bind_fat_volume(const struct fat12_volume *vol) { (void)vol; }

loader_status_t load_program_from_fat(const char *name83, const char *cmd_tail,
                                      uint32_t cmd_tail_len, uint8_t *out_rc)
{
    (void)name83; (void)cmd_tail; (void)cmd_tail_len; (void)out_rc;
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

    /* Copy the program image to PROGRAM_IMAGE (0x30100). Volatile-correct: the
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

    /* Re-initialize the MCB memory arena (beads initech-509.6) so the freshly
     * loaded program owns its WHOLE window [PROGRAM_BASE, PROGRAM_ALLOC_END) as
     * one block (the authentic single-big-block a .COM owns at load). MUST run
     * AFTER int21_set_psp above so the arena's lone block is stamped with THIS
     * program's PSP as owner (a later child EXEC re-binds + re-resets for itself;
     * kmain restores the kernel PSP on return). int21_mcb_reset reads the now-
     * current PSP. With no arena bound (host loader oracle) this is a no-op. */
    (void)int21_mcb_reset();

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

/* ------------------------------------------------------------------------ *
 * FAT-sourced load (beads initech-saw): read a .COM BY NAME, then load_program.
 *
 * loader.c reads the named file off the mounted volume into the off-stack
 * staging buffer (LOAD_STAGING_BASE) and runs it. It caches the volume + the
 * whole FAT at bind time (fat12_read_file needs the FAT for the cluster-chain
 * walk). All scratch is kernel BSS -- NEVER a multi-KB buffer on the kernel
 * stack (spec/memory_map.h Risk 2). Ref: fat12.h (fat12_find / fat12_read_file).
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

loader_status_t load_program_from_fat(const char *name83, const char *cmd_tail,
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

    /* Locate the .COM in the (root) directory. */
    dir_entry_t de;
    int rc = fat12_find(g_load_vol, g_load_sector, name83, &de);
    if (rc == FAT12_ERR_NOT_FOUND) {
        return LOADER_ERR_NOT_FOUND;
    }
    if (rc != FAT12_OK) {
        return LOADER_ERR_READ;
    }

    /* Reject before reading anything that cannot fit the program region (the
     * staging cap is >= PROGRAM_IMAGE_MAX, so PROGRAM_IMAGE_MAX is the binding
     * limit; load_program re-checks). */
    if (de.file_size > PROGRAM_IMAGE_MAX || de.file_size > LOAD_STAGING_MAX) {
        return LOADER_ERR_TOO_BIG;
    }
    if (de.file_size == 0u) {
        /* An empty file is not a runnable program (load_program rejects zero
         * length too, but fail loud here with the FAT-specific bad-format-ish
         * code rather than read 0 bytes and run garbage). */
        return LOADER_ERR_TOO_BIG;   /* reuse: nothing runnable (size 0) */
    }

    /* Read the whole .COM into the off-stack staging buffer. */
    uint8_t *staging = (uint8_t *)(uintptr_t)LOAD_STAGING_BASE;
    uint32_t got = 0;
    rc = fat12_read_file(g_load_vol, g_load_fat, g_load_fat_len, &de,
                         staging, LOAD_STAGING_MAX, g_load_cluster, &got);
    if (rc != FAT12_OK) {
        return LOADER_ERR_READ;
    }

    /* Run it. Mark the load active across the run so a nested EXEC issued by the
     * child is rejected (the guard above). load_program copies staging DOWN to
     * PROGRAM_IMAGE (disjoint regions), so the staging buffer is free to be
     * clobbered only AFTER the copy -- which load_program does before the JMP. */
    g_load_active = 1;
    uint8_t rcv = 0;
    loader_status_t st = load_program(staging, got, cmd_tail, cmd_tail_len, &rcv);
    g_load_active = 0;

    if (st != LOADER_OK) {
        return st;   /* propagate load_program's fail-loud status (e.g. TOO_BIG) */
    }
    if (out_rc) {
        *out_rc = rcv;
    }
    return LOADER_OK;
}

#endif /* freestanding vs hosted */
