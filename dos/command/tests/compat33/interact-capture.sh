#!/bin/sh
# Genuine DOS 3.30 INTERACTIVE expansion capture for initech-xfg8.
# Types commands at the real DOS prompt (drive.c sendkey) and reads the
# resulting files back with mtools. Covers what batch files cannot:
# whether %VAR%, %% and FOR variables expand interactively, plus the
# unmatched-% batch variants the BATCH case did not isolate.
# Run from repository root. Output: build/dos-compat33/interact-reference.
# Minting into expected/INTERACT is a separate deliberate step.
set -eu
root=$(pwd)
d=$root/build/dos-compat33/interact-reference
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
mkdir -p "$d"
test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
cp "$media" "$d/boot.img"
truncate -s 1474560 "$d/data.img"
mformat -i "$d/data.img" -f 1440 -N 19920302 ::
printf '@echo off\r\necho A=[hello%%]\r\necho B=[%%XYZ]\r\necho C=[50%% done]\r\necho D=[trailing%%]\r\necho E=[%%ZZ%%]\r\necho IBAT-DONE\r\n' > "$d/IBAT.BAT"
mcopy -o -i "$d/data.img" "$d/IBAT.BAT" ::IBAT.BAT
printf '@echo off\r\nver > VERSION.TXT\r\npath A:\\\r\nB:\r\necho INTERACT-READY\r\n' > "$d/AUTOEXEC.BAT"
mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
# Emit steps: drive.c type() covers alphanumerics and a few symbols;
# %, ", [, ], > go through raw key steps.
awk '
function keyfor(c) {
  if (c=="%") return "shift-5"; if (c=="\"") return "shift-apostrophe"
  if (c=="[") return "bracket_left"; if (c=="]") return "bracket_right"
  if (c==">") return "shift-dot"; if (c=="<") return "shift-comma"
  return ""
}
function emit(line,   i,c,k,run) {
  run=""
  for (i=1;i<=length(line);i++) {
    c=substr(line,i,1); k=keyfor(c)
    if (k!="") { if (run!="") { print "type " run; run="" } print "key " k }
    else run=run c
  }
  if (run!="") print "type " run
  print "key ret"; print "sleep 1"
}
BEGIN {
  print "wait INTERACT-READY"; print "sleep 1"
  emit("SET ZZ=hello")
  emit("ECHO [%ZZ%] >INT.TXT")
  emit("ECHO [%ABSENT%] >>INT.TXT")
  emit("ECHO 100%% >>INT.TXT")
  emit("FOR %F IN (ONE TWO) DO ECHO F=[%F] >>INT.TXT")
  emit("ECHO INT-DONE >>INT.TXT")
  emit("IBAT >INTB.TXT")
  print "sleep 2"; print "shot result"; print "end"
}' > "$d/steps"
"$d/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps"
mcopy -o -i "$d/boot.img" ::VERSION.TXT "$d/VERSION.TXT"
for f in INT.TXT INTB.TXT; do
  mcopy -o -i "$d/data.img" "::$f" "$d/$f" || echo "MISSING $f"
done
mdir -a -i "$d/data.img" :: > "$d/after.mdir"
echo "interact reference captured in $d"
