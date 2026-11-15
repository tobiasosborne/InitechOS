#!/bin/sh
# DOS builtins: real MINT07 values, independent raw PSP evidence, both compilers.
set -eu
cd "$(dirname "$0")/../../.."
root=$PWD
. "$root/turbo/tests/checks.sh"
setup mkdir -p "$root/build/turbo/ide-runtime"
out=$(mktemp -d "$root/build/turbo/ide-runtime/run.XXXXXX")
echo "DOS builtin evidence: $out"
setup mkdir -p "$out"
export MTOOLS_SKIP_CHECK=1 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
setup gcc -m32 -march=i386 -std=c11 -Os -ffreestanding -nostdlib -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -c turbo/tests/ide/runtime/raw-psp.c -o "$out/raw.o"
setup ld -m elf_i386 --build-id=none -Ttext 0x40100 -e _start --oformat binary "$out/raw.o" -o "$out/RAW.COM"
printf "program Child; begin Writeln('child ran'); Halt(7) end.\n" > "$out/child.pas"
setup build/turbo/seed/initechc --emit-asm -o "$out/child.s" "$out/child.pas"
setup sh turbo/compiler/tests/package.sh "$out/child.s" "$out/child" > "$out/child.log" 2>&1
# Flat children execute below the extended resident compiler.
setup ld -m elf_i386 --build-id=none -T turbo/seed/rt/seed_dos.ld -o "$out/child.elf" build/turbo/seed/rt/start_dos.o build/turbo/seed/rt/fileio.o "$out/child/program.o"
setup objcopy -O binary "$out/child.elf" "$out/CHILD.COM"
for path in seed hosted; do
 for prog in arguments-all execute; do
  dir=$out/$path-$prog
  setup mkdir -p "$dir"
  if [ "$path" = seed ]; then
   setup build/turbo/seed/initechc --emit-asm -o "$dir/program.s" "turbo/tests/ide/runtime/$prog.pas"
   setup sh turbo/compiler/tests/package.sh "$dir/program.s" "$dir/package" > "$dir/build.log" 2>&1
   setup cp "$dir/package/program.exe" "$dir/PROGRAM.COM"
  else
   setup cp "turbo/tests/ide/runtime/$prog.pas" "$dir/INPUT.PAS"
   setup sh -c 'cd "$1" && "$2" INPUT.PAS > build.log' sh "$dir" "$root/build/turbo/compiler/turbo_fpc"
   setup cp "$dir/TURBOOUT.EXE" "$dir/PROGRAM.COM"
  fi
 done
done
for emulator in ${*:-qemu bochs}; do
 dir=$out/$emulator
 setup mkdir -p "$dir"
 disk=$dir/test.img
 setup dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
 setup mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x54555242 -v TEST ::
 setup mcopy -i "$disk" build/dos/COMMAND.COM ::
 setup mmd -i "$disk" ::TP55
 setup mcopy -i "$disk" "$out/RAW.COM" "$out/CHILD.COM" ::TP55/
 printf '@echo off\r\ncd TP55\r\nraw alpha "two words" omega\r\n' > "$dir/RUN.BAT"
 for path in seed hosted; do
  for prog in arguments-all execute; do
   leaf=SARG.COM; [ "$path" = hosted ] && leaf=HARG.COM
   if [ "$prog" = execute ]; then leaf=SEXE.COM; [ "$path" = hosted ] && leaf=HEXE.COM; fi
   setup mcopy -i "$disk" "$out/$path-$prog/PROGRAM.COM" ::TP55/$leaf
   printf 'echo begin-%s-%s\r\n%s alpha "two words" omega\r\necho end-%s-%s\r\n' "$path" "$prog" "$leaf" "$path" "$prog" >> "$dir/RUN.BAT"
  done
 done
 printf 'echo turbo-runtime-complete\r\n' >> "$dir/RUN.BAT"
 printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
 setup mcopy -i "$disk" "$dir/CONFIG.SYS" "$dir/RUN.BAT" ::
 printf '\n\nrun\n' | build/factory/keys > "$dir/keys"
 extra=
 [ "$emulator" = bochs ] && extra='--vga-ext none --renew-ms 300000'
 setup build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" $extra --keys-after SHELL-READY --keys "$(cat "$dir/keys")" --expect turbo-runtime-complete --quit-after turbo-runtime-complete --timeout-ms 300000 --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"
 setup tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
 setup mcopy -o -i "$disk" ::TP55/RAWPSP.BIN "$dir/RAWPSP.BIN"
 for path in seed hosted; do
  leaf=SARG.COM; [ "$path" = hosted ] && leaf=HARG.COM
  # The TEST volume is A:; only the drive/media path differs from MINT07.
  printf 'COUNT=4\nARG0=A:\\TP55\\%s\nARG1=alpha\nARG2="two\nARG3=words"\nARG4=omega\nARG5=\nDIR=A:\\TP55\n' "$leaf" > "$dir/$path-arguments-all.expected"
  printf 'child ran\nCHILD-STATUS 7\nMISSING-STATUS -2\n' > "$dir/$path-execute.expected"
  for prog in arguments-all execute; do
   setup sed -n "/^begin-$path-$prog\$/,/^end-$path-$prog\$/p" "$dir/serial.txt" > "$dir/bracket"
   setup sed '1d;$d' "$dir/bracket" > "$dir/$path-$prog.actual"
   assert "$emulator:$path:$prog real DOS values" diff -u "$dir/$path-$prog.expected" "$dir/$path-$prog.actual"
  done
 done
done
finish
