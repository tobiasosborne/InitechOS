<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0014 -- Kernel Overlays (FLAIR.OVL: cold kernel code and data read from the boot volume into a fixed overlay window)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0014 |
| Title | Kernel Overlays |
| Version | 0.9 (draft for ratification) |
| Status | **RATIFIED** 2026-10-25 by the operator, with DEC-14-5 = Alternative 5A (preload every section at desktop start). Binding. |
| Classification | Internal Use Only |
| Primary Author | Architecture Review Board, Overlay Committee (Seat A: Toolbox, kernel memory, cooperative pump; Seat B: period ground truth, container, loader and boot risk; Chair: synthesis). Drafted by the Board Secretary. |
| Date | 2026-10-24 |
| Ruling served | Operator 2026-10-24 (bead initech-x8gsi): overlays, not a bounce redesign, for period correctness |
| Related Documents | ADR-0001 (flat 32-bit, no v8086); ADR-0002 (C artifact); ADR-0003 DEC-03 (resident/transient split), DEC-13 + Appendix C (controlled vocabulary), Amendment DEC-08a (InitechMZ; DEC-08a.7 user overlays deferred), Amendment DEC-08b (kernel stack window), Amendment DEC-18 (SYS / FORMAT /S); ADR-0013 + AC-3 (Toolbox gate INT 81h, disk tenants); [docs/design/kernel-runway-relocation.md](../design/kernel-runway-relocation.md) K2; [docs/design/kernel-size-policy-round8.md](../design/kernel-size-policy-round8.md) |
| Evidence base (Law 1) | [docs/research/overlays-period-ground-truth.md](../research/overlays-period-ground-truth.md) |
| Related Issues | initech-x8gsi (this programme, EPIC); initech-oiygj (r3 cap retirement, in parallel); initech-hztg (size round 8); initech-dtw.4 (user AH=4Bh AL=03h overlays, unaffected); initech-x0i (86Box driver) |
| Numbering note | Decisions here are numbered DEC-14-n. They are ADR-0014 decisions and are unrelated to ADR-0003 Amendment DEC-14 (INT 21h user-pointer validation). |

---

## 1. Context

The resident kernel is loaded by stage2 through the real-mode bounce, whose hard cap is
`KERNEL_BOUNCE_CAP` 0x30000 = 196,608 B = 384 sectors (`spec/memory_map.h`;
`Makefile` `KERNEL_SECTORS := 384`). `KERNEL_SECTORS` sits at that ratified ceiling
(kernel-runway-relocation.md K2). The flagship `kernel_flairtenants` loads 180,208 B
(.text 156,841 + .rodata 22,548 + .data 796 + padding), leaving 16,400 B of headroom and a
16 B margin under the r4 cap (kernel-size-policy-round8.md, final acceptance). A feature slice
costs 2-11 KB (kernel-size-policy-round2.md:32). Round 8 exhausted the flag and link levers.
The r3 arrival cap is being retired separately (initech-oiygj).

**Finding A-0 (Seat A).** The binding constraint is the stage2 load window, not RAM. The
unassigned runway `[0x5C0000, KERNEL_STACK_BOT)` is 192 KiB of zeroed, RAM-gated memory
(`spec/memory_map.h`, runway sub-windows), and `.bss` (95,856 B on the flagship) is already
outside the window. Bytes moved out of `INITDOS.SYS` into a disk file are saved whether or not
they are later resident, so an overlay need never be evicted to pay for itself.

**Finding A-1 / Seat B Sec 0 (both seats).** InitechOS already ships a kernel overlay: the
transient COMMAND.COM. It is a flat, position-dependent image linked at a fixed window
(`os/milton/transient.ld`), resolved against its own kernel with `ld --just-symbols`
(`Makefile:10776-10792`), carrying a generated import-address table that the kernel checks
entry by entry before jumping in, refusing a foreign kernel build fail-loud
(`os/milton/transient.h`; `os/milton/kmain.c:exitdos_load_transient`, lines 6132-6211;
gates `harness/emu/execguard.mk`, `harness/emu/exit_dos.mk`). ADR-0003 DEC-03 ratifies the
resident/transient split as a fidelity feature.

**Period ground truth (Law 1; research file Secs 1-4).** dBASE III PLUS ships `DBASE.EXE`
beside two raw, non-MZ overlay files it cannot run without; Turbo Pascal 5.0 puts every overlaid
unit in one `.OVR`, raises run-time error 208 without it and never overlays interrupt handlers;
DOS 3.30 EXEC AL=03h loads an overlay with no PSP.

