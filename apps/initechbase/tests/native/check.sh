#!/bin/sh
# External tier-2 observer: one product boot image, separate disposable TEST disk.
set -eu
mode=$1 root=$2 exe=$3 ovl=$4 out=$5
src=$PWD
bins=$root/build/apps/initechbase/host
mkdir -p "$out"
export MTOOLS_SKIP_CHECK=1 INITECH_QMP_STDIO=1 INITECH_TEST_RAM_MB=8
fail() { echo "ERROR native $mode: $*" >&2; exit 2; }
keys() { printf '%s\n' "$1" | "$root/build/factory/keys" | sed 's/\(^\|,\)/\1k/g'; }
disk=$out/test.img
cp "$root/build/image/boot.img" "$out/boot.img"
cp "$root/build/system/system.elf" "$out/system.elf"
mcopy -o -i "$out/boot.img@@32256" ::INITDOS.SYS "$out/INITDOS.SYS"
objcopy -O binary "$out/system.elf" "$out/elf.bin"
cmp "$out/INITDOS.SYS" "$out/elf.bin" >/dev/null || fail 'image and observer ELF do not match'
dd if=/dev/zero of="$disk" bs=512 count=2880 2>/dev/null
mformat -i "$disk" -f 1440 -N 0x49424153 -v TEST ::
mmd -i "$disk" ::APPS
mcopy -i "$disk" "$exe" ::APPS/INITBASE.EXE
mcopy -i "$disk" "$ovl" ::APPS/INITBASE.OVL
mcopy -i "$disk" "$root/build/dos/COMMAND.COM" ::
printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$out/CONFIG.SYS"
mcopy -i "$disk" "$out/CONFIG.SYS" ::
launch=$(keys '

cd apps
initbase')
case $mode in
 forms)
  cat > "$out/PROBE.PRG" <<'PRG'
SET TALK OFF
SET SAFETY OFF
SET STATUS OFF
CLEAR
@ 0,0 TO 24,79
WAIT ""
PRG
  oracle=$src/tests/xbase_prog_diff/screen-mint/at-to-b/TE.screen
  spec="$launch,w300,$(keys 'do probe'),w500,D$out/page.ppm" ;;
 enter)
  cp tests/prompt-tail/SETUP/TAIL.PRG "$out/PROBE.PRG"
  oracle=$src/tests/prompt-tail/entered.screen
  spec="$launch,w300,$(keys 'do probe'),w500,kshift-x,kequal,k1,kret,w500,D$out/page.ppm" ;;
 keys)
  spec="$launch,w300,kleft,w150,D$out/page.ppm" ;;
 color|color-state|controls|continuation)
  printf 'SET TALK OFF\nSET STATUS OFF\nCLEAR\n' > "$out/PROBE.PRG"
  case $mode in
  color) printf 'SET COLOR TO W+/B\n@ 2,3 SAY "S"\n?? "O"\n' >> "$out/PROBE.PRG" ;;
  color-state)
   printf '@ 2,3 SAY "S"\n?? "O"\n' >> "$out/PROBE.PRG"
   launch="$launch,w300,ks,ke,kt,kspc,kc,ko,kl,ko,kr,kspc,kt,ko,kspc,kw,kshift-equal,kslash,kb,kret,w150,$(keys 'set status off'),w150,$(keys 'set status on')" ;;
  controls)
   printf '@ 23,78 SAY "XYZ"+CHR(13)+"Q"+CHR(10)+"R"\n' >> "$out/PROBE.PRG"
   launch=$(keys '

cd apps
initbase >out.txt') ;;
  continuation) printf '@ 24,75 SAY "abcdefgh"\n?? REPLICATE("X",80)\n' >> "$out/PROBE.PRG" ;;
  esac
  printf 'WAIT ""\n' >> "$out/PROBE.PRG"
  spec="$launch,w300,$(keys 'do probe'),w500,D$out/page.ppm" ;;
 *) fail 'unknown oracle';;
esac
if [ -f "$out/PROBE.PRG" ]; then mcopy -i "$disk" "$out/PROBE.PRG" ::APPS/PROBE.PRG; fi
rc=0
INITECH_TEXT_SCREEN_TOOL="$bins/screen_capture" INITECH_TEXT_SCREEN_ELF="$out/system.elf" \
 "$root/build/factory/qemu_harness" --disk "$out/boot.img" --disk2 "$disk" \
 --name run --out "$out" --timeout-ms 15000 --mouse "$spec" --keys-after SHELL-READY --legacy-quit \
 > "$out/stdout" 2> "$out/report" || rc=$?
if [ "$rc" -ne 0 ] || ! grep -Fx '[harness] OK=1' "$out/report" >/dev/null; then cat "$out/report";fail 'incomplete guest';fi
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report" >/dev/null || fail 'emulator fault'
if grep -E 'TRIPLE FAULT|PANIC|HALTED|STACK OVERFLOW' "$out/run.serial" >/dev/null; then fail 'unexpected guest fault';fi
[ -s "$out/page.ppm.screen" ] || fail 'no completed screen capture'
case $mode in
 forms|enter) "$bins/screen_tool" compare "$oracle" "$out/page.ppm.screen" ;;
 *) "$bins/test_console_review" "$mode" "$out/page.ppm.screen" ;;
esac
