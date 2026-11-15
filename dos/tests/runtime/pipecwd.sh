#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?output}
cd "$root"
mkdir -p "$out"; out=$(realpath "$out")
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
mmd -i "$out/data.img" ::START ::SUB
nasm -f bin dos/command/tests/compat33/stream.asm -o "$out/PASS.COM"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/PASS.COM" "$out/DONE.COM" ::
printf '@echo off\r\ntime 12:34:10\r\ncd START\r\necho hello|A:\\PASS >A:\\ONE.OUT\r\ncd ..\\SUB|cd >A:\\CWD.OUT\r\ncd >A:\\AFTER.OUT\r\nA:\\DONE\r\n' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
if ! timeout 45 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 25000 --out "$out" --name pipecwd > "$out/driver.log" 2> "$out/report"; then error 'pipe CWD transport failed'; fi
transport_pass "$out/report"
mdir -b -i "$out/data.img" ::START > "$out/remaining"
require 'pipe cleanup follows original directory' test ! -s "$out/remaining"
for name in ONE CWD AFTER; do
 mtype -i "$out/data.img" "::$name.OUT" > "$out/actual" || error 'pipe output missing'
 # Same single data volume: B: on the reference machine, A: on the product.
 sed 's/B:\\/A:\\/' "${PIPE_REFERENCE:-$root/dos/command/tests/dos33/reference/pipe-cwd}/$name.OUT" > "$out/expected"
 require "pipe $name output matches captured DOS33" cmp "$out/expected" "$out/actual"
done
echo 'GREEN pipe CWD: captured contents/directories and no stranded owned files'
