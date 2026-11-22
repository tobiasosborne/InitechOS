#!/bin/sh
# Plan step 7. Launch an ordinary disk client on each system volume.
# The expected INT 81h return values and persisted bytes come from the
# chosen media layout and IBM DOS 3.30 Technical Reference p. 6-81.
# The two layout names expire at C activation plan step 20.
set -eu
cd "$(dirname "$0")/../../.."
. factory/image/test-media.sh
. factory/desktop-trace.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
out=$(mktemp -d build/sdk/drive.XXXXXX)
echo "File-session drive evidence: $out"
fail() { echo "FAIL file-session drive: $* ($run)" >&2; exit 1; }
tenant=$1
printf 'A: search fixture\r\n' > "$out/probe.txt"
printf 'Other volume\r\n' > "$out/decoy.txt"
sha256sum build/image/boot.img build/image/data.img build/image/c.img build/image/a-data.img > "$out/masters.sha256"
for emulator in qemu bochs; do
 for layout in canonical staged; do
  run=$out/$emulator-$layout
  mkdir "$run"
  media_prepare "$layout" with-data "$run"
  case $layout in
   canonical) letter=A; drive=0; arm=FLAIR-LIVE-READY; other=$media_boot@@32256;;
   staged) letter=C; drive=2; arm=SHELL-READY; other=$media_data;;
  esac
  [ "$media_system" = "$letter" ] || fail "expected $letter: system"
  mcopy -i "$media_work" "$tenant" ::APPS/SDKDRIVE.EXE
  mcopy -i "$media_data" "$out/probe.txt" ::SDKPROBE.TXT
  mcopy -i "$other" "$out/decoy.txt" ::SDKDRIVE.TXT
  app_entries=$(mdir -b -i "$media_work" ::APPS/)
  markers=$run/markers; : > "$markers"
  trace=$(
   if [ "$layout" = staged ]; then
    inject 'kf,kl,ka,ki,kr,kret'
    await_marker FLAIR-LIVE-READY
   fi
   inject "$open_disk"; await_marker FINDER-OPEN-VOLUME
   inject "$open_apps"; await_marker 'FINDER-OPEN-FOLDER name=APPS'
   launch_app SDKDRIVE.EXE
  )
  trace=${trace%WFLAIR-DISPATCH app=SDKDRIVE,}
  case $emulator in
   qemu) driver=build/factory/qemu_harness; set -- --mouse "${trace%,}";;
   bochs) driver=build/factory/desktop-bochs; set -- --keys "${trace%,}";;
  esac
  export DESKTOP_SERIAL="$run/drive.serial"
  media_run "$emulator" "$driver" "$@" --keys-after "$arm" \
   --expect 'TENANT-EXIT rc=0 via=exit' --quit-after 'TENANT-EXIT rc=0 via=exit' \
   --timeout-ms 90000 --name drive --out "$run" > "$run/stdout" 2> "$run/report" ||
   fail "$emulator $layout disk client did not finish"
  grep -F 'triple_fault=0' "$run/report" >/dev/null || fail "CPU fault verdict"
  grep -Fx "FLAIR-FAT-MOUNT-DRIVE $letter" "$DESKTOP_SERIAL" >/dev/null || fail "system mount"
  grep -F 'TENANT-REGISTER ok app=SDKDRIVE' "$DESKTOP_SERIAL" >/dev/null || fail "disk client never registered"
  printf '%s\n' -2 "$drive" "$drive" -2 > "$run/drive.expected"
  sed -n 's/^TENANT-GATE ax=0x008c -> //p' "$DESKTOP_SERIAL" > "$run/drive.actual"
  cmp "$run/drive.expected" "$run/drive.actual" || fail "008Ch drive or registration result"
  media_readback "$emulator" "$run/report"
  printf '%s:\r\n' "$letter" > "$run/written.expected"
  mcopy -i "$media_volume" ::SDKDRIVE.TXT "$run/written.actual"
  cmp "$run/written.expected" "$run/written.actual" || fail "$letter: file bytes"
  if [ "$layout" = canonical ]; then other=$media_written@@32256; fi
  mcopy -i "$other" ::SDKDRIVE.TXT "$run/decoy.actual"
  cmp "$out/decoy.txt" "$run/decoy.actual" || fail "other volume file bytes"
  echo "ok $emulator $layout: 008Ch=$drive, unqualified write on $letter:, other volume decoy preserved"
 done
done
sha256sum -c "$out/masters.sha256"
