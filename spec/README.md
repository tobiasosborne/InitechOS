# spec/ — LOCKED specs-as-data (JSON / C headers)

The contract the swarm consumes. Region algebra, the xBase coercion table,
chrome metrics, the asset sheet, and the hardware contract live here as
versioned **plain JSON / C headers** — not prose in someone's head
(CLAUDE.md Rule 8, PRD §9). The locked spec is the contract; mush is what
you get without it.

Planned contents (create as the milestones land):

- `hardware.json` — the PRD §5 hardware contract.
- `region_algebra.h` — region ops + normal-form rules (PRD §6.2).
- `xbase_coercion.json` — the PRD §6.6 type-coercion table.
- `chrome_metrics.json` — title-bar height, pinstripe period, scrollbar
  widths, etc. (PRD §6.3).
- `assets/` — palette, glyph strikes, icon sprites (PRD §6.4, §10).

Changing locked spec-data is a deliberate act with a beads issue + a
worklog note, **never** a silent edit to make one test pass (CLAUDE.md
Rule 8, Stop conditions).

Governed by: **PRD §5, §6.2, §6.4, §6.6, §9.**
