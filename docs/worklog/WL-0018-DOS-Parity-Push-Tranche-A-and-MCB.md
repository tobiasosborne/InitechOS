# WL-0018 -- DOS 3.3 feature-parity push: gap map, Tranche A, MCB arena

Epic: **initech-bsy** ("DOS 3.3 feature-parity push (MILTON): close the
Appendix-A INT 21h + shell gap"). Branch: `command-com-default`. This shard
covers the kickoff (a grounded gap analysis + sequenced plan) and the first
landings: all three Tranche-A items plus the MCB arena allocator.

## Context

Operator directive: "orchestrate work on getting InitechDOS features landed.
Create rock solid, correct, robust, and tested features ... keep working until
the DOS has feature parity with DOS 3.3-5.0 era (spirit, not literal, where it
conflicts with the north star)." And: "review the relevant beads, and orient
work that way."

## Gap analysis + plan (oriented around the beads)

A multi-agent gap-map workflow mapped the kernel's real state vs the DOS
parity target, reviewed the ~20 open MILTON-layer beads, and produced a
sequenced build order -- all reconciled against ADR-0003 Appendix A as the
GOVERNING INT 21h scope.

**Critical ground-truth correction (verified at the source, Law 4):** the INT
21h dispatch switch (int21.c:1862) implements 39 functions, but
`39h/3Ah/3Bh` (MKDIR/RMDIR/CHDIR), `43h` (CHMOD), `44h` (IOCTL),
`48h/49h/4Ah` (ALLOC/FREE/SETBLOCK), `56h` (RENAME), `57h` (FILETIME), `5Bh`
(CREATNEW), `31h` (KEEP), and `0Fh-24h` (FCB) are RECOGNIZED by `ah_is_listed()`
but NOT dispatched -- they fall through to `not-yet-impl` CF=1/AX=1. The real
Appendix-A parity gap is substantial. (The initial case-label grep was misled by
the recognition table; the dispatch switch is the truth.)

Created epic **initech-bsy** with the full sequenced build order (Tranches A-I,
conflict-group serialized) in its design field, and wired the deps the analysis
flagged (`4iq->509.6`, `4iq->ti8`, `atf->ti8`). Scope is strictly MILTON;
GUI/Toolbox excluded; MZ .EXE (`dtw`), FCB (`509.9`), and the INT 2Fh net
redirector deferred per ADR + north-star (FCB flagged for an operator decision
before closing).

## What landed (each: Law 1 grounding -> root fix -> RED->GREEN -> mutation-proven -> full gate -> commit)

**Tranche A (INT 21h correctness foundation):**

- **dww** (`650b23d`, closed) -- FINDFIRST/FINDNEXT DTA now uses the REAL-DOS
  byte offsets (attr@0x15, time@0x16, date@0x18, size@0x1A, name@0x1E) instead
  of a simplified layout (attr@0x0C..name@0x15) inherited from a wrong table in
  `fs-mount-sft-ground-truth.md` Sec 4.5. Root cause was a Law-1 source conflict;
  corrected the spec, the writer, BOTH baked asm DTA readers (dir_program +
  irqstorm -- the analysis undercounted, caught by re-verifying), and the brief.
  Added a compile-time `offsetof` oracle in spec/find_data.h + a runtime byte
  golden; both mutation-proven.

- **62m** (`b5750c7`, closed) -- the PS/2 keyboard now decodes Enter to CR
  (0x0D) in both scancode tables; AH=0Ah terminates on CR only and the LF->CR
  normalization bandaid is retired (Rule 3). The former LF test is repurposed
  into a regression test that LOCKS the retired behaviour. Both mutations bite;
  the test-conin emu gate proves it end-to-end (a QMP-injected Enter).

- **456** (`9eed3de`, closed) -- AH=4Bh EXEC carries the command tail to the
  child PSP:80h. Added an authentic flat EXEC parameter block in EBX
  (`exec_param_block_t` in spec/dos_structs.h, forward-compatible with the env
  block + FCB API); extended the int21_exec_fn seam to match
  load_program_from_fat; `cmd_parse` captures the VERBATIM DOS tail (vs the
  trimmed `arg`). Coverage is link-by-link, each mutation-proven (cmd_parse /
  do_exec EBX extraction / loader threading / psp_build PSP:80h write).

**Tranche B (start) -- the MCB memory arena (initech-509.6, IN PROGRESS):**

- The PURE arena allocator landed: `os/milton/mcb.{c,h}` + the property suite
  `test_mcb.c` + the `test-mcb` / `test-mcb-mutant` gates. It is the heap behind
  AH=48h/49h/4Ah (DEC-03 / Appendix B.3) -- a walkable 'M'/'Z' MCB chain, owner-
  tagged, with first-fit alloc + split, coalesce-on-free, setblock grow/shrink,
  the insufficient+largest-free failure report, and fail-loud bad-block/owner
  guards. Addressed in arena-relative PARAGRAPHS so it is decoupled from the DOS
  segment convention and fully host-testable. Oracle: chain invariants + a 20k-op
  randomized fuzz with DATA-INTEGRITY checking (no allocation is ever stomped) +
  free-all-coalesces-to-one-block. Two mutants (no-coalesce, no-owner-check) bite.

## Why this shape

Tranche A items share int21.c/kbd.c/command.c, so they were landed SERIALLY on
the main thread, oracle-gated one at a time (the WL-0017 discipline). The
analysis/bead-review FANNED OUT (read-only). The MCB allocator is a NEW isolated
file, so it landed without touching the kernel dispatch yet.

## Acceptance

`make test` = **57 host + 21 emu gates GREEN** (was 55+21; +`test-mcb` and
+`test-mcb-mutant`). Each new gate mutation-proven. New struct
`exec_param_block_t` in `spec/dos_structs.h`; new subsystem `os/milton/mcb.{c,h}`.

## Frictions

- The analysis agents undercounted the FINDFIRST DTA readers (claimed 2, found 3
  -- dir_program.asm AND irqstorm_program.asm both hardcode the name offset).
  Re-verifying at the source before editing the locked spec caught it (Law 4).
- `make test-exec` / `test-conin` are EMU gates; the host units are
  `test-exec-unit` / `test-conin-unit`. Mutation-proving must target the host
  unit, not the emu gate (which can mask a dead branch).

## Pointers / remaining (initech-bsy sequence)

- **initech-509.6 (IN PROGRESS, next):** wire AH=48h/49h/4Ah in int21.c to an
  `mcb_arena_t` over a real program-memory region, and DECIDE the arena region.
  The program window 0x30000..0x70000 is fully tiled (PSP/image/env/stack), so
  the heap needs either (A) a carved heap window -- reduce PROGRAM_IMAGE_MAX in
  spec/memory_map.h (a deliberate load-bearing change + worklog note), or (B) the
  authentic "program shrinks its block via 4Ah, then allocs from the freed tail"
  model. Segment mapping: AX/ES segment = (arena_base_linear >> 4) + data_para;
  owner = the current PSP. Add a host dispatch test + an emu gate
  (alloc/free/setblock from a baked .COM).
- Then per the epic design: `dao` (streaming cluster walk) -> `z01` (FAT16) ->
  `ti8` (subdirs + real GETCWD) -> MKDIR/RMDIR/CHDIR -> env store + SET ->
  wildcards + COPY/DEL/DATE/TIME -> batch + PROMPT + PATH + AUTOEXEC -> RENAME +
  IOCTL + CON-read + CHMOD/FILETIME/CREATNEW -> ATA hardening + INT 24h + devices
  + msg-catalogue. See `bd show initech-bsy` for the full ordered list.
