#!/bin/sh
# Compare the shipped FIND with MS-DOS 3.30 stdout, diagnostics and status.
# With stdout redirected, the native console receives FIND stderr.
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/transport.ok" "$out/assertion" "$out/result"
. "$root/dos/tests/runtime/verdict.sh"
case_dir=$root/dos/find/tests/compat33
. "$case_dir/fixture.sh"
export LC_ALL=C TZ=UTC INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
find_fixture
find_batches product
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/DONE.COM" "$programs/FIND.COM" ::
{
    printf '%s\r\n' '@echo off'
    call_find_batches
    if [ -z "${FIND_CASE:-}" ]; then
        printf '%s\r\n' 'FIND /C /I "APPLE" <WORDS.TXT >ICOUNT.OUT' \
            'FIND "apple" words.txt <OTHER.TXT >SHELL.OUT'
    fi
    printf '%s\r\n' DONE
} > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
timeout 70 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" \
    --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 55000 \
    --out "$out" --name findargs > "$out/driver.log" 2> "$out/report" || error 'FIND args transport failed'
transport_pass "$out/report"
tr -d '\r' < "$out/findargs.serial" > "$out/console"
forbid 'FIND returned an unexpected status' grep -Fx 'FIND-PROBE-FAIL' "$out/console"
mdir -i "$out/data.img" :: > "$out/volume" || error 'cannot read the result volume'

# Collect all comparisons before giving the runner the first failed property.
# This also shows which of the unchanged baseline cases already conformed.
first_failure=
compare_bytes() {
    if cmp -s "$1" "$2"; then
        printf 'PASS %s\n' "$3"
        return
    else
        rc=$?
        [ "$rc" -eq 1 ] || error "could not compare $2 ($rc)"
    fi
    printf 'FAIL %s\n' "$3"
    first_failure=${first_failure:-$3}
}
compare() {
    mcopy -o -i "$out/data.img" "::$1" "$out/$1" || error "missing or unreadable result $1"
    compare_bytes "$2" "$out/$1" "$3"
}
while IFS='|' read -r name input args; do
    [ -z "${FIND_CASE:-}" ] || [ "$FIND_CASE" = "$name" ] || continue
    compare "$name.OUT" "$case_dir/expected/$name.OUT" "FIND $name stdout"
    sed -n "/^BEGIN-$name$/,/^END-$name$/ { /^BEGIN-$name$/d; /^END-$name$/d; p; }" "$out/console" | \
        awk '{printf "%s\r\n", $0}' > "$out/$name.ERR"
    compare_bytes "$case_dir/expected/$name.ERR" "$out/$name.ERR" "FIND $name diagnostic"
    compare "$name.RC" "$case_dir/expected/$name.RC" "FIND $name ERRORLEVEL"
done < "$case_dir/cases"
if [ -z "${FIND_CASE:-}" ]; then
    printf '2\r\n' > "$out/count-expected"
    compare ICOUNT.OUT "$out/count-expected" 'FIND /I extension'
    compare SHELL.OUT "$case_dir/expected/NAMED.OUT" 'FIND shell passes named operand'
fi
[ -z "$first_failure" ] || semantic "$first_failure"
echo 'GREEN FIND stdout, stderr and status'
