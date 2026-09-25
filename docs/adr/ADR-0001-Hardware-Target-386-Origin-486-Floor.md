<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0001 -- Target Hardware Platform and Memory Model: the 386 Origin and the 486-Class Floor

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0001 |
| Title | Target Hardware Platform and Memory Model: the 386 Origin and the 486-Class Floor |
| Version | 2.0 (Reconstructed; Amended) |
| Status | **RECONSTRUCTED (records reconstruction, 2026-09-25) + AMENDED (operator, 2026-09-25)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Office of Enterprise Architecture (Records Reconstruction) |
| Original Effective Date (reconstructed) | 2026-06-08 |
| Amendment Effective Date | 2026-09-25 |
| Next Scheduled Review | Upon first Phase involving hardware-FP or CPUID-gated code, per RECORDS-POL-002 |
| Supersedes | (none as a filed Record -- see Sec 2, "the missing Record") |
| Superseded By | (none) |
| Related Documents | InitechOS-PRD.md Sec 5 (Target Platform / Hardware Contract); `spec/hardware.json` (the on-disk hardware contract, ADR-0009 DEC-07); ADR-0002 (Toolchain, Implementation Language, Executable Format); ADR-0003 (InitechDOS Base OS Personality, Sec 2, which cites this Record); ADR-0009 (SAMIR numeric mechanism, DEC-01/DEC-07, FPU optional); `docs/worklog/WL-0087-gui-remediation-wave-one.md` (the Bochs `cmov` catch, R3.1); `docs/design/kernel-runway-relocation.md` Part K2 (the `KERNEL_SECTORS`/bounce-ceiling design, unrelated to CPU generation) |
| Related Issues | beads `initech-qw0t` (this Record; 486 floor ratification); `initech-fgs1` (2026-06-21 operator note proposing the 486 floor -- **SUPERSEDED by this Record**, closed accordingly); `initech-zj6w` (kernel x87 init path, unlocked by DEC-02, not implemented here); `initech-x0i` (86Box driver, still not wired; DEC-05 sets its future CPU model) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 1.0 (reconstructed) | 2026-09-25 | OEA (Records Reconstruction) | **Reconstructs, as a filed Record for the first time, the 386+/32-bit-protected-flat platform decision.** This decision has been cited throughout the corpus since the Programme's earliest ratified records -- ADR-0003 Sec 2 ("pursuant to the platform direction ratified under ADR-0001 (Target Hardware Platform; 386-or-greater; 32-bit protected/flat memory model)"), effective 2026-06-08; CLAUDE.md ("ADR-0001 (386+, 32-bit flat)"); PRD Sec 5; `spec/hardware.json` `cpu.ref` -- but no ADR-0001 file exists in `docs/adr/`. Per HANDOFF lk29, this is a genuine gap: the original decision was ratified in substance (the whole Programme built on it, unchallenged, for three-plus months) but the Record itself was either never filed or lost prior to the current document-control regime. **DEC-01 below reconstructs that decision as history, from its citations, and is explicit that it is a reconstruction, not a rediscovered original.** No original draft, markup, or sign-off sheet survives; the Approval & Sign-Off Matrix below for Rev 1.0 is annotated accordingly. | (n/a -- reconstruction; no original reviewers of record) |
| 2.0 | 2026-09-25 | OEA (Strategy / Charter) | **Operator ratifies DEC-02: 486-class hardware is GREENLIT as the authentic target for the final build; 386 functionality is desirable but no longer required.** This closes bead `initech-fgs1` (raised 2026-06-21, period-authenticity research note on the dBASE III+/IV + Lotus 1-2-3 + WordPerfect -> *Office Space* era skewing later than 386) as **superseded** by this ratification, not merely acknowledged. DEC-03 records what does NOT change as a consequence (the `-march=i386` codegen pin; the `KERNEL_SECTORS=384` bounce ceiling; no v8086). DEC-04 records what the floor UNLOCKS as forward-looking, not-yet-implemented follow-up work. DEC-05 records the emulator-regime consequence. InitechOS-PRD.md Sec 5 and Sec 2, and `spec/hardware.json`, are amended in the same change-set (beads `initech-qw0t`). | T. Osborne (Operator) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| *Rev 1.0 (reconstructed) -- no original signatories of record* | | | |
| Author / Reconstructor | Office of Enterprise Architecture (Records Reconstruction) | Submitted (reconstruction) | 2026-09-25 |
| Records Management | M. Waddams (Archive Annex B) | Filed (reconstruction, gap noted) | 2026-09-25 |
| *Rev 2.0 -- the 486-class floor amendment* | | | |
| Author / Drafter | Office of Enterprise Architecture (Strategy / Charter) | Submitted (DRAFT) | 2026-09-25 |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Approved | 2026-09-25 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Approved -- "486DX on-die x87 is the consequential delta; this is period-honest for the app era" | 2026-09-25 |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Approved | 2026-09-25 |
| Stakeholder Engagement | T. Osborne (Operator) | **Ratified (2026-09-25)** | 2026-09-25 |
| Records Management | M. Waddams (Archive Annex B) | Filed | 2026-09-25 |

