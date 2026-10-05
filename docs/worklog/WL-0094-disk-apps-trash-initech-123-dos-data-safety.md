<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0094 -- THE FIRST REAL APPLICATION OPENS FROM THE FINDER; THE TRASH WORKS; COPY AND DEL STOP EATING DATA

**Date:** 2026-10-04 evening to 2026-10-05 (orchestrated session; operator
directives in-session, see Context)
**Beads CLOSED:** initech-tdnl.81 (clips + Makefile prerequisites),
initech-tdnl.31 (menu delivery to disk apps), initech-tdnl.39 (Trash window),
initech-6k12 (Empty Trash), initech-9u8w + initech-w96l (Initech 123 P1 +
disk-launch packaging), initech-8zii (tenant draws presented), initech-d9tt
(Bochs port collision), initech-vj28 / wdzq / 8uad / p4h7 (DOS data loss),
initech-gtge.1 (physical-edition research).
**Beads FILED:** epic initech-gtge (physical edition); the fifth audit's 12
(label `audit-2026-10-pass5`); tdnl.87-.91, 68iw.1, 68iw.2, ehz4, wrlb, iocr,
1nsj.
**Certificate:** ALL GREEN 376 host + 173 emu at 7030e2f (first-person,
`make clean && make test` in the clean sibling checkout ../initech-os-cert,
2026-10-05 01:16-01:58). Earlier the same night: ALL GREEN 370 + 154 at
d9fa02e (00:31-01:14).

## Context
Operator, 2026-10-04: (1) happy with the 41 clips recorded at 37334ee;
(2) orchestration rule changed to "work up to 5 points OVER pace on Claude and
Codex" (was: stay under), rest unchanged (Opus codes, Sonnet does busywork,
no Fable subagents, Codex gpt-6.1-sol for computer use, 2-3 agents);
"astra" = Codex model gpt-6-astra, for occasional hard cognition and creative
computer use; (3) no opinion on band-1 Quit (tdnl.50): the PM rules;
(4) the product needs BIG BOXES AND BIG MANUALS (epic initech-gtge);
(5) "move the frontier: bring Initech closer to a fully fledged OS";
(6) when good progress is made, send an astra to drive everything and record
all nits.

## What changed
1. **Makefile parse-order defect (8b1bf6d).** Three prerequisites named a
   variable before its definition and expanded to nothing (the samir-suspend
   data disk, test-boot's banner, kmain_flairtenants_int.o's gate headers).
   New host gate `test-makefile-vars`.
2. **Menu delivery to disk applications (tdnl.31, opus, 54f8ef9 / d659b39).**
   The kernel pushes TBX_EVT_MENU (What=0x51, Message=(menuID<<16)|item) to
   the tenant's event handler for mouse and command-key; 0x0051 MENUSELECT
   retired; DRAWMENUBAR 0x0052; SETMBAR legal after the first window. Grounded
   in Macintosh Toolbox Essentials p.3-72 / 2-44 / 3-78 and ADR-0013.
3. **Trash (tdnl.39 + 6k12, opus, f8d08e5 / d3118a1 / 9f169fa).** The Trash
   opens as a window over \TRASH, drag out = put back; Empty Trash with the
   confirm alert (wording and geometry from s8_alert_modal.png), recursive
   purge, FULL icon (authored). Root-cause fixes on the way: drop onto an
   occupied grid cell, finder_fat_enumerate swallowing the early-stop value,
   dialog hit-testing in local instead of global coordinates.
4. **Initech 123 P1 + P1b (9u8w, w96l, 8zii; opus; 97dcaac / f7122b6 /
   6a53ddc / 751fb2b).** Worksheet core + .WK1 codec (44/44 corpus goldens
   rewrite byte-identical; 41 display rows match the 1-2-3 screens; formulas
   carried byte-exact, not evaluated); the C disk-tenant path `os/apps/tbx/`
   (reusable by InitechWord); 123.EXE opens from the Finder with the classic
   screen. New verb TBX_DRAWCELLS 0x0033; the tenant carve honours e_minalloc.
5. **Bochs one-at-a-time (d9tt, d9fa02e).** bochs_run holds a host-wide flock
   on /tmp/initech-bochs.lock; gates test-bochs-concurrent (+mutant).
