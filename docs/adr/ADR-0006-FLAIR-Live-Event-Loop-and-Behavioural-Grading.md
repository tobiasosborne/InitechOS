<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0006 -- FLAIR Live Event Loop and Behavioural Grading (the booted cooperative WaitNextEvent desktop)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0006 |
| Title | ADR-0006: FLAIR Live Event Loop and Behavioural Grading |
| Version | 1.0 (Ratified) |
| Status | **RATIFIED (ADR-by-committee + chief-architect reconciliation, operator-delegated authority, 2026-06-21; operator-ratified 2026-06-22)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme (Committee E -- Behaviour / Event-Loop) |
| Effective Date | 2026-06-21 |
| Next Scheduled Review | Upon operator final confirmation, per RECORDS-POL-002 |
| Supersedes | (none) |
| Superseded By | (none) |
| Related Documents | ADR-0004 (FLAIR Toolbox Architecture) + Amendment DEC-09 (Mechanism/Policy Split) + D-9 era-layering amendment; ADR-0005 (ATKINSON Region Engine) + Amendment AM-1 (Dual-Heritage Region Spine); ADR-0010 (FLAIR Grading and Goldens); ADR-0001 (386+, 32-bit flat); ADR-0003 (InitechDOS base OS) + Amendment DEC-04a (INT 21h / IDT-PIC vector map); REVOCATION-RECORD-2026-06-21 (FLAIR Heresy Purge) |
| Related Issues | beads initech-q0gy (86Box / tri-emulator funded follow-up); the re30.8 (tick) -> re30.9 (keyboard) -> mouse-IRQ12 -> raw-ring -> WaitNextEvent-pump -> test-flair-drag bead chain (FO-4..FO-9) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | (draft) | Architecture Review Board, STAPLER Programme (Committee E) | Initial draft. Records the booted FLAIR cooperative WaitNextEvent loop (`flair_tick_advance` into the PIT ISR; the FLAIR raw single-producer/single-consumer ring fed by kbd IRQ1 + mouse IRQ12 ISR-enqueue-only; the pump replacing render-once-and-HLT, behind `-DBOOT_FLAIR_LIVE`); the two behavioural oracles by KIND (`test-interact` HOST internal-geometry + `test-flair-drag` EMU two-tier), neither by-construction; the Bochs mouse leg (dual-PIC EOI); the mutant set incl. the HER-14 drag-noop; `test-skin-teal` + `test-skin-era-frozen` + `check-win95isms`; and the forward gaps (default-boot flip, 86Box, SSIM). | (pending committee review) |
| 1.0 | 2026-06-21 | ARB Chair (synthesis) + Chief Architect (reconciliation) | Ratified by ADR-by-committee and folded into the cross-committee chief-architect reconciliation (VERDICT: COMPOSES; no module fork; zero by-construction value oracles). Records E-D1..E-D8 as ratified. All ground-truth claims verified against the live tree and the sibling decomp repos. | ARB (Bolton/Nagheenanajar/Smykowski + Fidelity Steward) + Chair |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Architecture Review Board, STAPLER Programme (Committee E) | Submitted (DRAFT) | (draft) |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Approved (2026-06-21) | 2026-06-21 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Approved (2026-06-21) | 2026-06-21 |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Approved (2026-06-21) | 2026-06-21 |
| ARB Reviewer -- Fidelity Steward | Fidelity Steward (Heritage Conformance) | Approved (2026-06-21) | 2026-06-21 |
| ARB Chair (Synthesis) | ADR-by-committee Chair | Synthesized + Approved (2026-06-21) | 2026-06-21 |
| Chief Architect (Reconciliation) | Chief Architect, STAPLER Programme | Reconciled + VERIFIED-COMPOSES (2026-06-21) | 2026-06-21 |
| Operator Ratification | T. Osborne (Operator) | **Granted via delegated committee authority (2026-06-21; operator-ratified 2026-06-22)** | 2026-06-21 |
| Records Management | M. Waddams (Archive Annex B) | Filed (2026-06-21) | 2026-06-21 |

*Note on status: This ADR is RATIFIED by ADR-by-committee under operator-delegated authority and reconciled by the chief-architect cross-committee composition (which returned COMPOSES, module-fork-resolved, with an empty by-construction-oracles-found set). It is pending the operator's final confirmation. Decisions E-D1..E-D8 below are settled and binding. This ADR is a downstream consumer of the ADR-0004 Amendment DEC-09 / ADR-0010 color-canon arbitration and the ADR-0005 Amendment AM-1 region spine; those preconditions are recorded as Forward Obligations FO-1 and FO-2 and gate this deliverable's landing sequence.*

---

## 1. Purpose and Scope

### 1.1 Purpose

This ADR establishes the architecture by which **FLAIR becomes a live, interactive desktop on the 386** -- the booted, cooperative `WaitNextEvent` event loop that drives the already-ratified FLAIR Managers (Window / Menu / Control / Event / Dialog; ADR-0004 Sec 3.3) from real hardware interrupts, and the **behavioural grading regime** that proves the booted desktop actually drags, tracks menus, and re-orders windows rather than merely rendering a static frame dressed up as interactivity.

The motivating defect is recorded in the **REVOCATION-RECORD-2026-06-21 (FLAIR Heresy Purge)** as **HER-14**: the FLAIR `WaitNextEvent` spine was ratified (ADR-0004 Sec 3.6) but never wired to a live interrupt source and never had an emulator-level behavioural gate, so the "interactive desktop" was, on metal, a single static present followed by `cli; hlt`. Verified against the live tree: `flair_tick_advance` and `flair_raw_post` have ZERO ISR callers (`os/flair/event.c:98,168`); `kmain` renders once then halts (`os/milton/kmain.c:333,680`, with `flair_desktop_present` at `os/milton/kmain.c:711`). This ADR closes HER-14 by (a) wiring the existing spine to live interrupts behind a build flag and (b) grading the result with an oracle that no static frame can pass.

### 1.2 Scope

This ADR governs:

