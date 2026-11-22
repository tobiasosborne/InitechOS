#!/bin/sh
# The C: root window keeps its own saved position across a cold restart while
# the optional DATA A: (with its frame seed DESKTOP.DB) stays untouched (C
# activation plan step 9). Each emulator boots the staged installed C: with
# DATA attached twice from the same written C: master. Each boot opens the
# root, drags its title bar by the same horizontal amount, and exits to DOS,
# which commits DESKTOP.DB. mtools decodes C:'s root view record (kind 4,
# dir 0, IDB1 layout: x/y little-endian at record bytes 17/19). If the second
# boot reopened at the saved position, its x advanced by the same step again;
# a lost restore leaves both boots at the same x. A: must be byte-identical.
# ROOT_VIEW_CONTROL=pristine boots the second run from the unwritten C: and
# must fail (the check's red control). Staged names expire at plan step 20.
set -eu
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 TZ=UTC LC_ALL=C
out=${FINDER_ROOT_VIEW_OUT:-$(mktemp -d "$PWD/build/finder-root-view.XXXXXX")}
mkdir -p "$out"
fail() { echo "FAIL Finder root view: $* ($out)" >&2; exit 1; }
. factory/desktop-trace.sh
root_x() {
 mcopy -n -o -i "$1@@32256" ::DESKTOP.DB "$2" || fail "no C: DESKTOP.DB"
 od -An -tu1 -v "$2" | awk '
  {for(i=1;i<=NF;i++) b[n++]=$i}
  END{
   if(b[0]!=73||b[1]!=68||b[2]!=66||b[3]!=49) exit 1
   recs=b[6]+256*b[7]
   for(k=0;k<recs;k++){r=8+24*k
    if(b[r]==4 && b[r+2]==0 && b[r+3]==0 && b[r+4]==0){print b[r+17]+256*b[r+18], b[r+19]+256*b[r+20]; exit 0}}
   exit 1}' || fail "no C: root view record in $2"
}
# Pointer: (320,240) at boot; open_disk leaves it on the disk icon (600,44).
# The root title bar spans x 20..380 at the default frame (README: windows
# start at y 40). Grab it at (200,48), drag 60 right, release; after a
# correct restore the frame starts at x 80, so (200,48) is still title bar.
# Then park at (112,102), where exit_desktop's Special menu path starts.
grab='m-100:4,m-100:0,m-100:0,m-100:0,l1,m30:0,m30:0,l0,m-100:0,m-48:54'
markers=/dev/null
# The installed AUTOEXEC ends at the C:\> prompt; type FLAIR there.
trace=$(
 printf '%s' 'kf,kl,ka,ki,kr,kret,WFLAIR-LIVE-READY,'
 inject "$open_disk"; await_marker FINDER-OPEN-VOLUME
 inject "$grab"; await_marker 'FLAIR-DRAG win'
 inject "$exit_desktop"; await_marker FINDER-EXIT-DOS-ALERT; inject kret
)
trace=${trace%,}
for emulator in qemu bochs; do
 boot=$out/$emulator-boot.img
 cp build/image/c.img "$boot"; cp "$boot" "$out/$emulator-boot.pristine"
 data=$out/$emulator-data.img
 cp build/image/a-data.img "$data"; cp "$data" "$out/$emulator-data.before"
 for run in 1 2; do
  name=$emulator-$run
  if [ "$run" = 2 ] && [ "${ROOT_VIEW_CONTROL:-}" = pristine ]; then cp "$out/$emulator-boot.pristine" "$boot"; fi
  if [ "$emulator" = bochs ]; then
   driver=build/factory/desktop-bochs; set -- --keys "$trace" --rtc-time0 699440400
  else
   driver=build/factory/qemu_harness; set -- --mouse "$trace" --boot-rw
  fi
  # A fresh path per boot, as factory/desktop-bochs-sessions.sh does: Bochs
  # leaves its image lock behind when it is stopped at the quit marker.
  rundata=$out/$name-data.img
  cp "$data" "$rundata"
  export DESKTOP_SERIAL="$out/$name.serial"
  "$driver" --disk "$boot" --disk2 "$rundata" "$@" --keys-after SHELL-READY \
   --timeout-ms 90000 --expect FLAIR-EXIT-TO-DOS --quit-after FLAIR-EXIT-TO-DOS \
   --name "$name" --out "$out" > "$out/$name.stdout" 2> "$out/$name.report" || fail "$name journey"
  grep -F 'triple_fault=0' "$out/$name.report" >/dev/null || fail "$name CPU fault verdict"
  if [ "$emulator" = bochs ]; then
   written=$(sed -n 's/^\[bochs\] private_boot=//p' "$out/$name.report")
   [ -f "$written" ] || fail "$name missing retained boot clone"
   cp "$written" "$boot"
  fi
  grep -Fx 'FLAIR-FAT-MOUNT-DRIVE C' "$out/$name.serial" >/dev/null || fail "$name wrong startup drive"
  grep -F 'FLAIR-DRAG win' "$out/$name.serial" > "$out/$name.drag" || fail "$name no drag"
  view=$(root_x "$boot" "$out/$name.db") || fail "$name root view record"
  set -- $view; eval "x$run=$1 y$run=$2"
  cmp "$rundata" "$out/$emulator-data.before" || fail "$name wrote the A: data volume"
  cp "$rundata" "$data"
 done
 echo "$emulator root view: default x=20, boot 1 x=$x1 y=$y1, boot 2 x=$x2 y=$y2"
 [ "$y1" -eq 40 ] && [ "$y2" -eq 40 ] || fail "$emulator root view moved vertically"
 [ "$x1" -gt 20 ] || fail "$emulator first drag not saved"
 [ $((x2 - x1)) -eq $((x1 - 20)) ] || fail "$emulator second boot did not reopen at the saved position"
 echo "ok Finder $emulator: C root reopened at its saved view after a cold restart; A byte-for-byte unchanged ($out)"
done
