#!/bin/sh
# initech-s25. Locked injected trace: COMMAND.COM -> FAT PANIC.COM -> #UD/#GP.
# The capture is graded only after HALTED; timeout/boot failure cannot kill a
# mutant. mtools reads the SAME guest-written FAT image; dosfstools checks it.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
build=$1; image=$2; probe=$3; tag=$4; w=$5; h=$6; vec=$7; debug=$8
work=$build/panic-screen
mkdir -p "$work"
data=$work/$tag-data.img
dd if=/dev/zero of="$data" bs=512 count=2880 status=none
mformat -i "$data" -f 1440 ::
mcopy -i "$data" "$probe" ::PANIC.COM
printf '@ECHO OFF\r\n' > "$work/$tag.bat"
mcopy -i "$data" "$work/$tag.bat" ::AUTOEXEC.BAT
mtype -i "$data" ::PANIC.COM | cmp - "$probe"
"$build/qemu_harness" --disk "$image" --disk2 "$data" --out "$work" \
    --name "$tag" --timeout-ms 15000 --keys 'p,a,n,i,c,ret' \
    --keys-after SHELL-READY --expect HALTED --quit-after HALTED \
    --screendump --screendump-after HALTED > "$work/$tag.stdout" 2> "$work/$tag.report"
grep -q 'triple_fault=0' "$work/$tag.report"
grep -q 'guest_errors=0' "$work/$tag.report"
tr -d '\r' < "$work/$tag.serial" > "$work/$tag.norm"
s=$work/$tag.norm
grep -qx 'PANIC-PROBE-ARMED' "$s"
hex=$(printf '%02X' "$vec")
grep -q "^PANIC vec=$hex err=" "$s"
grep -qx HALTED "$s"
test "$(grep -cx KERNEL "$s")" -eq 1
if [ "$debug" = 1 ]; then
    grep -Eq '^  eip=[0-9A-F]{8} cs=0008 eflags=[0-9A-F]{8}$' "$s"
    grep -q ' ebx=23456789 ecx=3456789A edx=456789AB$' "$s"
    grep -Eq '^  esi=[0-9A-F]{8} edi=[0-9A-F]{8} ebp=[0-9A-F]{8}$' "$s"
else
    if grep -Eq '^  (eip=|eax=|esi=|#)' "$s"; then
        echo 'RED panic-serial: debug-disabled register dump leaked'; exit 1
    fi
fi
mcopy -i "$data" ::FONT.BIN "$work/$tag.font"
mtype -i "$data" ::FONT.BIN | cmp - "$work/$tag.font"
fsck.fat -n "$data" > "$work/$tag.fsck"
echo "GREEN panic-serial $tag: real vector $vec, PANIC + HALTED, debug=$debug, no reboot; mtools/dosfstools same image"
"$build/ppm_panic_check" "$work/$tag.ppm" "$work/$tag.font" "$w" "$h" "$vec"
