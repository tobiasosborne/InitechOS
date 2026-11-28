#!/bin/sh
# Reference Guide p159: Ctrl-C aborts DIR/P without printing another entry.
set -eu
root=$1 boot=$2 programs=$3 out=$4
. "$root/dos/tests/runtime/verdict.sh"
mkdir -p "$out";truncate -s 1474560 "$out/data.img";mformat -i "$out/data.img" -f 1440 ::
printf 'not displayed after abort\r\n' > "$out/file"
TZ=UTC touch -t 198001010000 "$out/file"
i=0;while [ "$i" -lt 30 ]; do n=$(printf 'F%02d.TXT' "$i");TZ=UTC mcopy -m -i "$out/data.img" "$out/file" "::$n";i=$((i+1));done
printf '@ECHO OFF\r\nDIR /P\r\nECHO dirbreak-done\r\n' > "$out/autoexec"
mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
# DIR/P owns the first key; the batch termination question owns N.
keys_after='Press any key'
if [ "${DOS_SESSION_EMULATOR:-qemu}" = bochs ]; then
 # Bochs gates on complete lines. F16 is the last entry before the cue.
 keys_after='F16      TXT       27   1-01-80  12:00a'
fi
DOS_SESSION_KEYS_AFTER="$keys_after" sh "$root/dos/tests/runtime/execute.sh" \
 "$root" "$boot" "$out" ctrl-c,n dirbreak-done || error 'DIR pause transport failed'
# The paused row is the 18th entry (header accounts for six lines).
# Assert no directory entry at all appears after the pause marker.
sed -n '/Press any key/,$p' "$out/text" > "$out/after"
if grep -E '^F[0-9][0-9] +TXT' "$out/after" >/dev/null; then semantic 'entry emitted after Ctrl-C';fi
require 'batch termination question' grep -F 'Terminate batch job (Y/N)?' "$out/after" >/dev/null
echo 'GREEN DIR: Ctrl-C at pagination unwinds before the next directory entry'
