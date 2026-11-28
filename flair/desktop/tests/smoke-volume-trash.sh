#!/bin/sh
# Plan step 11. Drive the same Finder gestures on QEMU and Bochs, cold
# restart, and compare the staged/restored files against mtools readback.
set -eu
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 TZ=UTC LC_ALL=C
out=${FINDER_VOLUME_TRASH_OUT:-$(mktemp -d "$PWD/build/finder-trash.XXXXXX")}
mkdir -p "$out"
fail() { echo "FAIL Finder volume Trash: $* ($out)" >&2; exit 1; }
. factory/desktop-trace.sh

printf 'Bytes from the C: source\r\n' > "$out/c.txt"
printf 'Independent A: source bytes\r\n' > "$out/a.txt"
printf 'Purge this file\r\n' > "$out/left.txt"

run() {
 boot=$1
 set --
 if [ "$layout" = ca ]; then set -- --disk2 "$data"; fi
 if [ "$emulator" = bochs ]; then
  driver=build/factory/desktop-bochs
  set -- "$@" --keys "$trace" --rtc-time0 699440400
 else
  driver=build/factory/qemu_harness
  set -- "$@" --mouse "$trace" --boot-rw
 fi
 export DESKTOP_SERIAL="$out/$name.serial"
 "$driver" --disk "$boot" "$@" --keys-after SHELL-READY --timeout-ms 90000 \
  --expect FLAIR-EXIT-TO-DOS --quit-after FLAIR-EXIT-TO-DOS --name "$name" --out "$out" \
  > "$out/$name.stdout" 2> "$out/$name.report" || fail "$name journey"
 grep -F 'triple_fault=0' "$out/$name.report" >/dev/null || fail "$name CPU fault verdict"
 sort "$markers" | uniq -c | while read -r count marker; do
  seen=$(grep -Fc "$marker" "$out/$name.serial" || :)
  [ "$seen" -ge "$count" ] || fail "$name missing $marker ($seen/$count)"
 done
 if [ "$emulator" = bochs ]; then
  boot=$(sed -n 's/^\[bochs\] private_boot=//p' "$out/$name.report")
  [ -f "$boot" ] || fail "$name missing retained boot clone"
 fi
}

put_fixture() {
 mmd -i "$1" "::$2"
 mcopy -i "$1" "$3" "::$2/SAME.TXT"
}
read_back() {
 mcopy -o -i "$1" "::$2" "$out/readback" || fail "$name missing $2"
 cmp "$3" "$out/readback" || fail "$name wrong bytes in $2"
}
absent() {
 if mdir -i "$1" "::$2" >/dev/null 2>&1; then fail "$name unexpected $2 on $1"; fi
}
# Verify the on-disk kind-5 record independently of the Finder DB codec.
origin() {
 cluster=$(mshowfat -i "$1" "::$2" | sed -n 's/.*<\([0-9][0-9]*\)>.*/\1/p')
 [ -n "$cluster" ] || fail "$name no cluster for $2"
 mcopy -o -i "$1" ::DESKTOP.DB "$out/readback.db" || fail "$name missing Desktop DB"
 od -An -tu1 -v "$out/readback.db" | awk -v cl="$cluster" '
  {for(i=1;i<=NF;i++) b[n++]=$i}
  END {
   for(r=8;r+24<=n;r+=24)
    if(b[r]==5 && b[r+2]+256*b[r+3]==cl &&
       b[r+4]==83 && b[r+5]==65 && b[r+6]==77 && b[r+7]==69 &&
       b[r+8]==46 && b[r+9]==84 && b[r+10]==88 && b[r+11]==84 && b[r+12]==0) found++
   exit found!=1
  }' || fail "$name wrong origin for $2"
}
enter_flair() {
 inject 'kf,kl,ka,ki,kr,kret'
 await_marker FLAIR-LIVE-READY
}
drop_on_trash() {
 # The newly opened folder is slot 1, first icon at (94,86). Its
 # center is (110,102), and the desktop Trash center is (600,420).
 inject l1; move_by 490 318; inject l0
 await_marker 'FINDER-TRASH name=SAME.TXT'
}
return_to_dos() {
 inject "$exit_desktop"; await_marker FINDER-EXIT-DOS-ALERT; inject kret
}

