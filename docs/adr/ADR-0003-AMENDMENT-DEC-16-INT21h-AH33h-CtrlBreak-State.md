<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0003 Amendment DEC-16 -- INT 21h AH=33h Get/Set CTRL-BREAK State (Appendix-A Admission)

> **STATUS: RATIFIED 2026-06-15 (operator T. Osborne, "ratify as drafted").**
> The operator ratified this amendment as drafted. Resolutions: the ^C
> check-point semantics are **Fork A (phased)** -- BREAK OFF checks ^C on
> CON/character-I/O, BREAK ON additionally on entry to every other INT 21h
> call; initech-4tw lands the CON check-point now, the ON-widening to every
> INT 21h call is a non-amendment forward obligation (C-6). Boot default
> **ON** (per the DOS 3.3 PRM, flagged for 86Box confirmation -- erratum-
> flippable per C-7). Class **Resident**, mnemonic **BREAK**, **DL normalized**
> on SET (GET returns 0/1). The proposed `spec/int21h_register.json` row +
> Appendix-A table row (3.4) are AUTHORIZED but are applied **atomically with
> the handler** in the implementing bead (initech-er3h / initech-4tw) per
> CLAUDE.md Rule 8 -- NOT by this document; AH=33h stays undispatched (and out
> of the locked register) until then.

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0003-A4 |
| Title | ADR-0003 Amendment DEC-16: INT 21h AH=33h Get/Set CTRL-BREAK State |
| Version | 1.0 (Ratified) |
| Status | **RATIFIED 2026-06-15 (operator, "ratify as drafted")** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | P. Gibbons (Software Engineering II) |
| Effective Date | 2026-06-15 |
| Next Scheduled Review | (set on ratification; semi-annual per RECORDS-POL-002) |
| Supersedes | (none; admits one new AH function into the ADR-0003 Appendix-A controlled scope) |
| Superseded By | (none) |
| Related Documents | ADR-0003 (OEA-ADR-0003); ADR-0003 Amendment DEC-04a (OEA-ADR-0003-A1); ADR-0003 Amendment DEC-14 (OEA-ADR-0003-A2); ADR-0003 Amendment DEC-15 (OEA-ADR-0003-A3) |
| Related Issues | beads initech-69po (this amendment); initech-4tw (INT 23h / ^C check-point consumer); initech-er3h (CONFIG BREAK= + AH=33h query + BREAK built-in); initech-bsy (epic); initech-40oq (capstone Appendix-A coverage oracle); initech-f9z4 (BREAK shell built-in) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | (draft) | P. Gibbons (Software Engineering II) | Initial committee DRAFT for ARB review. Incorporates RBIL INT 21h/AH=33h, the DOS 3.3 Programmer's Reference Manual Function 33h ABI, the DEC-04a flat 32-bit calling convention, and the DEC-13 controlled-vocabulary discipline. Proposes the single Appendix-A / `int21h_register.json` row, the register contract, the kernel `g_break_flag` state model, and the ^C check-point fork. PENDING OPERATOR RATIFICATION. | -- |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | P. Gibbons (Software Engineering II) | Submitted (DRAFT) | (pending) |
| ARB Reviewer -- Period Authenticity (DOS 3.3 historian) | S. Nagheenanajar (Engineering, Heritage Conformance) | (pending) | (pending) |
| ARB Reviewer -- Robustness / Fail-Loud | M. Bolton (Senior Engineer, Platform) | (pending) | (pending) |
| ARB Reviewer -- Scope / North-Star Minimalism | (Scope function) | (pending) | (pending) |
| ARB Reviewer -- Forward-Compatibility | (Architecture function) | (pending) | (pending) |
| ARB Chair (Synthesis) | (delegated per beads initech-69po) | **Recommendation accepted by operator** | 2026-06-15 |
| Operator | T. Osborne (Operator) | **RATIFIED (as drafted)** | 2026-06-15 |
| Records Management | M. Waddams (Archive Annex B) | (pending) | (pending) |

*Note on committee composition: four independent perspectives were taken
in this DRAFT (period-authenticity per real DOS 3.3; robustness /
fail-loud; scope / north-star minimalism; forward-compatibility). The
Chair's synthesis in this DRAFT is a **recommendation**, not a
ratification. Unlike DEC-15 (where the operator had issued an AMEND-NOW
delegation), this amendment is presented for explicit operator
ratification; the operator may ratify as-drafted, ratify with the
alternative on the contested fork (6), or remand.*

---

## 1. Purpose and Scope

### 1.1 Purpose

The purpose of this Amendment (the "Amendment" or "DEC-16") is to admit
one new INT 21h AH-selected function into the ADR-0003 Appendix-A
controlled scope: **INT 21h Function AH=33h -- Get/Set CTRL-BREAK State**.
This function reads or writes the system-wide BREAK flag, the boolean that
governs *how often* DOS polls for the operator's Ctrl-Break (Ctrl-C)
key-chord and dispatches the INT 23h Ctrl-Break handler.

