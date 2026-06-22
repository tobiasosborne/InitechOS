<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Revocation Record 2026-06-21 -- FLAIR Heresy Purge

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Revocation Record / Management Decree of Record (REV-R)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-REV-2026-06-21 |
| Title | Revocation Record 2026-06-21: FLAIR Heresy Purge |
| Version | 1.0 |
| Status | **RATIFIED (ADR-by-committee + chief-architect reconciliation, operator-delegated authority, 2026-06-21; operator-ratified 2026-06-22)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board (FLAIR Heresy-Purge Committee), STAPLER Programme |
| Effective Date | 2026-06-21 |
| Next Scheduled Review | 2026-12-21 (semi-annual, per RECORDS-POL-002) |
| Supersedes | (none in whole -- this is the authoritative revocation-of-record for the decisions enumerated in Sec 3) |
| Superseded By | (none at time of ratification) |
| Related Documents | ADR-0004 (OEA-ADR-0004, FLAIR Toolbox Architecture); ADR-0004-AMENDMENT-DEC-09 (Mechanism/Policy Split, NEW); ADR-0005 (OEA-ADR-0005, ATKINSON Region Engine); ADR-0005-AMENDMENT-AM-1 (Dual-Heritage Region Spine, NEW); ADR-0006 (FLAIR Live Event Loop and Behavioural Grading, NEW); ADR-0010 (FLAIR Grading and Goldens, NEW); WL-0053 (Initech Color Canon, operator directive); locked spec-data `spec/assets/color_canon.json` |
| Related Issues | epic `initech-qipc` (FLAIR heresy adjudication and revocation); beads issue for the Rule-8 locked-data act on `color_canon.json`; beads initech-re30.4 (switch collapse), initech-28kn (P1 ADR amendment), initech-re30.8/.9 (live event loop) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-06-21 | FLAIR Heresy-Purge Committee (26-agent comb of all FLAIR governance artifacts) | Initial Heresy Register: 20 candidate heresies combed from ADR-0004/0005, the PRD, CLAUDE.md, the worklog corpus, bd memory, and the live `os/flair` / `harness/proptest` / `spec` sources. | -- |
| 0.2 | 2026-06-21 | ARB Adjudicator (per-candidate verdicts) | Each candidate independently re-read against its cited ground truth and adjudicated: 19 upheld, 1 struck (HER-10, category error). Dispositions assigned. | ARB (full committee) |
| 1.0 | 2026-06-21 | Chief-Architect Reconciliation (synthesis) | Reconciled the verdicts with the executing-ADR map (ADR-0004-AMENDMENT-DEC-09, ADR-0005-AMENDMENT-AM-1, ADR-0006, ADR-0010). Status updated to RATIFIED under operator-delegated authority; operator-ratified 2026-06-22. | ARB (full committee) + Chief Architect |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | FLAIR Heresy-Purge Committee (epic `initech-qipc`) | Submitted | 2026-06-21 |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Concur | 2026-06-21 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Concur | 2026-06-21 |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Concur | 2026-06-21 |
| Fidelity Steward -- Law 4 Look-and-Feel | Fidelity Steward, STAPLER Programme | Concur | 2026-06-21 |
| ARB Chair (Synthesis / Chief-Architect Reconciliation) | Slydell & Porter Consulting (delegated) | Ratified | 2026-06-21 |
| Operator Ratification | T. Osborne (Operator) | Delegated to ARB; final confirmation pending | 2026-06-21 |
| Records Management | M. Waddams (Archive Annex B) | Filed | 2026-06-21 |

*Note on committee composition: The Technical/Correctness, Period-Authenticity, and Governance/Compliance reviewers correspond to the in-programme engineering functions designated in ADR-0003 Sec 1.3. The Fidelity Steward holds the Law-4 look-and-feel mandate (PRD Sec 1, Sec 3). All reviewers submitted independent findings; the Chair synthesized the reconciliation and recorded the resulting ratified decree. The operator delegated revocation authority to the committee for this purge; final operator confirmation is recorded as pending and does not block the binding effect of the decrees herein.*

---

## 1. Purpose and Scope

### 1.1 Purpose

The purpose of this Revocation Record (the "Record" or "the decree") is to constitute the **single authoritative revocation-of-record** for the FLAIR governance corpus as of 2026-06-21. A 26-agent comb of every FLAIR governance artifact -- ADR-0004, ADR-0005, the PRD, CLAUDE.md, the worklog shards (WL-0050, WL-0052, WL-0053), bd memory, and the live `os/flair` / `harness/proptest` / `spec` sources -- surfaced twenty (20) candidate decisions or capability-claims that contradict a locked principle (P1 mechanism/policy split; P2 single dual-heritage region spine; P3 grade against an independent golden, never by construction; P4 the decomp goldens + operator Initech Color canon are canon; P5 cleaner-than-1991 factoring is free and invisible). Each candidate was independently re-read against its cited ground truth and adjudicated. This Record fixes, for each, the disposition, the exact decree text, and the ADR that executes the remedy, **so that no future agent re-anchors on a revoked decision** (CLAUDE.md Rule 4, Rule 13).

This Record is itself a management decree under operator-delegated authority (epic `initech-qipc`). It does not, by itself, edit the contradicting source files; it constitutes the binding revocation and **delegates the mechanical edits to the named executing ADRs** (Sec 3, Sec 6). Where a candidate is a worklog/bd-memory supersession rather than a ratified ADR decision, this Record makes the already-recorded supersession binding (HER-04); where a candidate is a live, code-bearing anti-oracle, this Record HARD-REVOKES the offending claim outright (HER-02).

### 1.2 Scope

This Record governs:

- The disposition (HARD_REVOKE / FORMAL_REVOKE_OF_SUPERSEDED / AMEND / KEPT) of all twenty (20) candidate heresies HER-01 through HER-20 (Sec 3).
- The exact decree text binding on the executing ADR for each upheld candidate (Sec 3).
- The executing-ADR assignment that mechanically discharges each decree (Sec 3, Sec 6).
- The binding constraints and forward obligations arising from the purge (Sec 5).

### 1.3 Out of Scope

The following are expressly out of scope of this Record and are governed by the named executing ADRs:

- The complete design of the one arbitrated color-canon module (`spec/assets/color_canon.json` -> `color_canon.h` -> `flair_canon_rgb(uint8_t idx)`), the P1 mechanism/policy cut-line (constraint C-8), and the `flair_look_pixel` / `flair_skin_t` layering -- governed by **ADR-0004-AMENDMENT-DEC-09 (Mechanism/Policy Split)**.
- The heritage-neutral re-cast of ATKINSON and the GDI/HRGN peer wrapper family -- governed by **ADR-0005-AMENDMENT-AM-1 (Dual-Heritage Region Spine)**.
- The grading architecture, the one value oracle `test-color-canon`, the `ppm_flair_check` re-key, the `WIN31_DECOMP` wiring, and the SSIM defer -- governed by **ADR-0010 (FLAIR Grading and Goldens)**.
- The live cooperative `WaitNextEvent` event loop and behavioural grading -- governed by **ADR-0006 (FLAIR Live Event Loop and Behavioural Grading)**.

The locked color values themselves are reproduced in this Record solely for revocation traceability; the authoritative source is `spec/assets/color_canon.json` (Rule 8 locked spec-data).

---

## 2. Context

### 2.1 The management decree

Under operator-delegated authority (epic `initech-qipc`, 2026-06-21), the ARB was directed to comb every FLAIR governance artifact for decisions and capability-claims that contradict the locked principles, adversarially adjudicate each, and produce a Heresy Register with revocation dispositions for management decree. The chief-architect reconciliation then folded the verdicts into the four executing ADRs and verified the result composes. This Record is the resulting decree.

The precipitating event was operator worklog **WL-0053 (Initech Color Canon, 2026-06-21)**, which ruled the live FLAIR palette "VERY WRONG," set the desktop background to Initech teal `#8DDCDC` (VIC-20 cyan, hardware colour 3) **superseding ADR-0004 OD-4 seafoam `#6FA08E`**, and made the System-7 / Win-3.1 decomp corpora the canon master rather than the `spec/assets/preview.webp` Office-Space mock-up. WL-0053 was a decision-only shard (no code landed; the implementation is the subsequent `re30.4`+ work). The comb confirmed that the supersession had propagated into worklog and bd memory but **never into the ratified ADRs, the PRD, CLAUDE.md, the locked spec-data, or the live code** -- leaving a cluster of un-struck heresies-of-record plus a set of live by-construction anti-oracles.

### 2.2 The locked Initech Color Canon

The one arbitrated canon is `spec/assets/color_canon.json` (`schema_version 1`, `canon_version` "WL-0053", `era` "system7.0-7.1"). Its values, reproduced here from the locked file for revocation traceability (ASCII hex; the authoritative source is the JSON, Law 1):

