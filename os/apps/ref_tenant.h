/*
 * os/apps/ref_tenant.h -- the two reference co-resident FLAIR tenants (THE ARTIFACT).
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013) + amendment bead
 *        initech-fka6 (Wave-4 Step 1). HELLO + NOTES are the two reference tenants
 *        the -DFLAIR_LIVE_TENANTS demo scene launches (the 4th demo image
 *        flair_tenants.img); kmain's pump arm references these two FlairAppProcs
 *        vtables and hands every event to flair_app_dispatch (the SOLE routing call).
 *
 * WHAT THIS IS: each tenant is a const FlairAppProcs vtable (ADR-0013 Sec 3.1) over a
 * private per-instance state struct the tenant's open() allocates from self->arena and
 * stashes in self->userData (the additive FlairApp.userData slot, bead fka6). HELLO and
 * NOTES share ONE event handler (the per-tenant config is recovered through userData);
 * they differ only by their demo descriptor (bounds + content FILL canon index), all of
 * which is the SHARED demo contract spec/flair_tenants_demo.h (HELLO uses
 * FLAIR_TEN_HELLO_*, NOTES uses FLAIR_TEN_NOTES_*). The layout MUST match that header
 * exactly -- it is the single source of truth the O-5 appswitch grader probes against.
 *
 * Ref: ADR-0013 Sec 3.1 (tenant ABI + binding rule), Sec 3.3 (event routing /
 *      activation); spec/flair_tenants_demo.h (the demo layout + canon indices);
 *      os/flair/process.h (FlairAppProcs, FlairApp.userData, FlairLaunchParams.surface).
 *      CLAUDE.md Law 3 (freestanding artifact), Rule 12 (ASCII-clean).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_APPS_REF_TENANT_H
#define INITECH_OS_APPS_REF_TENANT_H

#include "process.h"   /* FlairAppProcs (-Ios/flair) */

/* The two reference tenants' entry-point vtables (the A5 jump-table analogue).
 * kmain's FLAIR_LIVE_TENANTS arm launches them via FlairProcess_launch with the
 * names FLAIR_TEN_HELLO_NAME / FLAIR_TEN_NOTES_NAME (spec/flair_tenants_demo.h). */
extern const FlairAppProcs hello_procs;
extern const FlairAppProcs notes_procs;

#endif /* INITECH_OS_APPS_REF_TENANT_H */
