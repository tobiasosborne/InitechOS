#!/bin/sh
# GM5/7/8 and C activation step 19: ordinary startup and installed samples.
set -eu
cd "$(dirname "$0")/../.."
. factory/image/test-media.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 MTOOLS_SKIP_CHECK=1 LC_ALL=C
emulators=${SYSTEM_SMOKE_CPUS-qemu bochs}
workflows=${SYSTEM_SMOKE_WORKFLOWS-tps base turbo}
sh factory/smoke-selection.sh system-emulators "$emulators" 'qemu bochs'
sh factory/smoke-selection.sh system-workflows "$workflows" 'tps base turbo'
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
cmp -s turbo/compiler/turbo.pas "$out/installed/TURBO/TURBO.PAS" ||
 fail 'C: installed compiler source differs from turbo/compiler/turbo.pas'
cmp -s system/samples/DEMO.PAS "$out/installed/TURBO/DEMO.PAS" ||
 fail 'C: installed DEMO.PAS differs from the pinned MINT02 source'

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

encode_trace() {
 build/factory/keys < "$1" > "$1.keys" || fail 'session contains an unsupported key'
 # Leave the final command terminal and give Base QUIT time to return to DOS.
 sed 's/^/k/; s/,/,k/g; s/kret,/kret,w1000,/g; s/kq,ku,ki,kt,kret,w1000,/kq,ku,ki,kt,kret,w5000,/' "$1.keys"
}

