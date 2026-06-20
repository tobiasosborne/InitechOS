/* test_mz.c -- host unit oracle for the InitechMZ header-parse +
 *              flat-relocation-apply module (mz.c / mz.h).
 *
 * beads: initech-dtw.1
 * Ref:   docs/adr/ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md
 *          DEC-08a.1 (flat-32 reloc: add load_base to 32-bit dword).
 *          DEC-08a.4 (dispatch by content: 'MZ'/'ZM').
 *          DEC-08a.5 (tag word at e_res[0]: untagged -> MZ_ERR_FOREIGN).
 *        CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6
 *        (mutation-proven), Rule 12 (ASCII).
 *
 * Compiles HOSTED by #including mz.c directly (same TU trick as test_env.c /
 * test_command.c).  All mz_* functions are pure + I/O-free; no kernel defines
 * required.
 *
 * MUTATION proof (Rule 6) -- driven by make test-mz-mutant:
 *   -DMZ_MUTATE_RELOC_NOADD     : mz_apply_relocs skips the load_base add;
 *                                  the reloc-apply assertions go RED.
 *   -DMZ_MUTATE_RELOC_PARAGRAPH : mz_apply_relocs adds (load_base >> 4);
 *                                  the reloc-apply assertions go RED.
 *   -DMZ_MUTATE_ACCEPT_FOREIGN  : mz_parse_header accepts an untagged MZ;
 *                                  the MZ_ERR_FOREIGN assertion goes RED.
 *
 * Clean build MUST exit 0 ("n checks, 0 failures").
 * Each mutant build MUST exit non-zero (the oracle bites).
 *
 * ALL source strictly ASCII (Rule 12).
 */

#include <stdint.h>
#include <string.h>   /* memcmp, memset -- libc OK in host test (Law 3) */
#include <stdio.h>

#include "mz.h"
#include "test_assert.h"

/* Pull in the artifact source directly (same-TU trick).  The mutation hooks
 * propagate automatically from the gcc -D flag on the command line. */
#include "mz.c"

TEST_HARNESS();

/* ---- MZ fixture builder helpers ------------------------------------------ */

/* Minimum MZ header size (through e_res[0] at 0x1C, plus one byte at 0x1D).
 * We use 0x20 (32 bytes) as our canonical header so the reloc table can live
 * right after the header. */
#define FIXTURE_HDR_SIZE  0x20u

/* Build a minimal InitechMZ header into `buf` (must be >= FIXTURE_HDR_SIZE
 * bytes).  Sets:
 *   e_magic   = 'MZ' (0x4D, 0x5A)
 *   e_cblp    = cblp   (bytes on last page; 0 = full page)
 *   e_cp      = cp     (total 512-byte pages)
 *   e_crlc    = crlc   (reloc count)
 *   e_cparhdr = 2      (2 paragraphs = 32 bytes = FIXTURE_HDR_SIZE)
 *   e_minalloc= minall
 *   e_maxalloc= maxall (typically 0xFFFF)
 *   e_ss      = ss, e_sp = sp
 *   e_ip      = ip, e_cs = cs
 *   e_lfarlc  = FIXTURE_HDR_SIZE (reloc table immediately after header)
 *   e_res[0]  = tag    (MZ_INITECH_TAG for valid, 0 for foreign test)
 */
