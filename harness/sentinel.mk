# Local post-push sentinel oracle (initech-3nc3v); PRD Sec 8/9/14.
# The fixture uses F8's real C gate mapper and make dispatcher, never the OS
# certificate. $(value) writes this POSIX shell verbatim into build/.
define SENTINEL_SELFTEST_SH
#!/bin/sh
set -eu
export LC_ALL=C
unset MAKEFLAGS MFLAGS MAKELEVEL MAKEOVERRIDES GNUMAKEFLAGS MAKEFILES
source_repo=$1
scripts=$2
out=$3
mkdir -p "$out"
fixture=$(mktemp -d "$out/case.XXXXXX")
repo=$fixture/pm
fail() { printf 'RED sentinel %s: %s\n' "$1" "$2"; exit 1; }
need() { grep -Fxq "$2" "$1" || fail "$3" "missing: $2 (in $1)"; }
for tool in sentinel culprit; do
    [ -f "$scripts/$tool.sh" ] || fail SETUP "missing tools/$tool.sh"
done
git init -q --bare "$fixture/origin.git"
git init -q -b main "$repo"
git -C "$repo" config user.name 'Sentinel Fixture'
git -C "$repo" config user.email 'sentinel@example.invalid'
git -C "$repo" config commit.gpgsign false
mkdir -p "$repo/tools" "$repo/harness" "$repo/spec"
cp -f "$scripts/sentinel.sh" "$scripts/culprit.sh" "$repo/tools/"
cp -f "$source_repo/tools/gate_map.c" "$repo/tools/"
cp -f "$source_repo/harness/gate_map.mk" "$repo/harness/"
cat > "$repo/Makefile" <<'MAKE'
BUILD := build
CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror
TEST_UNIT_GATES := test-probe test-unrelated
TEST_EMU_GATES :=
.PHONY: all test-probe test-unrelated
all:
# Tiny fixture stand-ins for the mandatory metadata gates in F8's schema.
.PHONY: test-makefile-vars test-gate-honesty test-kernel-repro test-kernel-flair-repro test-ovl-repro
test-makefile-vars test-gate-honesty test-kernel-repro test-kernel-flair-repro test-ovl-repro:
	@:
$(BUILD):
	@mkdir -p $@
test-probe: | $(BUILD)
	@printf '%s\n' '$(MAKEFLAGS)' >> build/probe-jobs
	@if [ "$${SENTINEL_FIXTURE_FLAKE:-0}" = 1 ]; then \
	    if [ ! -f build/probe.once ]; then touch build/probe.once; exit 1; fi; \
	else test "$$(cat gate.state)" = green; fi
test-unrelated:
	@echo 'unrelated gate must not be selected'; exit 1
include harness/gate_map.mk
MAKE
cat > "$repo/spec/gate_map.json" <<'JSON'
{"families":{"always":{"gates":["test-makefile-vars","test-gate-honesty","test-gate-map","test-gate-map-mutant"],"paths":[]},"build-system":{"gates":["test-kernel-repro","test-kernel-flair-repro","test-ovl-repro"],"paths":["Makefile","tools/**","harness/**","spec/**"]},"aggregates":{"gates":["test-for"],"paths":[]},"probe":{"gates":["test-probe"],"paths":["probe.txt","gate.state","notes.txt"]},"other":{"gates":["test-unrelated"],"paths":["other.txt"]}}}
JSON
printf 'build/\n' > "$repo/.gitignore"
printf 'green\n' > "$repo/gate.state"
printf 'root\n' > "$repo/probe.txt"
git -C "$repo" add .
git -C "$repo" commit -qm 'fixture root'
base=$(git -C "$repo" rev-parse HEAD)
for n in 1 2 3; do
    git -C "$repo" checkout -qb "lane$n"
    if [ "$n" = 2 ]; then printf 'red\n' > "$repo/gate.state"; fi
    if [ "$n" = 3 ]; then printf 'unrelated doc\n' > "$repo/notes.txt"
    else printf 'merge %s\n' "$n" > "$repo/probe.txt"; fi
    git -C "$repo" add .
    git -C "$repo" commit -qm "lane $n"
    git -C "$repo" checkout -q main
    git -C "$repo" merge -q --no-ff "lane$n" -m "M$n fixture lane"
    git -C "$repo" rev-parse HEAD > "$fixture/m$n"
