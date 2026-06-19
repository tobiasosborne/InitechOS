/* test_arena_disjoint.c -- host oracle for the AH=48h MCB arena being PROVABLY
 * DISJOINT from the loaded program (beads initech-1q4u; ADR-0009 DEC-04).
 *
 * THE BUG (ADR-0009 Sec 1 / DEC-04): the AH=48h heap arena used to be bound over
 * the WHOLE program window [PROGRAM_BASE, PROGRAM_ALLOC_END) with flat base ==
 * PROGRAM_BASE -- the EXACT region the loaded program image (0x30100), its PSP
 * (0x30000), env (0x5F000) and stack (top 0x6FFFC) occupy. So a 48h ALLOC handed
 * a running program memory OVERLAPPING its own code/BSS/stack. Latent for toy
 * .COMs (they never call 48h; the emulator zero-inits RAM), but SAMIR -- the
 * first heap-using app -- would corrupt itself.
 *
 * THE FIX (DEC-04): the loader computes the arena window DISJOINT from the loaded
 * program -- base = roundup_para(PROGRAM_IMAGE + image_len + PROGRAM_BSS_RESERVE),
 * ceiling = PROGRAM_ARENA_CEIL (== ENV_BLOCK) -- and binds it via
 * int21_mcb_bind_program. See spec/memory_map.h's ARENA DISJOINTNESS INVARIANT.
 *
 * THIS ORACLE proves the invariant TWO ways (Law 2):
 *   A. loader_prepare's COMPUTED window (the pure seam) for several image sizes is
 *      disjoint: arena_base >= PROGRAM_IMAGE + image_len, arena_ceil <=
 *      PROGRAM_STACK_BOT, and the env+stack are above the ceiling.
 *   B. the LIVE AH=48h dispatch: bind the arena at the SAME real-kernel linear
 *      base loader_prepare computed, drive the authentic 4Ah-shrink-then-48h-alloc
 *      (and a fill-the-window alloc loop), and assert EVERY returned block's flat
 *      linear address satisfies  block >= image_end  AND  block + size <=
 *      PROGRAM_STACK_BOT -- i.e. disjoint from [PROGRAM_IMAGE, image_end) and from
 *      [PROGRAM_STACK_BOT, PROGRAM_STACK_TOP].
 *
 * Compiles HOSTED against the REAL artifact loader.c + int21.c + mcb.c + sft.c +
 * psp.c + irq.c (the SAME TUs the kernel runs; loader.c's asm path is elided in
 * the hosted build). The arena buffer lives in the low 4 GiB (MAP_32BIT) but is
 * BOUND with base_linear == the real kernel arena_base so the reported DOS
 * segments map to the kernel's disjoint linear addresses -- we assert on THOSE
 * (the addresses the kernel would hand the program), never dereference them.
 *
 * MUTATION (Rule 6), driven by make test-arena-disjoint-mutant:
 *   -DLOADER_MUTATE_ARENA_OVERLAP : loader_prepare reverts arena_base to
 *     PROGRAM_BASE (the EXACT pre-fix bug). The arena then overlays the program;
 *     a 48h block lands inside [PROGRAM_IMAGE, image_end) or the stack -> the
 *     disjointness assertions go RED. A mutant that PASSES means the oracle is
 *     decoration.
 *
 * CLAUDE.md Law 1 (cite ADR-0009 DEC-04 + spec/memory_map.h), Law 2 (oracle is
 * truth), Rule 1 (RED->GREEN), Rule 6 (mutation-prove), Rule 11 (deterministic),
 * Rule 12 (ASCII).
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#include "loader.h"
#include "int21.h"
#include "mcb.h"
#include "psp.h"
#include "memory_map.h"
#include "test_assert.h"

TEST_HARNESS();

#define CF_BIT 0x1u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }

/* Low-4-GiB allocation so a flat pointer fits a uint32_t segment computation
 * (mirrors test_mcb_int21.c). */
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

