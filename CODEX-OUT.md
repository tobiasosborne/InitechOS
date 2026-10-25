Committed on `lane/wave1-logsqrt-r`: `d0e30d92` preserves all four independent reviewer captures; `66db8ea5` fixes the three in-scope findings. No push or bd operations.

EXP now replaces its computed picture suffix with signed intermediate room and clamps the final picture; its x87 block is unchanged. Inline ON ERROR RETURN skips the pending call and continues after DO. Argument-phase RETRY retains the original arguments, producing the captured traps 58 then 16, sixteen stars, and CNT=2. The RETRY-ignoring mutant now fails on host and completed emulator traces. Counterfactual transfer RETRY/CANCEL unit expectations were replaced by minted comparisons that check actual received arguments.

`limitpic`, `returnarg` and `retryconfirm` are ACCEPTED. Their source, golden, wrapper and original hash files remain byte-identical to the reviewer's captures. The harness reproduces returnarg's explicit wrapper end marker and pins that wrapper. `matrixb` remains PENDING for `initech-1x6x2.7`; host and emulator each exit 1 at the excluded SQRT rounding failure. Scientific fallback and that rounding boundary were not changed. No accepted case contains those excluded probes.

```sh
make -j64 test-merge-samir DBASE3_DECOMP=/home/tobiasosborne/Projects/dbase3-decomp LOTUS123_DECOMP=/home/tobiasosborne/Projects/lotus123-decomp SYSTEM7_DECOMP=/home/tobiasosborne/Projects/system7-decomp WIN31_DECOMP=/home/tobiasosborne/Projects/win31-decomp
```

Cold run: exit 2 at incomplete emulator boot traces. Full warm rerun: exit 0, **388 gates green**, including all **64 numeric/NDX/guard host/emulator/mutant legs**, size, overlay checks and golden hashes. The shared deferred unit passes 100 checks. Logs: `/tmp/logsqrt-r7-merge-{cold,warm}.log`; initial RED evidence: `/tmp/logsqrt-r7-red.log`.

Verbatim verdicts:

```text
>>> test-merge-samir: 388 gates (test-pubs-lockstep-src: yes; no LaTeX)
>>> test-samir-fns-limitpic: green
>>> test-samir-fns-limitpic-emu: green
>>> SAMIR_FNS_MUTANT_limitpic RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:327: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_limitpic emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:327: FNS transcript byte-identical to real III+
>>> test-samir-fns-returnarg: green
>>> test-samir-fns-returnarg-emu: green
>>> SAMIR_FNS_MUTANT_returnarg RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:327: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_returnarg emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:327: FNS transcript byte-identical to real III+
>>> test-samir-fns-retryconfirm: green
>>> test-samir-fns-retryconfirm-emu: green
>>> SAMIR_FNS_MUTANT_retryconfirm RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:327: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_retryconfirm emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:327: FNS transcript byte-identical to real III+
>>> test-samir-deferred-errors: green
>>> test-samir-size: green (resident >=4096 B headroom against the samir.ld PAL-memory budget; slot == largest overlay)
>>> samir-fns-goldens: green (SF-index/listoff/recsize/type/errors/struct/unknown/division/numlist/exprcol real III+ captures pinned)
>>> test-merge-samir: ALL GREEN
```

Resident: **87,760 B (+64 B)**; headroom **6,016 B**, **1,920 B above the 4,096 B floor**. SAMIR.EXE: 91,248 B; SAMIR.OVL: 27,136 B; slot: 9,888 B. Function and transfer paths remain resident.

Authored-source ASCII and whitespace checks passed; independent capture bytes are preserved. Tracked tree/index are clean after the report commit; existing untracked logs/backups remain. Worklog: `docs/worklog/wave1-logsqrt-round7.md`.
