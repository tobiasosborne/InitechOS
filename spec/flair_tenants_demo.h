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
 * ERA AXIS: Mac OS 8 Platinum (DEC-10) is the FLAIR BASE. TODO_GOLDEN: the two
 * menu bands in this demo deliberately retain their System 7 heritage face in
 * this arc; the Platinum menu-bar face has not shipped. Their canonical strings
 * and app-switch behavior remain unchanged (DEC-10 Sec 3.3.3).
 *
 * Layout rationale (640x480, two 20px bars => content area y >= 40):
 * HELLO is launched LAST => foreground => drawn ON TOP, and it PARTIALLY OCCLUDES
 * NOTES. So a click on NOTES's visible sliver raises NOTES, which EXPOSES the
 * previously-covered overlap region -> NOTES must repaint it via updateEvt routing
 * (flair_route_updates). That overlap is the probe that makes the drop-updateEvt
 * mutant bite (it would leave HELLO' colour showing in NOTES's raised content).
 *
 * WHY HELLO (the Photoshop-menu app) IS THE BOOT FOREGROUND (bead initech-4w15,
 * Law 4): the two stacked menu bars must be the DISTINCT Office Space chimera at
 * REST -- band 1 = the static System-7 shell bar ("[Apple] File Edit View
 * Special"), band 2 = the Photoshop bar ("File Edit Image Layer Select View
 * Window Help"). Band 2 reflects the ACTIVE app's own menu (the MultiFinder
 * swap); HELLO's menu IS the Photoshop bar, so with HELLO foreground the resting
 * scene is the distinct chimera with NO extra draw. The O-5 switch then ACTIVATES
 * NOTES (a System-7-menu app), so band 2 swaps Photoshop -> System-7: an
 * observable, mutation-provable transition (the no-menubar-swap mutant leaves
 * band 2 unchanged -> RED). Activating HELLO instead would be a no-op in band 2
 * (already Photoshop) AND would require NOTES(System-7) foreground at boot, whose
 * menu == band 1 => two identical stacked bars => Law-4 violation. Hence HELLO
 * foreground + activate-NOTES is the only 2-app arrangement that keeps (i) band 1
 * static, (ii) the band-2 swap observable, and (iii) the distinct chimera as the
 * RESTING look all at once.
 */
#ifndef FLAIR_TENANTS_DEMO_H
#define FLAIR_TENANTS_DEMO_H

#include "assets/color_canon.h"   /* CIDX_* */
#include "chrome_metrics.h"       /* FLAIR_CHROME_FRAME / FLAIR_CHROME_TITLEBAR_H
                                   * -- the NOTES active-accent probe is derived
                                   * from the documentProc chrome inset, EXACTLY as
                                   * os/apps/ref_tenant.c computes NOTES's content
                                   * top-left (initech-4w15). */

/* --- Tenant names (must match the FlairApp.name the kmain arm launches; the
 *     O-5 serial marker is "FLAIR-DISPATCH app=<name>"). --- */
#define FLAIR_TEN_HELLO_NAME   "HELLO"
#define FLAIR_TEN_NOTES_NAME   "NOTES"

/* --- Structure-rect bounds (left,top,right,bottom), half-open, global coords.
 *     HELLO: 60,60 .. 360,260 (300x200).  NOTES: 260,120 .. 560,340 (300x220).
 *     HELLO is launched LAST (bead initech-4w15) so HELLO covers NOTES's top-left.
 *     Overlap (intersection): x[260,360) y[120,260). NOTES's clickable sliver is
 *     the rest of NOTES (its right/bottom, never under HELLO). --- */
#define FLAIR_TEN_HELLO_L   60
#define FLAIR_TEN_HELLO_T   60
#define FLAIR_TEN_HELLO_R   360
#define FLAIR_TEN_HELLO_B   260

#define FLAIR_TEN_NOTES_L   260
#define FLAIR_TEN_NOTES_T   120
#define FLAIR_TEN_NOTES_R   560
#define FLAIR_TEN_NOTES_B   340

/* The overlap region (NOTES content newly exposed when NOTES is raised). */
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
/* (1) OVERLAP probe: pre-switch shows HELLO_FILL (HELLO on top); post-switch
 *     shows NOTES_FILL (NOTES raised + updateEvt-repainted). The drop-updateEvt
 *     mutant leaves HELLO_FILL here -> RED. Also bitten by no-raise / ignore-refCon.
 *     (bead initech-4w15: HELLO is the boot foreground, so the pre/post canon
 *     VALUES are flipped vs the pre-4w15 arrangement.) */
#define FLAIR_TEN_PROBE_OVERLAP_X  300
#define FLAIR_TEN_PROBE_OVERLAP_Y  185
/* (2a) HELLO ACTIVE-ACCENT probe: a 12x12 accent block at a graded location in
 *      HELLO's always-visible left sliver; present only when HELLO is active. Kept
 *      for ref_tenant HELLO_CFG (HELLO's graded accent site); NOT the O-5 TIER-B
 *      probe anymore (HELLO is now the boot foreground). */
#define FLAIR_TEN_HELLO_ACCENT_X   74
#define FLAIR_TEN_HELLO_ACCENT_Y   88
#define FLAIR_TEN_ACCENT_SIZE      12
/* (2b) NOTES ACTIVE-ACCENT probe (the O-5 TIER-B probe; bead initech-4w15): a
 *      FLAIR_TEN_ACCENT_SIZE block NOTES paints at ITS content top-left when it
 *      becomes active. Derived from the documentProc chrome inset EXACTLY as
 *      os/apps/ref_tenant.c computes NOTES's content top-left (bounds.left+FRAME,
 *      bounds.top+FRAME+TITLEBAR_H) = (261,140). This block is UNDER HELLO before
 *      the switch (=> reads HELLO_FILL) and shows FLAIR_TEN_ACTIVE_ACCENT after
 *      NOTES is raised + activated. The skip-activate-pair mutant leaves it
 *      NOTES_FILL (no accent) -> RED. */
#define FLAIR_TEN_NOTES_ACCENT_X   (FLAIR_TEN_NOTES_L + FLAIR_CHROME_FRAME)
#define FLAIR_TEN_NOTES_ACCENT_Y   (FLAIR_TEN_NOTES_T + FLAIR_CHROME_FRAME + FLAIR_CHROME_TITLEBAR_H)
/* (3) NOTES sliver CLICK point: inside NOTES content, NOT under HELLO, so the
 *     O-5 trace can click the background tenant. x>=360 => right of HELLO's right
 *     edge AND y>=260 => below HELLO's bottom edge (doubly clear of HELLO). */
#define FLAIR_TEN_NOTES_CLICK_X    460
#define FLAIR_TEN_NOTES_CLICK_Y    300

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
