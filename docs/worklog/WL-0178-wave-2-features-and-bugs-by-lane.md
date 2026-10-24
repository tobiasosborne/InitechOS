# WL-0178 -- PM session 2026-10-23/24, 21:00-: wave 2 after the audit fix wave -- features and bugs by lane, merged as they came

## Context

The operator wound the 2026-10-23 audit fix wave down at the Codex reset, then at 21:00 asked for
work to resume: orchestrate, a mix of bugs and new features, Codex gpt-6.1-sol xhigh for coding
and review, Opus for coding, an occasional Fable lane told about the quota app, 2-3+ concurrent
lanes, merges landing at velocity, no review spirals, commit and push regularly. Quota band:
up to and a little over pace on both Claude and Codex; Fable within Claude + 2.

## What changed on main (lane merges, in order; PM commits between them)

| Lane | Model | Beads | What |
|---|---|---|---|
| calcquit | Sonnet (from wave 1) | qxft | InitechCalc: repeated Save keeps the model; staging never clobbers TABLES.TMP |
| panic | Codex | 480p, dbp1 | typed loader/startup reasons on the PC LOAD LETTER screen |
| i123cells | Opus | 3qtt, 3s12 | oversized first column clipped to the area (real R2.2: >72 clips to 72); default-cell erase frees; absent cells keep default protection |
| iwordnav | Opus | 1zk9, 10bx | PgUp/PgDn + Go To (real WP 5.1 mint); "Invalid file name" on spaces |
| calcda | Codex | 0xs6 | Calculator answers the shutdown quit query; refused quit explained on screen |
| execguard | Fable | 13ig (P1) | flagship EXEC COMMAND.COM -> resident COMMAND /C; COMMAND3 overlay refused by content; transient imports derived at build time |
| itextundo | Opus | 4hkk | one-level Undo in InitechText (Mac guide 6.0 p.118) |
| finderopen | Fable | fbo4 | Finder File > Open / Ctrl-O launch a selected application through the one launch spine |
| samirabbr | Codex | 13d3 | four-letter abbreviations; REPLACE scope grammar; three real-III+ mints |
| menubars | Codex | c8w7 | preference redraw through the shell compositor (both bars, clock, foreground menu) |
| i123copy | Codex | fyji | /Copy destination rectangles + shared POINT selection (real R2.2 mints) |
| doscmd | Codex | f8f0, 0qw1 | DEL <directory> with the DOS question; "Terminate batch job (Y/N)?" incl. Ctrl-C during an EXEC child (real DOS 3.30 mint) |
| iwordlist | Codex | 7aou | InitechWord List Files keys 2 Delete / 3 Move-Rename / 8 Copy live (real WP 5.1 headless mints); review fix: a trailing-dot FAT alias of the same file (OPS. == OPS) no longer deletes the only copy -- FAT-backed regression |
| samirstatus | Codex | f99h | SAMIR STATUS / DISPLAY MEMORY from real III+ captures; SAMIR.COM size round (100,385 -> 97,705 B under the 98,175 cap); review fix: STATUS scans INTO as a whole keyword outside literals (statusinto corpus) -- merged 71bd666e, both SAMIR Bochs legs green |

Rule 8 notes (locked spec / controlled vocabulary changes, each with its bead):
- spec/dos_messages.json + ADR-0003 Appendix C: MSG-DOS-0063 "Terminate batch job (Y/N)?"
  (MS-DOS 3.3 Reference PAUSE, printed p.149; initech-0qw1). Catalogue route, no dosmsg.allow
  exception.
- spec/iword/listfiles.h + spec/iword/fkeys.json: List Files keys 2/3/8 (Delete, Move/Rename,
  Copy) live=1 (real WP 5.1; initech-7aou) -- merged 0dc0b22d.
- i123 test-i123-range-menu R6 re-keyed: /Range Value into protected empty cells is refused
  (it only passed because of the B02 bug; initech-3s12).

## Backlog triage (operator question: "406 open beads sounds like a lot")

One Opus agent checked all 406 open beads against main (HEAD 3e32c47f) with six read-only
sub-checks; 396 verified in code/git/worklogs, 10 title-only guesses. Headline: 41 already done
on main (closed tonight: 32; the other 9 -- w06, f8v, f8v.4, 2rv, 5em, 6h6, alg, ax9, 17n -- sit
behind operator-decision or stale-epic deps dam/fyy/7az/iqe/0tl and were NOT forced), 10 epics
closable after re-parenting, 21 TRIVIAL (Haiku), 69 SMALL (Sonnet), 184 MEDIUM (one lane),
39 CORE, 42 blocked on external material (19 of them DOS utilities with no reference binary:
does the DOS 3.3 manual alone satisfy Law 1? -- operator). Staleness: 133 beads untouched 30+ days,
116 over 90; 7 of 9 P0s are stale milestone epics that also manufacture most of `bd blocked`.
Two layers: a June planning layer (112) and the October audit layer (143 touched 2026-10-23).
Thirteen lane branches hold unmerged WIP for untouched beads (flairman -> ipu5.11, l123man ->
ipu5.9, flaircmd -> tdnl.106, finderalerts -> tdnl.72, iwordp4 -> 8q6j, i123p4 -> 1z3o, dupinfo
-> p6st, pascaltbx, mints, finderrobust, bootscreen2): each needs a merge/rebase/abandon ruling
before its bead is dispatched again. Report, TIERS.tsv and CLOSE-NOW.txt lived in the PM
scratchpad; the numbers above are the durable record. Also surfaced: bsy.5 (BREAK default ON at
boot vs the DOS 3.3 manual's OFF), b1q0 (duplicate Makefile recipe pair: deleting the wrong one
changes kernel flags), CLAUDE.md still says ssim.c is unbuilt (it is built: f04b1511).

