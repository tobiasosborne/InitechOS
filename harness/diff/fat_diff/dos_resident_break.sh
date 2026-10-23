#!/bin/sh
# initech-c80y: replay audit controlshort/gaps2 on PRIVATE disk copies.
# User's Guide p.159 / PDF244; fixture bytes, never kernel output, are golden.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
boot=$1; tag=${2:-green}; only=${3:-all}
root=build/dos-resident-break-$tag
mkdir -p "$root"
red() { echo "RED dos-resident-break: $*"; exit 1; }
for mode in controlshort gaps2 redirect; do
 [ "$only" = all ] || [ "$only" = "$mode" ] || continue
 out=$root/$mode; mkdir -p "$out"
 dd if=/dev/zero of="$out/data.img" bs=512 count=2880 status=none
 mformat -i "$out/data.img" -f 1440 -N 102 ::
 mmd -i "$out/data.img" ::T ::MANY
 cp -f docs/audits/2026-10-23-dos/evidence/resume-fixtures/HUGE.TXT "$out/huge"
 mcopy -i "$out/data.img" "$out/huge" ::T/HUGE.TXT
 printf 'stdin sentinel\r\n' > "$out/input"
 mcopy -i "$out/data.img" "$out/input" ::T/INPUT.TXT
 case $mode in
 controlshort) command='TYPE HUGE.TXT'; trigger='HUGE LINE 00005'; delay='';;
 gaps2)
   command='DIR \MANY /P'; trigger='Press any key'; delay=''
   n=0; while [ "$n" -lt 300 ]; do
     name=$(printf 'F%03d.TXT' "$n")
     mcopy -i "$out/data.img" "$out/input" "::MANY/$name"
     n=$((n+1))
   done;;
 redirect) command='TYPE HUGE.TXT < INPUT.TXT > CUT.TXT'; trigger='C80Y-BEGIN'; delay='w100,';;
 esac
 printf '@echo off\r\ncd \\T\r\necho C80Y-BEGIN\r\n%s\r\necho C80Y-MUST-NOT-RUN\r\n' "$command" > "$out/autoexec"
 mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
 cp -f "$out/data.img" "$out/reference.img"
 cp -f "$boot" "$out/boot.img"
 # Wait for the returned prompt BEFORE typing a command: buffered input is
 # not evidence of a usable prompt. No spaces to advance a broken DIR pager.
 build/qemu_harness --disk "$out/boot.img" --disk2 "$out/data.img" --out "$out" --name run \
   --mouse "${delay}kctrl-c,WA:\\T>,ke,kc,kh,ko,kspc,ka,kf,kt,ke,kr,kminus,kc,ka,kn,kc,ke,kl,kret,ke,kx,ki,kt,kret" \
   --keys-after "$trigger" --quit-after SHELL-DONE --timeout-ms 20000 \
   --screendump --screendump-after SHELL-DONE > "$out/stdout" 2> "$out/report" || true
 grep -q 'OK=1' "$out/report" || red "$mode did not return a usable prompt"
 build/dos_resident_break_check "$out/run.serial" "$out/huge" "$mode" || exit 1
 build/console_text /usr/share/seabios/vgabios-stdvga.bin "$out/run.ppm" > "$out/screen"
 grep -qx after-cancel "$out/screen" || red "$mode screen lacks command result"
 cmp "$boot" "$out/boot.img" || red 'boot disk changed'
 mcopy -i "$out/data.img" ::T/HUGE.TXT - > "$out/source"
 cmp "$out/huge" "$out/source" || red 'source changed'
 mcopy -i "$out/data.img" ::T/INPUT.TXT - > "$out/stdin"
 cmp "$out/input" "$out/stdin" || red 'stdin changed'
 if [ "$mode" = redirect ]; then
   mcopy -i "$out/data.img" ::T/CUT.TXT - > "$out/prefix"
   n=$(wc -c < "$out/prefix"); full=$(wc -c < "$out/huge")
   [ "$n" -gt 0 ] && [ "$n" -lt "$full" ] || red 'redirect did not cancel inside TYPE'
   head -c "$n" "$out/huge" > "$out/expected"
   cmp "$out/expected" "$out/prefix" || red 'redirect contains non-file bytes'
 else
   cmp "$out/reference.img" "$out/data.img" || red 'read-only command changed disk'
 fi
 fsck.fat -n "$out/data.img" > "$out/fsck" 2>&1 || red 'fsck damage'
 echo "GREEN dos-resident-break $mode: prompt, console, independent file prefix, intact input, clean disk"
done
