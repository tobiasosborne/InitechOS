#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/bigkernel
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
size=$(wc -c < build/system/INITDOS-big.SYS)
[ "$size" -gt 196608 ] || { echo 'FAIL big kernel did not cross old cap'; exit 1; }
marker="BIGKERNEL-OK size=$size sum=00FF0000"
for emu in qemu bochs; do
 cp build/image/data.img "build/bigkernel/$emu-data.img"
 extra=''
 [ "$emu" != bochs ] || extra='--vga-ext none'
 "build/factory/${emu}_harness" --disk build/image/boot.img --disk2 "build/bigkernel/$emu-data.img" \
 $extra --expect "$marker" --quit-after "$marker" --timeout-ms 45000 \
 --name "$emu" --out build/bigkernel > "build/bigkernel/$emu.stdout" 2> "build/bigkernel/$emu.report"
 grep -Fx "$marker" "build/bigkernel/$emu.serial"
 echo "ok $emu: loaded beyond 196608 bytes; all 131072 blob bytes summed after BSS clearing"
done
echo 'BIGKERNEL: 2 checks, 0 failures'
