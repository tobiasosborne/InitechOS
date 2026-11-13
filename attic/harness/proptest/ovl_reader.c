/* ovl_reader.c -- INDEPENDENT host reader of C:\FLAIR.OVL (ADR-0014 DEC-14-9:
 * "An independent host reader of the header, directory and CRC is written in
 * C under harness/, not by reusing the kernel's parser"). Factory C (Law 3).
 *
 *   ovl_reader <FLAIR.OVL> <kernel.bin (unpatched)> <INITDOS.SYS (patched)>
 *              <kernel-nm.txt> <overlay-nm.txt> <imports.txt> <exports.txt>
 *
 * Independent of the kernel (os/milton/kmain.c ovl_load) and of the packer
 * (tools/flairovl.c): it does not include spec/overlay_format.h -- the
 * layout below is transcribed from the ADR-0014 DEC-14-2 field order and the
 * spec's prose -- and its CRC-32 is TABLE-driven (the kernel and packer are
 * bitwise). Every address is graded against SYMBOL NAMES from `nm` of the two
 * ELFs, never against a value the artifact computed:
 *   - magic "FLAIROVL", version 1, one section, the file a whole number of
 *     512 B sectors with zero padding past image_len;
 *   - imports[i] == nm address of the i-th generated import name in the
 *     KERNEL ELF; exports[i] == nm address of the i-th export in the OVERLAY ELF;
 *   - directory: id 1, PRELOAD, load == 0x5C0000 + file_off, file_off right
 *     after the crc32 dword, length reaching image_len, bss_end in the window;
 *   - crc32 == CRC-32/IEEE of every other byte of [0, image_len);
 *   - identity == CRC-32/IEEE of the unpatched kernel.bin;
 *   - INITDOS.SYS differs from kernel.bin in exactly 8 bytes: the placeholder
 *     "OVLCRC??" replaced by {crc32, identity}.
 * Prints one GREEN line, or a RED line naming the first failed property and
 * exits 1. MUTANT (Rule 6; test-ovl-format-mutant): OVL_READER_MUT_NO_CRC
 * skips the CRC property and must then pass a corrupted file -- the gate
 * asserts the real reader rejects it. */
#define _GNU_SOURCE 1   /* memmem */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN_LO 0x005C0000u
#define WIN_HI 0x005E0000u

static uint32_t tab[256];
static void crc_init(void)
{
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        tab[n] = c;
    }
}
static uint32_t crc_upd(uint32_t c, const uint8_t *p, size_t n)
{
    while (n--) c = tab[(c ^ *p++) & 0xFFu] ^ (c >> 8);
    return c;
}

static void red(const char *what) { printf("RED ovl-reader: %s\n", what); exit(1); }

