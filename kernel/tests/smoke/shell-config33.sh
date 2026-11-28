#!/bin/sh
set -eu
root=$(cd "$1" && pwd)
parent=${2:?output parent}
mkdir -p "$parent"
out=$(mktemp -d "$parent/run.XXXXXX")
cd "$root"
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800 INITECH_QMP_STDIO=1 INITECH_TEST_RAM_MB=8
printf 'SHELL configuration evidence: %s\n' "$out"
for emulator in qemu bochs; do
 for scenario in missing empty long environment default requested large; do
  dir=$out/$emulator-$scenario
  mkdir -p "$dir"
  case $scenario in
   missing) shell='MISSING.COM /P'; marker='Bad or missing Command Interpreter';;
   empty) shell=; marker='Bad or missing Command Interpreter';;
   long) shell=$(printf '%064d' 0 | tr 0 x); marker='Bad or missing Command Interpreter';;
   environment) shell='COMMAND.COM /P /E:junk'; marker='Bad or missing Command Interpreter';;
   default) shell='COMMAND.COM /P'; marker='AUTOEXEC-RAN';;
   requested) shell='COMMAND.COM /P /E:512'; marker='AUTOEXEC-RAN';;
   large) shell='COMMAND.COM /P /E:1024'; marker='AUTOEXEC-RAN';;
  esac
  truncate -s 1474560 "$dir/data.img"
  mformat -i "$dir/data.img" -f 1440 ::
  printf 'SHELL=%s\r\n' "$shell" > "$dir/CONFIG.SYS"
  # DOS 3.30 CONFIG.SYS audit row 4 retains all twelve values.
  value=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  {
   printf '@echo off\r\n'
   n=1
   while [ "$n" -le 12 ]; do
    printf 'SET V%02d=%s\r\n' "$n" "$value"
    n=$((n+1))
   done
   printf '%s\r\n' 'echo [%V12%]>AUDIT.OUT' 'echo AUTOEXEC-RAN'
  } > "$dir/AUTOEXEC.BAT"
  mcopy -i "$dir/data.img" "$dir/CONFIG.SYS" ::CONFIG.SYS
  mcopy -i "$dir/data.img" "$dir/AUTOEXEC.BAT" ::AUTOEXEC.BAT
  cp "$dir/data.img" "$dir/before.img"
  if [ "$emulator" = qemu ]; then
   build/factory/qemu_harness --disk build/image/boot.img --disk2 "$dir/data.img" \
    --out "$dir" --name result --timeout-ms "${SHELL_CONFIG_TIMEOUT_MS:-30000}" \
    --expect "$marker" --quit-after "$marker" --screendump --screendump-after "$marker" \
    --rtc-base 1992-03-02T09:30:00 --rtc-deterministic > "$dir/driver.log" 2>&1
  else
   build/factory/bochs_harness --disk build/image/boot.img --disk2 "$dir/data.img" \
    --out "$dir" --name result --timeout-ms "${SHELL_CONFIG_TIMEOUT_MS:-30000}" \
    --expect "$marker" --quit-after "$marker" --screendump-after "$marker" \
    --vga-ext none --rtc-time0 699440400 > "$dir/driver.log" 2>&1
  fi
  if [ "$marker" = AUTOEXEC-RAN ]; then
   mcopy -i "$dir/data.img" ::AUDIT.OUT "$dir/audit.txt"
   printf '[%s]\r\n' "$value" > "$dir/expected.txt"
   if grep -F 'Out of environment space' "$dir/result.serial"; then exit 1; fi
   cmp "$dir/audit.txt" "$dir/expected.txt"
  else
   if grep -F 'SHELL-READY' "$dir/result.serial"; then exit 1; fi
   if mdir -i "$dir/data.img" ::AUDIT.OUT >/dev/null 2>&1; then exit 1; fi
   cmp "$dir/before.img" "$dir/data.img"
  fi
  printf 'ok %s %s: console marker and independent disk outcome\n' "$emulator" "$scenario"
 done
done