| idx | name | RGB | role | heritage | provenance |
|---|---|---|---|---|---|
| 0 | CIDX_BLACK | `#000000` | ink / window frame / text | system7-quickdraw | wctb part1 (wFrameColor) |
| 1 | CIDX_WHITE | `#FFFFFF` | window / content body white | system7-quickdraw | wctb part0 (wContentColor) |
| 2 | CIDX_DESKTOP | `#8DDCDC` | desktop background -- THE Initech teal | initech-identity | AUTHORED (WL-0053; supersedes seafoam `#6FA08E`) |
| 3 | CIDX_MENUBAR | `#FFFFFF` | menu-bar background (white bar, black baseline) | system7-quickdraw | wctb part0 |
| 4 | CIDX_TITLE_INK | `#000000` | title ink / title + control title text | system7-quickdraw | wctb part2 (wTextColor) |
| 5 | CIDX_ACCENT | `#000080` | accent navy -- progress / 116% pie / caption | win31-gdi | win31 default-colors-cross-check.txt |
| 6 | CIDX_CONTROL | `#C0C0C0` | control face / BTNFACE | win31-gdi | win31 ButtonFace 192 192 192 |
| 7 | CIDX_PIN_LIGHT | `#F3F3F3` | pinstripe LIGHT row | system7-quickdraw | pinstripe.md rendered row |
| 8 | CIDX_PIN_DARK | `#969696` | pinstripe DARK row | system7-quickdraw | pinstripe.md rendered row |
| bevel_light | (derived) | `#8DDCDC` | bevel/groove HIGHLIGHT (lavender->teal swap) | initech-identity | AUTHORED (WL-0053) |
| bevel_shadow | (derived) | `#4E9BA3` | bevel/groove SHADOW (darkened teal) | initech-identity | AUTHORED (WL-0053) |

The superseded values retained for revocation traceability ONLY (nothing renders them): seafoam `#6FA08E` (`spec/assets/color_canon.json` superseded.OD-4_seafoam) and the `preview.webp` 3x3 frame samples -- white `#7F7F86`, menubar `#67696C`, pinstripe `#6B6B74`, accent `#1E2F87` (superseded.preview_webp_samples, PROVENANCE ONLY).

### 2.3 Adjudication outcome

Of the twenty candidates: **one (1)** carried a HARD_REVOKE (HER-02, the live by-construction color anti-oracle); **eight (8)** carried FORMAL_REVOKE_OF_SUPERSEDED (HER-01, 03, 04, 05, 06, 08, 16, 20 -- operator-superseded but never formally struck); **ten (10)** carried AMEND (HER-07, 09, 11, 12, 13, 14, 15, 17, 18, 19 -- live, sound-in-intent obligations the artifact does not honor, or one-sided framing); and **one (1)** was struck as not a genuine heresy and KEPT_AS_IS (HER-10, a category error -- faulting a correctly-scoped device-CLUT spec). Over-revoking a sound decision is held as bad as missing a heresy (Stop conditions).

---

## 3. The Decision -- The Revocation Register

This section is the binding register. Each candidate carries: id + title; the contradicted principle; the disposition; the exact decree text; and the executing ADR. The candidates are grouped by disposition.

### 3.0 Summary Table

| Heresy | Title (abbreviated) | Principle | Disposition | Executing ADR |
|---|---|---|---|---|
| HER-01 | OD-4 seafoam superseded by teal, unrevoked in ADR-0004 | P4 (+P3) | FORMAL_REVOKE_OF_SUPERSEDED | ADR-0010 + ADR-0004 OD-4-REVOKED block |
| HER-02 | `ppm_flair_check` grades color BY CONSTRUCTION | P3 | **HARD_REVOKE** | ADR-0010 |
| HER-03 | `preview.webp` samples are the OS render source | P4 (+P3) | FORMAL_REVOKE_OF_SUPERSEDED | ADR-0004-AMENDMENT-DEC-09 + ADR-0010 |
| HER-04 | `re30.4-palette-source` committee ruling unify-on-`palette.h` | P3 + P4 | FORMAL_REVOKE_OF_SUPERSEDED | ADR-0004-AMENDMENT-DEC-09 (confirm worklog/memory supersession) |
| HER-05 | FO-3 / AM-9 / FO-F reconcile to seafoam value | P4 (+P3) | FORMAL_REVOKE_OF_SUPERSEDED | ADR-0010 |
| HER-06 | D-8 `canon` hard-gate row enforces "seafoam desktop" | P4 | FORMAL_REVOKE_OF_SUPERSEDED | ADR-0010 |
| HER-07 | Chrome "driven by chrome_metrics" but paints `preview.webp` samples | P4 (+Law 2) | AMEND | ADR-0010 (LEG C) + ADR-0004-AMENDMENT-DEC-09 |
| HER-08 | `test-palette-seafoam` self-checks vs hand-copied seafoam literal | P3 + P4 | FORMAL_REVOKE_OF_SUPERSEDED | ADR-0010 |
| HER-09 | Win-3.1 accents are literals self-checked; `win31-decomp` unwired | P3 + P4 | AMEND | ADR-0010 (WIN31_DECOMP wiring) |
| HER-10 | Quadra-650 ROM CLUT (`clut.h`) drives no rendering | -- (category error) | **KEPT_AS_IS** | (none) |
| HER-11 | SSIM documented present, but unbuilt + anchored to `preview.webp` | Law 2 / Law 4 / P3 | AMEND | ADR-0010 (SSIM defer) |
| HER-12 | P1/P2 directive never formalized; 5 color switches in mechanism | P1 + P2 | AMEND | ADR-0004-AMENDMENT-DEC-09 + ADR-0005-AMENDMENT-AM-1 |
| HER-13 | ADR-0005 ratifies ATKINSON mono-heritage QuickDraw | P2 | AMEND | ADR-0005-AMENDMENT-AM-1 |
| HER-14 | Booted OS runs NO FLAIR event loop; `flair_tick_advance` falsely doc-wired | Law 2 / Law 4 | AMEND | ADR-0006 |
| HER-15 | DEC-04 "framebuffer agreement runs now"; host model unbuilt | P3 / Law 2 | AMEND | ADR-0010 (host-model-per-mode status) |
| HER-16 | PRD headline names "the seafoam palette" as canonical look | P4 | FORMAL_REVOKE_OF_SUPERSEDED | ADR-0010 |
| HER-17 | Phase-3 GROW ruling "ONE palette module from `clut.h`" unhonored, framed as dedup | P3 + P1 | AMEND | ADR-0004-AMENDMENT-DEC-09 |
| HER-18 | ADR-0005 op-identity table labels ops QuickDraw-only | P2 | AMEND | ADR-0005-AMENDMENT-AM-1 |
| HER-19 | CLAUDE.md lists `ssim.c` present; commit 0e32c94 claims palette "corrected" | P3 / doc drift | AMEND | ADR-0010 (CLAUDE.md SSIM) |
| HER-20 | HANDOFF Sec 4 records "seafoam desktop" as built-and-green canon | P4 | FORMAL_REVOKE_OF_SUPERSEDED | ADR-0010 + ADR-0006 (SSIM resume note) |

---

### 3.1 HARD_REVOKE

A HARD_REVOKE strikes a live, code-bearing claim that is wrong now (not merely superseded by a later operator ruling). One candidate carries it.

#### HER-02 -- `ppm_flair_check` grades color BY CONSTRUCTION

