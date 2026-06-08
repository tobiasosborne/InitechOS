<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0008 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0008 |
| Title | The System File Table / Job File Table Handle Layer, Predefined Handles 0–4, and DUP/DUP2 I/O Redirection |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-08 |
| Recording Function | Build Orchestration (supervised single-agent) |
| Related | WL-0007; ADR-0003 (DEC-06, DEC-05); beads initech-509.3 (closed), initech-509.5, initech-509.2, initech-6hk (new) |

---

## 1. Purpose

This Record memorializes **Milestone 3, Step 3** (per the WL-0007 disposition):
the **JFT→SFT handle indirection** the ADR-0003 DEC-06 file model rests on. It
establishes the system-wide System File Table, the per-process Job File Table
lookup, the five predefined standard handles (0–4), and the `DUP` (AH=45h) /
`DUP2` (AH=46h) handle-duplication / I/O-redirection primitives — and re-routes
INT 21h `AH=40h WRITE` through the table instead of a hardcoded handle check.
This unblocks Step 4 (file-handle functions: OPEN/READ/CLOSE/LSEEK +
FINDFIRST/FINDNEXT, `initech-509.5` read-side).

## 2. What Landed

- **`os/milton/sft.{h,c}` — the handle layer (new, artifact, host-testable).**
  `sft_entry_t` (kind = FREE/DEVICE/FILE, ref_count, open_mode, dev_id, plus the
  FILE fields `dir_entry`/`file_offset`/`file_data` reserved for Step 4); the
  kernel-global `g_sft[20]` (FILES=20, ADR-0003 Appendix D.2); `sft_init()`
  (device slots 0–3 = CON-in, CON-out (shared stdout+stderr), AUX, PRN);
  `sft_from_handle()` (JFT→SFT resolve, NULL on closed/out-of-range, **fail-loud
  on a corrupt JFT entry**, Rule 2); `jft_alloc()` / `sft_alloc()` (lowest-free +
  saturation); `sft_dup()` / `sft_dup2()` (the duplication + redirect primitives
  with ref-counted slot release). Pure table manipulation, no I/O — compiles
  freestanding (kernel) **and** hosted (oracle), exactly like `psp.c`.
- **Predefined handles 0–4 IN FULL (DEC-06).** `psp.c` now lays
  `jft[3]=0x02` (AUX→slot 2) and `jft[4]=0x03` (PRN→slot 3) in addition to
  stdin/stdout/stderr; `jft[5..19]=0xFF`. AUX/PRN are vestigial (no device
  driver) but **predefined in full** per DEC-06 + the design stance (corporate
  software never deletes; it accretes and ships the cruft). `test_psp.c` updated.
