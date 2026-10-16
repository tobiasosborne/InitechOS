#!/bin/sh
# Four manual-derived parity gates. No expected text is read from command.c.
# MS-DOS 3.3 Guide pp.19-20; Reference pp.57-58,97,135,340,348.
# IBM DOS 3.30 Technical Reference pp.5-12/13. mtools is the disk oracle.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
mode=$1
boot=${2:-unused}
tag=${3:-green}
out=build/dos_parity_${mode}_${tag}
mkdir -p "$out"
fail() { printf '%s RED: %s\n' "$mode" "$*" >&2; exit 1; }
img=$out/data.img
dd if=/dev/zero of="$img" bs=512 count=2880 status=none
mformat -i "$img" -f 1440 ::
cp -f "$img" "$out/reference.img"
printf 'stamp\r\n' > "$out/ref.txt"
touch -t 198707061330.58 "$out/ref.txt"
mcopy -m -i "$out/reference.img" "$out/ref.txt" ::EARLY.TXT
touch -t 198802292359.59 "$out/ref.txt"
mcopy -m -i "$out/reference.img" "$out/ref.txt" ::LATE.TXT
mdir -i "$out/reference.img" :: > "$out/reference.mdir"
# Compare displayed last-write date/time, independently decoded by mdir on
# BOTH images. This also proves two-second packing through mcopy -m's stamp.
stamp() {
    if [ "$1" = SUB ]; then selector=::; else selector="::$1"; fi
    mdir -i "$img" "$selector" > "$out/actual.mdir"
    actual=$(awk -v n="$2" '$1==n {print $(NF-1),$NF}' "$out/actual.mdir")
    expect=$(awk -v n="$3" '$1==n {print $(NF-1),$NF}' "$out/reference.mdir")
    [ -n "$expect" ] && [ "$actual" = "$expect" ] || fail "$1 stamp [$actual] != mtools [$expect]"
    build/dos_parity_host --compare "$img" "$1" "$out/reference.img" "$3.TXT"
}
if [ "$mode" = stamp-host ]; then
    "${DOS_PARITY_HOST:-build/dos_parity_host}" "$img"
    stamp NEW.TXT NEW EARLY
    stamp OLD.TXT OLD LATE
    stamp TRUNC.TXT TRUNC LATE
    stamp SUB SUB EARLY
    stamp SUB/CHILD.TXT CHILD EARLY
    fsck.fat -n "$img" > "$out/fsck.txt" 2>&1 || fail 'fsck.fat damage'
    printf 'PASS stamp-host: create/write/truncate/mkdir/read-close == mtools; fsck.fat clean\n'
    exit 0
fi
keys='e,x,i,t,ret'
completion=SHELL-DONE
completion_count=1
rtc_base=1987-07-06T13:30:58
case "$mode" in
stamp)
    cat > "$out/autoexec" <<'BAT'
@echo off
echo stamp>new.txt
md sub
echo stamp>sub\child.txt
date 02-29-1988
time 23:59:59
echo later>old.txt
echo STAMP-DONE
BAT
    ;;
dir)
    mmd -i "$img" ::SUB
    printf 'selected\r\n' > "$out/pick"
    mcopy -i "$img" "$out/pick" ::SUB/PICK.TXT
    mcopy -i "$img" "$out/pick" ::SUB/PICK.BIN
    mcopy -i "$img" "$out/pick" ::OTHER.TXT
    for leaf in W0 W1 W2 W3 W4 W5; do mcopy -i "$img" "$out/pick" "::SUB/$leaf.TXT"; done
    cat > "$out/autoexec" <<'BAT'
@echo off
echo DRIVE-BEGIN
dir b:
echo DRIVE-END
echo DIR-BEGIN
dir a:\sub\*.txt
echo DIR-END
echo NAME-BEGIN
dir sub\pick
echo NAME-END
echo EXT-BEGIN
dir sub\.txt
echo EXT-END
echo FOLDER-BEGIN
dir sub
echo FOLDER-END
echo WIDE-BEGIN
dir/w sub
echo WIDE-END
echo CWD-BEGIN
cd sub
dir a:*.txt
cd ..
echo CWD-END
BAT
    ;;
