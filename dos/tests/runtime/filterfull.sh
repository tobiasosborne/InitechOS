#!/bin/sh
# initech-8ao8f: SORT and FIND must not exit 0 when a write to stdout fails
# short on a full disk (AH=40h short count, IBM DOS 3.30 TR pp.6-139/6-140).
# Genuine SORT/FIND media are missing (initech-0e5gu); the safety rule graded
# here is independent of the exact DOS diagnostic: a reported write failure
# never leaves ERRORLEVEL 0. The shell also prints "Insufficient disk space"
# on CON after the redirected write fault (initech-66oc); the batch marker
# below grades only the filter's own exit code, in the serial transcript,
# while mtools readback proves the output really is truncated (the write
# failure is not vacuous).
set -eu
root=$1 boot=$2 programs=$3 out=$4
. "$root/dos/tests/runtime/verdict.sh"
. "$root/dos/tests/runtime/utility-session.sh"
mkdir -p "$out"
out=$(realpath "$out")
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1

# 400 lines, every line matches FIND "needle"; SORT keeps the same lines.
# (Kept small: each short write rescans the nearly full FAT, and the
# session harness window is 20s on a possibly loaded machine.)
# Lines are unique and already in ascending order, so the host-expected
# output for both filters is the input byte for byte.
awk 'BEGIN {for (i=0;i<400;i++) printf "needle%04d\r\n", i}' > "$out/input"
cp "$out/input" "$out/expected"

run_case() {
    name=$1 command=$2 leave_free=$3
    d=$out/$name
    mkdir -p "$d"
    img=$d/data.img
    truncate -s 1474560 "$img"; mformat -i "$img" -f 1440 ::
    mcopy -i "$img" "$programs/SORT.COM" "$programs/FIND.COM" ::
    mcopy -i "$img" "$out/input" ::IN.TXT
    printf '@ECHO OFF\r\n%s\r\nIF ERRORLEVEL 1 ECHO FILTER-REFUSED\r\nIF NOT ERRORLEVEL 1 ECHO FILTER-OK\r\nECHO filterfull-done\r\n' "$command" > "$d/autoexec"
    mcopy -i "$img" "$d/autoexec" ::AUTOEXEC.BAT
    free=$(mdir -i "$img" :: | awk '/bytes free/{gsub(/ /,"");sub(/bytesfree.*/,"");print}')
    filler=$((free - leave_free))
    truncate -s "$filler" "$d/filler"; mcopy -i "$img" "$d/filler" ::FILL.BIN
    utility_session "$boot" "$img" "$d" --expect filterfull-done --quit-after filterfull-done \
      --timeout-ms 45000 --name run > "$d/stdout" 2> "$d/report" || error "$name transport did not complete"
    utility_transport "$d/report" "$d"
    cp "$d/transport.ok" "$out/transport.ok"
    tr -d '\r' < "$d/run.serial" > "$d/text"
    require "$name write failure sets nonzero ERRORLEVEL" grep -Fx 'FILTER-REFUSED' "$d/text"
    forbid "$name write failure must not report success" grep -Fx 'FILTER-OK' "$d/text"
    require "$name full-disk diagnostic reaches CON" grep -Fx 'Insufficient disk space' "$d/text"
    mtype -i "$img" ::OUT.TXT > "$d/got" || error "$name output missing"
    got=$(wc -c < "$d/got"); want=$(wc -c < "$out/expected")
    require "$name output is truncated by the full disk" test "$got" -lt "$want"
    if [ "$got" -gt 0 ]; then
        head -c "$got" "$out/expected" > "$d/prefix"
        require "$name truncated output is an expected prefix" cmp "$d/prefix" "$d/got"
    fi
}

run_case sort 'SORT <IN.TXT >OUT.TXT' 1536
run_case find 'FIND "needle" <IN.TXT >OUT.TXT' 1536
# /C writes only the count, so leave no cluster at all for it.
printf '400\r\n' > "$out/expected-count"
cp "$out/expected-count" "$out/expected"
run_case findc 'FIND /C "needle" <IN.TXT >OUT.TXT' 0
echo 'GREEN filters: SORT, FIND and FIND /C refuse with nonzero status on a full-disk write'