6. **DOS data safety (Codex gpt-6.1-sol, 3a29042..b519e84 / 7030e2f).** COPY
   refuses every spelling of the same file, copies INTO a directory; DEL keeps
   the path's directory, drains every match, asks before `DEL *.*`. Grounded
   in the MS-DOS 3.3 and IBM DOS manuals now held locally.
7. **Fifth hands-on audit (Codex gpt-6.1-sol)** on the 8b1bf6d images:
   `docs/audits/2026-10-04-flair-gui-codex-pass5/` (K01-K15, ranked gap list,
   TRIAGE.md with root causes at file:line). First pass over the DOS prompt,
   the in-OS database, the built-in apps and a 15-minute endurance run.
8. **Physical-edition research (opus):** `docs/packaging/` (five documents,
   15 operator decisions in the main report's Sec 9). The original manuals
   (65 files, 1.10 GB) are at /home/tobias/Projects/initech-manuals-ref/,
   outside the public repo; they include the complete dBASE III PLUS set.

## Frictions
- **Lanes in `.claude/worktrees/` cannot see the reference corpora** by the
  `../` defaults, and `dbf_ref.py` / `ndx_ref.py` hard-code the path
  (initech-ehz4): `test-samir` is RED in every such worktree. Sibling clones
  (`../initech-os-<name>`) do not have the problem; the certificate runs in
  `../initech-os-cert` so the main `build/` stays free.
- **Two lanes changed the same data disk.** Initech 123 added two programs to
  APPS; the Trash lane's Empty Trash clip marker (purged=3 -> 5) and the
  Finder-service damaged-volume trace (stranded folder moved from cell 1 to
  cell 3) broke at the merge. The 123 lane had not run test-flair-finder-
  service. Fixed in the merge commit 751fb2b; the trace is locked spec-data
  and was re-keyed deliberately.
- **Certificate 1 died on a false red** (concurrent Bochs, d9tt). Fixed at
  the root; future lane briefs must NOT wrap Bochs gates in flock(1) on the
  same lock file.
- **Codex sandbox:** `.git` is read-only (commits come back as a bundle),
  socket binds are refused (the harness gained opt-in INITECH_QMP_STDIO),
  Bochs cannot run. The PM runs the Bochs legs.
- **Quota:** three Opus agents plus the Fable orchestrator took Claude weekly
  from 3.4 under pace to 1.4 over in two hours; from 00:15 all new work went
  to Codex. Codex was used for coding from then on (a departure from "Opus
  codes", flagged to the operator in-session).

## Acceptance
See Certificate. Kernel binding headroom after everything: read
`make test-kernel-headroom` (about 11 KB; the floor is 4,096 B). Clips: all
RECORD_SCRIPTS (46, five new: tenant_menu, trash_window, empty_trash,
ctenant_keys, i123_typing) re-recorded on main at 7030e2f; the operator's
Law-4 eyeball of the five new ones is owed.

## Pointers -- NEXT AGENT START HERE
1. **In flight at the time of writing (Codex, in sibling clones):**
   `../initech-os-samirfix` (audit K05/K06/K09/K13: hw4j, 3t55, nds1, xrbj),
   `../initech-os-fileverbs` (tdnl.91: Toolbox Gate file verbs + Initech 123
   /File Retrieve and /File Save), and a gpt-6-astra full-system drive whose
   report lands in `docs/audits/2026-10-05-astra-full-drive/`. Check the
   HANDOFF for whether they merged.
2. **Then:** the rest of the fifth audit (j1lu Ctrl-C/Ctrl-Z, oqiu DIR
   operands, 68lh/jzhh wildcard REN/COPY, anf6 HELLO/NOTES dead commands,
   u78k the missing database commands), tdnl.78 Restart / Shut Down, the
   pass-4 scroll findings tdnl.82-.86, tdnl.87/.88 (alert icon, buttons act
   on release), I123 P2 (94ah, formulas), IWORD P1 (l4tj; the C tenant path
   now exists).
3. **Operator decisions waiting:** docs/packaging Sec 9 (D-1..D-15).