static void build_hdr(uint8_t *buf, uint32_t buf_len,
                      uint16_t cblp, uint16_t cp, uint16_t crlc,
                      uint16_t minall, uint16_t maxall,
                      uint16_t ss, uint16_t sp,
                      uint16_t cs, uint16_t ip,
                      uint16_t tag)
{
    uint32_t i;
    if (buf_len < FIXTURE_HDR_SIZE) { return; }
    for (i = 0; i < FIXTURE_HDR_SIZE; i++) { buf[i] = 0; }

    /* e_magic = 'MZ' */
    buf[0x00] = 0x4Du; buf[0x01] = 0x5Au;
    /* e_cblp */
    buf[0x02] = (uint8_t)(cblp & 0xFFu); buf[0x03] = (uint8_t)(cblp >> 8);
    /* e_cp */
    buf[0x04] = (uint8_t)(cp   & 0xFFu); buf[0x05] = (uint8_t)(cp   >> 8);
    /* e_crlc */
    buf[0x06] = (uint8_t)(crlc & 0xFFu); buf[0x07] = (uint8_t)(crlc >> 8);
    /* e_cparhdr = 2 (32 bytes) */
    buf[0x08] = 0x02u; buf[0x09] = 0x00u;
    /* e_minalloc */
    buf[0x0A] = (uint8_t)(minall & 0xFFu); buf[0x0B] = (uint8_t)(minall >> 8);
    /* e_maxalloc */
    buf[0x0C] = (uint8_t)(maxall & 0xFFu); buf[0x0D] = (uint8_t)(maxall >> 8);
    /* e_ss */
    buf[0x0E] = (uint8_t)(ss & 0xFFu); buf[0x0F] = (uint8_t)(ss >> 8);
    /* e_sp */
    buf[0x10] = (uint8_t)(sp & 0xFFu); buf[0x11] = (uint8_t)(sp >> 8);
    /* e_csum = 0 */
    buf[0x12] = 0x00u; buf[0x13] = 0x00u;
    /* e_ip */
    buf[0x14] = (uint8_t)(ip & 0xFFu); buf[0x15] = (uint8_t)(ip >> 8);
    /* e_cs */
    buf[0x16] = (uint8_t)(cs & 0xFFu); buf[0x17] = (uint8_t)(cs >> 8);
    /* e_lfarlc = FIXTURE_HDR_SIZE (reloc table right after header) */
    buf[0x18] = (uint8_t)(FIXTURE_HDR_SIZE & 0xFFu);
    buf[0x19] = (uint8_t)(FIXTURE_HDR_SIZE >> 8);
    /* e_ovno = 0 */
    buf[0x1A] = 0x00u; buf[0x1B] = 0x00u;
    /* e_res[0] = tag */
    buf[0x1C] = (uint8_t)(tag & 0xFFu); buf[0x1D] = (uint8_t)(tag >> 8);
    /* bytes 0x1E..0x1F already zero */
}

/* Write a 16-bit LE word to buf at offset off. */
static void put16(uint8_t *buf, uint32_t off, uint16_t v)
{
    buf[off + 0] = (uint8_t)(v & 0xFFu);
    buf[off + 1] = (uint8_t)(v >> 8);
}

/* Write a 32-bit LE dword to buf at offset off. */
static void put32(uint8_t *buf, uint32_t off, uint32_t v)
{
    buf[off + 0] = (uint8_t)(v        & 0xFFu);
    buf[off + 1] = (uint8_t)((v >>  8) & 0xFFu);
    buf[off + 2] = (uint8_t)((v >> 16) & 0xFFu);
    buf[off + 3] = (uint8_t)((v >> 24) & 0xFFu);
}

/* Read a 32-bit LE dword from buf at offset off. */
static uint32_t get32(const uint8_t *buf, uint32_t off)
{
    return (uint32_t)buf[off + 0]
         | ((uint32_t)buf[off + 1] <<  8)
         | ((uint32_t)buf[off + 2] << 16)
         | ((uint32_t)buf[off + 3] << 24);
}

/* ---- test cases ---------------------------------------------------------- */

/* mz_is_mz: probe the dispatch bytes (DEC-08a.4). */
static void test_is_mz(void)
{
    /* 'MZ' canonical */
    {
        uint8_t b[4] = { 0x4Du, 0x5Au, 0x00u, 0x00u };
        CHECK(mz_is_mz(b, 4u) == 1,  "is_mz: 'MZ' bytes -> 1");
    }
    /* 'ZM' byte-swap alias */
    {
        uint8_t b[4] = { 0x5Au, 0x4Du, 0x00u, 0x00u };
        CHECK(mz_is_mz(b, 4u) == 1,  "is_mz: 'ZM' alias -> 1");
    }
    /* plain bytes that are neither */
    {
        uint8_t b[4] = { 0x00u, 0x00u, 0x00u, 0x00u };
        CHECK(mz_is_mz(b, 4u) == 0,  "is_mz: zero bytes -> 0");
    }
    {
        uint8_t b[4] = { 0x4Du, 0x00u, 0x00u, 0x00u };
        CHECK(mz_is_mz(b, 4u) == 0,  "is_mz: 'M' only -> 0");
    }
    {
        uint8_t b[4] = { 0xEBu, 0x3Cu, 0x00u, 0x00u };
        CHECK(mz_is_mz(b, 4u) == 0,  "is_mz: COM jmp stub -> 0");
    }
    /* too short */
    {
        uint8_t b[2] = { 0x4Du, 0x5Au };
        CHECK(mz_is_mz(b, 1u) == 0,  "is_mz: len==1 -> 0 (too short)");
    }
    /* NULL pointer */
    {
        CHECK(mz_is_mz((const uint8_t *)0, 4u) == 0,
              "is_mz: NULL file -> 0");
    }
}

