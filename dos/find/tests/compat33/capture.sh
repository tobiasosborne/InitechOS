#!/bin/sh
# Reacquire the genuine capture on a private copy of MS-DOS 3.30 Disk 1.
set -eu
export LC_ALL=C TZ=UTC
case_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
root=$(CDPATH= cd -- "$case_dir/../../../.." && pwd)
boot=${1:?genuine DOS 3.30 disk 1}
out=${2:?new capture directory}
mkdir "$out"
out=$(realpath "$out")
[ "$(md5sum "$boot" | cut -d ' ' -f1)" = 5c2c58602d688c636ea1c9801e6106f4 ]
. "$case_dir/fixture.sh"
cp "$boot" "$out/boot.img"
mdel -i "$out/boot.img" ::4201.CPI
mcopy -i "$boot" ::FIND.EXE "$out/FIND.EXE"
[ "$(sha256sum "$out/FIND.EXE" | cut -d ' ' -f1)" = 513f58937c7713572d8f204ce3f8a769c93d35c5e66c008f21b4fa54af5ccfa4 ]
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
find_fixture
find_batches reference
nasm -f bin "$case_dir/execute.asm" -o "$out/EXECUTE.COM"
mcopy -i "$out/data.img" "$out/EXECUTE.COM" ::
{
    printf '%s\r\n' '@echo off' 'ver >B:\VERSION.TXT' 'B:'
    call_find_batches
    printf '%s\r\n' 'echo CAPTURE-DONE'
} > "$out/AUTOEXEC.BAT"
printf 'files=20\r\nbuffers=20\r\n' > "$out/CONFIG.SYS"
mcopy -i "$out/boot.img" "$out/AUTOEXEC.BAT" "$out/CONFIG.SYS" ::
printf 'wait CAPTURE-DONE\nshot complete\nend\n' > "$out/steps"
sed 's/i<80/i<400/' "$root/goldens/dataloss/drive.c" > "$out/drive.c"
cc -std=c11 -O2 -Wall -Wextra -Werror "$out/drive.c" -o "$out/drive"
"$out/drive" "$out" "$out/boot.img" "$out/data.img" - "$out/steps"
mkdir "$out/results"
mcopy -i "$out/data.img" '::*.OUT' '::*.RC' '::*.ERR' ::VERSION.TXT "$out/results/"
