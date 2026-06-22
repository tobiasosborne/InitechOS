<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0005 — ATKINSON Region Engine (the load-bearing GUI math)

**Issuing Body:** Initech Systems Corporation — Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0005 |
| Title | ADR-0005: ATKINSON Region Engine |
| Version | 1.0 (Ratified) |
| Status | **RATIFIED (ADR-by-committee, operator-delegated authority, 2026-06-19)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme |
| Effective Date | 2026-06-19 |
| Next Scheduled Review | Upon operator ratification, per RECORDS-POL-002 |
| Supersedes | (none) |
| Superseded By | (none) |
| Related Documents | ADR-0004 (FLAIR Toolbox Architecture — umbrella, RATIFIED 2026-06-19); ADR-0001 (386+, 32-bit flat); ADR-0002 (impl language C); ADR-0003 (InitechDOS base OS) |
| Related Issues | beads initech-jmo (region rep + normal-form constructor); initech-b5g (scanline merge: union -> sect/diff/xor/complement); initech-6dy (C property suite: homomorphism + identities + shrinker, mutation-proven) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | (draft) | Architecture Review Board, STAPLER Programme | Initial draft. Records the clean-room inversion-list decision (DEC-04a-R1), the per-scanline normal form and its five invariants, the four op truth tables + frame-relative complement, the no-0x7FFF guardrail, the storage caps, and the homomorphism property suite as the ENTIRE correctness signal (no external golden — QuickDraw's region body is proprietary/unpublished). `spec/region_algebra.h` is the locked artifact, authored first. | (pending committee review) |
| 1.0 | 2026-06-19 | Architecture Review Board, STAPLER Programme | Ratified AS-IS, no amendments (DEC-02). Chair verified `make test-region` green (31 checks, 0 failures) and the three named mutants (RGN_MUTATE_NO_VRLE / RGN_MUTATE_PARITY_OFF1 / RGN_MUTATE_EMIT_NOCHANGE) satisfy Rule 6. Locks the already-implemented engine + `spec/region_algebra.h` as the binding contract. Ratified by ADR-by-committee (wf_573c1cf5-537), operator-delegated authority, no gridlock. | ARB (Bolton/Nagheenanajar/Smykowski + Fidelity Steward) + Chair |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Architecture Review Board, STAPLER Programme | Submitted (DRAFT) | (draft) |
| ARB Reviewer — Technical Correctness | M. Bolton (Senior Engineer, Platform) | Approved (2026-06-19) | 2026-06-19 |
| ARB Reviewer — Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Approved (2026-06-19) | 2026-06-19 |
| ARB Reviewer — Governance & Compliance | T. Smykowski (QA / Change Advisory) | Approved (2026-06-19) | 2026-06-19 |
| ARB Reviewer — Fidelity Steward | Fidelity Steward (Heritage Conformance) | Approved (2026-06-19) | 2026-06-19 |
| ARB Chair (Synthesis) | ADR-by-committee Chair | Approved (2026-06-19) | 2026-06-19 |
| Operator Ratification | T. Osborne (Operator) | **Granted via delegated committee authority (2026-06-19)** | 2026-06-19 |
| Records Management | M. Waddams (Archive Annex B) | Filed (2026-06-19) | 2026-06-19 |

*Note on status: RATIFIED (ADR-by-committee, operator-delegated authority, workflow wf_573c1cf5-537, 2026-06-19; no gridlock). Ratified AS-IS, no amendments (DEC-02 -- unanimous). The clean-room design herein (DEC-04a-R1) and the locked `spec/region_algebra.h` are the binding contract per Rule 8; the chair verified `make test-region` green (31 checks, 0 failures) and the three named mutants satisfy Rule 6. The committee-ratified clean-room region design referenced in the commissioning session is hereby formally ratified.*

---

## 1. Purpose and Scope

### 1.1 Purpose

This ADR fixes the design of the **ATKINSON region engine** — the QuickDraw-style region algebra that is, in the words of the PRD (§6.2), *"the load-bearing math of the whole Toolbox."* Clipping, window overlap, `visRgn`/`clipRgn`, and the update/damage model (ADR-0004 §3.5) all reduce to operations on regions. A region engine that is subtly wrong corrupts the entire Toolbox (CLAUDE.md "Tool of last resort"); therefore its design, its normal form, and — above all — its **correctness oracle** are fixed here and locked as data in `spec/region_algebra.h`.

### 1.2 Scope

This ADR governs:

- The **clean-room inversion-list representation** (DEC-04a-R1) and why it is clean-room, not a QuickDraw port. (§3.1)
- The **per-scanline normal form** and its **five invariants**. (§3.2)
- The **four op truth tables** (`UNION`/`INTERSECT`/`DIFF`/`XOR`) and **frame-relative complement**. (§3.3)
- The **no-0x7FFF in-band-sentinel guardrail**. (§3.4)
- The **storage caps** and the a-priori merge bound. (§3.5)
- The **homomorphism property suite** as the **entire** correctness signal — no external golden. (§4)
- `spec/region_algebra.h` as the **locked artifact, authored first**. (§5)

### 1.3 Out of Scope

- The **higher Toolbox layers** that consume regions (Window/Menu/Control/Dialog Managers, GrafPort, surface module) — governed by the umbrella **ADR-0004**.
- The **engine .c implementation** (`os/flair/atkinson/`) — this ADR fixes the contract and the oracle; the implementation is a downstream deliverable (beads initech-jmo/b5g/6dy) built RED->GREEN against the suite this ADR mandates.
- **Pixel depth, palette, and blitting** — regions are pure coordinate-space pixel sets, depth-agnostic (ADR-0004 OD-2 indexed-8 affects imaging, not regions).

### 1.4 Additional Defined Terms

| Term | Definition |
|---|---|
| Region | A pixel set R subset of Z^2 (PRD §6.2), represented per scanline by a sorted list of inversion points. |
| Inversion point | An x-coordinate at which scanline membership toggles (in/out). A normalized run-length encoding of the scanline. |
| Inversion list (x-list) | The sorted, strictly-increasing, even-length list of inversion points for one scanline; pairs are half-open "inside" spans [x0,x1), [x2,x3), … |
| Normal form | The canonical, minimal representation: minimal inversion-point set, vertical-RLE-collapsed, sorted, sentinel-free. Two equal regions have byte-identical normal forms. |
| Homomorphism property | rasterize(A OP B) == rasterize(A) OP_set rasterize(B), bit-exact, for every op — rasterization is a Boolean-algebra homomorphism from the region rep to the powerset of pixels over the bounding box. |
| Frame | An explicit bounding rectangle relative to which complement is defined (there is no infinite universe). |
| x_pool | The single contiguous int16 backing store holding every row's inversion list (arena/static-backed; no per-row malloc). |

---

## 2. Context

### 2.1 Why Regions Are the Foundation

The entire FLAIR Toolbox stands on region algebra (ADR-0004 §3.1, layer 2). The damage model (ADR-0004 §3.5) is `DiffRgn`; clipping is the intersection of `visRgn` and `clipRgn`; window overlap is a sequence of region ops. The PRD names this *the* sprawl risk's anchor (§9, §12): with the region-algebra anchor and a decisive oracle, the Toolbox is tractable; without it, it degenerates into plausible nonsense. The region engine also has the singular virtue of **zero emulator dependency** — it is pure host-testable math — so it can be built and proven correct immediately, ahead of and independent of the boot/graphics pipeline (the operator's parallel-track decision, ADR-0004 OD-1).

