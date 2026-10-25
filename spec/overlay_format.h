/* spec/overlay_format.h -- the LOCKED FLAIR.OVL container (Rule 8).
 *
 * Ref: ADR-0014 DEC-14-2 (container: flat sections in one file with a locked
 *      header; kernel-held CRC-32 patched after link), DEC-14-3 (fixed window,
 *      spec/memory_map.h OVERLAY_BASE/OVERLAY_CEIL), DEC-14-4 (C:\FLAIR.OVL),
 *      DEC-14-5 Alternative 5A (RATIFIED 2026-10-25: every section preloaded
 *      at desktop start, never evicted), DEC-14-6 (stubs), DEC-14-7 (named
 *      refusals); research docs/research/overlays-period-ground-truth.md Sec 1
 *      (dBASE III PLUS: raw non-MZ overlay files, whole 512 B sectors) and
 *      Sec 2.1 (Turbo Pascal 5.0: one .OVR holds every overlaid unit).
 *      Epic initech-x8gsi, lane 1.
 *
 * Honesty boundary (ADR-0014 Sec 4): this is an INITECH format, period in
 * shape only -- not a period format, and not InitechMZ (DEC-14-2).
 *
 * THE FILE IS ITS OWN MEMORY IMAGE. FLAIR.OVL is read whole into
 * [OVERLAY_BASE, OVERLAY_CEIL): file offset X lands at OVERLAY_BASE + X. The
 * header sits at OVERLAY_BASE; every section is linked at the address it
 * occupies there (OVL_SECTION_LOAD_RULE). Little-endian dwords throughout.
 *
 *   off 0   magic[8]      "FLAIROVL" (no NUL)                 OVL_MAGIC
 *   off 8   version       OVL_VERSION
 *   off 12  identity      CRC-32 of the INITDOS.SYS bytes this overlay was
 *                         linked against, taken BEFORE the kernel's expect
 *                         dwords are patched (OVL_IDENTITY_RULE)
 *   off 16  n_sections
 *   off 20  n_imports
 *   off 24  n_exports
 *   off 28  image_len     meaningful bytes (header + sections), before the
 *                         zero padding to a whole 512 B sector
 *   off 32  directory[n_sections]  (ovl_dir_t, 20 B each)
 *           imports[n_imports]     overlay -> resident: the kernel address of
 *                                  every kernel symbol the sections reference,
 *                                  in the GENERATED order (LC_ALL=C sort)
 *           exports[n_exports]     resident -> overlay: entry points, in the
 *                                  GENERATED stub order (OVL_EXPORT_RULE)
 *           crc32                  OVL_CRC_RULE
 *   then    section payloads (.text/.rodata/.data; .bss is not on disk)
 *   then    zero padding to a multiple of OVL_SECTOR
 *
 * The section directory, not the file size, carries lengths (DEC-14-2).
 */
#ifndef INITECH_SPEC_OVERLAY_FORMAT_H
#define INITECH_SPEC_OVERLAY_FORMAT_H

#include <stdint.h>

#define OVL_MAGIC           "FLAIROVL"
#define OVL_MAGIC_LEN       8u
#define OVL_VERSION         1u
#define OVL_SECTOR          512u
#define OVL_FILE_NAME83     "FLAIR.OVL"

/* Fixed header: magic, version, identity, the three counts, image_len. */
#define OVL_HDR_FIXED       32u
#define OVL_DIR_BYTES       20u

/* Section directory flags. Under DEC-14-5 Alternative 5A every section is
 * preloaded; lane 1's section carries PRELOAD explicitly (DEC-14-5 base
 * text) so a future base-text loader keeps the Bochs read-path coverage. */
#define OVL_FLAG_PRELOAD    0x0001u

/* Section ids. Append-only. Lane 1 (DEC-14-10) has ONE section, COLD: the
 * JTC easter-egg mat (os/flair/egg_jtc.c) and, its measured net not being
 * negative, About This Computer (os/milton/about_box.c + kmain_cold.c). */
#define OVL_SECTION_COLD    1u

typedef struct ovl_dir {
    uint16_t id;        /* OVL_SECTION_*                                    */
    uint16_t flags;     /* OVL_FLAG_*                                       */
    uint32_t load;      /* link == load address in the overlay window       */
    uint32_t file_off;  /* file offset of the first payload byte            */
    uint32_t length;    /* payload bytes on disk (.text+.rodata+.data)      */
    uint32_t bss_end;   /* first byte past .bss (zeroed by the kernel)      */
} ovl_dir_t;

