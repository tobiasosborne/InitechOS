<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0004 — FLAIR Toolbox Architecture (the System-7-style GUI layer)

**Issuing Body:** Initech Systems Corporation — Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0004 |
| Title | ADR-0004: FLAIR Toolbox Architecture |
| Version | 1.0 (Ratified) |
| Status | **RATIFIED (ADR-by-committee, operator-delegated authority, 2026-06-19)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme |
| Effective Date | 2026-06-19 |
| Next Scheduled Review | Upon operator ratification, per RECORDS-POL-002 |
| Supersedes | (none) |
| Superseded By | (ADR whole) none. **OD-4 (seafoam `desktop_bg` canon) SUPERSEDED 2026-06-22** by ADR-0004-AMENDMENT-DEC-09 + ADR-0010 -> Initech teal `#8DDCDC`; see REVOCATION-RECORD-2026-06-21 (HER-01/06/08). |
| Related Documents | ADR-0001 (386+, 32-bit flat); ADR-0002 (toolchain / impl language / exec format); ADR-0003 (InitechDOS base OS) + Amendments DEC-04a, DEC-14; ADR-0005 (ATKINSON region engine — companion, RATIFIED 2026-06-19); CDR-0001 (interim toolchain) |
| Related Issues | beads initech-jmo, initech-b5g, initech-6dy (region engine); initech-i50 (blitter w/ region clip); initech-87a (window drag w/ clip); initech-f8v.4 (tracer keystone) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | (draft) | Architecture Review Board, STAPLER Programme | Initial draft. Records the 5-layer FLAIR stack, the GrafPort/surface single-pixel-path decision, the Manager decomposition (verbatim Inside Macintosh records/part-codes), the ISR-enqueue-only event model, the region-difference damage model, cooperative WaitNextEvent, input/fonts/chrome, and the layered GUI oracle vector. Folds the operator-ratified session decisions (indexed-8 depth, 640x480, seafoam kept); records the canonical-depth dissent and the two still-open questions. | (pending committee review) |
| 1.0 | 2026-06-19 | Architecture Review Board, STAPLER Programme | Ratified by ADR-by-committee (wf_573c1cf5-537), no gridlock. Carries amendments AM-1..AM-9. OQ-1 resolved (DEC-03, extended-memory heap); OQ-2 resolved (DEC-04, defer 86Box). See new Section 8. | ARB (Bolton/Nagheenanajar/Smykowski + Fidelity Steward) + Chair |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Architecture Review Board, STAPLER Programme | Submitted (DRAFT) | (draft) |
| ARB Reviewer — Technical Correctness | M. Bolton (Senior Engineer, Platform) | Approved (2026-06-19) | 2026-06-19 |
| ARB Reviewer — Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Approved (2026-06-19) | 2026-06-19 |
| ARB Reviewer — Governance & Compliance | T. Smykowski (QA / Change Advisory) | Approved (2026-06-19) | 2026-06-19 |
| ARB Reviewer — Fidelity Steward | Fidelity Steward (Heritage Conformance) | Approved (2026-06-19) | 2026-06-19 |
| ARB Chair (Synthesis) | ADR-by-committee Chair | Synthesized + Approved (2026-06-19) | 2026-06-19 |
| Operator Ratification | T. Osborne (Operator) | **Granted via delegated committee authority (2026-06-19)** | 2026-06-19 |
| Records Management | M. Waddams (Archive Annex B) | Filed (2026-06-19) | 2026-06-19 |

*Note on status: This ADR is RATIFIED (ADR-by-committee, operator-delegated authority, workflow wf_573c1cf5-537, 2026-06-19; no gridlock). The four operator-ratified decisions recorded in Sec 3.0 are settled and are folded in as binding inputs; the remainder of the architecture (Manager decomposition, damage model, oracle vector) is ratified as the binding architecture and carries amendments AM-1..AM-9 (see Section 8). The two items in Sec 7 (Open Questions) are no longer open: OQ-1 and OQ-2 are RESOLVED at ratification (DEC-03 / DEC-04, Section 8).*

---

## 1. Purpose and Scope

### 1.1 Purpose

This ADR establishes the architecture of **FLAIR**, the System-7-style Toolbox layer of InitechOS (PRD §6.3, §4). FLAIR is the "Mac chimera" half of the product: the window/menu/control/event/dialog machinery that, layered over the InitechDOS (MILTON) personality, reproduces the live, draggable desktop of the *Office Space* reference frame (PRD §1, §3, Appendix A). The purpose of this document is to fix the layering, the imaging model, the Manager decomposition, the event and damage models, and — critically — the **oracle vector** that keeps the Toolbox from degenerating into "thousands of lines of plausible nonsense" (PRD §9, §12; the single largest sprawl risk in the programme).

### 1.2 Scope

This ADR governs:

- The **5-layer FLAIR stack** (surface/GrafPort -> ATKINSON regions -> Managers -> cooperative Event core -> desktop shell). (§3.1)
- The **GrafPort imaging model** and the decision that there is exactly **one surface (pixel) module**, extracted from `console.c`, with no second pixel path. (§3.2)
- The **Manager decomposition** (Window / Menu / Control / Event / Dialog), carrying **verbatim Inside Macintosh records and part-codes**. (§3.3)
- The **event model**: ISR enqueue-only; `EventRecord` synthesis in task context. (§3.4)
- The **damage / update model**: region-difference (`DiffRgn`) update regions. (§3.5)
- The **scheduling model**: cooperative, non-preemptive `WaitNextEvent` (decided canon, PRD §2, §15). (§3.6)
- **Input, fonts, and chrome**: PS/2 mouse (IRQ12) with dual-PIC EOI and bounded spin, Bochs-verified; the hourglass fixed-bytes cursor canon; proportional `NFNT` text measurement; `chrome_metrics` v1. (§3.7)
- The **layered GUI oracle vector** (§4) — the mechanical truth that gates every FLAIR subsystem.

### 1.3 Out of Scope

- The **internal byte format and algorithms of the ATKINSON region engine** — governed by the companion **ADR-0005** and the locked `spec/region_algebra.h`. This ADR depends on ATKINSON as a layer; it does not specify its math.
- The **bundled application** behaviour (InitechCalc, File Manager, InitechPaint, FILE COPY) beyond the chrome/menu they exercise — those are PRD §6.5 / M5 deliverables.
- The **resource-fork analogue** detailed format (PRD §6.4) beyond FLAIR's consumption of `FONT`/`NFNT`/`CURS`/`WIND`/`MENU` blobs.
- **InitechBase** (SAMIR) and **Turbo Initech** (TPS), which are tenants of, not part of, FLAIR.
- Resolutions other than the native 640x480 (PRD §5 lists 800x600 / 832x624 as future modes; this ADR fixes 640x480 as the M3/M4 native per the operator decision in §3.0).

