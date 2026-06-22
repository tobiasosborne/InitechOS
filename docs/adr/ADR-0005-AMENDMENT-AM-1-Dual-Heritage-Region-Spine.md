<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0005 Amendment AM-1 -- ATKINSON Dual-Heritage Region Spine

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0005-A1 |
| Title | ADR-0005 Amendment AM-1: ATKINSON Dual-Heritage Region Spine |
| Version | 1.0 |
| Status | **RATIFIED (ADR-by-committee + chief-architect reconciliation, operator-delegated authority, 2026-06-21; operator-ratified 2026-06-22)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme (Committee B -- Region Spine), reconciled by chief-architect synthesis |
| Effective Date | 2026-06-21 |
| Next Scheduled Review | 2026-12-21 (semi-annual, per RECORDS-POL-002) |
| Supersedes | (none; this Amendment appends to ADR-0005 v1.0 and does NOT re-issue it -- see Sec 6) |
| Superseded By | (none at time of ratification) |
| Related Documents | ADR-0005 (OEA-ADR-0005, ATKINSON Region Engine, RATIFIED 2026-06-19); ADR-0004 (OEA-ADR-0004, FLAIR Toolbox Architecture); ADR-0004 Amendment DEC-09 (OEA-ADR-0004-A1, Mechanism/Policy Split -- sibling, this reconciliation); ADR-0010 (FLAIR Grading and Goldens); ADR-0006 (FLAIR Live Event Loop and Behavioural Grading); REVOCATION-RECORD-2026-06-21 (FLAIR Heresy Purge); ADR-0001 (386+, 32-bit flat); ADR-0002 (implementation language C) |
| Related Issues | beads initech-jmo (region rep + normal-form constructor); initech-b5g (scanline merge); initech-6dy (homomorphism property suite); FO-D2-2 (this amendment -- file at ratification); FO-D2-3 (landing step 1, region amendment); FO-D2-5 (test-region-gdi + wine reference shim) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-06-21 | Architecture Review Board, STAPLER Programme (Committee B -- Region Spine) | Initial draft. Re-casts the ATKINSON engine purpose heritage-neutral; adds the GDI/HRGN peer wrapper family (CombineRgn/GetRgnBox/PtInRegion/RectInRegion); the heritage-symmetric three-column truth table; the RectInRgn containment->overlap deep-bug fix; the structural single-engine grep guard (binding constraint C-7); and test-region-gdi (L1/L2/L3) with the M-RECTIN/M-DISPATCH mutants. ADR-0005 v1.0 D-1..D-5 and the Sec 4 homomorphism oracle are recorded as UNCHANGED. | (pending committee review) |
| 1.0 | 2026-06-21 | Chief-Architect Reconciliation (synthesis) | Ratified following committee review and chief-architect cross-committee reconciliation (composes with sibling ADR-0004 Amendment DEC-09 and ADR-0010). Binding critic resolutions R1/R2/R3/R5/R7/R9 folded. Status set to RATIFIED, operator final confirmation pending. | ARB (full committee) + Chief-Architect Reconciliation |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Architecture Review Board (Committee B -- Region Spine) | Submitted | 2026-06-21 |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Concur-with-comment | 2026-06-21 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Concur-with-comment | 2026-06-21 |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Concur-with-comment | 2026-06-21 |
| Fidelity Steward -- Frame Conformance | (Fidelity Steward, FLAIR) | Concur (no region color golden introduced; see Sec 3, AM-2/R2) | 2026-06-21 |
| ARB Chair (Synthesis) | Chief-Architect Reconciliation (delegated per beads FO-D2-2) | Ratified | 2026-06-21 |
| Operator Ratification | T. Osborne (Operator) | Delegated to ARB + Chief-Architect per reconciliation; **final confirmation pending** | 2026-06-21 |
| Records Management | M. Waddams (Archive Annex B) | Filed | 2026-06-21 |

*Note on committee composition and authority: The Technical/Correctness, Period-Authenticity, and Governance/Compliance reviewers correspond to the in-programme engineering functions designated in ADR-0003 Sec 1.3 and carried forward through the FLAIR ADR series. The operator delegated ratification authority to the Architecture Review Board and the chief-architect reconciliation; this Amendment is ratified under that delegated authority with operator final confirmation pending. This Amendment APPENDS to ADR-0005 v1.0; it does not re-issue the v1.0 record, and the v1.0 homomorphism oracle is expressly untouched (Sec 3, AM-6).*

---

## 1. Purpose and Scope

### 1.1 Purpose

The purpose of this Amendment (the "Amendment" or "AM-1") is to re-cast the **ATKINSON region engine** (ADR-0005, OEA-ADR-0005, RATIFIED 2026-06-19) as the **single, heritage-neutral** scanline-inversion-list Boolean set engine that is the shared spine behind **both** the System-7 QuickDraw `Rgn` family and the Win-3.1 GDI `HRGN` family. ADR-0005 v1.0 fixed the engine as a "QuickDraw-style region algebra" (ADR-0005 Sec 1, line 61). InitechOS is a deliberate chimera (CLAUDE.md "What this is"): the desktop fuses a DOS personality with a System-7 Toolbox **and** carries Win-3.1/GDI chrome. A region engine framed as the property of a single heritage understates what it already is and what the chimera requires.

The operator-ratified principle (P2, recorded in the chief-architect reconciliation) is that the region engine is **one** engine behind **two** heritage facades, and **neither heritage owns it**. This Amendment makes that principle STRUCTURALLY true: it adds the GDI/HRGN peer wrapper family as a strict peer of the existing QuickDraw shims, makes the op-naming symmetry machine-checked, fixes a confirmed deep bug surfaced by the dual-heritage framing, binds a single-engine structural guard, and adds a dual-heritage conformance oracle -- all **without disturbing** the v1.0 correctness contract.