DEC-04 (ADR-0003 5.4) established the INT 21h AH-function surface and
DEC-13 made Appendix A a controlled vocabulary -- "Functions not listed
are out of scope absent an approved change." AH=33h is **not** in
Appendix A or in `spec/int21h_register.json` today. This Amendment is the
approved change that admits it, fixes its register contract, and records
the state model and the consumer obligations on the gated beads
(initech-4tw, initech-er3h).

### 1.2 Why This IS an Appendix-A Change (contrast with DEC-15)

This is the **mirror-image** governance posture of DEC-15. DEC-15 admitted
INT 25h/26h, which are *separate CPU interrupt vectors* (not AH-selected),
and therefore explicitly did **not** add an Appendix-A row -- it created a
sibling spec file and an explicit Scope-Clause Delta. **AH=33h is the
opposite case.** It is a genuine AH-selected function of the INT 21h
dispatcher, exactly like AH=30h GETVER or AH=25h SETVECT. The correct
governance mechanism is therefore the one DEC-15 deliberately avoided: a
single new **row in Appendix A** and the corresponding single new object
in `spec/int21h_register.json`. There is no new vector, no IDT change, no
PIC change. (See the load-bearing disambiguation in 1.4 -- AH=33h must not
be confused with the INT 23h *vector*, which already exists per DEC-10.)

### 1.3 Out of Scope

The following are expressly out of scope of this Amendment:

- **DOS 5.0+ sub-functions of AH=33h.** Real later DOS overloaded AH=33h
  with AL=02h (legacy CPSW), AL=05h (get boot drive), AL=06h (get true
  DOS version), and AL=07h (set DOS version). These are **post-3.3**
  features and are a period-creep risk for a "3.30" personality (DEC-12,
  NFR-1). Only **AL=00h (get)** and **AL=01h (set)** are in scope. Any
  other AL is rejected loud (DEC-16.2; see Consequence C-5).
- **The INT 23h handler itself and the ^C detection path.** The INT 23h
  vector and its dispatcher already exist (DEC-10; `int23_dispatch`,
  per the initech-4tw notes). The CON-input ^C check-point that reads the
  flag this function controls is implemented under beads initech-4tw; this
  Amendment ratifies the *flag and its contract*, and the *semantics* the
  4tw check-point must honor (DEC-16.3), but does not itself add the
  check-point code.
- **The CONFIG.SYS `BREAK=` parser and the `BREAK` shell built-in.**
  Those are the consumers (beads initech-er3h, initech-f9z4). This
  Amendment fixes the shared `g_break_flag` state and its DOS-3.3 default
  (ON per PRM -- see 3.3 and 4.3) that both the parser and the built-in
  set, and that AH=33h AL=00h reports, but does not itself implement them.
- **Diagnostic-message-catalogue changes.** No new MSG-DOS-NNNN entry is
  added; AH=33h has no operator-visible diagnostic.

### 1.4 Additional Defined Terms

These supplement ADR-0003 1.4 and DEC-04a 1.4. The first two rows are
**load-bearing** and exist to prevent the single highest-risk reading
error of this Amendment -- conflating the AH=33h *function* with the INT
23h *vector*.

| Term | Definition |
|---|---|
| **INT 21h function AH=33h** (the subject) | A *function* of the INT 21h dispatcher (Get/Set CTRL-BREAK State), selected by AH=33h. The subject of this Amendment. It READS or WRITES a boolean; it does NOT itself terminate, poll the keyboard, or invoke the break handler. |
| **INT 23h vector** (`int 0x23`) | The CPU interrupt vector 0x23 = the Ctrl-Break handler, already sanctioned by DEC-10 and installed (`int23_dispatch`). It is the action that fires *when* a ^C is detected. It is **entirely distinct** from the AH=33h function, which only governs the BREAK flag that decides *how often* DOS looks for that ^C. The numeral 23 in "INT 23h" names a vector; the numeral 33 in "AH=33h" names an AH function. |
| **BREAK flag** / `g_break_flag` | The system-wide boolean DOS state read by AH=33h AL=00h and written by AH=33h AL=01h, by `CONFIG.SYS BREAK=ON|OFF`, and by the `BREAK ON|OFF` shell built-in. ON = extended Ctrl-Break checking; OFF = console-I/O-only checking (see 3.3). Default **ON** per the DOS 3.3 PRM (see 4.3 -- to be confirmed against an 86Box golden). |
| **^C check-point** | A point in INT 21h servicing where the kernel tests for a pending Ctrl-Break (0x03) and, if found, invokes the INT 23h vector. Which check-points are active is governed by the BREAK flag (DEC-16.3). Implemented under beads initech-4tw. |

---

## 2. Context

### 2.1 What Is Missing and Why Now

Two gated beads need the BREAK flag and the AH=33h function:

- **initech-4tw** (INT 21h AH=01h/08h/0Ah: Ctrl-C / INT 23h break
  handling): CON input functions must detect 0x03 (^C) and invoke the
  already-installed INT 23h handler. *Whether* a given INT 21h call is a
  ^C check-point at all depends on the BREAK flag this function controls
  (DEC-16.3). 4tw cannot encode authentic DOS break semantics without a
  ratified flag and a ratified check-point model.
- **initech-er3h** (INT 21h AH=33h CTRL-BREAK state + CONFIG BREAK= +
  BREAK built-in): per its own description, AH=33h is presently absent --
  "not even recognized -> 'unknown AH'." It must become a real,
  dispatched function backed by `g_break_flag`, honored from
  `CONFIG.SYS BREAK=` and a `BREAK` built-in.

Per the bsy epic's governing rule ("ADR-0003 Appendix A is the controlled
INT21 scope -- no new funcs without amendment") and CLAUDE.md Rule 8, a
new AH function requires a ratified ADR-0003 amendment before the
`int21h_register.json` row is added. This DRAFT is that amendment.

### 2.2 Why a Function, Not a Vector

ADR-0003 Appendix A ends with the closed-scope clause: *"This register is
the controlled scope. Functions not listed are out of scope absent an
approved change."* AH=33h is selected by the AH byte through the existing
INT 21h dispatch switch (`int21.c`), shares the DEC-04a flat-32-bit ABI,
and returns through the same uniform IRETD frame. It is an Appendix-A
function in exactly the sense AH=30h GETVER is. The approved-change
mechanism is therefore a single Appendix-A row plus the matching
`spec/int21h_register.json` object (3.4) -- no new vector, no IDT/PIC work,
no sibling spec file. This is the cheapest possible governance act and the
correct one.

### 2.3 Relationship to the North Star

Per epic initech-bsy's north-star filter, neither Turbo Initech (tps) nor
InitechBase (samir) calls AH=33h directly. Its value is **DR-1 (Fidelity)**
-- a period DOS exposes Get/Set BREAK -- and **DR-6 (Experiential
Conformance)**: the Office-Space shell feel includes `BREAK ON`/`BREAK OFF`
and a Ctrl-C that actually breaks. It is a small, authentic completion of
the DOS-3.3 controlled scope (it is one of the AH functions the
initech-40oq Appendix-A coverage oracle will count), not a North-Star
advance. DR-5 (continuity with the North Star) is satisfied negatively:
this does not preclude the North Star.

---

## 3. The Decision (DEC-16)

It is hereby **PROPOSED** (DRAFT; pending ratification) that the following
four Sub-Decisions collectively constitute Amendment DEC-16 to ADR-0003.

### 3.1 Sub-Decision DEC-16.1 -- Admission of AH=33h into Appendix A (Resident class)

INT 21h Function **AH=33h (Get/Set CTRL-BREAK State)** is admitted into
ADR-0003 Appendix A (the INT 21h Function Register) and into its locked
transcription `spec/int21h_register.json`, with **Conformance Class =
Resident**. Rationale for the class: Appendix A defines Resident as
"console / date-time / TSR" -- the system-state / console-behaviour family
(00h TERMINATE, 01h-0Ch CON I/O, 2Ah-2Dh DATE/TIME, 30h GETVER, 31h KEEP).
The BREAK flag is a system-wide console-input-behaviour state set at boot
(CONFIG.SYS) and toggled at the console (BREAK built-in), and the function
performs no file/handle/memory operation. It is Resident, not Core. (To be
confirmed against the DOS 3.3 PRM Function 33h listing -- the PRM groups it
with the system-control functions; the historian reviewer must confirm no
Core-class file dependency exists, which there is none.)

The exact row text and the exact JSON object are in 3.4. Per DEC-13
(controlled vocabulary), the row's mnemonic and description become locked
on ratification and may not be reworded except by further amendment.

### 3.2 Sub-Decision DEC-16.2 -- Register Contract (the ABI)

AH=33h shares the DEC-04a flat 32-bit calling convention (trap gate 0x8F
at vector 0x21, CF-via-saved-EFLAGS). It takes **AL** as the sub-function
selector and uses **DL** as the BREAK-flag byte in/out. Only AL=00h and
AL=01h are in scope (1.3).

**Input registers.**

| AL (sub-fn) | Role | Input | Output |
|---|---|---|---|
| **00h** -- GET current BREAK flag | Read the system BREAK flag | (AL=00h only) | **DL = current flag** (0 = OFF, 1 = ON). CF=0. |
| **01h** -- SET BREAK flag | Write the system BREAK flag | **DL = new flag** (0 = OFF, non-zero -> ON) | flag updated; CF=0. (See note on DL>1 below.) |

**Return.** Success: **CF=0**. For AL=00h, **DL** carries the current flag
(0/1); AX is preserved per the DEC-04a benign-superset preservation policy
(only the documented outputs change). For AL=01h, the flag is updated and
CF=0; no register other than the saved flag state changes.

