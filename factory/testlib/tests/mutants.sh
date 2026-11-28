#!/bin/sh
# Independent negative controls for the production sweep, not component graders.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
sweep=${MUTATION_SWEEP:-$root/factory/mutants.sh}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
export TMPDIR=$work
failed=0
fixture() {
 case_dir=$work/$1
 mkdir -p "$case_dir/a/mutants"
 : > "$case_dir/a/mutants/a.patch"
 printf '.PHONY: mutants\nmutants:\n\t@%s\n' "$2" > "$case_dir/a/Makefile"
}
run_red() {
 rc=0
 (cd "$case_dir" && sh "$sweep" "$case_dir/out") > "$case_dir/result.log" 2>&1 || rc=$?
 if [ "$rc" -eq 0 ]; then
  echo "FAIL false pass: $1"; cat "$case_dir/result.log"; failed=$((failed+1))
 else echo "ok rejected: $1"; fi
}
fixture survivor 'echo "a: 1 mutants, 0 killed, 1 survived, 0 errors"'
run_red 'survivor with successful target exit'
fixture duplicate 'printf "%s\n" "INITECH_MUTANT_KILLED a/mutants/a.patch" "INITECH_MUTANT_KILLED a/mutants/a.patch" "a: 2 mutants, 2 killed, 0 survived, 0 errors"'
: > "$case_dir/a/mutants/no-op.patch"
run_red 'duplicate patch hides a missing patch with equal counts'
grep -Fq 'duplicate patch receipt a/mutants/a.patch' "$case_dir/result.log"
grep -Fq 'missing patch receipt a/mutants/no-op.patch' "$case_dir/result.log"
fixture no_receipt 'echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"'
run_red 'successful count without a named receipt'
fixture wrong_receipt 'printf "%s\n" "INITECH_MUTANT_KILLED a/mutants/other.patch" "a: 1 mutants, 1 killed, 0 survived, 0 errors"'
run_red 'successful count with an unexpected patch'
fixture errors 'echo "a: 1 mutants, 0 killed, 0 survived, 1 errors"'
run_red 'error with successful target exit'
fixture empty 'echo "a: 0 mutants, 0 killed, 0 survived, 0 errors"'
run_red 'unexecuted active patch'
fixture empty_inventory 'echo "a: 0 mutants, 0 killed, 0 survived, 0 errors"'
rm "$case_dir/a/mutants/a.patch"
run_red 'empty global active inventory'
fixture short 'echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"'
: > "$case_dir/a/mutants/b.patch"
run_red 'partial summary missing an active patch'
fixture malformed 'echo "a: nope mutants, 1 killed, 0 survived, 0 errors"'
run_red 'nonnumeric summary'
fixture silent ':'
run_red 'missing summary'
fixture nonzero 'echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"; exit 1'
run_red 'failed target with passing counts'
fixture pending 'exit 1'
mkdir "$case_dir/a/mutants/pending"
mv "$case_dir/a/mutants/a.patch" "$case_dir/a/mutants/pending/"
mkdir -p "$case_dir/b/mutants"
: > "$case_dir/b/mutants/b.patch"
printf '.PHONY: mutants\nmutants:\n\t@echo "b: 1 mutants, 1 killed, 0 survived, 0 errors"\n' > "$case_dir/b/Makefile"
run_red 'unimplemented pending-only target'
fixture orphan ':'
rm "$case_dir/a/Makefile"
run_red 'orphan mutation directory'
fixture killed 'echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"; sh stop-worker.sh'
printf '.PHONY: mutants\nmutants:\n\t@echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"\n\t@sh stop-worker.sh\n' > "$case_dir/a/Makefile"
cat > "$case_dir/a/stop-worker.sh" <<'CONTROL'
set -eu
worker=$(ps -o ppid= -p "$PPID" | tr -d ' ')
test "$(ps -o args= -p "$worker")" = "$MUTANT_EXPECTED_SUPERVISOR"
echo killed > "$MUTANT_KILL_PROOF"
kill -KILL "$worker"
CONTROL
export MUTANT_EXPECTED_SUPERVISOR="sh $sweep $case_dir/out"
export MUTANT_KILL_PROOF=$case_dir/killed-proof
# A previous invocation's success record must not conceal this killed worker.
mkdir -p "$case_dir/out/mutants"
echo 0 > "$case_dir/out/mutants/a.rc"
run_red 'killed worker, missing completion, stale previous success'
test -f "$MUTANT_KILL_PROOF"
fixture missing_log 'echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"; sh remove-log.sh'
cat > "$case_dir/a/remove-log.sh" <<'CONTROL'
set -eu
log=$(readlink /proc/$$/fd/1)
rm "$log"
CONTROL
run_red 'missing worker log despite successful exit'
fixture missing_result 'echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"; sh obstruct-result.sh'
cat > "$case_dir/a/obstruct-result.sh" <<'CONTROL'
set -eu
log=$(readlink /proc/$$/fd/1)
mkdir "${log%.log}.rc"
CONTROL
run_red 'missing regular completion record despite successful worker exit'
# Positive controls: grouped counts, reused external patch, explicit pending.
fixture green 'printf "%s\n" "INITECH_MUTANT_KILLED a/mutants/a.patch" "INITECH_MUTANT_KILLED shared.patch"; echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"; echo "a: 1 mutants, 1 killed, 0 survived, 0 errors"'
: > "$case_dir/shared.patch"
echo shared.patch > "$case_dir/a/mutants/extra.list"
mkdir "$case_dir/a/mutants/pending"
: > "$case_dir/a/mutants/pending/later.patch"
mkdir -p "$case_dir/b/mutants/pending"
: > "$case_dir/b/mutants/pending/later.patch"
printf '.PHONY: mutants\nmutants:\n\t@echo "b: 0 mutants, 0 killed, 0 survived, 0 errors"\n' > "$case_dir/b/Makefile"
if (cd "$case_dir" && sh "$sweep" "$case_dir/out") > "$case_dir/result.log" 2>&1; then
 grep -q 'INCOMPLETE: 2 pending patches remain unproved' "$case_dir/result.log"
 test "$(cat "$case_dir/out/mutants/pending-count")" -eq 2
 test "$(cat "$case_dir/out/mutants/b.counts")" = '0 0 0 0'
 echo 'ok active kills accepted; pending coverage explicitly incomplete'
