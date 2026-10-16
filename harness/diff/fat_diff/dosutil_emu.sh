#!/bin/sh
# Real FAT-loaded COMs + COMMAND.COM VOL, m0dc injected-trace pattern.
set -eu
export INITECH_QMP_STDIO=1 SOURCE_DATE_EPOCH=315532800 TZ=UTC
boot=$1
attrib=$2
label=$3
tag=$4
b=build/dosutil_emu_$tag
fail() { echo "FAIL: $1"; exit 1; }
sh harness/diff/fat_diff/dosutil_fixture.sh "$b.img"
mmd -i "$b.img" ::DOS
mcopy -i "$b.img" "$attrib" ::DOS/ATTRIB.COM
mcopy -i "$b.img" "$label" ::DOS/LABEL.COM
printf '\r\nN\r\n' > "$b.no"
printf '\r\ny\r\n' > "$b.yes"
mcopy -i "$b.img" "$b.no" ::NO.TXT
mcopy -i "$b.img" "$b.yes" ::YES.TXT
printf '@ECHO OFF\r\nPATH=A:\\DOS\r\nATTRIB +R -A A:\\ONE.TXT /S\r\nATTRIB A:\\SUB\\ONE.TXT > ATTR.OUT\r\nLABEL A:NEW LABEL\r\nVOL A: > VOL.OUT\r\nLABEL A: < NO.TXT > NO.OUT\r\nVOL > KEEP.OUT\r\nLABEL A: < YES.TXT > YES.OUT\r\nVOL > NONE.OUT\r\nLABEL A:LAST LABEL\r\nVOL B: > BAD.OUT\r\nECHO DOSUTIL-DONE\r\n' > "$b.bat"
mcopy -i "$b.img" "$b.bat" ::AUTOEXEC.BAT
cp -f "$b.img" "$b.ref"
build/qemu_harness --disk "$boot" --disk2 "$b.img" --name "dosutil_emu_$tag" \
    --out build --timeout-ms 30000 --keys 'e,x,i,t,ret' --keys-after DOSUTIL-DONE \
    --quit-after SHELL-DONE 2> "$b.report" || true
grep -q '^SHELL-EXIT$' "$b.serial" || fail 'E0 clean shell return after real COM execution'
if grep -q 'triple_fault=1' "$b.report"; then fail 'E0 triple fault'; fi
for p in ONE.TXT SUB/ONE.TXT SUB/DEEP/ONE.TXT OTHER/ONE.TXT; do mattrib -i "$b.ref" +r -a "::$p"; done
for p in ONE.TXT TWO.TXT OTHER.BIN SUB/ONE.TXT SUB/DEEP/ONE.TXT OTHER/ONE.TXT; do
    mattrib -i "$b.ref" "::$p" > "$b.expected"
    mattrib -i "$b.img" "::$p" > "$b.actual"
    cmp -s "$b.actual" "$b.expected" || fail 'E1 ATTRIB R/A /S vs mattrib'
done
mtype -i "$b.img" ::ATTR.OUT > "$b.actual"
printf 'R        A:\\SUB\\ONE.TXT\r\n' > "$b.expected"
cmp -s "$b.actual" "$b.expected" || fail 'E2 ATTRIB display columns/full path'
for p in VOL KEEP; do
    mtype -i "$b.img" "::$p.OUT" > "$b.actual"
    printf '\r\n Volume in drive A is NEW LABEL\r\n' > "$b.expected"
    cmp -s "$b.actual" "$b.expected" || fail 'E3 VOL current root label / refused deletion'
done
mtype -i "$b.img" ::NONE.OUT > "$b.actual"
printf '\r\n Volume in drive A has no label\r\n' > "$b.expected"
cmp -s "$b.actual" "$b.expected" || fail 'E4 VOL no-label after Y'
for p in NO YES; do
    mtype -i "$b.img" "::$p.OUT" > "$b.actual"
    grep -Fq 'Volume label (11 characters, ENTER for none)?' "$b.actual" || fail 'E5 LABEL input prompt'
    grep -Fq 'Delete current volume label (Y/N)?' "$b.actual" || fail 'E5 LABEL deletion prompt'
done
mtype -i "$b.img" ::BAD.OUT > "$b.actual"
grep -Fq 'Invalid drive specification' "$b.actual" || fail 'E6 VOL drive validation'
mlabel -i "$b.ref" ::'LAST LABEL'
mlabel -s -i "$b.ref" :: > "$b.expected"
mlabel -s -i "$b.img" :: > "$b.actual"
cmp -s "$b.actual" "$b.expected" || fail 'E7 LABEL root entry vs mlabel'
fsck.fat -n "$b.img" > "$b.fsck" 2>&1 || fail 'E8 LABEL BPB/root agreement / disk integrity'
echo 'PASS: ATTRIB/LABEL/VOL QEMU differential, dialogue, disk and clean return'
