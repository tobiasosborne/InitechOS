/*
 * os/flair/resource.c -- the FLAIR Resource Manager reader (the artifact).
 *
 * Parses a REAL big-endian Mac resource-fork subset (fork header + map + type
 * list + reference lists + data area) and does (ResType, Int16 ID) lookup.
 * See resource.h for the full spec citations (resource-manager.md Sec 1.1-1.6,
 * Sec 2; wind-menu-dlog-ditl.md Sec 1a). Every multibyte field is read by
 * POSITIONAL byte assembly so the reader is endian-neutral (host==kernel) and
 * an on-disk OSType compares EQUAL to a FLAIR_OSTYPE() literal.
 *
 * FAIL LOUD (Rule 2): every offset is bounds-checked against `len`; a truncated
 * or out-of-bounds or internally-inconsistent fork fails loud (load <0, lookups
 * NULL). FREESTANDING (Law 3): <stdint.h> only; no libc -- a local libc-free
 * byte copy/zero, matching process.c (proc_zero) and heap.c discipline.
 *
 * Two #ifdef-guarded SELF-MUTANTS prove the oracle bites (Rule 6):
 *   RES_MUT_IGNORE_TYPE      -- the type-list match ignores the 4-char type key
 *                               (matches the first type regardless).
 *   RES_MUT_COUNT_OFF_BY_ONE -- the type-list walk iterates numTypes-1 entries
 *                               instead of numTypes (drops the last type).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include "resource.h"

#include <stdint.h>
#include <stddef.h>   /* NULL (freestanding-safe; window.h/menu.h convention) */

/* ==========================================================================
 * Positional big-endian readers (endian-NEUTRAL; host==kernel -- Law 3).
 * The fork is big-endian on disk; assemble from bytes, never reinterpret-cast.
 * ==========================================================================*/
static uint16_t rd_u16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}
static int16_t rd_i16(const uint8_t *p)
{
    /* resID is a SIGNED Int16 (Sec 1.5 / Sec 2 -- IDs may be negative). */
    return (int16_t)rd_u16(p);
}
static uint32_t rd_u24(const uint8_t *p)
{
    /* the 24-bit big-endian resDataOffset24 (Sec 1.2/1.5). */
    return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];
}
static uint32_t rd_u32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}
static flair_ostype_t rd_ostype(const uint8_t *p)
{
    /* the 4-char OSType packed big-endian -- equals FLAIR_OSTYPE(a,b,c,d). */
    return (flair_ostype_t)rd_u32(p);
}

/* ==========================================================================
 * Libc-free byte helpers (Law 3 -- no memcpy/memset dependency).
 * ==========================================================================*/
static void res_zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) b[i] = 0u;
}
static void res_copy(uint8_t *dst, const uint8_t *src, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) dst[i] = src[i];
}

/* ==========================================================================
 * find_type -- walk the type list (Sec 1.4) for `type`; on a match, hand back
 * the resCount-1 and refListOffset fields and return a pointer to the 8-byte
 * type entry; else NULL. Bounds-checked (Rule 2).
 *
 * BOTH self-mutants live here so they perturb every type-keyed call (GetResource
 * and CountResources) consistently.
 * ==========================================================================*/
