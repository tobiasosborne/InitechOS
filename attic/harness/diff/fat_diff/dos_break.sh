#!/bin/sh
# Written before C-6 implementation. Ref: DEC-16 C-6, MS-DOS 3.3 p.38.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
boot=$1; tag=${2:-green}; work=build/dos-break-$tag
mkdir -p "$work"
red() { echo "RED dos-break-emu: $*"; exit 1; }
for mode in on off; do
 out=$work/$mode; mkdir -p "$out"
 dd if=/dev/zero of="$out/data.img" bs=512 count=2880 status=none
 mformat -i "$out/data.img" -f 1440 -N 102 ::
 printf 'disk input\r\n' > "$out/input"
 printf '@echo off\r\nbreak %s\r\nbreakio %s\r\necho BREAK-RETURNED\r\n' "$mode" "$mode" > "$out/autoexec"
 mcopy -i "$out/data.img" "$out/input" ::INPUT.TXT
 mcopy -i "$out/data.img" build/dos_break_program.bin ::BREAKIO.COM
 mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
 cp -f "$out/data.img" "$out/reference.img"
 if [ "$mode" = on ]; then
   # A broken EXEC child leaves a pending batch decision, answered N here.
   keys='kctrl-c,WTerminate batch job (Y/N)?,kn,WA:\>,ke,kx,ki,kt,kret'
 else
   keys='kctrl-c,WA:\>,ke,kx,ki,kt,kret'
 fi
 build/qemu_harness --disk "$boot" --disk2 "$out/data.img" --out "$out" --name run \
   --mouse "$keys" --keys-after BREAK-DISK-READY \
   --quit-after SHELL-DONE --timeout-ms 14000 --screendump --screendump-after SHELL-DONE \
   > "$out/stdout" 2> "$out/report" || true
 tr -d '\r' < "$out/run.serial" > "$out/text"
 grep -qx BREAK-RETURNED "$out/text" || red "$mode did not return after disk Ctrl-C"
 grep -q 'OK=1' "$out/report" || red "$mode emulator failed"
 build/console_text /usr/share/seabios/vgabios-stdvga.bin "$out/run.ppm" > "$out/screen"
 grep -qx BREAK-RETURNED "$out/screen" || red "$mode console did not return"
 if [ "$mode" = on ]; then
   grep -qx INT23-BREAK "$out/text" || red 'ON disk read never invoked INT23'
   grep -qF 'Terminate batch job (Y/N)?' "$out/text" || red 'ON child break lost batch decision'
   ! grep -qx BREAK-OFF-RAW-OK "$out/text" || red 'ON delivered raw Ctrl-C'
 else
   ! grep -qF 'Terminate batch job (Y/N)?' "$out/text" || red 'OFF raw key unexpectedly asked batch question'
   grep -qx BREAK-OFF-RAW-OK "$out/text" || red 'OFF consumed Ctrl-C at disk entry'
   ! grep -qx INT23-BREAK "$out/text" || red 'OFF aborted disk read'
 fi
 cmp "$out/reference.img" "$out/data.img" || red 'read-only fixture changed disk'
 mcopy -i "$out/data.img" ::INPUT.TXT - > "$out/actual"
 cmp "$out/input" "$out/actual" || red 'input differs from mtools'
 fsck.fat -n "$out/data.img" > "$out/fsck" 2>&1 || red 'fsck damage'
 echo "GREEN test-dos-break-emu $mode: injected Ctrl-C during AH=3Fh, decoded console, identical mtools image + fsck.fat"
done