**`DL` normalization on SET (AL=01h).** Real DOS treats any non-zero DL as
"ON". InitechOS shall **store the normalized boolean** (`g_break_flag =
(DL != 0)`), so that a subsequent GET returns exactly 0 or 1 (never a raw
DL like 0xFF). This is a deliberate normalize-on-write choice for a clean,
oracle-checkable round-trip (GET after SET(DL=0xFF) returns DL=1). It is
documented here so the implementer does not store DL verbatim. *Implementer
must confirm against RBIL INT 21h/AH=33h whether real DOS GET ever returns
a non-{0,1} value; the historian's expectation is no -- DOS stores a single
bit -- but this is flagged, not assumed.*

**Error contract.** AL=00h and AL=01h **never set CF** (this is a pure
state read/write with no failure mode -- mirrors the "cf: never set" rows
GETVER/PUTCHAR in `int21h_calling_convention.json`). An **out-of-scope
AL** (02h, 05h, 06h, 07h, or any AL not 00h/01h) is **rejected loud**: it
takes the DEC-04a "listed-but-unimplemented" path -- **CF=1, AX=0x0001
(invalid function)** plus the serial diagnostic `INT21 not-yet-impl
AH=33 AL=NN` -- NOT a silent no-op and NOT a misread as get/set. (CLAUDE.md
Rule 2; this keeps the DOS-5 sub-functions explicitly fenced off, DEC-16
out-of-scope, rather than accidentally honored.)

**Register-role caution (anti-transposition).** Unlike most DEC-04a
functions, AH=33h uses neither EDX-as-pointer nor EBX-as-handle; it uses
**AL as a sub-selector and DL as a 1-byte value**. The dispatcher must read
`f->eax & 0xFF00 >> 8` for AH (as always), then the low byte AL and the DL
byte -- it must NOT treat EDX as a pointer. This is stated to prevent the
field-transposition class of bug DEC-04a Appendix E.3 warns of.

### 3.3 Sub-Decision DEC-16.3 -- The `g_break_flag` State, its Default, and the ^C Check-Point Semantics (the contested fork)

**State.** A single kernel-global boolean **`g_break_flag`** holds the
BREAK state. It is the single source of truth read by AH=33h AL=00h and
written by (a) AH=33h AL=01h, (b) the CONFIG.SYS `BREAK=ON|OFF` directive
at boot (initech-er3h), and (c) the `BREAK ON|OFF` shell built-in
(initech-er3h / initech-f9z4). A bare `BREAK` with no argument prints the
current state (`BREAK is on` / `BREAK is off`) -- *exact wording to be
confirmed against a real DOS 3.3 `BREAK` golden; not a controlled
MSG-DOS-NNNN message, so it is the built-in's own string.*

**Default = ON.** `g_break_flag` initializes to **ON (1)** at kernel boot,
before CONFIG.SYS is read. This is the DOS 3.3 default per the DOS 3.3 PRM
/ RBIL (`BREAK` defaults ON in DOS 3.3+; earlier the documented default was
OFF). *Load-bearing fidelity claim flagged for confirmation: the
period-authenticity reviewer / implementer MUST confirm the DOS 3.3 boot
default is ON against the DOS 3.3 PRM "BREAK" entry or an 86Box round-trip
(boot clean DOS 3.3, run `BREAK` with no CONFIG.SYS line, observe `BREAK
is on`/`off`). If the golden shows OFF, this default flips and the spec-data
default is corrected by editorial erratum without a further full
amendment.* CONFIG.SYS may then override it (the baseline
`spec/dos_config_sys_baseline.txt` currently has **no** `BREAK=` line, so
the boot default stands unless er3h adds one -- see C-4).

**The ^C check-point fork (the contested point -- Chair recommends, operator
ratifies).** The committee split on what the BREAK flag actually changes,
i.e. *which* INT 21h calls become ^C check-points:

- **Fork A -- the documented DOS model (RECOMMENDED).** When BREAK is
  **OFF**, DOS checks for a pending Ctrl-Break **only during the character
  I/O functions** (the AH=01h-0Ch CON family -- console/keyboard/printer
  I/O). When BREAK is **ON**, DOS *additionally* checks for Ctrl-Break on
  the entry to **every other INT 21h function call** (disk reads, writes,
  etc.). The flag thus *widens* the set of check-points from "CON I/O only"
  to "every INT 21h call." This is the model the DOS 3.3 PRM / RBIL
  describe and the one a period programmer expects.

- **Fork B -- minimal: CON-only always; flag is state-only.** Implement
  the ^C check-point ONLY in the CON-input functions (AH=01h/08h/0Ah, per
  4tw) regardless of the flag; treat `g_break_flag` as a *faithfully
  stored and reported* boolean that AH=33h/CONFIG/BREAK get and set, but
  that does not (yet) gate any additional check-points. Defer the
  "every INT 21h call" check-points to a later bead.

**Chair recommendation: Fork A as the ratified *target*, with a phased
implementation that is honest about what 4tw lands now.** Specifically:

1. **4tw lands the CON-input check-point now** (AH=01h/08h/0Ah detect 0x03
   -> invoke INT 23h). This is active whenever BREAK is **ON or OFF**,
   because under Fork A the CON family is *always* a check-point. (When
   OFF, CON-I/O is the *only* check-point; when ON, it is one of many.)
   This is the irreducible minimum and matches both forks for the CON path.
2. **The "every other INT 21h call" check-point (the ON-only widening)**
   is the part that differs between forks. The Chair recommends it be
   ratified as the *documented target semantics* (Fork A) and implemented
   when a non-CON ^C check-point is needed -- but **4tw is NOT required to
   land the full every-call sweep to close**; 4tw closes when the CON-input
   path is correct and `g_break_flag` is wired and respected at the points
   that exist. The spec-data records Fork A as the contract; the
   implementation note records that the ON-widening check-points beyond CON
   are added as the relevant INT 21h services that can block/poll appear.

Rationale for recommending A-as-target over B-as-final: Law 4 fidelity is
about *observable* behaviour, and a period programmer who sets `BREAK ON`
and then mashes Ctrl-C during a long file operation expects it to break --
that is the observable difference. Adopting B as the *final* model would
bake a quietly-wrong fidelity (the flag would be a no-op for its headline
purpose), which is the Law-2 failure mode. But forcing 4tw to implement
every check-point now is scope-creep against a cooperative single-task
kernel where few INT 21h calls actually block. The phased A is the honest
middle: ratify the real semantics, implement them where they bite, never
claim the flag does something it does not.

**What initech-4tw MUST implement (the binding consumer obligation).**
4tw must: (i) detect 0x03 (^C) in the CON-input functions AH=01h/08h/0Ah
and invoke the INT 23h vector instead of delivering 0x03 as an ordinary
char; (ii) read `g_break_flag` rather than hard-coding; (iii) leave a
foot-gun comment citing DEC-16.3 Fork A as the target and noting that
non-CON check-points are added later. 4tw must NOT: deliver ^C as an
ordinary character once the handler path exists (its current behaviour),
nor implement the flag as a literal stored DL (3.2 normalization).

### 3.4 Sub-Decision DEC-16.4 -- The Exact Locked Spec-Data Row (PROPOSAL ONLY)

On ratification, the implementing bead (initech-er3h) shall add **exactly
one row** to ADR-0003 Appendix A and **exactly one object** to
`spec/int21h_register.json`. The proposed edits are below. **They are NOT
applied by this DRAFT (Rule 8 -- the locked edit is a post-ratification
act in the implementing bead).**

**Proposed Appendix-A row** (insert between the `31h KEEP (TSR)` row and
the `36h GETSPACE` row, preserving the existing ascending-AH ordering):

```
| 33h | BREAK | Get/set CTRL-BREAK checking state | Resident |
```

**Proposed `spec/int21h_register.json` object** (insert in the `functions`
array between the `"31h" "KEEP (TSR)"` object and the `"36h" "GETSPACE"`
object, matching the existing ascending order and the exact row schema
`{ ah, mnemonic, description, class }`):

```json
{ "ah": "33h", "mnemonic": "BREAK", "description": "Get/set CTRL-BREAK checking state", "class": "Resident" },
```

Notes on the proposed text (so the historian can adjudicate wording before
it becomes a controlled-vocabulary string):

- **Mnemonic `BREAK`.** Chosen to match the user-facing DOS verb (the
  `BREAK` built-in / `BREAK=` directive). RBIL labels the function
  "GET/SET CONTROL-BREAK FLAG"; the DOS 3.3 PRM uses "Ctrl-Break Check."
  `BREAK` is the period-recognizable token and parallels the other
  Resident mnemonics. *Historian: confirm `BREAK` is acceptable vs a more
  literal `CTRLBREAK`/`GETSETBRK`; once ratified it is DEC-13-locked.*
- **Description** mirrors the terse Appendix-A house style ("Get/set ..."
  as used by 43h CHMOD "Get/set file attributes" and 57h FILETIME). The
  word "checking" disambiguates that it controls *how often DOS checks*,
  not whether Ctrl-Break is "enabled" in any deeper sense.
- **No row is added to `spec/int21h_calling_convention.json` by this
  Amendment**, but the implementer SHOULD add the AH=33h convention stanza
  there as part of er3h (see C-3); doing so is *permitted and encouraged*
  because the test-spec gate requires every convention `ah` to exist in
  the register (which this row supplies). The register row is the
  load-bearing controlled-scope edit; the convention stanza is the ABI
  detail and is consistent with adding the register row first.

---

## 4. Rationale and Committee Deliberation (four perspectives)

### 4.1 Period-Authenticity (real DOS 3.3) -- CONCUR