## 2. Decisions

### DEC-14-1 -- Mechanism: the transient, generalised

Kernel overlays are a generalisation of the transient COMMAND.COM mechanism: a fixed window,
`ld --just-symbols` against the kernel ELF the overlay belongs to, a generated import table
checked address by address at load, and fail-loud refusal of a foreign kernel build. The
generated `--undefined` response file that keeps every imported kernel symbol alive through
LTO (`Makefile:10762-10775`) is reused. No new application model: overlays are kernel code, not
App Contract tenants (ADR-0013), with no PSP, MCB or heap carve. The transient is not migrated.

*Basis:* Findings A-1 and Seat B Sec 0; ADR-0003 DEC-03. Both seats concurred; no dissent.

### DEC-14-2 -- Container: flat sections in one file with a locked header

Each section is a flat, position-dependent image linked at a fixed address inside the overlay
window (DEC-14-3) and produced by `objcopy -O binary`, exactly as `transient.ld` is. All sections
are carried in one file whose header holds, in order: an 8-byte magic (proposed `FLAIROVL`), a
format version, the kernel-build identity, the section directory (per section: id, load address,
file offset, length, bss end), the generated import table (overlay to resident), the generated
export (stub) table (resident to overlay entry points), and a payload CRC-32. The exact layout,
field widths and the identity derivation are locked as Rule-8 spec-data
(`spec/overlay_format.h`, new) atomically with the implementing bead. The file is padded to a
whole number of 512 B sectors (dBASE precedent, research Sec 1.1); the directory, not the file
size, carries lengths.

The kernel holds the expected CRC-32 in a reserved resident dword patched after link (kernel
link, then overlay link against that ELF, then patch; no address moves). An overlay whose import
addresses are unchanged but whose bytes are stale is therefore still refused. AH=4Bh EXEC
refuses a file bearing the overlay magic with DOS error 11, as it refuses the transient today
(`os/milton/loader.c:loader_image_is_transient`).

The container is NOT InitechMZ, for three reasons (Seat B Sec 2): dBASE overlays are not MZ
(`re/disasm-targets.md:60-61`); MZ relocations add the load base to every fixup
(`os/milton/mz.c:mz_apply_relocs`), the wrong fixup class for absolute calls into a fixed resident
kernel; and an overlay has no PSP (TR33 p. 11-5), which the MZ path builds. InitechMZ remains the
application format (DEC-08a, AC-3).

*Dissent:* none on the container; Seat A proposed per-section CRCs, which the layout may carry
under the DEC-14-5 base text (the kernel-held value then covers the directory).

### DEC-14-3 -- Window: a fixed overlay window under a stack guard

A new spec-locked window `OVERLAY_BASE = 0x005C0000`, `OVERLAY_CEIL = 0x005E0000` (128 KiB) is
added to `spec/memory_map.h` at the bottom of the unassigned runway, with new
`MMAP_ASSERT_DISJOINT` rows against TRANSIENT, the kernel image ceiling and the kernel stack.
`[0x5E0000, 0x5F0000)` remains unassigned as a 64 KiB guard below `KERNEL_STACK_BOT`, because the
kernel stack grows down toward the runway (DEC-08b). The window is below `KERNEL_CEIL`, so the
existing `INITECH_MIN_EXT_KB` gate already backs it, and kstart.asm already zeroes it. It is
disjoint from TRANSIENT because the exit-to-DOS path may itself be overlaid and must not be
overwritten by the transient it loads.

*Rejected:* (i) heap carves from the FLAIR window: the load address would depend on launch
order, and a busy desktop could starve a kernel service of memory (AC-3 makes carves fail-loud
budgets); relocation would still not resolve kernel imports. (ii) A single swapped slot (the
DBASE.OVL / TP5 buffer): return addresses into evicted code dangle, code pointers held in
resident data dangle, and no frame chain exists to walk under `-fomit-frame-pointer`
(`Makefile` `KERNEL_COLD_LTO_FLAGS`); TP5 needs FAR frames for exactly this (RG5 p. 160). The
format must not preclude a later eviction design, but none is authorized. No dissent.

### DEC-14-4 -- File: `C:\FLAIR.OVL` on the boot volume

