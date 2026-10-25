NDX round 4 is complete. Fix commit 24bbea83 is on lane/wave1-ndx, based on ab5fca21. All four review blockers are fixed and the requested gates pass. No push was made. deferred-v2.patch is deleted.

The shared executor now distinguishes procedure handlers from inline handlers. A false guard carrying a deferred error under ON ERROR DO enters its body without calling the handler and clears ERROR(); a true guard and ordinary statements use the ordinary trap path. Immediate errors keep their existing precedence. This is independent of NDX and error 87. False CASE guards publish their pending code to ERROR() before the next guard while retaining the captured coalescing behavior.

ERROR() clearing now belongs to successful routine completion, including nested helpers, explicit RETURN, fallthrough and disk-program completion. It is no longer attached to the outer handler's return. Inline command handlers retain their code, as SF-errors requires. The synthetic guard callback tests explicitly model inline handlers; no contradictory procedure-handler assertion remains.

STATUS renders indexes from their owned display spelling, preserving both basename and explicit-extension case. Only a defaulted extension becomes .ndx. LIST STATUS and DISPLAY STATUS share this renderer; REINDEX retains the owned metadata and SET INDEX supplies its own display spelling. Normalized I/O names remain separate. No PAL primitive was added or changed.

Real evidence was minted with fns_mint.sh against ../dbase3-decomp, never synthesized. The guard-min, case-min and handlers reviewer programs are embedded in the extended ndx case with section markers; the new ndxdisplay sibling contains the display reproducer plus mixed explicit-extension STATUS checks. Each source declares MINT-ERRORS. Mint outputs passed the end-marker, ordered-error and DOS EOF checks. Native GOLD.OUT bytes, including CRLF, spacing and EOF, are committed with mint provenance and hashes. The function driver now uses GOLD.OUT on host and guest, matching the real mint's alternate filename so STATUS can compare byte-for-byte; the comparator is unchanged.

Commands:
```text
sh harness/diff/dbf_diff/fns_mint.sh ../dbase3-decomp ndx
sh harness/diff/dbf_diff/fns_mint.sh ../dbase3-decomp ndxdisplay
```

Committed SHA256:
```text
809744f8dc712658306c3a6e26315c0eff0f90a92cf860a246691a26a505af23  corpus/fns/ndx.prg
46a3787965ceec3c158549a22f42128890e6c50f2636b8f848cfd86c7f598991  fns-mint/ndx/GOLD.OUT
ff49fca52fbbb89fa81d14936055a16cdd6e51b4426eba69d9fec6a198c4006b  corpus/fns/ndxdisplay.prg
a85003622c78ff6b2f925a9dbbbf91463ddc12d30718708f65cae3fc3b2a1c91  fns-mint/ndxdisplay/GOLD.OUT
```

The new real captures directly pin BODY / counter 0 / ERROR() 0, false procedure CASE body selection, SAW87 after an inline-handled error 9, normal/helper/fallthrough return clearing, and index spelling in STATUS after creation, REINDEX and SET INDEX. The existing capture still pins true-guard handling, coalescing and retry. DISPLAY STATUS uses the same renderer; its interactive pagination was not separately minted in this round.

The round-3 implementation was restored temporarily against the new real captures and went RED:
```text
FNS transcript differs at byte 2085 (got 0x54, real 0x42; lengths 2535/2518)
  got: TRAP         87
  real: BODY
FNS transcript differs at byte 148 (got 0x42, real 0x62; lengths 6680/6680)
  got:     Master index file:  C:BARE.ndx  Key: NAME 
  real:     Master index file:  C:bare.ndx  Key: NAME 
```

Four separate temporary mutations were caught by the final NDX golden, then restored:
```text
guard: got TRAP 87; real BODY
case: got STALE; real SAW87
routine: got NORMAL-AFTER 9; real NORMAL-AFTER 0
nested: got HNEST-NORMAL-RETURN 9; real HNEST-NORMAL-RETURN 0
```

Validation commands completed with exit status 0:
```text
make test-samir test-samir-fns test-samir-deferred-errors test-samir-size test-interp-flow test-interp-proc test-xbase-eval test-samir-dml samir-dml-goldens samir-prog-goldens test-dbase-diff
make test-samir-fns-mutants test-samir-fns-ndx-deferred-mutant test-interp-proc-mutant
INITECH_QMP_STDIO=1 make test-samir-fns-ndx-emu test-samir-fns-ndx-emu-mutant test-samir-fns-ndxdisplay-emu test-samir-fns-ndxdisplay-emu-mutant
make test-samir-fns-ndx test-samir-fns-ndxdisplay test-samir-deferred-errors test-samir-size samir-fns-goldens
make test-samir-ctl
```

