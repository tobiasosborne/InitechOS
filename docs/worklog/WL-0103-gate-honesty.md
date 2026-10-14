# WL-0103 - Reference gate honesty (initech-iv8c, initech-ehz4)

## Context

Lane based on 9cc1848. CLAUDE.md Law 2 and PRD Sections 6.2, 6.4, 6.6,
6.7 and 8 require independent oracles. Several absent-corpus branches still
returned success after a LOUD-SKIP. Both independent SAMIR readers also ignored
DBASE3_DECOMP and anchored their selftests to the repository's sibling corpus.
The existing SKIP_FPC / SKIP_BOCHS pattern is the policy reference.

## What changed

- dbf_ref.py and ndx_ref.py honor absolute and caller-relative DBASE3_DECOMP.
  Their repository-relative sibling path is only the default. Make exports all
  four corpus roots and the explicit opt-outs. fat12_ref.py / fat16_ref.py take
  file arguments and have no corpus-root hard-coding; wk1_canon.py already uses
  LOTUS123_DECOMP. No local wk1_ref.py, wp5_ref.py, WordPerfect corpus variable,
  or WordPerfect gate exists at this base.
- Missing System 7, Windows 3.1, or dBASE goldens now FAIL LOUD, naming the root
  and its sole opt-out: SKIP_SYSTEM7_DECOMP=1, SKIP_WIN31_DECOMP=1, or
  SKIP_DBASE3_DECOMP=1. Lotus already failed on missing goldens; it now names
  SKIP_LOTUS123_DECOMP=1 and supports that shouting opt-out only when absent.
- oracle_policy.h and missing_reference.sh enforce the C and thin-shell
  missing-reference paths. SKIP_REFERENCE_TOOLS=1 explicitly opts out of the
  formerly optional Python-reader, ASAN, and mtools extraction legs. A reader
  that runs and fails, corrupt goldens, and harness I/O failures stay RED.
- test-clut-mutant and the color-canon mutants require the unmutated baseline,
  so a missing golden cannot count as a killed mutant. Opted-out color legs
  cannot claim a mutation proof. GDI's missing-wine mutant branch follows the
  same fail-loud policy.
- Corpus configuration invalidates Chicago/CLUT/color/GDI binaries when a root
  changes; wine source presence is tracked too. This prevents cached binaries
  from concealing a changed root. Configuration knobs are defined before use;
  no test-makefile-vars allowlist relaxation was needed.
- The dormant test-interp-set Tier-1 path used nonexistent Sample_Data. It now
  reads the same CLIENTS/CNAMES goldens as the other SAMIR tests under
  Sample_Programs_and_Utilities, enabling its existing SET ORDER assertions.
- test-gate-honesty invokes four real host gates with empty corpus roots,
  checks their nonzero status AND named diagnostics, then checks the four
  successful, shouting explicit opt-outs. Its mutant compiles Chicago with
  the old LOUD-SKIP-to-green branch and requires the same honesty check to
  reject it for the named reason. Both targets are in TEST_UNIT_GATES.
- CLAUDE.md was not edited; proposed Build & test lines are in the handoff.

## Why

Printing a skip message does not turn an unexecuted reference comparison into
an oracle. The policy changes only dependency handling, preserves every
comparison and tolerance, and makes the opt-out visible and deliberate.
The positive runs below use the real independent corpora; mutation checks
still catch their original engine/value errors.

## Frictions

- bd was unavailable by lane instruction. No tracker operation or push was
  attempted. Git metadata was copied once into build/lane.git; commits and the
  lane-iv8c branch use that copy. Original .git was not changed.
- The local-only preview.webp frame skip in test-assets is outside the named
  corpus-root/reference-tool task; the frame is absent in this clone. That
  gate was not changed or run. Unminted/GATED behavior notices and Tier-2
  real-DBASE.EXE authenticity notices were read and left as existing scope
  declarations; they are not missing-root branches for implemented oracles.
- test-fatwrite's absent-mtools branch was changed but the emulator gate was
  not run, per lane constraints. Its shell policy was exercised separately.
- No Bochs, make test, make test-unit, or make test-emu was run. This is targeted
  acceptance, not a new full-vector certificate. FPC's existing B8 NOTICE
  remains: file_shared seed execution requires the absent OS runner.

## Acceptance

All make invocations used at most -j2. SAMIR host/mutant targets ran one target
at a time to avoid their existing fixed temporary-file collisions. Tests with
explicit roots used absolute paths resolved from the sibling corpora; no host
path is committed here.

Path proof:

- make test-samir DBASE3_DECOMP=<absolute corpus root> passed here AND from
  build/path-probe, where ../dbase3-decomp does not exist. The scratch directory
  contains only symlinks to source directories/Makefile and its build outputs.
  Evidence in both runs: `>>> test-samir: SAMIR foundation green (PAL + rt + value + host binding + spec lock + dbf_ref + ndx_ref)`.