### 1.2 Scope

This Amendment governs:

- The **heritage-neutral re-framing** of the engine's purpose and identity in `spec/region_algebra.h` and ADR-0005 Sec 1.1/Sec 2.1 (Decision AM-1).
- The **GDI/HRGN peer wrapper family** -- `CombineRgn`, `GetRgnBox`, `PtInRegion`, `RectInRegion` -- and the GDI region-type return convention (Decision AM-2).
- The **heritage-symmetric three-column truth table** (internal `rgn_op_t` | QuickDraw name | GDI `CombineRgn` mode) and its build-time `_Static_assert` pins (Decision AM-3).
- The **RectInRgn containment->overlap deep-bug fix** (Decision AM-4).
- The **single-engine structural grep guard** as binding constraint C-7 (Decision AM-5).
- The explicit recording that the **Sec 4 homomorphism oracle is UNCHANGED** (Decision AM-6).
- The **`test-region-gdi`** conformance oracle (legs L1/L2/L3) and its `M-RECTIN`/`M-DISPATCH` mutants (Sec 7).

### 1.3 Out of Scope

The following are expressly out of scope of this Amendment:

- **Color, palette, and pattern policy.** Regions are pure coordinate-space pixel sets, depth- and color-agnostic (ADR-0005 Sec 1.3, line 79; ADR-0004 OD-2 indexed-8 affects imaging, not regions). This Amendment introduces **no color golden** into the region lane. The Initech-teal bevel value and all FLAIR color policy are owned by the sibling color-canon arbitration (ADR-0004 Amendment DEC-09 + `spec/assets/color_canon.json`) and graded under ADR-0010 (FLAIR Grading and Goldens), NOT here (R2, relayed).
- **The FLAIR mechanism/policy split (constraint C-8).** The cut-line that forbids a mechanism module from naming a color is the subject of the sibling ADR-0004 Amendment DEC-09 (Mechanism/Policy Split). The region-clip path is one of the mechanism modules subject to C-8 there; this Amendment records the dependency but does not re-author C-8.
- **The ATKINSON v1.0 correctness oracle.** ADR-0005 Sec 4 (the homomorphism property suite) and `harness/proptest/test_region.c`'s homomorphism core are NOT touched by this Amendment except for one additive property case (AM-4/AM-6). Weakening a sound oracle is a CLAUDE.md Stop condition.
- **The `RGBColor`-verb-signature direction for System-8 Platinum.** Reserved (P5), deferred; recorded as a forward path in the sibling DEC-09 amendment.

### 1.4 Additional Defined Terms

The following terms, when used herein, supplement the definitions in ADR-0005 Sec 1.4.

| Term | Definition |
|---|---|
| Heritage | One of the two period GUI lineages InitechOS fuses: System-7 QuickDraw, or Win-3.1 GDI. A *heritage facade* is the named, period-authentic wrapper surface a heritage's programmer expects. |
| Heritage-neutral | A property of the engine core: it carries no name, return convention, or argument order specific to a single heritage; both heritage facades dispatch to it. |
| `region_op` | The single internal four-argument Boolean set primitive `region_op(out, A, B, op)` (`os/flair/atkinson/region.c:403`). The lone engine; every region Boolean op of either heritage bottoms out here. |
| `HRGN` | The Win-3.1/GDI opaque region handle. In InitechOS the GDI facade operates on the same in-engine region representation; `HRGN` is the heritage-facing name, not a second representation. |
| `CombineRgn` | The GDI region-combine entry point: `CombineRgn(dst, src1, src2, mode)`, selecting the Boolean op by an integer `mode` code. |
| Region-type code | The GDI complexity return convention: `ERROR=0`, `NULLREGION=1`, `SIMPLEREGION=2`, `COMPLEXREGION=3`, classified off the normalized result. QuickDraw has no analogous return. |
| `gdi_ref_` | The mandatory namespace prefix under which the host-only foreign reference region engine (the wine banded-rect engine) is compiled for the conformance differential. Host-only; NEVER linked into any kernel or artifact line (Law 3). |
| Containment | The relation "every pixel of `rect` lies inside region `r`" (`region_rect_fully_in`). |
| Overlap | The relation "`rect` and region `r` share at least one pixel" (`region_rect_overlaps`). The documented semantic of `RectInRgn` (QuickDraw) AND `RectInRegion` (GDI). |

---

## 2. Context

### 2.1 What ADR-0005 v1.0 Established, and Why a Re-Framing -- Not a Rewrite -- Is Correct

ADR-0005 v1.0 ratified the ATKINSON engine as a clean-room per-scanline inversion-list Boolean set engine (D-1), its canonical normal form and five invariants (D-2), the four op truth tables and frame-relative complement (D-3), the no-`0x7FFF`-sentinel guardrail (D-4), the storage caps (D-5), and -- above all -- the homomorphism property suite as the **entire** correctness signal, with no external golden because QuickDraw's region body format is proprietary and unpublished (ADR-0005 Sec 2.2, Sec 4). All of that stands verbatim.

The dual-heritage re-cast is a **framing + missing-peer + missing-neutrality-oracle** correction, NOT an engine duplication, because the engine is **already** heritage-neutral at the core. Ground truth (Law 1):

- `os/flair/atkinson/region.c:403` is the **lone** `region_op` -- the single four-argument Boolean primitive. There is exactly one engine.
- `os/flair/atkinson/region.c:614-630` are **one-line shims** that give QuickDraw names (`UnionRgn`, `SectRgn`, `DiffRgn`, `XorRgn`) to `region_op` calls. The QuickDraw names are already a thin facade over the neutral primitive.