- The **live cooperative event loop on the 386**: the `WaitNextEvent` pump that replaces render-once-and-HLT, behind `-DBOOT_FLAIR_LIVE`, driving the ALREADY-BUILT Managers (E-D2, E-D4). (Sec 3, E-D2/E-D4)
- The **three ISR raw-input producers**: PIT IRQ0 calling `flair_tick_advance()`; kbd IRQ1 posting the raw scancode; the new PS/2 aux mouse IRQ12 ISR -- all ISR-enqueue-only per ADR-0004 D-4, additive, and build-gated (E-D3). (Sec 3, E-D3)
- The **FLAIR raw input ring**: the single-producer/single-consumer (SPSC) ring fed by the three ISRs, drained by the pump in task context. (Sec 3, E-D3/E-D4)
- The **two behavioural oracles by KIND**, NEITHER by-construction: `test-interact` (HOST, internal geometry, independent golden by recomputation) and `test-flair-drag` (EMU, two tiers: damage-law + independent WDEF geometry golden / menu contour) (E-D5). (Sec 3, E-D5; Sec 4)
- The **Bochs mouse leg** verifying the dual-PIC EOI sequence on the IRQ12 path under strict PIC accuracy (E-D3/E-D6, Rule 5). (Sec 3; Sec 4)
- The **mutant set** that mutation-proves the behavioural gate, including the HER-14 drag-noop mutant (M1) and the mandatory geometry VALUE mutant (M3). (Sec 4)
- The **FROZEN can-golden / cannot-golden boundary** (E-D8): the observable end-state is decomp-gradable; the internal minimal-damage update region is NOT, and a future agent is forbidden from strengthening the emu gate by diffing intermediate frames. (Sec 3, E-D8)
- The **era/skin behavioural oracles** consumed alongside the loop: `test-skin-teal`, `test-skin-era-frozen`, and the `check-win95isms` era-boundary guardrail (companions to the ADR-0004 D-9 era-layering amendment). (Sec 4)

### 1.3 Out of Scope

- The **ADR-0004 D-9 / D-9b era-layering decision itself** (the data-only `flair_skin_t` registry view, the Win-3.1/Photoshop peer skin, the GDI-facade-load-bearing menu clip). That decision is recorded in **ADR-0004 Amendment DEC-09** (the mechanism/policy split, the one canon module) and the ADR-0004 D-9 amendment block; this ADR consumes it and grades the behavioural consequences only.
- The **one color-canon module** (`spec/assets/color_canon.json` -> generated `spec/assets/color_canon.h` exposing `flair_canon_rgb(uint8_t idx)`). That is owned by **ADR-0004 Amendment DEC-09** (the P1 mechanism/policy split and canon arbitration) with grading owned by **ADR-0010 (FLAIR Grading and Goldens)**. This ADR is a downstream CONSUMER: E-D5 Tier-A reads `idx2` Initech teal `#8DDCDC` from the canon. It is a HARD PRECONDITION (FO-2), not a deliverable of this ADR.
- The **GDI peer-region family and the ATKINSON dual-heritage spine** (`CombineRgn` and friends, the `RectInRgn` containment-to-overlap fix). Owned by **ADR-0005 Amendment AM-1 (Dual-Heritage Region Spine)**; a hard precondition (FO-1).
- The **`ppm_flair_check` re-key** off the by-construction grader. Owned by the grading committee under **ADR-0010**; recorded here only as a cross-committee dependency (FO-12), explicitly NOT a five-way merge.
- The **default-boot flip, the 86Box leg, and SSIM** -- recorded as named Forward Gaps (Sec 5, FO-GAP-A/B/C), not delivered here.

### 1.4 Additional Defined Terms

| Term | Definition |
|---|---|
| Live loop | The booted FLAIR cooperative `WaitNextEvent` pump that replaces render-once-and-HLT, behind `-DBOOT_FLAIR_LIVE`. The mechanism is the same `window.c`/`event.c`/`desktop.c`/`menu.c` symbols the host suites mutation-prove; only the input source (ISR ring vs injected ring) and output sink (live LFB vs offscreen PPM) differ. |
| FLAIR raw ring | The single-producer/single-consumer ring of `flair_raw_event_t` records (`spec/event_model.h`) posted by the three ISRs and drained by the pump. ISRs post raw; `EventRecord` synthesis happens in task context (ADR-0004 D-4). |
| `flair_tick_advance` | The task-time-base advance the PIT ISR calls (additive; `g_ticks++` untouched), enabling a bounded, deterministic `WaitNextEvent` and the cooperative HLT yield. |
| Damage law | The observable invariant graded by `test-flair-drag` Tier-A: after a drag, chrome appears at the new structure rect; the old-minus-new exposed area reads bare Initech teal `#8DDCDC`; every pixel outside (old structure UNION new structure) is byte-identical to the pre-drag dump (no over-repaint; ADR-0004 D-5). |
| WDEF geometry scan | `wdef0_geometry_scan.txt`, a binary 68K opcode decode of `WDEF_0.bin`, INDEPENDENT of the `StandardWDEF_a.html` listing that `spec/chrome_metrics.h` cites. The Tier-B independent VALUE golden for chrome geometry. |
| Two oracles by KIND | The deliberate split: a HOST property test for the ungoldenable internal geometry (`test-interact`, independent-by-recomputation) and an EMU gate for the observable look (`test-flair-drag`, independent decomp golden). Neither is by-construction. |

---

## 2. Context

### 2.1 The verified live-tree state at decision time

All claims below were verified against the working tree on 2026-06-21:

- `flair_tick_advance` / `flair_raw_post` have ZERO ISR callers (`os/flair/event.c:98,168`) -- the cooperative spine is dead code on the artifact.
- `pit_irq_handler` does `g_ticks++` then issues the PIC EOI (`os/milton/pit.c:75,93`).
- `kmain` renders once then `cli; hlt` (`os/milton/kmain.c:333,680`); `flair_desktop_present` is at `os/milton/kmain.c:711`.
- `isr.asm` has `irq0_entry` and `irq1_entry` but NO `irq12` entry (`os/milton/isr.asm:465,481`).
- `region_op` is the lone region-engine definition (`os/flair/atkinson/region.c:330`); NO `CombineRgn`/GDI peer exists yet in `region_algebra.h` (only QuickDraw shims at `region_algebra.h:328-331`).
- `os/flair/menu.c` draws through a raw `const region_t *clip` (`os/flair/menu.c:344-355`), NOT through `CombineRgn` -- confirming the GDI facade is verified-but-unused.

The Managers themselves (`DragWindow` + `DiffRgn` damage, `desktop_paint_damage`, `WaitNextEvent`, `MenuSelect`, z-order raise, `FindWindow` part-codes) are built, ratified (ADR-0004), and host-green. The missing piece is the live caller and the live interrupt producers -- this is wiring, not a new algorithm.

### 2.2 Why a second event loop is forbidden

