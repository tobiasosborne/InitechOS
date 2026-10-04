<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# Triage of the third FLAIR desktop audit pass (2026-10-04)

REPORT.md is the unedited output of the external reviewer (Codex gpt-6.1-sol,
xhigh) on images built from commit 52ede68: 11 QEMU sessions, 02:12-02:35.
It was asked to try to break six developer claims and then keep hunting.
None of the findings below has been independently reproduced yet.

Verdicts on the claims: released modifiers (initech-tdnl.58) HOLDS; close /
zoom / collapse tracking (tdnl.60) HOLDS; a disk app's own menu bar HOLDS;
held gestures (tdnl.59) PARTLY (no timeout up to 32 s, but see H01);
drag-and-drop (initech-34dh) PARTLY (moves commit; H02-H05); proportional
Chicago (tdnl.33) PARTLY (menus and titles confirmed, no button reached;
the reviewer notes real Mac OS 8.1 is Charcoal -- initech-tdnl.69).

| Finding | Bead | Priority |
|---|---|---|
| H01 long cross-menu gesture dispatches the wrong command | initech-tdnl.71 | P1 |
| H02 refused moves give no explanation | initech-tdnl.72 | P2 |
| H03 TRASH directory can be moved; Trash breaks and splits | initech-tdnl.73 | P1 |
| H04 Trash icon never shows it is full | initech-tdnl.74 | P2 |
| H05 DESKTOP.DB can be trashed and is silently regenerated | initech-tdnl.75 | P2 |
| H06 disk app paint overdraws the scroll gutter after Zoom | initech-tdnl.76 | P2 |
| H07 wide 8.3 labels overlap in the default grid | initech-tdnl.77 | P2 |

Look: the reviewer judges the drag outline (a thin rectangle around the whole
icon cell) rudimentary next to the period description of an outline of the
icon and its name; noted on initech-tdnl.53 (icon art).
