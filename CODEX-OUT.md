NDX round 5 is complete. Commit 5b56d615 adds both reviewer probes and classifier mutant gates on lane/wave1-ndx. No push was made.

The extended ndx case includes the renamed OTHERNAME handler and installation inside WRAPPER at DO depth 2. Both were minted through fns_mint.sh against real III+ with the existing declared MINT-ERRORS. Each prints BODY, counter 0, ERROR()=0. Native captures and hashes are committed; the prior capture prefix is unchanged.

No production behavior changed. proc.c adds only conditional mutant branches. The normal SAMIR.COM remains byte-identical to round 4: SHA256 cb33e5ab20ba7e4ef9ce49c721f6f660091bc8360326e3448b74dbcb8e21d5e6.

Validation completed with exit status 0:
```text
make test-samir-fns test-samir-fns-mutants samir-fns-goldens test-samir-size
INITECH_QMP_STDIO=1 make test-samir-fns-emu test-samir-fns-emu-mutants
```

All 15 accepted cases pass on host and QEMU. All 35 mutant runs are caught: 15 standard mutants on each platform, both classifier mutants on each platform, and the deferred-error host mutant. The new gates join the aggregate mutant targets and default unit/emulator gate lists.

Verbatim verdicts:
```text
>>> test-samir-fns-ndx: green
>>> test-samir-fns-ndx-emu: green
>>> SAMIR_FNS_MUTANT_ndx_handler_name RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx_handler_depth RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx_handler_name emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx_handler_depth emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:97: FNS transcript byte-identical to real III+
samir-size: 100665 B; headroom 5687 B (budget 106352 B)
>>> test-samir-size: green (>=4096 B headroom against samir.ld PAL-memory budget)
```

The name mutant first differs at byte 2582, the depth mutant at byte 2628: both print TRAP 87 where real III+ prints BODY. Normal image delta: 0 bytes. All changed text is ASCII; source whitespace checks pass, with native golden CRLF preserved.

Committed SHA256:
```text
81ab11a3a785378ca3708bf27fc07c84b11ff9261402efa43f67f2281f4337ae  corpus/fns/ndx.prg
0e27875ae39f997707114e24c5d1791c016ad24bd3cd25638171b5fe81cd883f  fns-mint/ndx/GOLD.OUT
```

These probes establish handler-name and installation-depth independence for the exercised error 87; they make no broader claim about other deferred error numbers. Logs: build/ndx-r5/mint.log, host-matrix.log and emu-matrix.log; classifier failure transcripts are in build/samirfns/{mut,emu-mut}-ndx-handler-{name,depth}.log.

Beads remains unavailable: issue_prefix is missing and the Dolt origin remote is absent.
