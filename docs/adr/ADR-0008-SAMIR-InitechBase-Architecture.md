<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0008 -- SAMIR (InitechBase) Architecture: the PAL, the storage split, and the III+ 1.1 target

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0008 |
| Title | ADR-0008: SAMIR (InitechBase) Architecture |
| Version | 1.0 (Ratified with revisions) |
| Status | **RATIFIED 2026-06-17 (operator-delegated; ADR-by-committee, all revisions applied)** |
| Classification | Internal Use Only |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme |
| Related Documents | `docs/plans/SAMIR-implementation-plan.md` (the granular DAG); `docs/research/dbase-ground-truth.md` (M6 brief); ADR-0002 (impl language C); ADR-0003 (InitechDOS); ADR-0004 (FLAIR); PRD §6.6/§8/§11-M6/§14; sister corpus `../dbase3-decomp` |
| Related Issues | beads `initech-586` + children (`aul/7az/ahu/gmo/0tl/17n/ax9`, `586.1-.4`) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Architecture Review Board, STAPLER Programme | Submitted | 2026-06-17 |
| ARB Reviewer -- Technical Correctness (PAL / freestanding) | M. Bolton (Senior Engineer, Platform) | Ratify-with-revisions (SS-1 terminal scope, SS-2 seek=fsize, SS-3 DEC-07 computed-key) -- **applied** | 2026-06-17 |
| ARB Reviewer -- Oracle / Harness | T. Smykowski (QA / Change Advisory) | Ratify-with-revisions (loud-skip exit-1, python-refs->Phase0, narrow DEC-06 lock, +mutant gaps) -- **applied** | 2026-06-17 |
| ARB Reviewer -- Period Authenticity (dBASE III+ 1.1 fidelity) | S. Nagheenanajar (Heritage Conformance) | Ratify-with-revisions (PRD reconciliation, coercion/normalization provenance pinning, IV-leak guards) -- **applied** | 2026-06-17 |
| Operator Ratification | T. Osborne (Operator) | **Granted** (targeting decision 2026-06-17; ratification delegated -- "draft and ratify the adr") | 2026-06-17 |

---

## 1. Context

M6 (InitechBase, codename SAMIR) is a dBASE-compatible database that **really runs** inside InitechOS (PRD §6.6). It is **C** (ADR-0002; bundled app -- not Pascal). The engine must (a) survive InitechDOS churn without cascading refactors, (b) be gradable with fast, mechanical feedback against the byte-verified sister corpus `../dbase3-decomp`, and (c) ship a byte-faithful dBASE artifact. This ADR fixes the architecture; the granular ~40-step DAG lives in `docs/plans/SAMIR-implementation-plan.md`.

## 2. Decisions

**DEC-01 -- Storage split (artifact/factory).** The engine lives in `os/samir/` (`include/samir/`, `core/`, `fs/`, `cmd/`, `ui/`, `pal/`, `samir_main.c`); the grader lives in `harness/diff/dbf_diff/` (mirroring `harness/diff/fat_diff/`); locked spec-data lives in `spec/samir/`. No factory C in the shipped engine; the python references live in the C harness dir exactly as `fat12_ref.py` does (Law 3).

**DEC-02 -- Platform Abstraction Layer (the portability guarantee).** The engine's ONLY OS surface is the `samir_pal` vtable (`os/samir/include/samir/pal.h`): byte file-I/O, cooked console, an **injectable** clock, and a fixed-arena allocator. Two implementations: `pal_host.c` (factory/oracle, libc + fixed clock) and `pal_milton.c` (artifact, InitechDOS INT 21h handle API -- `3Dh/3Eh/3Fh/40h/42h/3Ch/41h/56h`, `2Ah`, CON, `48h`). The engine never issues `int 0x21`. **Consequence:** an InitechDOS drift/revert touches only `pal_milton.c`; the engine + every host oracle are insulated, and Phases 0-7 are fully developable/gradable on the host with zero kernel/GUI dependency.

