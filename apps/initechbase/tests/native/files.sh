#!/bin/sh
# Real-capture transcript/foreign-file oracles; no emulator in tier 0.
set -eu
mode=$1 root=$2 exe=$3 ovl=$4 out=$5
src=$PWD
mkdir -p "$out"
export MTOOLS_SKIP_CHECK=1 INITECH_QMP_STDIO=1 INITECH_TEST_RAM_MB=8
fail() { echo "ERROR native $mode: $*" >&2;exit 2; }
red() { echo "RED native $mode: $*" >&2;exit 1; }
copy() { mcopy -o -i "$disk" "$1" "::APPS/$2"; }
disk=$out/test.img
dd if=/dev/zero of="$disk" bs=512 count=2880 2>/dev/null
mformat -i "$disk" -f 1440 -N 0x49424153 -v TEST ::
mmd -i "$disk" ::APPS
copy "$exe" INITBASE.EXE;copy "$ovl" INITBASE.OVL
mcopy -i "$disk" "$root/build/dos/COMMAND.COM" ::
printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$out/CONFIG.SYS"
mcopy -i "$disk" "$out/CONFIG.SYS" ::
case $mode in
 io)
  (cd tests/conformance && awk '$2=="cases/C21/BASE.DBF"' INPUTS.sha256 | sha256sum --quiet -c -) || fail 'input capture checksum'
  (cd tests/conformance/reference && awk '$2=="./C21/SAVED.DBF"' SHA256SUMS | sha256sum --quiet -c -) || fail 'result capture checksum'
  copy tests/conformance/cases/C21/BASE.DBF CHECK.DBF
  cat > "$out/PROBE.PRG" <<'PRG'
SET TALK OFF
SET SAFETY OFF
SET STATUS OFF
ON ERROR ? "NATIVE-ERROR",ERROR()
? "[NATIVE-BEGIN]"
USE CHECK
GO 2
REPLACE VALUE WITH 9
USE
? "[NATIVE-END]"
RETURN
PRG
  copy "$out/PROBE.PRG" PROBE.PRG
  program=probe ;;
 stream|stack)
  (cd tests/xbase_prog_diff/do-file-mint/depth-with && sha256sum --quiet -c sha256.txt) || fail 'DO capture checksums'
  for file in tests/xbase_prog_diff/do-file-mint/depth-with/*.PRG; do copy "$file" "$(basename "$file")";done
  program=mint ;;
 *) fail 'unknown oracle';;
esac
printf '\n\ncd apps\ninitbase\ndo %s\n' "$program" > "$out/session"
"$root/build/factory/keys" < "$out/session" > "$out/keys"
launch=$(sed 's/\(^\|,\)/\1k/g' "$out/keys")
printf 'echo nativefilecomplete\n' | "$root/build/factory/keys" | sed 's/\(^\|,\)/\1k/g' > "$out/echo-keys"
if [ "$mode" = io ]; then suffix='W[NATIVE-END],w200,kq,ku,ki,kt,kret';else suffix='W[DO-END]';fi
spec="$launch,$suffix,w1000,$(cat "$out/echo-keys")"
# Require the executed ECHO line, not its text in the shell's input echo.
done_marker=$(printf '\r\nnativefilecomplete\r\n')
rc=0
"$root/build/factory/qemu_harness" --disk "$root/build/image/boot.img" --disk2 "$disk" \
  --keys-after SHELL-READY --mouse "$spec" --expect "$done_marker" --quit-after "$done_marker" \
  --name run --out "$out" --timeout-ms 20000 > "$out/stdout" 2> "$out/report" || rc=$?
stack_fault=0
if [ "$mode" = stack ] && grep -F 'PANIC vec=06 err=00000000' "$out/run.serial" >/dev/null && grep -Fx HALTED "$out/run.serial" >/dev/null; then
 # A normal return is part of executing QUIT. Still require the complete,
 # correct captured DO transcript before grading this specific exit failure.
 stack_fault=1
elif grep -E 'PANIC|HALTED|STACK OVERFLOW' "$out/run.serial" >/dev/null; then fail 'unexpected guest fault';fi
if [ "$stack_fault" -eq 0 ] && { [ "$rc" -ne 0 ] || ! grep -Fx '[harness] OK=1' "$out/report" >/dev/null; }; then cat "$out/report";fail 'incomplete guest';fi
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report" >/dev/null || fail 'emulator fault'
if [ "$mode" = io ]; then
 grep -F '[NATIVE-END]' "$out/run.serial" >/dev/null || fail 'program did not complete'
 mcopy -o -i "$disk" ::APPS/CHECK.DBF "$out/CHECK.DBF" || red 'completed guest lost table'
 python3 tests/conformance/read_dbf.py tests/conformance/reference/C21/SAVED.DBF > "$out/expected"
 python3 tests/conformance/read_dbf.py "$out/CHECK.DBF" > "$out/actual" 2> "$out/read-error" || red 'completed guest saved an unreadable DBF'
 cmp -s "$out/expected" "$out/actual" || red 'completed guest failed to persist physical row B/9'
else
 # Preserve every internal CR/LF; trim only bytes outside the captured markers.
 for input in expected actual; do
  file=$out/run.serial
  if [ "$input" = expected ]; then file=tests/xbase_prog_diff/do-file-mint/depth-with/GOLD.OUT;fi
  awk 'index($0,"[DO-BEGIN]"){live=1;$0=substr($0,index($0,"[DO-BEGIN]"))}
       live {p=index($0,"[DO-END]");if(p){print substr($0,1,p+7);done=1;exit}print}
       END {if(!done)exit 2}' "$file" > "$out/$input" || fail 'missing completed DO transcript'
 done
 if [ "$stack_fault" -eq 1 ]; then
  cmp -s "$out/expected" "$out/actual" || fail 'exception before correct captured program completion'
  grep -F nativefilecomplete "$out/run.serial" >/dev/null && fail 'inconsistent DOS return and exception'
  red 'captured DO program completed, but QUIT raised UD6 and halted instead of returning to DOS'
 fi
 cmp -s "$out/expected" "$out/actual" || red 'native stream differs from captured complete DO transcript'
fi
echo "GREEN native $mode: completed guest matches independent capture"
