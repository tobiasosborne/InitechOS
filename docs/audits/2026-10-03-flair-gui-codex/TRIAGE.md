<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# Triage of the 2026-10-03 FLAIR desktop audit

REPORT.md in this directory is the unedited output of an external reviewer
(Codex gpt-6.1-sol, xhigh) that booted a copy of
`build/flair_tenants_interactive.img` + `build/flair_data.img` built from
commit `0194733` and drove it through QMP for about 13 minutes. BRIEF.md is
the brief it was given, `drive.py` its replay harness, `shots/` and
`evidence/` its raw evidence. Findings F01 and F02 were confirmed by the
programme manager from the screenshots (shots 13/17 and 50/55). The rest of
this file is the triage: where each finding is tracked, the root causes read
from the source, and corrections to the report.

## Tracker mapping (label `gui-audit-2026-10`, epic initech-tdnl)

| Finding | Bead | Priority |
|---|---|---|
| F01 window contents do not follow the window | initech-tdnl.34 | P0 |
| F02 scroll bar drags the window, nothing scrolls | initech-tdnl.35 | P0 |
| F03 Get Info / Duplicate / Select All enabled no-ops | initech-tdnl.36 (+ initech-p6st) | P1 |
| F04 Trash cannot be opened | initech-tdnl.39 | P1 |
| F04 drag to Trash / Empty Trash | initech-34dh, initech-6k12 | P1 |
| F05 Restart / Shut Down no-ops | initech-tdnl.37 | P1 |
| F06 foreground app and active window disagree | initech-tdnl.40 | P1 |
| F07 opening a document is silent | initech-tdnl.38 | P1 |
| F08 group drag moves one icon | initech-tdnl.41 | P2 |
| F09 icon placement not remembered | initech-tdnl.42 | P2 |
| F10 window size not remembered | initech-tdnl.43 | P2 |
| F11 no naming / rename | initech-p6st | P1 |
| F12 views; File menu; Edit menu | initech-tdnl.44, .45, .46 | P2 |
| F13 Apple menu; Help | initech-tdnl.47, .48 | P2 |
| F14 application switcher | initech-tdnl.49 | P2 |
| F15 upper-bar About / Quit placeholders | initech-tdnl.50 | P1 |
| F16 body frame; widget shading; icon art | initech-tdnl.51, .52, .53 | P2 |
| F16 typography | initech-tdnl.33 (Font Manager) | P1 |
| F17 item count / free space header | initech-tdnl.54 | P2 |
| F18 contextual menus | initech-tdnl.55 | P2 |

## Root causes (read from the source at 1cf74a6 by the triage lane)

- F01: `finder_win_populate` / `fw_snap` (os/flair/finder_windows.c) store
  every icon position in ABSOLUTE screen coordinates when the folder opens;
  the drag, zoom and grow paths in os/milton/kmain.c only change the
  WindowRecord, so `finder_win_paint` fills the new content region and draws
  the icons at the old coordinates. Finder disk windows only.
- F02: the scroll-bar column is inside the structure region and outside the
  content region; `FindWindow` (os/flair/window.c) treats every other chrome
  pixel as the drag region. No scroll state or control tracking exists for
  any document window; the bars are drawn chrome only.
- F03 / F05 / F07: `finder_dispatch` (os/flair/finder_cmd.c) returns after
  the shell hook, whose default arm returns silently; the FINDER-NYI fallback
  its comment refers to is never reached.
- F06: nothing demotes the Finder when its last window closes
  (os/milton/kmain.c), so the Finder stays the foreground app with its bar
  while NOTES is drawn as the active window.

## Corrections to REPORT.md

- F06 is mis-described: in shots 66 and 68 the lower bar is the Finder bar
  (`File Edit View Special Help`), and (196,28) is on the Special title, not
  on a blank area. The defect is the one stated above.
- F09 and F10 are documented deferrals in the source, not regressions; F10
  needs a change to the locked 24-byte desktop-database record (Rule 8).
- F12 "Find" is deliberately disabled ("inert V1 decoration"): absent, not
  fake.
- The audited image predates commits 1d654c2 (SETMBAR) and d365f77 (x87).
