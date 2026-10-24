# WL-0176 -- 2026-10-23: six hands-on audits, then the fix wave (Opus/Sonnet workers, gpt-6.1-sol reviewers and coders, serial merges)

## Context

Operator, 14:30: "loads of codex quota", select subsystems for gpt-6-astra
xhigh hands-on drives ("catch all the edge cases, weird bugs, etc etc").
17:00: "time to get to work -- landing fixes, then north star coverage";
Opus workers, gpt-6.1-sol xhigh reviewers, Sonnet for lower-tier work, Haiku
for busywork, up to 10 concurrent subagents, velocity without drowning the
merge queue, one agent per job, no review spirals. Later: gpt-6.1-sol may
code as well. Base: main e67b8311 (WL-0174 + the 08:15 clip addendum).

## The audits (docs/audits/2026-10-23-<lane>/, each with REPORT.md, NITS.tsv,
shots/, evidence/, the QMP driver, then TRIAGE.md + BEADS.tsv from a
gpt-6.1-sol triage lane)

| Lane | Findings | P1 | Headline |
|---|---|---|---|
| iword | 91 | 1 | full-disk overwrite destroys the previous document |
| desktop | 9 new | 1 | a full volume without a Trash cannot be freed |
| das | 45 | 9 | full-disk save data loss in Text/Calc/TPSREP/Note Pad; film image panics without its data disk |
| dos | 47 | 8 | pipes/redirection on a full disk silent; XCOPY D: writes A:; FORMAT+SYS corruption |
| samir | 48 | 3 | invalid-date REPLACE bytes; Esc cannot stop a loop; recursion kills the session |
| i123 | 259 | 1 | 224 unimplemented slash-menu leaves; full-disk save data loss |

Refuted (REFUTATIONS.md beside the report): i123 L002 (F2/Enter keeps 239
of 240 exactly as real R2.2, T240.WK1 / K22); i123 triage B03 (format
ceilings match real R2.2, minted under fixtures/i123-numeric); das L001 is
a wrong on-screen reason, not a wrong halt (B08). 133 triage beads created
under label audit-2026-10-23 (ledger docs/audits/2026-10-23-BEADS-CREATED.tsv).

## The merge discipline that held

