LOG and SQRT, PM round 3: argument-picture fix committed locally. The requested function, mutant, QEMU, foundation and size gates pass. Shared-runtime follow-ups remain open as detailed below. Nothing was pushed.

1. Local commits and scope. Merge commit 7e9edf0d combines origin/lane/wave1-sqrt (d1411291) into lane/wave1-log. Implementation commit 69347583 contains the shared helper, real III+ captures, fixture support and manual updates. The brief conflict retained the LOG lane brief; the Makefile keeps both accepted functions.

The three deferred-v1 files (eval.h, flow.c and workarea.c) are byte-identical to the merge baseline. No guard-path hunks were edited. deferred-v2.patch has not been supplied, so the PM-owned guard fix remains pending. The EXP function is also unchanged relative to the merge baseline.

2. Shared picture rule. fn_math_picture is used by both LOG and SQRT for normal and overflow results:

```text
D = max(argument decimal places, SET DECIMALS)
I = max(1, argument width - original point/fraction suffix)
W = I + (D ? D + 1 : 0)
```

The helper preserves argument integer/sign room and replaces its fractional suffix once. It never sizes from the result or classifies infinity/NaN. SET DECIMALS is a minimum: fresh D0/D1 fractional and field probes preserve greater argument precision. The ordinary dec_format_fit rounding guard retries within the selected width, including carries; it does not pre-round the stored double or widen the picture from the result. The comment cites the committed real captures and local formatting specification.

Removed the incorrect LOG(22027) five-column assertion and the other inferred result-width/output expectations. Numeric presentation expectations now come from independently minted bytes. Generic deferred-error contract checks remain.

3. Independent real III+ mints. Ran fns_mint.sh against ../dbase3-decomp for log, sqrt, exppicture, logdoarg and sqrtdoarg. All five passed their declared error-code sequences, reached one FNS-END marker, flushed DOS EOF, and passed their SHA-256 checks. Verbatim mint verdicts:

```text
MINT SF-log: /home/tobiasosborne/Projects/initech-os-cx-fn-log/build/samirfns/golden/log/GOLD.OUT
MINT SF-sqrt: /home/tobiasosborne/Projects/initech-os-cx-fn-log/build/samirfns/golden/sqrt/GOLD.OUT
MINT SF-exppicture: /home/tobiasosborne/Projects/initech-os-cx-fn-log/build/samirfns/golden/exppicture/GOLD.OUT
MINT SF-logdoarg: /home/tobiasosborne/Projects/initech-os-cx-fn-log/build/samirfns/golden/logdoarg/GOLD.OUT
MINT SF-sqrtdoarg: /home/tobiasosborne/Projects/initech-os-cx-fn-log/build/samirfns/golden/sqrtdoarg/GOLD.OUT
```

Committed corpus programs, helper PRGs, captures, provenance and hashes. The fixtures cover literals, memvars, N(8,2) fields, expressions, fractional arguments, DECIMALS 0/1/2/3, positive nested arguments and domain failures inside nested routines. Matching external PRGs and procedure bodies let both existing DO resolution paths execute identical helper bodies. The mint and test driver copy and verify these helper inputs. No licensed dBASE executable, overlay or message file was committed.

Real III+ outputs for the PM examples, quoted to preserve spaces:

| Probe | Real III+ output |
|---|---|
| LOG(22027), D2 | `"   10.00"` |
| LOG(22026), D2 | `"   10.00"` |
| LOG(-100), D2 | `"*******"` |
| STORE -100 TO X; LOG(X) | `"*************"` |
| SQRT(-1.00), D2 | `"*****"` |
| SQRT(-BAL), BAL N(8,2) | `"*********"` |
| SQRT(-100), D0 | `"****"` |
| SQRT(10000), D2 | `"  100.00"` |

Additional captured examples include SQRT(99.998) = "10.000", LOG(0.000001) = "-13.8155", LOG(0.36) = "-1.0", and SQRT(4.0000) = "2.0000" even at DECIMALS 0. These bytes, rather than inferred result widths, govern the implementation.

4. RED before the shared helper. The fresh probes caught both original implementations:

