#!/bin/sh
# IBM DOS 3.30 TR INT21 program probes on a separate writable data volume.
set -eu
cd "$(dirname "$0")/.."
. factory/image/test-media.sh
mkdir -p build/smoke-programs
out=${PROGRAM_SMOKE_OUT:-$(mktemp -d "$PWD/build/smoke-programs/run.XXXXXX")}
mkdir -p "$out"
out=$(cd "$out" && pwd)
echo "Program smoke evidence: $out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
fail() { echo "FAIL program smoke: $*" >&2; exit 1; }
programs=${PROGRAM_SMOKE_PROGRAMS:-build/dos/test-programs}
printf 'Hello, world!\r\n' > "$out/hello.txt"
printf 'Second file\r\n' > "$out/second.txt"
truncate -s 98304 "$out/big.dat"
printf 'BEYOND-64KiB!' | dd of="$out/big.dat" bs=1 seek=80000 conv=notrunc status=none
# FCBX T13/14 embeds drive 01 and T19 grades the captured FAT12 A:
# geometry (kernel/tests/fixtures/fcb). Only that probe switches to A.
prepare_programs() {
 mkdir -p "$out/$1"
 media_prepare "${FACTORY_MEDIA_PAIR:-canonical}" with-data "$out/$1" build/image "${SMOKE_BOOT:-}"
 if [ "$media_system" = A ]; then
  mdel -i "$media_work" ::AUTOEXEC.BAT
 else
  printf 'different proof on A\r\n' > "$out/decoy"
  mcopy -i "$media_data" "$out/decoy" ::OUT.TXT
 fi
 for program in "$programs"/*.COM; do
  case ${program##*/} in
   FCBX.COM) mcopy -i "$media_data" "$program" ::;;
   *) mcopy -i "$media_work" "$program" ::;;
  esac
 done
 mmd -i "$media_work" ::SUB
 mcopy -o -i "$media_work" "$out/hello.txt" ::HELLO.TXT
 mcopy -i "$media_work" "$out/second.txt" ::SECOND.TXT
 mcopy -i "$media_work" "$out/big.dat" ::BIG.DAT
 # Preserve the per-boot default drive for the following nested EXEC.
 printf '%s\r\n' '@ECHO OFF' 'A:' 'FCBX' "$media_system:" > "$out/fcbrun.bat"
 mcopy -i "$media_work" "$out/fcbrun.bat" ::FCBRUN.BAT
}
cat > "$out/session.txt" <<'EOF'