### 2.2 No External Golden Is Possible

QuickDraw's region **body format** (the bytes after the 10-byte `rgnSize`/`rgnBBox` header) is **proprietary and unpublished**. There is no period reference whose region bytes we could differentially diff against (unlike FAT vs `mtools`, or `.dbf` vs real dBASE, or codegen vs Free Pascal — the project's other oracles, which all have a real external reference). Therefore the region engine's correctness signal **cannot** be a differential against a golden; it **must** be an intrinsic property test against the pixel ground truth. This is not a weakness — the homomorphism property (§4) is a complete, decisive, brute-forceable specification of correctness (PRD §6.2: *"this brute-force differential against the pixel ground truth is the spine; it needs no proof assistant to be decisive"*).

---

## 3. The Decision (DEC-04a-R1)

### 3.1 D-1 — Clean-Room Per-Scanline Inversion-List Representation

A region is represented per scanline by a sorted list of inversion points (PRD §6.2). The concrete rep (locked in `spec/region_algebra.h`):

- `rgn_rect_t { int16 top, left, bottom, right; }` — QuickDraw Rect **field order**, int16 (period-authentic; framebuffer-bounded to 640x480, ADR-0004 OD-3).
- `rgn_row_t { int16 y_top; uint16 x_count; int16 *x; }` — one scanline run; `x` is **strictly increasing, EVEN count**, valid for scanlines [y_top, next.y_top).
- `region_t { rgn_rect_t bbox; uint16 n_rows, cap_rows; rgn_row_t *rows; int16 *x_pool; uint32 x_pool_used, x_pool_cap; uint8 is_rect, is_empty; }` — `bbox` mirrors QuickDraw `rgnBBox`; `is_rect`/`is_empty` are the fast-path flags and the `rgnSize==10` rectangular-special-case analogue; **all** x-data lives in one contiguous `x_pool` (arena/static-backed, deterministic layout, **no per-row malloc**, Rule 11).

