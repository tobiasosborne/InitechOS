# spec/ — LOCKED specs-as-data (JSON / C headers)

The contract the swarm consumes. Region algebra, the xBase coercion table,
chrome metrics, the asset sheet, and the hardware contract live here as
versioned **plain JSON / C headers** — not prose in someone's head
(CLAUDE.md Rule 8, PRD §9). The locked spec is the contract; mush is what
you get without it.

Authored (locked):

- `region_algebra.h` — **the ATKINSON region engine contract** (PRD §6.2;
  ADR-0005, DRAFT). Types (`rgn_rect_t`/`rgn_row_t`/`region_t`,
  per-scanline inversion lists in one contiguous `x_pool`, no per-row
  malloc); the **five normal-form invariants** (the basis of
  `region_assert_normal`, asserted at the top of every op); the four op
  truth tables (`UNION`/`INTERSECT`/`DIFF`/`XOR`) + frame-relative
  complement; the storage caps (`RGN_ROWS_CAP`/`RGN_X_POOL_CAP`, fail-loud
  on overflow); the no-`0x7FFF`-sentinel guardrail; and the verbatim
  QuickDraw op-name prototypes (`UnionRgn`/`SectRgn`/`DiffRgn`/`XorRgn`/…).
  Compiles in both modes (hosted suite + freestanding kernel); its
  `_Static_assert`s are a build-time oracle. The shared include for both
  `os/flair/atkinson` and `harness/proptest/test_region.c`.
- `chrome_metrics.json` — title-bar height, pinstripe period, scrollbar
  widths, etc. (PRD §6.3).
- `assets/` — palette, glyph strikes, icon sprites (PRD §6.4, §10).
- `dos_structs.h` / `find_data.h` / `memory_map.h` /
  `int21h_calling_convention.json` / … — the MILTON (InitechDOS) contracts.

Planned contents (create as the milestones land):

- `hardware.json` — the PRD §5 hardware contract.
- `xbase_coercion.json` — the PRD §6.6 type-coercion table.

Changing locked spec-data is a deliberate act with a beads issue + a
worklog note, **never** a silent edit to make one test pass (CLAUDE.md
Rule 8, Stop conditions).

Governed by: **PRD §5, §6.2, §6.4, §6.6, §9.**
