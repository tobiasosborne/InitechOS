/*
 * os/flair/resource.h -- the FLAIR Resource Manager (the artifact).
 *
 * A clean-room reader for a REAL big-endian Macintosh resource-fork SUBSET:
 * parse the documented on-disk fork structure and do type + ID lookup. FLAIR
 * does NOT ship a runtime Resource Manager (System 7 boots its windows/menus
 * programmatically); this builds the *addressing model* the era used, parsing
 * the actual on-disk format so a hand-authored fork goes in and the resource
 * bytes the tenant authored come back out.
 *
 * beads: initech-0w45 (FLAIR Phase 4.5 -- Platform Services, Resource Manager).
 *        Sibling of the Scrap (initech-b2vk); both key by the shared 4-char
 *        OSType from os/flair/ostype.h.
 *
 * GROUND TRUTH (Law 1):
 *   ../system7-decomp/specs/resources/resource-manager.md
 *     Sec 1.1  fork header (16 bytes: dataOffset/mapOffset/dataLength/mapLength,
 *              all big-endian u32).
 *     Sec 1.2  data area: each resource = 4-byte BE length prefix + data bytes;
 *              the 24-bit reference-list offset is relative to dataOffset, so
 *              abs = dataOffset + resDataOffset24.
 *     Sec 1.3  resource map: a 24-BYTE preamble = 16 (copy of fork header)
 *              + 4 (nextMapHandle) + 2 (fileRefNum) + 2 (mapAttrs); then
 *              typeListOffset (u16 BE) at MAP offset 24 and nameListOffset
 *              (u16 BE) at MAP offset 26. (24, not 22 -- the preamble counts
 *              the 16-byte header copy.)
 *     Sec 1.4  type list at mapOffset+typeListOffset: first u16 = numTypes-1;
 *              then 8-byte entries (resType OSType, resCount-1 u16,
 *              refListOffset u16 RELATIVE TO THE START OF THE TYPE LIST).
 *     Sec 1.5  reference list (per type) at typeList + refListOffset: 12-byte
 *              entries (resID i16 SIGNED, resNameOffset u16 / 0xFFFF if unnamed,
 *              resAttrs 1 byte, resDataOffset24 3-byte BE, 4 bytes reserved).
 *     Sec 2    addressing is (ResType, Int16 ID); IDs may be negative.
 *   ../system7-decomp/specs/resources/wind-menu-dlog-ditl.md Sec 1a
 *              the 24-byte WIND template layout (boundsRect/procID/visible/
 *              goAwayFlag/refCon/Pascal title) -- grounds the oracle fixture.
 *
 * ENDIAN DISCIPLINE (Law 3 / Rule 11): every multibyte field is read by
 *   POSITIONAL byte assembly (b[0]<<24 | ... | b[3]), endian-NEUTRAL, so the
 *   reader dual-compiles host(LE) + flat-32 kernel identically and an on-disk
 *   OSType compares EQUAL to a FLAIR_OSTYPE() literal on either build
 *   (ostype.h). The fork bytes are big-endian on disk; the reader never does a
 *   `char[4]` reinterpret-cast.
 *
 * MEMORY (ADR-0013 Sec 3.6): FlairResMap_load copies the source fork into the
 *   caller's arena (FLAIR_CLASS_GENERAL) -- resources are tenant DATA, read only
 *   by the tenant, so the DATA arena is correct; resource lifetime == tenant
 *   lifetime, which is why ReleaseResource is a documented no-op here.
 *
 * FAIL LOUD (Rule 2): NULL args, a truncated/malformed/internally-inconsistent
 *   fork, an out-of-bounds offset, or an OOM on the arena copy all fail loud
 *   (load returns <0; lookups return NULL). The reader bounds-checks every
 *   offset against `len` and NEVER reads past the loaded buffer.
 *
 * FREESTANDING (Law 3): <stdint.h> only; no libc (no memcpy -- a local libc-free
 *   byte copy, matching process.c/heap.c). Compiles BOTH under the kernel flags
 *   (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra -Werror) AND
 *   hosted for the property suite (harness/proptest/test_resource.c).
 *
 * ERA CEILING: this is the Mac Resource Manager model. NO Windows RT_* resource
 *   constants, NO Win resource/clipboard API names (self-policed; the check-
 *   win95isms gate is color-only and would not catch a naming Win-ism).
 *
 * Ref: CLAUDE.md Law 1 (ground truth), Law 2 (independent golden -- the oracle
 *      grades the real parser against a hand-authored fork + an independent
 *      hand-authored expected payload), Law 3 (freestanding, host==kernel),
 *      Rule 2 (fail loud), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_RESOURCE_H
#define INITECH_OS_FLAIR_RESOURCE_H

#include <stdint.h>

#include "ostype.h"   /* flair_ostype_t + FLAIR_OSTYPE (shared 4-char key)      */
#include "heap.h"     /* flair_heap_t + flair_alloc (the tenant arena)          */

