#!/bin/sh
set -eu
self=$(realpath "$0")
root=${1:?root} capture_case=${2:?case} out=${3:?output}
if [ "$capture_case" = ALL ];then
 status=0
 for name in SWITCH REDIR NAMES BATCH FULL EXTRA;do
  rc=0
  sh "$self" "$root" "$name" "$out/$name" > "$out-$name.log" 2>&1 || rc=$?
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
case $capture_case in
 EXTRA) ref=$root/dos/command/tests/dos33/reference/diagnostics;;
 *) ref=$root/dos/command/tests/compat33/expected/$capture_case;;
esac
gzip -dc "$ref/data.img.gz" > "$out/data.img"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::SUB/DONE.COM
sed 's/^B:/A:/' "$ref/AUTOEXEC.BAT" > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf 'sub\\done\n' | build/factory/keys > "$out/keys"
if ! timeout 40 build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/data.img" --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 15000 --rtc-base 1987-07-06T13:30:58 --out "$out" --name diagnostics > "$out/driver.log" 2> "$out/report";then error 'diagnostics transport did not complete';fi
transport_pass "$out/report"
mdir -a -b -i "$out/data.img" :: > "$out/names" || error 'cannot read result volume'
extract() {
 require "created $1" grep -Fx "::/$1" "$out/names" >/dev/null
 mtype -i "$out/data.img" "::$1" > "$2" || error "cannot read $1"
}
tr -d '\r' < "$out/diagnostics.serial" > "$out/text"
pattern='^(Invalid parameter(\(s\))?|Too many parameters|Invalid directory|Invalid number of parameters|Required parameter missing|Label not found|File not found|Bad command or file name|Invalid drive specification|Insufficient disk space|Intermediate file error during pipe|Syntax error)[[:space:]]*$'
grep -E "$pattern" "$ref/result.txt" | sed 's/ *$//' > "$out/expected-console"
if grep -E "$pattern" "$out/text" > "$out/actual-console";then :;else rc=$?;[ "$rc" = 1 ] || error 'diagnostic extraction failed';fi
require "$capture_case captured diagnostics on CON" cmp "$out/expected-console" "$out/actual-console"
extract TRACE.TXT "$out/trace"
if grep -E "$pattern" "$out/trace";then semantic "$capture_case diagnostics leaked to stdout";else rc=$?;[ "$rc" = 1 ] || error 'stdout scan failed';fi
case $capture_case in
 SWITCH|REDIR|FULL|EXTRA)
  trace=$ref/files/TRACE.TXT
  [ "$capture_case" != EXTRA ] || trace=$ref/TRACE.TXT
  sed -e 's/B:/A:/g' -e 's/drive B/drive A/g' -e 's/[0-9][0-9]* bytes free/<free> bytes free/g' "$trace" > "$out/expected"
  sed 's/[0-9][0-9]* bytes free/<free> bytes free/g' "$out/trace" > "$out/actual"
  require "$capture_case captured stdout bytes" cmp "$out/expected" "$out/actual";;
esac
if [ "$capture_case" = SWITCH ];then
 gzip -dc "$ref/data.img.gz" > "$out/reference.img"
 for file in A.TXT B.TXT CP.TXT DEL.TXT;do
  mtype -i "$out/reference.img" "::$file" > "$out/expected-file"
  extract "$file" "$out/actual-file"
  require "$file captured bytes preserved" cmp "$out/expected-file" "$out/actual-file"
 done
 mdir -a -b -i "$out/data.img" :: > "$out/names"
 forbid 'refused commands created BAD.TXT or R.TXT' grep -E '/(BAD|R)\.TXT$' "$out/names"
fi
if [ "$capture_case" = EXTRA ];then
 for file in MISS.OUT NOARG.OUT;do
  extract "$file" "$out/actual-file"
  require "$file captured empty bytes" cmp "$ref/$file" "$out/actual-file"
 done
 mdir -a -b -i "$out/data.img" :: > "$out/names"
 forbid 'unknown command created UNKNOWN.OUT' grep -F /UNKNOWN.OUT "$out/names"
fi
echo "GREEN $capture_case: captured diagnostic stream, wording and stdout"
