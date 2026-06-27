/*
 * test_resource.c -- the FLAIR Resource Manager host oracle (initech-0w45).
 *
 * THE ANTI-BY-CONSTRUCTION DESIGN (Law 2 / HER-02):
 *   GOLDEN INPUT  -- a hand-authored mini Mac resource fork, FORK[], whose bytes
 *                    are written DIRECTLY from the offset tables in
 *                    ../system7-decomp/specs/resources/resource-manager.md
 *                    Sec 1.1-1.6 and wind-menu-dlog-ditl.md Sec 1a (byte-by-byte
 *                    commented below).
 *   GENUINE PARSER -- os/flair/resource.c walks that fork (positional BE reads,
 *                    bounds-checked) to extract resources by (type, id).
 *   GOLDEN OUTPUT  -- a SEPARATE, independently hand-authored literal,
 *                    WIND128_EXPECT[24] (and WIND129_EXPECT / MENU1_EXPECT), the
 *                    exact payload bytes we expect back. NEITHER FORK NOR the
 *                    EXPECT arrays is produced by the parser or any encoder, so a
 *                    wrong parse cannot agree by construction -- it must match an
 *                    independent value or go RED.
 *
 * The fork holds >= 2 TYPES with the first type holding >= 2 resources, so the
 * count-1 decode (+1), the ref-list iteration, the 3-byte data offset, the
 * multi-type type-list walk, and the signed/positive ID compare are all
 * non-trivially exercised:
 *   type WIND { id 128 (a real 24-byte WIND template), id 129 } ; type MENU { id 1 }.
 *
 * SELF-CHECK (verifier amendment): the fork's map preamble is EXACTLY 24 bytes
 *   per resource-manager.md Sec 1.3 -- 16 (fork-header copy) + 4 (nextMapHandle)
 *   + 2 (fileRefNum) + 2 (mapAttrs) -- so typeListOffset sits at MAP offset 24
 *   (NOT 22) and nameListOffset at 26. Asserted directly on the fixture bytes +
 *   on the parser's computed type_list_abs (= mapOffset + 28).
 *
 * Self-mutants (compile + go RED -- Rule 6; #ifdef in os/flair/resource.c):
 *   RES_MUT_IGNORE_TYPE      -- match the first type regardless of the type key.
 *   RES_MUT_COUNT_OFF_BY_ONE -- iterate numTypes-1 type entries (drop the last).
 *
 * Ref: CLAUDE.md Law 1/Law 2/Law 3, Rule 1, Rule 6, Rule 11, Rule 12;
 *      ../system7-decomp/specs/resources/resource-manager.md Sec 1.1-1.6, Sec 2;
 *      ../system7-decomp/specs/resources/wind-menu-dlog-ditl.md Sec 1a/2a.
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>     /* harness-only: memcmp/memset (the TEST is factory code) */

#include "ostype.h"     /* FLAIR_OSTYPE (-Ios/flair)                              */
#include "heap.h"       /* flair_heap_t + flair_heap_init (-Ios/flair)            */
#include "resource.h"   /* THE reader under test (-Ios/flair)                     */
#include "test_assert.h"/* TEST_HARNESS / CHECK / TEST_SUMMARY (-Iseed)           */

TEST_HARNESS();

/* Fixture geometry (independently re-derived; cross-checked in the self-check). */
enum {
    F_DATA_OFF      = 16,   /* dataOffset                                         */
    F_MAP_OFF       = 72,   /* mapOffset = dataOffset + dataLength               */
    F_TYPE_LIST_ABS = 100,  /* mapOffset + typeListOffset(28) -- 28 = 24 + 2 + 2  */
    F_FORK_LEN      = 154   /* mapOffset + mapLength                              */
};

/* ===========================================================================
 * GOLDEN INPUT -- the hand-authored mini resource fork.
 *
 * Authored byte-by-byte from resource-manager.md Sec 1.1-1.6 + wind-menu-dlog-
 * ditl.md Sec 1a. Offsets in [brackets] are ABSOLUTE fork offsets.
 * ===========================================================================*/