The overlay is one visible file, `FLAIR.OVL`, in the root of the boot volume (primary master,
C: per DEC-18) beside `IO.SYS`, `INITDOS.SYS` and `COMMAND.COM`. It has an 8.3 name, the archive
attribute only (as COMMAND.COM, `tools/mk_boot_system.sh`), and directory date 1980-01-01 00:00
(`touch -d @315532800`; Rule 11). The kernel reads it through the private boot-volume mount
exactly as `exitdos_load_transient` reads COMMAND.COM, which never rebinds drive A:. Every image
that boots a FLAIR tenant kernel carries the `FLAIR.OVL` linked against its own kernel (the
`transient_kernel_%` pattern, `Makefile:10792`).

It is not on the data volume: one `FLAIR_DATA_IMG` is shared by many kernels and mutants, so
the import check would fail on every mutant; the file would be visible and trashable in the
Finder as A:; and it would change every fixture-volume contract. A second file `FLAIRFNT.OVL`
is permitted later only if cold data needs a lifetime different from cold code (dBASE's
DBASE.OVL / DBASEINL.OVL split); it requires an amendment.

*Dissent (naming):* Seat A proposed `INITDOS.OVL`. The Chair ruled `FLAIR.OVL`: the DOS-only
kernels (`build/kernel.bin`, `tracer_boot`) must never want it, and the name must say what it
holds. Those kernels compile no overlay manager and their images ship no overlay.

### DEC-14-5 -- Lifetime: load once, never evict

**Base text.** Each section is loaded on its first call and is never evicted while FLAIR runs; it
is discarded only by Exit to InitechDOS. The header, directory, import table and CRC of the
directory are validated when the desktop starts, before any section is needed, so a missing or
foreign file fails before the user has work to lose. At least one section is loaded before the
`FLAIR-TENANTS-READY` marker, so the Bochs leg covers the read path (Rule 5; Seat B Sec 3); in
lane 1 the JTC section carries a PRELOAD flag for this purpose.

