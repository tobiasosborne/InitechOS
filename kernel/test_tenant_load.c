/* test_tenant_load.c -- host oracle for the DEC-08a PARAMETERIZED-BASE
 * extension that disk-launched FLAIR tenants ride (bead initech-tdnl.14).
 *
 * Ref: docs/design/GUI-remediation-ADR-reconciliation.md Part B DEC-AC3-1
 *        ("host fixtures for parameterized mz_apply_relocs + psp_build at a
 *        non-PROGRAM_BASE address (mutation-proven)"), Part A.4 (the psp_build
 *        VERIFY residual), Part C item 3 (BLOCKING: the missing oracle);
 *      os/milton/loader.h (loader_prepare_tenant contract);
 *      spec/toolbox_gate.h Sec 8 (block layout); spec/dos_structs.h (psp_t).
 *
 * WHAT IS GRADED (Law 2: every expected value is computed HERE from the
 * fixture's own numbers, never read back from the loader's arithmetic):
 *   T1 a tagged MZ in a block whose FAKE linear base is FAR from
 *      PROGRAM_IMAGE relocates every site to (link value + base + 256), the
 *      header-resident AND trailer reloc tables alike, the module lands at
 *      block+256, entry = base+256+entry_off, and the BSS tail is zeroed;
 *   T2 psp_build at an arbitrary address is ADDRESS-INDEPENDENT: the same
 *      params built at two different buffers give byte-identical 256 bytes, and
 *      the stored fields are the params' fake paragraphs (the VERIFY residual:
 *      no PSP field is derived from where the PSP sits);
 *   T3 refusals: untagged MZ -> FOREIGN (the kernel panics), a flat .COM ->
 *      BAD_FORMAT (V1 tenants are InitechMZ only), an e_minalloc that does not
 *      fit the block -> BAD_FORMAT, an OOB reloc -> BAD_FORMAT;
 *   T4 the EXEC path is untouched: loader_prepare_mz still relocates against
 *      PROGRAM_IMAGE (DEC-08a.1/.2 byte-identical).
 * Mutation (Rule 6): -DLOADER_MUTATE_TENANT_PROGRAM_IMAGE_BASE relocates the
 * tenant against PROGRAM_IMAGE -> T1 goes RED (test-tenant-load-mutant).
 * ASCII-clean (Rule 12). Host libc is fine in a factory test (Law 3).
 */
#include <stdint.h>
#include <string.h>

#include "loader.h"
#include "memory_map.h"
#include "mz.h"
#include "toolbox_gate.h"
#include "test_assert.h"

TEST_HARNESS();

#define HDR_SIZE 0x20u

