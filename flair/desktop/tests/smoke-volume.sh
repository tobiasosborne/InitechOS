#!/bin/sh
# The Finder belongs to the installed system volume (C activation plan step
# 8). Boot the staged installed C: with the DATA A: attached; on this test
# clone only, AUTOEXEC switches the default drive to A: before FLAIR, so a
# Finder bound to the DOS default drive would open, create and save on A:.
# Then repeat with C: alone and its AUTOEXEC as installed, typing FLAIR at
# the C:\> prompt. Every case opens
# the C: root, creates a folder, opens C:\APPS, launches the installed Note
# Pad and saves; mtools reads C: back at byte offset 32256 and Note Pad's own
# independent reader decodes the save. A: must stay byte-for-byte unchanged.
# Staged pair names (c.img, a-data.img) expire with plan step 20.
set -eu
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 TZ=UTC LC_ALL=C
out=${FINDER_VOLUME_SMOKE_OUT:-$(mktemp -d "$PWD/build/finder-volume.XXXXXX")}
mkdir -p "$out"
fail() { echo "FAIL Finder volume: $* ($out)" >&2; exit 1; }
. factory/desktop-trace.sh
for f in build/image/c.img build/image/a-data.img; do
 [ -f "$f" ] || fail "missing staged $f (make image)"
done
printf '@ECHO OFF\r\nPATH C:\\;C:\\APPS\r\nA:\r\nFLAIR\r\n' > "$out/autoexec-a"
for emulator in qemu bochs; do
 for layout in ca c; do
  name=$emulator-$layout
  cp build/image/c.img "$out/$name-boot.img"
  boot=$out/$name-boot.img
  volume=$boot@@32256
  data=
  set --
  if [ "$layout" = ca ]; then
   mcopy -o -i "$volume" "$out/autoexec-a" ::AUTOEXEC.BAT
   cp build/image/a-data.img "$out/$name-data.img"
   data=$out/$name-data.img
   cp "$data" "$out/$name-data.before"
   set -- --disk2 "$data"
  fi
  mdir -i "$volume" ::NEWFOLD >/dev/null 2>&1 && fail "$name C: already has NEWFOLD"
  mdir -i "$volume" ::NOTEPAD.DAT >/dev/null 2>&1 && fail "$name C: already has NOTEPAD.DAT"
  # The test clone's AUTOEXEC starts FLAIR from A:; the installed AUTOEXEC
  # ends at the C:\> prompt, where the user types FLAIR (plan step 4).
  start=FLAIR-LIVE-READY typed=
  [ "$layout" = ca ] || { start=SHELL-READY typed='kf,kl,ka,ki,kr,kret,WFLAIR-LIVE-READY,'; }
  markers=$out/$name.markers; : > "$markers"
  app_entries=$(mdir -b -i "$volume" ::APPS/)
  trace=$(
   printf '%s' "$typed"
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
  if [ "$emulator" = bochs ]; then
   driver=build/factory/desktop-bochs; args=--keys; set -- "$@" --rtc-time0 699440400
  else
   driver=build/factory/qemu_harness; args=--mouse; set -- "$@" --boot-rw
  fi
  export DESKTOP_SERIAL="$out/$name.serial"
  "$driver" --disk "$boot" "$@" \
   --keys-after "$start" "$args" "$trace" --timeout-ms 90000 \
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
  mdir -i "$volume" ::NEWFOLD > "$out/$name-folder" || fail "$name no NEWFOLD on C:"
  mcopy -o -i "$volume" ::NOTEPAD.DAT "$out/$name-notepad.dat" || fail "$name no NOTEPAD.DAT on C:"
  build/apps/notepad/host/readback "$out/$name-notepad.dat" NPAD alpha || fail "$name C save bytes"
  if [ "$layout" = ca ]; then
   cmp "$data" "$out/$name-data.before" || fail "$name wrote the A: data volume"
  fi
  grep -F 'triple_fault=0' "$out/$name.report" >/dev/null || fail "$name CPU fault verdict"
  echo "ok Finder $name: C enumeration/create/launch/save${data:+; A byte-for-byte unchanged} ($out)"
  data=
 done
done