/* mz_parse_header: correct parse for e_cblp != 0 (partial last page).
 *
 * Fixture:
 *   e_cp      = 2  (2 pages)
 *   e_cblp    = 64 (last page has 64 bytes)
 *   => image_bytes = (2-1)*512 + 64 = 576
 *   e_cparhdr = 2  (2*16 = 32 bytes header)
 *   => load_module_off = 32
 *   => load_module_len = 576 - 32 = 544
 *   e_cs = 0, e_ip = 0  => entry_off = 0
 *   e_ss = 0, e_sp = 0  => stack_off = 0
 *   e_crlc = 0          => reloc_count = 0
 *   tag = MZ_INITECH_TAG
 *
 * File layout: header (32) + load_module (544) = 576 bytes total. */
static void test_parse_partial_last_page(void)
{
    /* Total file = 576 bytes; image_bytes = 576, header = 32. */
    uint8_t file[576];
    uint32_t file_len = 576u;
    mz_image_t out;
    int r;

    memset(file, 0xCCu, sizeof(file));
    build_hdr(file, FIXTURE_HDR_SIZE,
              /* cblp=64, cp=2, crlc=0, minall=0x10, maxall=0xFFFF */
              64u, 2u, 0u, 0x10u, 0xFFFFu,
              /* ss=0, sp=0, cs=0, ip=0, tag */
              0u, 0u, 0u, 0u, MZ_INITECH_TAG);

    r = mz_parse_header(file, file_len, &out);
    CHECK(r == MZ_OK,                    "parse_partial: returns MZ_OK");
    CHECK(out.load_module_off == 32u,    "parse_partial: load_module_off == 32");
    CHECK(out.load_module_len == 544u,   "parse_partial: load_module_len == 544");
    CHECK(out.reloc_table_off == FIXTURE_HDR_SIZE,
                                         "parse_partial: reloc_table_off == hdr size");
    CHECK(out.reloc_count == 0u,         "parse_partial: reloc_count == 0");
    CHECK(out.entry_off == 0u,           "parse_partial: entry_off == 0");
    CHECK(out.stack_off == 0u,           "parse_partial: stack_off == 0");
    CHECK(out.min_alloc_paras == 0x10u,  "parse_partial: min_alloc_paras == 0x10");
    CHECK(out.max_alloc_paras == 0xFFFFu,"parse_partial: max_alloc_paras == 0xFFFF");
    CHECK(out.is_initechmz == 1,         "parse_partial: is_initechmz == 1");
}

/* mz_parse_header: correct parse for e_cblp == 0 (all full pages).
 *
 * Fixture:
 *   e_cp   = 1, e_cblp = 0  => image_bytes = 1 * 512 = 512
 *   header = 32 bytes, load_module_len = 480
 *   e_cs = 0, e_ip = 8    => entry_off = 8
 *   e_ss = 0, e_sp = 0x100 => stack_off = 0x100
 *   e_crlc = 0 */
static void test_parse_full_last_page(void)
{
    uint8_t file[512];
    uint32_t file_len = 512u;
    mz_image_t out;
    int r;

    memset(file, 0xCCu, sizeof(file));
    build_hdr(file, FIXTURE_HDR_SIZE,
              0u, 1u, 0u, 0u, 0xFFFFu,
              0u, 0x100u, 0u, 8u, MZ_INITECH_TAG);

    r = mz_parse_header(file, file_len, &out);
    CHECK(r == MZ_OK,                    "parse_full: returns MZ_OK");
    CHECK(out.load_module_off == 32u,    "parse_full: load_module_off == 32");
    CHECK(out.load_module_len == 480u,   "parse_full: load_module_len == 480");
    CHECK(out.entry_off == 8u,           "parse_full: entry_off == e_ip");
    CHECK(out.stack_off == 0x100u,       "parse_full: stack_off == e_sp");
    CHECK(out.reloc_count == 0u,         "parse_full: reloc_count == 0");
    CHECK(out.is_initechmz == 1,         "parse_full: is_initechmz == 1");
}

