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
