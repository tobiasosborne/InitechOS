#!/bin/sh
# INT 21h AH=1Fh / AH=32h differential (WL-0167, bead initech-dpb1).
# The artifact's DPB bytes (host driver or the in-emulator program) are graded
# against an INDEPENDENT golden: minfo/mdir (mtools) of the same images, with
# the DOS 3.x field table and $SETDPB arithmetic transcribed here from
# docs/research/dpb-ground-truth.md Sec 3 (MS-DOS 2.0/4.0 source, RBIL r61
# Table 01395) -- not from spec/dos_structs.h and not from fat12.c.
# usage: int21_dpb.sh host <driver> <boot-image> [tag]
#        int21_dpb.sh emu  <boot-image> [tag]
set -eu
export LC_ALL=C TZ=UTC INITECH_QMP_STDIO=1 SOURCE_DATE_EPOCH=315532800
mode=$1
if [ "$mode" = host ]; then driver=$2; boot=$3; tag=${4:-green}
else boot=$2; tag=${3:-green}; fi
mkdir -p build/int21-dpb
w=$(mktemp -d "build/int21-dpb/$mode-$tag.XXXXXX")
red() { echo "RED int21-dpb $*"; exit 1; }

# The fixed query script: <AH><DL>:<expected drive letter, or - for AL=FFh>.
# Default drive is A for the first eight calls, C for the last two (AH=0Eh).
EXPECT='1F00:A 3200:A 3201:A 3202:- 3203:C 3204:D 3205:- 321B:- 1F00:C 3200:C'
NLINES=10

# C = the boot image's primary 1 (FAT12 @63). D = a FAT16 primary 2 appended
# after it (DEC-18: D is 80h primary 2), hidden sectors == its start (DEC-07a).
cp -f "$boot" "$w/c.img"
C="$w/c.img@@32256"
DSTART=2943 DSIZE=16384
D="$w/c.img@@$((DSTART * 512))"
truncate -s $(( (DSTART + DSIZE) * 512 )) "$w/c.img"
echo "start=$DSTART, size=$DSIZE, type=4" | sfdisk --append --no-reread --no-tell-kernel "$w/c.img" > "$w/sfdisk.log" 2>&1 || red 'sfdisk D'
dd if=/dev/zero of="$w/d.img" bs=512 count=$DSIZE status=none
mkfs.fat --invariant -F 16 -s 1 -h $DSTART "$w/d.img" > "$w/mkfs-d.log"
dd if="$w/d.img" of="$w/c.img" bs=512 seek=$DSTART conv=notrunc status=none
awk 'BEGIN { for(i=0;i<40;i++) printf "D volume filler %04d\r\n",i }' > "$w/fill-d.txt"
mcopy -i "$D" "$w/fill-d.txt" ::FILL.TXT
mkimage() { # $1 = fat16 | fat12 -> $w/a.img
    if [ "$1" = fat16 ]; then
        dd if=/dev/zero of="$w/a.img" bs=512 count=16384 status=none
        mkfs.fat --invariant -F 16 -s 1 "$w/a.img" > "$w/mkfs.log"
    else
        dd if=/dev/zero of="$w/a.img" bs=512 count=2880 status=none
        mformat -i "$w/a.img" -f 1440 -N 0x19870330 -v DPBDATA ::
    fi
    # A few clusters in use so the free count is not the whole volume.
    awk 'BEGIN { for(i=0;i<300;i++) printf "DPB free-count filler %04d\r\n",i }' > "$w/fill.txt"
    mcopy -i "$w/a.img" "$w/fill.txt" ::FILL.TXT
}

