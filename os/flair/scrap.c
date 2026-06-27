/*
 * os/flair/scrap.c -- the FLAIR Scrap Manager implementation -- THE ARTIFACT.
 *
 * beads: initech-b2vk (FLAIR Phase 4.5 -- Platform Services: Scrap).
 * Ref:   scrap.h (the full contract + heap-seam rationale); ADR-0013 Sec 6
 *        (shell-owned cross-tenant Scrap); Inside Macintosh Vol I Scrap Mgr.
 *        CLAUDE.md Law 1 (ground truth before code), Law 2 (oracle is truth),
 *        Law 3 (freestanding artifact; no libc, no malloc; dual-compile),
 *        Rule 2 (fail loud), Rule 6 (mutants: each #ifdef MUST compile and go
 *        RED), Rule 11 (deterministic), Rule 12 (ASCII-clean source).
 *
 * MUTANT GUARDS (Rule 6 -- mutation-proven oracle):
 *   SCRAP_MUT_IGNORE_FLAVOR  -- ignore the type key in flavor lookup; match
 *       the first flavor regardless. PutScrap(PICT) then overwrites the TEXT
 *       slot, so GetScrap(TEXT) returns the PICT bytes -> step 3 RED.
 *   SCRAP_MUT_NO_INCREMENT   -- FlairZeroScrap skips scrapCount++.
 *       FlairInfoScrap returns 1 instead of 2 after two ZeroScraps -> RED.
 *   SCRAP_MUT_NO_CLEAR       -- FlairZeroScrap bumps the count but leaves
 *       n_flavors unchanged. GetScrap after ZeroScrap returns stale -> RED.
 *
 * ERA CEILING (Law 3): No Win32 clipboard names (CF_TEXT / CF_BITMAP /
 *   OpenClipboard / EmptyClipboard / SetClipboardData). Mac Scrap Manager
 *   model only.
 *
 * FREESTANDING (Law 3): <stdint.h> only (via scrap.h). No string.h / libc /
 *   malloc. Byte operations are provided by the internal scrap_memcpy helper.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stddef.h>   /* NULL -- freestanding-safe (C11 S4p6 required header) */
#include "scrap.h"

/* ---------------------------------------------------------------------------
 * Fail-loud dual-compile pattern (mirrors os/flair/process.c PROC_PANIC and
 * os/flair/window.c WIN_PANIC). On the hosted test build, abort() gives a
 * clean crash with a traceable cause. In the freestanding kernel, a deliberate
 * infinite loop is the loud deterministic halt (Rule 2 / Rule 11).
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort() -- hosted only */
#  define SCRAP_PANIC(msg)  abort()
#else
/* Freestanding: deliberate, loud, deterministic hang (Rule 2 / Rule 11). */
#  define SCRAP_PANIC(msg)  do { for (;;) { } } while (0)
#endif

/* ---------------------------------------------------------------------------
 * scrap_memcpy -- freestanding byte copy (no string.h / libc).
 * Copies `n` bytes from src to dst. Regions must not overlap (the callers
 * here always copy from external input into flavor->data, or from flavor->data
 * into a caller-supplied buffer, so overlap is impossible).
 * ------------------------------------------------------------------------- */
static void scrap_memcpy(void *dst, const void *src, uint32_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *p = (const uint8_t *)src;
    uint32_t i;
    for (i = 0; i < n; i++)
        d[i] = p[i];
}

/* ---------------------------------------------------------------------------
 * find_flavor -- search flavors[0..n_flavors) for type; return the index or
 * -1 if absent. This is the ONLY location where the type-key comparison
 * lives, so SCRAP_MUT_IGNORE_FLAVOR perturbs it in exactly one place.
 *
 * MUTANT SCRAP_MUT_IGNORE_FLAVOR: ignore the type key and always "match"
 * index 0 when any flavor exists. Effect on the test steps:
 *   PutScrap(PICT): find_flavor returns 0 (TEXT slot) -> replaces TEXT slot
 *     payload with PICT bytes (type field stays TEXT).
 *   GetScrap(TEXT): find_flavor returns 0 -> returns the now-PICT bytes ->
 *     step 3 "GetScrap(TEXT) STILL returns 5 with hello bytes" goes RED.
 * ------------------------------------------------------------------------- */
static int find_flavor(const FlairScrap *s, flair_ostype_t type)
{
#ifdef SCRAP_MUT_IGNORE_FLAVOR
    /* MUTANT (Rule 6 -- NEVER define in a real build): ignore the type key;
     * always return index 0 if any flavor exists. Causes PutScrap(PICT) to
     * overwrite the TEXT slot, so GetScrap(TEXT) yields PICT bytes -> RED. */
    (void)type;
    return (s->n_flavors > 0) ? 0 : -1;
#else
    uint32_t i;
    for (i = 0; i < s->n_flavors; i++) {
        if (s->flavors[i].type == type)
            return (int)i;
    }
    return -1;
#endif
}

