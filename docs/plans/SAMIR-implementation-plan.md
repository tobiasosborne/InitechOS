<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# SAMIR (InitechBase) -- Granular Swarm-Ready Implementation Plan

**Issuing Body:** Initech Systems Corporation -- Platform Engineering, Data Services Section (SAMIR)
**Document Class:** Implementation Plan (living; supersede in place)
**Programme / Milestone:** InitechOS (STAPLER) -- M6, InitechBase, codename SAMIR (PRD §6.6, §11)
**Governing Epic:** `initech-586`. Engine lattice: `aul / 7az / ahu / gmo / 0tl / 17n / ax9` + `586.1-.4`.
**Status:** DRAFT -- the architecture (§2) is **proposed ADR-0008, pending operator ratification**. No code ships until §2 is ratified and the §3 spec-data is locked. (Mirrors the ADR-0004/0005 "draft awaiting ratification" pattern.)
**Last Reconciled:** 2026-06-17

> Read order for an implementing agent: `CLAUDE.md` (Laws/Rules) -> PRD §6.6/§8/§11-M6/§14 -> `docs/research/dbase-ground-truth.md` (the M6 evidence base) -> **the sister corpus `../dbase3-decomp`** (byte-verified specs + minted ground truth + goldens) -> this plan -> `bd show initech-586` and the step's bead. The oracle is the truth, not this plan (Law 2): where a step is tagged **GATED**, the byte/behavior is NOT settled here -- mint or acquire it first.

---

## 1. Executive summary

SAMIR is a **dBASE III PLUS-compatible database that really runs**, in **C** (ADR-0002; it is a bundled app, not Pascal -- Pascal is Turbo Initech only). This plan decomposes the coarse `586` lattice into **~40 steps of <=~200 LOC each**, each with a contract, a dependency set, a parallel group, and its own mechanical oracle. Every step leaves `make test` green and is independently shippable, so the plan is valid **both** as a parallel swarm DAG **and** as a strictly serial backlog (for quota-/rate-limited solo runs).

Four bets drive the design:

1. **The sister repo `../dbase3-decomp` is the grader.** Its byte-verified specs become our locked spec-data; its "Verification" byte-dumps become committed assertion manifests; its `goldens/` are the differential corpus; its live `re/` dosbox-x mint harness is the golden-minting facility. We adapt it, we do not re-derive it (Law 1).
2. **Platform-agnostic via a narrow Platform Abstraction Layer (PAL).** The engine touches the OS only through `pal.h`. `pal_host.c` (factory) runs the whole engine + every oracle on the host at full speed with **zero** dependency on the kernel/tracer/FLAIR; `pal_milton.c` (artifact) binds InitechDOS INT 21h. If InitechDOS drifts or reverts, only `pal_milton.c` changes -- the engine and all oracles are untouched. This is both the portability guarantee the operator asked for **and** the fast-feedback loop for the swarm.
3. **dBASE III PLUS 1.1 ONLY for M6** (operator decision, 2026-06-17). We have III+ 1.1 goldens + the corpus + a live mint harness; we have **no** dBASE IV goldens. IV-only features (`.mdx`, `F`, `==`, `0x8B` memo) are **OUT OF SCOPE for M6** -- deferred to a post-M6 IV epic, never half-built (Law 1; corpus CLAUDE.md Law 3). The lexer rejects `==` as a III+ error; the codecs reject IV version bytes (`0x04`/`0x8B`) as unsupported, fail-loud.
4. **The corpus's MINT sessions already closed the brief's biggest unknowns** (§3.3), so `gmo`'s coercion table and the NDX numeric-key encoding can be **locked now**, not left DRAFT.

---

## 2. Architecture decisions (proposed ADR-0008 -- pending ratification)

### 2.A Storage layout (the "where does the dbase dev live" answer)

Three trees, by the artifact/factory split (Law 3):

```
os/samir/                      THE ENGINE -- freestanding C; depends ONLY on pal.h (the artifact)
  include/samir/               public contracts (the headers in §8)
    pal.h value.h rt.h dbf.h dbt.h ndx.h eval.h interp.h workarea.h
  core/    value.c coerce.c eval.c lex.c parse.c rt.c fn_*.c   (NO I/O -- pure)
  fs/      dbf.c dbt.c ndx.c   (+ mdx.c later)                 (PAL byte-I/O only)
  cmd/     interp.c use.c nav.c flow.c query.c replace.c set.c proc.c
  ui/      say_get.c read.c browse.c                           (PAL terminal only)
  pal/     pal_host.c          (factory/oracle binding; libc + injectable clock)
           pal_milton.c        (the artifact binding; InitechDOS INT 21h)
  samir_main.c                 (the dot-prompt REPL; links engine + one PAL)

harness/diff/dbf_diff/         THE GRADER -- C factory, mirrors harness/diff/fat_diff/
  dbf_dump.c                   ARTIFACT dumper (mounts via the REAL os/samir reader)
  dbf_ref.py ndx_ref.py        INDEPENDENT python readers (the "third implementation")
  dbf_coerce_fuzz.c            C property-test coercion fuzzer + shrinker
  xbase_prog_diff/             shared .prg corpus + differential driver
  test_*.c                     host oracles (header/fields/read/roundtrip/keys/seek/...)
  fixtures/                    committed deterministic .prg + seed schemas + assertion manifests

spec/samir/                    LOCKED spec-data, imported/derived from ../dbase3-decomp
  dbf_format.h ndx_format.h    byte-offset constants (from dbf.md / ndx.md)
  xbase_coercion.json          the machine-consumable coercion table (corpus coercion-table.md §6)
  dbf_normalization.json       meaningful-vs-normalize byte map (bead 586.3)
  dbase_msg_codes.tsv          the 151-code DBASE.MSG error table (corpus session 004)
  README.md                    provenance ledger (which corpus file each row came from)
```

**Goldens are NOT committed** (copyrighted abandonware -- same stance as `spec/assets/preview.webp`). The harness resolves `DBASE3_DECOMP ?= ../dbase3-decomp` and reads `$(DBASE3_DECOMP)/goldens/...`. Three tiers of authority, decreasing portability:

- **Tier 0 (committed, operator-free, day-1):** our derived **assertion manifests** in `fixtures/` -- the expected normalized values, encoded from the corpus's verified byte-dumps. These are *our synthesized work*, ASCII, committed, and gate with no external dependency.
- **Tier 1 (golden-diff):** raw corpus goldens referenced by path. If `$(DBASE3_DECOMP)/goldens` is absent, these legs print a **loud skip** naming the missing fixtures (never a silent pass -- the 86Box-leg discipline) and the gate proceeds on Tier 0.
- **Tier 2 (authenticity / minting):** new goldens minted by the sister repo's `re/harness-setup.md` dosbox-x recipe, plus the periodic real-`DBASE.EXE` differential. Operator-/env-gated.

Rationale: the bulk of grading (property tests, coercion fuzz, Tier-0 manifests) is portable and needs nothing external; the copyrighted bytes stay out of our git; the factory stays C-only (the python refs live in the C harness dir exactly as `fat12_ref.py` does today).

### 2.B The Platform Abstraction Layer (portability guarantee)

The engine's **only** OS surface is the `samir_pal` vtable (full contract in §8.1). It covers exactly what SAMIR needs: byte file-I/O (open/close/read/write/seek/remove/rename), cooked console out + line-in, an **injectable** clock (for reproducibility), and a fixed-arena allocator (no `malloc` on Milton). Two implementations:

- `pal_host.c` -- libc `stdio` + a host or **fixed** clock. Factory only; may use libc freely.
- `pal_milton.c` -- InitechDOS INT 21h handle API (`3Dh/3Eh/3Fh/40h/42h/3Ch/41h/56h`), `AH=2Ah` get-date, the CON functions, `AH=48h` arena. All of these exist in the kernel today (HANDOFF §4: WL-0008..0028). The engine **never** issues `int 0x21` directly.

