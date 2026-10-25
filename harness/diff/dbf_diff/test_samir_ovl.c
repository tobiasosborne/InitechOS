/* test_samir_ovl.c -- host oracle for the SAMIR overlay manager's pure parts
 * and for the shipped SAMIR.EXE / SAMIR.OVL pair (initech-5etom.1;
 * docs/design/samir-overlays.md).
 *
 * Expected values are LITERALS, not recomputed from the artifact (Law 2):
 *   - the CRC-32 check value of "123456789" is 0xCBF43926 (the IEEE/zlib
 *     CRC, spec/samir/ovl_format.h);
 *   - the container offsets are pinned byte by byte (a silent edit of the
 *     locked spec header goes RED here);
 *   - relocation is proven on hand-written bytes, including the refusal of an
 *     out-of-range offset with NOTHING written (Rule 2);
 *   - the header check refuses each wrong field on its own.
 * Then the REAL build/SAMIR.OVL is walked: the stamp equals the CRC of the
 * body, every directory CRC matches, every module begins with its slot header
 * (magic, id), the slot is the largest overlay, the file is a 512-byte
 * multiple, and the stamp occurs in SAMIR.EXE's module exactly once (the
 * packer's patch: the pair is tied).
 *
 * Usage: test_samir_ovl [build/SAMIR.EXE]
 * Mutants (test-samir-ovl-mutant): -DSAMIR_OVL_MUTATE_NO_RELOC,
 * -DSAMIR_OVL_MUTATE_NO_STAMP -- each RED on its named check.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "test_assert.h"
#include "samir/ovl.h"
#include "samir/ovl_format.h"
#include "samir_install.h"
TEST_HARNESS();

static unsigned char *slurp(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    unsigned char *b;
    if (!f || fseek(f, 0, SEEK_END)) { if (f) fclose(f); return NULL; }
    *len = ftell(f);
    rewind(f);
    b = malloc(*len > 0 ? (size_t)*len : 1u);
    if (!b || fread(b, 1, (size_t)*len, f) != (size_t)*len) { fclose(f); free(b); return NULL; }
    fclose(f);
    return b;
}
static uint32_t rd32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void wr32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16); p[3] = (unsigned char)(v >> 24);
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : "build/SAMIR.EXE";
    char ovlpath[1024];

    /* ---- B0: the locked layout, literally ---- */
    CHECK(SAMIR_OVL_HDR_SIZE == 32 && SAMIR_OVL_DIR_SIZE == 16, "B0 header 32 B, directory entry 16 B");
    CHECK(SAMIR_OVL_HDR_VER_OFF == 8 && SAMIR_OVL_HDR_COUNT_OFF == 10 && SAMIR_OVL_HDR_STAMP_OFF == 12 &&
          SAMIR_OVL_HDR_SLOT_OFF == 16 && SAMIR_OVL_HDR_FLEN_OFF == 20, "B0 header field offsets");
    CHECK(SAMIR_OVL_DIR_OFF_OFF == 0 && SAMIR_OVL_DIR_LEN_OFF == 4 && SAMIR_OVL_DIR_NREL_OFF == 8 &&
          SAMIR_OVL_DIR_CRC_OFF == 12, "B0 directory field offsets");
    CHECK(SAMIR_OVL_MAGIC == 0x4C564F53u && !memcmp("SOVL", "\x53\x4f\x56\x4c", 4), "B0 slot magic is 'SOVL'");
    CHECK(SAMIR_OVL_ALIGN == 512, "B0 file padded to 512 B (DBASE.OVL is a 512-byte multiple)");

    /* ---- B1: CRC-32 check value ---- */
    CHECK(samir_ovl_crc32(0, "123456789", 9) == 0xCBF43926u, "B1 CRC-32(\"123456789\") == 0xCBF43926");
    CHECK(samir_ovl_crc32(samir_ovl_crc32(0, "1234", 4), "56789", 5) == 0xCBF43926u, "B1 CRC-32 continues across calls");

    /* ---- B2: relocation on literal bytes ---- */
    {
        unsigned char slot[16] = { 0x10, 0, 0, 0,  0xAA, 0xBB, 0xCC, 0xDD,  0xFF, 0xFF, 0xFF, 0xFF,  1, 2, 3, 4 };
        unsigned char rel[8], keep[16];
        wr32(rel, 0); wr32(rel + 4, 8);
        CHECK(samir_ovl_relocate(slot, sizeof slot, rel, 2, 0x40100u) == 0, "B2 relocate two sites");
        CHECK(rd32(slot) == 0x40110u, "B2 site 0: 0x10 + base 0x40100 == 0x40110");
        CHECK(rd32(slot + 8) == 0x000400FFu, "B2 site 8: 0xFFFFFFFF + 0x40100 wraps mod 2^32 to 0x400FF");
        CHECK(rd32(slot + 4) == 0xDDCCBBAAu && rd32(slot + 12) == 0x04030201u, "B2 bytes between sites untouched");
        memcpy(keep, slot, sizeof slot);
        wr32(rel, 4); wr32(rel + 4, 13);   /* 13 + 4 > 16: out of range */
        CHECK(samir_ovl_relocate(slot, sizeof slot, rel, 2, 1u) == -1, "B2 out-of-range offset refused");
        CHECK(!memcmp(keep, slot, sizeof slot), "B2 refusal wrote NOTHING (the valid first site too)");
    }

    /* ---- B3: header check, one wrong field at a time ---- */
    {
        unsigned char h[32], b[32];
        memset(h, 0, sizeof h);
        memcpy(h, "SAMIROVL", 8);
        h[8] = 1; h[10] = 5;
        wr32(h + 12, 0x12345678u); wr32(h + 16, 9712u); wr32(h + 20, 26112u);
        CHECK(samir_ovl_check_header(h, 0x12345678u, 9712u, 5u) == SAMIR_OVL_OK, "B3 matching header accepted");
        memcpy(b, h, 32); b[0] = 'X';
        CHECK(samir_ovl_check_header(b, 0x12345678u, 9712u, 5u) == SAMIR_OVL_FOREIGN, "B3 signature refused");
        memcpy(b, h, 32); b[8] = 2;
        CHECK(samir_ovl_check_header(b, 0x12345678u, 9712u, 5u) == SAMIR_OVL_FOREIGN, "B3 version refused");
        CHECK(samir_ovl_check_header(h, 0x12345679u, 9712u, 5u) == SAMIR_OVL_FOREIGN, "B3 STAMP: another build's overlay file refused");
        CHECK(samir_ovl_check_header(h, 0x12345678u, 9728u, 5u) == SAMIR_OVL_FOREIGN, "B3 slot length refused");
        CHECK(samir_ovl_check_header(h, 0x12345678u, 9712u, 4u) == SAMIR_OVL_FOREIGN, "B3 overlay count refused");
    }

    /* ---- B4: directory bounds ---- */
    {
        unsigned char e[16];
        uint32_t o, l, n, c;
        wr32(e, 0x70); wr32(e + 4, 100); wr32(e + 8, 3); wr32(e + 12, 0xABCDu);
        CHECK(samir_ovl_check_dir(e, 112, 0x70 + 100 + 12, &o, &l, &n, &c) == SAMIR_OVL_OK &&
              o == 0x70 && l == 100 && n == 3 && c == 0xABCDu, "B4 entry exactly fitting the file accepted");
        CHECK(samir_ovl_check_dir(e, 112, 0x70 + 100 + 11, &o, &l, &n, &c) == SAMIR_OVL_CORRUPT, "B4 relocation table past EOF refused");
        CHECK(samir_ovl_check_dir(e, 99, 0x1000, &o, &l, &n, &c) == SAMIR_OVL_CORRUPT, "B4 module larger than the slot refused");
        wr32(e + 4, 8);
        CHECK(samir_ovl_check_dir(e, 112, 0x1000, &o, &l, &n, &c) == SAMIR_OVL_CORRUPT, "B4 module shorter than its slot header refused");
        wr32(e, 0xFFFFFFF0u); wr32(e + 4, 100);
        CHECK(samir_ovl_check_dir(e, 112, 0x1000, &o, &l, &n, &c) == SAMIR_OVL_CORRUPT, "B4 offset wrap refused");
    }

    /* ---- B5: the shipped pair ---- */
    if (samir_ovl_path(exe, ovlpath, sizeof ovlpath)) {
        CHECK(0, "B5 SAMIR.EXE path ends in .EXE");
        return TEST_SUMMARY("samir-ovl");
    }
    {
        long el, ol;
        unsigned char *e = slurp(exe, &el), *o = slurp(ovlpath, &ol);
        uint32_t i, count, slot, maxlen = 0, stamp, hdr, image, hits = 0;
        CHECK(e && o && ol >= 32 && el >= 32, "B5 SAMIR.EXE and SAMIR.OVL readable");
        if (!e || !o || ol < 32 || el < 32) return TEST_SUMMARY("samir-ovl");
        count = (uint32_t)o[10] | ((uint32_t)o[11] << 8);
        slot = rd32(o + 16);
        stamp = rd32(o + 12);
        CHECK(!memcmp(o, "SAMIROVL", 8) && o[8] == 1 && o[9] == 0, "B5 signature and version 1");
        CHECK(count == SAMIR_OVL_COUNT, "B5 one directory entry per overlay id");
        CHECK(rd32(o + 20) == (uint32_t)ol && ol % 512 == 0, "B5 recorded length == file length, a 512-byte multiple");
        CHECK(samir_ovl_crc32(0, o + 32, (uint32_t)ol - 32) == stamp, "B5 stamp == CRC-32 of the body");
        for (i = 0; i < count && 32 + 16 * (i + 1) <= (uint32_t)ol; i++) {
            uint32_t off, len, nrel, crc;
            char what[96];
            snprintf(what, sizeof what, "B5 overlay %u directory entry in bounds", (unsigned)i);
            CHECK(samir_ovl_check_dir(o + 32 + 16 * i, slot, (uint32_t)ol, &off, &len, &nrel, &crc) == SAMIR_OVL_OK, what);
            if (off + len + 4u * nrel > (uint32_t)ol) continue;
            snprintf(what, sizeof what, "B5 overlay %u CRC matches", (unsigned)i);
            CHECK(samir_ovl_crc32(0, o + off, len + 4u * nrel) == crc, what);
            snprintf(what, sizeof what, "B5 overlay %u starts with SOVL + its id", (unsigned)i);
            CHECK(rd32(o + off) == 0x4C564F53u && rd32(o + off + 4) == i, what);
            if (len > maxlen) maxlen = len;
        }
        CHECK(slot == ((maxlen + 15u) & ~15u), "B5 slot == largest overlay rounded to 16");
        /* The stamp is patched into the resident module exactly once. */
        hdr = ((uint32_t)e[8] | ((uint32_t)e[9] << 8)) * 16u;
        image = e[2] | (e[3] << 8) ? ((uint32_t)(e[4] | (e[5] << 8)) - 1u) * 512u + (uint32_t)(e[2] | (e[3] << 8))
                                   : (uint32_t)(e[4] | (e[5] << 8)) * 512u;
        CHECK(e[0] == 'M' && e[1] == 'Z' && e[0x1C] == 0x43 && e[0x1D] == 0x49 && hdr <= image && image <= (uint32_t)el,
              "B5 SAMIR.EXE is a tagged InitechMZ");
        for (i = hdr; i + 4 <= image; i++) hits += rd32(e + i) == stamp;
        CHECK(hits == 1, "B5 the overlay stamp occurs once in the resident module (the pair is tied)");
        free(e); free(o);
    }
    return TEST_SUMMARY("samir-ovl");
}
