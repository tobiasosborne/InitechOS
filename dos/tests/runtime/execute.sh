#!/bin/sh
# No scenario may classify a failed emulator invocation as a semantic result.
set -eu
root=$1 boot=$2 out=$3 keys=$4 marker=$5
out=$(CDPATH= cd -- "$out" && pwd)
short=$(mktemp -d /tmp/dos-run.XXXXXX); rmdir "$short"; ln -s "$out" "$short"
trap 'rm -f "$short"' 0 HUP INT TERM
cp "$boot" "$out/boot.img"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
if ! timeout -k 5 60 "$root/build/factory/qemu_harness" --disk "$short/boot.img" --boot-rw --disk2 "$short/data.img" \
 --keys "$keys" --keys-after SHELL-READY --expect "$marker" --quit-after "$marker" \
 --timeout-ms 20000 --name run --out "$short" > "$out/stdout" 2> "$out/report"; then exit 2;fi
tr -d '\r' < "$out/run.serial" > "$out/text"
grep -Fx '[harness] OK=1' "$out/report" >/dev/null || exit 2
grep -F 'timed_out=0' "$out/report" >/dev/null || exit 2
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report" >/dev/null || exit 2
printf '%s\n' "$out/report" > "$out/transport.ok"
