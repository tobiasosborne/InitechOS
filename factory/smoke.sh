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
finish=$(printf '\nsmoke-complete\r\n')
# Raw transcript, fault log, emulator stderr and verdict are retained even green.
if ! build/factory/qemu_harness --disk "${SMOKE_BOOT:-build/image/boot.img}" \
 --disk2 "$out/qemu-data.img" --keys-after SHELL-READY --keys "$(cat "$out/keys")" \
 --expect "$finish" --quit-after "$finish" --timeout-ms 150000 \
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
if grep -E '^(batch-failed|Bad command or file name|Invalid drive specification|PANIC|File not found)' "$out/qemu.txt" >/dev/null; then fail 'QEMU unexpected DOS error'; fi
echo 'ok QEMU: banner, VER, DIR, directories, TYPE, COPY, REN, DEL, IF/GOTO, SORT, MORE, FIND, CHKDSK'
# Oracle is mtools, independently reading the guest's on-disk FAT/dir entries.
printf 'written-by-command\r\n' > "$out/expected.txt"
mcopy -o -i "$out/qemu-data.img" ::CHECK/PROOF.TXT "$out/proof.txt"
cmp "$out/expected.txt" "$out/proof.txt" || fail 'mtools persisted file bytes'
printf 'apple\r\nzebra\r\n' > "$out/expected-sort.txt"
mcopy -o -i "$out/qemu-data.img" ::SORTED.TXT "$out/sorted.txt"
cmp "$out/expected-sort.txt" "$out/sorted.txt" || fail 'SORT file bytes'
mcopy -o -i "$out/qemu-data.img" ::WORDS.TXT "$out/words.txt"
mcopy -o -i "$out/qemu-data.img" ::MORE.TXT "$out/more.txt"
cmp "$out/words.txt" "$out/more.txt" || fail 'MORE file bytes'
if mdir -i "$out/qemu-data.img" ::CHECK/RENAMED.TXT >/dev/null 2>&1; then fail 'DEL did not remove renamed copy'; fi
if mdir -i "$out/qemu-data.img" ::EMPTY >/dev/null 2>&1; then fail 'RD did not remove directory'; fi
echo 'ok mtools: written bytes, SORT order, MORE passthrough, REN/DEL and RD persisted'
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/qemu.report" >/dev/null || fail 'QEMU fault/reset verdict'
[ "$(grep -c 'CPU Reset' "$out/qemu.qemu.log")" -eq 2 ] || fail 'QEMU unexpected CPU reset'
if grep -Ei 'triple fault|check_exception old: 0x8' "$out/qemu.qemu.log" >/dev/null; then fail 'QEMU fault trace'; fi
echo 'ok QEMU: no triple fault, guest error or unexpected CPU reset'
cp build/image/data.img "$out/bochs-data.img"
mdel -i "$out/bochs-data.img" ::AUTOEXEC.BAT
if ! build/factory/bochs_harness --disk build/image/boot.img --disk2 "$out/bochs-data.img" \
 --vga-ext none --keys-after SHELL-READY --keys ret,ret,v,e,r,ret,d,i,r,ret,e,c,h,o,spc,b,o,c,h,s,ret \
 --expect bochs --quit-after bochs --timeout-ms 45000 --name bochs --out "$out" \
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
