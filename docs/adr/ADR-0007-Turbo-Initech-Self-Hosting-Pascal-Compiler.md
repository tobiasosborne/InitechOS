<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0007 -- Turbo Initech: Self-Hosting Pascal Compiler, Language Subset, and the K2==K3 Fixed Point

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0007 |
| Title | ADR-0007: Turbo Initech -- Self-Hosting Pascal Compiler |
| Version | 1.0 (Ratified with amendments) |
| Status | **RATIFIED (2026-07-11, three-seat committee, unanimous ratify-with-amendments; amendments applied at ratification)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board, STAPLER Programme (drafted from the `initech-qvtj` committee analysis) |
| Effective Date | 2026-07-11 |
| Next Scheduled Review | Upon operator counter-signature, per RECORDS-POL-002 |
| Supersedes | (none -- retires the "ADR-0007, pending" citation: ADR-0003 Sec 1.2 / Sec 3 DR-5 / Sec 9, PRD Sec 6.7 / Sec 14, and CLAUDE.md are all updated in the same change-set to cite this filed, ratified document) |
| Superseded By | (none) |
| Related Documents | `docs/plans/TPS-M7-subset-plan.md` (the sizing analysis this ADR ratifies, esp. Sec 3/Sec 5); `InitechOS-PRD.md` Sec 1.2, Sec 4, Sec 6.7, Sec 7, Sec 11 (M7/M8), Sec 14; ADR-0002 (toolchain/impl language/executable format); ADR-0003 (InitechDOS base OS, the `INT 21h` surface the RTL boundary sits on); CDR-0001 (interim toolchain deviation) |
| Related Issues | beads `initech-79s` (this ADR); `initech-qvtj` (M7 planning, closed, produced the scope draft in Sec 5 of the plan); `initech-znb` (seed cross-compiler scaffold, closed -- the genesis); epic `initech-rnh` (M7); epic `initech-ls4` (M8); the B-chain `initech-f0uc/80iw/7mo3/63ce/54uu/rug7/39k2/ogxv/6m52/9xu5`; `initech-3yv` (reproducible-build discipline, the DEC-06 precondition) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-07-11 | Architecture Review Board, STAPLER Programme | Initial draft. Closes the citation loop opened by ADR-0003 Sec 1.2/Sec 9, PRD Sec 6.7/Sec 14, and CLAUDE.md, all of which have referenced "ADR-0007, pending" since 2026-06-08. Authored from the `initech-qvtj` gap-sizing analysis and its scope paragraph (`docs/plans/TPS-M7-subset-plan.md` Sec 5). Records DEC-01 through DEC-08: the two-compilers separation, the minimal non-TP7 self-host subset, complete boolean evaluation, single-pass stack-machine codegen, the RTL boundary, the K2==K3 self-host criterion + DDC, the three-rung oracle ladder, and the IDE/M7-vs-M8 scope line. | (submitted for committee review) |
| 1.0 | 2026-07-11 | Architecture Review Board, STAPLER Programme | Ratified with amendments by the three-seat ratification committee (Governance/Consistency, Technical Soundness, Oracle/Process), unanimous. All thirteen committee amendments applied at ratification: OQ-1 RESOLVED option (b) -- PRD Sec 6.7 staging clause authored; citation-loop closure (ADR-0003 Sec 1.2/Sec 3 DR-5/Sec 9, PRD Sec 6.7/Sec 14, CLAUDE.md); DEC-02 dialect pinning (32-bit `integer`, fpc `{$B+}` differential, case-insensitive identifiers, `for` `to`/`downto`), `forward` directive added to the REQUIRED list, fixed-capacity/fail-loud static-arena consequence, enums-sets bucketing footnote; DEC-03 sentinel-accessor + boolean-flag-exit idioms named; DEC-04 insertion-ordered symbol tables + single threaded label counter (seed re-walk anti-pattern cited), seed precedent scoped to expression level; DEC-05 in-subset int-to-decimal-string boundary sentence; DEC-06 `initech-3yv` wiring + new FO-5 pre-B4 repro gate; DEC-07 host-tooling clarification, B7-before-B8 rationale, B8 FILEIO-class mutant recorded. | Ratification committee (3 seats), unanimous |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter | Architecture Review Board, STAPLER Programme | Submitted (DRAFT 0.1) | 2026-07-11 |
| Committee Seat -- Governance / Consistency | ADR ratification committee, STAPLER Programme | Ratified 2026-07-11 (ratify-with-amendments; amendments applied) | 2026-07-11 |
| Committee Seat -- Technical Soundness | ADR ratification committee, STAPLER Programme | Ratified 2026-07-11 (ratify-with-amendments; amendments applied) | 2026-07-11 |
| Committee Seat -- Oracle / Process | ADR ratification committee, STAPLER Programme | Ratified 2026-07-11 (ratify-with-amendments; amendments applied) | 2026-07-11 |
| Operator Ratification | T. Osborne (Operator) | (pending) | -- |
| Records Management | M. Waddams (Archive Annex B) | Filed (2026-07-11) | 2026-07-11 |

*Note on status: RATIFIED 2026-07-11 by the three-seat ratification committee (Governance/Consistency, Technical Soundness, Oracle/Process) -- unanimous ratify-with-amendments, all amendments applied at ratification and recorded in Revision History rev 1.0. The DEC-01..DEC-08 decisions, the DEC-02 subset table, and the DEC-06 criterion are binding per Rule 8. The Operator Ratification row remains open in the matrix above (operator counter-signature pending).*

---

## 1. Purpose and Scope

### 1.1 Purpose