ADR-0004 D-2 fixed the single-pixel-path principle: exactly one surface module, no second pixel path. The behaviour dimension has the analogous trap. If `kmain` re-implemented event drain, geometry, or dispatch logic of its own, it would create a second event path untested by the host property suites -- the same D-2 / HER-12 dual-path drift, now in the behaviour dimension. Therefore the live loop must be a THIN source/sink adapter over the identical host-proven symbols (E-D2).

### 2.3 Why behaviour must be graded by KIND, and why no single golden suffices

The booted scene is a chimera: a System-7 desktop fused with a Win-3.1/Photoshop menu bar, rendered in the Initech teal canon. There is no real-System-7 capture of this scene. Two distinct grading problems follow:

1. The **internal minimal-damage update region** is not externally observable in any period capture -- the same reason ADR-0005 ratified the homomorphism suite as the entire region signal. It can only be graded against a SECOND, DIFFERENT in-host recomputation.
2. The **observable post-interaction look** (chrome at the new rect, bare teal exposed area, dropped menu panel) IS gradeable -- but the available decomp captures are not color-comparable to the teal chimera: `cal_filemenu.png` is 4-bit colormap and the System-7 move/raise captures are 8-bit grayscale.

The legacy temptation -- promote the by-construction screendump grader (`ppm_flair_check`, which admits its colors agree BY CONSTRUCTION) into the behavioural value oracle -- is exactly HER-02/HER-14 and is rejected here as a Stop condition (Sec 3, E-D5; Law 2). The honest independent VALUE golden is the WDEF geometry scan (a binary opcode decode, independent of the listing the artifact sourced), which a title-bar `19->18` mutant bites without any color comparison.

---

## 3. The Decision

The following decisions (E-D1..E-D8) are RATIFIED. Each cites its ground truth (Law 1).

### E-D1 -- Oracle-first: design the behavioural gate before the live `.c` lands

The EMU behavioural gate `test-flair-drag`, its grader `ppm_flair_behav_check.c`, and the M1..M5 mutants are AUTHORED and proven RED-against-stub / GREEN-against-wiring **BEFORE** any `-DBOOT_FLAIR_LIVE` source replaces render-once-and-HLT.

*Rationale (Law 2; Rule 1).* HER-14 exists precisely because the `WaitNextEvent` spine was ratified but never had an emulator gate -- behaviour was host-only and the spine was dead code on the artifact (`os/flair/event.c:98,168`). Designing the gate first prevents shipping another static frame dressed as interactivity. (Reconciliation landing Step 5; FO-9.)

### E-D2 -- Purist single-spine: `kmain` is a thin source/sink adapter

`kmain` runs the IDENTICAL `window.c`/`event.c`/`desktop.c`/`menu.c` symbols the host property suites mutation-prove, differing ONLY in input source (real ISR ring vs injected ring) and output sink (live LFB vs offscreen PPM). NO event/geometry logic is re-implemented in `kmain`. ISR-level posts are ADDITIVE (`g_ticks++` kept, the DOS `g_kbd` ASCII ring kept, the FLAIR raw post added); a SECOND event LOOP is forbidden.

*Rationale.* A second untested-by-host event path in `kmain` is the D-2 / HER-12 dual-path drift in the behaviour dimension. The spine already exists and is host-green; the work is wiring, not algorithm. (ADR-0004 D-2 extended to behaviour.)

### E-D3 -- Three ISR raw-input producers, ISR-enqueue-only, additive, build-gated

Three raw-input producers, all ISR-enqueue-only per ADR-0004 D-4, all additive, all behind `-DBOOT_FLAIR_LIVE`:

(a) **PIT IRQ0** ALSO calls `flair_tick_advance()` BEFORE the existing `outb(PIC1_CMD, PIC_EOI)` at `os/milton/pit.c:93`; `g_ticks++` untouched.
(b) **PS/2 aux mouse IRQ12** -- a NEW `os/milton/mouse.c` plus an `irq12_entry` in `isr.asm` cloning the `irq0`/`irq1` `pushad` + `irq_enter` + C-call + `irq_leave` + `iretd` bracket (`os/milton/isr.asm:465,481`). It reads the 3-byte PS/2 packet, posts ONE `FLAIR_RAW_MOUSE` event, and issues a **DUAL-PIC EOI: slave `0xA0` THEN master `0x20`**.
(c) **kbd IRQ1** ALSO `flair_raw_post`s the raw scancode (the DOS `g_kbd` ASCII ring is untouched, so `COMMAND.COM` / `test-shell` / `test-samir-boot` stay green).
(d) `pic_unmask_irq12()` unmasks slave IRQ12 and the master cascade IRQ2.

The ISRs do the D-4 minimum (read device, post raw, EOI, return) -- NO `EventRecord` synthesis, NO Manager call, NO allocation in interrupt context. The hourglass `CURS` is canon (NOT a wristwatch; ADR-0004 Sec 3.7; PRD Law 4).

*Rationale (Rule 5, Stop condition).* `event.c` is already written to D-4 (`flair_raw_post` IS the documented ISR producer); the only gap is that nothing calls it. IRQ12 on the slave 8259A behind the IRQ2 cascade REQUIRES a slave-then-master EOI -- a single master EOI silently wedges all further mouse IRQs on real hardware. This is Bochs-verified (E-D6), NOT QEMU-only.

### E-D4 -- The `WaitNextEvent` pump replaces render-once-and-HLT (keep the initial present)

Replace render-once-and-HLT (`os/milton/kmain.c:333,680`, `flair_desktop_present` at `os/milton/kmain.c:711`) with the `WaitNextEvent` pump driving the ALREADY-BUILT Managers:

1. Install the HLT yield via `flair_event_set_yield` BEFORE `sti`.
2. `flair_event_init` the shared ring.
3. `sti`, then loop: drain a cooked `EventRecord`; `FindWindow(where) -> part-code`; dispatch:
   - `inDrag` -> `DragWindow` + `desktop_paint_damage` per step (ADR-0004 D-5 minimal repaint).
   - `inMenuBar` -> `MenuSelect` track + hilite.
   - `inContent` -> select / raise z-order.
   - `inGoAway` -> close + expose.

KEEP the initial present so a no-input boot still shows the canon frame for the existing static screendump gate (byte-stable, Rule 11). Emit deterministic serial markers `FLAIR-LIVE-READY`, `FLAIR-DRAG-DONE`, `FLAIR-MENU-DOWN`, `FLAIR-TICK n=<count>`.

