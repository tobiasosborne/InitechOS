/* test_keep.c -- host oracle for INT 21h AH=31h KEEP (Terminate and Stay
 * Resident) and the resident-block MCB memory model (beads initech-bo40).
 *
 * Two layers, both against the REAL artifact translation units (mcb.c + int21.c,
 * the SAME TUs the kernel runs), compiled HOSTED:
 *
 *  PART A -- the PURE allocator layer (mcb_keep_resident, mcb.c): set up an arena,
 *    claim it for a PSP (the authentic single-big-block a freshly-loaded program
 *    owns), KEEP-shrink it to N data paragraphs, and assert: the block shrank to
 *    N, it is re-owned MCB_OWNER_SYSTEM (resident), a subsequent mcb_alloc by the
 *    program does NOT return the kept region, and a terminate-time
 *    mcb_free_owner(psp) SKIPS the kept block (it survives) while reclaiming the
 *    program's NON-resident blocks. Plus the clamp + fail-loud guards.
 *
 *  PART B -- the SYSCALL SEAM (int21_dispatch AH=31h KEEP, int21.c): bind a real
 *    arena + PSP through the int21 seams, drive AH=31h via int21_dispatch with
 *    DX=keep paragraphs / AL=exit code, and assert the WHOLE contract end to end:
 *    the exit hook fired with AL, the PSP's block is now SYSTEM-owned at the kept
 *    size, a following AH=48h ALLOC avoids the kept region, AH=4Dh reports the
 *    exit code in AL and termination type 3 (KEEP) in AH, and a NORMAL terminate
 *    reclaim leaves the resident block standing.
 *
 * MUTATION (Rule 6), driven by make test-keep-mutant:
 *   -DMCB_MUTATE_KEEP_NO_RESIDENT : mcb_keep_resident shrinks the block but does
 *     NOT re-own it resident (it stays the terminating PSP's). The terminate
 *     reclaim then frees it and a later ALLOC re-hands the kept region, so the
 *     "kept region survives terminate" + "ALLOC avoids the kept region"
 *     assertions go RED -- proving those assertions actually bite.
 *
 * Self-contained CHECK harness (no -Iseed needed) so the bead's literal
 * `gcc -I os/milton -I spec` compile command works verbatim. CLAUDE.md Law 2
 * (oracle is truth), Rule 1 (RED->GREEN), Rule 6, Rule 11 (deterministic),
 * Rule 12 (ASCII).
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

#include "int21.h"
#include "mcb.h"
#include "psp.h"
#include "sft.h"   /* sft_init -- the terminate path closes the PSP's JFT handles */

/* ---- tiny zero-dependency CHECK harness (mirrors seed/test_assert.h) ------- */
static int g_checks = 0;
static int g_fails  = 0;
#define CHECK(cond, msg)                                                   \
    do {                                                                   \
        g_checks++;                                                        \
        if (!(cond)) {                                                     \
            g_fails++;                                                     \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__,      \
                    (msg));                                                \
        }                                                                  \
    } while (0)

/* ========================================================================== *
 * PART A -- the pure mcb_keep_resident layer (mcb.c), on a plain malloc arena.
 * ========================================================================== */

/* Header at paragraph `para` (the test knows the same layout mcb.c uses). */
static mcb_t *H(mcb_arena_t *a, uint32_t para)
{
    return (mcb_t *)(a->base + (size_t)para * 16u);
}

/* Walk the chain: does ANY block belong to `owner`? (resident-survival check). */
static int owner_present(mcb_arena_t *a, uint16_t owner)
{
    uint32_t para = 0u;
    for (;;) {
        mcb_t *h = H(a, para);
        if (h->owner == owner) {
            return 1;
        }
        if (h->signature == MCB_SIG_TERMINAL) {
            return 0;
        }
        para += 1u + h->size_paras;
    }
}

/* Find the (header) paragraph of the FIRST block owned by `owner`, or -1. */
static long owner_block_para(mcb_arena_t *a, uint16_t owner)
{
    uint32_t para = 0u;
    for (;;) {
        mcb_t *h = H(a, para);
        if (h->owner == owner) {
            return (long)para;
        }
        if (h->signature == MCB_SIG_TERMINAL) {
            return -1;
        }
        para += 1u + h->size_paras;
    }
}

