#!/bin/sh
# initech-q45q: drive operands of CD and the external commands on the DEC-18
# two-volume boot disk (C = BIOS 80h primary 1, D = primary 2, A = slave).
# RED reproduction of audit 2026-10-23-dos L011-L015, L028, L032.
# Ground truth: MS-DOS 3.3 User's Guide/Reference (initech-manuals-ref/dos33):
#   Reference p.33 (PDF118) ATTRIB [drive:]pathname; p.42 (PDF127) CHDIR d:
#   displays that drive's directory; p.43 (PDF128) CHKDSK [drive:];
#   p.86 (PDF171) LABEL [drive:][label]; p.122 (PDF207) TREE [drive:];
#   p.127 (PDF212) XCOPY: "The second [set] names the target".
#   ADR-0003 DEC-18 + spec/drive_letters.json: the letter <-> partition map.
# Independent judges: mtools (mtype/mattrib/mlabel/mdir) and fsck.fat on each
# volume, never the artifact. Only SCRATCH copies of the images are touched.
set -eu
export INITECH_QMP_STDIO=1 SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
mode=$1; boot=$2; tag=$3
xcopy=$4; tree=$5; attrib=$6; label=$7; chkdsk=$8; fdisk=$9
w=build/drive-operands/$mode-$tag
mkdir -p "$w"
# Every finding is graded on every run (mutants grep their own RED line).
failed=0
red() { echo "RED drive-operands $*"; failed=1; }
die() { echo "RED drive-operands $*"; exit 1; }
D_BASE=4096; D_SIZE=2880
# --- C/D boot disk: the shipped tracer C volume + a second primary FAT12 ---
cp -f "$boot" "$w/c.img"
truncate -s $((8192*512)) "$w/c.img"
dd if=/dev/zero of="$w/d.img" bs=512 count=$D_SIZE status=none
mformat -i "$w/d.img" -t 80 -h 2 -s 18 -m 248 -H $D_BASE -N 0x19870331 -v EXTRA ::
# MBR entry 2 (IBM DOS 3.30 TR pp9-6..9): type 01, LBA 4096, 2880 sectors,
# CHS for the 16-head/63-sector translation (start 4/1/2, end 6/14/46).
printf '\000\001\002\004\001\016\056\006\000\020\000\000\100\013\000\000' |
    dd of="$w/c.img" bs=1 seek=462 conv=notrunc status=none
C="$w/c.img@@32256"
D="$w/c.img@@$((D_BASE*512))"
printf 'D-DRIVE\r\n' > "$w/d-same.txt"
mmd -i "$w/d.img" ::DHOME
mcopy -i "$w/d.img" "$w/d-same.txt" ::DHOME/SAME.TXT
dd if="$w/d.img" of="$w/c.img" bs=512 seek=$D_BASE conv=notrunc status=none
printf 'C-DRIVE\r\n' > "$w/c-same.txt"
mmd -i "$C" ::CHOME
mcopy -i "$C" "$w/c-same.txt" ::CHOME/SAME.TXT
mcopy -o -i "$C" build/transient_flairtenants.bin ::COMMAND.COM
# --- A: the data disk carrying the externals and the trace ---
dd if=/dev/zero of="$w/a.img" bs=512 count=2880 status=none
mformat -i "$w/a.img" -f 1440 -N 0x19870330 -v USERDATA ::
mmd -i "$w/a.img" ::DOS ::T ::DHOME
for pair in "$xcopy:XCOPY" "$tree:TREE" "$attrib:ATTRIB" "$label:LABEL" "$chkdsk:CHKDSK" "$fdisk:FDISK"; do
    mcopy -i "$w/a.img" "${pair%%:*}" "::DOS/${pair##*:}.COM"
done
printf 'ALPHA\r\n' > "$w/one.txt"
printf 'KEEP-A\r\n' > "$w/keep.txt"
mcopy -i "$w/a.img" "$w/one.txt" ::T/ONE.TXT
mcopy -i "$w/a.img" "$w/keep.txt" ::DHOME/ONE.TXT
run() {
    mcopy -o -i "$w/a.img" "$w/$1.bat" ::AUTOEXEC.BAT
    build/qemu_harness --disk "$w/c.img" --disk2 "$w/a.img" --name "$1" --out "$w" \
        --timeout-ms 30000 --keys "$2" --keys-after "$3" \
        --quit-after "$4" 2> "$w/$1.report" || true
    if grep -q 'triple_fault=1' "$w/$1.report"; then die "$1 triple fault"; fi
    grep -q 'DOS-VOLUME-OK D' "$w/$1.serial" || die "$1 fixture: D was not mounted"
    tr -d '\r' < "$w/$1.serial" > "$w/$1.txt"
}
if [ "$mode" = fdisk ]; then
    printf '@ECHO OFF\r\nPATH=A:\\DOS\r\nFDISK\r\nECHO FDISK-RETURN\r\n' > "$w/fdisk.bat"
    run fdisk '4,ret' 'Enter choice:' 'Press ESC to return to Fdisk Options'
    # L028: DEC-18 C = 80h primary 1, D = 80h primary 2 (spec/drive_letters.json).
    grep -Eq '^ C:  1  ' "$w/fdisk.txt" || red 'L028 FDISK first primary not C:'
    grep -Eq '^ D:  2  ' "$w/fdisk.txt" || red 'L028 FDISK second primary not D:'
    [ $failed = 0 ] || exit 1
    echo "GREEN test-drive-operands fdisk: partition rows C: 1 and D: 2 per DEC-18"
    exit 0
