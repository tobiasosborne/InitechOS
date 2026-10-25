/* test_overlay.c -- host oracle for INT 21h AH=4Bh AL=03h LOAD OVERLAY.
 *
 * Ref: docs/adr/ADR-0003-AMENDMENT-DEC-19-INT21h-AH4Bh-AL03h-Load-Overlay.md
 *      (DEC-19.1..19.6, un-deferring ADR-0003 DEC-08a.7); IBM DOS 3.30
 *      Technical Reference p. 6-196 (AL=03H "Load, do not create the program
 *      segment prefix, and do not begin execution"), p. 6-197 (the block: WORD
 *      load segment, WORD relocation factor), p. 11-5 (no PSP, no allocation,
 *      the caller is assumed to own the target block), p. 6-42 (error codes 2,
 *      9, 11); spec/dos_structs.h exec_overlay_block_t (locked layout, Rule 8);
 *      os/milton/loader.c loader_overlay_core; os/milton/int21.c
 *      do_exec_overlay + int21_overlay_dest_ok; os/milton/mcb.c mcb_owned_block.
 *      CLAUDE.md Law 2, Rule 1 (RED first), Rule 2, Rule 6, Rule 12 (ASCII).
 *
 * Three parts, all against the REAL artifact sources:
 *   A. loader_overlay_core over an in-memory file and a fake owned window:
 *      relocation by the caller's factor, the exact footprint (canaries past
 *      it stay intact), raw verbatim copy, the untagged-MZ refusal, malformed
 *      MZ refused BEFORE a byte is written, the ownership refusal.
 *   B. int21_dispatch AH=4Bh AL=03h with a mock overlay backend: the EBX
 *      block is decoded per spec/dos_structs.h, a NULL block is 0009h, a
 *      backend error is returned, other subfunctions stay 0001h.
 *   C. int21_overlay_dest_ok against a bound PSP + MCB arena: the image/BSS
 *      window and an AH=48h block are accepted to the exact byte; one byte
 *      past either, the PSP, an MCB header, a freed or foreign block refused.
 *
 * Every expected value is a literal computed by hand from the fixture bytes
 * below, never from the code under test (Law 2).
 *
 * MUTATION (Rule 6), driven by make test-overlay-mutant:
 *   -DLOADER_MUTATE_OVL_NO_FACTOR      -> A1/A2 relocated-dword checks RED
 *   -DINT21_MUTATE_OVL_BOUNDS_OBO      -> C one-byte-past checks RED
 *   -DLOADER_MUTATE_OVL_ACCEPT_FOREIGN -> A5 untagged-MZ refusal RED
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#include "int21.h"
#include "loader.h"
#include "mz.h"
#include "psp.h"
#include "dos_structs.h"
#include "test_assert.h"

TEST_HARNESS();

/* ---- byte helpers ------------------------------------------------------ */
static void put16(uint8_t *b, uint32_t off, uint16_t v)
{
    b[off] = (uint8_t)v;
    b[off + 1u] = (uint8_t)(v >> 8);
}
static void put32(uint8_t *b, uint32_t off, uint32_t v)
{
    b[off] = (uint8_t)v;
    b[off + 1u] = (uint8_t)(v >> 8);
    b[off + 2u] = (uint8_t)(v >> 16);
    b[off + 3u] = (uint8_t)(v >> 24);
}
static uint32_t get32(const uint8_t *b, uint32_t off)
{
    return (uint32_t)b[off] | ((uint32_t)b[off + 1u] << 8) |
           ((uint32_t)b[off + 2u] << 16) | ((uint32_t)b[off + 3u] << 24);
}

/* ---- the in-memory "file" the core reads -------------------------------- */
static uint8_t  g_file[2048];
static uint32_t g_file_len;
static int      g_read_fail;     /* make every read fail (A9) */
static int      g_reads;

static int mem_read(void *ctx, uint32_t off, uint32_t len, uint8_t *dst)
{
    (void)ctx;
    g_reads++;
    if (g_read_fail || off > g_file_len || len > g_file_len - off) {
        return 1;
    }
    memcpy(dst, g_file + off, len);
    return 0;
}

/* ---- the fake owned window ---------------------------------------------- */
static uint32_t g_win_lo, g_win_hi;
static uint32_t g_owns_lin, g_owns_len;
static int      g_owns_calls;
static int win_owns(uint32_t lin, uint32_t len)
{
    g_owns_calls++;
    g_owns_lin = lin;
    g_owns_len = len;
    return lin >= g_win_lo && lin <= g_win_hi && len <= g_win_hi - lin;
}