- **Location:** `tools/ppm_flair_check.c:14-19, 53, 65, 160-163` (gate `test-flair-desktop`); same `flair_palette_rgb` paints live at `os/milton/kmain.c:733` and is defined at `spec/assets/palette.h:93`.
- **Contradicted principle:** **P3** -- an oracle must not compute its expected values from the SAME source the artifact renders from. `IDX(i)` returns `flair_palette_rgb(i)`, the identical function `kmain`'s present path paints with, so a wrong palette entry flows identically into both the rendered pixel and the "expected" value; the `TOL=2` diff can never bite on color. The file frames this as a virtue (`:16` "the live desktop and the host oracle agree BY CONSTRUCTION (Law 2). Colors are therefore EXACT"). This is the precise anti-pattern P3 was operator-ratified against -- it is what let the wrong `preview.webp` palette pass every gate. The `re30.3` mutation-proof (WL-0052) does NOT cover color: its mutants are structural only (ONE_MENUBAR / NO_MODAL / MODAL_BEHIND); the color limb was never shown to bite (an unmet Rule-6 obligation).
- **Disposition:** **HARD_REVOKE.** Not FORMAL_REVOKE_OF_SUPERSEDED: WL-0053 is decision-only (no code landed), so this is a live defect, not a struck-but-superseded record.
- **Decree text (binding):** REVOKED -- the "colors agree BY CONSTRUCTION (Law 2), therefore EXACT" framing in `tools/ppm_flair_check.c` (lines 14-19, 53, 67-70) and every color assertion that keys on `IDX()`=`flair_palette_rgb()` -- the oracle's expected RGB and `kmain`'s rendered pixel are the same source, so per P3 this is a self-consistency check, NOT an oracle (it is precisely what let the wrong `preview.webp` palette pass every gate). AMENDED TO: re-key every COLOR assertion (desktop bg, menubar, window white, title ink, pinstripe L/D, Win-3.1 accents) to LIVE diffs against the decomp corpora in the `test-clut` pattern -- chrome / menubar / white / pinstripe from `../system7-decomp` goldens, accents from `../win31-decomp`, desktop bg pinned to the operator teal `#8DDCDC` canon (WL-0053) -- loud-skipping when the sibling repo is absent; keep the structural probes (two-bar chimera tell, period-2 pinstripe, 1px frame, z-order occlusion) which are palette-value-independent and sound. The `re30.3` mutation-proof (WL-0052) does NOT cover color: its mutants are structural only (ONE_MENUBAR / NO_MODAL / MODAL_BEHIND); a palette-VALUE mutant MUST be added and proven to bite (Rule 6). Record in CLAUDE.md / PRD that by-construction palette checks are not oracles (P3).
- **Executing ADR:** **ADR-0010 (FLAIR Grading and Goldens)** -- which HARD-REVOKES HER-02 in writing, owns the one value oracle `test-color-canon` (legs A wctb binary / B win31 text / C pinstripe.md / D authored-teal), owns the `ppm_flair_check` re-key (one edit: re-key color expectations onto `flair_canon_rgb`, strike the "agree by construction" comment, seafoam->teal `#8DDCDC`, keep structural probes, add the HER-02-mandated palette-VALUE mutant `FLAIR_CANON_MUTATE_MENUBAR_WHITE` `#FFFFFF`->preview `#67696C`), and keeps `ppm_flair_check` as the STRUCTURE/screendump oracle ONLY (never promoted to the color value oracle, which would rebirth HER-01).

---

### 3.2 FORMAL_REVOKE_OF_SUPERSEDED

A FORMAL_REVOKE_OF_SUPERSEDED strikes, of record, a decision the operator already superseded (in WL-0053, HANDOFF, or bd memory) but which was never formally struck in the ratified ADR / PRD / CLAUDE.md / live code, so it survives as a heresy-of-record an agent could re-anchor on. Eight candidates carry it.

#### HER-01 -- OD-4 seafoam superseded by teal, unrevoked in ADR-0004

