# WL-0194 -- Raising the quality floor with Codex lanes (2026-11-05/06)

## Context

Operator directive for the session: Codex quota plentiful, Claude thin;
orchestrate 2-3 beads in parallel on gpt-6.1-sol xhigh coding lanes with
reviews, focus on the quality floor (hidden divergences from ground truth,
gate honesty, factory trust). Muse Wonder Full ("wonder full",
muse-wonder-full-1.0) allowed for review of important code, at most four
uses; two were spent. Resumed from WL-0193 on main 8a9b177b. The PM
(Claude Fable 5.1) wrote briefs, answered one lane question, merged
serially and re-gated on main after every merge (Rule 4); no lane pushed.

## What changed (main 8a9b177b -> this shard)

- c57b8400 Merge lane t3/ndx-incremental-parity (initech-7tj3, CLOSED):
  SAMIR multi-level .NDX incremental insert/delete and the streaming bulk
  layout now match real dBASE III PLUS 1.1 on all 516 minted snapshots in
  ../dbase3-decomp/mint/work. New oracle test-ndx-incremental (+mutant,
  five named mutants) pins the snapshots by sha256. RED first on unchanged
  main: 529 checks, 249 failures (first RED B021.NDX size 4608 vs 5120);
  GREEN 1731/1731. Main's index gates had been green only because none
  compared those goldens. Resident SAMIR 87,616 -> 88,192 B, headroom
  5,584 B, caps unchanged. Review: Muse Wonder Full, MERGE.
- 399680d8 Merge lane t3/fix-date-oracle-semantics (initech-bkmv, CLOSED):
  SAMIR dates follow real III+. CTOD rolls calendar-invalid dates forward
  with the III+ linear month term (99/99/85 -> 06/23/1993 captured); blank
  DTOC keeps separators (already on main, now mutation-pinned); the date
  comparison matrix is per captures. Rule 8 locked-spec edit:
  spec/samir/xbase_coercion.json R_blankdate_high re-worded (name kept)
  with rule_notes citing the bead and capture paths. Real III+ captures,
  programs, sha256 manifests and a DOSBox-X recipe committed under
  harness/diff/dbf_diff/xbase_prog_diff/datatype-mint/datecontract/.
  References re-keyed RED first (fn-a 6, eval 26, coercion 24, setfmt 2
  failures). InitechBase Volume One/Two entries and three transcripts
  updated. Review: Muse Wonder Full, MERGE.
- ce7a0a7a Merge lane/pubsdrift (initech-z8lca, from WL-0193): SET
  HEADINGS text; c-display, l1-display, l2-memo re-minted vs real III+
  at MEMOWIDTH 50. test-pubs-tps and test-pubs-examples green (296/296).
- 43bbad93 Merge lane t3/fix-parallel-producers-qemu-logs (initech-lroek,
  initech-tfn6r, both CLOSED): one producer per shared kernel object and
  pubs transcript (overlay.mk:264 shared-BUILD sub-make removed, competing
  mouse.o / mouse_mut_eoi.o recipes removed with flags preserved, declared
  private transcript producers in pubs/turbo.mk); the 'no kernel symbol
  kept' -j16 race reproduced on a clean build before, three clean -j16
  runs after. QEMU -d log retention in harness/emu/qemu.c: a watcher polls
  the log during QMP waits and kills QEMU through pidfd_send_signal past
  INITECH_QEMU_LOG_CAP_BYTES (RED QEMU-LOG-CAP, log kept); after OK=1 the
  raw log is removed, OK=0 keeps it. Default 64 MiB
  (harness/emu/qemu_log.mk); scoped 512 MiB for test-samir-forms-emu
  (+mutant), 2 GiB for test-samir-dml-*-emu(+mutant),
  test-samir-abbrev-emu, test-samir-print-emu-mutant; historical sizes in
  harness/README.md. test-harness-qemu-log(+mutant) in TEST_EMU_GATES.
  Review: gpt-6-astra FIX-THEN-MERGE with three blocking items (cap vs the
  large green legs, watcher SIGKILL on a reused pid, gates dropped from
  TEST_EMU_GATES by include order), applied in 1fa8c704.
- 471267d2 Makefile: the test-ndx-incremental rules read SAMIR_HEADERS
  before its definition (silently empty prerequisite); inline wildcard
  like the neighbouring rules. Found by the Lane C re-gate on main.

## RULING AMENDMENT (operator to confirm): blank-date comparison

