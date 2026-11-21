#!/bin/sh
# Observe the genuine MORE on the same Microsoft DOS 3.30 disk as FIND.
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
mcopy -i "$boot" ::MORE.COM "$out/MORE.COM"
[ "$(sha256sum "$out/MORE.COM" | cut -d ' ' -f1)" = 9a71219dbcc0ebe9b082a1ee8450ca4ed7150ce069dfdd7dcdc15a641465bb06 ]
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
more_fixture
printf '%s\r\n' '@echo off' 'ver >B:\VERSION.TXT' 'B:' 'path A:\' 'prompt P$G' 'echo CAPTURE-READY' > "$out/AUTOEXEC.BAT"
printf 'files=20\r\nbuffers=20\r\n' > "$out/CONFIG.SYS"
mcopy -i "$out/boot.img" "$out/AUTOEXEC.BAT" "$out/CONFIG.SYS" ::
cat > "$out/steps" <<'STEPS'
wait P>
type more
key shift-comma
type pages.txt
key ret
wait PAGE-L24
sleep 0.4
shot first
sleep 0.4
shot still
key ret
wait PAGE-L47
sleep 0.4
shot second
key x
wait PAGE-L60
wait P>
shot after
type more
key shift-comma
type stop.txt
key ret
wait STOP-L24
sleep 0.4
shot cancel-wait
key ctrl-c
wait P>
shot cancelled
type type short.txt
key shift-backslash
type more
key shift-dot
type short.out
key ret
sleep 0.8
type type pages.txt
key shift-backslash
type more
key shift-dot
type pipe.out
key ret
sleep 1.5
shot pipe-wait
key ret
sleep 1.5
key ret
sleep 1.5
type echo PIPE-DONE
key ret
wait PIPE-DONE
shot pipe-after
end
STEPS
sed 's/i<80/i<200/' "$root/goldens/dataloss/drive.c" > "$out/drive.c"
cc -std=c11 -O2 -Wall -Wextra -Werror "$out/drive.c" -o "$out/drive"
"$out/drive" "$out" "$out/boot.img" "$out/data.img" - "$out/steps"
cmp "$out/first.vga" "$out/still.vga"
grep -Eq "^PIPE-DONE *$" "$out/pipe-after.txt"
mcopy -i "$out/data.img" ::SHORT.OUT ::PIPE.OUT ::VERSION.TXT "$out/"