static const uint8_t FORK[] = {
    /* === Sec 1.1 Fork header (16 bytes, all big-endian u32) === */
    0x00, 0x00, 0x00, 0x10,  /* [0]  dataOffset = 16                              */
    0x00, 0x00, 0x00, 0x48,  /* [4]  mapOffset  = 72                              */
    0x00, 0x00, 0x00, 0x38,  /* [8]  dataLength = 56  (16+56 = 72 = mapOffset)    */
    0x00, 0x00, 0x00, 0x52,  /* [12] mapLength  = 82  (72+82 = 154 = fork total)  */

    /* === Sec 1.2 Data area (starts at dataOffset = 16) ===
       Each resource = 4-byte BE length prefix + that many data bytes. */

    /* -- Resource WIND 128: data-area offset 0, abs 16 -- */
    0x00, 0x00, 0x00, 0x18,  /* [16] length prefix = 24                          */
    /* [20..43] the 24-byte WIND template (wind-menu-dlog-ditl.md Sec 1a):       */
    0x00, 0x32,              /* [20] boundsRect.top    = 50                       */
    0x00, 0x50,              /* [22] boundsRect.left   = 80                       */
    0x00, 0xC8,              /* [24] boundsRect.bottom = 200                      */
    0x01, 0x7C,              /* [26] boundsRect.right  = 380                      */
    0x00, 0x00,              /* [28] procID = 0 (documentProc)                    */
    0x01,                    /* [30] visible = 1                                  */
    0x00,                    /* [31] (filler -- word align)                       */
    0x01,                    /* [32] goAwayFlag = 1                               */
    0x00,                    /* [33] (filler -- word align)                       */
    0x00, 0x00, 0x00, 0x00,  /* [34] refCon = 0                                   */
    0x05,                    /* [38] title length = 5 (Pascal Str255)             */
    0x41, 0x62, 0x6F, 0x75, 0x74, /* [39] "About"                                 */

    /* -- Resource WIND 129: data-area offset 28, abs 44 -- */
    0x00, 0x00, 0x00, 0x04,  /* [44] length prefix = 4                            */
    0x00, 0x01, 0x00, 0x00,  /* [48] payload (distinct length != 24 -- exercises  */
                             /*      a SECOND ref entry + its own 3-byte offset)  */

    /* -- Resource MENU 1: data-area offset 36, abs 52 -- */
    0x00, 0x00, 0x00, 0x10,  /* [52] length prefix = 16                           */
    /* [56..71] a real 16-byte MENU template (wind-menu-dlog-ditl.md Sec 2a):     */
    0x00, 0x01,              /* [56] menuID = 1                                   */
    0x00, 0x00,              /* [58] menuWidth = 0                                */
    0x00, 0x00,              /* [60] menuHeight = 0                               */
    0x00, 0x00, 0x00, 0x00,  /* [62] menuProc = 0 (template placeholder)          */
    0xFF, 0xFF, 0xFF, 0xFF,  /* [66] enableFlags = all enabled                    */
    0x00,                    /* [70] title length = 0                             */
    0x00,                    /* [71] item sentinel = 0 (no items)                 */

    /* === Sec 1.3 Resource map (starts at mapOffset = 72) ===
       24-BYTE preamble = 16 (fork-header copy) + 4 + 2 + 2, THEN the 2 offsets. */
    /* [72..87] copy of the 16-byte fork header: */
    0x00, 0x00, 0x00, 0x10,  /* [72] (dataOffset copy)                            */
    0x00, 0x00, 0x00, 0x48,  /* [76] (mapOffset copy)                             */
    0x00, 0x00, 0x00, 0x38,  /* [80] (dataLength copy)                            */
    0x00, 0x00, 0x00, 0x52,  /* [84] (mapLength copy)                             */
    0x00, 0x00, 0x00, 0x00,  /* [88] nextMapHandle (runtime; 0 on disk)           */
    0x00, 0x00,              /* [92] fileRefNum (runtime; 0)                       */
    0x00, 0x00,              /* [94] mapAttrs = 0  (preamble ends at offset 24)    */
    0x00, 0x1C,              /* [96] typeListOffset = 28  (MAP offset 24)          */
    0x00, 0x52,              /* [98] nameListOffset = 82  (MAP offset 26)          */

    /* === Sec 1.4 Type list (at mapOffset + 28 = 100) === */
    0x00, 0x01,              /* [100] numTypes - 1 = 1  (=> 2 types)              */
    /* -- type entry 0: WIND (8 bytes) -- */
    0x57, 0x49, 0x4E, 0x44,  /* [102] resType = 'W''I''N''D'                       */
    0x00, 0x01,              /* [106] resCount - 1 = 1  (=> 2 WIND resources)     */
    0x00, 0x12,              /* [108] refListOffset = 18 (rel. to type-list start)*/
    /* -- type entry 1: MENU (8 bytes) -- */
    0x4D, 0x45, 0x4E, 0x55,  /* [110] resType = 'M''E''N''U'                       */
    0x00, 0x00,              /* [114] resCount - 1 = 0  (=> 1 MENU resource)      */
    0x00, 0x2A,              /* [116] refListOffset = 42 (rel. to type-list start)*/

    /* === Sec 1.5 Reference lists (12 bytes each, rel. to type-list start) ===
       WIND ref list at type-list offset 18 -> abs 100+18 = 118. */
    /* -- WIND entry 0: id 128 -- */
    0x00, 0x80,              /* [118] resID = 128                                 */
    0xFF, 0xFF,              /* [120] resNameOffset = 0xFFFF (unnamed)            */
    0x00,                    /* [122] resAttrs = 0                                */
    0x00, 0x00, 0x00,        /* [123] resDataOffset24 = 0   (abs = 16 + 0  = 16)  */
    0x00, 0x00, 0x00, 0x00,  /* [126] (reserved -- runtime handle, 0 on disk)     */
    /* -- WIND entry 1: id 129 -- */
    0x00, 0x81,              /* [130] resID = 129                                 */
    0xFF, 0xFF,              /* [132] resNameOffset = 0xFFFF (unnamed)            */
    0x00,                    /* [134] resAttrs = 0                                */
    0x00, 0x00, 0x1C,        /* [135] resDataOffset24 = 28  (abs = 16 + 28 = 44)  */
    0x00, 0x00, 0x00, 0x00,  /* [138] (reserved)                                  */
    /* MENU ref list at type-list offset 42 -> abs 100+42 = 142. */
    /* -- MENU entry 0: id 1 -- */
    0x00, 0x01,              /* [142] resID = 1                                   */
    0xFF, 0xFF,              /* [144] resNameOffset = 0xFFFF (unnamed)            */
    0x00,                    /* [146] resAttrs = 0                                */
    0x00, 0x00, 0x24,        /* [147] resDataOffset24 = 36  (abs = 16 + 36 = 52)  */
    0x00, 0x00, 0x00, 0x00   /* [150] (reserved)                                  */

    /* === Sec 1.6 Name list (at mapOffset + 82 = 154) === empty: all unnamed. */
};

