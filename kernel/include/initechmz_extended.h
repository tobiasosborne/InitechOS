/* LOCKED: ADR-0009 Amendment DEC-03a (RATIFIED 2026-11-06), initech-ayj36.
 * DEC-03a.3/.4: little-endian native flat-32 model, selected by CONTENT.
 * The descriptor at file offset 0x20 consists of six little-endian uint32
 * fields. Version is 1. Length, reserve, minimum usable heap, stack and
 * guard are byte counts.
 *
 * Extended tag 0x5849: e_lfarlc is at least 0x38, the end of the descriptor.
 * The exclusive table end e_lfarlc + 4 * e_crlc must be at or below
 * 16 * e_cparhdr, the declared end of the header. Both bounds apply to
 * an empty table. Tables ending exactly at the header boundary fit.
 * The loader reports MZ_ERR_BAD_RELOC for placement outside these bounds.
 *
 * Legacy tag 0x4943 retains the DEC-08a placement rules. */
#ifndef INITECHMZ_EXTENDED_H
#define INITECHMZ_EXTENDED_H
#define INITECHMZ_LEGACY_TAG 0x4943u
#define INITECHMZ_EXT_TAG 0x5849u
#define INITECHMZ_EXT_VERSION 1u
#define INITECHMZ_EXT_DESC_OFF 0x20u
#define INITECHMZ_EXT_DESC_LEN 24u
#define INITECHMZ_TAG_OFF 0x1cu
#define INITECHMZ_EXT_VERSION_OFF (INITECHMZ_EXT_DESC_OFF + 0u)
#define INITECHMZ_EXT_LENGTH_OFF  (INITECHMZ_EXT_DESC_OFF + 4u)
#define INITECHMZ_EXT_RESERVE_OFF (INITECHMZ_EXT_DESC_OFF + 8u)
#define INITECHMZ_EXT_HEAP_OFF    (INITECHMZ_EXT_DESC_OFF + 12u)
#define INITECHMZ_EXT_STACK_OFF   (INITECHMZ_EXT_DESC_OFF + 16u)
#define INITECHMZ_EXT_GUARD_OFF   (INITECHMZ_EXT_DESC_OFF + 20u)
#define INITECHMZ_EXT_HEADER_MIN 0x38u
#define INITECHMZ_EXT_STACK_BYTES 65536u
#define INITECHMZ_EXT_GUARD_BYTES 4096u
#define INITECHMZ_EXT_MODULE_MAX 262144u
#define INITECHMZ_NATIVE_HEADER_BYTES 16u
#endif