page)
    for leaf in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19; do
        mcopy -i "$img" "$out/ref.txt" "::P$leaf.TXT"
    done
    printf '@echo off\r\ndir /p\r\necho PAGE-DONE\r\n' > "$out/autoexec"
    keys='spc,e,x,i,t,ret'
    ;;
path)
    mmd -i "$img" ::ONE ::TWO
    printf '@echo FIRST\r\n' > "$out/first.bat"
    printf '@echo SECOND\r\n' > "$out/second.bat"
    mcopy -i "$img" "$out/first.bat" ::ONE/PROBE.BAT
    mcopy -i "$img" "$out/second.bat" ::TWO/PROBE.BAT
    mcopy -i "$img" build/greet_program.bin ::TWO/GREET.COM
    cat > "$out/autoexec" <<'BAT'
@echo off
path
path a:\one;a:\two
path
set path
probe
greet
set path=a:\two
path
probe
path;
path
probe
echo PATH-DONE
BAT
    ;;
startup)
    # A primary processor, no AUTOEXEC: blank accepts the pinned RTC.
    rtc_base=1980-01-01T00:00:00
    keys='ret,ret,v,e,r,ret'
    completion='A:\>'
    completion_count=2
    ;;
startup-set)
    keys='0,2,minus,3,0,minus,1,9,8,8,ret,0,2,minus,2,9,minus,1,9,8,8,ret,2,5,shift-semicolon,0,0,ret,1,4,shift-semicolon,1,5,shift-semicolon,0,0,ret,d,a,t,e,ret,t,i,m,e,ret'
    completion='A:\>'
    completion_count=3
    ;;
startup-autoexec)
    printf '@echo STARTUP-AUTOEXEC\r\n' > "$out/autoexec"
    keys='v,e,r,ret'
    completion='A:\>'
    completion_count=2
    ;;
*) fail 'unknown mode' ;;
esac
if [ -f "$out/autoexec" ]; then
    mcopy -i "$img" "$out/autoexec" ::AUTOEXEC.BAT
fi
build/qemu_harness --disk "$boot" --disk2 "$img" --out "$out" --name run \
    --rtc-base "$rtc_base" --rtc-deterministic --timeout-ms 25000 \
    --keys-after SHELL-READY --keys "$keys" \
    --quit-after "$completion" --quit-after-count "$completion_count" \
    2> "$out/report" || true
serial=$out/run.serial
[ -s "$serial" ] || fail 'no serial'
! grep -q triple_fault=1 "$out/report" || fail 'triple fault'
tr -d '\r' < "$serial" > "$out/text"
grep -q quit_after_found=1 "$out/report" || fail 'trace did not finish'
block() { sed -n "/^$1-BEGIN$/,/^$1-END$/p" "$out/text"; }
case "$mode" in
stamp)
    grep -q '^STAMP-DONE$' "$out/text" || fail 'batch incomplete'
    stamp NEW.TXT NEW EARLY
    stamp SUB SUB EARLY
    stamp SUB/CHILD.TXT CHILD EARLY
    stamp OLD.TXT OLD LATE
    ;;
