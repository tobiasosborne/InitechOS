<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0004 Amendment DEC-09 -- FLAIR Mechanism/Policy Split, the Single Color Canon Module, and Era-Layered Decoration

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0004-A1 |
| Title | ADR-0004 Amendment DEC-09: FLAIR Mechanism/Policy Split, the Single Color Canon Module, and Era-Layered Decoration |
| Version | 1.0 |
| Status | **RATIFIED (ADR-by-committee + chief-architect reconciliation, operator-delegated authority, 2026-06-21; operator-ratified 2026-06-22)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme (synthesis: Chief-Architect Reconciliation) |
| Effective Date | 2026-06-21 |
| Next Scheduled Review | 2026-12-21 (semi-annual, per RECORDS-POL-002) |
| Supersedes | ADR-0004 OD-4 (seafoam desktop canon `#6FA08E`) -- REVOKED 2026-06-21; see Sec 3.7 and Sec 6 |
| Superseded By | (none at time of ratification) |
| Related Documents | ADR-0004 (OEA-ADR-0004, FLAIR Toolbox Architecture); ADR-0005 (OEA-ADR-0005, ATKINSON Region Engine) and Amendment AM-1 (Dual-Heritage Region Spine); ADR-0010 (FLAIR Grading and Goldens); ADR-0006 (FLAIR Live Event Loop and Behavioural Grading); REVOCATION-RECORD-2026-06-21 (FLAIR Heresy Purge) |
| Related Issues | beads -- Rule-8 locked-data act for `spec/assets/color_canon.json` (FO-1); ADR-0004-AMENDMENT-DEC-09 ratification tracking (FO-D2-1) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Fidelity Stewardship; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-06-21 | Committee A (Mechanism/Policy) | Initial draft of the P1 mechanism/policy split (C-8 cut-line) and the proposed `flair_look.{h,c}` PART-keyed color accessor. | -- |
| 0.2 | 2026-06-21 | Chief-Architect Reconciliation | Reconciliation of the four-way canon-module fork (Committees A/C/D/F) onto ONE locked `color_canon.json`; deletion of the three competing modules; demotion of `flair_look_pixel` to an in-mechanism PART->idx resolver; folding of the era-layering decision (D-9/D-9b) and the OD-4 seafoam->teal supersession. | -- |
| 1.0 | 2026-06-21 | ARB Chair (synthesis) | RATIFIED following committee review and chief-architect reconciliation under operator-delegated authority. VERIFIED-COMPOSES. Status updated; operator-ratified 2026-06-22. | ARB (full committee) + Fidelity Steward |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Committee A (Mechanism/Policy), STAPLER Programme | Submitted | 2026-06-21 |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Concur-with-comment | 2026-06-21 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Concur-with-comment | 2026-06-21 |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Concur-with-comment | 2026-06-21 |
| Fidelity Steward -- Frame Conformance | Fidelity Stewardship Function (per ADR-0004 Sec 1.3) | Concur | 2026-06-21 |
| ARB Chair (Synthesis) | Slydell & Porter Consulting (delegated per beads ratification tracking) | Ratified | 2026-06-21 |
| Operator Ratification | T. Osborne (Operator) | Delegated to ARB + Chief-Architect Reconciliation; operator-ratified 2026-06-22 | 2026-06-21 |
| Records Management | M. Waddams (Archive Annex B) | Filed | 2026-06-21 |

*Note on committee composition: The Technical/Correctness, Period-Authenticity, and Governance/Compliance reviewers correspond to the in-programme engineering functions designated in ADR-0004 Sec 1.3. The Fidelity Steward reviews under Law 4 (fidelity is the product). This Amendment was produced by a multi-committee ARB process (Committees A, C, D, F) whose competing proposals were reconciled by a chief-architect reconciliation pass that VERIFIED-COMPOSES the result against the live tree and the sibling repos. The operator delegated ratification authority to the committee and the reconciliation; an operator final confirmation is pending and recorded as such in the Status line.*

---

## 1. Purpose and Scope

### 1.1 Purpose

The purpose of this Amendment (the "Amendment" or "DEC-09") is to ratify, as binding architecture, the **separation of mechanism from policy** in the FLAIR imaging and decoration layer, and to arbitrate the **single, locked color-policy authority** that the mechanism consumes.

Prior to this Amendment, the live FLAIR tree carried the color policy as **five drifted index-to-RGB switches** (`chrome.c` `chrome_pal_rgb`, `control.c` `ctrl_pal_rgb`, `dialog.c` `dlg_pal_rgb`, `harness/render/render.c` `render_palette_rgb`, `spec/assets/palette.h` `flair_palette_rgb`) plus inline color literals in `shell.c` and `desktop.c`, each carrying drifted preview-mock-up / seafoam values, all graded by-construction (the oracle read the same source the artifact rendered from). Four independent ARB committees (A, C, D, F) each proposed a *different* new color-policy module to collapse those switches; the single largest composition failure was that this would replace "collapse five to one" with "collapse five to four."

This Amendment ratifies the **P1 mechanism/policy split** (binding constraint **C-8**), the **ONE arbitrated canon module** (`spec/assets/color_canon.json` -> generated `spec/assets/color_canon.h`), the in-mechanism PART->index resolver layered on top of it, the era/heritage registry **view** over the same table, the deletion of the three competing modules, the era-layered decoration decisions (**D-9**, **D-9b**), the **OD-4 seafoam->teal supersession**, and the part<->index crosswalk. It adds two structural oracles to the ADR-0004 D-8 oracle vector.

### 1.2 Scope

This Amendment governs:

