#!/bin/sh
# Same-image mtools/dosfstools differential, m0dc injected-console pattern.
# Ref: MS-DOS 3.3 User's Guide pp.162-193; model.c maps cases to pages.
set -eu
export INITECH_QMP_STDIO=1
mode=$1
build=$2
program=$(realpath "$3")
model=$(realpath "$4")
boot=${5:-}
root=$build/edlin-$mode-$(basename "$program")
mkdir -p "$root"
fail() { echo "RED EDLIN-$1 case=$case"; exit 1; }
cases='new edit replace repeat commands search confirm transfer keys binary eof staged insert relative robackup path move bakname'
case ${EDLIN_ONLY:-} in
 NUMBER) cases=replace;; BACKUP) cases=repeat;; FIRST) cases=repeat;; QUIT) cases=search;;
esac
for case in $cases; do
 out=$root/$case
 rm -rf "$out"
 mkdir -p "$out/model" "$out/actual"
 "$model" "$case" "$out/model"
 for f in IN.TXT IN.BAK irsharpe.mem; do
  if [ -f "$out/model/$f" ]; then cp -f "$out/model/$f" "$out/actual/$f"; fi
 done
 tail=$(cat "$out/model/tail")
 leaf=IN.TXT
 bak=IN.BAK
 if [ "$case" = path ]; then
  mkdir -p "$out/actual/SUB"
  mv -f "$out/actual/IN.TXT" "$out/actual/SUB/IN.TXT"
  mv -f "$out/actual/IN.BAK" "$out/actual/SUB/IN.BAK"
  leaf=SUB/IN.TXT
  bak=SUB/IN.BAK
 fi
 if [ "$case" = robackup ]; then chmod 400 "$out/actual/IN.BAK"; fi
 if [ "$mode" = host ]; then
  rc=0
  (cd "$out/actual"; "$program" "$tail" < ../model/keys.bin > ../got.txt) || rc=$?
  if [ "$case" = bakname ]; then [ "$rc" = 1 ] || fail rejection
  else [ "$rc" = 0 ] || fail execution; fi
 else
  img=$out/data.img
  dd if=/dev/zero of="$img" bs=512 count=2880 status=none
  mformat -i "$img" -f 1440 -N 0x45444c49 ::
  for f in IN.TXT IN.BAK irsharpe.mem; do
   if [ -f "$out/model/$f" ]; then mcopy -i "$img" "$out/model/$f" "::$f"; fi
  done
  if [ "$case" = path ]; then mmd -i "$img" ::SUB; mmove -i "$img" ::IN.TXT ::SUB/IN.TXT; mmove -i "$img" ::IN.BAK ::SUB/IN.BAK; fi
  if [ "$case" = robackup ]; then mattrib -i "$img" +r ::IN.BAK; fi
  mcopy -i "$img" "$program" ::EDLIN.COM
  printf '@echo off\r\nEDLIN %s > TRACE.TXT\r\necho EDLIN-DONE\r\n' "$tail" > "$out/autoexec.bat"
  mcopy -i "$img" "$out/autoexec.bat" ::AUTOEXEC.BAT
  cp -f "$img" "$out/reference.img"
  name=edlin_$(basename "$program")_$case
  "$build/qemu_harness" --disk "$boot" --disk2 "$img" --name "$name" --out "$build" \
   --keys "$(cat "$out/model/keys.qmp")" --keys-after SHELL-READY \
   --quit-after SHELL-EXIT --timeout-ms 45000 > "$out/stdout" 2> "$out/report" || true
  grep -q 'OK=1' "$out/report" || { cat "$out/report"; fail boot; }
  if grep -q 'unknown token\|truncating' "$out/report"; then cat "$out/report"; fail injection; fi
  tr -d '\r' < "$build/$name.serial" | grep -qx EDLIN-DONE || fail completion
  grep -q triple_fault=1 "$out/report" && fail fault
  mcopy -o -i "$img" ::TRACE.TXT "$out/got.txt" 2>/dev/null || fail transcript
  mcopy -o -i "$img" "::$leaf" "$out/actual/$leaf" 2>/dev/null || true
  chmod -f u+w "$out/actual/$bak" 2>/dev/null || true
  rm -f "$out/actual/$bak"
  mcopy -o -i "$img" "::$bak" "$out/actual/$bak" 2>/dev/null || true
 fi
 saved=0
 [ ! -f "$out/model/OUTPUT.TXT" ] || saved=1
 if [ "$saved" = 1 ]; then
  cmp -s "$out/model/OUTPUT.TXT" "$out/actual/$leaf" || fail output
  if [ -f "$out/model/MANUAL.TXT" ]; then cmp -s "$out/model/MANUAL.TXT" "$out/actual/$leaf" || fail manual-file; fi
  if [ "$case" = robackup ]; then
   cmp -s "$out/model/IN.BAK" "$out/actual/$bak" || fail backup
  elif [ -f "$out/model/IN.TXT" ]; then
   cmp -s "$out/model/IN.TXT" "$out/actual/$bak" || fail backup
  else
   # E on a new file does not create a BAK; a pre-existing BAK is removed (p.186).
   [ ! -f "$out/actual/$bak" ] || fail backup
  fi
 else
  cmp -s "$out/model/IN.TXT" "$out/actual/$leaf" || fail quit-output
  cmp -s "$out/model/IN.BAK" "$out/actual/$bak" || fail quit-backup
 fi
 cmp -s "$out/model/expected.txt" "$out/got.txt" || { diff -u "$out/model/expected.txt" "$out/got.txt" | head -70; fail transcript; }
 if [ "$mode" = emu ]; then
  ref=$out/reference.img
  if [ "$saved" = 1 ]; then
   if [ "$case" = robackup ]; then mdel -i "$ref" "::$leaf";
   else
    mdel -i "$ref" "::$bak"
    if [ -f "$out/model/IN.TXT" ]; then mren -i "$ref" "::$leaf" "::$bak"; fi
   fi
   mcopy -i "$ref" "$out/model/OUTPUT.TXT" "::$leaf"
  fi
  mcopy -i "$ref" "$out/model/expected.txt" ::TRACE.TXT
  # Compare independently extracted semantic FAT contents, not timestamps or allocation order.
  for f in "$leaf" "$bak" TRACE.TXT; do
   base=$(basename "$f")
   mcopy -o -i "$ref" "::$f" "$out/ref-$base" 2>/dev/null || true
   mcopy -o -i "$img" "::$f" "$out/got-$base" 2>/dev/null || true
   if [ -f "$out/ref-$base" ]; then cmp -s "$out/ref-$base" "$out/got-$base" || fail fat-bytes
   else [ ! -f "$out/got-$base" ] || fail fat-name; fi
  done
  mdir -b -i "$ref" :: | sort > "$out/ref.names"
  mdir -b -i "$img" :: | sort > "$out/got.names"
  cmp -s "$out/ref.names" "$out/got.names" || fail fat-names
  fsck.fat -n "$img" > "$out/fsck" 2>&1 || { cat "$out/fsck"; fail fat-structure; }
 fi
 echo "GREEN test-edlin-$mode $case (transcript, output, backup)"
done
echo "GREEN test-edlin-$mode (all command scripts; independent manual model)"
