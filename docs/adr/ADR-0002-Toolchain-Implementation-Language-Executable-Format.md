<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0002 -- Toolchain, Implementation Language, and Executable Format

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0002 |
| Title | Toolchain, Implementation Language, and Executable Format |
| Version | 1.0-R (Reconstructed) |
| Status | **RECONSTRUCTED 2026-10-17 from the citations; ratification status: as cited** ("Accepted", ADR-0003 Sec 9; recited as ratified in ADR-0003 Sec 2, effective 2026-06-08) |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Office of Enterprise Architecture (Records Reconstruction) |
| Original Effective Date (as cited) | on or before 2026-06-08 (ADR-0003 and CDR-0001, both effective 2026-06-08, cite this Record as already in force) |
| Reconstruction Date | 2026-10-17 |
| Next Scheduled Review | Closure of CDR-0001 (bead `initech-6pm`), per RECORDS-POL-002 |
| Supersedes | (none) |
| Superseded By | (none) |
| Deviations In Force | CDR-0001 (Interim Implementation Toolchain -- Deviation from ADR-0002), effective 2026-06-08, Approved -- Time-Limited |
| Related Documents | ADR-0001 (Target Hardware Platform and Memory Model); ADR-0003 Sec 2, Sec 9, DEC-08 and Amendment DEC-08a (executable format); ADR-0007 DEC-01 (the two-compilers separation that rests on this Record); CDR-0001; InitechOS-PRD.md Sec 1.2, Sec 4, Sec 7, Sec 14, Sec 15; `docs/worklog/WL-0001-Programme-Engineering-Work-Record.md` Sec 7-8 (PNC-1, PNC-2); `docs/worklog/WL-0002-ADR-0003-Reconciliation.md` Sec 2 (the PNC-1 ruling); `docs/HANDOFF.md` Sec 3 (binding decisions); `docs/worklog/WL-0161-adr-0001-0002-reconstruction.md` (this reconstruction) |
| Related Issues | beads `initech-lk29` (this reconstruction); `initech-6pm` (provision the `i686-elf` cross-toolchain; CDR-0001 remediation, open); `initech-msol` (the `KERNEL_CC` / `CC_KERNEL` naming audit) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 1.0 (original, as cited) | on or before 2026-06-08 | (unknown -- no file survives) | The original Record. Its existence, title and status are attested only by citation: ADR-0003 Sec 2 recites it as ratified platform direction ("ADR-0002 (Toolchain and Executable Format; `i686-elf` cross-compilation; C as the implementation language)"); ADR-0003 Sec 9 lists it "*(Accepted.)*"; CDR-0001 deviates from it by id. No draft, markup, or sign-off sheet survives, and no `ADR-0002*` path has ever existed in the repository history (`git log --all -- 'docs/adr/ADR-0002*'` is empty). | (n/a -- no reviewers of record) |
| 1.0-R | 2026-10-17 | OEA (Records Reconstruction) | **Reconstructs the Record from its citations** (bead `initech-lk29`). Each DEC block below states only what the citing corpus attributes to ADR-0002, cites every site that relies on it (Appendix A), and records, in Appendix R, every place the citations disagree with each other or with the shipped build, choosing the reading the shipped code follows. No decision is introduced that the corpus does not already attribute to this Record or to the operator's PNC-1 ruling that reconciled the Programme to it. Mechanically guarded by `make test-adr-citations` (Sec 5). | (n/a -- reconstruction; no operator re-ratification claimed) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| *Rev 1.0 (original) -- no signatories of record survive* | | | |
| *Ratification status* | *as cited: "Accepted" (ADR-0003 Sec 9); operator PNC-1 ruling Option A, 2026-06-08 (WL-0002 Sec 2), reconciled the PRD and CLAUDE.md to it* | | |
| *Rev 1.0-R -- the reconstruction* | | | |
| Author / Reconstructor | Office of Enterprise Architecture (Records Reconstruction) | Submitted (reconstruction) | 2026-10-17 |
| Records Management | M. Waddams (Archive Annex B) | Filed (reconstruction, gap noted) | 2026-10-17 |

*Note on status: this reconstruction confers no authority the original did not already have. Where it states a decision, the authority is the citing document named against that decision (Appendix A), not this file. Where this Record and a citing document ever diverge going forward, the divergence is reconciled through change control (CLAUDE.md Rule 8), and Appendix R is extended -- never silently edited.*

---

## 1. Purpose and Scope

### 1.1 Purpose