static const uint8_t *find_type(const FlairResMap *m, flair_ostype_t type,
                                uint16_t *count_m1_out, uint16_t *reflist_off_out)
{
    uint16_t ntypes = m->n_types;          /* = numTypes-1 + 1 (decoded in load) */

#ifdef RES_MUT_COUNT_OFF_BY_ONE
    /* MUTANT: iterate (numTypes-1) entries instead of numTypes -> the LAST type
     * is never visited. Proves the count-1 decode (+1) is load-bearing. */
    uint16_t limit = (ntypes > 0u) ? (uint16_t)(ntypes - 1u) : 0u;
#else
    uint16_t limit = ntypes;
#endif

    for (uint16_t i = 0; i < limit; i++) {
        uint32_t e = m->type_list_abs + 2u + (uint32_t)i * 8u;  /* skip numTypes-1 */
        if (e + 8u > m->len) return NULL;                       /* fail-loud OOB  */

        flair_ostype_t et   = rd_ostype(m->fork + e);
        uint16_t       cm1  = rd_u16(m->fork + e + 4u);
        uint16_t       rlo  = rd_u16(m->fork + e + 6u);

#ifdef RES_MUT_IGNORE_TYPE
        /* MUTANT: ignore the type key -- match the FIRST type regardless. Proves
         * the 4-char type comparison is load-bearing (wrong-type lookups must
         * miss; cross-type lookups must not leak the first type's resources). */
        (void)et;
        (void)type;   /* the discarded key (keeps the mutant -Werror-clean) */
        int matched = 1;
#else
        int matched = (et == type);
#endif
        if (matched) {
            if (count_m1_out)    *count_m1_out = cm1;
            if (reflist_off_out) *reflist_off_out = rlo;
            return m->fork + e;
        }
    }
    return NULL;
}

/* ==========================================================================
 * FlairResMap_load -- copy + parse (resource.h contract).
 * ==========================================================================*/
int FlairResMap_load(FlairResMap *m, flair_heap_t *arena,
                     const uint8_t *src_fork, uint32_t len)
{
    if (m == NULL || arena == NULL || src_fork == NULL) return -1;
    res_zero(m, (uint32_t)sizeof *m);
    if (len < 16u) return -1;              /* need the 16-byte fork header (1.1) */

    /* Copy the fork into the tenant DATA arena (ADR-0013 Sec 3.6). Fail loud on
     * OOM -- never parse the caller's transient buffer in place. */
    uint8_t *copy = (uint8_t *)flair_alloc(arena, FLAIR_CLASS_GENERAL, len);
    if (copy == NULL) return -1;
    res_copy(copy, src_fork, len);
    m->fork = copy;
    m->len  = len;

    /* Fork header, Sec 1.1 (all big-endian u32). */
    m->data_off = rd_u32(copy + 0);
    m->map_off  = rd_u32(copy + 4);
    m->data_len = rd_u32(copy + 8);
    m->map_len  = rd_u32(copy + 12);

    /* Bounds the data area + map inside the fork (overflow-safe subtraction). */
    if (m->data_off > len || m->map_off > len)              goto bad;
    if (m->data_len > len - m->data_off)                    goto bad;
    if (m->map_len  > len - m->map_off)                     goto bad;

    /* The 24-byte preamble + the 2 offset words at MAP offsets 24..27 (Sec 1.3).
     * (24, NOT 22: the preamble counts the 16-byte fork-header copy.) */
    if (m->map_off + 28u > len)                             goto bad;
    {
        uint16_t tlo = rd_u16(copy + m->map_off + 24u);     /* typeListOffset    */
        uint16_t nlo = rd_u16(copy + m->map_off + 26u);     /* nameListOffset    */
        m->type_list_abs = m->map_off + (uint32_t)tlo;
        m->name_list_abs = m->map_off + (uint32_t)nlo;
    }

    /* The numTypes-1 word at the start of the type list (Sec 1.4). */
    if (m->type_list_abs > len || len - m->type_list_abs < 2u) goto bad;
    m->n_types = (uint16_t)(rd_u16(copy + m->type_list_abs) + 1u);
    return 0;

bad:
    flair_free(arena, FLAIR_CLASS_GENERAL, copy);   /* release the failed copy   */
    res_zero(m, (uint32_t)sizeof *m);               /* leave fork == NULL        */
    return -1;
}

/* ==========================================================================
 * FlairGetResource -- type + ID lookup (resource.h contract).
 * ==========================================================================*/
