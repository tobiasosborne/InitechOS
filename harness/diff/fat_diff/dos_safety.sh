#!/bin/sh
# Thin mtools/QEMU differential. References: Microsoft MS-DOS 3.3 User's
# Reference pp. 50-51, 56; User's Guide pp. 12-13 (qualified *.* + prompt).
set -eu
mode=$1
boot=$2
name=dos_safety_$mode${3:-}
out=build/$name
mkdir -p "$out"
fail() { printf 'FAIL %s: %s\n' "$mode" "$*" >&2; exit 1; }
for tool in mformat mmd mcopy mdir mattrib mlabel fsck.fat; do
    command -v "$tool" >/dev/null || fail "missing $tool"
done
img=$out/data.img
dd if=/dev/zero of="$img" bs=512 count=2880 status=none
mformat -i "$img" -f 1440 ::
mlabel -i "$img" ::SAFETY
build/dos_safety_fixture pattern 8192 > "$out/self.bin"
build/dos_safety_fixture pattern 80928 > "$out/big.bin"
build/dos_safety_fixture pattern 114 > "$out/small.bin"
printf 'ROOT-SENTINEL\r\n' > "$out/root.txt"
printf 'SUB-SENTINEL\r\n' > "$out/sub.txt"
printf 'README-CONTENT\r\n' > "$out/readme.txt"
mcopy -i "$img" "$out/self.bin" ::SELF.BIN
mcopy -i "$img" "$out/big.bin" ::BIG.BIN
mcopy -i "$img" "$out/small.bin" ::SMALL.BIN
mcopy -i "$img" "$out/root.txt" ::KEEP.TXT
mcopy -i "$img" "$out/readme.txt" ::README.TXT
mmd -i "$img" ::FILLED ::EMPTY ::DELTEST ::SUB ::SUB/DEEP
mcopy -i "$img" "$out/root.txt" ::FILLED/SAVE.TXT
mcopy -i "$img" "$out/sub.txt" ::DELTEST/KEEP.TXT
mcopy -i "$img" "$out/self.bin" ::SUB/SELF.BIN
touch "$out/zero"
mcopy -i "$img" "$out/zero" ::ZERO1
mcopy -i "$img" "$out/zero" ::ZERO2
case "$mode" in
identity)
    cp -f "$img" "$out/before.img"
    "${DOS_SAFETY_HOST:-build/test_dos_safety}" "$img" identity || fail 'resolved entry identity'
    cmp -s "$img" "$out/before.img" || fail 'read-only identity changed disk'
    printf 'VERDICT: PASS -- test-dos-safety-identity (resolved directory + slot, disk unchanged)\n'
    exit 0
    ;;
k01)
    cat > "$out/commands.txt" <<'CMDS'
copy self.bin .\self.bin
dir
copy self.bin a:\self.bin
dir
copy big.bin a:\big.bin
copy small.bin a:\small.bin
copy self.bin \self.bin
copy self.bin sub\..\self.bin
cd sub
copy self.bin a:\sub\self.bin
copy self.bin .\deep\..\self.bin
copy self.bin \self.bin
exit
CMDS
    ;;
*) fail 'unknown mode' ;;
esac
keys=$(build/dos_safety_fixture keys < "$out/commands.txt")
build/qemu_harness --disk "$boot" --disk2 "$img" --name "$name" --out build \
    --timeout-ms 60000 --keys "$keys" --keys-after SHELL-READY \
    --quit-after SHELL-DONE 2> "$out/emu.report" || true
serial=build/$name.serial
[ -s "$serial" ] || fail 'missing serial'
! grep -q 'triple_fault=1' "$out/emu.report" || fail 'triple fault'
grep -q '^SHELL-READY' "$serial" || fail 'never entered shell'
grep -q '^SHELL-DONE' "$serial" || fail 'script did not finish'
tr -d '\r' < "$serial" > "$out/repl.txt"
bytes() {
    mcopy -i "$img" "::$1" - > "$out/extracted" 2> "$out/mcopy.log" || fail "$1 missing"
    cmp -s "$out/extracted" "$2" || fail "$1 bytes changed"
}
case "$mode" in
k01)
    bytes SELF.BIN "$out/self.bin"
    bytes BIG.BIN "$out/big.bin"
    bytes SMALL.BIN "$out/small.bin"
    bytes SUB/SELF.BIN "$out/self.bin"
    [ "$(grep -c 'File cannot be copied onto itself' "$out/repl.txt")" = 8 ] || fail 'expected eight resolved self refusals'
    grep -q '1 file(s) copied' "$out/repl.txt" || fail 'different parent copy refused'
    ;;
esac
fsck.fat -n "$img" > "$out/fsck.txt" 2>&1 || fail 'fsck.fat found damage'
printf 'VERDICT: PASS -- test-dos-safety-%s (booted shell + mtools bytes + fsck.fat)\n' "$mode"
