/* mz.c -- InitechMZ header-parse + flat-relocation-apply (pure logic unit).
 *
 * beads: initech-dtw.1
 * Ref:   docs/adr/ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md
 *          DEC-08a.1: container real MZ; code flat 32-bit; relocs flat dword.
 *          DEC-08a.4: dispatch by content bytes 0-1 ('MZ'/'ZM').
 *          DEC-08a.5: e_res[0] (offset 0x1C) == MZ_INITECH_TAG required;
 *                     untagged MZ -> MZ_ERR_FOREIGN (kernel will PANIC).
 *        kernel/include/memory_map.h: PROGRAM_IMAGE (0x40100) is the flat load base.
 *        IBM DOS 3.3 Technical Reference (MZ header layout / field semantics).
 *        the repository contract, the repository contract, the repository contract (artifact = C, freestanding),
 *        the repository contract (fail loud), the repository contract (mutation-proven),
 *        the repository contract (deterministic), the repository contract (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 * No libc.  Host-testable by #include "mz.c" in test_mz.c (same TU trick as
 * env.c / test_env.c).
 *
 * MUTATION hooks (the repository contract) -- guarded by #ifdef so clean builds are
 * unaffected; activated by -DMZ_MUTATE_... on the gcc command line:
 *   MZ_MUTATE_RELOC_NOADD     : mz_apply_relocs skips += load_base.
 *   MZ_MUTATE_RELOC_PARAGRAPH : mz_apply_relocs adds (load_base >> 4).
 *   MZ_MUTATE_ACCEPT_FOREIGN  : mz_parse_header skips the tag check.
 */

#include "mz.h"

/* ---- freestanding LE read/write helpers (byte-wise; alignment-safe) ------ */

/* Read a 16-bit little-endian word from `p`. */
static uint16_t mz_rd16(const uint8_t *p)
{
    return (uint16_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8));
}

/* Read a 32-bit little-endian dword from `p`. */
static uint32_t mz_rd32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] <<  8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

