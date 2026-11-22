#!/bin/sh
# Plan steps 4/6: the ordinary C AUTOEXEC leaves a prompt. Type commands on
# real keys; independently read each written drive, including Bochs's clone.
set -eu
cd "$(dirname "$0")/../../.."
. factory/image/test-media.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 LC_ALL=C
out=$(mktemp -d "$PWD/build/staged-media.XXXXXX")
echo "Staged-media evidence: $out"
sha256sum build/image/*.img > "$out/masters.sha256"
for emulator in qemu bochs; do
 for attachment in with-data alone; do
  run=$out/$emulator-$attachment
  mkdir "$run"
  media_prepare staged "$attachment" "$run"
  {
   printf '%s\r\n' '@ECHO OFF' 'ECHO from-c>PROOF.TXT'
   printf '%s\r\n' 'IF EXIST A:\LEDGER.TXT ECHO MEDIA-A-PRESENT' \
     'IF NOT EXIST A:\LEDGER.TXT ECHO MEDIA-A-ABSENT'
   if [ "$attachment" = with-data ]; then
    printf '%s\r\n' 'ECHO from-a>A:\PROOF.TXT'
   fi
   printf '%s\r\n' 'ECHO MEDIA-DONE'
  } > "$run/MEDIA.BAT"
  mcopy -i "$media_work" "$run/MEDIA.BAT" ::MEDIA.BAT
  trace='km,ke,kd,ki,ka,kret,WMEDIA-DONE,kf,kl,ka,ki,kr,kret'
  case $emulator in
   qemu) driver=build/factory/qemu_harness; set -- --mouse "$trace";;
   bochs) driver=build/factory/desktop-bochs; set -- --keys "$trace"
    export DESKTOP_SERIAL="$run/$emulator.serial";;
  esac
  media_run "$emulator" "$driver" "$@" --keys-after SHELL-READY --expect FLAIR-LIVE-READY \
   --quit-after FLAIR-LIVE-READY --timeout-ms 90000 --name "$emulator" --out "$run" \
   > "$run/stdout" 2> "$run/report" || { cat "$run/report"; exit 1; }
  tr -d '\r' < "$run/$emulator.serial" > "$run/serial.txt"
  grep -Fxi 'C:\>media' "$run/serial.txt" >/dev/null
  grep -Fxi 'C:\>flair' "$run/serial.txt" >/dev/null
  grep '^MEDIA-A-' "$run/serial.txt" > "$run/attachment"
  case $attachment in with-data) expected=MEDIA-A-PRESENT;; alone) expected=MEDIA-A-ABSENT;; esac
  [ "$(cat "$run/attachment")" = "$expected" ] || { cat "$run/attachment"; exit 1; }
  grep -Fx MEDIA-DONE "$run/serial.txt" >/dev/null
  grep -F 'triple_fault=0' "$run/report" >/dev/null
  media_readback "$emulator" "$run/report"
  fsck.fat -n "$media_volume" > "$run/fsck.c"
  mcopy -i "$media_volume" ::PROOF.TXT "$run/c.txt"
  printf 'from-c\r\n' > "$run/c.want"
  cmp "$run/c.want" "$run/c.txt"
  if [ "$attachment" = with-data ]; then
   fsck.fat -n "$media_data" > "$run/fsck.a"
   mcopy -i "$media_data" ::PROOF.TXT "$run/a.txt"
   printf 'from-a\r\n' > "$run/a.want"
   cmp "$run/a.want" "$run/a.txt"
  fi
  echo "ok $emulator $attachment: prompt, typed FLAIR, persisted volume bytes"
 done
done
sha256sum -c "$out/masters.sha256"
