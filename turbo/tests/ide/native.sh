#!/bin/sh
# Native compiler/editor integration, checked through an independent DOS reader.
set -eu
cd "$(dirname "$0")/../../.."
root=$PWD
. "$root/turbo/tests/checks.sh"
setup mkdir -p "$root/build/turbo/ide-native"
out=${TURBO_IDE_NATIVE_OUT:-$(mktemp -d "$root/build/turbo/ide-native/run.XXXXXX")}
echo "Native IDE evidence: $out"
corpus=${TURBOPASCAL_DECOMP:-$HOME/Projects/turbopascal-decomp}
export MTOOLS_SKIP_CHECK=1 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
setup mkdir -p "$out"
setup gcc -m32 -march=i386 -std=c11 -Os -ffreestanding -nostdlib -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -c turbo/tests/ide/runtime/page-reader.c -o "$out/page.o"
setup ld -m elf_i386 --build-id=none -Ttext 0x40100 -e _start --oformat binary "$out/page.o" -o "$out/PAGE.COM"
main=$(awk '/^begin$/ {n=NR} END {print n}' turbo/compiler/turbo.pas)
setup head -n $((main-1)) turbo/compiler/turbo.pas > "$out/driver.pas"
main=$(awk '/^begin$/ {n=NR} END {print n}' turbo/ide/tests/host.pas)
setup head -n $((main-1)) turbo/ide/tests/host.pas >> "$out/driver.pas"
setup cat turbo/tests/ide/runtime/driver.pas >> "$out/driver.pas"
setup build/turbo/seed/initechc --emit-asm -o "$out/driver.s" "$out/driver.pas"
setup sh turbo/compiler/tests/package.sh "$out/driver.s" "$out/package" > "$out/package.log" 2>&1
setup env LC_ALL=C awk 'BEGIN {for(i=0;i<2000;i++)printf "%c%c",65+i%26,16+i%16;printf "%c%c%c",7,3,1}' > "$out/pattern"
for emulator in ${*:-qemu bochs}; do
 dir=$out/$emulator
 setup mkdir -p "$dir"
 disk=$dir/test.img
 setup dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
 setup mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x54555242 -v TEST ::
 setup mcopy -i "$disk" build/dos/COMMAND.COM ::
 setup mcopy -i "$disk" "$out/package/program.exe" ::DRIVER.COM
 setup mcopy -i "$disk" "$out/PAGE.COM" ::
 setup mcopy -i "$disk" "$corpus/goldens/minted/MINT06/disk/DEMO.BAK" ::DEMO.PAS
 setup mcopy -i "$disk" "$corpus/mint/src/MINT07/EXIT7.PAS" "$corpus/mint/src/MINT07/EXIT0.PAS" ::
 printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
 setup mcopy -i "$disk" "$dir/CONFIG.SYS" ::
 printf '@echo off\r\ndriver\r\necho native-driver-returned\r\n' > "$dir/RUN.BAT"
 setup mcopy -i "$disk" "$dir/RUN.BAT" ::
 printf '\n\nrun\n' | build/factory/keys > "$dir/keys"
 extra=
 [ "$emulator" = bochs ] && extra='--vga-ext vbe --renew-ms 300000'
 setup build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" $extra --keys-after SHELL-READY --keys "$(cat "$dir/keys")" --expect native-driver-returned --quit-after native-driver-returned --timeout-ms 300000 --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"
 setup tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
 setup grep -Fx turbo-native-ide-complete "$dir/serial.txt"
 assert "$emulator:unpacked page cells and cursor" grep -Fx PAGE-READ-FAILURES=0 "$dir/serial.txt"
 setup mcopy -o -i "$disk" '::SHOT*.SCR' "$dir/"
 assert "$emulator:independent packed page cells and cursor" cmp "$out/pattern" "$dir/SHOT001.SCR"
 setup mkdir -p "$dir/compile" "$dir/run"
 i=1
 while [ "$i" -le 3 ]; do
  from=$(printf '%03d' $((i+1))); into=$(printf '%03d' "$i")
  setup cp "$dir/SHOT$from.SCR" "$dir/compile/SHOT$into.SCR"; i=$((i+1))
 done
 printf 'error85\ncompile\nafter_compile\n' > "$dir/compile/shots.list"
 assert "$emulator:MINT06 actual screen cells" build/turbo/ide/screens "$corpus/goldens/minted/MINT06" "$dir/compile" turbo/ide/tests/compile.expect
 i=1
 while [ "$i" -le 6 ]; do
  from=$(printf '%03d' $((i+4))); into=$(printf '%03d' "$i")
  setup cp "$dir/SHOT$from.SCR" "$dir/run/SHOT$into.SCR"; i=$((i+1))
 done
 printf 'after_run\nuser7\nafter_user\nafter_zero\nuser0\nafter_zero_user\n' > "$dir/run/shots.list"
 assert "$emulator:MINT07 actual Run/User screen cells" build/turbo/ide/screens "$corpus/goldens/minted/MINT07" "$dir/run" turbo/tests/ide/runtime/native.expect
done
finish
