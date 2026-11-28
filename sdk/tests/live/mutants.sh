#!/bin/sh
# Live ABI mutants: each patch in sdk/mutants/live changes the tenant runtime,
# which the structural audit accepts; only the booted gate, observed by GDB in
# abi-smoke.sh, can kill it. Kill contract (factory/testlib/README.md): a
# healthy baseline, then the patch's own '# kill:' diagnostic in two failing
# runs, and no emulator run that never completed. Everything runs in an
# isolated snapshot with a debug image; the working tree is never patched.
set -eu
cd "$(dirname "$0")/../../.."
root=$PWD
cflags=$1
mkdir -p build/sdk/live-mutants
work=$(mktemp -d "$root/build/sdk/live-mutants/run.XXXXXX")
start=$(date +%s)
total=0 killed=0 survived=0 errors=0
for patch_file in sdk/mutants/live/*.patch; do total=$((total+1)); done
error_all() {
    echo "!!! sdk live: $1 ($work)"
    echo "sdk: $total mutants, 0 killed, 0 survived, $total errors; $(($(date +%s)-start))s"
    exit 1
}
tree=$work/tree
mkdir -p "$tree"
sh factory/testlib/snapshot.sh "$tree" --exclude=./build --exclude=./.git --exclude=./.beads --exclude=./attic . ||
    error_all 'snapshot failed'
(cd "$tree" && make -j2 image TARGET_CFLAGS="$cflags" &&
    make -C sdk -f tests/live/Makefile TOP="$tree" OUT="$tree/build/sdk/abi-live" ABI=1) \
    > "$work/baseline-build.log" 2>&1 || error_all 'baseline build failed'
: > "$work/baseline.infra"
(cd "$tree" && INITECH_INFRA_LOG="$work/baseline.infra" SDK_ABI_TENANT=build/sdk/abi-live \
    SDK_ABI_OUT="$work/baseline" sh sdk/tests/live/abi-smoke.sh) > "$work/baseline.log" 2>&1 ||
    error_all 'baseline live ABI run failed'
for patch_file in sdk/mutants/live/*.patch; do
    name=$(basename "$patch_file" .patch)
    signature=$(sed -n 's/^# kill: //p' "$patch_file")
    files=$(sed -n 's/^+++ b\///p' "$patch_file")
    mut=$work/$name
    mkdir -p "$mut"
    if [ -z "$signature" ] || [ "$files" != rt/tbx_rt.c ]; then
        echo "!!! sdk live $name: needs one '# kill:' line and must change only rt/tbx_rt.c"; errors=$((errors+1)); continue
    fi
    cp -R "$tree/sdk" "$mut/sdk"
    if ! patch --batch --fuzz=0 -p1 -d "$mut/sdk" < "$patch_file" > "$mut/patch.log" 2>&1; then
        echo "!!! sdk live $name: PATCH FAILED"; errors=$((errors+1)); continue
    fi
    cat > "$mut/override.mk" <<MAKE
\$(OUT)/tenant/tbx_rt.o: $mut/sdk/rt/tbx_rt.c FORCE
	\$(call run-command,\$(TARGET_CC) \$(TARGET_CFLAGS) -I\$(TOP)/sdk/include -MMD -MP -c $mut/sdk/rt/tbx_rt.c -o \$@)
MAKE
    if ! (cd "$tree" && make -C sdk -f tests/live/Makefile -f "$mut/override.mk" TOP="$tree" OUT="$mut/build" ABI=1) > "$mut/build.log" 2>&1; then
        echo "!!! sdk live $name: BUILD FAILED (not a kill)"; errors=$((errors+1)); continue
    fi
    good=1 infra=0
    for pass in 1 2; do
        rc=0; : > "$mut/infra-$pass"
        (cd "$tree" && INITECH_INFRA_LOG="$mut/infra-$pass" SDK_ABI_TENANT="$mut/build" \
            SDK_ABI_OUT="$mut/run-$pass" sh sdk/tests/live/abi-smoke.sh) > "$mut/test-$pass.log" 2>&1 || rc=$?
        [ -z "$(sh factory/testlib/infra-new.sh "$work/baseline.infra" "$mut/infra-$pass")" ] || infra=1
        if [ "$rc" -ne 1 ] || ! grep -Fq "$signature" "$mut/test-$pass.log"; then good=0; fi
    done
    if [ "$infra" -eq 1 ]; then echo "!!! sdk live $name: INFRASTRUCTURE FAILURE (not a kill)"; errors=$((errors+1))
    elif [ "$good" -eq 1 ]; then
        echo "  killed $name ($signature, twice)"; killed=$((killed+1))
        sh "$root/factory/testlib/mutation-receipt.sh" "$root" "$patch_file"
    else echo "!!! sdk live $name: SURVIVED/UNSTABLE (logs $mut/test-*.log)"; survived=$((survived+1)); fi
done
echo "sdk: $total mutants, $killed killed, $survived survived, $errors errors; $(($(date +%s)-start))s"
if [ "$survived" -eq 0 ] && [ "$errors" -eq 0 ]; then rm -rf "$work"; else echo "live evidence: $work"; exit 1; fi
