#!/bin/sh
# Accepted GM8 layout, DOS 3.30 primary processor rules, mtools disk oracle.
# Exercise the factory C: system image and optional ATA data volume.
# No alternate kernel or compiled-in scenario.
set -eu
cd "$(dirname "$0")/../.."
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C MTOOLS_SKIP_CHECK=1
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
mkdir -p build/kernel/startup-native
out=$(mktemp -d "$PWD/build/kernel/startup-native/run.XXXXXX")
echo "C startup evidence: $out"
fail() { echo "FAIL C startup: $*" >&2; exit 1; }
for tool in mcopy nasm; do
 command -v "$tool" >/dev/null || { echo "ERROR missing $tool" >&2; exit 2; }
done
# Drive the factory's ordinary C system image; only test inputs are injected.
cp "${CSTART_BOOT_IMAGE:-build/image/c.img}" "$out/master.img"
cp "${CSTART_DATA_IMAGE:-build/image/a-data.img}" "$out/data.img"
nasm -f bin kernel/tests/programs/startup_drive.asm -o "$out/STARTCHK.COM"
printf 'FILES=7\r\nSHELL=COMMAND.COM /P\r\nDEVICE=ANSI.SYS\r\n' > "$out/CONFIG.SYS"
printf '@ECHO OFF\r\nPATH C:\\;C:\\APPS\r\nECHO CSTART-AUTOEXEC-C\r\nECHO %%COMSPEC%%\r\nPATH\r\nSTARTCHK\r\n' > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/master.img@@32256" "$out/STARTCHK.COM" ::STARTCHK.COM
mcopy -o -i "$out/master.img@@32256" "$out/CONFIG.SYS" ::CONFIG.SYS
mcopy -o -i "$out/master.img@@32256" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf 'independent ATA data\r\n' > "$out/expected-a"
mcopy -o -i "$out/data.img" "$out/expected-a" ::KEEP.TXT
printf '@ECHO OFF\r\nECHO CSTART-WRONG-A\r\n' > "$out/a-autoexec"
mcopy -o -i "$out/data.img" "$out/a-autoexec" ::AUTOEXEC.BAT
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