This ADR fixes the architecture of **Turbo Initech** (codename TPS): the resident, self-hosting Pascal compiler that is InitechOS's in-universe North Star (PRD Sec 1.2). Four documents have cited "ADR-0007, pending" since the project's first week (ADR-0003 Sec 1.2 and Sec 9; PRD Sec 6.7 and Sec 14; CLAUDE.md's "Two compilers, never conflated" and "Self-host certificate" callouts) without a real document behind the citation. This Record closes that loop: what Turbo Initech is, what language it accepts, how it compiles, where its runtime boundary sits, what "self-hosting" means as a falsifiable criterion, how that criterion is graded before it is real, and what is in versus out of the M7/M8 scope line.

### 1.2 Scope

This ADR governs:

- The **identity of Turbo Initech** and its separation from the seed cross-compiler and from the C OS. (DEC-01)
- The **language level**: the minimal non-TP7 subset sized by the `initech-qvtj` gap analysis to be just sufficient for Turbo Initech to express itself. (DEC-02)
- **Complete boolean evaluation** (not short-circuit) and the source-discipline consequence. (DEC-03)
- **Codegen strategy**: single-pass, stack-machine, no optimizer, deterministic. (DEC-04)
- The **RTL boundary**: what the hand-assembled runtime does and does not provide. (DEC-05)
- The **self-host criterion**: the Phi fixed point, `K2 == K3` bit-for-bit, scoped to Turbo Initech, with DDC as the trusting-trust antidote. (DEC-06)
- The **oracle ladder**: what grades correctness today, at the codegen pivot, and at M7 close. (DEC-07)
- The **IDE and its milestone boundary** (M7 vs the M8 finale). (DEC-08)

### 1.3 Out of Scope

- The **seed cross-compiler's** own internal implementation (`seed/`) -- governed by its own beads (`initech-znb` and the B-chain below); this ADR fixes what Turbo Initech (the Pascal artifact the seed emits and later self-hosts) must be, not how the seed's C source is organized.
- The **C kernel/Toolbox/InitechDOS** -- out of scope by definition (PRD Sec 4, "two compilers, never conflated"; ADR-0002; ADR-0003). Nothing in this Record touches C OS architecture.
- **InitechBase (SAMIR)** -- a separate C bundled application (ADR-0008), unrelated to Turbo Initech beyond both consuming InitechDOS's `INT 21h` surface.
- The **granular per-step implementation DAG** -- that lives in `docs/plans/TPS-M7-subset-plan.md` Sec 4 (the B1-B10 dependency chain) and the individual beads; this ADR ratifies the *scope* the plan sizes, not the day-to-day sequencing.
- **Turbo-Vision widget-level design** (menu bar layout, editor keybindings, window chrome) -- deferred to an M8-era design note; DEC-08 fixes only the scope boundary, not the pixel-level design.

### 1.4 Additional Defined Terms

| Term | Definition |
|---|---|
| Turbo Initech | The resident, self-hosting Pascal compiler and its Turbo-Vision-style IDE -- the in-universe North Star (PRD Sec 1.2, codename TPS). Runs *on* InitechOS. |
| The seed | The C, host-hosted, single-pass Pascal-to-x86 cross-compiler (`seed/`, beads `initech-znb` + the B-chain) that emits Turbo Initech's first binary. The **genesis**, not the OS bootstrap (PRD Sec 4). |
| Resident compiler | Turbo Initech once it runs *on* InitechOS and compiles from inside the OS -- the object of the self-host fixed point (Sec 7 below; distinct from the seed). |
| Phi (`Phi`) | The compilation operator `Phi(K) = K(src)`: run compiler binary `K` on the canonical Turbo Initech source `src`, producing a new binary (PRD Sec 7). |
| K1 / K2 / K3 | `K1 = X(src)` (seed `X` builds the resident compiler, cross); `K2 = K1(src)` (resident compiler rebuilds itself, hosted on InitechOS); `K3 = K2(src)`. The certificate is `K2 == K3` bit-for-bit, **not** `K1 == K2` (K1 is emitted by a different compiler and may differ byte-wise; PRD Sec 7, CLAUDE.md hallucination-risk callout). |
| DDC | Diverse Double-Compilation (Wheeler) -- compile `src` with an **independent** seed and confirm the resulting `K*` matches, as the antidote to a Thompson "Reflections on Trusting Trust" self-perpetuating backdoor in a fixed point. Formal semantic-preservation proofs are out of scope (PRD Sec 7). |
| Non-TP7 subset | A deliberately smaller language than Turbo Pascal 7: no objects, no full dynamic-string/heap machinery as a language default, no unit system -- sized to what Turbo Initech needs to express itself (`docs/plans/TPS-M7-subset-plan.md` Sec 3), not to compete feature-for-feature with a commercial IDE. |
| Complete evaluation | Both operands of a boolean `and`/`or` expression are always evaluated (no short-circuit / McCarthy evaluation). The opposite of Free Pascal's default and of C's `&&`/`\|\|`. |
| RTL | Run-Time Library -- here, the thin hand-assembled entry stub + `INT 21h` file-I/O shim Turbo Initech links against, in the `seed/rt/start.asm` idiom, not a full Borland-style RTL. |

---

## 2. Context

### 2.1 The citation predates the document

ADR-0003 (ratified 2026-06-08) listed, until this ratification, "The self-hosting compiler (\"Turbo Initech\"), addressed under ADR-0007 (pending)" in its Sec 1.2 Out of Scope and again in its Sec 9 Related Decisions. PRD Sec 6.7 and Sec 14 both cited "ADR-0007, pending." CLAUDE.md's Laws and hallucination-risk callouts describe Turbo Initech's architecture (two-compilers separation, the K2==K3 criterion, DDC) in enough detail that engineers have been *treating the prose as if it were already an ADR* without a real ratified Record behind it. This is exactly the "mush is what you get without [locked spec]" failure mode PRD Sec 9 warns about, applied to governance documents rather than data.

With ratification (2026-07-11) the citation loop is closed in one act, enumerated for the record: **ADR-0003 Sec 1.2, Sec 3 (DR-5), and Sec 9**; **ADR-0003-AMENDMENT-DEC-04a** (Sec 1.3, Sec 4.2, C-4, and its Sec 8 reference list -- the ring-3 DPL obligation there is a dependency of a *future ring-3 milestone*, which this ADR does not introduce); **ADR-0004 Sec** (related-decisions list) **and ADR-0012** (Document Control + Sec 6); **PRD Sec 6.7** (which also gains the OQ-1 staging clause, see Sec 7) **and Sec 14**; and **CLAUDE.md's** "(ADR-0007, pending)" mention -- all updated in this change-set to cite this filed, ratified Record. No "(pending)" ADR-0007 citation remains anywhere in the corpus; PRD Sec 15's decisions-log row already cited ADR-0007 without a pending qualifier and needed no change.

### 2.2 Why now, and why this shape

Until 2026-07-11, the shape of "the minimal subset Turbo Initech needs" was unsized: the seed itself was integer-only with no control flow or procedures (`docs/plans/TPS-M7-subset-plan.md` Sec 2, citing `parser.c`/`codegen.c` line ranges), `os/tps/` was empty, and the compiler milestone targets were deliberate failing placeholders. The `initech-qvtj` committee analysis (closed, `docs/plans/TPS-M7-subset-plan.md`) did the sizing work: it separated REQUIRED language families from ones AVOIDABLE-with-a-named-idiom from ones DEFERRED past the fixed point, and produced a ten-step dependency chain (B1-B10) with a beads issue per step. Its Sec 5 delivers a scope paragraph explicitly marked "for `initech-79s` to ratify." This ADR is that ratification. Implementation status as of B9.5: `test-compiler` is real; only the M8 `selfhost` and `ddc` placeholders remain.

### 2.3 Ground truth cited

- PRD Sec 1.2 (self-hosting compiler vision), Sec 4 (system architecture -- "two compilers, never conflated"), Sec 6.7 (Turbo Initech subsystem spec), Sec 7 (the bootstrap as a fixed point -- the Phi/K1/K2/K3 formalism, DDC), Sec 11 M7/M8 (milestone acceptance), Sec 14 (tooling/stack -- language ownership).
- `docs/plans/TPS-M7-subset-plan.md`, all six sections, in particular Sec 3 (the R/A(idiom)/D verdict table) and Sec 5 (the scope paragraph this ADR ratifies almost verbatim).
- CLAUDE.md "Two compilers, never conflated" and "Self-host certificate is K2==K3, not K1==K2" hallucination-risk callouts.
- `seed/rt/start.asm` (the hand-assembled entry-stub pattern DEC-05 mirrors) and `seed/examples/arith/*.pas` (the hand-computed exact-serial golden pattern DEC-07's rung 1 mirrors).
- beads `initech-znb`, `initech-qvtj`, epics `initech-rnh`/`initech-ls4`, and the B-chain `f0uc/80iw/7mo3/63ce/54uu/rug7/39k2/ogxv/6m52/9xu5` (bead titles re-verified against the live tracker for this draft, not inferred from the plan document alone).

---

## 3. Decision Drivers

1. **DR-1 -- Continuity with the North Star.** The architecture must be realizable by the seed (already scaffolded, `initech-znb`) and must terminate at a real, checkable fixed point (PRD Sec 1.2/Sec 7), not an open-ended feature race.
2. **DR-2 -- Minimality over completeness.** The subset must be sized to *what Turbo Initech needs to express itself*, not to TP7 feature parity (`docs/plans/TPS-M7-subset-plan.md` Sec 6: "the smallest sufficient language").
3. **DR-3 -- Verifiability at every step.** Each language family lands with its own oracle (hand-computed exact-serial fixture now, Free Pascal differential once meaningful, mutation-proof on every deep-bug flag) -- Law 2/Rule 6, not "looks right."
4. **DR-4 -- Determinism.** Codegen output must be byte-reproducible (Rule 11); the K2==K3 certificate is meaningless otherwise.
5. **DR-5 -- Honest scope boundaries.** What is deferred past the fixed point must be *recorded*, not silently absorbed later (Rule 8; bead `initech-9xu5` exists for exactly this).

---

## 4. The Decision

The following decisions (DEC-01 through DEC-08) are RATIFIED (2026-07-11, three-seat committee, unanimous ratify-with-amendments; the amendments are applied inline below and itemized in Revision History rev 1.0).

### 4.1 DEC-01 -- What Turbo Initech Is (the two-compilers separation)

Turbo Initech is **the resident, self-hosting Pascal compiler** -- the in-universe North Star (PRD Sec 1.2) -- together with its Turbo-Vision-style IDE (DEC-08). It is one of exactly **two** compilers in this project, and they are **never conflated** (PRD Sec 4, CLAUDE.md hallucination-risk callout):

- **The seed** (`seed/`, C, host-hosted, bead `initech-znb`) is the **genesis**, not the OS bootstrap. It cross-compiles Pascal to freestanding x86 on the development machine so that Turbo Initech has a first binary to start from (`K1`, Sec 5.6 below). Bugs in the seed are seed bugs, not Turbo Initech bugs (CLAUDE.md hallucination-risk callout).
- **Turbo Initech** (`os/tps/`, Pascal, resident) is the artifact that self-hosts: it compiles itself and the Pascal user programs authored on InitechOS (PRD Sec 6.7).

The **C kernel/Toolbox/InitechDOS/bundled apps for the current release are explicitly out of scope of this ADR and of Turbo Initech's self-host claim.** "Recompile from inside the OS" means Turbo Initech and the Pascal programs it compiles -- it does **not** mean the C OS, which is built and rebuilt by the factory cross-toolchain (ADR-0002; interim per CDR-0001). This is restated here because it is the single most common conflation risk in the project (CLAUDE.md flags it explicitly) and because this ADR is the canonical place to pin it down.

### 4.2 DEC-02 -- Language Level: the Minimal Non-TP7 Subset

Turbo Initech's self-host-critical-path language is the subset sized by `docs/plans/TPS-M7-subset-plan.md` Sec 3 -- **not** Turbo Pascal 7 feature parity. The subset is fixed as follows.

**Included, required (no idiom needed -- the language provides it directly):**

- `if`/`then`/`else`, `while`/`do` as primitive control flow.
- Procedures and functions: nested scopes, **both value and var parameters**, recursion, results. (Var parameters are technically avoidable via a globals idiom, per the plan's own AVOIDABLE list, but are included as a *real* language feature for source-code cleanliness -- see the note at the end of this list.)
- The **`forward` directive** for procedure/function declarations (ratification amendment) -- required, not optional: the parser's own block <-> statement mutual recursion (a statement contains a block contains statements) is inexpressible in a single-pass compiler without a forward declaration, and both Turbo Pascal and ISO 7185 provide it.
- Relational operators (`=`, `<>`, `<`, `<=`, `>`, `>=`), a boolean type, and `and`/`or`/`not` -- under **complete evaluation** (DEC-03).
- `const` declarations.
- `char`, `ord`, `chr`.
- Static arrays (`array[lo..hi] of T`), indexed as both l-value and r-value.
- Records (`record ... end`), field access, arrays of records, with a deterministic field layout (Rule 11). The parallel-array idiom that would avoid records was **rejected as too painful** (plan Sec 3) -- records are a real, first-class language feature here.
- Minimal fixed/`ShortString`-style strings (length/index/compare/concat) -- **not** dynamic/heap strings.
- A thin file-I/O RTL over InitechDOS `INT 21h` handles (DEC-05).
- Static-arena memory (every variable is a statically allocated slot or stack-frame slot; no heap allocator). **Binding consequence (ratification amendment):** every compiler-internal collection -- the AST/node pool, symbol tables, parameter lists -- is therefore a FIXED-capacity static array; capacity overflow is a Rule-2 fail-loud diagnostic that names the exceeded limit constant, never a silent truncation or wraparound.

**Included, but avoidable with a named idiom (kept anyway for clean source, per the plan's own "AVOIDABLE... but included" note on var parameters above; the remaining two sugar/idiom items ARE desugared rather than implemented as separate codegen paths):**

- `for`/`repeat`/`case` -- pure sugar over `while`/`if`-chains; a self-host source file may use them, but they desugar to the primitive forms above rather than requiring new codegen machinery.

**Explicitly OUT of the self-host subset (deferred past the K2==K3 fixed point, bead `initech-9xu5` -- the scope guard that exists precisely so no lane pulls these in early):**

| Excluded family | Named workaround idiom (if any) | Why deferred |
|---|---|---|
| Heap / pointers | Static index-arenas (an integer "handle" indexes a fixed array instead of a real pointer) | Full pointer/heap machinery is not needed to express a single-pass compiler over static-arena data structures; it is a post-fixpoint niceity. |
| Enumerated types | Named integer constants (`const`) | An enum is sugar over a small set of named ordinals; the const mechanism already covers it. |
| Sets / `in` | Predicate functions (a boolean function replacing a set-membership test) | Set-of-char codegen is a **known trap** (plan Sec 3) with disproportionate implementation risk for a feature the compiler's own source does not need. |
| Units | Single-file bootstrap (the whole compiler lives in one translation context) | A module/unit system is real engineering weight that a from-scratch bootstrap compiler does not need to reach self-hosting; revisit once the fixed point is proven. |
| Real (floating-point) numbers | (none -- excluded entirely) | The compiler's own logic (lexing, parsing, symbol tables, codegen) is integer/ordinal; no real-number arithmetic is load-bearing for self-hosting. |
| Objects | (none -- excluded entirely) | Object-oriented dispatch is orthogonal to a single-pass stack-machine compiler; out of scope for the fixed point. |
| Language-level inline `asm` blocks | RTL is separately assembled (hand-written `.asm`, linked in -- DEC-05) | Turbo Initech never needs to *emit* inline-asm semantics for its own source; the one place assembly is genuinely required (the runtime entry stub, low-level I/O) is handled outside the compiled language entirely. |

> **Bucketing note (deliberate, recorded; ratification amendment).** The plan's Sec 3 prose places enums and sets/`in` in the AVOIDABLE-with-idiom bucket; this ADR follows the plan's Sec 5 scope paragraph and the `initech-9xu5` scope guard in placing both families PAST the K2==K3 fixed point (the deferred table above). The named idioms (const-int; predicate functions) are the self-host source's mechanism either way; what this resolves is which side of the fixed-point line the language *features themselves* land on -- post-fixpoint. A deliberate resolution of the plan's internal ambiguity, not an oversight.

**Dialect pinning (binding; ratification amendment).** Four dialect points are pinned so that "Turbo-Pascal-flavoured" is a checkable claim, not a vibe:

1. `integer` is **32-bit signed** -- deliberately NOT TP7's 16-bit `integer`. This matches what the seed already emits (32-bit signed int, stack-machine `eax`; `docs/plans/TPS-M7-subset-plan.md` Sec 2) and what the flat-32 target wants (ADR-0001); the divergence from TP7 is recorded, not accidental.
2. The Free Pascal differential (DEC-07 Rungs 2/3) MUST invoke `fpc` with **complete boolean evaluation enabled** (`{$B+}` / `{$BOOLEVAL ON}`) so that both compilers implement DEC-03's semantics; run at fpc's short-circuit default, the differential would diff a deliberate semantics divergence instead of detecting codegen bugs.
3. Identifiers are **case-insensitive** (Pascal canon; the seed already lower-cases identifiers to derive deterministic labels, `seed/codegen.c:58-59`).
4. `for` supports both **`to` and `downto`** (each desugars to the primitive `while` form per the sugar rule above).

> **Note on scope tension with PRD Sec 6.7 (recorded; RESOLVED at ratification, OQ-1 option (b)).** PRD Sec 6.7 describes the Turbo Initech language as "a Turbo-Pascal-flavoured subset with `pointer`, `record`, inline `asm`, units." Of those four, this DEC-02 subset includes **records** but explicitly **defers pointers/heap, language-level inline asm, and units** past the self-host fixed point (per the `initech-qvtj` committee sizing and the `initech-9xu5` scope guard). This ADR treats PRD Sec 6.7's fuller list as describing Turbo Initech's **eventual/aspirational** language surface -- what the compiler may grow toward after M7/M8 -- rather than an M7/M8 requirement, since the qvtj analysis is the more granular, more recently ratified sizing input and this ADR's own charter is to fix the *self-host-critical-path* subset. The ratification committee resolved this as **OQ-1 option (b)** (2026-07-11): the staging clause is authored into PRD Sec 6.7 in this change-set ("of these, `record` is in the M7/M8 self-host-critical subset; `pointer`/heap, inline `asm`, and units are post-fixpoint additions per ADR-0007 DEC-02"), so the PRD and this ADR now agree explicitly. See Sec 7 OQ-1.

### 4.3 DEC-03 -- Complete Boolean Evaluation (Not Short-Circuit) + Order-Independence Discipline

Turbo Initech's `and`/`or` operators use **complete evaluation**: both operands are always evaluated, with no short-circuit. This is a deliberate divergence from Free Pascal's default (and from C's `&&`/`||`), decided at bead `initech-f0uc` (B1) and recorded here as binding.

**Consequence (the source-discipline half of the decision).** Because both operands always evaluate, a self-host source file must **never** write a guard pattern that depends on short-circuit protection, e.g. `if (i < n) and (arr[i] = x) then ...` is unsafe under complete evaluation if `i >= n`, because `arr[i]` is evaluated regardless of the first operand's value. Turbo Initech's own source (and any self-host-critical Pascal it compiles) must express such guards as **nested `if` statements** instead:

```pascal
if i < n then
  if arr[i] = x then ...
```

This is the **nested-if guard idiom**, and it is a **source-writing discipline**, not a codegen feature -- the compiler is not required to detect or rewrite unsafe boolean guards; the discipline is enforced by how Turbo Initech's own source is written (plan Sec 3: "self-host source written order-independent").

Two further order-independence idioms are canonical alongside nested-if (ratification amendment): **sentinel-returning accessors** -- an accessor that returns a sentinel instead of requiring a bounds guard at every call site, e.g. a `peek()` that returns `#0` at end-of-input, which is the seed's own lexer pattern (`seed/lexer.c` `peek`/`peek2`) -- and **boolean-flag loop exits** -- a `done` flag tested alone in the `while` guard instead of a compound condition mixing a bounds test with a dereference. Compound `while` guards are the common lexer case where a short-circuit habit would otherwise creep in; the three idioms together are the complete discipline.

The oracle for this decision is `bool.pas`, a hand-computed exact-serial truth-table fixture, plus a Rule-6 mutant that flips `setl`/`setg` in the relational codegen (bead `initech-f0uc`).

### 4.4 DEC-04 -- Single-Pass Stack-Machine Codegen, No Optimizer, Deterministic Output

Turbo Initech's code generator is **single-pass** and **stack-machine**: expressions evaluate onto a small, fixed register/stack discipline (the seed's own codegen establishes this precedent **at expression level, and at expression level only** -- every seed variable is a static `.bss` slot and expression evaluation uses `eax`/`ecx`/`edx` as a stack machine, per `docs/plans/TPS-M7-subset-plan.md` Sec 2 citing `codegen.c:14-78,286-361`; the seed has **no procedures**, so the frame/call discipline arrives new at B4 and is not inherited from any precedent). There is **no optimizer and no register allocator** (PRD Sec 6.7: "small and sufficient"). This is a Wirth-style single-pass design choice, not an oversight: Pascal was designed for single-pass compilation, and a compiler with no optimization passes is trivially easier to keep deterministic.

**Determinism is load-bearing, not a nicety** (Rule 11): symbol ordering, code layout, and every other codegen decision must be reproducible byte-for-byte given the same source, because the entire self-host certificate (DEC-06) is a bit-identity check. No timestamps, no host-path leakage, no nondeterministic iteration order (e.g. unordered hash-map traversal) may reach the emitted binary.

**Two concrete determinism mechanisms are binding (ratification amendment):**

- **Symbol tables are insertion-ordered arrays -- no hashing anywhere in the compiler.** Hash-order iteration is precisely the nondeterministic traversal Rule 11 forbids from reaching emitted bytes, and at this compiler's scale an insertion-ordered array is also the simplest thing that works.
- **ONE shared label/ordinal counter is threaded through emission and is never re-derived by a second, parallel pass.** The recorded anti-pattern is the seed's own `.rodata`/`.text` string-label scheme (`seed/codegen.c:113-116`), which walks the AST once to emit labelled string data and then **re-walks it in the same order to re-derive the ordinals** -- deterministic today, but only because two independent walks happen to agree; any future divergence between the walks silently mislabels. Turbo Initech threads the single counter through emission instead, so agreement is structural, not coincidental.

### 4.5 DEC-05 -- The RTL Boundary

Turbo Initech links against a **thin, hand-assembled runtime**, mirroring the `seed/rt/start.asm` pattern already proven in the seed (multiboot/entry setup, a minimal stack, serial helpers written in hand-assembled x86, with the compiled program body as the only codegen-emitted content). The RTL boundary is:

- A **hand-assembled entry stub** (not compiled from Pascal) that sets up the initial execution context and calls into the compiled program's entry point, exactly as `seed/rt/start.asm` does for the seed's output today.
- **File I/O over InitechDOS `INT 21h` handles**: `assign`/`reset`/`rewrite` plus byte/block read/write, mapped onto the handle-based file API (ADR-0003 Sec 5.4/5.6) -- bead `initech-ogxv` (B8).
- Explicitly **NOT** a full typed-file or `Text`-file machinery (no `readln`/`writeln`-with-formatting generality beyond what the seed's `write`/`writeln` helpers already provide, no variant-record typed files). The RTL is scoped to what Turbo Initech's own source needs to read its input and write its output, nothing more.
- The **static-arena memory convention** (DEC-02) is fixed at this boundary too: no heap allocator is provided by the RTL: all storage is static or stack-frame-resident.

**Binding boundary clarification (ratification amendment):** integer-to-decimal-string conversion -- which Turbo Initech needs both for emitting assembly text and for `line:col` error messages -- is **hand-written in-subset Pascal** (`div`/`mod` + `chr` + concatenation) inside Turbo Initech's own source. It is NOT an RTL service; the RTL stays byte/block I/O only.

This keeps the RTL small and auditable, consistent with DR-2 (minimality) and with Law 3 (no Pascal in the harness, no factory-only C leaking into the artifact) -- the RTL is artifact code (hand-assembled x86 shipped with Turbo Initech), not factory tooling.

### 4.6 DEC-06 -- The Self-Host Criterion: the Phi Fixed Point, `K2 == K3`, and DDC

Self-hosting correctness is defined exactly as PRD Sec 7 states it, and is restated here as the binding criterion for Turbo Initech (and reproduced, not paraphrased, because the exact formalism matters):

```
Phi : S -> S ,   Phi(K) = K(src)      ("run binary K on the canonical source -> new binary")

K1 = X(src)          # seed builds the resident compiler (cross)
K2 = K1(src)         # resident compiler rebuilds itself (now hosted on InitechOS)
K3 = K2(src)         # ...
require  K_{n+1} == K_n   (bit-identical)
```

**The certificate is the two-stage result: `K2 == K3` bit-for-bit, NOT `K1 == K2`.** `K1` is emitted by the seed -- a *different* compiler -- and may legitimately differ byte-wise from `K2`; that difference is expected and not a failure. The proof of self-reproduction is that the **resident** compiler, once it exists, reproduces itself exactly: `K2 == K3`. This scoping is restated explicitly because it is a documented hallucination risk (CLAUDE.md callout: "Self-host certificate is K2==K3, not K1==K2 -- and it scopes to Turbo Initech").

**Scope.** This criterion concerns **Turbo Initech and the Pascal programs it compiles only** -- never the C kernel/Toolbox, which is rebuilt by the factory cross-toolchain and has no fixed-point claim (PRD Sec 7 restates this scope note verbatim; DEC-01 above restates the same separation from the identity side).

**Preconditions.** `K2 == K3` is only a meaningful proof under **reproducible builds** -- no timestamps, deterministic symbol ordering (Rule 11, DEC-04). A nondeterministic build that happens to converge twice is not evidence of anything. This precondition is **wired, not aspirational** (ratification amendment): bead `initech-3yv` (reproducible-build discipline) carries it, and a `test-seed-repro` / `test-tps-repro` gate in the proven `test-kernel-repro` pattern (`Makefile:16467`: build twice, sha256-diff the binaries, mutation-proven) MUST exist before B4 (`initech-63ce`) closes -- see FO-5.

**The trusting-trust caveat and DDC.** A fixed point can harbour a self-perpetuating backdoor (Thompson, *Reflections on Trusting Trust*) that survives every self-compilation because the compiler quietly reinserts it. The in-scope antidote is **Diverse Double-Compilation** (Wheeler): compile `src` with an **independent** seed implementation and confirm the resulting `K*` matches the one produced by the primary seed. DDC is cheap and decisive at this project's scale; formal semantic-preservation proof assistants are explicitly out of scope (PRD Sec 7; PRD Sec 14's C-only decree; CLAUDE.md "Tool of last resort").

### 4.7 DEC-07 -- The Oracle Ladder

Correctness for the language subset (DEC-02) is graded by **three rungs**, escalating in strength as the codegen surface grows, per Law 2 (an oracle that never ran is not a signal) and Rule 6 (a golden must be mutation-proven):

**Rung 1 -- Hand-computed exact-serial fixtures (available now).** Each language family lands with a `.pas` fixture whose expected output is **hand-computed from the language definition**, run through the QEMU harness's `--expect` serial-output check -- the pattern already proven in `seed/examples/arith/negative_divmod.pas` (goldens are hand-derived from ISO 7185/Turbo Pascal semantics, not minted by an external compiler, and are verified GREEN under the real harness). This rung covers B1-B3 and remains the fallback oracle throughout.

**Rung 2 -- Free Pascal differential (stood up at the codegen pivot, B4).** Once procedures/functions/stack frames exist (bead `initech-63ce`, "THE CODEGEN PIVOT" -- the first construct complex enough that a hand-computed golden per case stops scaling), the **Free Pascal differential** is stood up: compile the same source with Turbo Initech and with `fpc`, run both outputs, diff (PRD Sec 6.7's stated oracle) -- with `fpc` invoked under `{$B+}`/`{$BOOLEVAL ON}` per the DEC-02 dialect pinning, so both compilers implement DEC-03's complete evaluation. From B4 onward, every subsequent family (B5-B8) gets an fpc-differential fixture alongside its exact-serial fallback.

**Rung 3 -- `test-compiler` is a real fpc-differential corpus gate (M7 close, B9; implemented B9.5).** At bead `initech-6m52` (B9), Turbo Initech's own source was authored in `os/tps/` in the subset landed by B1-B8, and `make test-compiler` became the real shared-corpus host gate vs Free Pascal. `test-compiler-os` executes every TPS-generated corpus program on InitechDOS and gates the PRD Sec 6.7/Sec 8 `differential_pass_rate` at 100%. The controlled 17-fixture register is `os/tps/CORPUS.md`. This discharges Rung 3 and closes M7 once the orchestrator certifies the emulator leg.

In both Rung 2 and Rung 3, the differential itself lives in **host harness tooling** (factory C + make glue comparing captured outputs of the two compilers' compiled programs) -- it is not a Turbo Initech language feature, and none of it ships in the artifact (Law 3; ratification amendment).

**The B-chain (`docs/plans/TPS-M7-subset-plan.md` Sec 4), for citation:**

| Step | Bead | Family | Oracle rung |
|---|---|---|---|
| B1 | `initech-f0uc` | Relational + boolean + `and`/`or`/`not`, complete-eval (DEC-03) | Rung 1 |
| B2 | `initech-80iw` | `if`/`while` (+ `for`/`repeat` sugar), ordinal labels | Rung 1 |
| B3 | `initech-7mo3` | `const` + `char` + `ord`/`chr` | Rung 1 |
| B4 | `initech-63ce` | Procedures/functions, stack frames, value/var params, recursion -- **the codegen pivot** | Rung 1 -> Rung 2 stood up here |
| B5 | `initech-54uu` | Static arrays + indexed l/r-value | Rung 2 |
| B6 | `initech-rug7` | Records + field access, deterministic layout | Rung 2 |
| B7 | `initech-39k2` | Fixed/`ShortString` strings | Rung 2 |
| B8 | `initech-ogxv` | Thin file-I/O RTL over `INT 21h` (DEC-05) | Rung 2 |
| B9 | `initech-6m52` | Author Turbo Initech in `os/tps/`; `test-compiler` becomes real -- **closes M7** | Rung 3 |
| B10 | `initech-9xu5` | Scope guard recording the DEC-02 deferrals (not an oracle step) | -- |

Note on chain order (ratification amendment): **B7 (strings) precedes B8 (file I/O)** because `assign()` takes a filename string -- the string type must exist before the file-I/O surface can be expressed.

**Mutation-proof (Rule 6) is a per-family obligation, not an afterthought.** Each family names its own deep-bug flag and mutant in its bead (e.g. B1's setl/setg flip, B4's var-param-aliasing and frame-offset mutants, B7's string-temporary-lifetime clobber, and B8's FILEIO-class mutant -- e.g. a short-write or wrong-handle fault -- recorded in `initech-ogxv`) -- consistent with the region-engine and FAT-diff precedent elsewhere in this project (ADR-0005 Sec 4).

### 4.8 DEC-08 -- IDE Scope (Resident Blue Screen, M8 Finale)

Turbo Initech's IDE is the **Turbo-Vision-style resident "blue screen"**: editor, compile, and run, running as a resident InitechOS app with file I/O via InitechDOS (PRD Sec 6.7). Its scope boundary is fixed explicitly against the M7/M8 milestone line (PRD Sec 11):

- **M7 (epic `initech-rnh`, this ADR's DEC-02 through DEC-07 above) is seed-built, not resident.** The seed (cross, on the host) builds Turbo Initech; Turbo Initech compiles a Pascal corpus and itself; the Free Pascal differential goes green (Rung 3, DEC-07). **No IDE work, and no resident execution of Turbo Initech, is in scope for M7.**
- **M8 (epic `initech-ls4`, "Resident IDE + self-host") is where the IDE ships.** Turbo Initech is ported *onto* InitechOS itself (the blue IDE, InitechDOS file I/O for the editor); the resident compiler compiles itself and the Pascal programs it compiles from inside the OS; the bit-identical two-stage fixpoint (`K2 == K3`, DEC-06) is verified for the **resident Pascal compiler**, never for the C kernel/Toolbox. The finale demo (PRD Sec 1.2/Sec 11 M8): boot the machine, open the IDE, load the Turbo Initech (Pascal) source, hit Compile, get a self-reproduced resident compiler.

Any IDE-adjacent work that appears before B9 (M7 close) is out of order against this scope line and should be flagged, not quietly absorbed (Rule 8).

---

## 5. Consequences

### 5.1 Binding Constraints (binding as of ratification, 2026-07-11)

- **C-1.** The two-compilers separation (DEC-01) is binding: no bead, commit, or design note may describe the seed as "the OS bootstrap" or describe the C kernel/Toolbox as subject to the self-host fixed point.
- **C-2.** The DEC-02 subset table (included / avoidable-with-idiom / deferred) is the **locked scope** for the self-host critical path. Pulling a deferred family (heap/pointers, enums, sets/`in`, units, reals, objects, language-level inline asm) into a pre-B9 lane requires a documented amendment to this ADR, not a silent scope creep (bead `initech-9xu5` exists to guard exactly this).
- **C-3.** Complete boolean evaluation (DEC-03) is fixed; Turbo Initech's own source, and any self-host-critical Pascal, must use the nested-if guard idiom wherever a guard would otherwise rely on short-circuit protection.
- **C-4.** Codegen determinism (DEC-04) is a hard requirement, not an optimization target: any nondeterministic codegen path (unordered iteration reaching emitted bytes, embedded timestamps, host paths) is a Rule-11 violation and blocks the self-host certificate.
- **C-5.** The RTL boundary (DEC-05) stays thin: no full typed-file/`Text`-variant machinery, no heap allocator, ships inside the RTL without a new ADR amendment.
- **C-6.** The self-host criterion is **`K2 == K3`**, scoped to Turbo Initech and the Pascal it compiles (DEC-06); no milestone may claim "self-hosting" on a `K1 == K2` comparison or on a C-OS rebuild.
- **C-7.** The oracle ladder (DEC-07) escalates in the fixed order (Rung 1 available now; Rung 2 stood up no later than B4; Rung 3 at B9/M7 close) -- a family may not claim "done" on a weaker rung than its position in the B-chain calls for.
- **C-8.** No IDE or resident-execution work is in scope before B9 (DEC-08); M8 is a distinct epic (`initech-ls4`) gated on M7 closing.

### 5.2 Forward Obligations

- **FO-1.** Land B1-B9 per `docs/plans/TPS-M7-subset-plan.md` Sec 4, each with its named oracle rung and mutant, closing M7.
- **FO-2.** Stand up DDC (an independent second seed implementation) before or alongside the M8 finale, per DEC-06 -- not deferred indefinitely, since the trusting-trust caveat is a named risk (PRD Sec 12).
- **FO-3.** (Discharged at ratification.) The PRD Sec 6.7 editorial pass this draft recommended is DONE: the committee resolved OQ-1 as option (b), and the staging clause is authored into PRD Sec 6.7 in this change-set (DEC-02 note; Sec 7 OQ-1).
- **FO-4.** (Discharged.) ADR-0003's three ADR-0007 citations (Sec 1.2, Sec 3 DR-5, Sec 9) point at this filed, ratified document; the sibling citations in ADR-0003-AMENDMENT-DEC-04a, ADR-0004, and ADR-0012, plus PRD Sec 6.7/Sec 14 and CLAUDE.md's "pending" mention, are updated to "ratified 2026-07-11" in the same change-set. No "(pending)" ADR-0007 citation remains in the corpus (Sec 2.1 enumerates every touched site).
- **FO-5.** (Ratification amendment.) Stand up a `test-seed-repro` / `test-tps-repro` reproducibility gate in the `test-kernel-repro` pattern (build twice, sha256-diff the binaries, mutation-prove the gate bites) BEFORE B4 (`initech-63ce`) closes -- bead `initech-3yv`, the DEC-06 precondition.

### 5.3 Neutral Consequences

- This ADR does not change ADR-0002's toolchain decision, ADR-0003's InitechDOS architecture, or CDR-0001's interim-toolchain deviation; it consumes all three as given.
- No change to the procurement of foam packaging inserts.

### 5.4 Risks

- **Scope-creep risk on the deferred families.** Mitigated by bead `initech-9xu5` and by C-2 above; the risk is recorded, not eliminated.
- **Nondeterminism risk in codegen.** Mitigated by DEC-04/C-4 and by the same reproducible-build discipline used elsewhere in the project (Rule 11).
- **Trusting-trust risk in the fixed point.** Mitigated by DDC (DEC-06); acknowledged as the accepted residual risk PRD Sec 7 already names.

---

## 6. Alternatives Considered

- **Target full TP7 parity (objects, units, heap, inline asm, real numbers) as the M7 subset.** Rejected: unsized, open-ended, and unnecessary -- Turbo Initech does not need to compete feature-for-feature with commercial Turbo Pascal 7 to reach a self-host fixed point; it needs to express *itself* (DR-2). The `initech-qvtj` gap analysis exists precisely to avoid this trap.
- **Short-circuit boolean evaluation (the Free Pascal / C default).** Rejected in favor of complete evaluation (DEC-03) at bead `initech-f0uc`; recorded here as binding rather than left as an implicit codegen detail, because it has a real source-discipline consequence (the nested-if idiom) that must be visible to anyone writing self-host-critical Pascal.
- **A multi-pass optimizing codegen.** Rejected: PRD Sec 6.7 and the Wirth single-pass precedent both favor "small and sufficient" over an optimizer whose determinism would itself need separate proof; a single-pass stack machine is easier to keep deterministic (DEC-04) and is what the seed already establishes.
- **A full Borland-style RTL (typed files, heap-backed strings, unit-scoped I/O).** Rejected as disproportionate to what Turbo Initech's own source needs (DEC-05); the thin `seed/rt/start.asm`-style boundary is sufficient and keeps the RTL auditable.
- **Treat `K1 == K2` as the self-host certificate.** Rejected: `K1` is emitted by the seed, a structurally different compiler, and byte-wise divergence from `K2` is expected, not a defect. `K2 == K3` is the only meaningful fixed-point claim (DEC-06); this is restated because it is a documented hallucination risk.
- **Skip DDC; rely on `K2 == K3` alone.** Rejected: a fixed point alone cannot rule out a self-perpetuating backdoor (Thompson); DDC is cheap enough at this project's scale that skipping it would be declining a real, low-cost antidote for no gain (PRD Sec 7).

---

## 7. Open Questions

- **OQ-1 -- PRD Sec 6.7 language-list staging. RESOLVED at ratification (option (b), committee 2026-07-11).** PRD Sec 6.7 is amended in this change-set to carry the staging clause explicitly: "of these, `record` is in the M7/M8 self-host-critical subset; `pointer`/heap, inline `asm`, and units are post-fixpoint additions per ADR-0007 DEC-02" -- so the PRD and DEC-02 now agree on their face. Original question (retained for the record): PRD Sec 6.7 lists `pointer`, `record`, inline `asm`, and units as part of the "Turbo-Pascal-flavoured subset," while DEC-02 (following the `initech-qvtj` sizing and the `initech-9xu5` scope guard) includes records but defers the other three past the self-host fixed point; the open options were (a) leave PRD Sec 6.7 as aspirational context, (b) amend it to state the staging explicitly, or (c) pull deferred families forward into the M7/M8 critical path. Option (c) was NOT taken -- DEC-02 and `initech-9xu5` stand unchanged.
- **OQ-2 -- DDC's independent seed.** DEC-06 commits to DDC as the trusting-trust antidote but does not fix *how* the second, independent seed is sourced (a from-scratch reimplementation? a different toolchain reusing the same grammar? PRD Sec 14's C-only decree applies either way). Left to an M8-era design note or bead, not resolved here.
- **OQ-3 -- IDE design detail.** DEC-08 fixes the M7/M8 scope *boundary* for the IDE but not its pixel-level design (menu layout, editor UX, integration with FLAIR's Window/Menu/Dialog managers per ADR-0004). Left to a future M8-era ADR or plan document once M7 closes.

---

## 8. Related Decisions and References

- **ADR-0002 -- Toolchain, Implementation Language, and Executable Format.** Establishes C as the OS implementation language and the seed as C; this ADR's DEC-01 rests on that separation.
- **ADR-0003 -- InitechDOS Base OS Personality.** Sec 1.2, Sec 3 (DR-5), and Sec 9 carried the "ADR-0007, pending" citation since its ratification; all three are updated in this change-set to cite this filed, ratified document (Sec 5.2 FO-4 above). The `INT 21h` handle-based file API (Sec 5.4/5.6) is the surface DEC-05's RTL boundary sits on.
- **CDR-0001 -- Toolchain Conformance Deviation.** The interim host-`gcc` toolchain the seed itself builds under; unaffected by this ADR.
- **PRD Sec 1.2, Sec 4, Sec 6.7, Sec 7, Sec 11 (M7/M8), Sec 14.** The primary spec this ADR ratifies against; see Sec 2.3 above for the full citation list.
- **`docs/plans/TPS-M7-subset-plan.md`.** The sizing analysis (bead `initech-qvtj`, closed) this ADR ratifies; its Sec 5 scope paragraph is the direct source of DEC-01 through DEC-08.
- **ADR-0005 -- ATKINSON Region Engine.** Cited here only as the project's precedent for "oracle ladder + mutation-proof per family" (DEC-07 mirrors its Sec 4 discipline), not otherwise related.
- beads: `initech-79s` (this ADR), `initech-qvtj`, `initech-znb`, `initech-3yv` (reproducible-build discipline, DEC-06/FO-5), epics `initech-rnh`/`initech-ls4`, and the B-chain `initech-f0uc/80iw/7mo3/63ce/54uu/rug7/39k2/ogxv/6m52/9xu5`.

<!-- END OEA-ADR-0007 (RATIFIED 2026-07-11, THREE-SEAT COMMITTEE, AMENDMENTS APPLIED; OPERATOR COUNTER-SIGNATURE OPEN) -- INITECH CONFIDENTIAL -->