/* mz_parse_header: nonzero e_cs and e_ss shift entry_off and stack_off.
 *
 * e_cs=2 (=> +0x20), e_ip=4 => entry_off = 0x24
 * e_ss=1 (=> +0x10), e_sp=8 => stack_off = 0x18
 * image: 1 full page (512 bytes), header 32, load_module_len 480 */
static void test_parse_cs_ss_nonzero(void)
{
    uint8_t file[512];
    mz_image_t out;
    int r;

    memset(file, 0x00u, sizeof(file));
    build_hdr(file, FIXTURE_HDR_SIZE,
              0u, 1u, 0u, 0u, 0xFFFFu,
              /* ss=1, sp=8, cs=2, ip=4 */
              1u, 8u, 2u, 4u, MZ_INITECH_TAG);

    r = mz_parse_header(file, 512u, &out);
    CHECK(r == MZ_OK,               "parse_cs_ss: MZ_OK");
    CHECK(out.entry_off == 0x24u,   "parse_cs_ss: entry_off == cs*16+ip == 0x24");
    CHECK(out.stack_off == 0x18u,   "parse_cs_ss: stack_off == ss*16+sp == 0x18");
}

/* mz_parse_header: foreign MZ (tag word == 0) -> MZ_ERR_FOREIGN.
 * (DEC-08a.5: the kernel loader will PANIC on this.) */
static void test_parse_foreign(void)
{
    uint8_t file[512];
    mz_image_t out;
    int r;

    memset(file, 0x00u, sizeof(file));
    /* Build a valid MZ but with tag = 0 (genuine 16-bit DOS EXE sentinel). */
    build_hdr(file, FIXTURE_HDR_SIZE,
              0u, 1u, 0u, 0u, 0xFFFFu,
              0u, 0u, 0u, 0u,
              0u   /* tag = 0, NOT MZ_INITECH_TAG */);

#ifndef MZ_MUTATE_ACCEPT_FOREIGN
    /* CLEAN BUILD: must reject. */
    r = mz_parse_header(file, 512u, &out);
    CHECK(r == MZ_ERR_FOREIGN,
          "parse_foreign: untagged MZ -> MZ_ERR_FOREIGN (RED under MZ_MUTATE_ACCEPT_FOREIGN)");
#else
    /* MUTANT MZ_MUTATE_ACCEPT_FOREIGN: the parse ACCEPTS the foreign header.
     * This CHECK asserts it returned MZ_OK -- which proves the mutant PASSES
     * where the clean build correctly FAILS.  The oracle goes RED on the
     * mutant because the test below (immediately after) would catch it:
     * the presence of this else-branch proves to the human reviewer that
     * the mutant alters behavior.  The main oracle for this mutant is the
     * clean-build assertion above going RED (failing to return MZ_ERR_FOREIGN
     * when the clean build expects it). */
    r = mz_parse_header(file, 512u, &out);
    /* The mutant should NOT return MZ_ERR_FOREIGN (it accepts foreign). */
    CHECK(r == MZ_OK,
          "parse_foreign (MZ_MUTATE_ACCEPT_FOREIGN): accepts untagged MZ as MZ_OK");
    /* This assertion is TRUE under the mutant (r != MZ_ERR_FOREIGN) but
     * would make the clean-build block above go RED -- the oracle bites
     * because the clean build does NOT reach this branch at all. */
    CHECK(r != MZ_ERR_FOREIGN,
          "parse_foreign (MZ_MUTATE_ACCEPT_FOREIGN): confirmed not MZ_ERR_FOREIGN");
#endif
}

/* mz_parse_header: truncated file (shorter than header). */
static void test_parse_truncated_header(void)
{
    /* A 20-byte "file" -- too short to even read the tag word at 0x1C. */
    uint8_t file[20];
    mz_image_t out;
    int r;

    file[0] = 0x4Du; file[1] = 0x5Au;   /* 'MZ' */
    r = mz_parse_header(file, 20u, &out);
    CHECK(r == MZ_ERR_TRUNCATED,
          "truncated_header: file < MZ_MIN_HDR_READ -> MZ_ERR_TRUNCATED");
}

/* mz_parse_header: file truncated before the full load module. */
static void test_parse_truncated_image(void)
{
    /* Header says 512 bytes total (1 full page), but we only give 256. */
    uint8_t file[256];
    mz_image_t out;
    int r;

    memset(file, 0x00u, sizeof(file));
    build_hdr(file, FIXTURE_HDR_SIZE,
              0u, 1u, 0u, 0u, 0xFFFFu,
              0u, 0u, 0u, 0u, MZ_INITECH_TAG);

    r = mz_parse_header(file, 256u, &out);   /* give only 256 of the 512 needed */
    CHECK(r == MZ_ERR_TRUNCATED,
          "truncated_image: file_len < image_bytes -> MZ_ERR_TRUNCATED");
}

