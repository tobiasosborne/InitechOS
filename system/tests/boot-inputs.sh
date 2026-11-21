#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
out=${1:-../build/system/installed}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM

check() {
 cmp -s "$work/expected" "$1" || {
  echo "FAIL boot inputs: $2 differs from the required CRLF bytes" >&2
  exit 1
 }
}

# C: activation plan step 4 and the accepted GM8 delivery design, first use.
# COMMAND must reach the DOS prompt before the user types FLAIR.
printf '%s\r\n' 'FILES=20' 'SHELL=COMMAND.COM /P' 'DEVICE=ANSI.SYS' > "$work/expected"
check "$out/c-boot/CONFIG.SYS" 'C: CONFIG.SYS'
check "$out/CONFIG.SYS" 'A: CONFIG.SYS'

printf '%s\r\n' '@ECHO OFF' 'PATH C:\;C:\APPS' 'SET COMSPEC=C:\COMMAND.COM' 'PROMPT $P$G' > "$work/expected"
check "$out/c-boot/AUTOEXEC.BAT" 'C: AUTOEXEC.BAT'

# The current A: working volume retains the exact startup files it ships.
printf '%s\r\n' '@ECHO OFF' 'PATH A:\;A:\APPS' 'FLAIR' > "$work/expected"
check "$out/AUTOEXEC.BAT" 'A: AUTOEXEC.BAT'
echo 'system: A: and staged C: boot input bytes green'
