/* spec/flair_tenants_demo.h -- FLAIR App Contract tenants DEMO layout contract
 *
 * The single source of truth for the -DFLAIR_LIVE_TENANTS demo scene shared by:
 *   - os/apps/ref_tenant.c   (the HELLO + NOTES reference tenants)
 *   - os/milton/kmain.c      (the FLAIR_LIVE_TENANTS pump arm builds the scene)
 *   - tools/ppm_flair_appswitch_check.c (the O-5 gate: WHERE to look)
 *
 * This is shared GEOMETRY + the canon-INDEX choices for the demo, NOT a locked
 * canon (cf. ADR-0010): the O-5 grader reads these rects to know WHERE to probe,
 * but grades the pixel VALUES against the independent color canon
 * (spec/assets/color_canon.h -> flair_canon_rgb, vouched by test-color-canon),
 * never by-construction, never preview.webp. ADR-0013 + amendment bead initech-fka6;
 * Wave-4 committee wf_8a917ec8-514.
 *
 * Layout rationale (640x480, two 20px bars => content area y >= 40):
 * NOTES is launched LAST => foreground => drawn ON TOP, and it PARTIALLY OCCLUDES
 * HELLO. So a click on HELLO's visible sliver raises HELLO, which EXPOSES the
 * previously-covered overlap region -> HELLO must repaint it via updateEvt routing
 * (flair_route_updates). That overlap is the probe that makes the drop-updateEvt
 * mutant bite (it would leave NOTES' colour showing in HELLO's raised content).
 */
#ifndef FLAIR_TENANTS_DEMO_H
#define FLAIR_TENANTS_DEMO_H

#include "assets/color_canon.h"   /* CIDX_* */

/* --- Tenant names (must match the FlairApp.name the kmain arm launches; the
 *     O-5 serial marker is "FLAIR-DISPATCH app=<name>"). --- */
#define FLAIR_TEN_HELLO_NAME   "HELLO"
#define FLAIR_TEN_NOTES_NAME   "NOTES"

/* --- Structure-rect bounds (left,top,right,bottom), half-open, global coords.
 *     HELLO: 60,60 .. 360,260 (300x200).  NOTES: 260,120 .. 560,340 (300x220),
 *     launched last so it covers HELLO's bottom-right.  Overlap (intersection):
 *     x[260,360) y[120,260). HELLO's clickable sliver is the rest of HELLO. --- */
#define FLAIR_TEN_HELLO_L   60
#define FLAIR_TEN_HELLO_T   60
#define FLAIR_TEN_HELLO_R   360
#define FLAIR_TEN_HELLO_B   260

#define FLAIR_TEN_NOTES_L   260
#define FLAIR_TEN_NOTES_T   120
#define FLAIR_TEN_NOTES_R   560
#define FLAIR_TEN_NOTES_B   340

/* The overlap region (HELLO content newly exposed when HELLO is raised). */
#define FLAIR_TEN_OVERLAP_L  260
#define FLAIR_TEN_OVERLAP_T  120
#define FLAIR_TEN_OVERLAP_R  360
#define FLAIR_TEN_OVERLAP_B  260

/* --- Content fill + active-accent canon indices (distinct per tenant so the
 *     grader can tell whose content occupies a pixel by VALUE).  Both render
 *     their own FILL when inactive and add an ACCENT block when active. --- */
#define FLAIR_TEN_HELLO_FILL    CIDX_WHITE      /* HELLO content background */
#define FLAIR_TEN_NOTES_FILL    CIDX_CONTROL    /* NOTES content background (gray) */
#define FLAIR_TEN_ACTIVE_ACCENT CIDX_ACCENT     /* the active-tenant accent (navy) */

/* --- Probe points for the O-5 gate (chosen INSIDE the content region, not on
 *     chrome). The grader reads VALUES at these points against the canon. --- */
/* (1) OVERLAP probe: pre-switch shows NOTES_FILL (NOTES on top); post-switch
 *     shows HELLO_FILL (HELLO raised + updateEvt-repainted). The drop-updateEvt
 *     mutant leaves NOTES_FILL here -> RED. Also bitten by no-raise / ignore-refCon. */
#define FLAIR_TEN_PROBE_OVERLAP_X  300
#define FLAIR_TEN_PROBE_OVERLAP_Y  185
/* (2) HELLO ACTIVE-ACCENT probe: a 12x12 accent block at HELLO content top-left;
 *     present only when HELLO is the active tenant (activateEvt active=1). The
 *     skip-activate-pair mutant leaves it FILL -> RED. */
#define FLAIR_TEN_HELLO_ACCENT_X   74
#define FLAIR_TEN_HELLO_ACCENT_Y   88
#define FLAIR_TEN_ACCENT_SIZE      12
/* (3) HELLO sliver CLICK point: inside HELLO content, NOT under NOTES, so the
 *     O-5 trace can click the background tenant. (x<260 => left of the overlap.) */
#define FLAIR_TEN_HELLO_CLICK_X    150
#define FLAIR_TEN_HELLO_CLICK_Y    150

/* --- Per-tenant DATA-arena budget (bytes) carved from the 4 MiB FLAIR heap by
 *     FlairProcess_launch.  Under ADR-0013 Amendment AC-2 (the initech-ubd0 split-
 *     arena resolution) this is the DATA arena ONLY: per-instance private state
 *     (tenant_priv_t) + slack.  The WindowRecord + its region pools now carve from
 *     the SEPARATE records arena sized by FLAIR_TENANT_RECORDS_DEFAULT (process.h),
 *     NOT from this budget -- which is why a scribbled DATA arena cannot corrupt the
 *     window records (BC-6).  VALUE UNCHANGED (Rule 8): only the now-stale comment is
 *     corrected; 2*(FLAIR_TEN_BUDGET + FLAIR_TENANT_RECORDS_DEFAULT) ~= 160 KiB stays
 *     well under FLAIR_HEAP_SIZE (4 MiB). --- */
#define FLAIR_TEN_BUDGET   (64u * 1024u)

#endif /* FLAIR_TENANTS_DEMO_H */
