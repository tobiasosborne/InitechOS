#!/bin/sh
# Authored before diskutil.c. Law 1: MS-DOS UG pp74-76,119; IBM TR
# pp1-7,5-4..11. mformat/mcopy/fsck examine the identical resulting images.
set -eu
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800
b=$1; boot=$2; exe=$3; only=${4:-all}; w=$b/formatsys
mkdir -p "$w"
red(){ echo "RED formatsys $*"; exit 1; }
prep(){
 cp -f "$boot" "$w/master.img"; truncate -s $((8064*512)) "$w/master.img"
 mcopy -o -i "$w/master.img@@32256" "$b/transient_flairtenants.bin" ::COMMAND.COM
 printf 'label: dos\nunit: sectors\nstart=63,size=2880,type=1,bootable\nstart=4032,size=2880,type=1\n' | sfdisk --no-reread --no-tell-kernel "$w/master.img" > "$w/sfdisk.log" 2>&1
 truncate -s $((2880*512)) "$w/a.img"
 mformat -i "$w/a.img" -f 1440 -N 0x494e4954 ::
}
run(){ "$exe" "$w/master.img" "$w/a.img" "$1" "$2" "$3" "$4" > "$w/console" 2> "$w/exit" || { cat "$w/console" "$w/exit"; red "command $2"; }; }
extract(){ dd if="$w/master.img" of="$w/ours.img" bs=512 skip=4032 count=2880 status=none; }
clean(){ fsck.fat -n "$w/ours.img" > "$w/fsck" 2>&1 || { cat "$w/fsck"; red filesystem; }; }
prep
# Hard-volume media F8, FAT12 by cluster count, SPC=1. Independent mformat.
run 0 'D:' 'y
' 0
extract
truncate -s $((2880*512)) "$w/ref.img"
mformat -i "$w/ref.img" -T 2880 -h 16 -s 63 -c 1 -r 14 -H 4032 -N 0x4b4f4433 ::
# mformat selects F0 for this arbitrary small geometry; fixed disk F8 is the
# IBM TR p5-6 literal, independently asserted rather than copied from ours.
printf '\370' | dd of="$w/ref.img" bs=1 seek=21 conv=notrunc status=none
for off in 512 5120; do printf '\370' | dd of="$w/ref.img" bs=1 seek=$off conv=notrunc status=none; done
"$b/formatsys_check" "$w/ours.img" "$w/ref.img" empty "$boot" || red 'format BPB/FAT differential'
clean
# Volume label must be a root label, readable identically through mtools.
prep; run 0 'D: /V /S' 'y
OFFICE
' 0; extract
"$b/formatsys_check" "$w/ours.img" "$w/ref.img" system "$boot" || red 'system order attributes COMMAND'
for f in IO.SYS INITDOS.SYS COMMAND.COM; do
 mcopy -i "$w/master.img@@32256" "::$f" - > "$w/ref-$f"
 mcopy -i "$w/ours.img" "::$f" - > "$w/ours-$f" 2>/dev/null || red 'system order attributes COMMAND'
 cmp "$w/ref-$f" "$w/ours-$f" || red 'system file bytes'
