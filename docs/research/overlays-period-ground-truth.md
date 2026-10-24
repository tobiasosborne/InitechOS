# Overlays: period ground truth (initech-x8gsi; ADR-0014 evidence base)

Law-1 evidence for [ADR-0014](../adr/ADR-0014-Kernel-Overlays.md). Each fact
names a local file or manual page (printed and PDF page); manual text is quoted
only in short fragments. Sibling trees are relative to this repository's parent.
INFERRED items were not observed locally and must not be cited as fact.

Manuals (local PDFs, checked with `pdftotext -f N -l N -layout`):

| Short | Path under ../initech-manuals-ref |
|---|---|
| PRG3 | `dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Programming with dBase III Plus.pdf` |
| USE3 | `dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Using dBase III Plus.pdf` |
| RG5 | `turbopascal/Turbo_Pascal_Version_5.0_Reference_Guide_1989.pdf` |
| TR33 | `dos33/80X0945_DOS_3.30_Technical_Reference_Apr87.pdf` |

## 1. dBASE III PLUS: a product shipped as an EXE plus overlay files

**1.1 Files and sizes** (`ls -l`, verified 2026-10-24).

| Release | File | Bytes | Sectors | Path in ../dbase3-decomp |
|---|---|---:|---:|---|
| 1.1 | DBASE.EXE | 133,632 | 261 | `mint/work/`, `goldens/dbase-iii-plus-1.1/extracted/disk1/` |
| 1.1 | DBASE.OVL | 266,240 | 520 | `mint/work/`, `.../extracted/disk2/` |
| 1.1 | DBASEINL.OVL | 27,648 | 54 | `mint/work/`, `.../extracted/disk1/` |
| 1.0 | DBASE.OVL | 272,384 | 532 | `goldens/dbase-iii-plus-1.0/extracted/disk2/` |
| 1.0 | DBASEINL.OVL | 27,648 | 54 | `goldens/dbase-iii-plus-1.0/extracted/disk1/` |
| 1.1 Admin | DBA.OVL | 307,712 | 601 | `goldens/dbase-iii-plus-1.1-pristine/files/Administrator_2_REPLACEMENT/` |

Every overlay is an exact multiple of 512 B. DBASE.OVL changed size between
1.0 and 1.1, so an overlay belongs to one release of its EXE. What 1.1 does
when handed the 1.0 overlay was not minted: INFERRED.

**1.2 Format: not MZ.** `file` reports `data` for both OVLs; the decomp notes
record "raw overlay payload, no MZ" (`re/disasm-targets.md:60-61`), while
DBASE.EXE is MZ. The EXE is "the bootstrap loader, not the runtime"
(`re/ghidra-plan.md:58-63`). The OVLs hold zero `CD 21` (INT 21h) bytes and
reach DOS only through the EXE's `CALLDOS` shim (`re/disasm-targets.md:71-81`).
Lesson for ADR-0014 DEC-14-6: calls out of an overlay go through a resident
interface, not inline system calls.

**1.3 All files are required.** `INSTALL.BAT` copies the files, then aborts
unless `dbase.ovl`, `dbase.exe`, `dbase.msg` and `dbaseinl.ovl` all exist
(`goldens/dbase-iii-plus-1.1-pristine/files/System_1/INSTALL.BAT:61-64`), with
`dBASE III PLUS is not installed.  Install is aborted.` (same file, line 150).
The harness plan records DBASE.OVL "REQUIRED at runtime" and DBASEINL.OVL
"must be present" (`re/differential-harness-plan.md:129,137`).

**1.4 The file is held open while the program runs.** PRG3, RunTime+ chapter,
printed p. R4-4 (PDF 518): "the overlay file remains open while running your
applications program". (The seat papers gave R4-3; the R4-3 footer precedes
this paragraph in the text extract, but the rendered page is R4-4.)

