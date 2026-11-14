#!/bin/sh
# Genuine disk client checks the OEM gate and restarts through its public call.
set -eu
root=$(cd "$1" && pwd)
mkdir -p "$2"
parent=$(cd "$2" && pwd)
out=$(mktemp -d "$parent/run.XXXXXX")
echo "OEM smoke evidence: $out"
cd "$root"
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800 INITECH_QMP_STDIO=1 INITECH_TEST_RAM_MB=8
nasm -f bin kernel/tests/programs/oem_program.asm -o "$out/OEM.COM"
awk 'BEGIN {for(i=0;i<2000;i++)printf "%c%c",90,46;printf "%c%c",3,11}' > "$out/expected.bin"
for emu in qemu bochs; do
 dir=$out/$emu
 mkdir -p "$dir"
 truncate -s 0 "$dir/data.img"
 truncate -s 1474560 "$dir/data.img"
 mformat -i "$dir/data.img" -f 1440 ::
 mcopy -i "$dir/data.img" "$out/OEM.COM" ::OEM.COM
 printf '@ECHO OFF\r\nOEM\r\n' > "$dir/AUTOEXEC.BAT"
 mcopy -i "$dir/data.img" "$dir/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 "build/factory/${emu}_harness" --disk build/image/boot.img --disk2 "$dir/data.img" \
  --out "$dir" --name result --allow-reboot --expect OEM-END \
  --quit-after OEM-END --timeout-ms 60000 > "$dir/driver.log" 2>&1
 tr -d '\r' < "$dir/result.serial" > "$dir/transcript"
 grep -Fx OEM-CHECKS-OK "$dir/transcript"
 grep -Fx OEM-RESTART-OK "$dir/transcript"
 if grep -E 'OEM-FAIL|PANIC|RESTART-FAILED' "$dir/transcript"; then exit 1; fi
 test "$(grep -c '^SHELL-READY' "$dir/transcript")" -ge 2
 grep -F triple_fault=0 "$dir/driver.log"
 mcopy -o -i "$dir/data.img" ::PAGE.BIN "$dir/page.bin"
 cmp "$out/expected.bin" "$dir/page.bin"
 echo "ok $emu OEM: summary, exact page payload, restore/update, actual restart"
done
