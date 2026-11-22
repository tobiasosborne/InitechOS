#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. factory/image/test-media.sh
mkdir -p build/smoke
out=${SMOKE_OUT:-$(mktemp -d "$PWD/build/smoke/run.XXXXXX")}
mkdir -p "$out"
echo "DOS smoke evidence: $out"
fail() { echo "FAIL $*" >&2; exit 1; }
line() { grep -Fx "$2" "$1" >/dev/null || fail "$3: missing line [$2]"; }
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
. factory/smoke-emulators.sh
prepare_shell() {
 mkdir -p "$out/$1"
 media_prepare "${FACTORY_MEDIA_PAIR:-canonical}" with-data "$out/$1" build/image "${SMOKE_BOOT:-}"
 if [ "$media_system" = A ]; then
  mdel -i "$media_work" ::AUTOEXEC.BAT
 else
  printf 'untouched ATA A\r\n' > "$out/$1/slave.want"
  mmd -i "$media_data" ::CHECK
  mcopy -i "$media_data" "$out/$1/slave.want" ::CHECK/PROOF.TXT
  cp "$media_data" "$media_dir/slave.before"
 fi
}
read_shell() {
 media_readback "$1" "$out/$1.report"
 if [ "$media_system" = C ]; then
  cmp "$media_dir/slave.before" "$media_data" || fail "$1 changed independent DATA"
 fi
}
qemu_smoke() {
build/factory/keys < factory/session.txt > "$out/keys"
prepare_shell qemu
# Deletion fixtures are independently known to exist before guest execution.
printf 'delete-me\r\n' > "$out/delete.txt"
mcopy -i "$media_work" "$out/delete.txt" ::DELETEME.TXT
mcopy -i "$media_work" ::DELETEME.TXT "$out/delete-before.txt"
cmp "$out/delete.txt" "$out/delete-before.txt"
mmd -i "$media_work" ::REMOVED
mdir -b -i "$media_work" ::/ > "$out/before.list"
grep -Fx '::/REMOVED/' "$out/before.list" >/dev/null || fail 'RD fixture missing before execution'
finish=$(printf '\nsmoke-complete\r\n')
# Raw transcript, fault log, emulator stderr and verdict are retained even green.
if ! media_run qemu build/factory/qemu_harness --keys-after SHELL-READY --keys "$(cat "$out/keys")" \
 --expect "$finish" --quit-after "$finish" --timeout-ms "${DOS_SMOKE_TIMEOUT_MS:-150000}" \
 --name qemu --out "$out" > "$out/qemu.stdout" 2> "$out/qemu.report"; then
 cat "$out/qemu.report"; fail "QEMU session did not complete";
fi
read_shell qemu
tr -d '\r' < "$out/qemu.serial" > "$out/qemu.txt"
line "$out/qemu.txt" 'InitechDOS  Version 3.30' 'QEMU banner'
line "$out/qemu.txt" 'InitechDOS Version 3.30' 'QEMU VER'
line "$out/qemu.txt" " Directory of $media_system:\\" 'QEMU DIR'
line "$out/qemu.txt" 'written-by-command' 'QEMU TYPE'
line "$out/qemu.txt" '        1 File(s) copied' 'QEMU COPY'
line "$out/qemu.txt" 'batch-ok' 'QEMU IF/GOTO'
line "$out/qemu.txt" 'apple' 'QEMU filters'
line "$out/qemu.txt" 'smoke-complete' 'QEMU completion'
grep -F 'bytes total disk space' "$out/qemu.txt" >/dev/null || fail 'QEMU CHKDSK did not run'
echo 'ok QEMU: banner, VER, DIR, directories, TYPE, COPY, REN, DEL, IF/GOTO, SORT, MORE, FIND, CHKDSK'
# Oracle is mtools, independently reading the guest's on-disk FAT/dir entries.
printf 'written-by-command\r\n' > "$out/expected.txt"
mcopy -o -i "$media_volume" ::CHECK/PROOF.TXT "$out/proof.txt"
cmp "$out/expected.txt" "$out/proof.txt" || fail 'mtools persisted file bytes'
for file in SAVED RENAMED; do
 mcopy -o -i "$media_volume" ::CHECK/$file.TXT "$out/$file.txt"
 cmp "$out/expected.txt" "$out/$file.txt" || fail "$file copied/renamed bytes"
 attr=$(mattrib -i "$media_volume" "::CHECK/$file.TXT" | awk '{$NF="";gsub(/ /,"");print}')
 [ "$attr" = A ] || fail "$file copied/renamed attributes [$attr]"
done
printf 'apple\r\nzebra\r\n' > "$out/expected-sort.txt"
mcopy -o -i "$media_volume" ::SORTED.TXT "$out/sorted.txt"
cmp "$out/expected-sort.txt" "$out/sorted.txt" || fail 'SORT file bytes'
mcopy -o -i "$media_volume" ::WORDS.TXT "$out/words.txt"
mcopy -o -i "$media_volume" ::MORE.TXT "$out/more.txt"
cmp "$out/words.txt" "$out/more.txt" || fail 'MORE file bytes'
printf 'apple\r\n' > "$out/expected-find.txt"
mcopy -o -i "$media_volume" ::FOUND.TXT "$out/found.txt"
cmp "$out/expected-find.txt" "$out/found.txt" || fail 'FIND file bytes'
# A failed listing is an I/O failure, never evidence of absence.
mdir -b -i "$media_volume" ::/ > "$out/after-root.list"
mdir -b -i "$media_volume" ::CHECK/ > "$out/after-check.list"
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
}
bochs_smoke() {
prepare_shell bochs
printf '%s\r\n' '@ECHO OFF' 'MD CHECK' 'ECHO bochs proof>CHECK\PROOF.TXT' > "$out/proof.bat"
mcopy -i "$media_work" "$out/proof.bat" ::PROOF.BAT
if ! media_run bochs build/factory/bochs_harness \
 --vga-ext none --keys-after SHELL-READY --keys ret,ret,v,e,r,ret,d,i,r,ret,p,r,o,o,f,ret,e,c,h,o,spc,b,o,c,h,s,ret \
 --expect bochs --quit-after bochs --timeout-ms "${DOS_SMOKE_TIMEOUT_MS:-45000}" --renew-ms 30000 --name bochs --out "$out" \
 > "$out/bochs.stdout" 2> "$out/bochs.report"; then
 cat "$out/bochs.report"; fail 'Bochs session did not complete';
fi
read_shell bochs
printf 'bochs proof\r\n' > "$out/bochs.want"
mcopy -o -i "$media_volume" ::CHECK/PROOF.TXT "$out/bochs-proof.txt"
cmp "$out/bochs.want" "$out/bochs-proof.txt" || fail 'Bochs persisted proof'
tr -d '\r' < "$out/bochs.serial" > "$out/bochs.txt"
line "$out/bochs.txt" 'InitechDOS  Version 3.30' 'Bochs banner'
line "$out/bochs.txt" 'InitechDOS Version 3.30' 'Bochs VER'
line "$out/bochs.txt" " Directory of $media_system:\\" 'Bochs DIR'
line "$out/bochs.txt" 'bochs' 'Bochs completion'
grep -E '^MORE +COM' "$out/bochs.txt" >/dev/null || fail 'Bochs DIR contents'
line "$out/bochs.txt" 'VGA13' 'Bochs VGA fallback'
echo 'ok Bochs: VGA fallback, banner, prompt, VER and DIR'
}
run_emulator() { "${1}_smoke"; }
smoke_emulators qemu bochs
echo 'SMOKE: 4 checks, 0 failures (QEMU + Bochs, 8 MiB)'
