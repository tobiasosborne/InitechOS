#!/bin/sh
# The same compiler source and ordinary product image as the QEMU certificate.
# Each guest compilation has a fresh TEST disk, 8 MiB and a measured deadline.
set -eu
cd "$(dirname "$0")/../../.."
out=$PWD/build/turbo/selfhost/bochs
mkdir -p "$out"
sha256sum turbo/compiler/turbo.pas > "$out/source.sha256"
cmp build/turbo/selfhost/source.sha256 "$out/source.sha256"
sh turbo/tests/smoke/compile.sh build/turbo/compiler/TURBO.COM turbo/compiler/turbo.pas "$out/K2" bochs
cp "$out/K2/program.COM" "$out/K2.COM"
sh turbo/tests/smoke/compile.sh "$out/K2.COM" turbo/compiler/turbo.pas "$out/K3" bochs
cp "$out/K3/program.COM" "$out/K3.COM"
build/turbo/smoke/compare "$out/K2.COM" "$out/K3.COM"
build/turbo/smoke/compare build/turbo/selfhost/K2.COM "$out/K2.COM"
sha256sum "$out/K2.COM" "$out/K3.COM" | tee "$out/fixedpoint.sha256"

# Refuse a corrupted K2 in a real Bochs boot, rather than only comparing two
# host files. Keep the MZ/tag intact to avoid the raw-COM fallback, but set
# descriptor version 2: the public extended-container contract accepts only 1
# (kernel/include/initechmz_extended.h). The independent reader must reject it.
cp "$out/K2.COM" "$out/corrupt-K2.COM"
printf '\002\000\000\000' | dd of="$out/corrupt-K2.COM" bs=1 seek=32 conv=notrunc 2>/dev/null
if build/factory/mz/mzcheck "$out/corrupt-K2.COM" > "$out/corrupt-mzcheck.log" 2>&1; then
 exit 1
fi
grep -Fx 'mzcheck: extended version' "$out/corrupt-mzcheck.log"
if sh turbo/tests/smoke/compile.sh "$out/corrupt-K2.COM" turbo/compiler/turbo.pas "$out/corrupt" bochs > "$out/corrupt-run.log" 2>&1; then
 echo 'FAIL: Bochs accepted the corrupted compiler' >&2
 exit 1
else
 test "$?" -eq 1
fi
# This red must be clean loader refusal, not an unrelated timeout/fault.
grep -Fx '[bochs] OK=1' "$out/corrupt/report"
grep -E '^\[bochs\] launched=1 rfb_unblocked=1 timed_out=0 ' "$out/corrupt/report"
grep -Fx '[bochs] triple_fault=0' "$out/corrupt/report"
grep -Fx 'compile-complete' "$out/corrupt/serial.txt"
grep -Fx 'Bad command or file name' "$out/corrupt/serial.txt"
test "$(grep -c '^TICK=' "$out/corrupt/serial.txt")" -eq 2
if grep -E 'PANIC|^TURBO-NATIVE-OK ' "$out/corrupt/serial.txt"; then exit 1; fi
# Read back the actual candidate attempted by the guest, independently of the
# host copy, and require that no executable was emitted on its fresh disk.
MTOOLS_SKIP_CHECK=1 mcopy -o -i "$out/corrupt/test.img" ::TURBO.COM "$out/corrupt/attempted.COM"
cmp "$out/corrupt-K2.COM" "$out/corrupt/attempted.COM"
if MTOOLS_SKIP_CHECK=1 mdir -i "$out/corrupt/test.img" ::TURBOOUT.EXE > "$out/corrupt/output-check.log" 2>&1; then exit 1; fi
if build/turbo/smoke/compare "$out/K3.COM" "$out/corrupt-K2.COM" > "$out/red-control.log" 2>&1; then
 exit 1
else
 test "$?" -eq 1
fi
cat "$out/red-control.log"
# The whole-run acceptance bound must reject an impossible one-millisecond
# budget too. Keep this separate from the clean corrupted-K2 refusal above.
if TURBO_BOCHS_TIMEOUT_MS=1 sh turbo/tests/smoke/compile.sh "$out/K2.COM" turbo/compiler/turbo.pas "$out/deadline" bochs > "$out/deadline.log" 2>&1; then
 echo 'FAIL: Bochs ignored the requested whole-run deadline' >&2
 exit 1
else
 test "$?" -eq 1
fi
grep -Fx 'FAIL: Bochs whole-run deadline exceeded' "$out/deadline.log"
echo 'SELFHOST Bochs: in-OS K2 == K3 == QEMU K2; corrupted-K2 guest control red'
