#!/bin/sh
# Real no-argument TURBO.COM, injected keys and independent disk read-back.
set -eu
cd "$(dirname "$0")/../../.."
root=$PWD
. "$root/turbo/tests/checks.sh"
corpus=${TURBOPASCAL_DECOMP:-$HOME/Projects/turbopascal-decomp}
setup mkdir -p build/turbo/ide-interactive
out=$(mktemp -d "$root/build/turbo/ide-interactive/run.XXXXXX")
echo "Interactive IDE evidence: $out"
export MTOOLS_SKIP_CHECK=1 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
setup env LC_ALL=C awk 'NR==1{sub(/\r$/, "");printf "%s \r\n",$0;next}{printf "%s\n",$0}' "$corpus/mint/src/MINT07/EXIT7.PAS" > "$out/expected"
printf '\032' >> "$out/expected"
# MINT08 specifies the Rename/Save/Quit flow and EOF. The direct keyboard
# leg uses its text with the semicolon deleted (shared Bochs key vocabulary);
# the native file-driven journey checks the exact captured Pascal text.
setup tr -d ';' < "$corpus/goldens/minted/MINT08/disk/UNNAMED.PAS" > "$out/unnamed.expected"
for emulator in ${*:-qemu bochs}; do
 dir=$out/$emulator
 setup mkdir -p "$dir"
 disk=$dir/test.img
 setup dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
 setup mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x54555242 -v TEST ::
 setup mcopy -i "$disk" build/dos/COMMAND.COM build/turbo/compiler/TURBO.COM ::
 setup mmd -i "$disk" ::TP55
 setup mcopy -i "$disk" "$corpus/mint/src/MINT07/EXIT7.PAS" ::TP55/
 printf '@echo off\r\ncd TP55\r\n\\turbo\r\necho turbo-unnamed-start\r\n\\turbo\r\necho turbo-interactive-complete\r\n' > "$dir/RUN.BAT"
 printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
 setup mcopy -i "$disk" "$dir/CONFIG.SYS" "$dir/RUN.BAT" ::
 trace='kret,kret,kr,ku,kn,kret,w3000,kf3,ke,kx,ki,kt,k7,kdot,kp,ka,ks,kret,w1000,kend,kspc,kf2,w1000,kf9,w2000,kspc,kctrl-f9,w3000,kalt-f5,w500,kspc,kalt-x'
 trace="$trace,w10000,kalt-e,kp,kr,ko,kg,kr,ka,km,kspc,kx,kspc,kb,ke,kg,ki,kn,kspc,ke,kn,kd,kdot,kalt-x,ky,ku,kn,kn,ka,km,ke,kd,kdot,kp,ka,ks,kret"
 extra='--mouse'
 [ "$emulator" = bochs ] && extra='--vga-ext vbe --renew-ms 300000 --keys'
 if [ "$emulator" = bochs ]; then
  # Respect the finite guest key ring while VBE paint/EXEC is busy.
  # This changes input delivery, never the shared run deadline.
  trace=$(printf '%s\n' "$trace" | awk -F, '{for(i=1;i<=NF;i++){if(i>1)printf ",";printf "%s",$i;if(substr($i,1,1)=="k")printf ",w150"}print ""}')
 fi
 setup build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" --keys-after SHELL-READY $extra "$trace" --expect turbo-interactive-complete --quit-after turbo-interactive-complete --timeout-ms 300000 --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"
 setup tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
 setup test "$(grep -c '^KBD-RING-DROP' "$dir/serial.txt")" -eq 0
 assert "$emulator:actual child ran" grep -Fx 'exit seven' "$dir/serial.txt"
 assert_not "$emulator:no lexer dump in ordinary IDE" grep -F TURBO-LEX-BEGIN "$dir/serial.txt"
 setup mcopy -o -i "$disk" ::TP55/EXIT7.PAS ::TP55/EXIT7.BAK "$dir/"
 assert "$emulator:edited source has exact saved bytes" cmp "$out/expected" "$dir/EXIT7.PAS"
 assert "$emulator:backup retains original source" cmp "$corpus/mint/src/MINT07/EXIT7.PAS" "$dir/EXIT7.BAK"
 setup mcopy -o -i "$disk" ::TP55/UNNAMED.PAS "$dir/UNNAMED.PAS"
  assert "$emulator:unnamed save and Quit preserve typed bytes" cmp "$out/unnamed.expected" "$dir/UNNAMED.PAS"
 setup mdir -b -i "$disk" ::TP55/ > "$dir/files"
 assert_not "$emulator:private files removed at quit" grep -Ei 'TURBO[0-9]{3}[.](PAS|\$0[01])|COMPI[0-9]{3}[.]' "$dir/files"
done
finish
