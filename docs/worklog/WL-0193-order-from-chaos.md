# WL-0193 -- Order from chaos: landing the parked lanes (2026-11-02)

## Context

Operator directive for the session: merge the unmerged work, tidy up after the
parallel waves, reduce entropy; Claude subagents only (Opus for core lanes,
Sonnet for verification and pubs), no Codex, no Fable subagents; work up to
pace. Resumed from WL-0190 on main ca10414d with 33 remote branches, 23 of them
unmerged, and 23 GB under build/.

## What changed (main)

- 5060706c Merge lane/samirmisc: F2b per-COM scratch keys for the SAMIR audit
  gate (initech-1x6x2.2); test-dbase-corpus-pristine pins CNAMES.NDX and the
  sample corpus to the pristine III+ disk image, sha256-pinned, mutant bites
  (initech-586.4.1 closed).
- b2ac83ce Merge lane/testpar-r (initech-1x6x2.1): the parked
  wip/f7b-testpar-stack reduced from 9 commits to the 4 with content not on
  main. First ever `make -j16 test-par`: 1677 gates, 1581 s.
- 203cab07 Merge lane/mainreds: four gates that were RED ALONE on main repaired
  at the root (initech-1ipnt, 7bqud, dieap, eug5v; all closed).
- ea175e28 Merge lane/ccrepro (initech-4yzna closed): the three repro gates that
  compile inside the gate run their private rebuild with CCACHE_DISABLE=1.
  Proven with a __TIME__ mutant under simulated ccache sloppiness.
- fe5c430c Merge lane/kroom: Restart / Shut Down / Exit to InitechDOS into
  section COLD (os/milton/power_cold.c, power_overlay.mk,
  test-ovl-absence-power-mutant); the DB_WRITE_SKIP mutant object built with
  production flags; then ADR-0003 Amendment DEC-19 (INT 21h AH=4Bh AL=03h Load
  Overlay) merged onto the freed room (initech-qshpn, initech-dewv closed).
- DEC-19 RATIFIED 2026-11-02 (operator, delegated to the PM in session), v1.0.
- a06e82a8 Merge lane/manuals-r (initech-ipu5.11): the FLAIR User's Guide
  (168 pp), Accessories Guide (64 pp), Getting Started (32 pp) and Quick
  Reference card (4 pp), brought to today's product; 125 of 145 figures
  re-minted. 95a5229e: two PM fixes the merge needed on today's main.
- REVIEW-BRIEF.md reads CLAUDE.md from the reviewed repository root.

Kernel sizes (cap 169,984 B unchanged, ADR-0014): largest kernel 169,892 B at
start, 166,200 B after the power/exit move, 167,360 B with DEC-19 in
(2,624 B margin); flagship 168,724 -> 167,168 B. COLD section 21,624 of
32,768 B; FLAIR.OVL 19,968 of 131,072 B.

## Why

The merge queue had grown faster than the PM could re-verify it, and the
parked branches were drifting out of mergeability (the oldest were 670 commits
behind). Each lane was replayed onto today's main in its own sibling clone,
reviewed, gated there, then merged by the PM and re-gated on main before the
push. Nothing was merged on a lane's word alone (Rule 4).

## Branch dispositions

Every unmerged branch was bundled under
`../initech-lane-archive/branches-2026-11-02/` before anything was deleted;
the four parked WIP lanes also as plain patches against their merge base.
The operator ran `delete-remote-branches-2026-11-02.sh`: 7 fully merged
branches and 12 whose content is on main in another form (f4b, f7b-build-race-2,
recorder-sync, kov2, wave1-say/log/ndx/sqrt, numfmt, bootscreen2, finderrobust,
i123p4) are gone. Remote count 33 -> 12 before this session's lanes landed.

Parked with dispositions on their beads: lane/dupinfo (initech-p6st),
lane/iwordp4 (initech-8q6j), lane/pascaltbx (initech-tdnl.22), lane/flaircmd
(initech-tdnl.106; its ADR-0003 Amendment "DEC-19 Resident Toolbox Entry"
collides with the ratified DEC-19 and must become DEC-20 on revival), the
lane/mints ndx.c rewrite (initech-7tj3; see Findings), and lane/l123man
(initech-ipu5.9; see Findings).

