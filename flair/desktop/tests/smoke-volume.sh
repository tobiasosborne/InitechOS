#!/bin/sh
# Real product kernel; select C: with ordinary AUTOEXEC commands. The current
# delivery kernel still needs A: for startup. C-alone boot is its joint gate.
set -eu
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 TZ=UTC LC_ALL=C
out=${FINDER_VOLUME_SMOKE_OUT:-$(mktemp -d "$PWD/build/finder-volume.XXXXXX")}
mkdir -p "$out"
fail() { echo "FAIL Finder volume: $* ($out)" >&2; exit 1; }
. factory/desktop-trace.sh
printf '@ECHO OFF\r\nC:\r\nFLAIR\r\n' > "$out/autoexec"
for emulator in qemu bochs; do
 name=$emulator-c
 cp build/image/boot.img "$out/$name-boot.img"
 boot=$out/$name-boot.img
 volume=$boot@@32256
 mmd -i "$volume" ::APPS
 mcopy -i "$volume" build/apps/notepad/NOTEPAD.EXE ::APPS/NOTEPAD.EXE
 cp build/image/data.img "$out/$name-data.img"
 data=$out/$name-data.img
 mcopy -o -i "$data" "$out/autoexec" ::AUTOEXEC.BAT
 cp "$data" "$out/$name-data.before"
 markers=$out/$name.markers; : > "$markers"
 app_entries=$(mdir -b -i "$volume" ::APPS/)
 trace=$(
  inject "$open_disk"; await_marker FINDER-OPEN-VOLUME
  inject kctrl-n; await_marker 'FINDER-NEW-FOLDER name=NEWFOLD parent=0'
  inject "$open_apps"; await_marker 'FINDER-OPEN-FOLDER name=APPS'
  inject m22:20
  launch_app NOTEPAD.EXE
  inject 'ka,kl,kp,kh,ka,kctrl-w'
  await_marker 'TENANT-EXIT rc=0 via=exit'
  inject "$exit_desktop"; await_marker FINDER-EXIT-DOS-ALERT; inject kret
 )
 trace=${trace%,}
 driver=build/factory/qemu_harness; args=--mouse; set -- --boot-rw
 if [ "$emulator" = bochs ]; then
  driver=build/factory/desktop-bochs; args=--keys; set -- --rtc-time0 699440400
 fi
 export DESKTOP_SERIAL="$out/$name.serial"
 "$driver" --disk "$boot" --disk2 "$data" "$@" \
  --keys-after FLAIR-LIVE-READY "$args" "$trace" --timeout-ms 90000 \
  --expect FLAIR-EXIT-TO-DOS --quit-after FLAIR-EXIT-TO-DOS --name "$name" --out "$out" \
  > "$out/$name.stdout" 2> "$out/$name.report" || fail "$name journey"
 if [ "$emulator" = bochs ]; then
  boot=$(sed -n 's/^\[bochs\] private_boot=//p' "$out/$name.report")
  [ -f "$boot" ] || fail "$name missing retained boot clone"
 fi
 volume=$boot@@32256
 grep -Fx 'FLAIR-FAT-MOUNT-DRIVE C' "$out/$name.serial" >/dev/null || fail "$name wrong startup drive"
 while IFS= read -r marker; do
  grep -F "$marker" "$out/$name.serial" >/dev/null || fail "$name missing $marker"
 done < "$markers"
 mdir -i "$volume" ::NEWFOLD > "$out/$name-folder"
 mcopy -o -i "$volume" ::NOTEPAD.DAT "$out/$name-notepad.dat"
 build/apps/notepad/host/readback "$out/$name-notepad.dat" NPAD alpha || fail "$name C save bytes"
 cmp "$data" "$out/$name-data.before" || fail "$name wrote the other A: volume"
 grep -F 'triple_fault=0' "$out/$name.report" >/dev/null || fail "$name CPU fault verdict"
 echo "ok Finder $name: C enumeration/create/launch/save; A byte-for-byte unchanged ($out)"
done