static void part_a(void)
{
    const uint16_t PSP = 0x3000u;          /* a plausible flat-mode PSP owner   */

    /* --- A1: KEEP shrinks the program's block + marks it resident. --------- */
    {
        mcb_arena_t a;
        void *buf = malloc(256u * 16u);
        memset(buf, 0xCC, 256u * 16u);
        mcb_init(&a, buf, 256u);
        CHECK(mcb_set_arena_owner(&a, PSP) == 1, "A1: program claims its window");
        CHECK(H(&a, 0)->size_paras == 255u, "A1: block spans the whole window");

        /* KEEP 16 data paragraphs resident. */
        CHECK(mcb_keep_resident(&a, PSP, 16u) == MCB_OK, "A1: KEEP succeeds");
        CHECK(H(&a, 0)->size_paras == 16u, "A1: block shrank to 16 paragraphs");
        CHECK(H(&a, 0)->owner == MCB_OWNER_SYSTEM,
              "A1: kept block is re-owned RESIDENT (system)");
        CHECK(H(&a, 0)->owner != PSP,
              "A1: kept block is no longer the terminating PSP's");
        CHECK(mcb_chain_intact(&a), "A1: chain intact after KEEP");
        /* The freed tail (255-16-1 = 238 data paras) is one free block. */
        CHECK(H(&a, 17)->owner == MCB_OWNER_FREE, "A1: shrunk tail is free");
        CHECK(H(&a, 17)->size_paras == 238u, "A1: freed tail = 255-16-1");
        free(buf);
    }

    /* --- A2: a subsequent ALLOC does NOT return the kept region. ----------- */
    {
        mcb_arena_t a;
        void *buf = malloc(256u * 16u);
        memset(buf, 0xCC, 256u * 16u);
        mcb_init(&a, buf, 256u);
        mcb_set_arena_owner(&a, PSP);
        CHECK(mcb_keep_resident(&a, PSP, 16u) == MCB_OK, "A2: KEEP ok");

        /* The program (still PSP) allocs a heap block; it must come from the
         * freed tail (data para >= 18), NEVER overlap the kept [data 1..16]. */
        uint32_t dp = 0;
        CHECK(mcb_alloc(&a, PSP, 8u, &dp, 0) == MCB_OK, "A2: alloc from the tail");
        CHECK(dp >= 18u, "A2: alloc lands ABOVE the kept resident block");
        /* The kept block is untouched: still SYSTEM, still 16 paras at para 0. */
        CHECK(H(&a, 0)->owner == MCB_OWNER_SYSTEM, "A2: kept block still resident");
        CHECK(H(&a, 0)->size_paras == 16u, "A2: kept block still 16 paras");
        free(buf);
    }

    /* --- A3: terminate-reclaim SKIPS the resident block (it survives). ----- */
    {
        mcb_arena_t a;
        void *buf = malloc(256u * 16u);
        memset(buf, 0xCC, 256u * 16u);
        mcb_init(&a, buf, 256u);
        mcb_set_arena_owner(&a, PSP);
        CHECK(mcb_keep_resident(&a, PSP, 16u) == MCB_OK, "A3: KEEP ok");
        /* The program allocates a NON-resident heap block before exiting. */
        uint32_t dp = 0;
        CHECK(mcb_alloc(&a, PSP, 8u, &dp, 0) == MCB_OK, "A3: heap alloc");
        CHECK(owner_present(&a, PSP) == 1, "A3: PSP owns its heap block pre-exit");

        /* do_terminate's reclaim: free everything the terminating PSP owns. */
        uint32_t freed = mcb_free_owner(&a, PSP);
        CHECK(freed == 1u, "A3: reclaim frees the PSP's NON-resident block only");
        CHECK(owner_present(&a, PSP) == 0, "A3: no PSP-owned block survives");
        long kp = owner_block_para(&a, MCB_OWNER_SYSTEM);
        CHECK(kp == 0, "A3: the resident block SURVIVES the terminate reclaim");
        CHECK(H(&a, 0)->size_paras == 16u, "A3: resident block still 16 paras");
        CHECK(mcb_chain_intact(&a), "A3: chain intact after reclaim");
        free(buf);
    }

    /* --- A4: clamp -- DX > the block size keeps AT MOST the program's block. */
    {
        mcb_arena_t a;
        void *buf = malloc(64u * 16u);
        memset(buf, 0xCC, 64u * 16u);
        mcb_init(&a, buf, 64u);             /* block data size = 63 paras       */
        mcb_set_arena_owner(&a, PSP);
        CHECK(mcb_keep_resident(&a, PSP, 1000u) == MCB_OK, "A4: KEEP clamps ok");
        CHECK(H(&a, 0)->size_paras == 63u, "A4: clamped to the block's own size");
        CHECK(H(&a, 0)->owner == MCB_OWNER_SYSTEM, "A4: still marked resident");
        CHECK(H(&a, 0)->signature == MCB_SIG_TERMINAL,
              "A4: a no-split clamp keeps the terminal 'Z' (whole arena)");
        free(buf);
    }

    /* --- A5: fail loud -- KEEP on an owner that owns no block / a bad owner. */
    {
        mcb_arena_t a;
        void *buf = malloc(64u * 16u);
        memset(buf, 0xCC, 64u * 16u);
        mcb_init(&a, buf, 64u);
        mcb_set_arena_owner(&a, PSP);
        /* An owner that holds NO block -> BAD_BLOCK (the do_keep fail-loud case). */
        CHECK(mcb_keep_resident(&a, 0x9999u, 4u) == MCB_ERR_BAD_BLOCK,
              "A5: KEEP by an owner holding no block -> BAD_BLOCK");
        CHECK(H(&a, 0)->owner == PSP, "A5: the real owner's block is untouched");
        /* FREE / SYSTEM are not live processes -> BAD_BLOCK. */
        CHECK(mcb_keep_resident(&a, MCB_OWNER_FREE, 4u) == MCB_ERR_BAD_BLOCK,
              "A5: KEEP with owner FREE rejected");
        CHECK(mcb_keep_resident(&a, MCB_OWNER_SYSTEM, 4u) == MCB_ERR_BAD_BLOCK,
              "A5: KEEP with owner SYSTEM rejected");
        free(buf);
    }
}

