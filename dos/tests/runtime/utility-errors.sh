#!/bin/sh
set -eu
self=$(realpath "$0")
root=${1:?root} capture_case=${2:?case} out=${3:?out}
if [ "$capture_case" = ALL ];then
 status=0
 for name in U04 U08 U09 U10 U13;do
  rc=0;sh "$self" "$root" "$name" "$out/$name" > "$out-$name.log" 2>&1 || rc=$?
  printf '%s exit=%s\n' "$name" "$rc"
  [ "$rc" -eq 0 ] || status=1
 done
 exit "$status"
fi
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/assertion" "$out/result" "$out/transport.ok"
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
ref=$root/dos/command/tests/compat33/expected/$capture_case
case $capture_case in U04) program=CHKDSK;; U08) program=SYS;; U09) program=DISKCOPY;; U10) program=DISKCOMP;; U13) program=ATTRIB;ref=$root/dos/command/tests/dos33/reference/attrib-error;;esac
gzip -dc "$ref/data.img.gz" > "$out/data.img"
mcopy -o -i "$out/data.img" "build/dos/$program.COM" "::$program.COM"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::SUB/DONE.COM
sed 's/^B:/A:/' "$ref/AUTOEXEC.BAT" > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf 'sub\\done\n' | build/factory/keys > "$out/keys"
if ! timeout 40 build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/data.img" --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 15000 --rtc-base 1987-07-06T13:30:58 --out "$out" --name utility > "$out/driver.log" 2> "$out/report";then error 'utility transport did not complete';fi
transport_pass "$out/report"
mdir -a -b -i "$out/data.img" :: > "$out/names" || error 'cannot read utility result volume'
require 'utility created output file' grep -Fx ::/UTIL.OUT "$out/names" >/dev/null
mtype -i "$out/data.img" ::UTIL.OUT > "$out/actual" || error 'cannot read utility output'
case $capture_case in
 U04)
  head -3 "$ref/files/UTIL.OUT" > "$out/expected-prefix"
  head -3 "$out/actual" > "$out/actual-prefix"
  require 'CHKDSK captured invalid-switch prefix' cmp "$out/expected-prefix" "$out/actual-prefix"
  require 'CHKDSK continues disk report' grep -F 'bytes total disk space' "$out/actual" >/dev/null
  require 'CHKDSK continues memory report' grep -F 'bytes total memory' "$out/actual" >/dev/null
  forbid 'CHKDSK added zero-hidden row' grep -E '^ *0 bytes in 0 hidden files' "$out/actual";;
 U13)
  require 'ATTRIB empty redirected output' cmp "$ref/UTIL.OUT" "$out/actual"
  awk '/^ATTRIB-BEGIN/{p=1;next} /^ATTRIB-END/{p=0} p{sub(/ *$/, "");print}' "$ref/result.txt" > "$out/expected-console"
  tr -d '\r' < "$out/utility.serial" | awk '/^ATTRIB-BEGIN$/{p=1;next} /^ATTRIB-END$/{p=0} p{print}' > "$out/actual-console"
  require 'ATTRIB captured CON framing' cmp "$out/expected-console" "$out/actual-console";;
 *) require "$capture_case exact raw refusal bytes" cmp "$ref/files/UTIL.OUT" "$out/actual";;
esac
echo "GREEN $capture_case: genuine utility refusal and continuation"
