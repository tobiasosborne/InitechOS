#!/bin/sh
# Oracle first: mtools mints each image, fsck.fat judges the identical bytes.
# Ref: PRD 6.1/8; MS-DOS 3.3 p43/319/334/344; IBM pp7-48..51/A-46.
set -eu
kind=$1; build=$2; candidate=$3; fixture=$4; boot=${5:-}
export SOURCE_DATE_EPOCH=315532800 TZ=UTC INITECH_QMP_STDIO=1
mkdir -p "$build/chkdsk"
work="$build/chkdsk/$kind"
mkdir -p "$work"
for tool in mformat mcopy mmd mattrib mdir fsck.fat; do command -v "$tool" >/dev/null; done
printf 'Y\r\n' > "$work/yes.txt"
printf 'N\r\n' > "$work/no.txt"
printf 'fixture\r\n' > "$work/small.txt"
dd if=/dev/zero of="$work/frag.txt" bs=512 count=5 status=none
mint() {
 image=$1; scenario=$2; format=$3; tail=$4
 if [ "$format" = 12 ]; then
  dd if=/dev/zero of="$image" bs=512 count=2880 status=none
  mformat -i "$image" -f 1440 -N 0x43484b44 -v CHKTEST ::
 else
  dd if=/dev/zero of="$image" bs=1024 count=16384 status=none
  mkfs.fat --invariant -F 16 -s 2 -n CHKTEST "$image" >/dev/null
 fi
 mcopy -i "$image" "$work/small.txt" ::ALPHA.TXT
 mcopy -i "$image" "$work/small.txt" ::BRAVO.TXT
 mcopy -i "$image" "$work/small.txt" ::HIDDEN.TXT
 mattrib -i "$image" +h ::HIDDEN.TXT
 mmd -i "$image" ::SUB
 mmd -i "$image" ::SUB/DEEP
 mcopy -i "$image" "$work/small.txt" ::SUB/DEEP/NOTE.TXT
 mcopy -i "$image" "$work/frag.txt" ::FRAG.TXT
 printf '' | mcopy -i "$image" - ::EMPTY.TXT
 mcopy -i "$image" "$work/yes.txt" ::YES.TXT
 mcopy -i "$image" "$work/no.txt" ::NO.TXT
 if [ "$kind" = emu ]; then
  mcopy -i "$image" "$candidate" ::CHKDSK.COM
  printf '@ECHO OFF\r\nECHO CHKDSK-BEGIN\r\nCHKDSK %s\r\nECHO CHKDSK-END\r\nDIR\r\n' "$tail" > "$work/autoexec.bat"
  mcopy -i "$image" "$work/autoexec.bat" ::AUTOEXEC.BAT
 fi
 "$fixture" corrupt "$image" "$scenario"
}
run() {
 image=$1; tail=$2; log=$3
 if [ "$kind" = host ]; then
  "$candidate" "$image" "$tail" < "$work/yes.txt" > "$log"
 else
  name=$(basename "$log" .log)
  "$build/qemu_harness" --disk "$boot" --disk2 "$image" --name "$name" --out "$work" \
   --timeout-ms 30000 --keys 'e,x,i,t,ret' --keys-after SHELL-READY \
   --quit-after SHELL-DONE > "$work/$name.stdout" 2> "$work/$name.report" || true
  test -s "$work/$name.serial"
  if grep -q 'triple_fault=1' "$work/$name.report"; then cat "$work/$name.report"; exit 2; fi
  grep -q '^SHELL-EXIT' "$work/$name.serial" || { tail -25 "$work/$name.serial"; exit 2; }
  tr -d '\r' < "$work/$name.serial" | awk '/^CHKDSK-BEGIN$/{p=1;next}/^CHKDSK-END$/{p=0}p' > "$log"
 fi
}
formats=12
[ "$kind" != host ] || formats='12 16'
for format in $formats; do
 for scenario in clean fragment cross invalid badstart lost; do
  image="$work/$format-$scenario.img"; log="$work/$format-$scenario.log"
  tail='A:FRAG.TXT /V < YES.TXT'
  mint "$image" "$scenario" "$format" "$tail"
  cp -f "$image" "$image.before"
  rc=0; fsck.fat -n "$image" > "$image.fsck" 2>&1 || rc=$?
  case "$scenario" in
   clean|fragment) test "$rc" = 0 ;;
   *) test "$rc" != 0 ;;
  esac
  case "$scenario" in
   cross) grep -q 'share clusters' "$image.fsck" ;;
   invalid) grep -q 'out of range (1)' "$image.fsck" ;;
   badstart) grep -q 'Start cluster beyond limit' "$image.fsck" ;;
   lost) grep -q 'Reclaimed 2 unused clusters' "$image.fsck" ;;
  esac
  if [ "$kind" = host ]; then tail='A:FRAG.TXT /V'; fi
  run "$image" "$tail" "$log"
  cmp -s "$image.before" "$image" || { echo 'RED CHKDSK-NOFIX'; exit 1; }
  "$fixture" check "$image" "$scenario" "$log" "$kind"
  # Independent mtools free-space number must appear in the manual report.
  free=$(mdir -i "$image" :: | sed -n '/bytes free/s/[^0-9]//gp')
  tr -d '\r' < "$log" | grep -q " *$free bytes available on disk" || { echo 'RED CHKDSK-REPORT-FREE'; exit 1; }
  echo "GREEN CHKDSK FAT$format $scenario: fsck.fat verdict, report, read-only bytes, mdir free space"
 done
 # /F truncates invalid chains/starts, but preserves cross-links for manual
 # recovery (IBM A-46). Same corruption, same candidate, same fsck reference.
 for scenario in invalid badstart cross; do
  image="$work/$format-fix-$scenario.img"; log="$work/$format-fix-$scenario.log"
  mint "$image" "$scenario" "$format" 'A: /F'
  cp -f "$image" "$image.ref"
  if [ "$scenario" != cross ]; then
   fsck.fat -a "$image.ref" > "$image.ref.log" 2>&1 || test "$?" = 1
  fi
  run "$image" 'A: /F' "$log"
  "$fixture" check "$image" "$scenario" "$log" "$kind"
  "$fixture" compare "$image" "$image.ref"
  if [ "$scenario" = cross ]; then
   if fsck.fat -n "$image" > "$image.fsck" 2>&1; then echo 'RED CHKDSK-CROSS'; exit 1; fi
  else fsck.fat -n "$image" > "$image.fsck" 2>&1; fi
  echo "GREEN CHKDSK FAT$format /F $scenario: manual repair policy and reference image diff"
 done
 for answer in recover discard; do
  image="$work/$format-$answer.img"; log="$work/$format-$answer.log"
  input=YES.TXT; [ "$answer" != discard ] || input=NO.TXT
  tail="A: /F < $input"
  mint "$image" lost "$format" "$tail"
  cp -f "$image" "$image.ref"
  if [ "$answer" = recover ]; then fsck.fat -a -f "$image.ref" > "$image.ref.log" 2>&1 || test "$?" = 1
  else printf '1\n' | fsck.fat -r "$image.ref" > "$image.ref.log" 2>&1 || test "$?" = 1; fi
  if [ "$kind" = host ]; then
   inputfile="$work/yes.txt"; [ "$answer" != discard ] || inputfile="$work/no.txt"
   "$candidate" "$image" 'A: /F' < "$inputfile" > "$log"
  else run "$image" "$tail" "$log"; fi
  "$fixture" check "$image" lost "$log" "$kind-$answer"
  "$fixture" compare "$image" "$image.ref"
  fsck.fat -n "$image" > "$image.fsck" 2>&1
  if [ "$kind" = emu ]; then
   free=$(mdir -i "$image" :: | sed -n '/bytes free/s/[^0-9]//gp')
   tr -d '\r' < "$work/$(basename "$log" .log).serial" | grep -q " *$free bytes free" || { echo 'RED CHKDSK-CACHE'; exit 1; }
  fi
  echo "GREEN CHKDSK FAT$format /F $answer: full image differential vs fsck.fat, clean rescan"
 done
done
echo "VERDICT: PASS CHKDSK $kind"