/* A pseudo-image of `len` bytes (contents irrelevant to loader_prepare -- only
 * the length feeds the arena computation). Static so its address is stable. */
static uint8_t g_image[0x20000];

static psp_t  *g_psp;
static uint16_t g_psp_seg;

static void bind_psp(void)
{
    psp_params_t params;
    params.alloc_end_linear  = PROGRAM_ALLOC_END;
    params.env_linear        = ENV_BLOCK;
    params.parent_psp_linear = 0u;
    params.cmd_tail          = (const char *)0;
    params.cmd_tail_len      = 0u;
    (void)psp_build(g_psp, &params);
    int21_set_psp((struct psp *)g_psp);
    g_psp_seg = (uint16_t)(((uintptr_t)g_psp >> 4) & 0xFFFFu);
}

/* Drive one INT 21h call with a pre-zeroed frame, AH preset, CF preloaded to 1. */
static int_frame_t call(uint8_t ah, uint32_t ebx, uint32_t ecx)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u | CF_BIT;     /* IF + reserved; CF=1 -> success clears */
    f.eax = (uint32_t)ah << 8;
    f.ebx = ebx;
    f.ecx = ecx;
    int21_dispatch(&f);
    return f;
}

/* Part A: the pure loader_prepare arena window is disjoint for a given image_len. */
static void check_prepare_disjoint(uint32_t image_len)
{
    loader_plan_t plan;
    loader_status_t st = loader_prepare(g_image, image_len, (const char *)0, 0u,
                                        &plan);
    CHECK(st == LOADER_OK, "loader_prepare must accept a valid image");
    CHECK(plan.arena_present == 1u, "a normal image must yield a present arena");

    uint32_t image_end = PROGRAM_IMAGE + image_len;

    /* arena_base is ABOVE the loaded image+BSS. */
    CHECK(plan.arena_base >= image_end,
          "arena_base must be at/above the loaded image end");
    CHECK(plan.arena_base >= image_end + PROGRAM_BSS_RESERVE,
          "arena_base must clear image_end + the BSS reserve");
    CHECK((plan.arena_base & 0xFu) == 0u,
          "arena_base must be paragraph-aligned (exact DOS segment math)");

    /* arena_ceil is at/below PROGRAM_STACK_BOT (in fact == ENV_BLOCK), so the
     * heap reaches neither the env block nor the 64 KiB stack. */
    CHECK(plan.arena_ceil <= PROGRAM_STACK_BOT,
          "arena_ceil must be at/below PROGRAM_STACK_BOT (below the stack)");
    CHECK(plan.arena_ceil == PROGRAM_ARENA_CEIL,
          "arena_ceil must be PROGRAM_ARENA_CEIL (== ENV_BLOCK)");
    CHECK(plan.arena_ceil <= ENV_BLOCK,
          "arena_ceil must not reach the env block (ENV_BLOCK)");
    CHECK(plan.arena_base < plan.arena_ceil,
          "a present arena spans a positive window");
}

/* Part B: every LIVE AH=48h block lands at a flat linear address disjoint from
 * the program image and the stack. Binds the arena at the kernel-real linear
 * base so the reported DOS segments map to the kernel's disjoint addresses. */