/* mz_parse_header: header_bytes > image_bytes -> MZ_ERR_BAD_HEADER. */
static void test_parse_bad_header_size(void)
{
    /* Set e_cparhdr = 100 (1600 bytes) but image = 512 bytes. */
    uint8_t file[512];
    mz_image_t out;
    int r;

    memset(file, 0x00u, sizeof(file));
    build_hdr(file, FIXTURE_HDR_SIZE,
              0u, 1u, 0u, 0u, 0xFFFFu,
              0u, 0u, 0u, 0u, MZ_INITECH_TAG);
    /* Overwrite e_cparhdr with 100 paragraphs (100*16 = 1600 > 512). */
    put16(file, MZ_OFF_CPARHDR, 100u);

    r = mz_parse_header(file, 512u, &out);
    CHECK(r == MZ_ERR_BAD_HEADER,
          "bad_header_size: header > image -> MZ_ERR_BAD_HEADER");
}

/* mz_parse_header: reloc table overruns file -> MZ_ERR_BAD_RELOC. */
static void test_parse_bad_reloc(void)
{
    /* file = 512 bytes, e_crlc = 50 reloc entries => reloc table needs
     * FIXTURE_HDR_SIZE + 50*4 = 32 + 200 = 232 bytes just for the table.
     * That's fine if the file has 512 bytes.  Make it fail by setting
     * e_lfarlc close to the end of the file. */
    uint8_t file[512];
    mz_image_t out;
    int r;

    memset(file, 0x00u, sizeof(file));
    build_hdr(file, FIXTURE_HDR_SIZE,
              0u, 1u, 2u /*crlc*/, 0u, 0xFFFFu,
              0u, 0u, 0u, 0u, MZ_INITECH_TAG);
    /* Place e_lfarlc so that lfarlc + crlc*4 > file_len.
     * file_len=512, crlc=2, so we need lfarlc > 512-8 = 504. */
    put16(file, MZ_OFF_LFARLC, 510u);   /* 510 + 2*4 = 518 > 512 */

    r = mz_parse_header(file, 512u, &out);
    CHECK(r == MZ_ERR_BAD_RELOC,
          "bad_reloc: reloc table past file end -> MZ_ERR_BAD_RELOC");
}

/* mz_parse_header: 'MZ' magic not present -> MZ_ERR_NOT_MZ. */
static void test_parse_not_mz(void)
{
    uint8_t file[64];
    mz_image_t out;
    int r;

    memset(file, 0x00u, sizeof(file));
    file[0] = 0xEBu; file[1] = 0x3Cu;   /* COM JMP stub, not MZ */
    r = mz_parse_header(file, 64u, &out);
    CHECK(r == MZ_ERR_NOT_MZ, "not_mz: non-MZ magic -> MZ_ERR_NOT_MZ");
}

/* ---- mz_apply_relocs tests ----------------------------------------------- */

/* The core reloc-apply fixture.
 *
 * Load module layout (64 bytes):
 *   offset 0x00: dword = 0x00001000  (reloc site A)
 *   offset 0x04: dword = 0x00002000  (reloc site B)
 *   offset 0x08: dword = 0x000000C0  (reloc site C)
 *   offset 0x0C: byte  = 0xCC fill (no reloc here; must be UNTOUCHED)
 *   ... rest = 0xCC fill (all untouched)
 *
 * Reloc table: 3 entries -> offsets 0x00, 0x04, 0x08 (the 3 dword sites).
 * load_base = 0x30100 (PROGRAM_IMAGE).
 *
 * Expected after mz_apply_relocs:
 *   image[0x00] = 0x00001000 + 0x30100 = 0x00031100
 *   image[0x04] = 0x00002000 + 0x30100 = 0x00032100
 *   image[0x08] = 0x000000C0 + 0x30100 = 0x000301C0
 *   image[0x0C..] = 0xCC (untouched)
 */