done
m1=$(cat "$fixture/m1"); m2=$(cat "$fixture/m2"); m3=$(cat "$fixture/m3")
git -C "$repo" remote add origin "$fixture/origin.git"
git -C "$repo" push -q origin main
git --git-dir="$fixture/origin.git" symbolic-ref HEAD refs/heads/main
cd "$repo"
mkdir -p build/sentinel
printf '%s\n' "$base" > build/sentinel/last-green
export SENTINEL_FIXTURE_CERT_LOG=$fixture/cert.calls
export SENTINEL_CERT_CMD=$fixture/fake-cert
export SENTINEL_LOADAVG=0
cat > "$SENTINEL_CERT_CMD" <<'CERT'
#!/bin/sh
set -eu
printf '%s\n' "$*" >> "$SENTINEL_FIXTURE_CERT_LOG"
[ "$1" = -j ] && [ "$2" = 3 ] && [ "$3" = --out ]
mkdir -p "$4"
if [ "${SENTINEL_FIXTURE_CERT_RED:-0}" = 1 ]; then
    printf 'FAIL unit test-probe 0 w1\nFLAKY unit test-probe 0 w1\n' > "$4/results.txt"
    exit 1
fi
printf 'PASS unit test-probe 0 w1\n' > "$4/results.txt"
CERT
chmod +x "$SENTINEL_CERT_CMD"
run() {
    want=$1; shift
    rc=0; tools/sentinel.sh --once --jobs 3 "$@" > "$fixture/run.log" 2>&1 || rc=$?
    if [ "$rc" != "$want" ]; then cat "$fixture/run.log"; fail "$leg" "exit $rc, expected $want"; fi
    summary=build/sentinel/$(git rev-parse origin/main)/summary
    [ -f "$summary" ] || fail "$leg" 'missing summary'
}
leg=RED
run 1
need "$summary" "base $base" RANGE
need "$summary" "head $m3" RANGE
need "$summary" 'stage-1 gates 5' RANGE
need "$summary" 'stage-1 red test-probe' RANGE
need "$summary" 'stage-1 result red' RED
need "$summary" "CULPRIT test-probe $m2 M2 fixture lane" CULPRIT
need "$summary" "tools/revert-merge.sh -y -r 'sentinel: test-probe' -l 'build/sentinel/$m3/gates.log' $m2" REVERT
need build/sentinel/last-green "$base" RED
grep -Eq -- '(^| )-j3( |$)' ../initech-os-sentinel/build/probe-jobs || fail JOBS 'requested -j3 not observed'
if grep -Eq -- '(^| )-j(8|16)( |$)' ../initech-os-sentinel/build/probe-jobs; then fail JOBS 'F8 recursive -j16 escaped the bound'; fi
cp -f "$summary" "$fixture/red.summary"
leg=CERT
SENTINEL_FIXTURE_CERT_RED=1 run 1
need "$summary" 'stage-2 result red' CERT
need "$summary" 'FLAKY test-probe stage-2' CERT
need "$summary" "CULPRIT test-probe $m2 M2 fixture lane" CERT
# Rewind only the throwaway bare origin, so the next push snapshot is green.
git --git-dir="$fixture/origin.git" update-ref refs/heads/main "$m1"
rm -f build/sentinel/last-green
leg=GREEN
run 0
need "$summary" "base $base" BASE
need "$summary" 'base-source origin/main~1 (last-green absent)' BASE
need "$summary" 'stage-1 result green' GREEN
need "$summary" 'stage-2 result green' GREEN
need "$summary" 'result green' GREEN
need build/sentinel/last-green "$m1" GREEN
grep -q 'last-green absent' "$fixture/run.log" || fail BASE 'fallback was silent'
calls=$(wc -l < "$SENTINEL_FIXTURE_CERT_LOG")
leg=NO-CERT
run 0 --no-cert
need "$summary" 'stage-2 result skipped (--no-cert)' NO-CERT
[ "$(wc -l < "$SENTINEL_FIXTURE_CERT_LOG")" = "$calls" ] || fail NO-CERT 'certificate invoked'
SENTINEL_HEAD=$m1 tools/culprit.sh test-probe "$base" > "$fixture/green-culprit.log"
need "$fixture/green-culprit.log" 'no culprit in range (flake class)' CULPRIT
# The production sleep remains 60 seconds; only the fixture PATH replaces it.
mkdir -p "$fixture/bin"
cat > "$fixture/bin/sleep" <<'SLEEP'
#!/bin/sh
printf '%s\n' "$*" >> "$SENTINEL_FIXTURE_SLEEP_LOG"
SLEEP
chmod +x "$fixture/bin/sleep"
export SENTINEL_FIXTURE_SLEEP_LOG=$fixture/sleep.calls
printf '%s\n' "$base" > build/sentinel/last-green
leg=LOAD
PATH="$fixture/bin:$PATH" SENTINEL_LOADAVG=999999999 run 3
need "$summary" 'stage-2 result deferred' LOAD
need "$summary" 'result deferred' LOAD
need build/sentinel/last-green "$base" LOAD
[ "$(wc -l < "$SENTINEL_FIXTURE_SLEEP_LOG")" -eq 10 ] || fail LOAD 'expected ten retries'
[ "$(sort -u "$SENTINEL_FIXTURE_SLEEP_LOG")" = 60 ] || fail LOAD 'retry interval was not 60 seconds'
[ "$(wc -l < "$SENTINEL_FIXTURE_CERT_LOG")" = "$calls" ] || fail LOAD 'certificate ran under high load'
git --git-dir="$fixture/origin.git" update-ref refs/heads/main "$m3"
leg=FLAKE
SENTINEL_FIXTURE_FLAKE=1 run 1 --no-cert
need "$summary" 'FLAKY test-probe stage-1' FLAKE
need "$summary" 'stage-1 red test-probe' FLAKE
need "$summary" 'stage-1 result red' FLAKE
need "$summary" 'result red' FLAKE
need build/sentinel/last-green "$base" FLAKE
grep -q '^FLAKY test-probe ' "$summary" || fail FLAKE 'solo rerun not reported'
cp -f "$summary" "$fixture/flake.summary"
[ "$(git rev-parse HEAD)" = "$m3" ] || fail SAFETY 'PM HEAD changed'
[ "$(git symbolic-ref --short HEAD)" = main ] || fail SAFETY 'PM branch changed'
[ -z "$(git status --porcelain)" ] || fail SAFETY 'PM working tree changed'
[ -z "$(git -C ../initech-os-sentinel symbolic-ref -q HEAD || :)" ] || fail SAFETY 'sentinel is attached'
[ "$(git worktree list --porcelain | grep -c '^worktree ')" -eq 2 ] || fail SAFETY 'culprit leaked a worktree'
printf '>>> test-sentinel: green (range, M2 culprit/revert, green pointer, fake cert, load guard, FLAKY stays red)\n'
printf 'Fixture artifacts: %s\n' "$fixture"
endef