AH=33h Get/Set CTRL-BREAK is a genuine DOS 1.0+ function present in DOS 3.3
(RBIL INT 21h/AH=33h; DOS 3.3 PRM Function 33h "Ctrl-Break Check"). AL=00h
returns the flag in DL; AL=01h sets it from DL. The DOS-5 overloads
(AL=05h boot drive, AL=06h true version, AL=07h set version) are post-3.3
and correctly excluded for the "3.30" personality (DEC-12). The headline
fidelity claim -- **default ON in DOS 3.3** -- is asserted from the PRM but
is **flagged for an 86Box confirmation** (4.3 / evidence-needed), because
the documented default famously changed between early DOS (OFF) and later
DOS (ON) and getting it wrong is a visible authenticity miss. *No external
contents are fabricated here; the register roles (AL/DL) are confidently
period-correct, the boot default is asserted-pending-confirmation.*

### 4.2 Robustness / Fail-Loud -- CONCUR

The out-of-scope-AL reject (CF=1, AX=0x0001, `not-yet-impl AH=33 AL=NN`)
is the Rule-2 fail-loud posture: the DOS-5 sub-functions are fenced off
explicitly rather than silently misread as get/set. The DL normalization
(`g_break_flag = (DL != 0)`) gives a clean, oracle-checkable round-trip and
removes the "stored 0xFF, GET returns 0xFF" ambiguity. AH=33h itself has no
buffer pointer, so the DEC-14 user-pointer guard is not engaged -- the
function touches no caller memory, only a kernel boolean. The reentrancy
posture is inherited from the existing dispatcher; AH=33h does no I/O and
shares no FAT/sector scratch, so it is reentrancy-trivial.

### 4.3 Scope / North-Star Minimalism -- CONCUR-WITH-COMMENT

This is the cheapest correct governance act: one Appendix-A row, one JSON
object, one kernel boolean. No new vector, no IDT/PIC change, no sibling
spec file (contrast DEC-15). The comment is on the ^C check-point fork
(3.3): the minimalist instinct is Fork B (state-only), but the Chair's
phased-Fork-A keeps the implementation minimal *now* (CON-only check-point
in 4tw) while not ratifying a quietly-wrong flag. The scope reviewer
concurs with phased Fork A on the condition that 4tw is explicitly NOT
required to land the every-INT-21h-call sweep to close -- which 3.3 states.

### 4.4 Forward-Compatibility -- CONCUR

Reserving AH=33h's AL space cleanly (00h/01h in scope; everything else
fail-loud) leaves room for a future DEC to admit DOS-5 AL=05h/06h/07h *if*
the personality ever advances past 3.30 (it should not, per DEC-12, but the
fence is clean). `g_break_flag` as a single global is the right shape for
the cooperative single-task model; if a process model arrives, BREAK is
classically a global (not per-PSP) DOS state, so no per-process migration
is implied. Fork-A-as-target means the "every call" check-points can be
added incrementally without re-amending.

### 4.5 Chair Synthesis

The four perspectives concur on the admission, the class (Resident), the
ABI (AL selector / DL value, 00h/01h only, out-of-scope-AL fail-loud), and
the DL normalization. The one contested point is the ^C check-point fork,
resolved by the Chair to **phased Fork A** (ratify the documented "OFF =
CON-only, ON = every call" semantics as the target; 4tw lands the CON-input
check-point now and is not blocked on the ON-widening sweep). The one
load-bearing open item is the **DOS 3.3 boot default (ON vs OFF)**, which
is asserted ON from the PRM but must be confirmed against an 86Box golden
before the spec-data default is treated as load-bearing (4.1).

---

## 5. Consequences

### 5.1 Binding Constraints (on ratification)

**C-1 -- One new Appendix-A row / one new register object.** AH=33h `BREAK`
Resident, exact text per 3.4. Added to ADR-0003 Appendix A and
`spec/int21h_register.json` by the implementing bead (initech-er3h), NOT by
this DRAFT.

**C-2 -- The register contract is locked.** AL=00h get (DL out), AL=01h set
(DL in, normalized), out-of-scope AL fail-loud (CF=1/AX=0x0001). Any change
requires a further amendment.

**C-3 -- Convention stanza is encouraged, gate stays green.** The
implementer SHOULD add the AH=33h stanza to
`spec/int21h_calling_convention.json` under er3h (`in`/`out`/`cf`/`notes`
shape; cf "never set" for AL=00h/01h, "set on out-of-scope AL" note).
Because C-1 adds the register row, the DEC-04a 7.1 test-spec
AH-consistency gate (every convention `ah` must exist in the register)
stays green. Order matters: register row first (or same commit), then the
convention stanza.

**C-4 -- CONFIG.SYS `BREAK=` is a separate consumer edit.** The baseline
`spec/dos_config_sys_baseline.txt` has **no** `BREAK=` line today, so the
boot default (C-? / 4.3, ON-pending-confirmation) stands. er3h adds the
`BREAK=` parser; *whether* the baseline CONFIG.SYS gains a `BREAK=ON` line
is an er3h decision, not this Amendment's -- and if it does, that is an edit
to a locked spec-data file requiring its own er3h worklog note (Rule 8).

### 5.2 Forward Obligations