typedef struct ovl_hdr {
    char     magic[8];
    uint32_t version;
    uint32_t identity;
    uint32_t n_sections;
    uint32_t n_imports;
    uint32_t n_exports;
    uint32_t image_len;
    /* ovl_dir_t dir[n_sections]; uint32_t imports[n_imports];
     * uint32_t exports[n_exports]; uint32_t crc32; payloads... */
} ovl_hdr_t;

_Static_assert(sizeof(ovl_dir_t) == OVL_DIR_BYTES, "ovl_dir_t is 20 bytes");
_Static_assert(sizeof(ovl_hdr_t) == OVL_HDR_FIXED, "ovl_hdr_t is 32 bytes");

/* Byte offsets of the variable tables, from the counts. */
#define OVL_IMPORTS_OFF(nsec)              (OVL_HDR_FIXED + OVL_DIR_BYTES * (nsec))
#define OVL_EXPORTS_OFF(nsec, nimp)        (OVL_IMPORTS_OFF(nsec) + 4u * (nimp))
#define OVL_CRC_OFF(nsec, nimp, nexp)      (OVL_EXPORTS_OFF(nsec, nimp) + 4u * (nexp))

/* OVL_CRC_RULE. CRC-32/IEEE 802.3 (reflected polynomial 0xEDB88320, initial
 * value 0xFFFFFFFF, final XOR 0xFFFFFFFF) over every byte of [0, image_len)
 * EXCEPT the four crc32 bytes themselves, in file order. It therefore covers
 * the header, directory, both tables and every payload: an overlay whose
 * import addresses still match but whose bytes are stale is refused
 * (DEC-14-2). The kernel compares the computed value with its OWN expected
 * value, a reserved resident dword patched after link (OVL_KERNEL_EXPECT),
 * not with the header field; the header field is for the host reader. */
#define OVL_CRC_POLY        0xEDB88320u

/* OVL_IDENTITY_RULE. identity = CRC-32 (OVL_CRC_RULE polynomial, whole file)
 * of the kernel flat binary (INITDOS.SYS, padded to KERNEL_SECTORS) the
 * overlay was linked against, with the OVL_KERNEL_EXPECT slot still holding
 * OVL_KERNEL_PLACEHOLDER. The patched kernel holds the same value in
 * expect[1]; a foreign overlay is refused FOREIGN before any byte runs. */

/* OVL_KERNEL_EXPECT: two resident dwords {crc32, identity}, assembled with
 * the placeholder below and replaced after the overlay link by the factory
 * packer (tools/flairovl.c). Exactly one placeholder may occur in a kernel
 * binary, or the packer fails loud. ASCII "OVLCRC??" "OVLIDNT?". */
#define OVL_KERNEL_PLACEHOLDER0  0x434C564Fu  /* "OVLC" */
#define OVL_KERNEL_PLACEHOLDER1  0x3F3F4352u  /* "RC??" */

/* OVL_SECTION_LOAD_RULE. For every directory entry:
 *   load == OVERLAY_BASE + file_off;   file_off >= crc32 offset + 4;
 *   file_off + length <= image_len;    load + length <= bss_end <= OVERLAY_CEIL.
 * OVL_EXPORT_RULE. Every export lies in [OVERLAY_BASE + first payload offset,
 *   OVERLAY_BASE + image_len). Exports are the symbols the resident kernel
 *   objects reference and the section objects define (generated, never
 *   hand-kept; initech-13ig). The kernel reaches them only through stubs
 *   (DEC-14-6.1/.2). */

/* Named refusals (DEC-14-7): the serial PANIC line and the PC LOAD LETTER
 * subject line name one of these. No console message (DEC-14-7 / DEC-13). */
#define OVL_WHY_MISSING     "MISSING"   /* no file, or the read failed      */
#define OVL_WHY_FOREIGN     "FOREIGN"   /* magic/version/count/identity/import */
#define OVL_WHY_EXTENTS     "EXTENTS"   /* sizes/offsets outside the window */
#define OVL_WHY_CORRUPT     "CORRUPT"   /* CRC-32 != the kernel's own value */
#define OVL_WHY_CONTEXT     "CONTEXT"   /* load from IRQ / panic / busy disk */

#endif /* INITECH_SPEC_OVERLAY_FORMAT_H */
