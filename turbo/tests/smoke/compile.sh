#!/bin/sh
# One in-OS self compilation per fresh boot. QEMU retains its existing route;
# Bochs has an explicit whole-run bound pending factory phase-wait support.
# Identical source and disk names each time.
set -eu
compiler=$1 source=$2 out=$3
emulator=${4:-qemu}
case $emulator in qemu|bochs) ;; *) echo "Unknown compiler emulator: $emulator" >&2; exit 1;; esac
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
if [ "$emulator" = qemu ]; then
if ! build/factory/qemu_harness --disk build/image/boot.img --disk2 "$disk" \
 --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect compile-complete \
 --quit-after compile-complete --timeout-ms 180000 --name run --out "$out" \
 > "$out/stdout" 2> "$out/report"; then cat "$out/report"; exit 1; fi
else
 # The factory owner will replace this bound with ordered phase waits;
 # P2/turbo-bochs-factory-request.md specifies the requested interface.
 # The monotonic acceptance check also rejects the old harness's late-marker
 # false-green race. It includes queueing and cleanup, so it is conservative.
 # Calibration at 600s and 1200s expired during native emission. The complete
 # Bochs proof measured up to 1738.492s; this bound allows load variation.
 # P2/turbo-bochs.md records the measured runs and factory resource isolation.
 bochs_limit=${TURBO_BOCHS_TIMEOUT_MS:-3600000}
 case $bochs_limit in ''|*[!0-9]*) echo 'Invalid TURBO_BOCHS_TIMEOUT_MS' >&2; exit 1;; esac
 if ! [ "$bochs_limit" -ge 1 ] || ! [ "$bochs_limit" -le 2147483647 ]; then
  echo 'TURBO_BOCHS_TIMEOUT_MS must be a positive 32-bit millisecond bound' >&2; exit 1
 fi
 sha256sum build/image/boot.img > "$out/boot.sha256"
 bochs_start=$(build/turbo/smoke/monotonic-ms)
 printf 'start-ms=%s limit-ms=%s\n' "$bochs_start" "$bochs_limit" > "$out/wall-time.txt"
 bochs_status=0
 build/factory/bochs_harness --disk build/image/boot.img --disk2 "$disk" \
  --vga-ext none --rtc-time0 699440400 \
  --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect compile-complete \
  --quit-after compile-complete --timeout-ms "$bochs_limit" --name run --out "$out" \
  > "$out/stdout" 2> "$out/report" || bochs_status=$?
 bochs_end=$(build/turbo/smoke/monotonic-ms)
 bochs_elapsed=$((bochs_end - bochs_start))
 printf 'whole-run-ms=%s limit-ms=%s (includes queueing and cleanup)\n' "$bochs_elapsed" "$bochs_limit" | tee -a "$out/wall-time.txt"
 # Clock samples truncate to milliseconds. Add one for a conservative upper
 # bound, rejecting rather than accepting an ambiguous deadline boundary.
 if [ "$((bochs_elapsed + 1))" -ge "$bochs_limit" ]; then
  echo 'FAIL: Bochs whole-run deadline exceeded' >&2; exit 1
 fi
 if [ "$bochs_status" -ne 0 ]; then cat "$out/report"; exit 1; fi
 grep -E '^\[bochs\] launched=1 rfb_unblocked=1 timed_out=0 ' "$out/report"
 grep -Fx '[bochs] triple_fault=0' "$out/report"
 grep -Fx 'megs: 8' "$out/run.bochsrc"
 sha256sum -c "$out/boot.sha256"
fi
tr -d '\r' < "$out/run.serial" > "$out/serial.txt"
if grep -E 'TURBO-.*(ERROR|SKIP)|PANIC|Bad command|File not found' "$out/serial.txt"; then exit 1; fi
test "$(grep -c '^TURBO-NATIVE-OK form=1 ' "$out/serial.txt")" -eq 1
if [ "$emulator" = qemu ]; then
 grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report"
 test "$(grep -c 'CPU Reset' "$out/run.qemu.log")" -eq 2
fi
mcopy -o -i "$disk" ::TURBOOUT.EXE "$out/program.COM"
build/factory/mz/mzcheck "$out/program.COM" > "$out/mzcheck.log"
awk '/^TICK=/ {t=substr($0,6)+0;if(n++==0)start=t;else {d=t-start;if(d<0)d+=8640000;printf "compile=%.2fs (guest clock, 1s resolution)\n",d/100}} END{if(n!=2)exit 1}' "$out/serial.txt" | tee "$out/time.txt"
