/* LOCKED: ADR-0009 Amendment DEC-03a (RATIFIED 2026-11-06), initech-ayj36.
 * DEC-03a.3/.4: little-endian native flat-32 model, selected by CONTENT.
 * Descriptor at 0x20: version,length,reserve,usable minimum heap,stack,guard.
 * All six fields are uint32 bytes except version. Unknown versions refuse.
 * Legacy 0x4943 retains DEC-08a; native pointers never become fake segments. */
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