Consequence (the operator's requirement): an InitechDOS drift/revert touches `pal_milton.c` and nothing else. The engine, the format codecs, the interpreter, and **every host oracle** are insulated. Conversely, the engine is fully developable and gradable before the kernel/GUI legs exist (Phases 0-7 are host-only).

### 2.C Targeting: dBASE III PLUS 1.1 ONLY (operator-decided 2026-06-17)

PRD §11-M6 names "dBASE IV", but our entire ground-truth base (corpus + goldens + mint harness) is **III+ 1.1**, and no IV golden exists. **Operator decision (2026-06-17): M6 targets dBASE III PLUS 1.1 ONLY; dBASE IV is dropped from M6 scope** and deferred to a post-M6 IV epic. Consequences: `.mdx` is **out of scope** (M6 ships `.ndx` only -- this narrows bead `ahu` to `.ndx`); `==`, `F` (float), and the `0x8B` IV memo dialect are out of scope; the lexer treats `==` as a III+ lex error; the codecs **fail loud** on IV version bytes (`0x04`/`0x8B`) rather than half-supporting them. This is a clean win: everything in scope is byte-grounded by the corpus + goldens, nothing is guessed.

### 2.D Engine / factory split (freestanding discipline)

`os/samir/core` and `os/samir/fs` compile **freestanding** (`-ffreestanding -nostdlib`, the kernel's interim toolchain, CDR-0001): no libc, only `rt.h` helpers + the PAL. The host oracles compile the *same* `.c` files with `pal_host.c` and a tiny freestanding-shim (or `gcc` hosted) for speed. This is exactly the kernel's pattern (`os/milton/*.c` + `harness/.../test_*.c` host oracles with mocks). No factory-only C leaks into the shipped engine (Law 3).

### 2.E Determinism & reproducibility (Rule 11)

The self-host-style fixed point does not apply to SAMIR, but the round-trip oracle does: SAMIR output must be **byte-deterministic** so goldens bite. Pins: injectable clock (header date is normalized anyway, but pinned for known expected values); fixed APPEND order before bulk `INDEX ON` (NDX node layout is insertion-ordered -- corpus env-3); deterministic field-address bytes (write `0`, normalize on diff); no timestamps in any artifact.

### 2.F Numeric strategy (flagged -- partial GATED)

Internal numeric rep = **IEEE-754 `double`** (corpus mint session 004, not BCD); dates = **Julian Day Number as a double**. On the target this needs x87 (period 386+387 / 486 has it); the kernel today has only a `#MF` panic handler and **no FPU init**, and there is **no `spec/hardware.json`**. So: a small PAL/kernel "FPU ready" step (§Phase 8), and the hardware contract gains an x87 line. Crucially, **all observable numeric behavior is confined to the decimal *formatter*** (the STR/`?`/N-field writer), which rounds per the minted rule (ties toward +inf) and is golden-tested -- so x87 80-bit-intermediate concerns never reach a gate. Bit-exact match of *computed* N values against real dBASE remains **GATED** (corpus numfn-7); it is not required for round-trip (the stored N field is formatted ASCII).

---

## 3. How we make best use of everything (the leverage map)

| Asset in `../dbase3-decomp` | How SAMIR uses it |
|---|---|
| `specs/file-formats/*.md` (byte-verified) | Source for `spec/samir/dbf_format.h` / `ndx_format.h` offset constants + the codec logic. |
| The **"Verification"** byte-dumps in each spec | Transcribed into Tier-0 **assertion manifests** (`fixtures/*.json`) -- the operator-free day-1 oracle. |
| `specs/language/coercion-table.md` §6 (`xbase_coercion.json`) | **Imported verbatim** as `spec/samir/xbase_coercion.json` (the contract the evaluator + fuzzer share). Minted-confirmed, so LOCKABLE now. |
| `GAPS.md` `[oracle-resolves]` / `[inferred]` items | The authoritative list of **GATED** steps (§7) -- 1:1 with our unclear-until-info markers. |
| `re/mint-results-00{1,2,3,4}.md` | Closed unknowns folded into locked decisions (§3.3) -- removes "DRAFT" from gmo + the NDX numeric key. |
| `goldens/*.dbf/.ndx/.dbt` (11 .ndx, 10 .dbf, ...) | Tier-1 differential corpus (read by path; loud-skip if absent). |
| `re/harness-setup.md` (live dosbox-x) | Tier-2 golden-minting for cases we lack (overflow `.dbf`, UNIQUE `.ndx`, IV files). |
| Confidence tags `[verified]/[documented]/[inferred]/[oracle-resolves]` | Drive **test strictness**: `[verified]` -> hard assertion; `[inferred]` -> soft/flagged; `[oracle-resolves]` -> loud skip (logged, never silent). |
| `oracles/harbour` etc. | Semantic cross-check for language behavior ONLY -- never an on-disk-byte authority (corpus Law 2). |

### 3.3 Corpus corrections that the brief left open (now locked)

The brief (`docs/research/dbase-ground-truth.md`) flagged these `oracle-resolves`; the corpus **minted** them. SAMIR adopts the minted truth:

- **NDX numeric keys = raw little-endian IEEE-754 doubles, NO sign-flip, compared ARITHMETICALLY** (corpus mint-001; brief §4.2 feared a transform). Date keys = **JDN as double**.
- **NDX node header = 2-byte count @0 + 2-byte filler @2**; **key-expr at 0x18, cap 100, verbatim case** (NOT 488/lowercased) (corpus ndx.md; brief §4.1/§4.2 was unsure).
- **`.dbf` header terminator: both `+1` (lone `0x0D`) and `+2` (`0x0D 0x00`) occur in genuine III+ 1.1** -- trust `header_length`, find field count by scanning to `0x0D` (corpus dbf.md).
- **`.dbt` block-0 next-free ptr = little-endian** for III+ (corpus dbt.md; brief §3.1 flagged the big/little contradiction).
- **`C+N` = error #9 "Data type mismatch."** (minted), no auto-stringification; **rounding ties -> +inf** (`STR(2.5,2,0)=3`, `STR(-2.5,2,0)=-2`); **internal rep = IEEE double**; **`==` and `DTOS` absent in III+** (lexing `==` is an error; `DTOS` -> error #31).
- **"570-" trailing-minus is application-level**, not a native dBASE PICTURE (native: leading `-`, `@(` parens, `@X` ` DB`, `@C` ` CR`). Law 4 canon note for InitechCalc, not a SAMIR default.

---

## 4. The harness (the grader) -- fast, structured, mutation-proven

Mirrors `harness/diff/fat_diff/` exactly (HANDOFF "Gotchas"; the proven structure: artifact dumper + independent reference + fuzzer/shrinker + fail-loud Make gate + mutation variants). Properties required of every SAMIR oracle:

- **Fast (<1 s where possible)** so agents get tight RED/GREEN loops.
- **Structured, localized error signal**, not pass/fail -- e.g. `dbf-header: CLIENTS off 0x0A reclen expected 106 got 105` or `coerce: (C,+,N) EXACT=OFF expected error#9 got type C [seed 0x8133]`. This is the feedback signal the swarm consumes.
- **Layered (Tier 0/1/2, §2.A)** -- cheapest, operator-free first; golden-diff next; authenticity last.
- **Mutation-proven (Rule 6):** every golden/manifest gets a `-mutant` gate that perturbs one constant in the engine and asserts RED for the *right* reason.
- **Fail-loud, never skip silently:** a missing tool/golden prints what is missing and why (the `fat-fault-rollback` idiom), and Tier-0 still runs.

Make targets (mirroring `test-fat`): umbrella **`test-dbase`** -> `test-dbf-*` / `test-dbt-*` / `test-ndx-*` / `test-xbase-*` / `test-interp-*` / `test-dbase-roundtrip` / `test-dbase-diff` / `test-xbase-coercion`, each with a `-mutant` sibling, all wired into `TEST_UNIT_GATES` (the WL-0028 "agents forget to wire the gate" lesson -- verify the gate-count delta).

---

## 5. The phased DAG

Notation per step: **id -- title** `[~LOC | deps | P:parallel-group | gate]`, then **Contract** (the interface it owns) and **Oracle** (how it is graded). `GATED` = blocked on new info; see §7. Phases 0-7 are **host-only** (no kernel/GUI dep). Phase 8 is the OS-coupled tail.

> **ARB-review riders (2026-06-17), binding on every step below:**
> (a) **Every `test-*` gate carries a mandatory `+mutant` sibling** (§4 / Rule 6), even where not re-stated in the step line; a step whose mutation-proof is structurally impossible must say so explicitly. The §10 pre-flight verifies no `test-*` ships without a mutant.
> (b) **The independent python readers `dbf_ref.py` (S6.1) and `ndx_ref.py` (S6.2) are pulled forward to run in Phase 0** (right after S0.5) -- they are the independence barrier for the Tier-0 manifests; until they exist the Phase 1-5 golden gates are "Tier-0 soft" (engine and manifest share `spec/samir/*_format.h`). Manifests cite corpus byte-dump offsets directly.
> (c) **Provenance pin:** `spec/samir/xbase_coercion.json` is sourced from `../dbase3-decomp/specs/language/coercion-table.md §6` (III+-only, no `==`); `dbf_normalization.json` from `../dbase3-decomp/specs/file-formats/dbf.md §2` (III+ table, `0x1C`/`0x1F` = NORMALIZE). The brief (`dbase-ground-truth.md`) is superseded for III+ codec/coercion provenance.

### Phase 0 -- Foundations & portability (host-only) -- decomposes new infra
- **S0.1 -- PAL contract `pal.h` + error enums** `[~120 | -- | P:A | compiles + pal_null stub]`
  Contract: the `samir_pal` vtable (§8.1); `pal_fd`, `PAL_RD/WR/RDWR/CREATE`, `pal_err`. Oracle: a `pal_null` impl links; header is self-contained.
- **S0.2 -- `pal_host.c` (libc + injectable clock)** `[~180 | S0.1 | P:B | test-samir-pal]`
  Contract: `pal_host_make(struct pal_host_cfg{date,root})`. Oracle: open/write/read/seek/close round-trip a temp file; injected date returned by `today()`.
- **S0.3 -- `rt.c` freestanding runtime** `[~200 | -- | P:A | test-samir-rt(+mutant)]`
  Contract (`rt.h`): `rt_mem*/rt_str*`, `jdn_from_ymd`/`ymd_from_jdn` (1900-2155), `dec_format(double,width,dec)` (ties->+inf), `dec_parse`. Oracle: JDN round-trips; `dec_format` matches the corpus STR manifest incl. the tie cases. Mutants (both RED): round-half-away (`STR(-2.5,2,0)`) AND round-half-to-even/banker's (`STR(2.5,2,0)` must be 3 not 2) -- ties go toward +inf, not away-from-zero and not banker's.
- **S0.4 -- harness skeleton + Make scaffolding** `[~150+mk | S0.1 | P:B | targets exist, stub-fail]`
  Contract: `harness/diff/dbf_diff/` dir; shared file-blockdev; `DBASE3_DECOMP` resolution + loud-skip macro; `test-dbase` umbrella. Oracle: stub honesty (gate targets exit non-zero until implemented).
- **S0.5 -- spec import + `test-samir-spec`** `[~data+120 | S0.1 | P:B | test-samir-spec]`
  Contract: `spec/samir/{dbf_format.h,ndx_format.h,xbase_coercion.json,dbf_normalization.json,dbase_msg_codes.tsv,README.md}`. Oracle: schema/consistency assertions (offsets monotonic, every error code 1..151 present, coercion rows reference defined rules) -- the `test-spec` idiom.
- **S0.6 -- `value.c` typed value** `[~150 | S0.3 | P:C | test-samir-value]`
  Contract (`value.h`): `xb_val` (C/N/D/L/M), ctor/dtor, `xb_typeof`, `xb_eq`, arena-backed C storage. Oracle: unit round-trips for each type.

### Phase 1 -- `.dbf` codec (host-only) -- decomposes `aul` (part 1)
- **S1.1 -- header parse + invariants** `[~160 | S0.6,S0.2,S0.5 | P:D | test-dbf-header(+mutant)]`
  Contract (`dbf.h`): `dbf_open(pal,name,*tbl)`, header struct; assert invariant-1/1b/2; handle `+1`/`+2` terminator; YY,MM,DD. Oracle: parse all corpus `.dbf`; Tier-0 manifest + Tier-1 goldens. Mutant: reclen off-by-one -> RED.
- **S1.2 -- field-descriptor array** `[~140 | S1.1 | P:D | test-dbf-fields(+mutant)]`
  Contract: `dbf_field(tbl,i)`; name-to-first-NUL; scan-to-`0x0D` for count. Oracle: descriptors of CLIENTS/TOURS/TRAVEL/BANK. Mutant: 48-byte (dBASE-7) stride -> RED.
- **S1.3 -- record read -> typed values** `[~180 | S1.2,S0.6 | P:D | test-dbf-read]`
  Contract: `dbf_read_rec(tbl,k,*rec)`; decode C/N/D/L + M-pointer; delete flag. Oracle: TOURS rec1 etc -> typed values vs `dbf_ref.py` + corpus dump.
- **S1.4 -- deterministic write + round-trip** `[~190 | S1.3 | P:D | test-dbf-roundtrip(+mutant)]`
  Contract: `dbf_create(pal,name,fields,n)`, `dbf_flush`; `+1` form, injectable date, optional `0x1A`; uses `dbf_normalization.json`. Oracle: SAMIR-write -> normalize -> `cmp` vs golden + python read-back (bidirectional). Mutant: emit `0x03` where `0x83` required -> RED.
- **S1.5 -- record mutation** `[~200 | S1.4 | P:D | test-dbf-mutate]`
  Contract: `dbf_append_blank`, `dbf_replace`, `dbf_delete`/`recall`, `dbf_pack`, `dbf_zap`; record-count bump. Oracle: append+replace+delete, read back; delete-flag `0x2A` (needs the minted `DTEST.DBF`, present in sister `mint/work`).

### Phase 2 -- `.dbt` memo (host-only) -- decomposes `aul` (part 2)
- **S2.1 -- `.dbt` III+ read** `[~160 | S1.3 | P:E | test-dbt-read]`
  Contract (`dbt.h`): `dbt_open`, `dbt_read(blockno,*buf,*len)`; LE block-0 next-free; 512-blocks; `0x1A 0x1A` term; 10-byte ASCII M pointer. Oracle: TOURS/TRAVEL `.dbt`.
- **S2.2 -- `.dbt` III+ write/append + round-trip** `[~170 | S2.1,S1.4 | P:E | test-dbt-roundtrip(+mutant)]`
  Contract: `dbt_append(text,len)->blockno`; `ceil((len+2)/512)` rounding; pointer write-back. Oracle: content-keyed, tail-normalized round-trip. Mutant: big-endian next-free ptr -> RED.
- **S2.3 -- `.dbt` dBASE IV (0x8B)** `[OUT OF SCOPE -- post-M6 IV epic]`
  Dropped from M6 per §2.C. III+ codec rejects `0x8B` fail-loud.

### Phase 3 -- Expression evaluator + coercion (host-only) -- decomposes `gmo` + `7az` eval core
- **S3.1 -- lexer** `[~190 | S0.6 | P:F | test-xbase-lex]`
  Contract (`eval.h`): `xb_lex(src)->tokens`; C/N literals, `.T.`/`.AND.` dotted forms, identifiers, operators, parens; reject `==` (III+). Oracle: token round-trip; `==` -> lex error.
- **S3.2 -- precedence parser -> AST** `[~200 | S3.1 | P:F | test-xbase-parse]`
  Contract: `xb_parse(tokens)->ast`; precedence unary > `^`(left-assoc) > `* /` > `+ -` > relational/`$` > `.NOT.` > `.AND.` > `.OR.`. Oracle: `2^3^2`=group-left, `-2^2` unary-binds-tighter (corpus mint-002).
- **S3.3 -- evaluator + operator coercion** `[~200 | S3.2,S0.5 | P:F | test-xbase-eval]`
  Contract: `xb_eval(ast,ctx,*out,*err)`; dispatch on `(lhs,op,rhs)` from `xbase_coercion.json`; SET EXACT-aware `C=`; D/N arithmetic; `$`; error#9. Oracle: the §3 dispatch-table cells as units.
- **S3.4 -- coercion fuzzer** `[~200 | S3.3 | P:F | test-xbase-coercion(+mutant)]`
  Contract: seeded PRNG -> random typed literals × ops × `{EXACT ON/OFF}`; shrink to minimal expr + replay seed; diff vs table-driven reference. This is `gmo`'s deliverable. Mutant: flip a table cell (make `C+N` succeed) -> RED.
- **S3.5 -- built-in functions A (bridges + core)** `[~200 | S3.3 | P:G | test-xbase-fn-a]`
  Contract: `fn_*` table; STR/VAL/CTOD/DTOC/UPPER/LOWER/TRIM/LTRIM/SUBSTR/LEN/SPACE/CHR/ASC/DATE/DAY/MONTH/YEAR/IIF/TYPE. Oracle: vs minted transcripts; `DTOS` -> error#31.
- **S3.6 -- built-in functions B (db + numeric/date)** `[~200 | S3.5,S5.1 | P:G | test-xbase-fn-b]`
  Contract: RECNO/RECCOUNT/EOF/BOF/FOUND/DELETED/FIELD/DBF/FILE; CDOW/CMONTH/DOW; INT/ROUND/MOD/ABS/MAX/MIN/SQRT/LOG/EXP. Oracle: vs transcripts. **Partial GATED:** MOD sign, INT-on-negatives, ROUND-tie, LOG/SQRT domain (corpus numfn-1..8 open) -> those cells loud-skip pending MINT.

### Phase 4 -- `.ndx` index (host-only) -- decomposes `ahu` (part 1); couples to Phase 3
- **S4.1 -- header + node parse** `[~180 | S0.5 | P:H | test-ndx-parse(+mutant)]`
  Contract (`ndx.h`): `ndx_open`; 10 header fields; `ceil4(keylen+8)` group, `(512-4)/group` kpp; 2+2 node header; group child/recno/key. Oracle: all 11 corpus `.ndx` (formulas + verbatim casing). Mutant: clicketyclick wrong 18-23 sublayout -> RED.
- **S4.2 -- key decode + collation** `[~150 | S4.1,S0.3 | P:H | test-ndx-keys]`
  Contract: char (CP437 byte order), numeric (**raw LE double, arithmetic compare**), date (JDN double). Oracle: TOURDATE JDN bytes + minted `NCOST.NDX` negatives (sister `mint/work`).
- **S4.3 -- B-tree search/traverse/SEEK** `[~190 | S4.2 | P:H | test-ndx-seek]`
  Contract: descent (first key>=target, rightmost child), in-order enumerate, SEEK w/ SET EXACT prefix, FOUND/EOF. Oracle: in-order == brute-force sort; SEEK resolves recno; property test over corpus.
- **S4.4 -- bulk `INDEX ON` build** `[~200 | S4.3,S3.3 | P:H | test-ndx-build(+mutant)]`
  Contract: `ndx_build(table,key_expr)`; eval key per record; pack leaves 100% L->R, remainder last; root = N keys / N+1 children. Oracle: rebuild CNAMES.NDX / minted BIGIDX.NDX -> normalize -> `cmp`. Mutant: 50/50 split -> RED.
- **S4.5 -- incremental maintenance** `[~180 | S4.4 | P:H | test-ndx-maintain]`
  Contract: APPEND/REPLACE update open index. Oracle: post-append SEEK correct (behavioral). **Partial GATED:** mid-leaf split byte-exactness is corpus-open (may be 50/50) -> behavioral only; byte-exact split loud-skips pending MINT.
- **S4.6 -- `.mdx` (dBASE IV)** `[OUT OF SCOPE -- post-M6 IV epic]`
  Dropped from M6 per §2.C (node bodies are undocumented anyway -- CDX != MDX). M6 ships `.ndx` only.

### Phase 5 -- Interpreter + commands (host-only) -- decomposes `7az`
- **S5.1 -- work-area model + USE/CLOSE** `[~190 | S1.5,S4.3 | P:I | test-interp-use]`
  Contract (`workarea.h`,`interp.h`): 10 work areas, SELECT/alias; open `.dbf`+`.dbt`+indexes; master order; RECNO=1. Oracle: scripted USE/SELECT.
- **S5.2 -- navigation** `[~150 | S5.1 | P:I | test-interp-nav]`
  Contract: GO/GOTO, SKIP, GO TOP/BOTTOM, EOF/BOF; physical vs index order. Oracle: walk in both orders.
- **S5.3 -- statement executor + control flow** `[~200 | S3.3 | P:I | test-interp-flow]`
  Contract: command dispatch; DO WHILE/ENDDO, IF/ELSE/ENDIF, DO CASE, LOOP/EXIT; guard-must-be-Logical (error, no truthiness). Oracle: nesting + guard-type errors.
- **S5.4 -- query/display** `[~200 | S5.2,S5.3,S4.3 | P:I | test-interp-list]`
  Contract: LIST/DISPLAY (scope/FIELDS/FOR/WHILE/OFF), `?`/`??`, LOCATE/CONTINUE, SEEK/FIND. Oracle: transcript vs minted golden; CONTINUE re-applies FOR not WHILE/scope.
- **S5.5 -- mutation verbs** `[~200 | S5.4,S1.5 | P:I | test-interp-replace]`
  Contract: REPLACE (scope/FOR/WHILE, **assignment-coercion**, index update, master-key pointer drift), APPEND, DELETE/RECALL/PACK/ZAP, STORE/`=`. Oracle: REPLACE matrix; overflow `*`-fill (minted).
- **S5.6 -- SET state** `[~180 | S5.3 | P:I | test-interp-set]`
  Contract: EXACT/DECIMALS/DATE/CENTURY/NEAR/ORDER/INDEX/FILTER/RELATION/TALK/SAFETY; defaults EXACT=OFF, DECIMALS=2, DATE=AMERICAN, CENTURY=OFF (minted). Oracle: default + override behavior.
- **S5.7 -- procedures + I/O** `[~200 | S5.3 | P:I | test-interp-proc]`
  Contract: DO/PROCEDURE/PARAMETERS/RETURN/DO WITH, PUBLIC/PRIVATE scope, ACCEPT/INPUT/WAIT, ON ERROR. Oracle: call/return; scope. **Partial GATED:** param by-ref exactness, uninitialized PUBLIC value, DO-name precedence (corpus §O open) -> loud-skip pending MINT.
- **S5.8 -- dot-prompt REPL `samir_main.c`** `[~150 | S5.5,S5.6,S0.2 | P:I | test-samir-repl]`
  Contract: read line via PAL, parse, execute, render via the 151-code error catalog. Oracle: scripted session over the host PAL.

### Phase 6 -- Differential oracle assembly -- decomposes `17n` + `586.4`
- **S6.1 -- `dbf_ref.py` (independent reader)** `[~py | S0.5 | P:B | agrees-with-corpus]`
  Contract: parse `.dbf`/`.dbt` from first principles, normalized dump (`--schema/--records/--memo`). Oracle: agrees with the corpus dump on all goldens. (Adapt `dbfread`/`python3-dbf` per brief §7.2; the index reader is authored.)
- **S6.2 -- `ndx_ref.py` (independent index reader)** `[~py | S6.1 | P:B | agrees-with-corpus]`
  Contract: parse `.ndx`, `--index-dump` (sorted keys + recnos), `SEEK`. Oracle: matches SAMIR traverse + brute-force.
- **S6.3 -- round-trip oracle** `[~180 | S1.4,S2.2,S4.4,S6.1,S6.2 | P:- | test-dbase-roundtrip(+mutant)]`
  Contract: bidirectional; value-diff + masked `memcmp` via `dbf_normalization.json`. Mutant: flip a normalization mask cell -> a passing round-trip goes RED (bead 586.3).
- **S6.4 -- xBase program differential** `[~200 | S5.8,S6.1 | P:- | test-dbase-diff]`
  Contract: shared `.prg` corpus; host reference-interpreter tier (day-1) + real-dBASE authenticity tier (sister mint). Diff normalized stdout + result `.dbf`; **gate 100%** (PRD §8). Mutant: flip `=` prefix direction -> corpus RED.
- **S6.5 -- golden-minting bridge** `[~mk | sister repo | P:- | GATED-env]`
  Contract: a Make/C wrapper invoking the sister `re/harness-setup.md` dosbox-x recipe + determinism checklist to mint a named golden. **GATED-env:** needs dosbox-x (present) + real `DBASE.EXE` (sister goldens, gitignored).

### Phase 7 -- Canon apps (host-runnable) -- decomposes `586.1`, `586.2`
- **S7.1 -- Initech accounting app + Y2K bug** `[~200 .prg | S5.8 | P:- | test-canon-y2k; label:canon]`
  Contract: a real `.prg` over the engine; the **deliberate** 2-digit-year store/compare (`00 < 99`). Oracle: differential -- the *wrong* rollover matches real dBASE (Law 4; enforced, not fixed).
- **S7.2 -- Bolton salami/rounding-error virus** `[~200 .prg | S7.1 | P:- | test-canon-salami; label:canon]`
  Contract: skims the sub-cent rounding remainder; the canonical too-much-too-fast bug. Oracle: differential rounding-remainder skim against the in-universe app only.

### Phase 8 -- InitechDOS + FLAIR integration (OS-coupled tail)
- **S8.1 -- `pal_milton.c` + FPU-ready** `[~200 | 509(M2),S0.1 | P:- | test-pal-milton(emu); GATED-on-M2-stable]`
  Contract: INT 21h handle binding + `AH=2Ah` + CON + `AH=48h` arena + x87 init (+ `spec/hardware.json` x87 line). Oracle: a tiny SAMIR build reads/writes a FAT file under QEMU. (M2 surface is present today -- low gate risk.)
- **S8.2 -- SAMIR as a flat `.COM`-equivalent app** `[~150 | S8.1,loader | P:- | boot->USE->LIST screendump]`
  Contract: build/link SAMIR for Milton; run the REPL on the LFB text console (pre-GUI, interim). Oracle: in-emulator boot -> SAMIR -> USE -> LIST screendump (the Law-4 "look at the screendump" gate).
- **S8.3 -- SAMIR<->FLAIR text-console window (the `ax9` gate)** `[~? | 8oi(M4),0tl | P:- | GATED-GUI]`
  **GATED:** needs the FLAIR window/event surface (M4 not yet built): 80x25 CP437 cell grid, keyboard routing from the cooperative loop, GUI-exit path. The M6 `ax9` gate.
- **S8.4 -- `@SAY/GET/READ` forms (decomposes `0tl`)** `[~200 ×2-3 | S5.8,terminal PAL | P:J | test-say-get (host) ; full fidelity GATED-GUI]`
  Contract (`say_get.h`): the GET list, PICTURE/FUNCTION/RANGE/VALID/WHEN, READ loop + write-back. Oracle: form-render + edit-writeback transcripts host-side; full CP437/Turbo-Vision fidelity GATED on S8.3.

---

## 6. Parallelism & serial linearization

**Parallel groups (swarm).** Within a group, steps have no edge between them and can run concurrently in isolated worktrees (the WL-0028 pattern -- but keep `main` fast-forwarded; the `t6nc` stale-`main` worktree caveat applies). Cross-group edges are the deps above.

- After **Phase 0** completes, **Phase 1 (P:D)** and **Phase 3 (P:F)** run fully in parallel (codec vs evaluator are independent until the interpreter). **Phase 6 python refs (S6.1/S6.2)** can start immediately after S0.5.
- **Phase 2 (P:E)** starts after S1.3/S1.4. **Phase 4 (P:H)** needs S3.3 (keys are expressions) + S0.5.
- **Phase 5 (P:I)** is the convergence point (needs 1,2,3,4). Its steps are mostly serial on the work-area/executor spine but S5.6/S5.7 can branch off S5.3.
- **Phase 7** after S5.8. **Phase 8** host-testable parts (S8.4 forms) parallel-startable; integration gates (S8.1/S8.2/S8.3) wait on M2/M4.

**Serial linearization (quota-/rate-limited solo).** Each leaves `make test` green with its own gate:
`S0.1 -> S0.2 -> S0.5 -> S0.3 -> S0.6 -> S0.4` -> `S1.1 -> S1.2 -> S1.3 -> S1.4 -> S1.5` -> `S2.1 -> S2.2` -> `S3.1 -> S3.2 -> S3.3 -> S3.4 -> S3.5` -> `S4.1 -> S4.2 -> S4.3 -> S4.4 -> S4.5` -> `S5.1 -> S5.2 -> S5.3 -> S5.4 -> S5.5 -> S5.6 -> S5.7 -> S5.8` -> `S3.6` -> `S6.1 -> S6.2 -> S6.3 -> S6.4` -> `S7.1 -> S7.2` -> `S8.1 -> S8.2 -> S8.4`. **Out of M6 scope** (post-M6 IV epic): S2.3, S4.6. **Deferred/GATED:** S8.3 (needs M4), S6.5 (needs real dBASE for the authenticity tier).

---

## 7. GATED / unclear-until-info register

Each is marked in §5; resolution mechanism in brackets. **No agent guesses these.**

| id | what is unclear | resolve by |
|---|---|---|
| S3.6 | MOD-sign, INT-on-neg, ROUND-tie, LOG/SQRT domain | **MINT** (corpus numfn-1..8) |
| S5.7 | param by-ref exactness, uninitialized PUBLIC, DO-name precedence | **MINT** (corpus §O) |
| S4.5 | incremental mid-leaf split byte-exactness | **MINT** (corpus ndx incremental-insert; behavioral OK now) |
| S5.5 | REPLACE on overflow: abort the command vs store `*`-fill | **MINT** (the `*`-fill width IS minted/resolved for N4; only the abort-vs-store policy is open) |
| S5.2 | SKIP at EOF/BOF (error vs silent); GO to a record hidden by SET DELETED/FILTER | **MINT** (corpus GAPS §P) -- behavioral nav is fine now; these edges loud-skip |
| S8.1 | kernel x87/FPU init + `spec/hardware.json` x87 line | verify against the **hardware contract** (none exists yet) |
| S8.3 | SAMIR<->FLAIR window seam | **M4 (FLAIR)** must land first |
| S6.5 | real-dBASE authenticity tier | dosbox-x (present) + real `DBASE.EXE` (sister, gitignored) |
| (num) | bit-exact *computed* N vs real dBASE | confined to the formatter (golden-tested); raw-bit match is `oracle-resolves` |

---

## 8. Module contracts (the load-bearing interfaces)

### 8.1 `os/samir/include/samir/pal.h` -- the ONLY OS surface
```c
/* SAMIR Platform Abstraction Layer. Engine code touches the OS ONLY through this
   vtable. pal_host.c = factory/oracle (libc); pal_milton.c = artifact (INT 21h).
   Ref: brief sec 6.1 (handle API, not FCB); CLAUDE.md "platform-agnostic" steer. */
typedef int32_t pal_fd;                 /* >=0 ok; <0 = -pal_err */
enum { PAL_RD=0, PAL_WR=1, PAL_RDWR=2, PAL_CREATE=4, PAL_TRUNC=8 };
enum { PAL_SEEK_SET=0, PAL_SEEK_CUR=1, PAL_SEEK_END=2 };
typedef enum { PAL_OK=0, PAL_ENOENT=2, PAL_EACCES=5, PAL_EIO=29, PAL_ENOSPC=28 } pal_err;

typedef struct samir_pal samir_pal_t;
struct samir_pal {
  /* file (maps to INT 21h 3Dh/3Eh/3Fh/40h/42h/3Ch/41h/56h on Milton) */
  pal_fd  (*open)  (samir_pal_t*, const char *name, int mode);
  int     (*close) (samir_pal_t*, pal_fd);
  int32_t (*read)  (samir_pal_t*, pal_fd, void *buf, uint32_t n);
  int32_t (*write) (samir_pal_t*, pal_fd, const void *buf, uint32_t n);
  int32_t (*seek)  (samir_pal_t*, pal_fd, int32_t off, int whence);  /* -> new pos.
                       seek(fd,0,PAL_SEEK_END) IS the file-size primitive --
                       required by dbf_open's truncation invariant + FILE(). */
  int     (*remove)(samir_pal_t*, const char *name);
  int     (*rename)(samir_pal_t*, const char *from, const char *to); /* same-dir only */
  /* console (cooked) -- REPL + LIST */
  void    (*conout)(samir_pal_t*, const char *s, uint32_t n);
  int32_t (*conin_line)(samir_pal_t*, char *buf, uint32_t cap);      /* -> len, <0 EOF */
  /* terminal extension -- @SAY/GET/READ + WAIT/INKEY (S8.4). Part of the
     ratified contract (ADR-0008 DEC-02 rev) so ui/ need not re-open pal.h. */
  int32_t (*conin_char)(samir_pal_t*);            /* single raw key, no echo (Milton AH=07h) */
  void    (*gotoxy)(samir_pal_t*, uint8_t row, uint8_t col);   /* console primitive, NOT INT 21h */
  void    (*set_attr)(samir_pal_t*, uint8_t attr);             /* + clear-to-eol via conout */
  /* clock -- INJECTABLE for reproducibility (Rule 11); packed YY,MM,DD */
  void    (*today) (samir_pal_t*, uint8_t *yy, uint8_t *mm, uint8_t *dd);
  /* memory -- fixed arena (no malloc on Milton; AH=48h there) */
  void   *(*alloc) (samir_pal_t*, uint32_t n);
  void    (*reset) (samir_pal_t*, void *mark);                       /* bump-arena unwind */
};
```

### 8.2 `value.h` (typed value), `dbf.h`, `ndx.h`, `eval.h`, `interp.h` -- signatures
```c
/* value.h -- internal N = IEEE double, D = Julian Day Number as double (corpus mint-004) */
typedef enum { XB_C, XB_N, XB_D, XB_L, XB_M, XB_U } xb_type;   /* U = undefined */
typedef struct { xb_type t; union { struct{char*p; uint16_t len;} c;
                 double n; double d; uint8_t l; } u; } xb_val;

/* dbf.h */
typedef struct dbf_table dbf_table;
int  dbf_open  (samir_pal_t*, const char *name, dbf_table**);   /* asserts invariants */
int  dbf_create(samir_pal_t*, const char *name, const dbf_field *f, int n, dbf_table**);
int  dbf_read_rec(dbf_table*, uint32_t recno, xb_val *out /*[nfields]*/);
int  dbf_append_blank(dbf_table*);
int  dbf_replace(dbf_table*, int field, const xb_val*);         /* assignment-coercion */
int  dbf_flush (dbf_table*);                                    /* deterministic write */

/* ndx.h -- numeric/date keys decode to double, compared ARITHMETICALLY (corpus mint-001) */
typedef struct ndx_index ndx_index;
int  ndx_open (samir_pal_t*, const char *name, ndx_index**);
int  ndx_build(dbf_table*, const char *key_expr, const char *out_name);  /* bulk pack */
int  ndx_seek (ndx_index*, const xb_val *key, int set_exact, uint32_t *recno_out);

/* eval.h -- the load-bearing shared core (REPLACE, LOCATE FOR, IF/DO-WHILE, index keys) */
int  xb_eval(const xb_ast*, xb_ctx*, xb_val *out, int *err_code);  /* err 9 = mismatch */

/* interp.h */
int  samir_repl(samir_pal_t*, xb_interp*);     /* the dot-prompt loop */
int  samir_do  (xb_interp*, const char *prg);  /* run a .prg */
```

(Full headers are produced at S0.1/S0.6/S1.1/S3.3/S5.1; the above is the contract surface that fixes the module boundaries.)

---

## 9. Beads mapping (how the steps slot into `586`)

The existing children are the **phases**; the steps become their children (`bd create --parent`), preserving the existing dep edges and the brief's R1/R2 wiring:

| Existing bead | Becomes | Steps |
|---|---|---|
| (new) `586.pal` | Phase 0 portability + harness skeleton | S0.1-S0.6, S0.4 |
| `aul` | Phase 1 + Phase 2 | S1.1-S1.5 (.dbf), S2.1-S2.2 (.dbt III+) |
| `gmo` | Phase 3 eval+coercion | S3.1-S3.4 |
| `7az` | Phase 3 fns + Phase 5 interpreter | S3.5/S3.6, S5.1-S5.8 |
| `ahu` | Phase 4 index (`.ndx` only; `.mdx` out of M6 scope) | S4.1-S4.5 |
| `586.3` | already = `dbf_normalization.json` | consumed by S6.3 |
| `586.4` + `17n` | Phase 6 oracle | S6.1-S6.5 |
| `586.1`/`586.2` | Phase 7 canon | S7.1/S7.2 |
| `0tl`/`ax9` | Phase 8 forms + GUI gate | S8.4 / S8.1-S8.3 |

**Materialization is a deliberate act (Rule 8/9)** -- recommended only after §2 (ADR-0008) is ratified and `spec/samir/*` is locked, so the bead tree reflects a ratified architecture (the ADR-0004/0005 sequencing). Until then this doc is the authority; `bd show initech-586` points here.

---

## 10. Pre-flight checklist (before any Phase 0 code)

1. **Ratify proposed ADR-0008 (§2)** -- the PAL, the storage layout, III+1.1-first targeting, the numeric strategy. (ADR-by-committee for the contested calls, per CLAUDE.md "Core" tier.)
2. **Lock `spec/samir/*`** -- import the corpus artifacts; mutation-prove each (`586.3` discipline).
3. **Confirm `../dbase3-decomp` is the standing dependency** -- the harness resolves it by path; document the loud-skip-if-absent.
4. **ASCII-clean** all engine/harness source + JSON (Rule 12).
5. **Wire every new gate into `TEST_UNIT_GATES`** and verify the count delta (WL-0028 lesson); run `make clean && timeout 1200 make test` at each integration.

---

## Sources

Local (Law 1): `CLAUDE.md` (Laws/Rules, TDD shapes, file map); `InitechOS-PRD.md` §6.6/§8/§11-M6/§14; `docs/research/dbase-ground-truth.md` (the M6 brief, all sections); `docs/HANDOFF.md` §4 (M2 substrate, gate idiom); `harness/README.md` + `harness/diff/fat_diff/*` (the differential pattern mirrored here); `spec/README.md` + `spec/int21h_calling_convention.json` (house JSON conventions); `Makefile` (`test-fat*` gate machinery, `TEST_UNIT_GATES`); beads `initech-586` + children.

Sister corpus `../dbase3-decomp` (byte-verified + minted ground truth): `CLAUDE.md`/`README.md`/`INDEX.md`/`GAPS.md`/`SOURCES.md`; `specs/file-formats/{dbf,dbt,ndx}.md`; `specs/language/coercion-table.md` (§6 = the importable `xbase_coercion.json`); `re/harness-setup.md` (the live dosbox-x mint recipe); `re/mint-results-00{1,2,3,4}.md` (the closed unknowns folded into §3.3); `goldens/` (the Tier-1 differential corpus).

---

*-- End of Plan (DRAFT; architecture pending ADR-0008 ratification) --*

<!-- Tedium certified compliant with NFR-7. The Y2K bug and the rounding-error virus are enforced canon, not defects. -->