- **Location:** `docs/adr/ADR-0004-FLAIR-Toolbox-Architecture.md:131` (also `:27` "Superseded By | (none)"; restatements at `:37, :201, :253, :327, :342`); live value `spec/assets/palette.h:60`, `os/boot/stage2.asm:42-44`; mirroring oracle `tools/ppm_seafoam_check.c`.
- **Contradicted principle:** **P4** (WL-0053 sets desktop bg = Initech teal `#8DDCDC` and explicitly SUPERSEDES OD-4 seafoam `#6FA08E`); compounded by **P3** (`ppm_seafoam_check` mirrors stage2's own `SEAFOAM_*` constant -- a by-construction self-check).
- **Decree text (binding):** OD-4 hard-revoked. Per WL-0053 `desktop_bg` canon is teal `#8DDCDC` superseding seafoam `#6FA08E`. Mark ADR-0004 Superseded-By, strike the seafoam restatements, retarget `ppm_seafoam_check` to grade teal LIVE vs the decomp canon, and re-point `palette.h`, `palette.json`, `stage2.asm`.
- **Executing ADR:** **ADR-0010** (SUPERSEDES ADR-0004 OD-4 / the D-8 seafoam-desktop canon row, repointing to `#8DDCDC` + decomp-golden grading) together with the **ADR-0004 OD-4-REVOKED amendment block** appended under ADR-0004-AMENDMENT-DEC-09 (seafoam `#6FA08E` -> Initech teal `#8DDCDC`, idx2).

#### HER-03 -- `preview.webp` mock-up samples in `palette.h` are the OS render source

- **Location:** `spec/assets/palette.h:8, 24, 30, 36, 97-104`; `os/flair/chrome.c:61-68`; `os/milton/kmain.c:733`; provenance gotcha `docs/HANDOFF.md:769`; `docs/research/gui-ground-truth.md:45-46`.
- **Contradicted principle:** **P4** (the decomp goldens are canon, NOT `preview.webp`; the mock-up is provenance ONLY, never an oracle datum or render source). The muddy menubar `#67696C` and "white" `#7F7F86` are exactly the operator's WL-0053 complaint. Also **P3** (paired with HER-02 the oracle reads from the same mock-up source).
- **Decree text (binding):** Revoked of record: `preview.webp` as a palette/render source (`palette.h:8`, `chrome.c:61-68`, ADR-0004 OD-4, `HANDOFF.md:769`); it is provenance ONLY. Amended: `flair_palette_rgb`, `chrome_pal_rgb` and the sibling switches source the decomp goldens wired as live diffs -- white `#FFFFFF`, desktop teal `#8DDCDC` (supersedes OD-4 seafoam), pinstripe `#F3F3F3`/`#969696`, navy `#000080`, BTNFACE `#C0C0C0`. Implements WL-0053.
- **Executing ADR:** **ADR-0004-AMENDMENT-DEC-09 (Mechanism/Policy Split)** -- re-values the render path onto the one canon module (`flair_canon_rgb`), demoting the `preview.webp` 3x3 samples to `INITECH_*_FRAME_V0` provenance-only constants nothing renders from -- together with **ADR-0010** wiring the live decomp diff and the grep gate that fails if any renderer or oracle reads `preview.webp` as a color source.

#### HER-04 -- `re30.4-palette-source` committee ruling (unify FLAIR palette onto `preview.webp` `palette.h`)

- **Location:** `docs/worklog/WL-0053-...:21-22, 94-96`; `docs/HANDOFF.md:8`; bd memory `canonical-palette-directive-operator-2026-06-21-flair`.
- **Contradicted principle:** **P3 + P4** -- the ruling made `preview.webp` (via `palette.h`) the unified render source AND, paired with `ppm_flair_check`'s by-construction grading, made the oracle compute expected from the same source the artifact renders from.
- **Adjudicator note:** This ruling was **never a ratified ADR/CDR decision-of-record** (grep `re30.4-palette-source` over `docs/adr/` is EMPTY; it appears only in WL-0053 / HANDOFF / bd memory). It was overridden by the operator in the SAME shard and the supersession is already captured in three places. The proportionate, honest act is therefore NOT a new ADR revocation (which would manufacture a formal-revocation record for a never-ratified decision) but a management decree confirming the already-recorded supersession is binding. The ADR-level OD-4 strike is HER-01's job.
- **Decree text (binding):** The `re30.4-palette-source` committee ruling (unify the single FLAIR palette onto the `spec/assets/palette.h` `preview.webp` samples) is REVOKED of record and confirmed permanently superseded by the operator Initech Color canon (WL-0053): the single FLAIR palette/policy module sources ONLY from the decomp canon (`../system7-decomp` goldens + `../win31-decomp` accents) plus the operator teal canon and is graded by a LIVE decomp diff, NEVER from `preview.webp` `palette.h` samples (provenance-only, P4); the by-construction grading the ruling relied on is barred (P3). This ruling was never a ratified ADR/CDR decision-of-record, so it requires no new ADR revocation -- this decree makes its already-recorded supersession binding so no future agent re-anchors on it.
- **Executing ADR:** **ADR-0004-AMENDMENT-DEC-09** records the one arbitrated canon module (`color_canon.json` -> `flair_canon_rgb`) sourced from the decomp/operator canon, against which this decree's confirmation stands; no new formal-revocation ADR is created for the worklog ruling.

#### HER-05 -- Forward obligations FO-3 / AM-9 / FO-F reconcile to the seafoam value

- **Location:** `docs/adr/ADR-0004-FLAIR-Toolbox-Architecture.md:253` (FO-3), `:327` (AM-9), `:342` (FO-F); compounded by `:131` (OD-4), `:27` ("Superseded By | (none)").
- **Contradicted principle:** **P4** (these ratified forward obligations reconcile the palette TO seafoam and bake a biting gate on seafoam, contradicting the operator teal canon); also **P3** (the named oracle is a self-consistency assert against a hand-copied seafoam literal, not a graded golden).
- **Decree text (binding):** ADR-0004 FO-3 / AM-9 / FO-F (and their parent OD-4) are HEREBY FORMALLY REVOKED insofar as they reconcile `palette.json`/`palette.h` `desktop_bg` to, and assert any oracle equality against, the seafoam value `(0x6F,0xA0,0x8E)` -- that value was superseded by operator ruling WL-0053 (Initech teal `#8DDCDC`). They are AMENDED to read: reconcile the canonical `desktop_bg` (`palette.json`/`palette.h`, stage2 fill, and the renamed seafoam oracle) to Initech teal `#8DDCDC`, graded by a LIVE diff of the rendered desktop background against the operator/decomp teal canon (the `test-clut` live-diff pattern, `../system7-decomp` and `../win31-decomp` wired as goldens) and NOT by a self-consistency equality assert against a hand-copied literal; the seafoam-equality assertion is retired and ADR-0004 Superseded-By / OD-4 is updated to record WL-0053.
- **Executing ADR:** **ADR-0010** -- which SUPERSEDES ADR-0004 OD-4 / FO-3 / AM-9 / FO-F in writing and replaces the seafoam-equality assert with the LEG-D authored-teal grading (locked-constant equality + green-cyan-octant luminance bound + the `FLAIR_CANON_MUTATE_DESKTOP_TEAL` `#8DDCDC`->seafoam `#6FA08E` relapse tripwire).

#### HER-06 -- The D-8 `canon` hard pass/fail oracle row enumerates "seafoam desktop"

- **Location:** `docs/adr/ADR-0004-FLAIR-Toolbox-Architecture.md:201`.
- **Contradicted principle:** **P4** -- the ratified hard-gate canon row enforces "seafoam desktop"; an enforced oracle on seafoam would actively resist the teal correction (the more dangerous instance than OD-4 itself, because it is an ENFORCED oracle, not just a decision).
- **Decree text (binding):** In the ADR-0004 D-8 `canon` hard pass/fail oracle row (line 201), the enforced-canon datum "seafoam desktop" is REVOKED as superseded by WL-0053 (operator Initech Color Canon) and AMENDED to "Initech teal `#8DDCDC` desktop (WL-0053, supersedes OD-4)", graded LIVE against the decomp/operator teal canon per P3/P4 -- never against the by-construction seafoam value. The hourglass-not-wristwatch cursor bytes, the Photoshop menu bar, the 116% pie, and the `570-` format remain enforced canon unchanged.
- **Executing ADR:** **ADR-0010** -- amends ADR-0004 D-8 (Section 3.8 + 8.2 as AM-10): replaces the canon row's "seafoam desktop" with "Initech teal `#8DDCDC` + decomp-golden color grade" and adds the `test-color-canon` hard pass/fail row.

#### HER-08 -- `test-palette-seafoam` asserts `palette.h` `desktop_bg` == a hand-copied stale literal of stage2's seafoam constant

- **Location:** `harness/proptest/test_palette_seafoam.c:62, 65, 148-149` (gate `test-palette-seafoam`); `os/boot/stage2.asm:42-44`; `spec/assets/palette.h:57-60`.
- **Contradicted principle:** **P3** (a stale-copy self-check between two hand-maintained restatements of the same value, not a diff against an independent golden) and **P4** (it actively enforces the superseded seafoam and must go RED to adopt teal). The file labels itself "the oracle" / desktop-background gate, so the oracle-status claim is the heresy.
- **Decree text (binding):** The `test-palette-seafoam` gate (`harness/proptest/test_palette_seafoam.c`) and the ADR-0004 OD-4 / FO-3 / AM-9 obligation it implements are REVOKED: asserting `palette.h` `desktop_bg` == a hand-copied restatement of `stage2.asm`'s SEAFOAM constant is a by-construction self-check between two hand-maintained copies of one value (violating P3) that enforces the seafoam `#6FA08E` value already superseded by WL-0053's Initech teal `#8DDCDC` (violating P4). It is REPLACED by a gate that live-diffs `palette.h` `desktop_bg` against the single locked Initech-teal canon datum `#8DDCDC` sourced from the operator/decomp canon (not a literal copied out of `stage2.asm`), with `stage2.asm` and `palette.h` both consuming that one canon value so agreement is structural rather than a re-typed copy, the duplicated `STAGE2_SEAFOAM_*` literals removed, and the gate renamed off "seafoam"; ADR-0004's Superseded-By / OD-4 / FO-3 / AM-9 entries are amended to record the supersession.
- **Executing ADR:** **ADR-0010** -- RETIRES the by-construction `test-palette-seafoam` and DELETES/repoints `tools/ppm_seafoam_check.c` to teal; its desktop-bg grading folds into `test-color-canon` LEG D (authored teal vs the locked JSON datum).

#### HER-16 -- PRD headline acceptance test names "the seafoam palette" as the canonical look

- **Location:** `InitechOS-PRD.md:17`.
- **Contradicted principle:** **P4** -- the PRD markets "the seafoam palette" as the fidelity target, but WL-0053 replaced the desktop/palette canon with Initech teal `#8DDCDC` and made the System-7 decomp goldens the master.
- **Decree text (binding):** In `InitechOS-PRD.md:17` the acceptance-test phrase "down to the seafoam palette" is REVOKED as a superseded canon (operator ruling WL-0053:46-47, ratified, SUPERSEDES ADR-0004 OD-4) and AMENDED to read "down to the Initech-teal (`#8DDCDC`) desktop and the System-7 decomp-golden chrome palette (per WL-0053; supersedes ADR-0004 OD-4 seafoam `#6FA08E`)," leaving the Chicago/Geneva bitmaps, folder icons, and 116% pie unchanged.
- **Executing ADR:** **ADR-0010** -- owns the decomp-golden master and the canon-source-correction record; the PRD line is corrected under its supersession of ADR-0004 OD-4.

#### HER-20 -- HANDOFF Sec 4 records "the seafoam desktop" as built-and-green canon

- **Location:** `docs/HANDOFF.md:645, 657` (section 4 "What is built and green (do not redo)"), `:661` (gray-white self-report), `:8` (the operator supersession), `:269` (SSIM resume note).
- **Contradicted principle:** **P4** -- section 4 presents "the seafoam desktop" / "COMPOSES THE OFFICE SPACE FRAME: seafoam" as built-and-green canon-accurate reality, with no supersession marker, while the live render is still the superseded seafoam (`palette.h:60`) because `re30.4` is unlanded. The "canon oracle" it cites is the self-named seafoam gate. (The line-269 SSIM note is a secondary clarity amendment, not itself a heresy.)
- **Decree text (binding):** REVOKED from the do-not-redo record: HANDOFF.md section 4's labelling of "the seafoam desktop, the canon oracle" (line 645) and "COMPOSES THE OFFICE SPACE FRAME: seafoam" (line 657) as built-and-green canon -- WL-0053 (HANDOFF.md:8, operator) superseded desktop seafoam `#6FA08E` -> Initech teal `#8DDCDC` and ruled the `preview.webp`-sampled palette non-canon, and the seafoam value still renders (`palette.h:60`) because `re30.4` is unlanded; section 4 is AMENDED to read "desktop (teal `#8DDCDC` per WL-0053; the live seafoam `#6FA08E` render is PRE-CANON, mock-up-sourced, retargeted by the pending `re30.4`)" and the line-661 gray-white `#7F7F86` note is tied to that operator override. SECONDARY CLARITY AMENDMENT (not itself a heresy): annotate the line-269 resume note to record that "SSIM guide-not-gate" is the intended Law-4 policy but `harness/ssim.c` is UNBUILT (`make ssim` = `stub_fail`; `ssim_params.h` crop table all TODO_GOLDEN) -- M4 fidelity currently rests on `test-chrome`/`test-canon` structural gates + operator eyeball until the SSIM harness is built against the decomp goldens (P3/P4).
- **Executing ADR:** **ADR-0010** (the desktop seafoam->teal supersession + the do-not-redo correction of record) together with **ADR-0006** (the SSIM resume-note clarity amendment, which records the live-event-loop and SSIM forward gaps honestly).

---

### 3.3 AMEND

An AMEND keeps a sound decision or sound intent but corrects a live de-facto contradiction (the code/doc does not honor the obligation) or a one-sided framing. Ten candidates carry it.

#### HER-07 -- Chrome ratified "driven by chrome_metrics" but `chrome.c` paints `preview.webp` samples

- **Location:** `docs/adr/ADR-0004-...:189, 198`; `spec/chrome_metrics.h:108-118, 330, 337`; `os/flair/chrome.c:36, 67-68`; `harness/proptest/test_chrome.c:219`.
- **Contradicted principle:** **P4** (the locked spec carries decomp-golden pinstripe RGBs `#F3F3F3`/`#969696` cited to `../system7-decomp`, but the renderer paints the forbidden `preview.webp` samples `0x6B6B74`/`0x8A8A93`; the decomp constants are cited-but-not-wired decoration) and a **Law-2** de-facto contradiction (`test-chrome` asserts only index identity + alternation, never `== 243`/`150`, so the wrong RGB passes every gate).
- **Decree text (binding):** ADR-0004 D-7 is AMENDED: chrome GEOMETRY (title-bar height, pinstripe period, box/scrollbar metrics) is measured from the frame, but chrome COLOR is canon from the decomp golden (P4) -- the pinstripe MUST render the `chrome_metrics.h` decomp constants `#F3F3F3`/`#969696`, and the `preview.webp` samples `0x6B6B74`/`0x8A8A93` in `os/flair/chrome.c:67-68` (and the `control.c`/`dialog.c`/`render.c`/`palette.h` twins) are REVOKED as a render source. D-8 is AMENDED: `test-chrome` MUST live-diff the rendered title-bar pinstripe RGB against the `../system7-decomp` golden (`s7_doc_window.png` / `pinstripe.md`), not merely assert index identity and alternation -- the currently-dead `FLAIR_CHROME_PINSTRIPE_LIGHT/DARK_R/G/B` constants must drive both the renderer and the oracle, until then D-8's "Chrome renders match chrome_metrics v1" is recorded as an unmet forward obligation.
- **Executing ADR:** **ADR-0010** (the LEG-C pinstripe grading vs `pinstripe.md` rendered rows, three-way corroborated == `chrome_metrics.h`, resolving HER-07) together with **ADR-0004-AMENDMENT-DEC-09** (the `chrome_pal_rgb` / `ctrl_pal_rgb` / `dlg_pal_rgb` / `render_palette_rgb` re-body onto `flair_canon_rgb`, deleting the `preview.webp` samples).

#### HER-09 -- Win-3.1 accent colors are bare in-file literals self-checked; `../win31-decomp` has ZERO Makefile wiring

- **Location:** `harness/proptest/test_control_record.c:248-261` (SECTION G); `spec/control_record.h:263`; Makefile (`grep -c win31-decomp` = 0).
- **Contradicted principle:** **P3** (goldens come from the decomp corpora wired as LIVE diffs, not hand-copied literals with a citation comment) + **P4** (Win-3.1 supplies accents from `../win31-decomp`). The colors are correct, so the harm is an unproven oracle, not a wrong value (hence AMEND).
- **Decree text (binding):** The SECTION G Win-3.1 accent assertions in `harness/proptest/test_control_record.c` (and the `spec/control_record.h` `W31_BTN_*` color literals) are AMENDED: as ratified by P3/P4 they MUST be graded by a LIVE diff against `../win31-decomp` goldens (button-bevel / system-metrics), mirroring the `SYSTEM7_DECOMP`/`test-clut` pattern with a loud-skip when the sibling repo is absent, and the Makefile MUST be wired with `WIN31_DECOMP ?= ../win31-decomp` and pass it to the test build; the prior self-referential `W31_BTN_*_RGB == <identical literal>` checks are revoked as by-construction self-consistency that P3 forbids -- until the live diff lands the Win-3.1 accent colors are recorded as UNPROVEN.
- **Executing ADR:** **ADR-0010** -- wires `WIN31_DECOMP ?= ../win31-decomp` (the first win31 wiring) and grades idx5 navy `#000080` + idx6 BTNFACE `#C0C0C0` on LEG B vs `default-colors-cross-check.txt`, with the win-depth-trap guardrail (indexed-8 `#000080` PRIMARY; `#0000AA` corroboration-only; ~42-level tolerance REJECTED).

#### HER-11 -- SSIM documented as a present per-window fidelity guide, but unbuilt + anchored to `preview.webp`

- **Location:** `docs/adr/ADR-0004-...:203`; `InitechOS-PRD.md:15, 17, 210, 243, 282`; `CLAUDE.md:106, 343, 388`; `Makefile:8249-8250`; `spec/ssim_params.h:163, 256-259`; `harness/ssim.c` ABSENT.
- **Contradicted principle:** **Law 2 / Law 4 / P3** de-facto contradiction (a documented oracle the code does not honor: `make ssim` is `stub_fail`, `harness/ssim.c` does not exist, the crop table is all `STATUS_TODO_GOLDEN`) plus **P4** (the fidelity anchor is the forbidden `preview.webp` frame). The guide-not-gate POLICY is sound; only the present-tense capability claim and the `preview.webp` anchor are the heresy.
- **Decree text (binding):** SSIM is hereby restated as an UNIMPLEMENTED FORWARD OBLIGATION wherever it is asserted as present (ADR-0004 ssim row line 203; PRD lines 15, 17, 210, 243, 282; CLAUDE.md lines 106, 343, 388; `spec/ssim_params.h` crop-table prose): no `harness/ssim.c` exists, `make ssim` is a deliberate gate `stub_fail`, and all `ssim_params.h` crops are `STATUS_TODO_GOLDEN` -- until built, fidelity rests on the structural oracles (`test-region`/`test-chrome`/`test-event`/`canon`/`drag-gate`) plus the operator Law-4 eyeball. The guide-not-gate intent (ADR-0004 D-8) is preserved; when built, `harness/ssim.c` shall grade per-window against the `../system7-decomp` and `../win31-decomp` RENDERED goldens (P4) with golden-resolved crops, and NEVER against `spec/assets/preview.webp` or the film-still frame (which are provenance-only mock-ups, forbidden as oracle data per P4).
- **Executing ADR:** **ADR-0010** -- defers SSIM honestly, keeps the `ssim` row GUIDE-ONLY but marked DEFERRED-UNBUILT with its beads id, and binds the eventual golden to the decomp corpora.

#### HER-12 -- P1/P2 operator directive never formalized; code smears color policy across 5 mechanism switches

- **Location:** bd memory `ratified-operator-directive-2026-06-21-no-relitigation`; WL-0052/WL-0053; `os/flair/{chrome.c:57, control.c:64, dialog.c:84, render.c:56}`, `spec/assets/palette.h:93`; ADR-0004/0005 (no mechanism/policy-split or shared-HRGN-spine language; grep = 0).
- **Contradicted principle:** **P1 + P2** de-facto -- the directive IS P1 and P2 and is SOUND, but its own forward obligation ("to be formalized as an ADR amending ADR-0004/0005") is unhonored, while the live code duplicates color policy across five hand-maintained index->RGB switches inside the imaging mechanism. The directive must NOT be revoked -- it must be FORMALIZED (hence AMEND).
- **Decree text (binding):** ADR-0004 and ADR-0005 are AMENDED to carry the operator-ratified directive of 2026-06-21 as binding invariants: ADR-0004 gains a mechanism/policy(decoration) split making the imaging/window MECHANISM (surface/GrafPort verbs, ATKINSON clip, Window/Event geometry) policy-free and parameterized on color/pattern/metric, with chrome/control/dialog look as a decoration layer on top; ADR-0005 gains a companion note declaring `os/flair/atkinson` the SINGLE shared dual-heritage spine behind BOTH QuickDraw Rgn AND GDI HRGN, neither heritage owning it. The forward obligation to extract color into ONE policy module the mechanism reads as a parameter -- killing the five hand-maintained index->RGB switches (`chrome_pal_rgb`, `ctrl_pal_rgb`, `dlg_pal_rgb`, `render_palette_rgb`, `flair_palette_rgb`) as a policy/mechanism cut sourced from the decomp/operator canon, not a dedup -- is recorded and remains binding until the code honors it (tracked: initech-28kn, initech-re30.4).
- **Executing ADR:** **ADR-0004-AMENDMENT-DEC-09 (Mechanism/Policy Split)** -- binds constraint C-8 (mechanism names palette INDICES via `flair_look_pixel`, ZERO color literal/switch below the cut-line; color/pattern/metric cross as PARAMETERS via the `grafport.h` seam) and adds the two structural oracles (`test-mech-policy` source-scanner, `test-flair-mechanism-colorblind` sentinel-render) to the D-8 vector -- together with **ADR-0005-AMENDMENT-AM-1** (the companion single-spine note).

#### HER-13 -- ADR-0005 ratifies ATKINSON mono-heritage QuickDraw; QuickDraw-only public face + obligations

- **Location:** `docs/adr/ADR-0005-ATKINSON-Region-Engine.md:4, 60, 65-73, 205-207`; `spec/region_algebra.h:321-345`; `ADR-0004:144`.
- **Contradicted principle:** **P2** (ATKINSON = the SINGLE shared dual-heritage spine behind BOTH QuickDraw Rgn AND GDI HRGN; NEITHER heritage owns it). The ADR's purpose clause defines the engine mono-heritage QuickDraw, never mentions GDI/HRGN/Win-3.1/CombineRgn, exposes only a QuickDraw public wrapper set, and no forward obligation requires the Win-3.1 chrome path to route through ATKINSON. The underlying `region_op` (region.c:403) is genuinely heritage-neutral, so the contradiction is in the framing + missing peer + missing obligation (hence AMEND, not HARD_REVOKE).
- **Decree text (binding):** ADR-0005's purpose clause ("the QuickDraw-style region algebra," :60) and its QuickDraw-only public face and forward obligations are AMENDED to honor P2: ATKINSON is hereby re-stated as the SINGLE heritage-neutral scanline inversion-list Boolean set engine that is the shared spine behind BOTH the System-7 QuickDraw Rgn AND the Win-3.1/GDI HRGN, neither heritage owning it; `spec/region_algebra.h` Section 7 is renamed "VERBATIM HERITAGE OP-NAME WRAPPERS" and gains a GDI/HRGN wrapper block (`CombineRgn` RGN_AND/OR/DIFF/XOR, `GetRgnBox`, `PtInRegion`, `RectInRegion`) that is a peer of the QuickDraw wrappers over the identical Section-6 `region_op` primitives; and a new FO-4 is added requiring both chrome heritages to clip through ATKINSON with a conformance check asserting both wrapper families dispatch to the same primitives and that no second region engine exists, cross-referenced from ADR-0004 layer 2.
- **Executing ADR:** **ADR-0005-AMENDMENT-AM-1 (Dual-Heritage Region Spine)** -- re-casts ATKINSON heritage-neutral (the ONE scanline inversion-list Boolean spine behind BOTH QuickDraw Rgn AND GDI HRGN), adds the GDI/HRGN peer wrapper family dispatching to the ONE `region_op`, the heritage-symmetric truth table, the `RectInRgn` containment->overlap deep-bug fix (`region_rect_fully_in` vs `region_rect_overlaps`), and the structural grep gate as the single-engine guard (`gdi_ref_` namespacing mandatory; the homomorphism oracle UNTOUCHED).

#### HER-14 -- Booted OS runs NO FLAIR event loop; `flair_tick_advance` falsely doc-wired as PIT-ISR

- **Location:** `os/milton/kmain.c:1138` (`cli;hlt` halt); `os/flair/event.c:28, 73, 98` + `event.h:134` (doc-claim "called from the PIT ISR"); `grep flair_tick_advance os/milton os/boot` = empty; `os/milton/pit.c:75`, `kbd.c:201`; `ADR-0004:140-143, 200`.
- **Contradicted principle:** **Law 2 / Law 4** de-facto contradiction -- the ADR ratifies a live cooperative event spine (WaitNextEvent pump, EventRecord synthesis in task context, PIT-tick yield) as the running model, but `flair_tick_advance` has ZERO ISR callers, the PIT ISR bumps only DOS `g_ticks`, the kbd ISR feeds only the DOS `g_kbd` ring, and `BOOT_FLAIR_SHELL` composes/presents once and halts. The present-tense "called from the PIT ISR" doc-claims are a Law-1 falsehood. D-1/D-4/D-6 are sound TARGET architecture (hence AMEND, not REVOKE).
- **Decree text (binding):** ADR-0004 D-1/D-4/D-6 (cooperative WaitNextEvent spine, ISR enqueue-only, PIT-tick yield) are AFFIRMED as TARGET architecture and NOT revoked; the ADR is AMENDED with an M3.1 STATUS note in Sec 5.2/D-8 and a new forward obligation FO-H: as of M3.1 the booted OS (`kmain flair_desktop_run`, `BOOT_FLAIR_SHELL`) renders ONE static FLAIR frame and halts (`cli;hlt`, `kmain.c:1138`) -- `flair_tick_advance` has ZERO ISR callers, `pit_irq_handler` bumps only DOS `g_ticks`, `kbd_irq_handler` feeds only the DOS `g_kbd` ring, and FLAIR behaviour is gated ONLY by the host replay oracle (`make test-event`) vs self-computed ground truth, with NO live event-loop gate in `TEST_EMU_GATES`; a live ISR-fed WaitNextEvent pump (`flair_tick_advance` into `pit_irq_handler`, a FLAIR raw-input ring fed by the kbd/PIT ISRs, an emu-level behaviour gate) is a FORWARD OBLIGATION (beads initech-re30.8/.9) gating the Law-4 live-desktop claim, to land before that claim is asserted. Concurrently the present-tense doc-claims that `flair_tick_advance` is called from the PIT ISR (`event.c:28, 73`; `event.h:134`) are STRUCK and re-worded to INTENDED to be called from the PIT ISR (NOT yet wired as of M3.1; beads initech-re30.8) -- documenting an unwired path as wired is a Law-1 violation.
- **Executing ADR:** **ADR-0006 (FLAIR Live Event Loop and Behavioural Grading)** -- wires the booted-OS cooperative WaitNextEvent loop (`flair_tick_advance` into the PIT ISR; FLAIR raw-input SPSC ring fed by kbd IRQ1 + mouse IRQ12 ISR-enqueue-only; the pump replacing render-once-and-HLT; behind `-DBOOT_FLAIR_LIVE`) and records the two behavioural oracles by KIND (HOST `test-interact`; EMU `test-flair-drag`), neither by-construction, plus the HER-14 drag-noop mutant. HER-14 is marked RESOLVED only when the OS boots interactively AND the drag gate is green and mutation-proven.

#### HER-15 -- DEC-04 asserts cross-emulator framebuffer agreement "runs now"; the predictive host model is unbuilt

- **Location:** `docs/adr/ADR-0004-...:205, 305` (Sec 4.5 / DEC-04); `Makefile:8249-8250`; `spec/ssim_params.h:163`; `win31-decomp` wiring = 0.
- **Contradicted principle:** **P3 / Law-2** de-facto contradiction -- the predictive host model an emulator framebuffer would be diffed against is the unbuilt host-render/SSIM skeleton. The boot-marker tri-emulator gate (QEMU + Bochs) DOES run; the per-window host-model framebuffer agreement does not. The FROZEN AM-6 definition is sound (hence AMEND, annotate status).
- **Decree text (binding):** ADR-0004 DEC-04 / Sec 4.5 (D-8) is AMENDED: the host-model-per-mode FRAMEBUFFER agreement oracle (each emulator's screendump digest diffed against the host model's prediction for its own mode) is hereby recorded as a FORWARD OBLIGATION, not a standing oracle -- it depends on the as-yet-unbuilt host-render/SSIM pixel skeleton (`make ssim` is `stub_fail`; `spec/ssim_params.h` crop rects are all `STATUS_TODO_GOLDEN`; `win31-decomp` is unwired). What RUNS NOW cross-emulator is the boot-marker tri-emulator gate only (`test-boot` QEMU + `test-boot-bochs`: serial markers + banner-text screendump + no-triple-fault). The FROZEN AM-6 definition ("each emulator vs its own host-model prediction, NEVER a cross-emulator byte-CRC") is preserved verbatim; the word "runs now" is struck for the framebuffer arm and replaced by "pending host-model implementation" so it is not read as a standing pixel oracle.
- **Executing ADR:** **ADR-0010** -- records the cross-emulator host-model-per-mode status, preserves the AM-6 frozen definition verbatim, and marks the framebuffer arm pending the unbuilt host-render/SSIM skeleton.

#### HER-17 -- Phase-3 GROW ruling "ONE palette module from `clut.h`" unhonored + framed as a dedup

- **Location:** `docs/worklog/WL-0050:22-24`; bd memory `flair-phase-3-committee-ruling-2026-06-21`; `os/flair/chrome.c:57` + `control.c`/`dialog.c`/`render.c` + `palette.h:93`; `grep clut os/` = empty.
- **Contradicted principle:** **P3 + P1** -- the ruling's stated source (`clut.h`, the real ROM decomp table) is P3-compatible, but it is a FORWARD OBLIGATION the code does not honor (the 5 switches still exist, all render `preview.webp` samples; `clut.h` drives no rendering), and it framed the fix as a dedup'd lookup, not a policy layer the mechanism reads (P1). The GROW + single-module intent is sound and operator-re-affirmed (hence AMEND).
- **Decree text (binding):** The WL-0050 Phase-3 GROW ruling "ONE palette module from `clut.h` (kills the 5 duplicated index->RGB switches + centralizes the bpp depth-conversion)" is AMENDED: the unified module is a policy-free MECHANISM reading a COLOR/PATTERN/METRIC POLICY layer as parameters (P1), not a dedup'd index->RGB lookup; its policy values are sourced via LIVE decomp diffs from the operator-ratified Initech Color canon (per WL-0053 -- system7/win31-decomp goldens + Initech teal `#8DDCDC`, NOT the `preview.webp` `palette.h` samples, and `clut.h` remains the device CLUT, not the chrome source) (P3/P4); and the obligation is RECORDED AS OPEN AND UNHONORED -- as of `re30.3` the five switches (`chrome.c:57`, `control.c:64`, `dialog.c:84`, `palette.h:93`, `render.c:56`) all still render `preview.webp` samples and `clut.h` drives no rendering (`grep clut os/` = empty), to be discharged by `re30.4`.
- **Executing ADR:** **ADR-0004-AMENDMENT-DEC-09 (Mechanism/Policy Split)** -- reframes the unified module as a P1 policy/mechanism cut sourced from the decomp/operator canon (not a dedup), with the source question settled by WL-0053 (`clut.h` stays the device CLUT). Couples with HER-12.

#### HER-18 -- ADR-0005 op-identity truth table labels each Boolean op with only a QuickDraw name

- **Location:** `docs/adr/ADR-0005-ATKINSON-Region-Engine.md:135-140`; `spec/region_algebra.h:176-179`.
- **Contradicted principle:** **P2** -- the most neutral, purely-Boolean part of the mechanism labels each op with a single "QuickDraw name" column as if QuickDraw vocabulary were the canonical naming of the shared spine, quietly conferring ownership to one heritage. The fix is purely additive (hence AMEND).
- **Decree text (binding):** ADR-0005 section 3.3 and `spec/region_algebra.h` are AMENDED to make the op-identity naming heritage-symmetric: the truth table at `ADR-0005:135-140` gains a "GDI name" column beside the "QuickDraw name" column (`UnionRgn` / `CombineRgn RGN_OR`; `SectRgn` / `RGN_AND`; `DiffRgn` / `RGN_DIFF`; `XorRgn` / `RGN_XOR`) with a one-line note that the `boolfn` IS the op and the two name columns are equal aliases over the one ATKINSON engine (neither heritage owns it, P2); the `rgn_op` enum comments at `region_algebra.h:176-179` are mirrored to cite both aliases. No change to the Boolean ops, the truth-table values, the wrapper API, or the internal format.
- **Executing ADR:** **ADR-0005-AMENDMENT-AM-1 (Dual-Heritage Region Spine)** -- adds the GDI name column (heritage-symmetric truth table, 3 name columns) and mirrors the enum comments.

#### HER-19 -- CLAUDE.md lists `harness/ssim.c` as present; commit 0e32c94 claims palette "corrected to the decomp goldens"

- **Location:** `CLAUDE.md:106, 388`; git commit `0e32c94`; `WL-0053:5`; `spec/assets/palette.h:60`.
- **Contradicted principle:** **P3 / capability-claim doc drift** -- CLAUDE.md documents `harness/ssim.c` as an existing per-window fidelity guide and Law 4 asserts the harness reports it, but it does not exist (`make ssim` = `stub_fail`; `ssim_params.h` all `TODO_GOLDEN`). Commit `0e32c94`'s message claims the FLAIR palette source was "corrected to the decomp goldens," but it is a decision-only worklog -- no code landed; FLAIR still renders `preview.webp` samples. (Subset of HER-11's SSIM doc fixes; hence AMEND.)
- **Decree text (binding):** CLAUDE.md is AMENDED: in the file map mark `harness/ssim.c` as NOT-YET-BUILT (PLANNED; `make ssim` = `stub_fail`/exit 1, an open M0 obligation, `spec/ssim_params.h` crop rects all `STATUS_TODO_GOLDEN`), and change Law 4's "SSIM is a guide the harness reports per-window" to "SSIM is INTENDED as a per-window guide (PLANNED -- `harness/ssim.c` not yet built); until then fidelity is the structural oracles plus the operator Law-4 eyeball" -- the guide-not-gate and canon-bug clauses unchanged. The git-history framing of commit `0e32c94` is corrected of record to "Initech Color canon palette correction DECIDED (`re30.4` pending), no code landed": no render path or oracle was repointed to the decomp goldens and `palette.h` still renders `preview.webp` samples (`INITECH_DESKTOP_BG_RGB 0x6FA08E` unchanged).
- **Executing ADR:** **ADR-0010** -- owns the SSIM defer and the provenance-honesty contract; the CLAUDE.md SSIM lines and the commit-framing correction are carried under it.