Only the **prose** is mono-heritage: ADR-0005 Sec 1 (line 61) calls the engine "the QuickDraw-style region algebra," and `spec/region_algebra.h` Sec 0/Sec 1 carry the same framing. Re-framing the prose and adding the missing GDI peer facade is therefore the smallest honest correction, with **zero** engine-regression risk: the locked, green homomorphism contract is not perturbed, and an amendment widens scope without re-issuing the v1.0 record. (This Amendment chips the heresies recorded as HER-13/HER-18 in REVOCATION-RECORD-2026-06-21 -- the mono-heritage region framing.)

### 2.2 Why the GDI Peer Is Required, and Why It Is a Facade, Not a Second Engine

InitechOS carries Win-3.1/GDI chrome on the chimera desktop (the Photoshop-style menu bar over System-7 windows is the spec, not a tell to correct -- CLAUDE.md "Hallucination-risk callouts"). A Win-3.1 programmer reaches for `CombineRgn(dst, src1, src2, mode)`, integer `RGN_*` mode codes, GDI argument order, and the `NULLREGION`/`SIMPLEREGION`/`COMPLEXREGION` complexity return. None of those exist on the QuickDraw facade. The GDI family is **pure policy** -- naming, argument order, integer mode codes, the complexity-return convention -- over the policy-free neutral mechanism. Building it as a strict **peer facade** (each entry a `<=5`-line shim over `region_op`) rather than as a parallel engine is what makes "neither heritage owns the engine" structurally true rather than a slogan.

### 2.3 The Wine Reference Engine and the Single-Engine Guard

The dual-heritage conformance differential (Sec 7, L2) diffs the ATKINSON GDI facade against the wine server's banded-rect region engine -- an **independent**, different-heritage representation, suitable as a non-by-construction golden (the engine cannot grade itself). A common but **overstated** assumption is that a second `region_op` would collide at link time and so guard itself. Ground truth refutes this:

- The wine engine's own static `region_op` (`server/region.c:208`) has a **different signature** -- a six-argument band-callback form `region_op(newReg, reg1, reg2, overlap_func, non1, non2)` -- versus FLAIR's four-argument `region_op(out, A, B, op)` (`region.c:403`).
- Different signatures are a compile error only within one translation unit; static internal linkage gives **no** collision across translation units.

Therefore a "duplicate engine = multiple-definition link error" backstop does **not** exist. The load-bearing single-engine guard is the **structural grep gate** (no second `region_op`/`xmerge`/band-sweep DEFINITION outside `region.c`), and `gdi_ref_` namespacing of the host-only wine reference is **mandatory**, not a safety net (R7; Decision AM-5; Sec 7 L3).

### 2.4 The RectInRgn Deep Bug Surfaced by the Dual-Heritage Framing

While reconciling the two heritage facades against their documented semantics, a confirmed Rule-3 deep bug was surfaced. `RectInRgn` is documented in **both** heritages as an **overlap** predicate:

- `regions-api.md:125` (QuickDraw): "any overlap."
- The wine `rect_in_region` reference: "at least partially inside."

FLAIR's `region.c:580-595` (`region_rect_in_region`) instead implements **containment** (`rect DIFF r == empty`). FLAIR is wrong for **both** heritages. This is not a heritage policy fork to be "decoded from the Win golden" -- that framing would freeze a bug as canon and is a Stop-condition violation. It is a single deep bug, fixed once for both facades (Decision AM-4).

---

## 3. The Decision (AM-1)

It is hereby recorded that the following Sub-Decisions AM-1 through AM-6 collectively constitute Amendment AM-1 to ADR-0005. Each is binding upon the implementation unless and until amended through the change-control process. For the avoidance of doubt, these Sub-Decisions widen and correct ADR-0005 v1.0; they do not supersede or re-issue it, and ADR-0005 Sec 4 (the homomorphism oracle) is unchanged save for one additive property case (AM-4/AM-6).

### 3.1 AM-1 -- Heritage-Neutral Re-Cast (Amend, Not Rewrite)

The engine PURPOSE is re-cast heritage-neutral: **ATKINSON is the SINGLE scanline-inversion-list Boolean set engine that is the shared spine behind BOTH the System-7 QuickDraw `Rgn` family AND the Win-3.1 GDI `HRGN` family; BOTH clip through the ONE engine; NEITHER owns it** (verbatim P2).

- ADR-0005 v1.0 Decisions D-1..D-5 and the Sec 4 homomorphism oracle stand **verbatim and UNCHANGED**.
- "QuickDraw-style region engine" is struck as the engine **IDENTITY** in `spec/region_algebra.h` Sec 0/Sec 1; QuickDraw becomes ONE of TWO heritage facades. `spec/region_algebra.h` Sec 7 is renamed **HERITAGE FACADES**.
- ADR-0005 Sec 1.1/Sec 2.1 are amended to the heritage-neutral framing with dual provenance (Inside Macintosh QuickDraw + Win-3.1 `wingdi.h`).

*Ground truth: `region.c:403` (lone `region_op`); `region.c:614-630` (QuickDraw 1-line shims); ADR-0005 status RATIFIED-AS-IS; `wingdi.h` dual provenance. The code is already neutral; only the prose is mono-heritage -- the smallest honest correction with zero engine-regression risk. Stop conditions forbid weakening a sound oracle, so this Amendment widens scope without disturbing the locked green contract.*

### 3.2 AM-2 -- The GDI/HRGN Peer Wrapper Family

Add `spec/region_algebra.h` Sec 7b **GDI HERITAGE FACADE** as a strict PEER of Sec 7 (the QuickDraw facade). The family is implemented as `<=5`-line shims over `region_op` plus the existing self-union-copy idiom (`region.c:520`) plus a region-type classifier, placed in `region.c` or a sibling `region_gdi.c` in the **same engine translation unit**. It introduces ZERO new region math and ZERO second engine.

The family:

| GDI entry | Signature | Dispatches to | Notes |
|---|---|---|---|
| `CombineRgn` | `CombineRgn(dst, src1, src2, mode)` | `region_op` selected by `mode` | Returns the GDI region-type code off the normalized result. |
| `GetRgnBox` | `GetRgnBox(rgn, out)` | bbox read + classifier | Returns the complexity code. |
| `PtInRegion` | `PtInRegion(rgn, x, y)` | point-in-region test | GDI argument order. |
| `RectInRegion` | `RectInRegion(rgn, rect)` | `region_rect_overlaps` | Semantics fixed to OVERLAP by AM-4 (NOT containment). |

The `CombineRgn` mode-to-op mapping (constants carried VERBATIM from `wingdi.h` with `_Static_assert`s):

| `mode` code | Value | `rgn_op_t` | Semantics |
|---|---|---|---|
| `RGN_AND` | 1 | INTERSECT | `src1` AND `src2` |
| `RGN_OR` | 2 | UNION | `src1` OR `src2` |
| `RGN_XOR` | 3 | XOR | `src1` XOR `src2` |
| `RGN_DIFF` | 4 | DIFF | `src1` AND NOT `src2` (identical argument order to `DiffRgn`) |
| `RGN_COPY` | 5 | (self-union identity copy) | copy of `src1`, normalized |

The GDI region-type code is derived from the normalized result: `is_empty -> NULLREGION (1)`; `is_rect -> SIMPLEREGION (2)`; else `COMPLEXREGION (3)`; `ERROR = 0`.

*Ground truth: `wingdi.h:1609-1621` (`ERROR`/`NULLREGION`/`SIMPLEREGION`/`COMPLEXREGION` = 0/1/2/3; `RGN_AND`=1/`OR`=2/`XOR`=3/`DIFF`=4/`COPY`=5); `region.c:520` (self-union-copy idiom); `region.c:614-630` (the QuickDraw shim pattern the GDI facade peers). The GDI family is pure policy over the policy-free mechanism; the region-type return is a GDI-shaped surface QuickDraw lacks.*

### 3.3 AM-3 -- The Heritage-Symmetric Three-Column Truth Table

Add a GDI column to ADR-0005 Sec 3.3 and to the `spec/region_algebra.h` Sec 3 `rgn_op` comments, yielding **THREE name columns**: internal `rgn_op_t` | QuickDraw name | GDI `CombineRgn` mode.

| Internal `rgn_op_t` | QuickDraw name | GDI `CombineRgn` mode |
|---|---|---|
| `RGN_OP_UNION` | `UnionRgn` | `RGN_OR` |
| `RGN_OP_INTERSECT` | `SectRgn` | `RGN_AND` |
| `RGN_OP_DIFF` | `DiffRgn` | `RGN_DIFF` |
| `RGN_OP_XOR` | `XorRgn` | `RGN_XOR` |

`_Static_assert`s pin **each** GDI mode constant AND its `rgn_op_t` mapping (`map(RGN_AND) == RGN_OP_INTERSECT`, etc.). A swapped GDI-mode-to-op wiring becomes a **compile error** -- the cheapest oracle in the vector.

Record that GDI's `XOR` = `(A SUB B) UNION (B SUB A)` is provably the **same set** as the one XOR primitive, citing the existing `region_algebra.h:359` static assert (`XOR == UNION minus INTERSECT` on the 4-bit truth table). This XOR-decomposition note is the key dual-heritage proof point: GDI reaches XOR a different syntactic way but the SAME set -- exactly "neither owns the engine."

The four `rgn_op_t` values and the four `boolfn` truth tables are **UNCHANGED**; the GDI column is pure annotation plus a machine-checked mapping table.

*Ground truth: `region_algebra.h:175-188` (the `rgn_op_t` + `RGN_TRUTH_*` macros), `region_algebra.h:359` (the XOR-decomposition static assert); `wingdi.h:1615-1619`. Internal truth tables stay frozen because they ARE the mechanism.*

### 3.4 AM-4 -- The RectInRgn Containment->Overlap Deep-Bug Fix

Fix the confirmed Rule-3 deep bug (NOT a heritage policy fork):

