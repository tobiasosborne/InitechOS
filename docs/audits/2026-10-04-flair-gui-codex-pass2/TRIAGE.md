<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# Triage of the second FLAIR desktop audit pass (2026-10-03/04)

REPORT.md is the unedited output of the external reviewer (Codex gpt-6.1-sol,
xhigh), second pass, on images built from commit `1cf74a6` (before the
drag-move merge 30fad28). 18 QEMU sessions, 27 minutes of guest testing, one
240-cycle launch/close soak with no panic and no heap drift. The programme
manager spot-checked G11 against `evidence/ctrl-only-serial.log` (the
NEW_FOLDER dispatch with src=key is there); the other findings are as
reported and not yet independently reproduced.

| Finding | Bead | Priority |
|---|---|---|
| G11 Ctrl / Shift latched after release | initech-tdnl.58 | P0 |
| G01 menus and drags time out while the button is held | initech-tdnl.59 | P0 |
| G02 close box acts on mouse-down, cannot be cancelled | initech-tdnl.60 | P1 |
| G03 disk app's own menu items do nothing | initech-tdnl.31 (existing) | P1 |
| G04 entries beyond 64 dropped silently | initech-tdnl.61 | P1 |
| G05 disk-full silent; uncommitted DESKTOP.DB shown | initech-tdnl.62 | P1 |
| G06 fifth folder window refused silently | initech-tdnl.63 | P2 |
| G07 no keyboard selection in the Finder | initech-tdnl.64 | P2 |
| G08 activating click delivered to app content | initech-tdnl.65 | P2 |
| G09 no alert on launch refusal or app crash | initech-tdnl.66 | P2 |
| G10 Arrange (by Name) enabled no-op | initech-tdnl.67 | P1 |

Not reached by either pass: the pie chart and `570-` figure (the tested film
scene had empty document bodies), an interactive film image
(`flair_live_interactive.img` was not built), real Open/Save dialogs,
editable modal controls, and any real application.