/* ---------------------------------------------------------------------------
 * FlairScrap_init -- zero-initialize the scrap record.
 * Sets scrapCount=0, n_flavors=0, and clears the type + length fields of
 * every flavor slot. The data arrays are NOT zeroed (only bytes [0..length)
 * are ever read; length is set by PutScrap before any read).
 * ------------------------------------------------------------------------- */
void FlairScrap_init(FlairScrap *s)
{
    uint32_t i;
    if (s == NULL) SCRAP_PANIC("FlairScrap_init: NULL argument (contract error)");
    s->scrapCount = 0;
    s->n_flavors  = 0u;
    for (i = 0u; i < FLAIR_SCRAP_MAX_FLAVORS; i++) {
        s->flavors[i].type   = 0u;
        s->flavors[i].length = 0u;
    }
}

/* ---------------------------------------------------------------------------
 * FlairZeroScrap -- discard all flavors and increment the generation count.
 *
 * MUTANT SCRAP_MUT_NO_CLEAR: skip n_flavors=0 (leave stale flavors) ->
 *   GetScrap after ZeroScrap returns stale data instead of -1 -> RED.
 * MUTANT SCRAP_MUT_NO_INCREMENT: skip scrapCount++ ->
 *   FlairInfoScrap returns the wrong generation number -> RED.
 * ------------------------------------------------------------------------- */
void FlairZeroScrap(FlairScrap *s)
{
    if (s == NULL) SCRAP_PANIC("FlairZeroScrap: NULL argument (contract error)");
#ifndef SCRAP_MUT_NO_CLEAR
    s->n_flavors = 0u;
#endif
#ifndef SCRAP_MUT_NO_INCREMENT
    s->scrapCount++;
#endif
}

/* ---------------------------------------------------------------------------
 * FlairPutScrap -- write `len` bytes from `data` as flavor `type`.
 *
 * Policy: REPLACE the existing slot if type already exists (no duplicate);
 * APPEND a new slot if the type is new (fail loud at MAX_FLAVORS).
 * A rejected put (non-zero return) MUST NOT corrupt any existing flavor.
 * ------------------------------------------------------------------------- */
int32_t FlairPutScrap(FlairScrap *s, flair_ostype_t type,
                      const void *data, uint32_t len)
{
    int idx;

    if (s == NULL) SCRAP_PANIC("FlairPutScrap: NULL argument (contract error)");

    /* Rule 2: fail loud on over-size -- NEVER silently truncate. */
    if (len > FLAIR_SCRAP_MAX_PAYLOAD)
        return FLAIR_SCRAP_ERR_TOO_LARGE;

    idx = find_flavor(s, type);
    if (idx >= 0) {
        /* REPLACE in place: update length and payload; leave the type field
         * (we found the slot by its type, so it is already correct). */
        s->flavors[(uint32_t)idx].length = len;
        scrap_memcpy(s->flavors[(uint32_t)idx].data, data, len);
        return 0;
    }

    /* APPEND a new flavor. */
    if (s->n_flavors >= FLAIR_SCRAP_MAX_FLAVORS)
        return FLAIR_SCRAP_ERR_NO_FLAVOR;

    {
        uint32_t slot = s->n_flavors;
        s->flavors[slot].type   = type;
        s->flavors[slot].length = len;
        scrap_memcpy(s->flavors[slot].data, data, len);
        s->n_flavors++;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * FlairGetScrap -- retrieve flavor `type` into `buf` (bufsz bytes available).
 *
 * Returns the byte count on success; -1 if type absent; FLAIR_SCRAP_ERR_BUF
 * if bufsz < length (Rule 2: fail loud, NO partial copy).
 * ------------------------------------------------------------------------- */
int32_t FlairGetScrap(const FlairScrap *s, flair_ostype_t type,
                      void *buf, uint32_t bufsz, uint32_t *len_out)
{
    int idx;
    uint32_t flen;

    if (s == NULL) SCRAP_PANIC("FlairGetScrap: NULL argument (contract error)");

    idx = find_flavor(s, type);
    if (idx < 0)
        return -1;   /* type absent */

    flen = s->flavors[(uint32_t)idx].length;

    /* Rule 2: fail loud if the caller's buffer is too small -- NO partial copy. */
    if (bufsz < flen)
        return FLAIR_SCRAP_ERR_BUF;

    if (buf != NULL)
        scrap_memcpy(buf, s->flavors[(uint32_t)idx].data, flen);
    if (len_out != NULL)
        *len_out = flen;

    return (int32_t)flen;
}

/* ---------------------------------------------------------------------------
 * FlairInfoScrap -- return the current scrapCount (generation number).
 * ------------------------------------------------------------------------- */
int32_t FlairInfoScrap(const FlairScrap *s)
{
    if (s == NULL) SCRAP_PANIC("FlairInfoScrap: NULL argument (contract error)");
    return s->scrapCount;
}