---

### 3.4 KEPT (Struck -- Not a Genuine Heresy)

One candidate was struck on adjudication. Over-revoking a sound decision is held as bad as missing a heresy; the candidate is recorded here so no future agent re-files it.

#### HER-10 -- Quadra-650 ROM CLUT (`clut.h`) drives no rendering; consumed only by `test_clut.c`

- **Location:** `spec/assets/clut.h:6, 30-32`; `harness/proptest/test_clut.c:50-52, 340-414`; `grep clut os/` = no matches; `spec/CANON-MANIFEST.md:70`.
- **Alleged principle:** P3 / P4 (the decomp golden should be the LIVE diff driving rendering, but the ROM CLUT is consumed only by `test_clut.c`).
- **Disposition:** **KEPT_AS_IS** (`is_genuine_heresy: false`). No decree text.
- **Adjudicator reasoning (category error):** HER-10 mis-scopes `clut.h`. `clut.h` is the **DEVICE CLUT** -- the 256-entry indexed-8 System-7 ROM surface color table -- declared explicitly DISTINCT from the chrome canon: "`palette.*` ... is the 7-color frame-sampled canon ... `clut.*` is the full System-7 ROM table. They COEXIST; do NOT merge or edit either to match the other" (`clut.h:30-32`). `CANON-MANIFEST.md:70` maps `clut.h` to the QuickDraw `GDevice`->`PixMap`->`ColorTable`->`ColorSpec` / `Color2Index`/`Index2Color` chain -- the indexed-8 surface MECHANISM (P1 mechanism), not chrome DECORATION. The operator already adjudicated this exact role: `WL-0053:96` -- "(`clut.h` stays the device CLUT; the chrome source is the decomp canon, not `palette.h` preview-samples)." So no ratified decision obligates `clut.h` to drive chrome rendering, and the candidate's proposed replacement (make the device CLUT the chrome canon `ppm_flair_check` diffs against) is exactly the device-CLUT-vs-chrome conflation the operator disambiguated. Against P3, `test_clut.c` BLOCK 6 (`:340-414`) live-diffs every entry against `clut_8_rom.bin` -- a value the artifact does NOT render from -- making it the **model EXEMPLAR** of "grade against an independent golden, never by construction," not a violation. Against P4, `clut.h` IS a decomp golden correctly wired into its own independent oracle; P4 names `preview.webp` (not `clut.h`) as the non-oracle mock-up. The real heresies nearby (chrome render path uses `preview.webp` samples; `ppm_flair_check` by-construction) are separate, upheld candidates (HER-02, HER-03, HER-07). HER-10 as framed rests on a category error -- faulting a correctly-scoped, operator-confirmed device-CLUT spec whose sole consumer is the independent oracle the locked principles endorse. **KEPT.**

