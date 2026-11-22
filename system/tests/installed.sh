#!/bin/sh
# GM5/8 and C activation step 19a: ordinary startup and installed samples.
set -eu
cd "$(dirname "$0")/../.."
. factory/image/test-media.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 MTOOLS_SKIP_CHECK=1 LC_ALL=C
emulators=${SYSTEM_SMOKE_CPUS-qemu bochs}
sh factory/smoke-selection.sh system-emulators "$emulators" 'qemu bochs'
mkdir -p build/system
out=$(mktemp -d "$PWD/build/system/installed-smoke.XXXXXX")
echo "system installed workflow evidence: $out"
fail() { echo "FAIL installed workflow: $* (evidence: $out)" >&2; exit 1; }
line() { grep -Fx -- "$2" "$1" >/dev/null || fail "missing [$2] in $1"; }

# The staged distribution names expire at activation step 20.
images=${1:-build/image}
boot=$images/c.img
data=$images/a-data.img
volume=$boot@@32256
sha256sum "$boot" "$data" > "$out/masters.sha256"
sfdisk -d "$boot" > "$out/partition"
grep -Fx 'sector-size: 512' "$out/partition" >/dev/null || fail 'C: sector size'
sed -n '/ : start=/p' "$out/partition" > "$out/partition.entries"
[ "$(wc -l < "$out/partition.entries")" -eq 1 ] || fail 'C: expected one partition'
grep -Eq 'start= *63, size= *16384, type=1, bootable$' "$out/partition.entries" ||
 fail 'C: expected one active FAT12 partition at sector 63 with 16384 sectors'

awk '$1 !~ /^#/ && NF {print $2}' system/working-volume.list | sort > "$out/inventory.expected"
mdir -b -s -i "$volume" :: > "$out/visible"
sed '/\/$/d; s|^::/||' "$out/visible" | sort > "$out/inventory.actual"
diff -u "$out/inventory.expected" "$out/inventory.actual" || fail 'unlisted or missing visible C: file'

extract() {
 path=$out/installed/$2
 mkdir -p "$(dirname "$path")"
 mcopy -i "$volume" "::/$2" "$path" || fail "missing C:\\$2"
 cmp -s "$1" "$path" || fail "C:\\$2 differs from $1"
}
while read -r source destination rest; do
 case "$source" in ''|\#*) continue;; esac
 test -z "$rest" || fail 'invalid manifest'
 case "$destination" in
 CONFIG.SYS|AUTOEXEC.BAT) source=build/system/installed/c-boot/$destination;;
 esac
 extract "$source" "$destination"
done < system/working-volume.list

# GM1's two hidden boot entries precede the visible root, with H/S/R.
mdir -a -b -i "$volume" :: > "$out/root"
printf '%s\n' '::/IO.SYS' '::/INITDOS.SYS' > "$out/boot.expected"
head -n 2 "$out/root" > "$out/boot.actual"
cmp -s "$out/boot.expected" "$out/boot.actual" || fail 'C: boot file order'
mattrib -i "$volume" ::IO.SYS ::INITDOS.SYS > "$out/boot.attributes"
for file in IO.SYS INITDOS.SYS; do
 grep -Eq "^[[:space:]]*SHR[[:space:]]+::/${file%.*}\\.SYS$" "$out/boot.attributes" ||
  fail "C:\\$file must be hidden, system and read-only"
done
extract build/boot/IO.SYS IO.SYS
extract "${SYSTEM_IMAGE:-build/system/INITDOS.SYS}" INITDOS.SYS
python3 system/tests/read-samples.py "$out/installed/SAMPLES"
SYSTEM_WORK_IMAGE=$volume sh system/tests/capacity.sh > "$out/space.txt" ||
 { cat "$out/space.txt"; fail 'C: working capacity'; }
cat "$out/space.txt"

