#!/bin/sh
# Independent check of the C: system disk and the A: data volume minted by
# build.sh (initech-tlhqy): sfdisk and mtools read the bytes, fsck.fat judges
# both FATs, and the expected content comes from the manifests and the
# legacy working volume, not from the builder. Usage: c-disk-check.sh [DIR]
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
minfo -i "$c@@32256" > "$work/minfo"
for want in 'cluster size: 8 sectors' 'small size: 16384 sectors' 'hidden sectors: 63' 'heads: 16' 'sectors per track: 63'; do
 grep -Fqx "$want" "$work/minfo" || fail "c-disk: BPB lacks [$want]"; ok
done
build/factory/boot_system "$c" check > /dev/null || fail "c-disk: IO.SYS/INITDOS.SYS root order, HSR or chains"; ok
free=$(mdir -i "$c@@32256" ::/ | awk '/bytes free/ {gsub(/ /,""); sub(/bytesfree/,""); print}')
[ "$free" -ge $((2*1024*1024)) ] || fail "c-disk: only $free bytes free on C:"; ok
# C: carries the working volume exactly: the same tree and bytes as data.img,
# except its own AUTOEXEC.BAT.
mdir -/ -b -i "$d/data.img" ::/ | sort > "$work/a.tree"
mdir -/ -b -i "$c@@32256" ::/ | sort > "$work/c.tree"
cmp -s "$work/a.tree" "$work/c.tree" || { diff "$work/a.tree" "$work/c.tree" | head; fail "c-disk: C: tree differs from the working volume"; }; ok
while read -r path; do
 case $path in */) continue;; esac
 [ "$path" = ::/AUTOEXEC.BAT ] && continue
 mcopy -n -i "$d/data.img" "$path" "$work/x" && mcopy -n -i "$c@@32256" "$path" "$work/y" || fail "c-disk: cannot read $path"
 cmp -s "$work/x" "$work/y" || fail "c-disk: $path differs on C:"
 rm -f "$work/x" "$work/y"
done < "$work/a.tree"; ok
while read -r source destination rest; do
 case "$source" in ""|\#*) continue;; esac
 grep -Fqix "::/$destination" "$work/c.tree" || fail "c-disk: $destination missing on C:"
done < system/working-volume.list; ok
printf '@ECHO OFF\r\nPATH C:\;C:\\APPS\r\nFLAIR\r\n' > "$work/autoexec"
mcopy -n -i "$c@@32256" ::/AUTOEXEC.BAT "$work/y"; cmp -s "$work/autoexec" "$work/y" || fail "c-disk: C:\\AUTOEXEC.BAT"; ok
# A: holds exactly the data manifest, with its attributes.
mdir -/ -b -a -i "$a" ::/ | sort > "$work/data.tree"
awk '!/^#/ && NF==3 {print "::/" toupper($2)}' system/data-volume.list | sort > "$work/data.want"
cmp -s "$work/data.tree" "$work/data.want" || { diff "$work/data.want" "$work/data.tree"; fail "c-disk: A: content differs from system/data-volume.list"; }; ok
while read -r source destination attributes; do
 case "$source" in ""|\#*) continue;; esac
 mcopy -n -i "$a" "::/$destination" "$work/x"; cmp -s "$source" "$work/x" || fail "c-disk: A:$destination bytes"; rm -f "$work/x"
 attr=$(mattrib -i "$a" "::/$destination" | cut -c1-8)
 case $attributes in H) case $attr in *H*) ;; *) fail "c-disk: A:$destination not hidden";; esac;; *) case $attr in *H*) fail "c-disk: A:$destination hidden";; esac;; esac
done < system/data-volume.list; ok
dd if="$c" of="$work/cvol" bs=512 skip=63 count=16384 status=none
fsck.fat -n "$work/cvol" > "$work/fsck.c" 2>&1 || { cat "$work/fsck.c"; fail "c-disk: fsck.fat C:"; }; ok
fsck.fat -n "$a" > "$work/fsck.a" 2>&1 || { cat "$work/fsck.a"; fail "c-disk: fsck.fat A:"; }; ok
echo "c-disk: C: 8 MiB system disk and A: data volume as manifested"
summary