/* Destination: 256 bytes of footprint room + canaries, all preset to 0xA5. */
#define CANARY 0xA5u
static uint8_t g_dest[512];
static uint8_t g_scratch[LOADER_OVL_SCRATCH];

static void dest_reset(void)
{
    memset(g_dest, CANARY, sizeof g_dest);
    g_owns_calls = 0;
    g_owns_lin = g_owns_len = 0u;
    g_reads = 0;
    g_read_fail = 0;
}
static int dest_canary_from(uint32_t from)
{
    for (uint32_t i = from; i < sizeof g_dest; i++) {
        if (g_dest[i] != CANARY) return 0;
    }
    return 1;
}

/* An InitechMZ: 32-byte header (e_cparhdr = 2), a 64-byte load module whose
 * dwords at module offsets 4, 16 and 60 hold link-time values, and a trailer
 * relocation table naming those three offsets (mzlink's layout). */
#define MOD_LEN 64u
static void build_mz(uint16_t tag, uint32_t reloc_bad)
{
    memset(g_file, 0, sizeof g_file);
    uint32_t image = 0x20u + MOD_LEN;          /* 96 bytes */
    put16(g_file, 0x00, 0x5A4Du);              /* 'MZ'                 */
    put16(g_file, 0x02, (uint16_t)(image % 512u));
    put16(g_file, 0x04, (uint16_t)((image + 511u) / 512u));
    put16(g_file, 0x06, 3u);                   /* e_crlc               */
    put16(g_file, 0x08, 2u);                   /* e_cparhdr            */
    put16(g_file, 0x0A, 0x0100u);              /* e_minalloc (ignored) */
    put16(g_file, 0x0C, 0xFFFFu);              /* e_maxalloc (ignored) */
    put16(g_file, 0x18, (uint16_t)image);      /* e_lfarlc: trailer    */
    put16(g_file, 0x1C, tag);                  /* e_res[0]             */
    for (uint32_t i = 0; i < MOD_LEN; i++) {
        g_file[0x20u + i] = (uint8_t)(0x40u + i);
    }
    put32(g_file, 0x20u + 4u,  0x00000010u);
    put32(g_file, 0x20u + 16u, 0x00000020u);
    put32(g_file, 0x20u + 60u, 0x0000003Cu);   /* the module's last dword */
    put32(g_file, image + 0u, 4u);
    put32(g_file, image + 4u, 16u);
    put32(g_file, image + 8u, reloc_bad ? reloc_bad : 60u);
    g_file_len = image + 12u;
}

/* ---- Part B: the mock overlay backend ----------------------------------- */
static int      g_ovl_called;
static char     g_ovl_name[16];
static uint16_t g_ovl_dir;
static uint32_t g_ovl_load, g_ovl_factor;
static uint16_t g_ovl_result;
static uint16_t ovl_mock(const char *name83, uint16_t dir_start,
                         uint32_t load_addr, uint32_t reloc_factor)
{
    g_ovl_called++;
    strncpy(g_ovl_name, name83, sizeof g_ovl_name - 1u);
    g_ovl_name[sizeof g_ovl_name - 1u] = '\0';
    g_ovl_dir = dir_start;
    g_ovl_load = load_addr;
    g_ovl_factor = reloc_factor;
    return g_ovl_result;
}
static int      g_exec_called;
static uint16_t exec_mock(const char *n, uint16_t d, const char *t, uint32_t tl,
                          uint32_t e, uint8_t *rc)
{
    (void)n; (void)d; (void)t; (void)tl; (void)e;
    g_exec_called++;
    if (rc) *rc = 0u;
    return 0u;
}