- **INT 21h routed through the SFT.** `int21.c` gained `int21_set_psp()` (the
  current-process binding) and re-implemented `do_write` to resolve the handle
  via `sft_from_handle`: a CON device entry writes to the console (so handles
  1/2 — and anything `DUP2`'d onto CON-write — still reach the screen); AUX/PRN
  and FILE writes return access-denied (no backing yet); a closed/out-of-range
  handle returns invalid-handle. `AH=45h DUP` and `AH=46h DUP2` are now
  dispatched (they leave the "listed-but-deferred" set).
- **Current-PSP lifecycle.** The kernel builds a file-scope `g_kernel_psp` and
  binds it (with `sft_init`) at boot so kernel-context INT 21h has valid standard
  handles before any program loads; the **loader** rebinds to the loaded
  program's PSP (`int21_set_psp(plan.psp_addr)`); the kernel **restores**
  `g_kernel_psp` when `load_program` returns (alongside the existing exit-hook
  restore).

## 3. Oracle (Law 2)

- **`make test-sft` — the Step-3 deliverable oracle (NEW).** 47 checks across
  predefined-handle resolution, allocator lowest-free + saturation, DUP
  (lowest-free new handle, shared SFT slot, `ref_count++`), DUP-exhausts-JFT
  (too-many-open), DUP2 (redirect stdout→a FILE slot, old-target release, file
  slot freed on last reference, `src==dst` no-op, bad src/dst). **Mutation-proven**
  (Rule 6): `make test-sft-mutant` confirms both mutants go RED — a DUP that
  omits `ref_count++` and a DUP2 that omits the old-target release.
- **`make test-int21` strengthened.** Now binds a standard PSP+SFT and exercises
  `AH=40h` through the real JFT→SFT path, plus new dispatch-level coverage of
  `AH=45h`/`AH=46h` (DUP'd handle writes to CON like stdout; DUP2(AUX→stdout)
  makes a subsequent WRITE to handle 1 land on AUX = access-denied — proving the
  redirect changed the handle's backing). 48 checks; both prior mutants still bite.
- **Full gate vector green.** `test-sft`(+mutant), `test-int21`(+mutant),
  `test-psp`(84,+mutant), `test-loader`(+mutant), `test-boot`, `test-program`
  (program runs through the loader-bound PSP and prints via INT 21h, returns
  rc=0), `test-fs` (mount + proto-DIR), `test-panic`, `test-tracer-boot`,
  `test-console`, `test-idt`(+mutant), `test-fat`, `test-spec`. No triple-fault.
  New sources ASCII-clean (Rule 12), reproducible, freestanding-clean.

## 4. Decisions / Frictions of Record

- **`AH=40h` to a CON device ignores the slot's nominal mode.** DOS treats CON as
  the writable console regardless of read/write designation, so the DUP2 oracle
  redirects onto **AUX** (no driver → access-denied) rather than stdin to keep
  the observable result unambiguous.
- **Single-process scope (documented limitation).** `sft_init()` runs once; the
  SFT is **not** torn down on process EXIT, so a future second program would
  inherit a prior program's open/redirected slots. Correct under the cooperative
  single-program model; filed as **`initech-6hk`** for the multi-process /
  COMMAND.COM-relaunch milestone.
- **`initech-509.3` force-closed over its `509.2` dependency.** The formal
  dependency (IO.SYS/INITDOS.SYS two-file partition + a distinct SYSINIT phase,
  DEC-01) is not yet built; `sft_init` + the kernel-PSP bind run in `kernel_main`
  (the de-facto SYSINIT). When 509.2 lands, relocate them into the real SYSINIT.
  WL-0007 designated 509.3 as the next action, and the work is green.

## 5. Phase Disposition — MILESTONE 3 STEP 4 IS NEXT (next agent: start here)

Step 3 is **done**. Remaining M3 steps (per the ground-truth brief
`docs/research/fs-mount-sft-ground-truth.md` §4 / §6):

1. ~~Ground-truth brief~~ — done.
2. ~~FAT12 mount over ATA + proto-DIR~~ — done (WL-0007).
3. ~~SFT/JFT + handles 0–4 + DUP/DUP2~~ — **done (this Record).**
4. **NEXT — file-handle INT 21h functions (`initech-509.5` read-side).** `3Dh`
   OPEN / `3Fh` READ / `3Eh` CLOSE / `42h` LSEEK over the mounted FAT12 via the
   SFT, then `4Eh`/`4Fh` FINDFIRST/FINDNEXT into the DTA (lock the 43-byte
   find-data layout as `spec/find_data.h`). The SFT FILE fields
   (`dir_entry`/`file_offset`/`file_data`) are **already in place** awaiting OPEN
   to populate them. Brief §4 recommends **whole-file-read-into-a-static-buffer
   at OPEN** (Risk 2: do NOT put the file buffer on the stack; reserve it in
   `spec/memory_map.h`). A baked `TYPE.COM` proves a program OPENs/READs a real
   file and WRITEs it to handle 1.
5. **THEN — verify + reconcile beads + worklog** (advance/close `509.5`,
   `initech-saw` FAT-sourced load; WL-0009).

After M3: the shell (`initech-7pc`) gated on CON input / the keyboard
(`initech-n62`, `initech-3rs`); FAT-sourced program load (`initech-saw`); then
the `f8v.4` keystone (boot → banner → COMMAND.COM → DIR → TYPE).

## 6. Follow-On Items / Open Beads of Note

- `initech-509.5` (file-handle + CON-input functions — Step 4, in progress).
- `initech-6hk` (**new** — SFT teardown / re-init on process EXIT; multi-process).
- `initech-saw` (FAT-sourced `.COM` load — after Step 4).
- `initech-509.2` (IO.SYS/INITDOS.SYS partition + SYSINIT — relocate `sft_init`
  + the kernel-PSP bind there when it lands).
- `initech-n62`/`initech-3rs` (CON input 01h/06h/0Ah → keyboard).
- `initech-xk2` (INT 21h reentrancy when IRQ0/IRQ1 unmasked — the SFT/find
  globals make this live once IRQs are unmasked; still masked this milestone).
- `initech-dao` (`fat12_read_file` on-stack chain buffer — relevant to Step 4's
  OPEN/READ buffering).

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
