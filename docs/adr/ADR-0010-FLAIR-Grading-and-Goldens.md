<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0010 -- FLAIR Grading and Goldens (the decomp corpora as live golden master)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0010 |
| Title | ADR-0010: FLAIR Grading and Goldens |
| Version | 1.0 (Ratified) |
| Status | **RATIFIED (ADR-by-committee + chief-architect reconciliation, operator-delegated authority, 2026-06-21; operator-ratified 2026-06-22)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme (Grading & Goldens committee) |
| Effective Date | 2026-06-21 |
| Next Scheduled Review | Upon operator final confirmation, per RECORDS-POL-002 |
| Supersedes | ADR-0004 OD-4 (seafoam desktop canon); ADR-0004 FO-3, AM-9, FO-F (the seafoam grading apparatus and the by-construction live-desktop color grade) |
| Superseded By | (none) |
| Related Documents | ADR-0004 (FLAIR Toolbox Architecture) + Amendment DEC-09 (Mechanism/Policy Split -- the one canon module); ADR-0005 (ATKINSON region engine) + Amendment AM-1 (Dual-Heritage Region Spine); ADR-0006 (FLAIR Live Event Loop and Behavioural Grading); REVOCATION-RECORD-2026-06-21 (FLAIR Heresy Purge); CDR-0001 (interim toolchain); PRD Sec 8, Sec 9, Sec 12 |
| Related Issues | beads initech-q0gy (Bochs/86Box host-model legs, DEC-04 follow-up); SSIM-deferred issue (filed per FO-5); WIN31_DECOMP wiring (HER-09); System-7 doc-window capture filename reconciliation (FO-8) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | (draft) | Architecture Review Board, STAPLER Programme (Grading & Goldens committee C) | Initial draft. Records the grading architecture that makes the System-7 / Win-3.1 decomp corpora the LIVE golden master for FLAIR color; the one value oracle test-color-canon (4 legs + named VALUE mutants); PNG-pixel loud-skip; the Win depth-trap guardrail; the single ppm_flair_check re-key; the seafoam-apparatus retirement; the SSIM defer; the loud-skip / provenance-honesty contract; WIN31_DECOMP wiring. As drafted the committee forked the canon module and stood up a fourth value oracle. | (pending reconciliation) |
| 1.0 | 2026-06-21 | Architecture Review Board, STAPLER Programme + Chief-Architect Reconciliation | Reconciled and ratified. De-forked: this ADR CONSUMES the one arbitrated canon module (ADR-0004 Amendment DEC-09), does not author a table (CD-1). De-duplicated: exactly one value oracle test-color-canon (CD-2), C's test-flair-canon retired. Carries decisions CD-1..CD-8. SUPERSEDES ADR-0004 OD-4 / FO-3 / AM-9 / FO-F; HARD-REVOKES heresy HER-02 in writing. Verified-composes by chief-architect reconciliation; all cited goldens verified present in-tree. | ARB (Bolton / Nagheenanajar / Smykowski + Fidelity Steward) + Chair + Chief-Architect Reconciliation |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Architecture Review Board, STAPLER Programme (Grading & Goldens committee) | Submitted (DRAFT) | (draft) |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Approved (2026-06-21) | 2026-06-21 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Approved (2026-06-21) | 2026-06-21 |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Approved (2026-06-21) | 2026-06-21 |
| ARB Reviewer -- Fidelity Steward | Fidelity Steward (Heritage Conformance) | Approved (2026-06-21) | 2026-06-21 |
| ARB Chair (Synthesis) | ADR-by-committee Chair | Synthesized + Approved (2026-06-21) | 2026-06-21 |
| Chief-Architect Reconciliation | Chief Architect (cross-committee composition) | Verified-Composes + Reconciled (2026-06-21) | 2026-06-21 |
| Operator Ratification | T. Osborne (Operator) | **Granted via delegated committee authority (2026-06-21; operator-ratified 2026-06-22)** | 2026-06-21 |
| Records Management | M. Waddams (Archive Annex B) | Filed (2026-06-21) | 2026-06-21 |

*Note on status: This ADR is RATIFIED (ADR-by-committee + chief-architect reconciliation, operator-delegated authority, 2026-06-21; operator-ratified 2026-06-22). The architecture was authored by the Grading & Goldens committee and reconciled against the sibling FLAIR deliverables by a chief-architect composition pass. The reconciliation verdict is COMPOSES: this ADR consumes the single arbitrated canon module (ADR-0004 Amendment DEC-09) rather than forking it, and contributes exactly one color VALUE oracle rather than a fourth. The decisions CD-1..CD-8 are settled and binding. The forward gaps in Section 6.3 (the Bochs/86Box host-model legs, the SSIM build, the System-7 capture filename) are recorded as funded forward obligations, not open questions.*

---

## 1. Purpose and Scope

### 1.1 Purpose

This ADR establishes the **grading architecture** of FLAIR color -- the mechanical truth that makes the **System-7 and Win-3.1 decomposition corpora the LIVE golden master** for every color value FLAIR renders, and that ends the by-construction color grade FLAIR shipped under (PRD Sec 8, Sec 9; the canonical anti-oracle the Heresy Purge was convened against). Its purpose is to fix:

- the **single FLAIR color VALUE oracle** (`test-color-canon`) and its four grading legs and named VALUE mutants;
- the **PNG-pixel loud-skip** binding factory fact;
- the **Win-accent depth-trap guardrail**;
- the **single re-key** of the live-desktop screendump oracle (`tools/ppm_flair_check.c`);
- the **retirement of the seafoam apparatus**;
- the **honest deferral of SSIM**; and
- the **loud-skip + provenance-honesty contract** binding on every FLAIR color oracle.

