/*
 * os/flair/scrap.h -- the FLAIR Scrap Manager (clipboard) -- THE ARTIFACT.
 *
 * beads: initech-b2vk (FLAIR Phase 4.5 -- Platform Services: Scrap).
 * Ref:   ADR-0013 Sec 6 ("shell-owned cross-tenant Scrap; copy/paste between
 *        co-resident apps -- the payoff of co-residency"); ADR-0012 D-2b
 *        (Phase 4.5 platform services); Inside Macintosh Vol I Scrap Manager
 *        (ZeroScrap/PutScrap/GetScrap/InfoScrap; the monotonic scrapCount;
 *        multiple coexisting flavors keyed by 4-char OSType, classically TEXT
 *        and PICT).
 *
 * MODEL (era-authentic Inside Macintosh Scrap Manager):
 *   The Scrap is a SHELL-OWNED SINGLETON. It survives every tenant
 *   launch/terminate/death (it is not in any child arena; a tenant death cannot
 *   corrupt it). The API takes an EXPLICIT `FlairScrap *` pointer so the host
 *   oracle can construct its own record with no global.
 *
 *   Flavor coexistence: PutScrap of a NEW type APPENDS a flavor (up to
 *   FLAIR_SCRAP_MAX_FLAVORS); PutScrap of an EXISTING type REPLACES that flavor
 *   in place (no duplicate). ZeroScrap CLEARS all flavors (n_flavors=0) AND
 *   increments scrapCount (the era "new clipboard generation"). GetScrap of an
 *   absent type returns -1.
 *
 * HEAP SEAM (committee ruling, ADR-0013 Sec 6):
 *   Flavor payloads are FIXED inline byte arrays inside the struct (NOT per-put
 *   flair_alloc): the FLAIR heap free-list does NOT roll back the bump cursor,
 *   so per-put alloc would leak bump space (heap.h:179-203). Wiring the
 *   singleton into kmain / the shell desktop is a FOLLOW-UP, not this wave.
 *
 * CAPACITY CONSTANTS (Rule 8: raising either is a DELIBERATE ACT with an issue
 *   + worklog note -- never a silent edit. InitechWord/Initech123 will need
 *   larger payloads; touch ONLY with an ADR amendment and a Rule-8 spec-lock):
 *   FLAIR_SCRAP_MAX_FLAVORS = 4   (TEXT, PICT, and two reserve slots)
 *   FLAIR_SCRAP_MAX_PAYLOAD = 8192 bytes (covers the PRD-era app payloads)
 *
 * ERA CEILING (Law 3 / hallucination-risk):
 *   Do NOT import Win32 clipboard API names (CF_TEXT/CF_BITMAP/OpenClipboard/
 *   EmptyClipboard/SetClipboardData). The model is Mac Scrap Manager only.
 *
 * FREESTANDING (Law 3): <stdint.h> + "ostype.h" only; no libc, no malloc.
 *   Compiles BOTH under the kernel flags (gcc -m32 -ffreestanding -nostdlib
 *   -std=c11 -Wall -Wextra -Werror) AND hosted for the property suite
 *   (harness/proptest/test_scrap.c). The dual-compile pattern of heap.c /
 *   process.c.
 *
 * Ref: os/flair/ostype.h (flair_ostype_t, FLAIR_OSTYPE -- USE IT; do not
 *      redefine OSType); os/flair/heap.h (heap.h:179-203 -- the bump-only
 *      free-list justification for inline payloads); ADR-0013 Sec 6.
 *      CLAUDE.md Law 1 (ground truth before code), Law 2 (oracle is truth,
 *      not the agent), Law 3 (freestanding artifact, no 2026-ism), Rule 2
 *      (fail loud: over-MAX_PAYLOAD = error, bufsz-too-small = error; never
 *      silent truncation), Rule 8 (spec as locked data), Rule 11
 *      (deterministic), Rule 12 (ASCII-clean source).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_SCRAP_H
#define INITECH_OS_FLAIR_SCRAP_H

#include <stdint.h>
#include "ostype.h"   /* flair_ostype_t, FLAIR_OSTYPE -- shared 4-char type key */

/* --------------------------------------------------------------------------
 * Error codes returned by PutScrap / GetScrap (all negative; 0 = success).
 * Positive return values from GetScrap are byte counts.
 * -------------------------------------------------------------------------- */
#define FLAIR_SCRAP_ERR_TOO_LARGE   (-2)   /* PutScrap: len > FLAIR_SCRAP_MAX_PAYLOAD  */
#define FLAIR_SCRAP_ERR_NO_FLAVOR   (-3)   /* PutScrap: all MAX_FLAVORS slots in use   */
#define FLAIR_SCRAP_ERR_BUF         (-4)   /* GetScrap: bufsz < flavor length          */

/* --------------------------------------------------------------------------
 * Capacity constants (Rule 8: locked; raise only with issue + ADR amendment).
 * -------------------------------------------------------------------------- */
#define FLAIR_SCRAP_MAX_FLAVORS  4u        /* TEXT, PICT, + 2 reserve slots            */
#define FLAIR_SCRAP_MAX_PAYLOAD  8192u     /* bytes; covers PRD-era app payloads       */

