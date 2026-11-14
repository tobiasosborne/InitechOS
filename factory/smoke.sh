#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. factory/testlib/smoke-lock.sh
smoke_lock
out=${SMOKE_OUT:-build/smoke}
mkdir -p "$out"
fail() { echo "FAIL $*" >&2; exit 1; }
line() { grep -Fx "$2" "$1" >/dev/null || fail "$3: missing line [$2]"; }
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
build/factory/keys < factory/session.txt > "$out/keys"
cp build/image/data.img "$out/qemu-data.img"
mdel -i "$out/qemu-data.img" ::AUTOEXEC.BAT
# Deletion fixtures are independently known to exist before guest execution.
printf 'delete-me\r\n' > "$out/delete.txt"
mcopy -i "$out/qemu-data.img" "$out/delete.txt" ::DELETEME.TXT
mcopy -i "$out/qemu-data.img" ::DELETEME.TXT "$out/delete-before.txt"
cmp "$out/delete.txt" "$out/delete-before.txt"
mmd -i "$out/qemu-data.img" ::REMOVED
mdir -b -i "$out/qemu-data.img" ::/ > "$out/before.list"
grep -Fx '::/REMOVED/' "$out/before.list" >/dev/null || fail 'RD fixture missing before execution'
finish=$(printf '\nsmoke-complete\r\n')
# Raw transcript, fault log, emulator stderr and verdict are retained even green.
if ! build/factory/qemu_harness --disk "${SMOKE_BOOT:-build/image/boot.img}" \
 --disk2 "$out/qemu-data.img" --keys-after SHELL-READY --keys "$(cat "$out/keys")" \
 --expect "$finish" --quit-after "$finish" --timeout-ms "${DOS_SMOKE_TIMEOUT_MS:-150000}" \
 --name qemu --out "$out" > "$out/qemu.stdout" 2> "$out/qemu.report"; then
 cat "$out/qemu.report"; fail "QEMU session did not complete";
fi
tr -d '\r' < "$out/qemu.serial" > "$out/qemu.txt"
line "$out/qemu.txt" 'InitechDOS  Version 3.30' 'QEMU banner'
line "$out/qemu.txt" 'InitechDOS Version 3.30' 'QEMU VER'
line "$out/qemu.txt" ' Directory of A:\' 'QEMU DIR'
line "$out/qemu.txt" 'written-by-command' 'QEMU TYPE'
line "$out/qemu.txt" '        1 File(s) copied' 'QEMU COPY'
line "$out/qemu.txt" 'batch-ok' 'QEMU IF/GOTO'
line "$out/qemu.txt" 'apple' 'QEMU filters'
line "$out/qemu.txt" 'smoke-complete' 'QEMU completion'
grep -F 'bytes total disk space' "$out/qemu.txt" >/dev/null || fail 'QEMU CHKDSK did not run'
echo 'ok QEMU: banner, VER, DIR, directories, TYPE, COPY, REN, DEL, IF/GOTO, SORT, MORE, FIND, CHKDSK'
# Oracle is mtools, independently reading the guest's on-disk FAT/dir entries.
printf 'written-by-command\r\n' > "$out/expected.txt"
mcopy -o -i "$out/qemu-data.img" ::CHECK/PROOF.TXT "$out/proof.txt"
cmp "$out/expected.txt" "$out/proof.txt" || fail 'mtools persisted file bytes'
for file in SAVED RENAMED; do
 mcopy -o -i "$out/qemu-data.img" ::CHECK/$file.TXT "$out/$file.txt"
 cmp "$out/expected.txt" "$out/$file.txt" || fail "$file copied/renamed bytes"
done
printf 'apple\r\nzebra\r\n' > "$out/expected-sort.txt"
mcopy -o -i "$out/qemu-data.img" ::SORTED.TXT "$out/sorted.txt"
cmp "$out/expected-sort.txt" "$out/sorted.txt" || fail 'SORT file bytes'
mcopy -o -i "$out/qemu-data.img" ::WORDS.TXT "$out/words.txt"
mcopy -o -i "$out/qemu-data.img" ::MORE.TXT "$out/more.txt"
cmp "$out/words.txt" "$out/more.txt" || fail 'MORE file bytes'
printf 'apple\r\n' > "$out/expected-find.txt"
mcopy -o -i "$out/qemu-data.img" ::FOUND.TXT "$out/found.txt"
cmp "$out/expected-find.txt" "$out/found.txt" || fail 'FIND file bytes'
# A failed listing is an I/O failure, never evidence of absence.
mdir -b -i "$out/qemu-data.img" ::/ > "$out/after-root.list"
mdir -b -i "$out/qemu-data.img" ::CHECK/ > "$out/after-check.list"
grep -Fx '::/EMPTY/' "$out/after-root.list" >/dev/null || fail 'MD did not persist EMPTY directory'
if grep -Fx '::/CHECK/COPY.TXT' "$out/after-check.list" >/dev/null; then fail 'REN left its source name'; fi
if grep -Fx '::/DELETEME.TXT' "$out/after-root.list" >/dev/null; then fail 'DEL left verified file'; fi
if grep -Fx '::/REMOVED/' "$out/after-root.list" >/dev/null; then fail 'RD left verified directory'; fi
if grep -E '^(batch-failed|Bad command or file name|Invalid drive specification|PANIC|File not found|Unable to create directory|Invalid path, not directory, or directory not empty)' "$out/qemu.txt" >/dev/null; then fail 'QEMU unexpected DOS error'; fi
echo 'ok mtools: copied/renamed bytes and source absence, verified DEL/RD fixtures, persisted MD, SORT/MORE/FIND'
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/qemu.report" >/dev/null || fail 'QEMU fault/reset verdict'
[ "$(grep -c 'CPU Reset' "$out/qemu.qemu.log")" -eq 2 ] || fail 'QEMU unexpected CPU reset'
if grep -Ei 'triple fault|check_exception old: 0x8' "$out/qemu.qemu.log" >/dev/null; then fail 'QEMU fault trace'; fi
echo 'ok QEMU: no triple fault, guest error or unexpected CPU reset'
cp build/image/data.img "$out/bochs-data.img"
mdel -i "$out/bochs-data.img" ::AUTOEXEC.BAT
if ! build/factory/bochs_harness --disk build/image/boot.img --disk2 "$out/bochs-data.img" \
 --vga-ext none --keys-after SHELL-READY --keys ret,ret,v,e,r,ret,d,i,r,ret,e,c,h,o,spc,b,o,c,h,s,ret \
 --expect bochs --quit-after bochs --timeout-ms "${DOS_SMOKE_TIMEOUT_MS:-45000}" --name bochs --out "$out" \
 > "$out/bochs.stdout" 2> "$out/bochs.report"; then
 cat "$out/bochs.report"; fail 'Bochs session did not complete';
fi
tr -d '\r' < "$out/bochs.serial" > "$out/bochs.txt"
line "$out/bochs.txt" 'InitechDOS  Version 3.30' 'Bochs banner'
line "$out/bochs.txt" 'InitechDOS Version 3.30' 'Bochs VER'
line "$out/bochs.txt" ' Directory of A:\' 'Bochs DIR'
line "$out/bochs.txt" 'bochs' 'Bochs completion'
grep -E '^MORE +COM' "$out/bochs.txt" >/dev/null || fail 'Bochs DIR contents'
line "$out/bochs.txt" 'VGA13' 'Bochs VGA fallback'
echo 'ok Bochs: VGA fallback, banner, prompt, VER and DIR'
echo 'SMOKE: 4 checks, 0 failures (QEMU + Bochs, 8 MiB)'
