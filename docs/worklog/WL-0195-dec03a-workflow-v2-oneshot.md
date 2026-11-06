# WL-0195 -- DEC-03a ratified, merge workflow v2, the one-hour one-shot (2026-11-06)

## Context

Resumed from WL-0194 on main ca0c4f65 with three wave-2 lanes parked
unverified. Operator directives during the day: Codex gpt-6.1-sol xhigh
coding lanes with reviews (Muse Wonder Full at xhigh, six uses budgeted,
two spent on reviews, one as a coding trial); "merge time as small as
possible with a robust revert, fast regression finding, and lanes that
prove their work"; reconsider priorities if 100 KB of dBASE grinds us
to a halt ("good, do it"); run a one-hour relaxed-rules experiment;
"ratify as drafted"; finally (~18:30) Claude quota over pace: land what
arrives, no new work, no Claude work. The PM (Claude Fable 5.1) wrote
briefs, answered lane questions, merged serially and re-gated on main.

## What changed (main ca0c4f65 -> this shard)

- 797d87c5 Merge t3/tsr-exec-relocation (initech-h4bk, CLOSED): after a
  TSR is resident, top-level EXEC relocates an InitechMZ image to the
  first free MCB above the resident; a raw flat .COM after KEEP is refused
  before any state change with 'Insufficient memory' (DOS 08h). New QEMU
  leg test-tsr-exec(+mutant), Bochs boot, 39 gates. Largest production
  kernel 168,764 B (margin 1,220). Review: Muse Wonder Full, MERGE.
- ab5d69f8 Merge t3/fix-samir-do-file (initech-104j3, CLOSED): DO of a
  disk .PRG from inside a running program through one shared loader in
  os/samir/cmd/proc.c (nested DO, SET PROCEDURE precedence, ON ERROR DO,
  20-level limit 'DOs nested too deep.'); CLOSE PROCEDURE / CLOSE ALL
  per new real captures (library retained while one of its routines is
  active); teardown releases the library (32-cycle heap 2,800 -> 72 B).
  Coder: Muse Wonder Full (first coding trial; thorough, honest, found
  three defects in the parked draft). Review: gpt-6-astra FIX-THEN-MERGE,
  both items applied red-first. test-merge-samir ALL GREEN (413).
- 3bdb1b3f + e767254e ADR-0009 Amendment DEC-03a InitechMZ Extended
  Memory, committee (Seat A gpt-6.1-sol: period ground truth + loader;
  Seat B Claude Sonnet 5.5: SAMIR/oracles/pubs/FLAIR tenancy + W4
  re-grade; Chair gpt-6.1-sol), RATIFIED 2026-11-06 v1.0 by the operator
  as drafted. Decisions: 8 MiB machine (7168 KiB extended), application
  region [0x600000,0x800000), MZ tag 0x5849 v1 routes an image high,
  legacy tag / flat .COMs / TSRs / CHKDSK unchanged, low PSP, native
  48h/49h/4Ah in the region, 64 KiB high stack + guard, one high owner,
  256 KiB module ceiling tied to the declared loader reserve,
  SAMIR_SIZE_CFLAGS frozen, SAMIR.OVL kept, packaging B (initech-5etom)
  and resident shrink (1x6x2.18) retired, W4 re-graded to L1 byte-exact
  output files + L2 locked key trace with structural check + L3 Rule 14
  clip (W3 keeps exact screens), kernel cap policy unchanged (margin
  1,220 B). Ship plan Sec 7 re-points the critical path; seven
  implementation beads filed (initech-jarzu runtime, ayj36 locked specs,
  r2lqd emulator profiles, fxolx size gates, 9nzpr W4 acceptance, j0x4o
  pubs lockstep, 3gynk capacity).
- 08833cde Merge t3/implement-merge-revert (initech-ror2g, F10, CLOSED):
  tools/revert-merge.sh + docs/process/MERGE-REVERT.md; 22 fixture cases,
  3 mutants. Coder Sonnet 5.5; review gpt-6-astra, 3 items applied.
- ba59ed6b Merge t3/lane-certification-proof (initech-xses3, F9, CLOSED):
  tools/lane-cert.sh, tools/cert-check.sh, tools/proof-replay.sh,
  docs/process/LANE-PROOF.md, REVIEW-BRIEF.md proof-pack rule; review
  gpt-6-astra found three ways to forge a green certificate (ignore-errors
  make modes, cert not bound to the merge head, coverage failing open),
  all closed red-first with mutants.
- Pubs drift (initech-z8lca) re-gated green; initech-mm8vz (overlay.mk
  shared-build race) reproduced again on main, raised to P2.

## Merge workflow v2 (epic initech-0uphq) -- the new process

