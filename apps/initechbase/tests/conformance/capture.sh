#!/bin/sh
# Explicit mint operation: disposable real DOS 3.30 + real III+ on QEMU.
# Uses the existing independent golden capture driver, never InitechOS code.
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
repo=$(CDPATH= cd -- "$src/../../../.." && pwd)
out=${1:?absolute empty output directory}
case "$out" in /*) ;; *) echo 'output path must be absolute' >&2;exit 2;; esac
if [ -e "$out" ] && [ -n "$(ls -A "$out")" ]; then
 echo 'capture output must be empty; use a new directory' >&2;exit 2
fi
corpus=${DBASE3_DECOMP:-"$HOME/Projects/dbase3-decomp"}
media=${DOS33_BOOT:-"$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img"}
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror "$repo/goldens/dataloss/drive.c" -o "$out/drive"
sha256sum "$media" "$corpus"/mint/work/DBASE.EXE "$corpus"/mint/work/DBASE.OVL \
 "$corpus"/mint/work/DBASE.MSG "$corpus"/mint/work/DBASEINL.OVL > "$out/reference.sha256"
# Each case boots separately: a malformed file cannot hide later cases.
selected=0;failed=0
for case_dir in "$src"/cases/C*; do
 name=${case_dir##*/}
 case " ${CASES:-all} " in *" all "*|*" $name "*) ;; *) continue;; esac
 selected=$((selected+1))
 d=$out/$name; mkdir -p "$d/before" "$d/after"
 cp "$case_dir"/* "$d/before/"
 cp "$media" "$d/boot.img"
 printf 'FILES=40\r\nBUFFERS=20\r\n' > "$d/CONFIG.SYS"
 printf '@ECHO OFF\r\nC:\r\nDBASE MINT\r\nECHO CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
 mcopy -o -i "$d/boot.img" "$d/CONFIG.SYS" "$d/AUTOEXEC.BAT" ::
 truncate -s 33554432 "$d/data.img"
 printf 'label: dos\nlabel-id: 0x19920302\nunit: sectors\n\n63,65457,4,*\n' | sfdisk "$d/data.img" > "$d/partition.log"
 mformat -i "$d/data.img@@32256" -T 65457 -h 16 -s 63 -c 4 -r 32 -H 63 -N 19920302 ::
 mcopy -i "$d/data.img@@32256" "$corpus"/mint/work/DBASE.EXE "$corpus"/mint/work/DBASE.OVL \
  "$corpus"/mint/work/DBASE.MSG "$corpus"/mint/work/DBASEINL.OVL "$d"/before/* ::
 mdir -i "$d/data.img@@32256" :: > "$d/before.mdir"
 printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
 rc=0
 timeout -k 2 30 "$out/drive" "$d" "$d/boot.img" - "$d/data.img" "$d/steps" > "$d/driver.txt" 2>&1 || rc=$?
 printf '%s\n' "$rc" > "$d/exit-status"
 mdir -i "$d/data.img@@32256" :: > "$d/after.mdir"
 for file in BASE.DBF BASE.DBT OLD.NDX SAVED.DBF RESULT.OUT; do
  if mdir -i "$d/data.img@@32256" "::$file" > /dev/null 2>&1; then
   mcopy -o -i "$d/data.img@@32256" "::$file" "$d/after/$file"
  fi
 done
 printf '%s\t%s\n' "$name" "$rc"
 test "$rc" -eq 0 || failed=1
done
test "$selected" -gt 0 || { echo 'FAIL: no selected capture cases' >&2;exit 2; }
exit "$failed"
