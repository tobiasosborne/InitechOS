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
(cd "$out/compile" && "$root/build/turbo/compiler/turbo_fpc" > raw)
grep -q '^TURBO-NATIVE-OK ' "$out/compile/raw"
"$build/keys" turbo/ide/tests/journey.steps "$out/JOURNEY.BIN" "$out/journey.list"
"$build/keys" turbo/ide/tests/paging.steps "$out/PAGING.BIN" "$out/paging.list"
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
  mmd -i "$disk" ::J ::P
  mcopy -i "$disk" "$out/JOURNEY.BIN" ::J/IDEKEYS.BIN
  mcopy -i "$disk" "$corpus/mint/src/MINT06/LONG.PAS" ::J/LONG.PAS
  mcopy -i "$disk" "$out/PAGING.BIN" ::P/IDEKEYS.BIN
  mcopy -i "$disk" "$out/BIG.PAS" ::P/BIG.PAS
  printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
  mcopy -i "$disk" "$dir/CONFIG.SYS" ::
  printf '@echo off\r\ncd j\r\n\\ide.exe\r\ncd \\p\r\n\\ide.exe\r\ncd \\\r\necho ide-smoke-complete\r\n' > "$dir/RUN.BAT"
  mcopy -i "$disk" "$dir/RUN.BAT" ::
  printf '\n\nrun\n' | build/factory/keys > "$dir/keys"
  extra=
  [ "$emulator" = bochs ] && extra='--vga-ext none'
  if ! build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" $extra \
       --keys-after SHELL-READY --keys "$(cat "$dir/keys")" --expect ide-smoke-complete \
       --quit-after ide-smoke-complete --timeout-ms "${IDE_SMOKE_TIMEOUT_MS:-900000}" \
       --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"; then
    cat "$dir/report"; failed=1; continue
  fi
  tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
  if grep -E 'PANIC|RTL-FILE|Bad command|File not found' "$dir/serial.txt"; then failed=1; fi
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
