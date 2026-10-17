/*
 * harness/proptest/test_kernel_stack.c -- host oracle for the RATIFIED HIGH
 * KERNEL STACK (bead initech-xu97; WL-0163).
 *
 * WHAT IT PROVES
 *   1. THE MAP. The kernel stack window [KERNEL_STACK_BOT, KERNEL_STACK_TOP+4)
 *      is pairwise DISJOINT from every other window spec/memory_map.h hands
 *      out -- the FLAIR heap, the program window, the load staging, the stage2
 *      bounce, the transient COMMAND.COM -- lies inside the runway, and is
 *      64 KiB topped by an aligned dword. (The header carries the same facts as
 *      _Static_asserts; this file restates them as a TABLE of half-open
 *      intervals checked by one generic overlap test, so a future window added
 *      to the table is checked against the stack without anyone writing a new
 *      assert.)
 *   2. THE COPIES. NASM cannot include a C header, so os/milton/kstart.asm
 *      duplicates KERNEL_BASE/RUNWAY/CEIL/STACK_BOT/STACK_TOP as `equ` lines,
 *      and os/milton/kernel.ld duplicates the image limit (_kernel_end <=
 *      KERNEL_STACK_BOT) as a literal. Both files are PARSED here (paths on the
 *      command line) and every literal is compared with the header. A drifted
 *      copy -- the classic way a ratified map quietly stops being true -- is
 *      RED here before any emulator boots.
 *   3. THE PROGRAM WINDOW IS UNTOUCHED (the bead's acceptance: "PROGRAM_* and
 *      FLAIR_HEAP_* literals remain unchanged"): the literals are checked
 *      against HAND-WRITTEN values (the tdnl.29 floor golden), not against the
 *      header, so a header edit cannot agree with itself.
 *
 * MUTANTS (make test-kernel-stack-mutant; Rule 6):
 *   MEMMAP_MUT_STACK_IN_HEAP      the tdnl.29 design's rejected
 *                                 [0x4F0000,0x500000) home -> the header's
 *                                 compile-time guard must REFUSE to compile.
 *   MEMMAP_MUT_STACK_IN_TRANSIENT the stack slid into [0x580000,0x5C0000) ->
 *                                 likewise a compile error.
 *   KSTART_DRIFT                  the gate feeds this binary a kstart.asm copy
 *                                 whose STACK_TOP equate is the OLD 0x0009FFFC
 *                                 -> leg 2 must go RED.
 *
 * Ref: spec/memory_map.h (THE RATIFIED HIGH STACK; THE KERNEL STACK);
 *      docs/design/kernel-runway-relocation.md K1/K2 (the runway, and the
 *      rejected [0x4F0000,0x500000) stack); docs/worklog/WL-0087 item 10
 *      (tdnl.29, the Step-0.3 stop). CLAUDE.md Law 2, Rule 2, Rule 6, Rule 8,
 *      Rule 12 (ASCII).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory_map.h"
#include "test_assert.h"

TEST_HARNESS();

typedef struct win {
    const char *name;
    uint32_t    lo;   /* inclusive */
    uint32_t    hi;   /* exclusive */
} win_t;

static int overlap(const win_t *a, const win_t *b)
{
    return a->lo < b->hi && b->lo < a->hi;
}

/* Find `key` at the start of a line (after optional blanks) and parse the
 * first 0x literal after it. Returns 1 on success. */
static int parse_hex_after(const char *text, const char *key, uint32_t *out)
{
    const char *p = text;
    size_t kl = strlen(key);

    while ((p = strstr(p, key)) != NULL) {
        const char *ls = p;
        while (ls > text && (ls[-1] == ' ' || ls[-1] == '\t')) ls--;
        if (ls == text || ls[-1] == '\n') {
            const char *h = strstr(p + kl, "0x");
            const char *eol = strchr(p, '\n');
            if (h != NULL && (eol == NULL || h < eol)) {
                *out = (uint32_t)strtoul(h, NULL, 16);
                return 1;
            }
        }
        p += kl;
    }
    return 0;
}

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *b;
    if (f == NULL) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    n = ftell(f);
    rewind(f);
    b = (char *)malloc((size_t)n + 1u);
    if (b == NULL || fread(b, 1, (size_t)n, f) != (size_t)n) {
        fclose(f); free(b); return NULL;
    }
    b[n] = '\0';
    fclose(f);
    return b;
}

