#!/bin/sh
# Two FAT12 volumes minted by mtools with \SUB first on each, so both SUB
# directories occupy the same physical cluster; disjoint payloads. After
# the binding runs with A: identities, mtools must find every A: change on
# A: and C: must differ from its original only by the one system-root NEWC.
set -eu
export SOURCE_DATE_EPOCH=699350400 TZ=UTC
bin=$1 w=$2
mkdir -p "$w"
for v in c a; do
 image=$w/$v.img
 if [ "$v" = c ]; then sectors=16384 spc=8 P=C; else sectors=2880 spc=1 P=A; fi
 truncate -s 0 "$image"; truncate -s $((sectors*512)) "$image"
 mformat -i "$image" -T "$sectors" -h 16 -s 63 -c "$spc" -N 0x19910330 ::
 mmd -i "$image" ::SUB
 printf '%s payload\r\n' "$P" > "$w/$v-file.txt"
 printf '%s gone\r\n' "$P" > "$w/$v-gone.txt"
 printf '%s only\r\n' "$P" > "$w/$v-only.txt"
 mcopy -i "$image" "$w/$v-file.txt" ::SUB/FILE.TXT
 mcopy -i "$image" "$w/$v-gone.txt" ::SUB/GONE.TXT
 mcopy -i "$image" "$w/$v-only.txt" "::${P}ONLY.TXT"
 cp "$image" "$w/$v.before"
done
"$bin" "$w/c.img" "$w/a.img"
absent() { if mdir -i "$1" "::$2" >/dev/null 2>&1; then echo "FAIL $2 present on $1" >&2; exit 1; fi; }
# A: received every A: operation.
mdir -i "$w/a.img" ::NEWA >/dev/null
mtype -i "$w/a.img" ::SUB/NEW.TXT > "$w/a.new"
printf 'written on A\r\n' | cmp - "$w/a.new"
mtype -i "$w/a.img" ::SUB/AONLY.TXT | cmp "$w/a-only.txt" -
absent "$w/a.img" AONLY.TXT
absent "$w/a.img" SUB/GONE.TXT
mtype -i "$w/a.img" ::SUB/FILE.TXT | cmp "$w/a-file.txt" -
absent "$w/a.img" NEWC
# C: holds only its own NEWC; every C: payload is intact.
mdir -i "$w/c.img" ::NEWC >/dev/null
for f in NEWA SUB/AONLY.TXT SUB/NEW.TXT; do absent "$w/c.img" "$f"; done
mtype -i "$w/c.img" ::CONLY.TXT | cmp "$w/c-only.txt" -
mtype -i "$w/c.img" ::SUB/FILE.TXT | cmp "$w/c-file.txt" -
mtype -i "$w/c.img" ::SUB/GONE.TXT | cmp "$w/c-gone.txt" -
fsck.fat -n "$w/a.img" >/dev/null
echo "ok Finder binding: A: operations on A:, C: only its system-root folder"
