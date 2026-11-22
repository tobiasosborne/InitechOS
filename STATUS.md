# Status

Rewritten 2026-11-22 at the close of phase 4 day 2, on Spiral (WSL2, 64
cores, home /home/tobiasosborne; `-j16` is the normal figure). Setup:
`docs/SETUP.md`. Reports and handoffs live in the private repository
cloned at `/home/tobiasosborne/Projects/InitechOS-reports/`
(`phase2/`, `phase3/`, `phase4/`); today's running log is
`phase4/PROGRESS.md` there and the next session's start is
`phase4/BRIEFING.md`.

## State of main

- Head db339b7b at the stop of 2026-11-22 (11:20 CET, the operator's
  early stop). Main is green: every landing ran top `make -j16 test`,
  and full `make smoke` where CLAUDE.md asks. Nobody holds a directory
  or a shared place (`mk/`, `sdk/`, `factory/`, top Makefile).
- No certificate ran on 2026-11-22: the operator stopped all work at
  11:20, before the evening run. The last certificate is the strict run
  on 964fa280 (2026-11-21), every stage green once its status and
  tracked-files stages were completed on 2026-11-22 (evidence
  ~/w/cert-evidence/2026-11-21T171431Z-964fa280). Run a strict
  certificate early next session, detached, limit above 7,500 s.
  initech-o7eod stays open until one run completes every stage.

## Landed on 2026-11-22

- d24f7a4f FAT12 appends allocate fragmented extensions from free space,
  as DOS 3.30 does (9wx9).
- 4e24e3dd MORE shows and clears the DOS 3.30 cue (07yn); 24c790b1 TYPE
  stops at the Ctrl-Z text end (s3okl).
- 8bbede45 low-RAM boot refusals at 5 and 7 MiB qualified on both
  emulators (r2lqd).
- C: move: 9bfb9bd8 factory installs and qualifies the system-owned boot
  files on staged C (plan step 6, n17kx); 43bf4527 the Toolbox publishes
  the file-session drive, INT 81h 0x008C (step 7, 1cbp3); 36a012a1 File >
  Open opens a selected desktop disk (xrnwk, needed by step 2);
  e12732a8, 5c25a063, db339b7b the Finder binds to the installed system
  volume, scopes windows and Desktop DBs per volume, and resolves each
  file operation on its named volume (steps 8-10, ektw0.1-.3).
- Checks: 2bd948ed working-disk persistence in the reproducible-build
  gate (fedln). tte51 requalified green, closed with no change.
- Reviews after landing: 964fa280 (13 findings, 4 P1, all kernel; most
  fixed on kernel-stack-park) and six 11-21 landings (4 check findings).

## Parked branches (on origin; each handoff in the reports repo phase4/)

- `kernel-stack-park` 8d68e608: 13 commits, one per issue (8ktom, cpyxb,
  0kuwb, 2avqc, eico5, om9z6, k2e3x, loqhk, yzxcc, bdfno, e1gio); host
  tests and kernel startup smoke green; needs full `make smoke` and
  landing (kernel-line.md). Lands first: it fixes the P1s and geu6u.
- `nt01q-park` fd359f1e on top of it: redirected stdin, part WIP;
  blocked on initech-5hq49 (MORE must read keys from CON), dos owner.
- `initech-tyvt-park` 5800660d SHIFT beyond %9 (dos-line.md).
- `initech-fvicp-park` 8f1b56f1 (step 2), `initech-md1oz-park` f068114d
  (Bochs punctuation keys), `initech-iy2bi-park` c0c55aad,
  `initech-k1fdx-park` e2f82f5b (step 15a; needs geu6u) (factory-line.md).
- `initech-xqz3o-park` 5ca76bb7 (step 19a; needs md1oz on Bochs),
  `initech-8xhks-park` e507f5dc (19b) (system-line.md).
- `initech-ta2-park` 00ede72e boot pitch piece (boot-line.md).
- `desktop-10b-park` fefd45df DATA A: as a second Finder volume
  (ektw0.4, desktop-line.md); `desktop-stack-prerebase-park` b9267cb7 is
  the pre-rebase copy of the landed stack and can go.
- Older parked branches are as listed in git show e908f7ce:STATUS.md;
  `find-more-park` and `fat12-growth-park` are landed and can go.

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

## What a user can do now, by golden-master item

1. Boots to InitechDOS and FLAIR on QEMU (486) and Bochs (Pentium) at
   8 MiB from the A: system; the kernel can now also start from a staged
   C: disk (not yet the image a user boots). Same bytes from two clean
   builds.
2. InitechDOS: as in phase 3, plus CLOSE on standard handles, FIND's
   named operands and quoting, FDISK's running-disk protection, and
   ParamStr(0); since 11-22 fragmented FAT12 appends, the MORE cue and
   TYPE's Ctrl-Z end. What is left: `phase4/dos-exit-list.md`.
3. The frame: the ledger, 116% pie in the still's blues, FILE COPY in
   the still's colours and geometry, the hourglass; graded on QEMU.
   Bochs frame pixels wait for initech-6l2tk, then initech-nmib9.
4. Desktop and small apps: File > Open opens a selected desktop disk;
   the Finder's windows and Desktop DBs are per volume (C: move work).
5. InitechBase: unchanged in product; its mutation sweep is clean.
   Booted forms replay (stage 2) not started.
6. Initech 123 and InitechWord: unchanged.
7. Turbo Initech: ParamStr(0) now works in compiled programs; the IDE's
   stage E (initech-wkp2p) is next for it.
8. Delivery: the user's disks survive restarts. C: move: plan steps
   1 and 3-10 landed except step 2 (fvicp, parked); next are landing
   the parked branches, then steps 11-13 and 14a. The default image
   still boots the A: system.

## Notes for this week

- Emulator harness output paths must be short (QMP socket path cap of 108
  characters); clean worktrees go under ~/w/.
- OpenAI's content filter stopped Codex children twice on InitechBase
  mutation-testing issues, and twice on 11-22 on the kernel's disk
  write-refusal and startup fallback fixes. Give that kind of work to
  Claude or Muse.
- Landing with several lines at once: one machine-wide lock for the
  landing sequence (`flock ~/w/landing.lock ...`), emulator runs under
  `~/w/emu.lock` (reports `phase4/DAY2-LANDING.md`). With six lines the
  queue held up to six waiters; stack landings where possible.
- Only one emulator smoke on the machine at a time keeps the session
  deadlines honest.
