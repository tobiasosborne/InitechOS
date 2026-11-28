#!/bin/sh
# Native utility-only route for the existing FIND/SORT/MORE pipe mutations.
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/transport.ok" "$out/assertion" "$out/result"
. "$root/dos/tests/runtime/verdict.sh"
. "$root/dos/tests/runtime/utility-session.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
printf 'zebra\r\napple\r\nDROP\r\n' > "$out/IN.TXT"
printf 'apple\r\nzebra\r\n' > "$out/expected"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$programs/FIND.COM" "$programs/SORT.COM" "$programs/MORE.COM" "$out/IN.TXT" "$out/DONE.COM" ::
printf '%s\r\n' '@ECHO OFF' 'TYPE IN.TXT | FIND /V "DROP" | SORT | MORE > GOT.TXT' DONE > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" ::
utility_session "$boot" "$out/data.img" "$out" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE \
    --timeout-ms 45000 --name filterorder > "$out/stdout" 2> "$out/report" || error 'filter pipeline transport failed'
utility_transport "$out/report"
mtype -i "$out/data.img" ::GOT.TXT > "$out/got" || error 'filter pipeline output missing'
require 'FIND/SORT/MORE pipeline bytes' cmp "$out/expected" "$out/got"
echo 'GREEN FIND, SORT and MORE pipeline bytes'