done
mdir -i "$w/ours.img" :: > "$w/mdir"; grep -q OFFICE "$w/mdir" || red label
mattrib -i "$w/ours.img" ::IO.SYS ::INITDOS.SYS > "$w/attrs"
grep -q 'SHR.*IO.SYS\|RSH.*IO.SYS\|SH.*R.*IO.SYS' "$w/attrs" || red attributes
clean
# SYS excludes COMMAND.COM, preserves occupied files, refuses occupied slots.
prep; run 0 'D:' 'y
' 0; run 1 'D:' '' 0; extract
"$b/formatsys_check" "$w/ours.img" "$w/ref.img" sys "$boot" || red 'SYS root or COMMAND'
clean
prep; run 0 'D:' 'y
' 0
printf occupied > "$w/occupied"; mcopy -i "$w/master.img@@2064384" "$w/occupied" ::FIRST.TXT
cp -f "$w/master.img" "$w/before.img"; run 1 'D:' '' 1
grep -qFx 'No room for system on destination disk' "$w/console" || red 'SYS occupied message'
cmp "$w/master.img" "$w/before.img" || red 'SYS occupied writes'
# Every unassigned letter must use the catalogue row, with no writes.
for kind in 0 1 2 3; do run "$kind" 'B:' '' 4; grep -qFx 'Invalid drive specification' "$w/console" || red 'invalid drive'; done
# Real running-volume identity = mounted disk + partition, never just letter.
cp -f "$w/a.img" "$w/a-before.img"
"$exe" "$w/master.img" "$w/a.img" 0 'A:' '
N' 4 > "$w/console" 2> "$w/exit" || red 'running volume not refused'
cmp "$w/a-before.img" "$w/a.img" || red 'running volume not refused'
grep -q 'Format not supported on drive A:' "$w/console" || red 'running volume not refused'
# All source/target sectors, including last sector, pass through single A:.
printf marker | dd of="$w/a.img" bs=1 seek=$((2879*512+37)) conv=notrunc status=none
cp -f "$w/a.img" "$w/source.img"; dd if=/dev/zero bs=512 count=2880 status=none | tr '\000' '\245' > "$w/target.img"
"$exe" "$w/master.img" "$w/a.img" 2 'A: A:' N 0 "$w/source.img" "$w/target.img" > "$w/copy.console" 2> "$w/copy.exit" || red diskcopy
cmp "$w/source.img" "$w/target.img" || red 'DISKCOPY every sector'
"$exe" "$w/master.img" "$w/a.img" 3 'A: A:' N 0 "$w/source.img" "$w/target.img" > "$w/compare.console" 2> "$w/compare.exit" || red diskcomp
grep -q 'Compare OK' "$w/compare.console" || red 'compare OK'
printf X | dd of="$w/target.img" bs=1 seek=$((2879*512+38)) conv=notrunc status=none
"$exe" "$w/master.img" "$w/a.img" 3 'A: A:' N 1 "$w/source.img" "$w/target.img" > "$w/compare.console" 2> "$w/compare.exit" || red 'DISKCOMP missing differing sector'
grep -q 'side 1, track 79' "$w/compare.console" || red 'DISKCOMP missing differing sector'
echo 'GREEN test-formatsys: mformat BPB/FAT, /S bytes/HSR/order/label, SYS exclusions/occupied, invalid/running, single-drive copy/compare all sectors; fsck.fat'
# FAT16 is selected by actual cluster count at SPC=1, independently mformatted.
prep
truncate -s $((40000*512)) "$w/master.img"
printf 'label: dos\nunit: sectors\nstart=63,size=2880,type=1,bootable\nstart=4032,size=32768,type=6\n' | sfdisk --no-reread --no-tell-kernel "$w/master.img" > "$w/sfdisk.log" 2>&1
run 0 'D:' 'y
' 0
dd if="$w/master.img" of="$w/ours.img" bs=512 skip=4032 count=32768 status=none
dd if=/dev/zero of="$w/ref16.img" bs=512 count=32768 status=none
mformat -i "$w/ref16.img" -T 32768 -h 16 -s 63 -c 1 -r 32 -H 4032 -N 0x4b4f4433 ::
"$b/formatsys_check" "$w/ours.img" "$w/ref16.img" empty "$boot" || red FAT16
clean
# A size outside DOS 3.3/SPC1 must fail before changing even one disk byte.
run 0 'D: /S' '
y
' 0
dd if="$w/master.img" of="$w/ours.img" bs=512 skip=4032 count=32768 status=none
"$b/formatsys_check" "$w/ours.img" "$w/ref16.img" system "$boot" || red 'FAT16 /S'
for f in IO.SYS INITDOS.SYS COMMAND.COM; do
 mcopy -i "$w/master.img@@32256" "::$f" - > "$w/ref-$f"
 mcopy -i "$w/ours.img" "::$f" - > "$w/ours-$f"
 cmp "$w/ref-$f" "$w/ours-$f" || red 'FAT16 /S bytes'
