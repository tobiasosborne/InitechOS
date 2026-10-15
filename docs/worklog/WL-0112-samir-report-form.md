# WL-0112 -- SAMIR REPORT FORM, graded against real dBASE output

## Context

Bead initech-airx (B78, audit L056): `REPORT FORM` fell to #16 because no
module owned the verb. First sitting, base e7a8e62. Read CLAUDE.md, PRD 6.6,
ADR-0008/0009, the SAMIR plan, WL-0097/0101/0109, and the corpus frm.md,
report-form-generator.md and harness notes. Manual: Using dBase III Plus
U5-177/U5-178 (REPORT), U7-8 (group expression types), U7-16 (#126),
U7-17 (Report file invalid).

## Mint first (Law 1/2)

Corpus session 004 (dbase3-decomp fd28b97, re/mint-results-004.md): real
DBASE.EXE 1.1 under dosbox-x wrote REPORT FORM ... TO FILE output for the
shipped FLT_INFO / TRAVELER / TRIP forms (TRIP over a table real dBASE
created), with PLAIN / SUMMARY / HEADING / NOEJECT / NEXT / FOR / WHILE, 83
lines-per-page sweeps (footer word 0x7B8 patched), wrap / ';' / totals /
century probes, and three screenshots. 111 outputs and their inputs are
md5-pinned in harness/diff/dbf_diff/report_mint004.md5. tools/frm_ref.py is
the independent .frm reader (from frm.md only). Two traps found while
minting: dosbox-x ignores `DATE 01-01-1986` (needs MM/DD/YYYY; the corpus
harness doc said otherwise), and with SET ALTERNATE ON a no-eject report
continues the transcript's line count, so the goldens are re-minted without
ALTERNATE (E1..E4 prove the fresh start).

## What changed

- os/samir/cmd/report.c (new): the renderer, registered by samir_main.c;
  flow.c needed no change (unknown verbs already reach the hook chain).
  Reads the .frm through the PAL read-only (no SAFETY involvement), validates
  it (size, version/end words, table sums, NUL-terminated heap strings,
  column indices), renders header / heading band / wrapped cells / group and
  subgroup bands / subtotal bands / grand total, pages with the fitted model,
  writes the TO FILE sink (CR LF lines, CR FF ejects, closing ^Z) and the
  console (same stream minus FF). Refuses loud: TO PRINT #126, `?` and
  unknown clauses #36, untested .frm flags (double space, summary, eject,
  option bits 1/2) #50, memo #34, N/L/M group expressions and non-numeric
  totals #9, no table #52, missing form #1. query.c exports q_eval and
  q_render_val for it.
- Memory (capacity-neutral): SAMIR.COM grew 87,776 -> 93,520 bytes, past the
  2 KiB image headroom. The PAL keeps 16 KiB scratch + 20 KiB owned heap, but
  6 KiB of the owned heap now lives in the unused tail of the program's own
  64 KiB BSS reserve (pal_heap_add; coalescing never joins regions), so the
  AH=48h request drops 36 -> 30 KiB. samir.ld asserts both fits: image
  margin 2,448 bytes, BSS-tail margin 2,672 bytes. The error catalog is now
  one packed string (saves 581 bytes; text and ordinals unchanged).
- Gates: test-samir-report (+ -mutant) in TEST_UNIT_GATES;
  test-samir-report-emu (+ -mutant) in TEST_EMU_GATES; data volume
  build/samir_report_data.img with a fixture contract and the verifier.

## Acceptance (final lines)

- test-samir-report: `test-samir-report: 1056 checks, 0 failures` /
  `green (111 REPORT FORM files byte-identical to real dBASE III PLUS 1.1)`
- test-samir-report-mutant: subtotal RED (G8TP.TXT), column RED (G1FLT.TXT),
  page RED (G2TRV.TXT); green.
- test-samir-report-emu: files 48 checks / screen 154 checks, 0 failures; the
  guest wrote G1FLT/G2TRV/G8TP/G9TS byte-identical to real dBASE; the final
  screendump matches 1920 cells of the real G2TRV console projection (the
  R4S3.PNG command; FF invisible, Page No. 2 under Pope).
- test-samir-report-emu-mutant: dropped subtotal band RED in the guest
  (G8TP/G9TS differ, G1FLT identical, triple_fault=0).
- Regressions green: test-samir, -view(+mutant), -use(+mutant),
  -input(+mutant), -repl(+mutant), -query, -lifetime(+mutant),
  -append-safe(+mutant), -safety(+mutant), -status(+mutant),
  -numeric-format, all test-interp-* and mutants, test-dbase-diff,
  test-dbase-roundtrip, test-makefile-vars, test-fixture-volumes(+mutant);
  QEMU one at a time: test-samir-boot, -write, -audit-emu, -input-emu,
  -lifetime-emu, -full-emu, -safety-emu, -canon-y2k, -status-emu,
  -console-emu, test-flair-samir-suspend. No full make test; no Bochs.

## Found, not fixed (outside this bead)

- USE ... INDEX does not default the .NDX extension (real dBASE does); the
  gates name TRIPS.NDX explicitly.
- USE with an index leaves physical record 1 current; real dBASE starts at
  the first index-order record. The gates GO TOP before NEXT-scoped reports.
- LIST NEXT n leaves the pointer on record n+1 in SAMIR; real REPORT FORM
  NEXT n leaves it on record n (R4ALOG) -- LIST is likely the same [inferred].
- set.c state (e.g. SET CENTURY) survives xb_interp_free when the next
  interpreter reuses the address (host test hygiene).
- re/harness-setup.md DATE form (fixed in the corpus commit).
