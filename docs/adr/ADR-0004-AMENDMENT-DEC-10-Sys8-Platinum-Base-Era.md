<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0004 Amendment DEC-10 -- The Platinum Base Era: `ERA_SYS8_PLATINUM` becomes the FLAIR base chrome era (operator Law-4 frame ruling, 2026-08-15)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0004-A2 |
| Title | ADR-0004 Amendment DEC-10: The Platinum Base Era (`ERA_SYS8_PLATINUM` as the FLAIR base chrome era) |
| Version | 0.1 |
| Status | **DRAFT -- NOT RATIFIED.** Pending (a) the minted Mac OS 8.1 goldens (beads `initech-kd60` -> `initech-slwi` -> `initech-4lmg`) and (b) operator ratification. NO value in this document may be treated as locked spec-data until both conditions are met. |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Drafted per operator directive, beads `initech-s97h` (epic `initech-t1i0`) |
| Effective Date | (none -- DRAFT) |
| Amends | ADR-0004 (FLAIR Toolbox Architecture) Sec 3.7 (D-7 chrome), Sec 3.8 (D-8 oracle vector); ADR-0004-AMENDMENT-DEC-09 Sec 3.7 (D-9 era layering, D-9b peer skin) |
| Supersedes | (1) The **era STAGING** clause of ADR-0004-AMENDMENT-DEC-09 D-9 -- "`ERA_SYS8_PLATINUM` is a RESERVED enum value with ZERO rows today", "Default selector `(0,0) = (ERA_SYS7_0_1, HERITAGE_QUICKDRAW)`". (2) The **2026-06-20 operator era-axis ruling** as recorded in `spec/CANON-MANIFEST.md` ("Era tagging rule ... 2026-06-20 operator ruling") and `spec/control_record.h:18` ("ERA AXIS (operator, 2026-06-20): System 7.0/7.1 is the BASE built now"), and its restatement in `spec/win95ism_guardrails.md:12` ("a System 7.0/7.1 BASE"). (3) The WL-0049 / WL-0053 / WL-0054 P5 staging ("System 7.0/7.1 now, ACCRETE System 8 Platinum later"). It supersedes NOTHING else: DEC-09's ARB-1..ARB-8, C-8, D-9's data-only/no-vtable rule, D-9b, OD-4-REVOKED, and the era-TAGGING rule itself all stand verbatim -- the tagging rule is what makes this flip cheap and is re-affirmed by BC-10.10. |
| Superseded By | (none) |
| Related Documents | ADR-0004 (OEA-ADR-0004); ADR-0004-AMENDMENT-DEC-09 (Mechanism/Policy Split); ADR-0010 (FLAIR Grading and Goldens); ADR-0006 (FLAIR Live Event Loop and Behavioural Grading); ADR-0005-AMENDMENT-AM-1 (Dual-Heritage Region Spine); REVOCATION-RECORD-2026-06-21 (FLAIR Heresy Purge); WL-0049, WL-0053, WL-0054, WL-0059 |
| Related Issues | `initech-s97h` (this amendment); `initech-t1i0` (EPIC, System 8 pivot); `initech-kd60` (Mac OS 8.1 image + Basilisk II boot); `initech-slwi` (mint Platinum captures); `initech-4lmg` (pixel-measured `sys8` specs); `initech-3knt` (populate rows + re-key oracles); `initech-i3si` (`chrome.c` Platinum rendering); `initech-l9cd` (GUI-interaction video harness) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-08-15 | Drafted per operator ruling (beads `initech-s97h`) | Initial DRAFT recording the operator's Law-4 frame ruling (the frame is Mac OS 8 Platinum), the base-era flip, the Initech-identity invariants, the TODO_GOLDEN discipline, the Rule-8 mechanics, the default-selector analysis, and the video-validation directive. **Contains no Platinum pixel values** -- every Platinum datum is `TODO_GOLDEN` pending the mint. | -- (pending) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Operator (Law-4 judge) | T. Osborne (Operator) | **RULED** the frame chrome is Mac OS 8 Platinum, 2026-08-15 (after extensive research). Ratification of THIS document pending. | 2026-08-15 |
| Author / Drafter | STAPLER Programme (beads `initech-s97h`) | Submitted as DRAFT | 2026-08-15 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Pending | -- |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Pending | -- |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Pending | -- |
| Fidelity Steward -- Frame Conformance | Fidelity Stewardship Function (ADR-0004 Sec 1.3) | Pending | -- |
| Records Management | M. Waddams (Archive Annex B) | Not filed (DRAFT) | -- |

---

## 1. Why this amendment

### 1.1 The ruling

On **2026-08-15**, after extensive research, the **operator ruled** that the Mac window
chrome in the *Office Space* "Saving tables to disk..." frame -- the frame that IS the
FLAIR spec (PRD Sec 1, Sec 3; Law 4) -- is **Mac OS 8 (Platinum appearance)**, not
System 7. FLAIR has been building the System-7 look as its base era. The base era is
therefore **wrong against the frame**, which is the only thing Law 4 grades against.

This is an **operator Law-4 override of a standing committee/architecture ruling**, and
it has direct precedent: **WL-0053**, where the operator inspected the shipped desktop
and ruled the palette "VERY WRONG", overturning the committee-ratified `preview.webp`
seafoam canon in favour of the decomp goldens and the Initech teal. The worklog records
the governing principle verbatim: *"The committee + I had anchored on the preview.webp
'seafoam' as ratified canon; **the operator is the human Law-4 judge and outranks
that**."* (`docs/worklog/WL-0053-initech-color-canon-palette-source-decision.md`;
summarised in `docs/HANDOFF.md` under "THE INITECH COLOR (DEFINITIVE)".) DEC-10 is the
same act applied to the era axis instead of the color axis.

### 1.2 The staging this supersedes

The era axis was staged, from the beginning, as *Sys7-now / Platinum-later*:

- **WL-0049** (the FLAIR plan): *"the ERA AXIS -- build System 7.0/7.1 now, ACCRETE
  System 8 Platinum later ('never delete, always accrete'; **the Office Space frame is
  likely Platinum, not Sys7**)."*
- **The 2026-06-20 operator era-axis ruling**, recorded in two locked places:
  `spec/control_record.h:18` -- *"ERA AXIS (operator, 2026-06-20): **System 7.0/7.1 is
  the BASE built now.** A System 8 Platinum layer accretes later as additive era-deltas
  WITHOUT rewriting the base. Every constant below carries an `era=` tag so the future
  layer can override additively."* -- and `spec/CANON-MANIFEST.md` -- *"Era tagging rule
  (Sec 1 era axis, 2026-06-20 operator ruling): every spec/ header and os/flair source
  that carries chrome metrics, color tables, WDEF geometry, or title-bar rendering logic
  carries an `era` tag so a future `../system8-decomp` or `platinum/` layer lands without
  a base rewrite."*
- **WL-0053** rule 1: *"System 7 colors are GOLDEN (System 8/Platinum accretes later
  under the same rule)."*
- **WL-0054** P5: *"era-layered (Sys7 base now / Sys8 Platinum accretes) as decoration."*
- **ADR-0004-AMENDMENT-DEC-09 D-9** (RATIFIED): *"`ERA_SYS8_PLATINUM` is RESERVED with
  ZERO rows ... Default selector `(0,0) = (ERA_SYS7_0_1, HERITAGE_QUICKDRAW)`."*
