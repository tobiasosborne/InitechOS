#!/bin/sh
# Compare completed observations with captured references. A missing reference
# or failed observer is infrastructure failure, never a semantic mutation kill.
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
repo=$(CDPATH= cd -- "$src/../../../.." && pwd)
out=${1:-"$repo/build/apps/initechbase/conformance"}
reference=${2:-"$src/reference"}
(cd "$src" && sha256sum --quiet -c INPUTS.sha256)
(cd "$reference" && sha256sum --quiet -c SHA256SUMS)
fail=0;selected=0
for case_dir in "$src"/cases/C*; do
 name=${case_dir##*/}; host=$out/host-results/$name; ref=$reference/$name
 case " ${CASES:-all} " in *" all "*|*" $name "*) ;; *) continue;; esac
 selected=$((selected+1))
 if [ ! -f "$ref/RESULT.OUT" ] || ! grep -q "END $name" "$ref/RESULT.OUT"; then
  printf 'ERROR %s: completed real III+ capture required\n' "$name";exit 2
 fi
 if [ ! -f "$host/exit-status" ] || [ "$(cat "$host/exit-status")" != 0 ]; then
  printf 'ERROR %s: host crashed, timed out, or returned failure\n' "$name";exit 2
 fi
 mismatch=0
 if ! cmp -s "$ref/RESULT.OUT" "$host/RESULT.OUT"; then
  printf 'FAIL: %s transcript\n' "$name";mismatch=1
 fi
 for file in BASE.DBF SAVED.DBF; do
  test -f "$ref/$file" || { printf 'ERROR %s missing reference %s\n' "$name" "$file";exit 2; }
  if [ ! -f "$host/$file" ]; then
   printf 'FAIL: %s missing %s\n' "$name" "$file";mismatch=1;continue
  fi
  python3 "$src/read_dbf.py" "$ref/$file" > "$host/$file.reference" || exit 2
  if ! python3 "$src/read_dbf.py" "$host/$file" > "$host/$file.meaningful"; then
   printf 'FAIL: %s invalid %s\n' "$name" "$file";mismatch=1;continue
  fi
  if ! cmp -s "$host/$file.meaningful" "$host/$file.reference"; then
   printf 'FAIL: %s meaningful %s bytes\n' "$name" "$file";mismatch=1
  fi
 done
 if [ -f "$ref/BASE.DBT" ]; then
  if [ ! -f "$host/BASE.DBT" ]; then
   printf 'FAIL: %s missing memo file\n' "$name";mismatch=1
  else
   python3 "$src/read_dbf.py" --dbt "$ref/BASE.DBT" > "$host/memo.reference" || exit 2
   if ! python3 "$src/read_dbf.py" --dbt "$host/BASE.DBT" > "$host/memo.txt" ||
      ! cmp -s "$host/memo.txt" "$host/memo.reference"; then
    printf 'FAIL: %s memo bytes\n' "$name";mismatch=1
   fi
  fi
 fi
 if [ -f "$ref/OLD.NDX" ] && ! cmp -s "$host/OLD.NDX" "$ref/OLD.NDX"; then
  rc=0
  python3 "$src/../ndx_ref.py" --index-dump "$host/OLD.NDX" > "$host/index.txt" 2>&1 || rc=$?
  python3 "$src/../ndx_ref.py" --index-dump "$ref/OLD.NDX" > "$host/index.reference" 2>&1 || exit 2
  if [ "$rc" -ne 0 ] || ! cmp -s "$host/index.txt" "$host/index.reference"; then
   printf 'FAIL: %s index readback (inspect reader diagnostics)\n' "$name";mismatch=1
  fi
 fi
 if [ "$mismatch" = 0 ]; then printf 'GREEN %s real III+ transcript and table readback\n' "$name";else fail=$((fail+1));fi
done
test "$selected" -gt 0 || { echo 'FAIL: no selected conformance cases' >&2;exit 2; }
printf 'INITECH_CHECKS %s failures=%s\n' "$selected" "$fail"
test "$fail" -eq 0