static void check_live_alloc_disjoint(uint32_t image_len, uint8_t *arena_buf,
                                      size_t arena_buf_paras)
{
    loader_plan_t plan;
    loader_status_t st = loader_prepare(g_image, image_len, (const char *)0, 0u,
                                        &plan);
    CHECK(st == LOADER_OK, "loader_prepare OK for the live-alloc image");
    CHECK(plan.arena_present == 1u, "live-alloc image yields a present arena");

    uint32_t image_end = PROGRAM_IMAGE + image_len;
    uint32_t want_paras = (plan.arena_ceil - plan.arena_base) / 16u;
    /* The host arena buffer must be large enough for the kernel window's paras. */
    CHECK((size_t)want_paras <= arena_buf_paras,
          "host arena buffer covers the kernel window paragraphs");
    if ((size_t)want_paras > arena_buf_paras) {
        return;
    }

    /* Bind the REAL arena buffer over `want_paras`, but tell int21 the base LINEAR
     * address is the KERNEL value (plan.arena_base) -- so reported DOS segments
     * map (seg<<4) to the kernel's disjoint flat addresses, which we assert on.
     * int21_mcb_bind_program needs base_linear == the buffer it reads from, so we
     * cannot use it here (the host buffer != plan.arena_base). We bind the seam
     * directly: int21_set_mcb_arena(host_buf, want_paras, KERNEL_base_linear),
     * then int21_mcb_reset to hand it to the current PSP (the loader's effect). */
    int21_set_mcb_arena(arena_buf, want_paras, plan.arena_base);
    (void)int21_mcb_reset();   /* whole window -> the current PSP (single big block) */

    uint16_t seg_base = (uint16_t)((plan.arena_base >> 4) & 0xFFFFu);

    /* The program owns the whole window; the authentic path is 4Ah-shrink then
     * 48h-alloc from the freed tail. Shrink the program block (data paragraph 1,
     * segment seg_base+1) to 16 paragraphs. */
    uint16_t prog_seg = (uint16_t)(seg_base + 1u);
    {
        int_frame_t f = call(0x4Au, (uint32_t)prog_seg, 16u);
        CHECK(frame_cf(&f) == 0, "4Ah shrink the program block -> CF=0");
    }

    /* Now repeatedly ALLOC blocks until the window is exhausted, asserting each
     * block's flat extent is disjoint from the image and the stack. A fixed
     * per-alloc size; loop a bounded number of times (Rule 2: never unbounded). */
    const uint32_t blk_paras = 64u;        /* 1 KiB per block */
    int allocations = 0;
    for (int i = 0; i < 4096; i++) {
        int_frame_t f = call(0x48u, blk_paras, 0u);
        if (frame_cf(&f) != 0) {
            break;     /* window exhausted -- insufficient memory (expected end) */
        }
        uint16_t seg = (uint16_t)(f.eax & 0xFFFFu);
        /* Flat linear of the block = seg << 4 (the kernel-disjoint address). */
        uint32_t blk_lin = (uint32_t)seg << 4;
        uint32_t blk_end = blk_lin + blk_paras * 16u;

        /* DISJOINT from the loaded program image [PROGRAM_IMAGE, image_end). */
        CHECK(blk_lin >= image_end,
              "48h block must start at/above the loaded image end");
        CHECK(blk_lin >= PROGRAM_IMAGE,
              "48h block must not be below the program image base");
        /* DISJOINT from the program stack [PROGRAM_STACK_BOT, PROGRAM_STACK_TOP]. */
        CHECK(blk_end <= PROGRAM_STACK_BOT,
              "48h block must end at/below PROGRAM_STACK_BOT (clear of the stack)");
        /* And inside the computed arena window (sanity: never above the ceiling). */
        CHECK(blk_end <= plan.arena_ceil,
              "48h block must lie inside the arena ceiling");
        allocations++;
    }
    CHECK(allocations > 0,
          "at least one 48h ALLOC must succeed from the disjoint window");
}

/* Part D: the ACTUAL on-target program flow (bead initech-hdlb / S8.2). The loader
 * binds the disjoint arena via int21_mcb_bind_program and the program's FIRST
 * AH=48h carves its heap STRAIGHT from the free region -- with NO AH=4Ah SETBLOCK
 * first (pal_milton.c's documented contract: "AH=48h draws straight from it").
 *
 * This is the flow SAMIR actually uses, distinct from Part C (which binds via
 * int21_set_mcb_arena + int21_mcb_reset and SETBLOCK-shrinks first). The bug this
 * locks down: an earlier int21_mcb_bind_program handed the WHOLE disjoint arena to
 * the PSP (int21_mcb_reset), leaving ZERO free space, so a no-SETBLOCK 48h returned
 * insufficient-memory and SAMIR panicked at construction (the silent triple-fault
 * the S8.2 emu gate surfaced). int21_mcb_bind_program MUST leave the arena FREE so
 * the direct 48h succeeds; with the old reowning bind, the very first CHECK below
 * (a direct 48h with CF=0) goes RED.
 *
 * The arena buffer is the host RAM; we pass its OWN linear base to
 * int21_mcb_bind_program (base_linear == the buffer it reads from -- the function's
 * documented requirement), so the reported DOS segments map (seg<<4) back into the
 * buffer. We assert the alloc succeeds, the block lies inside the bound window, and
 * a second alloc also succeeds (the arena is genuinely a free pool, not a one-shot). */