*Rationale.* The mechanism (`DragWindow` + `DiffRgn` damage, `desktop_paint_damage`, `WaitNextEvent`, `MenuSelect`) is built, ratified, and host-green; the only missing piece is the live caller. Cooperative-not-preemptive (ADR-0004 D-6): the pump holds the CPU, the ISRs only enqueue, and a non-yielding app hangs the desktop -- that is period-authentic, NOT a bug to "fix" with preemption.

### E-D5 -- Behavioural grading splits by KIND; neither leg is by-construction (the P3 keystone)

**(A) HOST `test-interact` -- BINDING for INTERNAL GEOMETRY.** Drive a scripted raw-event trace into the SPSC ring, drain via `WaitNextEvent`, and assert the post-drag / post-menu internal state -- the damaged-pixel set, z-order, hit-test routing, and exposed-region content -- against ground truth recomputed by a SECOND, DIFFERENT algorithm than the compositor (an independent front-to-back first-writer-wins owner-grid + a from-scratch `desktop_paint_all` at the final positions; the `test_drag.c` idiom extended to menu-track / z-order / hit-test). NO external golden. This is P3-LEGITIMATE because no published QuickDraw region reference exists (ADR-0005 Sec 2.2). The live binary runs these exact symbols, so it inherits this proof.

**(B) EMU `test-flair-drag` -- the OBSERVABLE LOOK, two tiers.**
- **Tier-A (the DAMAGE LAW, structural):** chrome appears at the NEW structure rect `{L+dh, T+dv}`; the OLD-minus-NEW area reads bare Initech teal `#8DDCDC` (consumed from the one canon `idx2`); a digest of every pixel OUTSIDE (old-structure UNION new-structure) is BYTE-IDENTICAL to the pre-drag dump (no over-repaint, ADR-0004 D-5); post-menu, the dropped panel rect below the 20px bar is non-desktop fill and the `File` title is hilited.
- **Tier-B (INDEPENDENT GOLDEN):** live-diff the post-drag chrome GEOMETRY (title-bar 19, frame 1, goAway inset 9, zoom -20, dBox 7) against `wdef0_geometry_scan.txt` (binary 68K opcode decode of `WDEF_0.bin`, independent of the `StandardWDEF_a.html` listing `spec/chrome_metrics.h` cites); plus a STRUCTURAL / CONTOUR diff of the post-menu drop vs `cal_filemenu.png` (NEVER byte/color).

*Rationale (Law 2; ADR-0010 P3).* `ppm_flair_check.c` admits its colors agree BY CONSTRUCTION -- the revoked HER-02 heresy. The KILLED fix is a color/byte diff of the post-gesture captures: VERIFIED that `cal_filemenu.png` is 4-bit colormap and the move/raise captures are 8-bit grayscale, not color-comparable to the teal chimera, and the scene is a two-menu-bar chimera with no System-7 analogue -- a color diff would false-positive and tempt loosening (a Stop condition). The honest independent VALUE golden is the WDEF geometry scan: a title-bar `19->18` mutant bites it without any color comparison.

### E-D6 -- EMU input injection: reuse the QMP key path, add ONE mouse verb, lock the trace

Reuse the EXISTING QMP send-key path for the keyboard half. ADD the ONE new factory capability `qmp_inject_mouse` (QMP `input-send-event` rel `InputMoveEvent` + `InputBtnEvent` down/up) to `harness/emu/qemu.c`, driven by a LOCKED scripted trace `spec/traces/flair_drag_menu.trace`. The trace fires AFTER the `FLAIR-LIVE-READY` serial marker via the existing `wait_for_serial_marker`; the screendump is taken AFTER `FLAIR-DRAG-DONE` / `FLAIR-MENU-DOWN`. The Bochs leg follows the existing `test-flair-desktop-bochs` pattern; 86Box is deferred (beads initech-q0gy).

*Rationale.* Minimal new surface: the QEMU harness already has the QMP socket, handshake, marker-trigger, send-key, screendump-after, and `wait_for_serial_marker`; ONLY the `input-send-event` mouse verb is missing. Serial-marker synchronisation keeps the trace deterministic under load (Rule 11). Pure-C harness (Law 3).

### E-D8 -- FROZEN scope boundary: the update region is ungoldenable and must stay host-bound

The observable post-interaction END-STATE is decomp-gradable (the WDEF geometry-scan VALUE golden + the menu-drop STRUCTURAL contour + the bare-teal exposed area). The internal MINIMAL-DAMAGE update region is NOT externally observable in any System-7 capture and remains bound by the HOST property test (`test-interact`). **A future agent is FORBIDDEN from strengthening the emu gate into a minimal-damage proof by diffing intermediate frames** -- the update region is not externally observable, and that claim would be a false oracle (a Stop condition).

*Rationale (ADR-0010 P3/P4 honesty).* A decomp capture is canon for the GENERIC chrome look, but the InitechOS scene is a chimera with no real-System-7 frame, and the damage-region math is provably ungoldenable -- the same reason ADR-0005 ratified the homomorphism suite as the ENTIRE region signal.

> *Note on numbering: this ADR records E-D1, E-D2, E-D3, E-D4, E-D5, E-D6, and E-D8 as ratified. The era-layering decisions that share Committee E's reconciliation deliverable (the ADR-0004 D-9 / D-9b / OD-4-supersession amendments and the associated era oracles) are recorded in **ADR-0004 Amendment DEC-09** and the ADR-0004 D-9 amendment block, not duplicated here. There is no E-D7; the intervening reconciliation index is reserved.*

---

## 4. Rationale -- The Oracle Vector

A subsystem is "done" only when its mechanical oracle is green (Law 2). The behavioural oracle set below is split by KIND so that the right tool grades each claim, and is mutation-proven (Rule 6) so the goldens are known to bite. No oracle in this vector grades the artifact against the same source the artifact renders from.

### 4.1 `test-interact` (HOST property test; INTERNAL GEOMETRY; BINDING; independent golden by recomputation)

Drives a scripted raw-event trace into the SPSC ring, drains via `WaitNextEvent`, and asserts the post-drag / post-menu internal state (damaged-pixel set, z-order, hit-test routing, exposed-region content) against ground truth recomputed by a SECOND, DIFFERENT algorithm than the compositor (an independent front-to-back first-writer-wins owner-grid + a from-scratch `desktop_paint_all` at final positions -- the `test_drag.c` idiom extended to menu-track / z-order / hit-test). NO external golden; P3-LEGITIMATE per ADR-0005 Sec 2.2 (no published QuickDraw region body). The live binary runs these exact symbols, inheriting the proof.