static void put16(uint8_t *b, uint32_t off, uint16_t v)
{
    b[off + 0] = (uint8_t)(v & 0xFFu);
    b[off + 1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *b, uint32_t off, uint32_t v)
{
    b[off + 0] = (uint8_t)(v & 0xFFu);
    b[off + 1] = (uint8_t)((v >> 8) & 0xFFu);
    b[off + 2] = (uint8_t)((v >> 16) & 0xFFu);
    b[off + 3] = (uint8_t)((v >> 24) & 0xFFu);
}

static uint32_t get32(const uint8_t *b, uint32_t off)
{
    return (uint32_t)b[off] | ((uint32_t)b[off + 1] << 8) |
           ((uint32_t)b[off + 2] << 16) | ((uint32_t)b[off + 3] << 24);
}

static uint16_t get16(const uint8_t *b, uint32_t off)
{
    return (uint16_t)(b[off] | (b[off + 1] << 8));
}

/* A 32-byte InitechMZ header; image_bytes = header + module. */
static void build_hdr(uint8_t *b, uint16_t crlc, uint16_t lfarlc,
                      uint16_t minalloc, uint16_t ip, uint16_t tag,
                      uint32_t image_bytes)
{
    memset(b, 0, HDR_SIZE);
    b[0] = 0x4Du; b[1] = 0x5Au;
    put16(b, 0x02, (uint16_t)(image_bytes % 512u));
    put16(b, 0x04, (uint16_t)((image_bytes + 511u) / 512u));
    put16(b, 0x06, crlc);
    put16(b, 0x08, (uint16_t)(HDR_SIZE / 16u));
    put16(b, 0x0A, minalloc);
    put16(b, 0x0C, 0xFFFFu);
    put16(b, 0x14, ip);
    put16(b, 0x18, lfarlc);
    put16(b, 0x1C, tag);
}

/* The fake FLAT LINEAR base of the block: inside the FLAIR heap window
 * [0x100000,0x500000) and deliberately nowhere near PROGRAM_IMAGE. */
#define FAKE_BASE 0x00123450u

int main(void)
{
    /* ---- T1: trailer reloc table, three sites, entry_off 8, minalloc 2. ---- */
    {
        uint8_t block[1024];
        const uint32_t mod_len = 0x60u;
        const uint32_t sites[3] = { 0x04u, 0x20u, 0x5Cu };
        const uint32_t link[3]  = { 0x00000010u, 0x00000040u, 0x0000005Cu };
        uint8_t *file = block + TBX_TENANT_PSP_BYTES;
        uint32_t file_len;
        tenant_plan_t plan;
        loader_status_t st;

        memset(block, 0xCCu, sizeof block);           /* a dirty heap block */
        build_hdr(file, 3u, (uint16_t)(HDR_SIZE + mod_len), 2u, 8u,
                  MZ_INITECH_TAG, HDR_SIZE + mod_len);
        for (uint32_t i = 0; i < mod_len; i++)
            file[HDR_SIZE + i] = (uint8_t)(0x90u);    /* nop filler */
        for (int i = 0; i < 3; i++) {
            put32(file, HDR_SIZE + sites[i], link[i]);
            put32(file, HDR_SIZE + mod_len + 4u * (uint32_t)i, sites[i]);
        }
        file_len = HDR_SIZE + mod_len + 12u;

        st = loader_prepare_tenant(block, (uint32_t)sizeof block, file_len,
                                   FAKE_BASE, &plan);
        CHECK(st == LOADER_OK, "T1 tagged MZ in a heap block prepares OK");
        for (int i = 0; i < 3; i++) {
            CHECK(get32(file, sites[i]) ==
                      link[i] + FAKE_BASE + TBX_TENANT_PSP_BYTES,
                  "T1 every site relocated to link + block base + PSP bytes");
        }
        CHECK(plan.psp_addr == FAKE_BASE, "T1 PSP at the block base");
        CHECK(plan.load_base == FAKE_BASE + TBX_TENANT_PSP_BYTES,
              "T1 load base = block + 256");
        CHECK(plan.entry == FAKE_BASE + TBX_TENANT_PSP_BYTES + 8u,
              "T1 entry = load base + e_ip");
        CHECK(plan.module_len == mod_len, "T1 module length (header dropped)");
        CHECK(file[0] == 0x90u && file[1] == 0x90u,
              "T1 module moved down over the header to the load base");
        {
            int zero = 1;
            for (uint32_t i = mod_len; i < sizeof block - TBX_TENANT_PSP_BYTES; i++)
                if (file[i] != 0u) zero = 0;
            CHECK(zero, "T1 the BSS tail (and dropped header bytes) are zeroed");
        }
        CHECK(block[0] == 0xCDu && block[1] == 0x20u,
              "T1 psp_build ran at the block head (INT 20h at PSP:0)");
        CHECK(get16(block, 0x02) ==
                  (uint16_t)(((FAKE_BASE + (uint32_t)sizeof block) >> 4) & 0xFFFFu),
              "T1 alloc_end_seg = fake paragraph of the BLOCK end");
        CHECK(get16(block, 0x2C) == (uint16_t)((ENV_BLOCK >> 4) & 0xFFFFu),
              "T1 env_seg = ENV_BLOCK (inherit-empty, reconciliation S4)");
    }

    /* ---- T1b: header-resident reloc table (lfarlc inside the header gap). ---- */
    {
        uint8_t block[640];
        uint8_t *file = block + TBX_TENANT_PSP_BYTES;
        tenant_plan_t plan;
        const uint32_t hdr = 0x40u;                 /* 4 paragraphs */
        const uint32_t mod_len = 0x20u;
        memset(block, 0xEEu, sizeof block);
        build_hdr(file, 1u, 0x30u, 0u, 0u, MZ_INITECH_TAG, hdr + mod_len);
        put16(file, 0x08, (uint16_t)(hdr / 16u));
        memset(file + 0x20, 0, hdr - 0x20u);
        put32(file, 0x30u, 0x10u);                   /* the one reloc entry */
        memset(file + hdr, 0x90, mod_len);
        put32(file, hdr + 0x10u, 0x00000004u);
        CHECK(loader_prepare_tenant(block, (uint32_t)sizeof block,
                                    hdr + mod_len, FAKE_BASE, &plan) == LOADER_OK,
              "T1b header-resident reloc table prepares OK");
        CHECK(get32(file, 0x10u) == 0x4u + FAKE_BASE + TBX_TENANT_PSP_BYTES,
              "T1b header-resident site relocated BEFORE the move-down");
    }

    /* ---- T2: psp_build is address-independent. ---- */
    {
        uint8_t a[TBX_TENANT_PSP_BYTES], b[TBX_TENANT_PSP_BYTES];
        psp_params_t p;
        memset(&p, 0, sizeof p);
        p.alloc_end_linear = 0x00200000u;
        p.env_linear       = ENV_BLOCK;
        memset(a, 0x11, sizeof a);
        memset(b, 0x77, sizeof b);
        (void)psp_build((psp_t *)(void *)a, &p);
        (void)psp_build((psp_t *)(void *)b, &p);
        CHECK(memcmp(a, b, sizeof a) == 0,
              "T2 same params at two addresses -> byte-identical PSPs "
              "(no address-derived field; closes the Part A.4 VERIFY)");
        CHECK(get16(a, 0x02) == (uint16_t)((0x00200000u >> 4) & 0xFFFFu),
              "T2 alloc_end_seg is the params' fake paragraph (mod 2^16 above 1 MiB)");
        CHECK(get16(a, 0x16) == 0u, "T2 parent_psp 0 (kernel parent)");
        CHECK(a[0x18] == 0x00u && a[0x19] == 0x01u && a[0x1A] == 0x01u,
              "T2 JFT carries the CON standard handles");
    }

    /* ---- T3: refusals. ---- */
    {
        uint8_t block[512];
        uint8_t *file = block + TBX_TENANT_PSP_BYTES;
        tenant_plan_t plan;

        memset(block, 0, sizeof block);
        build_hdr(file, 0u, HDR_SIZE, 0u, 0u, 0u /* untagged */, HDR_SIZE + 16u);
        CHECK(loader_prepare_tenant(block, (uint32_t)sizeof block, HDR_SIZE + 16u,
                                    FAKE_BASE, &plan) == LOADER_ERR_FOREIGN_MZ,
              "T3 an untagged 16-bit MZ is FOREIGN (the kernel panics)");

        memset(block, 0, sizeof block);
        file[0] = 0xB4u; file[1] = 0x4Cu;              /* mov ah,4Ch -- a .COM */
        CHECK(loader_prepare_tenant(block, (uint32_t)sizeof block, 16u,
                                    FAKE_BASE, &plan) == LOADER_ERR_BAD_FORMAT,
              "T3 a flat .COM is refused (V1 tenants are InitechMZ only)");

        memset(block, 0, sizeof block);
        build_hdr(file, 0u, HDR_SIZE, 64u /* 1 KiB BSS */, 0u, MZ_INITECH_TAG,
                  HDR_SIZE + 16u);
        CHECK(loader_prepare_tenant(block, (uint32_t)sizeof block, HDR_SIZE + 16u,
                                    FAKE_BASE, &plan) == LOADER_ERR_BAD_FORMAT,
              "T3 an e_minalloc the block cannot hold is refused, never overrun");

        memset(block, 0, sizeof block);
        build_hdr(file, 1u, (uint16_t)(HDR_SIZE + 16u), 0u, 0u, MZ_INITECH_TAG,
                  HDR_SIZE + 16u);
        put32(file, HDR_SIZE + 16u, 14u);              /* site 14: 14+4 > 16 */
        CHECK(loader_prepare_tenant(block, (uint32_t)sizeof block, HDR_SIZE + 20u,
                                    FAKE_BASE, &plan) == LOADER_ERR_BAD_FORMAT,
              "T3 an out-of-module reloc site is refused");
    }

    /* ---- T4: the EXEC path still relocates against PROGRAM_IMAGE. ---- */
    {
        uint8_t img[256];
        loader_plan_t plan;
        memset(img, 0, sizeof img);
        build_hdr(img, 1u, (uint16_t)(HDR_SIZE + 16u), 0u, 0u, MZ_INITECH_TAG,
                  HDR_SIZE + 16u);
        put32(img, HDR_SIZE + 4u, 0x8u);
        put32(img, HDR_SIZE + 16u, 4u);
        CHECK(loader_prepare_mz(img, HDR_SIZE + 20u, (const char *)0, 0u,
                                &plan) == LOADER_OK,
              "T4 EXEC-path MZ prepares OK");
        CHECK(get32(img, 4u) == 0x8u + (uint32_t)PROGRAM_IMAGE,
              "T4 EXEC path relocates against PROGRAM_IMAGE (DEC-08a unchanged)");
    }

    return TEST_SUMMARY("test_tenant_load");
}