- The C-8 mechanism/policy cut-line: no mechanism module names a color (Decision DEC-09-D1).
- The policy seam (the locked `grafport.h` GrafPort surface plus the `flair_look_pixel` PART->idx resolver) and the ownership of the canon RGB table (Decision DEC-09-D2; arbitration ARB-1..ARB-8).
- The honest sequencing of the five-switch collapse as a deliberate Rule-8 value change, not a transparent refactor (Decision DEC-09-D3).
- The two surviving structural oracles `test-mech-policy` and `test-flair-mechanism-colorblind`, added to the ADR-0004 Sec 3.8 D-8 oracle vector (Decision DEC-09-D4).
- The era-layered decoration model: the data-only `flair_skin_t` registry view (Decision D-9) and the peer-skin chimera with the GDI facade made load-bearing on metal (Decision D-9b).
- The supersession of ADR-0004 OD-4 (seafoam `#6FA08E`) by Initech teal `#8DDCDC` at canon idx2, and the lavender->teal bevel swap (Decision OD-4-REVOKED).
- The registration of `spec/assets/color_canon.json`, the generated `spec/assets/color_canon.h`, `spec/flair_skins.h`, and the part<->index crosswalk as LOCKED spec-data (CLAUDE.md Rule 8).

### 1.3 Out of Scope

The following are expressly **out of scope** of this Amendment and are governed by the sibling instruments noted:

- The ATKINSON region engine, its heritage-neutral re-cast, the GDI/HRGN peer wrapper family (`CombineRgn` etc.), and the `RectInRgn` containment->overlap deep-bug fix -- governed by **ADR-0005-AMENDMENT-AM-1 (Dual-Heritage Region Spine)**. This Amendment merely *consumes* the GDI `CombineRgn` shim (D-9b) and *coordinates* the `RectInRgn` audit (FO-8).
- The grading architecture, the single value oracle `test-color-canon`, the `ppm_flair_check` re-key, the `WIN31_DECOMP` wiring, the SSIM defer, and the loud-skip/provenance-honesty contract -- governed by **ADR-0010 (FLAIR Grading and Goldens)**. ADR-0010 *consumes* the canon module this Amendment arbitrates and HARD-REVOKES heresy HER-02.
- The booted-OS cooperative `WaitNextEvent` event loop, the ISR raw-input ring, and behavioural grading (`test-interact`, `test-flair-drag`) -- governed by **ADR-0006 (FLAIR Live Event Loop and Behavioural Grading)**. The live loop *consumes* the locked idx2 teal `#8DDCDC` for the bare-desktop assert.
- The exact VALUE-oracle leg construction (LEG A wctb / LEG B win31 text / LEG C pinstripe.md / LEG D authored-teal) and the named value mutants -- owned by ADR-0010; cross-referenced here only insofar as the part<->index crosswalk pins the slot each leg grades.
- The management decree register of the 20 candidate FLAIR heresies and their dispositions -- governed by **REVOCATION-RECORD-2026-06-21 (FLAIR Heresy Purge)**, which records HER-12/HER-17 (mechanism/policy) as AMEND, executed by this Amendment.

---

## 2. Context

### 2.1 The four-way canon fork (the composition failure)

The reconciliation verified, against the live tree, that **none** of the four proposed color-policy modules existed yet (clean slate); the live state was exactly the five drifted switches plus `shell.c`/`desktop.c` inline literals, all carrying preview-mock-up / seafoam values, and the live grader returned the render source as the "expected" value (a by-construction grade). The four competing proposals were:

- **Committee A:** `spec/assets/flair_look.{h,c}` -- a PART-keyed accessor `flair_look_rgb` / `flair_look_pixel` with its own table.
- **Committee C:** `spec/assets/flair_color_canon.{json,h}` -- an idx-keyed `flair_canon_rgb`.
- **Committee D:** `color_canon.{json,h}` -- an idx-keyed `flair_canon_rgb`, 9 indexed entries plus 2 derived bevel rows.
- **Committee F:** `spec/flair_skins.h` + `spec/initech_color_canon.h` -- a named-slot `flair_skin_t` plus `flair_skin_resolve` acting as a third value authority.