int main(int argc, char **argv)
{
    const win_t stack = { "kernel stack", KERNEL_STACK_BOT, KERNEL_STACK_TOP + 4u };
    const win_t others[] = {
        { "FLAIR heap",        FLAIR_HEAP_BASE, FLAIR_HEAP_BASE + FLAIR_HEAP_SIZE },
        { "program window",    PROGRAM_BASE,    PROGRAM_ALLOC_END },
        { "load staging",      LOAD_STAGING_BASE, LOAD_STAGING_BASE + LOAD_STAGING_MAX },
        { "stage2 bounce",     KERNEL_BOUNCE_BASE, KERNEL_BOUNCE_BASE + KERNEL_BOUNCE_CAP },
        { "transient COMMAND", TRANSIENT_BASE,  TRANSIENT_CEIL },
        /* The image's own window: KERNEL_BASE up to the transient (the build
         * fails loud if any kernel's _kernel_end reaches TRANSIENT_BASE). */
        { "kernel image",      KERNEL_BASE,     TRANSIENT_BASE },
        /* The conventional home the stack LEFT: nothing may silently put it
         * back there (the KSTART_DRIFT mutant's whole point). */
        { "old conventional stack", 0x00090000u, 0x000A0000u },
    };
    char *ks, *ld;
    uint32_t v = 0u;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <kstart.asm> <kernel.ld>\n", argv[0]);
        return 2;
    }

    /* ---- leg 1: the map ---------------------------------------------------- */
    CHECK(stack.hi - stack.lo == 0x00010000u, "L1 the stack is 64 KiB");
    CHECK((KERNEL_STACK_TOP & 3u) == 0u, "L1 KERNEL_STACK_TOP is dword aligned");
    CHECK(stack.lo >= KERNEL_BASE && stack.hi <= KERNEL_CEIL,
          "L1 the stack lies inside the kernel runway [KERNEL_BASE, KERNEL_CEIL)");
    CHECK(KERNEL_CEIL - 0x00100000u <= (uint32_t)INITECH_MIN_EXT_KB * 1024u,
          "L1 the RAM gate (INITECH_MIN_EXT_KB) backs every stack byte");
    for (size_t i = 0; i < sizeof others / sizeof others[0]; i++) {
        char msg[96];
        snprintf(msg, sizeof msg, "L1 the stack is disjoint from the %s",
                 others[i].name);
        CHECK(!overlap(&stack, &others[i]), msg);
    }

    /* ---- leg 3: the untouched literals (hand-written golden) -------------- */
    CHECK(PROGRAM_BASE == 0x00040000u && PROGRAM_IMAGE == 0x00040100u &&
          ENV_BLOCK == 0x0006F000u && PROGRAM_STACK_TOP == 0x0007FFFCu &&
          PROGRAM_STACK_BOT == 0x00070000u && PROGRAM_ALLOC_END == 0x00080000u,
          "L3 every PROGRAM_* literal is the tdnl.29 floor golden");
    CHECK(FLAIR_HEAP_BASE == 0x00100000u && FLAIR_HEAP_SIZE == 0x00400000u &&
          FLAIR_HEAP_MIN == 0x00400000u,
          "L3 every FLAIR_HEAP_* literal is unchanged");
    CHECK(KERNEL_STACK_BOT == 0x005F0000u && KERNEL_STACK_TOP == 0x005FFFFCu,
          "L3 the ratified stack is [0x5F0000, 0x5FFFFC] (bead initech-xu97)");

    /* ---- leg 2: the copies ------------------------------------------------- */
    ks = slurp(argv[1]);
    ld = slurp(argv[2]);
    CHECK(ks != NULL, "L2 kstart.asm is readable");
    CHECK(ld != NULL, "L2 kernel.ld is readable");
    if (ks != NULL) {
        static const struct { const char *key; uint32_t want; } eq[] = {
            { "KERNEL_BASE",      KERNEL_BASE },
            { "KERNEL_RUNWAY",    KERNEL_RUNWAY },
            { "KERNEL_CEIL",      KERNEL_CEIL },
            { "KERNEL_STACK_BOT", KERNEL_STACK_BOT },
            { "KERNEL_STACK_TOP", KERNEL_STACK_TOP },
        };
        for (size_t i = 0; i < sizeof eq / sizeof eq[0]; i++) {
            char key[48], msg[128];
            snprintf(key, sizeof key, "%s ", eq[i].key);
            v = 0u;
            snprintf(msg, sizeof msg, "L2 kstart.asm %s equ == spec/memory_map.h",
                     eq[i].key);
            CHECK(parse_hex_after(ks, key, &v) && v == eq[i].want, msg);
            if (v != eq[i].want)
                fprintf(stderr, "        kstart.asm %s = 0x%08X, header 0x%08X\n",
                        eq[i].key, (unsigned)v, (unsigned)eq[i].want);
        }
        CHECK(strstr(ks, "mov esp, KERNEL_STACK_TOP") != NULL,
              "L2 kstart.asm loads ESP from its KERNEL_STACK_TOP equate");
    }
    if (ld != NULL) {
        const char *a = strstr(ld, "ASSERT(_kernel_end <= ");
        v = 0u;
        if (a != NULL) v = (uint32_t)strtoul(a + strlen("ASSERT(_kernel_end <= "), NULL, 16);
        CHECK(a != NULL && v == KERNEL_STACK_BOT,
              "L2 kernel.ld bounds _kernel_end by KERNEL_STACK_BOT");
    }
    free(ks);
    free(ld);
    return TEST_SUMMARY("test_kernel_stack");
}