Goal: merge in minutes, revert in one command, find the culprit merge
mechanically, lanes prove done. Pieces: F8 locked path-to-gate map
(spec/gate_map.json, `make gates-for DIFF=`, `make test-for`,
test-gate-map honesty audit) -- in review fix; F9 certificate + proof
pack (landed); F10 revert (landed); F11 local sentinel + culprit finder
(tools/sentinel.sh, tools/culprit.sh, docs/process/SENTINEL.md) -- in
review fix. Rules: one lane = one no-ff merge commit; goldens and locked
spec rows travel in the same merge as the code; lane branches stay on
origin until the next green certificate; main may be red for one
sentinel cycle because the revert is mechanical; no GitHub CI (Rule 10).
Lane reports are PROOF PACKS: BRANCH/COMMITS, RED FIRST, GREEN, MUTANTS,
CERT, NOT PROVEN. Reviews read REVIEW-BRIEF.md + LANE-CONTEXT.txt written
by the PM; small lanes share one block review; the reviewer never shares
a model family with the coder on core code.

## The one-hour experiment (initech-7cbu8, branch exp/dbase-oneshot)

Opus 5.5 orchestrator, six gpt-6.1-sol coders, relaxed rules, 42 minutes.
Floor (test-merge-samir, spec/, goldens) held at every checkpoint. Shipped
in a separate SAMIRX.EXE with a fixed 8 MiB heap hack: W4.1 kit, BROWSE,
EDIT/CHANGE/APPEND/INSERT, ASSIST main paths, HELP (authored), CREATE and
MODIFY STRUCTURE, MODIFY COMMAND, REPORT + LABEL designers (real TP28.FRM
and ADDRESS.LBL round-trip byte-identical); 16/16 QEMU legs, 49
screendumps, all.sh replays in ~42 s. Not started: QUERY/SCREEN/VIEW,
CONFIG.DB, W3 slices 2-4, W2, DRAFT messages. Orchestrator estimate: 6-8
h more at that bar, 5-10x at ours. Two findings: 54 KB of UI does not fit
the 188 KiB window (DEC-03a confirmed empirically), and os/milton/kbd.c
drops the navigation keys before any program sees them (initech-wuxo0,
P1, lane running). Cherry-pick plan: key traces first (L2 seeds for
initech-9nzpr), then app by app behind L1/L2/L3 after Lane R and Lane W4A
land, in the order designers, kit, CREATE/MODIFY STRUCTURE, BROWSE,
EDIT/APPEND, ASSIST, MODIFY COMMAND, HELP. Never: samirx.ld, the SAMIRX
target, the pal heap hack, the samir_main #ifdef.

## Rulings recorded this session

- Real III+ governs hand-written expectations: SAY paints CHR(13)/CHR(10)
  as cells; LIST heading row from the capture; serial transcripts keep
  CR/LF. Generalised by the PM: re-key when SAMIR matches a fresh verified
  capture; stop only when SAMIR disagrees with the capture outside the
  beads (which happened for SET STATUS -> initech-ebwsw, P1, with its
  captures committed under a pending gate).
- Muse Wonder Full: xhigh, not max; 6 uses budgeted, 3 spent (two reviews
  and one coding trial), operator checks quota before more.

## Acceptance

Per merge, re-run on main: Lane D 41 gates incl. Bochs + repro trio;
Lane E2 16 gates + test-merge-samir (413); F10 four gates; F9 four
gates; ADR citations after the DEC-03a docs. Not run: full make test,
the sharded certificate, 86Box.

## In flight at the directive (Codex only; land as they arrive)

- t3/samir-dotprompt-cursor25-fidelity (initech-0spyf, fzsvo): finish 4
  -- STATUS carved out to ebwsw as a pending gate, serial re-keys, boot
  join check, mutants, kernel gates, test-merge-samir. Resident headroom
  exactly 4,096 B (at the guard).
- t3/mechanical-gate-map (initech-fw3x0, F8): review fix (three false-
  negative mappings + a source-to-gate audit); F11 rebases on it.
- t3/implement-local-sentinel (initech-3nc3v, F11): review fix (BUILD
  isolation, guarded cd, missing-gate culprit rule, red over deferred).
- t3/implement-extended-memory (initech-ayj36, jarzu, fxolx): DEC-03a
  specs + runtime + size-gate retarget; first attempt stopped on the
  size gate's tag check, relaunched with fxolx in scope.
- Lane W4A (initech-9nzpr): spec/samir/screen_regrade.json + L1 diff
  tools + L2 trace harness.
- kbd lane (initech-wuxo0).

## Pointers

- Beads closed: h4bk, 104j3, yndl2, 5etom, 1x6x2.18, ror2g, xses3, 7cbu8.
  Filed: 0uphq (epic), fw3x0, xses3, ror2g, 3nc3v, yndl2, 7cbu8, jarzu,
  ayj36, r2lqd, fxolx, 9nzpr, j0x4o, 3gynk, wuxo0, plus review follow-ups
  on h4bk and 104j3. Notes: 0spyf, fzsvo, ebwsw, mm8vz, z8lca, 9x8cn,
  j7lld, y29t4, p3nn0.
- Memories: operator directive 18:30 (over pace), experiment lesson,
  merge workflow v2 usage, Muse budget.
- Lane reports: build/*proof*.txt and build/*REPORT*.txt in the T3
  worktrees under /home/tobiasosborne/.t3/worktrees/InitechOS/.