static uint8_t *load(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (!f) red("cannot open an input");
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    uint8_t *b = malloc((size_t)len + 1);
    if (!b || fread(b, 1, (size_t)len, f) != (size_t)len) red("short read");
    fclose(f);
    *n = (size_t)len;
    return b;
}
static uint32_t le(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

/* nm line "0050a32c T name" -> address of `name`, or exit RED. */
static uint32_t nm_addr(const char *nmfile, const char *name)
{
    FILE *f = fopen(nmfile, "r");
    char line[512], sym[256], type;
    unsigned long a;
    uint32_t found = 0; int hits = 0;
    if (!f) red("cannot open an nm listing");
    while (fgets(line, sizeof line, f))
        if (sscanf(line, "%lx %c %255s", &a, &type, sym) == 3 && strcmp(sym, name) == 0) { found = (uint32_t)a; hits++; }
    fclose(f);
    if (hits != 1) { printf("RED ovl-reader: symbol %s found %d times\n", name, hits); exit(1); }
    return found;
}

static int names(const char *path, char out[][128], int max)
{
    FILE *f = fopen(path, "r");
    int n = 0;
    if (!f) red("cannot open a name list");
    while (n < max && fscanf(f, "%127s", out[n]) == 1) n++;
    fclose(f);
    return n;
}

int main(int argc, char **argv)
{
    static char imp[256][128], exp_[64][128];
    size_t n, kn, sn;
    if (argc != 8) { fprintf(stderr, "usage: see header\n"); return 2; }
    crc_init();
    uint8_t *o = load(argv[1], &n), *k = load(argv[2], &kn), *s = load(argv[3], &sn);
    int ni = names(argv[6], imp, 256), ne = names(argv[7], exp_, 64);

    if (n < 32 || memcmp(o, "FLAIROVL", 8) != 0) red("magic");
    if (le(o + 8) != 1) red("version");
    if (n % 512 != 0) red("file is not whole 512-byte sectors");
    uint32_t nsec = le(o + 16), nimp = le(o + 20), nexp = le(o + 24), ilen = le(o + 28);
    if (nsec != 1) red("section count");
    if (nimp != (uint32_t)ni || nexp != (uint32_t)ne) red("table counts != generated name lists");
    if (ilen > n || n - ilen >= 512) red("image_len vs padded file length");
    for (size_t i = ilen; i < n; i++) if (o[i]) red("non-zero sector padding");
    const uint8_t *d = o + 32;
    uint32_t imp_off = 32 + 20 * nsec, exp_off = imp_off + 4 * nimp, crc_off = exp_off + 4 * nexp;
    if (crc_off + 4 > ilen) red("tables overrun image_len");
    if ((d[0] | d[1] << 8) != 1 || (d[2] | d[3] << 8) != 1) red("directory id/flags (JTC, PRELOAD)");
    uint32_t load_ = le(d + 4), off = le(d + 8), len = le(d + 12), bss = le(d + 16);
    if (off != crc_off + 4 || load_ != WIN_LO + off) red("directory load/file_off");
    if (off + len != ilen) red("directory length != image_len");
    if (bss < load_ + len || bss > WIN_HI) red("directory bss_end");
    for (int i = 0; i < ni; i++)
        if (le(o + imp_off + 4 * i) != nm_addr(argv[4], imp[i])) {
            printf("RED ovl-reader: import %d (%s) != the kernel ELF's address\n", i, imp[i]); return 1;
        }
    for (int i = 0; i < ne; i++) {
        uint32_t e = le(o + exp_off + 4 * i);
        if (e != nm_addr(argv[5], exp_[i])) { printf("RED ovl-reader: export %d (%s) != the overlay ELF's address\n", i, exp_[i]); return 1; }
        if (e < WIN_LO + off || e >= WIN_LO + ilen) red("export outside the section payload");
    }
    uint32_t crc = ~crc_upd(crc_upd(0xFFFFFFFFu, o, crc_off), o + crc_off + 4, ilen - crc_off - 4);
#ifndef OVL_READER_MUT_NO_CRC
    if (crc != le(o + crc_off)) red("crc32 field != CRC-32 of every other byte");
#endif
    uint32_t ident = ~crc_upd(0xFFFFFFFFu, k, kn);
    if (ident != le(o + 12)) red("identity != CRC-32 of the unpatched kernel image");
    if (sn != kn) red("patched kernel length differs");
    {
        uint8_t *p = memmem(k, kn, "OVLCRC??", 8);
        if (!p) red("unpatched kernel has no OVLCRC?? placeholder");
        size_t at = (size_t)(p - k);
        if (memmem(p + 1, kn - at - 1, "OVLCRC??", 8)) red("two placeholders in the kernel");
        if (le(s + at) != le(o + crc_off) || le(s + at + 4) != ident)
            red("INITDOS.SYS expect slot != {crc32, identity}");
        for (size_t i = 0; i < kn; i++) if ((i < at || i >= at + 8) && k[i] != s[i]) red("patch outside the slot");
    }
    printf("GREEN ovl-reader: %s %zu B (image %u B), %d imports and %d exports match the ELF symbols, crc32=%08X identity=%08X, kernel slot patched\n",
           strrchr(argv[1], '/') ? strrchr(argv[1], '/') + 1 : argv[1], n, ilen, ni, ne, crc, ident);
    return 0;
}