Mutants (each must go RED, then restore): the existing `DRAG_MUTATE_SKIP_EXPOSED` and `DRAG_MUTATE_NO_CLIP`, plus the NEW `MENU_MUTATE_WRONG_HILITE`, `ZORDER_MUTATE_RAISE_WRONG`, and `HITTEST_MUTATE_OFF_BY_TITLEBAR`.

### 4.2 `test-flair-drag` (EMU gate; OBSERVABLE LOOK; two tiers; neither by-construction)

Boots `build/flair_live.img` (`kmain -DBOOT_FLAIR_LIVE`), injects the LOCKED `spec/traces/flair_drag_menu.trace` (the existing QMP send-key for the menu half + the NEW `qmp_inject_mouse` for the drag half), synchronised on the `FLAIR-LIVE-READY -> FLAIR-DRAG-DONE` / `FLAIR-MENU-DOWN` markers, then screendumps. Grader: `ppm_flair_behav_check.c` (C-only).

- **Tier-A (structural -- the DAMAGE LAW):** chrome at the NEW structure rect `{L+dh, T+dv}`; the OLD-minus-NEW area reads Initech teal `#8DDCDC` (consumed from the one canon `idx2`); a digest of every pixel OUTSIDE (old-structure UNION new-structure) byte-identical to the pre-drag dump (no over-repaint, ADR-0004 D-5); post-menu the dropped panel rect below the 20px bar is non-desktop fill and the `File` title is hilited.
- **Tier-B (independent golden):** live-diff the post-drag chrome GEOMETRY (title-bar 19, frame 1, goAway inset 9, zoom -20, dBox 7) vs `../system7-decomp/goldens/resources/wdef0_geometry_scan.txt`; STRUCTURAL / CONTOUR diff of the post-menu drop vs `../system7-decomp/goldens/captures/cal_filemenu.png` (NEVER byte/color -- the captures are 4-bit / grayscale).

Wired `SYSTEM7_DECOMP ?= ../system7-decomp`, LOUD-SKIP when absent (never silent-pass). Expected values come from the WDEF SCAN / the capture CONTOUR -- NEVER from `flair_canon_rgb`, NEVER from `preview.webp`. Graded per host-model-per-mode (ADR-0004 D-8 / AM-6), never a cross-emu byte-CRC. All PNG-pixel arms LOUD-SKIP (the factory has only a P6 PPM reader, no PNG decoder).

### 4.3 `test-flair-drag-mutant` (Rule 6; includes the HER-02-mandated VALUE mutant)

| Mutant | Perturbation | Expected | What it proves |
|---|---|---|---|
| M1 DRAG-NOOP | `DragWindow` no-op; window stays at old rect | Tier-A NEW-rect assert RED | The direct HER-14 static-frame mutant: a static present cannot pass. |
| M2 OVER-REPAINT | `desktop_paint_damage` repaints the full frame unclipped | Tier-A no-over-repaint digest RED | The damage law (D-5) bites over-repaint. |
| M3 GEOMETRY-VALUE | Live title-bar-height constant `19->18` | Tier-B WDEF-scan diff RED | The mandatory VALUE mutant the by-construction `ppm_flair_check` can NEVER have. |
| M4 MENU-NODROP | Menu track returns without drawing the panel | Tier-B `cal_filemenu` CONTOUR diff RED | The menu drop is observable structure. |
| M5 NO-MOUSE-INJECT | A no-injection run; window left at origin | Tier-A NEW-rect assert RED | The injection actually reaches the guest. |

Each must go RED, then restore.

### 4.4 `test-flair-drag-bochs` (Rule 5; Stop condition -- NOT QEMU-only)

A Bochs leg that brings up the IRQ12 PS/2 mouse path and VERIFIES the dual-PIC EOI sequence (slave `0xA0` then master `0x20`) under Bochs strict PIC accuracy. A single master EOI silently wedges the mouse on real hardware; QEMU is too forgiving to catch it. Follows the existing `test-flair-desktop-bochs` pattern. 86Box is the funded follow-up (beads initech-q0gy), NOT in this deliverable.

### 4.5 Liveness markers (catches cooperative-hang-as-frozen-paint)

The pump emits `FLAIR-TICK n=<advancing count>`, `FLAIR-DRAG-DONE`, and `FLAIR-MENU-DOWN` on serial. The gate asserts no-triple-fault (`-d int,guest_errors,cpu_reset`) + markers present + tick ADVANCING. A non-yielding dispatch hangs the desktop and looks identical to a paint failure on a single screendump; the advancing-tick marker fails loud as a stalled tick (Rule 2, fail loud).

### 4.6 Era / skin behavioural oracles consumed alongside the loop

These grade the ADR-0004 D-9 era-layering consequences; they are retained companions to this loop and are wired into the ADR-0004 D-8 oracle vector.

- **`test-skin-teal`** (the INJECTED-VALUE oracle for the AUTHORED teal datum; two independent proofs, neither by-construction): (3a) the teal-substituted bevel / groove / desktop skin fields equal the LOCKED canon constants -- Initech teal `#8DDCDC` and teal-shadow `#4E9BA3` (`spec/assets/color_canon.json` derived rows `bevel_light` / `bevel_shadow`); the oracle reads the canon datum, the render reads the skin record, so it is independent of the render path. (3b) a STRUCTURE-PRESERVING proof: render a chrome swatch with the teal skin and with the pristine `SYS7_GOLDEN` swatch and assert every NON-tinge-slot pixel is BIT-IDENTICAL while every former-lavender pixel now equals EXACTLY the canon teal. Mutants: perturb the teal screen value by one count -> 3a RED; change a NON-tinge slot to teal -> 3b RED; the seafoam / lavender-relapse mutant bites. NEVER graded against the `pinstripe.md` lavender rows.
- **`test-skin-era-frozen`** (the accretion guardrail): a frozen-row sha-of-fields digest over the LOCKED `ERA_SYS7_0_1` and `ERA_WIN31` registry rows; asserts a later `ERA_SYS8_PLATINUM` accretion commit did NOT mutate a base row. Mutant: change one byte of the 7.0.1 row -> RED. Mechanical enforcement of the additive-only accretion rule (ADR-0004 D-9 P5).
- **`check-win95isms`** (era-boundary contract): a grep gate over `spec/` AND `os/flair/` asserting that NO forbidden Win95 token (`#DFDFDF`, `COLOR_3DLIGHT`, `COLOR_3DDKSHADOW`, `COLOR_*` idx>20, gradient caption, `HTCLOSE`, `CTL3D`, `DrawEdge`, `MENUEX`, a second inner bevel ring) appears as a live constant (the guardrails doc itself excluded). Mutant: insert `#DFDFDF` -> RED; restore -> green. Keeps the `HERITAGE_GDI` peer skin flat-2-D Win-3.1 and out of Win95 CTL3D (ADR-0004 D-9b, R5 depth-trap).

