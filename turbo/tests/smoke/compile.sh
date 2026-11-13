#!/bin/sh
# One in-OS self compilation per fresh boot: factory's QMP driver has an
# absolute 120-second serial gate. Identical source and disk names each time.
set -eu
compiler=$1 source=$2 out=$3
mkdir -p "$out"
root=$PWD
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 MTOOLS_SKIP_CHECK=1
disk=$out/test.img
dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x54555242 -v TEST ::
mcopy -i "$disk" build/dos/COMMAND.COM ::
mcopy -i "$disk" "$compiler" ::TURBO.COM
mcopy -i "$disk" build/turbo/smoke/TIMER.COM ::
mcopy -i "$disk" "$source" ::TURBOIN.PAS
printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$out/CONFIG.SYS"
mcopy -i "$disk" "$out/CONFIG.SYS" ::
printf '@echo off\r\ntimer\r\nturbo\r\ntimer\r\necho compile-complete\r\n' > "$out/RUN.BAT"
mcopy -i "$disk" "$out/RUN.BAT" ::
printf '\n\nrun\n' | build/factory/keys > "$out/keys"
if ! build/factory/qemu_harness --disk build/image/boot.img --disk2 "$disk" \
 --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect compile-complete \
 --quit-after compile-complete --timeout-ms 180000 --name run --out "$out" \
 > "$out/stdout" 2> "$out/report"; then cat "$out/report"; exit 1; fi
tr -d '\r' < "$out/run.serial" > "$out/serial.txt"
if grep -E 'TURBO-.*(ERROR|SKIP)|PANIC|Bad command|File not found' "$out/serial.txt"; then exit 1; fi
test "$(grep -c '^TURBO-NATIVE-OK form=1 ' "$out/serial.txt")" -eq 1
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report"
test "$(grep -c 'CPU Reset' "$out/run.qemu.log")" -eq 2
mcopy -o -i "$disk" ::TURBOOUT.EXE "$out/program.COM"
build/factory/mz/mzcheck "$out/program.COM" > "$out/mzcheck.log"
awk '/^TICK=/ {t=substr($0,6)+0;if(n++==0)start=t;else {d=t-start;if(d<0)d+=8640000;printf "compile=%.2fs (guest clock, 1s resolution)\n",d/100}} END{if(n!=2)exit 1}' "$out/serial.txt" | tee "$out/time.txt"
