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
 for scenario in missing environment supported; do
  dir=$out/$emulator-$scenario
  mkdir -p "$dir"
  case $scenario in
   missing) shell='MISSING.COM /P'; marker='Bad or missing Command Interpreter';;
   environment) shell='COMMAND.COM /P /E:1024'; marker='Bad or missing Command Interpreter';;
   supported) shell='COMMAND.COM /P /E:512'; marker='AUTOEXEC-RAN';;
  esac
  truncate -s 1474560 "$dir/data.img"
  mformat -i "$dir/data.img" -f 1440 ::
  printf 'SHELL=%s\r\n' "$shell" > "$dir/CONFIG.SYS"
  printf '@echo off\r\necho ran>AUDIT.OUT\r\necho AUTOEXEC-RAN\r\n' > "$dir/AUTOEXEC.BAT"
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
  if [ "$scenario" = supported ]; then
   mcopy -i "$dir/data.img" ::AUDIT.OUT "$dir/audit.txt"
   printf 'ran\r\n' > "$dir/expected.txt"
   cmp "$dir/audit.txt" "$dir/expected.txt"
  else
   if grep -F 'SHELL-READY' "$dir/result.serial"; then exit 1; fi
   if mdir -i "$dir/data.img" ::AUDIT.OUT >/dev/null 2>&1; then exit 1; fi
   cmp "$dir/before.img" "$dir/data.img"
  fi
  printf 'ok %s %s: console marker and independent disk outcome\n' "$emulator" "$scenario"
 done
done