/* ===========================================================================
 * GOLDEN OUTPUT -- INDEPENDENT hand-authored expected payloads.
 * These are written as plain literals (NOT produced by the parser or any
 * encoder); FlairGetResource must reproduce them byte-for-byte (Law 2).
 * ===========================================================================*/

/* The exact 24 payload bytes FlairGetResource(WIND, 128) must return. */
static const uint8_t WIND128_EXPECT[24] = {
    0x00, 0x32,              /* boundsRect.top    = 50  */
    0x00, 0x50,              /* boundsRect.left   = 80  */
    0x00, 0xC8,              /* boundsRect.bottom = 200 */
    0x01, 0x7C,              /* boundsRect.right  = 380 */
    0x00, 0x00,              /* procID = 0              */
    0x01,                    /* visible = 1             */
    0x00,                    /* filler                  */
    0x01,                    /* goAwayFlag = 1          */
    0x00,                    /* filler                  */
    0x00, 0x00, 0x00, 0x00,  /* refCon = 0              */
    0x05,                    /* title length = 5        */
    0x41, 0x62, 0x6F, 0x75, 0x74   /* "About"           */
};

/* The 4 payload bytes for WIND 129 (a distinct length proves ref-list walk). */
static const uint8_t WIND129_EXPECT[4] = { 0x00, 0x01, 0x00, 0x00 };

