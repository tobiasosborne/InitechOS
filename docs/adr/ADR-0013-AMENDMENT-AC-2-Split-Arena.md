<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0013 Amendment AC-2 -- The split-arena resolution of initech-ubd0 (RECORDS arena vs DATA arena; FlairProcess_kill; BC-6 satisfied)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** ADR Amendment (binding; amends ADR-0013 in place)
**Programme:** STAPLER (InitechOS)
**Status:** **RATIFIED** (Architecture Review Board, App-Contract Committee, option-(b) ruling; oracle-proven host + kernel; 2026-06-27)
**Amends:** ADR-0013 (The FLAIR App Contract) Sec 3.2, Sec 3.4, Sec 3.6, Sec 7 O-3, BC-6
**Supersedes:** the AC-1 deferral of corrupt-arena death survival (`ADR-0013-AMENDMENT-AC-1-App-Contract-Integration.md` Sec 6)
**Related Issues:** initech-ubd0 (this amendment -- the death-survival tension), initech-4e35 (the App Contract epic), initech-fka6 / AC-1 (the first cut that deferred it)

---

## 1. Why this amendment

ADR-0013 Sec 3.4 asserts that application **death** (a crash, not a clean quit) routes
through window-removal and successor-promotion "**independently of the dead app's
possibly-corrupt arena**," reasoning that "`WindowRecord`s are caller-owned records the
WindowMgr threads, so removing them from the z-order does not read the dead heap."

That reasoning is **false against the implementation.** Sec 3.6 and the reference
tenants allocate each tenant's `WindowRecord` **and** its region pools **from the
tenant child arena**: `os/apps/ref_tenant.c open_common` carves `rec` (the
`WindowRecord`) plus `rs`/`rc`/`ru`/`rk` (the four region bundles) from `&self->arena`.
So `FlairProcess_terminate`'s `DisposeWindow` loop dereferences `w->nextWindow`,
`w->refCon`, and `w->strucRgn` -- every one of which lives in the arena a crash
corrupts. **BC-6 was unsatisfied**, and AC-1 (Sec 6) honestly scoped the first cut to
**clean teardown only**, deferring corrupt-arena death survival to this committee
ruling.

The Architecture Review Board ruled **option (b): split the arena.** This amendment
records the locked-contract change (Rule 8), the new death entry-point, and the
oracle that proves BC-6 -- mechanically, not by reviewer prose (Law 2).

## 2. The memory model change (Sec 3.6 amended)

A tenant now owns **TWO** child blocks carved from the master FLAIR heap at launch,
not one:

- **AC-2.1 RECORDS block (`FLAIR_CLASS_HANDLE`).** A new `records_arena`
  (`flair_heap_t`) is `flair_heap_init`'d over it. It holds the tenant's
  `WindowRecord`(s) **and ALL** its `region_t` pools -- `strucRgn`/`contRgn`/`updateRgn`
  plus the clip scratch. The shell reads **only** these during teardown; they survive a
  scribbled DATA arena.
- **AC-2.2 DATA block (`FLAIR_CLASS_GENERAL`).** The **existing** `arena` is init'd over
  it. It holds everything else: per-instance private state, the future `TERec`,
  `userData`, and future resource blobs.
- **AC-2.3 Field names preserved.** `block`/`arena` keep their meaning as the **DATA**
  side (unchanged -- this preserves the O-3 `app->block` reference and the
  `TEARDOWN_MUT_LEAK_BLOCK` mutant). Siblings `records_block`/`records_arena` are added
  to `FlairApp` (`os/flair/process.h`).
- **AC-2.4 `FLAIR_TENANT_RECORDS_DEFAULT` (16 KiB).** A documented default RECORDS-arena
  budget sized for a `WindowRecord` + four region bundles + slack
  (`sizeof(WindowRecord)` ~256 B + 4 * ref_tenant `ten_rgn_t` ~816 B + block headers ~=
  3600 B host; rounded up to 16 KiB). It is comfortably **>** `sizeof(FlairApp)` -- a
  hard precondition of the LIFO free order (Sec 4). The demo passes it for both tenants;
  `2 * (FLAIR_TEN_BUDGET + FLAIR_TENANT_RECORDS_DEFAULT)` ~= 160 KiB stays far under the
  4 MiB FLAIR heap.

## 3. Launch carves THREE blocks, fail-loud (Sec 3.2 amended)

- **AC-2.5 New `records_budget` parameter.** `FlairProcess_launch` gains a `uint32_t
  records_budget` immediately **before** the existing `budget`. It carves, in order:
  (a) the `FlairApp` handle (`HANDLE`, `sizeof(FlairApp)`); (b) the RECORDS block
  (`HANDLE`, `records_budget`) then `flair_heap_init(&app->records_arena, ...)`;
  (c) the DATA block (`GENERAL`, `budget`) then `flair_heap_init(&app->arena, ...)`.