/* Flat pointers must round-trip through uint32_t on a 64-bit host. */
static void *alloc_low(size_t n)
{
    void *p = MAP_FAILED;
#ifdef MAP_32BIT
    p = mmap(NULL, n, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#endif
    if (p == MAP_FAILED || (uintptr_t)p > 0xFFFFFFFFu) {
        return NULL;
    }
    return p;
}

#define CF_BIT 0x1u
static int_frame_t frame(uint32_t eax, uint32_t ebx, uint32_t edx)
{
    int_frame_t f;
    memset(&f, 0, sizeof f);
    f.eflags = 0x00000203u;          /* CF preloaded so a clear is observed */
    f.eax = eax;
    f.ebx = ebx;
    f.edx = edx;
    return f;
}
static int cf(const int_frame_t *f) { return (f->eflags & CF_BIT) != 0u; }
static uint16_t ax(const int_frame_t *f) { return (uint16_t)f->eax; }

int main(void)
{
    loader_status_t st;
    uint32_t got;
    const uint32_t LOAD = 0x00052000u;     /* fake flat address of g_dest */
    const uint32_t FACTOR = 0x00123400u;   /* deliberately != LOAD        */

    /* ===================== Part A: loader_overlay_core ================== */
    g_win_lo = LOAD;
    g_win_hi = LOAD + 0x100u;

    /* A1: tagged InitechMZ, factor != load address. Each relocated dword is
     * its link-time value + FACTOR (DEC-19.3), never + LOAD; the footprint is
     * exactly the 64-byte module (header + reloc table never written). */
    build_mz(MZ_INITECH_TAG, 0u);
    dest_reset();
    got = 0xDEADu;
    st = loader_overlay_core(mem_read, 0, g_file_len, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_OK, "A1 tagged InitechMZ overlay loads");
    CHECK(got == MOD_LEN, "A1 footprint == load module length (64)");
    CHECK(g_owns_calls == 1 && g_owns_lin == LOAD && g_owns_len == MOD_LEN,
          "A1 ownership asked for (load address, module length)");
    CHECK(get32(g_dest, 4u) == 0x00123410u,
          "A1 reloc @4: 0x10 + factor 0x123400 = 0x123410");
    CHECK(get32(g_dest, 16u) == 0x00123420u,
          "A1 reloc @16: 0x20 + factor = 0x123420");
    CHECK(get32(g_dest, 60u) == 0x0012343Cu,
          "A1 reloc @60 (the module's last dword): 0x3C + factor");
    CHECK(g_dest[0] == 0x40u && g_dest[8] == 0x48u && g_dest[59] == 0x7Bu,
          "A1 unrelocated module bytes copied verbatim");
    CHECK(dest_canary_from(MOD_LEN),
          "A1 not one byte written past the module (TR p. 11-5 footprint)");

    /* A2: relocation table in the header gap (the period layout) with 70
     * entries -- more than one 64-entry scratch chunk. Entry k names module
     * offset 4*k; every dword starts at k and must end k + FACTOR. */
    {
        uint32_t n = 70u, hdr = 0x20u + n * 4u;   /* 0x138, paragraph-padded */
        uint32_t cparhdr = (hdr + 15u) / 16u;     /* 20 paragraphs = 0x140   */
        uint32_t mod = n * 4u;                    /* 280-byte module          */
        uint32_t image = cparhdr * 16u + mod;
        memset(g_file, 0, sizeof g_file);
        put16(g_file, 0x00, 0x5A4Du);
        put16(g_file, 0x02, (uint16_t)(image % 512u));
        put16(g_file, 0x04, (uint16_t)((image + 511u) / 512u));
        put16(g_file, 0x06, (uint16_t)n);
        put16(g_file, 0x08, (uint16_t)cparhdr);
        put16(g_file, 0x18, 0x20u);               /* table right after header */
        put16(g_file, 0x1C, MZ_INITECH_TAG);
        for (uint32_t k = 0; k < n; k++) {
            put32(g_file, 0x20u + k * 4u, k * 4u);
            put32(g_file, cparhdr * 16u + k * 4u, k);
        }
        g_file_len = image;
        static uint8_t big[512];
        memset(big, CANARY, sizeof big);
        g_win_hi = LOAD + 0x200u;
        g_owns_calls = 0;
        st = loader_overlay_core(mem_read, 0, g_file_len, big, LOAD, FACTOR,
                                 win_owns, g_scratch, &got);
        CHECK(st == LOADER_OK && got == 280u,
              "A2 header-gap table, 70 relocs: loads, footprint 280");
        int all = 1;
        for (uint32_t k = 0; k < n; k++) {
            if (get32(big, k * 4u) != k + FACTOR) all = 0;
        }
        CHECK(all, "A2 all 70 dwords (across two scratch chunks) == k + factor");
        CHECK(big[280] == CANARY && big[511] == CANARY,
              "A2 nothing past the 280-byte module");
        g_win_hi = LOAD + 0x100u;
    }

    /* A3: a raw (non-MZ) file is copied verbatim; the factor is ignored. */
    memset(g_file, 0, sizeof g_file);
    memcpy(g_file, "OVL-RAW-VERBATIM\r\n$", 19u);
    put32(g_file, 19u, 0x00000010u);
    g_file_len = 23u;
    dest_reset();
    st = loader_overlay_core(mem_read, 0, g_file_len, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_OK && got == 23u, "A3 raw file loads, footprint = size");
    CHECK(memcmp(g_dest, "OVL-RAW-VERBATIM\r\n$", 19u) == 0 &&
          get32(g_dest, 19u) == 0x00000010u,
          "A3 raw bytes verbatim, no factor added");
    CHECK(g_owns_len == 23u && dest_canary_from(23u),
          "A3 ownership asked for 23 bytes; nothing written past them");

    /* A4: 'M' then not 'Z' is raw, not MZ (DEC-08a.4 content dispatch). */
    memcpy(g_file, "MQ", 2u);
    g_file_len = 2u;
    dest_reset();
    st = loader_overlay_core(mem_read, 0, g_file_len, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_OK && got == 2u && g_dest[1] == 'Q',
          "A4 two-byte non-MZ file copied raw");

    /* A5: an UNTAGGED MZ (genuine 16-bit DOS .EXE) is refused, never copied
     * as data and never relocated (DEC-08a.5; the kernel caller panics). */
    build_mz(0u, 0u);
    dest_reset();
    st = loader_overlay_core(mem_read, 0, g_file_len, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_ERR_FOREIGN_MZ, "A5 untagged MZ -> LOADER_ERR_FOREIGN_MZ");
    CHECK(dest_canary_from(0u) && g_owns_calls == 0,
          "A5 untagged MZ: destination untouched, ownership never asked");

    /* A6: an out-of-module relocation offset (61: the dword would run 1 byte
     * past the 64-byte module) is refused BEFORE any byte lands. */
    build_mz(MZ_INITECH_TAG, 61u);
    dest_reset();
    st = loader_overlay_core(mem_read, 0, g_file_len, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_ERR_BAD_FORMAT, "A6 OOB reloc (61 + 4 > 64) -> BAD_FORMAT");
    CHECK(dest_canary_from(0u), "A6 OOB reloc: destination untouched");

    /* A7: a truncated MZ (file shorter than e_cp/e_cblp declare). */
    build_mz(MZ_INITECH_TAG, 0u);
    dest_reset();
    st = loader_overlay_core(mem_read, 0, 0x40u, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_ERR_BAD_FORMAT && dest_canary_from(0u),
          "A7 truncated MZ -> BAD_FORMAT, nothing written");

    /* A8: the destination is not owned -> NOT_OWNED, nothing written. The
     * window ends exactly where a 64-byte module would need one more byte. */
    build_mz(MZ_INITECH_TAG, 0u);
    dest_reset();
    g_win_hi = LOAD + MOD_LEN - 1u;
    st = loader_overlay_core(mem_read, 0, g_file_len, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_ERR_NOT_OWNED && dest_canary_from(0u),
          "A8 footprint one byte past the window -> NOT_OWNED, untouched");
    g_win_hi = LOAD + MOD_LEN;
    dest_reset();
    st = loader_overlay_core(mem_read, 0, g_file_len, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_OK && dest_canary_from(MOD_LEN),
          "A8 exact-fit window accepted");
    g_win_hi = LOAD + 0x100u;

    /* A9: a read failure is LOADER_ERR_READ. */
    dest_reset();
    g_read_fail = 1;
    st = loader_overlay_core(mem_read, 0, g_file_len, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_ERR_READ, "A9 read failure -> LOADER_ERR_READ");

    /* A10: an empty file is a zero-byte raw overlay. A11: NULL arguments. */
    dest_reset();
    st = loader_overlay_core(mem_read, 0, 0u, g_dest, LOAD, FACTOR,
                             win_owns, g_scratch, &got);
    CHECK(st == LOADER_OK && got == 0u && dest_canary_from(0u),
          "A10 empty file: OK, nothing written");
    st = loader_overlay_core(mem_read, 0, g_file_len, g_dest, LOAD, FACTOR,
                             0, g_scratch, &got);
    CHECK(st == LOADER_ERR_NULL_OUT, "A11 NULL ownership callback refused");

    /* ===================== Part B: AH=4Bh AL=03h dispatch ================ */
    CHECK(sizeof(exec_overlay_block_t) == 8u &&
          offsetof(exec_overlay_block_t, load_addr) == 0u &&
          offsetof(exec_overlay_block_t, reloc_factor) == 4u,
          "B0 spec/dos_structs.h exec_overlay_block_t: 00h load, 04h factor");

    int21_set_exec_backend(exec_mock);
    int21_set_overlay_backend(ovl_mock);
    {
        uint8_t *low = alloc_low(4096u);
        CHECK(low != NULL, "B low-memory buffer");
        if (low == NULL) return TEST_SUMMARY("test_overlay");
        uint32_t lin = (uint32_t)(uintptr_t)low;
        memcpy(low, "OVLT.OVL", 9u);
        put32(low, 0x40u, 0x00041000u);           /* load address   */
        put32(low, 0x44u, 0x00098760u);           /* factor         */

        g_ovl_called = 0; g_exec_called = 0; g_ovl_result = 0u;
        int_frame_t f = frame(0x4B03u, lin + 0x40u, lin);
        int21_dispatch(&f);
        CHECK(g_ovl_called == 1 && g_exec_called == 0,
              "B1 AL=03h reaches the overlay backend, not EXEC");
        CHECK(strcmp(g_ovl_name, "OVLT.OVL") == 0 && g_ovl_dir == 0u,
              "B1 the resolved leaf + root dir_start are passed");
        CHECK(g_ovl_load == 0x00041000u && g_ovl_factor == 0x00098760u,
              "B1 EBX block decoded: load @00h, factor @04h");
        CHECK(!cf(&f), "B1 success clears CF");

        g_ovl_called = 0;
        f = frame(0x4B03u, 0u, lin);
        int21_dispatch(&f);
        CHECK(cf(&f) && ax(&f) == INT21_ERR_INVALID_MEMORY && g_ovl_called == 0,
              "B2 EBX = 0 -> CF, AX = 0009h, backend not called");

        g_ovl_result = INT21_ERR_BAD_FORMAT;
        f = frame(0x4B03u, lin + 0x40u, lin);
        int21_dispatch(&f);
        CHECK(cf(&f) && ax(&f) == 0x000Bu, "B3 backend 000Bh is returned");
        g_ovl_result = INT21_ERR_INVALID_MEMORY;
        f = frame(0x4B03u, lin + 0x40u, lin);
        int21_dispatch(&f);
        CHECK(cf(&f) && ax(&f) == 0x0009u, "B3 backend 0009h is returned");
        f = frame(0x5900u, 0u, 0u);
        int21_dispatch(&f);
        CHECK(ax(&f) == 0x0009u, "B3 AH=59h reports the overlay's 0009h");
        g_ovl_result = 0u;

        memcpy(low + 0x80u, "SUB\\OVLT.OVL", 13u);
        g_ovl_called = 0;
        f = frame(0x4B03u, lin + 0x40u, lin + 0x80u);
        int21_dispatch(&f);
        CHECK(cf(&f) && ax(&f) == INT21_ERR_PATH_NOT_FOUND && g_ovl_called == 0,
              "B4 AL=03h shares AL=00h path resolution (0003h)");

        int21_set_overlay_backend(NULL);
        f = frame(0x4B03u, lin + 0x40u, lin);
        int21_dispatch(&f);
        CHECK(cf(&f) && ax(&f) == INT21_ERR_FILE_NOT_FOUND,
              "B5 no overlay backend -> 0002h, never a silent success");
        int21_set_overlay_backend(ovl_mock);

        g_ovl_called = 0; g_exec_called = 0;
        f = frame(0x4B01u, lin + 0x40u, lin);
        int21_dispatch(&f);
        CHECK(cf(&f) && ax(&f) == INT21_ERR_INVALID_FUNCTION,
              "B6 AL=01h stays invalid function (TR p. 6-196 lists 00h, 03h)");
        f = frame(0x4B04u, lin + 0x40u, lin);
        int21_dispatch(&f);
        CHECK(cf(&f) && ax(&f) == INT21_ERR_INVALID_FUNCTION &&
              g_ovl_called == 0 && g_exec_called == 0,
              "B6 AL=04h stays invalid function; no backend called");
        f = frame(0x4B00u, 0u, lin);
        int21_dispatch(&f);
        CHECK(g_exec_called == 1 && g_ovl_called == 0 && !cf(&f),
              "B7 AL=00h still reaches EXEC, not the overlay backend");
    }

    /* ===================== Part C: int21_overlay_dest_ok ================= */
    {
        /* One region AT the real PROGRAM_BASE (0x40000) so every flat address
         * and DOS segment (linear >> 4, 16-bit) is the kernel's own number: PSP
         * at +0, image/BSS to +0x1000, then a 64-paragraph arena. The hint is
         * honored on an unused low range; anything else fails loud. */
        uint8_t *reg = mmap((void *)(uintptr_t)0x00040000u, 0x3000u,
                            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);
        if (reg != (uint8_t *)(uintptr_t)0x00040000u) reg = NULL;
        CHECK(reg != NULL, "C region mapped at PROGRAM_BASE 0x40000");
        if (reg == NULL) return TEST_SUMMARY("test_overlay");
        uint32_t psp = (uint32_t)(uintptr_t)reg;
        uint32_t arena = psp + 0x1000u;
        psp_params_t pp;
        memset(&pp, 0, sizeof pp);
        pp.alloc_end_linear = psp + 0x2000u;
        (void)psp_build((psp_t *)(void *)reg, &pp);
        int21_set_psp((struct psp *)(void *)reg);
        int21_set_mcb_arena(reg + 0x1000u, 64u, arena);

        /* (A) the image/BSS window [PSP+100h, arena). */
        CHECK(int21_overlay_dest_ok(psp + 0x100u, 0x1000u - 0x100u) == 1,
              "C1 image/BSS window accepted to the exact byte");
        CHECK(int21_overlay_dest_ok(psp + 0x100u, 0x1000u - 0x100u + 1u) == 0,
              "C1 one byte past the window (onto the first MCB) refused");
        CHECK(int21_overlay_dest_ok(arena - 4u, 4u) == 1,
              "C1 the window's last dword accepted");
        CHECK(int21_overlay_dest_ok(arena - 4u, 5u) == 0,
              "C1 the window's last dword + 1 refused");
        CHECK(int21_overlay_dest_ok(psp, 16u) == 0 &&
              int21_overlay_dest_ok(psp + 0xFFu, 1u) == 0,
              "C2 the caller's own PSP refused");
        CHECK(int21_overlay_dest_ok(0u, 4u) == 0 &&
              int21_overlay_dest_ok(0xFFFFFFF0u, 0x20u) == 0,
              "C3 NULL and 32-bit wrap refused (DEC-14)");
        CHECK(int21_overlay_dest_ok(arena, 16u) == 0 &&
              int21_overlay_dest_ok(arena + 16u, 16u) == 0,
              "C4 the arena header and FREE data refused");

        /* (B) an AH=48h block of 4 paragraphs owned by the caller. */
        int_frame_t f = frame(0x4800u, 4u, 0u);
        int21_dispatch(&f);
        CHECK(!cf(&f), "C5 AH=48h 4 paragraphs");
        uint32_t blk = (uint32_t)(uint16_t)f.eax << 4;
        CHECK(blk == arena + 16u, "C5 first block data right after the header");
        CHECK(int21_overlay_dest_ok(blk, 64u) == 1,
              "C5 owned AH=48h block accepted to the exact byte");
        CHECK(int21_overlay_dest_ok(blk, 65u) == 0,
              "C5 one byte past the block (the next MCB header) refused");
        CHECK(int21_overlay_dest_ok(blk + 60u, 4u) == 1 &&
              int21_overlay_dest_ok(blk + 60u, 5u) == 0,
              "C5 the block's last dword accepted, + 1 byte refused");
        CHECK(int21_overlay_dest_ok(blk - 16u, 16u) == 0,
              "C5 the block's own MCB header refused");
        CHECK(int21_overlay_dest_ok(blk + 64u, 16u) == 0,
              "C5 the following header / free block refused");

        /* A block another process owns is not the caller's. */
        {
            uint8_t *other = reg + 0x2000u;   /* a different PSP paragraph */
            int21_set_psp((struct psp *)(void *)other);
            CHECK(int21_overlay_dest_ok(blk, 16u) == 0,
                  "C6 a block owned by another PSP refused");
            int21_set_psp((struct psp *)(void *)reg);
        }

        /* A freed block is no longer owned. */
        f = frame(0x4900u, blk >> 4, 0u);
        int21_dispatch(&f);
        CHECK(!cf(&f), "C7 AH=49h frees the block");
        CHECK(int21_overlay_dest_ok(blk, 16u) == 0, "C7 freed block refused");
    }

    return TEST_SUMMARY("test_overlay");
}
