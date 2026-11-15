#!/bin/sh
# Independent foreign-format and displayed timestamp comparisons. The producer
# must have completed all real INT21 operations before any property is graded.
set -eu
. kernel/tests/assertions.sh
image=$1 reference=$2 log=$3
if ! grep -qx 'STAMP HOST: real INT21 create/write/truncate/mkdir/read-close completed' "$log"; then
 echo 'ERROR: stamp producer did not complete the INT21 operations' >&2
 exit 2
fi
for pair in 'NEW.TXT EARLY.TXT' 'OLD.TXT LATE.TXT' 'TRUNC.TXT LATE.TXT' 'SUB EARLY.TXT' 'SUB\CHILD.TXT EARLY.TXT'; do
 set -- $pair
 want=$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py "$reference" --stat-path-time "$2")
 got=$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py "$image" --stat-path-time "$1")
 test -n "$want" || exit 2
 assert test "$got" = "$want"
done
finish
