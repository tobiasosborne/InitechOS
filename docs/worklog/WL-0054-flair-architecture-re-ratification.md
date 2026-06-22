<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0054 -- FLAIR architecture re-ratification: the heresy purge + the corrected design (mechanism/policy split, dual-heritage region spine, grade-against-independent-decomp-golden, the locked Initech Color Canon)

**Type:** governance / architecture shard (design docs ratified; no implementation code landed -- the build-out is epic `initech-qipc`, steps 1-7).
**Date:** 2026-06-22.
**Operator:** ratified the full set ("Ratify & commit/push all").
**Epic:** `initech-qipc` (P0). **Children roadmap:** `initech-n79q` / `h714` / `mwpw` / `7x9k` / `6bq2` / `m6qx` / `5l5z`.
**Memories:** `bd memories flair-heresy-purge` ; `bd memories ratified-operator-directive-2026-06-21` ; `bd memories initech-color`.

## Context -- why this shard exists

The operator asked, before re30.4, how FLAIR's testing/grading/goldens actually
work. A grounded grading audit (workflow `wf_a094680b-fce`) found the mental
model ("System 7/8 is the live golden master; behaviour + pixels graded against
those goldens; Win 3.1 accents layered in") was **the design intent, not the
build**. The load-bearing finding: the live FLAIR oracle graded **BY
CONSTRUCTION** -- `tools/ppm_flair_check.c` computed its expected RGBs from
`flair_palette_rgb()`, the SAME function `kmain.c:733` paints with, so a wrong
palette flowed identically into both the rendered pixel and the "expected"
value and the +/-2 diff could never bite on color. That is precisely why the
operator-condemned `preview.webp` palette (WL-0053) passed every gate green.
Adjacent findings: the decomp corpora were not a live golden master (only
`test-clut` diffed one 256-entry CLUT, and `clut.h` drove no rendering);
`win31-decomp` had ZERO Makefile wiring; SSIM was documented across
ADR/PRD/CLAUDE.md as a running per-window guide but was unbuilt
(`make ssim` = stub, `harness/ssim.c` absent); the booted OS ran no FLAIR event
loop; color policy was smeared across 5 duplicated `index->RGB` switches.

The operator then directed a comprehensive, orchestrated correction: root out
the heresies (hard-revoke contradictory committee decisions by management
decree), convene committees to architect everything to the locked principles,
and end with **ratified design docs** that lead to a full operational GUI.

## The locked principles (operator-ratified, no relitigation)

- **P1** mechanism/policy split: the imaging/window MECHANISM names palette
  INDICES, never RGB; color/pattern/metric cross as PARAMETERS.
- **P2** ATKINSON is the SINGLE shared dual-heritage spine (QuickDraw `Rgn` +
  GDI `HRGN`); both clip through the one `region_op`; neither owns it.
- **P3** grade against an INDEPENDENT decomp golden, NEVER by construction.
- **P4** decomp goldens are canon, `preview.webp` is provenance-only; the
  Initech Color Canon (teal `#8DDCDC` + decomp-sourced; lavender bevel->teal).
- **P5** era-layered (Sys7 base now / Sys8 Platinum accretes) as decoration
  policies over the one mechanism+spine; fidelity is the artifact look, not the
  source structure.

## What changed (this shard -- DOCS ONLY)

Orchestrated as four sequential workflows, each gated by me on the main thread:

1. **Heresy audit** (`wf_7dd9a33b-e7c`): 5 hunt lanes -> consolidation -> adversarial
   adjudication. **20 candidates, 19 upheld / 1 struck** (HER-10 `clut.h`
   correctly kept -- it is the device-CLUT MECHANISM + the in-repo exemplar of
   independent-golden grading, not chrome decoration).
2. **6 architecture committees + consistency critic** (`wf_80ad0434-5e1`): each
   committee = 3 independent proposals -> adversarial chair. The critic ruled
   **coheres: FALSE** -- 8 conflicts, chiefly that 4 committees each invented a
   different color-policy module (the de-dup would have multiplied to four).
3. **Chief-architect reconciliation** (`wf_9dc638d9-e8a`): folded the critic's 9
   binding resolutions into ONE coherent design; a verifier confirmed
   **composes: true, module fork resolved, ZERO by-construction oracles**, seams
   line up, against the live tree + both sibling repos.
4. **Authoring** (`wf_42e430d9-1d3`): the 5 governance docs written in house
   style; a consistency reviewer confirmed numbering/cross-refs/canon-values/
   no-by-construction/no-revoked-heresy clean (after 2 mechanical fixes:
   transliterate DEC-09 to ASCII; resolve the `A?`->`A1` placeholder).

**Ratified artifacts (committed):**
- `docs/adr/REVOCATION-RECORD-2026-06-21-FLAIR-Heresy-Purge.md` (the 19-item decree).
- `docs/adr/ADR-0004-AMENDMENT-DEC-09-Mechanism-Policy-Split.md` (C-8 cut-line;
  the ONE canon module; flair_look_pixel resolver; flair_skin_t registry view;
  D-9 era-layering + D-9b peer-skin/GDI-facade; OD-4 seafoam->teal).
- `docs/adr/ADR-0005-AMENDMENT-AM-1-Dual-Heritage-Region-Spine.md` (GDI/HRGN
  peer family; symmetric truth table; RectInRgn overlap fix; single-engine
  structural grep guard; homomorphism oracle untouched).
- `docs/adr/ADR-0010-FLAIR-Grading-and-Goldens.md` (one `test-color-canon`
  4-leg oracle; ppm_flair_check re-key; PNG loud-skip; win depth-trap; SSIM
  deferred; the loud-skip + provenance-honesty contract; SUPERSEDES OD-4/FO-3/
  AM-9/FO-F; HARD-REVOKES HER-02).
- `docs/adr/ADR-0006-FLAIR-Live-Event-Loop-and-Behavioural-Grading.md`.
- `spec/assets/color_canon.json` -- THE locked color authority (idx0..8 + 2
  derived teal-bevel rows; per-entry decomp golden + wctb part<->index crosswalk
  + graded_by; the seafoam + preview samples retained as superseded/provenance).
- Doc-drift purged in place: CLAUDE.md (Law 2 by-construction callout + Law 4
  SSIM-unbuilt + file-map), PRD (seafoam->teal headline + SSIM-planned notes),
  HANDOFF (new top reconcile entry), ADR-0004 (OD-4 supersede pointers x3).
- 7-step implementation roadmap beads under `initech-qipc`.

## Why (root causes the decree fixes)

The by-construction oracle (HER-02) is the deep root: a self-consistency check
cannot be a Law-2 oracle. The 5-switch color tangle (HER-12/17) is the P1
mechanism/policy violation made flesh; OD-4 seafoam (HER-01/06/08) and the
`preview.webp` render source (HER-03) are the P4 violations the operator already
overrode in WL-0053 but which were never struck from the ADR/code; the
mono-heritage ATKINSON (HER-13/18) blocked P2; the unwired `win31-decomp`
(HER-09) and the unbuilt-but-claimed SSIM (HER-11/15/19) are P3/honesty drift.

## Frictions / lessons

- The consistency critic earned its keep: the committees were each individually
  sound but quadruple-owned the color module -- a fan-out without a composition
  gate would have shipped "collapse 5 switches into 4 new modules." LESSON:
  always close a multi-committee design fan-out with a cross-committee
  consistency + completeness critic before authoring.
- An adjudicator correctly REJECTED one heresy (HER-10) on a category-error
  argument -- over-revoking is as harmful as missing a heresy.
- `color_canon.h` is GENERATED (FO-3 / step 2), so it is NOT authored here; only
  the locked `.json` authority is. The implementation (the generator, the
  oracle, the switch collapse) is steps 2-7, deliberately oracle-first.

## Acceptance

Design-phase acceptance (Law 2 for the design itself): the reconciliation
verifier confirmed the architecture composes with zero by-construction oracles
and the seams aligned, verified against the live tree + `../system7-decomp` +
`../win31-decomp`; the authoring reviewer confirmed the docs are consistent and
ASCII-clean. **No build/oracle change landed** -- `make test` is unaffected this
shard. The mechanical acceptance of the corrected GRADING is step 3
(`test-color-canon` GREEN + value-mutant RED) and step 4 (the teal render + the
re-keyed `ppm_flair_check` with a palette-VALUE mutant), per `initech-qipc`.

## Pointers

- Decree + register: `docs/adr/REVOCATION-RECORD-2026-06-21-FLAIR-Heresy-Purge.md`.
- The corrected architecture: ADR-0004-AMENDMENT-DEC-09, ADR-0005-AMENDMENT-AM-1,
  ADR-0010, ADR-0006.
- The locked canon: `spec/assets/color_canon.json`.
- Roadmap: `bd show initech-qipc` ; start at `initech-n79q` (region spine).
- Prior: WL-0053 (the color-canon decision shard this implements + supersedes
  the by-construction grading it could not yet see).
