#!/bin/sh
# One honest QMP medium replacement attempt, fixed ATA --disk2 hardware.
# A rejected replacement is RED-named as permitted by the PM second launch.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
b=$1; boot=$2; w=$b/diskcopy-emu
mkdir -p "$w"
cp -f "$boot" "$w/master.img"
dd if=/dev/zero of="$w/source.img" bs=512 count=2880 status=none
mformat -i "$w/source.img" -f 1440 -N 0x494e4954 ::
mcopy -i "$w/source.img" "$b/diskcopy_program.bin" ::DISKCOPY.COM
printf '@ECHO OFF\r\nDISKCOPY A: A:\r\nECHO COPY-RETURN\r\n' | mcopy -i "$w/source.img" - ::AUTOEXEC.BAT
cp -f "$w/source.img" "$w/target.img"
# W is the existing marker event. S is the new explicit disk2 medium event.
"$b/qemu_harness" --disk "$w/master.img" --disk2 "$w/source.img" --name swap --out "$w" \
 --keys-after 'Insert SOURCE diskette' --mouse "kret,WInsert TARGET diskette,S$PWD/$w/target.img,kret" \
 --quit-after COPY-RETURN --timeout-ms 25000 > "$w/stdout" 2> "$w/report" || true
if grep -q 'DISK2-SWAP-RED' "$w/report"; then
 grep 'DISK2-SWAP-RED' "$w/report"
 echo 'RED diskcopy-emu: QMP cannot replace the fixed ATA primary-slave medium; host single-drive logic gate remains authoritative'
 exit 1
fi
grep -q 'DISK2-SWAP-GREEN' "$w/report" || { echo 'RED diskcopy-emu: injected medium event did not run'; exit 1; }
# A successful first swap alone cannot certify a whole disk. The multi-buffer
# replay must alternate media for every group and prove byte equality.
echo 'RED diskcopy-emu: full alternating-media replay not certified'
exit 1
