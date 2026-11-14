#!/bin/sh
# Real DOS reference STATE.OUT is SEVEN after full COPY and self refusal.
set -eu
root=$1 boot=$2 programs=$3 out=$4
mkdir -p "$out"
ref=$root/dos/command/tests/dos33/reference/concat
img=$out/data.img
gzip -dc "$ref/data.img.gz" > "$img"
mcopy -i "$img" "$ref/files/OLD.BIN" ::TARGET.BIN
cat > "$out/seven.asm" <<'ASM'
bits 32
org 0x40100
mov eax,0x4c07
int 0x21
ASM
nasm -f bin "$out/seven.asm" -o "$out/SEVEN.COM"
mcopy -i "$img" "$out/SEVEN.COM" ::SEVEN.COM
cat > "$out/autoexec" <<'BAT'
@ECHO OFF
SEVEN
COPY /B OLD.BIN+SOURCE.BIN TARGET.BIN /B
IF ERRORLEVEL 8 GOTO HIGH
IF ERRORLEVEL 7 GOTO SEVEN
ECHO LOW>STATE.OUT
GOTO SELF
:HIGH
ECHO HIGH>STATE.OUT
GOTO SELF
:SEVEN
ECHO SEVEN>STATE.OUT
:SELF
COPY SOURCE.BIN .\source.bin /B
IF ERRORLEVEL 8 GOTO SELFHI
IF ERRORLEVEL 7 GOTO SELFSEV
ECHO LOW>SELF.OUT
GOTO DONE
:SELFHI
ECHO HIGH>SELF.OUT
GOTO DONE
:SELFSEV
ECHO SEVEN>SELF.OUT
:DONE
ECHO copy-state-done
BAT
mcopy -i "$img" "$out/autoexec" ::AUTOEXEC.BAT
sh "$root/dos/tests/runtime/execute.sh" "$root" "$boot" "$out" '' copy-state-done
mtype -i "$img" ::STATE.OUT > "$out/state"
cmp "$out/state" "$ref/STATE.OUT"
mtype -i "$img" ::SELF.OUT > "$out/state"
cmp "$out/state" "$root/dos/command/tests/dos33/reference/self/STATE.OUT"
if mdir -i "$img" ::TARGET.BIN >/dev/null 2>&1; then echo 'RED COPY: failed target retained' >&2;exit 1;fi
mtype -i "$img" ::SOURCE.BIN > "$out/got";cmp "$out/got" "$ref/files/SOURCE.BIN"
echo 'GREEN COPY exit state: ERRORLEVEL 7 retained on disk full and self-copy refusal, source intact, target removed'
