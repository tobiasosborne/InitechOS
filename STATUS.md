# Status

Rewritten 2026-11-15 at the close of phase 3, stopped on the operator's
deadline. Setup: `docs/SETUP.md`. Reports and handoffs cited below live in
`/home/tobias/Projects/InitechOS-reports/` (`phase2/...`, `phase3/...`);
the orchestrator's running log is `phase3/PROGRESS.md` there.

## State of main

- The session was stopped on the operator's hard deadline (2026-11-15,
  about 13:00 CET). Every agent stopped; nobody holds a directory.
- NO end-of-day certificate ran on the final head. The last strict
  certificate (phase 2, `make cert CERT_STRICT=1` on 09230b74) was RED at
  its mutation stage (clean-test, image-smoke and repro green; self-host
  unfinished). Much of what made it red has since been repaired (the
  mutation runner's kill contract, Turbo seed/compiler grading, kernel
  MKDIR-rollback survivors), but that is unproven until a certificate
  runs.
- Last clean sample, 0dc2b9e4 (detached clean worktree, 12:29, load 0.6):
  `make -j12 test` RED at once in the factory: console-symbols-check reads
  build/system/system.elf before anything builds it (added in b1b53c30),
  so top-level `make test` fails on every clean checkout. P0
  initech-qw018. The run stopped there; the smoke was cut off by the
  deadline. Earlier clean samples: 387a82b2 (09:47, load 10-18) and
  9611ad9f (07:41, load 6-13) ran `make smoke` GREEN on QEMU and Bochs,
  with `make test` red only on time budgets (plus two real failures at
  9611ad9f, since fixed).
- Self-host: one TURBO.COM holds the IDE and the compiler (9c0cd142);
  K2 == K3 == D2 on QEMU and Bochs at 354a143e, 241,239 bytes (its
  owner's run, not a certificate).
- `CERT_STRICT` stays at its default until a strict run is green.

## First things to do next session

1. Fix initech-qw018 (red first from an empty `build/`), then run
   `make cert CERT_STRICT=1` on a clean worktree of the head, on a quiet
   machine, before any new work; record each stage; fix what is red.
2. Delivery (GM8): stop `make run` reminting the disk images (a user's
   saves on A: are destroyed at the next start; initech-tlhqy, review
   `phase3/xreview-delivery.md`), then land the C: activation from branch
   `delivery-wip` with the Finder's C:/A: contexts (initech-ektw0, branch
   `desktop-2`), Initech 123's C: prompts (initech-z5per) and the smoke
   migration to C:.
3. The frame journey on both emulators (GM3): Drive A's seed in the
   image, the Bochs mid-run snapshots (branch `factory-2-f2-snapshots`),
   FILE COPY colours (branch `desktop-2-filecopy-colours`), slice blue
   (initech-h60sc); then the clip (initech-a54yw).
4. InitechBase forms stage 2, the both-emulator replay (initech-5beoa,
   initech-nwm80); Turbo's runtime requirement (initech-wkp2p, needs the
   kernel's ParamStr(0) trailer initech-311t2).

## Rulings in force