for emulator in qemu bochs; do
 for layout in ca c; do
  base=$emulator-$layout
  seed=$out/$base-seed.img
  cp build/image/c.img "$seed"
  put_fixture "$seed@@32256" ZZZC "$out/c.txt"
  if [ "$layout" = ca ]; then
   data=$out/$base-stage-data.img
   cp build/image/a-data.img "$data"
   put_fixture "$data" ZZZADATA "$out/a.txt"
  fi
  name=$base-stage; markers=$out/$name.markers; : > "$markers"
  trace=$(
   enter_flair
   open_volume
   inject 'kup,kz,kctrl-o'; await_marker 'FINDER-OPEN-FOLDER name=ZZZC'
   move_by -490 58
   drop_on_trash
   if [ "$layout" = ca ]; then
    inject kctrl-w; await_marker 'FINDER-CLOSE-WINDOW win=1'
    inject kctrl-w; await_marker 'FINDER-CLOSE-WINDOW win=0'
    move_by 0 -312
    inject 'l1,l0,kctrl-o'; await_marker FINDER-OPEN-VOLUME
    inject 'kup,kz,kctrl-o'; await_marker 'FINDER-OPEN-FOLDER name=ZZZADATA'
    move_by -490 -6
    drop_on_trash
   fi
   move_by -488 -318
   return_to_dos
  )
  trace=${trace%,}
  run "$seed"
  read_back "$boot@@32256" TRASH/SAME.TXT "$out/c.txt"
  absent "$boot@@32256" ZZZC/SAME.TXT
  origin "$boot@@32256" ZZZC
  restart=$out/$base-restart.img
  cp "$boot" "$restart"
  mcopy -i "$restart@@32256" "$out/left.txt" ::TRASH/LEFT.TXT
  if [ "$layout" = ca ]; then
   read_back "$data" TRASH/SAME.TXT "$out/a.txt"
   absent "$data" ZZZADATA/SAME.TXT
   origin "$data" ZZZADATA
   next_data=$out/$base-restart-data.img
   cp "$data" "$next_data"
   data=$next_data
   mcopy -i "$data" "$out/left.txt" ::TRASH/LEFT.TXT
  fi

  name=$base-restore; markers=$out/$name.markers; : > "$markers"
  trace=$(
   enter_flair
   move_by 280 180
   inject 'l1,l0,kctrl-o'; await_marker 'FINDER-OPEN-TRASH win=0'
   # Rebooted Trash lists C: before A:, two icons per row. Restore A:
   # first to catch name-only origin lookup when C: has the same name.
   if [ "$layout" = ca ]; then
    move_by -510 -286; inject 'l1,l0,kctrl-y'; await_marker 'FINDER-PUT-AWAY n=1'
    move_by 0 -52
   else
    move_by -510 -338
   fi
   inject 'l1,l0,kctrl-y'; await_marker 'FINDER-PUT-AWAY n=1'
   move_by 80 -72
   inject 'l1,m0:32,l0'; await_marker FINDER-TRASH-ALERT; inject kret
   if [ "$layout" = ca ]; then count=2; else count=1; fi
   await_marker "FINDER-TRASH-EMPTIED purged=$count refused=0"
   move_by -58 60
   return_to_dos
  )
  trace=${trace%,}
  run "$restart"
  read_back "$boot@@32256" ZZZC/SAME.TXT "$out/c.txt"
  absent "$boot@@32256" TRASH/SAME.TXT
  absent "$boot@@32256" TRASH/LEFT.TXT
  if [ "$layout" = ca ]; then
   read_back "$data" ZZZADATA/SAME.TXT "$out/a.txt"
   absent "$data" TRASH/SAME.TXT
   absent "$data" TRASH/LEFT.TXT
  fi
  echo "ok Finder Trash $base: cold Put Away and Empty Trash ($out)"
 done
done
