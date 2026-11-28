# Status

Rewritten 2026-11-28 at midday of phase 4 day 3, on Spiral (WSL2, 64
cores, home /home/tobiasosborne; `-j16` is the normal figure). Setup:
`docs/SETUP.md`. Reports and handoffs live in the private repository
cloned at `/home/tobiasosborne/Projects/InitechOS-reports/`; today's
running log is `phase4/PROGRESS.md` there and the next session's start
is `phase4/BRIEFING.md`.

## Goal this week

The operator, 2026-11-28: "our goal is to be laser focussed on landing
features for dos to have this base layer ship ready." InitechDOS
(GOLDEN-MASTER items 1 and 2) and nothing else. InitechDOS is a release
candidate when no tier A issue is open and one strict certificate is
green on that head.

## Tiers (operator, 2026-11-28)

Every finding is sorted before anyone works it.
- A, must be right for the release candidate: data loss, hang, crash,
  a missing GOLDEN-MASTER item 1-2 feature, something accepted
  silently, a bundled program depends on it, boot or DOS on staged C:.
- B, after A: an ordinary user at the prompt or in an ordinary batch
  file sees wrong output.
- C, a nit, not worked for this master: tracker issues carry the label
  `nit` and are deferred, not closed (31 deferred on 2026-11-28).
Reviews: at most three rounds on any piece; aim for one.

## Owners today

- dos/ has two owners with disjoint files: A owns the shell
  (`dos/command/` and its tests); B owns the utilities and disk tools
  (everything else in `dos/`). Both add their own lines to
  `dos/Makefile`, `dos/tests.mk`, `dos/tests-programs.mk`.
- kernel/: one owner (finished its list at midday).
- factory/ owns the shared places (`mk/`, `sdk/`, `factory/`, the top
  Makefile).
- system/ and boot/: one owner each (boot finished its list).

## Landing

As CLAUDE.md "Landing a change", under one machine-wide lock:
`flock ~/w/landing.lock sh -c 'git fetch origin && git rebase
origin/main && make -j16 test && make smoke && git push origin
HEAD:main'`, stacks of one to four commits. Emulators may run side by
side (the old `~/w/emu.lock` rule is gone). Top `make smoke` runs its
legs in parallel since 522a6186 (910 s to 245 s); the mutation sweep
and certificate stages run in parallel since 61b83be5. Tier-0 budgets
charge CPU time since 542a38ef, so machine load no longer fails them.

## Landed on 2026-11-28 (by midday)

- dos shell: SHIFT beyond %9 (tyvt); batch ends at Ctrl-Z; repeated
  output redirection refused; DEL with extra operands refused;
  unsupported built-in switches refused; binary COPY without a target
  keeps its bytes (dmhdo); primary and secondary COMMAND and the batch
  and pipe sessions run from installed C: (14a, 14d part).
- dos utilities: MORE reads keys from CON (5hq49); SYS and FORMAT /S
  transfer the whole system set (hdyz); FORMAT refuses conflicting
  geometry; EDLIN keeps read-only files and backups; FDISK explains its
  running-disk refusal; release-blocking switches refused (esp2z part);
  FAT16, ATA and utility sessions from installed C: (14b-14d part).
- kernel: the high word of the seek offset (a 64 KiB data loss);
  undersized FATs refused and directory read faults reported (bcg.14);
  CONFIG.SYS startup rules from the DOS 3.30 audit; redirected
  character input for DATE, PAUSE and the AH=01-0C family (nt01q);
  IOCTL device word (eb0n1); AH=2Eh/54h verify with read-back (9u4r4);
  MZ relocation bounds published (9dbgc).
- boot: installed C: master in the boot suite (plan step 16); BIOS
  pitch, 24-bit VBE and VGA13 legs (ta2).
- system: Turbo Initech example and directions on installed C: (19b).
- factory: parallel smoke; Bochs punctuation (md1oz) and Escape;
  per-smoke FAT16 images; CPU-time budgets; parallel mutation sweep.

## Audits on 2026-11-28 (one pass each, in the reports repository)

`phase4/audit-utilities.md`, `audit-command.md`, `audit-config.md`,
`audit-int21.md`, `audit-hands.md`: ranked differences from real DOS
3.30, each with a capture. Their tier A and B rows are on the owners'
lists; the rest is filed or dropped as tier C.

## Parked branches

- `initech-xqz3o-park` (19a): being landed by the system owner.
- `initech-k1fdx-park` (15a): being landed by the factory owner.
- `initech-fvicp-park`, `initech-iy2bi-park`: left parked (desktop;
  tier C check).
- `kernel-audit-day3-park`: landed (the Codex kernel child stopped on
  OpenAI's content filter; Claude Opus finished it).
- Landed and can go: `kernel-stack-park`, `desktop-10b-*`,
  `desktop-stack-prerebase-park`, `find-more-park`, `fat12-growth-park`,
  `initech-tyvt-park`, `initech-md1oz-park`, `initech-8xhks-park`,
  `initech-ta2-park`, `nt01q-park`.

## Direction and rulings

- InitechDOS first (items 1 and 2), then FLAIR, then the apps. Not on
  the DOS list: plan steps 11-13, 17-18 and 20-22 (switching the
  default image to C: waits for FLAIR's turn); initech-loy1q (queued
  behind the C: switch by the operator).
- InitechDOS COPY follows DOS 3.30; real DOS 3.30 captures are the
  oracle for every behaviour; COMMAND /C returns 0 as DOS 3.30 does.
- Bochs runs its Pentium model (no 486 in this build); GOLDEN-MASTER
  item 1 says so since d4bcac8f.
- Earlier rulings stand as recorded in git show 270371b3:STATUS.md:
  FAT12 read-write, FAT16 read-only; the interface crossings and two
  temporary exceptions; agents land their own work, review after
  landing; delivery is a ready-made disk image; the frame readings;
  the FAT12/FCB legacy is the joke and is never modernised.

## Notes for this week

- OpenAI's content filter stops Codex children on kernel disk-write
  refusal and startup work; give those to Claude Opus.
- Emulator harness output paths must be short (QMP socket path cap of
  108 characters); clean worktrees go under ~/w/.
- Known tier B gap: FLAIR halts in the VGA13 fallback without VBE
  (initech-hwks7, for FLAIR's turn; default profiles have VBE).
- No certificate has run since 964fa280; one strict run before today's
  stop.
