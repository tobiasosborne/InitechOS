#!/bin/sh
# Thin mtools/QEMU differential. References: Microsoft MS-DOS 3.3 User's
# Reference pp. 50-51, 56; User's Reference pp. 12-13 (qualified *.* + prompt).
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
: > "$out/zero"
mcopy -i "$img" "$out/zero" ::ZERO1
mcopy -i "$img" "$out/zero" ::ZERO2
# These EXIT-ending traces exercise an explicitly nonpermanent processor.
# Ref: MS-DOS_3.3_Users_Guide_198707.pdf, User's Reference pp. 46, 66.
printf 'SHELL=COMMAND.COM /E:512\r\n' > "$out/config.sys"
mcopy -i "$img" "$out/config.sys" ::CONFIG.SYS
case "$mode" in
create-ro|unlink-ro|write-ro|copy-ro|del-ro)
    for dir in '' 'SUB/'; do
        mcopy -i "$img" "$out/self.bin" "::${dir}LOCK.BIN"
        mcopy -i "$img" "$out/zero" "::${dir}LOCK0.TXT"
        mattrib -i "$img" +r "::${dir}LOCK.BIN" "::${dir}LOCK0.TXT"
    done
    cp -f "$img" "$out/before.img"
    if [ "$mode" = create-ro ] || [ "$mode" = unlink-ro ] || [ "$mode" = write-ro ]; then
        "${DOS_SAFETY_HOST:-build/test_dos_safety}" "$img" "$mode" || fail 'host protection check'
        cmp -s "$img" "$out/before.img" || fail 'refused protection operation changed disk'
        fsck.fat -n "$img" > "$out/fsck.txt" 2>&1 || fail 'fsck.fat found damage'
        printf 'VERDICT: PASS -- test-dos-safety-%s (INT21 refusal, whole disk unchanged, fsck.fat)\n' "$mode"
        exit 0
    fi
    if [ "$mode" = copy-ro ]; then
        verb='copy readme.txt'
    else
        verb=del
    fi
    printf '%s lock.bin\n%s lock0.txt\n%s sub\\lock.bin\n%s sub\\lock0.txt\nexit\n' \
        "$verb" "$verb" "$verb" "$verb" > "$out/commands.txt"
    ;;
k15n)
    cp -f "$img" "$out/before.img"
    cat > "$out/commands.txt" <<'CMDS'
del *.*
n
del *.*
x
del *.*

erase a:\deltest\*.*
n
del *.*
yes
exit
CMDS
    ;;
k15y)
    i=0
    while [ "$i" -lt 20 ]; do
        leaf=$(printf 'Z%02d.TXT' "$i")
        mcopy -i "$img" "$out/sub.txt" "::DELTEST/$leaf"
        i=$((i + 1))
    done
    mcopy -i "$img" "$out/root.txt" ::EMPTY/KEEP.TXT
    mmd -i "$img" ::EMPTY/CHILD
    cat > "$out/commands.txt" <<'CMDS'
del deltest\*.*
y
cd empty
del *.*
y
dir
exit
CMDS
    ;;
audit|failure)
    i=0
    while [ "$i" -lt 20 ]; do
        leaf=$(printf 'Z%02d.TXT' "$i")
        mcopy -i "$img" "$out/sub.txt" "::DELTEST/$leaf"
        i=$((i + 1))
    done
    if [ "$mode" = audit ]; then
        cat > "$out/commands.txt" <<'CMDS'
copy self.bin .\self.bin
dir
copy self.bin a:\self.bin
dir
type filled\save.txt
copy readme.txt filled
cd filled
type save.txt
cd ..
type keep.txt
type deltest\keep.txt
del deltest\*.txt
type keep.txt
type deltest\keep.txt
exit
CMDS
    else
        cp -f "$img" "$out/before.img"
        printf 'del deltest\\*.txt\nexit\n' > "$out/commands.txt"
    fi
    ;;
k03)
    mcopy -i "$img" "$out/sub.txt" ::SUB/KEEP.TXT
    mcopy -i "$img" "$out/sub.txt" ::SUB/DEEP/KEEP.TXT
    cat > "$out/commands.txt" <<'CMDS'
type keep.txt
type deltest\keep.txt
del deltest\*.txt
type keep.txt
type deltest\keep.txt
del a:\sub\deep\*.txt
cd sub
del .\deep\..\*.txt
exit
CMDS
    ;;
k04)
    : > "$out/commands.txt"
    for count in 0 1 16 17 20 129; do
        dir=$(printf 'D%03d' "$count")
        mmd -i "$img" "::$dir"
        mcopy -i "$img" "$out/root.txt" "::$dir/KEEP.TXT"
        i=0
        while [ "$i" -lt "$count" ]; do
            leaf=$(printf 'Z%03d.TXT' "$i")
            mcopy -i "$img" "$out/sub.txt" "::$dir/$leaf"
            i=$((i + 1))
        done
        printf 'cd %s\ndel z*.txt\ndir\ncd ..\n' "$dir" >> "$out/commands.txt"
    done
    printf 'exit\n' >> "$out/commands.txt"
    ;;
identity|create)
    cp -f "$img" "$out/before.img"
    "${DOS_SAFETY_HOST:-build/test_dos_safety}" "$img" "$mode" || fail 'host safety check'
    cmp -s "$img" "$out/before.img" || fail 'rejected operation changed disk'
    fsck.fat -n "$img" > "$out/fsck.txt" 2>&1 || fail 'fsck.fat found damage'
    printf 'VERDICT: PASS -- test-dos-safety-%s (real INT21/FAT stack, whole disk unchanged)\n' "$mode"
    exit 0
    ;;
k02)
    cat > "$out/commands.txt" <<'CMDS'