Operator, 2026-11-13/14 (also in CLAUDE.md and GOLDEN-MASTER.md): FAT12
read-write, FAT16 read-only with every write refused before the first
sector changes; the allowed interface crossings and the two temporary
exceptions; agents land their own work, review after landing; the
three-block Initech mark; InitechDOS COPY follows DOS 3.30 (including
removing the target after disk full) while FILE COPY never destroys its
destination; delivery is a ready-made disk image that starts, no
installer; Calc's own menus in the bar during the copy, InitechPaint
separately; unreadable ledger cells are empty ruled cells with "Blank
entries were not recovered from the surviving material."; Turbo keeps a
32-bit Integer; one TURBO.COM; the IDE shows TP's zoomed layout only. The
legacy layers of InitechDOS (FAT12, the FCB calls, the TPS Report
Generator's dependence on them) are the joke and are not modernised.

Operator, 2026-11-15, ratified in full:
- The frame readings at the top of `goldens/frame/README.md` are all
  accepted: 738.00; the still's spellings; the 57 probable cells are data;
  cut-off labels and hidden rows empty; the ledger window keeps the title
  InitechCalc; amounts left-aligned; navy until a mid-blue exists; Drive A
  behind both of InitechCalc's windows; the 46 by 12 ledger grid; the
  still stays out of the repository. One changes the product and is not
  started: the "Blank entries were not recovered" note moves below the
  total (issue filed).
- Capture commits may be pushed to the operator's private sibling corpus
  repositories (the operator pushes them).
- "A 486": try a 486 Bochs configuration; if this Bochs build cannot
  provide one, GOLDEN-MASTER item 1 is reworded to say Bochs is the second
  emulator at its nearest CPU (issue filed, not started).
- The Finder's direct FAT access stays for this master; routing it
  through DOS is Release 2 (initech-02siz deferred).
- One rule for file loss in GOLDEN-MASTER item 4, with InitechDOS COPY as
  the stated exception.
- InitechPaint's seven Photoshop menu titles stay as titles that open
  nothing: a stated exception to "works or absent".

## What a user can do now, by golden-master item

1. **Boots.** Boots to InitechDOS and FLAIR on QEMU and Bochs at 8 MiB.
   The working system still runs from A: (an ATA data volume); C: boots
   only. A C: system disk and an A: data volume are now minted and checked
   beside the product image (fdd696b6), not yet the image a user boots.
2. **InitechDOS.** Since yesterday: SORT and FIND refuse on failed or
   short writes and on unknown switches; XCOPY never destroys an existing
   destination on a read failure, with no file-size limit (62a37bce);
   batch and interactive `%` expansion graded against genuine DOS 3.30
   captures; CHKDSK and FDISK use published OEM INT 21h services; genuine
   DOS 3.30 utility media recovered and recorded (12fb2e57). Every DOS
   test prints a counted, named verdict; 170 host and 49 session mutants
   are semantic kills.
3. **The frame.** InitechCalc shows the transcribed ledger with honest
   gaps and the 116% pie in a gutterless, untitled chart window placed as
   in the still; FILE COPY sits at the still's box; the canon hourglass is
   graded where the still shows it; the booted chart and the FILE COPY
   moment are graded on QEMU against the reference package; the full
   repaint is graded against a picture built without Calc's drawing code;
   Calc paints with a third of the gate calls. Not yet: Drive A's window
   from the Finder seed, Bochs pixels, slice blue, the clip.
4. **Desktop and small apps.** Menu titles spaced as the System 7
   capture (StringWidth+13); FLAIR expectations graded against cited
   references, not the renderer (jeo8c closed); Finder File and Help
   menus resolved; Restart/Shut Down ask the front application and wait
   for its answer; a README-driven dead-command sweep for Note Pad,
   Scrapbook, Calculator, Control Panels, InitechText and InitechPaint
   (engine in sdk/host); Note Pad reads short stores correctly and an
   unreadable store takes no edits; InitechText cannot hang the desktop
   on a narrow window; the shared save helper never deletes a former
   stage and never blocks on leftovers.
5. **InitechBase.** `@ ... SAY ... GET`, `READ`, PICTURE/FUNCTION and the
   forms SET controls work on the host, graded exactly against real
   dBASE III+ 1.1 captures (F01-F22, data-d01..d10, each minted twice under
   real DOS 3.30); READ stores only edited fields, never writes at end of
   file or into another table; numeric entry is III+'s typed template;
   the frozen omission list is in the README. Also fixed: UNIQUE indexes,
   key width and limit, empty-table BOF/EOF, numeric overflow, failed new
   outputs. Not yet: the booted both-emulator replay (stage 2).
6. **Initech 123 and InitechWord.** Word saves and Move go through the
   shared replacement helper; Word's suite is within budget. 123 opens
   its menu on the bound system volume, its suite runs in 6-8 s, and it
   answers the Finder's quit request (wait while asking, accept or
   decline).
7. **Turbo Initech.** One TURBO.COM: the IDE with no arguments, the
   compiler with a file; IDE saves are write-aside then DOS renames (five
   data-loss findings and two follow-ups fixed); the self-host fixed point
   and the diverse bootstrap agree on both emulators at 241,239 bytes.
   Still to do: initech-wkp2p's runtime requirement (ParamStr(0) needs the
   kernel's environment trailer, initech-311t2) and the finale.
8. **Delivery.** Ruled: start from a ready-made image. The move of the
   working system onto C: is staged but not landed (branches below);
   `make run` must stop reminting user disks first. Manuals not started.

## Who held what (phase 3; nobody holds a directory now)