### 1.2 Scope

This ADR governs **GRADING and GOLDENS only**. It OWNS:

- The one value oracle `test-color-canon` (Section 3, CD-2).
- The `ppm_flair_check` re-key (Section 3, CD-5).
- The `WIN31_DECOMP` Makefile wiring (Section 3, CD-3 / CD-4; first Win-3.1 corpus wiring, HER-09).
- The SSIM defer and the host-model-per-mode cross-emulator status (Section 3, CD-7).
- The loud-skip / provenance-honesty contract (Section 3, CD-8).

### 1.3 Out of Scope (CONSUMED, not authored)

- **The one canon module.** This ADR does **not** author a color table. The single LOCKED `spec/assets/color_canon.json` and its generated `spec/assets/color_canon.h` (exposing `flair_canon_rgb(uint8_t idx)`) are arbitrated and owned by **ADR-0004 Amendment DEC-09 (Mechanism/Policy Split)** under critic resolution R1. This ADR CONSUMES that module and grades it (CD-1). (Ref: `spec/assets/color_canon.json:6` -- "ratified_by ... ADR-0004-AMENDMENT-DEC-09 + ADR-0010".)
- **The WL-0053 bevel -> teal correction.** The lavender-to-teal swap (`bevel_light #8DDCDC`, `bevel_shadow #4E9BA3`) lives in the canon module; Committee A's lavender-bevel proposal is overruled there (R2), not here. This ADR grades those rows with the authored posture (LEG D).
- **The mechanism/policy cut line** (constraint C-8), the `flair_look_pixel(port, PART)` resolver, and the `flair_skin_t` registry view -- all owned by ADR-0004 Amendment DEC-09. This ADR keeps the orthogonal structure oracles those decisions stand up (`test-mech-policy`, the colorblind sentinel render, `test-skin-teal`, `test-skin-era-frozen`) but does not author them.
- **The region engine** and its homomorphism property suite (ADR-0005 + Amendment AM-1) -- legitimately without an external golden (the QuickDraw region body is unpublished) and not folded into chrome color canon.
- **The booted live cooperative loop** and behavioural grading (ADR-0006) -- the bare-desktop teal assert is a consumer of this canon, downstream.

### 1.4 Additional Defined Terms

| Term | Definition |
|---|---|
| canon module | The one LOCKED `spec/assets/color_canon.json` -> generated `spec/assets/color_canon.h`, accessor `flair_canon_rgb(uint8_t idx) -> 0x00RRGGBB`. Authored by ADR-0004 Amendment DEC-09; consumed here. |
| value oracle | An oracle that grades a rendered/derived RGB VALUE against an INDEPENDENT decomp golden (a source distinct from the one the artifact renders from). The single FLAIR value oracle is `test-color-canon`. |
| by-construction grade | An oracle that computes its "expected" value from the same function the artifact renders from. This is NOT an oracle (Law 2, PRD Sec 9, principle P3). HER-02 is the canonical instance. |
| LOUD-SKIP | The test-clut discipline: when a sibling golden is absent the oracle prints the exact missing path and the override macro, runs only its value-INDEPENDENT structural checks, exits zero, and the aggregate distinguishes SKIPPED from GREEN. Never a silent pass. (Ref: `harness/proptest/test_clut.c:335-346`.) |
| depth-trap | Grading a documented indexed-8 WIN.INI value against a 16-color VGA emulation artifact under a tolerance wide enough to swamp the channel. A Stop condition (CLAUDE.md "Stop conditions"). |
| provenance tag | The per-row source class: `DECOMP_RESOURCE`, `DECOMP_DOC`, `DECOMP_PIXEL_16COLOR`, or `INITECH_CANON_DATUM`. `preview.webp` is `PROVENANCE_ONLY` and never a grading or render source. |

---

## 2. Context

### 2.1 The problem this ADR closes

FLAIR shipped its live-desktop color "oracle" as a **by-construction grade**. The screendump checker `tools/ppm_flair_check.c` computed its expected RGB from `flair_palette_rgb(idx)` -- the exact function `kmain`'s present path renders from -- so a wrong palette value flowed identically into both the rendered pixel and the "expected" value, and the +/-2 diff could never bite on color (Ref: `tools/ppm_flair_check.c:14-19, 53, 160-163`; `os/milton/kmain.c:709`). This is heresy **HER-02**, the precise anti-oracle that PRD principle P3 (grade against an INDEPENDENT golden) was ratified against. It is what let the wrong `preview.webp` palette pass every gate and earned a GREEN ratification at re30.3 (WL-0052) that it did not deserve (Law 2: "An agent that claims it works but never ran the oracle -- or ran it against a stub golden -- has produced nothing").

The render source itself was wrong: `flair_palette_rgb` returned the dim Office-Space CRT mock-up samples (`preview.webp` 3x3 samples -- muddy menu bar `#67696C`, "white" `#7F7F86`, pinstripe `#6B6B74`), which are exactly the operator's WL-0053 complaint (heresy HER-03). The fix to the render values is owned by ADR-0004 Amendment DEC-09 (the canon module). The fix to the GRADING -- making an independent decomp golden the truth -- is this ADR.

### 2.2 The reconciliation

As drafted, the Grading & Goldens committee did two things the chief-architect reconciliation corrected:

1. It **forked the canon module**: its `spec/assets/flair_color_canon.{json,h}` was one of FOUR competing index->RGB tables proposed across committees A/C/D/F. Reconciliation R1 collapses the four into the ONE arbitrated `spec/assets/color_canon.json`; this ADR DELETES `flair_color_canon.{json,h}` and CONSUMES `color_canon.h` (CD-1).
2. It **stood up a fourth value oracle** (`test-flair-canon`), one of four overlapping value oracles. Reconciliation R4 consolidates to the ONE `test-color-canon` with D's 4-leg structure; `test-flair-canon` is retired and its rigor folded into legs A and C (CD-2).

The architecture direction was already sound and independent -- every cited golden was verified to exist in-tree -- so the only fixes were de-forking the module, de-duplicating the oracle, and consuming the WL-0053 bevel -> teal correction that lives in the canon module.

### 2.3 Ground truth

- The live by-construction grader is real and is re-keyed, not deleted (Ref: `tools/ppm_flair_check.c:14-17, 53, 163`; the P6 PPM reader at `:208`; no PNG decoder in the factory).
- The four candidate modules do not yet exist in-tree (clean slate); the live state is five drifted index->RGB switches (`chrome.c chrome_pal_rgb`, `control.c ctrl_pal_rgb`, `dialog.c dlg_pal_rgb`, `harness/render/render.c render_palette_rgb`, `spec/assets/palette.h flair_palette_rgb`) plus `shell.c`/`desktop.c` inline literals, all carrying `preview.webp` values.
- The desktop value DIVERGES across sites today: seafoam `0x6FA08E` in `palette.h`/`render.c`, and `0x73696C` in `chrome.c`/`control.c`/`dialog.c` -- so the collapse onto the canon is a value change for three sites, sequenced oracle-first (CD-6, R3).
- All cited goldens are present: `../system7-decomp/goldens/resources/wctb_0_System_753.bin` (== `wctb_0_System_colorHD.bin`, byte-identical, MD5 `dede55082e92a3c6bce408335cf77614`); `../system7-decomp/specs/chrome/pinstripe.md`; `../win31-decomp/refs/win31-chrome/default-colors-cross-check.txt`; `../win31-decomp/specs/chrome/color-scheme.md`. Both decomp trees are gitignored, so absence is the COMMON case on a fresh checkout -- which is why loud-skip is load-bearing.

---

## 3. The Decision

The decisions CD-1 through CD-8 are binding. They are stated as the Grading & Goldens reconciliation rulings.

### 3.1 CD-1 -- CONSUME the one canon module (R1)

ADR-0010 does **NOT** author a color table. It CONSUMES the single LOCKED `spec/assets/color_canon.json` -> generated `spec/assets/color_canon.h` exposing `flair_canon_rgb(uint8_t idx)`, arbitrated under R1 by the Canon-Module deliverable (idx0..8 + the two derived teal-bevel rows + era/heritage tags + a `wctb_part` crosswalk field per entry). Committee C's `spec/assets/flair_color_canon.{json,h}` and the accessor-on-its-own-table are RETIRED/DELETED; the grading layer references ONLY `color_canon.h`. Every oracle in this ADR grades the SAME idx table against an INDEPENDENT decomp golden -- there is exactly one render source and exactly one thing to grade.

*Rationale.* R1 is the load-bearing fix: A/C/D/F each invented a different color-policy file, which would replace "collapse 5 switches to ONE" with "collapse 5 switches to FOUR." The grading committee must grade the ARBITRATED table, not its own. The values line up across all four (idx2 teal `#8DDCDC`, idx5 navy `#000080`, idx6 `#C0C0C0`, idx7/8 pinstripe `#F3F3F3`/`#969696`); only the file/schema was forked, so consuming `color_canon.h` is a pure pointer change with no value drift.

### 3.2 CD-2 -- ONE value oracle test-color-canon, four legs (R4)

There is exactly ONE FLAIR color VALUE oracle: **`test-color-canon`** (`harness/proptest/test_color_canon.c`, factory C, the `test_clut.c` fopen / entry-diff / loud-skip idiom, dual-compiled freestanding + hosted). It is an INDEPENDENT golden, NEVER by-construction: it grades the GENERATED `color_canon.h` against goldens from a source DISTINCT from the render. Four legs, each grading `flair_canon_rgb(idx)`:

- **LEG A -- wctb binary (strongest).** Parse `$(SYSTEM7_DECOMP)/goldens/resources/wctb_0_System_colorHD.bin` (TOL=0; header big-endian `wCSeed:u32`, `wCReserved:u16`, `ctSize:u16` then `ctSize+1` ColorSpec `{value:u16, r/g/b:u16 high-byte}`) for the System-7 part anchors: idx1 content `#FFFFFF` (part0), idx0 frame `#000000` (part1), idx4 text `#000000` (part2), idx3 menu bar `#FFFFFF` (part0). Cross-keyed via the `wctb_part` field of each canon row (R8) so every oracle grades the SAME slot against the SAME wctb part.
- **LEG B -- win31 TEXT (strong).** Parse `$(WIN31_DECOMP)/refs/win31-chrome/default-colors-cross-check.txt` HEX equivalents (three independent documented WIN.INI sources), cross-checked against `$(WIN31_DECOMP)/specs/chrome/color-scheme.md`, for the indexed-8 accents: idx5 navy `#000080`, idx6 BTNFACE `#C0C0C0`, with idx1/idx3 white `#FFFFFF` and idx0/idx4 `#000000` corroboration.
- **LEG C -- pinstripe.md (weakest gradeable).** Parse `$(SYSTEM7_DECOMP)/specs/chrome/pinstripe.md` rendered rows (idx7 `#F3F3F3` light, idx8 `#969696` dark) -- the RENDERED dither, NOT wctb part7/part8 (which are the `#FFFFFF`/`#000000` WDEF shade-table ENDPOINTS, a distinct lane). Three-way corroborated against `spec/chrome_metrics.h` pinstripe RGBs (resolves HER-07).
- **LEG D -- authored-teal.** The WL-0053 datum idx2 `#8DDCDC` + the derived teal bevel (`bevel_light #8DDCDC`, `bevel_shadow #4E9BA3`) graded as `INITECH_CANON_DATUM` against the LOCKED `color_canon.json` datum (generated-header-vs-JSON, catches transcription) + the WL-0053 provenance + the seafoam-relapse VALUE mutant + the derived-shadow lock + a green-cyan-octant luminance bound. The superseded System-7 gray/lavender baseline is recorded ALONGSIDE (deviation-audit), NEVER as the expected value.

