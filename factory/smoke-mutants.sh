#!/bin/sh
# Product faults exercised through the real image. Retain all evidence.
set -eu
cd "$(dirname "$0")/.."
root=$PWD
mkdir -p build/factory/smoke-mutants
work=$(mktemp -d "$root/build/factory/smoke-mutants/run.XXXXXX")
killed=0 survived=0 errors=0 total=0
snapshot() {
 mkdir -p "$1"
 tar -cf - --exclude=./build --exclude=./.git --exclude=./.beads --exclude=./attic . | tar -xf - -C "$1"
}
baseline=$work/baseline
snapshot "$baseline"
mkdir -p "$baseline/build"
tar -cf - --exclude=./factory/gate-mutants --exclude=./factory/smoke-mutants --exclude=./factory/mutants --exclude=./factory-gates --exclude=./smoke* --exclude=./test -C build . | tar -xf - -C "$baseline/build"
# No test scenario or wrong code is compiled into the ordinary product.
if ! (cd "$baseline" && make -j2 image && sh factory/smoke.sh && sh factory/tests/desktop-capture.sh) > "$work/baseline.log" 2>&1; then
 echo "factory: 5 mutants, 0 killed, 0 survived, 1 errors; baseline failed: $work/baseline.log"; exit 1
fi
for patch_file in factory/mutants/smoke/*.patch; do
 total=$((total+1));name=$(basename "$patch_file" .patch);scratch=$work/$name
 cp -R "$baseline" "$scratch"
 if ! patch --batch --fuzz=0 -p1 -d "$scratch" < "$patch_file" > "$work/$name.patch.log" 2>&1 ||
    ! (cd "$scratch" && make -j2 image) > "$work/$name.build.log" 2>&1; then
  errors=$((errors+1));echo "FAIL $name patch/build";continue
 fi
 case $name in
 copy-corrupt) command='sh factory/smoke.sh'; signature='FAIL SAVED copied/renamed bytes';;
 rename-copy) command='sh factory/smoke.sh'; signature='FAIL REN left its source name';;
 mkdir-fail) command='sh factory/smoke.sh'; signature='FAIL MD did not persist EMPTY directory';;
 find-invert) command='sh factory/smoke.sh'; signature='FAIL FIND file bytes';;
 icon-black) command='sh factory/tests/desktop-capture.sh'; signature='FAIL independent desktop pixels: desktop';;
 *) echo "FAIL unknown smoke mutant $name";exit 1;;
 esac
 good=1
 for pass in 1 2; do
  rc=0;(cd "$scratch" && sh -c "$command") > "$work/$name.$pass.log" 2>&1 || rc=$?
  if [ "$rc" -eq 0 ] || ! grep -F "$signature" "$work/$name.$pass.log" >/dev/null; then good=0; fi
 done
 if [ "$good" -eq 1 ]; then echo "  killed $name (two semantic failures)";killed=$((killed+1))
 else echo "FAIL $name survived or wrong failure";survived=$((survived+1));fi
done
echo "factory: $total mutants, $killed killed, $survived survived, $errors errors; smoke evidence: $work"
test "$survived" -eq 0 && test "$errors" -eq 0
