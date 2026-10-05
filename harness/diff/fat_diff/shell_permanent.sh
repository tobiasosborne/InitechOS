#!/bin/sh
# Ref: MS-DOS_3.3_Users_Guide_198707.pdf, Reference pp. 46-47, 66;
# IBM 6138519_DOS_3.10_Reference_Feb85.pdf pp. 7-53/7-54.
set -eu
boot=$1
name=shell_permanent${2:-}
out=build/$name
mkdir -p "$out"
fail() { printf 'FAIL shell-permanent: %s\n' "$*" >&2; exit 1; }
img=$out/data.img
dd if=/dev/zero of="$img" bs=512 count=2880 status=none
mformat -i "$img" -f 1440 ::
printf 'SHELL=COMMAND.COM /p /E:512\r\n' > "$out/config.sys"
cat > "$out/autoexec.bat" <<'BAT'
@echo off
exit
echo autoexec survived
if exist CONFIG.SYS exit
echo if survived
for %%f in (CONFIG.SYS) do exit
echo for survived
exit > EXIT.LOG
echo redirected exit survived
BAT
mcopy -i "$img" "$out/config.sys" ::CONFIG.SYS
mcopy -i "$img" "$out/autoexec.bat" ::AUTOEXEC.BAT
cp -f "$img" "$out/before.img"
keys=$(printf 'exit\nver\nexit\nver\necho primary survived\n' | build/dos_safety_fixture keys)
build/qemu_harness --disk "$boot" --disk2 "$img" --name "$name" --out build \
    --keys "$keys" --keys-after SHELL-READY --timeout-ms 15000 \
    --quit-after 'primary survived' > "$out/emu.report" 2>&1 || true
tr -d '\r' < "build/$name.serial" > "$out/repl.txt"
! grep -qE '^SHELL-(EXIT|DONE)$' "$out/repl.txt" || fail 'primary processor exited'
for marker in 'autoexec survived' 'if survived' 'for survived' 'redirected exit survived' 'primary survived'; do
    grep -qx "$marker" "$out/repl.txt" || fail "$marker missing"
done
[ "$(grep -cxF 'InitechDOS Version 3.30' "$out/repl.txt")" = 2 ] || fail 'VER did not run twice after EXIT'
grep -q 'OK=1' "$out/emu.report" || fail 'emulator did not complete'
mcopy -i "$img" ::AUTOEXEC.BAT - > "$out/autoexec.got"
cmp -s "$out/autoexec.bat" "$out/autoexec.got" || fail 'batch changed'
mcopy -i "$img" ::EXIT.LOG - > "$out/exit.got"
[ ! -s "$out/exit.got" ] || fail 'primary EXIT emitted output under redirection'
fsck.fat -n "$img" > "$out/fsck.txt" 2>&1 || fail 'fsck.fat found damage'
printf 'VERDICT: PASS -- test-shell-permanent (AUTOEXEC/IF/FOR/redirected/interactive EXIT ignored, VER live, mtools + fsck.fat)\n'
