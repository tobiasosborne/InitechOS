#!/bin/sh
set -eu
program=${FILTER_PROGRAM:?FILTER_PROGRAM}
root=${1:?root} boot=${2:?boot} programs=${3:?program directory} out=${4:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
mcopy -i "$out/data.img" "$programs/SORT.COM" "$programs/FIND.COM" ::
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/DONE.COM" ::DONE.COM
dd if=/dev/zero bs=70000 count=1 status=none | tr '\000' 'a' > "$out/large"
printf 'needle\r\n' >> "$out/large"
# Local resource boundaries, independent of the SORT implementation.
# MS-DOS 3.3 User's Guide p. 357 and compat33/UTILITY-MEDIA.md document
# memory refusal. The product's scratch layout sets its numeric limits.
awk 'BEGIN {for(i=0;i<7680;i++)printf "\r\n"}' > "$out/fit-lines"
cp "$out/fit-lines" "$out/lines"
printf '\r\n' >> "$out/lines"
awk 'BEGIN {for(i=0;i<20000;i++)printf "\r\n"}' > "$out/many-lines"
dd if=/dev/zero bs=65536 count=1 status=none | tr '\000' 'a' > "$out/exact"
if [ "$program" = FIND ]; then
 dd if=/dev/zero bs=65530 count=1 status=none | tr '\000' 'a' > "$out/exact"
 printf 'needle' >> "$out/exact"
fi
cat "$out/exact" > "$out/exact-expected"
printf '\r\n' >> "$out/exact-expected"
for spec in 'fit-lines FIT.TXT' 'many-lines MANY.TXT' 'exact EXACT.TXT'; do
 mcopy -i "$out/data.img" "$out/${spec%% *}" "::${spec##* }"
done
mcopy -i "$out/data.img" "$out/lines" ::LINES.TXT
mcopy -i "$out/data.img" "$out/large" ::LARGE.TXT
case $program in
 SORT)
  output=NUL
  if [ "${FILTER_BOUNDARIES:-0}" = 1 ]; then output=SORT.OUT; fi
  command="SORT <LINES.TXT >$output"
  refusal='SORT refuses 7681 lines with nonzero status'
  ;;
 FIND)
  output=FIND.OUT
  command='FIND "needle" <LARGE.TXT >FIND.OUT'
  refusal='FIND refusal exit'
  ;;
 *) error 'unknown filter';;
esac
{
 printf '@echo off\r\n'
 if [ "$output" = NUL ]; then
  # A missing guard can emit thousands of writes. NUL lets SORT finish
  # before the harness deadline, even when it accepts the excess line.
  printf '%s\r\n' 'SORT <FIT.TXT >NUL' 'if not errorlevel 1 echo ACCEPTED >FIT.RC'
 fi
 printf '%s\r\n' "$command" 'if errorlevel 1 goto refused' 'echo ACCEPTED >STATUS.TXT' \
  'goto checked' ':refused' 'echo REFUSED >STATUS.TXT' ':checked' DONE
} > "$out/AUTOEXEC.BAT"
if [ "$program" = SORT ] && [ "${FILTER_BOUNDARIES:-0}" = 1 ]; then
 sed '$d' "$out/AUTOEXEC.BAT" > "$out/auto-more"
 printf 'SORT <FIT.TXT >FIT.OUT\r\nSORT <EXACT.TXT >EXACT.OUT\r\nSORT <MANY.TXT >MANY.OUT\r\nSORT <LARGE.TXT >LARGE.OUT\r\nDONE\r\n' >> "$out/auto-more"
 mv "$out/auto-more" "$out/AUTOEXEC.BAT"
fi
if [ "$program" = FIND ]; then
 sed '$d' "$out/AUTOEXEC.BAT" > "$out/auto-more"
 printf 'FIND "needle" <EXACT.TXT >EXACT.OUT\r\nFIND /C "needle" <LARGE.TXT >COUNT.OUT\r\nDONE\r\n' >> "$out/auto-more"
 mv "$out/auto-more" "$out/AUTOEXEC.BAT"
fi
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
if ! timeout 60 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 45000 --out "$out" --name limits > "$out/driver.log" 2> "$out/report"; then error 'filter limits transport did not complete'; fi
transport_pass "$out/report"
if [ "$output" = NUL ]; then
 mtype -i "$out/data.img" ::FIT.RC > "$out/status" || error 'fit status missing'
 tr -d '\r' < "$out/status" > "$out/status-text"
 require 'SORT accepts 7680 lines' grep -Fx 'ACCEPTED' "$out/status-text"
else
 mtype -i "$out/data.img" "::$output" > "$out/output" || error 'output missing'
 tr -d '\r' < "$out/output" > "$out/text"
 require "$program bounded buffer refusal" grep -Fx 'Insufficient memory' "$out/text"
fi
mtype -i "$out/data.img" ::STATUS.TXT > "$out/status" || error 'status missing'
tr -d '\r' < "$out/status" > "$out/status-text"
require "$refusal" grep -Fx 'REFUSED' "$out/status-text"
echo "GREEN $program: input limit refusal returns nonzero status"
if [ "$program" = SORT ] && [ "${FILTER_BOUNDARIES:-0}" = 1 ]; then
 for spec in 'FIT.OUT fit-lines' 'EXACT.OUT exact-expected'; do
  mtype -i "$out/data.img" "::${spec%% *}" > "$out/actual" || error 'boundary output missing'
  require "SORT accepts ${spec%% *} exactly at local capacity" cmp "$out/actual" "$out/${spec##* }"
 done
 for name in MANY LARGE; do
  mtype -i "$out/data.img" "::$name.OUT" > "$out/actual" || error 'excess output missing'
  tr -d '\r' < "$out/actual" > "$out/text"
  require "SORT refuses $name before partial output" grep -Fx 'Insufficient memory' "$out/text"
 done
fi
if [ "$program" = FIND ]; then
 mtype -i "$out/data.img" ::EXACT.OUT > "$out/actual" || error 'exact output missing'
 require 'FIND accepts exact input capacity without dropping its last match' cmp "$out/actual" "$out/exact-expected"
 mtype -i "$out/data.img" ::COUNT.OUT > "$out/actual" || error 'count output missing'
 tr -d '\r' < "$out/actual" > "$out/text"
 require 'FIND refuses an incomplete count' grep -Fx 'Insufficient memory' "$out/text"
fi
