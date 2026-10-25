Committed `6bb3ecd0` on `lane/wave1-logsqrt-r`. No push or bd operations.

The shared flow/proc path now completes deferred DO WITH argument errors before transferring control, without function or error-code special cases. RETRY re-evaluates the DO; cancellation and unhandled errors prevent entry. String arguments are owned before a handler can reset scratch.

Fresh real III+ captures correct one premise: a normally returning inline handler reports the error first and then resumes the callee. Both the existing LOG/SQRT captures and fresh NDX(8)/SQRT(-1) captures show this. Those independent bytes were preserved.

Promoted `logdoarg` and `sqrtdoarg` unchanged. Added ACCEPTED real captures `ndxdoarg` (#87), `sqrtpass` (#61), `argorder` (61/87 argument ordering), and `argpicture` (NaN/infinity STORE/PARAMETERS pictures), with declared errors and hash pins. III+ retains the last failing argument's code across later valid arguments; first-error behavior remains within each expression. Shared nonfinite bindings retain their incoming columns plus decimal suffix. Boundary, ordering and picture mutants catch regressions on host and completed emulator traces.

Audit: DO WITH is the implemented expression-to-routine transfer. PARAMETERS parses names; disk DO uses bare names; RUN/! passes literal command text. SET PROCEDURE and UDF calls remain unimplemented. Existing LOG/NDX guard rules remain green.

```sh
make -j64 test-merge-samir DBASE3_DECOMP=/home/tobiasosborne/Projects/dbase3-decomp LOTUS123_DECOMP=/home/tobiasosborne/Projects/lotus123-decomp SYSTEM7_DECOMP=/home/tobiasosborne/Projects/system7-decomp WIN31_DECOMP=/home/tobiasosborne/Projects/win31-decomp
```

First run: exit 2 because the source-archive gate saw unstaged deleted PENDING markers and the output runner's absent CODEX-OUT.md. Staged the renames, restored the report, and reran the entire command: exit 0, all 376 gates green. All 52 numeric/NDX/guard case legs passed, including host/emulator mutants, plus the dedicated NDX classifiers, 134-check deferred unit suite, size and golden hashes. Logs: `/tmp/logsqrt-r6-merge-cold.log` and `/tmp/logsqrt-r6-merge-warm.log`. Each real `fns_mint.sh` capture command exited 0; initial differential RED evidence is in `/tmp/logsqrt-r6-red-{existing,new,argorder}.log`.

Verbatim gate verdicts:

```text
>>> test-merge-samir: 376 gates (test-pubs-lockstep-src: yes; no LaTeX)
>>> test-samir-fns-logdoarg: green
>>> test-samir-fns-sqrtdoarg: green
>>> test-samir-fns-ndxdoarg: green
>>> test-samir-fns-sqrtpass: green
>>> test-samir-fns-logdoarg-emu: green
>>> test-samir-fns-sqrtdoarg-emu: green
>>> test-samir-fns-ndxdoarg-emu: green
>>> test-samir-fns-sqrtpass-emu: green
>>> SAMIR_FNS_MUTANT_ndxdoarg RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndxdoarg emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_sqrtpass RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_sqrtpass emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> test-samir-deferred-errors: green
>>> test-samir-size: green (resident >=4096 B headroom against the samir.ld PAL-memory budget; slot == largest overlay)
>>> samir-fns-goldens: green (SF-index/listoff/recsize/type/errors/struct/unknown/division/numlist/exprcol real III+ captures pinned)
>>> test-merge-samir: ALL GREEN
```

Resident: **87,696 B**, up **160 B**; headroom **6,080 B** against 93,776 B, **1,984 B above the 4,096 B floor**. SAMIR.EXE: 91,184 B; SAMIR.OVL: 27,136 B; slot: 9,888 B. The transfer path stays resident; overlay checks pass.

Authored-source ASCII and whitespace checks passed. Real captures retain trailing spaces, CRLF and DOS EOF. Tracked tree/index are clean after the report commit; existing untracked logs/backups were preserved. Worklog: `docs/worklog/wave1-do-with-completion.md`.
