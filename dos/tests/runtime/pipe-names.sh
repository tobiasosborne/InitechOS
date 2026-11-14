#!/bin/sh
set -eu
root=${1:?root} out=${2:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/assertion" "$out/result" "$out/transport.ok"
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
ref=$root/dos/command/tests/dos33/reference/pipe
gzip -dc "$ref/before-data.img.gz" > "$out/data.img"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::SUB/DONE.COM
nasm -f bin dos/command/tests/compat33/stream.asm -o "$out/PASS.COM"
mcopy -o -i "$out/data.img" "$out/PASS.COM" ::SUB/PASS.COM
sed 's/^B:/A:/' "$ref/AUTOEXEC.BAT" > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf 'sub\\done\n' | build/factory/keys > "$out/keys"
if ! timeout 40 build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/data.img" --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 15000 --rtc-base 1987-07-06T13:30:58 --out "$out" --name pipe > "$out/driver.log" 2> "$out/report";then error 'pipe-name transport did not complete';fi
transport_pass "$out/report"
for name in ONE TWO THREE FOUR;do
 mtype -i "$out/data.img" "::$name.OUT" > "$out/$name.OUT"
 grep -E '^[0-9A-F]{8} ' "$ref/$name.OUT" | awk '{print $2}' > "$out/expected-sizes"
 if grep -E '^[0-9A-F]{8} ' "$out/$name.OUT" > "$out/temps";then :;else rc=$?;[ "$rc" = 1 ] || error 'grep failed';: > "$out/temps";fi
 awk '{print $2}' "$out/temps" > "$out/actual-sizes"
 require "$name: two hex temporary names and captured buffer rotation" cmp "$out/expected-sizes" "$out/actual-sizes"
 case $name in ONE|TWO) prefix=0C22;; *) prefix=173A;;esac
 forbid "$name: incorrect clock-derived temporary prefix" grep -Ev "^$prefix[0-9A-F]{4} " "$out/temps"
done
mtype -i "$out/data.img" ::FIVE.OUT > "$out/FIVE.OUT"
require 'four-stage pipe truncates reused buffer' cmp "$ref/FIVE.OUT" "$out/FIVE.OUT"
gzip -dc "$ref/data.img.gz" > "$out/reference.img"
mdir -a -b -i "$out/reference.img" :: | sort > "$out/expected-names"
mdir -a -b -i "$out/data.img" :: | sort > "$out/actual-names"
require 'all owned temporaries removed; foreign files retained' cmp "$out/expected-names" "$out/actual-names"
mtype -i "$out/reference.img" '::PIPE1.$$$' > "$out/foreign-expected"
mtype -i "$out/data.img" '::PIPE1.$$$' > "$out/foreign-actual"
require 'foreign pipe name bytes preserved' cmp "$out/foreign-expected" "$out/foreign-actual"
echo 'GREEN pipe names: captured clock fields, two rotating buffers and cleanup'