- **`spec/win95ism_guardrails.md:12`**: *"The chimera is Win 3.1 FLAT 2-D ACCENTS on a
  **System 7.0/7.1 BASE** -- never Win 95."*

Two things follow. First, DEC-10 is an operator ruling superseding **an earlier operator
ruling** (2026-06-20) on the same axis -- a clean, in-band supersession, not a challenge to
a committee. Second, WL-0049 **already recorded the suspicion** that the frame is
Platinum, and staged Sys7-first anyway (the System-7 corpus was the one that existed).
The 2026-08-15 research converts that hedge into a ruling.

DEC-10 therefore flips the *order of the accretion*, and **nothing else**. The era-TAGGING
rule from the same 2026-06-20 ruling is preserved and vindicated: it exists precisely so a
Platinum layer "lands without a base rewrite," and it is what makes this Amendment a
re-key rather than a rebuild. What is superseded is only the sentence naming which era is
BASE.

### 1.3 Why this needs an ADR amendment and not an edit

`spec/flair_skins.h` is **LOCKED spec-data** (Rule 8; DEC-09 Sec 5.1). Its base rows are
protected by a committed accretion digest (`test-skin-era-frozen`,
`SKIN_FROZEN_DIGEST 0xDEF099AC`) whose entire purpose is to make an unsanctioned base-row
edit go RED. Changing the base era by editing that file is exactly the "silent edit to
make one test pass" Rule 8 forbids. The change is therefore recorded here first, ratified,
and only then executed (`initech-3knt`).

---

## 2. Scope

### 2.1 In scope

- The **base/default chrome era** of FLAIR window and control decoration (Sec 3.1, 3.2).
- The **Initech-identity invariants** that survive the era flip (Sec 3.3).
- The **provenance discipline** for every Platinum value (Sec 3.4).
- The **Rule-8 mechanics** of the locked-spec change and the oracle re-key surface
  (Sec 3.5, Sec 4).
- The **default-selector change surface** in `spec/flair_skins.h` (Sec 3.6).
- The **validation regime**: captured interaction video + tri-emulator (Sec 3.7).

### 2.2 Out of scope (unchanged by this Amendment)

- **The mechanism/policy cut-line C-8** and the `flair_look_pixel(port, PART)` resolver
  (DEC-09 D1/D2, ARB-3). The mechanism still names no color; the era flip is a **policy
  data** change, and any Platinum work that puts an `0xRRGGBB` into a mechanism module is
  a C-8 violation and a Stop condition.
- **The data-only `flair_skin_t` rule** (DEC-09 D-9): no function pointers, no draw code,
  no `ElementDrawTable` vtable, no `switch (era)` in the mechanism. A Platinum row is a
  DATA row. Any proposal to add an era branch to the imaging path is a Stop condition.
- **The one canon module** (`spec/assets/color_canon.json` -> `color_canon.h`,
  `flair_canon_rgb`). DEC-10 does NOT author a second color table (ARB-1/ARB-2 stand).
- **D-9b, the Win-3.1/GDI peer skin and the chimera** (Sec 3.3.2 below restates that it
  is invariant).
- **ADR-0010's grading architecture** -- independent goldens, loud-skip, provenance tags,
  the depth-trap guardrail, BC-1/BC-2 (no by-construction color grade). DEC-10 *consumes*
  ADR-0010 unchanged and simply changes WHICH independent corpus the Mac-chrome legs
  grade against.
- **The region engine**, the DOS/MILTON personality, SAMIR, and Turbo Initech: untouched.

---

## 3. The Decision

Decisions are numbered **D-10.1 .. D-10.8**, continuing the ADR-0004 decision namespace
after D-9/D-9b.

### 3.1 D-10.1 -- `ERA_SYS8_PLATINUM` is the BASE era

**`ERA_SYS8_PLATINUM` becomes the BASE (default) era for FLAIR window chrome, control
chrome, menu, and dialog decoration.** The default `(era, heritage)` selector resolves to
`(ERA_SYS8_PLATINUM, HERITAGE_QUICKDRAW)`. The frame's Mac chrome -- title bar, close /
zoom / collapse boxes, grow box, scrollbars, menu bar, buttons, dialog frames -- is
rendered from the Platinum row.

The **heritage axis does not change**: Mac OS 8's Appearance-era chrome still draws over
QuickDraw, so Platinum is a new **era** over `HERITAGE_QUICKDRAW`, **not** a new
`heritage_id`. No `heritage_id` value is added, renamed, or renumbered by this Amendment.

