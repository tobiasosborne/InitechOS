#!/bin/sh
# MS-DOS 3.3 Reference pp.46-47: copied environment, /C then parent returns.
set -efu
export INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
out=build/command-c-${2:-green}
mkdir -p "$out"
fail() { echo "RED command-c: $*";exit 1; }
dd if=/dev/zero of="$out/data.img" bs=512 count=2880 status=none
mformat -i "$out/data.img" -f 1440 -N 102 ::
cat > "$out/autoexec" <<'BAT'
@echo off
set LANE=PARENT
command /c echo CHILD > child.txt
command /c set LANE=CHILD
set LANE > env.txt
command /c command /c echo NESTED > nested.txt
command /c exit
command /c missing
ver > ver.txt
echo PARENT > parent.txt
echo COMMAND-C-DONE
BAT
mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
cp -f "$out/data.img" "$out/reference.img"
build/qemu_harness --disk "$1" --disk2 "$out/data.img" --out "$out" --name run \
 --timeout-ms 25000 --keys-after SHELL-READY --keys 'e,x,i,t,ret' --quit-after SHELL-DONE \
 --rtc-base 1987-07-06T13:30:58 --rtc-deterministic 2> "$out/report" || true
grep -q quit_after_found=1 "$out/report" || fail 'primary did not accept injected EXIT'
tr -d '\r' < "$out/run.serial" > "$out/text"
grep -qx COMMAND-C-DONE "$out/text" || fail 'secondary EXIT escaped parent'
for row in 'child CHILD' 'nested NESTED' 'parent PARENT'; do
    set -- $row
    printf '%s\r\n' "$2" > "$out/expected"
    mtype -i "$out/data.img" "::$1.TXT" > "$out/actual"
    cmp -s "$out/actual" "$out/expected" || fail "$1 output/handle restoration"
done
mtype -i "$out/data.img" ::ENV.TXT > "$out/actual"
printf 'LANE=PARENT\r\n' > "$out/expected"
cmp -s "$out/actual" "$out/expected" || fail 'child environment leaked'
mtype -i "$out/data.img" ::VER.TXT | tr -d '\r' | grep -q 'InitechDOS Version 3.30' || fail 'primary VER failed'
fsck.fat -n "$out/data.img" > "$out/fsck" 2>&1 || fail 'dosfstools damage'
echo 'GREEN test-command-c-emu: nested /C ECHO, copied environment, EXIT/error, redirection and primary VER'
