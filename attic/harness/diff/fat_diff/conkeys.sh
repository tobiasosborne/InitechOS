#!/bin/sh
# Keyboard semantics use printed manual vectors; FAT extraction/allocation is
# differential against mtools/dosfstools on copies of the SAME starting image.
# Ref: MS-DOS 3.3 User's Guide pp.154-159,165-169; ADR-0003 App A / DEC-08.
set -eu
export INITECH_QMP_STDIO=1
build=$1; boot=$2; program=$3
out=$build/conkeys
mkdir -p "$out"
fail() { printf 'RED CONKEYS %s\n' "$1"; exit 1; }
dd if=/dev/zero of="$out/data.img" bs=512 count=2880 status=none
mformat -i "$out/data.img" -f 1440 -N 0x434f4e4b ::
mcopy -i "$out/data.img" "$program" ::CONKEYS.COM
printf '@echo off\r\nCONKEYS\r\n' > "$out/autoexec.bat"
mcopy -i "$out/data.img" "$out/autoexec.bat" ::AUTOEXEC.BAT
cp -f "$out/data.img" "$out/reference.img"
sed 's/$/\r/' harness/conkeys/lines.txt > "$out/expected.txt"
mcopy -i "$out/reference.img" "$out/expected.txt" ::LINES.TXT
"$build/qemu_harness" --disk "$boot" --disk2 "$out/data.img" --name conkeys --out "$build" \
 --keys "$(cat harness/conkeys/keys.qmp)" --keys-after CONKEYS-READY \
 --quit-after SHELL-EXIT --timeout-ms 45000 2> "$out/report" || { cat "$out/report"; fail boot; }
grep -q 'OK=1' "$out/report" || fail completion
if grep -Eq 'triple_fault=1|unknown token|truncating' "$out/report"; then fail injection; fi
mtype -i "$out/data.img" ::LINES.TXT > "$out/actual.txt" || fail saved-lines
mtype -i "$out/reference.img" ::LINES.TXT > "$out/oracle.txt"
cmp -s "$out/actual.txt" "$out/oracle.txt" || { diff -u "$out/oracle.txt" "$out/actual.txt" || true; fail template-lines; }
tr -d '\r' < "$build/conkeys.serial" > "$out/transcript"
for text in 'abcxdef' 'abcZ' 'new@' 'bad\' CONKEYS-PAUSE CONKEYS-RESUME CONKEYS-PRN-ON CONKEYS-PRN-OFF CONKEYS-DONE SHELL-EXIT; do
 grep -Fq "$text" "$out/transcript" || fail "transcript-$text"
done
mdir -b -i "$out/data.img" :: | sort > "$out/actual.names"
mdir -b -i "$out/reference.img" :: | sort > "$out/oracle.names"
cmp -s "$out/actual.names" "$out/oracle.names" || fail FAT-names
for disk in data reference; do
 fsck.fat -n "$out/$disk.img" > "$out/$disk.fsck" 2>&1 || { cat "$out/$disk.fsck"; fail FAT-structure; }
done
printf 'GREEN test-conkeys: injected template keys + pause transcript; LINES.TXT == mtools on identical FAT12 images; dosfstools clean\n'