### 1.4 Additional Defined Terms

| Term | Definition |
|---|---|
| FLAIR | The InitechOS Toolbox layer (PRD §4, §6.3); the System-7-style GUI. |
| GrafPort | The QuickDraw-equivalent drawing context: a record bundling the destination bitmap, the current `visRgn`/`clipRgn`, pen state, and coordinate origin. The unit through which all drawing is clipped. |
| ATKINSON | The region engine (PRD §6.2; ADR-0005); the load-bearing Boolean-algebra-of-pixel-sets math. |
| Surface module | The single low-level pixel writer (pixel/span/blit into the framebuffer or an offscreen bitmap), extracted from `os/milton/console.c`. There is exactly one. |
| EventRecord | The Inside Macintosh event record (`what`, `message`, `when`, `where`, `modifiers`) synthesized in task context from raw device input. |
| visRgn / clipRgn | A GrafPort's visible region (set by the Window Manager) and application clip region; drawing is masked by their intersection. |
| Damage / update region | The region of a window that must be repainted, computed by region difference when overlapping windows move or close. |
| WaitNextEvent | The cooperative event-pump entry point; a task blocks here yielding the CPU until an event or timeout (PRD §2). |
| Indexed-8 | An 8-bit palettized offscreen pixel depth (one byte per pixel indexing a palette), the canonical internal offscreen depth (operator decision, §3.0). |
| NFNT | The Inside Macintosh bitmap-font resource format; FLAIR measures proportional text via per-glyph advance widths. |
| Chrome | Window-frame furniture: pinstripe title bar, close/zoom boxes, scrollbar thumbs/arrows, grow box — measured in `spec/chrome_metrics.json`. |

---

## 2. Context

### 2.1 Why an Umbrella ADR Now

The region engine (ATKINSON) has **no emulator dependency** — it is pure host-testable Boolean algebra over pixel sets — and the operator has ratified that FLAIR work proceeds **now**, in parallel with finishing the f8v.4 tracer-bullet keystone, starting from the locked specs plus the pure-host region engine. Before any `os/flair/*.c` is written, the layering, the imaging contract, and the oracle vector must be fixed as data and as a decision record, exactly as ADR-0003 fixed the MILTON base. Without that anchor, the Toolbox is the programme's worst sprawl vector (PRD §9, §12 — "without the region-algebra anchor and the chrome-SSIM gate [it] degenerates into thousands of lines of plausible nonsense").

### 2.2 Relationship to the Locked Spec-Data

This ADR is the governance record for several locked artifacts:

- `spec/region_algebra.h` — authored as the companion to ADR-0005; the ATKINSON contract this stack's layer 2 implements. (Authored first, Rule 8.)
- `spec/chrome_metrics.json` (v1) — the title-bar height, pinstripe period, scrollbar widths the Window/Control Managers and the chrome oracle consume.
- `spec/assets/` — palette, Chicago/Geneva strikes, the hourglass cursor bytes the surface module and Font/Cursor handling consume.

Changing any of these is a deliberate, issue-tracked act (Rule 8), never a silent edit to make a render "look right" (Law 4 is a guide, not a license to fudge the locked data).

---

## 3. The Decision

### 3.0 Operator-Ratified Inputs (Settled — Folded In)

The following four decisions were ratified by the operator in the session that commissioned this ADR. They are **settled** and are binding inputs to the architecture below; they are recorded here for the record, not re-opened.

| # | Decision | Consequence for FLAIR |
|---|---|---|
| OD-1 | **FLAIR proceeds now**, in parallel with the f8v.4 tracer keystone, from the locked specs + the pure-host region engine. | The region math (layer 2) has no emulator dependency and is built and oracle-gated on the host immediately; the rest of the stack follows. |
| OD-2 | **Canonical internal offscreen depth = INDEXED-8.** | The surface module's offscreen bitmaps are 8-bit palettized. Affects `grafport.h`/imaging, **not** `region_algebra.h` directly (regions are pure int16-coordinate pixel sets, depth-agnostic). (Period grounding, AM-7: indexed-8 is the authentic early-1990s 8-bpp VGA/SVGA offscreen/backing-store depth -- VGA Mode 13h, VBE modes 0x101/0x103 -- not merely an operator preference; Law 1.) |
| OD-3 | **Native target resolution = 640x480.** | Region int16 coordinates are framebuffer-bounded; 640x480 << 32767, so no coordinate can collide with any in-band magic (and ADR-0005 carries no in-band sentinel anyway). |
| OD-4 | **`desktop_bg` canon = KEEP SEAFOAM teal `(0x6F,0xA0,0x8E)`.** | The live `tools/ppm_seafoam_check.c` oracle (mirroring `os/boot/stage2.asm` `SEAFOAM_R/G/B`) stands as the desktop-background gate. (Note: `spec/assets/palette.json` records a separate `desktop_bg` "ground-truth v0" gray sampled from the compressed still; the SEAFOAM boot/oracle value is the canon FLAIR paints. Reconciling the palette-v0 record to seafoam is a `palette` follow-up, not a FLAIR blocker.) **[SUPERSEDED 2026-06-22 by ADR-0004-AMENDMENT-DEC-09 + ADR-0010: `desktop_bg` canon = Initech teal `#8DDCDC` (idx2); seafoam `#6FA08E` REVOKED -- see Revocation Record HER-01. The `ppm_seafoam_check`/`test-palette-seafoam` by-construction gate is retired (HER-08).]** |

### 3.1 Decision D-1 — The 5-Layer FLAIR Stack

FLAIR is decomposed into five layers, lowest to highest. Each layer depends only on the layers below it; cross-layer reach-through is prohibited (a sprawl guard).

```
  Layer 5  DESKTOP SHELL        menu bar, desktop pattern (seafoam), window
                                z-order, app launch, the WaitNextEvent pump host
  Layer 4  COOPERATIVE EVENT    EventRecord queue; WaitNextEvent; PIT-tick
           CORE                  cooperative yield; null/update/activate events
  Layer 3  MANAGERS             Window / Menu / Control / Dialog Managers
                                (verbatim Inside Macintosh records + part-codes)
  Layer 2  ATKINSON REGIONS     region_op (union/sect/diff/xor) + complement,
                                visRgn/clipRgn, update regions  (ADR-0005)
  Layer 1  SURFACE / GrafPort   the ONE pixel path: pixel/span/blit into the
                                LFB or an indexed-8 offscreen, masked by a
                                GrafPort's visRgn ^ clipRgn
```

The contract is that **all drawing flows through a GrafPort (layer 1) and is clipped by an ATKINSON region (layer 2)**; the Managers (layer 3) never touch the framebuffer directly. This is the single most important structural invariant in the Toolbox.

### 3.2 Decision D-2 — One Surface Module; GrafPort Imaging; No Second Pixel Path