.PHONY: test-sentinel test-sentinel-mutant
test-sentinel: | $(BUILD)
	@$(file >$(BUILD)/sentinel-selftest.sh,$(value SENTINEL_SELFTEST_SH))
	@sh $(BUILD)/sentinel-selftest.sh "$(CURDIR)" "$(CURDIR)/tools" "$(abspath $(BUILD)/test-sentinel)"

test-sentinel-mutant: | $(BUILD)
	@$(file >$(BUILD)/sentinel-selftest.sh,$(value SENTINEL_SELFTEST_SH))
	@set -eu; for m in newest-first flake-excused; do \
	    d="$(abspath $(BUILD))/sentinel-mutant-$$m"; mkdir -p "$$d/tools"; \
	    cp -f tools/sentinel.sh tools/culprit.sh "$$d/tools/"; \
	    if [ "$$m" = newest-first ]; then \
	        sed -e 's/--reverse --first-parent/--first-parent/' -e 's/: # GREEN-CONTINUE/break # mutant stops at first green/' tools/culprit.sh > "$$d/tools/culprit.sh"; \
	        reason=CULPRIT; \
	    else \
	        sed 's|: # FLAKE-KEEPS-RED|stage1_rc=0; sed "/^$$gate/d" "$$run/stage-1.red" > "$$run/red.next"; mv -f "$$run/red.next" "$$run/stage-1.red"|' tools/sentinel.sh > "$$d/tools/sentinel.sh"; \
	        reason=FLAKE; \
	    fi; \
	    rc=0; sh $(BUILD)/sentinel-selftest.sh "$(CURDIR)" "$$d/tools" "$$d" > "$$d/oracle.log" 2>&1 || rc=$$?; \
	    [ "$$rc" = 1 ] && grep -q "^RED sentinel $$reason:" "$$d/oracle.log" || { cat "$$d/oracle.log"; printf 'RED sentinel mutant survived or failed for wrong reason: %s\n' "$$m"; exit 1; }; \
	    grep "^RED sentinel $$reason:" "$$d/oracle.log"; \
	    printf 'Mutant %s: RED as required\n' "$$m"; \
	done; printf '>>> test-sentinel-mutant: green (newest-first and flake-excused correctly RED)\n'

TEST_UNIT_GATES += test-sentinel test-sentinel-mutant
