#!/bin/sh
# GM3: actual INT 81h tenant, cooked input, modal completion and mtools bytes.
# Derived from phase-2 review-landed-toolbox-evidence/live-copytest.sh; no
# scenario or fault hook is compiled into the kernel or product desktop.
set -eu
repo=${FILECOPY_REPO:-$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)}
cd "$repo"
out=${FILECOPY_SMOKE_OUT:-build/flair/smoke-filecopy}
mkdir -p "$out"
make -s -C "${FILECOPY_TENANT_SRC:-flair/tests/copy_tenant}" TOP="$repo" OUT="$repo/$out/tenant" all
case_list=${FILECOPY_CASES:-'escape empty control button close success readonly missing bad-rate'}
for mode in $case_list; do
 d=$out/$mode
 mkdir -p "$d"
 cp build/image/data.img "$d/data.img"
 # Reuse a visible entry on this disposable disk; the product does not ship
 # CTENANT.EXE, and appending it would put it beyond the first icon page.
 mcopy -o -i "$d/data.img" "$out/tenant/COPYTEST.EXE" ::APPS/CALC.EXE
 printf 'Previous saved document.\r\n' > "$d/before.txt"
 printf 'Replacement document.\r\n' > "$d/source.txt"
 [ "$mode" != empty ] || : > "$d/source.txt"
 printf 'PROGRESS_BAR_RATE=20\r\n' > "$d/rate.cfg"
 [ "$mode" != bad-rate ] || printf 'PROGRESS_BAR_RATE=0\r\n' > "$d/rate.cfg"
 mcopy -o -i "$d/data.img" "$d/before.txt" ::TARGET.DAT
 mcopy -o -i "$d/data.img" "$d/source.txt" ::SOURCE.DAT
 mcopy -o -i "$d/data.img" "$d/rate.cfg" ::APPS/MICHAEL.CFG
 [ "$mode" != readonly ] || mattrib -i "$d/data.img" +r ::TARGET.DAT
 [ "$mode" != missing ] || mdel -i "$d/data.img" ::SOURCE.DAT
 markers=$d/markers; : > "$markers"
 emulator=qemu
 . factory/desktop-trace.sh
 app_entries=$(mdir -b -i "$d/data.img" ::APPS/)
 # Clamp to (0,0) before each absolute position using ordinary mouse packets.
 point() {
  inject 'm-100:-100,m-100:-100,m-100:-100,m-100:-100,m-100:-100,m-100:-100,m-100:-100'
  move_by "$1" "$2"
 }
 trace=$(
  await_marker 'FLAIR-CLOCK 9:00 am'
  inject "$open_disk"; await_marker FINDER-OPEN-VOLUME
  inject "$open_apps"; await_marker 'FINDER-OPEN-FOLDER name=APPS'
  inject m22:20; launch_app CALC.EXE COPYTEST
  point 520 350; inject "w100,D$d/idle.ppm,ks"
  case $mode in missing|bad-rate) :;; *)
   await_marker 'TENANT-FILECOPY up bytes='
   inject "w100,D$d/copy.ppm"
  esac
  case $mode in
   escape|empty) inject kesc;;
   control) inject kctrl-dot;;
   button) point 450 288; inject 'l1,l0';;
   close) point 150 210; inject 'l1,l0';;
  esac
  await_marker 'TENANT-EVT what=145'
  point 520 350; inject "w100,D$d/after.ppm,kq"
 )
 trace=${trace%,}
 printf '%s\n' "$trace" > "$d/trace"
 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 build/factory/qemu_harness \
  --disk build/image/boot.img --disk2 "$d/data.img" --mouse "$trace" \
  --keys-after FLAIR-LIVE-READY --expect 'TENANT-EXIT rc=0 via=exit' \
  --quit-after 'TENANT-EXIT rc=0 via=exit' --timeout-ms 45000 \
  --name copy --out "$d" > "$d/stdout" 2> "$d/report" || { cat "$d/report"; exit 1; }
 [ "$(grep -c 'TENANT-EVT what=145' "$d/copy.serial")" -eq 1 ]
 case $mode in
  escape|empty|control|button|close) expected=-4;;
  success) expected=$(wc -c < "$d/source.txt");;
  readonly) expected=-261;;
  missing) expected=-258;;
  bad-rate) expected=-3;;
 esac
 # The delivered event carries the result in hexadecimal; the modal/refusal
 # serial line independently records it in decimal.
 grep -E "TENANT-FILECOPY-(DONE|REFUSED) rc=$expected( |$)" "$d/copy.serial" >/dev/null
 mcopy -o -i "$d/data.img" ::TARGET.DAT "$d/after.txt"
 if [ "$mode" = success ]; then cmp "$d/source.txt" "$d/after.txt"; else cmp "$d/before.txt" "$d/after.txt"; fi
 if [ "$mode" != missing ]; then
  mcopy -o -i "$d/data.img" ::SOURCE.DAT "$d/source-after.txt"
  cmp "$d/source.txt" "$d/source-after.txt"
 fi
 echo "ok FILE COPY $mode: one completion rc=$expected; independent disk bytes"
done
