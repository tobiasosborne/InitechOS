<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0065 -- FLAIR Phase 4.5 Platform Services, first wave: ubd0 split-arena + Resource Manager + Scrap (49ez)

**Type:** orchestrated multi-lane session (committee for the scope/architecture fork;
delegated coding lanes; orchestrator owns grading/integration/commits/beads).
**Date:** 2026-06-27.
**Beads:** epic `initech-49ez` (Phase 4.5 Platform Services, ADR-0012 D-2b) -- children
`0w45` (Resource Manager) + `b2vk` (Scrap) CLOSED; `ubd0` (App Contract death-survival)
CLOSED, which AUTO-CLOSED the App Contract epic `initech-4e35`. Follow-ups filed: `ww9c`
(Scrap live-desktop wiring), `0lko` (Resource per-type record instantiation). Notes added
to `o5vm` (Print Manager BLOCKED-on-P3-pre) + `gymo` (Standard File Law-1 acquisition gate).
Commits `3eb39e4` (Wave 1) + `67f96f1` (Wave 2).

## Context

Operator directive: orchestrate the next most consequential FLAIR work -- delegate each
coding step, orchestrator grades/integrates/commits, raise beads, convene the committee for
serious decisions, keep going (no escalation unless committee gridlock). The App Contract
first cut (WL-0064) was complete; the plan-of-record (FLAIR-implementation-plan.md Sec 2,
Phase 4.5) names **Platform Services** as the next arc -- shared Toolbox services that LAND
BEFORE the canonical app suite (ADR-0012 D-2c), each oracle-backed, never by-construction.

## The committee (the serious fork)

ADR-by-committee `wf_00931e9e` -- 3 independent seats (sequencing / Toolbox-fidelity /
systems-heap-ownership, sonnet) -> opus chair synthesis -> opus adversarial verify. Ruled:
- **Next arc = Phase 4.5**, unanimous (epics `ib6`/`t4hp` both block on `49ez`).
- **First wave = ubd0 (split-arena) + Scrap + Resource Manager** (Tier 0 + the two independent
  foundational services; TextEdit+List -> Standard File -> Print Manager are later tiers).
- **ubd0 = option (b) split-arena, fold in.** A tenant carves TWO master-heap blocks: a RECORDS
  block (FLAIR_CLASS_HANDLE -> `records_arena`, holding WindowRecords + region pools) and the
  existing DATA block (GENERAL -> `arena`). The shell reads ONLY `records_arena` at teardown,
  so app DEATH provably survives a corrupt DATA arena -- making ADR-0013 Sec 3.4's existing
  claim literally true (it was FALSE: ref_tenant carved records from the data arena, BC-6
  unsatisfied). New `FlairProcess_kill` death path (= terminate minus close()).
- **Resource Manager** parses a REAL big-endian Mac resource-fork subset (strongest anti-by-
  construction oracle) into the tenant DATA arena. **Scrap** is a shell-owned cross-tenant
  singleton. **Print Manager BLOCKED-on-P3-pre** (grafProcs verified NULL today).

The adversarial verifier returned **PROCEED-WITH-AMENDMENTS** (upheld). The MAJOR catch: the
chair's prose said the resource-map preamble is "22 bytes" -- the verifier checked
`resource-manager.md` Sec 1.3 and found it is **24 bytes** (16 header-copy + 4 nextMapHandle
+ 2 fileRefNum + 2 mapAttrs), typeListOffset at map+24. The orchestrator independently
re-verified the spec and folded the correction (+ the 4 minor amendments) into the lane briefs.

## What changed (RED->GREEN per wave, each graded + committed)

- **Prelude (orchestrator):** `os/flair/ostype.h` -- the shared `flair_ostype_t` + endian-
  neutral `FLAIR_OSTYPE(a,b,c,d)` macro (_Static_assert-pinned), dodging the redefinition +
  endianness hazard all three seats flagged.
- **Wave 1 (`3eb39e4`), two file-disjoint OPUS lanes:**
  - **Lane A -- ubd0 (ADR-0013 Amendment AC-2):** `process.{h,c}` gain `records_block`/
    `records_arena` + `records_budget` launch param + `FlairProcess_kill`; `teardown_common`
    frees DATA->records->handle LAST (avail-stable LIFO coupling documented). `ref_tenant.c`
    rec+regions moved to `records_arena`. `kmain.c` passes `FLAIR_TENANT_RECORDS_DEFAULT`
    (16384). O-3 gains the previously-deferred corrupt-arena death sub-test + mutants
    `UBD0_MUT_RECORDS_IN_DATA_ARENA` (kill reads the 0xFF-scribbled arena -> SIGSEGV/RED) +
    `TEARDOWN_MUT_LEAK_RECORDS`; O-4 gains an over-records-budget leg. `flair_tenants_demo.h`
    comment fixed. ADR-0013 Amendment AC-2 (+ a separate AC-2 file).
  - **Lane C -- Resource Manager (`os/flair/resource.{c,h}`):** clean-room BE Mac resource-fork
    parser (24-byte preamble, count-1 type list, 12-byte ref lists w/ 3-byte data offset,
    length-prefixed data area), endian-neutral positional reads, every offset bounds-checked
    fail-loud; `FlairResMap_load`/`GetResource`/`CountResources` by type+ID into the tenant
    DATA arena. Oracle round-trips a hand-authored fork blob (authored from the spec offset
    tables, with a 24-byte-preamble self-check) vs an INDEPENDENT `WIND128_EXPECT` literal;
    mutants `RES_MUT_IGNORE_TYPE` + `RES_MUT_COUNT_OFF_BY_ONE` bite.
