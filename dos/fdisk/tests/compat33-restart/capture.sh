#!/bin/sh
set -eu
export LC_ALL=C TZ=UTC
root=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
boot=${1:?genuine DOS 3.30 disk 1}
out=${2:?new capture directory}
[ ! -e "$out" ] || { echo 'capture directory already exists' >&2; exit 2; }
[ "$(md5sum "$boot" | cut -d ' ' -f1)" = 5c2c58602d688c636ea1c9801e6106f4 ]
mkdir -p "$out"; out=$(realpath "$out")
cp "$boot" "$out/boot.img"
mcopy -i "$boot" ::FDISK.COM "$out/FDISK.COM"
[ "$(sha256sum "$out/FDISK.COM" | cut -d ' ' -f1)" = 7dec413ba8ffa5d7c8e825f481d5fcc3cdc8160011b072ed7c2834471a891a35 ]
mdel -i "$out/boot.img" ::4201.CPI
printf '@echo off\r\nver\r\nfdisk\r\necho FDISK-RETURNED\r\n' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/boot.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
truncate -s 20643840 "$out/fixed.img"
cat > "$out/steps" <<'EOF'
wait FDISK Options
shot menu
key 1
key ret
wait Create DOS Partition
key 1
key ret
wait Do you wish
shot maximum
key y
key ret
wait System will now restart
shot restart
end
EOF
sed 's/i<80/i<400/' "$root/goldens/dataloss/drive.c" > "$out/drive.c"
cc -std=c11 -O2 -Wall -Wextra -Werror "$out/drive.c" -o "$out/drive"
"$out/drive" "$out" "$out/boot.img" - "$out/fixed.img" "$out/steps"
sfdisk --dump "$out/fixed.img" > "$out/fixed.dump"
grep 'start= *63, size= *39249, type=4, bootable' "$out/fixed.dump"
cmp "$root/dos/fdisk/tests/compat33-restart/MENU.TXT" "$out/menu.txt"
cmp "$root/dos/fdisk/tests/compat33-restart/RESTART.TXT" "$out/restart.txt"