CLAUDE.md names "ADR-0001 ... ADR-0002 (toolchain, implementation language, executable format), and ADR-0003" as the ratified architecture of the Programme. Eighty-five mentions on eighty-one lines across CLAUDE.md, the PRD, HANDOFF, nine ADR/CDR files, the worklogs, the plans and the artifact's own source headers cite ADR-0002 as decided authority. The Record itself was never filed. This document files it, so that a governance document cited by id exists (Law 1; bead `initech-lk29`), and so that its decisions are numbered blocks a mechanical gate can hold the corpus to (Rule 8).

### 1.2 The Missing Record

No `docs/adr/ADR-0002-*.md` has ever existed in this repository's history. ADR-0001 Rev 1.0 (reconstructed 2026-09-25) states in its Sec 1.2 that `docs/adr/` "contains ADR-0002 through ADR-0013"; that sentence was inaccurate when written (ADR-0002 was absent, and ADR-0011 is a pending, unfiled Record per `docs/plans/FLAIR-implementation-plan.md`), and is corrected by erratum in ADR-0001 Appendix R. The bead `initech-lk29` (audit 2026-07-09) was narrowed to ADR-0002 alone once ADR-0001 was filed (`docs/worklog/WL-0114-the-second-night.md`, "lk29 (narrow to ADR-0002)").

### 1.3 Reconstruction method (binding on this document)