This is a **clean-room** design: only the public op **names** and the Rect **field order** are borrowed from Inside Macintosh; the internal byte format is ours (it has to be — the QuickDraw body is unpublished, §2.2). Stating "clean-room" is both an IP-provenance record (PRD §12 risk register) and a correctness statement: we owe nothing to a format we cannot see, so the format is designed for our oracle.

### 3.2 D-2 — The Per-Scanline Normal Form and Its Five Invariants

`region_normalize` establishes a **canonical** normal form, and `region_assert_normal` checks it at the **top of every op** (panic in-kernel / CHECK hosted — Rule 2; a merge fed non-normal input produces garbage that passes non-overlapping cases, exactly the deep bug, Rule 3). The five invariants (locked verbatim in the header as numbered comments):

1. **Strictly increasing** — within every row, x[0] < x[1] < … (no duplicate/out-of-order toggles).
2. **Even length** — every row's x_count is even (each inside-span is a [start,end) pair; an odd count would run a span to infinity, impossible in a bounded region).
3. **Vertical-RLE collapse** — no two consecutive rows share an identical x-list (a duplicate next row is redundant and dropped).
4. **No empty interior row** — only the final closing/sentinel row may have x_count==0 (it bounds the last live band from below at y_top == bbox.bottom).
5. **Sorted, unique y_top** — rows sorted ascending by y_top, no duplicate y_top.

`normalize` is **idempotent and bit-exact**: `normalize(normalize(R)) == normalize(R)`. Because the form is canonical, `region_equal` is a **structural compare** (memcmp of bbox/rows/x-lists) — equal regions are byte-identical, which is what makes the homomorphism suite's equality test exact.

### 3.3 D-3 — The Four Op Truth Tables and Frame-Relative Complement

Every binary op is a **y-band sweep** over (A.rows + B.rows); within each band a per-scanline **x-merge** walks both x-lists in sorted order under the state pair `(inA, inB)`, emitting an inversion point **only when `boolfn(inA,inB)` changes**. The four ops differ **only** by `boolfn` — the entire algebra is four truth tables:

| Op | boolfn(inA,inB) | 4-bit table (LSB=state 00) | QuickDraw name |
|---|---|---|---|
| UNION | inA OR inB | `0x0E` (1110b) | `UnionRgn` |
| INTERSECT | inA AND inB | `0x08` (1000b) | `SectRgn` |
| DIFF | inA AND NOT inB | `0x04` (0100b) | `DiffRgn` |
| XOR | inA XOR inB | `0x06` (0110b) | `XorRgn` |