done
clean
truncate -s $((75000*512)) "$w/master.img"
printf 'label: dos\nunit: sectors\nstart=63,size=2880,type=1,bootable\nstart=4032,size=70000,type=6\n' | sfdisk --no-reread --no-tell-kernel "$w/master.img" > "$w/sfdisk.log" 2>&1
cp -f "$w/master.img" "$w/before.img"; run 0 'D:' '' 4
cmp "$w/before.img" "$w/master.img" || red 'SPC1 size refusal writes'
grep -qFx 'Format requires more than one sector per cluster' "$w/console" || red 'SPC1 size message'
echo 'GREEN test-formatsys: FAT16 SPC1 == mformat and fsck.fat; larger-size refusal is byte-identical'
# Removable geometries and DOS switch spellings, with current=C in this host
# fixture. Actual shipped current=A still refuses its own volume above.
for desc in '2880||1440' '720|/4|360' '640|/4 /8|320' '320|/1 /8 /T:40 /N:8|160' '1440|/T:80 /N:9|720'; do
 sectors=${desc%%|*}; rest=${desc#*|}; switches=${rest%%|*}; kib=${rest##*|}
 dd if=/dev/zero of="$w/a.img" bs=512 count="$sectors" status=none
 mformat -i "$w/a.img" -f "$kib" -c 1 -N 0x4b4f4433 ::
 dd if=/dev/zero of="$w/ref-floppy.img" bs=512 count="$sectors" status=none
 mformat -i "$w/ref-floppy.img" -f "$kib" -c 1 -N 0x4b4f4433 ::
 # -c1 makes mformat call the small nonstandard cluster geometry F0.
 # IBM TR p5-6 fixes the DOS media code by diskette geometry, independently.
 case $kib in 160) media='\376';; 320) media='\377';; 360) media='\375';; 720) media='\371';; *) media='\360';; esac
 printf "$media" | dd of="$w/ref-floppy.img" bs=1 seek=21 conv=notrunc status=none
 spf=$(od -An -tu2 -j22 -N2 "$w/ref-floppy.img" | tr -d ' ')
 for off in 512 $(((1+spf)*512)); do printf "$media" | dd of="$w/ref-floppy.img" bs=1 seek=$off conv=notrunc status=none; done
 DU_CURRENT_C=1 "$exe" "$w/master.img" "$w/a.img" 0 "A: $switches" '
N' 0 > "$w/floppy.console" 2> "$w/floppy.exit" || red 'floppy switches'
 "$b/formatsys_check" "$w/a.img" "$w/ref-floppy.img" floppy "$boot" || red 'floppy geometry'
 fsck.fat -n "$w/a.img" > "$w/floppy-fsck" 2>&1 || red 'floppy fsck'
done
echo 'GREEN test-formatsys: /1 /4 /8 /T /N geometry and media match mformat; every volume SPC1 and fsck.fat clean'
# /B protects room for the two SYS files in root slots 0/1. Independent
# mtools reads must see HSR reservations with the source sizes and zero bytes;
# a later SYS fills them without consuming any additional free clusters.
prep
DU_CURRENT_C=1 "$exe" "$w/master.img" "$w/a.img" 0 'A: /B' '
N' 0 > "$w/reserve.console" 2> "$w/reserve.exit" || red '/B reserve'
for f in IO.SYS INITDOS.SYS; do
 mcopy -i "$w/master.img@@32256" "::$f" - > "$w/source-$f"
 mcopy -i "$w/a.img" "::$f" - > "$w/reserved-$f" 2>/dev/null || red '/B reserve'
 bytes=$(wc -c < "$w/source-$f")
 head -c "$bytes" /dev/zero > "$w/zeros"
 cmp "$w/reserved-$f" "$w/zeros" || red '/B reserve bytes'
done
fsck.fat -n "$w/a.img" > "$w/reserve-fsck" 2>&1 || red '/B fsck'
run 1 'A:' '' 0
for f in IO.SYS INITDOS.SYS; do
 mcopy -i "$w/a.img" "::$f" - > "$w/transferred-$f"
 cmp "$w/source-$f" "$w/transferred-$f" || red '/B then SYS'
done
fsck.fat -n "$w/a.img" > "$w/reserve-sys-fsck" 2>&1 || red '/B then SYS fsck'
echo 'GREEN test-formatsys: /B reserves system slots and clusters, SYS fills reservations; fsck.fat clean'
# FORMAT C: /S streams the existing contiguous shipped system in place,
# preserving the bootstrap prefix, with no host-memory-only implementation.
prep
for f in IO.SYS INITDOS.SYS COMMAND.COM; do mcopy -i "$w/master.img@@32256" "::$f" - > "$w/before-$f"; done
run 0 'C: /S' 'BOOT
y
' 0
for f in IO.SYS INITDOS.SYS COMMAND.COM; do
 mcopy -i "$w/master.img@@32256" "::$f" - > "$w/after-$f"
 cmp "$w/before-$f" "$w/after-$f" || red 'FORMAT C /S self transfer'
