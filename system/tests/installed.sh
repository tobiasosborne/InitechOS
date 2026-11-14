#!/bin/sh
# GM5/7/8: the user's installed programs and inputs on ordinary product disks.
# Only AUTOEXEC is removed on disposable clones to reach the DOS prompt.
set -eu
cd "$(dirname "$0")/../.."
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 MTOOLS_SKIP_CHECK=1
mkdir -p build/system
out=$(mktemp -d "$PWD/build/system/installed-smoke.XXXXXX")
echo "system installed workflow evidence: $out"
fail() { echo "FAIL installed workflow: $* (evidence: $out)" >&2; exit 1; }
line() { grep -Fx -- "$2" "$1" >/dev/null || fail "missing [$2] in $1"; }
boot=${SYSTEM_BOOT_IMAGE:-build/image/boot.img}
volume=${SYSTEM_WORK_IMAGE:-build/image/data.img}
awk '$1 !~ /^#/ && NF {print $2}' system/working-volume.list | LC_ALL=C sort > "$out/inventory.expected"
mdir -b -s -i "$volume" :: | sed '/\/$/d; s|^::/||' | LC_ALL=C sort > "$out/inventory.actual"
diff -u "$out/inventory.expected" "$out/inventory.actual" || fail 'unlisted or missing installed file'
# Independently extract every declared input. Missing/stale/truncated files fail.
mkdir -p "$out/installed"
while read -r source destination rest; do
 case "$source" in ''|\#*) continue;; esac
 test -z "$rest" || fail 'invalid manifest'
 path=$out/installed/$destination
 mkdir -p "$(dirname "$path")"
 mcopy -o -i "$volume" "::$destination" "$path" || fail "missing $destination"
 cmp "$source" "$path" || fail "stale installed $destination"
done < system/working-volume.list
python3 system/tests/read-samples.py "$out/installed/APPS"
mdir -i "$volume" :: > "$out/space.txt"
cat "$out/space.txt"
for emulator in ${SYSTEM_SMOKE_CPUS:-qemu bochs}; do
 for workflow in tps base turbo; do
  dir=$out/$emulator-$workflow
  mkdir -p "$dir"
  cp "$boot" "$dir/boot.img"
  cp "$volume" "$dir/data.img"
  mdel -i "$dir/data.img" ::AUTOEXEC.BAT
  # Ordinary commands; no program, table, source or report is injected.
  printf '\n\npath a:\\\ncd apps\n' > "$dir/session"
  case $workflow in
  tps)
   printf 'tpsrep tps.dbf\n       \n004417\n004417\n88-114\ny\n' >> "$dir/session";;
  base)
   printf 'initbase\nuse clients\nlist\nuse parts\nlist\nuse invoice\ndo y2kacct\nquit\n' >> "$dir/session";;
  turbo)
   printf 'copy hello.pas turboin.pas\nturbo\nturboout\n' >> "$dir/session";;
  esac
  printf 'echo installed%scomplete\n' "$workflow" >> "$dir/session"
  build/factory/keys < "$dir/session" > "$dir/keys"
  # The final completion command must be terminal in the key trace.
  sed -i 's/ret/ret,w1000/g; s/,w1000$//; s/q,u,i,t,ret,w1000,/q,u,i,t,ret,w5000,/' "$dir/keys"
  extra=
  [ "$emulator" != bochs ] || extra='--vga-ext none --rtc-time0 699440400'
  finish=$(printf '\ninstalled%scomplete\r\n' "$workflow")
  if ! build/factory/${emulator}_harness --disk "$dir/boot.img" --disk2 "$dir/data.img" \
    $extra --keys-after SHELL-READY --keys "$(cat "$dir/keys")" \
    --expect "$finish" --quit-after "$finish" --timeout-ms 90000 --name run --out "$dir" \
    > "$dir/stdout" 2> "$dir/report"; then
   cat "$dir/report"; fail "$emulator $workflow did not finish"
  fi
  tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
  line "$dir/serial.txt" "installed${workflow}complete"
  if [ "$emulator" = qemu ]; then
   grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$dir/report" >/dev/null || fail 'QEMU faults'
   test "$(grep -c 'CPU Reset' "$dir/run.qemu.log")" -eq 2 || fail 'QEMU resets'
   if grep -Ei 'triple fault|check_exception old:' "$dir/run.qemu.log"; then fail 'QEMU fault trace'; fi
  else
   line "$dir/report" '[bochs] triple_fault=0'
  fi
  case $workflow in
  tps)
   mcopy -i "$dir/data.img" ::APPS/TPSREPT.TXT "$dir/report.txt"
   # Frozen report from REALDOS on MS-DOS 6.22; never minted by this run.
   build/system/tps-report dos/tpsrep/tests/reference/tpsrept_expected.txt "$dir/report.txt" || fail 'real-DOS TPS report differs';;
  base)
   # These literal LIST rows were captured from real dBASE III+.
   sed -n '/^      [123]  /p' apps/initechbase/tests/xbase_prog_diff/screen-mint/boot-list-real/list.screen |
    sed 's/ *$//' > "$dir/clients.expected"
   tr -d '\r\032' < apps/initechbase/tests/xbase_prog_diff/mint/prog-mint/query/REAL.OUT |
    sed -n '/^Record#/,/^      4/p' > "$dir/parts.expected"
   # Strip only the dot-prompt transport joining the next command on a line.
   sed 's/\. [a-z].*//' "$dir/serial.txt" > "$dir/base.txt"
   cat "$dir/clients.expected" "$dir/parts.expected" apps/initechbase/tests/canon/y2k_accounting.out |
    while IFS= read -r expected; do
     [ -z "$expected" ] || line "$dir/base.txt" "$expected"
    done;;
  turbo)
   line "$dir/serial.txt" 'Hello from Turbo Initech.'
   grep '^TURBO-NATIVE-OK form=0 ' "$dir/serial.txt" >/dev/null || fail 'resident compiler did not emit flat hello'
   mcopy -i "$dir/data.img" ::APPS/TURBOOUT.COM "$dir/hello.com"
   test -s "$dir/hello.com" || fail 'empty Pascal executable'
   # The greeting is an authored Pascal-language result; compare with FPC
   # independently too, rather than deriving expected stdout from Turbo.
   mkdir -p "$dir/fpc"
   fpc -v0 -FE"$dir/fpc" -FU"$dir/fpc" -ohello "$out/installed/APPS/HELLO.PAS" > "$dir/fpc.log"
   "$dir/fpc/hello" > "$dir/hello.expected"
   line "$dir/serial.txt" "$(cat "$dir/hello.expected")";;
  esac
  echo "ok $emulator installed $workflow: ordinary product image, independent result"
 done
done
echo 'system: installed TPS, Base and Turbo workflows GREEN'
