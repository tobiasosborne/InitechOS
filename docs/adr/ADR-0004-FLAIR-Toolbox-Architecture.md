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
| Version | 0.1 (Draft) |
| Status | **DRAFT / Proposed (pending operator ratification)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme |
| Effective Date | (not yet effective — DRAFT) |
| Next Scheduled Review | Upon operator ratification, per RECORDS-POL-002 |
| Supersedes | (none) |
| Superseded By | (none) |
| Related Documents | ADR-0001 (386+, 32-bit flat); ADR-0002 (toolchain / impl language / exec format); ADR-0003 (InitechDOS base OS) + Amendments DEC-04a, DEC-14; ADR-0005 (ATKINSON region engine — companion, DRAFT); CDR-0001 (interim toolchain) |
| Related Issues | beads initech-jmo, initech-b5g, initech-6dy (region engine); initech-i50 (blitter w/ region clip); initech-87a (window drag w/ clip); initech-f8v.4 (tracer keystone) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | (draft) | Architecture Review Board, STAPLER Programme | Initial draft. Records the 5-layer FLAIR stack, the GrafPort/surface single-pixel-path decision, the Manager decomposition (verbatim Inside Macintosh records/part-codes), the ISR-enqueue-only event model, the region-difference damage model, cooperative WaitNextEvent, input/fonts/chrome, and the layered GUI oracle vector. Folds the operator-ratified session decisions (indexed-8 depth, 640x480, seafoam kept); records the canonical-depth dissent and the two still-open questions. | (pending committee review) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Architecture Review Board, STAPLER Programme | Submitted (DRAFT) | (draft) |
| ARB Reviewer — Technical Correctness | M. Bolton (Senior Engineer, Platform) | Pending | — |
| ARB Reviewer — Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Pending | — |
| ARB Reviewer — Governance & Compliance | T. Smykowski (QA / Change Advisory) | Pending | — |
| ARB Chair (Synthesis) | (to be assigned) | Pending | — |
| Operator Ratification | T. Osborne (Operator) | **Required — not yet granted** | — |
| Records Management | M. Waddams (Archive Annex B) | Pending filing | — |

*Note on status: This ADR is DRAFT / Proposed. The four operator-ratified decisions recorded in §3.0 are settled and are folded in as binding inputs; the remainder of the architecture (Manager decomposition, damage model, oracle vector) is proposed and awaits committee review and operator ratification of this ADR as a whole. The two items in §7 (Open Questions) are explicitly NOT decided herein.*

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
| OD-2 | **Canonical internal offscreen depth = INDEXED-8.** | The surface module's offscreen bitmaps are 8-bit palettized. Affects `grafport.h`/imaging, **not** `region_algebra.h` directly (regions are pure int16-coordinate pixel sets, depth-agnostic). |
| OD-3 | **Native target resolution = 640x480.** | Region int16 coordinates are framebuffer-bounded; 640x480 << 32767, so no coordinate can collide with any in-band magic (and ADR-0005 carries no in-band sentinel anyway). |
| OD-4 | **`desktop_bg` canon = KEEP SEAFOAM teal `(0x6F,0xA0,0x8E)`.** | The live `tools/ppm_seafoam_check.c` oracle (mirroring `os/boot/stage2.asm` `SEAFOAM_R/G/B`) stands as the desktop-background gate. (Note: `spec/assets/palette.json` records a separate `desktop_bg` "ground-truth v0" gray sampled from the compressed still; the SEAFOAM boot/oracle value is the canon FLAIR paints. Reconciling the palette-v0 record to seafoam is a `palette` follow-up, not a FLAIR blocker.) |

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

- **Mouse**: PS/2 mouse on **IRQ12**, requiring a **dual-PIC EOI** (slave then master) and a **bounded spin** on the 8042 status register (never an unbounded poll — Rule 2). The bring-up sequence is **Bochs-verified** (strict real->protected and PIC accuracy), not QEMU-only (Rule 5, Stop conditions).
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
| `canon` | hard pass/fail | The enforced canon: hourglass-not-wristwatch cursor bytes; the Photoshop menu bar; seafoam desktop; (with the apps) the 116% pie and `570-` format. |
| `ssim` | **GUIDE ONLY — never gates** | Per-window structural similarity vs the frame fixture; reported to point agents toward fidelity (Law 4, PRD §3, §8). Structurally a guide; it is NOT summed into a reward and never blocks a merge. |

