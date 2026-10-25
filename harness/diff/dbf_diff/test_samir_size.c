/* Factory size oracle. ADR-0009 DEC-03/04/05, samir.ld and memory_map.h.
 *
 * Since overlays Phase B (initech-5etom.1; docs/design/samir-overlays.md
 * Sec 5) SAMIR is SAMIR.EXE (InitechMZ) + SAMIR.OVL. The PHYSICAL arithmetic
 * is unchanged: the loaded module + the 64 KiB BSS reserve + the PAL's AH=48h
 * block + its MCB/rounding (0x20) must end before ENV_BLOCK, and the PM
 * requires 4096 B of headroom (WL-0182, initech-9swot). What moved is the split
 * of the 36,864-byte PAL heap: the 16,528-byte region that lived in the BSS
 * reserve is now in the AH=48h block (the slot took its place) and the
 * 3,952-byte region moved into the reserve, so the arena block is 0x8090 B
 * instead of 0x4F70 and the module budget is 106,352 - 16,528 + 3,952 =
 * 93,776 B. Nothing was relaxed: the same 4096 B floor applies to the same
 * window. The reserve itself (slot + .bss + region 1 <= 64 KiB) is a link-time
 * ASSERT in samir.ld.
 *
 * Usage: test_samir_size SAMIR.EXE   (SAMIR.OVL is the sibling X.OVL)
 *   1. the RESIDENT check: the InitechMZ load module (image bytes - header
 *      bytes; the header and its relocation table are never loaded,
 *      DEC-08a.2) <= budget - 4096 ("RED SAMIR_SIZE:");
 *   2. the SLOT check: SAMIR.OVL's slot length is exactly the largest overlay
 *      rounded to 16, and every overlay fits it ("RED SAMIR_SLOT:");
 *   3. the contract pins: the MZ is tagged InitechMZ and declares e_minalloc =
 *      the PAL block + MCB (2058 paragraphs), so the loader enforces the
 *      arena this arithmetic assumes (DEC-08a.3).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "memory_map.h"
#include "samir/ovl_format.h"

enum { HEADROOM = 4096,
       PAL_ARENA = 0x8090,          /* pal_milton.c PAL_MILTON_HEAP_PARAS * 16 */
       MINALLOC = PAL_ARENA / 16 + 1,
       BUDGET = ENV_BLOCK - PROGRAM_IMAGE - PROGRAM_BSS_RESERVE - PAL_ARENA - 0x20 };

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
static uint32_t rd16(const unsigned char *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }
static uint32_t rd32(const unsigned char *p) { return rd16(p) | (rd16(p + 2) << 16); }

int main(int argc, char **argv)
{
    char ovlpath[4096];
    long elen, olen;
    unsigned char *e, *o;
    uint32_t image, header, module, slot, maxlen = 0, count, i, imax = 0;
    size_t n;

    if (argc != 2 || (n = strlen(argv[1])) < 4 || n >= sizeof ovlpath || strcmp(argv[1] + n - 4, ".EXE")) {
        fputs("samir-size: usage: test_samir_size X.EXE (X.OVL beside it)\n", stderr);
        return 2;
    }
    memcpy(ovlpath, argv[1], n - 4);
    memcpy(ovlpath + n - 4, ".OVL", 5);
    if (BUDGET != 93776) {
        fputs("samir-size: memory contract changed; review ADR-0009 arithmetic\n", stderr);
        return 2;
    }
    e = slurp(argv[1], &elen);
    o = slurp(ovlpath, &olen);
    if (!e || elen < 0x20 || !o || olen < (long)SAMIR_OVL_HDR_SIZE) {
        fputs("samir-size: missing/short SAMIR.EXE or SAMIR.OVL\n", stderr);
        return 2;
    }
    /* ---- the resident module ---- */
    if (e[0] != 'M' || e[1] != 'Z' || rd16(e + 0x1C) != 0x4943u) {
        fputs("samir-size: not a tagged InitechMZ (DEC-08a.5)\n", stderr);
        return 2;
    }
    image = rd16(e + 2) ? (rd16(e + 4) - 1u) * 512u + rd16(e + 2) : rd16(e + 4) * 512u;
    header = rd16(e + 8) * 16u;
    if (header > image || image > (uint32_t)elen) {
        fputs("samir-size: malformed MZ header\n", stderr);
        return 2;
    }
    module = image - header;
    if (rd16(e + 0x0A) != (uint32_t)MINALLOC) {
        fprintf(stderr, "samir-size: e_minalloc %u != %u (PAL arena + MCB): the loader would not enforce this budget\n",
                (unsigned)rd16(e + 0x0A), (unsigned)MINALLOC);
        return 2;
    }
    if (module > (uint32_t)(BUDGET - HEADROOM)) {
        fprintf(stderr, "RED SAMIR_SIZE: resident %u B > %d B headroom cap\n",
                (unsigned)module, BUDGET - HEADROOM);
        return 1;
    }
    /* ---- the slot vs the overlays ---- */
    if (memcmp(o, SAMIR_OVL_SIG, SAMIR_OVL_SIG_LEN)) {
        fputs("samir-size: SAMIR.OVL signature missing\n", stderr);
        return 2;
    }
    count = rd16(o + SAMIR_OVL_HDR_COUNT_OFF);
    slot = rd32(o + SAMIR_OVL_HDR_SLOT_OFF);
    if (!count || count > SAMIR_OVL_MAX ||
        (long)(SAMIR_OVL_HDR_SIZE + count * SAMIR_OVL_DIR_SIZE) > olen) {
        fputs("samir-size: SAMIR.OVL directory malformed\n", stderr);
        return 2;
    }
    for (i = 0; i < count; i++) {
        uint32_t len = rd32(o + SAMIR_OVL_HDR_SIZE + i * SAMIR_OVL_DIR_SIZE + SAMIR_OVL_DIR_LEN_OFF);
        if (len > maxlen) { maxlen = len; imax = i; }
        if (len > slot) {
            fprintf(stderr, "RED SAMIR_SLOT: overlay %u is %u B > slot %u B\n", (unsigned)i,
                    (unsigned)len, (unsigned)slot);
            return 1;
        }
    }
    if (slot != ((maxlen + 15u) & ~15u)) {
        fprintf(stderr, "RED SAMIR_SLOT: slot %u B is not the largest overlay (%u B) rounded to 16\n",
                (unsigned)slot, (unsigned)maxlen);
        return 1;
    }
    printf("samir-size: resident %u B; headroom %ld B (budget %d B); slot %u B = overlay %u (%u B) of %u; SAMIR.OVL %ld B\n",
           (unsigned)module, (long)BUDGET - (long)module, BUDGET, (unsigned)slot,
           (unsigned)imax, (unsigned)maxlen, (unsigned)count, olen);
    free(e);
    free(o);
    return 0;
}
