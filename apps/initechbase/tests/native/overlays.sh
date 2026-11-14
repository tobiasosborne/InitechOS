#!/bin/sh
# Overlay format/lifetime properties; the oracle runs outside the target.
set -eu
mode=$1 root=$2 exe=$3 ovl=$4 out=$5
mkdir -p "$out"
export MTOOLS_SKIP_CHECK=1 INITECH_QMP_STDIO=1 INITECH_TEST_RAM_MB=8
fail() { echo "ERROR native $mode: $*" >&2;exit 2; }
red() { echo "RED native $mode: $*" >&2;exit 1; }
disk=$out/test.img
dd if=/dev/zero of="$disk" bs=512 count=2880 2>/dev/null
mformat -i "$disk" -f 1440 -N 0x49424153 -v TEST ::
mmd -i "$disk" ::APPS
mcopy -i "$disk" "$exe" ::APPS/INITBASE.EXE
case $mode in
 ovl-crc)
  cp "$ovl" "$out/BAD.OVL"
  # Own on-disk format: header 32, first 16-byte directory entry starts
  # with the little-endian payload offset. Change only payload byte 64.
  set -- $(od -An -tu1 -j 32 -N 4 "$ovl")
  [ "$#" -eq 4 ] || fail 'overlay directory absent'
  offset=$(($1+256*$2+65536*$3+16777216*$4+64))
  byte=$(od -An -tu1 -j "$offset" -N 1 "$ovl")
  [ -n "$byte" ] || fail 'overlay payload absent'
  octal=$(printf '%03o' "$((byte ^ 1))")
  printf '%b' "\0$octal" | dd of="$out/BAD.OVL" bs=1 seek="$offset" conv=notrunc 2>/dev/null
  mcopy -i "$disk" "$out/BAD.OVL" ::APPS/INITBASE.OVL ;;
 ovl-missing) ;;
 ovl-callback|ovl-hook) mcopy -i "$disk" "$ovl" ::APPS/INITBASE.OVL ;;
 *) fail 'unknown overlay oracle';;
esac
mcopy -i "$disk" "$root/build/dos/COMMAND.COM" ::
printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$out/CONFIG.SYS"
mcopy -i "$disk" "$out/CONFIG.SYS" ::
printf '?? "[OVL-ACCEPTED]"\nRETURN\n' > "$out/PROBE.PRG"
mcopy -i "$disk" "$out/PROBE.PRG" ::APPS/PROBE.PRG
printf '\n\ncd apps\ninitbase\n' | "$root/build/factory/keys" | sed 's/\(^\|,\)/\1k/g' > "$out/launch"
printf 'do probe\n' | "$root/build/factory/keys" | sed 's/\(^\|,\)/\1k/g' > "$out/probe"
rc=0
"$root/build/factory/qemu_harness" --disk "$root/build/image/boot.img" --disk2 "$disk" --name run --out "$out" \
 --keys-after SHELL-READY --mouse "$(cat "$out/launch"),w300,$(cat "$out/probe"),w600,D$out/page.ppm" \
 --legacy-quit --timeout-ms 15000 > "$out/stdout" 2> "$out/report" || rc=$?
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report" >/dev/null || fail 'emulator fault'
case $mode in
 ovl-callback|ovl-hook)
  why=fn;[ "$mode" != ovl-hook ] || why=hook
  if grep -E "IBASE OVERLAY PANIC: $why loaded=00000003 requested=00000003 depth=00000001 pointer=[0-9A-F]{8}" "$out/run.serial" >/dev/null; then
   grep -F 'PANIC vec=06 err=00000000' "$out/run.serial" >/dev/null || fail 'missing invariant exception'
   grep -Fx HALTED "$out/run.serial" >/dev/null || fail 'missing invariant stop'
   red "SET retained a slot $why; explicit lifetime guard rejected it"
  fi ;;
esac
if [ "$rc" -ne 0 ] || ! grep -Fx '[harness] OK=1' "$out/report" >/dev/null; then cat "$out/report";fail 'incomplete guest';fi
if grep -E 'PANIC|HALTED' "$out/run.serial" >/dev/null; then fail 'unexpected guest fault';fi
case $mode in
 ovl-crc|ovl-missing)
  if grep -F '[OVL-ACCEPTED]' "$out/run.serial" >/dev/null; then red 'executed a program with corrupt or missing overlay media';fi
  grep -F 'Insert System Disk and press ENTER' "$out/run.serial" >/dev/null || fail 'neither refusal nor acceptance observed' ;;
 *) grep -F '[OVL-ACCEPTED]' "$out/run.serial" >/dev/null || fail 'valid-media program did not execute';;
esac
echo "GREEN native $mode: overlay property holds"
