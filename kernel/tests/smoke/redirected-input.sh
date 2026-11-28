#!/bin/sh
# DATE and PAUSE with redirected standard input finish without a keystroke,
# as in the DOS 3.30 clock-input and pause-input captures
# (kernel/tests/fixtures/stdin33/README.md). No key is ever sent.
set -eu
root=$(cd "$1" && pwd)
mkdir -p "$2"
dir=$(mktemp -d "$2/run.XXXXXX")
cd "$root"
export LC_ALL=C TZ=UTC
truncate -s 1474560 "$dir/data.img"
mformat -i "$dir/data.img" -f 1440 ::
printf '@echo off\r\ndate 1-2-92\r\ndate <BLANK.IN\r\npause <KEY.IN >P2.OUT\r\necho CAPTURE-DONE\r\n' > "$dir/AUTOEXEC.BAT"
printf '\r\n' > "$dir/BLANK.IN"
printf 'y\r\n' > "$dir/KEY.IN"
mcopy -i "$dir/data.img" "$dir/AUTOEXEC.BAT" "$dir/BLANK.IN" "$dir/KEY.IN" ::
rc=0
timeout 60 qemu-system-i386 -cpu 486 -m 8 -machine acpi=off -display none -nic none \
 -drive "file=$root/build/image/boot.img,format=raw,if=ide,index=0,snapshot=on" \
 -drive "file=$dir/data.img,format=raw,if=ide,index=1" \
 -serial "file:$dir/result.serial" -monitor none -no-reboot \
 -rtc base=1992-03-02T09:30:00,clock=vm \
 -d int,guest_errors,cpu_reset -D "$dir/qemu.log" > "$dir/qemu.out" 2>&1 &
pid=$!
tries=0
while ! grep -Fq CAPTURE-DONE "$dir/result.serial" 2>/dev/null; do
 kill -0 "$pid" 2>/dev/null || { echo "QEMU stopped before CAPTURE-DONE: $dir"; exit 1; }
 tries=$((tries+1))
 test "$tries" -lt 400 || { kill "$pid"; echo "DATE or PAUSE waited for the keyboard: $dir"; exit 1; }
 sleep 0.1
done
kill "$pid" 2>/dev/null || :
wait "$pid" 2>/dev/null || :
grep -F 'Current date is Thu  1-02-1992' "$dir/result.serial" >/dev/null
mcopy -i "$dir/data.img" ::P2.OUT "$dir/p2.out"
grep -F 'Strike a key when ready . . .' "$dir/p2.out" >/dev/null
printf 'ok redirected DATE and PAUSE input: %s\n' "$dir"