static void test_apply_relocs_basic(void)
{
    uint8_t image[64];
    uint8_t reloc_table[12];   /* 3 * 4 bytes */
    uint32_t load_base = 0x00030100u;   /* PROGRAM_IMAGE */
    int r;

    memset(image, 0xCCu, sizeof(image));

    /* Write the original dwords at the reloc sites. */
    put32(image, 0x00u, 0x00001000u);
    put32(image, 0x04u, 0x00002000u);
    put32(image, 0x08u, 0x000000C0u);

    /* Build the reloc table: 3 LE32 entries. */
    put32(reloc_table, 0u,  0x00u);
    put32(reloc_table, 4u,  0x04u);
    put32(reloc_table, 8u,  0x08u);

    r = mz_apply_relocs(image, 64u, load_base, reloc_table, 3u);
    CHECK(r == MZ_OK, "apply_relocs: returns MZ_OK");

    /* Each relocated dword == original + load_base, BYTE-EXACT.
     * Under MZ_MUTATE_RELOC_NOADD the add is skipped so each dword stays at
     * its original value -> these CHECKs go RED.
     * Under MZ_MUTATE_RELOC_PARAGRAPH (load_base>>4 = 0x3010) the dwords get
     * a wrong addend -> these CHECKs also go RED (the oracle bites). */
    CHECK(get32(image, 0x00u) == (0x00001000u + load_base),
          "apply_relocs: site A == original + load_base (RED under both mutants)");
    CHECK(get32(image, 0x04u) == (0x00002000u + load_base),
          "apply_relocs: site B == original + load_base (RED under both mutants)");
    CHECK(get32(image, 0x08u) == (0x000000C0u + load_base),
          "apply_relocs: site C == original + load_base (RED under both mutants)");

    /* Non-reloc bytes MUST be untouched (fill byte 0xCC). */
    {
        int all_cc = 1;
        uint32_t i;
        for (i = 12u; i < 64u; i++) {
            if (image[i] != 0xCCu) { all_cc = 0; break; }
        }
        CHECK(all_cc == 1,
              "apply_relocs: bytes past reloc sites are untouched (0xCC fill)");
    }
    /* The dword at 0x0C (first non-reloc dword) should also be untouched
     * (0xCCCCCCCC in little-endian). */
    CHECK(get32(image, 0x0Cu) == 0xCCCCCCCCu,
          "apply_relocs: dword at 0x0C (no reloc) is untouched 0xCCCCCCCC");
}

/* mz_apply_relocs: zero reloc entries -> MZ_OK with image unchanged. */
static void test_apply_relocs_zero_count(void)
{
    uint8_t image[32];
    uint8_t rt[4];
    int r;

    memset(image, 0xAAu, sizeof(image));
    memset(rt,    0x00u, sizeof(rt));

    r = mz_apply_relocs(image, 32u, 0x30100u, rt, 0u);
    CHECK(r == MZ_OK, "apply_relocs_zero: MZ_OK for 0 relocs");
    {
        int all_aa = 1;
        uint32_t i;
        for (i = 0; i < 32u; i++) {
            if (image[i] != 0xAAu) { all_aa = 0; break; }
        }
        CHECK(all_aa == 1,
              "apply_relocs_zero: image unchanged with 0 reloc entries");
    }
}

/* mz_apply_relocs: a reloc offset that points OOB -> MZ_ERR_BAD_RELOC.
 * Also verifies that NO write is performed (image untouched before the OOB). */
static void test_apply_relocs_oob(void)
{
    uint8_t image[32];
    uint8_t reloc_table[4];
    int r;

    memset(image, 0xBBu, sizeof(image));

    /* Reloc offset 30: image[30..33] -- but image_len is only 32, so
     * 30 + 4 = 34 > 32.  Must be rejected without writing. */
    put32(reloc_table, 0u, 30u);

    r = mz_apply_relocs(image, 32u, 0x30100u, reloc_table, 1u);
    CHECK(r == MZ_ERR_BAD_RELOC,
          "apply_relocs_oob: OOB reloc offset -> MZ_ERR_BAD_RELOC (fail loud)");

    /* Verify the image is untouched (the OOB entry must not have written). */
    {
        int all_bb = 1;
        uint32_t i;
        for (i = 0; i < 32u; i++) {
            if (image[i] != 0xBBu) { all_bb = 0; break; }
        }
        CHECK(all_bb == 1,
              "apply_relocs_oob: image untouched on MZ_ERR_BAD_RELOC");
    }
}

