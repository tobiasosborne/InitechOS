# Status

Rewritten 2026-11-21 at the close of phase 4 day 1, on Spiral (WSL2, 64
cores, home /home/tobiasosborne; `-j16` is the normal figure). Setup:
`docs/SETUP.md`. Reports and handoffs live in the private repository
cloned at `/home/tobiasosborne/Projects/InitechOS-reports/`
(`phase2/`, `phase3/`, `phase4/`); today's running log is
`phase4/PROGRESS.md` there and the next session's start is
`phase4/BRIEFING.md`.

## State of main

- Head 964fa280 at the stop. Nobody holds a directory or a shared place
  (`mk/`, `sdk/`, `factory/`, top Makefile): the next orchestrator takes
  them.
- Morning: the P0 initech-qw018 (clean `make test` red) fixed in bcf0ed57;
  clean sample of bcf0ed57 green (test, image, smoke on QEMU and Bochs).
  Strict certificate on bcf0ed57: clean-test, image-smoke, repro,
  selfhost-ddc (K2 == K3 == D2, 241,239 bytes), status, tracked-files
  PASS; mutants and bigkernel FAIL, all in the checks themselves. All of
  those were repaired today (see below).
- Evening: strict certificate on 964fa280, started 18:14: clean-test PASS
  (155 s), image-smoke PASS (854 s, QEMU and Bochs), repro PASS (65 s),
  mutants PASS (4956 s, 0 pending patches), selfhost-ddc PASS (947 s),
  bigkernel PASS (35 s). It was cut off at 20:14 during the status
  stage by the two-hour limit on the command that ran it, not by a
  failure: status and tracked-files did not run, so its summary reads
  "failed stages: 1". Every product stage and the mutation stage are
  green on this head. Next session: run `make status` and the
  tracked-files stage on a clean worktree to complete it, and give the
  certificate command a limit above 7,500 s (the full mutation sweep now
  takes 83 minutes). Evidence: ~/w/cert-evidence/2026-11-21T171431Z-964fa280
  (and cert2.log beside it).

## Direction and rulings

- Direction (operator, 2026-11-21): InitechDOS first (GOLDEN-MASTER items
  1 and 2), then FLAIR, then the apps. Working lists, in the reports
  repository: `phase4/dos-exit-list.md` (what is left for items 1 and 2,
  ordered in 1-2 h pieces) and `phase4/c-activation-plan.md` (moving the
  working system onto C:, numbered steps).
- Velocity (operator, 2026-11-21): "velocity is what matters: as long as
  we can (a) find bugs quickly and loudly, and (b) fix forward or revert
  easily, we are good." Reading: a red that is a product fault (test,
  image, smoke, reproducible build, self-host) is fixed forward or
  reverted before new work; a red that is a fault in the checks
  themselves (stale patch, runner, harness) is filed and fixed beside
  feature work, never ahead of it. One issue per commit where possible.
  The certificate is an end-of-day detector, not a gate.
- Release 2 (operator, 2026-11-21, 139bd459): the remaining DOS 3.30
  utilities are on the Release 2 list; BACKUP, RESTORE and the 16-bit
  tools are not planned. GOLDEN-MASTER item 2 is unchanged.
