/* ADR-0009 DEC-03a.8, initech-fxolx: RETARGET module size to 256 KiB,
 * independent footprint/heap declaration checks. RETIRE BUDGET==93776 and
 * the 4096-byte floor: conventional placement no longer bounds this image.
 * KEEP exact largest-overlay slot, e_minalloc=2058 and pair/repro gates.
 * Real-III+ size-named behavioral gates are unchanged. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "memory_map.h"
#include "samir/ovl_format.h"
#include "samir/memory_contract.h"

/* DEC-03a.3/.8 independent literal expectations, not emitter-derived. */
_Static_assert(SAMIR_MINALLOC_PARAS == 2058u && SAMIR_MODULE_MAX == 262144u &&
               SAMIR_LOADER_RESERVE == 65536u && SAMIR_MINIMUM_HEAP_BYTES == 32912u,
               "DEC-03a: initial SAMIR memory contract must be deliberately re-keyed");

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
    e = slurp(argv[1], &elen);
    o = slurp(ovlpath, &olen);
    if (!e || elen < 0x38 || !o || olen < (long)SAMIR_OVL_HDR_SIZE) {
        fputs("samir-size: missing/short SAMIR.EXE or SAMIR.OVL\n", stderr);
        return 2;
    }
    /* ---- the resident module ---- */
    if (e[0] != 'M' || e[1] != 'Z' || rd16(e + 0x1C) != 0x5849u) {
        fputs("samir-size: not extended InitechMZ (DEC-03a.8)\n", stderr);
        return 2;
    }
    image = rd16(e + 2) ? (rd16(e + 4) - 1u) * 512u + rd16(e + 2) : rd16(e + 4) * 512u;
    header = rd16(e + 8) * 16u;
    if (header < 0x38u || header > image || image > (uint32_t)elen ||
        rd16(e+2)>511u || rd16(e+4)==0u) {
        fputs("samir-size: malformed MZ header\n", stderr);
        return 2;
    }
    module = image - header;
    if (rd16(e + 0x0A) != (uint32_t)SAMIR_MINALLOC_PARAS) {
        fprintf(stderr, "samir-size: e_minalloc %u != %u (PAL arena + MCB): the loader would not enforce this budget\n",
                (unsigned)rd16(e + 0x0A), (unsigned)SAMIR_MINALLOC_PARAS);
        return 2;
    }
    /* DEC-03a.8 RETARGET: v1 descriptor and full high footprint. Literal
     * v1 expectations intentionally independent from the emitter constants. */
    uint32_t reserve=rd32(e+0x28), heap=rd32(e+0x2c);
    uint32_t reloc_end=rd16(e+0x18)+rd16(e+6)*4u;
    if(rd32(e+0x20)!=1 || rd32(e+0x24)!=24 || rd32(e+0x30)!=65536 ||
       rd32(e+0x34)!=4096 || rd16(e+0x18)<0x38u || reloc_end>header ||
       reserve!=SAMIR_LOADER_RESERVE || heap!=SAMIR_MINIMUM_HEAP_BYTES) {
        fputs("RED SAMIR_RESERVE: invalid v1 descriptor / declared reserve or heap\n",stderr);
        return 1;
    }
    if (module > 262144u) {
        fprintf(stderr, "RED SAMIR_SIZE: resident %u B > 262144 B module ceiling\n",(unsigned)module);
        return 1;
    }
    uint32_t used=(module+reserve+15u)&~15u;
    uint32_t window=EXT_APP_GUARD_BASE-EXT_APP_BASE;
    uint32_t arena=used<=window ? window-used : 0;
    if(rd16(e+0xc)!=0xffffu && arena>rd16(e+0xc)*16u) arena=rd16(e+0xc)*16u;
    if(arena<16u || heap>arena-16u || rd16(e+0xa)*16u>arena) {
        fputs("RED SAMIR_RESERVE: high footprint / usable minimum / minalloc exceeds window\n",stderr);
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
    printf("samir-size: resident %u B; headroom %u B (ceiling 262144 B); reserve %u B; usable heap capacity %u B; slot %u B = overlay %u (%u B) of %u; SAMIR.OVL %ld B\n",
           (unsigned)module,(unsigned)(262144u-module),(unsigned)reserve,(unsigned)(arena-16u),
           (unsigned)slot,(unsigned)imax,(unsigned)maxlen,(unsigned)count,olen);
    free(e);
    free(o);
    return 0;
}
