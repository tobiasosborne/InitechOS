/* os/flair/asset_lz.h -- the kernel's decoder for packed strike STORAGE
 * (kernel size round 5, WL-0168). Freestanding C: no heap, no libc.
 *
 * Ref: tools/asset_lz.c (the factory packer; the format below is its
 *      contract); the round-4 chicago_packed.h precedent: a hosted build keeps
 *      reading the LOCKED spec/assets tables directly, only the freestanding
 *      kernel reads the packed copy, and harness/proptest/test_asset_lz.c
 *      grades THIS decoder against the unchanged locked bytes (Rule 8). The
 *      historical pixel gates still grade the strikes themselves (Law 2).
 *
 * FORMAT (LZSS): a flag byte announces up to eight items, least significant
 * bit first. A 1 bit is one literal byte. A 0 bit is a two-byte
 * little-endian word w: copy (w & 15) + 3 bytes from (w >> 4) + 1 bytes back
 * in the OUTPUT (overlap allowed, byte by byte). Decoding stops after exactly
 * n output bytes; the packer never emits an item that crosses n.
 *
 * MUTANT ASSET_LZ_MUT_BYTE (Rule 6; test-asset-lz-mutant): the 100th output
 * byte is flipped -- the equality gate must go RED. NEVER in a real build.
 */
#ifndef INITECH_ASSET_LZ_H
#define INITECH_ASSET_LZ_H

#include <stdint.h>

/* noinline: every TU that unpacks carries this same body, and the kernel's
 * LTO identical-code folding keeps one copy instead of one per call site. */
static __attribute__((noinline, unused)) void asset_lz_unpack(uint8_t *dst,
                                                              uint32_t n,
                                                              const uint8_t *src)
{
    uint8_t *end = dst + n;
    unsigned flags = 0u;

    while (dst < end) {
        if ((flags & 0x100u) == 0u)
            flags = 0xFF00u | *src++;
        if (flags & 1u) {
            *dst++ = *src++;
        } else {
            unsigned w = (unsigned)src[0] | ((unsigned)src[1] << 8);
            const uint8_t *from = dst - (w >> 4) - 1u;
            unsigned len = (w & 15u) + 3u;
            src += 2;
            while (len-- != 0u)
                *dst++ = *from++;
        }
        flags >>= 1;
    }
#if defined(ASSET_LZ_MUT_BYTE)
    if (n > 100u) end[100 - (int)n] ^= 1u;
#endif
}

#endif /* INITECH_ASSET_LZ_H */