fi
# L011 CD d:, L032 XCOPY, L014 TREE, L012 ATTRIB.
printf '%s\r\n' '@ECHO ON' 'PATH=A:\DOS' 'CD \T' 'CD C:\CHOME' 'CD D:\DHOME' \
    'CD C:' 'CD D:' 'CD' \
    'XCOPY A:\T\ONE.TXT D:\DHOME\ /V' 'XCOPY D:SAME.TXT A:\FROMD\' \
    'XCOPY C:\CHOME\SAME.TXT D:\FROMC\' \
    'TREE D: /F > A:\TREED.TXT' 'TREE D:\DHOME /F > A:\TREEDH.TXT' \
    'ATTRIB +R D:\DHOME\SAME.TXT' 'ATTRIB D:SAME.TXT > A:\ATTRD.TXT' \
    'ECHO OPERANDS-DONE' > "$w/ops.bat"
run ops 'e,x,i,t,ret' OPERANDS-DONE SHELL-DONE
grep -qx SHELL-EXIT "$w/ops.txt" || die 'EMU did not finish shell'
# L011: CHDIR d: displays that drive's directory, the default stays A:\T.
grep -qx 'C:\\CHOME' "$w/ops.txt" || red 'L011 CD C: did not display C:\CHOME'
grep -qx 'D:\\DHOME' "$w/ops.txt" || red 'L011 CD D: did not display D:\DHOME'
grep -qx 'A:\\T' "$w/ops.txt" || red 'L011 CD default drive A:\T'
# L032: the D: target receives the copy; A:\DHOME's sentinel is untouched.
mtype -i "$D" ::DHOME/ONE.TXT > "$w/got" 2>/dev/null || red 'L032 XCOPY wrote nothing to D:\DHOME'
cmp -s "$w/got" "$w/one.txt" || red 'L032 XCOPY D: target bytes'
mtype -i "$w/a.img" ::DHOME/ONE.TXT > "$w/got"
cmp -s "$w/got" "$w/keep.txt" || red 'L032 XCOPY overwrote A:\DHOME\ONE.TXT'
mtype -i "$w/a.img" ::FROMD/SAME.TXT > "$w/got" 2>/dev/null || red 'L032 XCOPY drive-relative D: source'
cmp -s "$w/got" "$w/d-same.txt" || red 'L032 XCOPY D: source bytes'
mtype -i "$D" ::FROMC/SAME.TXT > "$w/got" 2>/dev/null || red 'L032 XCOPY C: source to D: target'
cmp -s "$w/got" "$w/c-same.txt" || red 'L032 XCOPY C: to D: bytes'
if mdir -i "$C" ::FROMC >/dev/null 2>&1; then red 'L032 XCOPY C: to D: wrote to C:'; fi
# L014: TREE d: walks d:'s root and names d:'s volume (mdir inventory golden).
# Directories/files created after TREE: none (LABEL's entry is not listed).
mdir -/ -b -i "$D" :: > "$w/d.list"
build/test_xcopy_tree --tree-golden "$w/d.list" "" 1 "$w/treed.exp" EXTRA
build/test_xcopy_tree --tree-golden "$w/d.list" /DHOME 1 "$w/treedh.exp" EXTRA
mtype -i "$w/a.img" ::TREED.TXT > "$w/got" || red 'L014 TREE D: output missing'
cmp -s "$w/got" "$w/treed.exp" || red 'L014 TREE D: walked the wrong drive/volume'
mtype -i "$w/a.img" ::TREEDH.TXT > "$w/got" || red 'L014 TREE D:\DHOME output missing'
cmp -s "$w/got" "$w/treedh.exp" || red 'L014 TREE D:\DHOME path'
# L012: ATTRIB sets R on D:, and displays the full drive-qualified path.
cp -f "$w/d.img" "$w/d.ref"
mattrib -i "$w/d.ref" +r ::DHOME/SAME.TXT
mattrib -i "$w/d.ref" ::DHOME/SAME.TXT > "$w/exp"
mattrib -i "$D" ::DHOME/SAME.TXT > "$w/got"
cmp -s "$w/got" "$w/exp" || red 'L012 ATTRIB +R did not reach D: (mattrib)'
mtype -i "$w/a.img" ::ATTRD.TXT > "$w/got" || red 'L012 ATTRIB output missing'
printf 'R  A     D:\\DHOME\\SAME.TXT\r\n' > "$w/exp"
cmp -s "$w/got" "$w/exp" || red 'L012 ATTRIB D: display'
dd if="$w/c.img" of="$w/c-vol.img" bs=512 skip=63 count=2880 status=none
dd if="$w/c.img" of="$w/d-vol.img" bs=512 skip=$D_BASE count=$D_SIZE status=none
for v in a c-vol d-vol; do fsck.fat -n "$w/$v.img" > "$w/fsck-$v" 2>&1 || red "FSCK $v"; done
[ $failed = 0 ] || exit 1
echo "GREEN test-drive-operands emu: CD d:, XCOPY/TREE/ATTRIB on C: and D: vs mtools; fsck.fat clean"
