PM round 4: EXP picture fix committed on lane/wave1-log as d7fdf7da (initech-1x6x2.5). LOG, SQRT, EXP and exppicture pass their host, mutant, QEMU and QEMU-mutant gates. The two DO-argument diagnostics remain RED pending the ordered NDX/main integration; their normal mutant certificates are blocked. Nothing was pushed.

1. Implementation. Imported main's existing EXP implementation from origin/main f374fe02 and kept its x87 block byte-identical. EXP now calls fn_math_picture, retaining the argument's fractional metadata while starting from xb_n's standard computed-Numeric picture. This reproduces EXP's independently captured convention: EXP(1) has 13 columns at DECIMALS 2, while EXP(1.00) and N(8,2) field arguments have 10. Fractional arguments retain precision beyond the SET DECIMALS minimum. The stored double is not pre-rounded.

Promoted exp and exppicture to accepted cases and default host/EMU vectors. The existing EXP numeric mutant also grades exppicture. Added EXP Reference/Quick Reference entries and the existing real-capture worked example from main. No corpus, GOLD.OUT, provenance or hash pins were changed; the old exp and expanded exppicture inputs/captures were already pinned.

Added explicit four-leg targets for logdoarg/sqrtdoarg outside default acceptance vectors, plus test-only wrong-catalog-code mutants for their requested diagnostic runs. Expanded Make variables confirm that EXP/exppicture join default vectors and the two diagnostics do not. Clean LOG/SQRT semantics are unchanged.

2. Deferred mechanism. No deferred-v2.patch was applied. eval.h, flow.c and workarea.c are unchanged from the starting e69e059e revision. Committed main was still f374fe02 at the final check and did not contain fl_finish_statement/the NDX mechanism, so this lane has not merged main prematurely. The planned main merge must follow the NDX merge as directed.

3. RED before the EXP fix. Main's unmodified EXP body passed the original exp fixture, then failed the existing exppicture capture. Verbatim evidence:

```text
test-samir-fns: 4 checks, 0 failures
>>> test-samir-fns-exp: green
```

```text
FNS transcript differs at byte 97 (got 0x20, real 0x32; lengths 629/606)
  got:          2.72
  real:       2.72
  FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
test-samir-fns: 4 checks, 1 failures
```

4. EXP before/after table. Before is the imported, unmodified main EXP body; after is the shared-helper fix. The complete marked after transcript equals the committed real III+ capture byte for byte. Quotes preserve leading spaces.

| Probe | Before | After | Real III+ |
|---|---|---|---|
| EXP(0) | `"         1.00"` | `"         1.00"` | `"         1.00"` |
| EXP(1) | `"         2.72"` | `"         2.72"` | `"         2.72"` |
| EXP(1.00) | `"         2.72"` | `"      2.72"` | `"      2.72"` |
| EXP(-1.00) | `"         0.37"` | `"      0.37"` | `"      0.37"` |
| EXP(22027) | `"*************"` | `"*************"` | `"*************"` |
| EXP(22026) | `"*************"` | `"*************"` | `"*************"` |
| EXP(-100) | `"         0.00"` | `"         0.00"` | `"         0.00"` |
| EXP(10000) | `"*************"` | `"*************"` | `"*************"` |
| EXP(0.368) | `"         1.44"` | `"     1.445"` | `"     1.445"` |
| EXP(.5) | `"         1.65"` | `"       1.65"` | `"       1.65"` |
| EXP(0.000001) | `"         1.00"` | `"  1.000001"` | `"  1.000001"` |
| EXP(1+0) | `"         2.72"` | `"         2.72"` | `"         2.72"` |
| EXP(X), X=-100 | `"         0.00"` | `"         0.00"` | `"         0.00"` |
| EXP(X), X=1 | `"         2.72"` | `"         2.72"` | `"         2.72"` |
| EXP(-BAL), N(8,2) | `"         0.00"` | `"      0.00"` | `"      0.00"` |
| EXP(BAL/1000) | `"         3.44"` | `"      3.44"` | `"      3.44"` |
| EXP(10), DECIMALS 0 | `"     22026"` | `"     22026"` | `"     22026"` |
| EXP(1.00), DECIMALS 3 | `"         2.718"` | `"      2.718"` | `"      2.718"` |
| Nested parameter from EXP(1.00) | `"         2.72"` | `"         2.72"` | `"         2.72"` |

5. Passing acceptance matrix. This command exited 0:

```sh
INITECH_QMP_STDIO=1 make test-samir-fns-log test-samir-fns-sqrt test-samir-fns-exp test-samir-fns-exppicture test-samir-fns-log-mutant test-samir-fns-sqrt-mutant test-samir-fns-exp-mutant test-samir-fns-exppicture-mutant test-samir-fns-log-emu test-samir-fns-sqrt-emu test-samir-fns-exp-emu test-samir-fns-exppicture-emu test-samir-fns-log-emu-mutant test-samir-fns-sqrt-emu-mutant test-samir-fns-exp-emu-mutant test-samir-fns-exppicture-emu-mutant test-samir test-samir-size samir-fns-goldens
```

Verbatim verdicts:

