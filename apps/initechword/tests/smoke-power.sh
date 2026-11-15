#!/bin/sh
# Actual Finder -> native Word Exit, IM VI6-36/6-37, WP5.1 Save/Exit guard.
# Input, disk readers and debugger-free serial milestones are all external.
set -eu
repo=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
cd "$repo"
refroot=$1
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 TZ=UTC LC_ALL=C
out=${WORD_POWER_OUT:-$(mktemp -d "$PWD/build/word-power.XXXXXX")}
mkdir -p "$out"
fail(){ echo "FAIL Word power: $* ($out)" >&2;exit 1; }
readonly_source=goldens/dataloss/wordperfect/collision/before.NAME.WP5
printf '%s\n' 'b2b9df5cd68aee170538d21c36a405ea03dc7aa0d374fc9a26dba6d3002a2ab4  goldens/dataloss/wordperfect/collision/before.NAME.WP5' | sha256sum -c --quiet -
for emulator in ${WORD_POWER_EMULATORS:-qemu bochs};do
 for mode in ${WORD_POWER_MODES:-restart shutdown clean-restart clean-shutdown};do
  name=$emulator-$mode;d=$out/$name;mkdir -p "$d"
  boot=$d/boot.img;data=$d/data.img
  cp build/image/boot.img "$boot";cp build/image/data.img "$data"
  # Current staged product binds app storage to A. Set c only after coordinated
  # C activation; always use a cloned boot disk with --boot-rw for C readback.
  volume=$data;drive=A
  if [ "${WORD_POWER_VOLUME:-a}" = c ];then volume=$boot@@32256;drive=C;fi
  mcopy -o -i "$volume" "$readonly_source" ::READONLY.WP5
  mattrib -i "$volume" +r ::READONLY.WP5
  app_entries=$(mdir -b -i "$volume" ::APPS/)
  markers=$d/markers;: > "$markers"
  . factory/desktop-trace.sh
  pkeys(){ for key in $(printf '%s' "$1" | tr ',' ' ');do if [ "$emulator" = bochs ]&&[ "$key" = kesc ];then key=kf1;fi;inject "$key,w150";done; }
  point(){ inject 'm-100:-100,m-100:-100,m-100:-100,m-100:-100,m-100:-100,m-100:-100,m-100:-100';move_by "$1" "$2"; }
  request(){
   point 10 300;inject 'l1,l0';await_marker 'FLAIR-DISPATCH app=FINDER'
   point 170 10;inject l1
   if [ "$1" = restart ];then move_by 0 61;else move_by 0 77;fi
   inject l0;await_marker "FINDER-POWER name=$(printf '%s' "$1" | tr 'a-z' 'A-Z')"
  }
  saved(){ await_marker "s=\"$drive:\\$1"; }
  trace=$(
   if [ "$emulator" = qemu ];then await_marker 'FLAIR-CLOCK 9:00 am';fi
   inject "$open_disk";await_marker FINDER-OPEN-VOLUME
   inject "$open_apps";await_marker 'FINDER-OPEN-FOLDER name=APPS'
   inject 'kup,kw,ko,kr,kd,kdot,ke,kx,ke'
   await_marker 'FINDER-WIN-SELECT win=1 name=WORD.EXE count=1'
   inject kctrl-o;await_marker 'FLAIR-DISPATCH app=WORD'
   point 300 30;inject l1;move_by 80 0;inject l0;await_marker "FLAIR-DRAG win"
   case $mode in
    clean-*) request "${mode#clean-}";;
    *)
     pkeys 'kg,ku,ka,kr,kd,kspc,kt,ke,kx,kt,kdot'
     request "$mode"
     await_marker 's="Save Document? Yes (No)';await_marker FINDER-POWER-WAIT
     capture "$name-save-question"
     pkeys kesc;await_marker FINDER-POWER-NOTICE;pkeys kret
     # Persist cancelled work by the ordinary save route for independent readback.
     pkeys 'kf10,kc,ka,kn,kc,ke,kl,kdot,kw,kp,k5,kret';saved CANCEL.WP5
     if [ "$mode" = restart ];then
      pkeys kx;request restart
      await_marker 's="Save Document? Yes (No)';await_marker FINDER-POWER-WAIT
      pkeys 'ky,kr,ke,ka,kd,ko,kn,kl,ky,kdot,kw,kp,k5,kret'
      await_marker "s=\"Replace $drive:\\READONLY.WP5?"
      pkeys ky;await_marker FINDER-POWER-NOTICE;capture "$name-save-refused"
      pkeys 'kret,kspc,kesc,kf10,ka,kf,kt,ke,kr,kdot,kw,kp,k5,kret';saved AFTER.WP5
      pkeys ky;request restart
      await_marker 's="Save Document? Yes (No)';await_marker FINDER-POWER-WAIT
      pkeys 'ky,kp,ko,kw,ke,kr,kdot,kw,kp,k5,kret';await_marker 's="Exit WP? No (Yes)'
      pkeys ky
     else
      pkeys kx;request shutdown
      await_marker 's="Save Document? Yes (No)';await_marker FINDER-POWER-WAIT
      pkeys kn;await_marker 's="Exit WP? No (Yes)';pkeys ky
     fi;;
   esac
   await_marker 'TENANT-EXIT rc=0 via=exit'
   case $mode in *shutdown)
    await_marker FLAIR-SHUTDOWN-SAFE;capture "$name-safe";inject kret;;
   esac
   await_marker FLAIR-RESTART
  )
  trace=${trace%,};printf '%s\n' "$trace" > "$d/trace"
  driver=build/factory/qemu_harness;input=--mouse
  set -- --boot-rw
  if [ "$emulator" = bochs ];then driver=build/factory/desktop-bochs;input=--keys;set -- --rtc-time0 699440400 --renew-ms 180000;fi
  export DESKTOP_SERIAL=$d/$name.serial
  "$driver" --disk "$boot" --disk2 "$data" "$@" --allow-reboot \
   --keys-after FLAIR-LIVE-READY "$input" "$trace" --timeout-ms 180000 \
   --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --quit-after-count 2 \
   --name "$name" --out "$d" > "$d/stdout" 2> "$d/report" || { cat "$d/report";fail "$name journey"; }
  if [ "$emulator" = bochs ]&&[ "${WORD_POWER_VOLUME:-a}" = c ];then boot=$(sed -n 's/^\[bochs\] private_boot=//p' "$d/report");volume=$boot@@32256;fi
  grep -F 'triple_fault=0' "$d/report" >/dev/null || fail "$name CPU fault"
  [ "$(grep -c 'FLAIR-LIVE-READY' "$d/$name.serial")" -eq 2 ] || fail "$name did not actually restart"
  [ "$(grep -c 'TENANT-EXIT rc=0 via=exit' "$d/$name.serial")" -eq 1 ] || fail "$name quit count"
  mcopy -o -i "$volume" ::READONLY.WP5 "$d/readonly-after.wp5"
  cmp "$readonly_source" "$d/readonly-after.wp5" || fail "$name changed refused destination"
  case $mode in
   clean-*) ! grep -F 'Save Document?' "$d/$name.serial" >/dev/null || fail "$name asked to save clean work";;
   *)
    mcopy -o -i "$volume" ::CANCEL.WP5 "$d/cancel.wp5"
    python3 -I "$refroot/tools/wp5_ref.py" "$d/cancel.wp5" > "$d/cancel.ref"
    grep -qxF "  TEXT: 'guard text.'" "$d/cancel.ref" || fail "$name cancelled work changed"
    if [ "$mode" = restart ];then
     for spec in AFTER:guard\ text.x POWER:guard\ text.xy;do
      file=${spec%%:*};text=${spec#*:}
      mcopy -o -i "$volume" "::$file.WP5" "$d/$file.wp5"
      python3 -I "$refroot/tools/wp5_ref.py" "$d/$file.wp5" > "$d/$file.ref"
      grep -qxF "  TEXT: '$text'" "$d/$file.ref" || fail "$name $file text changed"
     done
    fi;;
  esac
  echo "ok Word $name: explicit app answer; preserved work; actual restart ($d)"
 done
done
