#!/bin/sh
# Baseline once per selection, strict patch, build, then repeat the same kill.
set -eu
top=$1 component=$2 out=$3 make_cmd=$4
shift 4
[ "$#" -gt 0 ] || { echo "FAIL: no mutation patches selected" >&2; exit 1; }
mkdir -p "$out/mutants"
run=$(mktemp -d "$out/mutants/run.XXXXXX")
start=$(date +%s)
ulimit -c 0
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
n=0 killed=0 survived=0 errors=0
for patch_file do
    n=$((n+1))
    name=$(basename "$patch_file" .patch)
    selection=$(sed -n 's/^# test: //p' "$patch_file")
    key=$(printf '%s' "$selection" | sha256sum | cut -d ' ' -f1)
    baseline=$run/baseline-$key
    set --
    [ -z "$selection" ] || set -- "TESTS=$selection"
    if [ ! -d "$baseline" ]; then
        echo "  baseline ${selection:-default}"
        mkdir -p "$baseline/component"
        cp -R . "$baseline/component/"
        : > "$baseline/infra"
        if INITECH_INFRA_LOG=$baseline/infra INITECH_INFRA_ROOT=$baseline "$make_cmd" --no-print-directory -C "$baseline/component" TOP="$top" OUT="$baseline/build" "$@" test > "$baseline/log" 2>&1; then
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
    if ! "$make_cmd" --no-print-directory -C "$scratch/component" TOP="$top" OUT="$scratch/build" "$@" test-build > "$scratch/build.log" 2>&1; then
        echo "!!! BUILD FAILED (not a kill): $patch_file"; errors=$((errors+1)); continue
    fi
    for pass in 1 2; do
        rm -f "$scratch"/build/host/*.result
        rc=0
        : > "$scratch/infra-$pass"
        INITECH_INFRA_LOG=$scratch/infra-$pass INITECH_INFRA_ROOT=$scratch "$make_cmd" --no-print-directory -C "$scratch/component" TOP="$top" OUT="$scratch/build" "$@" test-run > "$scratch/test-$pass.log" 2>&1 || rc=$?
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
if [ "$survived" -eq 0 ] && [ "$errors" -eq 0 ]; then rm -rf "$run"; else echo "scratch retained: $run"; exit 1; fi