---

## 4. Rationale

The purge is governed by four reconciliation principles:

1. **Disposition tracks the kind of defect, not its severity.** A live, code-bearing anti-oracle that is wrong NOW (HER-02) is HARD_REVOKED; an operator-superseded-but-unstruck decision (HER-01, 03, 04, 05, 06, 08, 16, 20) is FORMAL_REVOKE_OF_SUPERSEDED -- the value was already killed by WL-0053, this Record merely strikes the surviving heresy-of-record so an agent cannot re-anchor; a sound decision or intent the artifact does not yet honor, or a one-sided framing (HER-07, 09, 11, 12, 13, 14, 15, 17, 18, 19), is AMENDED -- the intent stands, the obligation is recorded as open or the framing corrected.

2. **Independent goldens, never by construction (P3).** The load-bearing failure across the corpus is by-construction grading: `ppm_flair_check` (HER-02), `test-palette-seafoam` (HER-08), the SECTION-G win31 literals (HER-09), and the FO-3/AM-9/FO-F self-consistency assert (HER-05) all compute their expected values from the same source the artifact renders from, or from a hand-copied restatement thereof. Every remedy re-keys grading onto a LIVE diff against an INDEPENDENT decomp golden (the `test-clut` idiom), the one value oracle `test-color-canon` (legs A wctb binary / B win31 text / C pinstripe.md / D authored-teal), loud-skipping when the sibling corpus is absent.

