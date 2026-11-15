#!/bin/sh
# One entry as one check: its output goes to LOG followed by the
# "N checks, M failures" summary. Exit 1 (a failed check: its FAIL lines and
# this entry's name are the assertion) is the only semantic failure; any
# other status (timeout, signal, setup) stays an error, never a kill.
runner=$1 out=$2 entry=$3 log=$4
rc=0
sh "$runner" "$out" "$entry" > "$log" 2>&1 || rc=$?
if [ "$rc" -eq 0 ]; then echo '1 checks, 0 failures' >> "$log"; exit 0; fi
if [ "$rc" -eq 1 ]; then
    echo "FAIL $entry" >> "$log"
    echo '1 checks, 1 failures' >> "$log"
fi
exit "$rc"
