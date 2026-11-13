#!/bin/sh
# Generic patch mutation runner. Invoked by mk/mutants.mk, not by the product.
set -eu
top=$1 component=$2 out=$3 make_cmd=$4
shift 4
mkdir -p "$out/mutants"
run=$(mktemp -d "$out/mutants/run.XXXXXX")
ulimit -c 0
n=0 killed=0 survived=0 errors=0
for patch_file do
    n=$((n + 1))
    name=$(basename "$patch_file" .patch)
    scratch=$run/$name
    mkdir -p "$scratch/component"
    cp -R . "$scratch/component/"
    selection=$(sed -n 's/^# test: //p' "$patch_file")
    # An empty selection means use the default, not TESTS="".
    set --
    if [ -n "$selection" ]; then set -- "TESTS=$selection"; fi
    # Exercise the exact selected suite before patching, including optional
    # sanitizers, so missing tools/references never masquerade as a kill.
    if ! "$make_cmd" --no-print-directory -C "$scratch/component" \
        TOP="$top" OUT="$scratch/build" "$@" test > "$scratch/baseline.log" 2>&1; then
        echo "!!! BASELINE FAILED: $patch_file (log: $scratch/baseline.log)"
        errors=$((errors + 1)); continue
    fi
    if ! patch --batch --fuzz=0 -p1 -d "$scratch/component" < "$patch_file" > "$scratch/patch.log" 2>&1; then
        echo "!!! PATCH FAILED: $patch_file (log: $scratch/patch.log)"
        errors=$((errors + 1)); continue
    fi
    # Fresh output avoids same-second mtimes concealing a patched source.
    rm -rf "$scratch/build"
    if ! "$make_cmd" --no-print-directory -C "$scratch/component" \
        TOP="$top" OUT="$scratch/build" "$@" test-build > "$scratch/build.log" 2>&1; then
        echo "!!! BUILD FAILED (not a kill): $patch_file (log: $scratch/build.log)"
        errors=$((errors + 1)); continue
    fi
    if "$make_cmd" --no-print-directory -C "$scratch/component" \
        TOP="$top" OUT="$scratch/build" "$@" test-run > "$scratch/test.log" 2>&1; then
        echo "!!! MUTANT SURVIVED: $patch_file (log: $scratch/test.log)"
        survived=$((survived + 1))
    else
        echo "  killed $name (log: $scratch/test.log)"
        killed=$((killed + 1))
    fi
done
echo "$component: $n mutants, $killed killed, $survived survived"
if [ "$errors" -ne 0 ]; then echo "!!! $component: $errors mutation setup error(s)"; fi
test "$survived" -eq 0 && test "$errors" -eq 0
