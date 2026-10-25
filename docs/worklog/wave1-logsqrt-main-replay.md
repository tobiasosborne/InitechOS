# LOG/SQRT/EXP replay on NDX main

Context: PM requested a new lane/wave1-logsqrt-r rooted at main 3c194c50,
replaying the combined numeric lane without its obsolete deferred-v1 code.
No push and no bd operations were requested or performed.

Replayed 6e17e546, the SQRT contribution of merge 7e9edf0d (-m 1),
69347583, e69e059e, d7fdf7da and 92668fb0. The merge replay includes
its d1411291 parent contribution; neither lane-brief commit was replayed.
Resolved old case-list edits to main's F4 marker files, ported LOG/SQRT
manual entries into separate reference and quick-reference files, kept
main's W2/locks/GOLD.OUT/trace/EXE-and-OVL test driver, and combined its
mint_capture.sh path with the lane's external-helper inputs and pins.
Main's eval.h, flow.c, workarea.c and proc.c are unchanged.

New independent evidence: fns-mint/logguards, minted by fns_mint.sh against
real III PLUS 1.1 with declared MINT-ERRORS: 58. It replaces the NDX
R4-GUARD-MIN and R4-CASE-MIN expression with LOG(0)=0. A procedure handler
is bypassed while the false IF and CASE bodies run, leaving CNT/ERROR at
zero. With the inline handler, a false CASE exposes pending ERROR()=58 to
the following CASE and reports #58. No production guard code was changed.
The immediate-error mutant catches a regression on host and QEMU, and
both emu legs prove the complete trace. logguards is ACCEPTED.

The conditional DO-argument promotions remain PENDING. Both host and QEMU
still enter PICOUT before trapping #58/#61, whereas the existing real
captures trap before entering and pass a ten-column overflow value.
Their standard mutant certificates are blocked by the red baselines;
direct wrong-code mutant runs also differ, but are not acceptance proofs.
All four explicit diagnostic legs remain available outside default vectors.
No speculative proc/flow repair was added to main's authoritative mechanism.

Acceptance: make -j64 test-merge-samir with all four explicit decomp paths
passed all 352 gates on the first run (exit 0); no warm retry was needed.
This includes LOG/SQRT/EXP/exppicture/logguards host, mutant, emu and
emu-mutant, foundation, size, static overlay, source manual lockstep and
function-golden hashes. Explicit DO diagnostic matrix exited 2 as above.

Resident 87,184 -> 87,536 B (+352); headroom 6,240 B against 93,776 B,
2,144 B above the 4,096 B floor. SAMIR.EXE 91,024 B; SAMIR.OVL 27,136 B;
slot 9,888 B. The fn_math_picture symbol and inlined LOG/SQRT/EXP x87 paths
are in resident .text; none was moved into an overlay. Link NOCROSSREFS and
static call-graph checks pass. The main baseline was rebuilt from an archive
in /tmp/logsqrt-r-main-baseline, not inferred from a stale build artifact.

Evidence: /tmp/logsqrt-r-merge-cold.log, /tmp/logsqrt-r-doarg-gates.log,
/tmp/logsqrt-r-mint-logguards.log, /tmp/logsqrt-r-main-size.log and
/tmp/logsqrt-r-doarg-raw-results.json. Full handoff is CODEX-OUT.md.
