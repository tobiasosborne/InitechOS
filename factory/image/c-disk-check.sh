#!/bin/sh
# Independent check of the C: system disk and the A: data volume minted by
# build.sh (initech-tlhqy): sfdisk and mtools read the bytes, fsck.fat judges
# both volumes, and the manifests name the independent source bytes. Usage: c-disk-check.sh [DIR]
set -eu
cd "$(dirname "$0")/../.."
d=${1:-build/image}
export LC_ALL=C MTOOLS_SKIP_CHECK=1
. factory/testlib/shell-checks.sh
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
c=$d/c.img a=$d/a-data.img
[ "$(wc -c < "$c")" -eq $((20160*512)) ] || fail "c-disk: c.img is not 20160 sectors"; ok
sfdisk -d "$c" 2>/dev/null | grep -E '^[^#].*start=' > "$work/parts" || fail "c-disk: no partition table"
[ "$(wc -l < "$work/parts")" -eq 1 ] || fail "c-disk: exactly one partition"; ok
grep -Eq 'start= *63, size= *16384, type=1, bootable' "$work/parts" || fail "c-disk: partition [$(cat "$work/parts")]"; ok
# The partition occupies bytes 446--509 of the MBR. Its preceding code
# and the stage2 sectors following the MBR come from boot/'s build.
cmp -n 446 build/boot/mbr.bin "$c" || fail "c-disk: MBR code"; ok
dd if="$c" of="$work/stage2" bs=512 skip=1 count=16 status=none
cmp build/boot/stage2.bin "$work/stage2" || fail "c-disk: stage2 bytes"; ok
minfo -i "$c@@32256" > "$work/minfo"
for want in 'sector size: 512 bytes' 'fats: 2' 'cluster size: 8 sectors' 'small size: 16384 sectors' 'hidden sectors: 63' 'heads: 16' 'sectors per track: 63'; do
 grep -Fqx "$want" "$work/minfo" || fail "c-disk: BPB lacks [$want]"; ok
done
build/factory/boot_system "$c" check build/boot/IO.SYS "${SYSTEM_IMAGE:-build/system/INITDOS.SYS}" > /dev/null || fail "c-disk: system file layout or payload"; ok
for volume in "$c@@32256" "$a"; do
 label=$(mlabel -i "$volume" -s :: | awk '{print $NF}')
 case $volume in "$a") expected=DATA;; *) expected=INITECH;; esac
 [ "$label" = "$expected" ] || fail "c-disk: expected label $expected, got [$label]"; ok
done
free=$(mdir -i "$c@@32256" ::/ | awk '/bytes free/ {gsub(/ /,""); sub(/bytesfree/,""); print}')
[ "$free" -ge $((2*1024*1024)) ] || fail "c-disk: only $free bytes free on C:"; ok
# Include hidden/system entries and directory entries in the inventory.
# The manifest describes nested paths. Every prefix names an installed directory.
awk '
 BEGIN {print "::/IO.SYS"; print "::/INITDOS.SYS"}
 !/^#/ && NF {
  path="::/"; n=split(toupper($2),part,"/")
  for(i=1;i<n;i++) {path=path part[i] "/"; if (!dirs[path]++) print path}
  print path part[n]
 }' system/working-volume.list > "$work/c.paths"
sort "$work/c.paths" > "$work/c.want"
mdir -/ -a -b -i "$c@@32256" ::/ > "$work/c.list"
sort "$work/c.list" > "$work/c.tree"
diff -u "$work/c.want" "$work/c.tree" || fail "c-disk: C: inventory"; ok
while read -r source destination rest; do
 case $source in ""|\#*) continue;; esac
 case $destination in
  CONFIG.SYS|AUTOEXEC.BAT) source=build/system/installed/c-boot/$destination;;
 esac
 mcopy -o -i "$c@@32256" "::/$destination" "$work/file" || fail "c-disk: cannot read $destination"
 cmp -s "$source" "$work/file" || fail "c-disk: C:$destination differs from $source"
done < system/working-volume.list; ok
# A: holds exactly the data manifest, with its attributes.
mdir -/ -b -a -i "$a" ::/ > "$work/data.list"
sort "$work/data.list" > "$work/data.tree"
awk '!/^#/ && NF==3 {print "::/" toupper($2)}' system/data-volume.list | sort > "$work/data.want"
cmp -s "$work/data.tree" "$work/data.want" || { diff "$work/data.want" "$work/data.tree"; fail "c-disk: A: content differs from system/data-volume.list"; }; ok
while read -r source destination attributes; do
 case "$source" in ""|\#*) continue;; esac
 mcopy -o -i "$a" "::/$destination" "$work/file"
 cmp -s "$source" "$work/file" || fail "c-disk: A:$destination bytes"
 actual=$(mattrib -i "$a" "::/$destination" | awk '{$NF="";gsub(/ /,"");print}')
 case "$attributes:$actual" in H:AH|A:A) ;; *) fail "c-disk: A:$destination attributes [$actual]";; esac
done < system/data-volume.list; ok
# IDB1 root-window record and little-endian origin from frame/geometry.tsv
# DRIVEA and flair/desktop/tests/test_frame_desktop.c.
mcopy -i "$a" ::DESKTOP.DB "$work/db"
seed=$(od -An -v -tx1 "$work/db" | tr -d ' \n')
[ "$seed" = '494442310100010004000000000000000000000000000000002e007300000000' ] ||
 fail "c-disk: DATA desktop seed, expected root window at 46,115"; ok
dd if="$c" of="$work/cvol" bs=512 skip=63 count=16384 status=none
fsck.fat -n "$work/cvol" > "$work/fsck.c" 2>&1 || { cat "$work/fsck.c"; fail "c-disk: fsck.fat C:"; }; ok
fsck.fat -n "$a" > "$work/fsck.a" 2>&1 || { cat "$work/fsck.a"; fail "c-disk: fsck.fat A:"; }; ok
echo "c-disk: C: 8 MiB system disk and A: data volume as manifested"
summary
