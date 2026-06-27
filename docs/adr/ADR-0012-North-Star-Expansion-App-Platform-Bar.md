<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0012 -- North-Star Expansion: FLAIR as a Credible Early-1990s Application Platform (PRD Sec 1.4)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0012 |
| Title | ADR-0012: North-Star Expansion -- the App-Platform Bar |
| Version | 1.0 (Ratified) |
| Status | **RATIFIED (operator, 2026-06-27)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Office of Enterprise Architecture (Strategy / Charter) |
| Effective Date | 2026-06-27 |
| Next Scheduled Review | Upon first Phase-4.5 deliverable, per RECORDS-POL-002 |
| Supersedes | (none -- amends PRD Sec 1 Vision in place) |
| Superseded By | (none) |
| Related Documents | InitechOS-PRD.md Sec 1 (Vision & North Star) + Sec 2 (Goals/Non-Goals); ADR-0013 (FLAIR App Contract); ADR-0004 (FLAIR Toolbox Architecture); ADR-0001 (386+, 32-bit flat); ADR-0003 (InitechDOS base OS) + Amendment DEC-08a (InitechMZ flat loader); ADR-0007 (Turbo Initech, pending); docs/plans/FLAIR-implementation-plan.md; spec/win95ism_guardrails.md |
| Related Issues | beads initech-4e35 (Phase 4 App Contract), the new Phase-4.5 "Platform Services" epic + children filed by this decision, initech-t4hp (Phase 6 hosting) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; Presentation Layer Section (FLAIR); QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-06-27 | OEA (Strategy / Charter) | Initial draft. Records the operator's expansion of the InitechOS north star to add a FOURTH commitment (PRD Sec 1.4): FLAIR, executed to plan, must be a *credible early-1990s application platform* -- plausibly shippable circa 1991-93 as a real competitor to System 7 / Windows 3.1 as a place third parties build and run software. Authored from the multi-agent gap analysis (workflow `wf_d1222888-838`, adversarially verified) that established the current plan lands a faithful desktop but a thin app platform. | (pending operator) |
| 1.0 | 2026-06-27 | OEA (Strategy / Charter) | Ratified by the operator in session. D-1..D-5 settled and binding. The two competitor-bar items that remain sanctioned non-goals (real 16-bit binary execution / v8086; real-iron boot) are recorded as ACCEPTED SHORTFALLS (D-3), not silently closed, per CLAUDE.md Stop Conditions. | T. Osborne (Operator) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Office of Enterprise Architecture (Strategy / Charter) | Submitted (DRAFT) | 2026-06-27 |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Approved | 2026-06-27 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Approved | 2026-06-27 |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Approved | 2026-06-27 |
| ARB Reviewer -- Fidelity Steward | Fidelity Steward (Heritage Conformance) | Approved | 2026-06-27 |
| Operator Ratification | T. Osborne (Operator) | **Granted (2026-06-27)** | 2026-06-27 |
| Records Management | M. Waddams (Archive Annex B) | Filed | 2026-06-27 |

*Note on status: This ADR records a CHARTER-LEVEL decision (a north-star expansion) and therefore amends the PRD Vision in place. The PRD Sec 1 and Sec 2 edits are made in the same change-set; where the PRD prose and this ADR ever diverge, this ADR governs and the divergence is reconciled (per HANDOFF Sec 3 / CLAUDE.md read-order).* 

---

## 1. Purpose and Scope

### 1.1 Purpose

This ADR records the operator's decision to **expand the InitechOS north star** with a fourth commitment: that **FLAIR, executed to its roadmap, is a credible early-1990s application platform** -- one a contemporary third party could plausibly have shipped real software on, as a genuine competitor to Apple System 7 and Microsoft Windows 3.1.

Until now the ratified north star (PRD Sec 1) was a triple, in priority order: **1.1 Fidelity** (the Office Space frame as a live, interactive arrangement -- the Turing test), **1.2 Self-hosting** (Turbo Initech reaching the `K2 == K3` fixed point), and **1.3 Anti-vibeOS** (deterministic and real all the way down). All three concern the artifact's *authenticity*. None of them asserts that FLAIR is a *platform* -- a contract that arbitrary applications target. The operator has now made that a goal.

### 1.2 Why this needed a decision (the gap)

A multi-agent gap analysis this session (workflow `wf_d1222888-838`, adversarially verified against the repo and the period record) established the honest state: **executed to its stated terminal state, the FLAIR plan lands a forensically faithful, live, self-hosting chimera desktop -- exactly its own ratified north star -- but not an application platform.** Concretely:

- `os/apps/` and `os/tps/` are empty (`.gitkeep` only); the only live event loop is the kernel-resident pump in `os/milton/kmain.c` behind `-DBOOT_FLAIR_LIVE`, over a HARDCODED scene.
- The **App Contract is undrafted and blocking** (epic `initech-4e35`); the plan and that epic both point it at "ADR-0010", but ADR-0010 is the (unrelated) Grading-and-Goldens ADR -- the App Contract has no design document at all.
- The **resource model is explicitly deferred** (plan P1-8, "locked as apps demand them"); **clipboard/Scrap, TextEdit, Standard File / common dialogs, and printing** are Phase-5 one-liners or absent.
- The loader (`os/milton/loader.c`, `mz.c`) runs ONE program in a single PSP context with zero Toolbox visibility; there is **no multi-app co-residency** (the MultiFinder / cooperative-Windows capability that defined both reference platforms).

That is a perfectly good *desktop* and a thin *platform*. The bar the operator has set requires closing the platform gap deliberately, which is a charter-level act -- it expands scope and brushes two sanctioned non-goals -- so it is recorded here rather than absorbed silently.

### 1.3 Scope

In scope: the addition of PRD Sec 1.4; the annotation of the two affected PRD Sec 2 non-goals as accepted shortfalls; the ratification of the path (the App Contract ADR-0013 + a new Phase-4.5 "Platform Services" epic); the era ceiling that bounds "competitive."

Out of scope (owned elsewhere): the App Contract's technical design (ADR-0013); the per-service designs and oracles (Phase 4.5 beads); the app-hosting arc (ADR-0011 / Phase 6); any change to the priority of Fidelity or Self-hosting (D-5 holds them above the platform bar).

### 1.4 Additional Defined Terms

| Term | Definition |
|---|---|
| The platform bar | "Could a contemporary third party have shipped a real application on this, and would a period developer/user have accepted it as a genuine System 7 / Win 3.1 competitor circa 1991-93?" -- the new Sec 1.4 success criterion. |
| Table-stakes union | The merged set of must-have app-platform capabilities of System 7 + Windows 3.1 that a credible competitor of the era could not omit (the gap-analysis scorecard). The contract-defining list this ADR commits to. |
| Initech version | A re-implemented or recompiled-from-source application native to InitechOS (e.g. InitechBase / SAMIR), as opposed to a real period binary. The project's honest substitute for binary compatibility (D-3a). |
| Era ceiling | The upper bound on "competitive": the System-7.0/7.1 + Win-3.1 era. Win95-era and later mechanisms (e.g. CTL3D chrome, the Win95 palette tokens) are forbidden constants (`spec/win95ism_guardrails.md`), so the bar is "competitive for ~1991-93," not "through 1998." |

---

## 2. Context

The reference platforms, as *application platforms*, shipped (gap-analysis bar): a window system + chrome + damage regions; menus + accelerators; a standard control set; templated dialogs + alerts; a unified cooperative event loop; a device-independent drawing model with the full verb vocabulary; regions as first-class clip objects; patterns / transfer modes / blitting; indexed-8 color; bitmap/picture interchange; text rendering + system fonts; editable text (TextEdit / EDIT); a clipboard (Scrap) with inter-app flavors; Standard File / common dialogs; a **resource model**; an **app loader + multi-app co-residency**; a handle/relocatable memory model; **printing**; a hierarchical filesystem + file API; a shell to launch programs.

FLAIR genuinely has the hardest of these already and oracle-green: the **region engine** (`os/flair/atkinson/region.c`, homomorphism property suite), the **window system + chrome + damage model** (`window.c` / `chrome.c` / `desktop.c`), the **cooperative event pump** (`event.c`), and the **FAT filesystem + INT 21h file API** (MILTON). The platform gap is everything that turns a desktop into a place apps live: the contract, multi-app co-residency, resources, clipboard/text, standard dialogs, printing.

Per CLAUDE.md, a charter-level scope change touching sanctioned non-goals is an operator decision (Stop Conditions). The operator made it. This ADR records it and the honest shortfalls that come with it.

---

## 3. The Decision

The following decisions (D-1..D-5) are RATIFIED.

### D-1 -- Add PRD Sec 1.4 as the fourth, lowest-priority north-star commitment

The PRD Sec 1 north star becomes **four commitments, in priority order**: 1.1 Fidelity, 1.2 Self-hosting, 1.3 Anti-vibeOS, and **1.4 the App-Platform Bar**. Sec 1.4 reads (authored into the PRD in this change-set): *FLAIR, executed to its roadmap, is a credible early-1990s application platform -- a contract real applications target, such that a contemporary third party could plausibly have shipped software on it and a period developer would accept it as a genuine System 7 / Windows 3.1 competitor circa 1991-93.*

