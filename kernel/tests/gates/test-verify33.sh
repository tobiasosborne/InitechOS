set -eu
. "$SOURCE_ROOT/kernel/tests/assertions.sh"
capture=$SOURCE_ROOT/kernel/tests/fixtures/verify33
assert sh -c "cd '$capture' && sha256sum -c --quiet SHA256SUMS"
w=build/verify33
mkdir -p "$w"
rm -f "$w/b.img"
truncate -s 1474560 "$w/b.img"
mformat -i "$w/b.img" -f 1440 ::
assert build/test_verify33 "$w/b.img" "$w/res.bin"
assert cmp "$capture/dos33-res.bin" "$w/res.bin"
mtype -i "$w/b.img" ::GOOD.TXT > "$w/good.txt"
printf data > "$w/expected.txt"
assert cmp "$w/expected.txt" "$w/good.txt"
finish
