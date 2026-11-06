# EXPERIMENT initech-7cbu8 -- dBASE one-shot (relaxed rules), branch exp/dbase-oneshot

START: 2026-11-06 17:15:03 +01:00 (first tool call). Orchestrator: Claude Opus 5.5.
Base: main ca0c4f65.

## Baseline (before any change)
`make -j16 factory samir-exe image` + `make -j16 test-merge-samir`: 2m14.7s wall
for build+gate together; final line `>>> test-merge-samir: ALL GREEN`.

## Setup decisions (17:15-17:20)
- New code goes into a SEPARATE binary, build/SAMIRX.EXE (`make samirx-exe`,
  -DSAMIR_W4, all os/samir/ui/*.c linked resident). SAMIR.EXE and all 410
  gates are byte-for-byte untouched by construction (only #ifdef SAMIR_W4
  lines in samir_main.c). SAMIRX_EXE=build/SAMIRX_<name>.EXE gives each coder
  a private build dir.
- os/samir/include/samir/kit.h: kit contract written by the orchestrator
  before coders started; os/samir/ui/ui.c: one cmd hook (after the work-area
  hook, before query/mutate) dispatching BROWSE / EDIT / CHANGE / bare APPEND
  / bare INSERT / ASSIST / HELP / CREATE <f> / MODIFY STRUCTURE / MODIFY
  COMMAND|FILE / CREATE|MODIFY REPORT|LABEL.
- harness/diff/dbf_diff/xbase_prog_diff/oneshot/run.sh <app> [EXE]: boots
  tracer_secondary.img in QEMU with SAMIRX as SAMIR.EXE on the data disk,
  replays oneshot/<app>/KEYS, dumps PPM + decoded text, extracts the disk,
  runs oneshot/<app>/CHECK. Smoke-proven (oneshot/smoke: USE/LIST).
- 17:21 six codex gpt-6.1-sol xhigh coders launched, disjoint files:
  KIT (ui/kit.c), BROWSE (ui/browse.c), EDIT (ui/edit.c: EDIT/CHANGE/APPEND/
  INSERT), CREATE (ui/create.c + ui/modcmd.c), FRM (ui/frmdesign.c +
  ui/lbldesign.c), ASSIST (ui/assist.c + ui/help.c). Hard stop 17:58.
- Scope cut up front: W3 forms slices 2-4, W2 SET/batch fan-outs and the 94
  DRAFT messages are NOT attempted -- they edit shipped engine files
  (set.c/flow.c/msgs) and would put the oracle-pinned floor at risk inside an
  hour with six concurrent writers; the full-screen apps are the visible part.

## Checkpoint 1 (2026-11-06 17:38-17:40, T+23 min)
`make -j16 test-merge-samir` (2m07.8s): `>>> test-merge-samir: 410 gates ...` /
`>>> test-merge-samir: ALL GREEN`. Branch pushed (origin/exp/dbase-oneshot).
Six coders mid-flight; all ui/*.c compile. BLOCKER hit at 17:34: the full UI
layer (~54 KB at -Oz) does not fit the conventional program window
(PROGRAM_IMAGE 0x40100 .. ENV_BLOCK 0x6F000, 188 KiB, minus the loader's 64 KiB
BSS reserve and the 33 KiB AH=48h PAL heap). Fix (relaxed, SAMIRX only):
(1) pal_milton.c under SAMIR_W4 skips AH=48h and puts the PAL heap at flat
8 MiB (QEMU default RAM 128 MiB; int21.c user_buf_ok does no range check);
(2) os/samir/boot/samirx.ld drops the PAL-memory assert (keeps the window
assert), minalloc 1; (3) SAMIRX is now built PER VARIANT, `SAMIRX_UI='<apps>'`
-> -DUI_HAVE_<app>, unlinked verbs fall through: A=browse edit (106.5 KB
resident), B=create modcmd (102.1 KB), C=frmdesign lbldesign (106.5 KB),
D=assist help (105.8 KB); ceiling ~126.7 KB. High-heap SAMIRX smoke leg GREEN
in QEMU. A real loader-above-1MB (ADR-0009 DEC-03a) is what one integrated
binary needs; not attempted in the hour.

## Checkpoint 2 (2026-11-06 17:51-17:54, T+39 min)
`make -j16 test-merge-samir` (2m01.8s), run WITH the kernel keyboard change:
`>>> test-merge-samir: 410 gates ...` / `>>> test-merge-samir: ALL GREEN`.
All six coders finished between 17:43 and 17:53 and every one reported its
own QEMU legs GREEN (ASSIST/HELP 17:43, BROWSE 17:47, FRM/LBL 17:48, KIT 17:49,
CREATE 17:50, EDIT 17:53); each was committed as it landed. Second blocker,
reported by four coders: os/milton/kbd.c only delivered F1-F5/Ins/Del/Left as
00,scan, so the physical Up/Down/Right/PgUp/PgDn/Home/End/Ctrl-End keys never
reached a program. Fixed at 17:51 in kbd_scancode_put (F1-F10 + the whole nav
cluster + IBM Ctrl scans); test-kbd-unit 490/0 green; new leg
oneshot/browse/arrows proves physical kdown/kright move BROWSE. Coder KEYS
still use the III+ Ctrl equivalents (Ctrl-X/E/D/S/W), which are authentic.
Dispatcher: MODIFY FILE now routed to ui_modify_file (CREATE coder's request).

## Integration (17:54-17:55)
`sh harness/diff/dbf_diff/xbase_prog_diff/oneshot/all.sh` rebuilds the four
variants from the integrated tree and replays all 16 legs in QEMU (42 s):
16/16 GREEN (append assist browse browse/arrows browse/edges browse/indexed
create create/memo create/modify edit help kit label modcmd modcmd/edit
report). 49 screendumps. New sources ASCII-clean (grep -P '[^\x00-\x7F]': none).

# FINAL REPORT

START: 2026-11-06 17:15:03 +01:00.  END: see the last line of this section
(written ~17:58; coders all finished by 17:53, so nothing was cancelled).

## WHAT SHIPPED (relaxed bar: locked key trace + QEMU screendump + data CHECK)
Build any variant: `make samirx-exe SAMIRX_UI='<apps>' SAMIRX_EXE=build/SAMIRX_<n>.EXE`
(A='browse edit', B='create modcmd', C='frmdesign lbldesign', D='assist help');
run one leg: `sh harness/diff/dbf_diff/xbase_prog_diff/oneshot/run.sh <leg> build/SAMIRX_<n>.EXE`;
everything: `sh harness/diff/dbf_diff/xbase_prog_diff/oneshot/all.sh`.
Dumps land in build/oneshot/<leg>/<name>.ppm (+ .txt decoded text view).

- W4.1 kit (ui/kit.c, kit.h): DONE. Shadow 80x25 char+attr buffer, status
  bar row 22, message rows 23-24, decoded keys (DOS 00,scan + III+ WordStar
  Ctrl keys), field editor C/N/D/L with PICTURE 9 # A X ! Y L and @! @A @R @S,
  menu bar + boxed pulldowns, pick list, prompt, save/restore.
  Demo: run.sh kit build/SAMIRX_A.EXE -> build/oneshot/kit/kit-menu.ppm, kit-field.ppm.
- BROWSE: DONE (partial options). Menu line, table, status, edit in place,
  Ctrl-U delete/recall, append prompt past EOF, NDX maintained; FIELDS LOCK
  FREEZE NOFOLLOW NOMENU WIDTH NOAPPEND parsed. Missing: Find action, memo,
  SET CARRY/CONFIRM/MENU/FIELDS. Demo: run.sh browse build/SAMIRX_A.EXE ->
  build/oneshot/browse/browse1.ppm; physical arrows: browse/arrows/arrows.ppm;
  NDX: browse/indexed/indexed.ppm.
- EDIT/CHANGE/APPEND/INSERT: DONE (partial). Default one-field-per-line screen,
  scopes/FIELDS/record#, PgUp/PgDn records, Ctrl-U, Ctrl-W save, Esc abandon,
  date/logical validation. Missing: FOR/WHILE, .FMT layouts, memo editor,
  SET CARRY/CONFIRM; INSERT refuses indexed/memo tables. Demo: run.sh append
  build/SAMIRX_A.EXE -> build/oneshot/append/append1.ppm, after.ppm (4th record).
- ASSIST: DONE (core paths). 8-pad menu bar + clock, all pulldowns, action
  line, status, nav + explanation rows; Set Up/Database file -> USE, Position/
  Goto, Retrieve/List, Update/Browse (when linked) execute via samir_do.
  Missing: directory pick list (filename prompt instead), many operand dialogs
  simplified. Demo: run.sh assist build/SAMIRX_D.EXE -> build/oneshot/assist/menu1.ppm, list.ppm.
- CREATE / MODIFY STRUCTURE: DONE. Two-pane structure screen, bytes remaining,
  validation, .DBF (0x03/0x83) + empty .DBT header, .BAK + data migration by
  field name. Missing: field insert/reorder, cross-type conversion. Demo:
  run.sh create build/SAMIRX_B.EXE -> build/oneshot/create/create1.ppm (WIDGETS.DBF).
- MODIFY COMMAND / MODIFY FILE: DONE (basic editor). Insert/overwrite, line
  split/join, Ctrl-Y, CRLF + 0x1A save, .BAK, 5000-byte cap. Missing: find,
  reformat, block ops. Demo: run.sh modcmd build/SAMIRX_B.EXE -> build/oneshot/modcmd/edit1.ppm.
- CREATE/MODIFY REPORT designer: DONE. Options/Groups/Columns/Locate/Exit,
  1990-byte .FRM; same-session REPORT FORM renders it; real TP28.FRM round-
  trips byte-identical through MODIFY REPORT + Save. Demo: run.sh report
  build/SAMIRX_C.EXE -> build/oneshot/report/design1.ppm, after.ppm.
- CREATE/MODIFY LABEL designer: DONE. 1034-byte .LBL, sizes, 5 lines; real
  ADDRESS.LBL round-trips byte-identical; LABEL FORM renders it. Demo: run.sh
  label build/SAMIRX_C.EXE -> build/oneshot/label/design1.ppm.
- HELP: DONE (authored content). Main menu + 15 authored topic screens
  (USE LIST DISPLAY APPEND EDIT BROWSE REPLACE DELETE PACK INDEX SEEK GO SKIP
  REPORT ASSIST), PgUp/PgDn, unknown-topic message. NOT the real HELP.DBS text.
  Demo: run.sh help build/SAMIRX_D.EXE -> build/oneshot/help/help1.ppm.
- CREATE QUERY / SCREEN / VIEW: NOT STARTED (dispatcher falls through).
- CONFIG.DB: NOT STARTED.
- W3 forms slices 2-4 (@GET/READ/PICTURE/RANGE/CLEAR GETS): NOT STARTED as
  engine work (the kit's field editor implements the PICTURE subset for the apps only).
- W2 fan-outs (2e6lp / vq9oc / qugk9) and the 94 DRAFT messages: NOT STARTED
  (deliberate: they edit shipped engine files under the oracle-pinned floor).
- Kernel: os/milton/kbd.c now delivers F1-F10 + the navigation cluster +
  Ctrl-Home/End/PgUp/PgDn/Left/Right as 00,scan (was F1-F5/Ins/Del/Left).

## FLOOR
- Baseline 17:15: test-merge-samir ALL GREEN (build+gate 2m14.7s).
- Checkpoint 1 17:38: `>>> test-merge-samir: ALL GREEN` (410 gates, 2m07.8s).
- Checkpoint 2 17:51: `>>> test-merge-samir: ALL GREEN` (410 gates, 2m01.8s),
  with the kbd.c change; test-kbd-unit 490 checks / 0 failures.
- Checkpoint 3 17:55-17:57 (final tree, T+42 min, run early because every
  coder had finished): `>>> test-merge-samir: ALL GREEN` (410 gates, 1m25.5s).
- Data diffs (inside each leg's CHECK): every written .DBF read back by the
  independent harness/diff/dbf_diff/dbf_ref.py (APPEND/EDIT/BROWSE/CREATE/
  CREATE memo/MODIFY STRUCTURE); browse/indexed also checks the .NDX with
  ndx_ref.py; .FRM/.LBL decoded by independent python decoders written from
  frm.md / lbl.md (no prior .FRM/.LBL diff tool existed) and checked against
  real goldens byte-for-byte on MODIFY round-trip. No spec/, golden, capture,
  manifest or fixture file changed (git diff ca0c4f65 -- spec harness/diff/dbf_diff/xbase_prog_diff/{golden,corpus,*.sha256}: empty).

## CODERS
Six codex gpt-6.1-sol (reasoningEffort xhigh), shared worktree, disjoint files,
launched 17:21, all finished 17:43-17:53 (none cancelled):
KIT (ui/kit.c + host ASan suite), BROWSE (ui/browse.c, 3 legs), EDIT
(ui/edit.c, 2 legs), CREATE (ui/create.c + ui/modcmd.c, 5 legs), FRM
(ui/frmdesign.c + ui/lbldesign.c, 2 legs + python decoders), ASSIST (ui/assist.c
+ ui/help.c, 2 legs). Rejected outputs: none wholesale. Two requests acted on
by the orchestrator: MODIFY FILE routing (ui.c) and the kernel nav-key fix.
Orchestrator-owned: kit.h contract, ui.c dispatcher, SAMIRX build + samirx.ld,
high PAL heap, oneshot run.sh/all.sh, kbd.c.

## WHERE CORNERS WERE CUT (what a byte-exact oracle would probably catch)
- No screen is graded against a real III+ capture. Layouts are from the spec
  prose/manual; column positions, separators, attributes, status-bar fields
  ("Rec: 3/2" shown on the CREATE screen where III+ shows a field counter),
  "Ovr/Ins" placement, message wording and the ASSIST clock all likely differ
  cell-for-cell. Box-drawing glyph choice unverified.
- HELP text is authored, not HELP.DBS.
- Key traces mostly use the III+ Ctrl equivalents; physical arrows proven only
  for BROWSE.
- Error ordinals for full-screen edge cases (no database in use, invalid date
  prompts) follow the engine's existing catalog where coders found one;
  not differentially checked.
- No mutation proofs for any new leg; CHECKs could be weaker than they look.
- The kbd.c change maps keypad keys to navigation unconditionally (no NumLock
  model) and was proven only by test-kbd-unit + test-merge-samir + one leg,
  not by the FLAIR/desktop/tri-emulator gates (full `make test` not run, per rules).
- SAMIRX's PAL heap at a fixed flat 8 MiB is a hack: no allocator, no
  INITECH_MIN_EXT_KB check, assumes nothing else uses that RAM.
- .DBF headers on CREATE are emitted by ui/create.c (dbf_create is overlay-only)
  rather than the engine codec.

## BLOCKERS HIT
1. SIZE/MEMORY (17:34): the UI layer (~54 KB at -Oz) cannot fit with the
   ~88 KB resident engine in the 188 KiB conventional program window minus the
   64 KiB BSS reserve and the 33 KiB AH=48h heap. Worked around with the high
   heap + per-variant binaries; ONE binary with every app needs the real
   ADR-0009 DEC-03a loader (image + heap above 1 MB) -- not done.
2. PAL/KERNEL KEYS: physical navigation keys never reached programs (kbd.c);
   fixed at 17:51.
3. Overlay-only engine entry points (dbf_create) are unreachable from
   resident UI code under the overlay NOCROSSREFS rules; CREATE re-emits headers.

## ESTIMATE TO FINISH THE REST AT THIS (RELAXED) BAR
~6-8 hours of wall clock with the same 6-coder pattern: 1-2 h for the
above-1MB loader so one SAMIRX holds every app; ~2 h for CREATE QUERY/SCREEN/
VIEW (.QRY/.SCR/.FMT/.VUE writers) + CONFIG.DB; ~2 h for W3 forms slices 2-4
in the engine; ~1-2 h for the W2 SET/batch fan-outs + DRAFT messages against
their already-minted goldens. At the project's normal bar (byte-exact real-III+
screen captures for every app screen, mutation-proven goldens, tri-emulator,
overlay placement and size caps) multiply by roughly 5-10x.

END: 2026-11-06 17:58 +01:00 (T+43 min). Final commit = branch head of origin/exp/dbase-oneshot
(SHA printed by `git rev-parse origin/exp/dbase-oneshot`; the SHA of the commit
before this log commit is in the next line).
Pre-log head: 09f7a0df531598fa33804c29ac4806821dc14578
