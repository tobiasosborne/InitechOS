#!/bin/sh
# DEC-03a.5, initech-jarzu. The real III+ by-value depth capture must complete;
# a stack-frame mutant must still flush identical output, then trip the guard.
set -eu
build=$1
exe=$2
mode=$3
source=harness/diff/dbf_diff/xbase_prog_diff/do-file-mint/depth-with
work=$build/samir-high-stack-$mode
mkdir -p "$work"
(cd "$source" && sha256sum --quiet -c sha256.txt)
truncate -s 1474560 "$work/data.img"
mformat -i "$work/data.img" -f 1440 ::
mcopy -o -i "$work/data.img" "$exe" ::SAMIR.EXE
mcopy -o -i "$work/data.img" "${exe%.EXE}.OVL" ::SAMIR.OVL
mcopy -o -i "$work/data.img" "$source"/*.PRG ::
printf 'DO MINT\r\nQUIT\r\n' > "$work/DOFILE.IN"
printf '@echo off\r\nsamir < dofile.in > console.out\r\n' > "$work/DOFILE.BAT"
mcopy -o -i "$work/data.img" "$work/DOFILE.IN" "$work/DOFILE.BAT" ::
INITECH_QMP_STDIO=1 "$build/qemu_harness" --disk "$build/tracer_secondary.img" \
 --disk2 "$work/data.img" --name stack --out "$work" --keys d,o,f,i,l,e,ret,e,x,i,t,ret \
 --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 12000 \
 > "$work/qemu.out" 2> "$work/report" || true
mcopy -o -i "$work/data.img" ::GOLD.OUT "$work/GOLD.OUT"
cmp "$source/GOLD.OUT" "$work/GOLD.OUT"
grep -q 'triple_fault=0' "$work/report"
if [ "$mode" = mutant ]; then
 grep -qx 'PANIC vec=06 err=00000000' "$work/stack.serial"
 ! grep -qx SHELL-DONE "$work/stack.serial"
 printf '>>> test-samir-high-stack-mutant: green (stack-backed WITH frame RED after byte-identical real III+ output; high guard caught it)\n'
else
 ! grep -q '^PANIC' "$work/stack.serial"
 grep -qx SHELL-DONE "$work/stack.serial"
 grep -q 'quit_after_found=1' "$work/report"
 printf '>>> test-samir-high-stack: green (20-level WITH; real III+ output exact; high guard intact; clean return)\n'
fi