static void check_bind_program_direct_alloc(uint8_t *arena_buf, size_t arena_buf_paras)
{
    /* A modest disjoint window inside the host buffer. base_linear == the buffer's
     * own flat address (paragraph-aligned: alloc_low + MAP gives page alignment). */
    uint32_t base_lin = (uint32_t)(uintptr_t)arena_buf;
    CHECK((base_lin & 0xFu) == 0u, "Part D: host arena buffer is paragraph-aligned");

    /* Use up to 1024 paragraphs (16 KiB) of the buffer for the window. */
    uint32_t win_paras = (arena_buf_paras < 1024u) ? (uint32_t)arena_buf_paras : 1024u;
    uint32_t ceil_lin  = base_lin + win_paras * 16u;

    int bound = int21_mcb_bind_program(base_lin, ceil_lin);
    CHECK(bound == 1, "Part D: int21_mcb_bind_program binds the disjoint window");

    /* Segment math runs in the kernel's 16-bit DOS-segment space:
     * arena_seg_base() = (base_linear >> 4) & 0xFFFF, and a 48h block reports
     * seg = arena_seg_base() + data_para. The host buffer can sit at a high flat
     * address (> 1 MiB), so reconstructing the FULL flat address from the 16-bit
     * segment is impossible (and not what the kernel does). We assert on the
     * data-paragraph OFFSET from the arena base instead -- which is exactly the
     * disjointness-relevant quantity (the block sits inside [0, win_paras) of the
     * window). seg_base is the low-16 of the arena base segment. */
    uint16_t seg_base16 = (uint16_t)((base_lin >> 4) & 0xFFFFu);

    /* THE KEYSTONE: a DIRECT AH=48h (no AH=4Ah SETBLOCK first) MUST succeed,
     * because the disjoint arena was left FREE. With the reowning bug this is the
     * exact assertion that bit RED on-target (SAMIR's pal_milton_make panicked). */
    int_frame_t f = call(0x48u, 256u /* paras */, 0u);
    CHECK(frame_cf(&f) == 0,
          "Part D: a DIRECT 48h (no SETBLOCK) succeeds from the FREE disjoint arena");

    uint16_t seg = (uint16_t)(f.eax & 0xFFFFu);
    uint16_t off = (uint16_t)(seg - seg_base16);   /* data_para offset into window */
    CHECK(off >= 1u,
          "Part D: the 48h block sits ABOVE the arena base header (data_para >= 1)");
    CHECK((uint32_t)off + 256u <= win_paras,
          "Part D: the 48h block lies wholly inside the bound window");

    /* A SECOND alloc also succeeds -> the arena is a genuine free pool, and the
     * first alloc carved a tail (not a one-shot whole-window grab). */
    int_frame_t f2 = call(0x48u, 64u, 0u);
    CHECK(frame_cf(&f2) == 0,
          "Part D: a SECOND 48h also allocates from the remaining free arena");
    uint16_t seg2 = (uint16_t)(f2.eax & 0xFFFFu);
    CHECK(seg2 != seg,
          "Part D: the two allocations return DISTINCT segments (bump, not alias)");
    (void)ceil_lin;

    /* Unbind so later parts that re-bind start clean. */
    int21_set_mcb_arena(0, 0u, 0u);
}

