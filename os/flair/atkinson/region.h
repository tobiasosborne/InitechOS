/*
 * os/flair/atkinson/region.h -- the ATKINSON region engine (the artifact).
 *
 * beads: initech-jmo (rep + normalize-on-construction) / initech-b5g (scanline
 *        merge + derived ops + complement + queries).
 * Ref:   PRD Sec 6.2 -- "the load-bearing math". QuickDraw-style regions are the
 *        spine of the FLAIR Toolbox; all ops are a SCANLINE MERGE of inversion
 *        lists; the normal form is the minimal inversion-point set.
 * Ref:   spec/region_algebra.h -- the LOCKED contract this engine implements
 *        VERBATIM. That header declares the ENTIRE public API (types, the 5
 *        normal-form invariants, the 4 op truth tables, the storage caps, the
 *        complement-frame semantics, and the verbatim QuickDraw wrappers). This
 *        engine header adds NOTHING to the artifact ALGEBRA contract -- it
 *        re-exports the locked spec so the .c sources and the property suite
 *        share one include -- plus the engine-plumbing surface from bead
 *        initech-44ab (committee 2026-07-11): the working-set slot shape +
 *        the freestanding-only bind entry (below), and ONE hosted-only,
 *        factory-facing test hook that never compiles freestanding (Law 3).
 *
 * DUAL-COMPILE (the console.c/int21.c pattern; CLAUDE.md Law 3): region.c
 * compiles BOTH freestanding for the kernel (gcc -m32 -ffreestanding -nostdlib)
 * AND hosted for the property suite (harness/proptest/test_region.c). It does NO
 * host malloc -- every region carries caller-supplied rows[]/x_pool storage
 * (arena/static-backed), and a cap overflow FAILS LOUD (Rule 2).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_ATKINSON_REGION_H
#define INITECH_OS_FLAIR_ATKINSON_REGION_H

#include "region_algebra.h"   /* the LOCKED contract -- the whole public API */

/* ---------------------------------------------------------------------------
 * ENGINE WORKING-SET SLOT (bead initech-44ab round 3, committee 2026-07-11;
 * tweak W3). One full-cap working-region backing block: region.c's
 * region_from_rects / query-helper working set is TWO of these. Exposed here
 * so kmain.c sizes the FLAIR-heap backing with sizeof(rgn_ws_slot_t) instead
 * of hand-computing RGN_ROWS_CAP/RGN_X_POOL_CAP byte math. HOSTED builds bind
 * two static slots internally (no init call needed); FREESTANDING kernels
 * MUST allocate two slots (FLAIR heap, FLAIR_CLASS_REGION -- ADR-0004 DEC-03)
 * and bind them via region_engine_bind_ws() BEFORE region_engine_reset() and
 * the first Toolbox region call. The engine itself stays heap-agnostic.
 * ---------------------------------------------------------------------------*/
typedef struct rgn_ws_slot {
    rgn_row_t rows[RGN_ROWS_CAP];
    int16_t   pool[RGN_X_POOL_CAP];
} rgn_ws_slot_t;

#if !defined(__STDC_HOSTED__) || !__STDC_HOSTED__
/* FREESTANDING ONLY: bind the two heap-backed working-set slots (distinct,
 * non-NULL; fail-loud otherwise). Boot order is load-bearing:
 *   flair_heap_init -> region_engine_bind_ws -> region_engine_reset ->
 *   first Toolbox region call (shell_build_scene). */
void region_engine_bind_ws(rgn_ws_slot_t *slot_a, rgn_ws_slot_t *slot_b);
#endif

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
/* HOSTED-ONLY TEST HOOK (factory-facing; Rule 6 -- prove the working-set
 * in-use guard bites; bead initech-44ab, committee 2026-07-11). Acquires
 * the guard, then calls region_from_rects so its nested acquire FAILS LOUD
 * (abort()). Never returns on a correct engine. Call from a fork()ed child
 * (test_region.c); never fork while the parent is inside a guarded call. Not
 * part of the artifact contract; never compiles freestanding (Law 3). */
void region_engine_probe_nested_acquire(region_t *scratch);
#endif

#endif /* INITECH_OS_FLAIR_ATKINSON_REGION_H */
