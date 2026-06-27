/*
 * test_scrap.c -- the FLAIR Scrap Manager host oracle (initech-b2vk).
 *
 * beads: initech-b2vk (FLAIR Phase 4.5 -- Platform Services: Scrap).
 * Ref:   scrap.h / scrap.c (the API under test); ADR-0013 Sec 6 (shell-owned
 *        cross-tenant Scrap); Inside Macintosh Vol I Scrap Manager.
 *        CLAUDE.md Law 2 (oracle is the truth -- NOT by-construction), Rule 1
 *        (Red->Green->Refactor), Rule 6 (mutation-proven), Rule 12 (ASCII).
 *
 * ANTI-BY-CONSTRUCTION (Law 2, the central discipline):
 *   The expected bytes are LITERAL CONSTANTS IN THIS FILE -- they are NOT
 *   read back out of the FlairScrap struct internals. Putting "hello" into
 *   the scrap and then reading scrap->flavors[0].data[0..4] to form the
 *   expectation would agree BY CONSTRUCTION and cannot catch a wrong value
 *   (the FLAIR palette-grading heresy; ADR-0010 / Revocation Record HER-02).
 *   Instead, the expected bytes are spelled out here:
 *     "hello" == { 0x68, 0x65, 0x6C, 0x6C, 0x6F }
 *     PICT    == { 0xDE, 0xAD, 0xBE, 0xEF }
 *     "world" == { 0x77, 0x6F, 0x72, 0x6C, 0x64 }
 *     "xyzzy" == { 0x78, 0x79, 0x7A, 0x7A, 0x79 }
 *   These are the INDEPENDENT golden; the implementation is proved against them.
 *
 * MUTANT COVERAGE (Rule 6 -- each mutant must COMPILE and go RED):
 *   SCRAP_MUT_IGNORE_FLAVOR  -- flavor-match ignores type key; PutScrap(PICT)
 *       overwrites the TEXT slot; GetScrap(TEXT) returns PICT bytes -> RED
 *       at step 3 (multi-flavor additive proof).
 *   SCRAP_MUT_NO_INCREMENT   -- ZeroScrap skips scrapCount++; InfoScrap
 *       returns 1 after two ZeroScraps -> RED at step 6.
 *   SCRAP_MUT_NO_CLEAR       -- ZeroScrap leaves n_flavors unchanged; GetScrap
 *       after ZeroScrap returns stale data -> RED at step 6.
 *
 * CROSS-TENANT LEG (step 7 -- the co-residency payoff, ADR-0013 Sec 3.5/Sec 6):
 *   Two stub FlairApps are registered into a FlairProcessList via
 *   FlairProcess_register (the caller-storage registrar, process.h). App A
 *   writes "xyzzy" through the SHARED scrap pointer; App B reads it back and
 *   checks the literal bytes {0x78,0x79,0x7A,0x7A,0x79}. One shell-owned
 *   scrap, two tenants: copy in A, paste in B.
 *
 * Compile (host, green):
 *   gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *       -Ispec -Ios/flair -Ios/flair/atkinson -Iseed \
 *       -o /tmp/laneB/test_scrap \
 *       harness/proptest/test_scrap.c \
 *       os/flair/scrap.c os/flair/process.c os/flair/window.c \
 *       os/flair/heap.c os/flair/atkinson/region.c
 *
 * Compile (kernel type-check):
 *   gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra -Werror \
 *       -Ispec -Ios/milton -Ios/flair -c os/flair/scrap.c
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ostype.h"       /* flair_ostype_t (-Ios/flair)                      */
#include "scrap.h"        /* FlairScrap, the API under test (-Ios/flair)       */
#include "process.h"      /* FlairApp, FlairProcessList, FlairProcess_register */
#include "test_assert.h"  /* TEST_HARNESS / CHECK / TEST_SUMMARY (-Iseed)      */

TEST_HARNESS();

