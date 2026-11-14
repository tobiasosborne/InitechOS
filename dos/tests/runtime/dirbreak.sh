#!/bin/sh
# Reference Guide p159: Ctrl-C aborts DIR/P without printing another entry.
set -eu
root=$1 boot=$2 programs=$3 out=$4
mkdir -p "$out";truncate -s 1474560 "$out/data.img";mformat -i "$out/data.img" -f 1440 ::
printf 'not displayed after abort\r\n' > "$out/file"
i=0;while [ "$i" -lt 30 ]; do n=$(printf 'F%02d.TXT' "$i");mcopy -i "$out/data.img" "$out/file" "::$n";i=$((i+1));done
printf '@ECHO OFF\r\nDIR /P\r\nECHO dirbreak-done\r\n' > "$out/autoexec"
mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
# DIR/P owns the first key; the batch termination question owns N.
short=$(mktemp -d /tmp/dos-dir.XXXXXX);rmdir "$short";ln -s "$out" "$short"
trap 'rm -f "$short"' 0 HUP INT TERM
cp "$boot" "$out/boot.img"
export INITECH_QMP_STDIO=1 INITECH_TEST_RAM_MB=8
timeout -k 5 60 "$root/build/factory/qemu_harness" --disk "$short/boot.img" --boot-rw --disk2 "$short/data.img" \
 --keys ctrl-c,n --keys-after 'Press any key' --expect dirbreak-done --quit-after dirbreak-done \
 --timeout-ms 15000 --name run --out "$short" > "$out/stdout" 2> "$out/report"
tr -d '\r' < "$out/run.serial" > "$out/text"
# The paused row is the 18th entry (header accounts for six lines).
# Assert no directory entry at all appears after the pause marker.
sed -n '/Press any key/,$p' "$out/text" > "$out/after"
if grep -E '^F[0-9][0-9] +TXT' "$out/after" >/dev/null; then echo 'RED DIR: entry emitted after Ctrl-C' >&2;exit 1;fi
grep -F 'Terminate batch job (Y/N)?' "$out/after" >/dev/null
echo 'GREEN DIR: Ctrl-C at pagination unwinds before the next directory entry'