**Cross-emulator agreement** (Rule 5) for the framebuffer is defined as: **each emulator's screendump digest is compared against the HOST model's prediction for THAT emulator's own mode** — NOT a cross-emulator byte-CRC. (QEMU, Bochs, and 86Box legitimately differ at the pixel level — palette ramps, VGA DAC, LFB layout — so a naive cross-emulator byte-identity gate would be wrong; the host model that predicts each one is the correct oracle, and disagreement between an emulator and its own predicted output is the bug signal.)

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

- FO-1. **Author the surface-module extraction** from `console.c` with `fb-agree` green before any Manager code lands.
- FO-2. **`chrome_metrics.json` v1** must be locked (or confirmed locked) before the Window/Control Managers consume it.
- FO-3. **Reconcile `palette.json` `desktop_bg` v0 (gray) to the SEAFOAM canon** (OD-4) as a `palette` follow-up issue, so the locked palette and the live boot/oracle value agree.
- FO-4. When **ring-3 / app isolation** is ever revisited (currently a non-goal, PRD §2), the cooperative model (D-6) and the ISR/event boundary (D-4) must be re-examined; out of scope here.

### 5.3 Neutral Consequences

- The indexed-8 offscreen depth (OD-2) localizes all depth conversion to the surface module; higher layers are depth-agnostic, and the region engine is entirely coordinate-only.

---

## 6. Recorded Dissent

For the record (this is a real architectural split, not unanimous):

- **D-DISSENT-1 — Canonical-depth split.** One reviewing function held that the canonical internal offscreen depth should be **32-bit direct color** (simpler blits, no palette management, closer to the modern LFB), against the ratified **indexed-8** (OD-2; period-authentic, smaller offscreens, matches the 8-bpp era and the palettized seafoam/chrome). The operator ratified **indexed-8**; the dissent is recorded so that if a future depth-related friction surfaces (e.g., 32-bpp-only LFB modes), the trade-off is already on record and re-opening it is informed, not rediscovered.

---

## 7. Open Questions (NOT decided herein)

Recorded as open per the operator; this ADR does **not** resolve them.

- **OQ-1 — FLAIR Toolbox heap home.** Where does the Toolbox allocate its handles/records/region pools — a **new high region** carved above the kernel/program windows, or **inside the MCB arena** (the existing AH=48h/49h/4Ah allocator, ADR-0003 / initech-509.6)? Trade-offs: a dedicated high region keeps Toolbox allocation off the DOS arena (no fragmentation interplay with loaded programs) but adds a second allocator; the MCB arena reuses proven, oracle-covered code but couples GUI lifetime to DOS memory semantics. **Deferred to a dedicated decision.**
- **OQ-2 — Real-Bochs / 86Box pixel-capture funding.** The chrome/fidelity oracles ideally capture real-Bochs and 86Box framebuffers (not only QEMU) for cross-emulator agreement (D-8). Standing up reliable 86Box + period-VGA-BIOS pixel capture is an unfunded effort; until it is funded, cross-emulator agreement runs against the emulators currently wired. **Open — funding/effort decision.**

---

## 8. Related Decisions and References

- ADR-0001 — 386+, 32-bit flat (the platform FLAIR draws on).
- ADR-0002 — Toolchain / implementation language (C) / executable format.
- ADR-0003 — InitechDOS base OS (MILTON), incl. the MCB arena (OQ-1) and `console.c` (the surface module's origin, D-2); Amendments DEC-04a (INT 21h convention), DEC-14 (user-pointer validation).
- **ADR-0005 — ATKINSON Region Engine** (companion, DRAFT): layer 2 of this stack; the locked `spec/region_algebra.h`.
- ADR-0007 — Turbo Initech (pending); a FLAIR tenant, not part of FLAIR.
- CDR-0001 — interim toolchain deviation.
- PRD §2 (non-goals: cooperative, no isolation), §3 (fidelity bar / SSIM guide), §4 (architecture), §6.2 (region engine), §6.3 (Toolbox + oracle), §6.4 (fonts/assets), §6.5 (apps / FILE COPY), §8 (oracle infra / gate vector), §9 (swarm discipline), §12 (Toolbox-sprawl risk), §15 (cooperative DECIDED), Appendix A (the frame), Appendix B (comedic constants / panic).
- Locked spec-data: `spec/region_algebra.h`, `spec/chrome_metrics.json`, `spec/assets/`.

<!-- END OEA-ADR-0004 (DRAFT) — INITECH CONFIDENTIAL -->