## Findings

- The first `make test-par` found six gates RED ALONE on main, so `make test`
  was red before this session: boot-system-repro and the trash_visible mutant
  image predate the overlay kernels; check_fixture_volumes refused a
  multi-target rule; the quit-after mutant had stopped biting because key
  pacing plus cache=unsafe let the 34-EXEC batch finish before the quit;
  three InitechBase pubs drifts (initech-z8lca). Four fixed on main this
  session; the pubs drifts are in a lane as this is written.
- db_write_skip was 1,168 B above the flagship because its mutant object was
  compiled without KERNEL_TENANTS_OPT, outside the LTO partition.
- Main's multi-level .NDX build/insert diverges from real dBASE III PLUS in
  249 of 529 minted snapshot checks (the parked lane/mints harness run against
  main's ndx.c). Main's index tests are green only because none compares the
  minted multi-level goldens. Recorded on initech-7tj3 as a port with
  test-ndx-incremental as the failing-first oracle.
- The repro gates had been comparing incremental-vs-ccache (66/67 hits).
- Initech 123 changed after the 123 manuals were written (POINT mode, /Copy
  rectangle destinations, clock, $ entry); the Reference documents refusals
  the app now accepts. The books need a content pass before their gates can
  run.
- Green emulator legs leave their full QEMU -d logs behind: 20 GB of
  300 MB-2.7 GB logs under build/ (initech-tfn6r). Deleted; 18 GB reclaimed.

## Frictions

- A single `make -j16` over three kernel gates raced on
  build/transient_imports.rsp ("no kernel symbol kept"); each gate green
  alone. Recorded on initech-lroek; the PM ran kernel gates one make call
  at a time.
- Lane B's worklog shards reused WL-0183/WL-0184, both taken on main;
  renumbered WL-0191/WL-0192 at merge time. Lanes must take their shard
  number from main at merge time, not at branch time.
- Lane D met today's main twice: the shutdown alert text had moved to
  power_cold.c after the lane keyed its inventory to kmain.c, and
  PUBS_FLAIR_SCREEN_DEPS (a := list) was referenced above its definition,
  so the mutant gate owned no boot-image prerequisite and only passed on a
  tree where the screens route had already run.
- A finished Sonnet lane left a `sleep 3` polling loop holding its clone;
  killed by pid before the clone was removed.

## Acceptance

Per merge, re-run by the PM on main after the merge commit: lane C
test-dbase-corpus-pristine(+mutant), test-merge-samir ALL GREEN (138 s);
lane A kernel-repro(+mutant), headroom(+mutant), fat-move-mutant; lane E its
seven gates; lane F the three repro gates alone; lane B 19 gates one per make
call (kernel-size-ovl, ovl-absence(+2 mutants), ovl-window, dosmsg(+mutant),
overlay(+mutant), exec-overlay(+mutant), flair-shutdown(+mutant),
flair-exit-to-dos(+mutant), flair-appswitch, fixture-volumes, ovl-repro,
kernel-repro); lane D test-pubs-lockstep-src, lint, honesty, build, flair,
flair-mutant (15 rejected), iword; test-adr-citations(+mutant) for DEC-19.
Not run this session: full `make test`, 86Box, Bochs legs beyond the lanes'
own appswitch-bochs run.

## Pointers

- Beads closed: initech-586.4.1, 1ipnt, 7bqud, dieap, eug5v, 4yzna, qshpn,
  dewv. Filed: initech-tfn6r, z8lca, lroek, gtzi4 (prove the 12 FLAIR DRAFT
  entries). Notes: 1x6x2.1, 1x6x2.2, 7tj3, x8gsi, p6st, 8q6j, tdnl.22,
  tdnl.62, tdnl.106, 1z3o, 4sny, ipu5.11, ipu5.9.
- Archive: ../initech-lane-archive/branches-2026-11-02/,
  delete-candidates-2026-11-02.txt, delete-remote-branches-2026-11-02.sh.