### 4.7 Retained host suites (NOT replaced)

The EMU gate proves the wiring runs on the 386 and the furniture matches the decomp; the host property suites prove the internal update-region math that no System-7 capture exposes. Both are kept: `test-event` (deterministic `EventRecord` sequence), `test-drag` (`DiffRgn` update-region bit-exact vs an independent owner-grid), `test-menu`, and `test-window`.

### 4.8 Locked spec-data and reference artifacts

| Artifact | Disposition |
|---|---|
| `spec/traces/flair_drag_menu.trace` | NEW LOCKED -- the deterministic scripted input trace for the EMU gate (Rule 11); mouse rel-deltas + button down/up + cmd-key for the menu half. |
| `spec/event_model.h` | REFERENCE (no change) -- the LOCKED `flair_raw_event_t` kind/tick/payload the three ISRs post. |
| `spec/chrome_metrics.h` | REFERENCE / AMEND -- the title-bar / frame / inset constants Tier-B diffs against the INDEPENDENT WDEF scan (the listing-vs-binary independence is the whole point); the lavender bevel `#DADAFF` / `#B3B3DA` constants become the teal-substituted slots per the canon, recorded as the SUPERSEDED baseline, never the expected value. |
| `spec/assets/color_canon.json` + generated `spec/assets/color_canon.h` | REFERENCE (NOT created here) -- the one LOCKED canon module (ADR-0004 Amendment DEC-09 / ADR-0010). E-D5's bare-teal assert consumes its `idx2 = #8DDCDC`. Hard precondition (FO-2). |
| `spec/flair_skins.h` | NEW LOCKED (era-layering build-out, ADR-0004 D-9) -- a data-only `flair_skin_t` registry view; consumed by `test-skin-teal` / `test-skin-era-frozen`. |

---

## 5. Consequences

### 5.1 Binding Constraints

The following are binding on all FLAIR work that touches the live loop or the behavioural gate:

- **BC-1 (single spine).** The live loop MUST run the identical `window.c`/`event.c`/`desktop.c`/`menu.c` symbols the host suites mutation-prove. A second event loop or any event/geometry logic re-implemented in `kmain` is forbidden (E-D2; ADR-0004 D-2).
- **BC-2 (ISR-enqueue-only, additive, gated).** The three ISR producers do the D-4 minimum (read device, post raw, EOI, return) -- no `EventRecord` synthesis, no Manager call, no allocation in interrupt context. They are ADDITIVE (`g_ticks++`, the DOS `g_kbd` ASCII ring, the legacy paths all kept) and behind `-DBOOT_FLAIR_LIVE` (E-D3; ADR-0004 D-4).
- **BC-3 (dual-PIC EOI).** The IRQ12 path MUST issue the slave (`0xA0`) EOI THEN the master (`0x20`) EOI, and this MUST be Bochs-verified under strict PIC accuracy, not QEMU-only (E-D3/E-D6; Rule 5, Stop condition).
- **BC-4 (keep the initial present).** The pump MUST keep the initial `flair_desktop_present` so a no-input boot still shows the canon frame and every existing static screendump gate stays byte-stable (E-D4; Rule 11).
- **BC-5 (two oracles by KIND; no by-construction value oracle).** Internal geometry is graded by `test-interact` (independent-by-recomputation); the observable look by `test-flair-drag` (independent decomp golden). No behavioural value oracle may grade the artifact against the same source it renders from. Promoting `ppm_flair_check` into the behavioural value oracle is forbidden (E-D5; Law 2; rebirths HER-02).
- **BC-6 (the frozen golden boundary).** A future agent is FORBIDDEN from strengthening the emu gate into a minimal-damage proof by diffing intermediate frames; the minimal-damage update region is not externally observable and that claim would be a false oracle (E-D8; Stop condition).
- **BC-7 (cooperative, not preemptive).** The pump holds the CPU; the ISRs only enqueue; a non-yielding app hangs the desktop. This is canon, not a defect to fix with preemption (E-D4; ADR-0004 D-6; PRD Sec 2).
- **BC-8 (the hourglass canon).** The busy cursor is the hourglass `CURS`, NOT a wristwatch (E-D3; ADR-0004 Sec 3.7; PRD Law 4).
- **BC-9 (mutation-proven goldens).** The behavioural gate is not "done" until M1..M5 are each proven to go RED, then restored; in particular M1 (drag-noop) is the direct HER-14 mutant and M3 (geometry value) is the mandatory VALUE mutant the by-construction grader can never have (Sec 4.3; Rule 6).

### 5.2 Forward Obligations

Recorded as binding future work. The landing sequence (Sec 6) is the ordered realization of these.

