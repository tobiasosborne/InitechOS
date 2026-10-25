Replayed and committed on `lane/wave1-logsqrt-r`, based on current main `3c194c50`. The 352-gate merge suite passes. `logdoarg` and `sqrtdoarg` remain PENDING: both still fail on main's deferred-v2 path. No push or bd operations were performed.

1. Commits and conflict resolutions.

| Original | Replayed commit | Contribution |
|---|---|---|
| `6e17e546` | `d3616280` | LOG and general fraction formatting |
| `7e9edf0d`, first-parent replay | `287218f4` | SQRT contribution, including parent `d1411291` |
| `69347583` | `73b51638` | Shared argument-picture helper and independent captures |
| `e69e059e` | `2f03b607` | Historical round-3 report |
| `d7fdf7da` | `6a575617` | EXP picture correction, preserving main's x87 computation |
| `92668fb0` | `883f49e8` | Historical round-4 report |
| New integration | `d9d4ed57` | Main-compatible harness, LOG guard capture and worklog |

The final report is recorded in the following documentation commit, titled `docs: report numeric lane replay on NDX main`. Both lane-brief commits were skipped. The SQRT parent was included through the merge replay rather than replayed twice.

Cherry-pick conflicts occurred in Makefile, the function driver, mint script, hash manifest, builtin EXP implementation, quick-reference includes, the deleted monolithic manual, flow.c, and historical CODEX-OUT.md. Resolutions preserve main's marker-derived case lists, set_options.def, split manual entries, W2 support, per-case locks, GOLD.OUT extraction, completed emulator traces and EXE/OVL installation. The hash manifest retains main's NDX pins and adds the lane's independent captures. External helper inputs and their source-byte checks were retained.

All deferred-v1 hunks were dropped. `eval.h`, `flow.c`, `workarea.c` and `proc.c` are byte-identical to `origin/main`. No deferred-v2.patch was applied. LOG and SQRT return their overflow values normally and set the first pending #58/#61 for main's mechanism. LOG/SQRT use the shared argument picture; EXP uses the same helper with its independently minted computed-picture convention. No result-width assertion was reintroduced.

2. New real III+ evidence.

`harness/diff/dbf_diff/xbase_prog_diff/fns-mint/logguards/` is a fresh real III PLUS 1.1 capture from `fns_mint.sh`, with declared `MINT-ERRORS: 58`. It replaces the NDX guard probes' expression with `LOG(0)=0`, and covers a false IF and CASE under a procedure handler plus a false CASE under an inline handler. It is ACCEPTED and pinned.

The procedure handler is bypassed, the bodies run, and CNT/ERROR remain zero. The inline-handler CASE exposes pending ERROR()=58 to its next guard. Relevant minted bytes, shown without CRLF or DOS EOF decoration:

```text
LOG-GUARD-PROC
BODY
AFTER          0          0
CASE-PROC-BODY
CASE-PROC-AFTER          0          0
LOG-CASE-FALSE
MINT-ERROR         58 LOG() : Zero or negative.
SAW58         58
CASE-AFTER         58
```

FNSTEST.PRG SHA-256: `c6b4ffa1df5830aefee0db178e3f9072a09d126aae6dc9c71aaa8ff22204b1cb`.
GOLD.OUT SHA-256: `255b70415d4873cf5192f507b8fc944b187ae51fc8a72858d2bd8cbdcfa8ee7b`.
The immediate-error mutant invokes the procedure handler and differs from this golden on host and emulator. Both emulator legs prove a completed trace.

3. Commands and exit codes.

Fetch, branch creation and each resolved cherry-pick completed with exit 0. Initial cherry-pick attempts exited 1 for the conflicts described above.

```sh
make -j64 test-merge-samir DBASE3_DECOMP=/home/tobiasosborne/Projects/dbase3-decomp LOTUS123_DECOMP=/home/tobiasosborne/Projects/lotus123-decomp SYSTEM7_DECOMP=/home/tobiasosborne/Projects/system7-decomp WIN31_DECOMP=/home/tobiasosborne/Projects/win31-decomp
```

Exit 0 on the first merge-suite run; no warm retry was needed. This ran every accepted LOG/SQRT/EXP/exppicture/logguards host, mutant, emulator and emulator-mutant gate, plus test-samir, size, static overlays, source manual lockstep and golden hashes. Full log: `/tmp/logsqrt-r-merge-cold.log`.

```sh
sh harness/diff/dbf_diff/fns_mint.sh /home/tobiasosborne/Projects/dbase3-decomp logguards
make test-samir-fns-logguards test-samir-fns-logguards-mutant test-pubs-lockstep-src
```

Each command exited 0. Logs: `/tmp/logsqrt-r-mint-logguards.log` and `/tmp/logsqrt-r-guard-host.log`.

The initial six-case host command exited 2 at logdoarg after LOG/SQRT/EXP/exppicture passed. The complete explicit diagnostic matrix was then run:

```sh
INITECH_QMP_STDIO=1 make -k test-samir-fns-logdoarg test-samir-fns-sqrtdoarg test-samir-fns-logdoarg-mutant test-samir-fns-sqrtdoarg-mutant test-samir-fns-logdoarg-emu test-samir-fns-sqrtdoarg-emu test-samir-fns-logdoarg-emu-mutant test-samir-fns-sqrtdoarg-emu-mutant
```

