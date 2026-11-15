#!/bin/sh
# MS-DOS 3.3 Reference p.56 / PDF141: a directory operand deletes its files.
# Expected bytes/attributes come from mtools, not the shell's listing.
set -eu
check_suite=del-directory
. dos/tests/assertions.sh
mode=$1
runner=$2
tag=${3:-clean}
out=build/del-directory-$mode-$tag
mkdir -p "$out"
img=$out/data.img
dd if=/dev/zero of="$img" bs=512 count=2880 status=none
mformat -i "$img" -f 1440 ::
printf 'SHELL=COMMAND.COM /E:512\r\n' > "$out/config"
mcopy -i "$img" "$out/config" ::CONFIG.SYS
printf 'ROOT-SENTINEL\r\n' > "$out/root"
printf 'CHILD-SENTINEL\r\n' > "$out/child"
printf 'ORDINARY-FILE\r\n' > "$out/file"
mmd -i "$img" ::T ::T/DELDIR ::T/DELDIR/NEST
mcopy -i "$img" "$out/root" ::ONE.TXT
mcopy -i "$img" "$out/child" ::T/DELDIR/NEST/ONE.TXT
for leaf in LOCK.TXT HIDDEN.TXT SYSTEM.TXT; do
    mcopy -i "$img" "$out/child" "::T/DELDIR/$leaf"
done
mattrib -i "$img" +r ::T/DELDIR/LOCK.TXT
mattrib -i "$img" +h ::T/DELDIR/HIDDEN.TXT
mattrib -i "$img" +s ::T/DELDIR/SYSTEM.TXT
i=0
while [ "$i" -lt 300 ]; do
    leaf=$(printf 'F%03d.TXT' "$i")
    mcopy -i "$img" "$out/file" "::T/DELDIR/$leaf"
    i=$((i+1))
done
cp -f "$img" "$out/before.img"
printf 'cd t\ndel deldir\n' > "$out/commands"
printf 'n\n' > "$out/answers"
run() {
    if [ "$mode" = host ]; then
        "$runner" "$img" "$out/commands" < "$out/answers" > "$out/text" 2>&1 || { rc=$?; cat "$out/text"; exit "$rc"; }
    else
        cat "$out/commands" "$out/answers" > "$out/keys.txt"
        printf 'exit\n' >> "$out/keys.txt"
        keys=$(build/dos_safety_fixture keys < "$out/keys.txt")
        build/qemu_harness --disk "$runner" --disk2 "$img" --name "del-directory-$mode-$tag" --out "$out" \
            --timeout-ms 60000 --keys "$keys" --keys-after SHELL-READY --quit-after SHELL-DONE > "$out/report" 2>&1 || true
        tr -d '\r' < "$out/del-directory-$mode-$tag.serial" > "$out/text"
        check 'QEMU did not finish' grep -q SHELL-DONE "$out/text"
    fi
}
run
check 'N changed disk bytes' cmp -s "$img" "$out/before.img"
check 'directory confirmation missing' grep -qF 'Are you sure (Y/N)?' "$out/text"
# Retain the complete non-Y disk assertion; then approve a qualified dotted path.
printf 'cd t\nerase a:\\t\\.\\deldir\\\n' > "$out/commands"
printf 'y\n' > "$out/answers"
run
check 'qualified directory confirmation missing' grep -qF 'Are you sure (Y/N)?' "$out/text"
if mdir -i "$img" '::T/DELDIR/F*.TXT' > "$out/list" 2>&1; then fail 'eligible immediate files remain'; fi
bytes() {
    read_product "$img" "$1" "$out/actual"
    check "$1 bytes changed" cmp -s "$out/actual" "$2"
}
bytes ONE.TXT "$out/root"
bytes T/DELDIR/NEST/ONE.TXT "$out/child"
for leaf in LOCK.TXT HIDDEN.TXT SYSTEM.TXT; do bytes "T/DELDIR/$leaf" "$out/child"; done
mattrib -i "$img" ::T/DELDIR/LOCK.TXT > "$out/attributes"
check 'read-only attribute preserved' grep -q R "$out/attributes"
mattrib -i "$img" ::T/DELDIR/HIDDEN.TXT > "$out/attributes"
check 'hidden attribute preserved' grep -q H "$out/attributes"
mattrib -i "$img" ::T/DELDIR/SYSTEM.TXT > "$out/attributes"
check 'system attribute preserved' grep -q S "$out/attributes"
mdir -i "$img" ::T/DELDIR/NEST > "$out/nest" || fail 'nested directory removed'
check_log 'FAT damage' "$out/fsck" fsck.fat -n "$img"
echo "GREEN del-directory-$mode: N whole disk unchanged; Y 300 immediate files only; protected/nested bytes + fsck.fat"
complete
