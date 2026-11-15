set -eu
w=build/system-drive
mkdir -p "$w"
truncate -s $(((16384+71)*512)) "$w/c.img"
printf 'drive z: file="%s/%s/c.img" partition=1\n' "$PWD" "$w" > "$w/mtoolsrc"
export MTOOLSRC="$PWD/$w/mtoolsrc"
mpartition -I z:
mpartition -a -c -b 63 -l 16384 -T 1 z:
mformat -i "$w/c.img@@32256" -T 16384 -h 16 -s 63 -c 8 -H 63 ::
truncate -s 1474560 "$w/a.img"
mformat -i "$w/a.img" -f 1440 ::
for letter in c a; do
 image="$w/$letter.img"; directory=USER; text='data A bytes'; sub='data directory bytes'
 if test "$letter" = c; then image="$image@@32256"; directory=SYS; text='system C bytes'; sub='system directory bytes'; fi
 printf '%s\n' "$text" > "$w/root"
 printf '%s\n' "$sub" > "$w/doc"
 mcopy -i "$image" "$w/root" ::ROOT.BIN
 mmd -i "$image" "::$directory"
 mcopy -i "$image" "$w/doc" "::$directory/DOC.BIN"
done
truncate -s $((16384*512)) "$w/d.img"
mformat -i "$w/d.img" -T 16384 -h 16 -s 63 -c 1 ::
for letter in a c d; do cp "$w/$letter.img" "$w/$letter.before"; done
build/test_system_drive "$w/c.img" "$w/a.img" "$w/d.img"
for letter in a c d; do cmp "$w/$letter.before" "$w/$letter.img"; done
