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
 *        engine header adds nothing to the contract -- it merely re-exports the
 *        locked spec so the .c sources and the property suite share one include.
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

#endif /* INITECH_OS_FLAIR_ATKINSON_REGION_H */