type filled\save.txt
copy readme.txt filled
cd filled
type save.txt
cd ..
copy readme.txt empty
copy a:\readme.txt a:\filled\
copy filled\readme.txt filled
copy readme.txt .\filled
copy readme.txt sub\deep
copy readme.txt a:\
exit
CMDS
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
absent() {
    if mdir -i "$img" "::$1" > "$out/mdir.txt" 2>&1; then fail "$1 still present"; fi
}
case "$mode" in
copy-ro|del-ro)
    cmp -s "$img" "$out/before.img" || fail 'read-only refusal changed disk'
    [ "$(grep -cF 'Access denied' "$out/repl.txt")" = 4 ] || fail 'four access denied messages missing'
    if [ "$mode" = copy-ro ]; then
        mattrib -i "$img" -r ::LOCK.BIN ::LOCK0.TXT ::SUB/LOCK.BIN ::SUB/LOCK0.TXT
        # Repeat the exact COPY commands after explicit unprotection.
    else
        mattrib -i "$img" -r ::LOCK0.TXT ::SUB/LOCK0.TXT
        printf 'del lock*.*\ndel sub\\lock*.*\nexit\n' > "$out/commands.txt"
    fi
    control=${name}_control
    keys=$(build/dos_safety_fixture keys < "$out/commands.txt")
    build/qemu_harness --disk "$boot" --disk2 "$img" --name "$control" --out build \
        --timeout-ms 60000 --keys "$keys" --keys-after SHELL-READY \
        --quit-after SHELL-DONE > "$out/control.report" 2>&1 || fail 'unprotect control did not finish'
    if [ "$mode" = copy-ro ]; then
        for leaf in LOCK.BIN LOCK0.TXT SUB/LOCK.BIN SUB/LOCK0.TXT; do bytes "$leaf" "$out/readme.txt"; done
        [ "$(grep -c '1 file(s) copied' "build/$control.serial")" = 4 ] || fail 'unprotect COPY did not succeed'
    else
        for leaf in LOCK.BIN SUB/LOCK.BIN; do
            bytes "$leaf" "$out/self.bin"
            mattrib -i "$img" "::$leaf" | grep -q 'R' || fail 'protected survivor lost +R'
        done
        absent LOCK0.TXT
        absent SUB/LOCK0.TXT
        grep -qF 'Access denied' "build/$control.serial" || fail 'wildcard protection diagnostic missing'
    fi
    ;;
k15n)
    cmp -s "$img" "$out/before.img" || fail 'non-Y response changed disk'
    [ "$(grep -cF 'Are you sure (Y/N)?' "$out/repl.txt")" = 5 ] || fail 'confirmation prompt missing'
    ;;
k15y)
    bytes KEEP.TXT "$out/root.txt"
    absent 'DELTEST/*.TXT'
    absent EMPTY/KEEP.TXT
    mdir -i "$img" ::EMPTY/CHILD > "$out/child.txt" 2>&1 || fail 'child directory deleted'
    [ "$(grep -cF 'Are you sure (Y/N)?' "$out/repl.txt")" = 2 ] || fail 'confirmation prompt missing'
    ;;
audit)
    bytes SELF.BIN "$out/self.bin"
    bytes FILLED/SAVE.TXT "$out/root.txt"
    bytes FILLED/README.TXT "$out/readme.txt"
    bytes KEEP.TXT "$out/root.txt"
    absent 'DELTEST/*.TXT'
    [ "$(grep -c 'File cannot be copied onto itself' "$out/repl.txt")" = 2 ] || fail 'self refusal missing'
    ;;
failure)
    cmp -s "$img" "$out/before.img" || fail 'failed unlink changed disk'
    grep -q 'Access denied' "$out/repl.txt" || fail 'deletion failure diagnostic missing'
    grep -qF 'DELTEST\KEEP.TXT' "$out/repl.txt" || fail 'first remainder undisclosed'
    grep -qF 'DELTEST\Z19.TXT' "$out/repl.txt" || fail 'last remainder undisclosed'
    ;;
k03)
    bytes KEEP.TXT "$out/root.txt"
    absent DELTEST/KEEP.TXT
    absent SUB/KEEP.TXT
    absent SUB/DEEP/KEEP.TXT
    bytes SUB/SELF.BIN "$out/self.bin"
    ;;
k04)
    for count in 0 1 16 17 20 129; do
        dir=$(printf 'D%03d' "$count")
        bytes "$dir/KEEP.TXT" "$out/root.txt"
        absent "$dir/Z*.TXT"
    done
    ;;
k01)
    bytes SELF.BIN "$out/self.bin"
    bytes BIG.BIN "$out/big.bin"
    bytes SMALL.BIN "$out/small.bin"
    bytes SUB/SELF.BIN "$out/self.bin"
    [ "$(grep -c 'File cannot be copied onto itself' "$out/repl.txt")" = 8 ] || fail 'expected eight resolved self refusals'
    grep -q '1 file(s) copied' "$out/repl.txt" || fail 'different parent copy refused'
    ;;
k02)
    bytes FILLED/SAVE.TXT "$out/root.txt"
    bytes FILLED/README.TXT "$out/readme.txt"
    bytes EMPTY/README.TXT "$out/readme.txt"
    bytes SUB/DEEP/README.TXT "$out/readme.txt"
    bytes README.TXT "$out/readme.txt"
    [ "$(grep -c 'File cannot be copied onto itself' "$out/repl.txt")" = 2 ] || fail 'directory self destination not refused'
    ;;
esac
fsck.fat -n "$img" > "$out/fsck.txt" 2>&1 || fail 'fsck.fat found damage'
printf 'VERDICT: PASS -- test-dos-safety-%s (booted shell + mtools bytes + fsck.fat)\n' "$mode"
