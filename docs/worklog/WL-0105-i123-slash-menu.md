# WL-0105 -- Initech 123 P3 slash menus and range transfers (initech-uqah)

## Context

Coding lane from BASE_COMMIT f7ed504, branch lane-uqah. The operator forbids
bd, pushes, Bochs, recording and aggregate test vectors in this lane. This is
an incomplete bead handoff: test-i123-copy-move remains RED because the
provided corpus has no command-minted Copy/Move evidence, and its replay
leg still needs implementation after that evidence is acquired.

Read CLAUDE.md, PRD 6.5/6.6, plan sections 4/5/7, core/codec/Face A, locked
WK1 headers, existing host and emulator gates, independent readers and
WL-0094/0095. No locked WK1 header or kernel source changed.

## What changed

- 82 menu levels, 381 entries, real names/descriptions captured from 123.RI.
  The independent factory C reader extracts the cited RI offsets and diffs
  both the compiled table and loaded menu resource against the text golden.
  Empty descriptions use an explicit quoted-empty text field; their runtime
  bytes remain empty. Arrow/first-letter navigation, Esc stack and MENU work.
- Face A Range Format/Label/Erase/Name/Prot/Unprot/Value/Trans; Worksheet
  global format/label/width/recalculation/protection, column width and erase;
  F2/F5. Name Create/Delete/Labels/Reset/Table persist NNAME records, formulas
  resolve relative NAME and absolute $NAME. Erase retains format/protection.
- Source snapshots into a spare worksheet bank implement Copy/Move and
  Value/Trans. Relative/absolute/mixed axes adjust; Move retargets referrers
  throughout the sheet and named-range corners. Overwritten defining
  references become ERR. A failure leaves the source bank intact.
- Sci and rounded +/- formats, formula Text, Hidden labels/protected panel
  contents, numeric error values in Value/Trans. Manual format examples are
  host checked. Existing formula engine and quit guard remain in use.
- New spec/i123/era_shortfalls.md: I123-P3-01..09 identify every deferred
  surface, including name rebinding with existing formulas (visibly refused),
  copied TO tiling, international/time formats, Justify/Input/Search, P4
  commands and the narrower viewport. This lane does not claim full parity.
- APPS/123.MNU is an authored, validated disk resource. RI strings and menu
  metadata live there to fit the unchanged 65536-byte executable cap.
  Missing/corrupt resources refuse startup. The resource follows 123.EXE in
  directory order, preserving the launch trace's icon position; APPS now has
  a fourth visible file. All variant data-volume recipes package it.
- I123-only -Oz/LTO, function/data section collection and 4-byte cdecl stack
  alignment; other tenants/kernel keep their compile policy. The image is
  linked at both relocation probe bases and mzlink still enforces the cap.
- New locked menu traces and RECORD_SCRIPTS entry i123_menu. Its marker is
  the final Range format description, after Copy/Move; an early MENU marker
  would have ended the clip prematurely. Recording was not run.
- Quitguard fixture renamed to flair_i123_quit_fixture.img. The old name was
  also the smoke gate's QUIT output and was overwritten during the combined
  emulator run, removing RO.WK1 and falsely allowing Save/Exit.

## Why

Law 1: installed 123.RI is the text authority, not the plan's shorthand.
Actual names include Sci, comma (,), Prot, Unprot and Trans. Print has
Printer/File parents, then their command menus. Copy reference rules come
from Reference pp. 3-1..5 (PDF 69..73); Move and defining-corner invalidation
from pp. 7-1..2 (PDF 191/192). Name layout is re-derived from MINT02.WK1:
NNAME header 0x86/body 0x8a, TOTAL=A5..A5. Name behavior is Reference
pp. 10-14..18 (PDF 232..236); Erase PDF 221; format rules PDF 224/225;
Value/Trans PDF 240..242. RI offsets for every item/description are in
spec/i123/menu_ri_offsets.tsv (root 0x2..0x336, Range 0x279e..0x2eef,
File/Print 0x2eef..0x3d6b, remaining included commands through 0x586f).

Law 2: randomized coordinate expectations are separate from the artifact's
reference-word helpers. The compiler's decompiled text is compared against
that model. The existing independent wk1_ref reader supplies FORMULA record
boundaries; wk1_formula_ref.py --bytes exposes raw bodies, including caches
and RPN, for the six existing mint sheets' 12 self-transfer baselines.
Those baselines are explicitly NOT command-minted Copy/Move goldens.

## Frictions

- The reference corpus contains zero type /c or type /m mint commands.
  The mandatory acceptance target fails loudly after model/baseline success.
  It must not be marked green. The remaining command-golden acquisition and
  differential replay are PM follow-up; bd is unavailable by instruction.
- Initial monolithic menu builds exceeded 64 KiB. No cap was raised: menus
  became a resource, I123 gained a local size policy. Final 123.EXE is
  65340 bytes (196 bytes below cap), MNU 17746 bytes. Future feature growth
  needs more measured size work, not a silent cap change.