There shall be exactly **one** low-level pixel writer in InitechOS, **extracted from `os/milton/console.c`** (which already blits the VGA ROM font into the LFB and fills seafoam). The FLAIR surface module generalizes that single writer to (a) pixel, (b) horizontal span, and (c) bitmap blit, each masked by a GrafPort's effective clip (visRgn intersect clipRgn). The console text path and the GUI path are the **same** pixel code; there is no second framebuffer path to drift out of agreement.

Imaging is **QuickDraw-equivalent via the GrafPort record**: a destination `bitmap_t` (the LFB or an indexed-8 offscreen, OD-2), an origin, a `visRgn`, a `clipRgn`, and pen state. `SetPort`/`GetPort` select the current port; every primitive (`FillRect`, `FrameRect`, `BlitBits`, text draw) consults the current port's effective clip. Offscreen bitmaps are **indexed-8** (OD-2); the LFB target may be 8- or 32-bpp per the hardware contract, with the surface module owning the one conversion site.

### 3.3 Decision D-3 — Manager Decomposition with Verbatim Inside Macintosh Records and Part-Codes

The Managers (layer 3) carry **verbatim Inside Macintosh records and part-codes** for period authenticity and to keep the API self-documenting against the reference:

- **Window Manager** — `WindowRecord` (port, `strucRgn`/`contRgn`/`updateRgn`, `windowKind`, visibility, title); `FindWindow` part-codes `inDesk`, `inMenuBar`, `inContent`, `inDrag`, `inGrow`, `inGoAway`, `inZoomIn`/`inZoomOut`. Drag/z-order/update-region maintenance via layer 2 (`DiffRgn`, §3.5).
- **Menu Manager** — `MenuInfo` (menu ID, title, items with mark/style/cmd-char); the menu bar including the **Photoshop-exact** bar for InitechPaint (`File Edit Image Layer Select View Window Help` — canon, NOT to be "corrected," PRD §1, Appendix A). `MenuSelect` returns `(menuID << 16 | item)`.
- **Control Manager** — `ControlRecord` (value/min/max, `contrlHilite`, `contrlRect`); part-codes `inButton`, `inCheckBox`, `inUpButton`/`inDownButton`/`inPageUp`/`inPageDown`/`inThumb`. Buttons, scrollbars, the FILE COPY progress bar.
- **Dialog Manager** — `DialogRecord` + item lists; `ModalDialog`; the modal **FILE COPY** box ("Saving tables to disk…", the comedic centerpiece, PRD §6.5 / Appendix B).

The Event Manager is layer 4 (§3.4), not layer 3, because the cooperative pump is the spine the Managers hang from.

### 3.4 Decision D-4 — ISR Enqueue-Only; EventRecord Synthesis in Task Context

Hardware interrupt service routines (PS/2 keyboard IRQ1, PS/2 mouse IRQ12, PIT IRQ0) do the **minimum**: read the device, push a compact raw record into a lock-free single-producer/single-consumer ring buffer, send the PIC EOI(s), and return. ISRs do **not** synthesize `EventRecord`s, allocate, or call Managers.

`EventRecord` **synthesis happens in task context** inside `WaitNextEvent`/`GetNextEvent`: the pump drains the raw ring, cooks raw scancodes/mouse-deltas/ticks into `EventRecord`s (`mouseDown`/`mouseUp`/`keyDown`/`autoKey`/`updateEvt`/`activateEvt`/`nullEvent`), stamps `when` (tick count) and `where` (cursor position), and dispatches. This keeps ISRs bounded and non-reentrant against the Toolbox (Rule 2 fail-loud lives in task context where a panic can render `PC LOAD LETTER`).

### 3.5 Decision D-5 — Region-Difference Damage Model

When a window moves, closes, or changes z-order, the **damaged** area of every exposed window is computed by **region difference** (`DiffRgn`) on layer 2: newly-exposed = (old-covered) DIFF (now-covered), accumulated into each window's `updateRgn`. The pump then issues `updateEvt`s; the app redraws clipped to its `visRgn` intersect `updateRgn`. No window ever repaints more than its damage. This is the direct, mechanical payoff of the ATKINSON algebra (PRD §6.2) and the reason the region engine is the load-bearing math.

### 3.6 Decision D-6 — Cooperative, Non-Preemptive WaitNextEvent (Decided Canon)

Scheduling is **cooperative**, non-preemptive, on the PIT tick — `WaitNextEvent`-style (PRD §2 non-goals; §15 DECIDED). A task holds the CPU until it calls back into `WaitNextEvent`; there is no preemption and no protected inter-process isolation. An app that fails to yield hangs the desktop — that is **period-authentic, not a bug** (CLAUDE.md hallucination-risk callout; do not "fix" with preemption). The PIT tick advances the global tick count (`when` stamps, `sleep` timeouts) and is the only thing the kernel does behind a running task's back.

### 3.7 Decision D-7 — Input, Fonts, Chrome

- **Mouse**: PS/2 mouse on **IRQ12**, requiring a **dual-PIC EOI** (slave then master) and a **bounded spin** on the 8042 status register (never an unbounded poll — Rule 2). (Ref: Intel 8259A PRM / IBM PC AT Technical Reference -- IRQ12 is on the slave PIC behind the IRQ2 cascade, so servicing it requires EOI to the SLAVE then the MASTER 8259A; bounded spin on the 8042 status register, never an unbounded poll. Law 1, AM-7.) The bring-up sequence is **Bochs-verified** (strict real->protected and PIC accuracy), not QEMU-only (Rule 5, Stop conditions).
- **Cursor**: the **hourglass** is canon (the wristwatch is the bug, PRD Appendix A / Law 4); the cursor is shipped as **fixed bytes** (a `CURS`-style 16x16 image + mask in `spec/assets/`), not procedurally generated, so it is byte-stable (Rule 11).
- **Fonts**: **proportional NFNT text measurement** — Chicago (system/dialog) and Geneva 9 (cell), hand-authored strikes (PRD §6.4; the still is too low-res for pixel extraction — do not claim extraction, Law 1). Text width is the sum of per-glyph advances; no fixed-pitch assumption.
- **Chrome**: driven by `spec/chrome_metrics.json` **v1** (title-bar height, pinstripe period, close/zoom-box geometry, scrollbar widths/thumb), measured from the frame.

### 3.8 Decision D-8 — The Layered GUI Oracle Vector

Every FLAIR subsystem advances only when its mechanical oracle is green (Law 2). The vector, all **mutation-proven** (Rule 6 — perturb -> RED -> restore):