**Alternative 5A (Chair's recommendation for v1).** Every section is read and verified at
desktop start, before `FLAIR-TENANTS-READY`. The window is fixed and the boot-window bytes are
saved either way (Finding A-0), so preloading costs only boot time; it removes any mid-session
overlay panic, and with it the risk of losing unsaved work. The stubs and context guards of
DEC-14-6 remain as defence in depth. The operator chooses between the base text and 5A (Sec 8).

### DEC-14-6 -- Calling convention and context

1. Resident code reaches overlaid code only through generated stubs, compiled with the kernel
   ABI (`KERNEL_ABI_FLAGS`: `-mregparm=3 -mrtd -freg-struct-return`) and the same flags as the
   kernel. A stub checks the section state, loads it if needed, and calls through the export
   table. The export list is generated from the overlay objects, never hand-kept (the
   initech-13ig lesson recorded in `transient.h`).
2. Every code pointer kept in resident data is a stub address, never an overlay address
   (e.g. `egg_jtc_procs`, `os/flair/egg_jtc.c:107`). TP routes procedure pointers the same way
   (RG5 p. 169). Overlay-to-overlay calls also go through stubs.
3. Overlaid code calls the kernel directly through the checked imports (the dBASE `CALLDOS`
   lesson, research Sec 1.2).
4. A load may be initiated only from the event pump or the INT 81h gate. A load attempted from
   an IRQ (`irq_depth() != 0`, `os/milton/irq.h`), from the panic path, or while a disk
   operation is in flight (a new busy flag; the boot and data volumes share the primary ATA
   channel) panics with reason CONTEXT. The slow path does not pump events.
5. Overlaid code may never install an IRQ hook (no import of the pit/kbd/mouse/panic hook
   setters or `idt_*`). Static ELF gates forbid the IRQ, driver and INT 21h objects (isr, irq,
   pit, kbd, mouse, the event producer, int21, fat12, fileio_fat, block_device, ata) from
   referencing any stub. TP5: units with interrupt handlers are not overlaid (RG5 p. 169).
6. A CPU exception with EIP in the overlay window is a kernel fault and panics naming the
   section; it is never unwound as a tenant crash (`tbx_fault_triage` excludes it by range
   today, `os/flair/tbxgate.c`; the context gate asserts that it stays so).

No dissent.

### DEC-14-7 -- Failure model: fail loud, by name

Any bad or missing overlay is the PC LOAD LETTER panic (PRD Appendix B) with a named reason --
`MISSING`, `FOREIGN` (import or identity mismatch), `EXTENTS`, `CORRUPT` (CRC), `CONTEXT`
(DEC-14-6.4) -- and the register dump on serial behind the debug flag. There is no degraded
desktop: greying out Finder operations when the file is absent would make shipped behaviour
depend on disk contents that no oracle grades (Rule 2). The panic path and its raster stay
resident (DEC-14-8), so the missing-overlay panic can draw itself. Period basis: TP5 error 208
(RG5 p. 162), dBASE `INSTALL.BAT` refusal (research Sec 1.3), DOS MSG-DOS-0014.

No console message is added by this Record. Any console wording for these reasons requires a
DEC-13 Appendix C row in ADR-0003 before it ships. `SYS` and `FORMAT /S` do not copy
`FLAIR.OVL`, consistent with DEC-18 not copying COMMAND.COM (MS-DOS 3.3 UG p. 119): a SYS'd
volume booting a FLAIR kernel without the file panics `MISSING`, as a missing COMMAND.COM
yields MSG-DOS-0014. The installation story is the image build. No dissent.

### DEC-14-8 -- What moves: by call frequency, not by file

Code moves when it is cold in the frequency sense, not merely size-optimised
(`KERNEL_COLD_LTO_FLAGS` marks compiler-cold managers, most of which are per-frame or loader
dependencies). A file may be split so that its hot entry stays resident. Seat A's candidates,
attributed from `build/kernel_flairtenants.elf` at b4db95aa (LTO inlining makes these
estimates; each move is measured):

| Family | Resident B | Disposition |
|---|---:|---|
| Finder windows (`finder_windows.c`) | 14,846 | warm: in use while a disk window is open; later lane |
| Finder ops and file ops | 8,986 | cold; lane 2, best yield |
| Standard File | 5,070 | cold; loads in INT 81h context |
| Dialog Manager and alert path | 4,000 | hub; stays resident at first |
| LZ image assets (desk, alert, small; not the logo) | 3,675 | cold data; logo stays (boot screen) |
| Shutdown, exit, power | 2,865 | once per session |
| JTC easter egg (`egg_jtc.c`, data, kmain pad/trigger) | 2,380 | lane 1 |
| Geneva-12 packed strike | 1,524 | cold data |
| Desktop DB persist and load | 1,453 | start and shutdown |
| Geneva-9 strike | 1,140 | every icon label; preload |
| About box | 417 | tiny; joins lane 1 if needed |

**Must stay resident:** all IRQ and exception context, including `panic.c` and the panic raster
(the Chair rejects moving the raster: the missing-overlay panic must draw itself); the ATA,
block-device, FAT and boot-mount chain; the INT 21h core; the per-frame Toolbox (region,
blitter, text, Chicago strike, window/chrome/menu/control/desktop, process, heap,
WaitNextEvent) and `tbx_gate_dispatch`; the event ring; the loader; anything that runs before
the disk is mounted (console, sysinit, IDT, boot screen and logo); and the overlay manager
itself. *Dissent:* the epic listed the panic raster as a candidate; both seats and the Chair
reject it.

### DEC-14-9 -- Gates (Law 2; Rules 5, 6, 11, 14)

Each gate is written RED first and mutation-proven; a gate that has not failed proves nothing.

- **(a) Behaviour equivalence, per moved family.** Serial transcript and screendumps
  byte-identical before and after the move, apart from one fixed `OVL-LOAD` line, and graded on
  the family's existing independent goldens (e.g. `fixtures/easter/*.png`, the mtools FAT
  diffs), so the before/after diff is a regression check and never the correctness oracle
  (ADR-0010 HER-02). Mutant: the family's existing behaviour mutant, rebuilt as a mutant overlay
  on a real kernel, still goes RED; a stub resolved to the wrong export goes RED.
- **(b) Panic gate.** Missing, foreign (import skew, the `TRANSIENT_LAST_IMPORT_SKEW` pattern),
  CRC flip, magic flip and wrong-volume overlays each panic with their named reason and the PC
  LOAD LETTER raster (the `test-panic-screen` checker), never reaching `FLAIR-TENANTS-READY`.
  Each leg has a mutant that disables its check and must go RED.
- **(c) Context gate.** Runtime guards (load with `irq_depth() > 0`, from the panic path, with a
  device busy) plus the static ELF checks of DEC-14-6.5 and a check that no resident `.data` or
  `.rodata` dword points into the overlay window. One mutant per rule.
