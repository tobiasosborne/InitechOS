#!/bin/sh
# Accepted GM8 layout, DOS 3.30 primary processor rules, mtools disk oracle.
# Transplant the current product's ordinary payload to C: while the factory
# changes its canonical layout. No alternate kernel or compiled-in scenario.
set -eu
cd "$(dirname "$0")/../.."
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C MTOOLS_SKIP_CHECK=1
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
mkdir -p build/kernel/startup-native
out=$(mktemp -d "$PWD/build/kernel/startup-native/run.XXXXXX")
echo "C startup evidence: $out"
fail() { echo "FAIL C startup: $*" >&2; exit 1; }
for tool in mpartition mformat mcopy mlabel mattrib nasm; do
 command -v "$tool" >/dev/null || { echo "ERROR missing $tool" >&2; exit 2; }
done
mkdir "$out/payload"
mcopy -s -i build/image/data.img '::*' "$out/payload/"
nasm -f bin kernel/tests/programs/startup_drive.asm -o "$out/payload/STARTCHK.COM"
printf 'FILES=7\r\nSHELL=COMMAND.COM /P\r\nDEVICE=ANSI.SYS\r\n' > "$out/payload/CONFIG.SYS"
printf '@ECHO OFF\r\nPATH C:\\;C:\\APPS\r\nECHO CSTART-AUTOEXEC-C\r\nECHO %%COMSPEC%%\r\nPATH\r\nSTARTCHK\r\n' > "$out/payload/AUTOEXEC.BAT"
find "$out/payload" -exec touch -d '@315532800' {} +
# 20160 sectors = 20*(16*63) = 315*(2*32): complete cylinders for both
# published emulator configurations, partition [63,16447), zero padding.
truncate -s $((20160*512)) "$out/master.img"
printf 'drive z: file="%s/master.img" partition=1\n' "$out" > "$out/mtoolsrc"
MTOOLSRC="$out/mtoolsrc" mpartition -I z:
MTOOLSRC="$out/mtoolsrc" mpartition -c -b 63 -l 16384 -T 1 z:
MTOOLSRC="$out/mtoolsrc" mpartition -a z:
dd if=build/boot/mbr.bin of="$out/master.img" bs=446 count=1 conv=notrunc status=none
dd if=build/boot/stage2.bin of="$out/master.img" bs=512 seek=1 conv=notrunc status=none
volume=$out/master.img@@32256
mformat -i "$volume" -T 16384 -h 16 -s 63 -c 8 -H 63 -N 0x43535433 ::
cp build/boot/IO.SYS "$out/IO.SYS"
cp build/system/INITDOS.SYS "$out/INITDOS.SYS"
touch -d '@315532800' "$out/IO.SYS" "$out/INITDOS.SYS"
mcopy -m -i "$volume" "$out/IO.SYS" ::IO.SYS
mcopy -m -i "$volume" "$out/INITDOS.SYS" ::INITDOS.SYS
mattrib -i "$volume" +h +s +r -a ::IO.SYS ::INITDOS.SYS
mlabel -i "$volume" ::INITECH
sh factory/image/copy-tree.sh "$out/payload" "$volume"
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 -N 0x44415433 -v DATA ::
printf 'independent ATA data\r\n' > "$out/expected-a"
mcopy -i "$out/data.img" "$out/expected-a" ::KEEP.TXT
printf '@ECHO OFF\r\nECHO CSTART-WRONG-A\r\n' > "$out/a-autoexec"
mcopy -i "$out/data.img" "$out/a-autoexec" ::AUTOEXEC.BAT
printf 'C system persisted bytes\r\n' > "$out/expected-c"
make -s -f factory/desktop-tools.mk "$PWD/build/factory/desktop-bochs"
for emu in ${CSTART_EMULATORS:-qemu bochs}; do
 for data in alone with-data; do
  run=$out/$emu-$data
  mkdir "$run"
  cp "$out/master.img" "$run/boot.img"
  cp "$out/data.img" "$run/data.img"
  set -- --disk "$run/boot.img"
  test "$data" != with-data || set -- "$@" --disk2 "$run/data.img"
  driver=build/factory/qemu_harness
  test "$emu" != qemu || set -- "$@" --boot-rw
  test "$emu" != bochs || driver=build/factory/desktop-bochs
  keys=$(printf '\n\necho cstartend\n' | build/factory/keys)
  export DESKTOP_SERIAL="$run/$emu.serial" DESKTOP_WAIT_MS=30000
  "$driver" "$@" --keys-after SHELL-READY --keys "$keys" \
    --expect cstartend --quit-after cstartend --timeout-ms "${CSTART_TIMEOUT_MS:-90000}" \
    --name "$emu" --out "$run" > "$run/stdout" 2> "$run/report" || {
     cat "$run/report"; fail "$emu $data did not complete";
    }
  tr -d '\r' < "$run/$emu.serial" > "$run/serial.txt"
  for marker in 'Directory of C:\' 'SYSINIT: source=CONFIG.SYS FILES=7 cap=7 SHELL=COMMAND.COM /P' 'CSTART-AUTOEXEC-C' 'C:\COMMAND.COM' \
    'PATH=C:\;C:\APPS' 'CSTART-OK'; do
   grep -Fx "$marker" "$run/serial.txt" >/dev/null || fail "$emu $data missing [$marker]"
  done
  if test "$data" = alone; then marker=CSTART-A-ABSENT; else marker=CSTART-A-PRESENT; fi
  grep -Fx "$marker" "$run/serial.txt" >/dev/null || fail "$emu $data wrong A identity"
  if grep -E '(CSTART-FAIL|CSTART-WRONG-A|PANIC|Bad command|KBD-RING-DROP)' "$run/serial.txt"; then
   fail "$emu $data guest failure"
  fi
  grep 'triple_fault=0' "$run/report" >/dev/null || fail "$emu $data CPU fault"
  written=$run/boot.img
  test "$emu" != bochs || written=$(sed -n 's/^\[bochs\] private_boot=//p' "$run/report")
  test -f "$written" || { echo "ERROR missing written boot copy" >&2; exit 2; }
  mcopy -i "$written@@32256" ::CSTART.TXT "$run/persisted.txt"
  cmp "$out/expected-c" "$run/persisted.txt" || fail "$emu $data persisted C bytes"
  cmp "$out/data.img" "$run/data.img" || fail "$emu $data changed independent A"
  echo "ok $emu $data: C CONFIG/COMSPEC/PATH/EXEC/write; independent A"
 done
done
echo 'C STARTUP: 0 failures'