> **DEC-02 revisions (ARB review, 2026-06-17):**
> - The vtable is the **complete binding contract** and therefore includes a clearly-marked **terminal extension** for `@SAY/GET/READ` (S8.4): `conin_char` (single raw keypress, no echo -> Milton `AH=07h`, `do_conin_raw` at `os/milton/int21.c:651`) and `gotoxy(row,col)` + an attribute/clear primitive (an OS console primitive, NOT an INT 21h call -- backed by the LFB/FLAIR text console). These are part of the ratified contract so implementing S8.4 does not re-open `pal.h`. The Phase-0 host oracles exercise only the file/cooked-console/clock/arena core.
> - **File size** is obtained via `seek(fd, 0, PAL_SEEK_END)` (the documented idiom; maps to Milton `AH=42h AL=2`, `do_lseek` at `int21.c:2274`). Required by the `dbf_open` truncation-invariant check (Rule 2) and `FILE()`.
> - **Known latent constraints (tracked, not M6 blockers):** the JFT bounds concurrent opens at ~15 user handles (`sft.h:36` `SFT_MAX_ENTRIES=20`); a many-work-area/many-index app needs `FILES=` raised -- the M6 canon apps use 1-2 work areas and fit. The bump-arena leaks on repeated large C-var rebinding in long `DO WHILE` loops -- sufficient for the canon apps, a known no-malloc limitation. `AH=56h` RENAME is same-directory only (matches dBASE usage). `pal_milton.c` obtains one large `AH=48h` block at startup and manages it as the arena (segment<<4 -> flat).

**DEC-03 -- Target dBASE III PLUS 1.1 ONLY for M6** (operator-decided 2026-06-17). All ground truth (corpus + goldens + live dosbox-x mint harness) is III+ 1.1; no IV golden exists. `.mdx`, `F` type, `==`, and the `0x8B` IV memo dialect are **out of M6 scope** (deferred to a post-M6 IV epic). The lexer treats `==` as a lex error; codecs fail loud on IV version bytes. Narrows bead `ahu` to `.ndx`.

**DEC-04 -- Freestanding engine, host-compiled oracles.** `core/` + `fs/` compile `-ffreestanding -nostdlib` (CDR-0001 interim toolchain), using only `rt.h` helpers + the PAL. Host oracles compile the same `.c` against `pal_host.c` -- the kernel's proven `os/milton/*.c` + `test_*.c` pattern.

**DEC-05 -- The grader is the sister corpus, three-tier.** Tier 0: committed assertion manifests derived from the corpus "Verification" byte-dumps (operator-free, day-1). Tier 1: raw goldens referenced by `DBASE3_DECOMP ?= ../dbase3-decomp` -- if absent the leg **exits non-zero**, prints the missing path `$(DBASE3_DECOMP)/goldens` and the phrase "a skipped oracle is worse than a red one" (the `test-fat-fault-rollback` `mformat`-guard idiom at `Makefile:1585`; never silent). Tier 2: new goldens minted via the sister `re/harness-setup.md`. Goldens are NOT committed (copyrighted; the `spec/assets/preview.webp` stance). Every golden/manifest is mutation-proven (Rule 6).

> **DEC-05 revision (ARB review):** the Tier-0 manifests share the engine's `spec/samir/*_format.h` offset constants, so until an **independent** reader exists they are a soft oracle (shared blind spot). The independent python readers (`dbf_ref.py`, `ndx_ref.py`) are therefore **pulled forward into Phase 0** (run right after S0.5), not deferred to Phase 6 -- exactly as `fat12_ref.py` ships alongside `fat_dump.c`. Tier-0 manifests must cite corpus byte-dump offsets directly, not derive them via `*_format.h`.