| Area | Last owner (provider) | Handoff |
|---|---|---|
| turbo/ | Turbo owner 2 (Codex) | `phase3/turbo-owner-2.md` |
| kernel/, system/ | delivery owner (Codex) | `phase3/delivery-owner.md` |
| dos/ | DOS owner 4 (Codex) | `phase3/dos-owner-4.md` |
| flair/, flair/desktop/, apps/initechword/ | desktop owner 2 (Codex) | `phase3/desktop-owner-2.md` |
| factory/, mk/, sdk/, Makefile | factory owner 2 (Opus) | `phase3/factory-owner-2.md` |
| apps/initechbase/ | InitechBase owner 4 (Opus) | `phase3/base-owner-4.md` (forms: `base-forms-3.md`) |
| apps/initech123/ | Initech 123 owner (Opus) | `phase3/i123-owner.md` |
| apps/initechcalc/, goldens/frame/ | frame owner 4 (Opus) | `phase3/frame-owner-4.md` |
| small apps | small apps owner 2 (Opus) | `phase3/small-apps-owner-2.md` |
Reviews: `phase3/xreview-*.md` (22 reports). Progress log:
`phase3/PROGRESS.md`.

## Unlanded branches (on origin unless marked; do not delete)

- `delivery-wip` 474b3df2: the C: activation checkpoint (C-only and C+A
  checks passed on both emulators; the desktop smoke was not reliable).
  Also `delivery-c-startup-validated` c7893651 (kernel C: startup),
  `delivery-c-startup` 0edbefe8, `delivery-system-prep` 323306b5 (local
  branch `delivery` is the same commit).
- `desktop-2` 5ec5738c: Toolbox and cursor-observer drafts (C:/A: Finder
  contexts in progress, initech-ektw0).
- `desktop-2-filecopy-colours` 82bc19ae: FILE COPY body and progress
  colours (initech-1swiy), not graded yet.
- `dos-4` e9893cd9: FDISK wording without "diskette" (initech-bctrr);
  gates and smoke not run.
- `factory-2-f2-snapshots` 705d75a7: Bochs lossless mid-journey snapshots
  for the frame (initech-6l2tk); needs a full smoke (local branch
  `factory-2` is the same commit).
- Local only, in this repository: `renew-wip`, `stage2-wip`, `nylrt-wip`
  (factory WIP superseded by landed commits); `turbo-batch-checkpoints`,
  `turbo-compiler-after-review`, `turbo-compiler-wip-saved`,
  `turbo-final-proof-before-rebase`, `turbo-progress-wip` (superseded by
  the landed Turbo batch, per its owner); `kernel-catalogue-pending` and
  `t3/kernel-fat16-write-policy` (phase 2, one unique commit each,
  unreviewed); older `lane/*`, `wip/*`, `archive/*` branches on origin are
  from before phase 2.

## Open P1, by golden-master item

`bd list --status open --priority 1` is the authority. At close:
- GM1: r2lqd (8 MiB boot qualification), tte51 (warm image build can ship
  an old resident desktop), frb3i (mutation hooks to patches), o7eod
  (cert end-of-day).
- GM2: 5o7p3 (FDISK must protect the running disk when C: is primary),
  gl8tj (installed C: COMMAND startup), z3urw (DOS FAT16/ATA checks on the
  C: image), hdyz (SYS and FORMAT /S full system set), loy1q (timestamp
  write after payload commit), p00t (FIND operands), 40211, di8qr.
- GM3: 14itf (the frame), ixsv4 (FILE COPY geometry), ektw0, 6grv (Geneva 9
  against period captures), tdnl.7 (desktop composition checks), tunh0.
- GM4: e1bma (dead commands: the small apps have a sweep; 123, Word and
  Finder need theirs in cert), li7r (123 power journey on both emulators),
  gtzi4.
- GM5: 5beoa (forms stage 2), y29t4 (GM5 acceptance), 51yc, s872, miu6,
  u78k, nbpj, 1x6x2.10, zw6sl, md1oz, ipu5.8.1.
- GM6: z5per (123 C: prompts), li7r.
- GM7: wkp2p, ry6ge (the IDE and finale), 311t2 (ParamStr(0)), cd2yi,
  fl7ce, b8vxr.
- GM8: tlhqy (the image and launchers; make-run data loss), fvicp (Bochs
  volume opener selects twice), ipu5 and the manuals (f8b1, j0x4o, gtzi4).

## Notes for this week

- The machine has 12 cores; above a load of about 16 the time budgets and
  Bochs session limits go red for no product reason. Key injection is now
  paced by guest completion and Bochs deadlines renew on progress (capped
  at 3x), which removed most load-only smoke failures.
- An incremental `make -j image` could boot a system older than its
  sources before e5d913ce (initech-ktd5j); trust only clean samples taken
  before that.
- Suites over budget under load at close: kernel (63-74 s of 60), dos
  (31-35 of 30), turbo/compiler (34 of 30, and its self-protocols test
  hits a 20 s timeout under load).