const uint8_t *FlairGetResource(const FlairResMap *m, flair_ostype_t type,
                                int16_t id, uint32_t *out_len)
{
    if (out_len) *out_len = 0;
    if (m == NULL || m->fork == NULL) return NULL;

    uint16_t cm1 = 0, rlo = 0;
    const uint8_t *te = find_type(m, type, &cm1, &rlo);
    if (te == NULL) return NULL;                 /* type absent (or mutant-dropped)*/

    uint32_t count = (uint32_t)cm1 + 1u;         /* resCount-1 decode (Sec 1.4)   */
    uint32_t rl    = m->type_list_abs + (uint32_t)rlo;  /* ref list (Sec 1.5)     */

    for (uint32_t j = 0; j < count; j++) {
        uint32_t r = rl + j * 12u;               /* 12-byte ref entries (Sec 1.5) */
        if (r + 12u > m->len) return NULL;       /* fail-loud OOB                 */

        int16_t rid = rd_i16(m->fork + r);       /* signed resID                  */
        if (rid == id) {
            uint32_t do24 = rd_u24(m->fork + r + 5u);     /* 24-bit data offset   */
            uint32_t abs  = m->data_off + do24;           /* abs = dataOff + off  */
            if (abs > m->len || m->len - abs < 4u) return NULL;   /* prefix room  */
            uint32_t dlen = rd_u32(m->fork + abs);         /* 4-byte length prefix */
            if (dlen > m->len - (abs + 4u)) return NULL;   /* data room (no wrap)  */
            if (out_len) *out_len = dlen;
            return m->fork + abs + 4u;                     /* zero-copy data ptr   */
        }
    }
    return NULL;                                  /* id absent in this type        */
}

/* ==========================================================================
 * FlairCountResources -- resource count for `type` (resource.h contract).
 * ==========================================================================*/
int16_t FlairCountResources(const FlairResMap *m, flair_ostype_t type)
{
    if (m == NULL || m->fork == NULL) return 0;
    uint16_t cm1 = 0;
    const uint8_t *te = find_type(m, type, &cm1, NULL);
    if (te == NULL) return 0;
    return (int16_t)((int32_t)cm1 + 1);           /* resCount-1 + 1 (Sec 1.4)      */
}

/* ==========================================================================
 * FlairGetResInfo -- reverse (type, id) lookup from a data pointer.
 * (Not perturbed by the self-mutants; uses the full type count.)
 * ==========================================================================*/
int FlairGetResInfo(const FlairResMap *m, const uint8_t *data,
                    int16_t *id_out, flair_ostype_t *type_out)
{
    if (m == NULL || m->fork == NULL || data == NULL) return -1;

    for (uint16_t i = 0; i < m->n_types; i++) {
        uint32_t e = m->type_list_abs + 2u + (uint32_t)i * 8u;
        if (e + 8u > m->len) return -1;

        flair_ostype_t et  = rd_ostype(m->fork + e);
        uint16_t       cm1 = rd_u16(m->fork + e + 4u);
        uint16_t       rlo = rd_u16(m->fork + e + 6u);
        uint32_t       cnt = (uint32_t)cm1 + 1u;
        uint32_t       rl  = m->type_list_abs + (uint32_t)rlo;

        for (uint32_t j = 0; j < cnt; j++) {
            uint32_t r = rl + j * 12u;
            if (r + 12u > m->len) return -1;
            int16_t  rid  = rd_i16(m->fork + r);
            uint32_t do24 = rd_u24(m->fork + r + 5u);
            uint32_t abs  = m->data_off + do24;
            if (abs > m->len || m->len - abs < 4u) return -1;
            if (m->fork + abs + 4u == data) {
                if (id_out)   *id_out = rid;
                if (type_out) *type_out = et;
                return 0;
            }
        }
    }
    return -1;
}

/* ==========================================================================
 * FlairReleaseResource -- documented NO-OP first cut (resource.h contract).
 * ==========================================================================*/
void FlairReleaseResource(const FlairResMap *m, const uint8_t *data)
{
    (void)m;
    (void)data;
}