**C-5 -- DOS-5 AL sub-functions deferred and fenced.** AL=02h/05h/06h/07h
are out of scope (1.3) and fail loud. A future personality-advance DEC may
admit them; until then they are rejected, never honored.

**C-6 -- The ON-widening ^C check-points (Fork A beyond CON).** Adding the
"every INT 21h call" check-point sweep when BREAK is ON is a forward
obligation realized as the relevant blocking/polling INT 21h services
appear; it does NOT require a further amendment (the semantics are ratified
here), only implementation + an oracle. 4tw closes on the CON-input path
without it.

**C-7 -- 86Box default-ON confirmation.** The DOS 3.3 boot default (ON)
must be confirmed against an 86Box round-trip (or the DOS 3.3 PRM "BREAK"
entry verbatim) before the `g_break_flag` boot-default is treated as
load-bearing; if the golden shows OFF, the default flips by editorial
erratum to the spec-data, no further full amendment (4.3).

### 5.3 Neutral Consequences

- The INT 23h *vector* (DEC-10) is unchanged; AH=33h only governs the flag
  that decides when it is invoked.
- No new MSG-DOS-NNNN diagnostic is added (`spec/dos_messages.json` and
  Appendix C unchanged). The `BREAK is on`/`off` text is the built-in's own
  string (er3h), not a controlled message.
- DEC-15's INT 25h/26h, the MZ loader (DEC-08), the DEC-14 buffer guards,
  and all other Sub-Decisions are unaffected.
- The initech-40oq Appendix-A coverage oracle's denominator gains one
  function (AH=33h), which it SHOULD count (unlike INT 25h/26h, which 40oq
  does not count because they are vectors).

---

## 6. Decisions Surfaced for the Operator

The operator ratifies. Two decisions need an explicit call; the rest of the
Amendment is a coherent package the Chair recommends ratifying as-drafted.

**FORK 1 (load-bearing) -- the ^C check-point semantics.**
- **Fork A (Chair recommendation): phased documented DOS model.** Ratify
  "BREAK OFF = CON-I/O check-point only; BREAK ON = every INT 21h call is a
  check-point" as the target semantics. 4tw lands the CON-input
  check-point now; the ON-widening sweep is a non-amendment forward
  obligation (C-6). Rationale: faithful to the observable DOS behaviour a
  period programmer expects (Law 4) without scope-creeping 4tw.
- **Fork B (alternative): state-only.** `g_break_flag` is faithfully
  get/set/reported but gates no check-points beyond the always-on CON path;
  defer all flag-gated semantics. Cheaper, but bakes a quietly-wrong flag
  (Law-2 risk) and would need a later amendment to add the real semantics.

