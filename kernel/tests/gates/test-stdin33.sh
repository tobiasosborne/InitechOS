set -eu
. "$SOURCE_ROOT/kernel/tests/assertions.sh"
capture=$SOURCE_ROOT/kernel/tests/fixtures/stdin33
assert sh -c "cd '$capture' && sha256sum -c --quiet SHA256SUMS"
w=build/stdin33
mkdir -p "$w"
rm -f "$w/b.img"
sh "$capture/mint.sh" "$w/b.img"
build/test_stdin33 "$w/b.img" "$w/res.bin"
assert cmp "$capture/dos33-res.bin" "$w/res.bin"
mtype -i "$w/b.img" ::ECHO.TXT > "$w/echo.txt"
assert cmp "$capture/dos33-echo.txt" "$w/echo.txt"
# Genuine DOS 3.30 never returns from these calls at end of a redirected file.
for ah in 01 07 08; do
    sh "$capture/mint.sh" "$w/eof.img"
    rc=0
    timeout 1 build/test_stdin33 "$w/eof.img" "$w/eof.bin" "$ah" || rc=$?
    assert test "$rc" -eq 124
    assert cmp "$capture/dos33-res.bin" "$w/eof.bin"
done
finish
