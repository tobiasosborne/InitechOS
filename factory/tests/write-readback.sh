#!/bin/sh
# Acceptance regression for initech-oecdm: correct disk bytes alone do not
# establish that the guest read them. Run the real factory path on a disposable
# WRITE probe that exits after its successful write/close, before any READ.
set -eu
cd "$(dirname "$0")/../.."
out=${WRITE_READBACK_OUT:-build/smoke-write-readback}
mkdir -p "$out/programs"
cp build/dos/test-programs/*.COM "$out/programs/"
awk '
 /; ---- Read it back THROUGH the OS/ {
   print "    mov eax, 0x4c00"; print "    int 0x21"; found++
 }
 { print }
 END { if (found != 1) exit 1 }
' kernel/tests/programs/write_program.asm > "$out/write-early-exit.asm"
nasm -f bin "$out/write-early-exit.asm" -o "$out/programs/WRITE.COM"
rc=0
PROGRAM_SMOKE_OUT="$out/negative" PROGRAM_SMOKE_PROGRAMS="$out/programs" \
 sh factory/smoke-programs.sh > "$out/negative.log" 2>&1 || rc=$?
# Independently check the control even when the harness rejects the transcript.
mcopy -o -i "$out/negative/qemu-data.img" ::OUT.TXT "$out/persisted.txt"
printf 'hello\r\n' > "$out/expected.txt"
cmp "$out/expected.txt" "$out/persisted.txt"
if grep -E 'WRITE-(.*-FAIL|READBACK-OK)' "$out/negative/qemu.txt"; then
 echo 'FAIL: early-exit fixture printed a WRITE verdict' >&2; exit 1
fi
if [ "$rc" -eq 0 ]; then
 echo 'FAIL: factory accepted correct disk bytes without guest readback' >&2; exit 1
fi
grep -Fx 'FAIL program smoke: qemu missing [WRITE-READBACK-OK]' "$out/negative.log"
echo 'WRITE readback negative control rejected; literal persisted bytes remain correct'
