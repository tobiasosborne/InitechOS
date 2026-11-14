#!/bin/sh
# EXEC feasibility probe for one TURBO.COM (see README.md here).
# Usage, from the repository top after `make image` and `make -C turbo`:
#   sh turbo/tests/ide/probe/probe.sh OUTDIR [qemu|bochs]
set -eu
out=$1 emulator=${2:-qemu}
here=turbo/tests/ide/probe
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 MTOOLS_SKIP_CHECK=1
flags='-m32 -march=i386 -std=c11 -Os -ffreestanding -nostdlib -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables'
gcc $flags -c "$here/parent.c" -o "$out/parent.o"
: > "$out/empty.s"
sh turbo/compiler/tests/package.sh "$out/empty.s" "$out/parent" "$out/parent.o" > "$out/parent.log"
gcc -m32 -march=i386 -std=c11 -Os -ffreestanding -nostdlib -fpie -fno-stack-protector \
  -fno-asynchronous-unwind-tables -c "$here/child.c" -o "$out/child.o"
ld -m elf_i386 --build-id=none -pie --no-dynamic-linker -Ttext 0 -e _start "$out/child.o" -o "$out/child.elf"
objcopy -O binary -j .text -j .rodata -j .data -j .got -j .got.plt "$out/child.elf" "$out/PIC.COM"
if readelf -r "$out/child.elf" | grep -q R_386; then echo 'child needs relocations' >&2; exit 1; fi
mkdir -p "$out/pascal"
cp "$here/hello.pas" "$out/pascal/TURBOIN.PAS"
(cd "$out/pascal" && "$OLDPWD/build/turbo/compiler/turbo_fpc" > build.log)
disk=$out/test.img
dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x54555242 -v TEST ::
mcopy -i "$disk" build/dos/COMMAND.COM ::
mcopy -i "$disk" "$out/parent/program.exe" ::PARENT.COM
mcopy -i "$disk" "$out/PIC.COM" ::PIC.COM
mcopy -i "$disk" "$out/pascal/TURBOOUT.EXE" ::EXT.EXE
mcopy -i "$disk" "$out/pascal/TURBOOUT.COM" ::FLAT.COM
printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$out/CONFIG.SYS"
mcopy -i "$disk" "$out/CONFIG.SYS" ::
printf '@echo off\r\npic\r\nflat\r\nparent\r\necho probe-complete\r\n' > "$out/RUN.BAT"
mcopy -i "$disk" "$out/RUN.BAT" ::
printf '\n\nrun\n' | build/factory/keys > "$out/keys"
extra=
[ "$emulator" = bochs ] && extra='--vga-ext none'
status=0
build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" $extra \
  --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect probe-complete \
  --quit-after probe-complete --timeout-ms 120000 --name run --out "$out" \
  > "$out/stdout" 2> "$out/report" || status=$?
tr -d '\r' < "$out/run.serial" > "$out/serial.txt"
grep -E 'PROBE|LOADER|probe-complete|PANIC' "$out/serial.txt" || :
echo "harness status=$status"