done
dd if="$w/master.img" of="$w/ours.img" bs=512 skip=63 count=2880 status=none
clean
echo 'GREEN test-formatsys: FORMAT C: /S preserves source bytes in place; original bootstrap prefix retained'
# SYS uses free holes without disturbing an existing third root entry.
prep; run 0 'D:' 'y
' 0
printf 'keep this file' > "$w/keep"
for f in FIRST.TXT SECOND.TXT KEEP.TXT; do mcopy -i "$w/master.img@@2064384" "$w/keep" "::$f"; done
mdel -i "$w/master.img@@2064384" ::FIRST.TXT ::SECOND.TXT
run 1 'D:' '' 0
mcopy -i "$w/master.img@@2064384" ::KEEP.TXT - > "$w/keep-after"
cmp "$w/keep" "$w/keep-after" || red 'SYS unrelated file'
for f in IO.SYS INITDOS.SYS; do
 mcopy -i "$w/master.img@@32256" "::$f" - > "$w/ref-$f"
 mcopy -i "$w/master.img@@2064384" "::$f" - > "$w/ours-$f"
 cmp "$w/ref-$f" "$w/ours-$f" || red 'SYS fragmented destination'
done
extract; clean
echo 'GREEN test-formatsys: SYS fills deleted first slots, preserves KEEP.TXT, writes fragmented chains mcopy/fsck.fat accept'
# Repeating /V must not carry the first medium's label to an unlabeled second.
prep
DU_CURRENT_C=1 "$exe" "$w/master.img" "$w/a.img" 0 'A: /V' '
ONE
Y

N' 0 > "$w/repeat.console" 2> "$w/repeat.exit" || red '/V repeat'
fsck.fat -n "$w/a.img" > "$w/repeat-fsck" 2>&1 || red '/V repeat label fsck'
mdir -i "$w/a.img" :: > "$w/repeat-mdir"
grep -q 'has no label' "$w/repeat-mdir" || red '/V repeat label'
echo 'GREEN test-formatsys: Format another Y repeats; an empty second /V clears the first label'
# Audit L041; MS-DOS 3.3 UG p336 (PDF421), DEC-18 source C:. Each missing
# /S file must prompt/retry/cancel before ANY destination writes.
for f in COMMAND.COM IO.SYS INITDOS.SYS; do
 prep
 cp -f "$w/master.img" "$w/retry.img"
 mattrib -i "$w/master.img@@32256" -r -s -h "::$f"
 mdel -i "$w/master.img@@32256" "::$f"
 cp -f "$w/master.img" "$w/missing-before.img"
 "$exe" "$w/master.img" "$w/a.img" 0 'D: /S' "$(printf '\n\003')" 3 > "$w/console" 2> "$w/exit" || red 'missing source message'
 printf 'Insert DOS diskette in drive C:\nand strike ENTER when ready\n\nInsert DOS diskette in drive C:\nand strike ENTER when ready\n' > "$w/missing.want"
 cmp -s "$w/missing.want" "$w/console" || red 'missing source message'
 cmp -s "$w/missing-before.img" "$w/master.img" || red 'missing source wrote disk'
 # Supply an independently copied complete medium on ENTER. Prove the
 # lookup is repeated and all three files are transferred, not just refused.
 DU_RETRY_SOURCE="$w/retry.img" run 0 'D: /S' '
y
' 0
 grep -qFx 'System transferred' "$w/console" || red 'source retry transfer'
 dd if="$w/retry.img" of="$w/retried.img" bs=512 skip=4032 count=2880 status=none
 fsck.fat -n "$w/retried.img" > "$w/retry-fsck" 2>&1 || red 'source retry filesystem'
 for file in IO.SYS INITDOS.SYS COMMAND.COM; do
  mcopy -i "$w/retry.img@@32256" "::$file" - > "$w/retry-ref"
  mcopy -i "$w/retried.img" "::$file" - > "$w/retry-got"
  cmp -s "$w/retry-ref" "$w/retry-got" || red 'source retry bytes'
 done
done
echo 'GREEN test-formatsys: each missing /S file prompts/retries/cancels without writes; restored source transfers all bytes'
# /B needs only the two system files: missing COMMAND is allowed, while
# a missing system file gets the same source prompt without formatting A:.
prep
mdel -i "$w/master.img@@32256" ::COMMAND.COM
DU_CURRENT_C=1 run 0 'A: /B' '
N' 0
for f in IO.SYS INITDOS.SYS; do
 prep
 mattrib -i "$w/master.img@@32256" -r -s -h "::$f"
 mdel -i "$w/master.img@@32256" "::$f"
 cp -f "$w/a.img" "$w/missing-a-before.img"
 DU_CURRENT_C=1 run 0 'A: /B' "$(printf '\003')" 3
 printf 'Insert DOS diskette in drive C:\nand strike ENTER when ready\n' > "$w/missing.want"
 cmp -s "$w/missing.want" "$w/console" || red '/B missing source message'
 cmp -s "$w/missing-a-before.img" "$w/a.img" || red '/B missing source wrote disk'
done
echo 'GREEN test-formatsys: /B needs two system files and uses missing-source prompt'
