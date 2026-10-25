# WL-0185 -- SAMIR overlays Phase B review fixes

The operator accepted the DBASE-style manager, refusal text, heap swap,
93,776 B budget and samir-exe target rename. Rebased the three Phase B commits
with `git rebase --onto main fc1ef161` onto c2ba6e16. Main subsequently advanced
independently; this worktree retains that explicit rebase base. No Phase A
DEC-19 service, loader implementation or kernel changes were carried over.

Conflict resolutions retain F4's ACCEPTED/PENDING registries and SET definition
rows, F1's source lockstep, and F2's scratch locks and completed-trace checks.
The F4 SET handler table and its handlers now reside together in the SET overlay.

The review fixes reject retained slot addresses at registration and audit public
PAL/context fields around each overlay activation. Binding and ownership APIs
cover command/suspend/output/resolver/error hooks, cursor vtables, PAL slots and
retained data. Synchronous NDX callbacks remain legal. Direct raw context writes
are detected before return. All manager reentry is rejected while live. Panic
context goes to stderr before UD2 enters the existing kernel panic screen.

The short-read mutation applies only to DBF handles and proves error 15 after
USE plus a completed shell trace. mzlink rejects relocation fread/fclose errors;
a factory close wrapper exercises the latter. EXE and OVL are real grouped
outputs, with a private missing-OVL rebuild/repro gate. No floor or tolerance
was relaxed. Runtime mutants finish explicit diagnostic panic traces; missing
output, failed launch and incomplete injection cannot count as a mutant bite.

Acceptance commands, all exit codes (including initial RED runs), artifact
sizes and rewritten commit ids are recorded in the session output report:
`build/phaseb-fix-logs/phase-b-o.txt`. Full command output is beside that report.
The initial full merge run caught a missing report in the new swap-mutant log;
the actual F2 trace check had passed, and the harness now includes that report.
F1 also caught its colophon syntax-version pin after the accepted COM-to-EXE
rename; the real typesetting/replay session gates precede its deliberate update.

Operator instruction: commit locally, do not push.