# minfo/mdir -> "<letter> drive unit bps mask shift res fats root first maxc
#                spf dir media free" for one image (the independent golden).
golden() { # $1 letter, $2 drive number, $3 mtools image spec
    minfo -i "$3" :: > "$w/minfo-$1"
    mdir -i "$3" :: > "$w/mdir-$1"
    awk -v L="$1" -v D="$2" -v MDIR="$w/mdir-$1" '
    function hx(s,  i, v) { s=tolower(s); sub(/^0x/, "", s); v=0
        for (i=1; i<=length(s); i++) v=v*16 + index("0123456789abcdef", substr(s,i,1)) - 1
        return v }
    /^sector size:/                       { bps=$3 }
    /^cluster size:/                      { spc=$3 }
    /^reserved \(boot\) sectors:/         { res=$4 }
    /^fats:/                              { fats=$2 }
    /^max available root directory slots:/ { root=$6 }
    /^small size:/                        { tot=$3 }
    /^big size:/                          { if (tot == 0) tot=$3 }
    /^media descriptor byte:/             { media=$4 }
    /^sectors per fat:/                   { spf=$4 }
    END {
        if (!bps || !spc || !fats || !root || !tot || !spf || media == "") {
            print "RED int21-dpb GOLDEN minfo parse " L > "/dev/stderr"; exit 1 }
        shift_=0; while ((2^shift_) < spc) shift_++
        dir=res + fats*spf                       # $SETDPB: MUL BH; ADD first_FAT
        first=dir + int((root*32 + bps - 1)/bps)  # + directory sectors (round up)
        maxc=int((tot - first)/spc) + 1           # SHR by shift; INC
        while ((getline line < MDIR) > 0)
            if (line ~ /bytes free/) { gsub(/[^0-9]/, "", line); free=line }
        if (free == "") { print "RED int21-dpb GOLDEN mdir free " L > "/dev/stderr"; exit 1 }
        printf "%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", L, D, D, bps, spc-1,
            shift_, res, fats, root, first, maxc, spf % 256, dir,
            hx(media), free/(bps*spc)
    }' "$w/minfo-$1" > "$w/golden-$1" || red "GOLDEN $1"
}

grade() { # $1 = artifact output file
    grep '^DPB ' "$1" > "$w/lines" || red 'no DPB lines'
    test "$(wc -l < "$w/lines")" -eq $NLINES || red "expected $NLINES DPB lines, got $(wc -l < "$w/lines")"
    cat "$w/golden-A" "$w/golden-C" "$w/golden-D" > "$w/golden"
    printf '%s\n' $EXPECT > "$w/expect"
    awk -v G="$w/golden" -v E="$w/expect" '
    function hx(s,  i, v) { s=tolower(s); v=0
        for (i=1; i<=length(s); i++) v=v*16 + index("0123456789abcdef", substr(s,i,1)) - 1
        return v }
    function b(i) { return hx(substr(hex, 2*i+1, 2)) }
    function wd(i) { return b(i) + 256*b(i+1) }
    function dw(i) { return wd(i) + 65536*wd(i+2) }
    function chk(name, got, want) {
        if (got != want) { printf "RED int21-dpb %s %s got %d want %d (%s)\n",
            L, name, got, want, call; bad=1 } }
    BEGIN {
        while ((getline g < G) > 0) { split(g, f, " "); for (k=2;k<=15;k++) gold[f[1],k]=f[k] }
        n=0; while ((getline e < E) > 0) exp_[++n]=e
    }
    {
        i++; split(exp_[i], ex, ":"); call=ex[1]; want=ex[2]
        if ($2 $3 != call) { printf "RED int21-dpb SCRIPT line %d is %s%s want %s\n", i, $2, $3, call; bad=1; next }
        if ($5 != "0") { printf "RED int21-dpb CF set on %s\n", call; bad=1 }
        if (want == "-") {
            if ($4 != "FF" || $6 != "-") { printf "RED int21-dpb INVALID %s AL=%s EBX=%s\n", call, $4, $6; bad=1 }
            next
        }
        if ($4 != "00") { printf "RED int21-dpb VALID %s AL=%s\n", call, $4; bad=1; next }
        L=want; hex=$6
        if (length(hex) != 64) { printf "RED int21-dpb %s short DPB\n", call; bad=1; next }
        chk("drive 00h",        b(0),    gold[L,2])
        chk("unit 01h",         b(1),    gold[L,3])
        chk("bytes/sector 02h", wd(2),   gold[L,4])
        chk("cluster mask 04h", b(4),    gold[L,5])
        chk("shift 05h",        b(5),    gold[L,6])
        chk("reserved 06h",     wd(6),   gold[L,7])
        chk("FAT count 08h",    b(8),    gold[L,8])
        chk("root entries 09h", wd(9),   gold[L,9])
        chk("first data sector 0Bh", wd(11), gold[L,10])
        chk("max cluster 0Dh",  wd(13),  gold[L,11])
        chk("FAT size 0Fh",     b(15),   gold[L,12])
        chk("dir sector 10h",   wd(16),  gold[L,13])
        chk("driver 12h",       dw(18),  0)
        chk("media 16h",        b(22),   gold[L,14])
        chk("access 17h",       b(23),   0)
        chk("free search 1Ch",  wd(28),  0)
        chk("free clusters 1Eh", wd(30), gold[L,15])
        nx = (L == "A") ? "C" : (L == "C") ? "D" : "END"   # mounted chain A -> C -> D -> end
        if ($7 != nx) { printf "RED int21-dpb %s next DPB %s want %s\n", L, $7, nx; bad=1 }
        seen[L]++
    }
    END {
        if (!seen["A"] || !seen["C"] || !seen["D"]) { print "RED int21-dpb missing A/C/D DPB"; bad=1 }
        exit bad
    }' "$w/lines" > "$w/grade" || { cat "$w/grade"; red "$(head -1 "$w/grade" | sed 's/^RED int21-dpb //')"; }
}