## Turbo Pascal ground truth (initech-v872)

Turbo Initech had no real-software golden (only Free Pascal). A Sonnet lane created
`../turbopascal-decomp` from the official Embarcadero museum tp55.zip (sha256 a04ff655...,
TP 6.0 not obtainable: 404 at the museum, only DE/PC-98 builds elsewhere) with a dosbox-x mint
harness on Xvfb :88 and goldens MINT00-05: 18 IDE shots + measured geometry, 70 verified
compiler error numbers, 17 runtime error numbers with the exact `Runtime error NNN at
SSSS:OOOO.` line (exit code = error number), Write/Str/Val formatting beside fpc output, and the
57-program initech-os corpus under real TPC. Divergences (re/fpc-vs-tp55.md): TP Integer is
16-bit without widening (200*200 = -25536; three corpus goldens carry REV=54321 only because of
{$MODE DELPHI} -- OPERATOR DECISION on Turbo Initech's Integer width), round() is half-away-from-
zero, default Real prints 3.1415900000E+00, 23 corpus files fail on the MODE header alone. Beads
k65i obk9 x2fd 9sie cu0f 590o lrqy filed.

## New-machine readiness (operator priority, end of session)

The six decomp siblings and the manuals library existed only on this laptop with no remote.
Tonight: private GitHub repos tobiasosborne/{dbase3,lotus123,system7,win31,wordperfect51,
turbopascal}-decomp (all branches + tags pushed) and tobiasosborne/initech-manuals-ref (manifest
+ fetch/verify scripts in git; the 93 scans, 1.1 GB, as release v2026-10-24 assets, one tarball
per directory). Each sibling's gitignored goldens (real software images, ROMs, Mac OS 8.1
captures, mint work; 16 of these files are read directly by the Makefile) are release
`goldens-2026-10-24` assets with sha256 manifests (dbase3 579 MB, win31 677 MB, system7 1.6 GB,
wordperfect51 44 MB, lotus123 17 MB, turbopascal 3.8 MB). docs/BOOTSTRAP.md is the rebuild
procedure; tools/bootstrap/doctor.sh checks every tool/sibling/manifest and fails loud;
tools/bootstrap/fetch-siblings.sh clones + restores + verifies (round-trip tested on two
siblings: byte-identical). GitHub LFS was rejected (1 GB free quota); release assets have a 2 GB
per-file limit and no quota.

## Frictions

- Agent isolation:worktree spawned three lanes from e67b8311 (not HEAD) and the harness removed
  the unchanged worktrees when the STEP-0 gate stopped them. Remediation (bd memory updated):
  the PM cuts ../initech-os-wt-<lane> at the pinned sha and spawns without isolation.
- Two merges landed Makefile hunks that were green in the lane and red on main: a
  read-before-definition (TRANSIENT_IMPORTS_LD) and a bare-path fixture volume with no derived
  contract. pm-merge now always runs test-makefile-vars, test-fixture-volumes and
  test-pubs-lint before the lane's gates.
- A grep-derived gate list picked up test-samir-fns-pending, which names a missing target
  (bead filed); gate lists are now filtered against the registered TEST_*_GATES vectors.
- The pubs period-year lint read "about 2000 characters" as a year (reworded).
- Tail-append Makefile conflicts (two lanes) resolved by keeping both sides in the lane
  worktree, then re-queuing.
- Codex lanes stop correctly on Rule-5 questions (doscmd: locked message; iwordlist: locked
  List Files table; calcda: no real-Mac paste fixture) -- each got a PM ruling or a follow-up.
- SAMIR.COM: DISPLAY STATUS/MEMORY at full fidelity cost +4,664 B (2,210 B over the
  then-live reclaim cap); a size round ran on the lane before merge. Operator ruling
  2026-10-24 retired that arrival criterion; the PAL-memory floor stays (WL-0182).

## Acceptance

Every merged lane re-verified by the PM on main (its families + mutants + the three lints +
headroom; Bochs legs where the family has them: panic, execguard x3, finderopen, menubars,
i123copy x2). Eleven Rule-14 clips recorded and repro-proven on main after the merges
(film_panic, film_scene, calculator_power, calculator_refused, i123_wide, iword_goto,
iword_badname, itext_undo, finder_open_app, cpanels_pattern_bars, i123_copy_tile) + iword_listfiles_ops: operator
eyeball owed. Full `make clean && make test` certificate: NOT run on this HEAD (first job next
session, ../initech-os-cert).

## Pointers

- Reviews: one gpt-6.1-sol read-only review per lane, one fix round max (every FIX-THEN-MERGE
  tonight was a real defect: protection on absent cells, REPLACE scope grammar, five unchecked
  transient imports + two unguarded loader entries, a uint8 launch count, a clip image).
- Follow-ups filed: uyi1 (Range Name protection), 0zm4 (B02 emu replay), kz49 ([Wn] width tag),
  COMSPEC on the flagship, iword goto emu leg, Go To completions, test-samir-fns-pending.
- fxrq (Calculator Copy/Paste) blocked on a real-Mac paste mint (bead note).
- 4zpa: SAMIR DISPLAY two/three-area paging pauses once more than real III+ (found by the
  samirstatus fix round; evidence under the lane's build/status-fix).
- Gate-list lesson (bd remember): a gate's rule may live in harness/**/*.mk, spec/*.mk or
  pubs/pubs.mk; a Makefile-only filter dropped 31 of 51 iwordlist gates from the merge pass and
  they ran as two further passes behind the lock (all green).