- QMP does not accept kf5; the new trace uses five Right arrows to H1.
  No unknown trace token remains. Long menu text is clipped in the 77-column
  viewport until its item is highlighted, when the view scrolls.
- Red emulator runs were repeated alone before diagnosis. The quit fixture
  collision was fixed; a later parallel formula-launch red passed alone.
  Final remaining gates were run sequentially, with INITECH_QMP_STDIO=1.
- Work uses copied build/lane.git metadata, never writes .git or another
  project. BASE_COMMIT.txt is left untracked. No push or tracker operation.

## Acceptance

New gates and final evidence:

| Make target | Final evidence |
|---|---|
| test-i123-menu-tree | >>> test-i123-menu-tree: green (82 levels, 381 items; independent 123.RI bytes and child links) |
| test-i123-menu-tree-mutant | >>> test-i123-menu-tree-mutant: green (Worksheet misspelled Vorksheet correctly RED) |
| test-i123-copy-move-model | i123-copy-move: 996023 checks, 0 failures (4000 random sheets); >>> test-i123-copy-move-model: green (4000 random sheets, manual reference model) |
| test-i123-copy-move-baseline | >>> test-i123-copy-move-baseline: green (12 real-sheet self-transfers, independent FORMULA bodies) |
| test-i123-copy-move-mutant | I123_MUT_COPY_ABSOLUTE correctly RED (absolute-relative); I123_MUT_MOVE_STALE correctly RED (move-referrers); >>> test-i123-copy-move-mutant: green (absolute adjustment and stale referrers correctly RED) |
| test-i123-copy-move | RED, exit 2: !!! test-i123-copy-move: corpus evidence unavailable: 0 /Copy and 0 /Move mint commands; real command goldens required |
| test-i123-range-menu | i123-range-menu: 23 checks, 0 failures; >>> test-i123-range-menu: green (Copy, Move, names, formats, protection, Value, Trans, Esc/Break) |
| test-flair-i123-menu | >>> test-flair-i123-menu: green (tree, RI descriptions, Copy/Move row and MENU pixel bands) |
| test-flair-i123-menu-mutant | >>> test-flair-i123-menu-mutant: green (Move left H1 stale: 0 instead of 80, correctly RED) |

The menu resource corruption case also refuses with `menu resource refused`.
Pixel evidence: `ppm_text_check: i123-menu bands=1515/1479/433
MENU=330 navy/182 white: green`. The final row is 10, -20, 40, blank, blank,
-80, 160, 80. The mutant's H1 is 0. The end-to-end trace visits every top
branch and representative nested menus; the host text oracle covers all
82 included levels, not every emulator leaf execution.

All new gates are in TEST_UNIT_GATES/TEST_EMU_GATES. Existing host family
run: test-i123-wk1-roundtrip (+mutant), test-i123-formula (+mutant and both
x87 control-word legs), test-i123-recalc-order (+mutant), test-i123-quit
(+mutant); green. Counts: WK1 405, formula 121 grouped checks, recalc 187,
quit 60. test-spec passed all six legs; test-makefile-vars: green.

Existing QEMU targets run: test-flair-i123: green;
test-flair-i123-formula: green (final alone rerun);
test-flair-i123-files: green; test-flair-i123-quitguard: green (all nine legs).
Only authorized targets were run, at most make -j2; no aggregates.
Bochs, record-flair and other emulator mutants: NOT RUN by lane instruction.

Rebuilt 123.EXE compares byte-identical; SHA256
adc07ccfc77a03d3dd0b315017f9bd85382d69310fc2a8d3a4389eb83d6c40c8.
MNU SHA256 4a747ea411f686d6d06a04a2238ab296315ccb8bee4ce3ec23b82ad35d122e59
stayed identical when its host source was made readable. Data volume builds;
file gates independently read saved worksheets with wk1_ref.py and fsck.fat.
Source ASCII and git diff --check pass.

## Pointers

- spec/i123/era_shortfalls.md and menu_ri_offsets.tsv: scope and full text
  citations. No WK1 locked spec changes.
- os/i123/core/range.c, menu.c/menu_data.h; app/face_a.c.
- harness/diff/wk1_diff/test_i123_copy_move.c,
  test_i123_transfer_baseline.c, test_i123_menu_tree.c;
  harness/proptest/test_i123_range_menu.c.
- spec/flair_i123_menu_traces.mk; Makefile RECORD_SPEC_i123_menu. PM records
  the clip and runs Bochs on main, then obtains Copy/Move mint evidence and
  completes replay. The required gate deliberately keeps the bead open.
- Local evidence: build/i123-all-host-final.log,
  i123-final-targeted-host.log, i123-final-property.log, i123-menu-final.log,
  i123-required-copy-move.log, i123-remaining-emu-final.log,
  i123-formula-final-alone.log, i123-repro.log. These logs are not in git.
- Transport: build/lane.bundle, f7ed504..lane-uqah; verified with git bundle
  verify. Original .git remains untouched; no push.