- **FO-1 (precondition; BLOCKS this deliverable).** Land ADR-0005 Amendment AM-1 -- the `CombineRgn`/`GetRgnBox`/`PtInRegion`/`RectInRegion` GDI peer family over the ONE `region_op`, plus the `RectInRgn` containment-to-overlap deep-bug fix. Color-independent; the ADR-0004 D-9b menu-clip-through-`CombineRgn` depends on the `CombineRgn` shim existing. Coordinate the `RectInRgn` overlap fix with the mechanism consumers before landing.
- **FO-2 (precondition; BLOCKS this deliverable).** Land the one canon-module arbitration (ADR-0004 Amendment DEC-09 / ADR-0010): the ONE LOCKED `spec/assets/color_canon.json` -> generated `color_canon.h` exposing `flair_canon_rgb(uint8_t idx)`. E-D5's bare-teal assert is downstream of this.
- **FO-4 (re30.8 TICK; BLOCKS the live loop).** Wire `flair_tick_advance()` into `pit_irq_handler` BEFORE the existing PIC EOI at `os/milton/pit.c:93` (additive, `g_ticks++` untouched); install the HLT yield via `flair_event_set_yield` BEFORE `sti`; emit `FLAIR-TICK n=<count>` from a bounded `WaitNextEvent`. Proves IDT/PIC/tick/yield/HLT-wakeup live BEFORE any new IRQ. Boots green standalone.
- **FO-5 (re30.9 KEYBOARD; depends FO-4).** `kbd_irq_handler` ALSO `flair_raw_post`s the RAW PS/2 scancode (the legacy `g_kbd` ASCII ring kept so `COMMAND.COM` / `test-shell` / `test-samir-boot` stay green); the pump synthesizes `keyDown` in task context; `FLAIR-KEY=<sc>` serial echo. The cheapest on-metal SPSC-ring-atomicity-under-async-producer test.
- **FO-6 (mouse IRQ12; depends FO-5).** NEW `os/milton/mouse.c` + `irq12_entry` in `isr.asm` cloning the `irq0`/`irq1` bracket (`isr.asm:465,481`), dual-PIC EOI, 8042 aux enable, bounded 8042 spin, 3-byte packet -> ONE `FLAIR_RAW_MOUSE` post; `pic_unmask_irq12()`; hourglass `CURS` canon. Bochs-VERIFIED (Rule 5, Stop condition). Lands LAST of the three ISRs so a mouse bug cannot masquerade as a loop/ring bug.
- **FO-7 (the live pump; depends FO-4..FO-6).** Replace the `BOOT_FLAIR_LIVE` render-once-and-HLT (`kmain.c:333,680`, present at `:711`) with `flair_event_init` + the `WaitNextEvent` loop dispatching `FindWindow` part-codes via the EXISTING `window.c`/`menu.c`/`desktop.c` verbs. `kmain` is a THIN source/sink adapter. KEEP the initial present.
- **FO-8 (QMP mouse injection; depends FO-7).** Add `qmp_inject_mouse` to `harness/emu/qemu.c` + a mouse field to `qemu.h`; LOCK `spec/traces/flair_drag_menu.trace`; reuse the existing keys / screendump / `wait_for_serial_marker` machinery for the keyboard half. Pure-C (Law 3).
- **FO-9 (the behavioural gate; ORACLE-FIRST per E-D1; depends FO-8 for the live run).** Author `test-flair-drag` + `ppm_flair_behav_check.c` (Tier-A damage-law invariant + Tier-B WDEF-scan VALUE golden + `cal_filemenu` CONTOUR) and the M1..M5 mutants; prove RED-against-stub / GREEN-against-wiring. Wire into the EMU gate set with the liveness markers + no-triple-fault.
- **FO-11 (close the critic gap; GDI facade load-bearing on metal).** Route the Win-3.1/Photoshop menu bar's clip through the GDI `CombineRgn` wrapper (`os/flair/menu.c:344` currently takes a raw `const region_t *clip`; change the WIN31/GDI-skin menu-draw path to compute its visible clip via `CombineRgn(dst, bar_rgn, vis_rgn, RGN_AND)` from FO-1's shim) so the peer GDI facade is exercised by the live render path, not only the oracle. `test-region-gdi` (ADR-0005 AM-1) proves the shim rides the one engine; this FO makes it consumed (ADR-0004 D-9b; P2).
- **FO-12 (cross-committee; NOT a five-way merge).** `test-flair-drag` is built P3-correct from day one and does NOT inherit `ppm_flair_check`'s by-construction color grading. The single `ppm_flair_check` re-key (re-key onto `flair_canon_rgb`, strike the agree-BY-CONSTRUCTION comment, seafoam->teal `#8DDCDC`, keep all structural / z-order / pinstripe-alternation probes, wire `test-color-canon` as a hard precondition) is owned by ONE committee (the grading committee, ADR-0010), not this deliverable.
- **FO-13 (ADR text + bead chain).** File this ADR recording E-D1..E-D8; cross-reference into ADR-0004 D-8 as the EMU-level companion to the host-only `test-event`/`test-drag` rows; file the ADR-0004 D-9 amendment. Relate the roadmap re30.8 tick -> re30.9 keyboard -> mouse-IRQ12 -> raw-ring producers -> `WaitNextEvent` pump -> `test-flair-drag` green, each bead blocked on the prior. Mark **HER-14 RESOLVED** when FO-7 boots interactively AND FO-9 is green and mutation-proven.

### 5.3 Forward Gaps (named, not delivered here)

- **FO-GAP-A (DEFAULT-BOOT FLIP).** The live loop ships behind `-DBOOT_FLAIR_LIVE` as a third demo kernel (alongside `BOOT_SHELL` / `BOOT_FLAIR_SHELL`); the default boot stays the static render-once-and-HLT frame until a LATER milestone flips the default. Accepted to keep every existing emu gate byte-stable (Rule 11). Promoting the live desktop to the default boot is a named future obligation.
- **FO-GAP-B (86Box / TRI-EMULATOR).** `test-flair-drag-bochs` is delivered (the new Bochs mouse leg); 86Box is the funded DEC-04 follow-up (beads initech-q0gy). A FULL operational GUI graded across all three emulators is NOT yet in scope -- the live draggable desktop is graded on QEMU primarily with the Bochs mouse leg new (Rule 5 partially met, named gap).
- **FO-GAP-C (SSIM).** SSIM remains an unbuilt honest stub and is DEFERRED. For this deliverable the operator-judge signal is the live screendump + the structural / value oracles only; the documented per-window SSIM fidelity guide stays a future obligation, consistent with Law 4 (SSIM is a guide, not a hard gate).

### 5.4 Risks and Honesty Notes

- The chimera scene has no real-System-7 capture; this is why E-D5 grades by KIND and E-D8 freezes the golden boundary. Any attempt to "tighten" the emu gate with intermediate-frame diffs is a regression to a false oracle (BC-6).
- The teal datum is an AUTHORED Initech identity injection (the one honest P4 exception); it is NEVER claimed decomp-sourced and is gated only by the locked-constant equality + the seafoam/lavender-relapse VALUE mutant. The bevel is teal (`bevel_light #8DDCDC`, `bevel_shadow #4E9BA3`), NOT lavender; it is NEVER graded against the `pinstripe.md` lavender rows. The pinstripe itself stays System-7 (`#F3F3F3` / `#969696`), graded vs `pinstripe.md` (ADR-0004 OD-4 supersession; ADR-0010).
- The cooperative loop can hang the desktop if an app does not yield; this is period-authentic (BC-7). The advancing-tick marker is the loud detector that distinguishes a stalled loop from a paint failure (Sec 4.5).