*Note on status: this is a Rule-8 locked-decision act (CLAUDE.md Rule 8), not a silent edit. Where this Record and any citing document (PRD Sec 5, `spec/hardware.json`, CLAUDE.md) ever diverge going forward, this ADR governs and the divergence is reconciled, per the same convention as ADR-0012.*

---

## 1. Purpose and Scope

### 1.1 Purpose

This Record does two things under one document number, because the second cannot be honestly written without the first existing:

1. **Reconstructs**, as a filed Architecture Decision Record, the foundational platform decision that every other ratified Record in this corpus (ADR-0002 onward) has cited as settled since 2026-06-08: that InitechOS targets 386-or-greater x86 hardware, boots real-mode-to-32-bit-protected, and runs entirely in a flat 32-bit segmentation model (DEC-01).
2. **Records the operator's 2026-09-25 ruling** that 486-class hardware is now the *authentic target* for the final build, with 386 functionality retained as desirable, non-blocking heritage scope (DEC-02), and the consequences of that ruling: what stays pinned regardless (DEC-03), what the new floor unlocks as future work (DEC-04), and the emulator-regime implication (DEC-05).

### 1.2 The Missing Record (why DEC-01 is a reconstruction, not a rediscovery)

`docs/adr/` contains ADR-0002 through ADR-0013 plus one CDR and one Revocation Record, and every one of them that touches hardware generation cites "ADR-0001" as an already-ratified decision -- ADR-0003 Sec 2 quotes it by name ("386-or-greater; 32-bit protected/flat memory model"); `spec/hardware.json`'s `cpu.ref` cites it; CLAUDE.md's opening section cites it. No `ADR-0001-*.md` file has ever existed in this repository's history. This is recorded plainly, per Law 1 (ground truth before code) and in the spirit of Law 1's honesty requirement applied to documentation: **the decision was real and was acted on uniformly across the Programme, but the Record documenting it was never filed, or was filed and subsequently lost, before the current document-control regime began.** DEC-01 reconstructs the decision's content from its citations and from the repo's actual, oracle-verified behavior (the `-march=i386` pin, the real-mode-to-protected boot sequence in `os/boot/`, the flat memory model throughout `os/milton/` and `os/flair/`) rather than inventing new content. It is filed now because `initech-qw0t` (itself triggered by the DEC-02 ruling) made the absence untenable: a Record cannot be amended if it does not exist.

### 1.3 Scope

In scope: the CPU generation floor and the memory/segmentation model (DEC-01, historical; DEC-02, current); the enumerated non-effects of the DEC-02 floor change on already-pinned build/runtime decisions (DEC-03); the enumerated forward-looking opportunities the floor change opens, filed as follow-up beads rather than implemented here (DEC-04); the tri-emulator CPU-model consequence (DEC-05).

