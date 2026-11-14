#!/bin/sh
# Actual Paint resource graph through INT81; seven holds plus cross-title drag.
set -eu
repo=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
cd "$repo"
base=${EMPTY_MENU_SMOKE_OUT:-build/flair/smoke-empty-menu}
mkdir -p "$base"
out=$(mktemp -d "$base/run.XXXXXX")
printf 'empty-menu evidence: %s\n' "$out"
${HOSTCC:-cc} -std=c11 -O1 -Wall -Wextra -Werror -Iflair/tests/goldens \
 flair/tests/menu_empty_pixels.c -o "$out/pixels"
"$out/pixels" positions > "$out/positions"
edit_x=$(awk '$2=="Edit" {print $3}' "$out/positions")
image_x=$(awk '$2=="Image" {print $3}' "$out/positions")
cp build/image/data.img "$out/data.img"
markers=$out/markers; : > "$markers"
emulator=qemu
. factory/desktop-trace.sh
app_entries=$(mdir -b -i "$out/data.img" ::APPS/)
point() {
 inject 'm-100:-100,m-100:-100,m-100:-100,m-100:-100,m-100:-100,m-100:-100,m-100:-100'
 move_by "$1" "$2"
}
trace=$(
 await_marker 'FLAIR-CLOCK 9:00 am'
 inject "$open_disk"; await_marker FINDER-OPEN-VOLUME
 inject "$open_apps"; await_marker 'FINDER-OPEN-FOLDER name=APPS'
 inject m22:20; launch_app PAINT.EXE PAINT
 while read -r id title x; do
  point "$x" 10; inject "w150,D$out/$title-idle.ppm,l1"
  await_marker "FLAIR-MENU-DROP menu=$id"
  inject "D$out/$title-held.ppm,l0"
  await_marker "FLAIR-MENU menu=$id item=0"
  inject "D$out/$title-released.ppm"
 done < "$out/positions"
 # Edit -> File -> Image, held throughout; then release in empty Image.
 point 52 10; inject "w150,D$out/file-idle.ppm"
 point "$edit_x" 10; inject l1; await_marker 'FLAIR-MENU-DROP menu=129'
 inject "m$((52-edit_x)):0"; await_marker 'FLAIR-MENU-XDROP menu=128'
 inject "D$out/file-open.ppm,m$((image_x-52)):0"; await_marker 'FLAIR-MENU-XDROP menu=130'
 inject "D$out/drag-empty.ppm,l0"; await_marker 'FLAIR-MENU menu=129 item=0'
 inject "D$out/drag-released.ppm,kctrl-q"
)
trace=${trace%,}
printf '%s\n' "$trace" > "$out/trace"
INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 build/factory/qemu_harness \
 --disk build/image/boot.img --disk2 "$out/data.img" --mouse "$trace" \
 --keys-after FLAIR-LIVE-READY --expect 'TENANT-EXIT rc=0 via=exit' \
 --quit-after 'TENANT-EXIT rc=0 via=exit' --timeout-ms 90000 --name empty \
 --out "$out" > "$out/stdout" 2> "$out/report" || { cat "$out/report"; exit 1; }
while read -r id title x; do
 "$out/pixels" "$title" "$out/$title-idle.ppm" "$out/$title-held.ppm" "$out/$title-released.ppm"
done < "$out/positions"
"$out/pixels" Image "$out/Image-idle.ppm" "$out/drag-empty.ppm" "$out/drag-released.ppm"
"$out/pixels" File "$out/file-idle.ppm" "$out/file-open.ppm" "$out/file-idle.ppm"
[ "$(grep -c 'FLAIR-MENU menu=.* item=0 ' "$out/empty.serial")" -eq 8 ]
echo 'EMPTY MENU SMOKE: seven holds and File/empty drag, no selected command'