Committee C's `test-flair-canon` as a separate oracle is RETIRED -- its rigor (resource-binary-preferred, golden-pixel reading) folds into legs A and C. The duplicate per-skin value oracles are RETIRED. A's `test-mech-policy` source-scanner and colorblind sentinel render, and F's `test-skin-teal` and `test-skin-era-frozen`, are KEPT as orthogonal (they grade STRUCTURE/boundary/accretion, not values) and are NOT folded in here.

All legs LOUD-SKIP per-arm when the corpus is absent (print the exact path; exit non-zero only on a real mismatch, exactly `test_clut.c:340-346`), NEVER silent-pass.

*Rationale.* R4: four value oracles for one table is the harness double-build. D's 4-leg structure is the most golden-grounded and survives. Each leg's expected value comes from a decomp binary/text/spec or the locked authored datum -- never from the render source -- so self-consistency cannot satisfy the diff (P3).

### 3.3 CD-3 -- PNG-pixel arms LOUD-SKIP (binding factory fact; R4)

Every PNG-pixel grading arm (the System-7 golden-PNG-pixel reading C preferred, and any Win-3.1 16-color PNG corroboration) **LOUD-SKIPS unconditionally today**, printing the missing-capability banner `PNG-pixel arm SKIPPED -- factory has no PNG decoder (only P6 PPM reader); graded via resource-binary/text leg instead` plus the absent path and the `-D` override macro. The factory has only a P6 PPM reader (`tools/ppm_flair_check.c:208`); there is no PNG decoder. The value legs that bite are LEG A (wctb binary, TOL=0), LEG B (win31 text), LEG C (pinstripe.md text rows), LEG D (authored datum). A PNG-pixel arm may be ENABLED later ONLY when a mutation-checked PPM crop is minted FROM the golden by a separate minter -- never hand-typed, else a laundered constant.

The committed System-7 capture filename is UNSETTLED (`pinstripe.md` prose says `s7_doc_window.png`; the captures present are `s7_701_*`). The oracle MUST resolve the path by macro priority `FLAIR_*_GOLDEN_PATH > $(SYSTEM7_DECOMP|WIN31_DECOMP)/goldens/... > default` and never hardcode one name (FO-8).

*Rationale.* R4: D's factual correction is right; A's and C's PNG-pixel-primary readings are UNRUNNABLE today. The legs that DO bite are all text/binary, which the factory can read. Loud-skip (never silent-pass) is the test-clut discipline that keeps the oracle honest on a fresh checkout where the gitignored goldens are absent.

### 3.4 CD-4 -- Win-accent depth-trap guardrail (R5)

Win-3.1 accent rows grade at the depth where indexed-8 FLAIR is EXACT: the DOCUMENTED WIN.INI indexed-8 value is the PRIMARY assertion -- ActiveTitle navy `#000080`, ButtonFace `#C0C0C0`, ButtonShadow `#808080`, Window / BTNHILIGHT `#FFFFFF` -- sourced from `default-colors-cross-check.txt` (three independent sources). The 16-color VGA value (`#0000AA` navy, `#C3C7CB` face, `#868A8E` shadow, with the documented `+3/+7/+11` and `+6/+10/+14` DOSBox-X offsets) is CORROBORATION-ONLY, tagged `DECOMP_PIXEL_16COLOR`, never the primary gate, and (being a PNG pixel) LOUD-SKIPS per CD-3.

