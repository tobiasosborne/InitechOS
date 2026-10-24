# WL-0179 -- Kernel size round 8: measured, LTO GC repaired, target still open

## Context

Bead initech-hztg, lane/ksize, base 2808f8b5. LANE-BRIEF.md requests every
kernel <=172492 loaded bytes and the flagship with >=16 KiB headroom, with
all acceptance gates run locally. The arrival scan measured all 309 kernels:
flagship 183956 B, binding G12ROW mutant 185292 B. A rebuilt round-7 tree
loads 176756 B, so the flagship grew by exactly 7200 B.

## Change and reason

Final LTO codegen now preserves function/data sections, completing the
round-2 Sec 4 lever C reachability policy. A new independent native-reference
fixture first failed with dead gc_dead_function retained; the fixed driver
removes that dead callee and fixes the C probe entry placement. A mutant
removes the final-codegen section flags and restores the named failure.
The isolated flagship link reclaims 256 B. No TU optimization level, native
hot/IRQ/loader/event boundary, ABI, ISA, spec, boot source, geometry or cap
changed. The r3 budget mutant also retains the real transient-import roots.

## Frictions and limits

More than 200 isolated codegen/outlining trials were measured; most grew the
image. These do not prove every possible source refactor has been exhausted.
The requested size recovery remains unmet. No payload-compression approach
was implemented. The issue stays in progress; there is no complete green
size-round certificate.

The local beads database initially had no issue_prefix or hztg row; repaired
its prefix through bd config, created the brief's explicit ID and claimed it.
The Dolt origin remote is absent. The common Git metadata is read-only, so
normal commit/worktree operations cannot write it in this sandbox.

## Acceptance and pointers

CODEX-OUT.md records the full per-object/symbol measurements, rejected trial
sizes, every final command and its output, and any remaining red gates.
The persistent design record is docs/design/kernel-size-policy-round8.md.
Bochs legs remain with the PM after merge, per the brief; none was run here.

Final measurements: flagship 183700 B (headroom 12908); binding G12ROW
185068 B (headroom 11540). r3/r4 and their mutant prerequisites remain red.
All requested QEMU gates and their behavior mutants passed; the two clean
kernel rebuilds are byte-identical. The native-boundaries FAIL lines are
byte-identical to arrival's 16 test_mzload.c assertions. CODEX-OUT.md includes
every command's final output and the reproducibility hashes.

Commit and staging were attempted but the common Git index is read-only.
Per the brief, changes remain for the PM to commit. Dolt pull/push also fail
because origin is not configured; no endpoint was invented. The issue stays
in_progress with a local JSON snapshot under build/ksize8/bead-final.json.
