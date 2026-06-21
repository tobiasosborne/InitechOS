/* test_mzload.c -- host unit oracle for the InitechMZ (.EXE) loader integration
 *                  (loader_prepare_mz, the DEC-08a content path in loader.c).
 *
 * beads: initech-wczy (dtw.2 -- loader.c .COM/MZ dispatch + reloc apply + the
 *        fail-loud foreign-MZ tag gate). The SIBLING oracle of test_loader.c;
 *        kept in its own TU (disjoint lane files) so dtw.2 never edits the
 *        already-green test_loader.c / test_exec.c.
 *
 * Ref:   docs/adr/ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md
 *          DEC-08a.1 (flat-32 reloc: add load_base to a 32-bit dword);
 *          DEC-08a.2 (load module at PROGRAM_IMAGE, header skipped; flat entry);
 *          DEC-08a.3 (e_minalloc teeth -> DOS 08h insufficient memory);
 *          DEC-08a.4 (dispatch by content 'MZ'/'ZM');
 *          DEC-08a.5 (untagged 16-bit MZ -> fail loud, never run).
 *        os/milton/loader.c (loader_prepare_mz -- the PURE prologue the kernel's
 *          load_program_mz_in_place wraps); os/milton/mz.c/mz.h (the parse/reloc
 *          unit, beads dtw.1); spec/memory_map.h (PROGRAM_IMAGE / PROGRAM_IMAGE_MAX
 *          / PROGRAM_ARENA_CEIL). CLAUDE.md Law 2 (oracle is truth), Rule 1
 *          (RED->GREEN), Rule 2 (fail loud), Rule 6 (mutation-proven), Rule 12
 *          (ASCII).
 *
 * STRATEGY (Law 2): loader_prepare_mz is the PURE, host-testable seam -- it
 * parses the in-place MZ, moves the load module down over the header, applies the
 * flat relocations IN PLACE, and computes the loader_plan_t. The kernel's
 * load_program_mz_in_place is a thin wrapper that PANICS on the foreign-MZ status
 * and then runs loader_run_plan (the asm transfer, elided hosted). So driving
 * loader_prepare_mz here proves the artifact's whole MZ content path WITHOUT the
 * kernel-only jump (mirrors how test_loader drives loader_prepare).
 *
 * The relocation adds the FLAT LOAD BASE -- the constant PROGRAM_IMAGE (0x40100),
 * the address the code will actually run at -- NOT this host staging buffer's
 * address. So the oracle can build the image anywhere in host memory and still
 * assert the dword == original + PROGRAM_IMAGE byte-exactly.
 *
 * FOREIGN-MZ FAIL-LOUD: the kernel PANICS (cli;hlt) on a foreign MZ, which a host
 * process cannot observe without crashing. The pure loader_prepare_mz instead
 * RETURNS the distinct LOADER_ERR_FOREIGN_MZ (the panic lives at the kernel call
 * site, load_program_mz_in_place). The oracle asserts that status -- proving the
 * loader REFUSES to produce a runnable plan for a foreign MZ (it never "runs").
 *
 * MUTATION proof (Rule 6) -- driven by make test-mzload-mutant:
 *   -DLOADER_MUTATE_MZ_NO_RELOC : loader_prepare_mz SKIPS mz_apply_relocs, so the
 *                                 relocated-dword assertion (== original +
 *                                 PROGRAM_IMAGE) goes RED. A mutant that PASSES
 *                                 means the reloc oracle is decoration.
 *
 * Clean build MUST exit 0; the mutant build MUST exit non-zero.
 * ALL source strictly ASCII (Rule 12).
 */

#include <stdint.h>
#include <string.h>   /* memset / memcmp -- libc OK in a host test (Law 3) */

#include "loader.h"
#include "memory_map.h"
#include "mz.h"
#include "test_assert.h"

TEST_HARNESS();

/* ---- InitechMZ fixture builder ------------------------------------------- *
 * Layout of the hand-authored image (all in one staging buffer):
 *   [0x00 .. 0x20)            32-byte MZ header (e_cparhdr = 2 paragraphs)
 *   [0x20 .. 0x20+rc*4)       reloc table (rc * uint32 flat offsets)
 *   [load_module_off .. )     the load module (header_bytes = 0x20)
 * We place the reloc table immediately AFTER the header and BEFORE the load
 * module, so it lives in the region the down-move drops (it is read from its
 * original file offset, untouched by the move of the load module).
 * ------------------------------------------------------------------------- */

#define HDR_SIZE      0x20u   /* 2 paragraphs (e_cparhdr=2) -- canonical fixture */

