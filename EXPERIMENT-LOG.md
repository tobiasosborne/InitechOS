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
