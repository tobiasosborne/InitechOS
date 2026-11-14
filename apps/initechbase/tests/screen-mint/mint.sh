#!/bin/sh
# Real dBASE III PLUS 1.1 screen mint on QEMU with genuine DOS 3.30 boot
# media (the tests/conformance route) plus the SCREEN.COM key/dump TSR
# claimed from attic. No InitechOS code runs here. Usage:
#   sh mint.sh CASE_DIR DEST_DIR SCREEN_TOOL SCREEN.COM
# CASE_DIR holds SETUP/ (CONFIG.DB, SETUP.PRG, data files), KEYS, DUMPS and
# optionally EXPORTS (one DOS file name per line, read back after the run).
set -eu
export LC_ALL=C TZ=UTC
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
repo=$(CDPATH= cd -- "$src/../../../.." && pwd)
case_dir=$(CDPATH= cd -- "${1:?case directory}" && pwd)
dest=${2:?output directory}
tool=${3:?screen_tool path}
screen_com=${4:?SCREEN.COM path}
mkdir -p "$dest"
dest=$(CDPATH= cd -- "$dest" && pwd)
corpus=${DBASE3_DECOMP:-"$HOME/Projects/dbase3-decomp"}
media=${DOS33_BOOT:-"$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img"}
"$tool" check-setup "$case_dir/SETUP"
work=$(mktemp -d "${TMPDIR:-/tmp}/forms-mint.XXXXXX")
trap 'if [ "${SCREEN_MINT_KEEP:-0}" = 1 ]; then echo "work: $work"; else rm -rf "$work"; fi' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror "$repo/goldens/dataloss/drive.c" -o "$work/drive"
mkdir "$work/output" "$work/exports"
cp "$media" "$work/boot.img"
printf 'FILES=40\r\nBUFFERS=20\r\n' > "$work/CONFIG.SYS"
printf '@ECHO OFF\r\nC:\r\nDBASE MINT\r\nSCREEN /S\r\nECHO CAPTURE-DONE\r\n' > "$work/AUTOEXEC.BAT"
mcopy -o -i "$work/boot.img" "$work/CONFIG.SYS" "$work/AUTOEXEC.BAT" ::
truncate -s 33554432 "$work/data.img"
printf 'label: dos\nlabel-id: 0x19920302\nunit: sectors\n\n63,65457,4,*\n' | sfdisk "$work/data.img" > "$work/partition.log"
mformat -i "$work/data.img@@32256" -T 65457 -h 16 -s 63 -c 4 -r 32 -H 63 -N 19920302 ::
"$tool" compile "$case_dir/KEYS" "$case_dir/DUMPS" "$work/EVENTS.BIN"
printf 'DO SETUP\r\nRUN SCREEN\r\nRETURN\r\n' > "$work/MINT.PRG"
cp "$screen_com" "$work/SCREEN.COM"
mcopy -i "$work/data.img@@32256" "$corpus"/mint/work/DBASE.EXE "$corpus"/mint/work/DBASE.OVL \
 "$corpus"/mint/work/DBASE.MSG "$corpus"/mint/work/DBASEINL.OVL \
 "$work/SCREEN.COM" "$work/EVENTS.BIN" "$work/MINT.PRG" ::
mcopy -s -i "$work/data.img@@32256" "$case_dir"/SETUP/* ::
# drive.c's wait polls a bounded number of times and fails the run when it
# expires. The TSR types one key per BIOS tick (18.2/s), so sleep for the
# script's tick budget (plus boot/processing margin) before waiting.
ticks=$(awk '/^WAIT /{t+=$2} /^TEXT /{t+=length($0)-5} /^KEY /{t+=1} END{print t+0}' "$case_dir/KEYS")
sleep_s=$(( ticks * 3 / 36 + 25 ))
printf 'sleep %s\nwait CAPTURE-DONE\nshot result\nend\n' "$sleep_s" > "$work/steps"
rc=0
timeout -k 2 "${SCREEN_MINT_TIMEOUT:-120}" "$work/drive" "$work" "$work/boot.img" - "$work/data.img" "$work/steps" > "$dest/mint.log" 2>&1 || rc=$?
printf '%s\n' "$rc" > "$dest/exit-status"
if [ "$rc" != 0 ]; then
 printf 'RED: forms mint driver exit=%s; see %s/mint.log\n' "$rc" "$dest" >&2
 exit 1
fi
img="$work/data.img@@32256"
mdir -i "$img" "::CAPTURE.OK" > /dev/null 2>&1 || { echo 'RED: no fresh CAPTURE.OK' >&2; exit 1; }
mcopy -o -i "$img" "::CAPTURE.OK" "$work/CAPTURE.OK"
"$tool" check-save "$case_dir/DUMPS" "$work/CAPTURE.OK"
i=0
while IFS= read -r name || [ -n "$name" ]; do
 case $name in ''|'#'*) continue ;; esac
 mcopy -o -i "$img" "::DUMP$i.BIN" "$work/DUMP$i.BIN"
 "$tool" encode "$work/DUMP$i.BIN" "$work/output/$name.screen"
 i=$((i + 1))
done < "$case_dir/DUMPS"
if [ -f "$case_dir/EXPORTS" ]; then
 while IFS= read -r f || [ -n "$f" ]; do
  case $f in ''|'#'*) continue ;; esac
  if mdir -i "$img" "::$f" > /dev/null 2>&1; then
   mcopy -o -i "$img" "::$f" "$work/exports/$f"
  else
   : > "$work/exports/$f.MISSING"
  fi
 done < "$case_dir/EXPORTS"
fi
cp -f "$work/output/"* "$dest/"
test -d "$work/exports" && cp -rf "$work/exports" "$dest/exports"
cp -f "$work/result.txt" "$work/result.vga" "$dest/" 2>/dev/null || :
(cd "$dest" && find . -name '*.screen' -o -path './exports/*' | LC_ALL=C sort | xargs sha256sum) > "$dest/SHA256SUMS"
(
 cd "$case_dir"
 find SETUP -type f -print | LC_ALL=C sort | xargs sha256sum
 sha256sum KEYS DUMPS
 test ! -f EXPORTS || sha256sum EXPORTS
) > "$dest/INPUTS.sha256"
(
 for f in DBASE.EXE DBASEINL.OVL DBASE.MSG DBASE.OVL; do
  sha256sum < "$corpus/mint/work/$f" | sed "s/  -\$/  $f/"
 done
 sha256sum < "$media" | sed 's/  -$/  DOS33-BOOT.img/'
 sha256sum < "$screen_com" | sed 's/  -$/  SCREEN.COM/'
 (cd "$work" && sha256sum EVENTS.BIN)
 sha256sum < "$(command -v qemu-system-i386)" | sed 's/  -$/  qemu-system-i386/'
 sha256sum < "$tool" | sed 's/  -$/  screen_tool/'
 sha256sum < "$repo/goldens/dataloss/drive.c" | sed 's/  -$/  drive.c/'
 sha256sum < "$src/mint.sh" | sed 's/  -$/  mint.sh/'
 sha256sum < "$src/screen_tsr.asm" | sed 's/  -$/  screen_tsr.asm/'
) > "$dest/ORACLE.sha256"
printf 'GREEN: real III+ forms mint %s (%s dump points)\n' "$(basename "$case_dir")" "$i"