The **values agreed across all four** (idx2 teal `#8DDCDC`, idx5 navy `#000080`, idx6 `#C0C0C0`, idx7/idx8 pinstripe `#F3F3F3`/`#969696`, teal shadow `#4E9BA3`); only the *file and schema* were forked. The reconciliation therefore collapses to **ONE** module (Committee D's name and schema) and demotes the other three contributions to a resolver layer (A), a deletion-and-fold (C), and a registry view (F) -- a lossless de-fork.

### 2.2 Authentic precedent (Law 1)

Naming an **index** rather than an RGB is authentic QuickDraw: the GrafPort drew through a color index into the device CLUT, and the RGB behind the index is **policy**. The cut at the index->pixel call is therefore both period-authentic and the minimal verifiable boundary, because it is a **source property** (no color literal below the line) gradeable by grep with no emulator. Ground truth for the cut point: `chrome.c:57-84` (`chrome_pal_rgb`, a 9-case index->RGB switch executing inside the imaging path); `grafport.h` `rgbFgColor`/`rgbBkColor`/`grafProcs`/`QDProcs`; ADR-0004:241-247 (constraints C-1..C-7, which end at C-7, so C-8 is the next ordinal).

---

## 3. The Decision

This Amendment ratifies the following decisions. The canon-module arbitration is recorded as **ARB-1..ARB-8**; the mechanism/policy split as **DEC-09-D1..DEC-09-D4**; the era-layering and supersession as **D-9**, **D-9b**, and **OD-4-REVOKED**.

### 3.1 The C-8 cut-line and the policy seam (DEC-09-D1, DEC-09-D2)

**DEC-09-D1 (C-8 cut-line).** Bind constraint **C-8** to ADR-0004 Sec 5.1 (after C-7): *no mechanism module names a color.* The mechanism -- `os/flair/surface.c`, `blitter.c`, the relocated `cfill`/`crect`/`cframe` span engine, the GrafPort imaging verbs, the ATKINSON region clip, and `window.c`/`event.c`/`desktop.c` geometry -- MAY name a palette **INDEX** and convert index->destination-pixel ONLY via the shared accessor `flair_look_pixel(port, PART)`. It ships **ZERO** `0xRRGGBB` literal, **ZERO** `INITECH_*_RGB`, and **ZERO** index->RGB switch below the cut-line. Color, pattern, and metric cross the boundary as **PARAMETERS** via the already-locked `grafport.h` seam (`rgbFgColor`/`rgbBkColor`/`fillPat`/`bkPat`/`grafProcs`/`QDProcs`) plus `spec/chrome_metrics.h`.

> Ref: chrome.c:57-84 (`chrome_px()` calls `chrome_pal_rgb()` -- a 9-case index->RGB switch executing inside the imaging path); grafport.h `rgbFgColor`/`rgbBkColor`/`grafProcs`; ADR-0004:241-247 (C-1..C-7).

**DEC-09-D2 (policy seam = grafport.h + the `flair_look_pixel` resolver; canon owned by the canon module).** The policy interface is the already-locked GrafPort `rgbFg`/`rgbBkColor` + `QDProcs` seam plus `flair_look_pixel(port, PART)`, where `PART` is a wctb-keyed enum (`FLAIR_PART_CONTENT`, `_FRAME`, `_TEXT`, `_DESKTOP`, `_MENUBAR`, `_CAPTION_NAVY`, `_BTNFACE`, `_PIN_LIGHT`, `_PIN_DARK`, `_BEVEL_LIGHT`, `_BEVEL_SHADOW`, ...). Per ARB-1/ARB-3, `flair_look_pixel` does **NOT** own a table: it maps PART->idx via the locked `wctb_part` crosswalk, then calls `flair_canon_rgb(idx)` -- the canon-module accessor over `spec/assets/color_canon.json`/`.h` -- and device-CLUT-quantizes at the single surface site. Committee A's originally-proposed standalone `flair_look.{h,c}` second table is **KILLED** (folded into the canon module).

### 3.2 The ONE canon module and the deletion of the three (ARB-1, ARB-2)

**ARB-1 (ONE canon module).** The LOCKED authority is **`spec/assets/color_canon.json`** (idx-keyed; `schema_version` 1; `canon_version` WL-0053; top-level `era` `system7.0-7.1`; 9 indexed entries idx0..8 plus 2 derived rows `bevel_light`/`bevel_shadow`; per-entry `{idx, name, rgb, rgb_bytes, role, heritage, wctb_part, source_golden, graded_by}`). A factory generator `tools/color_canon_extract.c` (modeled on `tools/palette_extract.c`, deterministic per Rule 11, ASCII-clean per Rule 12) emits the **DO-NOT-EDIT** `spec/assets/color_canon.h`: `const unsigned char color_canon[9][3]` plus the SINGLE accessor `uint32_t flair_canon_rgb(uint8_t idx)` (idx < 9 -> the 9-entry table; idx >= 9 -> the deterministic gray ramp already in `flair_palette_rgb`) plus `INITECH_CANON_<NAME>_RGB` macros, the two derived bevel macros, and `_Static_assert(sizeof(color_canon) == 27)`. The header is freestanding-safe (`<stdint.h>` only) so the kernel live-DAC path (`kmain.c:733`) and the host harness share **ONE** table by inclusion. The five switches plus the `shell.c`/`desktop.c` literals all re-body to `flair_canon_rgb`; `flair_palette_rgb` becomes a thin alias.

> Ref: `spec/assets/color_canon.json` (LOCKED, on disk); the `.json`->`.h` generated-pair shape mirrors `palette.json`->`palette.h` and `clut.json`->`clut.h` (an established in-repo factory shape, not a new runtime, Law 3).

**ARB-2 (DELETE the other three modules).** (1) Committee A's `spec/assets/flair_look.{h,c}` as a SECOND table is deleted -- its `flair_look_pixel` survives ONLY as the in-mechanism PART->idx resolver (ARB-3), carrying ZERO color table of its own. (2) Committee C's `spec/assets/flair_color_canon.{json,h}` is deleted -- folded into `color_canon.json` (same idx-keyed `flair_canon_rgb` accessor, so C loses nothing). (3) Committee F's `spec/initech_color_canon.h`-as-a-third-authority is deleted -- its teal constants (`#8DDCDC`, `#4E9BA3`) move INTO `color_canon.json` as the `bevel_light`/`bevel_shadow` plus idx2 authored rows; F's `spec/flair_skins.h` survives ONLY as the era/heritage registry VIEW (ARB-4 / D-9). Because the values already agree across A/C/D/F, deletion-and-fold is lossless.

### 3.3 The PART->idx resolver and the registry view (ARB-3, ARB-4)

**ARB-3 (`flair_look_pixel` is a resolver ON TOP, not a second table).** `flair_look_pixel(port, PART)` maps a wctb-style PART enum to an idx via a static `const` PART->idx map, then calls `flair_canon_rgb(idx)` and device-CLUT-quantizes at the single surface site. It contains ZERO `0xRRGGBB` literal and ZERO index->RGB switch -- the PART->idx map (pure data) is the only thing it owns. `chrome.c`/`control.c`/`dialog.c` become thin **decoration policies** that keep geometry and read color via `flair_look_pixel`. Keying PART on the wctb namespace makes the resolver's KEY identical to the golden KEY, so the value oracle (ADR-0010) diffs key-for-key.

**ARB-4 (`flair_skin_t` is the era/heritage registry VIEW; see also D-9).** `spec/flair_skins.h` defines a data-only `flair_skin_t` (named color slots each carrying the idx AND the canon RGB pulled by inclusion from `color_canon.h`, pattern bytes, metric scalars, `era_id`/`heritage_id` enum tags) plus a `const flair_skin_registry[]` with the `ERA_SYS7_0_1`/`QUICKDRAW` base row and the `ERA_WIN31`/`GDI` peer row; `ERA_SYS8_PLATINUM` is a RESERVED enum value with ZERO rows. Each field's RGB is BY INCLUSION from `color_canon.h` (e.g. `skin->caption_navy = {idx5, INITECH_CANON_ACCENT_NAVY_RGB}`), so there is no second copy of any value, and a frozen-row sha-of-fields digest protects the locked base rows (accretion = append, never mutate).

### 3.4 The bevel is teal, the pinstripe is System-7 (ARB-5)

**ARB-5 (BEVEL -> TEAL; overrules the proposed lavender bevel).** The two derived rows are `bevel_light` `#8DDCDC` and `bevel_shadow` `#4E9BA3` (the WL-0053 lavender->teal swap), `graded_by` `authored` with the locked-constant + structure-preserving proof + derived-shadow lock + seafoam/lavender-relapse VALUE-mutant posture. They are **NEVER** graded against pinstripe.md's lavender rows (`#DADAFF`/`#B3B3DA` at y=165/y=181); those lavender values are recorded in the JSON as the **SUPERSEDED baseline** (deviation-audit pattern), never as the expected value for the teal slot. The **PINSTRIPE itself** (idx7 `#F3F3F3` / idx8 `#969696`) STAYS System-7 and IS graded versus pinstripe.md rendered rows.

> Ref: WL-0053 (operator decree replacing ALL System-7 lavender tinge `#DADAFF`/`#CCCCFF`/`#B3B3DA`/`#8787B3`/`#333366` with Initech teal). Committee A's DEC-09-D3 lavender bevel is amended/overruled here; grading the teal-swapped parts against the lavender golden would be a permanent RED or a forced tolerance loosening (Stop condition).

### 3.5 Honest sequencing of the collapse (ARB-6, DEC-09-D3)

**ARB-6 / DEC-09-D3 (DROP the false byte-identical premise; oracle-first).** VERIFIED divergence: the five switches carry **THREE** desktop values today -- seafoam `0x6FA08E` in `palette.h`/`render.c` idx2, and `0x73696C` in `chrome.c` `CIDX_DESKTOP` / `control.c` / `dialog.c` -- plus divergent white (`0x7F7F86`) and menubar (`0x67696C`). Collapsing onto one table **IS a value change** for the three chrome/control/dialog sites even at "Step 1." The collapse is therefore sequenced **oracle-first** as a deliberate Rule-8 act: the value oracle (`test-color-canon`, ADR-0010) lands GREEN against the canon BEFORE any render flip, NOT as a transparent refactor. Bisectability is kept (each commit green) but the "no render change" claim is **struck**.

> Ref (verified): `palette.h:60` seafoam `0x6FA08E`; `chrome.c:62` / `control.c:69` / `dialog.c:89` `CIDX_DESKTOP` `0x73696C`; `render.c` `render_palette_rgb`.

### 3.6 The part<->index crosswalk (ARB-7, ARB-8)

**ARB-7 (PIN the wctb part<->index crosswalk in the one canon JSON).** Each entry carries a `wctb_part` field so that every value oracle grades the SAME slot against the SAME wctb part. The load-bearing distinction: idx7/idx8 carry `wctb_part: NOT part7/8` -- `part7` `wTitleBarLight` `#FFFFFF` and `part8` `wTitleBarDark` `#000000` are the **WDEF shade-table ENDPOINTS** (a distinct lane), so the rendered pinstripe dither is graded versus pinstripe.md, never versus wctb part7/8. The bevel rows carry `part9`/`part11` dialog lavender `#CCCCFF` teal-SUBSTITUTED (recording the superseded baseline). idx2 teal, idx5 navy, and idx6 BTNFACE carry `wctb_part: none` (Initech-identity / GDI, no wctb part).

**ARB-8 (WIN-ACCENT DEPTH-TRAP guardrail folded into idx5).** idx5 navy is graded indexed-8 documented value `#000080` **PRIMARY**; the 16-color DOSBox-X `#0000AA` is **corroboration-ONLY**; a swamping (~42-level) blue tolerance is **REJECTED** as a Stop condition (R5). The flat-2-D Win-3.1 target forbids the Win95 `#DFDFDF` `COLOR_3DLIGHT` (Law 3). This guardrail is enforced in the value oracle owned by ADR-0010; it is pinned here because the crosswalk fixes idx5's heritage and grading lane.

### 3.7 Era-layered decoration (D-9, D-9b)

**D-9 (NEW -- era-layering, folds R1).** FLAIR decoration is a **DATA-ONLY** `flair_skin_t` record tagged `(era_id, heritage_id)` -- the era/heritage REGISTRY VIEW over the ONE canon module, NOT a fourth color table. Each color field carries the palette IDX AND its canon `0x00RRGGBB` resolved via `flair_canon_rgb(idx)`; **NO** draw-code, **NO** function pointers in the record (the type prevents the engine fork; the proposed `ElementDrawTable` vtable is KILLED). The mechanism reads `skin->` fields as a PARAMETER and NEVER branches on era/heritage and NEVER names a literal RGB. A `const flair_skin_registry[]` holds the `ERA_SYS7_0_1`/`QUICKDRAW` base plus the `ERA_WIN31`/`GDI` peer rows; `ERA_SYS8_PLATINUM` is RESERVED with ZERO rows. `flair_skin_resolve(era, heritage)` is TOTAL and FAIL-LOUD (renders `PC LOAD LETTER` on an unknown pair, per Rule 2). Default selector `(0,0) = (ERA_SYS7_0_1, HERITAGE_QUICKDRAW)`.

**D-9b (NEW -- peer-skin chimera + GDI facade load-bearing).** The Win-3.1/Photoshop chrome is a PEER skin (`ERA_WIN31`, `HERITAGE_GDI`) over the SAME mechanism plus the ATKINSON spine; the Office-Space chimera is the natural co-residency of two peer skins selected per-element via origin tags, NEVER a special-case path. The Win-3.1 row holds the win31-decomp accents: caption navy `#000080` (indexed-8 documented value PRIMARY, R5), btnface `#C0C0C0`, btnshadow `#808080`, btnhilight `#FFFFFF`, menubar `#FFFFFF`. **THE GAP-CLOSURE:** the Photoshop menu bar's clip MUST route through the GDI `CombineRgn` wrapper (ADR-0005-AMENDMENT-AM-1, AM-2) -- `CombineRgn(dst, bar_rgn, vis_rgn, RGN_AND)` -- so the peer GDI facade is **load-bearing ON METAL**, not only in the oracle. Today `os/flair/menu.c:344` takes a raw `const region_t *clip` and nothing in the artifact calls `CombineRgn`. Both the QuickDraw window `strucRgn`/`contRgn` clip and the GDI menu clip run through the ONE `region_op` (P2). The single-engine guard remains the **STRUCTURAL grep gate** (no second `region_op` DEFINITION outside `region.c`), with `gdi_ref_` namespacing MANDATORY -- not a link-error backstop (the wine `region_op` signature differs, so a same-name link does not auto-collide; AM-5).

### 3.8 OD-4 supersession -- seafoam -> Initech teal (OD-4-REVOKED)

**OD-4 SUPERSEDE (folds R2/P4).** ADR-0004 **OD-4** ("desktop_bg canon = KEEP SEAFOAM `(0x6F,0xA0,0x8E)`") is **SUPERSEDED and REVOKED** (management decree 2026-06-21, heresies HER-01/HER-06/HER-08). The desktop background is **Initech teal `#8DDCDC`** (canon idx2). The title bevel is the WL-0053 lavender->teal swap (`bevel_light` `#8DDCDC`, `bevel_shadow` `#4E9BA3`). The teal/teal-shadow datum is graded with the **AUTHORED** posture (LOCKED canon constant + structure-preserving proof + derived-shadow lock + a seafoam/lavender-relapse VALUE mutant), **NEVER** against pinstripe.md lavender rows, and is **never claimed decomp-sourced** (P4 honesty -- no upstream golden exists; idx2 is a VIC-20 cyan / Initech-identity injection per WL-0053). The PINSTRIPE itself stays System-7 `#F3F3F3`/`#969696` graded versus pinstripe.md. The seafoam value `#6FA08E` is retained in the JSON `superseded` block for revocation-history traceability ONLY; nothing renders it, and the seafoam-relapse VALUE mutant (`CANON_MUTATE_TEAL #8DDCDC -> #6FA08E`) uses it as the RED-must-fire target.

> Ref: `spec/assets/color_canon.json` idx2 (`supersedes: ADR-0004 OD-4 seafoam #6FA08E (REVOKED 2026-06-21)`) and the `superseded.OD-4_seafoam` block.

### 3.9 The canon index table (reproduced from `spec/assets/color_canon.json`)

The following table reproduces the canonical index->RGB values and the part<->index crosswalk VERBATIM from the LOCKED `spec/assets/color_canon.json` (on disk; `canon_version` WL-0053, `era` `system7.0-7.1`). The hex values are authoritative; the JSON is the contract.

| idx | name | rgb | rgb_bytes | heritage | wctb_part | graded_by |
|---|---|---|---|---|---|---|
| 0 | `CIDX_BLACK` | `#000000` | `[0, 0, 0]` | system7-quickdraw | part1 (wFrameColor) | golden -- wctb binary part1 (LEG A, TOL=0) + win31 corroboration (LEG B) |
| 1 | `CIDX_WHITE` | `#FFFFFF` | `[255, 255, 255]` | system7-quickdraw | part0 (wContentColor) | golden -- wctb binary part0 (LEG A, TOL=0) + win31 corroboration (LEG B) |
| 2 | `CIDX_DESKTOP` | `#8DDCDC` | `[141, 220, 220]` | initech-identity | none (no wctb part is teal) | authored -- header-vs-JSON equality + green-cyan-octant luminance bound + seafoam-relapse mutant; supersedes OD-4 seafoam `#6FA08E` |
| 3 | `CIDX_MENUBAR` | `#FFFFFF` | `[255, 255, 255]` | system7-quickdraw | part0 (wContentColor; bar shares content white) | golden -- wctb binary part0 (LEG A, TOL=0) |
| 4 | `CIDX_TITLE_INK` | `#000000` | `[0, 0, 0]` | system7-quickdraw | part2 (wTextColor) | golden -- wctb binary part2 (LEG A, TOL=0) |
| 5 | `CIDX_ACCENT` | `#000080` | `[0, 0, 128]` | win31-gdi | none (Win-3.1/GDI accent) | golden -- win31 default-colors-cross-check.txt (LEG B); indexed-8 `#000080` PRIMARY; `#0000AA` corroboration-only; ~42-level tolerance REJECTED |
| 6 | `CIDX_CONTROL` | `#C0C0C0` | `[192, 192, 192]` | win31-gdi | none (Win-3.1/GDI BTNFACE) | golden -- win31 default-colors-cross-check.txt (LEG B); indexed-8 `#C0C0C0` PRIMARY; `#C3C7CB` is a VGA artifact, corroboration-only |
| 7 | `CIDX_PIN_LIGHT` | `#F3F3F3` | `[243, 243, 243]` | system7-quickdraw | none (rendered dither shade; NOT wctb part7 `#FFFFFF` endpoint) | golden -- pinstripe.md rendered rows (LEG C) |
| 8 | `CIDX_PIN_DARK` | `#969696` | `[150, 150, 150]` | system7-quickdraw | none (rendered dither shade; NOT wctb part8 `#000000` endpoint) | golden -- pinstripe.md rendered rows (LEG C) |

Derived rows (teal-substituted bevel; AUTHORED):

| name | rgb | rgb_bytes | heritage | wctb_part | graded_by |
|---|---|---|---|---|---|
| `bevel_light` | `#8DDCDC` | `[141, 220, 220]` | initech-identity | part9/part11 (System-7 dialog lavender `#CCCCFF`/`#DADAFF`, teal-SUBSTITUTED) | authored -- locked-constant (== idx2 teal) + lavender-relapse VALUE mutant (`#8DDCDC -> #DADAFF` MUST go RED); never graded vs pinstripe.md lavender |
| `bevel_shadow` | `#4E9BA3` | `[78, 155, 163]` | initech-identity | part9/part11 dark (System-7 lavender shadow `#B3B3DA`, teal-SUBSTITUTED) | authored -- locked-constant + DERIVED-SHADOW lock (same hue as `bevel_light`) + lavender-relapse VALUE mutant |

Superseded (retained for revocation-history traceability ONLY; nothing renders these):

| key | rgb | status |
|---|---|---|
| OD-4 seafoam | `#6FA08E` (`[111, 160, 142]`) | REVOKED 2026-06-21 (management decree, HER-01/HER-06/HER-08); superseded by idx2 teal `#8DDCDC`; the seafoam-relapse VALUE mutant uses it as the RED-must-fire target |
| preview.webp samples | `#7F7F86` white / `#67696C` menubar / `#6B6B74` pinstripe / `#1E2F87` accent | PROVENANCE ONLY (REVOKED as render/oracle source, HER-03); survive only as `INITECH_*_FRAME_V0` provenance constants |

### 3.10 The added structural oracles (DEC-09-D4)

**DEC-09-D4 (A's surviving unique oracles -- structure/boundary, not values).** Committee A contributes TWO orthogonal STRUCTURAL oracles that survive de-duplication. They grade STRUCTURE/boundary, **not values** (the value oracle is `test-color-canon`, owned by ADR-0010); neither is a by-construction oracle.

1. **`test-mech-policy`** -- a deterministic SOURCE-conformance scanner over the declared MECHANISM file set (`os/flair/surface.c`, `blitter.c`, the relocated `cfill`/`crect`/`cframe` span engine, `window.c`, `event.c`, `desktop.c` geometry, the region-clip path) asserting ZERO `0xRRGGBB` / `INITECH_*_RGB` / index->RGB switch **outside** an allowlist of EXACTLY the device-CLUT mechanism (`clut.h`) and the `flair_look_pixel` resolver TU. It grades source STRUCTURE against the C-8 invariant, never the renderer, so it cannot self-pass. MUTATION (Rule 6): `-DMECH_POLICY_MUTANT` injects one color literal into a mechanism file; the mutant build MUST go RED. This is the cheapest gate in the vector (no emulator, no golden).

2. **`test-flair-mechanism-colorblind`** -- a RENDER-SENTINEL boundary proof: compile `chrome`/`control`/`dialog` against a STUB decoration policy returning sentinel magenta for every PART; render a window; assert EVERY chrome pixel == sentinel. Any non-sentinel pixel means a mechanism module hard-coded a color -> RED. MUTATION: inject one literal `cfill(..., 0x000000u)` or one resurrected `CIDX_` constant into a mechanism file and confirm the sentinel render goes RED. This provides the behavioral teeth the grep cannot -- it catches an obfuscated or computed literal the source scanner misses.

Both oracles are added as **hard pass/fail rows** to the ADR-0004 Sec 3.8 D-8 oracle vector and run in `make test`. Neither is, or may be claimed to be, valid by construction.

---

## 4. Rationale

**On ONE module (ARB-1/ARB-2).** R1 is the load-bearing fix: four NEW modules for one concern would replace "collapse five to one" with "collapse five to four." The `.json`->`.h` generated pair is an established in-repo shape (Law 3, no new runtime). idx-keying (Committees C/D) is chosen over A's PART-keying as the BASE because the wctb and clut goldens are most naturally diffed entry-for-entry against a flat table; A's PART model is preserved as a layer ON TOP (ARB-3) rather than as a competing schema. The values already agreed across all four, so deletion-and-fold is lossless.

**On the resolver and the view (ARB-3/ARB-4/D-9).** P1 made concrete: A's PART namespace is genuinely useful (it makes the table KEY match the golden KEY) but must sit ON the idx table, not beside it; the PART->idx map is pure data and the resolution to RGB lives in exactly one place (`flair_canon_rgb`), satisfying the C-8 cut. F's registry is the right home for the era/heritage axis the chimera needs, but it must be a VIEW (fields reference the idx table, carrying idx+RGB so the field is gradeable without routing back through the render) not a third value authority. A data-only record cannot fork the engine -- the TYPE enforces P1.

**On the bevel (ARB-5).** WL-0053 P4 replaces ALL System-7 lavender tinge with Initech teal. Grading the teal-swapped parts against the lavender pinstripe.md golden would be a permanent RED or a forced tolerance loosening (Stop condition). The teal is an INJECTED Initech identity with no period golden -- the minimal honest P4 exception, gated against drift by a VALUE mutant, never claimed decomp-sourced.

**On honest sequencing (ARB-6/DEC-09-D3).** A's and C's "Step-1 collapse is byte-identical in index space, no render change" claim is factually false for chrome/control/dialog (verified: `CIDX_DESKTOP` `0x73696C` versus `palette.h` idx2 seafoam `0x6FA08E`). Landing the value oracle GREEN first, then the value flip, satisfies Rule 8 and Law 2 better than a flag-day refactor and never lands a bisect on a red desktop.

**On the crosswalk (ARB-7/ARB-8).** Pinning `wctb_part` per entry makes every value oracle grade the SAME slot against the SAME wctb part. The part7/8-versus-pinstripe.md distinction is load-bearing: conflating the WDEF endpoints (`#FFFFFF`/`#000000`) with the rendered dither (`#F3F3F3`/`#969696`) mis-grades and forces a permanent false deviation. The win-accent depth-trap is bound to idx5 because `#000080` is the documented WIN.INI value and a ~42-level tolerance would swamp the navy and let a wrong value pass.

**On the structural oracles (DEC-09-D4).** A source-scanner is decisive for a SOURCE property; the sentinel render is the behavioral teeth that catches an obfuscated/computed literal the grep misses. Both are mutation-proven. They are how the original five duplicates are prevented from recurring (`control.c` documents being copied from `chrome.c`).

---

## 5. Consequences

### 5.1 Binding Constraints

- **C-8 (NEW, bound to ADR-0004 Sec 5.1 after C-7).** No mechanism module names a color. The mechanism MAY name a palette INDEX and convert index->pixel ONLY via `flair_look_pixel(port, PART)`; it ships ZERO `0xRRGGBB` literal, ZERO `INITECH_*_RGB`, ZERO index->RGB switch below the cut-line. Color/pattern/metric cross as PARAMETERS via `grafport.h` + `spec/chrome_metrics.h`. C-8 changes ONLY by a deliberate ADR amendment.
- **ONE canon authority.** `spec/assets/color_canon.json` -> generated `spec/assets/color_canon.h` exposing `flair_canon_rgb(uint8_t idx)` is the SINGLE color-policy authority. The three competing modules (`flair_look.{h,c}` as-a-table, `flair_color_canon.{json,h}`, `initech_color_canon.h`-as-a-third-authority) are DELETED. `flair_look_pixel` is a resolver ON TOP (no table); `flair_skin_t` is a registry VIEW (no second value authority).
- **LOCKED spec-data (Rule 8).** `spec/assets/color_canon.json`, the generated `spec/assets/color_canon.h`, `spec/flair_skins.h`, and the part<->index crosswalk are LOCKED spec-data. They change only by a deliberate act with a beads issue + worklog note, never by a silent edit to make a test pass.
- **OD-4 REVOKED.** Desktop canon is Initech teal `#8DDCDC` (idx2); the bevel is teal (`bevel_light` `#8DDCDC`, `bevel_shadow` `#4E9BA3`); the pinstripe stays System-7 `#F3F3F3`/`#969696`. Seafoam `#6FA08E` is revocation-history only.
- **P4 honesty.** idx2 teal and both bevel rows are `graded_by: authored` (locked-constant + structure-preserving + derived-shadow lock + relapse mutant), NEVER claimed decomp-sourced.
- **Single-engine guard (consumed from ADR-0005-AMENDMENT-AM-1).** The D-9b menu-clip-through-`CombineRgn` is permitted ONLY because `CombineRgn` is a <=5-line shim over the ONE `region_op`; the STRUCTURAL grep gate is the load-bearing guard and `gdi_ref_` namespacing is MANDATORY.
- **Oracle additions.** `test-mech-policy` and `test-flair-mechanism-colorblind` are added to the ADR-0004 Sec 3.8 D-8 oracle vector as hard pass/fail rows and run in `make test`. Neither may be weakened or claimed valid by construction.

### 5.2 Forward Obligations

- **FO-1 (Rule-8 locked-data act).** File ONE beads issue and author `spec/assets/color_canon.json` with all 11 rows (9 indexed + 2 derived), the `wctb_part` crosswalk, era/heritage tags, and `graded_by` (`golden` for idx0/1/3/4/5/6/7/8, `authored` for idx2 + both bevel rows). NO render-site edit lands before the canon is locked. *(Satisfied on disk: `spec/assets/color_canon.json`, `canon_version` WL-0053, operator-ratified 2026-06-21.)*
- **FO-2 (ORACLE-FIRST; owned by ADR-0010).** Land `test-color-canon` (4 legs A/B/C/D) + the named value mutants GREEN and mutation-proven, with `SYSTEM7_DECOMP ?= ../system7-decomp` and a NEW `WIN31_DECOMP ?= ../win31-decomp`, loud-skip when absent, BEFORE any of the five switches is collapsed.
- **FO-3 (deliberate Rule-8 value change).** Write `tools/color_canon_extract.c` emitting `color_canon.h`; collapse all five index->RGB switches plus the `shell.c`/`desktop.c` inline literals onto `flair_canon_rgb`; flip idx2 to teal `#8DDCDC` and the bevel rows to teal; migrate `os/boot/stage2.asm` `SEAFOAM_R/G/B` (`0x6F`/`0xA0`/`0x8E`) -> `TEAL_R/G/B` (`0x8D`/`0xDC`/`0xDC`) and the `kmain.c` pre-FLAIR boot fill so there is no seafoam seam under the teal desktop. Re-run `test-color-canon` + `test-chrome` + `test-shell` + `ppm_flair_check` + `fb-agree` GREEN at each bisectable commit.
- **FO-5 (mechanism/policy structural oracles).** Land `flair_look_pixel(port, PART)` as the in-mechanism PART->idx resolver ON TOP of `flair_canon_rgb`; land `test-mech-policy` (source-scanner) + `test-flair-mechanism-colorblind` (sentinel-render), both mutation-proven, over the declared mechanism file set; add C-8 to ADR-0004 Sec 5.1. Land these GREEN and mutation-proven BEFORE relocating `cfill`/`crect`/`cframe` into the mechanism (the no-color-literal invariant must bite at the moment the span engines move).
- **FO-6 (registry view).** Land `spec/flair_skins.h` (fields by-inclusion from `color_canon.h`) + `flair_skin_resolve(era, heritage)` (total, fail-loud `PC LOAD LETTER`) + the frozen-row accretion digest over the locked base rows; reserve `ERA_SYS8_PLATINUM` with zero rows. The authored-grade and accretion oracles (`test-skin-teal`, `test-skin-era-frozen`) are owned by ADR-0006.
- **FO-8 (cross-committee with ADR-0005-AMENDMENT-AM-1 and the mechanism consumers).** Coordinate the `RectInRgn` containment->overlap fix with A's mechanism consumers (`window.c`/`desktop.c`) BEFORE landing. Verified the ONLY callers today are `region.c` and `test_region.c:435` (no live render path), so the coordination is a real but low-risk pre-landing audit; file the audit as a precondition regardless.
- **FO-9 (honesty flag, P4).** Record in the `color_canon.h` banner and the JSON that idx2 teal + both bevel rows are `graded_by: authored` (no upstream decomp golden exists), gated against drift by the VALUE mutant. Never claim independent-golden discipline for the authored value.

---

## 6. Related Decisions

- **ADR-0004 (FLAIR Toolbox Architecture, OEA-ADR-0004).** This Amendment AMENDS ADR-0004: it adds binding constraint C-8 to Sec 5.1, adds `test-mech-policy` + `test-flair-mechanism-colorblind` to the Sec 3.8 D-8 oracle vector, adds Decisions D-9 (era-layering) and D-9b (peer-skin chimera + GDI-facade-load-bearing), and SUPERSEDES OD-4 (seafoam desktop canon `#6FA08E` -> Initech teal `#8DDCDC`).
- **ADR-0005-AMENDMENT-AM-1 (Dual-Heritage Region Spine).** Owns the heritage-neutral ATKINSON re-cast, the GDI/HRGN peer wrapper family (`CombineRgn` `RGN_AND/OR/XOR/DIFF/COPY`, `GetRgnBox`, `PtInRegion`, `RectInRegion`), the heritage-symmetric truth table, the `RectInRgn` containment->overlap deep-bug fix, and the single-engine STRUCTURAL grep gate (`gdi_ref_` namespacing mandatory). This Amendment CONSUMES the `CombineRgn` shim for the D-9b menu clip and coordinates the `RectInRgn` audit (FO-8); it adds NO region math.
- **ADR-0010 (FLAIR Grading and Goldens).** CONSUMES this Amendment's canon module. Owns the ONE value oracle `test-color-canon` (4 legs A wctb / B win31 text / C pinstripe.md / D authored-teal) and its named value mutants, the `ppm_flair_check` re-key (strike "agree-BY-CONSTRUCTION", re-key onto `flair_canon_rgb`, seafoam->teal, keep structural probes), the `WIN31_DECOMP` wiring, the SSIM defer, and the loud-skip/provenance-honesty contract. SUPERSEDES ADR-0004 OD-4/FO-3/AM-9/FO-F; HARD-REVOKES HER-02.
- **ADR-0006 (FLAIR Live Event Loop and Behavioural Grading).** The booted cooperative `WaitNextEvent` loop (behind `-DBOOT_FLAIR_LIVE`) CONSUMES the locked idx2 teal `#8DDCDC` for the bare-desktop assert (E-D5 Tier-A). Owns `test-interact` (HOST, independent-by-recomputation), `test-flair-drag` (EMU), `test-skin-teal`, `test-skin-era-frozen`, and `check-win95isms`.
- **REVOCATION-RECORD-2026-06-21 (FLAIR Heresy Purge).** The management decree register of 20 candidate heresies (19 upheld, 1 struck). This Amendment EXECUTES the AMEND dispositions for HER-12 and HER-17 (mechanism/policy) and participates in the OD-4 seafoam supersession (HER-01/HER-06/HER-08 family).

---

*End of ADR-0004 Amendment DEC-09. This is a controlled document; verify revision before use. Status: RATIFIED (ADR-by-committee + chief-architect reconciliation, operator-delegated authority, 2026-06-21; operator-ratified 2026-06-22).*