| Oracle | Gate | What it proves |
|---|---|---|
| `test-region` | hard pass/fail | ATKINSON homomorphism + 5 invariants + algebra identities (ADR-0005). The spine. |
| `test-chrome` | hard pass/fail | Chrome renders match `chrome_metrics` v1 + fixture crops (structural compare, not SSIM). |
| `test-event` (event-replay) | hard pass/fail | A recorded raw-input trace replays to a deterministic `EventRecord`/dispatch sequence. |
| `fb-agree` | hard pass/fail | The console pixel path and the GUI pixel path agree on shared primitives (the one-surface invariant, D-2). |
| `canon` | hard pass/fail | The enforced canon: hourglass-not-wristwatch cursor bytes; the Photoshop menu bar; **Initech teal `#8DDCDC` desktop** (supersedes seafoam 2026-06-22, DEC-09/ADR-0010, graded LIVE vs the decomp/operator canon -- NOT by construction); (with the apps) the 116% pie and `570-` format. |
| `drag-gate` (initech-87a) | hard pass/fail | a window drags across the desktop with correct DiffRgn update regions, no over-repaint, chrome unchanged outside the damaged area, verified vs chrome_metrics v1 fixture crops -- the earliest human-verifiable Law-4 fidelity moment. (AM-8) |
| `ssim` | **GUIDE ONLY — never gates** | Per-window structural similarity vs the frame fixture; reported to point agents toward fidelity (Law 4, PRD §3, §8). Structurally a guide; it is NOT summed into a reward and never blocks a merge. |

**Cross-emulator agreement** (Rule 5) for the framebuffer is defined as: **each emulator's screendump digest is compared against the HOST model's prediction for THAT emulator's own mode** — NOT a cross-emulator byte-CRC. (QEMU, Bochs, and 86Box legitimately differ at the pixel level — palette ramps, VGA DAC, LFB layout — so a naive cross-emulator byte-identity gate would be wrong; the host model that predicts each one is the correct oracle, and disagreement between an emulator and its own predicted output is the bug signal.)

**Host-model parameterization (AM-1).** The host render skeleton (`harness/render`, beads initech-k8o5.7) is parameterized by the **RUNTIME** `boot_info_t` LFB geometry (`lfb_addr`, `lfb_pitch`, `lfb_bpp`, `lfb_width`, `lfb_height` as reported by the VBE `PhysBasePtr` on QEMU or the `0x000A0000` VGA fallback on the Bochs mode-0x13 path), **NEVER a hardcoded aperture**. The oracle MUST consume the same `boot_info_t` the kernel does, so emulator and host model predict the same physical address (verified against `os/boot/stage2.asm`: `lfb_addr` is the VBE `PhysBasePtr` at offset 0x28 on QEMU and `0x000A0000` on the Bochs mode-0x13 fallback). This is a forward obligation gating the host render skeleton (FO-C, Section 8).

**Frozen definition (AM-6).** The cross-emulator agreement definition above -- "each emulator vs its own host-model prediction, NOT a cross-emulator byte-CRC" -- is **FROZEN**. It may be changed **only** by a deliberate ADR amendment; a future agent must NOT silently revert to a naive cross-emulator byte-CRC or pin to QEMU (a Stop condition).

---

## 4. Rationale

### 4.1 One Surface Module (D-2)

Two pixel paths inevitably drift: the console renders text one way, the GUI another, and "looks right" hides the divergence until a glyph lands a pixel off in one but not the other. Extracting the single writer from `console.c` and making `fb-agree` a gate (D-8) means the divergence is a build failure, not a forensic hunt (Law 2, Rule 3 "all bugs are deep").

### 4.2 ISR Enqueue-Only (D-4)

A Toolbox call from interrupt context is the classic reentrancy bug: an ISR that synthesizes an `EventRecord` and dispatches can re-enter a Manager mid-mutation. Bounding ISRs to "read device, enqueue, EOI" and cooking events in task context makes the whole Toolbox single-threaded-by-construction under cooperative scheduling (D-6), which is exactly the period-authentic model and removes an entire bug class a priori.

### 4.3 Region-Difference Damage (D-5)