**1.5 The OVL anchors related files.** USE3 printed p. U7-7 (PDF 495):
`HELP text not found`, the help file being "not on the same disk or directory
as Dbase.ovl". USE3 printed p. U7-18 (PDF 506): `The HELP file is not
available`, same anchor. (The seat papers gave U7-17; the U7-17 footer
precedes the entry in the extract; the rendered page is U7-18.)

**1.6 Missing-overlay behaviour.** DBASE.EXE carries the strings `DBASE.OVL`,
`DBASEINL.OVL` and `Insert System Disk 2 and press ENTER, or press Ctrl-C to
abort.` (`strings goldens/dbase-iii-plus-1.1/extracted/disk1/DBASE.EXE`,
lines 421, 514-515), and DBASE.OVL ships on System Disk 2
(`re/differential-harness-plan.md:58`). That this prompt is what dBASE does
when DBASE.OVL is absent at run time is INFERRED: no mint in
`re/mint-results-00*.md` runs with an OVL removed. GAP G-1 below.

## 2. Turbo Pascal 5.x: one overlay file, units loaded on demand

**2.1 One .OVR holds every overlaid unit.** RG5 ch. 13, printed p. 159
(PDF 178): Turbo Pascal "manages overlays at the unit level"; the compiler
emits an overlay file (".OVR") beside the .EXE, and "the .OVR file contains
all the overlaid units". Overlays are placed in one overlay buffer between
the stack segment and the heap (same page).

**2.2 Buffer policy.** RG5 printed p. 160 (PDF 179): the manager "always
keeps as many overlays as possible in the overlay buffer" and evicts first
overlays "that have no active routines"; every routine active at an overlaid
call must use the FAR model, and failing that has "unpredictable and possibly
catastrophic results". InitechOS has no FAR frames and builds cold code with
`-fomit-frame-pointer` (Makefile `KERNEL_COLD_LTO_FLAGS`), which is why
ADR-0014 DEC-14-3 rejects a swapped slot.

**2.3 Missing file.** RG5 printed p. 162 (PDF 181), `OvrInit`: it searches the
current directory, the .EXE's directory and PATH; on error "the overlay manager
remains uninstalled", and calling an overlaid routine then produces "run-time
error 208". RG5 printed p. 448 (PDF 467): error 208 `Overlay manager not
installed`; error 209 `Overlay file read error`. The overlay is a hard
dependency that fails loud, by number, at the first overlaid call.

**2.4 What may not be overlaid.** RG5 printed pp. 168-169 (PDF 187-188):
units compiled `{$O-}` (System, Overlay, Crt, ...), and "Units that contain
interrupt handlers", because of "the non-reentrant nature" of DOS. Procedure
pointers to overlaid routines are supported (p. 169): TP routes them through
stubs. Basis of ADR-0014 DEC-14-6 (no overlaid IRQ code; stored code
pointers are stub addresses).

**2.5 Local binaries.** `../turbopascal-decomp/goldens/tp55/x/Disk1/TURBO.TPL`
(44,352 B; TP 5.5, not 5.0; sha256 in `SOURCES.md:21`) carries the Overlay
unit: `strings` shows `OVERLAY.PAS`, `OVRINIT`, `OVRINITEMS`, `OVRRESULT`,
`OVRNOTFOUND`, `OVRIOERROR`, `OVRSETRETRY`, `OVRLOADCOUNT`. TPC.EXE error
strings include `Cannot overlay this unit` and `Cannot compile overlays to
memory` (`specs/compiler-errors.md:177-180`); the IDE default is "Overlays
allowed Off" (`specs/ide-geometry.md:32`). No .OVR file exists locally and
errors 208/209 were never triggered (`GAPS.md` G-8): Sec 2.3 is manual-only.

## 3. DOS 3.30: EXEC AL=03h "Load overlay"

TR33 printed p. 6-196 (PDF 319), function 4Bh, AL value 03H: "Load, do not
create the program segment prefix, and do not begin execution. This is
useful in loading program overlays." Printed p. 6-197 (PDF 320): for AL=03H
the parameter block is a WORD load segment and a WORD "relocation factor to
be applied to the image". Printed p. 11-5 (PDF 416): for function 3 "no PSP
is built", and EXEC "assumes the calling program has already allocated
memory"; it does not check ownership of the target block.

This is the USER-program service, deferred by ADR-0003 Amendment DEC-08a.7
(initech-dtw.4), not the kernel's mechanism. ADR-0014 DEC-14-2 draws two
points from it: a period overlay has no PSP, and its relocation is a segment
factor added at each fixup, a class that does not fit absolute calls into a
fixed-address resident kernel.

## 4. Period-honest user-visible behaviour (derived from Secs 1-3)

- **Names:** product-named 8.3 `.OVL` (dBASE) or `<program>.OVR` (TP); visible
  files beside the executable, copied by plain `copy` (Sec 1.3).
- **Granularity:** one or two large files, not one per feature (dBASE two, split
  by role; TP one, units loaded individually from inside it).
- **Lifetime:** the file is held open for the run; parts are read on demand.
- **Coupling:** an overlay matches exactly one release of its executable.
- **Failure:** refuse to run (INSTALL.BAT), a numbered run-time error (TP
  208/209), or (INFERRED) ask for the disk by name. Nothing silently degrades.

## 5. In-tree precedent (InitechOS, not period)

The transient COMMAND.COM is already a kernel overlay: `os/milton/transient.h`,
`transient.ld`, `transient_hdr.asm`; loader `kmain.c:exitdos_load_transient`
(lines 6132-6211); import rule `Makefile:10762-10775` (bare `sort -u` at 10763);
per-kernel `--just-symbols` link `Makefile:10776-10792`; gates
`harness/emu/execguard.mk`, `exit_dos.mk`. Basis: ADR-0003 DEC-03, MSG-DOS-0014.

## 6. Gaps (Law 1)

- **G-1.** No local run of dBASE III PLUS with DBASE.OVL or DBASEINL.OVL
  absent. Close with one DOSBox or 86Box mint in ../dbase3-decomp before any
  Initech text claims what dBASE "does" without its overlay.
- **G-2.** No local .OVR and no triggered TP error 208/209 (`GAPS.md` G-8).
- **G-3.** dBASE EXE/OVL release-mismatch behaviour not minted.
- **G-4.** Locale dependence of the transient import-table order is inferred
  from the rule text (bare `sort -u`), not demonstrated across hosts.
- **G-5.** The seats' text extracts were scratch files; the PDFs above are the
  citable sources, and every page here was re-checked against them 2026-10-24.
