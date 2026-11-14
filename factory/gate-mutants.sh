#!/bin/sh
# Fresh source copies; product source is never mutated in the checkout.
set -eu
cd "$(dirname "$0")/.."
root=$PWD
mkdir -p build/factory/gate-mutants
work=$(mktemp -d "$root/build/factory/gate-mutants/run.XXXXXX")
killed=0 survived=0 errors=0 total=0
for patch_file in factory/mutants/gates/*.patch; do
 total=$((total+1)); name=$(basename "$patch_file" .patch); scratch=$work/$name
 mkdir -p "$scratch"
 tar -cf - --exclude=./build --exclude=./.git --exclude=./.beads --exclude=./attic . | tar -xf - -C "$scratch"
 if ! patch --batch --fuzz=0 -p1 -d "$scratch" < "$patch_file" > "$work/$name.patch.log" 2>&1; then errors=$((errors+1)); continue; fi
 case $name in
 worksheet-*) command='sh factory/tests/worksheet.sh'; signature='FAIL worksheet:';;
 bochs-timeout|bochs-exit) command='make -s -C factory ../build/factory/bochs-verdict-test'; signature='FAIL Bochs:'
  # Compile explicitly: OUT is an absolute path in component makefiles.
  command='mkdir -p build/factory; cc -std=c11 -O1 -D_POSIX_C_SOURCE=200809L factory/tests/bochs-verdict.c -o build/factory/verdict && build/factory/verdict';;
 empty-registration) command='make -s -C flair/region test-run'; signature='empty test registration/selection';;
 qemu-trace-truncated) command='mkdir -p build/factory; cc -std=c11 -O1 factory/tests/qemu-trace.c -o build/factory/trace && build/factory/trace'; signature='FAIL QEMU trace:';;
 qemu-completion-cap) command='mkdir -p build/factory; cc -std=c11 -O1 factory/tests/qemu-deadline.c -o build/factory/deadline && build/factory/deadline'; signature='FAIL QEMU deadline:';;
 occupied-port) command='mkdir -p build/factory; cc -std=c11 -O1 factory/tests/port-lock.c -o build/factory/port-lock && build/factory/port-lock'; signature='FAIL port isolation:';;
 rfb-default-port) command='mkdir -p build/factory; cc -std=c11 -O1 factory/tests/rfb-port.c -o build/factory/rfb-port && build/factory/rfb-port'; signature='FAIL RFB port:';;
 omit-boot-component) command='sh factory/tests/boot-registration.sh'; signature='FAIL boot registration:';;
 omit-smoke-component) command='make -s smoke-selection'; signature='FAIL smoke: incomplete components';;
 *) echo "FAIL unregistered gate mutant $name"; exit 1;;
 esac
 good=1
 for pass in 1 2; do
  rc=0; (cd "$scratch" && sh -c "$command") > "$work/$name.$pass.log" 2>&1 || rc=$?
  if [ "$rc" -eq 0 ] || ! grep -F "$signature" "$work/$name.$pass.log" >/dev/null; then good=0; fi
 done
 if [ "$good" -eq 1 ]; then
  echo "  killed $name (two semantic failures)"; killed=$((killed+1))
 else echo "FAIL $name survived or failed for another reason"; survived=$((survived+1)); fi
done
echo "factory: $total mutants, $killed killed, $survived survived, $errors errors; gate evidence: $work"
test "$survived" -eq 0 && test "$errors" -eq 0