for emulator in $emulators; do
 for workflow in $workflows; do
  # TPS writes its report with DATA attached. Base and Turbo boot C alone.
  attachment=alone
  [ "$workflow" != tps ] || attachment=with-data
  dir=$out/$emulator-$workflow
  mkdir -p "$dir"
  media_prepare staged "$attachment" "$dir" "$images"
  # Ordinary commands. Retain CONFIG, AUTOEXEC and the PATH under test.
  printf '%s\n' set > "$dir/session"
  case $workflow in
  tps) printf '%s\n' 'if exist a:\ledger.txt echo installed-a-present' >> "$dir/session"
   a_marker=installed-a-present;;
  base) printf '%s\n' 'if not exist a:\ledger.txt echo installed-a-absent' >> "$dir/session"
   a_marker=installed-a-absent;;
  turbo) a_marker=;;
  esac
  case $workflow in
  tps)
   printf 'cd samples\ntpsrep tps.dbf\n       \n004417\n004417\n88-114\ny\n' >> "$dir/session";;
  base)
   printf 'cd apps\ninitbase\nuse \\samples\\clients\nlist\nuse \\samples\\parts\nlist\nuse \\samples\\invoice\ndo \\samples\\y2kacct\nquit\n' >> "$dir/session"
   printf '%s\n' 'echo base-complete>BASE.OK' >> "$dir/session";;
  turbo)
   printf 'cd turbo\nturbo\n' >> "$dir/session"
   printf 'turbo hello.pas\nturboout.com\necho installedturbocomplete\n' > "$dir/compile";;
  esac
  if [ "$workflow" != turbo ]; then
   printf 'echo installed%scomplete\n' "$workflow" >> "$dir/session"
  fi
  trace=$(encode_trace "$dir/session")
  vga=none
  if [ "$workflow" = turbo ]; then
   case $emulator in
   qemu) capture=D$dir/diagnostic;;
   bochs) capture=Tdiagnostic; vga=vbe;;
   esac
   compile=$(encode_trace "$dir/compile")
   # F3 opens the installed DEMO.PAS, F9 compiles it and Alt-X returns to DOS.
   trace="$trace,w3000,kf3,w1000,kd,ke,km,ko,kdot,kp,ka,ks,kret,w1500,kf9,w3000,$capture,kalt-x,w3000,$compile"
  fi
  printf '%s\n' "$trace" > "$dir/trace"
  case $emulator in
  qemu) set -- --mouse "$trace";;
  bochs) set -- --vga-ext "$vga" --rtc-time0 699440400 --keys "$trace";;
  esac
  finish=installed${workflow}complete
  # QEMU matches a substring; require the output line, not the echoed command.
  [ "$emulator" != qemu ] || finish=$(printf '\n%s\r' "$finish")
  if ! (
   if [ "$workflow" = turbo ]; then
    export INITECH_TEXT_SCREEN_ELF=${SYSTEM_ELF:-build/system/system.elf}
    export INITECH_TEXT_SCREEN_TOOL=$PWD/build/apps/initechbase/host/screen_capture
   fi
   media_run "$emulator" "build/factory/${emulator}_harness" "$@" \
    --keys-after SHELL-READY \
    --expect "$finish" --quit-after "$finish" --timeout-ms 90000 --name run --out "$dir"
  ) > "$dir/stdout" 2> "$dir/report"; then
   cat "$dir/report"; fail "$emulator $workflow did not finish"
  fi
  tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
  line "$dir/serial.txt" 'SYSINIT: source=CONFIG.SYS FILES=20 cap=20 SHELL=COMMAND.COM /P'
  line "$dir/serial.txt" 'SYSINIT: DEVICE=ANSI.SYS ansi-enabled'
  line "$dir/serial.txt" 'C:\>set'
  line "$dir/serial.txt" 'COMSPEC=C:\COMMAND.COM'
  line "$dir/serial.txt" 'PATH=C:\;C:\APPS'
  [ -z "$a_marker" ] || line "$dir/serial.txt" "$a_marker"
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
  turbo)
   # Actual console cells. The wording comes from the pinned TP 5.5 MINT02
   # E05 diagnostic and its IDE form from MINT06.
   screen=$dir/diagnostic.screen
   case $emulator in
   qemu)
    line "$screen" 'INITECH-SCREEN 1 CP437 80 25'
    sed -n '3,27p' "$screen" > "$dir/diagnostic.rows";;
   bochs)
    [ -f "$screen" ] && [ "$(wc -c < "$screen")" -eq 4002 ] ||
     fail 'Bochs Turbo diagnostic screen missing or short'
    od -An -v -N4000 -tu1 "$screen" |
     awk '{for (i=1;i<=NF;i+=2) {printf "%c", $i; if (++col==80) {print ""; col=0}}}' > "$dir/diagnostic.rows";;
   esac
   sed 's/ *$//' "$dir/diagnostic.rows" > "$dir/diagnostic.txt"
   line "$dir/diagnostic.txt" 'Error 85: ";" expected'
   line "$dir/diagnostic.txt" 'program e05;'
   line "$dir/serial.txt" 'C:\TURBO>turbo hello.pas'
   mcopy -i "$media_volume" ::TURBO/TURBOOUT.COM "$dir/hello.com" ||
    fail "$emulator Turbo output absent from written C:"
   test -s "$dir/hello.com" || fail 'empty Pascal executable'
   mdir -a -b -i "$media_volume" ::TURBO/ > "$dir/turbo-files"
   if grep -E '/(TURBOOUT\.S|DEMO\.(EXE|COM))$' "$dir/turbo-files"; then
    fail "$emulator Turbo left an unexpected build output"
   fi
   mkdir "$dir/fpc"
   fpc -v0 -FE"$dir/fpc" -FU"$dir/fpc" -ohello "$out/installed/TURBO/HELLO.PAS" > "$dir/fpc.log"
   "$dir/fpc/hello" > "$dir/hello.expected"
   awk '$0 == "C:\\TURBO>turboout.com" {run=1; next}
        run && index($0, "C:\\TURBO>")==1 {exit}
        run {print}' "$dir/serial.txt" > "$dir/hello.actual"
   cmp -s "$dir/hello.expected" "$dir/hello.actual" || fail "$emulator Hello stdout differs from FPC";;
  esac
  if [ -n "$media_data" ]; then
   cmp -s "$data" "$media_data" || fail "$emulator $workflow changed A:"
  fi
  echo "ok $emulator installed $workflow ($attachment): ordinary C: startup and independent readback"
 done
done
sha256sum -c "$out/masters.sha256"
echo "system: installed C: workflows GREEN ($workflows)"
