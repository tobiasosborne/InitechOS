<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0009 Amendment DEC-01a -- Hardware x87 for Initech 123; kernel FPU bring-up in the common boot path

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0009-A1 |
| Title | ADR-0009 Amendment DEC-01a: Hardware x87 for Initech 123; kernel FPU bring-up |
| Version | 1.0 |
| Status | **Ratified** (operator ruling in session, 2026-10-03, on the programme manager's recommendation) |
| Classification | Internal Use Only |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Programme Manager, STAPLER Programme (drafted by the docs lane) |
| Effective Date | 2026-10-03 |
| Next Scheduled Review | 2027-04-03 (semi-annual) |
| Supersedes | **Partial** -- supersedes ADR-0009 DEC-07's `"init_by_kernel": false` and its "DOS never inits the FPU" text, and the DEC-01 deferral of kernel x87. DEC-01's soft-float choice **for SAMIR** is RETAINED, UNCHANGED. |
| Related Documents | ADR-0009 (DEC-01, DEC-07); ADR-0001 (DEC-02, DEC-04); `docs/plans/INITECH-123-plan.md` Sec 5(c), Sec 8; `spec/hardware.json` (`fpu` row); `spec/fpu_contract.h`; `os/milton/fpu.h`; `os/milton/sysinit.c` (`sysinit_fpu_init`); PRD Sec 5 |
| Related Issues | beads initech-yjlo (this amendment); initech-zj6w (kernel x87 bring-up, commit d365f77); initech-94ah (Initech 123 phase P2, control-word policy); initech-x0i (86Box driver) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |

---

## 1. Context

ADR-0009 DEC-01 chose software IEEE-754 double arithmetic for SAMIR and named kernel x87
initialisation "the correct 386+387/486 platform end-state", deferred to a later bead
(initech-zj6w) for when FLAIR or Turbo Initech wanted hardware floating point. DEC-07
accordingly locked `spec/hardware.json` with `"fpu": "optional"` and
`"init_by_kernel": false`. ADR-0001 DEC-04 recorded that the 486-class floor makes an
on-die x87 newly defensible, and INITECH-123-plan Sec 5(c) asked for an explicit ADR-0009
amendment before Initech 123 phase P2 needs the answer.

## 2. Decision

**AM-1. Operator ruling (2026-10-03, in session, on the programme manager's
recommendation): Initech 123 uses hardware x87.** The soft-float alternative offered in
INITECH-123-plan Sec 5(c) is not taken.

**AM-2. SAMIR is unchanged.** SAMIR stays on soft-float exactly as DEC-01 has it
(`-msoft-float -mno-80387`, libgcc helpers). There is no x87 profile for SAMIR.

**AM-3. Hardware contract consequence.** The OS boots and runs without a coprocessor
(`"fpu": "optional"` stands; `os_boots_without_fpu: true`). Initech 123 requires one: a
486DX (on-die x87), or a 387/487. `spec/hardware.json` records this as
`required_by: ["Initech 123"]`.

**AM-4. The kernel initialises the FPU in the common boot path** (bead initech-zj6w,
commit d365f77; `sysinit_fpu_init` in `os/milton/sysinit.c`, called from `sysinit_early`
after the IDT is live and before anything that could execute x87 code):

- CR0 EM=0, MP=1, NE=1, TS=0; then FNINIT.
- A spec-locked boot control word, **0x037F** (`X87_BOOT_CONTROL_WORD` in
  `spec/fpu_contract.h`: the FNINIT value; all exceptions masked, 64-bit significand,
  round to nearest), loaded and read back; a mismatch panics.
- A probe (FNSTSW and FNSTCW over a sentinel; present iff the status low byte is 0 and
  `CW AND 103Fh == 003Fh`, `os/milton/fpu.h`).
- If no FPU is found, EM=1 (MP=0) and boot continues; the first x87 instruction traps #NM
  into the fail-loud panic. With NE=1 an unmasked x87 fault arrives as #MF (vector 16) and
  panics likewise.
- The cooperative single task never switches FPU context; TS stays 0.

**AM-5. Supersession of DEC-07.** `spec/hardware.json` `fpu.init_by_kernel` is now `true`
(Rule 8 deliberate change, recorded in the row's `amendment` field). DEC-07's
`init_by_kernel: false` and the text "DOS never inits the FPU" no longer describe the
contract. The `"fpu": "optional"` clause of DEC-07 stands.

**AM-6. Period deviation, recorded honestly.** ADR-0009's own fidelity review (approval
matrix, Fidelity / Period-Authenticity & Law Steward) called kernel x87 initialisation a
Law-3 period deviation: DOS 3.3 never touched the FPU, and a period application brought its
own runtime. That finding stands. The deviation is accepted for two reasons. First,
InitechDOS runs in 32-bit protected flat mode and itself owns CR0; there is no real-mode
DOS state to inherit, so "leave it alone" is not a neutral choice. Second, the state it
would inherit was measured and differs between emulators (Sec 3).

## 3. Measured evidence (from the initech-zj6w red run, before the init)

- The stage2 handoff left CR0 EM=MP=NE=0 on both QEMU and Bochs. No #NM was raised on
  either.
- Bochs presented the post-RESET control word **0x0040**, exceptions unmasked; QEMU
  presented 0x037F.
- The same x87 sequence returned **0 on Bochs** and **63 on QEMU**, the fault on Bochs
  being routed to the masked IRQ13 instead of faulting.

The init removes that silent wrong answer and the emulator disagreement.

## 4. Precision is not decided here

The kernel deliberately does not choose 53-bit precision. Precision control is
application policy. **Which control word Initech 123 loads is NOT decided by this
amendment**; it is to be settled in Initech 123 phase P2 (bead initech-94ah) by
differential against the real 1-2-3 goldens in `/home/tobias/Projects/lotus123-decomp`.

## 5. Constraints carried forward

- Reproducible builds (CLAUDE.md Rule 11): the init is deterministic and adds no
  timestamps or host state to the artifact.
- The 15-significant-digit display rule (INITECH-123-plan Sec 5(c)) applies to
  Initech 123's display layer regardless of mechanism.

## 6. Open items (undecided or unverified)

1. **Law 1 gap.** The Intel SDM (Vol 3A Sec 2.5, Sec 9.2, Table 9-1; Vol 1 Sec 8.1.5) and
   AP-485 citations are NOT backed by a local copy; section numbers are unchecked.
2. **FPU-absent path** was exercised only by fault injection (`FPU_FORCE_ABSENT`),
   because neither QEMU nor Bochs can remove the x87 unit.
3. **CR0.NE on a real 386.** Writing CR0.NE on a 386, where bit 5 is reserved, is
   unverified.
4. **86Box check** is pending (bead initech-x0i, the 86Box driver).
5. The Initech 123 control word (Sec 4) is undecided pending P2.

---

*-- End of Amendment DEC-01a --*
