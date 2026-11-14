#!/bin/sh
# Exact QEMU Ctrl-Pause chord used by the independent DOS reference driver.
set -eu
root=$(pwd)
d=$root/build/dos-compat33/break-product
mkdir -p "$d"
truncate -s 0 "$d/data.img"
truncate -s 1474560 "$d/data.img"
mformat -i "$d/data.img" -f 1440 ::
printf '@echo off\r\npause\r\necho AFTER > AFTER.OUT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
mcopy -i "$d/data.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
rm -f "$d/monitor"
mkfifo "$d/monitor"
exec 3<>"$d/monitor"
qemu-system-i386 -cpu 486 -m 8 -machine acpi=off -display none -nic none \
 -drive "file=$root/build/image/boot.img,format=raw,if=ide,index=0,snapshot=on" \
 -drive "file=$d/data.img,format=raw,if=ide,index=1" \
 -serial "file:$d/result.serial" -monitor stdio -no-reboot \
 -rtc base=1992-03-02T09:30:00,clock=vm \
 -d int,guest_errors,cpu_reset -D "$d/qemu.log" <&3 > "$d/monitor.log" 2>&1 &
qemu_pid=$!
trap 'kill "$qemu_pid" 2>/dev/null || true; rm -f "$d/monitor"' EXIT HUP INT TERM
i=0
while ! test -f "$d/result.serial" || ! grep -q 'Strike a key' "$d/result.serial"; do
 i=$((i+1)); [ "$i" -lt 200 ] || exit 2
 sleep 0.1
done
printf 'sendkey ctrl-pause\n' >&3
sleep 1
cp "$d/result.serial" "$d/after-break.serial"
printf 'sendkey y\n' >&3
sleep 1
cp "$d/result.serial" "$d/after-y.serial"
printf 'quit\n' >&3
wait "$qemu_pid"
trap - EXIT HUP INT TERM
rm -f "$d/monitor"
mcopy -i "$d/data.img" ::AFTER.OUT "$d/AFTER.OUT" 2>/dev/null || true
