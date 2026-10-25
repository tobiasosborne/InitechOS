# WL-0186: land the parked wave-1 SAY slice after overlays

## Context

Operator task, 2026-10-25: port parked commit 5286f991 from lane/wave1-say
onto main c0ddb049 on lane/wave1-say2. The old flat SAMIR exceeded its size
gate; Phase B now provides SAMIR.EXE plus SAMIR.OVL. Do not push or run bd.
Ground truth: PRD Sec 6.6; dbase3-decomp specs/full-screen/say-get-read.md
Secs 1-2 and specs/commands/set-commands.md Sec 3.4; the committed real
III+ fns-mint/{row,col,prow,pcol} captures. Port-and-verify, no golden edits.

## What changed and why

Preserve the parked @ row,column SAY implementation, independent screen and
printer cursors, SET DEVICE routing, error codes and unsupported-clause
rejections. Keep SAY, all four cursor functions, cursor advancement, state,
screen synchronization and the PAL implementations resident: these are hot
interpreter/output paths. Only set_opt_device is new SET-overlay code,
registered by one appended set_options.def row. It calls no interpreter
executor or overlay manager and retains no pointers.

The new say_screen and printout slots remain at the end of samir_pal.
pal_null binds both to -PAL_EIO; its deliberate slot count is 30, up from
28. Preserve the parked deterministic host screen/printer model and Milton
CON / AH=05h bindings. Extend the resident PAL lifetime audit to both new
slots; ESCAPE_SAY and ESCAPE_PRINT prove that SET-overlay callbacks cannot
escape through samir_bind_pal. The null contract also asserts both errors.

Accept the four function cases through PENDING-to-ACCEPTED marker renames.
Move the parked reference/quick-reference entries into individual F4 files;
keep generated chapter indexes and the existing SET ALTERNATE clarification.

## Frictions and conflict resolution

- Makefile: retain main's marker-derived case lists; preserve the SAY test
  rule and register it in TEST_UNIT_GATES.
- cmd/set.c: retain the table-driven overlay dispatcher; translate old
  positional DEVICE case 17 into set_opt_device plus its definition row.
- Quick Reference qr-commands.tex, qr-functions.tex and qr-set.tex: retain
  generated inputs; put the six additions in entries/{cmd,fn,set} instead.
- Volume One u5-entries-a.tex, u5-entries-c.tex and u6-entries.tex: preserve
  main's deletions; transplant @, DEVICE, COL, PCOL, PROW and ROW entries
  with their source comments, and edit entries/cmd/set-alternate.tex.
- The first focused gate run failed in test-samir-def-deps because its
  git-ls-files archive still named unstaged deleted paths. Staging the
  resolved marker/manual renames fixed it; the identical gate command then
  passed. No oracle was weakened. The full -j64 merge run needed no retry.

## Acceptance

Source tools/lane-env.sh before all make runs, with DBASE3_DECOMP,
LOTUS123_DECOMP, SYSTEM7_DECOMP and WIN31_DECOMP pointing to the corresponding
directories under /home/tobiasosborne/Projects.

- make -j64 test-merge-samir: exit 0, all 321 gates green, including the four
  cursor functions on host/emulator and their mutants. The emulator harness
  requires a completed nine-key shell trace before comparing transcripts.
- test-samir-say: 61 checks, zero failures; null PAL contract green.
- test-samir-ovl-static: byte-identical repack, linker/static call graph green.
- test-samir-ovl-guard-emu-mutant: both new callback escapes (and all existing
  escapes) diagnosed with completed kernel-panic traces, 14 checks each.
- test-pubs-lockstep-src: 148 shipped / 148 documented names in both books;
  292 typed-line checks and one transcript pair. Existing RELATION/TALK
  DRAFT notices remain; the source gate passes.
- test-flair-launchkey and test-flair-launchkey-mutant: exit 0.
- test-samir-size: resident 84,528 bytes against 93,776 bytes; headroom 9,248
  bytes, 5,152 above the unchanged 4 KiB floor. SAMIR.EXE file 87,968 bytes;
  SAMIR.OVL 26,624 bytes; largest overlay FILE 9,724 bytes in a 9,728-byte
  slot; SET overlay 3,304 bytes. Resident growth from main: 2,496 bytes.

## Pointers

docs/design/samir-overlays.md, docs/plans/INITECHBASE-lane-howto.md;
os/samir/cmd/query.c q_say, cmd/set.c set_opt_device, core/ovl.c PAL audit;
harness/diff/dbf_diff/test_samir_say.c and samir_ovl.mk.