```text
FNS transcript differs at byte 406 (got 0x31, real 0x20; lengths 1941/1946)
  got: 10.00
  real:    10.00
  FAIL harness/diff/dbf_diff/test_samir_fns.c:173: FNS transcript byte-identical to real III+
>>> test-samir-log-contract: green (deferred error ordering, first error)
test-samir-fns: 12 checks, 1 failures
make: *** [Makefile:34813: test-samir-fns-log] Error 1
```

```text
FNS transcript differs at byte 333 (got 0x31, real 0x20; lengths 1873/1830)
  got: 100.00
  real:   100.00
  FAIL harness/diff/dbf_diff/test_samir_fns.c:173: FNS transcript byte-identical to real III+
test-samir-fns: 8 checks, 1 failures
make: *** [Makefile:34813: test-samir-fns-sqrt] Error 1
```

5. Final passing gates. The following command exited 0:

```sh
INITECH_QMP_STDIO=1 make test-samir-fns-log test-samir-fns-sqrt test-samir-fns-log-mutant test-samir-fns-sqrt-mutant test-samir-fns-log-emu test-samir-fns-sqrt-emu test-samir-fns-log-emu-mutant test-samir-fns-sqrt-emu-mutant test-samir test-samir-size test-samir-fns samir-fns-goldens
```

Verbatim verdicts:

```text
>>> test-samir-log-contract: green (deferred error ordering, first error)
>>> test-samir-fns-log: green
>>> test-samir-fns-sqrt: green
>>> SAMIR_FNS_MUTANT_log RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_sqrt RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR.COM: 98425 bytes (flat .COM @0x40100, soft-float, x87=4)
>>> test-samir-fns-log-emu: green
>>> test-samir-fns-sqrt-emu: green
>>> SAMIR_FNS_MUTANT_log emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_sqrt emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> test-samir-pal: green
>>> test-samir-rt: green
>>> test-samir-pal-host: green
>>> test-samir-spec: green
>>> test-samir-value: green
>>> test-samir-dbf-ref: green
>>> test-samir-ndx-ref: green
>>> test-samir: SAMIR foundation green (PAL + rt + value + host binding + spec lock + dbf_ref + ndx_ref)
samir-size: 98425 B; headroom 7927 B (budget 106352 B)
>>> test-samir-size: green (>=4096 B headroom against samir.ld PAL-memory budget)
>>> test-samir-fns-alias: green
>>> test-samir-fns-alternate: green
>>> test-samir-fns-console: green
>>> test-samir-fns-division: green
>>> test-samir-fns-errors: green
>>> test-samir-fns-exprcol: green
>>> test-samir-fns-index: green
>>> test-samir-fns-listoff: green
>>> test-samir-fns-numlist: green
>>> test-samir-fns-recsize: green
>>> test-samir-fns-struct: green
>>> test-samir-fns-type: green
>>> test-samir-fns-unknown: green
>>> samir-fns-goldens: green (SF-index/listoff/recsize/type/errors/struct/unknown/division/numlist/exprcol real III+ captures pinned)
```

An additional SAMIR_FNS_MUTANT_mathpicture build corrupts only wide negative argument pictures. Both runs exit 1 for the intended newly minted fractional-negative row:

```text
FNS transcript differs at byte 548 (got 0x2a, real 0x0d; lengths 1836/1830)
  got: ******
  real: *****
  FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
>>> test-samir-log-contract: green (deferred error ordering, first error)
test-samir-fns: 13 checks, 1 failures
```

```text
FNS transcript differs at byte 504 (got 0x2a, real 0x0d; lengths 1726/1720)
  got: ******
  real: *****
  FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
test-samir-fns: 9 checks, 1 failures
```

6. EXP on main does NOT match the fresh picture probes. Built the engine from origin/main 4884df087ff9a82ce61af9ffc1b6a8603e2103c9 in /tmp. Only the factory driver helper-file staging was adjusted; main runtime sources were unchanged. Its exppicture run exits 1:

```text
FNS transcript differs at byte 97 (got 0x20, real 0x32; lengths 629/606)
  got:          2.72
  real:       2.72
  FAIL /tmp/log-sqrt-r3-main/harness/diff/dbf_diff/test_samir_fns.c:150: FNS transcript byte-identical to real III+
test-samir-fns: 3 checks, 1 failures
```

