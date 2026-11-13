#!/bin/sh
# Execute seed output in the product DOS; FPC is the independent stdout oracle.
set -eu
root=$1 out=$2 name=$3
work=$out/runs/guest-$name
mkdir -p "$work"
work=$(CDPATH= cd -- "$work" && pwd)
source=examples/arith/$name.pas
"$out/initechc" --emit-asm -o "$work/program.s" "$source"
nasm -f elf32 "$work/program.s" -o "$work/program.o"
ld -m elf_i386 --build-id=none -T rt/seed_dos.ld -o "$work/program.elf" "$out/rt/start_dos.o" "$out/rt/fileio.o" "$work/program.o"
objcopy -O binary "$work/program.elf" "$work/PROBE.COM"
cp "$source" "$work/reference.pas"
fpc -Mdelphi -B -O- -v0 -FU"$work" -FE"$work" -o"$work/reference" "$work/reference.pas" > "$work/fpc.log"
(cd "$work" && ./reference > expected)
disk=$work/test.img
export MTOOLS_SKIP_CHECK=1 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
dd if=/dev/zero of="$disk" bs=512 count=2880 2>/dev/null
mformat -i "$disk" -f 1440 -N 0x53454544 ::
mcopy -i "$disk" "$root/build/dos/COMMAND.COM" "$work/PROBE.COM" ::
printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$work/CONFIG.SYS"
printf '@echo off\r\necho seed-output-start\r\nprobe\r\necho seed-output-end\r\n' > "$work/RUN.BAT"
mcopy -i "$disk" "$work/CONFIG.SYS" "$work/RUN.BAT" ::
printf '\n\nrun\n' | "$root/build/factory/keys" > "$work/keys"
"$root/build/factory/qemu_harness" --disk "$root/build/image/boot.img" --disk2 "$disk" \
 --keys-after SHELL-READY --keys "$(cat "$work/keys")" --expect seed-output-end \
 --quit-after seed-output-end --timeout-ms 12000 --name run --out "$work" > "$work/stdout" 2> "$work/report"
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$work/report"
tr -d '\r' < "$work/run.serial" | sed -n '/^seed-output-start$/,/^seed-output-end$/p' | sed '1d;$d' > "$work/actual"
diff -u "$work/expected" "$work/actual"
# FPC's file operations and the DOS runtime must persist identical bytes.
for file in "$work"/*.BIN; do
 test -f "$file" || continue
 mcopy -o -i "$disk" ::"$(basename "$file")" "$file.guest"
 cmp "$file" "$file.guest"
done
echo "seed $name: guest execution equals FPC stdout and files"
