#!/bin/sh
# Tier 2 target semantic guards. Ordinary compilation failures are errors.
set -eu
top=$1 out=$2 make_cmd=$3
shift 3
[ "$#" -gt 0 ] || { echo 'FAIL: no target mutations selected'; exit 1; }
src=$PWD
mkdir -p "$out"
run=$(mktemp -d "$out/run.XXXXXX")
ulimit -c 0
copy_source() {
 mkdir -p "$1"
 (cd "$src" && tar --exclude=./build -cf - .) | (cd "$1" && tar -xf -)
}
copy_source "$run/baseline/component"
if ! "$make_cmd" --no-print-directory -j2 -C "$run/baseline/component" TOP="$top" OUT="$run/baseline/build" package-check > "$run/baseline.log" 2>&1; then
 echo "ERROR target baseline (log: $run/baseline.log)"; exit 1
fi
find "$run/baseline/build/target" -name '*.o' -type f | sed "s@$run/baseline/build/@@" | sort > "$run/objects"
[ -s "$run/objects" ] || { echo 'ERROR baseline has no target objects'; exit 1; }
n=0 killed=0 errors=0
for patch_file do
 n=$((n+1)); name=$(basename "$patch_file" .patch)
 mode=$(sed -n 's/^# target: //p' "$patch_file")
 case $mode in
 static-cross) diagnostic='prohibited cross reference from .ovl_report to `ndx_rebuild' ;;
 static-resref) diagnostic='prohibited cross reference from .text to `.text.0ovl_report' ;;
 static-reenter) diagnostic='no-returning-into-an-evicted-overlay rule violated: ibase_ovl_call' ;;
 native-forms|native-enter|native-keys|native-color|native-color-state|native-controls)
  oracle=${mode#native-}
  if [ ! -f "$run/baseline/$mode.ok" ]; then
   if ! sh tests/native/check.sh "$oracle" "$top" "$run/baseline/build/INITBASE.EXE" "$run/baseline/build/INITBASE.OVL" "$run/baseline/$mode" > "$run/baseline/$mode.log" 2>&1; then
    echo "ERROR native baseline $mode ($run/baseline/$mode.log)";errors=$((errors+1));continue
   fi
   touch "$run/baseline/$mode.ok"
  fi
  ;;
 *) echo "ERROR unknown target oracle: $patch_file"; errors=$((errors+1)); continue;;
 esac
 scratch=$run/$name
 copy_source "$scratch/component"
 if ! patch --batch --forward --fuzz=0 -p1 -d "$scratch/component" < "$src/$patch_file" > "$scratch/patch.log" 2>&1; then
  echo "ERROR patch: $patch_file"; errors=$((errors+1)); continue
 fi
 case $mode in native-*)
  if ! "$make_cmd" --no-print-directory -j2 -C "$scratch/component" TOP="$top" OUT="$scratch/build" package > "$scratch/build.log" 2>&1; then
   echo "ERROR target compilation $name ($scratch/build.log)";errors=$((errors+1));continue
  fi
  good=1
  for pass in 1 2; do
   rc=0
   sh tests/native/check.sh "$oracle" "$top" "$scratch/build/INITBASE.EXE" "$scratch/build/INITBASE.OVL" "$scratch/observe-$pass" > "$scratch/observe-$pass.log" 2>&1 || rc=$?
   if [ "$rc" -ne 1 ] || ! grep -E '^RED: first difference|^RED console-review:' "$scratch/observe-$pass.log" > "$scratch/verdict-$pass"; then good=0; fi
  done
  if [ "$good" -eq 1 ] && cmp -s "$scratch/verdict-1" "$scratch/verdict-2"; then
   echo "  killed $name ($mode: completed guest, same independent screen rejection twice)";killed=$((killed+1))
  else
   echo "ERROR/SURVIVED $name ($scratch/observe-1.log)";errors=$((errors+1))
  fi
  continue;;
 esac
 good=1
 for pass in 1 2; do
  rm -f "$scratch/build/IBASE.elf" "$scratch/build/INITBASE.EXE" "$scratch/build/INITBASE.OVL"
  rc=0
  "$make_cmd" --no-print-directory -j2 -C "$scratch/component" TOP="$top" OUT="$scratch/build" package > "$scratch/build-$pass.log" 2>&1 || rc=$?
  if [ "$rc" -eq 0 ] || ! grep -F "$diagnostic" "$scratch/build-$pass.log" > "$scratch/diagnostic-$pass"; then good=0; fi
  if [ "$mode" = static-reenter ] && ! grep -F ' <- ibase_do <- report_ovl_entry' "$scratch/build-$pass.log" >/dev/null; then good=0; fi
  while IFS= read -r object; do
   if [ ! -s "$scratch/build/$object" ]; then good=0; fi
  done < "$run/objects"
 done
 if [ "$good" -eq 1 ]; then
  echo "  killed $name ($mode: all target objects compiled; semantic rejection twice)"
  killed=$((killed+1))
 else
  echo "ERROR/SURVIVED $name ($scratch/build-1.log)"; errors=$((errors+1))
 fi
done
echo "InitechBase target mutations: $n patches, $killed killed, $errors errors"
echo "evidence: $run"
[ "$errors" -eq 0 ]