/* Write a 32-bit little-endian dword to `p`. */
#ifndef MZ_ONLY_PARSER
static void mz_wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v        & 0xFFu);
    p[1] = (uint8_t)((v >>  8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

#endif

/* ---- MZ header field offsets (from the start of the file) ---------------- */
/* Ref: IBM DOS 3.3 Technical Reference / standard MZ layout. */
#define MZ_OFF_MAGIC     0x00u   /* 2 bytes: 'MZ' or 'ZM'                 */
#define MZ_OFF_CBLP      0x02u   /* 2 bytes: bytes on last page (0 = full) */
#define MZ_OFF_CP        0x04u   /* 2 bytes: total 512-byte pages          */
#define MZ_OFF_CRLC      0x06u   /* 2 bytes: relocation entry count        */
#define MZ_OFF_CPARHDR   0x08u   /* 2 bytes: header size in paragraphs     */
#define MZ_OFF_MINALLOC  0x0Au   /* 2 bytes: minimum extra paragraphs      */
#define MZ_OFF_MAXALLOC  0x0Cu   /* 2 bytes: maximum extra paragraphs      */
#define MZ_OFF_SS        0x0Eu   /* 2 bytes: initial SS (segment relative) */
#define MZ_OFF_SP        0x10u   /* 2 bytes: initial SP                    */
#define MZ_OFF_CSUM      0x12u   /* 2 bytes: checksum (ignored)            */
#define MZ_OFF_IP        0x14u   /* 2 bytes: initial IP                    */
#define MZ_OFF_CS        0x16u   /* 2 bytes: initial CS (segment relative) */
#define MZ_OFF_LFARLC    0x18u   /* 2 bytes: file offset of reloc table    */
#define MZ_OFF_OVNO      0x1Au   /* 2 bytes: overlay number                */
#define MZ_OFF_RES0      INITECHMZ_TAG_OFF   /* 2 bytes: e_res[0] -- InitechMZ TAG     */

/* Minimum number of bytes we must read to check the tag (through offset 0x1D). */
#define MZ_MIN_HDR_READ  0x1Eu

/* ---- API implementations ------------------------------------------------- */

#ifndef MZ_ONLY_PARSER
int mz_is_mz(const uint8_t *file, uint32_t len)
{
    if (file == (const uint8_t *)0 || len < 2u) {
        return 0;
    }
    /* 'MZ' (0x4D 0x5A) -- the canonical DOS EXE magic.
     * 'ZM' (0x5A 0x4D) -- the byte-swap alias (DEC-08a.4). */
    if (file[0] == 0x4Du && file[1] == 0x5Au) { return 1; }
    if (file[0] == 0x5Au && file[1] == 0x4Du) { return 1; }
    return 0;
}

#endif

#ifndef MZ_PARSE_EXTERNAL
int mz_parse_header(const uint8_t *file, uint32_t file_len, mz_image_t *out)
{
    uint16_t e_cblp, e_cp, e_crlc, e_cparhdr;
    uint16_t e_minalloc, e_maxalloc;
    uint16_t e_ss, e_sp, e_ip, e_cs, e_lfarlc;
    uint16_t e_res0;
    uint32_t image_bytes, header_bytes, reloc_end;

    if (out == (mz_image_t *)0) {
        return MZ_ERR_BAD_HEADER;   /* fail loud -- null output pointer */
    }

    /* 1. Magic check (DEC-08a.4). */
    if (!mz_is_mz(file, file_len)) {
        return MZ_ERR_NOT_MZ;
    }

    /* 2. Need enough bytes to read the tag at 0x1C..0x1D. */
    if (file_len < MZ_MIN_HDR_READ) {
        return MZ_ERR_TRUNCATED;
    }

    /* 3. Read the tag word (DEC-08a.5). */
    e_res0 = mz_rd16(file + MZ_OFF_RES0);

    /* CLEAN BUILD: enforce the InitechMZ tag.  An untagged MZ is a genuine
     * 16-bit DOS .EXE with paragraph fixups over 16-bit code -- running it
     * in 32-bit flat mode would silently misinterpret the instruction stream.
     * The kernel loader PANICS on MZ_ERR_FOREIGN (DEC-08a.5, the repository contract). */
    if (e_res0 != MZ_INITECH_TAG && e_res0 != INITECHMZ_EXT_TAG) {
        return MZ_ERR_FOREIGN;
    }

    /* 4. Read the header fields we need. */
    e_cblp      = mz_rd16(file + MZ_OFF_CBLP);
    e_cp        = mz_rd16(file + MZ_OFF_CP);
    e_crlc      = mz_rd16(file + MZ_OFF_CRLC);
    e_cparhdr   = mz_rd16(file + MZ_OFF_CPARHDR);
    e_minalloc  = mz_rd16(file + MZ_OFF_MINALLOC);
    e_maxalloc  = mz_rd16(file + MZ_OFF_MAXALLOC);
    e_ss        = mz_rd16(file + MZ_OFF_SS);
    e_sp        = mz_rd16(file + MZ_OFF_SP);
    e_ip        = mz_rd16(file + MZ_OFF_IP);
    e_cs        = mz_rd16(file + MZ_OFF_CS);
    e_lfarlc    = mz_rd16(file + MZ_OFF_LFARLC);

    /* 5. Compute image_bytes (DEC-08a.1 / IBM DOS 3.3 TR):
     *    if e_cblp == 0, all e_cp pages are full (last page is 512 bytes).
     *    else the last page holds only e_cblp bytes.
     * Use uint32_t arithmetic throughout to avoid 16-bit overflow. */
    if (e_cblp == 0u) {
        image_bytes = (uint32_t)e_cp * 512u;
    } else {
        if (e_cp == 0u) {
            /* e_cp must be >= 1 when e_cblp != 0 (last-page model). */
            return MZ_ERR_BAD_HEADER;
        }
        image_bytes = ((uint32_t)(e_cp - 1u) * 512u) + (uint32_t)e_cblp;
    }

    /* 6. Compute header_bytes. */
    header_bytes = (uint32_t)e_cparhdr * 16u;

    /* 7. Header-vs-image sanity FIRST (the repository contract).
     * header_bytes > image_bytes is a structural inconsistency in the MZ
     * header itself (not a file-truncation issue): the declared header size
     * exceeds the declared total image size, which makes load_module_len
     * underflow.  Catch this before the file-length truncation checks so
     * a file that is also short reports the more informative error. */
    if (header_bytes > image_bytes) {
        return MZ_ERR_BAD_HEADER;
    }

    /* 8. Truncation checks (the repository contract -- fail loud on any short read). */
    if (file_len < header_bytes) {
        return MZ_ERR_TRUNCATED;
    }
    if (file_len < image_bytes) {
        return MZ_ERR_TRUNCATED;
    }

    /* 9. Relocation table bounds check (the repository contract).
     * reloc_table_off + reloc_count * 4 must not exceed file_len. */
    reloc_end = (uint32_t)e_lfarlc + (uint32_t)e_crlc * 4u;
    if (reloc_end > file_len) {
        return MZ_ERR_BAD_RELOC;
    }

    /* ADR-0009 DEC-03a.3/.4, initech-jarzu. The caller must supply the
     * first min(file_len,56) bytes, even when streaming a large module. */
    out->extended = e_res0 == INITECHMZ_EXT_TAG;
    out->reserve_bytes = out->minimum_heap_bytes = 0;
    if (out->extended) {
        if (file_len < INITECHMZ_EXT_HEADER_MIN || header_bytes < INITECHMZ_EXT_HEADER_MIN ||
            e_cblp > 511u || !e_cp ||
            mz_rd32(file+INITECHMZ_EXT_VERSION_OFF) != INITECHMZ_EXT_VERSION ||
            mz_rd32(file+INITECHMZ_EXT_LENGTH_OFF) != INITECHMZ_EXT_DESC_LEN ||
            mz_rd32(file+INITECHMZ_EXT_STACK_OFF) != INITECHMZ_EXT_STACK_BYTES ||
            mz_rd32(file+INITECHMZ_EXT_GUARD_OFF) != INITECHMZ_EXT_GUARD_BYTES)
            return MZ_ERR_BAD_HEADER;
        if (e_lfarlc < INITECHMZ_EXT_HEADER_MIN || reloc_end > header_bytes)
            return MZ_ERR_BAD_RELOC;
        out->reserve_bytes = mz_rd32(file+INITECHMZ_EXT_RESERVE_OFF);
        out->minimum_heap_bytes = mz_rd32(file+INITECHMZ_EXT_HEAP_OFF);
    }

    /* 10. Fill output. */
    out->load_module_off  = header_bytes;
    out->load_module_len  = image_bytes - header_bytes;
    out->reloc_table_off  = (uint32_t)e_lfarlc;
    out->reloc_count      = e_crlc;
    out->entry_off        = (uint32_t)e_cs * 16u + (uint32_t)e_ip;
    out->stack_off        = (uint32_t)e_ss * 16u + (uint32_t)e_sp;
    out->min_alloc_paras  = e_minalloc;
    out->max_alloc_paras  = e_maxalloc;
    out->is_initechmz     = 1;

    return MZ_OK;
}

#endif

#ifndef MZ_ONLY_PARSER
int mz_apply_relocs(uint8_t *image, uint32_t image_len,
                    uint32_t load_base,
                    const uint8_t *reloc_table, uint16_t reloc_count)
{
    uint16_t i;

    /* Null-pointer guard (the repository contract). */
    if (image == (uint8_t *)0 || (reloc_count > 0u && reloc_table == (const uint8_t *)0)) {
        return MZ_ERR_BAD_RELOC;
    }

    for (i = 0u; i < reloc_count; i++) {
        uint32_t off;
        uint32_t old_val, new_val;

        /* Read the 32-bit flat offset of this reloc entry (LE32 from the
         * reloc table; each entry is 4 bytes). */
        off = mz_rd32(reloc_table + (uint32_t)i * 4u);

        /* Bounds-check: the dword at `off` must lie entirely within the
         * load module (fail loud -- NEVER write OOB, the repository contract). */
        if (off + 4u > image_len || off + 4u < off) {
            /* The second condition catches the uint32_t overflow case where
             * off is near UINT32_MAX. */
            return MZ_ERR_BAD_RELOC;
        }

        /* Read, add load_base, write back (byte-wise for alignment safety
         * and host portability; DEC-08a.1). */
        old_val = mz_rd32(image + off);

        /* CLEAN BUILD: flat-32 relocation -- add the flat load base to the
         * 32-bit dword at the reloc site (DEC-08a.1). */
        new_val = old_val + load_base;

        mz_wr32(image + off, new_val);
    }

    return MZ_OK;
}

#endif
