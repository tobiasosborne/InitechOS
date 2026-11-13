/* flairovl.c -- the FLAIR.OVL factory packer (ADR-0014 DEC-14-2; epic
 * initech-x8gsi lane 1). Factory C (Law 3); host only.
 *
 *   flairovl pack <raw> <kernel.bin> <out.ovl> <out.sys>
 *
 * <raw> is `objcopy -O binary` of the overlay ELF linked by os/milton/overlay.ld
 * (header at offset 0, identity and crc32 assembled as zero). <kernel.bin> is
 * the flat kernel (INITDOS.SYS before patching) the overlay was linked
 * against. The packer:
 *   1. checks the header shape against spec/overlay_format.h (magic, version,
 *      counts, image_len == raw length, the directory's load rule);
 *   2. writes identity = CRC-32 of <kernel.bin> (OVL_IDENTITY_RULE);
 *   3. pads to a whole OVL_SECTOR with zeros and writes crc32 (OVL_CRC_RULE);
 *   4. copies <kernel.bin> to <out.sys>, replacing the ONE 8-byte placeholder
 *      "OVLCRC??" (the generated ovl_kernel_expect slot) with {crc32,
 *      identity} -- kernel link, overlay link, then patch: no address moves.
 * Fails loud (exit 1) on any surprise, including zero or two placeholders.
 * Deterministic: no clock, no host path in any output (Rule 11).
 * The INDEPENDENT reader that grades this output is harness/proptest/
 * ovl_reader.c (table-driven CRC, ELF-name cross-checks); this file is never
 * its oracle (Law 2). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "overlay_format.h"

#define OVERLAY_BASE_ADDR 0x005C0000u
#define OVERLAY_CEIL_ADDR 0x005E0000u

static void die(const char *m) { fprintf(stderr, "!!! flairovl: %s\n", m); exit(1); }

static uint8_t *slurp(const char *path, size_t *n, size_t extra)
{
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open an input");
    if (fseek(f, 0, SEEK_END) != 0) die("seek");
    long len = ftell(f);
    if (len < 0) die("tell");
    rewind(f);
    uint8_t *b = calloc((size_t)len + extra + 1, 1);
    if (!b) die("oom");
    if (fread(b, 1, (size_t)len, f) != (size_t)len) die("short read");
    fclose(f);
    *n = (size_t)len;
    return b;
}

static void spill(const char *path, const uint8_t *b, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(b, 1, n, f) != n || fclose(f) != 0) die("cannot write an output");
}

static uint32_t rd(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static void wr(uint8_t *p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }

static uint32_t crc_step(uint32_t c, const uint8_t *p, size_t n)
{
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++) c = (c & 1u) ? (c >> 1) ^ OVL_CRC_POLY : c >> 1;
    }
    return c;
}

int main(int argc, char **argv)
{
    if (argc != 6 || strcmp(argv[1], "pack") != 0)
        die("usage: flairovl pack <raw> <kernel.bin> <out.ovl> <out.sys>");
    size_t n, kn;
    uint8_t *o = slurp(argv[2], &n, OVL_SECTOR);
    uint8_t *k = slurp(argv[3], &kn, 0);

    if (n < OVL_HDR_FIXED || memcmp(o, OVL_MAGIC, OVL_MAGIC_LEN) != 0) die("raw image has no FLAIROVL magic");
    if (rd(o + 8) != OVL_VERSION) die("version");
    uint32_t nsec = rd(o + 16), nimp = rd(o + 20), nexp = rd(o + 24), ilen = rd(o + 28);
    if (nsec != 1u || nimp == 0u || nexp == 0u) die("counts");
    if (ilen != n) die("image_len != linked image length");
    uint32_t crc_off = OVL_CRC_OFF(nsec, nimp, nexp);
    if (crc_off + 4u > ilen) die("tables run past the image");
    if (rd(o + 12) != 0u || rd(o + crc_off) != 0u) die("identity/crc32 not zero in the raw link");
    const uint8_t *d = o + OVL_HDR_FIXED;
    uint32_t load = rd(d + 4), off = rd(d + 8), len = rd(d + 12), bss = rd(d + 16);
    if (load != OVERLAY_BASE_ADDR + off || off < crc_off + 4u || off + len != ilen ||
        bss < load + len || bss > OVERLAY_CEIL_ADDR)
        die("directory violates OVL_SECTION_LOAD_RULE");

    /* OVL_IDENTITY_RULE: the unpatched kernel image, placeholder in place. */
    uint32_t identity = ~crc_step(0xFFFFFFFFu, k, kn);
    wr(o + 12, identity);
    size_t padded = (n + OVL_SECTOR - 1u) / OVL_SECTOR * OVL_SECTOR;
    if (padded > OVERLAY_CEIL_ADDR - OVERLAY_BASE_ADDR) die("overlay busts the window");
    uint32_t crc = ~crc_step(crc_step(0xFFFFFFFFu, o, crc_off), o + crc_off + 4u, ilen - crc_off - 4u);
    wr(o + crc_off, crc);
    spill(argv[4], o, padded);

    size_t at = 0, hits = 0;
    for (size_t i = 0; i + 8u <= kn; i++)
        if (rd(k + i) == OVL_KERNEL_PLACEHOLDER0 && rd(k + i + 4) == OVL_KERNEL_PLACEHOLDER1) { at = i; hits++; }
    if (hits != 1u) die("kernel image must carry exactly one OVLCRC?? placeholder");
    wr(k + at, crc);
    wr(k + at + 4, identity);
    spill(argv[5], k, kn);
    printf(">>> flairovl: %s %zu B (%u B image, %u imports, %u exports) crc32=%08X identity=%08X; kernel patched at +0x%zX\n",
           strrchr(argv[4], '/') ? strrchr(argv[4], '/') + 1 : argv[4], padded, ilen, nimp, nexp, crc, identity, at);
    return 0;
}
