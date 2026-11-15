#!/bin/sh
# initech-qqrz: independent mcopy -m / mdir / fsck.fat on identical seeds.
# MS-DOS 3.3 Reference p.50 (PDF135): copy retains source date/time;
# pp.127-128 (PDF212-213): XCOPY operands /S /V. IBM Tech Ref p.6-208
# (PDF331): AH=57h returns/sets CX=time, DX=date. No artifact-derived golden.
set -eu
export LC_ALL=C TZ=UTC INITECH_QMP_STDIO=1 DOS_COPY_CLOCK=1
mode=$1; boot=${2:-unused}; xcopy=${3:-build/xcopy_program.bin}; tag=${4:-green}
host=${COPY_STAMP_HOST:-build/dos_parity_host}
xhost=${COPY_STAMP_XHOST:-build/test_xcopy_tree}
out=build/copy_stamp_${mode}_${tag}
mkdir -p "$out"
check_suite=copy-stamp
. dos/tests/assertions.sh
img=$out/data.img
dd if=/dev/zero of="$img" bs=512 count=2880 status=none
mformat -i "$img" -f 1440 -N 0x12345678 ::
mmd -i "$img" ::SRC ::SRC/SUB ::COPY ::XCOPY
# Binary data spans multiple COPY (128-byte) and XCOPY (2048-byte) writes.
dd if=/dev/zero of="$out/large" bs=512 count=13 status=none
printf 'binary\032\000tail\r\n' >> "$out/large"
printf 'nested\r\n' > "$out/small"
: > "$out/empty"
touch -t 198707061330.58 "$out/large"
touch -t 198001010000.02 "$out/empty"
touch -t 198412312359.56 "$out/small"
mcopy -m -i "$img" "$out/large" ::SRC/LARGE.BIN
mcopy -m -i "$img" "$out/empty" ::SRC/EMPTY.TXT
mcopy -m -i "$img" "$out/small" ::SRC/SUB/CHILD.TXT
# Existing, nonempty destination must be truncated AND receive the source stamp.
mcopy -m -i "$img" "$out/large" ::COPY/EMPTY.TXT
mcopy -m -i "$img" "$out/large" ::XCOPY/EMPTY.TXT
cp -f "$img" "$out/seed.img"
cp -f "$img" "$out/reference.img"
for dir in COPY XCOPY; do
    mcopy -o -m -i "$out/reference.img" ::SRC/LARGE.BIN "::$dir/RENAMED.BIN"
    mcopy -o -m -i "$out/reference.img" ::SRC/EMPTY.TXT "::$dir/EMPTY.TXT"
    mcopy -m -i "$out/reference.img" ::SRC/SUB/CHILD.TXT "::$dir/CHILD.TXT"
done
if [ "$mode" = host ]; then
    for pair in 'SRC/LARGE.BIN COPY/RENAMED.BIN' 'SRC/EMPTY.TXT COPY/EMPTY.TXT' 'SRC/SUB/CHILD.TXT COPY/CHILD.TXT'; do
        # Word splitting is intentional: fixture paths contain no spaces.
        set -- $pair
        "$host" --copy "$img" "$1" "$2" > "$out/copy-api.log"
    done
    run_xcopy() {
        cx_name=$1; shift
        cx_rc=0
        "$xhost" --run "$img" XCOPY "$@" > "$out/xcopy.log" 2>&1 || cx_rc=$?
        grep -F "ORACLE RUN: exit=$cx_rc " "$out/xcopy.log" >/dev/null || error "XCOPY $cx_name producer incomplete"
        check "XCOPY $cx_name successful operation exit" test "$cx_rc" = 0
    }
    run_xcopy LARGE 'SRC\LARGE.BIN XCOPY\RENAMED.BIN' F
    run_xcopy EMPTY 'SRC\EMPTY.TXT XCOPY'
    run_xcopy CHILD 'SRC\SUB\CHILD.TXT XCOPY'
elif [ "$mode" = emu ]; then
    mcopy -i "$img" "$xcopy" ::XCOPY.COM
    cat > "$out/autoexec" <<'BAT'
@echo off
copy src\large.bin copy\renamed.bin
copy src\empty.txt copy
copy src\sub\child.txt copy
xcopy src\large.bin xcopy\renamed.bin
xcopy src\empty.txt xcopy
xcopy src\sub\child.txt xcopy
echo COPY-STAMP-DONE
BAT
    mcopy -i "$img" "$out/autoexec" ::AUTOEXEC.BAT
    build/qemu_harness --disk "$boot" --disk2 "$img" --name run --out "$out" \
        --rtc-base 1988-02-29T23:59:58 --rtc-deterministic --timeout-ms 25000 \
        --keys-after SHELL-READY --keys 'f,e,x,i,t,ret' --quit-after SHELL-DONE \
        2> "$out/report" || true
    [ -s "$out/run.serial" ] || fail 'no serial'
    ! grep -q triple_fault=1 "$out/report" || fail 'triple fault'
    grep -q quit_after_found=1 "$out/report" || fail 'injected trace incomplete'
    tr -d '\r' < "$out/run.serial" > "$out/text"
    grep -qx COPY-STAMP-DONE "$out/text" || fail 'batch incomplete'
    [ "$(grep -c '^        1 File(s) copied$' "$out/text")" -eq 3 ] || fail 'COPY completion count'
    [ "$(grep -c '^1 File(s) copied$' "$out/text")" -eq 3 ] || fail 'XCOPY completion count'
    grep -qx SHELL-EXIT "$out/text" || fail 'EXIT did not reach REPL'
else
    fail 'unknown mode'
fi
compare_stamp() {
    kind=$1; name=$2; reference=$3; refname=$4
    read_product "$img" "$name" "$out/actual.bytes"
    mtype -i "$reference" "::$refname" > "$out/reference.bytes"
    check "$kind $name bytes vs mcopy" cmp -s "$out/actual.bytes" "$out/reference.bytes"
    mdir -i "$img" "::$name" > "$out/actual.mdir"
    mdir -i "$reference" "::$refname" > "$out/reference.mdir"
    actual=$(awk 'NR==5 {print $(NF-1),$NF}' "$out/actual.mdir")
    expected=$(awk 'NR==5 {print $(NF-1),$NF}' "$out/reference.mdir")
    [ -n "$expected" ] || error "empty reference stamp $refname"
    check "$kind $name timestamp [$actual] vs mtools [$expected]" test "$actual" = "$expected"
    # mdir displays only minutes. Raw words independently packed by mcopy -m
    # also grade seconds, including 00:00:02 and 23:59:56 boundary values.
    check_log "$kind $name exact stamp words vs mcopy -m" "$out/words.log" build/dos_parity_host --compare "$img" "$name" "$reference" "$refname"
    cat "$out/words.log"
}
for dir in COPY XCOPY; do
    for name in RENAMED.BIN EMPTY.TXT CHILD.TXT; do
        compare_stamp "$dir" "$dir/$name" "$out/reference.img" "$dir/$name"
    done
done
for name in SRC/LARGE.BIN SRC/EMPTY.TXT SRC/SUB/CHILD.TXT; do
    compare_stamp SOURCE "$name" "$out/seed.img" "$name"
done
check_log 'dosfstools filesystem consistency' "$out/fsck.log" fsck.fat -n "$img"
printf 'PASS copy-stamp-%s: COPY + XCOPY bytes and exact source date/time == mcopy -m; mdir equal; fsck.fat clean\n' "$mode"
complete
