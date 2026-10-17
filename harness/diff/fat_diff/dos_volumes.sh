#!/bin/sh
# Two independent reference images: C has an MBR, A is the DEC-18 slave.
# MS-DOS 3.3 UG pp24-26; Reference pp4-11,41-42,50,57-58,340.
set -eu
export INITECH_QMP_STDIO=1 SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
mode=$1; boot=$2; tag=${3:-green}
w=build/dos-volumes/$mode-$tag
mkdir -p "$w"
red() { echo "RED dos-volumes $*"; exit 1; }
cp -f "$boot" "$w/c.img"
dd if=/dev/zero of="$w/a.img" bs=512 count=2880 status=none
mformat -i "$w/a.img" -f 1440 -N 0x19870330 -v USERDATA ::
printf 'user drive bytes\r\n' > "$w/user.txt"
awk 'BEGIN { for(i=0;i<120;i++) printf "system drive bytes %03d\r\n",i }' > "$w/system.txt"
mmd -i "$w/a.img" ::USER
mmd -i "$w/c.img@@32256" ::SYS
mcopy -i "$w/a.img" "$w/user.txt" ::USER/SAME.TXT
mcopy -i "$w/c.img@@32256" "$w/system.txt" ::SYS/SAME.TXT
mcopy -i "$w/c.img@@32256" "$w/system.txt" ::SYS/TWO.TXT
mtype -i "$w/a.img" ::USER/SAME.TXT > "$w/a.ref"
mtype -i "$w/c.img@@32256" ::SYS/SAME.TXT > "$w/c.ref"
if [ "$mode" = host ]; then
    "${DOS_VOLUMES_HOST:-build/dos_volumes_host}" "$w/a.img" "$w/c.img" "$w/a.ref" "$w/c.ref"
    mtype -i "$w/a.img" ::COPY.TXT > "$w/copy"
    cmp "$w/copy" "$w/c.ref" || red 'COPY vs mcopy'
else
    if [ "$mode" = emu ]; then
        mcopy -o -i "$w/c.img@@32256" build/transient_flairtenants.bin ::COMMAND.COM
    fi
    if [ "$mode" = fat16 ]; then
        dd if=/dev/zero of="$w/a.img" bs=512 count=16384 status=none
        mkfs.fat --invariant -F 16 -s 1 "$w/a.img" > "$w/mkfs.log"
        # A chain that crosses FAT-sector boundaries, generated without a
        # second factory language and compared in full after TYPE redirection.
        awk 'BEGIN { for(i=0;i<1400;i++) printf "FAT16 ATA line %04d\r\n",i }' > "$w/long.txt"
        mcopy -i "$w/a.img" "$w/long.txt" ::CHAIN.TXT
        # FAT16 is read-only: output goes to writable C via a retained handle.
        printf '@ECHO OFF\r\nDIR A:\\ > C:\\LIST.OUT\r\nTYPE A:\\CHAIN.TXT > C:\\READ.OUT\r\nECHO VOLUMES-DONE\r\n' > "$w/autoexec"
        expect=C
    else
        printf '@ECHO ON\r\nC:\r\nDIR\r\nCD \\SYS\r\nA:\r\nCD \\USER\r\nCOPY C:\\COMMAND.COM A:\\\r\nCOPY C:SAME.TXT A:\\COPIED.TXT\r\nTYPE A:\\AUTOEXEC.BAT > A:\\TYPE.OUT\r\nDIR C:\\SYS > A:\\LIST.OUT\r\nCD\r\nB:\r\nD:\r\nDIR B:\r\nCD D:\\\r\nTYPE B:\\ABSENT.TXT\r\nCOPY C:\\COMMAND.COM D:\\ABSENT.COM\r\nECHO VOLUMES-DONE\r\n' > "$w/autoexec"
        expect=A
    fi
    mcopy -o -i "$w/a.img" "$w/autoexec" ::AUTOEXEC.BAT
    build/qemu_harness --disk "$w/c.img" --disk2 "$w/a.img" --name run --out "$w" \
        --timeout-ms 30000 --keys 'e,x,i,t,ret' --keys-after VOLUMES-DONE \
        --quit-after SHELL-DONE 2> "$w/report" || true
    grep -qx SHELL-EXIT "$w/run.serial" || red 'EMU did not finish shell'
    if grep -q 'triple_fault=1' "$w/report"; then red 'EMU triple fault'; fi
    if [ "$mode" = fat16 ]; then
        mtype -i "$w/c.img@@32256" ::READ.OUT > "$w/read"
        cmp "$w/read" "$w/long.txt" || red 'FAT16 ATA TYPE bytes'
        mtype -i "$w/c.img@@32256" ::LIST.OUT | grep -q 'CHAIN' || red 'FAT16 ATA DIR'
    else
        grep -Fq 'C:\>' "$w/run.serial" || red 'PROMPT current C'
        grep -Fq 'A:\USER>' "$w/run.serial" || red 'CWD A prompt'
        test "$(grep -c 'Invalid drive specification' "$w/run.serial")" -eq 6 || red 'INVALID B/D message'
        mtype -i "$w/c.img@@32256" ::COMMAND.COM > "$w/command.ref"
        mtype -i "$w/a.img" ::COMMAND.COM > "$w/command.got"
        cmp "$w/command.ref" "$w/command.got" || red 'HANDLE cross-drive COPY COMMAND.COM'
        mtype -i "$w/a.img" ::COPIED.TXT > "$w/copied"
        cmp "$w/copied" "$w/c.ref" || red 'PATH drive-relative C'
        mtype -i "$w/a.img" ::TYPE.OUT > "$w/type"
        cmp "$w/type" "$w/autoexec" || red 'TYPE absolute A'
        mtype -i "$w/a.img" ::LIST.OUT | grep -Fq 'Directory of C:\SYS' || red 'DIR explicit C'
    fi
fi
mdir -i "$w/a.img" :: > "$w/mdir-a"
mdir -i "$w/c.img@@32256" :: > "$w/mdir-c"
dd if="$w/c.img" of="$w/c-volume.img" bs=512 skip=63 count=2880 status=none
fsck.fat -n "$w/a.img" > "$w/fsck-a" 2>&1 || red 'FSCK A'
fsck.fat -n "$w/c-volume.img" > "$w/fsck-c" 2>&1 || red 'FSCK C'
echo "GREEN test-dos-volumes $mode: real FAT/ATA, mtype bytes, mdir both images, fsck.fat clean"
