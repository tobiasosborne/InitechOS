<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0003 Amendment DEC-08a — Relocatable MZ (.EXE) Loader in 32-Bit Flat Mode ("InitechMZ")

**Issuing Body:** Initech Systems Corporation — Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0003-A6 |
| Title | ADR-0003 Amendment DEC-08a: Relocatable MZ (.EXE) Loader in 32-Bit Flat Mode |
| Version | 1.0 |
| Status | **Ratified** (ADR-by-committee; loader-architect + period-authenticity + north-star lenses; no gridlock) |
| Classification | Internal Use Only |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board (delegated committee), STAPLER Programme |
| Effective Date | 2026-06-20 |
| Next Scheduled Review | 2026-12-20 (semi-annual) |
| Supersedes | **Partial** — supersedes the *deferral clause* of ADR-0003 DEC-08 (§5.8); the flat `.COM`-equivalent provision of DEC-08 is **RETAINED, UNCHANGED**. |
| Related Documents | ADR-0001 (386+, 32-bit flat); ADR-0003 (InitechDOS); ADR-0009 DEC-03 (MZ+overlay authentic SAMIR end-state); CDR-0001 |
| Related Issues | beads initech-dtw (parent); initech-dtw.1 (mz unit), initech-dtw.2 (loader integration), initech-dtw.3 (harness 16-bit differential reader), initech-dtw.4 (overlay, deferred) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |

---

## 1. Context

ADR-0003 DEC-08 ships application executables as flat `.COM`-equivalent images and
**defers** the relocatable MZ `.EXE` loader. The Programme operator has directed that
MZ `.EXE` be implemented in the near term (bead `initech-dtw`), and ADR-0009 DEC-03
names MZ+overlay (`DBASE.EXE`/`DBASE.OVL`) as the authentic end-state packaging.

The architectural tension: a real DOS MZ `.EXE` is a **16-bit segmented** executable
(header, load module, then a relocation table of `{offset, segment}` paragraph fixups;
execution in 16-bit segmented real/protected mode). InitechOS executes **32-bit code in
flat protected mode** (ADR-0001): one flat address space, no segmentation, **no
virtual-8086 mode**. A genuine 16-bit code stream cannot execute correctly on our CPU
mode; loading one and jumping to it would silently misinterpret 16-bit encodings as
32-bit operand-size instructions.

## 2. Decision

InitechDOS shall load executables in the **InitechMZ** format: a **genuine MZ container,
interpreted in flat mode**, by exact analogy to the PSP's Option-B treatment of vestigial
segment fields (`psp.c` `flat_to_fake_paragraph`). The flat `.COM`-equivalent path
(DEC-08) is retained unchanged.

**DEC-08a.1 — The container is real; the code inside is flat; the relocations are flat.**
An InitechMZ executable carries a real MZ header (`e_magic == 0x5A4D` `'MZ'`; load-module
size from `e_cp`/`e_cblp`; header paragraphs `e_cparhdr`; `e_minalloc`/`e_maxalloc`;
`e_ss:e_sp`; `e_cs:e_ip`) followed by the load module and a relocation table. The load
module is **32-bit flat code** (what the InitechOS toolchain and Turbo Initech emit). The
relocation table is a list of **`uint32` flat offsets** into the load module; for each, the
loader **adds the flat load base** (`PROGRAM_IMAGE`, 0x30100) to the **32-bit dword** at
that offset. (An InitechMZ with `e_crlc == 0` is the degenerate position-independent case
and loads with zero fixups.)

**DEC-08a.2 — Layout.** PSP at `PROGRAM_BASE` (0x30000), built unchanged by `psp_build`;
the **load module** placed at `PROGRAM_IMAGE` (0x30100). The MZ path reads the header,
skips `e_cparhdr*16` bytes to the load module, and never copies the header into the image.
`e_cs:e_ip` and `e_ss:e_sp` are flat offsets from the load base; for the current release
the canonical emission is `e_ip=0,e_cs=0` (entry == load base, identical to `.COM`) with
the kernel-supplied `PROGRAM_STACK_TOP` governing ESP. `EBX = PROGRAM_BASE` on entry, as
for a flat `.COM`. The MZ path is a **prologue** on the single existing `loader_run_plan`
transfer (Rule 3 — one transfer path), and the AH=4Bh EXEC ABI is **unchanged**.

**DEC-08a.3 — `e_minalloc`/`e_maxalloc` get real teeth.** These paragraph counts feed the
disjoint MCB arena computation (DEC-04a; ADR-0009 DEC-04). A load whose `e_minalloc*16`
minimum cannot be satisfied below `PROGRAM_ARENA_CEIL` fails loud (DOS function 08h,
insufficient memory), never runs heap-starved. `e_maxalloc` (typically `0xFFFF`) clamps to
the ceiling. This makes the BSS reserve the flat `.COM` path fakes an explicit,
header-declared quantity — strictly *more* authentic.