int main(void)
{
    g_psp = (psp_t *)alloc_low(sizeof(psp_t));
    CHECK(g_psp != NULL, "PSP buffer in low 4 GiB");
    if (g_psp == NULL) {
        return TEST_SUMMARY("test_arena_disjoint");
    }
    bind_psp();

    /* A host arena buffer big enough for the largest kernel window we exercise
     * (the smallest image -> the largest window: ENV_BLOCK - roundup(PROGRAM_IMAGE
     * + PROGRAM_BSS_RESERVE)). Size it generously: the whole image arena span. */
    size_t buf_paras = (size_t)(PROGRAM_ARENA_CEIL - PROGRAM_IMAGE) / 16u;
    uint8_t *arena_buf = (uint8_t *)alloc_low(buf_paras * 16u);
    CHECK(arena_buf != NULL, "host arena buffer in low 4 GiB");
    if (arena_buf == NULL) {
        return TEST_SUMMARY("test_arena_disjoint");
    }

    /* === Part A: the computed window is disjoint for a spread of image sizes. == */
    check_prepare_disjoint(1u);            /* tiny .COM */
    check_prepare_disjoint(0x100u);        /* 256 B */
    check_prepare_disjoint(0x4000u);       /* 16 KiB */
    check_prepare_disjoint(0x10000u);      /* 64 KiB (a SAMIR-scale text) */
    /* The largest image that still leaves a positive heap below the ceiling:
     * image_end + PROGRAM_BSS_RESERVE must be < PROGRAM_ARENA_CEIL, i.e.
     * image_len < (ceiling - PROGRAM_IMAGE - PROGRAM_BSS_RESERVE). 0x1E000 is just
     * under that bound, so the arena is present (and minimal). */
    check_prepare_disjoint(0x1E000u);      /* ~120 KiB -- still hostable (minimal heap) */

    /* === Part B: a too-large image yields NO arena (fail loud, no overlap). ====
     * An image whose end + BSS reserve has no room below the ceiling must set
     * arena_present == 0 -- the program runs heap-less, never with an overlapping
     * arena. PROGRAM_IMAGE_MAX is the largest image the loader accepts at all;
     * image_len just under it leaves < 1 paragraph below the ceiling. */
    {
        loader_plan_t plan;
        uint32_t big = PROGRAM_IMAGE_MAX;   /* image_end == ENV_BLOCK == ceiling */
        loader_status_t st = loader_prepare(g_image, big, (const char *)0, 0u, &plan);
        /* image_len up to PROGRAM_IMAGE_MAX is accepted by the size guard; the
         * arena just has no room (image_end >= ceiling). */
        CHECK(st == LOADER_OK, "max-size image still loads (just heap-less)");
        CHECK(plan.arena_present == 0u,
              "an image filling the window leaves NO arena (no overlap)");
        CHECK(plan.arena_base == 0u && plan.arena_ceil == 0u,
              "absent arena reports a zero window");
    }

    /* === Part C: the LIVE AH=48h dispatch over the disjoint window. ============
     * g_image is only 0x20000 bytes, so exercise live allocation for image sizes
     * within it. The disjointness assertions ride the REAL int21 dispatch. */
    check_live_alloc_disjoint(1u, arena_buf, buf_paras);
    check_live_alloc_disjoint(0x100u, arena_buf, buf_paras);
    check_live_alloc_disjoint(0x10000u, arena_buf, buf_paras);
    check_live_alloc_disjoint(0x1C000u, arena_buf, buf_paras);

    /* === Part D: the ACTUAL on-target program flow (bead hdlb / S8.2). =========
     * int21_mcb_bind_program leaves the disjoint arena FREE so the program's first
     * DIRECT AH=48h (no SETBLOCK) succeeds -- the flow SAMIR uses. */
    check_bind_program_direct_alloc(arena_buf, buf_paras);

    return TEST_SUMMARY("test_arena_disjoint");
}
