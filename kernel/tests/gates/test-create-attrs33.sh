set -eu
. "$SOURCE_ROOT/kernel/tests/assertions.sh"
capture=$SOURCE_ROOT/kernel/tests/fixtures/create-attrs33
assert sh -c "cd '$capture' && sha256sum -c --quiet SHA256SUMS"
w=build/create-attrs33
mkdir -p "$w"
rm -f "$w/b.img"
truncate -s 1474560 "$w/b.img"
mformat -i "$w/b.img" -f 1440 -N 12345678 ::
build/test_create_attrs33 "$w/b.img" "$w/result.bin"
# Row 8 (CX=08h) is the documented label deviation: refused with 5.
label=$((2 + 7 * 16))
assert cmp -n "$label" "$capture/dos33-result.bin" "$w/result.bin"
printf '\001\005\000' > "$w/label.want"
head -c 13 /dev/zero | tr '\000' '\356' >> "$w/label.want"
dd if="$w/result.bin" of="$w/label.got" bs=1 skip="$label" count=16 2>/dev/null
assert cmp "$w/label.want" "$w/label.got"
assert cmp -i "$((label + 16))" "$capture/dos33-result.bin" "$w/result.bin"
mattrib -i "$w/b.img" -/ :: > "$w/attrib.txt"
assert cmp "$capture/attrib.txt" "$w/attrib.txt"
for f in F00 F01 F02 F04 F07 F20 F27 FN3 FH1; do
    printf '%s ' "$f"; mtype -i "$w/b.img" "::$f" | od -An -tx1 | xargs
done > "$w/files.txt"
assert cmp "$capture/files.txt" "$w/files.txt"
assert fsck.fat -n "$w/b.img"
finish