- **(d) Resident-size gate**, replacing the retired r3: every production kernel loads at or
  under **169,984 B** = 196,608 - 4,096 floor - 2 x 11,264 (two worst-case slices). It is
  honestly RED on landing (the flagship needs -10,224 B) and stays RED until lane 2 lands.
  Mutant kernels stay on the 4,096 B headroom floor. Mutant: cap + 1 pad (the r4 pattern,
  `harness/proptest/kernel_size_r4.mk`).
- **(e) Overlay-window budget gate.** Per-section link ASSERTs and a `.bin` size check (the
  `transient.ld` pattern); mutant: budget + 1 pad.
- **(f) Reproducibility.** `test-kernel-repro` is extended to `FLAIR.OVL` and the minted boot
  image, built twice in different directories and under different locales. The bare `sort -u`
  in the import-table rule (`Makefile:10763`) is pinned to `LC_ALL=C` first.
- **(g) Absence.** An `nm` check that each moved family's symbols are absent from the resident
  ELF (LTO can silently keep a copy).
- **(h) Bochs.** `OVL-OK` is added to the required markers of `test-flair-appswitch-bochs`.
  86Box remains deferred with initech-x0i.
- **(i) Unchanged families stay green:** `test-flair-desktop`, `test-flair-appswitch`,
  `test-kernel-headroom`, `test-kernel-size-r4`, the finder-alerts family
  (`harness/finder_alerts.mk`), `test-flair-execguard`, `test-flair-exit-to-dos`, and the
  memory-map mutant with the new OVERLAY rows, each with its mutant. Rule 14 clips are
  re-recorded through `record-flair` for every moved window feature (lane 1: the JTC mat).

An independent host reader of the header, directory and CRC is written in C under `harness/`
(Law 3), not by reusing the kernel's parser.

### DEC-14-10 -- Sequencing

- **Lane 1: the JTC easter-egg mat** (`egg_jtc.c` and its data, about 1.6 KiB, one section). It
  is hidden, on no boot or IRQ path, launched by vtable, and has independent goldens, a mutant
  gate (`harness/proptest/egg_jtc.mk`) and a record-flair script. First, `egg_jtc_take_pad`
  (`os/flair/egg_jtc.c:27`), polled on every event (`kmain.c:5734`), moves into the resident
  model with `egg_jtc_trigger` (`os/flair/egg_jtc_model.c:48`); otherwise every event would
  fault the section in. The About box joins the lane if the measured net is not negative.
  Lane 1 must leave the flagship no larger than before despite the added manager (the r4 margin
  is 16 B).
- **Lane 2: Finder ops** (about 9 KiB), the first lane that buys real headroom.
- **Then:** Standard File, LZ assets, shutdown and exit, strikes. Each lane is serial,
  re-verified on main, and measured; a move whose net is near zero is not merged.

## 3. Laws and Rules compliance

| Law / Rule | How this Record obeys it |
|---|---|
| Law 1 | Every period claim cites a local file or manual page in the research file; INFERRED items are marked there (G-1..G-5) and not relied on as fact |
| Law 2 | DEC-14-9 gates graded on independent goldens; the before/after diff is never the only oracle |
| Law 3 | Manager, stubs and sections are artifact C; generators are make and thin shell; the host reader is C; no timestamps or host paths reach `FLAIR.OVL` |
| Law 4 | The desktop is byte-identical after each move; the visible `C:\FLAIR.OVL` and the PC LOAD LETTER refusal are period-plausible |
| Rules 2, 5, 6, 8, 11, 12, 14 | DEC-14-7; DEC-14-9(h); mutants on every gate; `spec/memory_map.h` and `spec/overlay_format.h` changed by bead; DEC-14-9(f); ASCII-only sources; clips per moved feature |

## 4. Honesty boundary (Law 1)

**MAY claim:** "Cold parts of the InitechOS FLAIR kernel are read from `C:\FLAIR.OVL` into a fixed
overlay window, in the manner of DBASE.OVL"; "a missing or mismatched overlay is refused by name."
**MAY NOT claim:** "InitechOS supports program overlays" (AH=4Bh AL=03h stays deferred,
initech-dtw.4); "the format is a period format" (it is an Initech format, period in shape only);
"dBASE behaves this way without its overlay" (INFERRED, research G-1).

## 5. Risk register

