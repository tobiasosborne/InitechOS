#!/bin/sh
# Tier 1: the IDE compiled by Turbo Initech replays the tier-0 journeys in
# the booted product image; screens and saved files are read back off the
# TEST disk and graded exactly as in tier 0 (README.md).
# Usage from the repository top: sh turbo/ide/tests/smoke.sh [qemu|bochs]...
set -eu
cd "$(dirname "$0")/../../.."
root=$PWD
out=$root/build/turbo/ide/smoke
build=$root/build/turbo/ide
corpus=${TURBOPASCAL_DECOMP:-$HOME/Projects/turbopascal-decomp}
mint=$corpus/goldens/minted/MINT06
export MTOOLS_SKIP_CHECK=1 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
rm -rf "$out"
mkdir -p "$out/compile"
cp "$build/idehost.pas" "$out/compile/TURBOIN.PAS"
(cd "$out/compile" && "$root/build/turbo/compiler/turbo_fpc" /DUMP > raw)
grep -q '^TURBO-NATIVE-OK ' "$out/compile/raw"
sed 's|^type NODIR/X.PAS$|type \\NODIR\\X.PAS|' turbo/ide/tests/journey.steps > "$out/journey.steps"
grep -qx 'type \\NODIR\\X.PAS' "$out/journey.steps"
"$build/keys" "$out/journey.steps" "$out/JOURNEY.BIN" "$out/journey.list"
"$build/keys" turbo/ide/tests/paging.steps "$out/PAGING.BIN" "$out/paging.list"
for prog in readkey iocheck; do
  mkdir -p "$out/$prog"
  cp turbo/ide/tests/$prog.pas "$out/$prog/TURBOIN.PAS"
  (cd "$out/$prog" && "$root/build/turbo/compiler/turbo_fpc" /DUMP > raw)
  grep -q '^TURBO-NATIVE-OK ' "$out/$prog/raw"
done
awk 'BEGIN { for (i = 1; i <= 5000; i++) printf "line %d of the paging fixture, tab\there\r\n", i }' > "$out/BIG.PAS"
failed=0
for emulator in ${*:-qemu bochs}; do
  dir=$out/$emulator
  mkdir -p "$dir"
  disk=$dir/test.img
  dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
  mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x49444531 -v TEST ::
  mcopy -i "$disk" build/dos/COMMAND.COM ::
  mcopy -i "$disk" "$out/compile/TURBOOUT.EXE" ::IDE.EXE
  mmd -i "$disk" ::J ::P ::R
  mcopy -i "$disk" "$out/readkey/TURBOOUT.EXE" ::R/READKEY.EXE
  mcopy -i "$disk" "$out/iocheck/TURBOOUT.EXE" ::R/IOCHECK.EXE
  mcopy -i "$disk" "$out/JOURNEY.BIN" ::J/IDEKEYS.BIN
  mcopy -i "$disk" "$corpus/mint/src/MINT06/LONG.PAS" ::J/LONG.PAS
  mcopy -i "$disk" "$corpus/mint/src/MINT06/RO.PAS" ::J/RO.PAS
  mattrib -i "$disk" +r ::J/RO.PAS
  mcopy -i "$disk" "$out/PAGING.BIN" ::P/IDEKEYS.BIN
  mcopy -i "$disk" "$out/BIG.PAS" ::P/BIG.PAS
  printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
  mcopy -i "$disk" "$dir/CONFIG.SYS" ::
  printf '@echo off\r\ncd r\r\necho readkey-begin\r\nreadkey.exe\r\necho readkey-end\r\niocheck.exe\r\necho iocheck-end\r\ncd \\\r\ncd j\r\n\\ide.exe\r\ncd \\p\r\n\\ide.exe\r\ncd \\\r\necho ide-smoke-complete\r\n' > "$dir/RUN.BAT"
  mcopy -i "$disk" "$dir/RUN.BAT" ::
  # Type-ahead for READKEY.EXE follows the command line (DOS keeps it).
  printf '\n\nrun\n' | build/factory/keys > "$dir/keys.base"
  printf '%s,a,f9,alt-x,ctrl-f9,up,q' "$(cat "$dir/keys.base")" > "$dir/keys"
  extra=
  [ "$emulator" = bochs ] && extra='--vga-ext none'
  if ! build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" $extra \
       --keys-after SHELL-READY --keys "$(cat "$dir/keys")" --expect ide-smoke-complete \
       --quit-after ide-smoke-complete --timeout-ms "${IDE_SMOKE_TIMEOUT_MS:-900000}" \
       --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"; then
    cat "$dir/report"; failed=1; continue
  fi
  tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
  if grep -E 'PANIC|Bad command|File not found' "$dir/serial.txt"; then failed=1; fi
  sed -n '/^readkey-begin$/,/^readkey-end$/p' "$dir/serial.txt" | sed '1d;$d' > "$dir/readkey.out"
  diff -u turbo/ide/tests/readkey.expect "$dir/readkey.out" || { echo "FAIL $emulator readkey"; failed=1; }
  sed -n '/^readkey-end$/,/^iocheck-end$/p' "$dir/serial.txt" | sed '1d;$d' > "$dir/iocheck.out"
  diff -u turbo/ide/tests/iocheck.expect "$dir/iocheck.out" || { echo "FAIL $emulator iocheck"; failed=1; }
  test "$(grep -c '^IDE-HOST-END ' "$dir/serial.txt")" -eq 2 || { echo "FAIL $emulator: runs did not finish"; failed=1; }
  mkdir -p "$dir/J" "$dir/P"
  mcopy -n -i "$disk" '::J/*' "$dir/J/"
  mcopy -n -i "$disk" '::P/*' "$dir/P/"
  cp "$out/journey.list" "$dir/J/shots.list"
  "$build/screens" "$mint" "$dir/J" turbo/ide/tests/journey.expect > "$dir/screens.txt" || failed=1
  tail -1 "$dir/screens.txt"
  grep FAIL "$dir/screens.txt" || :
  cmp "$mint/disk/DEMO.PAS" "$dir/J/DEMO.PAS" || failed=1
  cmp "$mint/disk/DEMO.BAK" "$dir/J/DEMO.BAK" || failed=1
  cmp "$corpus/mint/src/MINT06/RO.PAS" "$dir/J/RO.PAS" || failed=1
  cmp "$corpus/mint/src/MINT06/LONG.PAS" "$dir/J/LONG.PAS" || failed=1
  if ls "$dir/J" | grep -E '^(RO[.]BAK|RO[.][$]{3}|DEMO[.][$]{3}|X[.]PAS)$'; then echo "FAIL $emulator: refused save left a file"; failed=1; fi
  awk 'BEGIN { RS = "\r\n" }
       NR == 1 { printf "TOP %s\r\n", $0; next }
       NR == 2500 { printf "%sMID %s\r\n", substr($0, 1, 4), substr($0, 5); next }
       NR == 2501 { next }
       { printf "%s\r\n", $0 }
       END { printf "END\032" }' "$out/BIG.PAS" > "$dir/expected"
  cmp "$dir/expected" "$dir/P/BIG.PAS" || failed=1
  cmp "$out/BIG.PAS" "$dir/P/BIG.BAK" || failed=1
  [ "$failed" -eq 0 ] && echo "PASS $emulator ide journeys"
done
test "$failed" -eq 0
