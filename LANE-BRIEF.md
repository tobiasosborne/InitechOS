# LANE-BRIEF: InitechBase function SQRT() -- one function, one lane

Read CLAUDE.md in full first: Law 1 (cite the local reference), Law 2 (the
real dBASE III PLUS golden is the truth, not your reasoning), Rule 1 (RED
first), Rule 6 (your mutant must bite), Rule 11 (no nondeterminism), Rule 12
(ASCII only). Do NOT git commit or push. Do not run Bochs. Do not run the full
`make test`. Do not edit anything outside the files named below. Other lanes
are implementing the other 17 functions in parallel in their own clones; you
share NOTHING with them except the stub block, so touch only YOUR stub.

## Your function

SQRT -- case name `sqrt`.
Reference (Law 1): ../dbase3-decomp/specs/functions/numeric-and-date-functions.md (search for
`SQRT`), and the III+ HELP text in ../dbase3-decomp/archive/golden-mined/
HELP.DBS.strings.txt. Golden (Law 2): harness/diff/dbf_diff/xbase_prog_diff/
fns-mint/sqrt/GOLD.OUT, produced from FNSTEST.PRG by REAL dBASE III PLUS
1.1 -- read both; every line of GOLD.OUT is what SAMIR must print for that
program, byte for byte, including numeric width/decimals and error numbers.


## What exists

- os/samir/core/fn_builtins.c: your name is already registered in fn_names /
  the FN_ enum / fn_signature[]; the dispatcher calls `fn_sqrt_stub` inside
  the block marked `fan-out wave 2026-10-24`. Replace ONLY that stub's body
  (rename it to fn_sqrt if you like, keeping the call site in sync). If the
  registered signature (arg count/types) disagrees with the III+ spec and the
  golden, fix the signature row for YOUR function only and say so.
- Gate: `make test-samir-fns-sqrt` exists today as a PENDING gate (host
  leg only; Makefile ~34761 SAMIR_FNS_PENDING, `samir-fns-pending-gate`). When
  you move the case to SAMIR_FNS_CASES the full `samir-fns-gates` set appears:
  host, `-mutant`, `-emu`, `-emu-mutant`, and the four join TEST_UNIT/EMU_GATES.
  Run the pending host leg FIRST and paste its RED line (the right reason:
  `got: MINT-ERROR 31 Invalid function name.` vs the real value).
- The test driver (harness/diff/dbf_diff/test_samir_fns.c) already runs the
  mint's ON ERROR line, so MINT-ERROR lines in GOLD.OUT are reproduced by
  returning the right xBase error code from your function: look up the code
  and text in spec/samir/dbase_msg_codes.tsv (e.g. 58 LOG() : Zero or negative,
  61 SQRT() : Negative, 87 NDX(): Invalid index number, 11 Invalid function
  argument). Numeric output width/decimals come from the existing numeric
  formatter (dec_format and friends); match GOLD.OUT, do not hand-pad.
- PAL: os/samir/include/samir/pal.h is the ONLY OS surface (Law 3). Host PAL
  = os/samir/pal/pal_host.c (or the file the Makefile names), Milton PAL =
  os/samir/pal/pal_milton.c (INT 21h only, DEC-04a flat convention). If your
  function needs a new PAL primitive (cursor position, key state, environment
  string, disk free space, colour capability), add ONE small primitive to
  pal.h with BOTH implementations and a comment citing the INT 21h/INT 10h
  service it maps to (ADR-0003 Appendix A; the IBM DOS 3.30 Technical
  Reference pages the Makefile comments cite). Host behaviour must be
  deterministic (fixed values the gate can pin).
- Manuals (lockstep, GC-11): every shipped name must have a Reference entry
  or `make test-pubs` goes RED. Add the entry for SQRT to the InitechBase
  Reference where the neighbouring functions live (find them:
  `grep -rln 'RECSIZE' pubs/manuals/`), in the same shape and register, with
  a worked example whose output is exactly a GOLD.OUT line.

## Steps

1. RED: `make test-samir-fns-sqrt` -> paste the first differing line.
2. Implement; freestanding C, no libm, no libc (SAMIR is a flat .COM on
   InitechDOS); for EXP/LOG/SQRT use x87 instructions through the existing
   numeric helpers (grep `fsqrt\|fyl2x\|f2xm1` in os/samir and os/i123 for the
   period-honest pattern) and match III+ output formatting via the existing
   numeric formatter, never a hand-rolled printf.
3. GREEN: host leg green. Move `sqrt` from SAMIR_FNS_PENDING to
   SAMIR_FNS_CASES in the Makefile (that line will be the only shared edit;
   keep the lists alphabetical so merges are trivial).
4. Mutant: add a `SAMIR_FNS_MUTANT_SQRT` branch in the pattern the
   existing cases use (test_samir_fns.c / the -D in the Makefile) that
   perturbs ONE thing in your implementation (off-by-one, wrong width, wrong
   error number) and confirm `make test-samir-fns-sqrt-mutant` is green
   because the perturbed build goes RED.
5. Emu: `INITECH_QMP_STDIO=1 make test-samir-fns-sqrt-emu
   test-samir-fns-sqrt-emu-mutant` (QEMU boots SAMIR.COM and runs the same
   program). If the emu leg differs from the host leg, the PAL is the bug.
6. `make test-samir test-samir-fns test-samir-fns-mutants` still green.
7. `make test-pubs`: paste the `pubs-samir-inventory` line (expect exactly 131
   shipped names: 130 + yours) and the Reference-entry check for SQRT. ONE
   expected red you must NOT fix: `SCREEN-DRIFT ... gs-dir.txt` (the practice
   diskette's DIR "bytes free" moves when SAMIR.COM crosses a cluster
   boundary; the PM re-captures that screen once after the wave merges).
   Never edit pubs/manuals/*/screens/*; report the drift line verbatim.
8. `ls -l build/SAMIR.COM` before and after: report the byte delta.
9. `LC_ALL=C grep -rnP '[^\x00-\x7F]' <every file you touched>` -> none.
10. `git status --short; git diff --stat`.

Write CODEX-OUT.md: the RED line, the GREEN verdicts verbatim, the mutant
verdict, the emu verdicts, the pubs line, the byte delta, the PAL primitive
you added (if any) with its INT 21h/10h citation, and anything the golden
made you unsure about. Then stop. Do not commit.