/* --------------------------------------------------------------------------
 * Era-authentic scrap type codes (Inside Macintosh Scrap Manager).
 * Built with FLAIR_OSTYPE from ostype.h -- do NOT define flair_ostype_t again.
 * -------------------------------------------------------------------------- */
#define FLAIR_SCRAP_TYPE_TEXT  FLAIR_OSTYPE('T', 'E', 'X', 'T')   /* 0x54455854 */
#define FLAIR_SCRAP_TYPE_PICT  FLAIR_OSTYPE('P', 'I', 'C', 'T')   /* 0x50494354 */

/* Pin TEXT to its canonical big-endian bit pattern (host == kernel; ostype.h
 * already pins the macro, this pins the named constant). */
_Static_assert(FLAIR_SCRAP_TYPE_TEXT == 0x54455854u,
               "FLAIR_SCRAP_TYPE_TEXT must equal 0x54455854 ('TEXT' big-endian)");

/* --------------------------------------------------------------------------
 * FlairScrapFlavor -- one clipboard flavor: type key + length + inline data.
 *
 * Payloads are INLINE (not heap-allocated) per the committee ruling: the FLAIR
 * bump heap does NOT roll back the cursor on free, so per-put flair_alloc would
 * leak bump space across every copy/paste cycle (heap.h:179-203).
 * -------------------------------------------------------------------------- */
typedef struct {
    flair_ostype_t type;                       /* 4-char flavor key            */
    uint32_t       length;                     /* valid byte count in data[]   */
    uint8_t        data[FLAIR_SCRAP_MAX_PAYLOAD]; /* inline payload            */
} FlairScrapFlavor;

/* --------------------------------------------------------------------------
 * FlairScrap -- the shell-owned singleton clipboard record.
 *
 * `scrapCount` is a monotonic generation number (Inside Macintosh Sec 7-4):
 * ZeroScrap increments it so consumers can detect a stale cache. `n_flavors`
 * is the count of active entries in flavors[]; entries [0..n_flavors) are
 * valid, the rest are ignored. Multiple flavors COEXIST (era-authentic
 * multiform clipboard: an app can put TEXT + PICT together and a consumer
 * can request whichever it understands).
 * -------------------------------------------------------------------------- */
typedef struct {
    int32_t        scrapCount;                      /* monotonic generation     */
    uint32_t       n_flavors;                       /* active flavor count      */
    FlairScrapFlavor flavors[FLAIR_SCRAP_MAX_FLAVORS]; /* inline flavor table  */
} FlairScrap;

/* --------------------------------------------------------------------------
 * API (all take an explicit FlairScrap * -- no implicit global in the artifact;
 * NULL FlairScrap * -> fail loud, Rule 2).
 * -------------------------------------------------------------------------- */

/* FlairScrap_init -- zero-initialize the scrap record (scrapCount=0, no
 * flavors). The shell calls this once on the master singleton before any
 * tenant launches. */
void FlairScrap_init(FlairScrap *s);

/* FlairZeroScrap -- discard all flavors (n_flavors=0) and increment
 * scrapCount (the era "new clipboard generation"). The consumer can detect
 * that its cached scrap is stale by comparing scrapCount against the saved
 * value from the last InfoScrap call.
 *
 * Ref: Inside Macintosh Vol I Scrap Manager -- ZeroScrap. */
void FlairZeroScrap(FlairScrap *s);

/* FlairPutScrap -- write `len` bytes from `data` as flavor `type`.
 *
 * If the type already exists: REPLACE in place (no duplicate).
 * If the type is new: APPEND (up to FLAIR_SCRAP_MAX_FLAVORS).
 *
 * Returns: 0 on success; FLAIR_SCRAP_ERR_TOO_LARGE (-2) if len >
 * FLAIR_SCRAP_MAX_PAYLOAD (FAIL-LOUD, never silent truncation);
 * FLAIR_SCRAP_ERR_NO_FLAVOR (-3) if appending past MAX_FLAVORS.
 *
 * A rejected put (non-zero return) MUST NOT corrupt an existing flavor. */
int32_t FlairPutScrap(FlairScrap *s, flair_ostype_t type,
                      const void *data, uint32_t len);

/* FlairGetScrap -- retrieve flavor `type` into `buf` (up to `bufsz` bytes).
 *
 * Returns: the byte count copied (>=0) on success; -1 if the type is absent;
 * FLAIR_SCRAP_ERR_BUF (-4) if bufsz < the stored flavor length (FAIL-LOUD,
 * NO partial copy -- the caller must supply a large enough buffer).
 * Sets *len_out (if non-NULL) to the stored flavor length on success.
 *
 * Ref: Inside Macintosh Vol I Scrap Manager -- GetScrap. */
int32_t FlairGetScrap(const FlairScrap *s, flair_ostype_t type,
                      void *buf, uint32_t bufsz, uint32_t *len_out);

/* FlairInfoScrap -- return scrapCount (the current generation number).
 *
 * Ref: Inside Macintosh Vol I Scrap Manager -- InfoScrap (the scrapCount
 * field of the ScrapStuff record). */
int32_t FlairInfoScrap(const FlairScrap *s);

#endif /* INITECH_OS_FLAIR_SCRAP_H */