The tables are locked as `RGN_TRUTH_*` macros with `_Static_assert`s (a build-time oracle: a typo is a compile error). `region_op` then `region_normalize`s the output (collapsing vertical adjacent-equal rows, dropping empty interior rows, recomputing bbox).

**Complement is RELATIVE TO AN EXPLICIT FRAME** — there is **no infinite universe** (a framebuffer-bounded region cannot represent one): `region_complement(out, A, frame) := region_op(out, frame_rgn, A, XOR)`, i.e. the frame minus A over the frame. The header states this guardrail at every complement reference.

Queries derive from the same machinery: `region_get_bbox`/`region_is_empty` are O(1) off maintained state; `region_contains_point` binary-searches the covering row then tests x-parity; `region_rect_in_region` is bbox-reject then (rect DIFF r)==empty; `region_intersects` is bbox-reject then INTERSECT-non-empty.

### 3.4 D-4 — The No-0x7FFF In-Band-Sentinel Guardrail

Because each row carries an **explicit `x_count`**, the inversion list is self-delimiting and **needs no in-band sentinel**. We therefore **do NOT adopt** the QuickDraw `0x7FFF` in-band row terminator: it is **unverified lore** about a proprietary format, and — decisively — `0x7FFF` (32767) is a **legal int16 coordinate**, so an in-band magic would **collide** with a valid coordinate the moment a region used it. Length-prefixed is unambiguous and sentinel-free. The header records this as an explicit guardrail (Law 1: do not import unverified lore as ground truth).

### 3.5 D-5 — Storage Caps and the A-Priori Merge Bound

