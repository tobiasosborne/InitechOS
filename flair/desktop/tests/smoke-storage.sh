#!/bin/sh
# Booted Finder policy journey, on the product boot disk and an mtools volume.
# Run from the repository root. All input is external; no test kernel paths.
set -eu
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 TZ=UTC LC_ALL=C
out=${FINDER_STORAGE_SMOKE_OUT:-$(mktemp -d "$PWD/build/finder-storage.XXXXXX")}
mkdir -p "$out"
fail() { echo "FAIL Finder storage: $* ($out)" >&2; exit 1; }
. factory/desktop-trace.sh
printf 'Finder source bytes\r\n' > "$out/source.txt"
printf '@ECHO OFF\r\nFLAIR\r\n' > "$out/autoexec"
for kind in 12 16; do
 sectors=2880; [ "$kind" = 12 ] || sectors=16384
 truncate -s $((sectors*512)) "$out/fat$kind.img"
 mformat -i "$out/fat$kind.img" -T "$sectors" -h 16 -s 63 -c 1 -N 0x19910330 ::
 mmd -i "$out/fat$kind.img" ::FOLDER ::TRASH
 mcopy -i "$out/fat$kind.img" "$out/source.txt" ::SOURCE.TXT
 mcopy -i "$out/fat$kind.img" "$out/source.txt" ::TRASH/GONE.TXT
 mcopy -i "$out/fat$kind.img" "$out/autoexec" ::AUTOEXEC.BAT
 mattrib -i "$out/fat$kind.img" +h ::AUTOEXEC.BAT ::TRASH
 for emulator in qemu bochs; do
  name=$emulator-fat$kind; markers=$out/$name.markers; : > "$markers"
  cp "$out/fat$kind.img" "$out/$name.img"
  trace=$(
   if [ "$kind" = 16 ]; then
    await_marker 'FINDER-ALERT-LINE Write protect error.'; inject kret
   elif [ "$emulator" = qemu ]; then await_marker 'FLAIR-CLOCK 9:00 am'; fi
   inject "$open_disk"; await_marker FINDER-OPEN-VOLUME
   inject kctrl-n
   if [ "$kind" = 16 ]; then
    await_marker 'FINDER-NEW-FOLDER-FAIL rc=-21'
    await_marker 'FINDER-ALERT-LINE Write protect error.'; inject kret
    # Root listing: FOLDER, SOURCE.TXT; fixed fixture, full-name grid.
    move_by -386 38
    inject 'l1,m-100:0,m-28:0,l0'
    await_marker 'FINDER-MOVE-REFUSED reason=write-protect name=SOURCE.TXT'
    await_marker 'FINDER-ALERT-LINE Write protect error.'; inject kret
    move_by 128 0
    inject l1; move_by 386 340; inject l0
    await_marker 'FINDER-MOVE-REFUSED reason=write-protect name=SOURCE.TXT'
    await_marker 'FINDER-ALERT-LINE Write protect error.'; inject kret
    # Special > Empty Trash, then approve the ordinary confirmation.
    move_by -430 -412; inject 'l1,m0:32,l0'
    await_marker 'FINDER-TRASH-ALERT n=1 k=1'; inject kret
    await_marker 'FINDER-TRASH-EMPTIED purged=0 refused=1'
    await_marker 'FINDER-ALERT-LINE Write protect error.'; inject kret
    inject kctrl-w; await_marker 'DESKTOP-DB-WRITE-FAIL rc=-15'
    await_marker 'FINDER-ALERT-LINE Write protect error.'; inject kret
    # Pointer is at (170,42); return to the common exit anchor (112,102).
    move_by -58 60
   else
    await_marker 'FINDER-NEW-FOLDER name=NEWFOLD parent=0'
    move_by -488 58
   fi
   inject "$exit_desktop"; await_marker FINDER-EXIT-DOS-ALERT; inject kret
  )
  trace=${trace%,}
  driver=build/factory/qemu_harness
  args='--mouse'
  set --
  if [ "$emulator" = bochs ]; then
   driver=build/factory/desktop-bochs; args='--keys'; set -- --rtc-time0 699440400
  fi
  export DESKTOP_SERIAL="$out/$name.serial"
  "$driver" --disk build/image/boot.img --disk2 "$out/$name.img" \
   "$@" \
   --keys-after FLAIR-LIVE-READY "$args" "$trace" --timeout-ms 60000 \
   --expect FLAIR-EXIT-TO-DOS --quit-after FLAIR-EXIT-TO-DOS --name "$name" --out "$out" \
   > "$out/$name.stdout" 2> "$out/$name.report" || fail "$name journey"
  sort "$markers" | uniq -c | while read -r count marker; do
   seen=$(grep -Fc "$marker" "$out/$name.serial" || :)
   [ "$seen" -ge "$count" ] || fail "$name missing $marker ($seen/$count)"
  done
  if [ "$kind" = 16 ]; then
   cmp "$out/fat$kind.img" "$out/$name.img" || fail "$name changed a sector"
   mtype -i "$out/$name.img" ::SOURCE.TXT > "$out/$name-source"
   cmp "$out/source.txt" "$out/$name-source" || fail "$name source bytes"
  else
   mdir -i "$out/$name.img" ::NEWFOLD > "$out/$name-positive"
  fi
  grep -F 'triple_fault=0' "$out/$name.report" >/dev/null || fail "$name CPU fault verdict"
  echo "ok Finder storage $name: real input, period alerts, independent disk oracle ($out)"
 done
done
