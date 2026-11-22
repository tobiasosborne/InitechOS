#!/bin/sh
# The optional DATA A: is a second Finder volume beside the installed C:
# (C activation plan step 10; accepted GM8 delivery design). Boot the staged
# C: with DATA attached: the A: disk sits 64 pixels below the system disk
# (Mac OS 8.1 capture s8_icons_floppy_sel.png). Double-click it; its root
# must open under A: at the frame seed position (46,115) read from A:'s
# own DESKTOP.DB (goldens/frame/geometry.tsv DRIVEA), shown by dragging its
# title bar 60 pixels right. New Folder there, then exit to DOS. mtools must
# find NEWFOLD and the moved root view (106,115) on A: and no NEWFOLD on C:.
# Then boot C: alone: two desktop icons and no A: disk. The installed
# AUTOEXEC ends at the C:\> prompt; each boot types FLAIR there. Staged names expire
# at plan step 20.
set -eu
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 TZ=UTC LC_ALL=C
out=${FINDER_DATA_VOLUME_OUT:-$(mktemp -d "$PWD/build/finder-data-volume.XXXXXX")}
mkdir -p "$out"
fail() { echo "FAIL Finder data volume: $* ($out)" >&2; exit 1; }
. factory/desktop-trace.sh
for f in build/image/c.img build/image/a-data.img; do
 [ -f "$f" ] || fail "missing staged $f (make image)"
done
root_view() { # image DB-copy -> "x y" of the root view record (IDB1, kind 4, dir 0)
 mcopy -n -o -i "$1" ::DESKTOP.DB "$2" || fail "no DESKTOP.DB on $1"
 od -An -tu1 -v "$2" | awk '
  {for(i=1;i<=NF;i++) b[n++]=$i}
  END{
   if(b[0]!=73||b[1]!=68||b[2]!=66||b[3]!=49) exit 1
   recs=b[6]+256*b[7]
   for(k=0;k<recs;k++){r=8+24*k
    if(b[r]==4 && b[r+2]==0 && b[r+3]==0 && b[r+4]==0){print b[r+17]+256*b[r+18], b[r+19]+256*b[r+20]; exit 0}}
   exit 1}'
}
# (320,240) -> system disk (600,44) -> A: disk 64 lower (600,108); open it.
# Title bar of a frame at (46,115): grab (200,122), drag 60 right; then
# park at (112,102) where exit_desktop's Special menu path starts.
open_a='m100:-98,m100:-98,m80:0,m0:64,l1,l0,l1,l0'
grab='m-100:14,m-100:0,m-100:0,m-100:0,l1,m30:0,m30:0,l0,m-100:0,m-48:-20'
for emulator in qemu bochs; do
 for layout in ca c; do
  name=$emulator-$layout
  boot=$out/$name-boot.img
  cp build/image/c.img "$boot"
  markers=$out/$name.markers; : > "$markers"
  if [ "$layout" = ca ]; then
   data=$out/$name-data.img
   cp build/image/a-data.img "$data"; cp "$data" "$out/$name-data.before"
   set -- --disk2 "$data"
   trace=$(
    printf '%s' 'kf,kl,ka,ki,kr,kret,WFLAIR-LIVE-READY,'
    inject "$open_a"; await_marker FINDER-OPEN-VOLUME
    inject "$grab"; await_marker 'FLAIR-DRAG win'
    inject kctrl-n; await_marker 'FINDER-NEW-FOLDER name=NEWFOLD'
    inject "$exit_desktop"; await_marker FINDER-EXIT-DOS-ALERT; inject kret
   )
  else
   set --
   # (320,240) -> (112,102), then the Special menu's Exit to InitechDOS.
   trace=$(printf '%s' 'kf,kl,ka,ki,kr,kret,WFLAIR-LIVE-READY,'; move_by -208 -138; inject "$exit_desktop"; await_marker FINDER-EXIT-DOS-ALERT; inject kret)
  fi
  trace=${trace%,}
  if [ "$emulator" = bochs ]; then
   driver=build/factory/desktop-bochs; set -- "$@" --keys "$trace" --rtc-time0 699440400
  else
   driver=build/factory/qemu_harness; set -- "$@" --mouse "$trace" --boot-rw
  fi
  export DESKTOP_SERIAL="$out/$name.serial"
  "$driver" --disk "$boot" "$@" --keys-after SHELL-READY --timeout-ms 90000 \
   --expect FLAIR-EXIT-TO-DOS --quit-after FLAIR-EXIT-TO-DOS --name "$name" --out "$out" \
   > "$out/$name.stdout" 2> "$out/$name.report" || fail "$name journey"
  grep -F 'triple_fault=0' "$out/$name.report" >/dev/null || fail "$name CPU fault verdict"
  if [ "$emulator" = bochs ]; then
   boot=$(sed -n 's/^\[bochs\] private_boot=//p' "$out/$name.report")
   [ -f "$boot" ] || fail "$name missing retained boot clone"
  fi
  s=$out/$name.serial
  grep -Fx 'FLAIR-FAT-MOUNT-DRIVE C' "$s" >/dev/null || fail "$name wrong system drive"
  while IFS= read -r marker; do
   grep -F "$marker" "$s" >/dev/null || fail "$name missing $marker"
  done < "$markers"
  if mdir -i "$boot@@32256" ::NEWFOLD >/dev/null 2>&1; then fail "$name created NEWFOLD on C:"; fi
  if [ "$layout" = ca ]; then
   grep -Fx 'FINDER-DESKTOP-ICONS n=3' "$s" >/dev/null || fail "$name desktop is not C:, A: and Trash"
   grep -Fx 'FINDER-VOLUME-ICON drive=A name=DATA' "$s" >/dev/null || fail "$name no DATA disk"
   grep -E '^FINDER-OPEN-VOLUME win=[0-9]+ n=[0-9]+ drive=A$' "$s" >/dev/null || fail "$name did not open A:"
   grep -E '^FLAIR-DRAG win [0-9]+ \(46,115\)->\(106,115\)$' "$s" >/dev/null || fail "$name A: root not at its seed position"
   mdir -i "$data" ::NEWFOLD >/dev/null 2>&1 || fail "$name no NEWFOLD on A:"
   view=$(root_view "$data" "$out/$name-a.db") || fail "$name no A: root view"
   [ "$view" = "106 115" ] || fail "$name A: root view is $view"
   echo "ok Finder $name: DATA A: opened at (46,115), folder and view on A:, C: untouched by them ($out)"
  else
   grep -Fx 'FINDER-DESKTOP-ICONS n=2' "$s" >/dev/null || fail "$name C-only desktop"
   if grep -F 'FINDER-VOLUME-ICON' "$s" >/dev/null; then fail "$name showed a missing A:"; fi
   echo "ok Finder $name: C: alone shows no A: disk ($out)"
  fi
 done
done
