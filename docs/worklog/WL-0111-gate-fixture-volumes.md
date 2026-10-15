# WL-0111: Verify gate data volumes before trusting them

Context: initech-tflp; CLAUDE.md Rule 2 and Rule 6; PRD Sec 8/14.
The old appl-boot QUIT scratch name could overwrite the quit-guard fixture.
The uqah rename is already on this lane's base. Image names, locked traces
and existing gate expectations stay unchanged.

Changed:

- Added test-fixture-volumes and QUIT_DATA_COLLISION mutation proof. A C
  scanner expands GNU make's database recipes without executing them. It
  inventories 210 gate/scratch-name pairs (205 paths, 105 gates), 95 fixture
  volume names (85 seeded contracts plus 10 blank/directory-only volumes),
  and 57 inline image writers. Pattern rules and shell name families are
  compared conservatively; lexical path aliases are normalized.
- Seeded image builders must declare a contract and end with verification.
  Inline minters must also verify the files they copy. Contracts cover all
  49 seeded image rules and 36 named inline volumes. The shared FLAIR
  template includes its variants; inherited files are verified too.
- mdir checks exact paths and mattrib checks the complete flag column.
  Source/copy checks and direct boot preflights run outside tolerant emulator
  commands, so stale fixtures are harness errors before any guest boots.

Acceptance:

- QUIT_DATA_COLLISION: RED on the historical flair_i123_quit_data.img alias;
  restored real gate GREEN. A ./build alias and omission of the final
  recipe verifier were also caught.
- A temporary mattrib wrapper used mdel after +r: the actual image recipe
  failed with "missing fixture ::RO.WK1" at its final verification step.
- Manual mattrib -r + mdel: the standalone recipe verifier and quit-guard
  preflight both failed with "missing fixture ::RO.WK1". The red emulator
  gate was repeated alone. Clearing only +r failed with attributes A vs AR.
  All temporary mutants were removed/restored and the fixture rebuilt.
- Final GREEN: test-makefile-vars, test-fixture-volumes and its mutant;
  test-fat, test-fat16, test-fat-subdir, test-gnrc, test-fat-move;
  test-flair-i123 and test-flair-i123-quitguard (INITECH_QMP_STDIO=1).
- Broader targeted image builds checked nested files, loops, hidden DB,
  read-only recording fixtures, damaged/purge/scroll data and file tenants.

No full make test or Bochs run: the lane restriction reserves host capacity
for the certificate and Bochs is unavailable. The PM owns beads and push.
The full recipe table, exact failure lines and final gate lines are in
build/lane-report.md; static inventories are build/fixture-volumes.*.

Outside scope: existing rename-golden comments still mention NEW.TXT while
the recipes emit NEW.BAK; existing linker RWX LOAD warnings are unchanged.
