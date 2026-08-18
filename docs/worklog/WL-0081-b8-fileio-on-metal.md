<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0081 — B8: The Seed Speaks File I/O, On Metal

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Worklog Shard (session record)
**Session:** 2026-08-18 (continuation of the WL-0080 sitting; codex exec
gpt-5.6-sol xhigh lane for the compiler surface, orchestrator first-person for
the emulator-coupled runtime target + gates)

## Context

With the Wave B menu arc and `chd4` closed (WL-0080), the North-Star B8 lane
(`initech-ogxv`, epic rnh/M7) was next: thin file-I/O RTL over InitechDOS
INT-21h handles, per docs/plans/TPS-M7-subset-plan.md and the B7 committee's
ratification amendment (named FILEIO-class mutants REQUIRED).

## What changed (commit `f7f1dc7`)

**The codex lane** delivered the compiler surface: `file` as a storage-only
type through parser+typecheck (a 260-line thin-surface validation — arrays,
record fields, params, results, Text/typed machinery all rejected loudly);
fixed 260-byte static file objects (dword JFT handle + 256-byte ASCIIZ name,
no heap); the hand-assembled `seed/rt/fileio.asm` RTL (AH=3Dh/3Ch/3Fh/40h/3Eh
per the DEC-04a flat ABI; any CF error prints `FILEIO-ERROR AX=` and exits
AH=4Ch AL=1); the `fileio.pas` exact-output fixture (TARGET-opened-before-SINK
so the wrong-handle mutant is observable as wrong values, not a crash);
`file_shared.pas` into SEED_FPC_CORPUS (the WL-0077 promise); host gates
test-seed-fileio-build / -fpc; 18/18 pre-B8 fileless `.s` byte-identical and
`start.asm` untouched (FROZEN per DEC-05).

**The orchestrator-owned finding and its resolution:** the seed's existing emu
legs boot bare-metal Multiboot ELFs — no INT-21h dispatcher exists there, so
B8's oracle could not run on that harness at all. Resolution (first-person,
the samir-boot precedent end-to-end): a SECOND seed runtime target —
`seed/rt/start_dos.asm` + `seed_dos.ld` (flat .COM at PROGRAM_IMAGE 0x40100,
entry-first, NOLOAD-.bss zeroed by the stub, INT 21h AH=4Ch exit; serial
helpers duplicated verbatim from the frozen start.asm, grep marker
SEED-RT-SERIAL-HELPERS) — linking the SAME compiled program objects the host
gate proved. FILEIO.COM goes onto a FAT12 data disk; the COMMAND.COM shell
kernel EXECs it on the booted 386.

**Gates (into TEST_EMU_GATES):** `test-seed-fileio-os` asserts the
hand-computed exact serial line `FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR` —
GREEN ON THE FIRST BOOT. `test-seed-fileio-os-mutant` boots both mutant
compilers' .COMs: SHORT_WRITE prints `RB=6 DATA=ORTHSTA??` (drops tail bytes
AND lies about actuals — the 1-byte write vanishes, the 8-byte block writes
7), WRONG_HANDLE prints `R1=0 RB=0 DATA=?????????` (every write went to the
SINK file) — both wrong-values-no-crash, Rule 6 proven ON METAL.

## Trap caught (Rule 4, my own gate)

The first mutant-gate grep for "the program ran" matched `FILEIO.COM   1298` —
the FAT DIRECTORY LISTING line, not program output; the wrong-values proof
was accidentally vacuous. Tightened to `^FILEIO W1=` with an in-gate comment
warning the next author (the shell's DIR echo shadows bare-substring greps
for any marker sharing a filename prefix).

## Acceptance

`make clean && make test` **ALL GREEN — 326 host + 87 emu** (the four B8
gates joined the vectors). Repro: 15 fixtures / 45 hashes byte-identical.

## Coordination record

The codex cross-session kill-trap protocol (WL-0080 postscript) ran through
this lane: windows serialized with the rk-campaign-D session via SendMessage —
final tally 4/4 clean lanes inside serialized windows vs 3 kills outside them
(their bead rk-qe0j carries the joint forensics; ours is bd memory
`codex-exec-cross-session-kill-trap-2026-08`).

## Pointers

- Bead closed: `ogxv`. **B9 (`6m52`) is now unblocked** — author Turbo
  Initech in os/tps/ in the seed subset; resolve `lh83` (symbol-table naming
  idiom) first; closing B9 closes M7.
- The two-runtime-target seed convention (bare-metal Multiboot vs InitechDOS
  .COM) is new load-bearing structure; B9's resident compiler work happens ON
  InitechDOS, so start_dos is its runtime from day one.
