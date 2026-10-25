# Numeric lane independent review, round 7

PM's independent Sol review required EXP suffix-exhaustion, inline handler
RETURN and argument-phase RETRY corrections. Preserved limitpic, returnarg,
retryconfirm and matrixb from the reviewer's real III+ staging immediately in
commit d0e30d92, including helper programs, input DBFs, wrappers, hashes and
provenance. No copyrighted DBASE files were copied. Source, GOLD.OUT,
MINT.PRG and sha256.txt remain byte-identical to the independent originals.

EXP now replaces its computed ten-column base's fractional suffix using signed
integer room, clamping the final picture. LOG/SQRT retain their existing natural
argument picture rule. The x87 EXP block is unchanged. limitpic reproduces the
reviewer's 2.71828183 and .367879441 outputs across DECIMALS settings.

The shared trap protocol preserves inline RETURN as a distinct outcome. A
normally returning handler procedure still resumes the pending call; inline
RETURN skips it and continues after the DO. returnarg's independent wrapper
prints the end marker after FNSTEST returns; the harness now reproduces that
explicit wrapper action only when present in the minted MINT.PRG, whose bytes
are pinned. It does not add an end marker for other captures.

Argument-phase RETRY keeps the evaluated argument vector. There is no completed
command to restart: it takes the ordinary unknown-command error/trap path,
then resumes the original call after normal handler completion. retryconfirm
pins traps 58 then 16, the original sixteen-star parameter, CNT=2 and ERROR=0.
The mutant that ignores this RETRY fails on host and completed QEMU traces.
The wrong hand-authored transfer RETRY/CANCEL unit expectations were removed;
actual transfer outcomes and received arguments are now graded by the two
independent minted programs. The existing shared deferred unit has 100 checks.

limitpic, returnarg and retryconfirm are ACCEPTED with host/emulator mutants.
matrixb remains PENDING for the PM-owned scientific fallback and SQRT fractional
rounding bead initech-1x6x2.7. Its current host and emulator runs each exit 1 at
SQRT(.999999999), confirming the excluded work was not silently accepted or
fixed. No accepted case contains those two excluded SQRT probes.

Acceptance: make -j64 test-merge-samir with all four explicit decomp directories:
cold run exit 2 at incomplete emulator boot traces; full warm rerun exit 0,
388 gates ALL GREEN. All 64 numeric/NDX/guard case legs (16 x host, mutant,
emu, emu-mutant) pass, including the older LOG/NDX guards. Size, static/runtime
overlays, source manual lockstep and function hash pins pass.

Resident 87,696 -> 87,760 B (+64); headroom 6,016 B against 93,776 B, or
1,920 B above the 4,096 B floor. SAMIR.EXE 91,248 B; SAMIR.OVL 27,136 B;
slot 9,888 B. Authored source is ASCII and whitespace checks pass; independent
captures preserve their original trailing spaces, CRLF and DOS EOF.

Logs: /tmp/logsqrt-r7-red.log, /tmp/logsqrt-r7-host-mutants.log,
/tmp/logsqrt-r7-merge-{cold,warm}.log, /tmp/logsqrt-r7-matrixb-{host,emu}.log.
No push or bd operations. Full handoff is CODEX-OUT.md.
