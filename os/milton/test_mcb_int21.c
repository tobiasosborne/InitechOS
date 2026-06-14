/* test_mcb_int21.c -- host dispatch oracle for the INT 21h memory functions
 * AH=48h ALLOC / AH=49h FREE / AH=4Ah SETBLOCK (beads initech-509.6, part 2).
 *
 * Part 1 (test_mcb.c) proved the PURE allocator (mcb.c). THIS oracle proves the
 * SYSCALL SEAM in int21.c: that int21_dispatch routes 48h/49h/4Ah into a bound
 * mcb_arena_t, converts a data-paragraph index to a DOS segment as
 * (arena_base_linear >> 4) + data_para, enforces owner == the current PSP, and
 * returns the DOS register/CF contract (Law 1: DOS 3.3 PRM AH=48h/49h/4Ah; the
 * flat-mode register adaptation documented in int21.c's memory-arena block).
 *
 * Compiles HOSTED against the REAL artifact int21.c + mcb.c (the SAME TUs the
 * kernel runs). The arena buffer + the current PSP both live in the low 4 GiB
 * (MAP_32BIT) so (uintptr_t)>>4 segment math round-trips exactly as on the
 * 32-bit flat kernel.
 *
 * MUTATION (Rule 6), driven by make test-mcb-int21-mutant:
 *   -DINT21_MUTATE_ALLOC_NO_SEGBASE : do_alloc returns the bare data-paragraph
 *     index instead of (arena_seg_base()+data_para), so the segment a 48h ALLOC
 *     reports no longer maps back through 49h FREE -- the exact-segment +
 *     round-trip assertions go RED.
 *
 * CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6, Rule 11
 * (deterministic), Rule 12 (ASCII).
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#include "int21.h"
#include "mcb.h"
#include "psp.h"
#include "test_assert.h"

TEST_HARNESS();

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

/* The current PSP lives in low memory; its DOS-segment owner id is (ptr>>4) --
 * the SAME value AH=62h GET PSP returns and the value do_alloc stamps as owner. */
static psp_t *g_psp;
static uint16_t g_psp_seg;

/* The arena buffer + its flat base linear address (what the kernel passes as
 * PROGRAM_BASE). The segment of data paragraph p is (base_linear>>4)+p. */
static uint8_t *g_arena_buf;
static uint32_t g_arena_base_linear;
static uint16_t g_arena_seg_base;     /* base_linear >> 4 */
#define ARENA_PARAS 4096u             /* 64 KiB arena: header + data paragraphs */

static void bind_psp(uint32_t alloc_end_linear)
{
    psp_params_t params;
    params.alloc_end_linear  = alloc_end_linear;
    params.env_linear        = 0u;
    params.parent_psp_linear = 0u;
    params.cmd_tail          = (const char *)0;
    params.cmd_tail_len      = 0u;
    (void)psp_build(g_psp, &params);
    int21_set_psp((struct psp *)g_psp);
    g_psp_seg = (uint16_t)(((uintptr_t)g_psp >> 4) & 0xFFFFu);
}

/* Re-bind a fresh arena owned by the current PSP (the authentic single-big-block
 * a freshly loaded program owns). Mirrors what the loader does per program. */
static void reset_arena(void)
{
    int21_set_mcb_arena(g_arena_buf, ARENA_PARAS, g_arena_base_linear);
    (void)int21_mcb_reset();   /* hand the whole window to the current PSP */
}

/* Drive one INT 21h call with a pre-zeroed frame, AH preset, and CF preloaded
 * to 1 so a handler that forgets to write CF is caught. */
static int_frame_t call(uint8_t ah, uint32_t ebx, uint32_t ecx)
{
    int_frame_t f = fresh_frame();
    f.eax = (uint32_t)ah << 8;
    f.ebx = ebx;
    f.ecx = ecx;
    f.eflags |= CF_BIT;        /* preload CF=1 -> prove success CLEARS it */
    int21_dispatch(&f);
    return f;
}

/* Walk the live arena chain (the test knows the same layout mcb.c uses) and
 * confirm the data paragraph for DOS segment `seg` is owned by `owner`. */
