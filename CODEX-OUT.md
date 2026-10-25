NDX round 3 is complete. Fix commit 37804a78 is on lane/wave1-ndx, based on the PM's round-2 commit 65cf35d0. Both blocking fixes and all requested validation gates pass. No push was made.

Display provenance: index handles now own an exact-sized display-name copy separately from the normalized I/O pathname. INDEX ON preserves the supplied basename and extension before I/O normalization. SET INDEX and USE INDEX retain the new operand's spelling. REINDEX preserves the current display metadata. Thus creation as bare returns C:bare.ndx, while reopening as BARE.NDX returns C:BARE.NDX. The cursor callback remains the only metadata boundary used by fn_builtins.c.

Shared guard completion: IF, CASE and WHILE now use the same trap/retry helper as ordinary statements. Normally handled deferred errors retain the evaluated condition even if the handler changes its inputs. IF/WHILE RETRY re-evaluates the condition. The new real III+ capture also pins coalescing across false CASE guards, and CASE RETRY resuming after ENDCASE without executing a CASE/OTHERWISE body. Untrapped errors and cancellation propagate normally. All three eval_guard call sites were audited.

Additional period evidence corrected an older procedure-unit assumption: a handled IF 1 error runs the handler, then the body and following statements. The test now checks separate handler/body counters instead of asserting an abort. The same capture pins ERROR()=9 before the guard, 37 inside a procedure handler, and 0 after its return. Procedure-handler completion now clears ERROR(), while inline-handler retention stays pinned by the existing SF-errors golden. The implementation detects an actual handler procedure call, not any particular function name or error code.

Fresh independent oracle generation exited 0:

```text
sh harness/diff/dbf_diff/fns_mint.sh ../dbase3-decomp ndx
MINT SF-ndx: /home/tobiasosborne/Projects/initech-os-cx-fn-ndx/build/samirfns/golden/ndx/GOLD.OUT
```

The real dBASE III PLUS 1.1 capture contains 2083 native bytes, a 2080-byte compared transcript, and 19 declared MINT-ERROR records. Its expected lines were not hand-written. The program, capture, MINT driver, configuration/log, hashes and accepted-hash manifest were promoted together. No copyrighted dBASE binaries were added.

fns_mint.sh now loads FNSTEST as a procedure file when its input declares procedures, matching the existing prog_mint.sh setup. SAMIR's driver already registers procedures in its current program source. Probe loops have body-side bounds so an immediate-error mutant cannot bypass a guard-only bound indefinitely; re-minting confirmed the safety exit did not change correct III+ output.

Final SHA256 values:

```text
76afe323262b107e7108691b4e4c04a24563be2135b216628ac3c0e8931e1beb  corpus/fns/ndx.prg
2c36c0f77e0d13dba06af28dad8d2ac01f0d038bd99e745134288a934d991dfa  fns-mint/ndx/GOLD.OUT
60921fd9bcb2b6109da6c6076d9480429d4c546f5099ef195cf16f1eed43a45e  deferred-v2.patch
```

Selected newly minted lines, verbatim:

```text
created-lower [C:bare.ndx]
reindexed-lower [C:bare.ndx]
set-explicit [C:BARE.NDX]
set-lower-default [C:bare.ndx]
created-mixed [C:MiXeD.nDx]
reindexed-mixed [C:MiXeD.nDx]
set-uppercase-extension [C:MIXED.NDX]
use-mixed-extension [C:mixed.nDx]
IF-PRESERVED
IF-PRESERVED-AFTER .F.
IF-RETRY-ELSE
CASE-RETRY-AFTER          1          0
WHILE-RETRY-AFTER          1          0
PREGUARD-ERROR          9
NONLOGICAL-GUARD-AFTER          5          1          0
```

The new oracle caught the old basename implementation before repair:

```text
  got: created-lower [C:BARE.ndx]
  real: created-lower [C:bare.ndx]
```

After basename repair, it caught the old guard path aborting before IF-BODY/IF-AFTER. A separate restoration mutation temporarily put v1 flow.c back; the expanded unit then reported:

```text
test-samir-deferred-errors: 100 checks, 50 failures
```