- **Wave 2 (`67f96f1`), one SONNET lane:**
  - **Lane B -- Scrap (`os/flair/scrap.{c,h}`):** shell-owned cross-tenant Scrap (IM Scrap
    Manager: ZeroScrap/PutScrap/GetScrap/InfoScrap, monotonic scrapCount, coexisting TEXT+PICT
    flavors, fixed inline payloads). Fail-loud over-MAX_PAYLOAD + bufsz-too-small. Independent
    op/byte oracle incl. the CROSS-TENANT App A->App B copy/paste leg via `FlairProcess_register`;
    mutants `SCRAP_MUT_IGNORE_FLAVOR`/`NO_INCREMENT`/`NO_CLEAR` bite. No Win-isms.
- **Orchestrator (serial):** all Makefile wiring (`test-resource`/`test-scrap` + mutant targets
  into `TEST_UNIT_GATES`; the two new teardown mutants); the authoritative `make clean &&
  make test` re-grade; the Bochs tenant-boot leg; commits; the bead ledger.

## Frictions / lessons

- **The orchestrator re-grades, never trusts the lane report (Rule 4 / WL-0064 lesson).** Each
  lane's oracle + mutants were independently re-run via direct gcc before integration; the
  highest-risk lane (Resource Manager) had its fixture eyeballed to confirm the 24-byte preamble
  + the independent expect literal are ACTUALLY there, not just claimed.
- **Lane parallelism vs the process.h edit-race:** Lane A edits `process.{h,c}`; Lane B's cross-
  tenant test #includes it. Ran Lane A || Lane C (both opus, fully file-disjoint -- Lane C takes
  `flair_heap_t*`, never touches process.h) first, then Lane B (sonnet) after process.h
  stabilized. Lanes compile to private `/tmp` dirs (never `make`) to avoid the build/ race.
- **The verifier earned its keep:** the 22-vs-24-byte map preamble would have silently broken
  every resource lookup; the fixture-authored-from-prose-not-spec trap was pre-empted.
- **Closing the last App Contract child (ubd0) auto-closed epic `4e35`** -- the App Contract is
  now fully complete (first cut + the death-survival hardening).
- **Heap free-list-no-rollback** keeps biting design: Scrap uses fixed inline payloads (not
  per-put flair_alloc) and the O-3 oracle asserts avail-STABLE (not returns-to-pre-launch).

## Acceptance

`make clean && make test` = **ALL GREEN 287 host + 57 emu** (was 283+57; +2 host = test-resource
+ test-scrap and their mutants gated). Bochs tenant-boot leg PASS (shared kernel + FLAIR-heap
milestones + the 640x480 fail-loud guard, no triple-fault). O-5 app-switch + O-7 SAMIR-suspend
+ the Office Space frame UNREGRESSED; `_kernel_end=0x38ec0 < 0x40000`. All new mutants RED for
the right reason; reproducible; ASCII-clean; freestanding host+kernel dual-compile.

## Pointers / next

- `os/flair/{ostype,resource,scrap}.{h,c}`, `harness/proptest/{test_resource,test_scrap}.c`,
  `os/flair/process.{h,c}` (records_arena + FlairProcess_kill), `os/apps/ref_tenant.c`,
  `docs/adr/ADR-0013-AMENDMENT-AC-2-Split-Arena.md`.
- **NEXT (Phase 4.5 remaining, dependency-ordered):** `77dj` TextEdit + List Manager (depends on
  the Scrap; TERec/ListRec into the DATA arena; grounded in system7-decomp textedit.md +
  list-manager.md) -> `gymo` Standard File (Law-1 acquisition gate first: no SFGetFile spec in
  the corpus) -> `o5vm` Print Manager (BLOCKED until the GrafPort verb layer / re30 P3-pre).
  Plus the 2 filed follow-ups (`ww9c` Scrap live-desktop wiring; `0lko` Resource per-type
  instantiation). Then the canonical app suite (Initech123 / InitechWord) builds ON these services.
