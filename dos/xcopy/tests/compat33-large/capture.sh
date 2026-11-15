#!/bin/sh
set -eu
export LC_ALL=C TZ=UTC
root=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
boot=${1:?genuine DOS 3.30 disk 1}
utility=${2:?genuine XCOPY.EXE}
out=${3:?new capture directory}
[ ! -e "$out" ] || { echo "capture directory already exists" >&2; exit 2; }
mkdir -p "$out"
out=$(realpath "$out")
[ "$(md5sum "$boot" | cut -d " " -f1)" = 5c2c58602d688c636ea1c9801e6106f4 ]
[ "$(sha256sum "$utility" | cut -d " " -f1)" = c4c694e1c1625e8d22bf1cfa658eb814ba1e1259400deef0fa4a9a1ee2bb8e31 ]
mkdir -p "$out/input" "$out/results"
cp "$boot" "$out/boot.img"
mdel -i "$out/boot.img" ::4201.CPI
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 -N 0x19920302 ::
mmd -i "$out/data.img" ::SRC ::DST
awk 'BEGIN{for(i=0;i<20000;i++)printf "%08X\r\n",(i*7919)%104729}' > "$out/input/LARGE.BIN"
printf 'after the large file\r\n' > "$out/input/ZZZ.TXT"
printf 'old destination\r\n' > "$out/input/OLD.TXT"
mcopy -m -i "$out/data.img" "$out/input/LARGE.BIN" "$out/input/ZZZ.TXT" ::SRC
mcopy -m -i "$out/data.img" "$out/input/OLD.TXT" ::DST/LARGE.BIN
mcopy -i "$out/data.img" "$utility" ::XCOPY.EXE
printf '@echo off\r\nver > B:\\VERSION.TXT\r\nB:\r\nB:\\XCOPY B:\\SRC B:\\DST /V >RESULT.TXT\r\nif errorlevel 1 echo BAD>STATUS.TXT\r\nif not errorlevel 1 echo OK>STATUS.TXT\r\necho CAPTURE-DONE\r\n' > "$out/AUTOEXEC.BAT"
printf 'files=20\r\nbuffers=20\r\n' > "$out/CONFIG.SYS"
mcopy -i "$out/boot.img" "$out/AUTOEXEC.BAT" "$out/CONFIG.SYS" ::
cp "$out/data.img" "$out/before-data.img"
printf 'wait CAPTURE-DONE\nshot complete\nend\n' > "$out/steps"
sed 's/i<80/i<400/' "$root/goldens/dataloss/drive.c" > "$out/drive.c"
cc -std=c11 -O2 -Wall -Wextra -Werror "$out/drive.c" -o "$out/drive"
"$out/drive" "$out" "$out/boot.img" "$out/data.img" - "$out/steps"
mcopy -i "$out/data.img" ::RESULT.TXT ::STATUS.TXT ::VERSION.TXT ::DST/LARGE.BIN ::DST/ZZZ.TXT "$out/results/"
cmp "$out/input/LARGE.BIN" "$out/results/LARGE.BIN"
cmp "$out/input/ZZZ.TXT" "$out/results/ZZZ.TXT"
cat "$out/results/STATUS.TXT" "$out/results/VERSION.TXT"
