#!/bin/sh
# Tier 1: the IDE compiled by Turbo Initech replays the tier-0 journeys in
# the booted product image; screens and saved files are read back off the
# TEST disk and graded exactly as in tier 0 (README.md).
# Usage from the repository top: sh turbo/ide/tests/smoke.sh [qemu|bochs]...
set -eu
cd "$(dirname "$0")/../../.."
root=$PWD
. "$root/turbo/tests/checks.sh"
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
"$build/keys" turbo/ide/tests/unnamed.steps "$out/UNNAMED.BIN" "$out/unnamed.list"
for prog in readkey iocheck; do
  mkdir -p "$out/$prog"
  cp turbo/ide/tests/$prog.pas "$out/$prog/TURBOIN.PAS"
  (cd "$out/$prog" && "$root/build/turbo/compiler/turbo_fpc" /DUMP > raw)
  grep -q '^TURBO-NATIVE-OK ' "$out/$prog/raw"
done
awk 'BEGIN { for (i = 1; i <= 5000; i++) printf "line %d of the paging fixture, tab\there\r\n", i }' > "$out/BIG.PAS"
for emulator in ${*:-qemu bochs}; do
  dir=$out/$emulator
  mkdir -p "$dir"
  disk=$dir/test.img
  dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
  mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x49444531 -v TEST ::
  mcopy -i "$disk" build/dos/COMMAND.COM ::
  mcopy -i "$disk" "$out/compile/TURBOOUT.EXE" ::IDE.EXE
  mmd -i "$disk" ::J ::P ::R ::U
  mcopy -i "$disk" "$out/readkey/TURBOOUT.EXE" ::R/READKEY.EXE
  mcopy -i "$disk" "$out/iocheck/TURBOOUT.EXE" ::R/IOCHECK.EXE
  mcopy -i "$disk" "$out/JOURNEY.BIN" ::J/IDEKEYS.BIN
  mcopy -i "$disk" "$corpus/mint/src/MINT06/LONG.PAS" ::J/LONG.PAS
  mcopy -i "$disk" "$corpus/mint/src/MINT06/RO.PAS" ::J/RO.PAS
  mattrib -i "$disk" +r ::J/RO.PAS
  mcopy -i "$disk" "$out/PAGING.BIN" ::P/IDEKEYS.BIN
  mcopy -i "$disk" "$out/UNNAMED.BIN" ::U/IDEKEYS.BIN
  mcopy -i "$disk" "$out/BIG.PAS" ::P/BIG.PAS
  printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
  mcopy -i "$disk" "$dir/CONFIG.SYS" ::
  printf '@echo off\r\ncd r\r\necho readkey-begin\r\nreadkey.exe\r\necho readkey-end\r\niocheck.exe\r\necho iocheck-end\r\ncd \\\r\ncd j\r\n\\ide.exe\r\ncd \\p\r\n\\ide.exe\r\ncd \\u\r\n\\ide.exe\r\ncd \\\r\necho ide-smoke-complete\r\n' > "$dir/RUN.BAT"
  mcopy -i "$disk" "$dir/RUN.BAT" ::
  # Type-ahead for READKEY.EXE follows the command line (DOS keeps it).
  printf '\n\nrun\n' | build/factory/keys > "$dir/keys.base"
  printf '%s,a,f9,alt-x,ctrl-f9,up,q' "$(cat "$dir/keys.base")" > "$dir/keys"
  extra=
  [ "$emulator" = bochs ] && extra='--vga-ext none --renew-ms 300000'
  setup build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" $extra \
       --keys-after SHELL-READY --keys "$(cat "$dir/keys")" --expect ide-smoke-complete \
       --quit-after ide-smoke-complete --timeout-ms "${IDE_SMOKE_TIMEOUT_MS:-900000}" \
       --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"
  tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
  assert_not "$emulator:no product diagnostics" grep -E 'PANIC|Bad command|File not found' "$dir/serial.txt"
  sed -n '/^readkey-begin$/,/^readkey-end$/p' "$dir/serial.txt" | sed '1d;$d' > "$dir/readkey.out"
  assert "$emulator:BIOS key values" diff -u turbo/ide/tests/readkey.expect "$dir/readkey.out"
  sed -n '/^readkey-end$/,/^iocheck-end$/p' "$dir/serial.txt" | sed '1d;$d' > "$dir/iocheck.out"
  assert "$emulator:checked I/O errors" diff -u turbo/ide/tests/iocheck.expect "$dir/iocheck.out"
  assert "$emulator:all journeys completed" test "$(grep -c '^IDE-HOST-END ' "$dir/serial.txt")" -eq 3
  mkdir -p "$dir/J" "$dir/P"
  mcopy -n -i "$disk" '::J/*' "$dir/J/"
  mcopy -n -i "$disk" '::P/*' "$dir/P/"
  cp "$out/journey.list" "$dir/J/shots.list"
  assert "$emulator:47 real TP screens" "$build/screens" "$mint" "$dir/J" turbo/ide/tests/journey.expect > "$dir/screens.txt"
  tail -1 "$dir/screens.txt"
  grep FAIL "$dir/screens.txt" || :
  assert "$emulator:saved source bytes" cmp "$mint/disk/DEMO.PAS" "$dir/J/DEMO.PAS"
  assert "$emulator:backup bytes" cmp "$mint/disk/DEMO.BAK" "$dir/J/DEMO.BAK"
  assert "$emulator:read-only source conserved" cmp "$corpus/mint/src/MINT06/RO.PAS" "$dir/J/RO.PAS"
  assert "$emulator:unsaved source conserved" cmp "$corpus/mint/src/MINT06/LONG.PAS" "$dir/J/LONG.PAS"
  ls "$dir/J" > "$dir/files"
  assert_not "$emulator:refused save leaves no new files" grep -E '^(RO[.]BAK|RO[.][$]{3}|DEMO[.][$]{3}|X[.]PAS)$' "$dir/files"
  awk 'BEGIN { RS = "\r\n" }
       NR == 1 { printf "TOP %s\r\n", $0; next }
       NR == 2500 { printf "%sMID %s\r\n", substr($0, 1, 4), substr($0, 5); next }
       NR == 2501 { next }
       { printf "%s\r\n", $0 }
       END { printf "END\032" }' "$out/BIG.PAS" > "$dir/expected"
  assert "$emulator:paged saved bytes" cmp "$dir/expected" "$dir/P/BIG.PAS"
  assert "$emulator:paged backup bytes" cmp "$out/BIG.PAS" "$dir/P/BIG.BAK"
  mkdir -p "$dir/U"
  mcopy -n -i "$disk" '::U/*' "$dir/U/"
  cp "$out/unnamed.list" "$dir/U/shots.list"
  assert "$emulator:MINT08 unnamed save screens" "$build/screens" "$corpus/goldens/minted/MINT08" "$dir/U" turbo/ide/tests/unnamed.expect
  assert "$emulator:MINT08 unnamed save bytes" cmp "$corpus/goldens/minted/MINT08/disk/UNNAMED.PAS" "$dir/U/UNNAMED.PAS"
  assert "$emulator:all three journeys finish Quit" test "$(grep -c '^IDE-HOST-MODE=5$' "$dir/serial.txt")" -eq 3
  echo "PASS $emulator ide journeys"
done
finish