Fixed per-region caps, sized for the common case (rects, chrome, single windows, the frame's worst window count), **fail loud** on overflow (Rule 2 — never a silent truncation):

- `RGN_ROWS_CAP = 256` (live rows incl. closing row).
- `RGN_X_POOL_CAP = 1024` (int16 inversion-point slots).
- `RGN_ROW_X_MAX = 256` (per-scanline x-merge scratch; 640px => <=320 spans => 640 points, covered with headroom).

A merge's output and scratch are **bounded a priori**: `rows_out <= rowsA + rowsB + 2` (each band boundary once, plus opening + closing rows) and `x_out <= xA + xB` (the merge never invents inversion points; a changed-output emit sits at an existing A-edge or B-edge). So an op into a region sized to its inputs cannot overflow if the inputs are within caps — the caps are a real, checkable bound, not a guess. Raising them is a deliberate, issue-tracked act (Rule 8).

---

## 4. The Correctness Oracle — the Homomorphism Property Suite (the ENTIRE signal)

Per Law 2 and PRD §6.2, ATKINSON's correctness is established by a **C property suite** (`harness/proptest/test_region.c`, beads initech-6dy), in the `test_mcb.c` idiom (`TEST_HARNESS`/`CHECK`, a **seeded LCG** so the fuzz is reproducible — Rule 11). There is **no external golden** (§2.2); this suite is the **complete** correctness specification.

**PRIMARY — the homomorphism property.** `rasterize(region)` paints the region into a bounded WxH bitmap (e.g. 64x64) by span-painting the inversion lists. For **thousands** of random regions, assert
`rasterize(A OP B) == rasterize(A) OP_set rasterize(B)` **bit-exact** for all four ops, and `rasterize(complement(A, frame)) == frame_bits AND NOT rasterize(A)`. The right-hand side computes the set op directly on the pixel bitmaps (the ground truth); the left-hand side runs the engine and rasterizes — equality is decisive.

**Generators MUST include RAW random scanline-span sets, not only rect-unions** — otherwise non-rectangular normal-form bugs (a bad vertical-RLE collapse, an odd-length x-list slipping through) hide behind always-rectangular inputs. This is an explicit, locked requirement of the suite.

**SECONDARY oracles** (all in the same suite):

- **Normalize-idempotence** — `normalize(normalize(R)) == normalize(R)`, bit-exact (D-2).
- **Algebra identities** — De Morgan; associativity and commutativity of UNION/INTERSECT/XOR; `A DIFF A == empty`; `A XOR A == empty`; `A UNION complement(A, frame) == frame`.
- **Rect-fast-path == general-path** — a region built via the `is_rect` fast path equals the same region built via the general path.
- **region_equal structural consistency** — equal-by-rasterization regions are equal-by-structure (the canonical-form guarantee, D-2).

**SHRINKER.** On any failure, a shrinker bisects the rect/span list and clamps coordinates toward a **minimal counterexample** (PRD §6.2 "shrinking on failure"), so a failure is reported as the smallest region pair that breaks the property — not a thousand-rect haystack.

**MUTATION PROOF (Rule 6 — a golden/oracle that never caught a regression is decoration).** The suite is proven to bite via `#ifndef`-guarded mutants, each of which MUST drive `test-region` **RED**, then be restored **GREEN**:

- `RGN_MUTATE_NO_VRLE` — skip the vertical-RLE collapse (invariant 3); the normal form is no longer canonical and idempotence/equality fail.
- `RGN_MUTATE_PARITY_OFF1` — off-by-one in the contains-point / rasterize parity test; spans land one pixel wrong and the homomorphism fails.
- `RGN_MUTATE_EMIT_NOCHANGE` — emit an inversion point even when `boolfn` output did not change; the x-lists gain spurious zero-width toggles and the homomorphism/normal-form fail.

`make test-region` (currently `stub_fail` at M3) is rewired to build and run this suite; `make test-region-mutant` (or equivalent) confirms each mutant goes RED. **This suite is the entire correctness signal for the region engine** — when it is green and mutation-proven, ATKINSON is done (Law 2); when it is not, no FLAIR layer above it may be trusted.

---

## 5. Consequences

### 5.1 Binding Constraints (upon ratification)

- C-1. `spec/region_algebra.h` is the **locked contract** (Rule 8); it is authored **first**, before any `os/flair/atkinson/` .c (it already exists as of this ADR).
- C-2. Both the engine and the property suite **include the same header**; the internal format and the oracle agree by construction.
- C-3. `region_assert_normal` (the five invariants) runs at the **top of every op**; a non-normal input is a fail-loud panic, never silently processed (Rule 2, Rule 3).
- C-4. The homomorphism suite (with raw-span generators + shrinker) is the **entire** correctness signal; it is **mutation-proven** before the engine is "done" (Law 2, Rule 6).
- C-5. **No in-band sentinel** (no 0x7FFF); length-prefixed x-lists (D-4).
- C-6. Complement is **frame-relative**; "complement" without a frame is rejected as meaningless (D-3).
- C-7. **One engine, two peer facades** (added by ADR-0005 Amendment AM-1, 2026-06-21): every region Boolean op of EITHER heritage -- the System-7 QuickDraw `Rgn` family OR the Win-3.1 GDI `HRGN` family -- bottoms out in the single `region.c` `region_op`. No second `region_op`/`xmerge`/band-sweep DEFINITION may appear outside `os/flair/atkinson/region.c`. The load-bearing guard is the **STRUCTURAL grep gate** (`test-region-gdi` L3), NOT a link-error backstop (the wine reference engine's `region_op` has a different signature, so static linkage gives no collision); any host-only foreign reference engine MUST be `gdi_ref_`-namespaced and NEVER linked into a kernel/artifact line (Law 3). See Amendment AM-1 Sec 5.1.

### 5.2 Forward Obligations

- FO-1. Implement `os/flair/atkinson/` against this header RED->GREEN: initech-jmo (rep + normalize-on-construction + `region_assert_normal`), then initech-b5g (the scanline merge: union, then sect/diff/xor/complement), then initech-6dy (the suite + shrinker + mutants).
- FO-2. The engine must **dual-compile** (freestanding for the kernel AND hosted for the suite — the `console.c`/`int21.c` pattern); the header already does, and its `_Static_assert`s pass in both modes.
- FO-3. Wire `make test-region` (and a `-mutant` companion) off the `stub_fail` and into `make test`.

### 5.3 Neutral Consequences

- Regions are **coordinate-only**; the indexed-8 offscreen depth (ADR-0004 OD-2) and 640x480 resolution (OD-3) touch imaging, not this engine (640x480 << 32767, so int16 coords never approach the type bound).

---

## 6. Alternatives Considered (and Rejected)

- **A1 — Port QuickDraw's region body format.** Rejected: the body format is proprietary and unpublished (§2.2); a "port" would be reverse-engineered guesswork with no oracle to check it against. The clean-room rep designed for our homomorphism oracle is strictly better-founded.
- **A2 — Adopt the 0x7FFF in-band sentinel.** Rejected: unverified lore that collides with a legal coordinate (D-4); explicit `x_count` is unambiguous.
- **A3 — Rectangle-list (R-tree / banded-rect) representation.** Rejected: inversion lists give the cleanest scanline-merge algebra and the most direct homomorphism oracle (rasterize-and-compare), which is the project's decisive correctness tool (PRD §6.2). A rect-list complicates XOR/complement and the normal-form canonicalization without a compensating oracle benefit.
- **A4 — Prove correctness with a proof assistant.** Rejected by decree (PRD §14, C-only, no proof assistants; Stop condition). The brute-force homomorphism suite is decisive without one (PRD §6.2).
- **A5 — Per-row malloc.** Rejected: not freestanding-legal (no host malloc in the kernel) and non-deterministic layout (Rule 11). All x-data lives in one contiguous arena-backed `x_pool`.

---

## 7. Open Questions (RESOLVED at ratification)

- **OQ-1 — region pool home.** **RESOLVED** by ADR-0004 **DEC-03** (2026-06-19, ADR-by-committee): the per-region `rows[]`/`x_pool` storage is backed by the **dedicated FLAIR extended-memory heap** (`FLAIR_HEAP_BASE = 0x00100000`, `FLAIR_HEAP_SIZE = 0x00400000`; one FLAIR-owned bump + typed-free-list arena), **NOT** the per-program MCB arena (which is rebound on every EXEC and would destroy persistent GUI state). This was the **same open question** as ADR-0004 OQ-1 (FLAIR Toolbox heap home) and is resolved there. The ATKINSON engine itself **remains allocator-agnostic** — it takes caller-supplied storage — so the FLAIR heap backs the region pools with **no change to the engine**, and this resolution did not block the engine or its oracle (both already green). Original question: where the per-region `rows[]`/`x_pool` storage is ultimately backed (a dedicated FLAIR region arena vs. the MCB arena). See ADR-0004 Section 9 DEC-03.

---

## 8. Related Decisions and References

- **ADR-0004 — FLAIR Toolbox Architecture** (umbrella, RATIFIED 2026-06-19): this engine is layer 2; the damage model (§3.5) and clipping consume it; OQ-1 (region pool home) is resolved by its DEC-03.
- ADR-0002 — C implementation language (the engine and suite are C).
- ADR-0003 — InitechDOS base OS (the MCB arena, the `console.c`/`int21.c` dual-compile pattern the engine follows).
- PRD §6.2 (the region engine spec + homomorphism oracle), §6.3 (the Toolbox that consumes it), §9 (region-algebra anchor as the swarm discipline), §12 (Toolbox-sprawl risk), §14 (C-only, no proof assistant).
- Locked spec-data: **`spec/region_algebra.h`** (the contract this ADR governs).
- beads: initech-jmo, initech-b5g, initech-6dy.

<!-- END OEA-ADR-0005 (RATIFIED) — INITECH CONFIDENTIAL -->