---

## 6. Landing Sequence

The reconciliation-ordered realization. Each step is one file / one bead where practical (Rule 7), re-graded on QEMU + Bochs (Rule 5) per step.

| Step | Obligation | Action |
|---|---|---|
| 0 | FO-1 (precondition) | Land ADR-0005 AM-1: the GDI peer-region family over the ONE `region_op` + the `RectInRegion` containment-to-overlap fix, coordinated with the mechanism consumers before landing. |
| 1 | FO-2 (precondition) | Land the one canon-module arbitration (LOCKED `color_canon.json`/`.h` with the wctb_part crosswalk) + the one value oracle GREEN against goldens (PNG arms loud-skip). Not owned here; hard-blocking. |
| 2 | FO-4 | Wire `flair_tick_advance` into `pit_irq_handler` before the PIC EOI (additive, behind `-DBOOT_FLAIR_LIVE`) + the HLT yield + `FLAIR-TICK` marker; boot green standalone, every existing gate byte-stable. |
| 3 | FO-5 | kbd IRQ1 additive raw-scancode post (DOS `g_kbd` ring untouched); `FLAIR-KEY` echo; confirm `test-shell` / `test-conin` / `test-samir-boot` stay green. |
| 4 | FO-6 | NEW `os/milton/mouse.c` + `irq12_entry` with dual-PIC EOI + `pic_unmask_irq12`; Bochs-verify the slave-then-master EOI (Rule 5). |
| 5 | FO-9 (ORACLE-FIRST per E-D1) | Author `test-flair-drag` + `ppm_flair_behav_check.c` + M1..M5 mutants; prove RED-against-stub. |
| 6 | FO-7 + FO-8 | Replace `kmain` render-once-and-HLT with the `WaitNextEvent` pump (keep the initial present) + add `qmp_inject_mouse` + LOCK the trace; `test-flair-drag` goes GREEN-against-wiring; mutation-prove M1..M5 bite then restore; add `test-flair-drag-bochs`. |
| 7 | FO-10 (era-layering, ADR-0004 D-9) | Build out the `flair_skin_t` registry VIEW over the canon + `flair_skin_resolve` (total, fail-loud) + the frozen-row digest; add the selector field + skin parameter; wire `test-skin-teal` + `test-skin-era-frozen` + `check-win95isms`; one-file-per-step, re-grade QEMU + Bochs. |
| 8 | FO-11 (gap closure) | Route the Win-3.1/Photoshop menu-bar clip through `CombineRgn` so the GDI peer facade is load-bearing on metal; confirm the live render path exercises it (not only the oracle). |
| 9 | FO-13 | File this ADR, the ADR-0004 D-9 amendment, the OD-4 seafoam->teal supersession, and the D-8 oracle-vector additions; mark HER-14 RESOLVED when Step 6 boots interactively and the gate is green + mutation-proven. |

---

## 7. Related Decisions

| Document | Relationship |
|---|---|
| **ADR-0004 (FLAIR Toolbox Architecture)** | Parent. This ADR realizes ADR-0004's ratified cooperative `WaitNextEvent` model (Sec 3.6), ISR-enqueue-only event model (D-4), region-difference damage model (D-5), single-pixel-path / single-spine principle (D-2), and cooperative-not-preemptive scheduling (D-6) as a live booted desktop. The EMU `test-flair-drag` row is added to the ADR-0004 D-8 oracle vector as the on-metal companion to the host-only `test-event` / `test-drag` rows. |
| **ADR-0004 Amendment DEC-09 (Mechanism/Policy Split)** | Owns the P1 mechanism/policy split (constraint C-8: mechanism names palette INDICES via `flair_look_pixel`, zero color literal below the cut-line) and the ONE arbitrated canon module (`spec/assets/color_canon.json` -> `color_canon.h` exposing `flair_canon_rgb`; the other three proposed modules DELETED), the D-9 / D-9b era-layering and peer-skin chimera, and the OD-4 seafoam->teal supersession. This ADR consumes that module (E-D5 bare-teal assert reads `idx2 #8DDCDC`) and grades the behavioural consequences (`test-skin-teal`, `test-skin-era-frozen`, `check-win95isms`). |
| **ADR-0005 Amendment AM-1 (Dual-Heritage Region Spine)** | Owns the heritage-neutral ATKINSON spine, the GDI/HRGN peer wrapper family (`CombineRgn`, `GetRgnBox`, `PtInRegion`, `RectInRegion`) over the ONE `region_op`, the `RectInRgn` containment-to-overlap deep-bug fix, and the structural single-engine grep gate. Hard precondition (FO-1); the FO-11 menu-clip-through-`CombineRgn` rides its shim. |
| **ADR-0010 (FLAIR Grading and Goldens)** | Owns the grading architecture (the decomp corpora as the live golden master), the one value oracle `test-color-canon` (legs wctb binary / win31 text / `pinstripe.md` / authored-teal), the PNG-pixel loud-skip and Win depth-trap guardrails, and the single `ppm_flair_check` re-key (FO-12 here). This ADR defers all color-value grading to ADR-0010 and grades only behaviour. |
| **ADR-0003 Amendment DEC-04a (INT 21h / IDT-PIC Vector Map)** | Owns the IDT / PIC vector remap and the dual-PIC EOI contract that the IRQ12 mouse path (E-D3, BC-3) and the PIT tick (E-D4) depend on. |
| **REVOCATION-RECORD-2026-06-21 (FLAIR Heresy Purge)** | The management decree this ADR executes against HER-14 (the WaitNextEvent spine ratified-but-never-wired / the static frame dressed as interactivity). HER-14 is marked RESOLVED when FO-7 boots interactively and FO-9 is green and mutation-proven (FO-13). This ADR also carries the HER-02-mandated VALUE mutant (M3) into the behavioural gate -- the geometry VALUE mutant the by-construction grader could never have. |
| **PRD Sec 2, Sec 6.3, Sec 8, Sec 9** | The cooperative-not-preemptive non-goal (Sec 2), the FLAIR Toolbox spec (Sec 6.3), and the every-subsystem-has-a-mechanical-oracle / emulator-as-fitness-signal mandate (Sec 8, Sec 9) this ADR's two-oracles-by-kind regime satisfies. |

---

*End of ADR-0006. This document is RATIFIED by ADR-by-committee and chief-architect reconciliation under operator-delegated authority (2026-06-21), operator-ratified 2026-06-22. Controlled Document; verify revision before use.*