The 2026-10-24 ruling on initech-bkmv said "follow real dBASE III PLUS
1.1" and derived "a blank date compares LOW in expressions". Fresh
captures from the real interpreter (DATECHK.PRG/.TXT, VERIFY.PRG/.TXT,
CMPLOG, CMP2LOG; literal CTOD(""), memvar and field operands; all six
operators both orders; SET EXACT OFF and ON) contradict the derived
detail: with exactly one blank operand EVERY comparison prints .F.; with
two blank operands EVERY comparison prints .T. (including <>, < and >).
Per the ruling's first sentence the PM told the lane to encode the
captures (Law 2) after a second independent capture agreed. INDEX key
order keeps blank LAST (separate contract, graded against the minted
.NDX snapshots, unchanged). The Muse review verified the captures line
by line against eval.c:416-421. If the operator prefers the LOW clause
as a deliberate deviation, revert eval.c's matrix and the JSON rule text
under a new bead; the captures stay as evidence either way.

## Acceptance (re-run by the PM on main after each merge)

- Lane A: ndx family 22 gates green, test-ndx-incremental 1731/1731,
  test-merge-samir ALL GREEN (410) -- build/pm-logs/laneA-*.log.
- Lane B: date/fn/datatype/ndx/lockstep family 67 gates green,
  test-merge-samir ALL GREEN -- build/pm-logs/laneB-*.log.
- pubsdrift: test-pubs-build, test-pubs-tps, test-pubs-examples green.
- Lane C: test-harness-qemu-log(+mutant), test-makefile-vars (after
  471267d2), test-mouse-producer(+mutant), test-flair-mouse,
  test-ovl-context-static-mutant, test-samir-forms-emu green; the -j16
  trio test-kernel-repro / test-kernel-flair-repro / test-ovl-repro green
  from clean; test-pubs-turbo green -- build/pm-logs/laneC-*.log.
- Not run: full make test, the sharded certificate, Bochs legs beyond the
  lanes' own, 86Box.

## Findings

- Three green suites hid real divergences from ground truth: the index
  engine (249/529), the date references (hand-written, never minted), and
  the -j gate vector (second producers). Each is now pinned by an oracle
  that computes nothing from our own output.
- Two Muse Wonder Full reviews at xhigh were precise (file:line evidence,
  every PM question answered, honest residuals); gpt-6-astra found three
  real blocking items in the factory lane, including a cap that would have
  turned two green emulator families red.
- Codex lanes do not run test-makefile-vars unless told; add it to every
  brief that touches the Makefile.

## Frictions

- The samir-forms and samirdml -d logs the cap was meant to bound had
  been deleted on 2026-11-02, so the lane's size survey could not see
  them; the bead text was the only evidence. Keep sizes in the bead when
  deleting.
- The datatype_mint.sh update mode never rewrites datecontract goldens
  (verify-only by design); re-minting is through
  datecontract/date_contract_mint.sh with DBASE3_DECOMP absolute.

## Wave 2 (parked at operator wind-up, UNVERIFIED, gates not run)

Three gpt-6.1-sol lanes were cancelled mid-work; each committed its
state on its branch. Pushed as origin/lane/*, bundled with vs-main
patches under ../initech-lane-archive/branches-2026-11-06/. Dispositions
on the beads (all back to open):

- lane/tsr-exec-relocation (initech-h4bk): loader/int21/command/kmain +
  a new test-tsr-exec leg (harness/emu/tsr_exec.mk); kernel size unknown.
- lane/samir-dotprompt-cursor25-fidelity (initech-0spyf, initech-fzsvo):
  screen capture/file/tool changes and several screen-mint ORACLE.sha256
  files REWRITTEN -- verify against real dumps before trusting.
- lane/fix-samir-do-file (initech-104j3): real III+ disk-DO captures
  (498b3683) plus a wip commit; collides with the dotprompt lane on
  screen-mint SETUP .PRGs.

## Pointers

- Beads closed: initech-7tj3, bkmv, lroek, tfn6r. Filed: ndx header
  reserved-bytes assertion; mint the two [inferred] ndx series; mint '#'
  and blank/blank >,>=,<> date probes; overlay.mk:231 residual sub-make.
  Notes: z8lca (gs-dir.txt screen re-mint still owed -- mint once after
  wave 2 lands), h4bk, 0spyf, fzsvo, 104j3.
- Memories: operator directive 2026-11-05 (Codex lanes), Muse allowance
  and model id, Muse review results, Makefile-lint lane lesson.
- Reviews and lane reports live in the T3 worktrees under
  /home/tobiasosborne/.t3/worktrees/InitechOS/ (build/*REPORT*.txt).
