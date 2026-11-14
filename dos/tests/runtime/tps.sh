#!/bin/sh
# Imported real-DOS report bytes and independent file-preservation invariants.
set -eu
root=$1 boot=$2 programs=$3 out=$4
. "$root/dos/tests/runtime/verdict.sh"
ref=$root/dos/tpsrep/tests/reference
(cd "$ref" && sha256sum -c SHA256SUMS >/dev/null)
fail() { semantic "$*"; }
for scenario in ${TPS_SCENARIOS:-ritual lines pages backup full}; do
 d=$out/$scenario;mkdir -p "$d"
 truncate -s 1474560 "$d/data.img";mformat -i "$d/data.img" -f 1440 ::
 mcopy -i "$d/data.img" "$programs/TPSREP.COM" ::TPSREP.COM
 cp "$ref/tpsparm.dat" "$d/parm";cp "$ref/tps.dbf" "$d/dbf"
 case $scenario in
 lines) sed 's/^LINES=.*/LINES=banana\r/' "$ref/tpsparm.dat" > "$d/parm";;
 pages) printf XYZ | dd of="$d/dbf" bs=1 seek=264 conv=notrunc status=none;;
 esac
 mcopy -i "$d/data.img" "$d/parm" ::TPSPARM.DAT
 mcopy -i "$d/data.img" "$d/dbf" ::TPS.DBF
 mcopy -i "$d/data.img" "$ref/old_tpsrept.txt" ::TPSREPT.TXT
 mcopy -i "$d/data.img" "$ref/old_tpsrept.bak" ::TPSREPT.BAK
 printf '@ECHO OFF\r\nECHO TPS-READY\r\nTPSREP TPS.DBF\r\nECHO tps-done\r\n' > "$d/autoexec"
 mcopy -i "$d/data.img" "$d/autoexec" ::AUTOEXEC.BAT
 [ "$scenario" != backup ] || mattrib -i "$d/data.img" +r ::TPSREPT.BAK
 if [ "$scenario" = full ]; then
  free=$(mdir -i "$d/data.img" :: | awk '/bytes free/ {gsub(/ /, "", $0);sub(/bytesfree.*/, "", $0); print $0}')
  truncate -s "$free" "$d/filler";mcopy -i "$d/data.img" "$d/filler" ::FILL.BIN
 fi
 keys=
 case $scenario in ritual) input=$ref/keys_a.txt;; *) input=$ref/keys_ok.txt;; esac
 if [ "${TPS_COUNT_CONTROL:-0}" = 1 ]; then
  printf '      \r       \r004417\r004417\r004417\r004417\r88-114\ry' > "$d/count-keys"
  input=$d/count-keys
 fi
 # SPACE, Enter, digits, hyphen and y/n: no host key encoding shortcuts.
 od -An -tu1 -v "$input" | awk '{for(i=1;i<=NF;i++){c=$i;if(c==32)k="spc";else if(c==13 || c==10)k="ret";else if(c==9)k="tab";else if(c>=48 && c<=57)k=sprintf("%c",c);else if(c==45)k="minus";else k=sprintf("%c",c);printf "%s%s",sep,k;sep=","}} END{print ""}' > "$d/keys"
 # AUTOEXEC starts the program before SHELL-READY, so use the program's marker.
 # The transport's input trigger is overridden by a local copy of its invocation.
 short=$(mktemp -d /tmp/tps-run.XXXXXX);rmdir "$short";ln -s "$d" "$short"
 cp "$boot" "$d/boot.img"
 export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
 if ! timeout -k 5 60 "$root/build/factory/qemu_harness" --disk "$short/boot.img" --boot-rw --disk2 "$short/data.img" \
  --keys "$(cat "$d/keys")" --keys-after TPS-READY --expect tps-done --quit-after tps-done \
  --timeout-ms 25000 --name run --out "$short" > "$d/stdout" 2> "$d/report"; then rm -f "$short";error "$scenario did not finish";fi
 rm -f "$short"
 transport_pass "$d/report"
 tr -d '\r' < "$d/run.serial" > "$d/text"
 case $scenario in
 ritual)
  if [ "${TPS_COUNT_CONTROL:-0}" = 1 ]; then require 'six alignment presses refused' grep -F 'FORM NOT ALIGNED (6 PRESSES)' "$d/text" >/dev/null;fi
  mtype -i "$d/data.img" ::TPSREPT.TXT > "$d/report.txt"
  require 'report differs from real DOS' cmp "$d/report.txt" "$ref/tpsrept_expected.txt"
  if grep '^ADVANCE THE FORM' "$d/text" | grep -Ei '[0-9]|space' >/dev/null; then fail 'alignment prompt states tacit count/key';fi;;
 lines|pages|backup)
  case $scenario in lines) code=010;; pages) code=011;; backup) code=012;; esac
  require "$scenario refused without required diagnostic" grep -F "TPS-$code " "$d/text" >/dev/null
  mtype -i "$d/data.img" ::TPSREPT.TXT > "$d/retained"
  require "$scenario lost previous report" cmp "$d/retained" "$ref/old_tpsrept.txt";;
 full)
  require 'full disk reported success' grep -F 'TPS-009 ' "$d/text" >/dev/null
  if mdir -i "$d/data.img" ::TPSREPT.TXT >/dev/null 2>&1; then fail 'failed report retained';fi
  mtype -i "$d/data.img" ::TPSREPT.BAK > "$d/retained"
  require 'full disk lost backup' cmp "$d/retained" "$ref/old_tpsrept.txt";;
 esac
done
echo 'GREEN TPS sessions: real-DOS report, tacit alignment, invalid inputs, backup collision and full disk'