**DEC-08a.4 — Dispatch is by content, not extension (DOS-authentic).** The loader and the
AH=4Bh path select format on the first two image bytes: `'MZ'` (and the `'ZM'` byte-swap
alias) routes to the InitechMZ path; anything else is the flat `.COM`-equivalent path
(DEC-08, unchanged).

**DEC-08a.5 — The tag byte and the fail-loud honesty gate.** A reserved header field
(the `e_res[0]` word at offset 0x1C, which genuine period linkers write as zero) carries a
nonzero InitechOS sentinel marking "flat-32 relocations." On encountering an **untagged**
MZ — i.e. a genuine 16-bit DOS `.EXE` whose relocations are paragraph fixups over 16-bit
code — the kernel loader **panics fail-loud** (`malformed/foreign MZ`, Rule 2) rather than
relocate-and-misexecute 16-bit code in 32-bit flat mode. The exact field offset and
sentinel value are locked in `spec/` atomically with the implementing bead (`initech-dtw.2`).

**DEC-08a.6 — Genuine 16-bit MZ parsing is a HARNESS-ONLY differential oracle.** A
parse-only reader for genuine 16-bit MZ binaries (real `{offset,segment}` paragraph fixups)
lives in `harness/` **only** — never in `os/milton/`, never on the execution path. It mints
free differential/authenticity goldens (a real period `.EXE` header parses correctly) but
**executes nothing** (bead `initech-dtw.3`).

**DEC-08a.7 — Overlays deferred.** AH=4Bh AL=03h "load overlay" (the `DBASE.OVL` mechanism,
ADR-0009 DEC-03) remains deferred (bead `initech-dtw.4`); the header and relocation
machinery are defined so the overlay loader extends, not replaces, this format.

## 3. Honesty boundary (Law 1)

**MAY claim:** "InitechOS loads and runs MZ `.EXE` programs compiled for the flat 32-bit
InitechOS target"; "the MZ header fields are parsed and honored with real MCB enforcement";
"the harness can parse genuine period 16-bit MZ binaries for a differential oracle."
**MAY NOT claim:** "InitechOS runs genuine 16-bit DOS `.EXE` files" (it cannot — no v8086;
the kernel panics fail-loud on a foreign MZ). "The relocation arithmetic is byte-identical
to DOS" (the InitechMZ fixup is a flat-base add to a 32-bit dword; the tag byte distinguishes
it). "InitechOS supports overlays" (deferred).

## 4. Consequences, risks, and the oracle

- **Principal risk:** the tag-byte discipline lapsing — an untagged 16-bit MZ reaching the
  loader. Mitigation: the fail-loud panic gate (DEC-08a.5) is mandatory and oracle-asserted
  (a test feeds a genuine-16-bit MZ to the kernel loader and asserts it PANICS, not runs).
- **Oracle (Law 2; Rule 6 mutation-proven):** a pure, host-testable `mz_parse_header()` +
  `mz_apply_relocs()` driven by `os/milton/test_mz.c`, plus an independent Python reader
  (`mz_ref.py`, the ADR-0008 independent-reader discipline). Hand-authored InitechMZ fixture
  with known fixups asserted byte-exact post-relocation; mutants `MZ_MUTATE_RELOC_NOADD`
  (skip the base add) and `MZ_MUTATE_RELOC_PARAGRAPH` (add `base>>4` not `base`) must go RED.
  Tri-emulator (Rule 5): boot, EXEC a relocated InitechMZ, assert a serial marker reachable
  only if a relocated absolute reference resolved — on QEMU and Bochs.

## 5. Alternatives rejected

- **Genuine 16-bit segmented execution (v8086 / 16-bit protected segments):** REJECTED —
  a frontal violation of ADR-0001 (flat, no segmentation, no v86) and unnecessary (our apps
  are our own 32-bit code; we never execute a real 16-bit binary). The most faithful in the
  limit; the correct *future* ADR only if literal third-party-binary execution ever enters
  scope.
- **MZ header as pure metadata, no relocation machinery:** REJECTED as the target — the
  operator, ADR-0009 DEC-03, and `initech-dtw` want the vestigial relocation/overlay
  machinery genuinely present ("corporate software accretes / never deletes"). Subsumed as
  the `e_crlc==0` degenerate case of the chosen model.

## 6. Supersession

This Amendment supersedes the deferral clause of ADR-0003 DEC-08 (§5.8). DEC-08's flat
`.COM`-equivalent provision and the flat-binary kernel are RETAINED. Where the PRD or
CLAUDE.md describe MZ `.EXE` as deferred, this Amendment governs.

---

*— End of Amendment DEC-08a —*

<!-- Tedium certified compliant with NFR-7. -->