Out of scope: the toolchain and executable format (ADR-0002); the InitechDOS personality built atop this platform (ADR-0003); the FPU/soft-float mechanism itself, which ADR-0009 DEC-01 already settled and which DEC-04 below only notes is now *unlockable*, not un-settles; any code change (this is a docs+spec-only Record; DEC-04's items are each a separate, not-yet-claimed bead).

---

## 2. Context

### 2.1 The historical decision (reconstructed)

At the outset of the STAPLER Programme, the Non-Goals section of the PRD recorded, in terms consistent with an already-made platform decision: "8088/80286 real-mode purism. *Dropped by decree -- no fond memories of segment:offset awkwardness.* 386+ only." ADR-0003 (ratified 2026-06-08), in framing its own context, states that the base-OS decision was reached "in alignment with the objectives of the STAPLER Programme and pursuant to the platform direction ratified under ADR-0001 (Target Hardware Platform; 386-or-greater; 32-bit protected/flat memory model)." Every subsequent Record that needed a hardware floor (ADR-0004's LFB addressing, ADR-0008/ADR-0009's flat-memory SAMIR engine, ADR-0013's App Contract) inherited this floor without re-litigating it. The reconstructed decision, DEC-01, is that inheritance made explicit: 386-or-greater, real-mode boot handing off to 32-bit protected mode, flat segmentation throughout, no 8088/80286 support.

### 2.2 The 2026-06-21 research note (initech-fgs1)

On 2026-06-21 the operator raised `initech-fgs1`, a P3 research note (not a decision) observing that the canonical application era this project targets -- dBASE III+/IV, Lotus 1-2-3, WordPerfect, running into the *Office Space*-frame period -- skews later than 386, closer to 486/Pentium, and that a 486 floor would be period-defensible and would resolve the FPU question (a 486DX has an on-die x87; SAMIR's soft-float profile, ADR-0009 DEC-01, was chosen partly because the FPU was *optional* on the then-ratified 386-class floor). The note explicitly deferred the decision ("NOT a decision to make now... decision owner: operator + committee") and PRD Sec 5 carried a matching FORWARD NOTE flagging the question as open. Both are resolved by this Record.

### 2.3 The 2026-09-25 ruling

The operator has now made the call the 2026-06-21 note deferred: **486-class hardware is GREENLIT as the authentic target for the final InitechOS build.** 386 functionality remains desirable -- nothing that already runs on the 386-class floor is broken or discouraged -- but a 486-or-later machine is no longer merely "under consideration," it is the platform ADR-0001 now names. This is recorded as DEC-02.

---

## 3. The Decision

### DEC-01 -- Historical ratification (reconstructed): 386+, 32-bit protected/flat

**RECONSTRUCTED, not newly decided.** InitechOS targets x86 hardware of the 80386 generation or later. The boot sequence starts in real mode and switches to 32-bit protected mode with flat (non-paged) segmentation; all kernel, Toolbox, and application code thereafter runs in a single flat 32-bit protected-mode segment, with no segment:offset arithmetic and no 8088/80286 real-mode-only support (dropped by decree; no v8086, no 16-bit-only execution path). This is the floor every other Record (ADR-0002 onward) has cited as settled since 2026-06-08, reconstructed here from those citations and from the repo's actual boot/build behavior per Sec 2.1. It is superseded in its *floor generation* only, by DEC-02; the 32-bit-protected/flat memory model it establishes is UNCHANGED and remains binding.

### DEC-02 -- (2026-09-25, operator T. Osborne) 486-class hardware is the authentic target; 386 is desirable, non-blocking

**RATIFIED.** For the final InitechOS build, 486-class hardware (i486 or later, pre-Pentium `-march` semantics per DEC-03) is the *authentic target platform* -- the machine class the project claims InitechOS could plausibly have shipped on and been judged against by a period reviewer (PRD Sec 3, the Fidelity Bar). 386-class hardware support is **retained as desirable, non-blocking heritage scope**: nothing built to run on a 386-class floor is required to be removed, and no currently-green oracle is permitted to regress because of this ruling (Rule 3 -- deep fix, not floor-raising churn). Concretely: the project MAY, at its discretion, exploit 486-class characteristics (DEC-04) without being obligated to preserve bit-for-bit 386 parity for every future feature, but MUST NOT gratuitously break 386-class operation where no 486-only feature is in use.

This directly answers, and resolves, `initech-fgs1`'s open question in the affirmative, and does so on the stated grounds in that note: the canonical application era (dBASE III+/IV, Lotus 1-2-3, WordPerfect, into the *Office Space*-frame period) is period-honest at 486-class, and the 486DX's on-die x87 is the most consequential unlocked capability (DEC-04).

### DEC-03 -- What does NOT change

The 486-class floor is a **hardware target claim**, not an instruction to relax existing, deliberately-pinned build or design decisions. Specifically, and without limitation:

- **`-march=i386` codegen stays pinned.** The freestanding artifact's compiler `-march` flag remains `i386`. This is unrelated to, and not loosened by, DEC-02: `cmov` is a P6-generation (Pentium Pro+) instruction, not a 486 one, and the catch that pinned `-march=i386` (`docs/worklog/WL-0087-gui-remediation-wave-one.md`, "THE BOCHS CATCH": R3.1's new code triple-faulted under Bochs at `cpu model=pentium` while QEMU silently passed, because the freestanding artifact had been compiling at the host's default arch -- `cmov` everywhere -- since forever, silently violating this Record's own 386+ target) would recur identically at a 486 floor. Raising the *hardware target* to 486-class does not raise the *codegen* ceiling past what actual 486 silicon executes; `-march=i386` remains the correct, conservative pin, and Bochs continues to run `model=pentium` as the strict-transition-checking accuracy leg regardless (that model choice tests correctness under a superset CPU, it does not license emitting superset instructions).
- **`KERNEL_SECTORS=384` (the real-mode bounce ceiling) is bootloader design, not a CPU-generation constant.** `docs/design/kernel-runway-relocation.md` Part K2 establishes `KERNEL_SECTORS (352) <= 384` as the capacity invariant of the real-mode load bounce buffer at `KERNEL_BOUNCE_BASE` (`0x00010000`) before the post-PE copy-up to `KERNEL_BASE` (`0x00500000`) -- a real-mode addressing/BIOS-disk-geometry constraint of the boot design, wholly independent of whether the executing CPU is a 386 or a 486. DEC-02 does not move, relax, or reinterpret this ceiling.
- **No v8086, still.** DEC-01's prohibition on a virtual-8086 subsystem / genuine 16-bit binary execution is unaffected. A 486 supports v8086 mode exactly as a 386 does; that is not why the floor moved, and it does not reopen ADR-0003 Sec 1.2's out-of-scope call or ADR-0012 D-3a's accepted shortfall.

### DEC-04 -- What the 486-class floor UNLOCKS (forward-looking; not implemented by this Record)

The following become *available to pursue*, each as its own future bead, not work performed or authorized by this docs-only Record:

- **The kernel x87 initialization path** (`CR0.EM=0`/`MP=1`/`NE=1` + `FNINIT` + a pinned control word), already identified as "the correct 386+387/486 platform end-state" and deferred to a post-M6 bead in ADR-0009 DEC-01 (`initech-zj6w`). A 486DX's on-die FPU makes this path load-bearing-worthy rather than merely optional-coprocessor-detecting; `initech-zj6w` remains the tracking bead and remains unclaimed by this Record.
- **A hardware-float profile for SAMIR and any future spreadsheet** (Initech 123), as a documented amendment to ADR-0009 (whose DEC-01 chose software IEEE-754 double specifically because the then-ratified floor made an 8087/387 *optional*): the 486-class floor makes a hardware-x87 numeric path a period-authentic *option* alongside the existing soft-float path, not a replacement for it. ADR-0009's soft-float decision and its stop-condition ("do NOT widen any numeric oracle tolerance to accommodate an x87 divergence") are UNCHANGED by this Record; any future hardware-float profile is a separate, explicit ADR-0009 amendment with its own oracle.
- **CPUID availability.** Later-model 486s (and all Pentium-class hardware used for Bochs accuracy testing) expose the `CPUID` instruction; a 486-class floor makes relying on its presence period-defensible where a strict 386 floor would not. No code currently depends on it; this is recorded as an option, not a requirement.
- **A 4-8 MiB RAM norm for `FLAIR_HEAP_MIN`.** `spec/hardware.json`'s `flair_heap` window (`FLAIR_HEAP_MIN` = 4096 KiB, i.e. 4 MiB, per ADR-0004 DEC-03) was sized against a 386-era extended-memory assumption; 486-class business machines of the app era typically shipped with more, making a higher `FLAIR_HEAP_MIN` period-defensible. This Record does not change `FLAIR_HEAP_MIN`; any change to that locked value is its own Rule-8 act against `spec/memory_map.h` and `spec/hardware.json`, with its own beads issue and worklog note.

### DEC-05 -- Emulator regime consequence

QEMU (dev loop) and Bochs (real-mode-to-protected accuracy) are unaffected in role by this Record; Bochs already runs `cpu model=pentium` for strict transition checking (DEC-03), which is a superset-CPU accuracy leg, not a target-CPU claim. **86Box** (period authenticity + reference software), which remains **not yet wired** (`initech-x0i`; no `box86.c` exists) per CLAUDE.md's file map and `spec/hardware.json`'s `emulators.box86` note, becomes -- when it is eventually built -- a **486 + period VGA BIOS** configuration (e.g. a Cirrus or ET4000 period card, already anticipated in `spec/hardware.json`'s `box86` field), consistent with DEC-02's 486-class authentic target. This Record does not build 86Box; it sets the CPU model `initech-x0i` should target when it lands.

---

## 4. Consequences

### 4.1 Positive Consequences

- The Programme's most-cited-but-never-filed Record now exists, closing the documentation gap flagged in HANDOFF lk29 (DEC-01).
- The hardware-target question `initech-fgs1` raised on 2026-06-21 is resolved, not left open indefinitely; the note is superseded, not merely acknowledged (DEC-02).
- Nothing already pinned for good, independently-verified reasons (`-march=i386`; the bounce-ceiling capacity invariant; no v8086) is disturbed by the floor change (DEC-03), so no oracle regresses as a side effect of this Record.
- Forward opportunities (kernel x87, hardware-float SAMIR profile, CPUID, a larger `FLAIR_HEAP_MIN`) are named and given tracking beads rather than left as folklore (DEC-04).

### 4.2 Negative / Debt Consequences

- `initech-zj6w` and any future ADR-0009 hardware-float amendment remain open work; this Record creates no new implementation, only permission and a paper trail.
- A future agent could misread DEC-02 as license to relax `-march=i386`; DEC-03 is written to foreclose that reading explicitly, and this risk is called out here for the Stop-Conditions record.

### 4.3 Risks

The principal residual risk is the same class DEC-03 exists to prevent: an agent conflating "486-class hardware is now the target" with "the compiler may emit 486-or-later instructions," which would reopen exactly the `cmov` defect WL-0087 caught. Mitigated by DEC-03's explicit, cited prohibition and by the existing `-march=i386` build-time pin remaining untouched by this change-set.

---

## 5. Compliance and Conformance

Conformance to this Record is verified by the existing mechanisms it does not alter: the `-march=i386` objdump-verified codegen pin (WL-0087); the Bochs `cpu model=pentium` real-mode-to-protected transition-checking leg (Rule 5); the `KERNEL_SECTORS <= 384` build-time bounce-capacity guard (`docs/design/kernel-runway-relocation.md` K2); and, going forward, whatever mechanical oracle each DEC-04 follow-up bead introduces when and if it is claimed (none is introduced by this Record). No new oracle is created or required by this docs+spec-only change-set.

---

## 6. Related Decisions and References

| Document | Relationship |
|---|---|
| **InitechOS-PRD.md Sec 5** | Amended in this change-set: the FORWARD NOTE is replaced by a one-line reconciliation to DEC-02; the CPU row is updated to the 486-class-authentic-target wording. |
| **InitechOS-PRD.md Sec 2** | Amended in this change-set: the Goals section's boot-target bullet is updated to name the 486-class floor / 386-desirable framing. |
| **`spec/hardware.json`** | Amended in this change-set (Rule 8 locked-spec act): `cpu.value` and `cpu.note` updated; a `ratified` citation to this Record's DEC-02 and to bead `initech-qw0t` is added; every other field is unchanged. |
| **ADR-0002 (Toolchain, Implementation Language, Executable Format)** | Sibling foundational Record; unaffected by this amendment. |
| **ADR-0003 (InitechDOS Base OS Personality)** | Sec 2 is the primary surviving citation of the reconstructed DEC-01; unaffected in substance by DEC-02. |
| **ADR-0009 (SAMIR <-> Milton Integration)** | DEC-01 (soft-float primary, FPU optional) and DEC-07 (`spec/hardware.json` FPU-optional contract) are the numeric-mechanism decisions DEC-04 identifies as now *amendable* (hardware-float option) but does not itself amend. |
| **`docs/worklog/WL-0087-gui-remediation-wave-one.md`** | Source of the `-march=i386` / Bochs `cmov` catch cited by DEC-03. |
| **`docs/design/kernel-runway-relocation.md`** | Source (Part K2) of the `KERNEL_SECTORS<=384` bounce-ceiling design cited by DEC-03. |
| **beads `initech-qw0t`** | This Record. |
| **beads `initech-fgs1`** | Superseded by DEC-02 (closed by the PM as part of this change's session close, not by this docs-lane agent). |
| **beads `initech-zj6w`** | Kernel x87 init path; unlocked (not implemented) by DEC-04. |
| **beads `initech-x0i`** | 86Box driver; DEC-05 sets its future CPU model, not its build status. |

---

*End of ADR-0001. DEC-01 RECONSTRUCTED 2026-09-25; DEC-02 through DEC-05 RATIFIED by the operator 2026-09-25. Controlled Document; verify revision before use.*

<!-- The Record that should have existed since the beginning now does. Tedium, once missing, is now retroactively certified. -->