**DEC-06 -- Locked spec-data imported from the corpus (III+-only sources, pinned).** `spec/samir/{dbf_format.h, ndx_format.h, xbase_coercion.json, dbf_normalization.json, dbase_msg_codes.tsv}` + a `README.md` provenance ledger. **The CORE coercion rules and the NDX numeric-key encoding are lockable now** (corpus MINT settled `C+N`=error#9, raw-LE-double-arithmetic NDX keys, LE `.dbt` ptr, ties->+inf, IEEE-double internal rep, `==`/`DTOS` absent -- plan §3.3); the `numfn-1..8` cells (MOD/INT/ROUND/LOG/SQRT edges) remain **GATED** pending MINT and are loud-skipped, not locked.

> **DEC-06 revision (ARB review) -- provenance pinning (prevents IV leakage):** the locked files are sourced from the **III+-only corpus tables**, NOT from `docs/research/dbase-ground-truth.md` (whose drafts straddle III+/IV):
> - `xbase_coercion.json` <- `../dbase3-decomp/specs/language/coercion-table.md §6` (has NO `==` row; `==` is in `not_in_iii_plus`). The brief's §5.6 schema (which contains a `==` row + "DRAFT pending dBASE IV") is **superseded** for III+ work.
> - `dbf_normalization.json` <- `../dbase3-decomp/specs/file-formats/dbf.md §2` (III+ table). The production-`.MDX` flag `0x1C` and the per-descriptor MDX-field flag `0x1F` are **NORMALIZE** for III+ (they are `0x00` in 100% of III+ fixtures) -- NOT "MEANINGFUL iff `.MDX`" as the brief's IV-straddling table marks them. Importing the brief's table would plant a latent IV footprint in the round-trip oracle.
> The `README.md` ledger must name these source files explicitly and flag the brief as superseded-for-III+.

**DEC-07 -- Numeric: IEEE-754 `double` internally; observable behavior in the formatter.** N and D (JDN) are doubles (corpus mint-004). The target needs x87 (no kernel FPU init today; no `spec/hardware.json`) -- a small "FPU-ready" step + a hardware-contract line (plan Phase 8). All graded numeric output for **stored ASCII N fields and simple field-reference index keys** flows through the decimal formatter (ties->+inf), so x87 intermediate-precision never reaches those gates.

> **DEC-07 revision (ARB review):** the "never reaches a gate" claim is qualified. For **computed numeric key expressions** (e.g. `INDEX ON PRICE * 1.1`), x87 80-bit intermediate precision can make SEEK / index ordering diverge from real dBASE III+ 1.1 (build-time round-to-64-bit vs seek-time 80-bit re-eval). This is **oracle-resolves / GATED** (corpus numfn-7) and is NOT required for M6 round-trip correctness (stored N is formatted ASCII). The long-term remedy -- set the x87 precision-control word to 64-bit double via `FLDCW` -- is deferred to post-M6.

## 3. Alternatives considered

- **Direct INT 21h in the engine (no PAL).** Rejected: couples the engine to InitechDOS churn (the exact failure mode the operator named) and blocks host grading.
- **dBASE IV target now.** Rejected (DEC-03): no IV golden; `.mdx` node bodies are undocumented (CDX != MDX); would force guessing (Law 1 violation).
- **BCD numerics.** Rejected (DEC-07): corpus minted internal rep = IEEE double; BCD would diverge from real dBASE.
- **Commit goldens.** Rejected (DEC-05): copyrighted abandonware; the path-reference + Tier-0 manifest design keeps the gate operator-free without committing copyrighted bytes.

## 4. Consequences

Positive: engine survives OS drift; bulk of work parallelizable and host-gradable; nothing guessed (everything in scope is corpus-grounded); the plan is both a swarm DAG and a serial backlog. Costs: a PAL indirection layer (small); IV deferred (acceptable per operator); x87 readiness work on the kernel side (one small step, flagged). Risks tracked in plan §7 (MINT-gated edge cases, FLAIR seam, authenticity tier).

## 5. Locked artifacts authored on ratification

`os/samir/include/samir/pal.h` (the contract, plan §8.1); `spec/samir/*` (DEC-06); the `harness/diff/dbf_diff/` skeleton + `test-dbase` umbrella. Per Rule 8 these are authored as the contract; ratification makes them binding.

---

*-- End of ADR-0008 (DRAFT; pending operator ratification) --*

<!-- Tedium certified compliant with NFR-7. -->
