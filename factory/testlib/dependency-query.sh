#!/bin/sh
# A make -q result1 is ordinary out-of-date work; all other errors propagate.
set -eu
root=$1 component=$2 make_cmd=$3 graph=$4
rc=0
"$make_cmd" --no-print-directory -qp -C "$component" all test > "$graph" 2> "$graph.stderr" || rc=$?
case $rc in 0|1) :;; *) cat "$graph.stderr" >&2; echo "FAIL dependency: query for $component exited $rc" >&2; exit 1;; esac
awk -v expected="$root/$component" -f "$(dirname "$0")/make-database.awk" "$graph" > "$graph.parent"