if [ "$mode" = host ]; then
    for kind in fat16 fat12; do
        mkimage "$kind"
        golden A 0 "$w/a.img"; golden C 2 "$C"; golden D 3 "$D"
        "$driver" "$w/a.img" "$w/c.img" > "$w/out-$kind" 2> "$w/err-$kind" || { cat "$w/err-$kind"; red "driver $kind"; }
        grade "$w/out-$kind"
        echo "GREEN int21-dpb host $kind A + FAT12 C + FAT16 D: $NLINES calls, every DPB field == minfo/mdir"
    done
else
    # A must be FAT12: EXEC loads only from a whole-FAT volume (loader.c);
    # FAT16 is witnessed in the emulator through D.
    mkimage fat12
    mcopy -i "$w/a.img" build/int21_dpb_program.bin ::DPB.COM
    printf '@ECHO OFF\r\nDPB\r\nECHO DPB-DONE\r\n' > "$w/autoexec"
    mcopy -i "$w/a.img" "$w/autoexec" ::AUTOEXEC.BAT
    golden A 0 "$w/a.img"; golden C 2 "$C"; golden D 3 "$D"
    build/qemu_harness --boot-rw --disk "$w/c.img" --disk2 "$w/a.img" --name run --out "$w" \
        --timeout-ms 30000 --keys 'e,x,i,t,ret' --keys-after DPB-DONE \
        --quit-after SHELL-DONE 2> "$w/report" || true
    grep -qx SHELL-EXIT "$w/run.serial" || red 'EMU did not finish shell'
    if grep -q 'triple_fault=1' "$w/report"; then red 'EMU triple fault'; fi
    tr -d '\r' < "$w/run.serial" > "$w/text"
    grade "$w/text"
    # The program described unchanged volumes: the golden still holds after.
    for v in A C D; do cp "$w/golden-$v" "$w/before-$v"; done
    golden A 0 "$w/a.img"; golden C 2 "$C"; golden D 3 "$D"
    for v in A C D; do cmp -s "$w/before-$v" "$w/golden-$v" || red "volume $v changed under the run"; done
    fsck.fat -n "$w/a.img" > "$w/fsck-a" 2>&1 || red 'FSCK A'
    fsck.fat -n "$w/d.img" > "$w/fsck-d" 2>&1 || red 'FSCK D'
    echo "GREEN int21-dpb emu: QEMU DPB.COM over ATA FAT12 A + FAT12 C + FAT16 D, $NLINES calls, every DPB field == minfo/mdir"
fi
