#!/bin/sh
# Observation only. Exit nonzero on crash/timeout; differences are check.sh's job.
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
repo=$(CDPATH= cd -- "$src/../../../.." && pwd)
out=${1:-"$repo/build/apps/initechbase/conformance"}
fail=0
for case_dir in "$src"/cases/C*; do
 name=${case_dir##*/}; work=$out/host-results/$name
 case " ${CASES:-all} " in *" all "*|*" $name "*) ;; *) continue;; esac
 rm -rf "$work"
 mkdir -p "$work"; cp "$case_dir"/* "$work/"
 rc=0
 timeout -k 1 5 "$out/host" "$work" > "$work/console.txt" 2>&1 || rc=$?
 printf '%s\t%s\n' "$name" "$rc"
 printf '%s\n' "$rc" > "$work/exit-status"
 test "$rc" -eq 0 || fail=1
done
exit "$fail"
