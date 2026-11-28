#!/bin/sh
# Independent mtools payload + normal installed flat XCOPY, no kernel scenario.
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?out}
cd "$root"
. "$root/dos/tests/runtime/utility-session.sh"
mkdir -p "$out"; out=$(realpath "$out")
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
for cpu in qemu bochs; do
 d=$out/$cpu; mkdir -p "$d"
 truncate -s 1474560 "$d/data.img"
 mformat -i "$d/data.img" -f 1440 ::
 mmd -i "$d/data.img" ::SRC ::DST
 dd if=/dev/zero bs=200000 count=1 status=none | tr '\000' 'S' > "$d/source"
 printf 'OLD DESTINATION\r\n' > "$d/previous"
 mcopy -i "$d/data.img" "$d/source" ::SRC/LARGE.BIN
 mcopy -i "$d/data.img" "$d/previous" ::DST/LARGE.BIN
 nasm -f bin dos/tests/runtime/done.asm -o "$d/DONE.COM"
 mcopy -i "$d/data.img" "$programs/XCOPY.COM" "$d/DONE.COM" ::
 printf '@echo off\r\nA:\\XCOPY SRC\\LARGE.BIN DST /V\r\nif errorlevel 1 echo XCOPY-BAD\r\nA:\\DONE\r\n' > "$d/AUTOEXEC.BAT"
 mcopy -i "$d/data.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 DOS_UTILITY_EMULATOR=$cpu utility_session "$boot" "$d/data.img" "$d" --expect TRANSPORT-DONE \
  --quit-after TRANSPORT-DONE --timeout-ms 90000 --name run > "$d/driver.log" 2> "$d/report"
 grep -F 'OK=1' "$d/report" >/dev/null
 grep -F 'timed_out=0' "$d/report" >/dev/null
 grep -F 'triple_fault=0' "$d/report" >/dev/null
 actual=$d/data.img
 if [ "$cpu" = bochs ]; then
  actual=$(sed -n 's/^\[bochs\] private_disk2=//p' "$d/report")
  [ -n "$actual" ] || actual=$d/data.img
 fi
 mtype -i "$actual" ::DST/LARGE.BIN > "$d/actual"
 cmp "$d/source" "$d/actual"
 if grep -F XCOPY-BAD "$d/run.serial"; then exit 1; fi
 fsck.fat -n "$actual" > "$d/fsck"
 echo "GREEN $cpu XCOPY: 200000-byte staged copy, /V, existing destination, mtools bytes and fsck"
done
