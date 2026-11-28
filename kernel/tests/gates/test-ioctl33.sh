set -eu
. "$SOURCE_ROOT/kernel/tests/assertions.sh"
capture=$SOURCE_ROOT/kernel/tests/fixtures/ioctl33
assert sh -c "cd '$capture' && sha256sum -c --quiet SHA256SUMS"
w=build/ioctl33
mkdir -p "$w"
rm -f "$w/b.img"
truncate -s 1474560 "$w/b.img"
mformat -i "$w/b.img" -f 1440 ::
printf 'file\r\n' > "$w/FILE.TXT"
mcopy -i "$w/b.img" "$w/FILE.TXT" ::
build/test_ioctl33 "$w/b.img" "$w/res.bin"
od -An -tx1 "$capture/dos33-res.bin" > "$w/expected.txt"
od -An -tx1 "$w/res.bin" > "$w/actual.txt"
assert diff -u "$w/expected.txt" "$w/actual.txt"
finish
