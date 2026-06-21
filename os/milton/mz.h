/* mz.h -- InitechMZ header-parse + flat-relocation-apply API.
 *
 * beads: initech-dtw.1
 * Ref:   docs/adr/ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md
 *          DEC-08a.1: container is real MZ; code inside is flat 32-bit;
 *                     relocations are flat (add load_base to 32-bit dword).
 *          DEC-08a.2: PROGRAM_IMAGE (0x40100) is the flat load base.
 *          DEC-08a.4: dispatch by content (first 2 bytes 'MZ'/'ZM'), not ext.
 *          DEC-08a.5: tag word at e_res[0] (offset 0x1C); untagged -> PANIC.
 *        spec/memory_map.h (PROGRAM_BASE, PROGRAM_IMAGE constants).
 *        CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Law 3
 *        (artifact = C), Rule 2 (fail loud), Rule 6 (mutation-proven),
 *        Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 * Pure logic -- no asm, no I/O -- host-testable by #include "mz.c".
 *
 * MUTATION hooks (CLAUDE.md Rule 6; guarded by #ifdef in mz.c):
 *   MZ_MUTATE_RELOC_NOADD     -- mz_apply_relocs skips the += load_base
 *                                 (leaves dword unchanged) -> reloc oracle RED.
 *   MZ_MUTATE_RELOC_PARAGRAPH -- add (load_base >> 4) instead of load_base
 *                                 -> reloc oracle RED.
 *   MZ_MUTATE_ACCEPT_FOREIGN  -- mz_parse_header treats untagged MZ as OK
 *                                 -> fail-loud-foreign oracle RED.
 */
#ifndef INITECH_MZ_H
#define INITECH_MZ_H

#include <stdint.h>

/* MZ_INITECH_TAG -- the sentinel word written at e_res[0] (MZ header offset
 * 0x1C) by the InitechOS toolchain/Turbo Initech to mark a flat-32 InitechMZ
 * executable.  Genuine period linkers write 0 at this reserved field; a
 * nonzero value here is our "flat-32 relocations" flag.
 *
 * Value 0x4943 == 'I','C' in little-endian (byte at 0x1C = 0x43 'C',
 * byte at 0x1D = 0x49 'I').  Mnemonic: "Initech Corporation".
 * Ref: DEC-08a.5.  LOCKED in this header; dtw.2 (loader.c) reads it via
 * mz_parse_header() and needs no separate definition. */
#define MZ_INITECH_TAG  ((uint16_t)0x4943u)

/* mz_image_t -- the parsed, normalised view of an InitechMZ image.
 * All offsets are flat byte offsets from the start of the MZ file unless
 * otherwise noted.  Computed by mz_parse_header(); consumed by the caller
 * (loader.c dtw.2) to:
 *   1. Copy file[load_module_off .. +load_module_len) to PROGRAM_IMAGE.
 *   2. Call mz_apply_relocs with image=PROGRAM_IMAGE, reloc_table from file,
 *      load_base=PROGRAM_IMAGE (0x40100).
 *   3. Jump to PROGRAM_IMAGE + entry_off (flat entry).
 *   4. Set ESP = PROGRAM_IMAGE + stack_off (or PROGRAM_STACK_TOP if cs==ss==ip==sp==0).
 * Ref: DEC-08a.1, DEC-08a.2. */
typedef struct {
    uint32_t load_module_off;   /* byte offset into the file of the load module */
    uint32_t load_module_len;   /* byte count of the load module                */
    uint32_t reloc_table_off;   /* byte offset into the file of the reloc table */
    uint16_t reloc_count;       /* number of 4-byte reloc entries               */
    uint32_t entry_off;         /* flat offset from load base: e_cs*16 + e_ip   */
    uint32_t stack_off;         /* flat offset from load base: e_ss*16 + e_sp   */
    uint16_t min_alloc_paras;   /* e_minalloc (paragraphs; DEC-08a.3)           */
    uint16_t max_alloc_paras;   /* e_maxalloc (paragraphs; DEC-08a.3)           */
    int      is_initechmz;      /* 1 -- always set on MZ_OK (sentinel verified) */
} mz_image_t;

/* mz_status -- return codes for mz_parse_header() and mz_apply_relocs().
 * Fail-loud (Rule 2): each error is distinct; the caller must act on each. */