Repainting whole windows on every move is both slow (irrelevant — these are slow machines, PRD §2) and **wrong-looking** (flicker the era's software did not have). The `DiffRgn` damage model is the authentic minimal-update behaviour and is the direct consumer of the ATKINSON algebra, which is why ADR-0005 is the foundation under this umbrella.

### 4.4 SSIM as Guide, Not Gate (D-8)

A numeric SSIM cutoff would either reject authentic frames (the still is compressed, low-res) or be set so loose it proves nothing. The judge is a person who used the era's software saying "yes, that's it" (Law 4, PRD §3). SSIM is reported per-window to steer agents; the **structural** oracles (chrome metrics, canon bytes, event replay) are what actually gate. Recording this prevents a future agent from "tightening" SSIM into a hard cutoff and weakening the real signal (Stop conditions).

### 4.5 Cross-Emulator Agreement Defined Correctly (D-8)

Naively CRC-ing three emulators' framebuffers against each other would fail on legitimate DAC/palette/LFB differences and tempt an agent to pin to QEMU (a Stop condition). Defining agreement as "each emulator vs the host model's prediction for its own mode" keeps the emulator-ism detector honest: it flags a real divergence (a transition bug, a clip bug) without false-positiving on benign hardware differences.

---

## 5. Consequences

### 5.1 Binding Constraints (upon ratification)

- C-1. All FLAIR drawing flows through a GrafPort and is clipped by an ATKINSON region; Managers never touch the framebuffer (D-1, D-2).
- C-2. Exactly one surface module exists, extracted from `console.c`; `fb-agree` gates it (D-2, D-8).
- C-3. ISRs are enqueue-only; `EventRecord` synthesis is task-context (D-4).
- C-4. Damage is computed by `DiffRgn`; no over-repaint (D-5).
- C-5. Scheduling is cooperative `WaitNextEvent`; no preemption is added (D-6).
- C-6. The mouse bring-up is Bochs-verified before it is trusted (D-7, Rule 5).
- C-7. Every FLAIR oracle in §3.8 is mutation-proven before its subsystem is "done" (Rule 6).

### 5.2 Forward Obligations

- FO-1. **Author the surface-module extraction** from `console.c` (beads initech-k8o5.6) with a **GREEN, MUTATION-PROVEN `fb-agree` gate** -- including at least one **NAMED** `fb-agree` mutant (e.g. route a shared span through a second pixel path and assert the gate goes RED), matching the ADR-0005 `RGN_MUTATE_*` discipline (Rule 6) -- before **ANY** Manager code lands. The one-surface invariant (D-2 / C-2) is unenforceable until `fb-agree` bites. (AM-2)
- FO-2. **`chrome_metrics.json` v1 must be LOCKED AND `test-chrome` MUTATION-PROVEN** -- with three named mutants (Rule 6) -- at the **same gate** as FO-1, before **ANY** Window/Control Manager drawing code ships (beads initech-k8o5.8). A locked metric with an unproven oracle lets plausible chrome through; "locked or confirmed locked" is no longer sufficient. (AM-3)
- FO-3. **Reconcile `palette.json` `desktop_bg` v0 (gray) to the SEAFOAM canon** (OD-4) as a follow-up issue (beads initech-ch81) that **includes an oracle asserting `palette.json` canonical `desktop_bg` == the live boot/oracle SEAFOAM value `(0x6F,0xA0,0x8E)`** -- not a visual check -- so the locked palette and the live boot/oracle value agree. (AM-9)
- FO-4. When **ring-3 / app isolation** is ever revisited (currently a non-goal, PRD Sec 2), the cooperative model (D-6) and the ISR/event boundary (D-4) must be re-examined; out of scope here.
- FO-5. The **OQ-1 / DEC-03 resolution** (extended-memory FLAIR heap, Section 8) is executed as **ONE** deliberate, beads-tracked Rule 8 act (beads initech-k8o5.5) touching **exactly** `os/milton/boot_info.h` (add `ext_mem_kb`) + `spec/memory_map.h` (`FLAIR_HEAP_BASE`/`FLAIR_HEAP_SIZE`/`FLAIR_HEAP_MIN`) + `spec/hardware.json` (memory section) + the stage2 INT 15h probe. **No FLAIR allocator `.c` may be written before that beads issue is open and linked.** No silent edits. (AM-5)

**Canon as frozen locked-data (AM-4).** D-3's Photoshop menu-bar string (`File Edit Image Layer Select View Window Help`) and D-7's hourglass `CURS` bytes are **FROZEN locked-data**, tracked by beads initech-zaqj as named `spec/assets/` files / locked constants -- **not prose an agent may silently "correct."** The `canon` oracle (D-8) gates these frozen bytes.

### 5.3 Neutral Consequences

- The indexed-8 offscreen depth (OD-2) localizes all depth conversion to the surface module; higher layers are depth-agnostic, and the region engine is entirely coordinate-only.

---

## 6. Recorded Dissent

For the record (this is a real architectural split, not unanimous):

- **D-DISSENT-1 — Canonical-depth split.** One reviewing function held that the canonical internal offscreen depth should be **32-bit direct color** (simpler blits, no palette management, closer to the modern LFB), against the ratified **indexed-8** (OD-2; period-authentic, smaller offscreens, matches the 8-bpp era and the palettized seafoam/chrome). The operator ratified **indexed-8**; the dissent is recorded so that if a future depth-related friction surfaces (e.g., 32-bpp-only LFB modes), the trade-off is already on record and re-opening it is informed, not rediscovered.

---

## 7. Open Questions (RESOLVED at ratification)

Both questions below were resolved at ratification by ADR-by-committee (2026-06-19). The original question text is retained for the record; the resolution and full ruling are in Section 8 (DEC-03 / DEC-04).

- **OQ-1 -- FLAIR Toolbox heap home.** RESOLVED -- see Section 8 DEC-03/DEC-04: resolved by **DEC-03** (dedicated extended-memory region `[0x100000, 0x500000)`, 4 MiB, fixed/spec-locked window with a fail-loud INT 15h probe; NOT the per-program MCB arena). Original question: Where does the Toolbox allocate its handles/records/region pools -- a **new high region** carved above the kernel/program windows, or **inside the MCB arena** (the existing AH=48h/49h/4Ah allocator, ADR-0003 / initech-509.6)? Trade-offs: a dedicated high region keeps Toolbox allocation off the DOS arena (no fragmentation interplay with loaded programs) but adds a second allocator; the MCB arena reuses proven, oracle-covered code but couples GUI lifetime to DOS memory semantics.
- **OQ-2 -- Real-Bochs / 86Box pixel-capture funding.** RESOLVED -- see Section 8 DEC-03/DEC-04: resolved by **DEC-04** (DEFER 86Box pixel-capture funding; run QEMU + Bochs now under the host-model-per-mode definition; a FUNDED follow-up (beads initech-q0gy) is filed and BLOCKS M4 sign-off). Original question: The chrome/fidelity oracles ideally capture real-Bochs and 86Box framebuffers (not only QEMU) for cross-emulator agreement (D-8). Standing up reliable 86Box + period-VGA-BIOS pixel capture is an unfunded effort; until it is funded, cross-emulator agreement runs against the emulators currently wired.

---

## 8. Ratification Record (ADR-by-committee, 2026-06-19)

This ADR was ratified by the InitechOS ADR-by-committee under operator-delegated authority (workflow wf_573c1cf5-537, 2026-06-19; no gridlock). The committee comprised four ARB reviewers (Bolton -- Technical Correctness; Nagheenanajar -- Period Authenticity; Smykowski -- Governance & Compliance; the Fidelity Steward) plus the ARB Chair (Synthesis). The committee's synthesis IS the ratification. The decision records, amendments, recorded dissent, and forward obligations below are recorded verbatim from the chair synthesis.

### 8.1 Decision Records (DEC-01 .. DEC-04)

**DEC-01 -- ADR-0004 (FLAIR Toolbox Architecture) RATIFIED with amendments.**
RATIFY-WITH-AMENDMENTS. The 5-layer stack (D-1), the single surface module extracted from console.c with no second pixel path (D-2), verbatim Inside-Macintosh Manager records/part-codes (D-3), ISR-enqueue-only events with task-context EventRecord synthesis (D-4), the DiffRgn region-difference damage model (D-5), cooperative non-preemptive WaitNextEvent (D-6), input/fonts/chrome with Bochs-verified IRQ12 dual-PIC EOI (D-7), and the layered oracle vector with SSIM-as-guide-not-gate (D-8) are all sound and ratified as the binding architecture. Carries amendments AM-1..AM-9 (host-model boot_info parameterization; FO-1 surface-extraction + fb-agree mutant before Managers; FO-2 test-chrome mutation-proven; canon frozen as locked data; FO-5 OQ-1 as a single Rule 8 act; D-8 definition frozen against silent revert; Law 1 EOI/indexed-8 citations; M3 drag-gate named; FO-3 palette oracle). No blocking concern. Operator ratification of this ADR as a whole is the remaining gate per its sign-off matrix.

**DEC-02 -- ADR-0005 (ATKINSON Region Engine) RATIFIED as-is.**
RATIFY as-is, no amendments. Unanimous. The clean-room per-scanline inversion-list rep (D-1), the five-invariant normal form with region_assert_normal at the top of every op (D-2/C-3), the four truth-table ops + frame-relative complement (D-3), the no-0x7FFF in-band-sentinel guardrail correctly grounded in Law 1 (D-4), the a-priori-bounded storage caps with fail-loud overflow (D-5), and the homomorphism property suite (with raw-span generators + shrinker) as the ENTIRE correctness signal -- no external golden, because the QuickDraw region body is proprietary/unpublished (Sec 2.2) -- are all correct. Verified GREEN by the chair: make test-region reports 31 checks, 0 failures; the three named mutants (RGN_MUTATE_NO_VRLE, RGN_MUTATE_PARITY_OFF1, RGN_MUTATE_EMIT_NOCHANGE) satisfy Rule 6. Ratification locks the already-implemented, mutation-proven engine and spec/region_algebra.h as the binding contract (5/26 of epic k8o5 complete).

**DEC-03 -- OQ-1 RESOLVED -- FLAIR heap = dedicated extended-memory region 0x100000..0x500000 (4 MiB), fixed window with fail-loud probe.**
DEDICATED HIGH REGION in extended memory, NOT the per-program MCB arena (which is rebound on every EXEC -- loader.c:450 -- and would destroy GUI state). FLAIR_HEAP_BASE=0x00100000, FLAIR_HEAP_SIZE=0x00400000 (4 MiB), window [0x100000,0x500000), FLAIR_HEAP_MIN=0x00400000. Window FIXED + spec-locked (Rule 11 determinism); stage2 PROBES installed RAM via INT 15h E820/E801/88h into a new boot_info_t.ext_mem_kb field and the kernel PANICs loud (Rule 2) if probed RAM < FLAIR_HEAP_MIN -- the probe gates boot but never alters the map. No LFB collision (lfb_addr is the VBE PhysBasePtr aperture or 0xA0000 fallback; neither in [0x100000,0x500000), verified in stage2.asm). Allocator: one FLAIR-owned flat arena, bump + typed free-list, no per-row malloc, fail-loud on exhaustion (PRD Sec 5). Executed as a single beads-tracked Rule 8 act touching exactly boot_info.h, spec/memory_map.h, spec/hardware.json; no existing memory_map.h constant changes.

*DEC-03 full ruling (base / size / probe / allocator / spec-impact):*

- *Ruling.* FLAIR allocates from a DEDICATED high region in extended memory, NOT the per-program MCB arena. Concrete: FLAIR_HEAP_BASE = 0x00100000 (first byte above 1 MiB; directly addressable in 32-bit flat protected mode per ADR-0001, no A20/segment/XMS gymnastics). FLAIR_HEAP_SIZE = 0x00400000 (4 MiB). Window = [0x00100000, 0x00500000). The window is FIXED and spec-locked (deterministic layout, Rule 11), NOT a runtime-computed base. Detect: stage2 PROBES installed extended memory via INT 15h E820 (E801 then AH=88h fallbacks for period hardware) and records the result in a NEW boot_info_t field (uint32_t ext_mem_kb); the kernel PANICs LOUD ('PC LOAD LETTER', Rule 2) at boot if probed RAM < FLAIR_HEAP_MIN (= FLAIR_HEAP_SIZE = 0x00400000). The probe result NEVER alters the memory map -- it only gates boot -- so the layout stays deterministic while the OS refuses to run on under-resourced hardware instead of scribbling into RAM that is not there.
- *Rationale.* Unanimous across all four reviewers on the core ruling (dedicated high region) and on base = 0x100000. MCB arena is disqualified on independent, verified grounds: (1) it is PER-PROGRAM and REBOUND on every EXEC -- loader.c:450 int21_mcb_bind_program(plan.arena_base, plan.arena_ceil) -- so GUI state (WindowRecords, z-order, visRgn/clipRgn, save-unders, EventRecord queues) would be destroyed on every app launch; GUI state MUST persist across launches, so coupling it to the DOS arena is a deep correctness bug (Rule 3), not a trade-off. (2) The MCB ceiling is PROGRAM_ARENA_CEIL == ENV_BLOCK (0x5F000) -- the arena is already tight for SAMIR. (3) Conventional memory 0x10000..0x80000 is fully allocated per spec/memory_map.h; the 0x90000..0xA0000 gap is ~64 KiB while a single indexed-8 640x480 offscreen is 307,200 bytes -- the gap cannot hold even one bitmap, and FLAIR needs several (backbuffer + save-unders + region pools + NFNT strikes) simultaneously. Extended memory above 1 MiB is the only viable home and is period-authentic for a 386 flat OS (the HIMEM/XMS-era hardware contract; OS/2 1.x, 386BSD, early Linux all kept kernel heaps above 1 MiB). NO collision with the LFB: verified in os/boot/stage2.asm -- lfb_addr is either the VBE PhysBasePtr (a PCI aperture far above 0xE0000000 on QEMU) or 0x000A0000 (the mode-0x13 VGA fallback on Bochs); neither lands in [0x100000, 0x500000). SIZE: I adopt the Fidelity Steward's 4 MiB over the 2 MiB of Bolton/Nagheenanajar -- the M3-M5 profile (multiple ~300 KiB offscreens + region pools at RGN_ROWS_CAP/RGN_X_POOL_CAP + handle tables + strikes) makes 2 MiB tight with no margin, while 4 MiB clears the LFB by a wide margin on the 386+ target at zero cost. DETECT-vs-FIXED: I adopt Smykowski's fail-loud probe as the strictly stronger synthesis. The Bolton/Nagheenanajar/Steward 'no probe, assume RAM' variant violates Law 1 / Rule 2 (it assumes >= 5 MiB installed without ever checking; on a 4 MiB-or-less machine the FLAIR heap silently runs into non-existent RAM). Keeping the WINDOW fixed preserves Rule 11 determinism (the layout is identical every boot; the self-host fixpoint K2==K3 is unaffected); adding the probe only makes the OS fail loud on under-provisioning. boot_info.h confirmed to carry no extended-memory field today (only LFB geometry + font_addr), so the field is a genuine, deliberate addition.
- *Spec impact.* ONE beads-tracked Rule 8 act touches exactly THREE locked files (no other constant changes): (1) os/milton/boot_info.h -- add `uint32_t ext_mem_kb;` to boot_info_t (extends the handoff block; update the byte-count comment and stage2's BOOT_INFO_ADDR field writes). (2) spec/memory_map.h -- add FLAIR_HEAP_BASE (0x00100000), FLAIR_HEAP_SIZE (0x00400000), FLAIR_HEAP_MIN (0x00400000), with a commentary block citing ADR-0001 (flat 32-bit direct addressability), the no-LFB-collision proof (stage2.asm lfb_addr paths), and the rebound-MCB-arena disqualifier (loader.c int21_mcb_bind_program). NO existing constant (PROGRAM_BASE/IMAGE/ENV_BLOCK/STACK_*/ARENA_CEIL/LOAD_STAGING_*) changes -- the conventional map and arena disjointness invariant are untouched. (3) spec/hardware.json -- add a flair_heap block under 'memory' recording base/size/min and the E820/E801/88h probe + fail-loud-below-min contract; add a provenance row; the existing mutation-proven test_hardware_spec.c gate covers it. stage2.asm also gains the INT 15h probe + the ext_mem_kb write (artifact code change tracked by the same bead, not a locked-spec file).
- *Allocator.* A single flat arena owned by FLAIR over the fixed [FLAIR_HEAP_BASE, FLAIR_HEAP_BASE+FLAIR_HEAP_SIZE) window: bump-pointer for allocation, typed free-list per allocation class (region rows[]/x_pool backing, indexed-8 offscreen bitmaps, WindowRecord/MenuInfo/ControlRecord/DialogRecord handles, NFNT strikes) for frees -- matching PRD Sec 5 'bump + free-list allocator'. NO per-row / per-handle malloc (Rule 11 deterministic layout; freestanding, no libc). Fail-loud (Rule 2) on exhaustion -- never silent truncation. ADR-0005's region rep is allocator-agnostic (caller-supplied storage), so this arena backs it with no change to the engine; OQ-1 does not block ATKINSON, which is already green.

**DEC-04 -- OQ-2 RESOLVED -- DEFER 86Box pixel-capture funding; funded follow-up gates M4.**
DEFER (unanimous). Cross-emulator framebuffer agreement runs now against wired QEMU + Bochs via the host-model-per-mode definition (each emulator vs its own host model, not a cross-emulator byte-CRC) -- a real, un-weakened oracle. The hard structural gates (test-region green, test-chrome, test-event, fb-agree, canon) do not need 86Box; SSIM-as-guide surfaces any DAC/palette drift early. File a FUNDED follow-up beads issue now -- 86Box + period Cirrus/ET4000 BIOS, headless capture, host-model DAC calibration, and a mutation proof the 86Box fb-agree arm goes RED on real divergence -- and BLOCK M4 sign-off on it so the period-authenticity leg lands before the chrome/window fidelity bar is locked and apps are frozen. Not a pre-ratification blocker; the 86Box leg of Rule 5's tri-emulator vector is pending, not abandoned.

*DEC-04 full ruling.* DEFER. Do NOT fund 86Box period-VGA pixel capture now. Run cross-emulator framebuffer agreement (D-8) against the currently-wired QEMU + Bochs using the host-model-per-mode definition (each emulator vs its own host model's prediction for its own mode). File a FUNDED follow-up beads issue NOW, scoped to: (1) 86Box + a period Cirrus CL-GD5422 / ET4000-W32 VGA BIOS setup and headless screendump automation; (2) host-model calibration for 86Box's specific DAC/palette ramp; (3) a mutation proof that the 86Box arm of fb-agree can go RED on a real divergence. The follow-up issue MUST be filed and MUST block M4 sign-off (the first full-desktop chrome milestone), so 86Box coverage lands before the fidelity bar is locked and before bundled apps are frozen -- it is NOT a pre-ratification blocker.

### 8.2 Amendments (AM-1 .. AM-9) -- carried by DEC-01

**AM-1 (D-8 host-model parameterization, Bolton):** The cross-emulator agreement model in D-8 must explicitly state that the host render skeleton is parameterized by the RUNTIME boot_info_t (lfb_addr, lfb_pitch, lfb_bpp, lfb_width, lfb_height as reported by VBE PhysBasePtr or the 0xA0000 VGA fallback) -- NOT any hardcoded aperture. Verified against os/boot/stage2.asm: lfb_addr is the VBE PhysBasePtr (offset 0x28) on QEMU and 0x000A0000 on the Bochs mode-0x13 fallback; the oracle MUST consume the same boot_info_t the kernel does so emulator and host model predict the same physical address. Stated as a forward obligation gating the host render skeleton (initech-k8o5.7).

**AM-2 (FO-1 sequencing, Bolton + Fidelity Steward):** FO-1 (surface-module extraction from console.c) and a GREEN, MUTATION-PROVEN fb-agree gate MUST precede ALL Manager code -- the one-surface invariant (D-2/C-2) is unenforceable until fb-agree bites. At least one named fb-agree mutant must be recorded in the ratified text (e.g. route a shared span through a second pixel path; assert the gate goes RED), matching the ADR-0005 RGN_MUTATE_* discipline (Smykowski C-7 gap).

**AM-3 (FO-2 strengthened, Fidelity Steward + Smykowski):** FO-2 is hardened from 'chrome_metrics.json v1 locked or confirmed locked' to: chrome_metrics.json v1 is LOCKED AND test-chrome is MUTATION-PROVEN (three named mutants, Rule 6) at the SAME gate as FO-1, before ANY Window/Control Manager drawing code ships. A locked metric with an unproven oracle lets plausible chrome through.

**AM-4 (canon as frozen data, Nagheenanajar):** D-3's Photoshop menu bar (File Edit Image Layer Select View Window Help) and D-7's hourglass cursor must be FROZEN locked-data (named spec/assets/ files or named locked constants), not prose an agent may silently 'correct.' Either confirm spec/assets/ contains the CURS bytes and the menu-bar string and name the files, or record each as a Forward Obligation with a beads issue. The canon oracle (D-8) gates these frozen bytes.

**AM-5 (FO-5 new, Smykowski governance):** Add FO-5 to Sec 5.2: the OQ-1 resolution (DEC-03 below) is executed as ONE deliberate, beads-tracked Rule 8 act touching exactly three locked files -- boot_info.h (new ext_mem_kb field), spec/memory_map.h (FLAIR_HEAP_BASE/SIZE/MIN constants), spec/hardware.json (memory section). No FLAIR allocator .c may be written before that beads issue is open and linked. No silent edits.

**AM-6 (D-8 definition frozen, Smykowski + Fidelity Steward):** The 'each emulator vs its own host-model prediction, NOT a cross-emulator byte-CRC' definition in D-8/Sec 4.5 is preserved verbatim and may only be changed by a deliberate ADR amendment -- a future agent must not be able to silently revert to a naive cross-emulator byte-CRC or pin to QEMU (a Stop condition).

**AM-7 (Law 1 source citations, Nagheenanajar):** D-7 must cite a local source for the dual-PIC EOI sequence (8259A / IBM PC AT Technical Reference for the IRQ2-cascade / IRQ12 slave-EOI-then-master-EOI order); OD-2's indexed-8 rationale must note its period grounding (8-bpp VGA/SVGA offscreen model -- Mode 13h, VBE 0x101/0x103 -- as the authentic backing-store depth of the era), not merely 'operator decided.'

**AM-8 (M3 drag-gate named, Fidelity Steward):** Add the M3 window-drag gate (beads initech-87a) as an explicit hard pass/fail row in the D-8 oracle vector: 'drag-gate -- a window drags across the desktop with correct DiffRgn update regions, no over-repaint, chrome unchanged outside the damaged area, verified against chrome_metrics v1 fixture crops.' This is the earliest human-verifiable Law-4 fidelity moment and belongs in the ADR, not only in beads.

**AM-9 (FO-3 palette gate, Smykowski):** FO-3 (reconcile palette.json desktop_bg v0 gray -> SEAFOAM) must name an oracle: the follow-up issue includes a test asserting the palette.json canonical desktop_bg entry equals the live boot/oracle SEAFOAM value (0x6F,0xA0,0x8E), not a visual check.

### 8.3 Recorded Dissent

- **D-DISSENT-1 (carried forward from Section 6, unresolved by ratification):** one reviewing function held the canonical internal offscreen depth should be 32-bit direct color (simpler blits, no palette management, closer to the modern LFB) rather than the operator-ratified indexed-8 (OD-2). The operator ratified indexed-8 as period-authentic (8-bpp VGA/SVGA era; smaller offscreens; matches the palettized seafoam/chrome). Recorded so a future depth-related friction (e.g. a 32-bpp-only LFB mode) re-opens an informed trade-off rather than rediscovering it. The committee did NOT re-open OD-2; Nagheenanajar (Heritage) affirms indexed-8 is the period-correct choice.
- **OQ-1 minority on heap size (Bolton, Nagheenanajar):** both proposed a 2 MiB window (0x100000..0x300000) rather than the ratified 4 MiB. Chair sided with the 4 MiB of the Fidelity Steward for M3-M5 offscreen + region-pool + strike + handle-table headroom at zero cost on the 386+ target. Recorded; if the hardware-minimum contract becomes a friction the size can be revisited as a Rule 8 act.
- **OQ-1 minority on detect (Bolton, Nagheenanajar, Fidelity Steward):** three reviewers proposed NO boot-time probe (fixed window, assume RAM present, validate later as a Forward Obligation). Chair adopted Smykowski's fail-loud INT 15h probe + boot_info ext_mem_kb + panic-below-min as the strictly stronger position (Law 1 / Rule 2: do not assume RAM that may not be installed), while keeping the WINDOW fixed so Rule 11 determinism is preserved. Recorded as a deliberate chair override of the 3-1 reviewer lean on this sub-point, justified because the no-probe variant silently runs the FLAIR heap into possibly-absent RAM.

### 8.4 Forward Obligations (FO-A .. FO-G)

- **FO-A (DEC-03 / AM-5, beads initech-k8o5.5):** Open ONE beads issue for the OQ-1 Rule 8 locked-spec act before any FLAIR allocator .c is written; it edits exactly boot_info.h (add ext_mem_kb), spec/memory_map.h (add FLAIR_HEAP_BASE=0x100000 / FLAIR_HEAP_SIZE=0x400000 / FLAIR_HEAP_MIN=0x400000 + commentary), spec/hardware.json (add flair_heap block + provenance row), and adds the stage2 INT 15h E820/E801/88h probe + kernel panic-below-min. No silent edits; link the issue in ADR-0004 Sec 5.2 as FO-5. This UNBLOCKS the FLAIR heap allocator bead and all Manager/offscreen-bitmap work in epic k8o5 (M4).
- **FO-B (DEC-01 / AM-2, AM-3, beads initech-k8o5.6 + initech-k8o5.8):** The surface-module extraction from console.c (initech-k8o5 FO-1) with a GREEN, MUTATION-PROVEN fb-agree gate AND a mutation-proven test-chrome (chrome_metrics.json v1 locked) MUST be green before ANY Window/Control/Menu/Dialog Manager drawing code lands. Record one named fb-agree mutant in the ADR text. UNBLOCKS Manager work (layer 3).
- **FO-C (DEC-01 / AM-1, beads initech-k8o5.7):** Before wiring the host render skeleton (initech-k8o5.7), state and implement that the host model is parameterized by the runtime boot_info_t LFB geometry (VBE PhysBasePtr or 0xA0000 fallback), never a hardcoded aperture. Gates the D-8 cross-emulator/fb-agree oracle correctness.
- **FO-D (DEC-04, beads initech-q0gy):** File the FUNDED 86Box pixel-capture follow-up beads issue now (86Box + period Cirrus/ET4000 BIOS, headless capture, host-model DAC calibration, mutation proof the 86Box fb-agree arm goes RED). Mark it BLOCKING for M4 sign-off so the period-authenticity emulator leg lands before the chrome/window fidelity bar is locked and bundled apps are frozen.
- **FO-E (DEC-01 / AM-4, AM-7, beads initech-zaqj):** Confirm-or-create locked-data + beads issues for the canon assets (the hourglass CURS bytes and the 'File Edit Image Layer Select View Window Help' menu-bar string as named spec/assets/ files or locked constants), and add the Law-1 source citations to D-7 (8259A/IBM AT EOI sequence) and OD-2 (period 8-bpp offscreen grounding).
- **FO-F (DEC-01 / AM-9, beads initech-ch81):** The palette follow-up (FO-3, reconcile palette.json desktop_bg v0 gray -> SEAFOAM 0x6F,0xA0,0x8E) must include an oracle asserting the palette.json canonical entry equals the live boot/oracle SEAFOAM value -- not a visual check.
- **FO-G (DEC-03, beads initech-k8o5.5):** When stage2 reports ext_mem_kb, the kernel validates installed RAM >= FLAIR_HEAP_BASE+FLAIR_HEAP_SIZE at boot and panics loud (PC LOAD LETTER) below FLAIR_HEAP_MIN; this fail-loud path itself must be covered (a test/mutant confirming the panic fires on under-provisioned RAM).

---

## 9. Related Decisions and References

- ADR-0001 — 386+, 32-bit flat (the platform FLAIR draws on).
- ADR-0002 — Toolchain / implementation language (C) / executable format.
- ADR-0003 — InitechDOS base OS (MILTON), incl. the MCB arena (OQ-1) and `console.c` (the surface module's origin, D-2); Amendments DEC-04a (INT 21h convention), DEC-14 (user-pointer validation).
- **ADR-0005 — ATKINSON Region Engine** (companion, RATIFIED 2026-06-19, DEC-02): layer 2 of this stack; the locked `spec/region_algebra.h`.
- ADR-0007 — Turbo Initech (pending); a FLAIR tenant, not part of FLAIR.
- CDR-0001 — interim toolchain deviation.
- PRD §2 (non-goals: cooperative, no isolation), §3 (fidelity bar / SSIM guide), §4 (architecture), §6.2 (region engine), §6.3 (Toolbox + oracle), §6.4 (fonts/assets), §6.5 (apps / FILE COPY), §8 (oracle infra / gate vector), §9 (swarm discipline), §12 (Toolbox-sprawl risk), §15 (cooperative DECIDED), Appendix A (the frame), Appendix B (comedic constants / panic).
- Locked spec-data: `spec/region_algebra.h`, `spec/chrome_metrics.json`, `spec/assets/`.

<!-- END OEA-ADR-0004 (RATIFIED) — INITECH CONFIDENTIAL -->