- Bochs: this Bochs build exposes no 486 CPU model (evidence:
  `phase4/c-startup.md`, bochs-cpu-models.log); Bochs runs the harness's
  Pentium model at 8 MiB. Under the 2026-11-15 ruling, GOLDEN-MASTER
  item 1 is now due its rewording (initech-k1vdw; the operator's file).
- Earlier rulings (2026-11-13/14/15) stand as recorded in the phase 3
  STATUS (git show 270371b3:STATUS.md): FAT12 read-write, FAT16
  read-only; the interface crossings and two temporary exceptions; agents
  land their own work, review after landing; InitechDOS COPY follows DOS
  3.30; delivery is a ready-made disk image that starts; the frame
  readings in goldens/frame/README.md; the FAT12/FCB legacy is the joke
  and is never modernised.

## Landed today (2026-11-21)

Product and features:
- 40bcbf34 ParamStr(0): each DOS program's environment carries its
  qualified path, nested EXEC included (initech-311t2).
- 52c86c80 CLOSE frees standard handles 0-4 as MS-DOS 3.30 does (kyrcw).
- 5b586c07 FIND operands, quotes and status match DOS 3.30 (p00t).
- 1bad835f FDISK protects the physical disk the running map names (5o7p3,
  open until its native C: check is recorded on the landed C: startup).
- dac9dc21 system-owned C: boot files and capacity check (8wcp6).
- 964fa280 the kernel mounts and binds C: as the system, alone or with A:
  (gl8tj; plan step 5). The default image still boots the A: system.
- 223c8028 `make run`/`run-bochs` keep the user's disks in user-disks/
  (the data-loss part of tlhqy; tlhqy stays open for delivery).
- 04d39f15 FILE COPY in the still's white and blue (1swiy); 075c1244 the
  pie in the still's slice blues (h60sc); 658a9e28 the ledger note below
  the total (tunh0). ixsv4 verified and closed.
Checks: qw018, crh8p, r6xvy (+ruu4t), 93fuy, s5gxc, ee1g5, qyn5y, 12vr4,
xgzjh, gldev, e4q9a, and the InitechBase sweep (4sqj2, 6ld9x, 98eaw, 07r1b,
7jahr, f84lh). Full mutation sweeps: kernel 312, dos 219, flair 229,
InitechBase 315, factory 38, InitechText 18, all killed. Two post-landing
reviews (Muse) found 5 P2, no P1.

## What a user can do now, by golden-master item

1. Boots to InitechDOS and FLAIR on QEMU (486) and Bochs (Pentium) at
   8 MiB from the A: system; the kernel can now also start from a staged
   C: disk (not yet the image a user boots). Same bytes from two clean
   builds.
2. InitechDOS: as in phase 3, plus CLOSE on standard handles, FIND's
   named operands and quoting, FDISK's running-disk protection, and
   ParamStr(0). What is left: `phase4/dos-exit-list.md`.
3. The frame: the ledger, 116% pie in the still's blues, FILE COPY in
   the still's colours and geometry, the hourglass; graded on QEMU.
   Bochs frame pixels wait for initech-6l2tk, then initech-nmib9.
4. Desktop and small apps: unchanged since phase 3.
5. InitechBase: unchanged in product; its mutation sweep is clean.
   Booted forms replay (stage 2) not started.
6. Initech 123 and InitechWord: unchanged.
7. Turbo Initech: ParamStr(0) now works in compiled programs; the IDE's
   stage E (initech-wkp2p) is next for it.
8. Delivery: the user's disks survive restarts. The C: move continues
   with plan step 6 (factory adopts the C: boot files).

## Parked branches (on origin; do not delete)

- `find-more-park` 0bb3c1c6: the MORE cue (initech-07yn), done but not
  landed; land first (handoff `phase4/find-more.md`).
- `fat12-growth-park` 51422eca: initech-9wx9 work in progress (handoff
  `phase4/9wx9-eico5.md`).
- From phase 3, still unlanded: `delivery-wip` 474b3df2,
  `delivery-c-startup-validated` c7893651, `delivery-c-startup` 0edbefe8
  (ported today as 964fa280), `delivery-system-prep` 323306b5,
  `desktop-2` 5ec5738c, `desktop-2-filecopy-colours` 82bc19ae (superseded
  by 04d39f15), `dos-4` e9893cd9, `factory-2-f2-snapshots` 705d75a7.
  `ee1g5-patch-check` is landed (6d758491) and can go.
- The laptop's stashes: `laptop-stash/00..09-*`.

## Notes for this week

- Emulator harness output paths must be short (QMP socket path cap of 108
  characters); clean worktrees go under ~/w/.
- OpenAI's content filter stopped Codex children twice on InitechBase
  mutation-testing issues; give those to Claude or Muse, or quote the facts
  into the brief (`phase4/PROGRESS.md`, "Lesson").
- Only one emulator smoke on the machine at a time keeps the session
  deadlines honest.