- Both readers passed from that nested cwd with the relative override
  DBASE3_DECOMP=../../../dbase3-decomp. Evidence:
  `dbf_ref selftest: 147 checks, 0 failures` and
  `ndx_ref selftest: 116 checks, 0 failures`.
- Both standalone readers returned 2 with an empty DBASE3_DECOMP and emitted
  `!!! DBASE3_DECOMP FAIL:` plus `SKIP_DBASE3_DECOMP=1`. make test-samir with
  that empty override also returned nonzero with the same named policy.
- Direct empty-root/opt-out checks passed for all 18 corpus-dependent SAMIR
  host binaries: `dependency-negative: 18 SAMIR corpus legs RED on empty root; 18 named opt-outs shout`.
- Hiding python3 through an empty PATH passed the negative/opt-out checks for
  test_dbf_roundtrip, test_dbf_mutate, test_dbase_roundtrip, and prog_diff:
  `dependency-negative: 4 independent-reader legs RED without python3; 4 named tool opt-outs shout`.
  A fake python3 returning 1 stayed RED with the opt-out enabled:
  `dependency-negative: 4 failing-reader legs remain RED with SKIP_REFERENCE_TOOLS=1`.
- The honesty test was RED before the policy change:
  `!!! test-gate-honesty FAIL: test-chicago-metrics accepted missing SYSTEM7_DECOMP`.

New gates (final evidence):

- make test-gate-honesty:
  `>>> test-gate-honesty: green (4 real corpus gates reject missing references; 4 explicit opt-outs shout)`.
- make test-gate-honesty-mutant:
  `>>> test-gate-honesty-mutant: green (old Chicago LOUD-SKIP-to-green correctly RED on missing SYSTEM7_DECOMP)`.

The four representatives are test-chicago-metrics (SYSTEM7_DECOMP),
test-region-gdi (WIN31_DECOMP), test-samir-dbf-ref (DBASE3_DECOMP), and
test-i123-wk1-roundtrip (LOTUS123_DECOMP). Each negative log names its missing
root and SKIP_<root>=1. Each opt-out log contains
`!!! <root> SKIPPED (SKIP_<root>=1 opt-out)`.

Positive targeted gates and original mutation proofs (43 SAMIR targets):

| Make target | Final evidence line |
| --- | --- |
| `test-dbf-header` | `>>> test-dbf-header: green` |
| `test-dbf-fields` | `>>> test-dbf-fields: green` |
| `test-dbf-read` | `>>> test-dbf-read: green` |
| `test-dbf-roundtrip` | `>>> test-dbf-roundtrip: green` |
| `test-dbf-mutate` | `>>> test-dbf-mutate: green` |
| `test-ndx-parse` | `>>> test-ndx-parse: green` |
| `test-ndx-keys` | `>>> test-ndx-keys: green` |
| `test-ndx-seek` | `>>> test-ndx-seek: green` |
| `test-ndx-build` | `>>> test-ndx-build: green` |
| `test-ndx-maintain` | `>>> test-ndx-maintain: green` |
| `test-dbt-read` | `>>> test-dbt-read: green` |
| `test-dbt-roundtrip` | `>>> test-dbt-roundtrip: green` |
| `test-interp-use` | `>>> test-interp-use: green` |
| `test-interp-nav` | `>>> test-interp-nav: green` |
| `test-dbase-roundtrip` | `>>> test-dbase-roundtrip: green` |
| `test-interp-list` | `>>> test-interp-list: green` |
| `test-interp-set` | `>>> test-interp-set: green` |
| `test-use-rw` | `>>> test-use-rw: green` |
| `test-xbase-fn-d` | `>>> test-xbase-fn-d: green` |
| `test-dbase-diff` | `>>> test-dbase-diff: green` |
| `test-dbf-header-mutant` | `>>> test-dbf-header-mutant: green (reclen off-by-one correctly RED)` |
| `test-dbf-fields-mutant` | `>>> test-dbf-fields-mutant: green (48-byte stride correctly RED)` |
| `test-dbf-read-mutant` | `>>> test-dbf-read-mutant: green (record-offset +1 correctly RED)` |
| `test-dbf-roundtrip-mutant` | `>>> test-dbf-roundtrip-mutant: green (0x03-for-memo correctly RED)` |
| `test-dbf-mutate-mutant` | `>>> test-dbf-mutate-mutant: green (delete-flag 0x20-for-0x2A correctly RED)` |
| `test-ndx-parse-mutant` | `>>> test-ndx-parse-mutant: green (group sublayout swap correctly RED)` |
| `test-ndx-keys-mutant` | `>>> test-ndx-keys-mutant: green (sign-flip correctly RED)` |
| `test-ndx-seek-mutant` | `>>> test-ndx-seek-mutant: green (wrong-child descent correctly RED)` |
| `test-ndx-build-mutant` | `>>> test-ndx-build-mutant: green (50/50 split correctly RED)` |
| `test-ndx-maintain-mutant` | `>>> test-ndx-maintain-mutant: green (no-sorted-insert correctly RED)` |
| `test-dbt-read-mutant` | `>>> test-dbt-read-mutant: green (511-byte block geometry correctly RED)` |
| `test-dbt-roundtrip-mutant` | `>>> test-dbt-roundtrip-mutant: green (BE next-free ptr correctly RED)` |
| `test-interp-use-mutant` | `>>> test-interp-use-mutant: green (SELECT misroute correctly RED)` |
| `test-interp-nav-mutant` | `>>> test-interp-nav-mutant: green (SKIP off-by-one correctly RED)` |
| `test-dbase-roundtrip-mutant` | `>>> test-dbase-roundtrip-mutant: green (un-masked date correctly RED)` |
| `test-interp-list-mutant` | `>>> test-interp-list-mutant: green (CONTINUE re-scope correctly RED)` |
| `test-interp-set-mutant` | `>>> test-interp-set-mutant: green (EXACT default-ON correctly RED)` |
| `test-use-rw-mutant` | `>>> test-use-rw-mutant: green (open-RW-but-read-only correctly RED)` |
| `test-xbase-fn-d-mutant` | `>>> test-xbase-fn-d-mutant: green (RECNO-at-EOF correctly RED)` |
| `test-dbase-diff-mutant` | `>>> test-dbase-diff-mutant: green (=-direction swap correctly RED)` |
| `test-dbase-diff-mutants` | `>>> test-dbase-diff-mutants: green (ALL SIX corpus goldens mutation-proven)` |
| `test-dbf-read-openrw-hdrlen-mutant` | `>>> test-dbf-read-openrw-hdrlen-mutant: green (eager +1-at-open-time correctly RED)` |
| `test-ndx-maintain-span-mutant` | `>>> test-ndx-maintain-span-mutant: green (no-equal-key-run-walk correctly RED)` |

