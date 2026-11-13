#!/bin/bash
# Ref: docs/design/parallel-certificate.md Sec 3; initech-9bgg6.
# Independent fake make gates exercise the real runner, without emulators.
set -euo pipefail
export LC_ALL=C
runner=${1:-tools/cert-sharded.sh}
runner=$(cd "$(dirname "$runner")" && pwd)/$(basename "$runner")
work=$(mktemp -d "${TMPDIR:-/tmp}/cert-selftest.XXXXXX")
pid=
cleanup() { if [[ -n $pid ]]; then kill "$pid" 2>/dev/null || :; wait "$pid" 2>/dev/null || :; fi; rm -rf "$work"; }
trap cleanup EXIT
red() { echo "RED cert-runner $1: $2"; exit 1; }
mkdir -p "$work/repo" "$work/out"
cat > "$work/repo/Makefile" <<'EOF'
.DEFAULT_GOAL := help
define misleading-definition
TEST_UNIT_GATES += bogus-$(1)
endef
TEST_UNIT_GATES := good1 good2 good3 good4 good5 good6 good7 good8 good9 good10 good11 good12 bad flaky
TEST_EMU_GATES := emu-good
.PHONY: help clean factory image $(TEST_UNIT_GATES) $(TEST_EMU_GATES)
help clean factory image:
	@true
good1 good2 good3 good4 good5 good6 good7 good8 good9 good10 good11 good12 emu-good:
	@sleep 0.05
bad:
	@false
flaky:
	@test -f ../flake-seen || { touch ../flake-seen; exit 1; }
EOF
git init -q "$work/repo"
git -C "$work/repo" add Makefile
git -C "$work/repo" -c user.name=cert-selftest -c user.email=cert@example.invalid commit -qm fixture
cd "$work/repo"
# Hold the actual queue lock before workers start: no scheduling luck is
# needed to catch a runner with its flock removed (Rule 6).
exec 6> "$work/out/queue.lock"
flock 6
"$runner" -j 4 --out "$work/out" > "$work/run.log" 2>&1 6>&- &
pid=$!
for ((i=0; i<400; i++)); do
    [[ ! -f $work/out/workers.ready ]] || break
    kill -0 "$pid" 2>/dev/null || { cat "$work/run.log"; red STARTUP 'runner stopped before workers'; }
    sleep 0.05
done
[[ -f $work/out/workers.ready ]] || red STARTUP 'workers never became ready'
sleep 0.3
if [[ -s $work/out/results.txt ]]; then
    flock -u 6
    wait "$pid" || :
    pid=
    red QUEUE-LOCK 'a worker recorded a gate while another process held queue.lock'
fi
flock -u 6
rc=0; wait "$pid" || rc=$?; pid=
[[ $rc == 1 ]] || red VERDICT "expected red exit 1, got $rc"
awk '
    $1 == "PASS" || $1 == "FAIL" { if (++seen[$3] != 1 || NF != 5 || $4 !~ /^[0-9]+$/ || $5 !~ /^w[1-4]$/) exit 1; n++ }
    $1 == "PASS" { pass++ }
    $1 == "FAIL" { fail++ }
    $1 == "FAIL-CONFIRMED" && $3 == "bad" { confirmed++ }
    $1 == "FLAKY" && $3 == "flaky" { flaky++ }
    END { if (n != 15 || pass != 13 || fail != 2 || confirmed != 1 || flaky != 1) exit 1 }
' "$work/out/results.txt" || red RESULTS 'lost/duplicated gates or wrong final verdicts'
for gate in bad flaky; do
    [[ -f $work/out/logs/$gate.log && -f $work/out/logs/$gate.solo.log ]] || red LOGS "$gate lacks both logs"
done
[[ $(git worktree list --porcelain | awk '/^worktree / {n++} END {print n}') == 1 ]] || red CLEANUP 'worker worktrees leaked'
"$runner" --out "$work/out" --diff "$work/out/results.txt" > "$work/equal"
[[ ! -s $work/equal ]] || red DIFF 'identical verdict sets differ'
awk '$1 == "FAIL-CONFIRMED" {$1="PASS"} {print}' "$work/out/results.txt" > "$work/control"
rc=0; "$runner" --out "$work/out" --diff "$work/control" > "$work/diff" || rc=$?
[[ $rc == 1 ]] && grep -qx 'unit bad PASS -> FAIL-CONFIRMED' "$work/diff" || red DIFF 'changed verdict missing'
awk '$3 != "good1"' "$work/out/results.txt" > "$work/missing"
rc=0; "$runner" --out "$work/out" --diff "$work/missing" > "$work/diff" || rc=$?
[[ $rc == 1 ]] && grep -qx 'unit good1 MISSING -> PASS' "$work/diff" || red DIFF 'missing gate not reported'
: > "$work/empty"
rc=0; "$runner" --out "$work/out" --diff "$work/empty" > "$work/diff" || rc=$?
[[ $rc == 1 ]] && grep -qx 'unit good1 MISSING -> PASS' "$work/diff" || red DIFF 'empty interrupted control reversed the comparison'
cp -f "$work/out/results.txt" "$work/before"
rc=0; "$runner" -j 4 --resume --out "$work/out" > "$work/resume.log" 2>&1 || rc=$?
[[ $rc == 1 ]] && cmp -s "$work/before" "$work/out/results.txt" || red RESUME 'completed gates were retried'
printf 'good1\ngood1\nemu-good\n' > "$work/gates"
"$runner" -j 2 --gates "$work/gates" --keep --out "$work/green" > "$work/green.log" 2>&1
grep -q '^ALL GREEN -- 1 host + 1 emu gates, 2 workers,' "$work/green.log" || red GREEN 'wrong subset verdict'
[[ -d $work/green/w1 && -d $work/green/w2 ]] || red KEEP 'worktrees were removed'
"$runner" -j 2 --gates "$work/gates" --resume --out "$work/green" > "$work/cleanup.log" 2>&1
[[ ! -d $work/green/w1 && ! -d $work/green/w2 ]] || red CLEANUP 'resumed kept worktrees leaked'
printf '\nTEST_UNIT_GATES += good1\n' >> Makefile
git add Makefile
git -c user.name=cert-selftest -c user.email=cert@example.invalid commit -qm duplicate
rc=0; "$runner" -j 1 --out "$work/duplicate" > "$work/duplicate.log" 2>&1 || rc=$?
[[ $rc == 2 ]] && grep -q 'gate count mismatch: banner 15/1, deduplicated 14/1' "$work/duplicate.log" || red COUNTS 'deduplicated count mismatch not rejected'
echo 'GREEN test-cert-runner: queue lock, exact coverage, solo FAIL/FLAKY, diff, resume, subset and cleanup'