/* The 16 payload bytes for MENU 1 (a real menu template; proves type-list walk). */
static const uint8_t MENU1_EXPECT[16] = {
    0x00, 0x01,              /* menuID = 1     */
    0x00, 0x00,              /* menuWidth = 0  */
    0x00, 0x00,              /* menuHeight = 0 */
    0x00, 0x00, 0x00, 0x00,  /* menuProc = 0   */
    0xFF, 0xFF, 0xFF, 0xFF,  /* enableFlags    */
    0x00, 0x00               /* title len + item sentinel */
};

/* A malformed fork: a 16-byte header whose mapOffset (200) lies past the buffer
 * -> a truncated/inconsistent map. Must fail loud (load < 0). */
static const uint8_t FORK_BADMAP[16] = {
    0x00, 0x00, 0x00, 0x10,  /* dataOffset = 16        */
    0x00, 0x00, 0x00, 0xC8,  /* mapOffset  = 200 (> 16 -- past the fork)          */
    0x00, 0x00, 0x00, 0x00,  /* dataLength = 0         */
    0x00, 0x00, 0x00, 0x00   /* mapLength  = 0         */
};

int main(void)
{
    static uint8_t arena_buf[4096];
    flair_heap_t arena;
    FlairResMap m;

    /* ---- fixture self-check: the map preamble is EXACTLY 24 bytes (Sec 1.3) --
     * 16 (fork-header copy) + 4 (nextMapHandle) + 2 (fileRefNum) + 2 (mapAttrs)
     * = 24, so typeListOffset lands at MAP offset 24 (NOT 22) and nameList at 26.
     * Verify directly on the fixture bytes before parsing. */
    CHECK(sizeof FORK == (size_t)F_FORK_LEN,
          "self-check: FORK is 154 bytes (mapOffset 72 + mapLength 82)");
    CHECK(FORK[F_MAP_OFF + 22] == 0x00 && FORK[F_MAP_OFF + 23] == 0x00,
          "self-check: mapAttrs occupies preamble offsets 22..23 (the 24-byte preamble)");
    CHECK(FORK[F_MAP_OFF + 24] == 0x00 && FORK[F_MAP_OFF + 25] == 0x1C,
          "self-check: typeListOffset(=28) sits at MAP offset 24 (24-byte preamble, NOT 22)");
    CHECK(FORK[F_MAP_OFF + 26] == 0x00 && FORK[F_MAP_OFF + 27] == 0x52,
          "self-check: nameListOffset(=82) sits at MAP offset 26");

    flair_heap_init(&arena, arena_buf, (uint32_t)sizeof arena_buf);

    /* ===================== Step 1: load succeeds ===================== */
    CHECK(FlairResMap_load(&m, &arena, FORK, (uint32_t)sizeof FORK) == 0,
          "step 1: FlairResMap_load parses the fork -> 0");
    /* parser cross-check of the 24-byte preamble: type_list_abs = mapOff + 28. */
    CHECK(m.map_off == (uint32_t)F_MAP_OFF,
          "step 1: parsed mapOffset == 72");
    CHECK(m.type_list_abs == (uint32_t)F_TYPE_LIST_ABS,
          "step 1: type_list_abs == mapOffset + 28 (proves the 24-byte preamble)");
    CHECK(m.n_types == 2,
          "step 1: decoded numTypes-1 + 1 == 2 types");

    /* ============ Step 2: GetResource(WIND, 128) == golden ============ */
    {
        uint32_t n = 0;
        const uint8_t *p = FlairGetResource(&m, FLAIR_RESTYPE_WIND, 128, &n);
        CHECK(p != NULL, "step 2: GetResource(WIND,128) is non-NULL");
        CHECK(n == 24, "step 2: WIND 128 length == 24");
        CHECK(p != NULL && n == 24 && memcmp(p, WIND128_EXPECT, 24) == 0,
              "step 2: WIND 128 payload == the independent WIND128_EXPECT golden");
    }

    /* ===== Step 3: GetResource(WIND, 129) -- multi-resource ref list ===== */
    {
        uint32_t n = 0;
        const uint8_t *p = FlairGetResource(&m, FLAIR_RESTYPE_WIND, 129, &n);
        CHECK(p != NULL, "step 3: GetResource(WIND,129) non-NULL (2nd ref entry; count-1+1)");
        CHECK(n == 4, "step 3: WIND 129 length == 4 (distinct length prefix)");
        CHECK(p != NULL && n == 4 && memcmp(p, WIND129_EXPECT, 4) == 0,
              "step 3: WIND 129 payload == WIND129_EXPECT");
    }

    /* ======== Step 4: GetResource(MENU, 1) -- multi-type type walk ======== */
    {
        uint32_t n = 0;
        const uint8_t *p = FlairGetResource(&m, FLAIR_RESTYPE_MENU, 1, &n);
        CHECK(p != NULL, "step 4: GetResource(MENU,1) non-NULL (2nd type -- type-list walk)");
        CHECK(n == 16, "step 4: MENU 1 length == 16");
        CHECK(p != NULL && n == 16 && memcmp(p, MENU1_EXPECT, 16) == 0,
              "step 4: MENU 1 payload == MENU1_EXPECT");
    }

    /* ============ Step 5: absent ID in a present type -> NULL ============ */
    {
        uint32_t n = 123;
        const uint8_t *p = FlairGetResource(&m, FLAIR_RESTYPE_WIND, 999, &n);
        CHECK(p == NULL, "step 5: GetResource(WIND,999) -> NULL (absent ID)");
        CHECK(n == 0, "step 5: out_len reset to 0 on a miss");
    }

    /* ============ Step 6: absent TYPE -> NULL (wrong-type guard) ============ */
    {
        uint32_t n = 123;
        const uint8_t *p = FlairGetResource(&m, FLAIR_RESTYPE_CURS, 128, &n);
        CHECK(p == NULL, "step 6: GetResource(CURS,128) -> NULL (type absent; key matters)");
        CHECK(n == 0, "step 6: out_len reset to 0 on a type miss");
    }

    /* =================== Step 7: CountResources(WIND) =================== */
    CHECK(FlairCountResources(&m, FLAIR_RESTYPE_WIND) == 2,
          "step 7: CountResources(WIND) == 2 (resCount-1 + 1)");
    CHECK(FlairCountResources(&m, FLAIR_RESTYPE_MENU) == 1,
          "step 7: CountResources(MENU) == 1");
    CHECK(FlairCountResources(&m, FLAIR_RESTYPE_CURS) == 0,
          "step 7: CountResources(CURS) == 0 (absent type)");

    /* ============= Step 8: malformed forks fail loud (load < 0) ============= */
    {
        FlairResMap mt;
        /* (a) truncated header: len < 16. */
        CHECK(FlairResMap_load(&mt, &arena, FORK, 10u) < 0,
              "step 8a: a truncated fork (len < 16) -> load < 0 (fail loud)");
        CHECK(mt.fork == NULL, "step 8a: failed load leaves fork == NULL");
        /* (b) inconsistent map: mapOffset (200) past the 16-byte buffer. */
        CHECK(FlairResMap_load(&mt, &arena, FORK_BADMAP, (uint32_t)sizeof FORK_BADMAP) < 0,
              "step 8b: a fork whose mapOffset lies past the buffer -> load < 0");
        CHECK(mt.fork == NULL, "step 8b: failed load leaves fork == NULL");
        /* NULL args fail loud too. */
        CHECK(FlairResMap_load(NULL, &arena, FORK, (uint32_t)sizeof FORK) < 0,
              "step 8c: NULL map arg -> load < 0");
        CHECK(FlairGetResource(NULL, FLAIR_RESTYPE_WIND, 128, NULL) == NULL,
              "step 8c: NULL map -> GetResource NULL");
    }

    /* ---- bonus: GetResInfo reverse lookup round-trips (type, id). ---- */
    {
        uint32_t n = 0;
        const uint8_t *p = FlairGetResource(&m, FLAIR_RESTYPE_WIND, 128, &n);
        int16_t id = 0;
        flair_ostype_t ty = 0;
        CHECK(p != NULL && FlairGetResInfo(&m, p, &id, &ty) == 0,
              "bonus: GetResInfo(WIND 128 data) -> 0");
        CHECK(id == 128 && ty == FLAIR_RESTYPE_WIND,
              "bonus: GetResInfo recovers (WIND, 128)");
    }

    return TEST_SUMMARY("test-resource");
}