for emulator in $emulators; do
 for workflow in tps base; do
  # TPS writes its report with DATA attached. Base also starts with no ATA slave.
  attachment=with-data
  [ "$workflow" != base ] || attachment=alone
  dir=$out/$emulator-$workflow
  mkdir -p "$dir"
  media_prepare staged "$attachment" "$dir" "$images"
  # Ordinary commands. Retain CONFIG, AUTOEXEC and the PATH under test.
  printf '%s\n' set > "$dir/session"
  case $attachment in
  with-data) printf '%s\n' 'if exist a:\ledger.txt echo installed-a-present' >> "$dir/session"
   a_marker=installed-a-present;;
  alone) printf '%s\n' 'if not exist a:\ledger.txt echo installed-a-absent' >> "$dir/session"
   a_marker=installed-a-absent;;
  esac
  case $workflow in
  tps)
   printf 'cd samples\ntpsrep tps.dbf\n       \n004417\n004417\n88-114\ny\n' >> "$dir/session";;
  base)
   printf 'cd apps\ninitbase\nuse \\samples\\clients\nlist\nuse \\samples\\parts\nlist\nuse \\samples\\invoice\ndo \\samples\\y2kacct\nquit\n' >> "$dir/session"
   printf '%s\n' 'echo base-complete>BASE.OK' >> "$dir/session";;
  esac
  printf 'echo installed%scomplete\n' "$workflow" >> "$dir/session"
  build/factory/keys < "$dir/session" > "$dir/keys"
  # The shared trace form accepts both keys and waits. Leave the final
  # completion command terminal.
  sed 's/^/k/; s/,/,k/g; s/kret,/kret,w1000,/g; s/kq,ku,ki,kt,kret,w1000,/kq,ku,ki,kt,kret,w5000,/' \
   "$dir/keys" > "$dir/trace"
  case $emulator in
  qemu) set -- --mouse "$(cat "$dir/trace")";;
  bochs) set -- --vga-ext none --rtc-time0 699440400 --keys "$(cat "$dir/trace")";;
  esac
  finish=installed${workflow}complete
  # QEMU matches a substring; require the output line, not the echoed command.
  [ "$emulator" != qemu ] || finish=$(printf '\n%s\r' "$finish")
  if ! media_run "$emulator" "build/factory/${emulator}_harness" "$@" \
    --keys-after SHELL-READY \
    --expect "$finish" --quit-after "$finish" --timeout-ms 90000 --name run --out "$dir" \
    > "$dir/stdout" 2> "$dir/report"; then
   cat "$dir/report"; fail "$emulator $workflow did not finish"
  fi
  tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
  line "$dir/serial.txt" 'SYSINIT: source=CONFIG.SYS FILES=20 cap=20 SHELL=COMMAND.COM /P'
  line "$dir/serial.txt" 'SYSINIT: DEVICE=ANSI.SYS ansi-enabled'
  line "$dir/serial.txt" 'C:\>set'
  line "$dir/serial.txt" 'COMSPEC=C:\COMMAND.COM'
  line "$dir/serial.txt" 'PATH=C:\;C:\APPS'
  line "$dir/serial.txt" "$a_marker"
  line "$dir/serial.txt" "installed${workflow}complete"
  if grep -E '^(FLAIR-BOOT-SCREEN|PANIC|KBD-RING-DROP)' "$dir/serial.txt"; then
   fail "$emulator $workflow started FLAIR or reported a guest fault"
  fi
  if [ "$emulator" = qemu ]; then
   grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$dir/report" >/dev/null || fail 'QEMU faults'
   test "$(grep -c 'CPU Reset' "$dir/run.qemu.log")" -eq 2 || fail 'QEMU resets'
   if grep -Ei 'triple fault|check_exception old:' "$dir/run.qemu.log"; then fail 'QEMU fault trace'; fi
  else
   line "$dir/report" '[bochs] triple_fault=0'
  fi
  media_readback "$emulator" "$dir/report"
  fsck.fat -n "$media_volume" > "$dir/fsck.c" || { cat "$dir/fsck.c"; fail 'written C: filesystem'; }
  case $workflow in
  tps)
   mcopy -i "$media_volume" ::SAMPLES/TPSREPT.TXT "$dir/report.txt" ||
    fail "$emulator TPS report absent from written C:"
   # Frozen report from REALDOS on MS-DOS 6.22.
   build/system/tps-report dos/tpsrep/tests/reference/tpsrept_expected.txt "$dir/report.txt" ||
    fail 'real-DOS TPS report differs';;
  base)
   # These literal LIST rows were captured from real dBASE III+.
   sed -n '/^      [123]  /p' apps/initechbase/tests/xbase_prog_diff/screen-mint/boot-list-real/list.screen |
    sed 's/ *$//' > "$dir/clients.expected"
   tr -d '\r\032' < apps/initechbase/tests/xbase_prog_diff/mint/prog-mint/query/REAL.OUT |
    sed -n '/^Record#/,/^      4/p' > "$dir/parts.expected"
   test -s "$dir/clients.expected" && test -s "$dir/parts.expected" ||
    fail 'missing retained Base LIST rows'
   # Strip only the dot-prompt transport joining the next command on a line.
   sed 's/\. [a-z].*//' "$dir/serial.txt" > "$dir/base.txt"
   cat "$dir/clients.expected" "$dir/parts.expected" apps/initechbase/tests/canon/y2k_accounting.out > "$dir/base.expected"
   while IFS= read -r expected; do
    [ -z "$expected" ] || line "$dir/base.txt" "$expected"
   done < "$dir/base.expected"
   mcopy -i "$media_volume" ::APPS/BASE.OK "$dir/base.ok"
   printf 'base-complete\r\n' > "$dir/base.want"
   cmp -s "$dir/base.want" "$dir/base.ok" || fail "$emulator Base did not return to the C: shell";;
  esac
  if [ -n "$media_data" ]; then
   cmp -s "$data" "$media_data" || fail "$emulator $workflow changed A:"
  fi
  echo "ok $emulator installed $workflow ($attachment): ordinary C: startup and independent readback"
 done
done
sha256sum -c "$out/masters.sha256"
echo 'system: installed C: startup, TPS and Base workflows GREEN'