static void put16(uint8_t *b, uint32_t off, uint16_t v)
{
    b[off + 0] = (uint8_t)(v & 0xFFu);
    b[off + 1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *b, uint32_t off, uint32_t v)
{
    b[off + 0] = (uint8_t)(v         & 0xFFu);
    b[off + 1] = (uint8_t)((v >>  8) & 0xFFu);
    b[off + 2] = (uint8_t)((v >> 16) & 0xFFu);
    b[off + 3] = (uint8_t)((v >> 24) & 0xFFu);
}

static uint32_t get32(const uint8_t *b, uint32_t off)
{
    return (uint32_t)b[off + 0]
         | ((uint32_t)b[off + 1] <<  8)
         | ((uint32_t)b[off + 2] << 16)
         | ((uint32_t)b[off + 3] << 24);
}

/* Write a minimal InitechMZ header at b[0]. e_cparhdr is fixed at 2 (HDR_SIZE).
 * The reloc table is at e_lfarlc; the load module follows it. image_bytes
 * (e_cp/e_cblp) is set so load_module_len == mod_len. */
static void build_hdr(uint8_t *b,
                      uint16_t crlc, uint16_t lfarlc,
                      uint16_t minalloc, uint16_t maxalloc,
                      uint16_t ss, uint16_t sp, uint16_t cs, uint16_t ip,
                      uint16_t tag, uint32_t image_bytes)
{
    uint16_t e_cp, e_cblp;
    memset(b, 0, HDR_SIZE);
    b[0x00] = 0x4Du; b[0x01] = 0x5Au;             /* 'MZ' */
    /* image_bytes -> (e_cp, e_cblp): full pages model. If image_bytes is a
     * multiple of 512, e_cblp = 0 and e_cp = image_bytes/512; else last page is
     * partial. We always round the buffer so the simple model holds. */
    if ((image_bytes % 512u) == 0u) {
        e_cp   = (uint16_t)(image_bytes / 512u);
        e_cblp = 0u;
    } else {
        e_cp   = (uint16_t)(image_bytes / 512u + 1u);
        e_cblp = (uint16_t)(image_bytes % 512u);
    }
    put16(b, 0x02, e_cblp);                        /* e_cblp */
    put16(b, 0x04, e_cp);                          /* e_cp   */
    put16(b, 0x06, crlc);                          /* e_crlc */
    put16(b, 0x08, HDR_SIZE / 16u);                /* e_cparhdr = 2 paragraphs */
    put16(b, 0x0A, minalloc);                      /* e_minalloc */
    put16(b, 0x0C, maxalloc);                      /* e_maxalloc */
    put16(b, 0x0E, ss);                            /* e_ss */
    put16(b, 0x10, sp);                            /* e_sp */
    put16(b, 0x14, ip);                            /* e_ip */
    put16(b, 0x16, cs);                            /* e_cs */
    put16(b, 0x18, lfarlc);                        /* e_lfarlc */
    put16(b, 0x1C, tag);                           /* e_res[0] -- InitechMZ tag */
}

int main(void)
{
    /* ---- Case 1: a VALID InitechMZ loads, relocates, lays out (the core). -- *
     * Build an image with ONE reloc site. The load module has a known dword at
     * reloc offset 8 whose link-time value is K. After loader_prepare_mz the
     * load module sits at buffer[0..], and buffer[8] dword == K + PROGRAM_IMAGE
     * (DEC-08a.1). entry_off = 0 (cs=ip=0) -> plan.entry == PROGRAM_IMAGE. ------ */
    {
        uint8_t img[256];
        const uint32_t mod_len    = 0x40u;       /* 64-byte load module */
        const uint32_t reloc_off  = HDR_SIZE;    /* reloc table right after header */
        const uint32_t reloc_site = 8u;          /* flat offset within the module */
        const uint32_t K          = 0x00001234u; /* the link-time dword value */
        const uint32_t image_bytes = HDR_SIZE + 4u /*one reloc*/ + mod_len;
        /* NOTE on layout: load_module_off = header_bytes = HDR_SIZE. The reloc
         * table occupies [HDR_SIZE, HDR_SIZE+4); the load module then begins at
         * load_module_off = HDR_SIZE too -- so the reloc table and the module
         * START at the same file offset?  No: load_module_off is header_bytes
         * (HDR_SIZE). To keep the reloc table OUT of the moved module window we
         * put e_lfarlc AFTER the module instead (trailer reloc table). image_bytes
         * counts header + module; the reloc table lives past image_bytes (DOS
         * permits the reloc table anywhere e_lfarlc points). */
        (void)image_bytes;

        const uint32_t hdr_bytes   = HDR_SIZE;
        const uint32_t img_bytes    = hdr_bytes + mod_len;     /* header + module */
        const uint32_t reloc_table  = img_bytes;               /* trailer reloc */
        const uint32_t total        = reloc_table + 4u;

        memset(img, 0, sizeof(img));
        build_hdr(img, /*crlc=*/1u, /*lfarlc=*/(uint16_t)reloc_table,
                  /*minalloc=*/0u, /*maxalloc=*/0xFFFFu,
                  /*ss=*/0u, /*sp=*/0u, /*cs=*/0u, /*ip=*/0u,
                  MZ_INITECH_TAG, img_bytes);
        /* The load module bytes start at hdr_bytes; the reloc dword K sits at
         * module offset reloc_site (file offset hdr_bytes + reloc_site). */
        put32(img, hdr_bytes + reloc_site, K);
        /* The reloc table: one entry naming flat offset reloc_site in the module. */
        put32(img, reloc_table, reloc_site);
        (void)reloc_off;

        loader_plan_t plan;
        memset(&plan, 0xAA, sizeof(plan));
        loader_status_t st = loader_prepare_mz(img, total, (const char *)0, 0u,
                                               &plan);
        CHECK(st == LOADER_OK, "valid InitechMZ must prepare OK");

        /* (a) The load module moved DOWN over the header to buffer[0]. The first
         *     module byte was at hdr_bytes; after the move it is at img[0]. The
         *     reloc-site dword landed at img[reloc_site]. */
        CHECK(get32(img, reloc_site) == K + (uint32_t)PROGRAM_IMAGE,
              "DEC-08a.1: reloc'd dword == original + PROGRAM_IMAGE (flat base add)");

        /* (b) Layout: image at PROGRAM_IMAGE, entry at PROGRAM_IMAGE + entry_off
         *     (entry_off=0 here), stack at PROGRAM_STACK_TOP, src NULL (in place). */
        CHECK(plan.image_dst == (uint32_t)PROGRAM_IMAGE,
              "DEC-08a.2: load module lands at PROGRAM_IMAGE");
        CHECK(plan.entry == (uint32_t)PROGRAM_IMAGE,
              "DEC-08a.2: entry == PROGRAM_IMAGE + entry_off (cs=ip=0)");
        CHECK(plan.stack_top == (uint32_t)PROGRAM_STACK_TOP,
              "ESP stays PROGRAM_STACK_TOP for the current release (DEC-08a.2)");
        CHECK(plan.image_src == (const uint8_t *)0,
              "in-place: image_src must be NULL (loader_run_plan must NOT copy)");
        CHECK(plan.image_len == mod_len,
              "plan.image_len is the LOAD MODULE length (header dropped)");

        /* (c) Arena honored: present + disjoint above the module + below ceiling. */
        CHECK(plan.arena_present == 1u, "a small MZ must leave a heap arena");
        CHECK(plan.arena_base >= (uint32_t)PROGRAM_IMAGE + mod_len,
              "arena base sits ABOVE the loaded module (disjointness)");
        CHECK(plan.arena_ceil == (uint32_t)PROGRAM_ARENA_CEIL,
              "arena ceiling == PROGRAM_ARENA_CEIL (== ENV_BLOCK)");
    }

    /* ---- Case 1b: a HEADER-RESIDENT reloc table (the classic DOS layout). - *
     * A real MZ commonly puts the reloc table in the gap BETWEEN the standard
     * 0x1C header end and header_bytes (e_cparhdr*16) -- i.e. reloc_table_off <
     * load_module_off. Here that means the reloc table sits at file offset 0x10
     * (inside the 0x20 header), and the load module begins at load_module_off =
     * 0x20. After the down-move the module lands at [0, mod_len) -- which OVERLAPS
     * the original reloc-table offset 0x10 if mod_len > 0x10. The loader must
     * relocate BEFORE the move (it reads the reloc table while it is still intact);
     * a relocate-AFTER-move ordering would read a clobbered fixup. This case asserts
     * the relocated dword is correct, proving the ordering. It is the regression
     * that a placement-specific "guard and reject" band-aid would have failed. --- */
    {
        uint8_t img[256];
        const uint32_t mod_len     = 0x40u;       /* 64-byte module (> 0x10) */
        const uint32_t hdr_bytes   = HDR_SIZE;    /* load_module_off = 0x20 */
        const uint32_t reloc_table = 0x10u;       /* INSIDE the header (< 0x20) */
        const uint32_t reloc_site  = 4u;          /* module-relative reloc offset */
        const uint32_t K           = 0x0000ABCDu;
        const uint32_t img_bytes   = hdr_bytes + mod_len;
        const uint32_t total       = img_bytes;   /* header(incl reloc tbl) + module */

        memset(img, 0, sizeof(img));
        build_hdr(img, /*crlc=*/1u, /*lfarlc=*/(uint16_t)reloc_table,
                  0u, 0xFFFFu, 0u, 0u, 0u, 0u, MZ_INITECH_TAG, img_bytes);
        /* reloc table entry (at 0x10): names module offset reloc_site. */
        put32(img, reloc_table, reloc_site);
        /* the module's reloc-site dword (file offset hdr_bytes + reloc_site). */
        put32(img, hdr_bytes + reloc_site, K);

        loader_plan_t plan;
        memset(&plan, 0xAA, sizeof(plan));
        loader_status_t st = loader_prepare_mz(img, total, (const char *)0, 0u,
                                               &plan);
        CHECK(st == LOADER_OK, "header-resident reloc table InitechMZ must prepare OK");
        CHECK(get32(img, reloc_site) == K + (uint32_t)PROGRAM_IMAGE,
              "relocate-BEFORE-move: header-resident reloc table still relocates correctly");
        CHECK(plan.entry == (uint32_t)PROGRAM_IMAGE,
              "header-resident-reloc entry == PROGRAM_IMAGE (cs=ip=0)");
    }

    /* ---- Case 2: a non-zero entry_off is honored (DEC-08a.2). ------------- *
     * cs=0, ip=0x10 -> entry_off = 0x10 -> plan.entry == PROGRAM_IMAGE+0x10. -- */
    {
        uint8_t img[256];
        const uint32_t mod_len = 0x40u;
        const uint32_t hdr_bytes = HDR_SIZE;
        const uint32_t img_bytes = hdr_bytes + mod_len;
        const uint32_t reloc_table = img_bytes;
        const uint32_t total = reloc_table; /* crlc=0 -> no reloc table bytes */

        memset(img, 0, sizeof(img));
        build_hdr(img, /*crlc=*/0u, /*lfarlc=*/(uint16_t)reloc_table,
                  0u, 0xFFFFu, 0u, 0u, /*cs=*/0u, /*ip=*/0x10u,
                  MZ_INITECH_TAG, img_bytes);

        loader_plan_t plan;
        memset(&plan, 0xAA, sizeof(plan));
        loader_status_t st = loader_prepare_mz(img, total, (const char *)0, 0u,
                                               &plan);
        CHECK(st == LOADER_OK, "degenerate (crlc=0) InitechMZ must prepare OK");
        CHECK(plan.entry == (uint32_t)PROGRAM_IMAGE + 0x10u,
              "DEC-08a.2: entry == PROGRAM_IMAGE + entry_off (ip=0x10)");
    }

    /* ---- Case 3: a FOREIGN (untagged) MZ -> fail loud, never runs. -------- *
     * Same valid structure but e_res[0] == 0 (a genuine 16-bit DOS .EXE). The
     * loader MUST return LOADER_ERR_FOREIGN_MZ (the kernel call site panics);
     * it must NOT produce a runnable plan (DEC-08a.5). --------------------- */
    {
        uint8_t img[256];
        const uint32_t mod_len = 0x40u;
        const uint32_t hdr_bytes = HDR_SIZE;
        const uint32_t img_bytes = hdr_bytes + mod_len;
        const uint32_t total = img_bytes;

        memset(img, 0, sizeof(img));
        build_hdr(img, /*crlc=*/0u, /*lfarlc=*/(uint16_t)img_bytes,
                  0u, 0xFFFFu, 0u, 0u, 0u, 0u,
                  /*tag=*/0x0000u,            /* UNTAGGED -- a foreign 16-bit MZ */
                  img_bytes);

        loader_plan_t plan;
        loader_status_t st = loader_prepare_mz(img, total, (const char *)0, 0u,
                                               &plan);
        CHECK(st == LOADER_ERR_FOREIGN_MZ,
              "DEC-08a.5: an untagged 16-bit MZ must fail loud (LOADER_ERR_FOREIGN_MZ)");
    }

    /* ---- Case 4: e_minalloc too large for the arena -> fail loud. --------- *
     * A huge e_minalloc (more paragraphs than the disjoint arena can hold)
     * maps to DOS 08h insufficient memory (DEC-08a.3): the loader must refuse
     * (LOADER_ERR_BAD_FORMAT), never run heap-starved. The whole arena is at
     * most PROGRAM_ARENA_CEIL - arena_base bytes; e_minalloc = 0xFFFF paragraphs
     * = ~1 MiB, far above the ~188 KiB region, so it cannot be satisfied. ----- */
    {
        uint8_t img[256];
        const uint32_t mod_len = 0x40u;
        const uint32_t hdr_bytes = HDR_SIZE;
        const uint32_t img_bytes = hdr_bytes + mod_len;
        const uint32_t total = img_bytes;

        memset(img, 0, sizeof(img));
        build_hdr(img, /*crlc=*/0u, /*lfarlc=*/(uint16_t)img_bytes,
                  /*minalloc=*/0xFFFFu,        /* ~1 MiB demanded -- impossible */
                  0xFFFFu, 0u, 0u, 0u, 0u,
                  MZ_INITECH_TAG, img_bytes);

        loader_plan_t plan;
        loader_status_t st = loader_prepare_mz(img, total, (const char *)0, 0u,
                                               &plan);
        CHECK(st == LOADER_ERR_BAD_FORMAT,
              "DEC-08a.3: e_minalloc beyond the arena must fail loud (08h insufficient)");
    }

    /* ---- Case 5: a load module too big for PROGRAM_IMAGE_MAX -> fail loud. - *
     * We cannot allocate a real PROGRAM_IMAGE_MAX+ buffer just to test the
     * bound, but loader_prepare_mz checks load_module_len BEFORE touching the
     * module body in the down-move... no: the move reads the module bytes. So we
     * craft a header that DECLARES image_bytes huge (load_module_len >
     * PROGRAM_IMAGE_MAX) while the file is short -- mz_parse_header then rejects
     * it as TRUNCATED (file_len < image_bytes), which loader_prepare_mz maps to
     * LOADER_ERR_BAD_FORMAT. Either way the over-large declaration fails loud and
     * never runs (Rule 2). ------------------------------------------------- */
    {
        uint8_t img[256];
        memset(img, 0, sizeof(img));
        /* Declare image_bytes far over PROGRAM_IMAGE_MAX while file is 256 B. */
        build_hdr(img, /*crlc=*/0u, /*lfarlc=*/0x40u,
                  0u, 0xFFFFu, 0u, 0u, 0u, 0u,
                  MZ_INITECH_TAG,
                  /*image_bytes=*/(uint32_t)PROGRAM_IMAGE_MAX + 0x1000u);

        loader_plan_t plan;
        loader_status_t st = loader_prepare_mz(img, (uint32_t)sizeof(img),
                                               (const char *)0, 0u, &plan);
        CHECK(st == LOADER_ERR_BAD_FORMAT,
              "an over-large MZ image must fail loud (LOADER_ERR_BAD_FORMAT)");
    }

    /* ---- Case 6: the .COM path is UNTOUCHED (byte-identical layout). ------ *
     * A non-MZ flat image must dispatch through the EXISTING loader_prepare path
     * (DEC-08, unchanged). We assert mz_is_mz says NO for a flat .COM, and that
     * loader_prepare yields the locked .COM layout -- i.e. the MZ work did not
     * perturb the default path (the kernel's load_program_from_fat routes a
     * non-MZ image to load_program_in_place exactly as before dtw.2). ------- */
    {
        const uint8_t com[] = { 0xB4, 0x09, 0xCD, 0x21, 0xB4, 0x4C, 0xCD, 0x21 };
        CHECK(mz_is_mz(com, (uint32_t)sizeof(com)) == 0,
              "DEC-08a.4: a flat .COM is NOT dispatched as MZ (content probe says no)");

        loader_plan_t plan;
        memset(&plan, 0xAA, sizeof(plan));
        loader_status_t st = loader_prepare(com, (uint32_t)sizeof(com),
                                            (const char *)0, 0u, &plan);
        CHECK(st == LOADER_OK, ".COM path must prepare OK (unchanged)");
        CHECK(plan.image_dst == (uint32_t)PROGRAM_IMAGE,
              ".COM image at PROGRAM_IMAGE (byte-identical .COM layout)");
        CHECK(plan.entry == (uint32_t)PROGRAM_IMAGE,
              ".COM entry == PROGRAM_IMAGE (flat .COM entry, unchanged)");
        CHECK(plan.image_src == com,
              ".COM image_src points at the caller's bytes (baked .COM copy path)");
    }

    return TEST_SUMMARY("test_mzload");
}
