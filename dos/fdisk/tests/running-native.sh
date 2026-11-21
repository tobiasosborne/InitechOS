#!/bin/sh
# Replay against the staged C: system after C: startup lands (initech-5o7p3).
# Run from the product root after make image. Use a short, fresh output path.
set -eu
cpu=$1 mode=$2 out=$3
case $cpu in qemu|bochs) ;; *) exit 2;; esac
case $mode in alone|with-data) ;; *) exit 2;; esac
mkdir "$out"
out=$(realpath "$out")
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 LC_ALL=C
fail() { echo "FAIL FDISK $cpu $mode: $*" >&2; exit 1; }

{
    printf '@ECHO OFF\r\nECHO FDISK-C-START\r\n'
    printf 'IF NOT EXIST C:\\FDISK.COM GOTO FAIL\r\n'
    if [ "$mode" = with-data ]; then
        printf 'A:\r\nIF NOT EXIST FDISKA.TXT GOTO FAIL\r\n'
    fi
    printf 'C:\\FDISK.COM\r\nIF ERRORLEVEL 2 GOTO FAIL\r\nIF NOT ERRORLEVEL 1 GOTO FAIL\r\n'
    printf 'ECHO FDISK-REFUSED\r\nGOTO DONE\r\n:FAIL\r\nECHO FDISK-WRONG-STATUS\r\n'
    printf ':DONE\r\nECHO FDISK-NATIVE-DONE\r\n'
} > "$out/C-AUTOEXEC.BAT"
cp build/image/c.img "$out/boot.img"
mcopy -o -i "$out/boot.img@@32256" "$out/C-AUTOEXEC.BAT" ::AUTOEXEC.BAT
cp "$out/boot.img" "$out/before-c.img"
set -- --disk "$out/boot.img"
if [ "$mode" = with-data ]; then
    cp build/image/a-data.img "$out/data.img"
    printf 'Independent A volume\r\n' > "$out/FDISKA.TXT"
    printf '@ECHO OFF\r\nECHO FDISK-WRONG-AUTOEXEC\r\nECHO FDISK-NATIVE-DONE\r\n' > "$out/A-AUTOEXEC.BAT"
    mcopy -o -i "$out/data.img" "$out/A-AUTOEXEC.BAT" ::AUTOEXEC.BAT
    mcopy -o -i "$out/data.img" "$out/FDISKA.TXT" ::FDISKA.TXT
    cp "$out/data.img" "$out/before-a.img"
    set -- "$@" --disk2 "$out/data.img"
fi
if [ "$cpu" = qemu ]; then set -- "$@" --boot-rw; fi

# The guard exits without a menu. A missing guard waits for input, so the
# batch completion marker never arrives.
rc=0
"build/factory/${cpu}_harness" "$@" --out "$out" --name fdisk \
    --expect FDISK-NATIVE-DONE --quit-after FDISK-NATIVE-DONE \
    --timeout-ms "${FDISK_NATIVE_TIMEOUT_MS:-30000}" \
    > "$out/stdout" 2> "$out/report" || rc=$?
actual=$out/boot.img
if [ "$cpu" = bochs ]; then
    actual=$(sed -n 's/^\[bochs\] private_boot=//p' "$out/report")
    [ -f "$actual" ] || fail 'missing Bochs writable boot copy'
fi
cmp "$out/before-c.img" "$actual" || fail 'whole physical C disk changed'
if [ "$mode" = with-data ]; then
    cmp "$out/before-a.img" "$out/data.img" || fail 'whole physical A disk changed'
fi
[ "$rc" = 0 ] || fail "session did not complete (harness exit $rc)"
tr -d '\r' < "$out/fdisk.serial" > "$out/text"
grep -Fx 'Directory of C:\' "$out/text" >/dev/null || fail 'C startup missing'
grep -Fx 'FDISK-C-START' "$out/text" >/dev/null || fail 'C AUTOEXEC missing'
grep -Fx 'FDISK-REFUSED' "$out/text" >/dev/null || fail 'FDISK did not return status 1'
if grep -E 'FDISK-WRONG-|Disk Options|Bad command or file name|PANIC' "$out/text"; then
    fail 'unexpected guest path'
fi
echo "GREEN FDISK $cpu $mode: running C refused, entire attached disks unchanged"