/* mz_apply_relocs: reloc offset exactly at the last valid dword (boundary). */
static void test_apply_relocs_boundary(void)
{
    uint8_t image[32];
    uint8_t reloc_table[4];
    int r;

    memset(image, 0x00u, sizeof(image));
    /* Last valid dword is at offset 28 (32 - 4 = 28). */
    put32(image, 28u, 0x00000010u);
    put32(reloc_table, 0u, 28u);

    r = mz_apply_relocs(image, 32u, 0x30100u, reloc_table, 1u);
    CHECK(r == MZ_OK,
          "apply_relocs_boundary: last valid dword offset -> MZ_OK");
    CHECK(get32(image, 28u) == (0x00000010u + 0x30100u),
          "apply_relocs_boundary: boundary dword relocated correctly");
}

/* mz_apply_relocs: reloc offset off-by-one past end -> MZ_ERR_BAD_RELOC. */
static void test_apply_relocs_boundary_oob(void)
{
    uint8_t image[32];
    uint8_t reloc_table[4];
    int r;

    memset(image, 0xDDu, sizeof(image));
    /* Offset 29: dword needs bytes 29..32 but image_len is 32, so 29+4=33>32. */
    put32(reloc_table, 0u, 29u);

    r = mz_apply_relocs(image, 32u, 0x30100u, reloc_table, 1u);
    CHECK(r == MZ_ERR_BAD_RELOC,
          "apply_relocs_boundary_oob: offset 29 in 32-byte image -> MZ_ERR_BAD_RELOC");
}

/* Full integration: parse a hand-authored InitechMZ file (with relocs),
 * apply relocs, verify byte-exact final state.
 *
 * File layout:
 *   [0x00..0x1F] header (FIXTURE_HDR_SIZE = 32 bytes)
 *                e_cp=2, e_cblp=0  => image_bytes = 1024
 *                e_cparhdr=2 (32 bytes header)
 *                => load_module_off = 32, load_module_len = 992
 *                e_crlc=2 (2 reloc entries), e_lfarlc=0x20 (right after header)
 *                e_cs=0, e_ip=0, e_ss=0, e_sp=0
 *   [0x20..0x27] reloc table: entry0=0x00 (LE32), entry1=0x08 (LE32)
 *   [0x28..0x3FF] load module (fill 0x55 except reloc sites)
 *                 At load module offset 0x00 (file offset 0x20+0x08=0x28 + 0 = 0x28
 *                 but reloc offsets are within the LOAD MODULE not the file):
 *                 Actually the reloc offsets are relative to the load module start
 *                 in memory (PROGRAM_IMAGE).  The file layout places the load
 *                 module at file offset 32 (= FIXTURE_HDR_SIZE).
 *                 load_module_off = 32.
 *                 reloc site A: load_module[0x00] = 0x0000AABB (at file offset 32)
 *                 reloc site B: load_module[0x08] = 0x0000CCDD (at file offset 40)
 *
 * load_base = 0x30100.
 *
 * Expected after apply:
 *   load_module[0x00] = 0x0000AABB + 0x30100 = 0x0030BBBB... let's compute:
 *     0x0000AABB + 0x00030100 = 0x0003ABBB
 *   load_module[0x08] = 0x0000CCDD + 0x00030100 = 0x0003CDDD
 */