dir)
    block DRIVE | grep -qx 'Invalid drive specification' || fail 'DIR B: ignored its drive'
    ! block DRIVE | grep -q 'Directory of A:' || fail 'invalid drive listed A'
    block DIR | grep -qF ' Directory of A:\SUB' || fail 'qualified directory header'
    block DIR | grep -q '^PICK     TXT' || fail 'qualified wildcard missing'
    ! block DIR | grep -qE '^OTHER |^PICK     BIN' || fail 'wildcard leaked files'
    block NAME | grep -q '^PICK     BIN' || fail 'filename.* shorthand'
    block EXT | grep -q '^PICK     TXT' || fail '*.ext shorthand'
    block FOLDER | grep -q '^PICK     BIN' || fail 'directory operand'
    block WIDE | grep -q 'PICK.TXT' || fail '/W filename layout'
    ! block WIDE | grep -qE '[0-9]+:[0-9]+[ap]' || fail '/W leaked timestamp'
    block WIDE | grep -qE '^\[\.\] +\[\.\.\] +PICK.TXT +PICK.BIN +W0.TXT' || fail '/W five columns'
    block CWD | grep -q '^PICK     TXT' || fail 'drive-relative directory'
    mdir -i "$img" ::SUB/*.TXT > "$out/selected.mdir"
    grep -q '^PICK ' "$out/selected.mdir" || fail 'mtools selector fixture'
    block DIR | awk '$2=="TXT" {print $1 "." $2}' | sort > "$out/actual.names"
    awk '$2=="TXT" {print $1 "." $2}' "$out/selected.mdir" | sort > "$out/reference.names"
    cmp -s "$out/actual.names" "$out/reference.names" || fail 'qualified selection differs from mdir'
    ;;
page)
    [ "$(grep -cF 'Press any key to continue . . .' "$out/text")" = 1 ] || fail '/P did not pause at a screen'
    grep -qx PAGE-DONE "$out/text" || fail '/P did not resume'
    grep -q '^P19 ' "$out/text" || fail '/P lost final file'
    ;;
path)
    [ "$(grep -c '^No Path$' "$out/text")" = 2 ] || fail 'bare PATH / PATH ;'
    [ "$(grep -cF 'PATH=A:\ONE;A:\TWO' "$out/text")" = 2 ] || fail 'PATH and SET not shared'
    grep -qxF 'PATH=a:\two' "$out/text" || fail 'SET PATH not visible to PATH'
    grep -qx FIRST "$out/text" || fail 'PATH order/search'
    grep -qx SECOND "$out/text" || fail 'SET PATH search'
    grep -qx 'GREETINGS FROM A:GREET.COM' "$out/text" || fail '.COM PATH search'
    grep -qx 'Bad command or file name' "$out/text" || fail 'PATH clear search'
    ;;
startup)
    grep -qx 'Current date is Tue 1-01-1980' "$out/text" || fail 'startup date/prompt missing'
    grep -q '^Enter new date (mm-dd-yy):' "$out/text" || fail 'date input missing'
    grep -qx 'Current time is 0:00:00.00' "$out/text" || fail 'startup time missing'
    grep -q '^Enter new time:' "$out/text" || fail 'time input missing'
    # Serial mount self-test stays intact for old boot/dos gates. It no longer
    # paints console; the implementation's serial-only sink is checked by the
    # existing test-boot banner pixel gate as well as the new prompt gate.
    grep -qx 'DIR-OK' "$out/text" || fail 'serial mount check lost'
    # Guide pp.19-20: the four startup lines are contiguous. AH=0Ah already
    # echoes CR/LF; an extra newline after each input is a visible regression.
    printf '%s\n' 'Current date is Tue 1-01-1980' 'Enter new date (mm-dd-yy): ' \
        'Current time is 0:00:00.00' 'Enter new time: ' 'A:\>ver' > "$out/startup.golden"
    awk '/^Current date is Tue/{on=1} on{print} /^A:\\>ver$/{exit}' "$out/text" > "$out/startup.actual"
    diff -u "$out/startup.golden" "$out/startup.actual" || fail 'startup line spacing differs from manual'
    ;;
startup-set)
    grep -qx 'Invalid date' "$out/text" || fail 'invalid date not rejected'
    grep -qx 'Invalid time' "$out/text" || fail 'invalid time not rejected'
    grep -qx 'Current date is Mon 2-29-1988' "$out/text" || fail 'entered date did not set clock'
    # Key injection takes virtual time too: grade the entered hour/minute,
    # away from midnight. Exact seconds/packing are independently graded by
    # stamp-host and the batch stamp gate, which do not wait between calls.
    grep -qE '^Current time is 14:15:[0-5][0-9]\.00$' "$out/text" || fail 'entered time did not set clock'
    ;;
startup-autoexec)
    grep -qx STARTUP-AUTOEXEC "$out/text" || fail 'AUTOEXEC did not run'
    ! grep -q 'Enter new' "$out/text" || fail 'AUTOEXEC still prompted for date/time'
    ;;
esac
fsck.fat -n "$img" > "$out/fsck.txt" 2>&1 || fail 'fsck.fat damage'
printf 'PASS %s: manual transcript, disk oracle, injected QEMU trace; fsck.fat clean\n' "$mode"
