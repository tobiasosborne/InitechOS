#!/bin/sh
# Baseline once per selection, strict patch, build, then repeat the same kill.
set -eu
top=$1 component=$2 out=$3 make_cmd=$4
shift 4
[ "$#" -gt 0 ] || { echo "FAIL: no mutation patches selected" >&2; exit 1; }
# These components already distribute exact selections before invoking us.
case $component in
    flair|apps/initechbase) jobs=1 ;;
    *) jobs=${MUTANT_JOBS:-4} ;;
esac
case $jobs in
    ''|*[!0-9]*|0*) echo 'FAIL: MUTANT_JOBS must be a positive integer' >&2; exit 1 ;;
esac
if [ "$jobs" -gt 1 ] && [ "$#" -gt 1 ]; then
    exec sh "$top/factory/testlib/parallel-mutants.sh" "$top" "$component" "$out" "$make_cmd" "$jobs" "$@"
fi
mkdir -p "$out/mutants"
job=$(mktemp -d "$out/mutants/run.XXXXXX")
start=$(date +%s)
ulimit -c 0
serial_make() {
    "$make_cmd" -j1 --no-print-directory "$@"
}
# An owner can start multiple selection groups in parallel. Each group
# owns the dependency builds used by all of its baseline and patch copies.
mkdir "$job/tree"
(cd "$top" && sh factory/testlib/snapshot.sh "$job/tree" --exclude=./build \
    --exclude=./.git --exclude=./.beads --exclude=./attic --exclude=".${out#"$top"}" .)
top=$job/tree
if [ -n "${INITECH_MUTANT_INPUTS:-}" ]; then
    cp -pR "$INITECH_MUTANT_INPUTS" "$top/build"
else
    # Standalone runs do not get the sweep's pristine image preparation.
    case $component in
        kernel) serial_make -C "$top" catalog > "$job/inputs.log" 2>&1 ;;
        turbo/seed|turbo/compiler) serial_make -C "$top" image > "$job/inputs.log" 2>&1 ;;
    esac
fi
# Keep the graded outputs within the component's tree for ownership audits.
run=$top/build/$component/mutation
mkdir -p "$run"
# Build tools run through recording shims. A tool that fails during a mutant's
# test run (and did not fail the same way in its baseline) is infrastructure:
# the result is an error, never a kill (initech-kkqbw). The emulator harnesses
# append to the same log when a guest run never completed (factory/README.md).
shims=$run/shims
qrecord=$(printf '%s' "$top/factory/testlib/infra-record.sh" | sed "s/'/'\\\\''/g")
mkdir -p "$shims"
# FPC finds ld on PATH and then runs ld.bfd from that same directory, so
# every linker variant present is shimmed too (initech-piuvw).
for tool in cc gcc clang nasm as ld ld.bfd ld.gold ld.lld ar objcopy fpc; do
    real=$(command -v "$tool" 2>/dev/null) || continue
    case $real in /*) ;; *) continue;; esac
    quoted=$(printf '%s' "$real" | sed "s/'/'\\\\''/g")
    printf '#!/bin/sh\n%s "$@" && exit 0\nrc=$?\nsh %s %s "$rc" "$@"\nexit "$rc"\n' \
        "'$quoted'" "'$qrecord'" "$tool" > "$shims/$tool"
    chmod +x "$shims/$tool"
done
PATH=$shims:$PATH
export PATH
# Scope the scratch output to this make invocation. A recursive build can
# still pass a specific OUT when its recipe calls for one.
export INITECH_MUTANT_BUILD=1
mutant_make() {
    source_dir=$1
    shift
    INITECH_MUTANT_SELECTION="$selection" serial_make -C "$source_dir/component" \
        TOP="$top" OUT="$source_dir/build" "$@"
}
n=0 killed=0 survived=0 errors=0
for patch_file do
    n=$((n+1))
    name=$(basename "$patch_file" .patch)
    selection=$(sed -n 's/^# test: //p' "$patch_file")
    key=$(printf '%s' "$selection" | sha256sum | cut -d ' ' -f1)
    baseline=$run/baseline-$key
    if [ ! -d "$baseline" ]; then
        echo "  baseline ${selection:-default}"
        mkdir -p "$baseline/component"
        cp -R . "$baseline/component/"
        : > "$baseline/infra"
        if INITECH_INFRA_LOG=$baseline/infra INITECH_INFRA_ROOT=$baseline mutant_make "$baseline" test > "$baseline/log" 2>&1; then
            touch "$baseline/ok"
        fi
    fi
    if [ ! -f "$baseline/ok" ]; then
        echo "!!! BASELINE FAILED: $patch_file (log: $baseline/log)"; errors=$((errors+1)); continue
    fi
    scratch=$run/$name
    mkdir -p "$scratch/component"
    cp -R . "$scratch/component/"
    if ! patch --batch --fuzz=0 -p1 -d "$scratch/component" < "$patch_file" > "$scratch/patch.log" 2>&1; then
        echo "!!! PATCH FAILED: $patch_file"; errors=$((errors+1)); continue
    fi
    if ! mutant_make "$scratch" test-build > "$scratch/build.log" 2>&1; then
        echo "!!! BUILD FAILED (not a kill): $patch_file"; errors=$((errors+1)); continue
    fi
    for pass in 1 2; do
        rm -f "$scratch"/build/host/*.result
        rc=0
        : > "$scratch/infra-$pass"
        INITECH_INFRA_LOG=$scratch/infra-$pass INITECH_INFRA_ROOT=$scratch mutant_make "$scratch" test-run > "$scratch/test-$pass.log" 2>&1 || rc=$?
        sh "$top/factory/testlib/host-results.sh" "$scratch/build" "$scratch/test-$pass.log" "$rc" > "$scratch/fail-$pass"
        sh "$top/factory/testlib/infra-new.sh" "$baseline/infra" "$scratch/infra-$pass" > "$scratch/infra-$pass.new"
        [ ! -s "$scratch/infra-$pass.new" ] || echo ERROR > "$scratch/fail-$pass"
    done
    if [ -s "$scratch/infra-1.new" ] || [ -s "$scratch/infra-2.new" ]; then
        echo "!!! INFRASTRUCTURE FAILURE (not a kill): $patch_file ($(cat "$scratch/infra-1.new" "$scratch/infra-2.new" | sort -u | tr '\n' ';'))"; errors=$((errors+1))
    elif grep -qx ERROR "$scratch/fail-1" || grep -qx ERROR "$scratch/fail-2"; then
        echo "!!! TEST ERROR (not a kill): $patch_file"; errors=$((errors+1))
    elif grep -q " SEMANTIC " "$scratch/fail-1" && cmp -s "$scratch/fail-1" "$scratch/fail-2"; then
        echo "  killed $name ($(tr '\n' ' ' < "$scratch/fail-1"))"; killed=$((killed+1))
        rm -rf "$scratch"
    else
        echo "!!! SURVIVED/UNSTABLE: $patch_file"; survived=$((survived+1))
    fi
done
echo "$component: $n mutants, $killed killed, $survived survived, $errors errors; $(($(date +%s)-start))s"
if [ "$survived" -eq 0 ] && [ "$errors" -eq 0 ]; then rm -rf "$job"; else echo "scratch retained: $job"; exit 1; fi
