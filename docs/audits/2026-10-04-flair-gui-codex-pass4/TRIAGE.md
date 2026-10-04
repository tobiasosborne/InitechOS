<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# Triage of the fourth FLAIR desktop audit pass (2026-10-04)

REPORT.md is the unedited output of the external reviewer (Codex gpt-6.1-sol,
xhigh) on images built from commit 40581c5, 04:45-05:04, 128 screenshots.
None of the findings below has been independently reproduced yet.

Verdicts on the developer claims: Finder icons follow the window
(initech-tdnl.34) HOLDS, including scrolled, at the screen edge and after 60
combined cycles; the foreground rule (tdnl.40) HOLDS; modifiers, held gestures
and box tracking (tdnl.58/.59/.60) still HOLD after the later changes; scroll
bars (tdnl.35) PARTLY (stepping, repeat, paging, thumb, scrolled selection and
no window drag all hold; J01, J04, J05 are edge defects; horizontal scrolling
was observed working on the emulator); no-dead-commands (tdnl.36/.67) PARTLY
(the Finder items hold; the disk app's own Quit is the known tdnl.31).

| Finding | Bead | Priority |
|---|---|---|
| J01 stale enabled/disabled scroll-bar art after resize | initech-tdnl.82 | P2 |
| J02 labels clamped into the viewport, ghost hit targets | initech-tdnl.83 | P2 |
| J03 Open handles one of two selected folders | initech-tdnl.84 | P2 |
| J04 no-motion thumb click changes the value | initech-tdnl.85 | P3 |
| J05 minimum-size window: invisible, active scroll parts | initech-tdnl.86 | P2 |

Look, unchanged from pass 3: the text reads as System 7 Chicago, not Mac OS
8.1 Charcoal (initech-tdnl.69); the drag outline is a bare rectangle
(initech-tdnl.53). No text-bearing button has been reached by any pass.