Other targeted acceptance:

| Make target | Final evidence line |
| --- | --- |
| `test-chicago-metrics` | `>>> test-chicago-metrics: all 931 checks green` |
| `test-chicago-metrics-mutant` | `>>> test-chicago-metrics-mutant: green (ONE_ADVANCE correctly RED on the perturbed glyph -- the oracle bites)` |
| `test-clut` | `>>> test-clut: green` |
| `test-clut-mutant` | `>>> test-clut-mutant: green (mutant correctly RED)` |
| `test-color-canon` | `>>> test-color-canon: green` |
| `test-color-canon-mutant` | `>>> test-color-canon-mutant: green (CANON_MUTATE_HILITE correctly RED -- LEG E title-bar bites)` |
| `test-color-canon-mutant-plat` | `>>> test-color-canon-mutant-plat: green (CANON_MUTATE_PLAT correctly RED -- LEG F sampled-domain golden bites)` |
| `test-region-gdi` | `>>> test-region-gdi: green` |
| `test-region-gdi-mutant` | `>>> test-region-gdi-mutant: green (RGN_MUTATE_PARITY_OFF1 correctly RED -- L2 cross-redden)` |
| `test-region-mutant` | `>>> test-region-mutant: green (NO_XMERGE_CAP mutant correctly RED under ASAN -- xmerge scratch-overrun caught)` |
| `test-i123-wk1-roundtrip` | `>>> test-i123-wk1-roundtrip: green` |
| `test-i123-wk1-roundtrip-mutant` | `>>> test-i123-wk1-roundtrip-mutant: green (both mutants RED for the named reason)` |
| `test-makefile-vars` | `>>> test-makefile-vars: green` |
| `test-spec` | `VERDICT   : PASS -- ADR-0003 spec-data parses, sizes hold, banner verbatim, INT 25h/26h locked` |
| `test-seed-fpc-diff INITECH_QMP_STDIO=1` | `>>> test-seed-fpc-diff: green -- four direct-QEMU seed/fpc diffs + file_shared fpc host round-trip + seed compile/link` |

Default sibling-root runs of test-chicago-metrics, test-clut, test-region-gdi,
and test-i123-wk1-roundtrip also passed after switching back from absolute
roots. test-interp-set now reports `test_interp_set: 57 checks, 0 failures`.
The thin-shell policy also passed direct absence/opt-out probes:
`shell-policy: green (5 missing references RED; 5 explicit opt-outs shout)`.
Shell syntax, Python compilation, git diff --check, and ASCII/host-path scans
of all changed/new source files passed.

## Pointers

- Makefile configuration header, corpus-config stamp, test-gate-honesty targets,
  and TEST_UNIT_GATES.
- harness/oracle_policy.h; harness/missing_reference.sh;
  harness/test_gate_honesty.sh.
- harness/diff/dbf_diff/dbf_ref.py and ndx_ref.py.
- harness/diff/dbf_diff/test_interp_set.c (restored SET ORDER corpus leg).
- Local evidence: build/ehz4-*.log, build/honesty-*.log,
  build/dependency-negative/, build/test-*.log, build/targeted-*.log.
- Delivery: build/lane.bundle, range 9cc1848..lane-iv8c; verify with
  git bundle verify build/lane.bundle in a repository containing 9cc1848.