```text
>>> test-samir-log-contract: green (deferred error ordering, first error)
>>> test-samir-fns-log: green
>>> test-samir-fns-sqrt: green
>>> test-samir-fns-exp: green
>>> test-samir-fns-exppicture: green
>>> SAMIR_FNS_MUTANT_log RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_sqrt RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_exp RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_exppicture RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR.COM: 98489 bytes (flat .COM @0x40100, soft-float, x87=10)
>>> test-samir-fns-log-emu: green
>>> test-samir-fns-sqrt-emu: green
>>> test-samir-fns-exp-emu: green
>>> test-samir-fns-exppicture-emu: green
>>> SAMIR_FNS_MUTANT_log emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_sqrt emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_exp emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_exppicture emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> test-samir-pal: green
>>> test-samir-rt: green
>>> test-samir-pal-host: green
>>> test-samir-spec: green
>>> test-samir-value: green
>>> test-samir-dbf-ref: green
>>> test-samir-ndx-ref: green
>>> test-samir: SAMIR foundation green (PAL + rt + value + host binding + spec lock + dbf_ref + ndx_ref)
samir-size: 98489 B; headroom 7863 B (budget 106352 B)
>>> test-samir-size: green (>=4096 B headroom against samir.ld PAL-memory budget)
>>> samir-fns-goldens: green (SF-index/listoff/recsize/type/errors/struct/unknown/division/numlist/exprcol real III+ captures pinned)
```

6. Requested DO-argument diagnostic matrix. Ran all host/mutant/EMU/EMU-mutant targets with make -k; the command exited 2. The clean cases retain the known discrepancy: SAMIR enters PICOUT with a thirteen-column overflow result before its error report; real III+ reports error 58/61 first and then passes a ten-column result. These expected bytes were not relaxed.

| Case | Host | Host mutant gate | EMU | EMU mutant gate |
|---|---|---|---|---|
| logdoarg | RED | Blocked by clean RED | RED | Blocked by clean RED |
| sqrtdoarg | RED | Blocked by clean RED | RED | Blocked by clean RED |

Verbatim diagnostic output, excluding compiler commands and linker warnings:

```text
FNS transcript differs at byte 33 (got 0x50, real 0x4d; lengths 155/152)
  got: PICOUT *************
  real: MINT-ERROR         58 LOG() : Zero or negative.
  FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
test-samir-fns: 4 checks, 1 failures
make: *** [Makefile:34819: test-samir-fns-logdoarg] Error 1
FNS transcript differs at byte 34 (got 0x50, real 0x4d; lengths 149/146)
  got: PICOUT *************
  real: MINT-ERROR         61 SQRT() : Negative.
  FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
test-samir-fns: 4 checks, 1 failures
make: *** [Makefile:34819: test-samir-fns-sqrtdoarg] Error 1
make: Target 'test-samir-fns-logdoarg-mutant' not remade because of errors.
make: Target 'test-samir-fns-sqrtdoarg-mutant' not remade because of errors.
FNS transcript differs at byte 33 (got 0x50, real 0x4d; lengths 155/152)
  got: PICOUT *************
  real: MINT-ERROR         58 LOG() : Zero or negative.
  FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
test-samir-fns: 10 checks, 1 failures
make: *** [Makefile:34819: test-samir-fns-logdoarg-emu] Error 1
FNS transcript differs at byte 34 (got 0x50, real 0x4d; lengths 149/146)
  got: PICOUT *************
  real: MINT-ERROR         61 SQRT() : Negative.
  FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
test-samir-fns: 10 checks, 1 failures
make: *** [Makefile:34819: test-samir-fns-sqrtdoarg-emu] Error 1
make: Target 'test-samir-fns-logdoarg-emu-mutant' not remade because of errors.
make: Target 'test-samir-fns-sqrtdoarg-emu-mutant' not remade because of errors.
```

The mutant binaries/COMs were nevertheless built and run directly for all four diagnostic combinations. Each returned 1 with a completed transcript failure, and every capture contains the deliberately wrong catalog code 87. These raw RED executions are not passing mutation certificates because the clean baselines are also RED.

| Raw mutant | Exit | Mutation observed |
|---|---|---|
| logdoarg host | 1 | MINT-ERROR 87 |
| logdoarg emu | 1 | MINT-ERROR 87 |
| sqrtdoarg host | 1 | MINT-ERROR 87 |
| sqrtdoarg emu | 1 | MINT-ERROR 87 |

The DO-argument and reviewed guard-path issues remain for the planned NDX/main integration. No proc/flow workaround or shared mechanism patch was added in this round. Full shared-statement conformance is not claimed.

7. Size. The final three-function SAMIR.COM is 98489 bytes with 7863 bytes of headroom against the 106352-byte budget. The initially present LOG/SQRT-only artifact was 98425 bytes, so adding EXP to this lane costs 64 bytes. For a like-for-like EXP-formatting comparison, rebuilt the old main EXP body with the same lane/profile in /tmp: 98568 bytes. The shared-helper fix is 79 bytes smaller than that old-EXP baseline.

```text
-rwxr-xr-x 1 tobiasosborne tobiasosborne 98568 Oct 25 12:58 /tmp/log-sqrt-exp-r4-old-exp-baseline/SAMIR.COM
-rwxr-xr-x 1 tobiasosborne tobiasosborne 98489 Oct 25 12:51 build/SAMIR.COM
```

8. Final checks. EXP's x87 assembly was compared directly with origin/main and is identical. The entire fixed exppicture transcript was compared with the independent capture and matches. Source/manual/report text is ASCII; source whitespace checks pass. Shared runtime files and all existing fixture/provenance/hash files have no diff against the starting revision. No full make test, Bochs, deferred-v2 application, premature main merge or push was performed.

The local Beads lookup did not find initech-1x6x2.5; the PM-provided bead ID is recorded in the implementation commit. This report is committed separately. Existing untracked logs and report backups are preserved.