1. **RENAME** the existing full-containment primitive `region_rect_in_region` -> `region_rect_fully_in` (semantic: every pixel of `rect` in `r`; retained for window-clip contexts that genuinely want containment).
2. **ADD** `region_rect_overlaps(r, rect)` = bbox-reject then INTERSECT-non-empty (the documented semantic of BOTH heritages).
3. **RE-POINT** both heritage wrappers -- `RectInRgn(rect, rgn)` (QuickDraw) AND `RectInRegion(rgn, rect)` (GDI) -- to `region_rect_overlaps`.
4. **EXTEND** `harness/proptest/test_region.c` with an overlap property case (extends, never weakens -- the homomorphism core and its three mutants are untouched).
5. **AUDIT** the consumers BEFORE landing: the ONLY callers are `region.c` itself and `test_region.c:435` -- verified that **no** live `os/flair` `window.c`/`desktop.c`/`blitter.c` path calls it today, so the mechanism geometry is not silently perturbed. Coordinate with the sibling A mechanism geometry and file the audit as a precondition regardless (R7; see Sec 5 C-9, and the sibling DEC-09 amendment's FO-D2-4).

*Ground truth (Law 1): TWO independent sources document OVERLAP -- `regions-api.md:125` ("any overlap") and the wine `rect_in_region` reference ("at least partially inside") -- while FLAIR `region.c:580-595` implements CONTAINMENT (`rect DIFF r == empty`). Both heritages agree; FLAIR is wrong for BOTH. The dual-heritage framing is what surfaced it. Ref: wine `server/region.c:874-888`. The REJECTED framing of this as "wrapper policy decoded from the Win golden" would freeze a bug as canon (Stop condition).*

### 3.5 AM-5 -- Single-Engine Structural Guard (C-7)

Bind ADR-0005 constraint **C-7** (after C-6 in ADR-0005 Sec 5.1): **one engine, two peer facades, no second region engine; every region Boolean op -- QuickDraw OR GDI -- bottoms out in `region.c` `region_op`.**

The load-bearing guard is a **STRUCTURAL grep gate**: FAIL loud if any `region_op` (or `xmerge` / band-sweep) DEFINITION appears outside `os/flair/atkinson/region.c`. Any host-only foreign reference engine used by a conformance oracle (the wine band engine) MUST be compiled under a `gdi_ref_` namespace and NEVER linked into any kernel or artifact line (Law 3). The implementation MUST NOT rely on a link-error backstop.

*Ground truth: the wine static `region_op` (`server/region.c:208`) has a different six-argument band-callback signature versus FLAIR's four-argument `region_op` (`region.c:403`); a same-name link does NOT auto-collide (different signatures error only in one TU; static internal linkage gives no cross-TU collision). The grep gate is the actual enforcement; `gdi_ref_` namespacing is mandatory, not a safety net (R7).*

### 3.6 AM-6 -- The Homomorphism Oracle Is Untouched

Do NOT touch ADR-0005 Sec 4 or `harness/proptest/test_region.c`'s homomorphism core: `region_op` / `region_normalize` / `region_assert_normal`, the four truth-table macros, and the three existing mutants (`RGN_MUTATE_NO_VRLE`, `RGN_MUTATE_PARITY_OFF1`, `RGN_MUTATE_EMIT_NOCHANGE`) are UNCHANGED. The ONLY additive touch is the AM-4 overlap property case (extends, never weakens). The dual-heritage conformance oracle (`test-region-gdi`, Sec 7) is a SEPARATE neutrality/heritage-fidelity signal, kept orthogonal to the correctness signal.

*Ground truth (Charter + Law 2 + Stop condition): the homomorphism suite is sound and is the complete correctness spec -- `region-body-closure.md`: "property-test only, no golden possible or wanted" (the QuickDraw region body is proprietary/unpublished). Freezing the proven oracle guarantees `make test-region` stays green at every bisect step. Ref: ADR-0005 Sec 4; `test_region.c` the three mutants.*

---

## 4. Rationale

### 4.1 Why Amend Rather Than Re-Issue (AM-1)

ADR-0005 v1.0 is RATIFIED. The engine is already neutral in code (`region.c:403`; `region.c:614-630`); the defect is purely in the framing prose. An amendment is the minimal, honest instrument: it re-states the engine's identity to match what the chimera requires and what the code already is, while leaving the locked green correctness contract entirely intact. Re-issuing v1.0 would invite an unnecessary re-litigation of the homomorphism oracle, which the Stop conditions protect.

### 4.2 Why a Peer Facade, Not a Parallel Engine (AM-2)

A second region engine would be the single largest correctness hazard in the Toolbox -- two implementations of the load-bearing math that can drift. A peer facade of `<=5`-line shims over the one `region_op` makes the dual-heritage claim true *by construction of the source*, while the structural guard (AM-5) and the conformance differential (L2) prove no drift exists. The GDI region-type return convention is the only genuinely new surface, and it is a pure classification of the normalized result -- no new set algebra.

### 4.3 Why the Truth Table Is Machine-Checked (AM-3)

A swapped GDI-mode-to-op wiring (e.g. `RGN_OR -> INTERSECT`) is the most likely facade defect and the hardest to spot by eye. Pinning every mode constant and its `rgn_op_t` mapping with `_Static_assert`s converts that class of defect into a compile error -- the cheapest possible oracle, with no emulator and no golden. The XOR-decomposition note (citing `region_algebra.h:359`) is the explicit dual-heritage proof point: two heritages reach the same set by different syntax.

### 4.4 Why RectInRgn Is a Bug, Not a Policy (AM-4)

Two independent period references document `RectInRgn`/`RectInRegion` as an overlap predicate, and they agree across heritages. FLAIR's containment implementation is therefore wrong for both. Treating the discrepancy as "heritage policy decoded from a golden" would canonize the defect and force either a permanent false deviation or a loosened oracle -- both Stop-condition violations. The fix is a single deep-bug correction shared by both facades, with the containment primitive retained (renamed) for the contexts that legitimately want it.

### 4.5 Why the Structural Grep Is Load-Bearing, Not the Link Error (AM-5)

The conventional assumption -- that a duplicate engine self-guards via a multiple-definition link error -- is false here, because the wine reference engine's `region_op` has a different signature and static linkage. The actual enforcement must be a source-structural property (one `region_op` definition, in `region.c`, full stop), and the foreign reference must be namespaced (`gdi_ref_`) and host-only so it can never reach a shipped line. Documenting this honestly prevents a future agent from trusting a backstop that does not exist.

### 4.6 Why No Region Color Golden (R2, relayed)

The Initech-teal bevel value (`#8DDCDC` highlight, `#4E9BA3` shadow per `spec/assets/color_canon.json`) and all FLAIR color policy are graded under the authored-value posture owned by the sibling color-canon arbitration (ADR-0004 Amendment DEC-09) and the grading architecture (ADR-0010). Regions are coordinate-only. Introducing a color golden into the region lane would be both a layering violation and a redundant, drift-prone second authority. This Amendment adds none.

### 4.7 Period Authenticity

The Period-Authenticity reviewer confirmed, without required change, that the GDI `CombineRgn` mode codes (`RGN_AND`=1 / `OR`=2 / `XOR`=3 / `DIFF`=4 / `COPY`=5), the region-type return convention (`NULLREGION`/`SIMPLEREGION`/`COMPLEXREGION` = 1/2/3), the GDI argument orders for `PtInRegion`/`RectInRegion`, and the QuickDraw `UnionRgn`/`SectRgn`/`DiffRgn`/`XorRgn` names all match the period references (`wingdi.h`; Inside Macintosh QuickDraw). The dual-heritage facade introduces no anachronism: it names existing period surfaces over the existing neutral engine.

---

## 5. Consequences

### 5.1 Binding Constraints

**C-7 -- One engine, two peer facades; the structural grep gate is the guard.** Every region Boolean op of either heritage MUST bottom out in `region.c` `region_op`. No second `region_op` / `xmerge` / band-sweep DEFINITION may appear outside `os/flair/atkinson/region.c`; the `test-region-gdi` L3 grep gate enforces this and MUST fail loud on a violation. Any host-only foreign reference engine (the wine band engine) MUST be compiled under the `gdi_ref_` namespace and MUST NEVER be linked into any kernel or artifact line (Law 3). The link-error backstop is explicitly NOT relied upon. This constraint is added to ADR-0005 Sec 5.1 after C-6 and changes only by a deliberate further ADR amendment.

**C-8 (cross-reference, owned by the sibling DEC-09 amendment).** The region-clip path is one of the FLAIR mechanism modules subject to the mechanism/policy cut-line C-8 (ADR-0004 Amendment DEC-09 Sec 5.1): it MAY name a palette INDEX and resolve it ONLY through `flair_look_pixel(port, PART)`; it ships ZERO `0xRRGGBB` literal, ZERO `INITECH_*_RGB`, ZERO index-to-RGB switch. This Amendment records the dependency; C-8 is authored and bound in the sibling amendment, not here.

**C-9 -- RectInRgn/RectInRegion semantics are OVERLAP, and the containment primitive is renamed.** `RectInRgn` (QuickDraw) and `RectInRegion` (GDI) MUST evaluate `region_rect_overlaps`. The full-containment primitive is `region_rect_fully_in` (renamed from `region_rect_in_region`) and is reserved for contexts that genuinely require containment. Any change to either semantic requires a further amendment and a re-run of `test-region` (overlap property case) and `test-region-gdi` (the `M-RECTIN` mutant).

### 5.2 Forward Obligations

**FO-D2-2 -- File and author this Amendment.** File ONE beads issue for ADR-0005 Amendment AM-1 (a deliberate Rule-8 act). Author this record (APPEND; do NOT re-issue ADR-0005 v1.0): the heritage-neutral re-frame, the GDI peer facade (new ADR-0005 Sec 3.6), the GDI truth-table column (ADR-0005 Sec 3.3), the RectInRgn deep-bug fix (recorded as resolved), binding constraint C-7, and `test-region-gdi` as a binding forward obligation. Mark resolves HER-13 / HER-18 (REVOCATION-RECORD-2026-06-21).

**FO-D2-3 -- LANDING STEP 1 (color-independent, FIRST per R9).** Land the region amendment: re-frame `spec/region_algebra.h` Sec 0/Sec 1/Sec 7 heritage-neutral; add Sec 7b GDI wrapper declarations + `_Static_assert`s; implement the GDI facade shims (`<=5`-line dispatch over `region_op` + self-union-copy + a region-type classifier) in `region.c` or a sibling `region_gdi.c` in the same engine TU; fix `RectInRgn` (split `region_rect_fully_in` vs `region_rect_overlaps`, re-point both wrappers to overlap); extend `test_region.c` with the overlap property case (homomorphism core + three mutants UNCHANGED). Confirm `make test-region` stays GREEN and the booting `BOOT_FLAIR_SHELL` static frame still renders (no current render path calls `CombineRgn`, so the new family cannot perturb the shell). This region amendment lands **before** the sibling canon arbitration and the mechanism switch collapse (R9 sequencing).

**FO-D2-4 -- AUDIT before landing AM-4 (R7 coordination).** Confirm the ONLY callers of `region_rect_in_region` / `RectInRgn` are `region.c` and `test_region.c:435` (verified now); re-confirm at land time and coordinate the rename / semantic change with the sibling A mechanism geometry (`window.c`/`desktop.c`) before the overlap re-point lands. File the audit as a precondition even though the live render path is currently unaffected.

**FO-D2-5 -- Implement `test-region-gdi` and the wine reference shim.** Implement `harness/proptest/test_region_gdi.c` (L1 wrapper-equivalence, L2 wine differential under `gdi_ref_`, L3 single-engine grep guard) plus the wine-engine host-only extraction shim header (`struct rectangle` + `mem_alloc -> malloc` + `set_error -> stub`; NEVER linked into any kernel/artifact line, Law 3). Add `Makefile` `WIN31_DECOMP ?= ../win31-decomp` (the FIRST win31 Makefile wiring -- chips HER-09; coordinate so this single line is added ONCE across all committees) plus targets `test-region-gdi` and `test-region-gdi-mutant`; wire into `make test` as a SEPARATE gate and into the FLAIR Manager real gate, leaving `test-region` UNTOUCHED. Mutation-prove EACH mutant (`M-DISPATCH` / `M-VALUE` / `M-RECTIN` red->green; `RGN_MUTATE_PARITY_OFF1` cross-redden) and confirm loud-skip fires when `../win31-decomp` is absent AND when wine extraction fails.

**FO-D2-7 -- Bind C-7 and wire the gate vector.** Add C-7 to ADR-0005 Sec 5.1; add `test-region-gdi` to the `make test` gate vector and to the FLAIR D-8 oracle table as a hard pass/fail row; FREEZE the single-spine boundary so C-7 changes only by a deliberate ADR amendment.

**FO-D2-8 -- Exercise the GDI facade on metal (cross-deliverable seam; NOT a blocker).** The GDI peer family is verified-but-unused (nothing in the artifact calls `CombineRgn` today). For a full dual-heritage GUI the Win/Photoshop menu-bar clip should later route through `CombineRgn` so the peer facade is exercised on metal, not only in the oracle. File a follow-up; do not block this Amendment on it.

### 5.3 Neutral Consequences

- ADR-0005 v1.0 D-1..D-5 and Sec 4 (the homomorphism oracle) are unchanged. AM-1 widens the engine's scope and adds the GDI peer facade beneath the same single `region_op`; it does not alter the representation, the normal form, the truth tables, or the correctness signal.
- Regions remain coordinate-only and color-agnostic; this Amendment introduces no color golden into the region lane (ADR-0005 Sec 1.3, line 79; Sec 3.3 above, R2).
- The `make test-region` gate is unaffected (the overlap property case extends it); `test-region-gdi` is a new, separate gate.

---

## 6. Relationship to ADR-0005 v1.0 and to the Sibling Reconciliation

**AM-1 appends to ADR-0005 v1.0; it does not supersede or re-issue it.** ADR-0005 v1.0 fixed the engine representation, normal form, truth tables, guardrails, storage caps, and the homomorphism oracle. AM-1 re-casts the engine's identity heritage-neutral, adds the GDI peer facade, makes the op-naming symmetry machine-checked, fixes the RectInRgn deep bug, and binds the single-engine guard -- all beneath the same single `region_op` and without disturbing the v1.0 correctness contract.

This Amendment is one of two ADR amendments forming the FLAIR "spine" deliverable of the 2026-06-21 chief-architect reconciliation. Its sibling, **ADR-0004 Amendment DEC-09 (Mechanism/Policy Split)**, binds the mechanism/policy cut-line (constraint C-8) and folds the color-policy quadruple-ownership into the **one** arbitrated color-canon module (`spec/assets/color_canon.json` -> generated `spec/assets/color_canon.h`; accessor `flair_canon_rgb(uint8_t idx)`; resolver `flair_look_pixel(port, PART)`; registry view `flair_skin_t`). The region-clip path is a mechanism module under that C-8 cut-line. The two amendments are sequenced (R9): **this region amendment lands first** (color-independent), then the canon arbitration and value oracle, then the mechanism switch collapse and color flip.

---

## 7. Verification

The correctness signal for the engine remains the ADR-0005 Sec 4 homomorphism property suite (`make test-region`), which is UNTOUCHED (AM-6). This Amendment adds a SEPARATE **dual-heritage conformance oracle**, `test-region-gdi`, which grades neutrality and heritage fidelity -- orthogonal to the correctness signal. No oracle in this Amendment is valid by construction; the by-construction leg (L1) is explicitly labeled necessary-but-not-sufficient and is paired with an independent golden (L2) and a structural gate (L3).

### 7.1 test-region-gdi L1 -- Cross-Family Equality (necessary, NOT sufficient)

Over the homomorphism suite's seeded-LCG region population, assert `region_equal(CombineRgn(A, B, RGN_AND), SectRgn(A, B))` and the `OR` / `DIFF` / `XOR` / `COPY` analogues for thousands of pairs -- proving both wrapper families bottom out in the IDENTICAL `region_op`. This leg is **labeled by-construction** (both wrappers call the same engine), so it CANNOT catch a wrong value; it establishes only that the two facades agree. The `M-VALUE` mutant (Sec 7.4) operationalizes the distinction between agreement and correctness.

### 7.2 test-region-gdi L2 -- The Independent Golden (P3, not by-construction)

A host-only differential of the ATKINSON GDI facade -- `CombineRgn` (all four modes + `COPY`), `GetRgnBox` extents and complexity code, `PtInRegion`, and `RectInRegion` (the AM-4 OVERLAP semantic) -- against the `gdi_ref_`-namespaced wine `server/region.c` banded-rect reference engine (`union_region` / `intersect_region` / `subtract_region` / `xor_region` / `point_in_region` / `rect_in_region` / `get_region_extents`), bit-exact on rasterized pixel sets over a bounded grid, for thousands of seeded-LCG region pairs. The golden is **independent**: the wine engine is a banded-rect representation from a different heritage, NOT computed from ATKINSON's source. L2 LOUD-SKIPS (banner, continue to the homomorphism floor) if `../win31-decomp` is absent or the wine extraction fails; it NEVER silent-passes.

### 7.3 test-region-gdi L3 -- The Single-Engine Structural Gate (load-bearing per R7)

A grep gate that FAILS LOUD if any `region_op` / `xmerge` / band-sweep DEFINITION appears outside `os/flair/atkinson/region.c`; the wine engine's own static `region_op` (`server/region.c:208`) stays `gdi_ref_`-namespaced and TU-isolated. This is the actual single-spine enforcement (the link-error backstop is NOT relied upon -- signatures differ; static linkage gives no collision). Mutation-proven by introducing a stub duplicate `region_op` and confirming the grep gate fails.

### 7.4 Mutation Proofs (Rule 6 -- each must drive RED)

| Mutant | Perturbation | Required result |
|---|---|---|
| `M-VALUE` | One-branch `region_op` perturbation (off-by-one span emit) | L2 RED while L1 stays GREEN -- the explicit demonstration that the INDEPENDENT golden, not self-consistency, catches a wrong value (the `test-clut` `0x33 -> 0x34` analogue). |
| `RGN_MUTATE_PARITY_OFF1` (existing engine mutant) | Off-by-one parity in span emit | Must ALSO redden L2 (both wrapper families ride the one engine). |
| `M-RECTIN` | `RectInRegion` wired to CONTAINMENT (the pre-AM-4 bug) instead of OVERLAP | The `RectInRegion`-vs-wine-`rect_in_region` row goes RED -- mutation-proves the deep-bug fix and that the overlap value is graded against an EXTERNAL authority (wine `rect_in_region` "at least partially inside"; `regions-api.md:125` "any overlap"), NOT by-construction. |
| `M-DISPATCH` | `CombineRgn` maps a mode to the wrong `rgn_op` (e.g. `RGN_OR -> INTERSECT`) | BOTH L1 (cross-family equality) AND L2 (wine differential) go RED -- proves the mode-to-op SEMANTICS are graded, not just plumbing. |

### 7.5 Build-Time GDI-Constant Oracle (AM-2/AM-3)

`_Static_assert`s in `region_algebra.h` Sec 7b + Sec 3 pin `RGN_AND`=1 / `OR`=2 / `XOR`=3 / `DIFF`=4 / `COPY`=5 and `ERROR`=0 / `NULLREGION`=1 / `SIMPLEREGION`=2 / `COMPLEXREGION`=3 to `wingdi.h:1609-1621`, AND pin each GDI mode to its `rgn_op_t` (`map(RGN_AND) == RGN_OP_INTERSECT`, etc.). A swapped wiring or typo'd constant is a COMPILE error. Mutation-proven by deliberately mis-mapping one mode and confirming the build fails.

### 7.6 The Homomorphism Suite (ADR-0005 Sec 4) -- Unchanged

`make test-region` (the Sec 4 homomorphism property suite, normalize-idempotence, structural-consistency, and the three engine mutants `RGN_MUTATE_NO_VRLE` / `RGN_MUTATE_PARITY_OFF1` / `RGN_MUTATE_EMIT_NOCHANGE`) is the complete correctness spec and is UNTOUCHED save for the additive AM-4 overlap property case. It MUST stay green at every bisect step of the landing sequence (AM-6).

### 7.7 Locked Spec Artifacts Affected

| Artifact | Disposition |
|---|---|
| `spec/region_algebra.h` | AMEND: Sec 0 banner + Sec 1 wording heritage-neutral with dual provenance (Inside Macintosh QuickDraw + Win-3.1 `wingdi.h`); rename Sec 7 -> HERITAGE FACADES; NEW Sec 7b GDI/HRGN wrapper declarations (`CombineRgn` + `RGN_*` mode macros, `GetRgnBox`, `PtInRegion`, `RectInRegion`, region-type codes) with `_Static_assert`s pinning every constant and the mode-to-op mapping to `wingdi.h`; Sec 3 GDI naming column; RectInRgn overlap-semantic note. ASCII-clean per Rule 12. |
| `os/flair/atkinson/region.c` (or sibling `region_gdi.c`, same TU) | The GDI facade shims; the `region_rect_fully_in` / `region_rect_overlaps` split; both wrappers re-pointed to overlap. The single `region_op` (`region.c:403`) unchanged. |
| `harness/proptest/test_region.c` | ADD the overlap property case (extends, never weakens); homomorphism core + three mutants UNCHANGED. |
| `harness/proptest/test_region_gdi.c` (NEW) | L1/L2/L3 + the wine host-only extraction shim under `gdi_ref_`. |
| `Makefile` | `WIN31_DECOMP ?= ../win31-decomp` (first win31 wiring; single shared edit) + `test-region-gdi` / `test-region-gdi-mutant` targets. |

---

## 8. Related Decisions and References

- **ADR-0005 (OEA-ADR-0005) -- ATKINSON Region Engine** (RATIFIED 2026-06-19): the v1.0 record this Amendment appends to. D-1..D-5 and Sec 4 (the homomorphism oracle) are UNCHANGED.
- **ADR-0004 Amendment DEC-09 -- Mechanism/Policy Split** (sibling, this reconciliation): binds constraint C-8 (mechanism names palette INDICES via `flair_look_pixel`, ZERO color literal/switch below the cut-line) and folds the color-policy quadruple-ownership into the ONE arbitrated canon module (`spec/assets/color_canon.json` -> `color_canon.h`; `flair_canon_rgb` accessor; `flair_look_pixel` resolver; `flair_skin_t` registry view). The region-clip path is a mechanism module under C-8. Records the seafoam->Initech-teal `#8DDCDC` supersession (OD-4 revoked).
- **ADR-0004 (OEA-ADR-0004) -- FLAIR Toolbox Architecture** (RATIFIED 2026-06-19): the umbrella; the region engine is layer 2 (Sec 3.1), the damage model is `DiffRgn` (Sec 3.5).
- **ADR-0010 -- FLAIR Grading and Goldens** (this reconciliation): the grading architecture and the ONE value oracle `test-color-canon`. Owns the color-value goldens; this Amendment introduces NONE into the region lane (R2).
- **ADR-0006 -- FLAIR Live Event Loop and Behavioural Grading** (this reconciliation): the booted-OS cooperative `WaitNextEvent` loop and the behavioural oracles. Independent of this Amendment.
- **REVOCATION-RECORD-2026-06-21 -- FLAIR Heresy Purge** (this reconciliation): the management decree. This Amendment EXECUTES the AMEND disposition of HER-13 / HER-18 (the mono-heritage region framing).
- **ADR-0001** (386+, 32-bit flat); **ADR-0002** (implementation language C; no second factory language -- Law 3, governing the `gdi_ref_` host-only namespacing).
- **PRD Sec 6.2** (the region engine spec + homomorphism oracle), Sec 6.3 (the Toolbox that consumes it), Sec 9 (region-algebra anchor), Sec 12 (Toolbox-sprawl risk).
- **CLAUDE.md** Laws 1-4; Rules 2, 3, 6, 8, 11, 12; "Hallucination-risk callouts" (regions must be normalized before every op; the chimera is the spec, not a tell to correct); Stop conditions (never weaken a sound oracle; never freeze a bug as canon).

---

*END OF DOCUMENT -- OEA-ADR-0005-A1, Rev 1.0. RATIFIED (ADR-by-committee + chief-architect reconciliation, operator-delegated authority, 2026-06-21; operator-ratified 2026-06-22). Controlled copy held by Records Management, Archive Annex B.*
