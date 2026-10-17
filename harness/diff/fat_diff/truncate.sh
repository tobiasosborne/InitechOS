#!/bin/sh
# Identical FAT12 seeds, independent mcopy replacements and free-space counts.
# IBM DOS 3.30 TR p.6-140; FAT allocation layout ADR-0003 Appendix B.1/DEC-07.
set -efu
export LC_ALL=C TZ=UTC INITECH_QMP_STDIO=1
mode=$1 runner=$2 tag=${3:-green}
out=build/truncate-$mode-$tag
mkdir -p "$out"
fail() { echo "RED truncate: $*" >&2; exit 1; }
dd if=/dev/zero of="$out/data.img" bs=512 count=2880 status=none
mformat -i "$out/data.img" -f 1440 -N 140 ::
build/truncate --body > "$out/body"
mmd -i "$out/data.img" ::SUB
for name in SHRINK BOUND EMPTY GROW GUARD SUB/CHILD EXTEND RONLY; do
    mcopy -i "$out/data.img" "$out/body" "::$name.BIN"
done
mattrib -i "$out/data.img" +r ::RONLY.BIN
if [ "$mode" = emu ]; then
    mcopy -i "$out/data.img" build/truncate_program.bin ::TRUNC.COM
    printf '@echo off\ntrunc\nif errorlevel 1 goto bad\necho TRUNCATE-PASS\ngoto done\n:bad\necho TRUNCATE-FAIL\n:done\necho TRUNCATE-DONE\n' > "$out/autoexec"
    mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
fi
cp -f "$out/data.img" "$out/seed.img"
cp -f "$out/data.img" "$out/reference.img"
head -c 600 "$out/body" > "$out/short"
head -c 512 "$out/body" > "$out/bound"
: > "$out/empty"
{ cat "$out/short"; head -c 1000 /dev/zero | tr '\000' R; } > "$out/grow"
{ cat "$out/body"; head -c 800 /dev/zero; } > "$out/extend"
for item in SHRINK:short BOUND:bound EMPTY:empty GROW:grow SUB/CHILD:short EXTEND:extend; do
    mcopy -o -i "$out/reference.img" "$out/${item#*:}" "::${item%:*}.BIN"
done
if [ "$mode" = host ]; then
    "$runner" "$out/data.img" matrix > "$out/operations" 2>&1 || { cat "$out/operations"; fail 'AH40 matrix'; }
else
    build/qemu_harness --disk "$runner" --disk2 "$out/data.img" --out "$out" --name run \
      --timeout-ms 25000 --keys-after SHELL-READY --keys 'e,x,i,t,ret' --quit-after SHELL-DONE \
      --rtc-base 1987-07-06T13:30:58 --rtc-deterministic 2> "$out/report" || true
    grep -q quit_after_found=1 "$out/report" || fail 'shell did not finish'
    grep -q keys_sent=5 "$out/report" || fail 'injected trace incomplete'
    ! grep -q triple_fault=1 "$out/report" || fail 'triple fault'
    tr -d '\r' < "$out/run.serial" > "$out/text"
    grep -qx TRUNCATE-DONE "$out/text" || fail 'program did not return'
    grep -qx TRUNCATE-PASS "$out/text" || fail 'AH40 matrix'
fi
for name in SHRINK BOUND EMPTY GROW GUARD SUB/CHILD EXTEND RONLY; do
    mtype -i "$out/data.img" "::$name.BIN" > "$out/got"
    mtype -i "$out/reference.img" "::$name.BIN" > "$out/want"
    cmp -s "$out/got" "$out/want" || fail "$name bytes != mcopy"
done
for img in data reference; do
    mdir -i "$out/$img.img" :: | sed -n '/bytes free/p' > "$out/$img.free"
done
cmp -s "$out/data.free" "$out/reference.free" || fail 'released clusters != mtools'
fsck.fat -n "$out/data.img" > "$out/fsck" 2>&1 || fail 'dosfstools damage'
if [ "$mode" = host ]; then
    for n in 1 2 3; do
        cp -f "$out/seed.img" "$out/fault.img"
        "$runner" "$out/fault.img" fault "$n"
        cmp -s "$out/seed.img" "$out/fault.img" || fail "fault $n did not restore identical image"
    done
    for n in 0 1 2 3; do
        cp -f "$out/seed.img" "$out/corrupt.img"
        "$runner" "$out/corrupt.img" corrupt "$n"
        cmp -s "$out/seed.img" "$out/corrupt.img" || fail 'corruption refusal changed disk'
    done
fi
echo "GREEN test-fat-truncate-$mode: AH40 position/size, tail release, zero, boundary, subdir, regrow == mtools; fsck.fat"
if [ "$mode" = host ]; then echo 'GREEN truncate rollback: FAT1/FAT2/directory write faults restore identical image and live FAT; short/cycle/free/bad chains refused'; fi