static void test_integration_parse_and_reloc(void)
{
    /* File = 1024 bytes (2 full 512-byte pages). */
    uint8_t file[1024];
    uint8_t load_module_copy[992];
    mz_image_t img;
    uint32_t load_base = 0x00030100u;
    int r;

    memset(file, 0x55u, sizeof(file));

    /* Build header: cp=2, cblp=0, crlc=2, cparhdr=2, lfarlc=0x20,
     * tag=MZ_INITECH_TAG. */
    build_hdr(file, FIXTURE_HDR_SIZE,
              0u /*cblp*/, 2u /*cp*/, 2u /*crlc*/,
              0x20u /*minall*/, 0xFFFFu /*maxall*/,
              0u, 0u, 0u, 0u, MZ_INITECH_TAG);
    /* lfarlc is already set to FIXTURE_HDR_SIZE=0x20 by build_hdr. */

    /* Write reloc table at file offset 0x20: two entries. */
    put32(file, 0x20u, 0x00u);   /* reloc 0: load_module offset 0x00 */
    put32(file, 0x24u, 0x08u);   /* reloc 1: load_module offset 0x08 */

    /* Write original dwords at the reloc sites within the load module.
     * load_module starts at file offset 32 (= FIXTURE_HDR_SIZE = 0x20).
     * BUT wait -- lfarlc = 0x20 and the reloc table is 2*4=8 bytes,
     * occupying file offsets 0x20..0x27.  The load_module_off = 32 = 0x20.
     * This means the reloc table OVERLAPS the start of the load module in
     * the file (in real MZ the reloc table lives IN the header region, i.e.
     * before the load module).  We need to make e_cparhdr large enough to
     * place the load module AFTER the reloc table.
     *
     * Fix: use e_cparhdr=4 (4*16=64 bytes header); lfarlc=0x20 (reloc table
     * at file offset 32, right after our 32-byte header data region);
     * load_module starts at file offset 64.
     * image_bytes = 2*512 = 1024; header_bytes = 64; load_module_len = 960.
     *
     * Rebuild the header with cparhdr=4.
     */
    memset(file, 0x55u, sizeof(file));
    /* Use a 64-byte header (e_cparhdr=4). */
    build_hdr(file, FIXTURE_HDR_SIZE,
              0u, 2u, 2u, 0x20u, 0xFFFFu,
              0u, 0u, 0u, 0u, MZ_INITECH_TAG);
    put16(file, MZ_OFF_CPARHDR, 4u);   /* 4*16 = 64 bytes */
    /* lfarlc = 0x20 (32): reloc table right after the 32-byte header data. */
    /* reloc table at file offset 0x20: */
    put32(file, 0x20u, 0x00u);   /* reloc 0: load_module[0x00] */
    put32(file, 0x24u, 0x08u);   /* reloc 1: load_module[0x08] */
    /* Load module starts at file offset 64 (0x40). */
    put32(file, 0x40u, 0x0000AABBu);   /* reloc site A */
    /* load_module[0x08] = file offset 64+8 = 72 */
    put32(file, 0x48u, 0x0000CCDDu);   /* reloc site B */

    /* Parse. */
    r = mz_parse_header(file, 1024u, &img);
    CHECK(r == MZ_OK,                      "integration: parse MZ_OK");
    CHECK(img.load_module_off == 64u,      "integration: load_module_off == 64");
    CHECK(img.load_module_len == 960u,     "integration: load_module_len == 960");
    CHECK(img.reloc_table_off == 0x20u,    "integration: reloc_table_off == 0x20");
    CHECK(img.reloc_count == 2u,           "integration: reloc_count == 2");

    /* Copy load module to a local buffer (simulating loader copying to PROGRAM_IMAGE). */
    memcpy(load_module_copy,
           file + img.load_module_off,
           img.load_module_len < 992u ? img.load_module_len : 992u);

    /* Apply relocs. */
    r = mz_apply_relocs(load_module_copy, img.load_module_len,
                        load_base,
                        file + img.reloc_table_off, img.reloc_count);
    CHECK(r == MZ_OK, "integration: apply_relocs MZ_OK");

    /* Verify relocated dwords.
     * Under MZ_MUTATE_RELOC_NOADD: the dwords stay at their original values ->
     * these CHECKs go RED.
     * Under MZ_MUTATE_RELOC_PARAGRAPH: addend is 0x30100>>4 = 0x3010, so
     * site A = 0x0000AABB + 0x3010 = 0x0000DBCB != 0x0003ABBB -> RED. */
    CHECK(get32(load_module_copy, 0x00u) == (0x0000AABBu + load_base),
          "integration: site A relocated (RED under RELOC_NOADD + RELOC_PARAGRAPH)");
    CHECK(get32(load_module_copy, 0x08u) == (0x0000CCDDu + load_base),
          "integration: site B relocated (RED under RELOC_NOADD + RELOC_PARAGRAPH)");
}

/* ---- main ---------------------------------------------------------------- */

int main(void)
{
    test_is_mz();
    test_parse_partial_last_page();
    test_parse_full_last_page();
    test_parse_cs_ss_nonzero();
    test_parse_foreign();
    test_parse_truncated_header();
    test_parse_truncated_image();
    test_parse_bad_header_size();
    test_parse_bad_reloc();
    test_parse_not_mz();
    test_apply_relocs_basic();
    test_apply_relocs_zero_count();
    test_apply_relocs_oob();
    test_apply_relocs_boundary();
    test_apply_relocs_boundary_oob();
    test_integration_parse_and_reloc();
    return TEST_SUMMARY("test_mz");
}
