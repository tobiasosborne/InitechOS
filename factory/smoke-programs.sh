#!/bin/sh
# IBM DOS 3.30 TR INT21 program probes on a separate writable data volume.
set -eu
cd "$(dirname "$0")/.."
out=${PROGRAM_SMOKE_OUT:-build/smoke-programs}
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
fail() { echo "FAIL program smoke: $*" >&2; exit 1; }
for tool in mformat mcopy mmd; do command -v "$tool" >/dev/null || fail "missing $tool"; done
truncate -s 0 "$out/test.img"
truncate -s 1474560 "$out/test.img"
mformat -i "$out/test.img" -f 1440 -N 0x50524f47 ::
mcopy -i "$out/test.img" "${PROGRAM_SMOKE_PROGRAMS:-build/dos/test-programs}"/*.COM ::
mmd -i "$out/test.img" ::SUB
printf 'Hello, world!\r\n' > "$out/hello.txt"
printf 'Second file\r\n' > "$out/second.txt"
truncate -s 98304 "$out/big.dat"
printf 'BEYOND-64KiB!' | dd of="$out/big.dat" bs=1 seek=80000 conv=notrunc status=none
mcopy -i "$out/test.img" "$out/hello.txt" ::HELLO.TXT
mcopy -i "$out/test.img" "$out/second.txt" ::SECOND.TXT
mcopy -i "$out/test.img" "$out/big.dat" ::BIG.DAT
cat > "$out/session.txt" <<'EOF'


multi
fcbx
nest
cd ..
services
write
conin
dir
echo programsdone
EOF
build/factory/keys < "$out/session.txt" > "$out/keys"
finish=$(printf '\nprogramsdone\r\n')
for emu in qemu bochs; do
 cp "$out/test.img" "$out/$emu-data.img"
 keys=$(cat "$out/keys")
 marker=$finish
 if [ "$emu" = bochs ]; then
  marker=programsdone
  keys=$(printf '%s' "$keys" | sed "s/ret/ret,w${PROGRAM_SMOKE_WAIT_MS:-1000}/g")
 fi
 if ! "build/factory/${emu}_harness" --disk "${SMOKE_BOOT:-build/image/boot.img}" \
  --disk2 "$out/$emu-data.img" --keys-after SHELL-READY --keys "$keys" \
  --expect "$marker" --quit-after "$marker" --timeout-ms "${PROGRAM_SMOKE_TIMEOUT_MS:-55000}" \
  --name "$emu" --out "$out" > "$out/$emu.stdout" 2> "$out/$emu.report"; then
  cat "$out/$emu.report"; fail "$emu did not complete";
 fi
 tr -d '\r' < "$out/$emu.serial" > "$out/$emu.txt"
 for line in 'MO-A1=Hel' 'MO-B1=Sec' 'MO-A2=Hello' 'MO-BIG=BEYOND-64KiB!' 'MO-DONE' \
  'FCBX-BEGIN' 'FCBX-END' 'EXEC-NESTED-DEPTH-4-BUSY' \
  'EXEC-NESTED-RC=7 PSP-RESTORED INT20-OK' 'SERVICES-MEMORY-VECTORS-CLOCK-OK' \
  'WRITE-READBACK-OK' 'CONIN-LINE=dir' 'programsdone'; do
  grep -Fx "$line" "$out/$emu.txt" >/dev/null || fail "$emu missing [$line]";
 done
 if grep -E '(SERVICES-FAIL|EXEC-NESTED-FAIL|WRITE-.*-FAIL|PANIC|Bad command)' "$out/$emu.txt"; then fail "$emu guest failure"; fi
 grep 'triple_fault=0' "$out/$emu.report" >/dev/null || fail "$emu CPU fault"
 if [ "$emu" = qemu ]; then
  grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/qemu.report" >/dev/null || fail 'QEMU fault/reset verdict'
  [ "$(grep -c 'CPU Reset' "$out/qemu.qemu.log")" -eq 2 ] || fail 'QEMU unexpected reset'
 fi
 # Independent captures: the same FCBX source executed under real MS-DOS 6.22.
 sed -n '/^FCBX-BEGIN$/,/^FCBX-END$/p' "$out/$emu.txt" > "$out/$emu-fcb.txt"
 cmp kernel/tests/fixtures/fcb/fcbx_expected.txt "$out/$emu-fcb.txt" || fail "$emu FCB transcript vs real DOS"
 { mattrib -i "$out/$emu-data.img" '::FCB*.DAT' | sed 's/  */ /g' | LC_ALL=C sort;
   mdir -a -i "$out/$emu-data.img" :: | awk '$1 ~ /^FCB/ && $2 == "DAT" {print $1, $2, $3}' | LC_ALL=C sort;
 } > "$out/$emu-fcb-fat.txt"
 cmp kernel/tests/fixtures/fcb/fcbx_fat_expected.txt "$out/$emu-fcb-fat.txt" || fail "$emu FCB metadata vs real DOS"
 mcopy -o -i "$out/$emu-data.img" ::FCBKEPT.DAT "$out/$emu-fcb-kept.dat"
 cmp kernel/tests/fixtures/fcb/fcbkept_expected.dat "$out/$emu-fcb-kept.dat" || fail "$emu FCB record bytes vs real DOS"
 mcopy -o -i "$out/$emu-data.img" ::OUT.TXT "$out/$emu-out.txt"
 printf 'hello\r\n' > "$out/expected-out.txt"
 cmp "$out/expected-out.txt" "$out/$emu-out.txt" || fail "$emu persisted write"
 echo "ok $emu: handles, FCB, nested EXEC/INT20/exit codes, memory, clock, vectors, CON; mtools persisted bytes"
done
echo 'PROGRAM SMOKE: 2 emulators, 0 failures'