else cat "$case_dir/result.log"; failed=$((failed+1)); fi

# Force a reader to run during a second worker's dependency relink. Only the
# rendezvous files are shared; an installed executable belongs to its worker.
fixture dependency 'sh ../dependency.sh read && printf "%s\n" "INITECH_MUTANT_KILLED a/mutants/a.patch" "a: 1 mutants, 1 killed, 0 survived, 0 errors"'
mkdir -p "$case_dir/b/mutants" "$case_dir/sync"
: > "$case_dir/b/mutants/b.patch"
printf '%s\n' '.PHONY: mutants' 'mutants:' '	@sh ../dependency.sh relink && printf "%s\n" "INITECH_MUTANT_KILLED b/mutants/b.patch" "b: 1 mutants, 1 killed, 0 survived, 0 errors"' > "$case_dir/b/Makefile"
cat > "$case_dir/dependency.sh" <<'CONTROL'
set -eu
cd ..
tool=build/shared/generate
mkdir -p build/shared
await() {
    timeout 10 sh -c 'while [ ! -f "$1" ]; do sleep 0.02; done' sh "$MUTANT_RENDEZVOUS/$1"
}
case $1 in
    read)
        printf '#!/bin/sh\necho generated\n' > "$tool"
        chmod +x "$tool"
        touch "$MUTANT_RENDEZVOUS/installed"
        await relinking
        rc=0
        "$tool" > build/shared/header || rc=$?
        touch "$MUTANT_RENDEZVOUS/read"
        [ "$rc" -eq 0 ] || exit "$rc"
        grep -qx generated build/shared/header
        ;;
    relink)
        await installed
        rm -f "$tool"
        printf '#!/bin/sh\necho generated\n' > "$tool"
        touch "$MUTANT_RENDEZVOUS/relinking"
        await read
        chmod +x "$tool"
        ;;
esac
CONTROL
if (cd "$case_dir" && MUTANT_RENDEZVOUS="$case_dir/sync" CERT_MUTANT_JOBS=2 sh "$sweep" "$case_dir/out") > "$case_dir/result.log" 2>&1; then
 test ! -e "$case_dir/build"
 echo 'ok parallel workers own their dependency builds'
else
 cat "$case_dir/result.log" "$case_dir/out/mutants/a.log"; failed=$((failed+1))
fi
test "$failed" -eq 0
echo 'mutation sweep controls: failures propagate, counts complete, pending explicit'