multi
fcbrun
nest
cd ..
services
write
conin
dir
echo programsdone
EOF
build/factory/keys < "$out/session.txt" > "$out/keys"
# Bochs types into the guest's keyboard ring with no flow control, so a command
# typed while the previous program still runs can overflow it (KBD-RING-DROP,
# initech-ix9by). Each command is typed only after the previous one printed
# its own completion line (W waits, desktop-bochs.c, in order, whole lines).
# CONIN's input line is typed with its command: CONIN reads it from the ring.
session_keys() { printf '%s\n' "$1" | build/factory/keys; }
bochs_keys="$(session_keys '')"
bochs_keys="$bochs_keys,$(session_keys ''),w1000"
for step in 'multi|MO-DONE' 'fcbrun|FCBX-END' 'nest|EXEC-NESTED-RC=7 PSP-RESTORED INT20-OK' \
  'cd ..|Invalid directory' 'services|SERVICES-MEMORY-VECTORS-CLOCK-OK' \
  'write|WRITE-READBACK-OK' 'conin|' 'dir|CONIN-LINE=dir' 'echo programsdone|'; do
 command=${step%%|*} done=${step#*|}
 bochs_keys="$bochs_keys,$(session_keys "$command")"
 [ -z "$done" ] || bochs_keys="$bochs_keys,W$done"
done
make -s -f factory/desktop-tools.mk "$PWD/build/factory/desktop-bochs" > "$out/frontend.log" 2>&1 || fail 'Bochs journey frontend build'
finish=$(printf '\nprogramsdone\r\n')
for emu in qemu bochs; do
 prepare_programs "$emu"
 keys=$(cat "$out/keys")
 marker=$finish driver=build/factory/qemu_harness
 if [ "$emu" = bochs ]; then
  marker=programsdone driver=build/factory/desktop-bochs keys=$bochs_keys
  export DESKTOP_SERIAL="$out/bochs.serial" DESKTOP_WAIT_MS=${PROGRAM_SMOKE_WAIT_MS:-30000}
 fi
 printf '%s\n' "$keys" > "$out/$emu.keys"
 if ! media_run "$emu" "$driver" --keys-after SHELL-READY --keys "$keys" \
  --expect "$marker" --quit-after "$marker" --timeout-ms "${PROGRAM_SMOKE_TIMEOUT_MS:-55000}" \
  --name "$emu" --out "$out" > "$out/$emu.stdout" 2> "$out/$emu.report"; then
  cat "$out/$emu.report"; fail "$emu did not complete";
 fi
 media_readback "$emu" "$out/$emu.report"
 printf '%s\n' "$media_volume" > "$out/$emu-volume"
 tr -d '\r' < "$out/$emu.serial" > "$out/$emu.txt"
 for line in 'MO-A1=Hel' 'MO-B1=Sec' 'MO-A2=Hello' 'MO-BIG=BEYOND-64KiB!' 'MO-DONE' \
  'FCBX-BEGIN' 'FCBX-END' 'EXEC-NESTED-DEPTH-4-BUSY' \
  'EXEC-NESTED-RC=7 PSP-RESTORED INT20-OK' 'SERVICES-MEMORY-VECTORS-CLOCK-OK' \
  'WRITE-READBACK-OK' 'CONIN-LINE=dir' 'programsdone'; do
  grep -Fx "$line" "$out/$emu.txt" >/dev/null || fail "$emu missing [$line]";
 done
 if grep -E '(SERVICES-FAIL|EXEC-NESTED-FAIL|WRITE-.*-FAIL|PANIC|Bad command|KBD-RING-DROP)' "$out/$emu.txt"; then fail "$emu guest failure"; fi
 grep 'triple_fault=0' "$out/$emu.report" >/dev/null || fail "$emu CPU fault"
 if [ "$emu" = qemu ]; then
  grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/qemu.report" >/dev/null || fail 'QEMU fault/reset verdict'
  [ "$(grep -c 'CPU Reset' "$out/qemu.qemu.log")" -eq 2 ] || fail 'QEMU unexpected reset'
 fi
 # Independent captures: the same FCBX source executed under real MS-DOS 6.22.
 sed -n '/^FCBX-BEGIN$/,/^FCBX-END$/p' "$out/$emu.txt" > "$out/$emu-fcb.txt"
 cmp kernel/tests/fixtures/fcb/fcbx_expected.txt "$out/$emu-fcb.txt" || fail "$emu FCB transcript vs real DOS"
 { mattrib -i "$media_data" '::FCB*.DAT' | sed 's/  */ /g' | LC_ALL=C sort;
   mdir -a -i "$media_data" :: | awk '$1 ~ /^FCB/ && $2 == "DAT" {print $1, $2, $3}' | LC_ALL=C sort;
 } > "$out/$emu-fcb-fat.txt"
 cmp kernel/tests/fixtures/fcb/fcbx_fat_expected.txt "$out/$emu-fcb-fat.txt" || fail "$emu FCB metadata vs real DOS"
 mcopy -o -i "$media_data" ::FCBKEPT.DAT "$out/$emu-fcb-kept.dat"
 cmp kernel/tests/fixtures/fcb/fcbkept_expected.dat "$out/$emu-fcb-kept.dat" || fail "$emu FCB record bytes vs real DOS"
 mcopy -o -i "$media_volume" ::OUT.TXT "$out/$emu-out.txt"
 printf 'hello\r\n' > "$out/expected-out.txt"
 cmp "$out/expected-out.txt" "$out/$emu-out.txt" || fail "$emu persisted write"
 if [ "$media_system" = C ]; then
  mcopy -o -i "$media_data" ::OUT.TXT "$out/$emu-decoy"
  cmp "$out/decoy" "$out/$emu-decoy" || fail "$emu overwrote A proof"
 fi
 echo "ok $emu: handles, FCB, nested EXEC/INT20/exit codes, memory, clock, vectors, CON; mtools persisted bytes"
done
echo 'PROGRAM SMOKE: 2 emulators, 0 failures'