> Ref: operator ruling 2026-08-15 (beads `initech-t1i0`); Law 4 ("the judge is a person
> who used early-90s Mac+DOS software saying 'yes, that's it'"); PRD Sec 1, Sec 3.

### 3.2 D-10.2 -- `ERA_SYS7_0_1` is RETAINED as a locked heritage row

**System 7 is demoted, never deleted.** `ERA_SYS7_0_1 / HERITAGE_QUICKDRAW` stays in the
registry as a **locked heritage row**, byte-identical to its ratified DEC-09 form, and
stays graded by its existing System-7 goldens.

Three reasons, all binding:

1. **Corporate software accretes** (D-9 P5, WL-0049). InitechOS's in-universe premise is
   a product that never deletes a code path. A shipped era that is quietly removed when
   fashion changes is exactly the 2026-ism the artifact must not contain.
2. **Deleting the SYS7 row destroys the accretion proof.** `test-skin-era-frozen`'s
   digest is computed over the SYS7 + WIN31 rows. Keeping them byte-identical while
   Platinum is appended is the *mechanical evidence* that the flip was an APPEND, not a
   mutation (Sec 3.5.2).
3. **The System-7 corpus stays a live golden.** `../system7-decomp` remains the graded
   source for the SYS7 row, so the existing `test-color-canon` LEG A (wctb) and LEG C
   (pinstripe.md) legs keep biting instead of being orphaned.

A later product decision MAY expose the era as a user-selectable "Appearance" (period-
authentic: Mac OS 8's Appearance control panel did exactly this). DEC-10 neither
requires nor forbids it; it only fixes the DEFAULT.

### 3.3 D-10.3 -- What is INVARIANT under the flip (the Initech identity)

The era flip changes the **Mac chrome dialect**. It changes **nothing** about what makes
the artifact InitechOS. The following are explicitly INVARIANT and any Platinum work that
disturbs them is a Stop condition:

#### 3.3.1 The Initech color identity (WL-0053, DEFINITIVE)

- **Desktop teal `#8DDCDC`** (canon idx2, `CIDX_DESKTOP`) -- the signature Initech teal,
  VIC-20 cyan hardware colour 3, screen/phosphor value. **Survives verbatim.** Mac OS 8's
  own default desktop is irrelevant: the teal is an Initech-identity injection, not a
  period value, and WL-0053 is the definitive ruling ("the teal is what endows InitechOS
  with its character"). The pigment value `#57B19F` remains PRINTED-PROPS-ONLY and never
  renders.
- **The teal bevel pair** `bevel_light #8DDCDC` / `bevel_shadow #4E9BA3` -- the WL-0053
  lavender->teal substitution. **Survives**, subject to Open Question OQ-2 (Sec 6): the
  Platinum bevel ramp is a *different structure* (a multi-step Appearance ramp, not a
  Sys7 two-tone groove), so the teal substitution RULE ("replace the era's bevel tinge
  with same-hue teal light/shadow") is what carries forward, and its Platinum instantiation
  is TODO_GOLDEN.
- **The navy accent `#000080`** (canon idx5, `CIDX_ACCENT`, `win31-gdi` heritage) --
  drives the 116% pie and the progress/selection accents. **Survives verbatim.** It is a
  Win-3.1 datum, graded by LEG B, entirely orthogonal to the Mac era axis.
- **`CIDX_CONTROL #C0C0C0`, `CIDX_WHITE`, `CIDX_BLACK`** -- unchanged.
- **`graded_by: authored` honesty (P4 / DEC-09 FO-9).** The teal rows are still never
  claimed decomp-sourced, and the seafoam-relapse and lavender-relapse VALUE mutants stay
  armed. A Platinum mint does NOT create a golden for the teal.

> Ref: `docs/worklog/WL-0053-initech-color-canon-palette-source-decision.md`
> ("The Initech Color canon (operator, DEFINITIVE)"); `docs/HANDOFF.md` "THE INITECH
> COLOR (DEFINITIVE)"; `spec/assets/color_canon.json` idx2 + the two derived bevel rows;
> DEC-09 Sec 3.8 (OD-4-REVOKED).

#### 3.3.2 The chimera

- **The Win-3.1 / GDI peer skin (`ERA_WIN31, HERITAGE_GDI`) is INVARIANT**, byte-identical.
  D-9b stands unchanged: the Photoshop menu bar and the Windows chrome bits are a PEER
  skin over the same mechanism, clipped through the GDI `CombineRgn` wrapper, load-bearing
  on metal. The Mac era flip does not touch the GDI lane.
- **`check-win95isms` stays armed.** Platinum is 1997 Mac, NOT Windows 95. The era ceiling
  (`spec/win95ism_guardrails.md`) is unchanged, and the Platinum gray ramp must be
  re-audited against the forbidden-token list rather than assumed compatible with it
  (Sec 4, `initech-3knt`).

#### 3.3.3 The deliberate frame inconsistencies and the canonical bugs

Unchanged and still enforced (CLAUDE.md "Hallucination-risk callouts"):

- The **Photoshop menu bar** (`File Edit Image Layer Select View Window Help`) on a Mac
  window over DOS. It is the spec, not a defect. InitechOS out-reals the prop by genuinely
  having those apps with those menus.
- The **pie chart summing to 116%** (`40+35+18+14+9`).
- The **`570-` trailing minus**.
- The **hourglass cursor** (the wristwatch is the bug; the hourglass is correct).
- **DOS drive letters** under Mac chrome.

Nothing in the Platinum mint may be used to "correct" any of these.

#### 3.3.4 The architecture

C-8, the data-only skin record, the ONE canon module, the ONE region engine, ADR-0010's
independent-golden discipline, and the loud-skip contract: all invariant (Sec 2.2).

### 3.4 D-10.4 -- Platinum values enter ONLY from an independently minted golden

**Binding, Law 1 + Law 2.** Every Platinum value -- every RGB, every metric, every
geometry, every pattern byte -- enters the repo **only** from the independently minted
Mac OS 8.1 goldens:

- **Mint path (verified 2026-08-15, `initech-t1i0`):** the local ROM library has a
  **Quadra 650 (68040)** ROM, which runs **Mac OS 8.0/8.1** (8.1 is the last 68k release)
  under the **existing `../system7-decomp` Basilisk II mint harness** (BasiliskII / xvfb /
  xdotool / ffmpeg already installed; `basilisk_prefs_quadra_701` pattern -- modelid 14,
  cpu 4, 640x480, `displaycolordepth 8`). Only the Mac OS 8.1 install image is missing
  (`initech-kd60`).
- **Chain:** `initech-kd60` (boot 8.1) -> `initech-slwi` (mint `s8_*` captures + extract
  Appearance-era resources) -> `initech-4lmg` (pixel-measure into `sys8`-tagged specs
  under `../system7-decomp/specs/`, each value citing its capture + coordinates) ->
  `initech-3knt` (populate rows + re-key oracles).
- **ONE-SHOT mint discipline (Rule 11):** artifacts are minted once and committed; no gate
  re-mints.

**Three prohibitions, each a Stop condition:**

1. **Never sampled from the movie still.** `preview.webp` / the frame still is
   `PROVENANCE_ONLY` (ADR-0010 CD-8.2, HER-03) and remains so. Platinum values do not
   come from a compressed low-res Photoshop mock-up of a CRT photograph. This is the exact
   error WL-0053 overturned; re-committing it on the era axis would be HER-03 reborn.
2. **Never guessed, never recalled.** "Platinum's title bar is `#DDDDDD`" from an agent's
   memory is a fabricated golden. It is worse than red, because it is green and wrong.
3. **Never graded by construction.** The oracle's expected Platinum value must come from
   the minted `sys8` specs, never from `flair_skins.h` / `color_canon.h` / `chrome.c` --
   the source the artifact renders from (Law 2; ADR-0010 BC-2; HER-02).

**`TODO_GOLDEN` marking.** Until `initech-4lmg` lands, **every** Platinum datum in every
spec, header, or oracle is written as a `TODO_GOLDEN` placeholder that **fails loud or
loud-skips** -- never a plausible-looking number. This document contains **zero Platinum
RGB values** by design. The `spec/ssim_params.h` `TODO_GOLDEN` crops are the in-repo
precedent for this marking (CLAUDE.md Law 4 note).

### 3.5 D-10.5 -- Rule-8 mechanics of the locked-spec change

This is a **deliberate locked-spec change** under Rule 8: one beads issue (`initech-s97h`
for the decision, `initech-3knt` for the execution), one worklog note, an in-file note at
each changed constant, and **no** silent edit to make a test pass.

#### 3.5.1 What changes in `spec/flair_skins.h`

| Change | Kind | Digest impact |
|---|---|---|
| APPEND one `ERA_SYS8_PLATINUM / HERITAGE_QUICKDRAW` row, all slots by-inclusion from `color_canon.h` (or the canon gray-ramp formula `FLAIR_CANON_GRAY_RGB`) | APPEND | none (Sec 3.5.2) |
| ADD `FLAIR_DEFAULT_ERA` / `FLAIR_DEFAULT_HERITAGE` macros + `flair_skin_default()` (Sec 3.6) | ADD | none |
| REPLACE the `_Static_assert` "default selector `(0,0)`" with an assert on the new default macros | REPLACE (non-row) | none |
| UPDATE the header banner: Platinum is BASE, SYS7 is heritage, cite DEC-10 | comment | none |
| `ERA_SYS7_0_1` and `ERA_WIN31` row FIELDS | **UNCHANGED, byte-identical** | none -- this is the point |
| `era_id` / `heritage_id` enum INTEGER VALUES | **UNCHANGED** (Sec 3.6) | none |

#### 3.5.2 The accretion digest: a correction to the working assumption

The `initech-s97h` description anticipated that `SKIN_FROZEN_DIGEST 0xDEF099AC` would be
"re-baselined ONCE with an in-file note." **Reading the oracle shows that is not required,
and that re-baselining it would be actively harmful.** `harness/proptest/test_skin_era_frozen.c`:

- computes the digest over **exactly two rows**, obtained **by key** via
  `flair_skin_resolve(ERA_SYS7_0_1, HERITAGE_QUICKDRAW)` and
  `flair_skin_resolve(ERA_WIN31, HERITAGE_GDI)` -- explicitly *"independent of physical
  row order"* (`:142-146`);
- so **appending a Platinum row does not change the digest**;
- but it separately asserts `FLAIR_SKIN_REGISTRY_COUNT != 2` -> FAIL (`:123`) and *"no row
  may carry the RESERVED `ERA_SYS8_PLATINUM`"* -> FAIL (`:129-139`). **Those two legs are
  what go RED**, and they are placeholder guards for the reserved-and-empty state, not
  accretion proof.

**Ruling (D-10.5a).** `SKIN_FROZEN_DIGEST` **stays `0xDEF099AC`**. It is the standing,
unbroken proof that the SYS7 and WIN31 base rows were not touched by the Platinum flip --
precisely the invariant D-9 P5 exists to protect. Re-baselining it would erase that proof
and make a future genuine base-row mutation indistinguishable from a sanctioned
re-baseline. The digest constant is re-pinned **only if and when a base-row field must
change**, which this Amendment does not require and does not authorise.

What IS re-keyed in `test_skin_era_frozen.c`, with an in-file note citing DEC-10 and
`initech-3knt`:

- `FLAIR_SKIN_REGISTRY_COUNT != 2` becomes `!= 3` (or an explicit `>= 3` plus a named
  per-era row-count table). **Strengthening required:** the replacement must still assert
  an exact expected count, so a stray fourth row is caught.
- The "`ERA_SYS8_PLATINUM` must have ZERO rows" leg **inverts** into "`ERA_SYS8_PLATINUM`
  has exactly ONE row and it is the default-selector row" -- a strictly stronger assertion
  than the placeholder it replaces (not a weakening; not a Stop condition).
- ADD a leg asserting `flair_skin_resolve(FLAIR_DEFAULT_ERA, FLAIR_DEFAULT_HERITAGE)`
  returns the Platinum row -- the mechanical statement of D-10.1.
- The existing `SKIN_FROZEN_MUTANT` (one-bit perturbation of `base.desktop.rgb`) is
  UNCHANGED and MUST still go RED; a NEW mutant appends a *mutated* SYS7 row-field to
  prove the digest still bites after the re-key (Rule 6).

#### 3.5.3 What changes in the canon module

`spec/assets/color_canon.json` carries a top-level `era: system7.0-7.1` tag. Under D-10.1
the *base* era is Platinum, so the tag is stale. Two dispositions are possible and the
choice is **Open Question OQ-1** (Sec 6); the DRAFT recommendation is that the canon
module remains an **era-neutral index table** (its rows are already mixed-heritage:
`system7-quickdraw`, `win31-gdi`, `initech-identity`), that the top-level tag becomes
`multi` with a per-row `era` field, and that **Platinum neutral grays need NO new canon
index** because `flair_canon_rgb(idx >= 9)` already returns the exact neutral gray
`(v<<16)|(v<<8)|v`.

That route is already load-bearing in three places, so it is a proven pattern rather than
a new mechanism: the WIN31 `btnshadow #808080` slot (`FLAIR_CANON_GRAY_RGB(0x80)`), and
`CIDX_HILITE_FRAME` (gray-ramp idx **119** -> `#777777`) / `CIDX_HILITE_TEXT` (idx **165**
-> `#A5A5A5`), which `test-color-canon` **LEG E** already grades against `title-bar.md`.
Platinum's Appearance chrome is predominantly a neutral gray ramp, so most of it should
land on existing gray-ramp indices with **no canon row appended at all**. Only a
**non-neutral** Platinum value (if the mint finds one) would require an appended canon
row. **This is provisional and TODO_GOLDEN**: it is a prediction about Platinum's ramp,
not a measurement, and `initech-4lmg` decides it (OQ-3).

`CIDX_PIN_LIGHT #F3F3F3` / `CIDX_PIN_DARK #969696` (the System-7 rendered pinstripe
dither) are **retained** as SYS7 heritage-row canon, still graded by LEG C against
`pinstripe.md`. The Platinum title bar's structure is a different one; the SYS7 pinstripe
rows are not deleted, re-valued, or re-pointed at a Platinum golden.

### 3.6 D-10.6 -- The default-selector change surface (ANALYSIS + RECOMMENDATION)

Today the default is **positional**: `flair_skin_resolve(0, 0)`, load-bearing in a
`_Static_assert` (`spec/flair_skins.h:212-213`) and in the banner (`:48`, `:84`, `:234`).
Three options were considered.

**Option A -- renumber the enum so `ERA_SYS8_PLATINUM = 0`. REJECTED.**
It would keep `(0,0)` meaning "the default" but at unacceptable cost:
1. It **mutates the locked base rows.** Each row's `era` field stores the enum value, and
   `feed_row()` digests `r->era` first. Renumbering changes the SYS7 row's `era` from 0
   and the WIN31 row's from 1, so `SKIN_FROZEN_DIGEST` changes -- **for the wrong reason**,
   and it violates D-9 P5 "accretion = APPEND, NEVER MUTATE" on its face.
2. It **breaks the digest's evidentiary value** exactly as described in Sec 3.5.2.
3. It is a **Rule-11 reproducibility hazard**: the header states "Stable integer values
   (Rule 11)". Era ids are a small ordinal namespace that appears in digests today and can
   appear in serialized state, capture filenames, or a future Appearance preference
   tomorrow. Silently re-tagging what `era == 0` means is the class of change that makes
   an old artifact and a new one disagree without either being visibly wrong.
4. It makes the diff **maximally misleading**: a one-line enum edit silently re-tags every
   existing row.

**Option B -- keep the enum stable; add an EXPLICIT default. RECOMMENDED.**
```
#define FLAIR_DEFAULT_ERA       ERA_SYS8_PLATINUM
#define FLAIR_DEFAULT_HERITAGE  HERITAGE_QUICKDRAW
static inline const flair_skin_t *flair_skin_default(void)
{ return flair_skin_resolve(FLAIR_DEFAULT_ERA, FLAIR_DEFAULT_HERITAGE); }
```
with the `(0,0)` `_Static_assert` replaced by asserts that (i) `FLAIR_DEFAULT_ERA ==
ERA_SYS8_PLATINUM`, (ii) `FLAIR_DEFAULT_HERITAGE == HERITAGE_QUICKDRAW`, and (iii) the
enum values `ERA_SYS7_0_1 == 0`, `ERA_WIN31 == 1`, `ERA_SYS8_PLATINUM == 2` are UNCHANGED
(a Rule-11 stability assert, newly explicit).

Why it wins:
- **Zero base-row mutation**; the digest survives verbatim as accretion proof.
- **The default becomes a named policy datum** instead of an implicit consequence of
  declaration order. "Which era is the default" becomes greppable, assertable, and
  amendable by one macro -- which is what a future Appearance selector will want anyway.
- **It removes a latent trap.** `(0, 0)` as "the default" is the same category of
  implicit-positional contract that DEC-09 spent its length eliminating from the color
  layer; enshrining a *second* era as the magic zero would repeat it.
- **Fail-loud is preserved.** `flair_skin_resolve` stays TOTAL and fail-loud; the new
  wrapper adds no branch and no fallback. An era with no row still panics.

Cost: two call sites in the harness (`test_skin_teal.c`, `test_skin_era_frozen.c`) plus
any future consumer must ask for the era they mean. That is the intended cost.

**Option C -- reorder the registry array so the Platinum row is physically row 0.
REJECTED.** The resolver is a key search, so physical order is already semantically inert
(`test_skin_era_frozen.c:142-143` resolves by key precisely so the digest is order-
independent). Reordering therefore buys nothing, still leaves the misleading `(0,0)`
`_Static_assert` asserting `ERA_SYS7_0_1 == 0` as "the default", and encodes policy in
array position -- the least greppable place available.

**RECOMMENDATION: Option B.** Recorded as **D-10.6**; execution in `initech-3knt`.

#### 3.6.1 The seam BEYOND the selector (the honest scope of the flip)

**Verified 2026-08-15: flipping the default selector alone changes no rendered pixel.**
The registry is not wired to the artifact:

- `flair_skin_resolve` has **zero callers in `os/`** -- only
  `harness/proptest/test_skin_teal.c:87-88` and `test_skin_era_frozen.c:145-146`.
- The live color seam is `os/flair/flair_look.c` -- `flair_look_pixel(port, PART)` over a
  **static const PART->canon-index map with no era parameter**.
- The live geometry seam is `spec/chrome_metrics.h`'s `#define`s, compiled directly into
  `os/flair/chrome.c`.

So there are two possible execution routes, and the Amendment must say which:

- **Route 1 -- thread an era selector through `flair_look` + `chrome.c`.** Makes the
  registry load-bearing on metal (the spirit of D-9b, which made the GDI facade
  load-bearing rather than oracle-only). Cost: a new parameter on the hottest policy call,
  and a standing temptation to `switch (era)` in the mechanism -- which C-8 and D-9
  forbid, so the era would have to arrive as *data* (a `const flair_skin_t *` on the
  `GrafPort`/policy side), not as a branch.
- **Route 2 -- re-key the base constants.** `chrome_metrics.{h,json}` and
  `chrome_fidelity_golden.h` are re-keyed so the BASE constants describe Platinum, with
  the System-7 values retained beside them under their existing `era=` tags (exactly what
  the 2026-06-20 tagging rule was built for). `flair_skins.h`'s `FLAIR_DEFAULT_ERA` is the
  declared policy statement; the registry stays a graded VIEW.

**DRAFT recommendation: Route 2 now, Route 1 as a funded follow-up.** Route 2 is the
minimum honest change that makes the artifact look like the frame, it keeps C-8 untouched,
and it is what `initech-3knt` + `initech-i3si` are already scoped to. Route 1 is the right
end-state (an unwired registry is decoration, and a second era that renders nothing is a
Law-2 smell) but it is a mechanism change that deserves its own issue and its own
mutation-proof, not a rider on a look change. **Recorded as OQ-7**; if the operator
prefers Route 1, `initech-3knt` grows a wiring dependency.

**Either way this Amendment states plainly:** a green `test-skin-*` after the flip proves
the *registry* is correct, **not** that the desktop is Platinum. The desktop is Platinum
when `test-chrome-fidelity` is green against the `sys8` golden and the video shows it
(D-10.7).

### 3.7 D-10.7 -- Validation: captured interaction VIDEO + tri-emulator

**Per operator directive (`initech-t1i0`): every GUI-visible change in this pivot is
validated with actual captured interaction VIDEOS, in addition to -- never instead of --
the structural oracles.**

- **Video is EVIDENCE, not an oracle.** It is the Law-4 eyeball channel, deterministic and
  regenerable, and it is the only channel that shows *temporal* artifacts (the
  frozen-then-teleport drag of `initech-34gp`, the modal erased mid-drag of `initech-pipa`)
  which single screendumps are structurally blind to. A clip may never be cited as a
  passing gate, and no numeric threshold is ever computed from one (that would be SSIM by
  the back door -- Law 4 / ADR-0010 BC-7).
- **Harness:** `initech-l9cd` (`make record-flair SCRIPT=...`): fixed `-icount shift=N`
  single-CPU TCG, screendumps clocked to the guest tick, ffmpeg bitexact GIF + MP4, no
  timestamps or host paths (Rule 11). **`initech-l9cd` is OPEN and unbuilt**; it is a hard
  prerequisite of the video-validated obligations (FO-10.6) and of `initech-i3si`.
- **What must be filmed** for the Platinum chrome (`initech-i3si`): window drag with
  correct clipping, active <-> inactive title transition, close/zoom/collapse box press
  and release, scrollbar thumb drag and arrow repeat, menu pull-down and dismiss,
  modal-dialog open over a live window.
- **Tri-emulator (Rule 5).** The re-keyed structural gates run QEMU (dev loop) + Bochs
  (real->protected accuracy) by default in `make test` (`SKIP_BOCHS=1` is the one shouting
  opt-out); 86Box remains the PLANNED period-authenticity leg (`initech-x0i`, no
  `box86.c` yet). A Platinum fix that passes QEMU and fails Bochs is a Stop condition, not
  a QEMU pin.

### 3.8 D-10.8 -- Sequencing (oracle-first, ADR-0010 BC-6 pattern)

The flip lands **oracle-first**, mirroring DEC-09 ARB-6 / ADR-0010 CD-6:

1. Ratify this Amendment (operator).
2. Mint the goldens: `kd60` -> `slwi` -> `4lmg`. **No `spec/` or `os/` value edit lands
   before `4lmg`.**
3. Land the Platinum-graded value legs GREEN against the `sys8` specs, with loud-skip when
   the corpus is absent, **before** any render flip (`3knt`).
4. Flip `flair_skins.h` (append row + explicit default) and re-key the affected gates,
   each **re-mutation-proven** (Sec 4).
5. Land the `chrome.c` Platinum rendering (`i3si`), video-validated, tri-emulator.

Each commit stays green and bisectable. The "no visible change" claim is **struck in
advance**: this IS a deliberate look change, and saying so is the DEC-09 ARB-6 discipline.

---

## 4. Oracle re-key surface (Rule 6: every re-keyed gate is re-mutation-proven)

A gate re-keyed to a new golden **has not been proven to bite**. Every row below marked
RE-KEY must have its mutant re-run and confirmed RED **for the right reason** after the
re-key; a mutant that still passes means the re-key produced decoration (Rule 6, Law 2).

| Gate / artifact | Disposition under DEC-10 | Mutant / re-proof obligation |
|---|---|---|
| `spec/flair_skins.h` | **APPEND** Platinum row; ADD `FLAIR_DEFAULT_ERA`/`_HERITAGE` + `flair_skin_default()`; base rows byte-identical | (locked data -- proven by the gates below) |
| `test-skin-era-frozen` | **RE-KEY** the row-count + "zero Platinum rows" legs (Sec 3.5.2). `SKIN_FROZEN_DIGEST` **stays `0xDEF099AC`** | `SKIN_FROZEN_MUTANT` MUST still go RED; ADD a mutant that perturbs a SYS7 base-row field post-re-key |
| `test-skin-era-frozen-mutant` | **RE-RUN** | must remain RED-on-mutation |
| `test-skin-teal` | **EXTEND**: assert the Platinum row's teal/identity slots equal the canon by inclusion, exactly as SYS7/WIN31 are asserted today | `test-skin-teal-mutant` re-run; ADD a Platinum-slot canon-drift mutant |
| `test-chrome-fidelity` | **RE-KEY** to the Platinum golden. This is the heaviest row: its legs grade pinstripe phase, title text, shadow, box geometry, scrollbars, bevel + 15-row runs, inactive title, inactive frame/text, and gadget retention -- all measured from System-7 | ALL ten named mutants re-proven RED: `CHROME_FID_MUT_PHASE`, `_TTL`, `_SHA`, `_BOX`, `_SBF`, `_BVL`, `_INA`, `_IBF`, `_IBT`, `_IKG`. Any mutant that no longer bites means the Platinum leg it guarded was silently dropped |
| `spec/chrome_fidelity_golden.h` | **RE-KEY** -- the single largest surface: ~30 `FG_*` constants in 8 sections, **every one System-7-specific** (`FG_TITLE_INTERIOR_PATTERN "LLDLDLDLDLDLDLL"`, `FG_TITLE_INTERIOR_ROWS 15`, `FG_PHASE_DOUBLED_LIGHT_AT_EDGES`, the bevel-hi/lo indices, `FG_BOX_*` geometry, `FG_SB_*` scrollbar topology, `FG_INACTIVE_*` dimming). Re-keyed to `sys8`-tagged values from `initech-4lmg`; SYS7 values retained under their era tag, never deleted | it IS the golden -- the ten mutants above are its proof. Its own banner rule holds: values are pixel-measured from `../system7-decomp` (now `s8_*`) captures, **never** derived from `chrome_metrics.h`, which `chrome.c` renders from (Law 2) |
| `spec/chrome_metrics.{h,json}` | **RE-MEASURE** from the Platinum captures (title-bar height, bevel width, pinstripe period, box geometry, scrollbar metrics). The JSON carries ~66 `era: system7.0-7.1` / `era_stability` tagged entries mirrored by the header `#define`s; SYS7 entries retained under their tag per the 2026-06-20 tagging rule | `test-chrome` STEP-1 header-vs-JSON consistency tooth re-run; `test-chrome-mutant` (`TEST_CHROME_MUT_TITLE` / `_FRAME` / `_SBW`) re-proven RED; TODO_GOLDEN until `4lmg` |
| `tools/ppm_flair_check.c` -- 4 named legs | **(a) TEAL DESKTOP: UNCHANGED** (`#8DDCDC`, Sec 3.3.1). **(b) TWO MENU BARS: RE-CHECK** -- the 20px `GetMBarHeight` band and the Apple-glyph ink tell are era-keyed; Mac OS 8's bar height and Apple menu are *believed* unchanged, which is not Law 1 -- confirm from the mint. **(c) WINDOW CHROME: RE-DERIVE** -- the period-2 idx7/idx8 pinstripe alternation is pure System 7 and does not survive Platinum; it must be **replaced by the Platinum structural relation the mint measures, never deleted**. **(d) MODAL FILE COPY: RE-KEY** the bevel/pinstripe/frame shade roles; keep the navy progress fill and Z-order occlusion verbatim. All other value-independent structural probes KEPT VERBATIM per ADR-0010 CD-5.4 | the COLOR VALUE-mutant arm (CD-5.6) re-run; SCENE mutants (`onebar`/`nomodal`/`modalbehind`) unchanged and re-run |
| Sibling graders: `ppm_flair_drag_check`, `ppm_flair_menu_check`, `ppm_flair_menu_crossdrag_check`, `ppm_flair_dc4v_check`, `ppm_flair_appswitch_check`, `ppm_flair_solid_check` | **RE-KEY** every embedded System-7 chrome assumption. `ppm_flair_solid_check` **leg C** explicitly asserts "stripes ACTIVE / flat inactive white" -- an era-specific active/inactive tell that must be re-derived from the Platinum active/inactive captures, not dropped | each gate's mutants re-run (`no_route_on_chrome`, `no_activate_inval`, the appswitch mutants) |
| `spec/flair_appswitch_trace.mk` | **RE-BASELINE** the locked trace geometry if any chrome metric moves | `test-flair-appswitch-mutant` re-run |
| `test-color-canon` (LEGs **A** wctb / **B** win31 / **C** pinstripe.md / **D** authored-teal / **E** title-bar inactive-hilite `CIDX_HILITE_FRAME` idx119 + `CIDX_HILITE_TEXT` idx165) | **UNCHANGED as they stand** -- A/C/E grade the SYS7 heritage row, B the WIN31 peer row, D the authored teal. ADD a **LEG F** grading the Platinum canon rows against the `sys8` specs, loud-skipping when absent. **LEG E is the one to watch**: Platinum's inactive-title treatment differs from System 7's, so its Platinum counterpart belongs in LEG F, and LEG E stays pinned to the SYS7 row rather than being re-pointed | all five named VALUE mutants (`CANON_MUTATE_TEAL` / `_NAVY` / `_WHITE` / `_PIN` / `_HILITE`) re-run RED; ADD a Platinum-value mutant for LEG F; `CANON_SKIP` must still print LOUD-SKIP |
| `gen-color-canon` / `test-color-canon-gen` | **RE-RUN** -- generated `color_canon.h` must still equal the locked JSON after any canon edit | regeneration is the proof; no hand-edit of the generated header |
| `check-win95isms` + `spec/win95ism_guardrails.md` | **RE-AUDIT**, not relaxed. Platinum's 3-D gray ramp is visually nearer Win95 than Sys7's flat chrome, so the forbidden-token list must be re-examined for false positives **and** for newly-needed entries. The doc's line 12 ("a System 7.0/7.1 BASE -- never Win 95") is re-worded to name Platinum as the base while keeping "never Win 95" verbatim. **Relaxing a token to make Platinum pass is a Stop condition.** Note the era ceiling does NOT move: Platinum is 1997 Mac and is here because the *frame* shows it, not because the era window widened | `check-win95isms-mutant` re-run (a planted `#DFDFDF` must still be caught) |
| `test-clut` / `spec/assets/clut.json` | **RE-CHECK, expected UNCHANGED.** The device CLUT is the standard Mac 256-color table read from the System-7 ROM (`clut_8_rom.bin`), and is believed identical under Mac OS 8 -- believed, not verified (OQ-8). If the mint confirms, only the `era` tag annotation changes | `test-clut` re-run; loud-skip discipline unchanged |
| `spec/chimera_element_map.json` | **RE-TAG**: the `origin` enum value `mac-system7` becomes the Platinum-era spelling for base elements; `win31-accent` and `chimera-intentional` are UNCHANGED (Sec 3.3.2/3.3.3) | consumers re-run |
| `spec/control_record.h`, `menu_record.h`, `dialog_record.h`, `window_record.h`, `drawing_ops.h` era tags | **RE-TAG the base era only.** These are Toolbox *record layouts* from Inside Macintosh, which Platinum does not change; what changes is the `era=` annotation and the `control_record.h:18` ERA-AXIS block. **`control_record.h:45`'s "System 7.5+/Appearance Manager renames are NOT in the target" delta must be revisited** -- Platinum IS the Appearance Manager era, so that recorded delta may become in-target | each record's `_Static_assert` size/offset gates re-run UNCHANGED (a re-tag that moves a struct offset is a bug, not a re-tag) |
| `test-mech-policy`, `test-flair-mechanism-colorblind` | **UNCHANGED** -- C-8 is invariant. Any Platinum literal appearing in a mechanism module MUST drive these RED | mutants re-run; these are the guard that the era flip stayed in policy |
| `test-flair-desktop` / `-mutant` / `-bochs`, `test-flair-shell` / `-mutant`, `test-flair-appswitch` / `-mutant` / `-bochs`, `test-flair-solid` / `-mutant` | **RE-BASELINE** the emu screendump expectations (the desktop's Mac chrome changes). `test-flair-desktop` keeps its hard `test-color-canon-gen` + `test-color-canon` precondition chain (ADR-0010 CD-5.5) -- that chain may not be severed to land Platinum | each gate's existing mutants re-run (`onebar` / `nomodal` / `modalbehind`; `TEST_SHELL_MUT_*`; the appswitch and solidity mutants); tri-emulator agreement required (Rule 5) |
| `test-flair-drag`, `test-flair-menu`, `test-flair-menu-crossdrag`, `test-flair-live`, `test-flair-key`, `test-interact` | **RE-CHECK**: behavioural and mostly era-neutral, but each screendumps and grades chrome geometry/pinstripe through a `ppm_flair_*_check`, and any hit-test tied to Sys7 box geometry moves with the Platinum metrics | re-run; ADD a Platinum box hit-test mutant if the geometry moved |
| `test-canon` / `-mutant`, `test-palette-seafoam` / `-mutant` | **RE-CHECK.** `test-canon`'s `_WATCH` mutant guards the hourglass-not-wristwatch canon (INVARIANT, Sec 3.3.3) and `_MENU` the menu canon; the palette gate carries the standing seafoam-relapse tripwire | both mutants re-run RED |
| `make ssim` | **UNCHANGED** -- still an honest deferred stub, still a GUIDE never a gate (ADR-0010 BC-7). If ever built, its crops re-anchor to the `s8_*` rendered goldens | n/a (not a gate) |
| `make record-flair` (`initech-l9cd`) | **NEW PREREQUISITE** for the video-validated obligations | byte-identical output for the same SCRIPT (Rule 11) |

**Aggregate obligation:** `make clean && make test` GREEN, host + emu counts recorded, with
the golden corpora present; and loud-skipping (never silent-passing) when they are absent
(ADR-0010 BC-5).

---

## 5. Consequences

### 5.1 Binding constraints (PROPOSED -- bind on ratification)

- **BC-10.1.** The default FLAIR chrome era is `ERA_SYS8_PLATINUM`; it changes only by a
  further ADR amendment.
- **BC-10.2.** `ERA_SYS7_0_1` and `ERA_WIN31` registry rows are **byte-identical**
  forever under this Amendment. Accretion is APPEND, never MUTATE, never DELETE (D-9 P5
  re-affirmed).
- **BC-10.3.** `SKIN_FROZEN_DIGEST` remains `0xDEF099AC` through the Platinum flip. Any
  future re-baseline requires its own ADR act plus an in-file note naming the base-row
  field that changed and why.
- **BC-10.4.** No Platinum value is written into `spec/` or `os/` except from the minted
  `sys8` goldens (D-10.4). Values from the movie still, from memory, or from an oracle
  that reads the render source are Stop conditions.
- **BC-10.5.** Era ids are stable integers. `ERA_SYS7_0_1 == 0`, `ERA_WIN31 == 1`,
  `ERA_SYS8_PLATINUM == 2` (Rule 11), enforced by `_Static_assert`.
- **BC-10.6.** The Initech identity invariants of Sec 3.3 survive the flip: teal `#8DDCDC`
  desktop, the same-hue teal bevel rule, navy `#000080`, the WIN31 peer skin, the
  Photoshop-bar chimera, and the canonical bugs (116% pie, `570-`, hourglass).
- **BC-10.7.** No era branch in the mechanism. `flair_skin_t` stays data-only; C-8 stands
  (DEC-09 D-9 / D1).
- **BC-10.8.** Every re-keyed gate is re-mutation-proven RED for the right reason before
  its re-key is considered landed (Rule 6).
- **BC-10.9.** GUI-visible Platinum changes carry a captured interaction video as evidence
  (D-10.7). Video is never a gate and never yields a numeric threshold.
- **BC-10.10.** The **era-tagging rule survives verbatim** (2026-06-20 operator ruling,
  `spec/CANON-MANIFEST.md`): every spec header and `os/flair` source carrying chrome
  metrics, color tables, WDEF geometry, or title-bar rendering logic keeps an `era=` tag.
  Only the identity of the BASE era changes. A Platinum value that lands untagged is a
  Rule-8 violation.

### 5.2 Forward obligations

- **FO-10.1** (`initech-kd60`, IN PROGRESS). Acquire a Mac OS 8.1 install image; install
  to a fresh HFS hardfile under Basilisk II / Quadra 650; boot headless to the Platinum
  Finder at 256 colors; images stay gitignored under `goldens/`.
- **FO-10.2** (`initech-slwi`). Mint the `s8_*` capture set (desktop pattern, ACTIVE +
  INACTIVE document-window chrome, close/zoom/collapse/grow boxes, scrollbars, menu bar +
  dropped menu, buttons/radio/checkbox, movable modal dialog); extract Appearance-era
  resources. ONE-SHOT, committed, never re-minted in a gate.
- **FO-10.3** (`initech-4lmg`). Pixel-measure into `sys8`-tagged specs under
  `../system7-decomp/specs/`, each value citing its capture and coordinates. **This, not
  FLAIR's own tables, is what the oracles grade against (HER-02).**
- **FO-10.4** (`initech-3knt`). Execute Sec 3.5 + Sec 3.6 (Option B) + the Sec 4 re-key,
  every gate re-mutation-proven; re-audit `check-win95isms`.
- **FO-10.5** (`initech-i3si`). `chrome.c` Platinum rendering (beveled title bar, Sys8
  boxes and scrollbars) through the existing PART resolver -- no color literal, no era
  branch (C-8).
- **FO-10.6** (`initech-l9cd`). Build the deterministic video harness; it gates FO-10.5's
  acceptance.
- **FO-10.7** (docs + era tags). On ratification, correct every place that names System 7
  as the BASE. Inventoried 2026-08-15:
  - **Prose:** `InitechOS-PRD.md` (Sec 1 "System 7 Toolbox", the Sec 1 acceptance
    sentence "System-7 decomp-golden chrome palette", Sec 3, M3 "Draw a System-7 window"),
    `CLAUDE.md` ("a System-7-style Toolbox (`FLAIR`)"), ADR-0004 Sec 3.7 (D-7 chrome).
  - **Locked policy docs:** `spec/CANON-MANIFEST.md` (the 2026-06-20 era rule -- re-word
    the BASE, KEEP the tagging rule, BC-10.10), `spec/win95ism_guardrails.md:12`.
  - **Header ERA-AXIS blocks:** `spec/control_record.h:18` (and its `:45` Appearance-
    Manager delta note), `spec/menu_record.h`, `spec/dialog_record.h`,
    `spec/window_record.h`, `spec/drawing_ops.h`, `spec/chrome_metrics.h`,
    `spec/chrome_fidelity_golden.h`, `spec/flair_skins.h`, `spec/assets/color_canon.h`
    (regenerated, never hand-edited), `spec/assets/clut.h`, `spec/assets/apple_glyph.h`,
    `spec/assets/menu_canon.h`, `spec/region_algebra.h`, `spec/flair_tenants_demo.h`.
  - **JSON era fields:** `spec/assets/color_canon.json` (top-level `era`),
    `spec/assets/clut.json`, `spec/chrome_metrics.json` (~66 entries),
    `spec/chimera_element_map.json` (`origin: mac-system7`).
  - **`os/flair` banners:** `chrome.{c,h}`, `shell.{c,h}`, `control.{c,h}`,
    `desktop.{c,h}`, `menu.{c,h}`, `dialog.{c,h}`, `event.c`, `process.c`,
    `textedit.{c,h}`, `resource.h`, `ostype.h`.

  Nothing may claim System-7-as-base after ratification, and nothing may claim
  Platinum-as-shipped before FO-10.4 lands. Where a claim is not yet true, it is written
  as `TODO_GOLDEN` (D-10.4), never as an optimistic present tense.
- **FO-10.8** (worklog + memory). Add the `docs/worklog/NNN-*.md` shard for the pivot and
  `bd remember` the ruling, so a post-compaction agent cannot re-derive Sys7-as-base from
  the stale prose.

### 5.3 Neutral / accepted consequences

- The System-7 corpus stays a live golden and a live registry row; nothing minted for
  System 7 is wasted.
- **The registry is not yet load-bearing on metal.** Verified 2026-08-15: `flair_skin_resolve`
  has **no caller in `os/`** -- only `harness/proptest/test_skin_teal.c:87-88` and
  `test_skin_era_frozen.c:145-146`. The base-era flip is therefore, today, a spec + oracle
  act; the visible Platinum look arrives with `chrome.c` (`initech-i3si`). This is recorded
  so no one mistakes a green `test-skin-*` for a Platinum desktop.
- The look change is deliberate and visible; screendump baselines move once, under
  FO-10.4.

---

## 6. Open questions (must be closed before or at ratification)

- **OQ-1 -- the canon module's era tag.** `spec/assets/color_canon.json` carries top-level
  `era: system7.0-7.1`. Does it become `multi` with a per-row `era` (DRAFT recommendation,
  Sec 3.5.3), or does the Platinum layer accrete as a second era-tagged block? Owner:
  `initech-3knt` + this Amendment's ratification.
- **OQ-2 -- the teal bevel under Platinum.** WL-0053 rule 2 replaces the *System-7*
  lavender bevel tinge with same-hue teal light/shadow. Platinum's bevel ramp is a
  different structure with more steps. Does the substitution rule apply per-step (teal
  ramp), only to the outer highlight/shadow pair, or does Platinum's ramp stay neutral gray
  with teal confined to the desktop + accents? **This is an operator Law-4 call**, and it
  cannot be answered until `4lmg` measures the real ramp.
- **OQ-3 -- does Platinum need any non-neutral canon row?** If every Platinum chrome value
  is a neutral gray, `flair_canon_rgb(idx >= 9)` covers it by formula and no canon index is
  appended (Sec 3.5.3). Decided by the mint.
- **OQ-4 -- which Platinum revision?** 8.0 vs 8.1 chrome are believed identical for these
  elements, and 8.1 is the last 68k release and the practical mint target -- but "believed"
  is not Law 1. If the mint shows a difference, the era row must name the revision
  (`ERA_SYS8_PLATINUM` documented as 8.0/8.1 Appearance) exactly as `ERA_SYS7_0_1` names
  7.0-7.1.
- **OQ-5 -- Appearance selectability.** Is the era ever user-selectable in-product (a
  period-authentic Appearance control panel), or is Platinum the sole shipped default with
  SYS7 as a compile-time heritage row only? DEC-10 fixes only the default.
- **OQ-6 -- SSIM crops.** `spec/ssim_params.h` crops are `TODO_GOLDEN` and `harness/ssim.c`
  is unbuilt. When built, they re-anchor to `s8_*`. No action now; recorded so the Sys7
  crops are not silently inherited.
- **OQ-7 -- Route 1 or Route 2 for the runtime seam (Sec 3.6.1).** Does the era axis get
  threaded into `flair_look` / `chrome.c` now (registry becomes load-bearing on metal), or
  do the base constants get re-keyed with the registry left as a graded VIEW? DRAFT
  recommendation: Route 2 now, Route 1 as a funded follow-up. **Operator call.**
- **OQ-8 -- does the 8-bit device CLUT move?** `spec/assets/clut.json` is minted from the
  System-7 ROM `clut_8_rom.bin` and is *believed* identical under Mac OS 8. Believed is not
  Law 1; the `initech-slwi` mint should read the 8.1 CLUT and confirm or refute.
- **OQ-9 -- the Appearance-Manager delta.** `spec/control_record.h:45` records System 7.5+
  / Appearance Manager part-code renames (`kControlButtonPart`, ...) as explicitly
  **out-of-target**. Platinum IS the Appearance era, so that delta may become in-target.
  Numeric values are unchanged either way, so this is a naming/scope question, not a
  layout change -- but it must be answered rather than left contradicting the new base.

---

## 7. Status and ratification conditions

**DRAFT.** This Amendment binds nothing until BOTH:

1. **The goldens are minted** -- `initech-kd60` -> `initech-slwi` -> `initech-4lmg`
   complete, so that ratification is not a promise to invent values later (Law 1: ground
   truth before code); and
2. **The operator ratifies** this document, closing OQ-1 and OQ-2 in the process.

Until then: no edit to `spec/flair_skins.h`, `spec/assets/color_canon.json`,
`spec/chrome_fidelity_golden.h`, `spec/chrome_metrics.h`, or any FLAIR gate is authorised
by this document. The operator's 2026-08-15 **ruling** stands regardless of this
document's status; what is DRAFT is the recorded mechanism, not the ruling.

---

## 8. Related decisions

| Document | Relationship |
|---|---|
| **ADR-0004** (FLAIR Toolbox Architecture) | Parent. Amended at Sec 3.7 (D-7 chrome) and Sec 3.8 (D-8 oracle vector). |
| **ADR-0004-AMENDMENT-DEC-09** (Mechanism/Policy Split) | Amended at Sec 3.7. DEC-10 supersedes ONLY D-9's era STAGING (reserved-zero-rows Platinum; `(0,0)` default). D-9's data-only rule, the no-vtable construction, D-9b, ARB-1..ARB-8, C-8, and OD-4-REVOKED all stand verbatim. |
| **ADR-0010** (FLAIR Grading and Goldens) | CONSUMED unchanged. DEC-10 changes which independent corpus the Mac-chrome legs grade against; it does not touch BC-1..BC-7, the loud-skip contract, the provenance tags, or the depth-trap guardrail. |
| **ADR-0006** (Live Event Loop and Behavioural Grading) | Owns `test-skin-teal`, `test-skin-era-frozen`, `check-win95isms`, `test-interact`, `test-flair-drag`. Their re-key obligations are listed in Sec 4; the bare-desktop teal assert (E-D5) is INVARIANT (Sec 3.3.1). |
| **ADR-0005-AMENDMENT-AM-1** (Dual-Heritage Region Spine) | Untouched. The Platinum era adds no region math and no second engine. |
| **REVOCATION-RECORD-2026-06-21** (FLAIR Heresy Purge) | HER-02 (by-construction grading) and HER-03 (`preview.webp` as a source) are the two heresies the Platinum mint is most likely to resurrect; D-10.4 is written to prevent both. |
| **WL-0053** | The precedent: an operator Law-4 override of a committee-ratified canon, and the DEFINITIVE Initech color ruling this Amendment preserves. |
| **WL-0049 / WL-0054 / WL-0059** | The superseded Sys7-now/Platinum-later staging, including WL-0049's own note that the frame is "likely Platinum, not Sys7". |
| **PRD Sec 1, Sec 3, Sec 8, Sec 9** | Fidelity is the product; every subsystem has a mechanical oracle. Sec 1 prose is corrected by FO-10.7 on ratification. |

---

*End of ADR-0004 Amendment DEC-10. **DRAFT -- NOT RATIFIED.** Pending the minted Mac OS 8.1
goldens and operator ratification. Controlled Document; verify revision before use.*