Quoted comparisons from the real capture and main engine:

| Probe | Real III+ | Main EXP |
|---|---|---|
| EXP(1) | `"         2.72"` | `"         2.72"` |
| EXP(1.00) | `"      2.72"` | `"         2.72"` |
| EXP(0.368) | `"     1.445"` | `"         1.44"` |
| EXP(0.000001) | `"  1.000001"` | `"         1.00"` |
| EXP(-BAL) | `"      0.00"` | `"         0.00"` |
| EXP(BAL/1000) | `"      3.44"` | `"         3.44"` |
| EXP(10), D0 | `"     22026"` | `"     22026"` |
| EXP(1.00), D3 | `"      2.718"` | `"         2.718"` |

The EXP mismatch includes fractional/field width and retained precision. EXP was not changed in this lane. Its independent probe is committed for the owning lane.

7. Explicit PENDING shared-runtime diagnostics. The added logdoarg and sqrtdoarg captures show that real III+ reports a failed DO argument before entering the callee, then passes a ten-column overflow value. Current SAMIR enters the callee with thirteen columns and reports the error afterward. These extra cases are named PENDING gates, outside the passing numeric acceptance cases; their expected bytes were not relaxed. The passing LOG/SQRT cases still contain nested positive arguments and nested in-routine domain failures.

Verbatim current diagnostic failures:

```text
FNS transcript differs at byte 33 (got 0x50, real 0x4d; lengths 155/152)
  got: PICOUT *************
  real: MINT-ERROR         58 LOG() : Zero or negative.
  FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
test-samir-fns: 4 checks, 1 failures
make: *** [Makefile:34825: test-samir-fns-logdoarg] Error 1
```

```text
FNS transcript differs at byte 34 (got 0x50, real 0x4d; lengths 149/146)
  got: PICOUT *************
  real: MINT-ERROR         61 SQRT() : Negative.
  FAIL harness/diff/dbf_diff/test_samir_fns.c:175: FNS transcript byte-identical to real III+
test-samir-fns: 4 checks, 1 failures
make: *** [Makefile:34825: test-samir-fns-sqrtdoarg] Error 1
```

The DO-argument issue needs the PM shared-runtime ruling; it was reported while the picture work continued. The separately reviewed IF/CASE/WHILE guard blocker also remains pending deferred-v2.patch. These outstanding shared statement semantics prevent claiming full conformance despite the passing numeric gates.

8. Binary size and checks. Rebuilt the merged pre-fix source (7e9edf0d) separately with the same profile for a like-for-like two-function baseline:

```text
-rwxr-xr-x 1 tobiasosborne tobiasosborne 98728 Oct 25 11:17 /tmp/log-sqrt-r3-merge-baseline/SAMIR.COM
-rwxr-xr-x 1 tobiasosborne tobiasosborne 98425 Oct 25 11:11 build/SAMIR.COM
```

The shared picture fix reduces SAMIR.COM from 98728 to 98425 bytes: -303 bytes. The final image has 7927 bytes of headroom against the 106352-byte budget. The initially present LOG-only artifact was 98489 bytes (-64 bytes versus that stale pre-merge artifact); it is not the merged-source baseline.

Changed source and probe text is ASCII. Synthetic binary DBF fixtures are preserved as binary. Real GOLD.OUT CRLF/DOS EOF and raw mint logs are retained verbatim. The source whitespace check passes with CR-at-EOL recognized and raw tool-generated mint.log files excluded; those logs contain original trailing spaces. All five per-capture hash manifests pass. The deferred-v1 files and EXP body compare unchanged against the merge baseline.

Implementation statistics:

```text
69347583a7470e07d00168f4a767011b3dae3653 samir(fns): preserve LOG and SQRT argument pictures against fresh III+ mints
 77 files changed, 1202 insertions(+), 162 deletions(-)
```

All work is on lane/wave1-log in local commits. This report is committed separately. Existing untracked log/report backups remain untouched; nothing was pushed. Beads pull/create were unavailable because its Dolt origin and issue_prefix configuration are missing.