| # | Risk | Sev | Mitigation |
|---|---|---|---|
| R1 | LTO boundary tax: each import pins a resident symbol global and unspecialised; small families net near zero | High | Measure every lane; DEC-14-10 merge rule |
| R2 | Dangling code pointers or eviction creep | High | Stub-address rule and ELF gate (DEC-14-6.2, 9c); no eviction (DEC-14-5) |
| R3 | Load in a forbidden context (IRQ, panic, nested ATA/FAT, INT 21h) | High | Runtime guards and static gates (DEC-14-6.4-5, 9c) |
| R4 | Half-loaded section after a read error or re-entry | High | State becomes VALID only after length, CRC and import checks; a call to a non-VALID section panics |
| R5 | Stale overlay with unchanged imports | High | Kernel-held CRC-32 patched post-link (DEC-14-2); gate 9b CRC leg |
| R6 | The 16 B r4 margin: the manager itself costs bytes | High | Lane 1 net-negative including the manager; reuse the boot-mount read |
| R7 | Build matrix: about 309 kernels, each overlay valid for one | Med | Per-kernel overlay rule (`transient_kernel_%` pattern) |
| R8 | Harness looks up moved symbols by name | Med | Audit before each move; gate 9g |
| R9 | Mid-session panic loses unsaved work | Med | Header check at desktop start; Alternative 5A removes it |
| R10 | Stack overflow into overlay code; window adjacent to stack and transient | Med | 64 KiB guard and `MMAP_ASSERT_DISJOINT` rows (DEC-14-3) |
| R11 | Boot-image geometry moves when a file is added | Med | Bochs leg (9h); 86Box obligation recorded |
| R12 | Locale-dependent import-table order across hosts | Med | `LC_ALL=C` before reuse; gate 9f varies locale |
| R13 | SYS / FORMAT /S make a C: without `FLAIR.OVL` | Low | DEC-14-7: fails `MISSING` by name, as for COMMAND.COM |
| R14 | Diagnostic wording outside the controlled vocabulary | Low | No console text without an ADR-0003 Appendix C row |

## 6. Accepted costs

Each overlay is valid for exactly one kernel build, as the transient is (`Makefile:10722-10723`);
small families may net near zero under LTO, so every lane measures; 128 KiB of unused runway is
committed; boot reads one more file (more bytes under Alternative 5A).

## 7. Alternatives rejected

A stage2 bounce redesign (the operator's ruling); the InitechMZ container (DEC-14-2); heap carves
and a swapped slot (DEC-14-3); the data volume and `INITDOS.OVL` (DEC-14-4); a degraded desktop
(DEC-14-7); overlaying the panic raster (DEC-14-8); one file per feature (contrary to dBASE's
two files and TP's one `.OVR`).

## 8. Open items for the operator

1. Ratification of this Record.
2. **DEC-14-5:** the base text (load on first call; a mid-session overlay panic can lose unsaved
   work) or Alternative 5A (preload every section at desktop start). The Chair recommends 5A for
   v1.
3. Whether gate DEC-14-9(d) enters the default `make test` vector while RED, or on first green
   with r4 and the headroom floor binding meanwhile.
4. Closing research gap G-1 (a dBASE mint with DBASE.OVL absent) before any user-facing text
   cites dBASE's behaviour.

### Operator resolution (2026-10-25)

1. Ratified.
2. DEC-14-5: **Alternative 5A** -- preload every section at desktop start (the Chair's recommendation).
3. Gate DEC-14-9(d) joins the default `make test` vector on its first green; r4 and the headroom
   floor bind meanwhile (PM default under Law 2: no known-red gate in the default vector).
4. Research gap G-1 stays open; it blocks only user-facing text that cites dBASE's missing-overlay
   behaviour, not implementation.

Trigger: SAMIR overlays Phase A (INT 21h AH=4Bh AL=03h load overlay, its ADR-0003 amendment ratified 2026-10-25 and landing with that lane) costs +832 B and the flagship had
16 B of r4 margin; kernel overlays run first so Phase A lands on freed room (beads initech-x8gsi,
initech-dewv).

## 9. Supersession and acceptance

Supersedes nothing; ADR-0003 DEC-03, DEC-08a (with DEC-08a.7) and DEC-18 are unchanged. On
ratification the implementing beads add the OVERLAY window to `spec/memory_map.h` and create
`spec/overlay_format.h` (Rule 8). This draft is documentation only: nothing was built or run for
it, and naming a gate records its acceptance role, not a claim that it has run.

---

*End of ADR-0014. PROPOSED 2026-10-24; awaiting ratification. Controlled Document; verify revision before use.*

<!-- Tedium certified compliant with NFR-7. The overlay you do not need is retained in full. -->
