#!/bin/sh
# GM7/8 and C activation step 19b: run the installed Turbo Initech examples.
set -eu
cd "$(dirname "$0")/../.."
. factory/image/test-media.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 MTOOLS_SKIP_CHECK=1 LC_ALL=C
emulators=${SYSTEM_SMOKE_CPUS-qemu bochs}
sh factory/smoke-selection.sh system-emulators "$emulators" 'qemu bochs'
out=$(mktemp -d "$PWD/build/system/installed-smoke.XXXXXX")
echo "system installed Turbo evidence: $out"
fail() { echo "FAIL installed Turbo: $* (evidence: $out)" >&2; exit 1; }
line() { grep -Fx -- "$2" "$1" >/dev/null || fail "missing [$2] in $1"; }

# The staged distribution names expire at activation step 20.
images=${1:-build/image}
boot=$images/c.img
sha256sum "$boot" > "$out/master.sha256"
volume=$boot@@32256
for file in HELLO.PAS DEMO.PAS TURBO.PAS TURBO.COM; do
 mcopy -i "$volume" "::TURBO/$file" "$out/$file" || fail "missing C:\\TURBO\\$file"
done
cmp -s build/system/installed/HELLO.PAS "$out/HELLO.PAS" || fail 'C: installed HELLO.PAS differs from its producer'
cmp -s system/samples/DEMO.PAS "$out/DEMO.PAS" || fail 'C: installed DEMO.PAS differs from the pinned MINT02 source'
cmp -s turbo/compiler/turbo.pas "$out/TURBO.PAS" || fail 'C: installed compiler source differs from turbo/compiler/turbo.pas'
cmp -s build/turbo/compiler/TURBO.COM "$out/TURBO.COM" || fail 'C: installed compiler executable differs from its producer'

mkdir "$out/fpc"
fpc -v0 -FE"$out/fpc" -FU"$out/fpc" -ohello "$out/HELLO.PAS" > "$out/fpc.log"
"$out/fpc/hello" > "$out/hello.expected"
encode_trace() {
 build/factory/keys < "$1" > "$1.keys" || fail 'session contains an unsupported key'
 sed 's/^/k/; s/,/,k/g; s/kret,/kret,w1000,/g' "$1.keys"
}

printf 'set\ncd turbo\nturbo\n' > "$out/session"
printf 'turbo hello.pas\nturboout.com\necho installedturbocomplete\n' > "$out/compile"
session=$(encode_trace "$out/session")
compile=$(encode_trace "$out/compile")
for emulator in $emulators; do
 dir=$out/$emulator-turbo
 mkdir "$dir"
 media_prepare staged alone "$dir" "$images"
 # F3 opens the installed DEMO.PAS, F9 compiles it and Alt-X returns to DOS.
 case $emulator in
 qemu) capture=D$dir/diagnostic; finish=$(printf '\ninstalledturbocomplete\r');;
 bochs) capture=Tdiagnostic; finish=installedturbocomplete;;
 esac
 trace="$session,w3000,kf3,w1000,kd,ke,km,ko,kdot,kp,ka,ks,kret,w1500,kf9,w3000,$capture,kalt-x,w3000,$compile"
 printf '%s\n' "$trace" > "$dir/trace"
 case $emulator in
 qemu) set -- --mouse "$trace";;
 bochs) set -- --vga-ext vbe --rtc-time0 699440400 --keys "$trace";;
 esac
 if ! INITECH_TEXT_SCREEN_ELF=${SYSTEM_ELF:-build/system/system.elf} \
      INITECH_TEXT_SCREEN_TOOL=$PWD/build/apps/initechbase/host/screen_capture \
      media_run "$emulator" "build/factory/${emulator}_harness" "$@" \
       --keys-after SHELL-READY --expect "$finish" --quit-after "$finish" \
       --timeout-ms 90000 --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"; then
  cat "$dir/report"; fail "$emulator did not finish"
 fi
 tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
 line "$dir/serial.txt" 'SYSINIT: source=CONFIG.SYS FILES=20 cap=20 SHELL=COMMAND.COM /P'
 line "$dir/serial.txt" 'C:\>set'
 line "$dir/serial.txt" 'COMSPEC=C:\COMMAND.COM'
 line "$dir/serial.txt" 'PATH=C:\;C:\APPS'
 line "$dir/serial.txt" 'installedturbocomplete'
 if grep -E '^(FLAIR-BOOT-SCREEN|PANIC|KBD-RING-DROP)' "$dir/serial.txt"; then
  fail "$emulator started FLAIR or reported a guest fault"
 fi
 screen=$dir/diagnostic.screen
 case $emulator in
 qemu)
  grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$dir/report" >/dev/null || fail 'QEMU faults'
  test "$(grep -c 'CPU Reset' "$dir/run.qemu.log")" -eq 2 || fail 'QEMU resets'
  if grep -Ei 'triple fault|check_exception old:' "$dir/run.qemu.log"; then fail 'QEMU fault trace'; fi
  line "$screen" 'INITECH-SCREEN 1 CP437 80 25'
  sed -n '3,27p' "$screen" > "$dir/diagnostic.rows";;
 bochs)
  line "$dir/report" '[bochs] triple_fault=0'
  [ -f "$screen" ] && [ "$(wc -c < "$screen")" -eq 4002 ] || fail 'Bochs diagnostic screen missing or short'
  od -An -v -N4000 -tu1 "$screen" |
   awk '{for (i=1;i<=NF;i+=2) {printf "%c", $i; if (++col==80) {print ""; col=0}}}' > "$dir/diagnostic.rows";;
 esac
 # The wording is from the pinned TP 5.5 MINT02 E05 diagnostic and its IDE form from MINT06.
 sed 's/ *$//' "$dir/diagnostic.rows" > "$dir/diagnostic.txt"
 line "$dir/diagnostic.txt" 'Error 85: ";" expected'
 line "$dir/diagnostic.txt" 'program e05;'
 line "$dir/serial.txt" 'C:\TURBO>turbo hello.pas'
 media_readback "$emulator" "$dir/report"
 fsck.fat -n "$media_volume" > "$dir/fsck.c" || { cat "$dir/fsck.c"; fail 'written C: filesystem'; }
 mcopy -i "$media_volume" ::TURBO/TURBOOUT.COM "$dir/hello.com" || fail "$emulator Turbo output absent from written C:"
 test -s "$dir/hello.com" || fail 'empty Pascal executable'
 mdir -a -b -i "$media_volume" ::TURBO/ > "$dir/turbo-files"
 if grep -E '/(TURBOOUT\.S|DEMO\.(EXE|COM))$' "$dir/turbo-files"; then
  fail "$emulator left an unexpected build output"
 fi
 awk '$0 == "C:\\TURBO>turboout.com" {run=1; next}
      run && index($0, "C:\\TURBO>")==1 {exit}
      run {print}' "$dir/serial.txt" > "$dir/hello.actual"
 cmp -s "$out/hello.expected" "$dir/hello.actual" || fail "$emulator Hello stdout differs from FPC"
 echo "ok $emulator installed Turbo (alone): ordinary C: startup and independent readback"
done
sha256sum -c "$out/master.sha256"