/* ========================================================================== *
 * PART B -- the int21_dispatch AH=31h KEEP seam (int21.c), end to end.
 * ========================================================================== */

#define CF_BIT 0x1u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }

static int_frame_t fresh_frame(void)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;   /* IF + reserved bit1; CF clear initially */
    return f;
}

/* Low-4-GiB allocation so flat pointers fit a uint32_t segment computation. */
static void *alloc_low(size_t n)
{
    void *p = MAP_FAILED;
#ifdef MAP_32BIT
    p = mmap(NULL, n, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#endif
    if (p == MAP_FAILED) {
        return NULL;
    }
    if ((uintptr_t)p > 0xFFFFFFFFu) {
        munmap(p, n);
        return NULL;
    }
    return p;
}

/* The exit hook the dispatcher's terminate path fires. The host hook RECORDS the
 * code + that it fired, then RETURNS (the kernel hook never returns -- it longjmps
 * back to the loader). So a KEEP/terminate via int21_dispatch is observable. */
static int     g_exit_fired = 0;
static uint8_t g_exit_code  = 0xFF;
static void rec_exit(uint8_t code) { g_exit_fired = 1; g_exit_code = code; }

static psp_t  *g_psp;
static uint16_t g_psp_seg;
static uint8_t *g_arena_buf;
static uint32_t g_arena_base_linear;
static uint16_t g_arena_seg_base;
#define ARENA_PARAS 4096u             /* 64 KiB arena */

static void bind_psp(void)
{
    psp_params_t params;
    params.alloc_end_linear  = 0x00070000u;
    params.env_linear        = 0u;
    params.parent_psp_linear = 0u;
    params.cmd_tail          = (const char *)0;
    params.cmd_tail_len      = 0u;
    (void)psp_build(g_psp, &params);
    int21_set_psp((struct psp *)g_psp);
    g_psp_seg = (uint16_t)(((uintptr_t)g_psp >> 4) & 0xFFFFu);
}

/* Re-bind a fresh whole-window arena owned by the current PSP (what the loader's
 * int21_mcb_reset does -- the single-big-block a freshly loaded program owns). */
static void reset_arena(void)
{
    int21_set_mcb_arena(g_arena_buf, ARENA_PARAS, g_arena_base_linear);
    (void)int21_mcb_reset();
}

/* Owner of the data paragraph for DOS segment `seg` (walks the live chain). */
static uint16_t seg_owner(uint16_t seg)
{
    uint32_t want_data_para = (uint32_t)seg - g_arena_seg_base;
    uint32_t para = 0u;
    for (;;) {
        mcb_t *h = (mcb_t *)(g_arena_buf + (size_t)para * 16u);
        uint32_t data_para = para + 1u;
        if (data_para == want_data_para) {
            return h->owner;
        }
        if (h->signature == MCB_SIG_TERMINAL) {
            return 0xDEADu;        /* not a block boundary */
        }
        para += 1u + h->size_paras;
    }
}

/* Drive one INT 21h call with a pre-zeroed frame (AH/AL/BX/CX/DX preset). */
static int_frame_t call(uint8_t ah, uint8_t al, uint32_t ebx,
                        uint32_t ecx, uint32_t edx)
{
    int_frame_t f = fresh_frame();
    f.eax = ((uint32_t)ah << 8) | (uint32_t)al;
    f.ebx = ebx;
    f.ecx = ecx;
    f.edx = edx;
    f.eflags |= CF_BIT;            /* preload CF=1 -> prove success CLEARS it */
    int21_dispatch(&f);
    return f;
}

static void part_b(void)
{
    g_psp       = (psp_t *)alloc_low(sizeof(psp_t));
    g_arena_buf = (uint8_t *)alloc_low((size_t)ARENA_PARAS * 16u);
    CHECK(g_psp != NULL, "B: PSP buffer in low 4 GiB");
    CHECK(g_arena_buf != NULL, "B: arena buffer in low 4 GiB");
    if (g_psp == NULL || g_arena_buf == NULL) {
        return;                    /* MAP_32BIT unavailable -- skip the seam */
    }
    g_arena_base_linear = (uint32_t)(uintptr_t)g_arena_buf;
    g_arena_seg_base    = (uint16_t)((g_arena_base_linear >> 4) & 0xFFFFu);

    /* The terminate path (do_terminate -> sft_close_process) walks the PSP's JFT
     * into the system SFT, so establish the resident device slots 0..3 exactly as
     * SYSINIT does -- otherwise the inherited standard handles point at FREE slots
     * and sft_close_process fails loud (Rule 2). */
    sft_init();

    int21_set_exit(rec_exit);
    bind_psp();
    reset_arena();

    /* The program's whole-window block is data paragraph 1 (header at para 0);
     * its DOS segment = seg_base + 1. */
    uint16_t prog_seg = (uint16_t)(g_arena_seg_base + 1u);

    /* --- B1: AH=31h KEEP DX=16, AL=7 -> terminate fired with code 7. ------- */
    {
        g_exit_fired = 0; g_exit_code = 0xFF;
        int_frame_t f = call(0x31u, 7u, 0u, 0u, 16u);   /* DL/DX = 16 paras */
        CHECK(g_exit_fired == 1, "B1: KEEP fired the terminate/exit hook");
        CHECK(g_exit_code == 7u, "B1: exit code AL=7 passed to the hook");
        (void)f;
        /* The program's block is now SYSTEM-owned (resident) at 16 paragraphs. */
        CHECK(seg_owner(prog_seg) == MCB_OWNER_SYSTEM,
              "B1: the program's block is now RESIDENT (system-owned)");
    }

    /* --- B2: AH=4Dh GET RETURN CODE -> AL=7, AH=3 (KEEP termination type). - */
    {
        int_frame_t f = call(0x4Du, 0u, 0u, 0u, 0u);
        CHECK(frame_cf(&f) == 0, "B2: 4Dh -> CF=0");
        CHECK((uint8_t)(f.eax & 0xFFu) == 7u, "B2: 4Dh AL = the KEEP exit code 7");
        CHECK((uint8_t)((f.eax >> 8) & 0xFFu) == 3u,
              "B2: 4Dh AH = 3 (terminated by KEEP / TSR)");
    }

    /* --- B3: a subsequent AH=48h ALLOC avoids the kept region. ------------- *
     * After KEEP the kept block is resident at the window base. To ALLOC, the
     * kernel re-binds the kernel PSP as current (the loader restores it after a
     * child run); we model that by binding a DIFFERENT current PSP, then ALLOC.
     * The returned segment must lie ABOVE the kept 16-paragraph block. */
    {
        psp_t *kpsp = (psp_t *)alloc_low(sizeof(psp_t));
        CHECK(kpsp != NULL, "B3: kernel-PSP buffer in low 4 GiB");
        if (kpsp != NULL) {
            int21_set_psp((struct psp *)kpsp);   /* current = a different PSP */
            int_frame_t a = call(0x48u, 8u, 0u, 0u, 0u);
            CHECK(frame_cf(&a) == 0, "B3: 48h ALLOC succeeds from the free tail");
            uint16_t got = (uint16_t)(a.eax & 0xFFFFu);
            /* The kept block is data paras [1..16]; its top segment = seg_base+16.
             * The allocation must start strictly above it. */
            CHECK(got > (uint16_t)(g_arena_seg_base + 16u),
                  "B3: ALLOC lands ABOVE the kept resident region");
            CHECK(seg_owner(prog_seg) == MCB_OWNER_SYSTEM,
                  "B3: the kept block is STILL resident after the ALLOC");
        }
    }

    /* --- B4: a normal terminate reclaim leaves the resident block standing. */
    {
        /* The current PSP (kpsp from B3) terminates normally (4Ch). Its reclaim
         * frees ITS blocks (the B3 heap alloc) but NOT the resident system block. */
        int_frame_t f = call(0x4Cu, 0u, 0u, 0u, 0u);
        (void)f;
        CHECK(seg_owner(prog_seg) == MCB_OWNER_SYSTEM,
              "B4: the resident block survives a NORMAL terminate reclaim");
    }
}

int main(void)
{
    part_a();
    part_b();
    printf("test_keep: %d checks, %d failures\n", g_checks, g_fails);
    return (g_fails == 0) ? 0 : 1;
}
