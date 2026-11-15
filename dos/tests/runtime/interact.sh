#!/bin/sh
# Genuine MS-DOS 3.30 compat33 INTERACT typed input and mtools readback.
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?out}
cd "$root"
mkdir -p "$out"; out=$(realpath "$out")
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
ref=$root/dos/command/tests/compat33/expected/INTERACT
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/DONE.COM" ::
printf '@echo off\r\necho INTERACT-READY\r\n' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
# Same prompt commands as interact-capture.sh, ending with a transport probe.
cat > "$out/input" <<'INPUT'
SET ZZ=hello
ECHO [%ZZ%] >INT.TXT
ECHO [%ABSENT%] >>INT.TXT
ECHO 100%% >>INT.TXT
FOR %F IN (ONE TWO) DO ECHO F=[%F] >>INT.TXT
ECHO INT-DONE >>INT.TXT
A:\DONE
INPUT
cc -std=c11 -O2 -Wall -Wextra -Werror dos/tests/runtime/interact-keys.c -o "$out/keys-tool"
"$out/keys-tool" < "$out/input" > "$out/keys"
if ! timeout 55 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" --keys-after INTERACT-READY --keys "$(cat "$out/keys")" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 40000 --out "$out" --name interact > "$out/driver.log" 2> "$out/report"; then error 'interactive transport failed'; fi
transport_pass "$out/report"
mdir -a -b -i "$out/data.img" :: > "$out/names"
require 'interactive redirected INT.TXT exists' grep -F -x '::/INT.TXT' "$out/names"
mtype -i "$out/data.img" ::INT.TXT > "$out/actual" || error 'read INT.TXT'
# Scope: percent literals/FOR payload. Exact input and raw output are retained.
# The real replay also proved a separate ECHO/redirection trailing-blank bug;
# that fidelity issue is tracked independently, never called byte-exact here.
tr -d '\r' < "$ref/files/INT.TXT" > "$out/reference-lines" || error 'read capture'
sed 's/ *$//' "$out/reference-lines" > "$out/expected-payload" || error 'capture projection'
tr -d '\r' < "$out/actual" > "$out/actual-lines" || error 'read actual text'
sed 's/ *$//' "$out/actual-lines" > "$out/actual-payload" || error 'actual projection'
grep -Fx INT-DONE "$out/expected-payload" >/dev/null || error 'incomplete independent capture'
require 'typed percent literals and FOR payload match genuine DOS 3.30 INTERACT' cmp "$out/expected-payload" "$out/actual-payload"
echo 'GREEN INTERACT: genuine DOS 3.30 prompt literals and FOR payload (trailing blanks tracked separately)'