Sec 1.4 is **lowest priority** by deliberate placement (D-5): it earns its keep only after, and never at the expense of, the fidelity and self-host commitments.

*Rationale (Law 1).* The PRD already frames Sec 1 as "commitments, in priority order," so the expansion is additive and order-preserving, not a rewrite.

### D-2 -- The bar is the table-stakes union; the path is the App Contract + a Phase-4.5 "Platform Services" epic

The success criterion for Sec 1.4 is coverage of the **table-stakes union** (Sec 1.4 defined term; the gap-analysis scorecard). The committed path:

(a) **Ratify the App Contract (ADR-0013)** -- the tenant model + **multi-app co-residency** over the existing cooperative `WaitNextEvent` pump. This is the load-bearing keystone: every downstream app blocks on it. It supersedes the stale "ADR-0010 = App Contract" references in `docs/plans/FLAIR-implementation-plan.md` (line 9, Phase 4 header) and epic `initech-4e35`, which are corrected to ADR-0013 in this change-set.

(b) **Insert a new Phase 4.5 "Platform Services" epic** between Phase 4 (App Contract) and Phase 5 (frame apps), promoting from one-liners/deferrals to first-class, oracle-backed subsystems: **Resource Manager** (un-defer plan P1-8), **Scrap / clipboard**, **TextEdit + List Manager**, **Standard File / common dialogs** (Open/Save/Color/Font/Print), and a **Print Manager** (device-independent draw-to-printer + Page Setup/Print + spool -- turning `PC LOAD LETTER` from a panic gag into a real, on-brand path).

(c) **Sequence Phase 4.5 BEFORE the canonical app suite** so InitechBase / Initech 123 / InitechWord are built *on the contract + services*, not hardwired -- the difference between "the frame apps run" and "a platform exists."

*Rationale (Law 2).* Each platform service gets its own independent mechanical oracle; "the platform bar is met" is a tracked, mutation-proven checklist, not a vibe.

### D-3 -- Accepted shortfalls (recorded, not silently closed)

Two items the literal competitor bar wants remain **sanctioned non-goals**. They are accepted as KNOWN, RECORDED shortfalls against Sec 1.4 -- the bar is met by honest substitution, not pretended away:

**D-3a -- Real 16-bit DOS/Windows binary execution / v8086 stays OUT (ADR-0001).** This is the Windows-3.1 binary-compatibility moat. InitechOS does not run real 16-bit binaries (no v8086; ADR-0003-DEC-08a's honesty boundary forbids claiming genuine `.EXE` execution). The honest substitute is the **Initech version**: recompiled-from-source or native re-implementation (SAMIR is the precedent). **Consequence stated plainly: the Sec 1.4 bar is met by re-implementation parity ("Initech versions of the era's apps run and interoperate"), NOT by binary compatibility.** A future operator may revisit via a new ADR; until then this shortfall is explicit.

**D-3b -- Real-iron boot stays a STRETCH (PRD Sec 2).** A literal shipping competitor runs on metal. InitechOS targets emulated 386/486 (QEMU dev / Bochs accuracy / 86Box authenticity). The Sec 1.4 bar is therefore read as **"could plausibly have run on period hardware,"** graded by the tri-emulator authenticity regime (Rule 5), not by literal shipping on iron. Promoting real-iron boot from stretch to milestone is a future operator call.

*Rationale (CLAUDE.md Stop Conditions).* Weakening a sanctioned non-goal to chase a bar is an operator decision; recording the shortfall keeps the project honest about what "credible competitor" does and does not mean here.

### D-4 -- The era ceiling bounds "competitive" (~1991-93, not 1998)

"Credible competitor" is scoped to the **System-7.0/7.1 + Win-3.1 era** and bounded above by the Win95-era ceiling (`spec/win95ism_guardrails.md`, enforced by `check-win95isms`). Capabilities that were real but later or marquee-differentiator -- scalable outline fonts (TrueType), OLE 2.0 / compound documents, virtual-memory swapping, multimedia/sound -- are recorded as **acceptable competitive shortfalls** within the era tag, NOT Phase-1 blockers, and are revisited only as the era axis accretes (the future Platinum/System-8 layer). Building Win95-era mechanisms to "win" is forbidden (it violates locked spec).

*Rationale.* The plan's era axis ("never delete, always accrete"; System 7.0/7.1 base now) already fixes the lower era; this decision fixes the upper bound so "competitive" is a dated, falsifiable claim, not an ever-rising target.

### D-5 -- Priority discipline: Sec 1.4 never overrides Fidelity or Self-hosting

Where the platform bar conflicts with **Law 4 fidelity** (PRD Sec 1.1 / Sec 3) or the **self-host finale** (Sec 1.2 / Sec 7), those win. The canon "bugs" stay canon (pie == 116, `570-`, hourglass-not-wristwatch, the Photoshop menu string). A platform feature is never an excuse to break the frame or the fixed point.

*Rationale.* Sec 1.4 is additive value, not a re-prioritization; the project's identity remains the faithful, self-hosting artifact.

---

## 4. Consequences

### 4.1 Binding Constraints

- **BC-1.** The PRD Sec 1 Vision now carries four commitments in priority order; Sec 1.4 is authored in this change-set and is lowest priority (D-1/D-5).
- **BC-2.** "Sec 1.4 is met" is defined by coverage of the table-stakes union and is tracked as a mutation-proven beads checklist; each platform service has an independent oracle (D-2; Law 2).
- **BC-3.** The two accepted shortfalls (D-3a real-binary/v8086; D-3b real-iron) are RECORDED and may be closed only by a new operator ADR, never silently. Marketing/presentation-layer copy must not claim binary compatibility or bare-metal shipping (the reveal stays honest; HANDOFF design stance).
- **BC-4.** No work under Sec 1.4 may introduce a Win95-era constant or mechanism (`check-win95isms`; D-4) or weaken Fidelity / Self-hosting (D-5).

### 4.2 Forward Obligations

- **FO-1.** Amend `InitechOS-PRD.md` Sec 1 (add 1.4) and annotate Sec 2 (the real-iron and preemption/isolation/VM non-goals reference D-3/D-4 as accepted shortfalls vs Sec 1.4). *(Done in this change-set.)*
- **FO-2.** Ratify **ADR-0013 (FLAIR App Contract)** with multi-app co-residency; correct the stale "ADR-0010 App Contract" references in the plan and epic `initech-4e35` to ADR-0013. *(ADR-0013 authored in this change-set; plan/epic corrected.)*
- **FO-3.** File the **Phase 4.5 "Platform Services" epic** + children (Resource Manager, Scrap/clipboard, TextEdit + List Manager, Standard File / common dialogs, Print Manager), each blocked on ADR-0013, sequenced before the canonical suite (Phase 5/6). *(Filed in this change-set.)*
- **FO-4.** Maintain an explicit **shortfall ledger** (the D-3/D-4 items) in the platform-bar epic so "credible competitor" is always read with its honest caveats.

### 4.3 Risks and Honesty Notes

- The Sec 1.4 bar is inherently partial under D-3a (no binary compatibility). This is acceptable and on-brand -- "Initech versions" is the joke played straight -- but it must be stated whenever "real competitor" is claimed.
- Scope-creep risk: Sec 1.4 could pull effort from the fidelity/self-host finale. D-5 + the lowest-priority placement are the guardrails; the operator sequences.

---

## 5. Alternatives Considered

- **Keep the charter as-is (triple north star); treat the competitor framing as aspirational context only.** Rejected by the operator: it leaves the platform gap undeclared and the App Contract drifting, and makes "real competitor" an unowned aspiration rather than a tracked goal.
- **Adopt the bar AND lift the non-goals (build v8086 / real-iron now).** Not chosen: those remain sanctioned non-goals (ADR-0001 / PRD Sec 2); lifting them is a separate, larger decision. Recorded as accepted shortfalls instead (D-3), revisitable by future ADR.

---

## 6. Related Decisions

| Document | Relationship |
|---|---|
| **InitechOS-PRD.md Sec 1 / Sec 2** | Parent. This ADR adds Sec 1.4 and annotates the Sec 2 non-goals; the PRD edits land in the same change-set. |
| **ADR-0013 (FLAIR App Contract)** | The keystone deliverable of D-2(a): the tenant model + multi-app co-residency. Driven by this charter. |
| **ADR-0004 (FLAIR Toolbox Architecture)** | The Toolbox stack the platform bar builds on; mechanism/policy + era-layering already support the bar. |
| **ADR-0001 (386+, 32-bit flat)** | Source of the D-3a v8086 non-goal. |
| **ADR-0003 + DEC-08a (InitechDOS / InitechMZ)** | The loader/executable substrate; the honesty boundary behind D-3a. |
| **ADR-0007 (Turbo Initech, pending)** | The self-host finale (Sec 1.2) that D-5 holds above the platform bar. |
| **docs/plans/FLAIR-implementation-plan.md** | Receives the Phase-4.5 insertion (D-2b) and the ADR-0010->ADR-0013 App-Contract correction (D-2a). |
| **spec/win95ism_guardrails.md** | The era-ceiling enforcement behind D-4. |

---

*End of ADR-0012. RATIFIED by the operator 2026-06-27. Controlled Document; verify revision before use.*