One bead per lane; Opus/Sonnet workers in `.claude/worktrees/` (Agent
isolation), gpt-6.1-sol coders in sibling worktrees `../initech-os-cx-<lane>`
on `lane/cx-<lane>` (they do not commit; the PM skims for isms and commits);
one gpt-6.1-sol read-only review per substantive lane with a MERGE /
FIX-THEN-MERGE / REJECT verdict and at most one fix round; the PM merges
serially on main through a flock'd runner (`pm-merge.sh`: merge-tree dry
run, --no-ff merge, the lane's gate family, the size-floor line), closes
the bead on green, removes the worktree. Two marker-file-chained runners
once interleaved and corrupted LTO objects on main ("resolution sub id not
in object file"); a `make clean` re-grade confirmed every family green and
the single lock has been the rule since.

## What landed on main (in merge order)

7z2z (kbd.c BIOS flag-byte pointer laundered; mutant build fixed),
formatsys (FORMAT/SYS raw writes keep mounted C:/D: coherent; DEC-15 L042),
dosredir (sticky SFT write_fault; MSG-DOS-0005/0046; AH=5Bh pipe temps),
samirdate (blank date, 1E+100 index key, '*' N as DBL_MAX, dec_format
int64 boundary; NEW real III+ datatype-mint), itextlines (LF-only line
ends kept; full-document notice; Standard File bad-name alert, both
pixel-graded with a draw-suppression mutant), ioke (eight red SAMIR unit
gates: two implementation bugs vs new mints SF-division/numlist/exprcol,
the rest re-keyed to real captures; 11 Volume One transcripts re-minted),
findertrash (Delete Now on a full volume, Put Away multi-selection with
metadata, New Folder grayed in the Trash; three clips repro-proven),
tpsrepfull (FCB write checks TPS-009, LINES/PAGES validation TPS-010/011),
drivext (dospath.h resolver; INT 25h/26h per mounted drive; ADR-0003
Amendment DEC-15a), samirctl (Esc dialogue, DO cap 20, RETURN TO MASTER,
QUIT, RETRY; NEW real III+ samirctl-mint committed in-repo), atomicsave
(tbx_file_save: CREATE_NEW temp, close, delete, rename; every tenant save
converted; TBX_FILE_RENAME 0x0089 + TBX_FILE_CREATE_NEW 0x008A), cx-dosmsg,
cx-listoff, cx-cpanels, cx-formatmsg, cx-samirfmt, cx-filesreap, cx-tpsbak
(TPS-012), cx-inputdrift, cx-deletenow, and (pending at writing) labelpubs,
typeint, itextfix, i123num, ctlfixtures, ctrlbreak, calcquit, samirsize,
screendrift.

## Rule 8 notes -- locked data changed deliberately this wave

- spec/toolbox_gate.h Sec 10b: TBX_FILE_RENAME 0x0089 (DOS AH=56h) and
  TBX_FILE_CREATE_NEW 0x008A (AH=5Bh), additive (initech-mcep).
- spec/tps_ritual.json: rows E9/E10/E11 (initech-ycvk) and E12 (initech-56ch),
  additive, with manual entries in lockstep.
- spec/absdisk_int2526.json: "AL is not a mounted volume", drive 3=D:
  (ADR-0003-AMENDMENT-DEC-15a, initech-yyi2).
- spec/flair_file_traces.mk FLAIR_FILE_CRASH: kret after each crash
  (PM-approved; the crash alert postdates the trace; initech-3j17).
- spec/flair_calc_traces.mk, spec/flair_fullsave_traces.mk,
  spec/flair_trash_edges_traces.mk: new locked traces (calcquit,
  atomicsave, findertrash).
- test-flair-trash-purge-views ctrln leg re-keyed: New Folder in the Trash
  makes nothing (period Finder; initech-vnl4).

## Frictions

- Codex refuses a non-git scratch dir; `git init` + `--skip-git-repo-check`.
- The OpenAI content filter killed the dos audit mid-run on a sentence about
  Ctrl-C echoing kernel bytes; a resume lane rebuilt the report from the
  evidence. Phrase lane briefs as QA bug reports.
- Codex sandboxes cannot open the Bochs RFB socket: every Bochs leg is run
  by the PM on main after merge.
- Worker agents waiting on their own `tail -f` watchers fire repeated
  "waiting" notices; a nudge resumes them.
- Shared budgets: flagship headroom 17,420 -> ~14,700 B across the wave;
  SAMIR.COM crossed the then-live lane reclaim cap (99,738 > 98,175 B) after
  five SAMIR lanes, prompting a size round (initech-vn4i). Operator ruling
  2026-10-24 retired that arrival criterion; see WL-0182. The physical
  PAL-memory headroom floor remains live.

## Acceptance

Each lane: RED gate + mutant green on its branch, family green on main after
merge (the runner log build/pm-merge-<lane>.log), bead closed with the
commit and the follow-ups. Named reds on main at writing: test-samir-size
(vn4i), test-pubs-screens after drivext (64yp), the Bochs copybreak leg
(run by the runner). The full certificate (`make clean && make test` in
../initech-os-cert) is still owed after the queue drains.

## Pointers

docs/audits/2026-10-23-*/, docs/adr/ADR-0003-AMENDMENT-DEC-15a.md,
harness/diff/dbf_diff/xbase_prog_diff/{datatype,samirctl}-mint/,
fixtures/i123-numeric/, os/apps/tbx/tbx_save.h, os/milton/dospath.h,
docs/research/mints/numlist-v1-2026-10-23/.