- **AC-2.6 Fail-loud, no partial install (BC-5 preserved).** On **any** carve NULL,
  launch reclaims whatever was carved **in reverse** order (data -> records -> handle),
  installs nothing, leaves `list`/`wm` unchanged, and returns NULL. The open()-failure
  path reclaims all three the same way. No overcommit, no partial state.
- **AC-2.7 open() placement.** A tenant's `open()` carves the `WindowRecord` + region
  pools from `self->records_arena` and its per-instance private state from
  `self->arena`. `ref_tenant.c` moves `rec`/`rs`/`rc`/`ru`/`rk` to `records_arena`; the
  `tenant_priv_t` stays in `arena`.

## 4. Two teardown entries over one helper; LIFO free order (Sec 3.4 amended)

- **AC-2.8 Shared helper.** A static helper performs {`DisposeWindow` loop + unlink +
  promote-next + the three one-shot frees}. It reads window structural state **only**
  from `records_arena`-resident fields and frees the DATA block **without** reading its
  payload (`flair_free` touches only the in-band master-heap header, which precedes the
  payload the crash scribbled).
- **AC-2.9 `FlairProcess_terminate` (clean quit)** calls `procs->close()` first, then
  the helper. **`FlairProcess_kill` (DEATH/crash)** -- new -- calls the helper **only**:
  a dead app's code is never run again.
- **AC-2.10 Free order is LIFO and load-bearing.** DATA block (`GENERAL`) **first**,
  then RECORDS block (`HANDLE`), then the `FlairApp` handle (`HANDLE`) **last**. The two
  `HANDLE` frees push records-then-handle, so the **small** `FlairApp` handle is the
  `HANDLE` free-list head; cycle-2's first (small) `HANDLE` alloc reuses it and the
  large records alloc reuses the records block -- no fresh bump, `avail` stable. The
  reverse order would hand the large records block to the small request and force a
  fresh bump for the large request every cycle (`avail` drift). This is why
  `FLAIR_TENANT_RECORDS_DEFAULT > sizeof(FlairApp)` is mandatory.

## 5. Oracle / acceptance (Law 2)

- **AC-2.11 O-3 death-survival sub-test (REQUIRED, no longer deferred).** O-3's cycle-1
  drop is now THREE spans (handle + records + data), recomputed independently. The new
  sub-test launches a tenant, `memset(app->block, 0xFF, budget)` to scribble the DATA
  arena, calls `FlairProcess_kill`, and asserts the process list + z-order go empty with
  **no panic** and `avail` stable across N>=3 cycles -- a NON-by-construction proof.
- **AC-2.12 Mutants (Rule 6, all confirmed RED for the right reason).**
  `UBD0_MUT_RECORDS_IN_DATA_ARENA` (the oracle stub carves records from the DATA arena)
  -> the 0xFF scribble corrupts `nextWindow`/`refCon` -> `FlairProcess_kill`'s dispose
  loop reads the corrupted z-order -> **RED** (segfault inside `teardown_common`, the
  BC-6 violation). `TEARDOWN_MUT_LEAK_RECORDS` (skip the records-block free) -> `avail`
  drifts down -> **RED**. The prior `TEARDOWN_MUT_LEAK_BLOCK` still bites; the
  `BUDGET_MUT_OVERCOMMIT` mutant additionally bites the new over-records-budget leg.
- **AC-2.13 O-4 over-records leg.** A `records_budget` the master cannot carve ->
  launch returns NULL, installs nothing, reclaims the data block + handle (`avail`
  returns to before). Added alongside the existing over-data-budget + open-fail legs.
- **AC-2.14 Dual-compile (Law 3).** `process.c` / `process.h` and `ref_tenant.c`
  compile both hosted (the O-3/O-4 property suite) and under the kernel flags
  (`gcc -m32 -ffreestanding -nostdlib`); `kmain.c` compiles under
  `-DFLAIR_LIVE_TENANTS` with the two call sites passing
  `FLAIR_TENANT_RECORDS_DEFAULT`.

## 6. Consequences and BC update

- **AC-2.15 BC-6 is now SATISFIED.** "Teardown removes windows from the z-order and
  one-shot-frees the child block without reading the dead arena" is no longer
  aspirational: the records the dispose loop reads live in a SEPARATE, uncorrupted
  HANDLE block, and the corrupt DATA block is freed without a payload read.
- **AC-2.16 Cost.** A second per-tenant master-heap block (`HANDLE` class) and a second
  child `flair_heap_t` per `FlairApp` (~`sizeof(flair_heap_t)` larger struct). Negligible
  against the 4 MiB heap; the demo footprint stays ~160 KiB for two tenants.
- **AC-2.17 Unchanged.** The single event spine (E-D2), the cooperative no-watchdog
  model (BC-4), the refCon magic tag (BC-3), and the fail-loud launch budget (BC-5) are
  all preserved verbatim; the canonical frame images stay byte-identical (no shell.c /
  chrome change).

---

*End of ADR-0013 Amendment AC-2. RATIFIED 2026-06-27. The initech-ubd0 split-arena resolution (option (b)). Controlled Document; verify revision before use.*
