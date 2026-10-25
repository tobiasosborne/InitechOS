# DO WITH deferred completion

Historical round-6 record: its argument-phase RETRY and inline RETURN model
is corrected by `wave1-logsqrt-round7.md`. The hand-authored transfer
RETRY/CANCEL expectations below have been replaced by independent real captures.

Context: PM authorized changes to the shared flow/proc mechanism on
lane/wave1-logsqrt-r after the replay onto NDX main. No push or bd operations.

Independent real III+ evidence corrects one premise of the request: under the
normal inline ON ERROR handler, an argument error is reported BEFORE entering
the procedure, then the procedure runs. The existing LOG/SQRT captures already
showed this. Fresh ndxdoarg (#87) and sqrtpass (SQRT(-1), #61) captures repeat it.
Untrapped and cancelled transfers must not enter; RETRY re-evaluates the DO.

The shared samir_finish_transfer uses the ordinary deferred completion and trap
path before p_call. String arguments are owned before the handler can reset
scratch. No function or catalogue code is special-cased. Existing DO diagnostic
mutants now skip this boundary instead of replacing LOG/SQRT error codes.
A shared unit test verifies ordering, one normal/retried entry, cancellation,
unhandled errors and an empty pending slot before the handler (134 checks total).

Additional real captures pin two necessary general rules. argorder reports the
last failing argument's deferred error, retaining it across later valid args;
first-error-wins remains in force inside each argument expression. argpicture
pins STORE/PARAMETERS nonfinite pictures: retain the incoming columns and add
the decimal suffix (LOG(-100): 7 -> 10, SQRT(-1): 5 -> 8, LOG(0): 4 -> 7).
Finite bindings still use ten integer columns. Independent ordering and binding
mutants catch regressions on host and on completed emulator traces.

Promoted logdoarg and sqrtdoarg without altering their original real captures.
New ndxdoarg, sqrtpass, argorder and argpicture captures are ACCEPTED and pinned.
Removed the obsolete PENDING-only gate scaffolding; Makefile matches main again.
Audit: DO WITH is the implemented expression-to-routine transfer. PARAMETERS
parses names, RUN/! passes literal text to COMMAND.COM, and disk DO accepts bare
names. SET PROCEDURE and user-defined function calls remain unimplemented;
IF/CASE/WHILE keep their separately captured guard rules.

Acceptance: make -j64 test-merge-samir with the four explicit decomp paths:
first run exit 2 only because the source-archive gate saw unstaged deleted
PENDING markers and the output runner's absent CODEX-OUT.md. Staged marker
renames/restored the report, then reran the entire vector: 376 gates ALL GREEN,
exit 0. All 52 numeric/NDX/guard/binding case legs (13 cases x host, mutant,
emu, emu-mutant), plus dedicated NDX classifiers, deferred unit, size, static
and runtime overlays, source manual lockstep and golden hashes pass.

Resident 87,536 -> 87,696 B (+160); headroom 6,080 B against 93,776 B,
1,984 B above the 4,096 B floor. EXE 91,184 B; OVL 27,136 B; slot 9,888 B.
The shared transfer path stays resident. Source ASCII and authored whitespace
checks pass; raw real GOLD.OUT trailing spaces, CRLF and DOS EOF are preserved.

Evidence: /tmp/logsqrt-r6-merge-{cold,warm}.log,
/tmp/logsqrt-r6-red-{existing,new,argorder}.log,
/tmp/logsqrt-r6-host-mutants.log and the committed mint provenance.
