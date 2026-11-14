#!/bin/sh
# Same physical QEMU Ctrl-Pause chord as the genuine DOS3.30 BREAK capture.
set -eu
root=$(cd "$1" && pwd)
mkdir -p "$2"
out=$(mktemp -d "$2/run.XXXXXX")
cd "$root"
export LC_ALL=C TZ=UTC
question=$(sed -n '/Terminate batch job (Y\/N)?/{s/[[:space:]]*$//;p;}' dos/command/tests/compat33/expected/BREAK/question.txt)
test -n "$question"
ctrl_break_pid=
cleanup() {
 if [ -n "$ctrl_break_pid" ]; then
  kill "$ctrl_break_pid" 2>/dev/null || :
  wait "$ctrl_break_pid" 2>/dev/null || :
 fi
}
trap cleanup 0
trap 'exit 1' HUP INT TERM
wait_for() {
 tries=0
 while ! grep -Fq "$1" "$dir/result.serial" 2>/dev/null; do
  kill -0 "$ctrl_break_pid" 2>/dev/null || { echo 'QEMU stopped before marker'; exit 1; }
  tries=$((tries+1)); test "$tries" -lt 200 || { echo "Missing marker: $1"; exit 1; }
  sleep 0.1
 done
}
printf 'Ctrl-Break evidence: %s\n' "$out"
for scenario in break continue; do
 dir=$out/$scenario
 mkdir -p "$dir"
 truncate -s 1474560 "$dir/data.img"
 mformat -i "$dir/data.img" -f 1440 ::
 printf '@echo off\r\npause\r\necho AFTER>AFTER.OUT\r\necho CAPTURE-DONE\r\n' > "$dir/AUTOEXEC.BAT"
 mcopy -i "$dir/data.img" "$dir/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 cp "$dir/data.img" "$dir/before.img"
 mkfifo "$dir/monitor"
 exec 3<>"$dir/monitor"
 qemu-system-i386 -cpu 486 -m 8 -machine acpi=off -display none -nic none \
  -drive "file=$root/build/image/boot.img,format=raw,if=ide,index=0,snapshot=on" \
  -drive "file=$dir/data.img,format=raw,if=ide,index=1" \
  -serial "file:$dir/result.serial" -monitor stdio -no-reboot \
  -rtc base=1992-03-02T09:30:00,clock=vm \
  -d int,guest_errors,cpu_reset -D "$dir/qemu.log" <&3 > "$dir/monitor.log" 2>&1 &
 ctrl_break_pid=$!
 wait_for 'Strike a key'
 if [ "$scenario" = break ]; then
  printf 'sendkey ctrl-pause\n' >&3
  wait_for "$question"
  grep -F '^C' "$dir/result.serial" >/dev/null
  sleep 0.2
 fi
 printf 'sendkey y\n' >&3
 if [ "$scenario" = break ]; then wait_for 'A:\>'; else wait_for 'CAPTURE-DONE'; fi
 printf 'quit\n' >&3
 wait "$ctrl_break_pid"
 ctrl_break_pid=
 exec 3>&-
 rm "$dir/monitor"
 if [ "$scenario" = break ]; then
  if mdir -i "$dir/data.img" ::AFTER.OUT >/dev/null 2>&1; then exit 1; fi
  if grep -F 'CAPTURE-DONE' "$dir/result.serial"; then exit 1; fi
  cmp "$dir/before.img" "$dir/data.img"
 else
  mcopy -i "$dir/data.img" ::AFTER.OUT "$dir/after.txt"
  printf 'AFTER\r\n' > "$dir/expected.txt"
  cmp "$dir/after.txt" "$dir/expected.txt"
 fi
 printf 'ok physical keyboard %s: native prompt and independent disk outcome\n' "$scenario"
done