/* ===========================================================================
 * Cross-tenant stub: a minimal no-op event entry-point (REQUIRED by the
 * FlairAppProcs contract; ADR-0013 Sec 3.1). The stub apps do not open
 * windows -- they exist only to prove the shared scrap pointer is reachable
 * from any tenant in the process list.
 * ===========================================================================*/
static void ct_event(FlairApp *self, const EventRecord *ev)
{
    (void)self; (void)ev;
}
static const FlairAppProcs ct_procs = {
    NULL,      /* open  -- not used in the _register path (no arena, no open) */
    ct_event,  /* event -- REQUIRED                                           */
    NULL,      /* idle  */
    NULL       /* close */
};

/* ===========================================================================
 * MAIN -- the scrap oracle.
 *
 * Steps follow the hand-authored op/expected sequence. Expected bytes are
 * LITERAL CONSTANTS (independent of the implementation -- see header note).
 * ===========================================================================*/
int main(void)
{
    /* Static storage: FlairScrap contains inline 8192-byte payload arrays;
     * four flavors = ~32 KB total. Static avoids a stack overflow. */
    static FlairScrap s;
    uint8_t buf[FLAIR_SCRAP_MAX_PAYLOAD];
    uint32_t n;
    int32_t  r;

    /* -----------------------------------------------------------------------
     * STEP 1: init + ZeroScrap -> scrapCount==1; TEXT absent.
     * ----------------------------------------------------------------------- */
    FlairScrap_init(&s);
    FlairZeroScrap(&s);

    CHECK(FlairInfoScrap(&s) == 1,
          "step-1: FlairInfoScrap == 1 after init + one ZeroScrap");

    r = FlairGetScrap(&s, FLAIR_SCRAP_TYPE_TEXT, buf, sizeof(buf), &n);
    CHECK(r == -1,
          "step-1: GetScrap(TEXT) == -1 (absent; no PUT yet)");

    /* -----------------------------------------------------------------------
     * STEP 2: PutScrap(TEXT, "hello", 5) -> 0;
     *         GetScrap(TEXT) -> 5, buf == {0x68,0x65,0x6C,0x6C,0x6F}.
     *
     * Independent expected bytes (Law 2):
     *   'h' = 0x68, 'e' = 0x65, 'l' = 0x6C, 'l' = 0x6C, 'o' = 0x6F
     * ----------------------------------------------------------------------- */
    {
        static const uint8_t hello[5] = { 0x68, 0x65, 0x6C, 0x6C, 0x6F };

        r = FlairPutScrap(&s, FLAIR_SCRAP_TYPE_TEXT, hello, 5u);
        CHECK(r == 0,
              "step-2: PutScrap(TEXT, hello, 5) == 0 (success)");

        memset(buf, 0, sizeof(buf));
        n = 0;
        r = FlairGetScrap(&s, FLAIR_SCRAP_TYPE_TEXT, buf, sizeof(buf), &n);

        CHECK(r == 5,
              "step-2: GetScrap(TEXT) returns 5 (byte count)");
        CHECK(n == 5u,
              "step-2: *len_out == 5");

        /* Literal independent golden -- NOT read from struct internals. */
        CHECK(buf[0] == 0x68u,
              "step-2: buf[0] == 0x68 ('h')");
        CHECK(buf[1] == 0x65u,
              "step-2: buf[1] == 0x65 ('e')");
        CHECK(buf[2] == 0x6Cu,
              "step-2: buf[2] == 0x6C ('l')");
        CHECK(buf[3] == 0x6Cu,
              "step-2: buf[3] == 0x6C ('l')");
        CHECK(buf[4] == 0x6Fu,
              "step-2: buf[4] == 0x6F ('o')");
    }

    /* -----------------------------------------------------------------------
     * STEP 3: PutScrap(PICT, {0xDE,0xAD,0xBE,0xEF}, 4) -- ADDITIVE (not
     *         clobbering TEXT); GetScrap(TEXT) STILL returns "hello";
     *         GetScrap(PICT) returns {0xDE,0xAD,0xBE,0xEF}.
     *
     * This is the multi-flavor coexistence proof. SCRAP_MUT_IGNORE_FLAVOR
     * makes PutScrap(PICT) overwrite the TEXT slot, so GetScrap(TEXT) then
     * returns PICT bytes -> RED here.
     * ----------------------------------------------------------------------- */
    {
        static const uint8_t pict_bytes[4] = { 0xDE, 0xAD, 0xBE, 0xEF };

        r = FlairPutScrap(&s, FLAIR_SCRAP_TYPE_PICT, pict_bytes, 4u);
        CHECK(r == 0,
              "step-3: PutScrap(PICT, {DE,AD,BE,EF}, 4) == 0 (appended)");

        /* TEXT must be untouched (additive, NOT clobbered). */
        memset(buf, 0, sizeof(buf));
        n = 0;
        r = FlairGetScrap(&s, FLAIR_SCRAP_TYPE_TEXT, buf, sizeof(buf), &n);
        CHECK(r == 5,
              "step-3: GetScrap(TEXT) STILL returns 5 (additive; not clobbered by PICT put)");
        CHECK(n == 5u,
              "step-3: *len_out == 5 (TEXT unchanged)");
        CHECK(buf[0] == 0x68u,
              "step-3: buf[0] == 0x68 ('h') -- TEXT payload intact");
        CHECK(buf[4] == 0x6Fu,
              "step-3: buf[4] == 0x6F ('o') -- TEXT payload intact");

        /* PICT must be the independent literal bytes. */
        memset(buf, 0, sizeof(buf));
        n = 0;
        r = FlairGetScrap(&s, FLAIR_SCRAP_TYPE_PICT, buf, sizeof(buf), &n);
        CHECK(r == 4,
              "step-3: GetScrap(PICT) returns 4");
        CHECK(n == 4u,
              "step-3: *len_out == 4 (PICT)");
        CHECK(buf[0] == 0xDEu,
              "step-3: buf[0] == 0xDE (PICT byte 0)");
        CHECK(buf[1] == 0xADu,
              "step-3: buf[1] == 0xAD (PICT byte 1)");
        CHECK(buf[2] == 0xBEu,
              "step-3: buf[2] == 0xBE (PICT byte 2)");
        CHECK(buf[3] == 0xEFu,
              "step-3: buf[3] == 0xEF (PICT byte 3)");

        /* n_flavors must be 2: one TEXT, one PICT. */
        CHECK(s.n_flavors == 2u,
              "step-3: n_flavors == 2 (TEXT + PICT coexist; no duplicate)");
    }

    /* -----------------------------------------------------------------------
     * STEP 4: PutScrap(TEXT, "world", 5) -- REPLACE in place.
     *         GetScrap(TEXT) -> buf[0]=='w'(0x77), buf[4]=='d'(0x64).
     *         n_flavors must still be 2 (no new slot; replace, not append).
     *
     * Independent expected bytes (Law 2):
     *   'w' = 0x77, 'o' = 0x6F, 'r' = 0x72, 'l' = 0x6C, 'd' = 0x64
     * ----------------------------------------------------------------------- */
    {
        static const uint8_t world[5] = { 0x77, 0x6F, 0x72, 0x6C, 0x64 };

        r = FlairPutScrap(&s, FLAIR_SCRAP_TYPE_TEXT, world, 5u);
        CHECK(r == 0,
              "step-4: PutScrap(TEXT, world, 5) == 0 (replace in place)");

        memset(buf, 0, sizeof(buf));
        n = 0;
        r = FlairGetScrap(&s, FLAIR_SCRAP_TYPE_TEXT, buf, sizeof(buf), &n);
        CHECK(r == 5,
              "step-4: GetScrap(TEXT) returns 5");
        CHECK(n == 5u,
              "step-4: *len_out == 5");
        CHECK(buf[0] == 0x77u,
              "step-4: buf[0] == 0x77 ('w') -- TEXT replaced by 'world'");
        CHECK(buf[4] == 0x64u,
              "step-4: buf[4] == 0x64 ('d') -- TEXT replaced by 'world'");

        /* No new slot: replace must not increment n_flavors. */
        CHECK(s.n_flavors == 2u,
              "step-4: n_flavors == 2 (replace does not create a duplicate TEXT slot)");
    }

    /* -----------------------------------------------------------------------
     * STEP 5: Overflow: PutScrap(TEXT, big, 9000) -> FLAIR_SCRAP_ERR_TOO_LARGE.
     *         The prior TEXT value ("world") MUST be intact after the rejected
     *         put (a rejected put must NOT corrupt an existing flavor).
     * ----------------------------------------------------------------------- */
    {
        static uint8_t big[9000];   /* > FLAIR_SCRAP_MAX_PAYLOAD (8192) */
        memset(big, 0xAA, sizeof(big));

        r = FlairPutScrap(&s, FLAIR_SCRAP_TYPE_TEXT, big, 9000u);
        CHECK(r == FLAIR_SCRAP_ERR_TOO_LARGE,
              "step-5: PutScrap with 9000-byte payload -> FLAIR_SCRAP_ERR_TOO_LARGE (-2)");

        /* TEXT must be untouched ("world" still present). */
        memset(buf, 0, sizeof(buf));
        n = 0;
        r = FlairGetScrap(&s, FLAIR_SCRAP_TYPE_TEXT, buf, sizeof(buf), &n);
        CHECK(r == 5,
              "step-5: after rejected put, GetScrap(TEXT) still 5 (prior value intact)");
        CHECK(buf[0] == 0x77u,
              "step-5: buf[0] == 0x77 ('w') -- TEXT payload not corrupted by rejected put");
        CHECK(buf[4] == 0x64u,
              "step-5: buf[4] == 0x64 ('d') -- TEXT payload not corrupted by rejected put");
    }

    /* -----------------------------------------------------------------------
     * STEP 5.5 (bufsz-too-small leg): GetScrap with a 2-byte buffer for a
     * 5-byte TEXT flavor -> FLAIR_SCRAP_ERR_BUF; NO partial copy.
     * ----------------------------------------------------------------------- */
    {
        uint8_t small_buf[2];
        small_buf[0] = 0xCCu;   /* sentinel: must be unchanged if no copy */
        small_buf[1] = 0xCCu;

        r = FlairGetScrap(&s, FLAIR_SCRAP_TYPE_TEXT, small_buf, 2u, NULL);
        CHECK(r == FLAIR_SCRAP_ERR_BUF,
              "step-5.5: GetScrap(TEXT) with bufsz=2 < length=5 -> FLAIR_SCRAP_ERR_BUF (-4)");
        /* Rule 2: no partial copy -- the sentinel bytes are untouched. */
        CHECK(small_buf[0] == 0xCCu && small_buf[1] == 0xCCu,
              "step-5.5: no partial copy (sentinel bytes 0xCC unchanged)");
    }

    /* -----------------------------------------------------------------------
     * STEP 6: ZeroScrap -> scrapCount==2; GetScrap(TEXT)==-1 (cleared).
     *
     * SCRAP_MUT_NO_INCREMENT makes InfoScrap return 1 (not 2) -> RED.
     * SCRAP_MUT_NO_CLEAR makes GetScrap return stale "world" (not -1) -> RED.
     * ----------------------------------------------------------------------- */
    FlairZeroScrap(&s);

    CHECK(FlairInfoScrap(&s) == 2,
          "step-6: FlairInfoScrap == 2 after two ZeroScraps (scrapCount monotone)");

    memset(buf, 0, sizeof(buf));
    n = 999u;
    r = FlairGetScrap(&s, FLAIR_SCRAP_TYPE_TEXT, buf, sizeof(buf), &n);
    CHECK(r == -1,
          "step-6: GetScrap(TEXT) == -1 (ZeroScrap cleared all flavors)");

    r = FlairGetScrap(&s, FLAIR_SCRAP_TYPE_PICT, buf, sizeof(buf), &n);
    CHECK(r == -1,
          "step-6: GetScrap(PICT) == -1 (ZeroScrap cleared all flavors)");

    CHECK(s.n_flavors == 0u,
          "step-6: n_flavors == 0 after ZeroScrap");

    /* -----------------------------------------------------------------------
     * STEP 7: CROSS-TENANT (the co-residency payoff; ADR-0013 Sec 3.5 / Sec 6).
     *
     * Two stub FlairApps (A and B) are registered into a FlairProcessList via
     * FlairProcess_register (the caller-storage registrar; no arena, no open).
     * They share ONE FlairScrap * -- the shell-owned singleton. App A writes
     * TEXT "xyzzy"; App B reads it back and checks the literal bytes.
     *
     * Independent expected bytes (Law 2):
     *   'x' = 0x78, 'y' = 0x79, 'z' = 0x7A, 'z' = 0x7A, 'y' = 0x79
     * ----------------------------------------------------------------------- */
    {
        static FlairScrap shared;       /* the shell-owned scrap singleton */
        static FlairApp   app_a;        /* tenant A (static; ~232 B) */
        static FlairApp   app_b;        /* tenant B (static) */
        FlairProcessList  plist;
        uint8_t           ct_buf[64];
        uint32_t          ct_n = 0;
        int32_t           ct_r;
        static const uint8_t xyzzy[5] = { 0x78, 0x79, 0x7A, 0x7A, 0x79 };

        /* The shell initializes the singleton (survives tenant launch/death). */
        FlairScrap_init(&shared);
        FlairZeroScrap(&shared);        /* shared.scrapCount = 1 */

        /* Zero-init both FlairApp records (all pointer / arena fields NULL). */
        memset(&app_a, 0, sizeof(app_a));
        memset(&app_b, 0, sizeof(app_b));
        app_a.procs   = &ct_procs;
        app_a.name    = "APP-A";
        app_a.windows = NULL;           /* no window; _register handles NULL */
        app_b.procs   = &ct_procs;
        app_b.name    = "APP-B";
        app_b.windows = NULL;

        /* Register both tenants (thin caller-storage registrar; no arena). */
        FlairProcessList_init(&plist);
        FlairProcess_register(&plist, &app_a);
        FlairProcess_register(&plist, &app_b);
        /* plist.head == &app_b (B registered last -> foreground) */
        CHECK(plist.head == &app_b,
              "step-7: after registering A then B, B is the foreground head");

        /* App A: copy "xyzzy" to the shared scrap. */
        ct_r = FlairPutScrap(&shared, FLAIR_SCRAP_TYPE_TEXT, xyzzy, 5u);
        CHECK(ct_r == 0,
              "step-7: App A FlairPutScrap(TEXT, xyzzy, 5) == 0");

        /* App B: paste from the SAME shared scrap pointer. */
        memset(ct_buf, 0, sizeof(ct_buf));
        ct_n = 0;
        ct_r = FlairGetScrap(&shared, FLAIR_SCRAP_TYPE_TEXT,
                             ct_buf, sizeof(ct_buf), &ct_n);
        CHECK(ct_r == 5,
              "step-7: App B FlairGetScrap(TEXT) returns 5 (xyzzy)");
        CHECK(ct_n == 5u,
              "step-7: *len_out == 5");

        /* Literal independent golden -- 'x','y','z','z','y' */
        CHECK(ct_buf[0] == 0x78u,
              "step-7: ct_buf[0] == 0x78 ('x')");
        CHECK(ct_buf[1] == 0x79u,
              "step-7: ct_buf[1] == 0x79 ('y')");
        CHECK(ct_buf[2] == 0x7Au,
              "step-7: ct_buf[2] == 0x7A ('z')");
        CHECK(ct_buf[3] == 0x7Au,
              "step-7: ct_buf[3] == 0x7A ('z')");
        CHECK(ct_buf[4] == 0x79u,
              "step-7: ct_buf[4] == 0x79 ('y')");
    }

    return TEST_SUMMARY("test-scrap");
}
