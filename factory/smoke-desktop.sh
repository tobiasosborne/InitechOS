#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
start=$(date +%s)
out=${DESKTOP_SMOKE_OUT:-build/smoke-desktop}
mkdir -p "$out"
fail() { echo "FAIL desktop: $*" >&2; exit 1; }
. factory/desktop-trace.sh
make -s -f factory/desktop-tools.mk test
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
list=${DESKTOP_JOURNEYS:-factory/desktop-journeys.list}
# Reject missing/empty entries; list ordering is also the journey ordering.
journeys=$(sed '/^[[:space:]]*#/d; /^[[:space:]]*$/d' "$list")
[ -n "$journeys" ] || fail 'empty journey list'
for dir in $journeys; do
 [ -f "$dir/journey.sh" ] && [ -f "$dir/verify.sh" ] || fail "incomplete journey $dir"
done
app_entries=$(mdir -b -i build/image/data.img ::APPS/)
n=0
for emulator in qemu bochs; do
 markers=$out/$emulator.markers
 : > "$markers"
 trace=$(
  if [ "$emulator" = qemu ]; then await_marker 'FLAIR-CLOCK 9:00 am'; fi
  capture desktop
  inject "$open_disk"; await_marker 'FINDER-OPEN-VOLUME'; capture disk
  inject "$open_apps"; await_marker 'FINDER-OPEN-FOLDER name=APPS'; capture apps
  inject "m22:20" # common pointer anchor: first APPS icon
  for dir in $journeys; do
   # A subshell isolates per-app variables and helper names.
   ( . "./$dir/journey.sh"; journey )
  done
  if [ "$emulator" = qemu ]; then
   inject "$exit_desktop"; await_marker 'FINDER-EXIT-DOS-ALERT'
   inject kret; await_marker FLAIR-EXIT-TO-DOS
   inject w300; capture dos; inject 'kf,kl,ka,ki,kr,kret'
  fi
 )
 trace=${trace%,}
 # The final exit is already the driver's completion marker. Leaving a W
 # token after it would correctly trip Bochs's "input after completion" guard.
 if [ "$emulator" = bochs ]; then trace=${trace%,WTENANT-EXIT rc=0 via=exit}; fi
 cp build/image/data.img "$out/$emulator-data.img"
 if [ "$emulator" = qemu ]; then
  set -- --mouse "$trace" --expect FLAIR-EXIT-TO-DOS --quit-after FLAIR-LIVE-READY --quit-after-count 2 \
   --screendump --screendump-after FLAIR-LIVE-READY --screendump-after-count 2
 else
  set -- --rtc-time0 699440400 --allow-reboot --keys "$trace" --expect 'TENANT-EXIT rc=0 via=exit' \
   --quit-after 'TENANT-EXIT rc=0 via=exit' --quit-after-count "$(printf '%s\n' "$journeys" | wc -l)"
 fi
 driver="build/factory/${emulator}_harness"
 [ "$emulator" != bochs ] || driver=build/factory/desktop-bochs
 export DESKTOP_SERIAL="$out/$emulator.serial"
 if ! "$driver" --disk build/image/boot.img --disk2 "$out/$emulator-data.img" \
  --keys-after FLAIR-LIVE-READY "$@" --timeout-ms 90000 --name "$emulator" --out "$out" \
  > "$out/$emulator.stdout" 2> "$out/$emulator.report"; then
  cat "$out/$emulator.report"; case $emulator in qemu) fail "QEMU session";; bochs) fail "Bochs session";; esac
 fi
 sort "$markers" | uniq -c | while read -r count marker; do
  seen=$(grep -Fc "$marker" "$out/$emulator.serial" || :)
  [ "$seen" -ge "$count" ] || fail "$emulator missing $marker ($seen/$count)"
 done
 if [ "$emulator" = qemu ]; then
  [ "$(grep -c FLAIR-LIVE-READY "$out/qemu.serial")" -eq 2 ] || fail 'QEMU FLAIR restart'
  grep -F 'A:\>' "$out/qemu.serial" >/dev/null || fail 'QEMU DOS prompt'
 fi
 echo "ok $emulator: Finder disk/APPS, all journeys exited cleanly; desktop lifecycle"
 n=$((n+1))
 for dir in $journeys; do
  app=$(basename "$(dirname "$(dirname "$dir")")")
  # Keep the original worksheet failure diagnostic for red-desktop.sh.
  label="$app saved content"
  [ "$app" != initech123 ] || label='worksheet content'
  sh "$dir/verify.sh" "$out/$emulator-data.img" "$out/$emulator-$app" || case $emulator in qemu) fail "QEMU $label";; bochs) fail "Bochs $label";; esac
  echo "ok $emulator: $app independent saved-file verifier"
  n=$((n+1))
 done
 if [ "$emulator" = qemu ]; then
  grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/qemu.report" >/dev/null || fail 'QEMU fault verdict'
  [ "$(grep -c 'CPU Reset' "$out/qemu.qemu.log")" -eq 2 ] || fail 'QEMU extra reset'
  if grep -Ei 'triple fault|check_exception' "$out/qemu.qemu.log" >/dev/null; then fail 'QEMU fault log'; fi
 else
  grep -F 'triple_fault=0' "$out/bochs.report" >/dev/null || fail 'Bochs fault verdict'
  grep -Fx VBE-MODE=0101 "$out/bochs.serial" >/dev/null || fail 'Bochs indexed VBE'
 fi
 echo "ok $emulator: video and CPU fault checks"
 n=$((n+1))
done
legacy=${LEGACY_TREE:-/home/tobiasosborne/Projects/InitechOS-legacy}
cp "$legacy/build/flair_data.img" "$out/legacy-data.img"
build/factory/qemu_harness --disk "$legacy/build/flair_tenants_interactive.img" \
 --disk2 "$out/legacy-data.img" --expect FLAIR-LIVE-READY --quit-after 'FLAIR-CLOCK 9:00 am' \
 --screendump --screendump-after 'FLAIR-CLOCK 9:00 am' --timeout-ms 15000 \
 --name legacy --out "$out" > "$out/legacy.stdout" 2> "$out/legacy.report" || fail 'legacy reference capture'
build/factory/pixels-desktop "$out/legacy.ppm" "$out/desktop.ppm" > "$out/pixels.txt"
cat "$out/pixels.txt"
n=$((n+1))
elapsed=$(($(date +%s)-start))
printf '%s\n' "$elapsed" > "$out/elapsed-seconds"
echo "DESKTOP SMOKE: $n checks, 0 failures (QEMU + Bochs, 8 MiB; ${elapsed}s)"
