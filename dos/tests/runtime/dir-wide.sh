#!/bin/sh
set -eu
root=${1:?root} out=${2:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/assertion" "$out/result" "$out/transport.ok"
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
ref=$root/dos/command/tests/dos33/reference/wide
gzip -dc "$ref/before-data.img.gz" > "$out/data.img"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::SUB/DONE.COM
sed 's/^B:/A:/' "$ref/AUTOEXEC.BAT" > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf 'sub\\done\n' | build/factory/keys > "$out/keys"
if ! timeout 40 build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/data.img" --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 15000 --rtc-base 1987-07-06T13:30:58 --out "$out" --name wide > "$out/driver.log" 2> "$out/report";then error 'wide DIR transport did not complete';fi
transport_pass "$out/report"
mtype -i "$out/data.img" ::TRACE.TXT > "$out/trace"
# The added completion probe and AUTOEXEC occupy clusters, and the product's
# data drive is A:. All meaningful layout, names, counts and CRLF remain exact.
sed -e 's/B:/A:/g' -e 's/drive B/drive A/g' -e 's/[0-9][0-9]* bytes free/<free> bytes free/g' "$ref/TRACE.TXT" > "$out/expected"
sed 's/[0-9][0-9]* bytes free/<free> bytes free/g' "$out/trace" > "$out/actual"
require 'DIR/W captured transcript bytes' cmp "$out/expected" "$out/actual"
echo 'GREEN DIR/W: complete captured single and multirow transcripts'