**FORK 2 -- the DOS 3.3 boot default of `g_break_flag`.**
- **Chair recommendation: default ON**, per the DOS 3.3 PRM, *pending an
  86Box confirmation* (C-7). If the operator or the historian already knows
  the 3.3 default is OFF, flip it before ratification; otherwise ratify ON
  with the C-7 confirmation obligation attached (the default is corrected
  by erratum if the golden disagrees -- CF/AL-style "load-bearing value
  pending golden" posture from DEC-15 3.2).

Lower-stakes items the Chair has decided but flags for visibility:
- **Class = Resident** (3.1) -- recommend ratify as-is.
- **Mnemonic = `BREAK`** (3.4) -- recommend ratify; historian may prefer a
  more literal token, which is a one-word edit before the row is
  DEC-13-locked.
- **DL normalization on SET** (3.2) -- recommend ratify (clean round-trip).

---

## 7. Verification (the oracle this amendment implies; lands with er3h/4tw)

This is a docs+spec DRAFT; no oracle lands with it. On ratification, the
implementing beads add the biting oracles:

### 7.1 The AH=33h round-trip oracle (er3h)

A host `test-int21` (or `test-break`) case, NO QEMU required for the flag
logic:
1. **GET initial** (AL=00h): assert DL == the ratified boot default (1 if
   Fork-2 ON). CF=0.
2. **SET OFF** (AL=01h, DL=0): CF=0. **GET**: DL==0.
3. **SET ON via non-1** (AL=01h, DL=0xFF): CF=0. **GET**: DL==**1** (proves
   the DL normalization of 3.2, not a verbatim 0xFF store).
4. **Out-of-scope AL** (AL=05h): assert CF=1, AX=0x0001, and the
   `not-yet-impl AH=33 AL=05` serial diagnostic (proves the DOS-5 fence).
5. **CONFIG.SYS `BREAK=OFF`** at boot then GET: DL==0 (proves the parser
   writes the same `g_break_flag`); `BREAK=ON` -> DL==1.
6. **`BREAK` built-in** with no arg prints state; `BREAK OFF` then GET ->
   DL==0 (proves the built-in writes the same global).

### 7.2 The ^C check-point oracle (4tw)

With BREAK ON, a 0x03 injected on the CON-input path (AH=01h/08h/0Ah)
invokes the INT 23h handler (observable via the int23 dispatch counter /
serial), NOT delivered as an ordinary 0x03 char. With BREAK OFF, the CON
path still check-points (Fork A: CON is always a check-point). *If Fork A's
ON-widening is implemented for a given non-CON call, an analogous case
asserts a ^C breaks that call only when BREAK is ON.*

### 7.3 Mutation plan (each one-branch perturbation must turn RED -- Rule 6)

| Mutant | Perturbation | RED signal |
|---|---|---|
| M1 | GET returns a hard-coded 1 instead of `g_break_flag` | 7.1 step 2 (SET OFF; GET still 1) |
| M2 | SET stores DL verbatim (drop the `!= 0` normalization) | 7.1 step 3 (GET returns 0xFF, not 1) |
| M3 | out-of-scope AL falls through to "treat as get" | 7.1 step 4 (no CF, no diagnostic) |
| M4 | CONFIG `BREAK=` writes a different/local flag | 7.1 step 5 (GET unchanged) |
| M5 | CON-input delivers 0x03 as a char (drop the ^C check) | 7.2 (int23 dispatch count stays 0) |
| M6 | boot default flipped | 7.1 step 1 (initial GET wrong) -- guards Fork-2 |

### 7.4 Locked-spec authority

On ratification, `spec/int21h_register.json` (the AH=33h row of 3.4) is the
authoritative controlled-scope contract; the optional
`int21h_calling_convention.json` stanza (C-3) is the authoritative ABI
detail and must stay consistent under `make test-spec`.

---

## 8. Implementation Disposition

The contract, on ratification, is recorded here; the kernel handler (the
AH=33h dispatch arm + `g_break_flag` in `int21.c`), the CONFIG.SYS `BREAK=`
parser, the `BREAK` built-in, and the ^C check-point are SEPARATE landings
under beads initech-er3h (AH=33h + CONFIG + built-in) and initech-4tw (the
CON-input ^C check-point). er3h DEPENDS-ON 4tw per the tracker, so the
INT 23h-handler-invocation path lands first; the AH=33h flag and the
CONFIG/built-in consumers land in er3h on top of it. This DRAFT blocks
neither -- it is the governance precondition (the Appendix-A admission) that
both need before the `int21h_register.json` row may be added (Rule 8).

---

## 9. Related Decisions and References

- ADR-0003 (OEA-ADR-0003) -- InitechDOS Base OS Personality. 5.4 (DEC-04
  INT 21h surface), 5.10 (DEC-10 INT 22h/23h/24h handlers), 5.12 (DEC-12
  3.30 personality), 5.13 (DEC-13 controlled vocabulary), Appendix A
  (closed-scope clause; Resident class definition).
- ADR-0003 Amendment DEC-04a (OEA-ADR-0003-A1) -- flat 32-bit ABI,
  CF-via-saved-EFLAGS, the listed-but-unimplemented / unlisted-AH fail-loud
  paths, the 7.1 test-spec AH-consistency gate.
- ADR-0003 Amendment DEC-14 (OEA-ADR-0003-A2) -- fail-loud divergence
  precedent (AH=33h needs no buffer guard; cited for the Rule-2 posture).
- ADR-0003 Amendment DEC-15 (OEA-ADR-0003-A3) -- the CONTRAST case: vectors
  get a sibling file + Scope-Clause Delta, NOT an Appendix-A row; AH=33h is
  the opposite (a true AH function -> Appendix-A row).
- `spec/int21h_register.json` -- the locked Appendix-A transcription; the
  proposed AH=33h row of 3.4 lands here post-ratification (Rule 8).
- `spec/int21h_calling_convention.json` -- the per-function ABI; optional
  AH=33h stanza per C-3.
- `spec/dos_config_sys_baseline.txt` -- baseline CONFIG.SYS (no `BREAK=`
  line today; C-4).
- RBIL INT 21h/AH=33h (Get/Set CTRL-BREAK flag; AL=00h get -> DL, AL=01h set
  <- DL; DOS-5 AL=05h/06h/07h post-3.3, out of scope) -- *register roles
  period-correct; boot default ON to be confirmed.*
- DOS 3.3 Programmer's Reference Manual, Function 33h (Ctrl-Break Check) --
  *historian/implementer to confirm the boot default (ON vs OFF) and the
  `BREAK` mnemonic wording verbatim against this source or an 86Box golden.*
- beads initech-69po (this amendment), initech-4tw (^C check-point),
  initech-er3h (AH=33h + CONFIG + built-in), initech-f9z4 (BREAK built-in),
  initech-40oq (capstone coverage oracle), initech-bsy (epic).

---

*-- End of DRAFT -- PENDING OPERATOR RATIFICATION --*

<!--
This document is the confidential and proprietary information of Initech Systems
Corporation. Unauthorized review, use, disclosure, or distribution is prohibited.
If you have received this document in error, please shred it and notify the Help
Desk (ext. 2504). This footer is part of the controlled document and shall not be
removed. Tedium certified compliant with NFR-7. DRAFT -- not yet a controlled record.
-->