3. **The decomp goldens + the operator Initech Color canon are canon; `preview.webp` is provenance ONLY (P4).** The mock-up is the dim Office-Space CRT still; its 570-/116%/Photoshop-menu-bar tells are canon, but its 3x3 color samples are not (HER-03, HER-07, HER-11, HER-16, HER-20). The desktop is Initech teal `#8DDCDC` (idx2), superseding seafoam `#6FA08E` (HER-01, 05, 06, 08, 16, 20); the seafoam and `preview.webp` values survive only as superseded baselines for revocation traceability and as the RED-must-fire targets of the value mutants.

4. **Mechanism/policy split (P1) and the single dual-heritage spine (P2) are formalized, not relitigated.** The operator P1/P2 directive is sound and was never the contradiction (HER-12); the contradiction was its unhonored ADR obligation and the five color switches inside the mechanism. The remedy formalizes C-8 (the cut-line) and re-casts ATKINSON heritage-neutral with a GDI peer family (HER-13, 18), neither relitigating the clean-room region decision nor weakening the homomorphism oracle (Stop condition). HER-10 is the discipline's mirror: a correctly-scoped device-CLUT spec is KEPT precisely because over-revoking a sound decision is as bad as missing a heresy.

---

## 5. Consequences

### 5.1 Binding Constraints

