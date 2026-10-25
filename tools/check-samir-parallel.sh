#!/bin/sh
# Acceptance evidence, PRD 8 / Rule 11. Never normalize a failing gate away.
# Usage: sh tools/check-samir-parallel.sh [output-directory]
set -eu
out=${1:-build/samir-safety}
mkdir -p "$out"
out=$(CDPATH= cd -- "$out" && pwd)
cat > "$out/gates.mk" <<'MAKE'
.PHONY: evidence-gates
evidence-gates:
	@printf '%s\n' $(MERGE_SAMIR_GATES)
MAKE
: > "$out/timings.txt"
for run in parallel1 parallel2 parallel3 serial; do
 jobs=64; [ "$run" != serial ] || jobs=1
 make -s -f Makefile -f "$out/gates.mk" evidence-gates > "$out/$run.gates"
 # Match the optional source-only gate detection used by the runner.
 if make -n test-pubs-lockstep-src >/dev/null 2>&1; then echo test-pubs-lockstep-src >> "$out/$run.gates"; fi
 sort -u "$out/$run.gates" -o "$out/$run.gates"
 start=$(date +%s)
 rc=0; make --output-sync=target "-j$jobs" test-merge-samir > "$out/$run.log" 2>&1 || rc=$?
 elapsed=$(( $(date +%s) - start ))
 printf 'make -j%s test-merge-samir (%s): exit=%s elapsed=%ss\n' "$jobs" "$run" "$rc" "$elapsed" | tee -a "$out/timings.txt"
 [ "$rc" = 0 ] && grep -qx '>>> test-merge-samir: ALL GREEN' "$out/$run.log" || { tail -n 40 "$out/$run.log"; exit 1; }
 # Each member is GREEN only when make certified the entire expanded vector.
 awk '{print $0 " GREEN"}' "$out/$run.gates" > "$out/$run.verdicts"
done
for run in parallel1 parallel2 parallel3; do
 diff -u "$out/serial.verdicts" "$out/$run.verdicts" > "$out/$run.diff"
 printf '%s vs serial: identical (%s GREEN gates)\n' "$run" "$(wc -l < "$out/$run.verdicts")" | tee -a "$out/timings.txt"
done
