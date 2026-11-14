#!/bin/sh
# This study is opt-in and honestly RED for known differences. Never mints.
# A missing/incomplete reference is PENDING and makes the command fail too.
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
  printf 'PENDING %s: completed real III+ capture required\n' "$name";fail=1;continue
 fi
 if [ ! -f "$host/exit-status" ] || [ "$(cat "$host/exit-status")" != 0 ]; then
  printf 'RED %s: host crashed, timed out, or returned failure\n' "$name";fail=1;continue
 fi
 mismatch=0
 if ! cmp -s "$ref/RESULT.OUT" "$host/RESULT.OUT"; then
  printf 'RED %s transcript\n' "$name";mismatch=1
 fi
 for file in BASE.DBF SAVED.DBF; do
  if [ ! -f "$ref/$file" ] || [ ! -f "$host/$file" ]; then
   printf 'RED %s missing %s\n' "$name" "$file";mismatch=1;continue
  fi
  python3 "$src/read_dbf.py" "$host/$file" > "$host/$file.meaningful"
  python3 "$src/read_dbf.py" "$ref/$file" > "$host/$file.reference"
  if ! cmp -s "$host/$file.meaningful" "$host/$file.reference"; then
   printf 'RED %s meaningful %s bytes\n' "$name" "$file";mismatch=1
  fi
 done
 if [ -f "$ref/BASE.DBT" ]; then
  if [ ! -f "$host/BASE.DBT" ]; then
   printf 'RED %s missing memo file\n' "$name";mismatch=1
  else
   python3 "$src/read_dbf.py" --dbt "$host/BASE.DBT" > "$host/memo.txt"
   python3 "$src/read_dbf.py" --dbt "$ref/BASE.DBT" > "$host/memo.reference"
   if ! cmp -s "$host/memo.txt" "$host/memo.reference"; then
    printf 'RED %s memo bytes\n' "$name";mismatch=1
   fi
  fi
 fi
 if [ -f "$ref/OLD.NDX" ] && ! cmp -s "$host/OLD.NDX" "$ref/OLD.NDX"; then
  rc=0
  python3 "$src/../ndx_ref.py" --index-dump "$host/OLD.NDX" > "$host/index.txt" 2>&1 || rc=$?
  python3 "$src/../ndx_ref.py" --index-dump "$ref/OLD.NDX" > "$host/index.reference" 2>&1 || rc=$?
  if [ "$rc" -ne 0 ] || ! cmp -s "$host/index.txt" "$host/index.reference"; then
   printf 'RED %s index readback (inspect reader diagnostics)\n' "$name";mismatch=1
  fi
 fi
 if [ "$mismatch" = 0 ]; then printf 'GREEN %s real III+ transcript and table readback\n' "$name";else fail=1;fi
done
test "$selected" -gt 0 || { echo 'FAIL: no selected conformance cases' >&2;exit 2; }
exit "$fail"
