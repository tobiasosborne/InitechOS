#!/bin/sh
# Strict patch/build, clean baseline first, repeated semantic red; no timeout
# or build/audit failure is accepted as a kill. Used by factory/mutants.sh.
set -eu
top=$1 out=$2 make_cmd=$3
shift 3
[ "$#" -gt 0 ] || { echo 'ERROR boot: empty mutation selection'; exit 1; }
run=$(mktemp -d "$out/mutants.XXXXXX")
echo "boot mutation evidence: $run"
n=0 killed=0 survived=0 errors=0
for patch_file do
    n=$((n+1))
    name=$(basename "$patch_file" .patch)
    scenario=$(sed -n 's/^# test: //p' "$patch_file")
    case $scenario in normal|indexed|ORDER|ATTR_R|CHAIN) ;; *) echo "ERROR missing scenario: $patch_file"; exit 1;; esac
    if [ ! -f "$run/baseline-$scenario.ok" ]; then
        if sh tests/boot.sh "$top" "$out" "$scenario" > "$run/baseline-$scenario.log" 2>&1; then
            touch "$run/baseline-$scenario.ok"
        else
            cat "$run/baseline-$scenario.log"; echo "ERROR baseline $scenario"; exit 1
        fi
    fi
    scratch=$run/$name
    mkdir -p "$scratch/component"
    cp -R . "$scratch/component/"
    if ! patch --batch --fuzz=0 -p1 -d "$scratch/component" < "$patch_file" > "$scratch/patch.log" 2>&1; then
        echo "ERROR patch $name"; errors=$((errors+1)); continue
    fi
    if ! "$make_cmd" --no-print-directory -C "$scratch/component" TOP="$top" OUT="$scratch/build" smoke-build > "$scratch/build.log" 2>&1; then
        echo "ERROR build $name (not a kill)"; errors=$((errors+1)); continue
    fi
    good=1
    for pass in 1 2; do
        rc=0
        sh tests/boot.sh "$top" "$scratch/build" "$scenario" > "$scratch/red-$pass.log" 2>&1 || rc=$?
        if [ "$rc" -ne 1 ] || grep -q '^ERROR ' "$scratch/red-$pass.log" ||
           ! grep -q '^RED boot ' "$scratch/red-$pass.log"; then good=0; fi
        sed -n '/^RED boot /p' "$scratch/red-$pass.log" > "$scratch/reasons-$pass"
    done
    if [ "$good" -eq 1 ] && cmp -s "$scratch/reasons-1" "$scratch/reasons-2"; then
        echo "  killed $name"
        cat "$scratch/reasons-1"
        killed=$((killed+1))
    else echo "ERROR survived/unstable $name"; survived=$((survived+1)); fi
done
echo "boot: $n mutants, $killed killed, $survived survived, $errors errors"
test "$survived" -eq 0 && test "$errors" -eq 0