1. **Every DEC is grounded.** Each DEC states what at least one citing document attributes to ADR-0002 (or to the operator's PNC-1 ruling, which WL-0002 records as the disposition that brought the PRD and CLAUDE.md into conformance with ADR-0002). Appendix A lists every citation site against the DEC it relies on.
2. **No invented decisions.** Matters the corpus never attributes to ADR-0002 -- reproducible builds (CLAUDE.md Rule 11; PRD Sec 7), the `-march=i386` ISA pin (ADR-0001 DEC-03), the cooperative scheduler, memory layout (`spec/memory_map.h`) -- are not decided here, even where they touch the toolchain. They are named only as cross-references.
3. **No DEC number of this Record was ever cited.** Every surviving citation of ADR-0002 cites it whole, by title or by subject. The DEC-01..DEC-05 numbering below is therefore the reconstruction's own, ordered by the title ("Toolchain, Implementation Language, and Executable Format") as ADR-0003 Sec 2 recites its content (language, toolchain, then format), with the two corollaries the corpus attributes to it (the language partition; the factory language) placed after the language and toolchain they qualify. From 2026-10-17 any citation of "ADR-0002 DEC-nn" binds to these numbers and is checked by `make test-adr-citations`.
4. **Conflicts are recorded, not resolved by fiat.** Where citations disagree, Appendix R records both readings and adopts the one the shipped build follows, citing the file.

### 1.4 Scope

In scope: the implementation language of the OS artifact (DEC-01); the toolchain that builds it (DEC-02); the partition between the C OS and Pascal (DEC-03); the factory language as the corpus attributes it to this Record (DEC-04); the executable-format provision named in this Record's title (DEC-05).

Out of scope: the CPU generation and memory model (ADR-0001); the InitechDOS personality and the concrete executable-format rulings (ADR-0003 DEC-08, Amendment DEC-08a); the Turbo Initech language level, codegen and self-host criterion (ADR-0007); the interim toolchain itself (CDR-0001, which deviates from DEC-02 and is not superseded here).

---

## 2. Context

### 2.1 What the corpus says this Record decided

- **ADR-0003 Sec 2** (ratified 2026-06-08): the base OS was decided "pursuant to the platform direction ratified under ADR-0001 (Target Hardware Platform; 386-or-greater; 32-bit protected/flat memory model) and ADR-0002 (Toolchain and Executable Format; `i686-elf` cross-compilation; C as the implementation language)". ADR-0003 Sec 9 lists "ADR-0002 -- Toolchain, Implementation Language, and Executable Format. *(Accepted.)*"
- **CDR-0001 Sec 2**: "ADR-0002 (as recited in ADR-0003 Sec 2) establishes `i686-elf` cross-compilation as the toolchain for the implementation of the Platform." CDR-0001 Sec 3 adds that "the executable-format decisions of ADR-0001 and ADR-0003 (DEC-08) are unaffected by this Deviation."
- **WL-0001 Sec 8, PNC-1**: "The PRD and CLAUDE.md record the OS artifact as Pascal; ADR-0002 (per ADR-0003 Sec 2) mandates C via `i686-elf`. *Escalated for operator ruling.*"
- **WL-0002 Sec 2, PNC-1 disposition** (operator, 2026-06-08): "**Option A.** The OS is implemented in **C**; **Pascal is reserved for Turbo Initech** ... The self-host fixed point concerns Turbo Initech, not the C kernel/Toolbox (which the factory cross-toolchain rebuilds)."
- **ADR-0007 Sec 8**: "ADR-0002 -- Toolchain, Implementation Language, and Executable Format. Establishes C as the OS implementation language and the seed as C; this ADR's DEC-01 rests on that separation."
- **ADR-0005 Amendment AM-1 Sec 8**: "ADR-0002 (implementation language C; no second factory language -- Law 3 ...)".
- **HANDOFF Sec 3** (binding decisions in force): OS implementation language "C (kernel, InitechDOS, Toolbox, bundled apps for now)", source "ADR-0002 / PNC-1"; Toolchain "Target `i686-elf` (ADR-0002). Interim: host `gcc -m32 -ffreestanding -nostdlib` + `nasm` + `ld`", source "CDR-0001"; Executable format "Flat binary kernel; flat `.COM`-equivalent apps; MZ `.EXE` deferred", source "ADR-0003 DEC-08".
- **The artifact's own headers**: thirteen `os/flair/` sources, `os/i123/core/i123.h` and `os/iword/core/iword.h` cite "freestanding C (ADR-0002)" or "ARTIFACT ... (C, ADR-0002)"; `os/milton/io.h` and `os/milton/boot_info.h` cite "CLAUDE.md ADR-0002 + CDR-0001 (freestanding C, gcc -m32 -ffreestanding -nostdlib)".

### 2.2 The PNC-1 reconciliation

Before ADR-0003 was ratified, the PRD and CLAUDE.md described the OS artifact as Pascal; a seed Pascal compiler (bead `initech-znb`) had already been built. WL-0001 raised the divergence from ADR-0002 as PNC-1; the operator ruled Option A on 2026-06-08 (WL-0002; beads memory "ruled-pnc-1-operator-2026-06-08-initechos"). The ruling did not amend ADR-0002: it brought the PRD and CLAUDE.md into conformance with it ("where the PRD or the engineering conventions diverge from a ratified ADR, the ADR shall govern", WL-0001 Sec 4), and re-cast the seed compiler as the genesis of Turbo Initech rather than the OS bootstrap. DEC-03 records the resulting partition, which the corpus cites as "ADR-0002 / PNC-1".

### 2.3 The interim toolchain

A bare-metal `i686-elf` toolchain was not provisioned on the development host. CDR-0001 (operator-approved, time-limited, effective 2026-06-08) authorises the host compiler in freestanding mode in its place, until development moves to a more capable device (bead `initech-6pm`, open). Every OS image the Programme has shipped to date was built under CDR-0001.

### 2.4 The executable-format provision

The Record's title names an executable-format provision, and ADR-0003 Sec 2 recites it as "Toolchain and Executable Format". Every surviving statement of the concrete format, however, is attributed to ADR-0003 DEC-08 (flat binary kernel; flat `.COM`-equivalent applications; MZ `.EXE` deferred -- PNC-2, WL-0002) and to its Amendment DEC-08a (the InitechMZ flat-32 `.EXE`, ratified 2026-06-20): HANDOFF Sec 3, PRD Sec 5 and Sec 15, CDR-0001 Sec 3, CLAUDE.md's hallucination-risk callout. DEC-05 is reconstructed accordingly; the discrepancy is Appendix R item R-1.

---

## 3. The Decision

### DEC-01 -- The OS implementation language is C, freestanding

**AS CITED.** The InitechOS artifact -- the InitechDOS kernel (`IO.SYS` / `INITDOS.SYS` / `COMMAND.COM`), the Toolbox (FLAIR), and the bundled applications for the current release -- is implemented in **C**, compiled **freestanding** for 32-bit x86 (no hosted C library in the artifact; no host headers). The language choice for the bundled applications holds "for the current release" (PRD Sec 14; HANDOFF Sec 3, "for now"); revisiting it is a change to this Record, not a per-app choice.

Assembly is the toolchain's companion language for what C cannot express (the real-mode boot sector and stage2 loader, the kernel entry stub, interrupt-entry stubs): CDR-0001 Sec 3 names `nasm` as part of the toolchain the Platform's artifacts are built with, and CLAUDE.md's file map gives `os/boot/` as "C + asm". The PRD Sec 14 sentence that lists "the bootloader" among the C components is reconciled at Appendix R item R-2: the shipped boot chain is assembly (`os/boot/mbr.asm`, `os/boot/stage2.asm`).

Cited by: CLAUDE.md (What this is; Law 3; file map); PRD Sec 0, Sec 1.2, Sec 4, Sec 6.7, Sec 14, Sec 15; HANDOFF Sec 3; ADR-0003 Sec 2; ADR-0004; ADR-0005; ADR-0008 Sec 1; ADR-0013 Sec 3.1 / Sec 4; the artifact source headers. Full list: Appendix A.1.

### DEC-02 -- The toolchain is `i686-elf` cross-compilation (interim: CDR-0001)

**AS CITED.** The OS artifact is built by the **factory cross-toolchain** targeting **`i686-elf`** (GCC and binutils configured for a bare-metal i686 ELF target). The same toolchain builds and rebuilds the C OS; Turbo Initech does not (DEC-03).

This provision is **under deviation**: CDR-0001 (Approved -- Time-Limited, 2026-06-08) substitutes the host compiler in freestanding mode -- `gcc -m32 -ffreestanding -nostdlib`, with `nasm` for assembly and `ld` for linking -- until the development environment moves to a more capable device; remediation is bead `initech-6pm` (provision `i686-elf` from source and repoint the kernel compiler variable, which the shipped Makefile names `KERNEL_CC`, Makefile line 659 -- Appendix R item R-3). CDR-0001 remains open; this reconstruction neither closes nor widens it.

Not decided here (cross-references only): the `-march=i386` codegen pin and its `cmov` rationale (ADR-0001 DEC-03; Makefile `KERNEL_CFLAGS`); reproducible, timestamp-free builds (CLAUDE.md Rule 11; PRD Sec 7).

Cited by: CLAUDE.md (Toolchain note; Build & test); PRD Sec 4, Sec 7, Sec 14, Sec 15; HANDOFF Sec 3; ADR-0003 Sec 2; CDR-0001 Sec 1-2; WL-0001 PNC-1; WL-0002 Sec 2; `os/milton/io.h`; `os/milton/boot_info.h`; beads `initech-6pm`. Full list: Appendix A.2.

### DEC-03 -- The language partition: the OS is C; Pascal is Turbo Initech's (PNC-1)

**AS CITED (operator PNC-1 ruling, Option A, 2026-06-08).** Pascal is **not** an implementation language of the OS. Pascal is the language of **Turbo Initech** -- the resident, self-hosting compiler (ADR-0007) -- and of the user programs it compiles. Consequently:

- **Two compilers, never conflated.** The seed cross-compiler (`seed/`) is written in C, runs on the host, and is the genesis of Turbo Initech, not the OS bootstrap. The C OS is built and rebuilt by the factory cross-toolchain (DEC-02), never by Turbo Initech.
- **The self-host fixed point (`K2 == K3`) concerns Turbo Initech** and the Pascal programs it compiles, not the C kernel/Toolbox (PRD Sec 7; ADR-0007 DEC-06).
- **Bundled applications are C** (DEC-01): InitechBase/SAMIR, Initech 123, InitechWord and the FLAIR tenants are never Pascal/TPS programs.

ADR-0007 DEC-01 ("What Turbo Initech Is (the two-compilers separation)") rests on this block (ADR-0007 Sec 8).

Cited by: HANDOFF Sec 3 ("ADR-0002 / PNC-1"); WL-0002 Sec 2; WL-0001 PNC-1; PRD Sec 0, Sec 1.2, Sec 4, Sec 6.7, Sec 7, Sec 14; CLAUDE.md (What this is; Law 3; hallucination-risk callout "Two compilers, never conflated"); ADR-0007 Sec 1.3, Sec 4.1 (DEC-01), Sec 5.3, Sec 8; ADR-0008 Sec 1; `docs/plans/SAMIR-implementation-plan.md`; `docs/plans/INITECH-123-plan.md` Sec 8; `docs/plans/INITECH-WORD-plan.md`. Full list: Appendix A.3.

### DEC-04 -- The factory is C; no second factory language

**AS CITED.** Everything that builds and judges the artifact -- the seed cross-compiler (DEC-03), the oracle and emulator harness, the asset tools -- is C (plus thin `make`/shell glue). No second factory language is introduced. This is the PRD Sec 14 decree ("the factory is C, and only C") and CLAUDE.md Law 3; the corpus additionally attributes it to this Record (ADR-0005 Amendment AM-1: "ADR-0002 (implementation language C; no second factory language -- Law 3 ...)"; ADR-0007 Sec 8: "the seed as C"; `tools/ppm_flair_cursor_check.c`: "factory C (ADR-0002 / CLAUDE.md Law 3)"). Factory C is host-built with the host `CC` (Makefile line 25) and is never linked into the artifact; this is not a use of the DEC-02 OS toolchain and not a deviation from it.

Cited by: ADR-0005 Amendment AM-1 Sec 8; ADR-0007 Sec 8; `tools/ppm_flair_cursor_check.c`; `docs/research/dbase-ground-truth.md` Sec 0.1. Full list: Appendix A.4.

### DEC-05 -- Executable format: the toolchain emits flat images; the format rulings are ADR-0003 DEC-08 / DEC-08a

**AS CITED (delegated).** The executable-format provision of this Record is **delegated to the OS-personality Record**: the concrete rulings are ADR-0003 DEC-08 (the resident kernel is a flat binary image; applications are flat `.COM`-equivalent images; the relocatable MZ `.EXE` loader is deferred) as amended by ADR-0003 Amendment DEC-08a (the InitechMZ `.EXE`: a genuine MZ container carrying a flat-32 load module with `uint32` flat-base relocations; untagged genuine 16-bit MZ images are refused). This Record does not restate, narrow or widen those rulings.

What remains to this Record is the toolchain side of the format, as the shipped build follows it: the DEC-02 toolchain's ELF32 objects and links are **build intermediates only**; every image that ships on an InitechOS volume is produced from them as a flat image -- the kernel by `os/milton/kernel.ld` (link address = load address = `KERNEL_BASE`) and `objcopy -O binary` (Makefile, kernel rules), applications as flat `.COM`-equivalent images or as InitechMZ containers (`tools/mzlink.c`; parsed by `os/milton/mz.c` and loaded by `loader_prepare_mz` in `os/milton/loader.c`). No ELF image is an InitechOS executable format.

Cited by: ADR-0003 Sec 2 (the "Executable Format" recital) and Sec 9 (title); ADR-0004 (Related Documents; Sec 9); CDR-0001 (title; Sec 3); ADR-0007 (Related Documents); ADR-0001 (Related Documents; Sec 1.3, Sec 6); CLAUDE.md (What this is). The concrete format is cited to ADR-0003 DEC-08 / DEC-08a by HANDOFF Sec 3, PRD Sec 5 and Sec 15, CLAUDE.md. Full list: Appendix A.5; discrepancy: Appendix R item R-1.

---

## 4. Consequences

### 4.1 Positive Consequences

- A Record mentioned eighty-five times (on eighty-one lines) as ratified architecture now exists, closing the `initech-lk29` gap; CLAUDE.md's "ratified architecture is in `docs/adr/`" is true for ADR-0001, ADR-0002 and ADR-0003 alike.
- The decisions are numbered, so future citations can be precise ("ADR-0002 DEC-03") and are mechanically held to the blocks they name (Sec 5).
- The executable-format split between this Record and ADR-0003 DEC-08, implicit since 2026-06-08, is explicit (DEC-05, R-1).

### 4.2 Negative / Debt Consequences

- DEC-02 has been under deviation for the whole life of the Programme; CDR-0001 and bead `initech-6pm` remain open. This reconstruction records the debt; it does not retire it.
- The PRD Sec 14 bootloader wording (R-2), the `CC_KERNEL` spelling in CDR-0001 and bead `initech-6pm` (R-3), and the HANDOFF Sec 3 "ADR-0007 *pending*" wording (R-4) remain in their documents; they are reconciled here rather than edited, because those documents belong to other change-sets (HANDOFF, beads) or are ratified Records (CDR-0001).

### 4.3 Risks

- A reconstructed Record can be mistaken for an original. Mitigated by the Document Control status line, the Revision History, and the per-DEC "AS CITED" marking: authority stays with the citing documents.
- A future edit could renumber or drop a DEC block that a later citation relies on. Mitigated by `make test-adr-citations` (Sec 5), which is RED on any unresolved "ADR-0002 DEC-nn" citation.

---

## 5. Compliance and Conformance

- **`make test-adr-citations`** (host gate, in `TEST_UNIT_GATES`; `harness/check_adr_citations.c`, `harness/adr_citations.mk`): scans the tree (CLAUDE.md, the PRD, HANDOFF, `docs/`, `spec/`, `os/`, `harness/`, `tools/`, `seed/`, the Makefile) and asserts (F) that ADR-0001, ADR-0002 and CDR-0001, being cited, are each filed under `docs/adr/`, and (D) that every "ADR-0001 DEC-nn" / "ADR-0002 DEC-nn" citation names a DEC heading in that Record. **`make test-adr-citations-mutant`** proves it bites, on copies of `docs/adr/`: with this file removed the gate is RED for the `initech-lk29` reason (ADR-0002 cited, not filed); with ADR-0001's `### DEC-02` heading renamed it is RED naming every site that cites ADR-0001 DEC-02; with this Record's `### DEC-03` heading renamed it is RED naming the sites that cite ADR-0002 DEC-03.
- DEC-01 / DEC-02 (freestanding C, interim toolchain): the kernel build flags (`KERNEL_CFLAGS`, Makefile line 665: `-m32 -march=i386 -ffreestanding -nostdlib ...`) and the reproducibility gates the Programme already runs (`test-kernel-repro`, Rule 11). No new oracle is introduced for them by this Record.
- DEC-03 (language partition): the Turbo Initech oracle ladder (ADR-0007 DEC-07: `test-seed-fpc-diff`, the `test-tps-*` and `test-compiler*` gates) exercises Pascal only through the seed and Turbo Initech; no Pascal source exists in the C artifact trees (`os/milton/`, `os/flair/`, `os/apps/`, `os/samir/`, `os/i123/`, `os/iword/`), Pascal living under `os/tps/` and `seed/` fixtures.
- DEC-05 (format): ADR-0003 DEC-08 / DEC-08a conformance gates (`test-loader`, `test-mz`, `test-mzload`, `test-mzxa-integration`) and the kernel flat-image size guards (`test-kernel-headroom`).

---

## 6. Related Decisions and References

| Document | Relationship |
|---|---|
| **ADR-0001 (Target Hardware Platform and Memory Model)** | Sibling foundational Record. Owns the CPU floor (DEC-01/DEC-02) and the `-march=i386` codegen pin (DEC-03) that DEC-02 here cross-references but does not decide. |
| **ADR-0003 (InitechDOS Base OS Personality)** | Sec 2 recites this Record; Sec 9 lists it Accepted. DEC-08 and Amendment DEC-08a carry the concrete executable-format rulings DEC-05 delegates to. |
| **ADR-0007 (Turbo Initech)** | DEC-01 rests on DEC-03's two-compilers separation (ADR-0007 Sec 8). |
| **CDR-0001 (Interim Implementation Toolchain)** | Deviation from DEC-02; open (bead `initech-6pm`). |
| **InitechOS-PRD.md Sec 4, Sec 7, Sec 14, Sec 15** | The product-level statements of DEC-01..DEC-05. |
| **WL-0001 Sec 7-8; WL-0002 Sec 2** | PNC-1 (language) and PNC-2 (executable format) raised and dispositioned, 2026-06-08. |
| **HANDOFF Sec 3** | Binding-decisions table citing "ADR-0002 / PNC-1" and "ADR-0002" + CDR-0001. |
| **beads `initech-lk29`** | This reconstruction. |
| **beads `initech-6pm`** | `i686-elf` provisioning; closes CDR-0001. |

---

## Appendix A -- Citation inventory (base commit `4755d030`, 2026-10-17)

Every line in the repository (excluding `.beads/`, `.git/`, `build/`) that cites ADR-0002, assigned to the DEC whose meaning the citing text relies on. Sites relying on more than one DEC are listed under each. "Governance" sites cite the Record's existence or absence, not a decision.

### A.1 -- DEC-01 (C, freestanding)

- `CLAUDE.md`: 24, 33, 101, 435
- `InitechOS-PRD.md`: 13, 27, 90, 163, 282, 286, 298
- `docs/HANDOFF.md`: 46
- `docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md`: 107, 284
- `docs/adr/ADR-0004-FLAIR-Toolbox-Architecture.md`: 28, 352
- `docs/adr/ADR-0005-ATKINSON-Region-Engine.md`: 28, 237
- `docs/adr/ADR-0005-AMENDMENT-AM-1-Dual-Heritage-Region-Spine.md`: 354
- `docs/adr/ADR-0008-SAMIR-InitechBase-Architecture.md`: 23, 40
- `docs/adr/ADR-0013-FLAIR-App-Contract.md`: 84, 166
- `docs/worklog/WL-0001-Programme-Engineering-Work-Record.md`: 78
- `docs/worklog/WL-0002-ADR-0003-Reconciliation.md`: 35
- `docs/plans/SAMIR-implementation-plan.md`: 19
- `docs/plans/INITECH-123-plan.md`: 409
- `docs/plans/INITECH-WORD-plan.md`: 46, 441
- `docs/research/dbase-ground-truth.md`: 25
- `os/flair/control.h`: 38; `os/flair/cursor.c`: 17; `os/flair/cursor.h`: 22; `os/flair/dialog.h`: 36; `os/flair/finder_cmd.h`: 49; `os/flair/finder_desktop.c`: 14; `os/flair/finder_icon.c`: 14; `os/flair/finder_menu.h`: 85; `os/flair/finder_ops.c`: 11; `os/flair/finder_windows.c`: 15; `os/flair/menu.h`: 37; `os/flair/text.c`: 17; `os/flair/text.h`: 17
- `os/i123/core/i123.h`: 3; `os/iword/core/iword.h`: 5
- `os/milton/io.h`: 3; `os/milton/boot_info.h`: 6

### A.2 -- DEC-02 (`i686-elf`; interim per CDR-0001)

- `CLAUDE.md`: 322, 323, 339
- `InitechOS-PRD.md`: 90, 170, 282, 300
- `docs/HANDOFF.md`: 49
- `docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md`: 107
- `docs/adr/ADR-0007-Turbo-Initech-Self-Hosting-Pascal-Compiler.md`: 141, 315
- `docs/adr/CDR-0001-Toolchain-Conformance-Deviation.md`: 17, 22, 40, 44
- `docs/worklog/WL-0001-Programme-Engineering-Work-Record.md`: 78
- `docs/worklog/WL-0002-ADR-0003-Reconciliation.md`: 37
- `docs/research/boot-to-text-ground-truth.md`: 127
- `docs/plans/INITECH-123-plan.md`: 409 ("compiled by the factory cross-toolchain")
- `os/milton/io.h`: 3; `os/milton/boot_info.h`: 6

### A.3 -- DEC-03 (language partition; PNC-1)

- `CLAUDE.md`: 33, 101
- `InitechOS-PRD.md`: 13, 27, 90, 163, 170, 286
- `docs/HANDOFF.md`: 46
- `docs/adr/ADR-0007-Turbo-Initech-Self-Hosting-Pascal-Compiler.md`: 28, 77, 141, 315, 347
- `docs/adr/ADR-0008-SAMIR-InitechBase-Architecture.md`: 40
- `docs/adr/ADR-0013-FLAIR-App-Contract.md`: 166
- `docs/worklog/WL-0002-ADR-0003-Reconciliation.md`: 35
- `docs/plans/SAMIR-implementation-plan.md`: 19
- `docs/plans/INITECH-123-plan.md`: 409
- `docs/plans/INITECH-WORD-plan.md`: 441
- `docs/research/dbase-ground-truth.md`: 25

### A.4 -- DEC-04 (factory C only)

- `docs/adr/ADR-0005-AMENDMENT-AM-1-Dual-Heritage-Region-Spine.md`: 354
- `docs/adr/ADR-0007-Turbo-Initech-Self-Hosting-Pascal-Compiler.md`: 347
- `docs/research/dbase-ground-truth.md`: 25
- `tools/ppm_flair_cursor_check.c`: 15

### A.5 -- DEC-05 (executable format, delegated)

- `CLAUDE.md`: 24
- `docs/adr/ADR-0001-Hardware-Target-386-Origin-486-Floor.md`: 29, 77, 165
- `docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md`: 107, 284
- `docs/adr/ADR-0004-FLAIR-Toolbox-Architecture.md`: 28, 352
- `docs/adr/ADR-0007-Turbo-Initech-Self-Hosting-Pascal-Compiler.md`: 28
- `docs/adr/CDR-0001-Toolchain-Conformance-Deviation.md`: 17, 22

### A.6 -- Governance (the Record's existence or absence)

- `docs/adr/ADR-0001-Hardware-Target-386-Origin-486-Floor.md`: 66, 71, 101
- `docs/plans/AUDIT-2026-07-09-improvement-plan.md`: 84
- `docs/audits/2026-10-17-beads-audit.md`: 23, 186, 376
- `docs/worklog/WL-0001-Programme-Engineering-Work-Record.md`: 72
- `docs/worklog/WL-0114-the-second-night.md`: 141
- `.beads/issues.jsonl`: `initech-509` (notes), `initech-6pm`, `initech-lk29`, and the PNC-1 / CDR-0001 memories (not scanned by the gate; listed for completeness)

**No surviving citation names an ADR-0002 DEC number** (`git grep -nE "ADR-0002[^0-9A-Za-z]{0,3}DEC"` at the base commit is empty). The numbering above is the reconstruction's (Sec 1.3).

---

## Appendix R -- Reconciliation of conflicting citations

| # | Conflict | Readings | Adopted reading (what the shipped build follows) |
|---|---|---|---|
| R-1 | **Who owns the executable format.** | (a) This Record's title, ADR-0003 Sec 2 ("ADR-0002 (Toolchain and Executable Format ...)"), ADR-0004, CLAUDE.md: ADR-0002 decides the executable format. (b) HANDOFF Sec 3, PRD Sec 5 / Sec 15, CDR-0001 Sec 3, CLAUDE.md's hallucination-risk callout, WL-0002 PNC-2: the format is ADR-0003 DEC-08, amended by DEC-08a. | **(b) for the format; (a) for the toolchain side only** (DEC-05). The shipped loader and link rules implement ADR-0003 DEC-08/DEC-08a: `os/milton/kernel.ld` cites "ADR-0003 DEC-08 (flat binary handed off by stage2)"; the Makefile kernel section is headed "Flat C kernel (os/milton, beads initech-d00; ADR-0003 DEC-08)" and emits it with `objcopy -O binary`; `os/milton/mz.c` and `loader_prepare_mz` (`os/milton/loader.c`) implement DEC-08a. No shipped source cites ADR-0002 for a format rule. |
| R-2 | **Is the bootloader C?** | (a) PRD Sec 14: "C (per ADR-0002) -- the bootloader, the InitechDOS kernel ...". (b) CLAUDE.md file map: `os/boot/` is "C + asm"; CDR-0001 Sec 3 names `nasm` in the toolchain. | **(b).** The shipped boot chain is assembly: `os/boot/mbr.asm`, `os/boot/stage2.asm`, `os/boot/io_sys.asm`; C begins at the kernel (`os/milton/`), entered from a nasm stub (`os/milton/kstart.asm`). DEC-01 states C as the implementation language with assembly as the toolchain's companion for code C cannot express. The PRD sentence is left as written (product-level shorthand); this row is its reconciliation. |
| R-3 | **The kernel compiler variable.** | (a) CDR-0001 Sec 5 and bead `initech-6pm`: repoint `CC_KERNEL`. (b) Makefile line 659: `KERNEL_CC ?= gcc`; CLAUDE.md Build & test note (audit 2026-07-09, `initech-msol`): "the Makefile variable is `KERNEL_CC`, not `CC_KERNEL`". | **(b) `KERNEL_CC`.** DEC-02 names `KERNEL_CC`. CDR-0001 is a ratified Record and is not edited here; its remediation step is read as "repoint `KERNEL_CC`". |
| R-4 | **Status of ADR-0007 in the DEC-03 partition.** | (a) HANDOFF Sec 3: "Turbo Initech (self-host compiler, ADR-0007 *pending*)"; WL-0002 Sec 2 (2026-06-08): "ADR-0007, pending". (b) PRD Sec 6.7 / Sec 14, CLAUDE.md, ADR-0003 Sec 1.2 / Sec 9: ADR-0007 "ratified 2026-07-11". | **(b).** `docs/adr/ADR-0007-Turbo-Initech-Self-Hosting-Pascal-Compiler.md` is filed and ratified; (a) predates 2026-07-11. DEC-03 cites ADR-0007 as ratified. |
| R-5 | **The Record's title.** | (a) "Toolchain, Implementation Language, and Executable Format" (ADR-0003 Sec 9, CDR-0001, ADR-0007 Sec 8; CLAUDE.md "toolchain, implementation language, executable format"). (b) "Toolchain and Executable Format" (ADR-0003 Sec 2, a recital that then names the language separately). | **(a).** The fuller title is the one cited as a title; (b) is a recital that names all three subjects anyway. |
| R-6 | **"Mandates C via `i686-elf`" vs. "the artifact is Pascal" (historical).** | (a) ADR-0002 as recited in ADR-0003 Sec 2: C. (b) The PRD and CLAUDE.md before 2026-06-08: the artifact is Pascal (WL-0001 PNC-1). | **(a)**, by operator ruling PNC-1 Option A (WL-0002 Sec 2), which brought the PRD and CLAUDE.md into conformance with this Record. Recorded for history; no surviving document still asserts (b). |

---

*End of ADR-0002. RECONSTRUCTED 2026-10-17 from the citations; ratification status: as cited. Controlled Document; verify revision before use.*

<!-- Eighty-five citations, one missing binder. Records Management regrets the delay and has located the stapler. -->