The following constraints are binding upon the FLAIR subsystem as a consequence of this purge; they are owned and elaborated by the named executing ADRs:

- **BC-1 (P3, owned by ADR-0010).** No FLAIR color oracle may compute its expected value from the source the artifact renders from. The one value oracle is `test-color-canon`, grading the generated `color_canon.h` against an INDEPENDENT decomp golden, with a mandatory VALUE mutant per leg (Rule 6). `ppm_flair_check` remains the STRUCTURE/screendump oracle ONLY and is never promoted to the color value oracle.
- **BC-2 (P4, owned by ADR-0010 / ADR-0004-AMENDMENT-DEC-09).** `spec/assets/preview.webp` may NOT be read by any renderer or oracle as a color source, enforced by a grep gate. The `preview.webp` 3x3 samples survive only as `INITECH_*_FRAME_V0` provenance constants. The desktop background is Initech teal `#8DDCDC` (idx2); seafoam `#6FA08E` is retired.
- **BC-3 (P1, constraint C-8, owned by ADR-0004-AMENDMENT-DEC-09).** No mechanism module may contain a `0xRRGGBB` literal, an `INITECH_*_RGB`, or an index->RGB switch; it names a palette INDEX and resolves it ONLY through `flair_look_pixel(port, PART)` -> `flair_canon_rgb(idx)`. The five drifted switches (`chrome_pal_rgb`, `ctrl_pal_rgb`, `dlg_pal_rgb`, `render_palette_rgb`, `flair_palette_rgb`) collapse onto the one accessor. Enforced by `test-mech-policy` (source-scanner) and `test-flair-mechanism-colorblind` (sentinel-render).
- **BC-4 (P2, constraint C-7, owned by ADR-0005-AMENDMENT-AM-1).** There is exactly ONE region engine (`region_op`) behind BOTH the QuickDraw Rgn AND the GDI HRGN wrapper families, neither heritage owning it. No second `region_op` DEFINITION may exist outside `region.c` (the load-bearing structural grep gate; `gdi_ref_` namespacing mandatory). The homomorphism oracle is UNTOUCHED.
- **BC-5 (provenance honesty, owned by ADR-0010).** Every color row carries an explicit provenance tag; `authored` rows (idx2 teal, both bevel rows) are NEVER claimed decomp-sourced and are gated by a locked-constant equality + a relapse VALUE mutant. A green run that loud-skipped N rows MUST print "N rows NOT graded (goldens absent)"; a skip-everything run can never be mistaken for a pass.

### 5.2 Forward Obligations

- **FO-1 (ADR-0004-AMENDMENT-DEC-09 + ADR-0010).** Author the one LOCKED `spec/assets/color_canon.json` with all 11 rows + the wctb_part crosswalk + era/heritage tags (Rule-8 act, one beads issue + worklog note); land `test-color-canon` (4 legs) + the named VALUE mutants GREEN and mutation-proven, with `WIN31_DECOMP ?= ../win31-decomp` wired, BEFORE any of the five switches is collapsed (oracle-first; HER-02/05/06/07/08/09).
- **FO-2 (ADR-0004-AMENDMENT-DEC-09).** Collapse the five switches and flip the three divergent chrome/control/dialog desktop values onto the canon as a deliberate Rule-8 act sequenced oracle-first (the value oracle GREEN against the canon BEFORE the render flip); the "no render change" claim is struck (HER-03/07/12/17).
- **FO-3 (ADR-0005-AMENDMENT-AM-1).** Land the GDI/HRGN peer wrapper family + the heritage-symmetric truth table + `test-region-gdi` (L1/L2/L3) + mutants + the `RectInRgn` containment->overlap fix; the homomorphism oracle unchanged (HER-13/18).
- **FO-4 (ADR-0006).** Wire the live ISR-fed WaitNextEvent pump behind `-DBOOT_FLAIR_LIVE`; land `test-interact` (HOST) + `test-flair-drag` (EMU, 2 tiers) + the bochs mouse leg + mutants (incl. the drag-noop). HER-14 is RESOLVED only when the OS boots interactively AND the drag gate is green and mutation-proven. Strike the false `flair_tick_advance` "called from the PIT ISR" doc-claims.
- **FO-5 (ADR-0010).** SSIM remains an UNIMPLEMENTED forward obligation everywhere it is asserted as present (HER-11/15/19); when built it grades per-window against the decomp RENDERED goldens, never `preview.webp`. The cross-emulator host-model-per-mode framebuffer arm is recorded as a forward obligation; the boot-marker tri-emulator gate is what runs now.
- **FO-6 (records).** The PRD (`:17`), CLAUDE.md (`:106, 388`), HANDOFF Sec 4 (`:645, 657, 661`), and the `0e32c94` commit framing are corrected of record to match the ratified teal canon and the unbuilt-SSIM reality (HER-16/19/20).

---

## 6. Related Decisions

This Record is the authoritative revocation-of-record for the FLAIR governance corpus as of 2026-06-21. The mechanical remedies are discharged by:

- **ADR-0004 (OEA-ADR-0004) -- FLAIR Toolbox Architecture.** The base ADR amended by this purge (OD-4, D-7, D-8 canon row, FO-3/AM-9/FO-F, DEC-04, the event-loop decisions D-1/D-4/D-6).
- **ADR-0004-AMENDMENT-DEC-09 -- Mechanism/Policy Split (NEW).** Executes HER-03 (render re-value), HER-04 (canon-module confirmation), HER-07 (chrome re-body), HER-12 (P1 formalization), HER-17 (P1 reframe). Owns binding constraint C-8 and the one canon module; adds `test-mech-policy` + `test-flair-mechanism-colorblind` to the D-8 vector; appends the ADR-0004 OD-4-REVOKED block (seafoam `#6FA08E` -> Initech teal `#8DDCDC`, idx2).
- **ADR-0005 (OEA-ADR-0005) -- ATKINSON Region Engine.** The base region ADR amended by this purge (purpose framing, op-identity table, forward obligations).
- **ADR-0005-AMENDMENT-AM-1 -- Dual-Heritage Region Spine (NEW).** Executes HER-13 (heritage-neutral re-cast), HER-18 (GDI name column), and the region half of HER-12. Owns binding constraint C-7 (one engine, two peer facades), the GDI/HRGN wrapper family, the heritage-symmetric truth table, the `RectInRgn` overlap fix, and `test-region-gdi`.
- **ADR-0006 -- FLAIR Live Event Loop and Behavioural Grading (NEW).** Executes HER-14 (the live WaitNextEvent loop + the struck doc-claims) and the SSIM-resume-note half of HER-20. Records the two behavioural oracles by kind and the forward gaps (default-boot flip, 86Box, SSIM).
- **ADR-0010 -- FLAIR Grading and Goldens (NEW).** HARD-REVOKES HER-02 in writing; executes HER-01, 05, 06, 08, 11, 15, 16 and the SSIM half of HER-19/HER-20, and the grading half of HER-07/HER-09. SUPERSEDES ADR-0004 OD-4 / FO-3 / AM-9 / FO-F. Owns the one value oracle `test-color-canon`, the `ppm_flair_check` re-key, the `WIN31_DECOMP` wiring, the SSIM defer, the cross-emulator status, and the loud-skip / provenance-honesty contract.
- **WL-0053 -- Initech Color Canon (operator directive, 2026-06-21).** The precipitating supersession; the source of truth for the teal canon, made binding-of-record by this purge.
- **Locked spec-data `spec/assets/color_canon.json`** (Rule 8) -- the one arbitrated canon module the remedies consume.

No part of this Record is itself subject to relitigation by a downstream agent without an operator-authorized escalation (CLAUDE.md Stop conditions). HER-10 is recorded KEPT precisely so that its (struck) candidacy is not re-filed.

---

*End of Revocation Record OEA-REV-2026-06-21. Filed to Archive Annex B. Controlled Document -- verify revision before use.*