Verbatim gate verdicts (all 15 accepted function cases, including SF-errors, and all 21 DML cases):
```text
>>> test-samir-pal: green
>>> test-samir-rt: green
>>> test-samir-pal-host: green
>>> test-samir-spec: green
>>> test-samir-value: green
>>> test-samir-dbf-ref: green
>>> test-samir-ndx-ref: green
>>> test-samir: SAMIR foundation green (PAL + rt + value + host binding + spec lock + dbf_ref + ndx_ref)
>>> test-samir-fns-alias: green
>>> test-samir-fns-alternate: green
>>> test-samir-fns-console: green
>>> test-samir-fns-division: green
>>> test-samir-fns-errors: green
>>> test-samir-fns-exprcol: green
>>> test-samir-fns-index: green
>>> test-samir-fns-listoff: green
>>> test-samir-fns-ndx: green
>>> test-samir-fns-ndxdisplay: green
>>> test-samir-fns-numlist: green
>>> test-samir-fns-recsize: green
>>> test-samir-fns-struct: green
>>> test-samir-fns-type: green
>>> test-samir-fns-unknown: green
>>> test-samir-deferred-errors: green
>>> test-samir-ndx-link: green (no undefined work-area/index symbols)
>>> test-samir-ndx-names: green
>>> test-samir-size: green (>=4096 B headroom against samir.ld PAL-memory budget)
>>> test-interp-flow: green
>>> test-interp-proc: green
>>> test-xbase-eval: green
>>> samir-dml-goldens: green (real III+ ledger rows pinned)
>>> test-samir-dml-clear: green
>>> test-samir-dml-macro: green
>>> test-samir-dml-close: green
>>> test-samir-dml-index: green
>>> test-samir-dml-reindex: green
>>> test-samir-dml-sort: green
>>> test-samir-dml-total: green
>>> test-samir-dml-append: green
>>> test-samir-dml-save: green
>>> test-samir-dml-restore: green
>>> test-samir-dml-copyfile: green
>>> test-samir-dml-type: green
>>> test-samir-dml-rename: green
>>> test-samir-dml-erase: green
>>> test-samir-dml-label: green
>>> test-samir-dml-run: green
>>> test-samir-dml-abbrev: green
>>> test-samir-dml-abbrscope: green
>>> test-samir-dml-status: green
>>> test-samir-dml-memory: green
>>> test-samir-dml-statusinto: green
>>> samir-prog-goldens: green (PROG-01..07, EXTRA-01..13, PUBS-01 real III+ captures)
>>> test-dbase-diff: green
test-samir-deferred-errors: 100 checks, 0 failures
test-samir-ndx-names: 59 checks, 0 failures
test-interp-flow: 79 checks, 0 failures
test-interp-proc: 44 checks, 0 failures
test-xbase-eval: 81 checks, 0 failures
xbase-prog-diff: 8 checks, 0 failures
test-samir-ctl: 82 checks, 0 failures
>>> test-samir-ctl: green
>>> samir-fns-goldens: green (SF-index/listoff/recsize/type/errors/struct/unknown/division/numlist/exprcol real III+ captures pinned)
```

Verbatim mutation and emulator verdicts:
```text
>>> SAMIR_FNS_MUTANT_alias RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_alternate RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_console RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_division RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_errors RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_exprcol RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_index RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_listoff RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndxdisplay RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_numlist RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_recsize RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_struct RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_type RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_unknown RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx_deferred RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> test-interp-proc-mutant: green (PRIVATE no-restore correctly RED)
test-samir-fns: 12 checks, 0 failures
>>> test-samir-fns-ndx-emu: green
>>> SAMIR_FNS_MUTANT_ndx emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
test-samir-fns: 12 checks, 0 failures
>>> test-samir-fns-ndxdisplay-emu: green
>>> SAMIR_FNS_MUTANT_ndxdisplay emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
```

SAMIR.COM grew from 100505 to 100665 bytes: +160 bytes. No unrelated size reductions were made. Final size verdicts:
```text
>>> SAMIR.COM: 100665 bytes (flat .COM @0x40100, soft-float, x87=0)
samir-size: 100665 B; headroom 5687 B (budget 106352 B)
>>> test-samir-size: green (>=4096 B headroom against samir.ld PAL-memory budget)
```

All changed source, corpus, capture and provenance text is ASCII. Source whitespace checks pass with CRLF recognized; native golden spacing and the original mint log's trailing space are preserved. The new DATA/BASE.DBF is a binary fixture and is excluded from the text scan. No dBASE executable, overlay or message file was committed.

Beads issue creation failed because issue_prefix is missing; bd dolt pull failed because the origin remote is absent. Tracker configuration was left intact. No Bochs, full make test, merge or push was run.

Evidence and logs: build/ndx-r4/{red.log,red-display.log,mut-guard.log,mut-case.log,mut-routine.log,mut-nested.log,host-final.log,mutants-final.log,emu-final.log,restored-final.log,control-final.log}. Committed mint evidence: harness/diff/dbf_diff/xbase_prog_diff/fns-mint/{ndx,ndxdisplay}/.