Exit 2. Both clean cases fail identically on host and emulator; their standard mutant targets are blocked by those red prerequisites. Log: `/tmp/logsqrt-r-doarg-gates.log`. Direct executions of each wrong-code mutant were also completed:

```sh
build/test_samir_fns_logdoarg logdoarg host
build/test_samir_fns_logdoarg logdoarg emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_logdoarg.EXE
build/test_samir_fns_sqrtdoarg sqrtdoarg host
build/test_samir_fns_sqrtdoarg sqrtdoarg emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_sqrtdoarg.EXE
```

Each exited 1 with a transcript mismatch; both emulator traces completed. These are diagnostic failures, not passing mutation certificates, because the clean baselines are also RED. Results: `/tmp/logsqrt-r-doarg-raw-results.json`.

4. Verbatim accepted gate verdicts.

```text
>>> test-samir-fns-exp: green
>>> test-samir-fns-exppicture: green
>>> test-samir-fns-logguards: green
>>> test-samir-fns-log: green
>>> test-samir-fns-sqrt: green
>>> SAMIR_FNS_MUTANT_exppicture RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_exp RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_logguards RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_log RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_sqrt RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> test-samir-fns-exp-emu: green
>>> test-samir-fns-logguards-emu: green
>>> test-samir-fns-exppicture-emu: green
>>> SAMIR_FNS_MUTANT_exp emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> test-samir-fns-log-emu: green
>>> test-samir-fns-sqrt-emu: green
>>> SAMIR_FNS_MUTANT_logguards emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_exppicture emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_sqrt emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_log emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
>>> test-merge-samir: 352 gates (test-pubs-lockstep-src: yes; no LaTeX)
>>> samir-fns-goldens: green (SF-index/listoff/recsize/type/errors/struct/unknown/division/numlist/exprcol real III+ captures pinned)
>>> test-pubs-lockstep-src: pubs/manuals/initechbase-volume-one: 151 shipped, 151 documented, 2 DRAFT
>>> test-pubs-lockstep-src: pubs/manuals/initechbase-quick-reference: 151 shipped, 151 documented, 2 DRAFT
>>> test-pubs-lockstep-src: green (295 DotExample typed-line checks, 1 Transcript pairs; restricted source grammar, no LaTeX run)
>>> test-samir-ovl-static: green (relocation kinds, slot, headers, call-graph rule; re-pack byte-identical)
>>> test-samir-size: green (resident >=4096 B headroom against the samir.ld PAL-memory budget; slot == largest overlay)
>>> test-samir: SAMIR foundation green (PAL + rt + value + host binding + spec lock + dbf_ref + ndx_ref)
>>> test-merge-samir: ALL GREEN
```

5. DO-argument promotion remains blocked by independent evidence.

The conditional promotion was not made. Host and emulator still enter PICOUT before reporting the argument's error; real III+ reports it first and passes a ten-column overflow value. Main's authoritative procedure/deferred code was preserved. The cases retain all four explicit diagnostic legs outside default acceptance vectors.

Verbatim clean differences on both host and emulator:

```text
FNS transcript differs at byte 33 (got 0x50, real 0x4d; lengths 155/152)
  got: PICOUT *************
  real: MINT-ERROR         58 LOG() : Zero or negative.
  FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
FNS transcript differs at byte 34 (got 0x50, real 0x4d; lengths 149/146)
  got: PICOUT *************
  real: MINT-ERROR         61 SQRT() : Negative.
  FAIL harness/diff/dbf_diff/test_samir_fns.c:322: FNS transcript byte-identical to real III+
```

6. Resident placement and sizes.

| Artifact | Rebuilt main | Final lane | Delta |
|---|---:|---:|---:|
| Resident load module | 87,184 B | 87,536 B | +352 B |
| SAMIR.EXE file | 90,672 B | 91,024 B | +352 B |
| SAMIR.OVL file | 27,136 B | 27,136 B | 0 B |
| Largest overlay slot | 9,888 B | 9,888 B | 0 B |

The resident budget is 93,776 B, leaving 6,240 B headroom: 2,144 B above the required 4,096 B floor. Baseline main was rebuilt from `git archive origin/main` in `/tmp/logsqrt-r-main-baseline`; `make -C /tmp/logsqrt-r-main-baseline samir-exe` exited 0.

`fn_math_picture` is in resident `.text` at 0x1517. LTO inlines LOG/SQRT/EXP into resident `xb_call_builtin`; objdump locates their FYL2X, FSQRT and F2XM1 instructions in `.text`, below the overlay slot at 0x155f0. NOCROSSREFS, the static call-graph gate and runtime overlay gates pass. No PAL slot was added or changed.

7. Final state.

Authored-source ASCII checks and whitespace checks passed. Raw independent captures retain CRLF and DOS EOF bytes. The tracked working tree and index are clean after the report commit; pre-existing untracked report backups and codex logs were preserved. No push, bd operation, LaTeX build or Bochs run was performed. Worklog: `docs/worklog/wave1-logsqrt-main-replay.md`.