Version 2 was restored before final validation. Both existing NDX host mutant targets exited 0, proving the mutants were caught:

```text
>>> test-samir-fns-ndx: green
>>> SAMIR_FNS_MUTANT_ndx RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx_deferred RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
```

The final combined host run exited 0 (under a 180-second safety timeout):

```text
make test-samir test-samir-fns-ndx test-samir-fns-ndx-mutant test-samir-fns-ndx-deferred-mutant test-samir-fns test-samir-deferred-errors test-samir-size test-interp-flow test-interp-proc test-samir-ctl test-xbase-eval samir-fns-goldens
```

Selected verdicts, verbatim:

```text
>>> test-samir: SAMIR foundation green (PAL + rt + value + host binding + spec lock + dbf_ref + ndx_ref)
>>> test-samir-fns-ndx: green
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
test-samir-deferred-errors: 100 checks, 0 failures
>>> test-samir-deferred-errors: green
>>> test-samir-ndx-link: green (no undefined work-area/index symbols)
test-samir-ndx-names: 59 checks, 0 failures
>>> test-samir-ndx-names: green
>>> test-interp-flow: green
test-interp-proc: 44 checks, 0 failures
>>> test-interp-proc: green
test-samir-ctl: 82 checks, 0 failures
>>> test-samir-ctl: green
>>> test-xbase-eval: green
>>> samir-fns-goldens: green (SF-index/listoff/recsize/type/errors/struct/unknown/division/numlist/exprcol real III+ captures pinned)
```

All existing host function mutants and the strengthened procedure test's PRIVATE-restoration mutant also passed their mutation gates:

```text
>>> test-interp-proc-mutant: green (PRIVATE no-restore correctly RED)
```

INITECH_QMP_STDIO=1 make test-samir-fns-ndx-emu test-samir-fns-ndx-emu-mutant exited 0. Verbatim verdicts:

```text
test-samir-fns: 12 checks, 0 failures
>>> test-samir-fns-ndx-emu: green
>>> SAMIR_FNS_MUTANT_ndx emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
```

The entire new transcript, including names, REINDEX, handled guards, retry and ERROR() observations, matches on both host and QEMU.

Size verdicts, verbatim:

```text
>>> SAMIR.COM: 100505 bytes (flat .COM @0x40100, soft-float, x87=0)
samir-size: 100505 B; headroom 5847 B (budget 106352 B)
>>> test-samir-size: green (>=4096 B headroom against samir.ld PAL-memory budget)
```

Round-3 COM delta: 99,897 -> 100,505 bytes, +608 bytes. The memory-budget assertion is unchanged.

Handoff: deferred-v2.patch is incremental against the accepted v1 mechanism in 65cf35d0. It contains only flow.c guard/statement completion changes. The deferred member in eval.h and initialization in workarea.c are unchanged, so those files need no deferred hunks. It deliberately excludes NDX naming and the separate proc.c procedure-handler ERROR() correction. The latter is in fix commit 37804a78 for the PM to merge separately; it is outside the requested eval/flow/workarea-only export.

The patch was applied to a pristine v1 flow.c outside the repository and compared byte for byte with the current flow.c: apply check exit 0, cmp exit 0. LOG/SQRT can apply it to their identical v1 flow mechanism.

All edited source/text/provenance files and the patch are ASCII-clean. Native CRLF and DOS EOF bytes in the real capture were retained. Source whitespace validation passed with Git's cr-at-eol setting. Generated patch context spaces are excluded from that style check and verified by actual patch application. No golden bytes were normalized for style checks.

No Bochs, full make test, publication rebuild, merge or push was run in this round. The report is recorded separately from fix commit 37804a78. Pre-existing untracked archives/logs were retained. Beads creation and Dolt pull remain unavailable because issue_prefix and the Dolt origin remote are missing; tracker configuration was not changed.

Logs: build/ndx-r3/mint.log, red.log, red-guards.log, guard-restoration-mutant.log, host-final.log, emu-final.log and mutants-final.log. Committed mint provenance: harness/diff/dbf_diff/xbase_prog_diff/fns-mint/ndx/.