It is a **Stop condition** to grade the indexed-8 datum `#000080` against `#0000AA` with a ~42-level blue tolerance that swamps the channel -- that is weakening an oracle to pass (CLAUDE.md "Stop conditions"). Two Law-3 guardrail asserts are enforced: REJECT `#DFDFDF` (Win95 COLOR_3DLIGHT) and `#008080` (Win desktop teal, COLOR_BACKGROUND -- NOT FLAIR's teal) ever being conflated with the FLAIR teal `#8DDCDC`.

*Rationale.* R5: `color-scheme.md` Sec 2.1 is explicit -- "For FLAIR: use #000080 ... indexed-8 depth renders the WIN.INI value exactly." The `+3/+7/+11` gray and the `#0000AA`-vs-`#000080` deltas are documented VGA/CGA emulation artifacts, not the target. Per-row exact sourcing is period-authentic; conflating depths and inflating the tolerance is not.

### 3.5 CD-5 -- the single ppm_flair_check re-key (owned HERE; R6)

ONE edit to `tools/ppm_flair_check.c`, owned by this (grading) committee, ending the five-way merge (A/C/D/E/F all wanted to edit this file):

1. **STRIKE** the "agree BY CONSTRUCTION" header paragraph (`:14-19`, `:53`) and the "SEAFOAM DESKTOP" language.
2. **Re-key** its COLOR expectations through `flair_canon_rgb(idx)` from `color_canon.h` (the now-independently-graded canon) instead of `flair_palette_rgb` (the render source) (Ref: `:163`).
3. **Rename** the desktop probe `SEAFOAM -> TEAL`, expected `#8DDCDC` (idx2).
4. **KEEP VERBATIM** every palette-value-INDEPENDENT structural probe: the two-menu-bar Apple-slot chimera ink-density tell, the far-right Photoshop title-ink tell, the pinstripe PERIOD-2 ALTERNATION as the value-free index relation `shade[k] != shade[k-1] && shade[k] == shade[k-2]`, 1px-frame exactness, centered-modal / 7px-border exactness, and Z-ORDER occlusion.
5. **WIRE** `test-color-canon` to RUN FIRST as a HARD precondition of the flair-desktop screendump gate (the whole gate FAILS if `test-color-canon` is RED or skipped-without-acknowledgment), so live-LFB color is graded TRANSITIVELY against the decomp master.
6. **ADD** a COLOR VALUE-mutant arm distinct from the existing SCENE mutants (`onebar`/`nomodal`/`modalbehind`): force one `EXPECT(role)` wrong and assert the screendump gate goes RED -- proving the color limb bites, not just the structural limb.

The tolerance stays tight (+/-2); its justification changes from "exact by construction" to "the canon is decomp-graded by `test-color-canon` and QEMU XRGB8888 -> P6 is exact." `ppm_flair_check` stays the STRUCTURE / screendump oracle ONLY; it is NEVER promoted to the color value oracle (that would rebirth HER-01).

*Rationale.* R6, assigned to ONE committee to avoid the five-way merge. The structural/geometry/z-order/alternation probes were ALWAYS legitimate oracles (they test topology/relationships, not RGB values), so deleting them would lose real Law-4 signal; they are kept. `ppm_flair_check` remains the only LIVE-on-the-LFB Law-4 gate (the booted OS renders the present path; this screendump proves it). Chaining `test-color-canon` as a hard precondition makes the transitive grading a wiring a future agent cannot silently sever.

### 3.6 CD-6 -- RETIRE the seafoam apparatus (R3 value-change discipline)

RETIRE `harness/proptest/test_palette_seafoam.c` and its Makefile target (heresy HER-08, the by-construction `palette.h == stage2.asm` self-equality, AND HER-06, the baked seafoam), and any `ppm_seafoam_check` apparatus. The `desktop_bg` datum role migrates to the `color_canon.json` teal row (idx2 `#8DDCDC`, `INITECH_CANON_DATUM`, WL-0053, LEG D). A SEPARATE structural consistency tooth (the `stage2.asm` boot-fill constant == canon idx2 `desktop_bg`, graded against the canon -- explicitly NOT the color oracle) replaces its structural role, plus the `FLAIR_CANON_MUTATE_DESKTOP_TEAL` (`#8DDCDC -> #6FA08E` seafoam) VALUE mutant as the standing seafoam-relapse tripwire.

The collapse of the five switches onto `flair_canon_rgb` is sequenced **ORACLE-FIRST** as a deliberate Rule-8 act, NOT a transparent refactor: per R3 the five switches carry THREE divergent desktop values today (seafoam `0x6FA08E` in palette/render, `0x73696C` in chrome/control/dialog), so the collapse IS a value change for three sites. `test-color-canon` must be GREEN against the canon BEFORE the render flip.

*Rationale.* R3 drops the false byte-identical "Step-1" premise: collapsing onto the canon is a deliberate value change, sequenced oracle-first. Retiring `test_palette_seafoam` removes the by-construction pattern AND the revoked seafoam constant in one bisectable step. The seafoam->teal VALUE mutant is the exact guard the old by-construction checker structurally could not have (teal->seafoam passed because both sides read the same switch).

### 3.7 CD-7 -- DEFER SSIM honestly; freeze host-model-per-mode SSIM-independent

SSIM is FORMALLY DEFERRED -- a GUIDE, never a gate (Law 4; ADR-0004 D-8 / AM-6 FROZEN). Do NOT build it now; the load-bearing grading is CD-2 (`test-color-canon`) + CD-5 (the re-keyed `ppm_flair_check`).

- Make `make ssim` an HONEST deferred stub printing `SSIM DEFERRED -- not built; guide-only when built; goldens = decomp captures (s7 desktop/doc-window, w31), tracked by bd <id>` (replacing the current `$(call stub_fail,ssim,M0)` at `Makefile:8250`).
- Correct every doc that implies it runs -- the PRD, ADR-0004 / ADR-0005 prose, `CLAUDE.md` ("make ssim FRAME=desktop"), the Makefile help text (`:6743`) -- to read `SSIM: DEFERRED (planned per-window guide, not yet built; bd <id>)`.
- RATIFY in writing: cross-emulator agreement is **host-model-per-mode** (each emulator's screendump digest vs the host model's prediction for THAT emulator's mode, parameterized by runtime `boot_info_t` LFB geometry) and is **SSIM-INDEPENDENT** -- HER-15 ("cross-emu host-model depends on SSIM") is DISMISSED. Host-model-per-mode is realized only for the QEMU 32bpp arm (`harness/render` + `ppm_flair_check`); the Bochs / 86Box legs are the funded DEC-04 follow-up (beads `initech-q0gy`).
- If SSIM is ever built it grades a rendered screendump against a decomp RENDERED golden (s7 desktop/doc-window PNG, w31) -- an INDEPENDENT golden -- NEVER `preview.webp` and never the host render of our own scene (by-construction again).

*Rationale.* Law-1 / Law-2 honesty: a documented-as-running but stubbed oracle is a latent by-construction trap. Nothing depends on SSIM gating so the boot stays green. Decoupling cross-emu agreement from SSIM in writing prevents a future agent blocking the tri-emulator gate on the unbuilt SSIM (a Stop condition). Re-anchoring any eventual SSIM crops to decomp rendered goldens keeps P4 intact for the day it lands.

### 3.8 CD-8 -- the loud-skip + provenance-honesty contract (binding)

Codify in this ADR and in the `color_canon.json` schema, binding on EVERY FLAIR color oracle:

1. **Per-row golden resolution** by macro priority `FLAIR_*_GOLDEN_PATH > $(SYSTEM7_DECOMP | WIN31_DECOMP)/goldens|refs/... > default` (the test-clut idiom). An ABSENT golden LOUD-SKIPS, printing the exact missing path + the `-D` override macro + "golden absent -- NOT graded", never silent-passes, and the row continues only its value-INDEPENDENT structural checks.
2. **Explicit provenance tag per row** (`DECOMP_RESOURCE` / `DECOMP_DOC` / `DECOMP_PIXEL_16COLOR` / `INITECH_CANON_DATUM`); `preview.webp` appears ONLY as a `PROVENANCE_ONLY` annotation, never a render or grading source (P4), enforced by a grep gate that FAILS if any renderer or oracle reads `preview.webp` / the `FRAME_V0` gray as a color source.
3. **Per-row documented tolerance** tied to the exact depth offset (`color-scheme.md`), never a blanket loosening (Stop condition).
4. **Skip accounting.** A green run that loud-skipped N rows MUST print "N rows NOT graded (goldens absent)", and the `make test` aggregate MUST distinguish GREEN (golden present + diffed) from SKIPPED, so a skip-everything run can never be mistaken for a pass.
5. **Deviation-audit.** The LEG-D teal datum records the superseded System-7 gray/lavender baseline + the WL-0053 citation alongside, so "why not the decomp value" is always mechanically answerable.

*Rationale.* P3 / P4 provenance-honesty spine: the difference between a live golden master and decoration is whether absence is LOUD and provenance is EXPLICIT. Both decomp trees are gitignored, so absence is the COMMON case on a fresh checkout. The `DBASE3_DECOMP` precedent (`Makefile:714` -- "A skipped oracle is worse than a red one") and the test-clut loud-skip (`test_clut.c:335-346`) are the operator-ratified discipline this mirrors.

### 3.9 The named VALUE mutants (Rule 6)

`make test-color-canon-mutant` asserts each mutant exits non-zero (drives `test-color-canon` RED, then restores GREEN), named to the exact revoked heresies:

| Mutant | Perturbation | Leg that bites |
|---|---|---|
| `FLAIR_CANON_MUTATE_PINSTRIPE_LIGHT` | `#F3F3F3 -> #F2F3F3` (one LSB) | LEG C (channel-discrimination proof) |
| `FLAIR_CANON_MUTATE_DESKTOP_TEAL` | `#8DDCDC -> #6FA08E` (seafoam) | LEG D (seafoam-relapse tripwire / HER-01, HER-06) |
| `FLAIR_CANON_MUTATE_MENUBAR_WHITE` | `#FFFFFF -> #67696C` (preview gray) | LEG A (HER-02 tripwire) |
| `FLAIR_CANON_MUTATE_WIN_NAVY` | `#000080 -> #0000AA` (16-color) | LEG B (proves the indexed-8 datum, not the DOSBox-X artifact, is gated; R5) |
| `CANON_MUTATE_INK` | `#000000 -> #010000` | LEG A wctb |
| `CANON_MUTATE_BEVEL` | `bevel_shadow` off the derived-shadow ratio | LEG D shadow lock |
| `CANON_SKIP` | rename a golden path | MUST print LOUD-SKIP and NOT silent-pass |

These are the VALUE mutants the old by-construction checker structurally could not have had: under HER-02, teal->seafoam passed because both sides read the same switch.

### 3.10 NON-ORACLE (explicitly NOT a gate)

`make ssim` is a DEFERRED honest stub (CD-7); when/if built it grades a rendered screendump against a decomp RENDERED golden, NEVER `preview.webp` and never the host render of our own scene, and it remains a per-window GUIDE, never summed into a gate (Law 4). `clut.h` / `test-clut` (HER-10, KEPT) stays the device-CLUT MECHANISM exemplar, NOT folded into chrome color canon. The region engine keeps its homomorphism property suite (legitimately no external golden; ADR-0005).

---

## 4. Rationale

The architecture is the application of the four Laws and PRD principles P3/P4 to FLAIR color:

- **Law 2 (the oracle is the truth).** HER-02 produced a GREEN that meant nothing because the oracle read the render source. The single fix that matters is making the EXPECTED value come from a decomp corpus the artifact does not render from. `test-color-canon` legs A/B/C and the `ppm_flair_check` re-key + hard-precondition chain (CD-5) are that fix.
- **Law 1 (ground truth before code).** Every gradeable row cites a local decomp golden by macro-resolved path; the authored rows (idx2 teal, the two bevel rows) are honestly tagged `INITECH_CANON_DATUM` and never claimed decomp-sourced (P4). The deviation-audit (CD-8.5) keeps "why not the decomp value" mechanically answerable.
- **Law 4 (look and feel like the frame).** SSIM is the per-window fidelity GUIDE, and it is deferred honestly rather than stubbed-as-running; the load-bearing color truth is the value oracle, the structural probes in `ppm_flair_check` carry the live Law-4 signal, and the canonical bugs stay canon (the 116% pie, the 570- minus, the hourglass) untouched.
- **Rule 6 (mutation-proven goldens).** A golden that never caught a regression is decoration; the seven named VALUE mutants (Section 3.9) prove each leg bites, and `CANON_SKIP` proves the loud-skip path is honest.
- **Rule 8 (specs are locked data).** The grading contract is codified in the locked `color_canon.json` schema (`grading_contract`, `win_depth_trap`, `wctb_crosswalk_note`), not in prose in someone's head.

No by-construction oracle is described as valid anywhere in this document; the entire purpose of the ADR is to extinguish the one that shipped.

---

## 5. Consequences

### 5.1 Binding Constraints

- **BC-1.** The single FLAIR color VALUE oracle is `test-color-canon`. No second color value oracle may be stood up; the grading layer references ONLY `color_canon.h` (CD-1, CD-2).
- **BC-2.** No FLAIR color oracle may grade against the source the artifact renders from. By-construction color grading is a Stop condition; `ppm_flair_check` may never be promoted to the color value oracle (CD-5; HER-01/HER-02).
- **BC-3.** Every PNG-pixel grading arm LOUD-SKIPS until a PNG decoder or a mutation-checked minted PPM crop exists; PNG-pixel values may never be hand-typed into an oracle (CD-3).
- **BC-4.** Win-accent rows grade the documented indexed-8 WIN.INI value as PRIMARY; the 16-color VGA value is corroboration-only at its own offset; a swamping (~42-level) tolerance is a Stop condition; `#DFDFDF` and `#008080` are guardrail-rejected against the FLAIR teal (CD-4).
- **BC-5.** Loud-skip is mandatory (never silent-pass); the `make test` aggregate MUST distinguish GREEN from SKIPPED; every row carries an explicit provenance tag; `preview.webp` is `PROVENANCE_ONLY`, enforced by a grep gate (CD-8).
- **BC-6.** The five index->RGB switches collapse onto `flair_canon_rgb` ORACLE-FIRST: `test-color-canon` GREEN before any render flip (CD-6, R3).
- **BC-7.** SSIM is a GUIDE, never a gate; `make ssim` is an honest deferred stub; cross-emulator agreement is host-model-per-mode and SSIM-INDEPENDENT (CD-7).

### 5.2 Forward Obligations

- **FO-1 (CD-2 / CD-3).** Land `test-color-canon` (legs A/B/C/D) + the named mutants GREEN and MUTATION-PROVEN, with `SYSTEM7_DECOMP ?= ../system7-decomp` and a NEW `WIN31_DECOMP ?= ../win31-decomp` wired ONCE in the Makefile (mirroring SYSTEM7_DECOMP / test-clut; first Win-3.1 wiring, HER-09), loud-skip when absent, BEFORE any of the five switches is collapsed.
- **FO-2 (CD-5 / R6).** Execute the SINGLE `ppm_flair_check.c` re-key here -- strike the by-construction language (`:14-19`, `:53`), re-key onto `flair_canon_rgb`, rename `SEAFOAM -> TEAL` (`#8DDCDC`), keep all structural / z-order / pinstripe-alternation probes verbatim, wire `test-color-canon` as a hard precondition, add the COLOR VALUE-mutant arm; verify the live flair-desktop image still boots green. Coordinate so A/D/E/F do NOT also edit this file.
- **FO-3 (CD-3 / CD-4).** Wire `WIN31_DECOMP ?= ../win31-decomp` once beside `SYSTEM7_DECOMP` (`Makefile:~8946`); grade win accents at the documented indexed-8 depth (primary), 16-color pixel corroboration-only (loud-skip); enforce the `#DFDFDF` / `#008080` Law-3 guardrail asserts. Closes HER-09.
- **FO-4 (CD-6 / R3).** Retire `test_palette_seafoam.c` and its target and any `ppm_seafoam_check` apparatus; migrate `desktop_bg` into the canon idx2 teal row + the stage2-vs-canon structural tooth + the teal VALUE mutant; record the superseded System-7 gray baseline as the deviation-audit record. Sequence the five-switch collapse oracle-first.
- **FO-5 (CD-7).** File the SSIM-deferred beads issue; convert `make ssim` (`Makefile:8250`) to an honest deferred stub; correct the PRD / ADR-0004 / ADR-0005 prose / `CLAUDE.md` / Makefile help (`:6743`) that imply `make ssim` runs; ratify in writing that cross-emu agreement (host-model-per-mode) is SSIM-INDEPENDENT (dismiss HER-15); record QEMU 32bpp realized / Bochs + 86Box pending (DEC-04, beads `initech-q0gy`); add the `preview.webp`-not-a-source grep gate.
- **FO-6 (CD-8).** Codify the loud-skip + provenance-honesty contract; add the "N rows NOT graded (goldens absent)" summary line; make the `make test` aggregate distinguish GREEN from SKIPPED; confirm `make test` stays green with goldens present and loud-skips (never silent-passes) when sister repos are absent.
- **FO-7.** Amend ADR-0004 D-8 (Section 3.8 + Section 8.2 as AM-10): (a) replace the canon row's seafoam desktop with "Initech teal #8DDCDC + decomp-golden color grade"; (b) add the `test-color-canon` hard pass/fail row (loud-skip, VALUE-mutant-proven, win accents at documented depth); (c) re-state the `ppm_flair_check` live-desktop row as graded against `color_canon.h` not `flair_palette_rgb`, chained after `test-color-canon`; (d) keep the ssim row GUIDE-ONLY but mark DEFERRED-UNBUILT with its beads id + the HER-15 host-model-per-mode note; preserve the AM-6 frozen definition verbatim; mark FO-3 / AM-9 SUPERSEDED.
- **FO-8 (FO-C10).** Resolve the System-7 doc-window capture filename (`pinstripe.md` prose `s7_doc_window.png` vs present captures `s7_701_*`): `test-color-canon` MUST resolve by macro priority + loud-skip, never hardcode one name; file a beads issue to confirm the canonical capture filename with the system7-decomp maintainer before committing PNG-pixel coords (relevant only when the PNG-pixel arm is later enabled per CD-3).

### 5.3 Recorded forward gaps

- The host-model-per-mode cross-emulator agreement is realized only for the QEMU 32bpp arm; the Bochs and 86Box legs are the funded DEC-04 follow-up (beads `initech-q0gy`).
- SSIM is unbuilt by design; it is a deferred honest stub behind a beads id.
- The PNG-pixel arms are unrunnable until a PPM minter or a PNG decoder lands; they loud-skip in the interim.

---

## 6. Supersession and Revocation

### 6.1 Supersedes

This ADR SUPERSEDES, in writing:

- **ADR-0004 OD-4** -- the seafoam `#6FA08E` desktop canon and the D-8 "canon" hard-gate seafoam row. The desktop background is now Initech teal `#8DDCDC` (idx2), graded against the locked canon datum (LEG D) with the seafoam-relapse VALUE mutant as the standing tripwire. (Ref: `spec/assets/color_canon.json:94-97` -- the OD-4 seafoam recorded REVOKED 2026-06-21, retained for revocation-history traceability only.)
- **ADR-0004 FO-3, AM-9, and FO-F** -- the seafoam grading apparatus and the by-construction live-desktop color grade. Replaced by `test-color-canon` (CD-2) + the re-keyed `ppm_flair_check` (CD-5) + the retired seafoam apparatus (CD-6).

### 6.2 Hard-revokes

This ADR HARD-REVOKES heresy **HER-02** in writing: "The FLAIR live-desktop oracle `ppm_flair_check` grades BY CONSTRUCTION -- it computes expected RGBs from `flair_palette_rgb`, the same function `kmain` paints with" (REVOCATION-RECORD-2026-06-21, FLAIR Heresy Purge). The contradicted principle is P3 (grade against an INDEPENDENT golden, never by construction); the by-construction grade let the wrong `preview.webp` palette pass every gate and is the precise anti-oracle P3 was ratified against. The executing decree is CD-5 (strike the by-construction language, re-key onto `color_canon.h`, chain `test-color-canon` as a hard precondition, add the COLOR VALUE-mutant arm) together with CD-2 (the independent value oracle that grades the canon against the decomp corpora). The re30.3 GREEN ratification (WL-0052) that rested on the by-construction check is, by this revocation, no longer a valid acceptance signal for FLAIR color.

---

## 7. Related Decisions

| Document | Relationship |
|---|---|
| ADR-0004 -- FLAIR Toolbox Architecture | Parent. This ADR amends its D-8 oracle vector (Section 3.8 + Section 8.2 as AM-10) and supersedes its OD-4 / FO-3 / AM-9 / FO-F. |
| ADR-0004 Amendment DEC-09 -- Mechanism/Policy Split | AUTHORS the one canon module (`spec/assets/color_canon.json` -> `color_canon.h`, `flair_canon_rgb`, the C-8 cut line, the `flair_look_pixel` resolver, the `flair_skin_t` registry view, the WL-0053 bevel -> teal correction). This ADR CONSUMES it (CD-1) and keeps its orthogonal structure oracles (`test-mech-policy`, the colorblind sentinel render, `test-skin-teal`, `test-skin-era-frozen`). |
| ADR-0005 Amendment AM-1 -- Dual-Heritage Region Spine | Sibling. The region homomorphism property suite is legitimately golden-free and is not folded into chrome color canon. |
| ADR-0006 -- FLAIR Live Event Loop and Behavioural Grading | Consumer. Its bare-desktop teal assert (E-D5) reads idx2 `#8DDCDC` FROM this canon for the exposed-area check, downstream of this grading layer. |
| REVOCATION-RECORD-2026-06-21 -- FLAIR Heresy Purge | The management decree. This ADR is the EXECUTING document for the HER-02 hard-revoke; it also stands behind the seafoam-relapse tripwires for HER-01 / HER-06 / HER-08 and the `preview.webp`-not-a-source grep gate for HER-03. |
| PRD Sec 8, Sec 9, Sec 12 | The "every subsystem has a mechanical oracle; nothing ships on looks-right" mandate and the FLAIR-sprawl risk this oracle vector exists to contain. |

---

*End of ADR-0010. Controlled Document -- INITECH CONFIDENTIAL. Verify revision before use.*
