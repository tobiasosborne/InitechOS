# WL-0180 -- New-box certificate, size round 8, and the parallel certificate runner

Session of 2026-10-24 on the new WSL2 box. HEAD at close: 61294e7e.

## Context

The programme moved off the old laptop onto a new machine: Ubuntu 24.04 on
WSL2, 64 cores, 62 GB. The full toolchain was installed per docs/BOOTSTRAP.md
(WL-0178 wrote it), the six decomp siblings and the manuals library were
restored from the private release assets, and tools/bootstrap/doctor.sh came
up green except 86Box. WL-0178 left the certificate un-run on its HEAD, so
the first job on the new box was to certify main, and the operator then asked
for a test workflow that uses the 64 cores.

Operator rulings of 2026-10-24 (recorded as bd memories): work mostly
serially; Claude/Fable lanes may run up to 4 points ahead of pace, Codex up
to 10 (banked reset); exploit Codex gpt-6.1-sol for coding and computer use.

## What changed

Box-isms (environment, the only two):
- vgabios was not a required package; a fresh 24.04 has a dangling
  VGABIOS-lgpl-latest symlink that broke every Bochs leg. c19a350f makes it a
  required package and doctor checks the three ROM paths the harness opens
  (initech-3kiy).
- The old laptop's /home/tobias paths were baked into executable files:
  2808f8b5 (test-samir-lifetime now takes the dbase3-decomp root from the
  Makefile, initech-3e7q; the L003 CNAMES.NDX corpus copy segfaulted and
  killed the first certificate 18 s in) and 326eff3f (drive_flair.py,
  batch_break mint.sh, samirctl-mint README; initech-pla5).
- dbase3-decomp mint/work was untracked and missed the first snapshot; the
  laptop agent re-snapshotted it and 842044ca adds snapshot-sibling.sh
  (refuses on untracked files) and doctor checks for every sibling file the
  gates read (initech-e8s3).

The per-gate serial sweep (the new-box certificate):
- A serial sweep, `make <gate>` for each of the 1478 gates with PASS/FAIL per
  gate, at 2808f8b5 took about 5 h and found 30 reds: 15 mint/work absent
  (initech-e8s3), 7 loader bug, 6 kernel size r3/r4, plus five single-gate
  reds below.
- initech-zxzi (P0): since 7850d4d2 a NULL image is LOADER_ERR_NULL_IMAGE in
  both loader modes, but loader_prepare_mz still passed NULL, so every valid
  InitechMZ .EXE was refused (test-mzload 16/26, test-mzexec,
  test-kernel-native-boundaries). Fixed in 5628a872 (pass the resident load
  module in in-place mode).
- initech-u01i: the SAMIR shipped-surface candidate feed shrank to 76 words
  after 0d5e6bbf moved the verb/keyword/function/SET tables into packed
  strings (ACCEPT, APPEND, DELETE, LIST missing; test-pubs-tps
  DOC-NOT-SHIPPED). c7e806ad follows the names to the grammar tables; the
  shipped surface goes 32 -> 130 (merge 1e59e6c3); gs-dir re-captured in
  40b1f444 (SAMIR.COM 94 -> 96 clusters).
- initech-kqr8: test-pubs-screens TPS screens drifted 13 clusters; f205fa7f
  re-captured tpsbak/tpsdir and accounts for all 13 (WORD.EXE +9, CALC.EXE +4,
  TEXT.EXE +1, 123.EXE -1 since f9da0ab3; CALC.EXE was the missing four).
  Merge 93a6c4ef.
- initech-buze: test-flair-desktop-icons-bochs still graded the dead
  ENOMODE / mode-13h claim; e763bd33 re-keys it to the WL-0170 vbe101 flairdb
  leg plus the pixel leg (merge 377ba439).
- initech-hk3py: 119b454e made a Ctrl-C ending an EXEC child ask 'Terminate
  batch job (Y/N)?' (MSG-DOS-0063), which changed the failure mode of the
  xcopy ctrlc mutant. 795e812a accepts the mutant only on INT23-BREAK plus the
  prompt evidence, never on the generic EXEC-completion red (merge c28b80f0).
- Only the two box-isms were environment. Everything else was a main
  regression the laptop never certified after the wave-2 merges.

Kernel size round 8 (initech-hztg, Codex gpt-6.1-sol lanes, Opus-reviewed):
- Part 1 (d77e7300, merge 6b02c678): measurement (flagship 176,756 ->
  183,956 B since round 7: text +4,912, rodata +2,290), LTO final codegen
  keeps per-function/data sections so section GC works through the LTO link
  (-256 B), test-kernel-gc-lto plus mutant. See WL-0179.
- Part 2 (merge 61294e7e): the round-2 parked levers: loader transfer
  constrained with full callee-saved context and EBX input (1ad0469f,
  85be0439), event and kbd rings with explicit compiler barriers (b59a73fb,
  f774410f), ATA/IDT/panic audited (87e6fb8f, 75d849a4, a717cb70),
  -malign-data=abi kernel-only (5d8641b2), compact panic raster (2351a872).
- Review fixes: 641cc6dc puts CLD on all 15 C-entry stubs and the loader
  resume (the CPU does not clear DF on interrupt entry, and the -Oz rings now
  use rep movs in IRQ1/IRQ12); the transfer checker fails closed (85be0439);
  two self-agreeing gates removed (0f8cf88c); test-kernel-df and
  test-kernel-transfer added with mutants.
