set -eu
. "$SOURCE_ROOT/kernel/tests/assertions.sh"
capture=$SOURCE_ROOT/kernel/tests/fixtures/find-dta33
assert sh -c "cd '$capture' && sha256sum -c --quiet SHA256SUMS"
w=build/find-dta33
mkdir -p "$w"
rm -f "$w/b.img"
sh "$capture/mint.sh" "$w/b.img"
cp "$w/b.img" "$w/before.img"
build/test_find_dta33 "$w/b.img" "$capture/dos33-result.bin"
assert cmp "$w/before.img" "$w/b.img"
finish