static int seg_owned_by(uint16_t seg, uint16_t owner)
{
    uint32_t want_data_para = (uint32_t)seg - g_arena_seg_base;
    uint32_t para = 0u;
    for (;;) {
        mcb_t *h = (mcb_t *)(g_arena_buf + (size_t)para * 16u);
        uint32_t data_para = para + 1u;
        if (data_para == want_data_para) {
            return h->owner == owner;
        }
        if (h->signature == MCB_SIG_TERMINAL) {
            return 0;
        }
        para += 1u + h->size_paras;
    }
}

int main(void)
{
    g_psp = (psp_t *)alloc_low(sizeof(psp_t));
    g_arena_buf = (uint8_t *)alloc_low((size_t)ARENA_PARAS * 16u);
    CHECK(g_psp != NULL, "PSP buffer in low 4 GiB");
    CHECK(g_arena_buf != NULL, "arena buffer in low 4 GiB");
    if (g_psp == NULL || g_arena_buf == NULL) {
        return TEST_SUMMARY("test_mcb_int21");
    }
    g_arena_base_linear = (uint32_t)(uintptr_t)g_arena_buf;
    g_arena_seg_base    = (uint16_t)((g_arena_base_linear >> 4) & 0xFFFFu);

    bind_psp(0x00070000u);

    /* === 1. ALLOC with NO arena bound -> insufficient memory, CF=1. ======== */
    {
        int21_set_mcb_arena(NULL, 0u, 0u);   /* unbound */
        int_frame_t f = call(0x48u, 16u, 0u);
        CHECK(frame_cf(&f) == 1, "48h with no arena -> CF=1");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == 0x0008u,
              "48h unbound -> AX=0008h insufficient memory");
        CHECK((uint16_t)(f.ebx & 0xFFFFu) == 0u, "48h unbound -> BX=largest=0");
    }

    /* From here on the arena is bound + reset to one PSP-owned block. The whole
     * 64 KiB window starts owned by the program (single big block), so a bare
     * ALLOC fails: the program must SHRINK first (authentic DOS). */
    reset_arena();

    /* === 2. The program owns its whole window -> a direct 48h fails. ======= */
    {
        int_frame_t f = call(0x48u, 16u, 0u);
        CHECK(frame_cf(&f) == 1, "48h before shrinking the program block -> CF=1");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == 0x0008u,
              "48h with no free tail -> AX=0008h");
        /* largest free == 0 (the lone block is owned, not free). */
        CHECK((uint16_t)(f.ebx & 0xFFFFu) == 0u, "48h no-free -> BX=0 largest");
    }

    /* === 3. Authentic shrink-then-alloc: 4Ah the program block down, then 48h
     *        carves from the freed tail. The program's block is data paragraph 1
     *        (header at paragraph 0); its segment = seg_base + 1. ============ */
    uint16_t prog_seg = (uint16_t)(g_arena_seg_base + 1u);
    {
        /* Shrink the program block to 16 data paragraphs (CX=new size). */
        int_frame_t f = call(0x4Au, (uint32_t)prog_seg, 16u);
        CHECK(frame_cf(&f) == 0, "4Ah shrink the program block -> CF=0");
    }

    /* === 4. 48h ALLOC 100 paragraphs -> AX = a valid DOS segment, CF=0,
     *        owned by the current PSP. ===================================== */
    uint16_t got_seg;
    {
        int_frame_t f = call(0x48u, 100u, 0u);
        CHECK(frame_cf(&f) == 0, "48h after shrink -> CF=0 success");
        got_seg = (uint16_t)(f.eax & 0xFFFFu);
        /* The block lands in the freed tail: above the program block (data para
         * 1, 16 paras) + its split remainder header. Segment must be > prog_seg
         * and inside the arena window. */
        CHECK(got_seg > prog_seg, "48h segment is above the program block");
        CHECK((uint32_t)got_seg < g_arena_seg_base + ARENA_PARAS,
              "48h segment lies inside the arena window");
        CHECK(seg_owned_by(got_seg, g_psp_seg),
              "48h block is owned by the current PSP");
    }

    /* === 5. A SECOND owner cannot 49h FREE the first owner's block. ======== */
    {
        /* Re-bind a DIFFERENT PSP (different low buffer -> different segment). */
        psp_t *other = (psp_t *)alloc_low(sizeof(psp_t));
        CHECK(other != NULL, "second PSP buffer in low 4 GiB");
        if (other != NULL) {
            psp_t *saved = g_psp;
            uint16_t saved_seg = g_psp_seg;
            g_psp = other;
            bind_psp(0x00070000u);   /* binds `other` as current; new owner seg */
            CHECK(g_psp_seg != saved_seg, "the second PSP has a distinct owner");
            int_frame_t f = call(0x49u, (uint32_t)got_seg, 0u);
            CHECK(frame_cf(&f) == 1, "49h cross-owner free -> CF=1");
            CHECK((uint16_t)(f.eax & 0xFFFFu) == 0x0009u,
                  "49h cross-owner -> AX=0009h invalid block");
            CHECK(seg_owned_by(got_seg, saved_seg),
                  "the block is STILL owned by the original PSP");
            /* Restore the original owner as current. */
            g_psp = saved;
            g_psp_seg = saved_seg;
            int21_set_psp((struct psp *)g_psp);
        }
    }

    /* === 6. The owner 49h FREE succeeds; CF=0, block becomes free. ========= */
    {
        int_frame_t f = call(0x49u, (uint32_t)got_seg, 0u);
        CHECK(frame_cf(&f) == 0, "49h by the owner -> CF=0");
        CHECK(seg_owned_by(got_seg, MCB_OWNER_FREE),
              "freed block's owner is now 0 (free)");
    }

    /* === 7. 49h FREE of a segment BELOW the arena base -> invalid block. === */
    {
        int_frame_t f = call(0x49u, (uint32_t)(g_arena_seg_base - 1u), 0u);
        CHECK(frame_cf(&f) == 1, "49h below the arena -> CF=1");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == 0x0009u,
              "49h below the arena -> AX=0009h");
    }

    /* === 8. 4Ah GROW that cannot be satisfied -> CF=1, AX=0008h, BX=max. ===
     *        Reset, shrink the program block to 1 paragraph so almost all the
     *        arena is one free tail; alloc a small block; then ask SETBLOCK to
     *        grow it past the whole arena. It must fail with the largest it
     *        could reach in BX. ============================================= */
    reset_arena();
    {
        int_frame_t s = call(0x4Au, (uint32_t)prog_seg, 1u);
        CHECK(frame_cf(&s) == 0, "4Ah shrink to 1 para -> CF=0");

        int_frame_t a = call(0x48u, 8u, 0u);
        CHECK(frame_cf(&a) == 0, "48h small block after shrink -> CF=0");
        uint16_t small = (uint16_t)(a.eax & 0xFFFFu);

        /* Ask for an absurd new size (bigger than the whole arena). */
        int_frame_t g = call(0x4Au, (uint32_t)small, ARENA_PARAS * 2u);
        CHECK(frame_cf(&g) == 1, "4Ah grow-too-big -> CF=1");
        CHECK((uint16_t)(g.eax & 0xFFFFu) == 0x0008u,
              "4Ah grow-too-big -> AX=0008h insufficient");
        uint16_t max = (uint16_t)(g.ebx & 0xFFFFu);
        CHECK(max >= 8u, "4Ah failure reports BX = a reachable max >= current");
        /* The reported max must actually be allocatable: shrink-grow to it. */
        int_frame_t g2 = call(0x4Au, (uint32_t)small, (uint32_t)max);
        CHECK(frame_cf(&g2) == 0, "4Ah grow to the reported BX max -> CF=0");
    }

    /* === 9. Round-trip: alloc, free, re-alloc the SAME size returns the SAME
     *        segment (coalesce restored the tail). Proves the seam threads the
     *        segment<->data_para conversion consistently end to end. ======== */
    reset_arena();
    {
        int_frame_t s = call(0x4Au, (uint32_t)prog_seg, 16u);
        CHECK(frame_cf(&s) == 0, "4Ah shrink for round-trip -> CF=0");

        int_frame_t a1 = call(0x48u, 32u, 0u);
        CHECK(frame_cf(&a1) == 0, "round-trip alloc #1 -> CF=0");
        uint16_t seg1 = (uint16_t)(a1.eax & 0xFFFFu);

        int_frame_t fr = call(0x49u, (uint32_t)seg1, 0u);
        CHECK(frame_cf(&fr) == 0, "round-trip free -> CF=0");

        int_frame_t a2 = call(0x48u, 32u, 0u);
        CHECK(frame_cf(&a2) == 0, "round-trip alloc #2 -> CF=0");
        uint16_t seg2 = (uint16_t)(a2.eax & 0xFFFFu);
        CHECK(seg2 == seg1,
              "alloc/free/alloc of the same size returns the same segment");
    }

    return TEST_SUMMARY("test_mcb_int21");
}
