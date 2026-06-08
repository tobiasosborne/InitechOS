<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0007 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0007 |
| Title | FAT12 Filesystem Mount over ATA, the Proto-DIR, and a Console-Lifetime Defect |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-08 |
| Recording Function | Build Orchestration (supervised multi-agent) |
| Related | WL-0006; ADR-0003 (DEC-06, DEC-07); beads initech-saw, initech-adf, initech-509.3, initech-509.5 |

---

## 1. Purpose

This Record memorializes the first step of Milestone 3 (the file-system handle
layer): mounting a FAT12 volume **in the kernel over the ATA PIO backend** and
rendering a directory listing to the screen. It also records a console-lifetime
defect found by visual verification and the consequent strengthening of the
screendump oracle. **This milestone is IN PROGRESS** — see §5 for the precise
next actions.

## 2. What Landed

- **Second disk + ATA drive-select.** The harness gained `--disk2` (a second
  `-drive ...,if=ide,index=1` = IDE primary slave). `ata.c` was generalized from
  a hardcoded primary-master to a per-device `ata_ctx_t {io_base, ctrl_base,
  drive_select}` carried in the `blockdev_t` context; primary-master (boot) and
  primary-slave (data) initialisers are provided.
- **First emulator validation of `ata.c`.** The ATA PIO backend, written long ago
  but never run on hardware, executed correctly on the first try (QEMU PIIX4
  accepted the READ SECTORS order). Fail-loud first-run guards were added: the
  400 ns post-select delay (four altstatus reads), a floating-bus (0xFF =
  no-drive) check, a **bounded** BSY/DRQ poll (timeout, never an infinite spin),
  and an ERR/DF check.
- **FAT12 mount + proto-DIR.** The kernel mounts the data volume via
  `fat12_mount` (the host-oracle-green reader), and on success renders a
  DOS-style listing (`Directory of A:\` + `NAME.EXT` + size per file) to the LFB
  console and serial. Mount failure is fail-loud-and-continue (a boot with no
  data disk still reaches the halt).
- **Oracle:** `make test-fs` boots with the data disk and asserts no triple-fault,
  the expected filenames + `DIR-OK` on serial, and (now) rendered text in the
  proto-DIR screen band.

## 3. Defect Found and Fixed (Verification Working as Intended)

The subagent's `make test-fs` was green, but visual inspection of the screendump
(Law 4) showed the directory listing was **not on screen** — only on serial. Root
cause: the text console object was declared inside a nested block in
`kernel_main`; its address was stored in the INT 21h / proto-DIR console pointers
and dereferenced long after that block exited, by which time the loader and mount
locals had reused the stack storage. The banner and the program demo rendered by
luck (stack not yet clobbered); the proto-DIR drew through a corrupted console.
The implementing agent had explicitly rationalised the lifetime as safe; it was
not. Fixed at root (Rule 3) by hoisting the console to function scope.

The screendump oracle had **false-passed** because it only asserted foreground
pixels in the banner band, which the banner satisfies regardless of the DIR.
Per Rule 6 the oracle was strengthened: `tools/ppm_text_check.c` gained an
optional `[band_y0 band_y1 min_fg]` argument (backward-compatible — the
single-argument banner callers are unchanged), and `make test-fs` now asserts the
proto-DIR band renders text. The strengthened gate was proven to bite by
reintroducing the defect (gate went red) and restoring.

## 4. Verification of Record

At close of period the full gate vector passes: `test-fs` (mount + proto-DIR +
the DIR-band screendump assertion), `test-boot`, `test-program`, `test-tracer-boot`,
`test-panic`, `test-idt`, `test-int21`, `test-psp`, `test-loader`, `test-console`,
`test-fat`, `test-spec`, plus all `*-mutant` checks. No triple-fault. The banner +
program line + directory listing render on the seafoam desktop (confirmed by eye).
New sources are ASCII-clean and reproducible; `ata.c` compiles freestanding.

## 5. Phase Disposition — MILESTONE 3 IS IN PROGRESS (next agent: start here)

Milestone 3 = the file-system handle layer, decomposed into five steps. **Step 2
(this record) is done; Steps 3–5 remain:**

1. ~~Ground-truth brief~~ — done (`docs/research/fs-mount-sft-ground-truth.md`).
2. ~~FAT12 mount over ATA + proto-DIR~~ — **done (this record).**
3. **NEXT — SFT + JFT + standard handles + DUP/DUP2 (`initech-509.3`).** Build the
   System File Table + the per-process Job File Table indirection (DEC-06);
   pre-open standard handles 0–4 to CON/AUX/PRN; implement DUP (45h) / DUP2 (46h);
   refactor INT 21h 40h WRITE to route through JFT→SFT. Host + boot oracle,
   RED→GREEN, mutation-proven. See the brief §3.
4. **THEN — file-handle INT 21h functions (`initech-509.5` read-side).** 3Dh OPEN
   / 3Fh READ / 3Eh CLOSE / 42h LSEEK over the mounted FAT12 via the SFT; 4Eh/4Fh
   FINDFIRST/FINDNEXT into the DTA (a 43-byte find-data block). A baked test
   program OPENs a file, READs it, and WRITEs it to handle 1 (a `TYPE`) — proving
   a *program* uses the OS to read a real file. The brief §4 recommends
   read-whole-file-into-a-static-buffer at OPEN (do NOT put the file buffer on the
   stack — see the brief Risk 2; the existing `fat12_read_file` already uses a
   ~5.6 KB stack array, tracked by `initech-dao`).
5. **THEN — verify + reconcile beads + worklog** (close `509.3`; advance/close
   `509.5` read-side and `saw`; WL-0008).

The ground-truth brief `docs/research/fs-mount-sft-ground-truth.md` is the plan for
Steps 3–4 (SFT struct design, the find-data/DTA layout `spec/find_data.h`, the
function map). After M3: the shell (`initech-7pc`) gated on CON input / the
keyboard (`initech-n62`, `initech-3rs`); FAT-sourced program load (`initech-saw`)
loads a `.COM` from the volume instead of the baked blob; then the `f8v.4` keystone
(boot → banner → COMMAND.COM → DIR → TYPE) closes.

## 6. Follow-On Items / Open Beads of Note

- `initech-509.3` (SFT/JFT — Step 3), `initech-509.5` (open: file-handle + CON-input
  functions — Step 4), `initech-saw` (FAT-sourced load — after M3).
- `initech-n62` (CON input 01h/06h/0Ah → keyboard), `initech-3rs` (PS/2 + PIT).
- `initech-we2` / `initech-xk2` (DEC-04a forward obligations: ring-3 DPL upgrade;
  INT 21h reentrancy when IRQs are unmasked — relevant once the SFT/find globals
  are touched by IRQ-driven code).
- `initech-dao` (fat12_read_file's on-stack chain buffer — revisit for FAT16/large
  files; relevant to Step 4's OPEN/READ buffering).

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
