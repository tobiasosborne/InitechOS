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
    timeout 0.3 build/test_stdin33 "$w/eof.img" "$w/eof.bin" "$ah" || rc=$?
    assert test "$rc" -eq 124
    assert cmp "$capture/dos33-res.bin" "$w/eof.bin"
done
# Buffered input (buffered33.asm): bytes, editing and echo, then the wait.
sh "$capture/mint.sh" "$w/b0a.img"
build/test_stdin33 "$w/b0a.img" "$w/res0a.bin" 0a
assert cmp "$capture/dos33-res0a.bin" "$w/res0a.bin"
mtype -i "$w/b0a.img" ::ECHO0A.TXT > "$w/echo0a.txt"
assert cmp "$capture/dos33-echo0a.txt" "$w/echo0a.txt"
sh "$capture/mint.sh" "$w/eof0a.img"
rc=0
timeout 0.3 build/test_stdin33 "$w/eof0a.img" "$w/eof0a.bin" 0a-eof || rc=$?
assert test "$rc" -eq 124
assert cmp "$capture/dos33-res0a.bin" "$w/eof0a.bin"
# Status and flush-then-input (status33.asm).
sh "$capture/mint.sh" "$w/b0c.img"
build/test_stdin33 "$w/b0c.img" "$w/res0c.bin" 0c
assert cmp "$capture/dos33-res0c.bin" "$w/res0c.bin"
mtype -i "$w/b0c.img" ::ECHO0C.TXT > "$w/echo0c.txt"
assert cmp "$capture/dos33-echo0c.txt" "$w/echo0c.txt"
finish