/* --------------------------------------------------------------------------
 * Resource type constants -- built from the shared big-endian OSType macro.
 * (resource-manager.md Sec 1.4 "resType: 4-character resource type code".)
 * NO Windows RT_* equivalents -- this is the Mac type key space.
 * -------------------------------------------------------------------------- */
#define FLAIR_RESTYPE_WIND FLAIR_OSTYPE('W', 'I', 'N', 'D')   /* window template */
#define FLAIR_RESTYPE_MENU FLAIR_OSTYPE('M', 'E', 'N', 'U')   /* menu template   */
#define FLAIR_RESTYPE_DLOG FLAIR_OSTYPE('D', 'L', 'O', 'G')   /* dialog template */
#define FLAIR_RESTYPE_DITL FLAIR_OSTYPE('D', 'I', 'T', 'L')   /* dialog item list*/
#define FLAIR_RESTYPE_NFNT FLAIR_OSTYPE('N', 'F', 'N', 'T')   /* bitmap font     */
#define FLAIR_RESTYPE_CURS FLAIR_OSTYPE('C', 'U', 'R', 'S')   /* cursor          */

/* --------------------------------------------------------------------------
 * FlairResMap -- a flat parsed VIEW over a loaded resource fork.
 *
 * `fork` points at the arena-resident COPY of the fork (FlairResMap_load owns
 * the copy). The remaining fields are the parsed header/map offsets; lookups
 * walk the parsed structure directly against `fork`, so the view is a pure
 * function of the loaded bytes (Rule 11). NULL `fork` means "not loaded".
 * -------------------------------------------------------------------------- */
typedef struct {
    const uint8_t *fork;            /* arena-resident fork copy (NULL if unloaded)*/
    uint32_t       len;             /* total fork length in bytes                 */
    uint32_t       data_off;        /* Sec 1.1 dataOffset                         */
    uint32_t       data_len;        /* Sec 1.1 dataLength                         */
    uint32_t       map_off;         /* Sec 1.1 mapOffset                          */
    uint32_t       map_len;         /* Sec 1.1 mapLength                          */
    uint32_t       type_list_abs;   /* mapOffset + typeListOffset (Sec 1.3/1.4)   */
    uint32_t       name_list_abs;   /* mapOffset + nameListOffset (Sec 1.3/1.6)   */
    uint16_t       n_types;         /* decoded type count (numTypes-1 + 1)        */
} FlairResMap;

/* --------------------------------------------------------------------------
 * FlairResMap_load -- copy `src_fork` (length `len`) into `arena`
 * (FLAIR_CLASS_GENERAL, the tenant DATA class) and parse the fork header + map.
 *
 * Returns 0 on success, <0 on a NULL argument, an OOM on the arena copy, or a
 * truncated / out-of-bounds / internally-inconsistent fork (fail loud, Rule 2).
 * On failure `*m` is left zeroed (fork == NULL) and any arena copy is released.
 * -------------------------------------------------------------------------- */
int FlairResMap_load(FlairResMap *m, flair_heap_t *arena,
                     const uint8_t *src_fork, uint32_t len);

/* --------------------------------------------------------------------------
 * FlairGetResource -- type + ID lookup. Returns a ZERO-COPY pointer INTO the
 * loaded data area (just past the 4-byte length prefix) and sets `*out_len` to
 * the resource's byte length, or NULL if the (type, id) pair is absent (and
 * sets `*out_len` to 0 when out_len != NULL). `id` is signed (IDs may be
 * negative). Fail loud on a NULL/unloaded map or an out-of-bounds entry (NULL).
 * -------------------------------------------------------------------------- */
const uint8_t *FlairGetResource(const FlairResMap *m, flair_ostype_t type,
                                int16_t id, uint32_t *out_len);

/* --------------------------------------------------------------------------
 * FlairCountResources -- the number of resources of `type` (the stored
 * resCount-1 decoded, i.e. +1), or 0 if the type is absent / the map is unloaded.
 * -------------------------------------------------------------------------- */
int16_t FlairCountResources(const FlairResMap *m, flair_ostype_t type);

/* --------------------------------------------------------------------------
 * FlairGetResInfo -- reverse lookup: given a data pointer previously returned
 * by FlairGetResource, recover its (type, id). Returns 0 on success (and sets
 * the non-NULL out params), <0 if the pointer is not a resource in this map or
 * on a NULL/unloaded argument. (Mirrors the Mac GetResInfo call.)
 * -------------------------------------------------------------------------- */
int FlairGetResInfo(const FlairResMap *m, const uint8_t *data,
                    int16_t *id_out, flair_ostype_t *type_out);

/* --------------------------------------------------------------------------
 * FlairReleaseResource -- documented NO-OP first cut. The real Mac
 * ReleaseResource only marks a purgeable resource's handle as releasable; here
 * resource lifetime == tenant lifetime (the bytes live in the tenant arena and
 * are reclaimed at teardown), so there is nothing to release. Provided so call
 * sites read like the era API.
 * -------------------------------------------------------------------------- */
void FlairReleaseResource(const FlairResMap *m, const uint8_t *data);

#endif /* INITECH_OS_FLAIR_RESOURCE_H */