typedef enum {
    MZ_OK            = 0,  /* success                                           */
    MZ_ERR_NOT_MZ       ,  /* magic bytes are not 'MZ' or 'ZM'                 */
    MZ_ERR_FOREIGN      ,  /* valid MZ magic but e_res[0] != MZ_INITECH_TAG     */
                           /*   (a genuine 16-bit DOS .EXE -- PANIC in loader)  */
    MZ_ERR_TRUNCATED    ,  /* file_len < header or < image_bytes                */
    MZ_ERR_BAD_HEADER   ,  /* header_bytes > image_bytes or computed underflow  */
    MZ_ERR_BAD_RELOC    ,  /* reloc table or a reloc offset runs past bounds    */
} mz_status_t;

/* ---- API ----------------------------------------------------------------- */

/* mz_is_mz -- content-based dispatch probe (DEC-08a.4).
 * Returns 1 if the first two bytes of `file` are 0x4D,0x5A ('MZ') or the
 * byte-swap alias 0x5A,0x4D ('ZM'); 0 otherwise.  Callers: loader_run, the
 * AH=4Bh EXEC path.  Does NOT check the tag word -- use mz_parse_header for
 * full validation. */
int mz_is_mz(const uint8_t *file, uint32_t len);

/* mz_parse_header -- validate and parse the MZ header (DEC-08a.1, DEC-08a.5).
 *
 * On success (MZ_OK) fills *out with the normalised image description.
 * On any failure the status code is returned and *out is left unspecified;
 * the caller MUST NOT use *out.
 *
 * Validation chain (fail loud, Rule 2):
 *   MZ_ERR_NOT_MZ      : magic != 'MZ' and != 'ZM'.
 *   MZ_ERR_FOREIGN     : magic OK but e_res[0] (offset 0x1C) != MZ_INITECH_TAG.
 *                        A genuine 16-bit DOS .EXE.  The kernel loader will
 *                        PANIC on this (DEC-08a.5).
 *   MZ_ERR_TRUNCATED   : file_len < 0x1E (minimum header to read tag) OR
 *                        file_len < header_bytes OR file_len < image_bytes.
 *   MZ_ERR_BAD_HEADER  : header_bytes > image_bytes, or arithmetic underflow
 *                        in load_module_len.
 *   MZ_ERR_BAD_RELOC   : reloc_table_off + reloc_count*4 > file_len.
 *
 * Computations (per IBM DOS Technical Reference / DEC-08a.1):
 *   image_bytes       = (e_cblp == 0) ? (uint32_t)e_cp * 512
 *                                     : ((uint32_t)(e_cp - 1) * 512 + e_cblp)
 *   header_bytes      = (uint32_t)e_cparhdr * 16
 *   load_module_off   = header_bytes
 *   load_module_len   = image_bytes - header_bytes
 *   entry_off         = (uint32_t)e_cs * 16 + e_ip   (flat from load base)
 *   stack_off         = (uint32_t)e_ss * 16 + e_sp   (flat from load base)
 *
 * `file` must point to the full file in memory; `file_len` is the byte count. */
int mz_parse_header(const uint8_t *file, uint32_t file_len, mz_image_t *out);

/* mz_apply_relocs -- apply flat-32 relocations to the load module (DEC-08a.1).
 *
 * For each reloc entry i in [0, reloc_count):
 *   off = LE32(reloc_table + i*4)
 *   bounds-check: off + 4 <= image_len, else return MZ_ERR_BAD_RELOC (FAIL
 *   LOUD -- do NOT write out of bounds, Rule 2).
 *   read LE32 dword at image[off], add load_base, write back LE32.
 *   The add is to the full 32-bit flat dword (not a paragraph add).
 *
 * `image`       -- the loaded module buffer (writable, base is PROGRAM_IMAGE).
 * `image_len`   -- byte count of the load module buffer.
 * `load_base`   -- flat linear load base (PROGRAM_IMAGE, 0x40100).
 * `reloc_table` -- pointer to the raw reloc table bytes from the MZ file.
 * `reloc_count` -- number of 4-byte reloc entries.
 *
 * Returns MZ_OK on success, MZ_ERR_BAD_RELOC on any OOB reloc offset. */
int mz_apply_relocs(uint8_t *image, uint32_t image_len,
                    uint32_t load_base,
                    const uint8_t *reloc_table, uint16_t reloc_count);

#endif /* INITECH_MZ_H */
