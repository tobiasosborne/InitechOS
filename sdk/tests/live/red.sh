#!/bin/sh
# Each changed product is isolated under build/. No working source is patched.
set -eu
cd "$(dirname "$0")/../../.."
out=build/sdk/red-live
mkdir -p "$out/runtime/sdk/rt" "$out/fake/sdk/host"
cp sdk/rt/tbx_rt.c "$out/runtime/sdk/rt/"
patch -s -d "$out/runtime/sdk" -p1 < sdk/mutants/live/text-arguments.patch
root=$PWD
cat > "$out/runtime/override.mk" <<EOF_MAKE
\$(OUT)/tenant/tbx_rt.o: $root/$out/runtime/sdk/rt/tbx_rt.c FORCE
	\$(call run-command,\$(TARGET_CC) \$(TARGET_CFLAGS) -I\$(TOP)/sdk/include -MMD -MP -c $root/$out/runtime/sdk/rt/tbx_rt.c -o \$@)
EOF_MAKE
nice -n 10 make -j2 -C sdk -f tests/live/Makefile -f "$root/$out/runtime/override.mk" \
 TOP="$root" OUT="$root/$out/runtime/build" ABI=1 > "$out/runtime/build.log" 2>&1
# The old structure/count audit must still pass the swapped-argument runtime.
build/sdk/tests/audit sdk/tests/traps.tsv sdk/include/toolbox_gate.h sdk/include/cp_dialog.h \
 "$out/runtime/sdk/rt/tbx_rt.c" flair/tbxgate.c > "$out/runtime/audit.log"
if SDK_ABI_TENANT="$out/runtime/build" SDK_ABI_OUT="$out/runtime/smoke" \
 sh sdk/tests/live/abi-smoke.sh > "$out/runtime/red.log" 2>&1;then
 echo 'FAIL: live argument mutant survived';exit 1
fi
grep -F 'ABI argument identity FAIL at row 5' "$out/runtime/red.log"
echo 'RED confirmed: swapped TEXTDRAW x/y, structural audit remains green'
cp sdk/host/tbx_host.c sdk/host/tbx_validate.h sdk/host/tbx_host.h "$out/fake/sdk/host/"
patch -s -d "$out/fake/sdk" -p1 < sdk/mutants/fake-window-goaway.patch
cc -std=c11 -O1 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Isdk/include \
 -I"$out/fake/sdk/host" sdk/tests/live/host.c "$out/fake/sdk/host/tbx_host.c" -o "$out/fake/check"
if "$out/fake/check" sdk/tests/live/gate-results.bin > "$out/fake/red.log" 2>&1;then
 echo 'FAIL: fake goAway mutant survived';exit 1
fi
grep -F 'gate case 16:' "$out/fake/red.log"
echo 'RED confirmed: fake accepts invalid goAway'