- Flagship 183,956 -> 180,208 B. r4 is GREEN with a 16 B margin. r3 (172,492 B
  all-kernel cap) is still 9,076 B over. The bead stays open.

Parallel certificate (initech-9bgg6, operator ask):
- Design: docs/design/parallel-certificate.md (28ead8fe). Runner:
  tools/cert-sharded.sh (4b2ef4ec): N detached worktrees, work stealing, Bochs
  last behind the host lock, a solo re-run of every FAIL classing it
  FAIL-CONFIRMED or FLAKY. test-cert-runner self-test with a flock mutant
  (9309005a). BOOTSTRAP Sec 4 and HANDOFF Sec 6 name it (a0deda79). Merge
  e7dcb494.
- First full run at c9a5cd53, -j 8: 1454 PASS in 81 min against about 5 h
  serial, zero timing flakes.
- Every parallel failure was a hidden inter-gate dependency that serial order
  masked. Scratch dirs not created by the gate that writes into them:
  c9a5cd53 (test-samir-full-emu-mutant, build/dbsafe, initech-hteno) and
  be4213b4 (test-samir-memwidth-emu-mutant, build/samir-size, initech-27clv).
  22 SAMIR gates read build/CLIENTS.DBF without declaring it: 00df3137 makes
  every DML/print/abbrev/fns/memwidth consumer declare $(CLIENTS_DBF)
  (initech-72872; merge 97a8588d). Nothing deleted the fixture; one worker
  simply ran a reader before any minter.

## Why

A certificate that nobody has run on the HEAD is not a certificate. The new
box was the first independent environment for main in a week, and it found
five main regressions (one P0) that the laptop's incremental habit had hidden,
which is the argument for keeping a from-scratch per-gate sweep as the
reference. At 5 h serial the sweep is too slow to run often, hence the
sharded runner; its first run immediately surfaced undeclared dependencies
that are real defects in the gate graph, not runner noise. The size round
exists because wave-2 merges grew the flagship past the r4 floor.

## Frictions

- `make -pn test` recurses and prints the whole make database per sub-make;
  use `make -pn` with no target to list gates.
- The interactive grep is a ugrep shell function; agent commands must use
  /usr/bin/grep.
- codex exec in a worktree could commit this time (an earlier memory said it
  could not), but creating worktrees from inside its sandbox needs a clone.
- The quota app's Windows-side Claude token expired mid-session.
- Serial order masks dependencies: three of the five parallel failures were
  green serially by Makefile order alone.

## Acceptance

- Serial sweep red list at 2808f8b5 (30) is closed except initech-hztg:
  zxzi, e8s3, u01i, kqr8, buze, hk3py, 3e7q, 3kiy, pla5 closed; hteno, 27clv,
  72872 closed.
- r4 green (16 B margin). r3 RED: 9,076 B over the 172,492 B cap. OPEN
  initech-hztg. The next kernel-growing lane re-breaks r4.
- Parallel run 1 (c9a5cd53): 1454 PASS, 81 min. All failures fixed on main.
- Parallel run 2 at the final HEAD 61294e7e: see the Run 2 addendum below.
  The operator ratified the sharded runner as the certificate on run 2 and
  VETOED run 3.
- No full serial `make test` has been run on 61294e7e.


Run 2 (added 2026-10-24 17:05): tools/cert-sharded.sh -j 8 at 61294e7e in
../initech-os-cx-parcert/build/cert-full2: 1483 PASS, 2 FAIL-CONFIRMED (test-kernel-size-r3
and its mutant, the owed r3 decision), 0 FLAKY, 66m45s wall clock (169m user). The
--diff against run 1 lists only the gates today's fixes changed (r4 pair, the 22
CLIENTS.DBF readers, memwidth-emu-mutant) and the four gates added by the size
lane (test-kernel-df, test-kernel-transfer, + mutants). A run 3 at the same HEAD was
started for the 'two consecutive runs without FLAKY' clause of design Sec 4 and was
VETOED by the operator as excess (ruling 2026-10-24: `tools/cert-sharded.sh -j 8` IS
the certificate on the strength of run 2; the clause stands as written, not as
applied). The partial run-3 log in ../initech-os-cx-parcert/build/cert-full3-run.log
is not a result. Serial `make test` is kept as a one-off cross-check only.

## Pointers

- Operator decision owed: kernel r3 cap. Either a structural overlay
  programme or a ruling on the semantics of the all-kernel cap. Without it
  hztg cannot close.
- Unmerged lane queue (11 branches, about 48 commits, 17-20 Oct, all need
  rebase and re-verification): flairman 19, l123man 8, flaircmd 6,
  finderalerts 3, finderrobust 3, iwordp4 2, pascaltbx 2, mints 2, i123p4 1,
  dupinfo 1, bootscreen2 1. Recommendation: certificate first, then smallest
  code lanes first (bootscreen2, dupinfo, pascaltbx), the book lanes after the
  pubs gate is stable.
- Run 2 is diffed and the flip to the sharded certificate is done (4d283f9b:
  BOOTSTRAP Sec 4, design Sec 4; bd memory certificate-2026-10-24). No
  further certificate run on 61294e7e; the next run is on the next merged HEAD.
- Docs: docs/design/parallel-certificate.md, docs/design/kernel-size-policy-round8.md,
  docs/BOOTSTRAP.md, WL-0178, WL-0179.
- Beads: zxzi 3kiy pla5 3e7q e8s3 buze u01i kqr8 hk3py hteno 27clv 72872
  (closed); hztg, 9bgg6 (open).
