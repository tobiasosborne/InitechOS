#!/bin/sh
# POSIX pipelines discard producer status. Complete an archive before reading it.
set -eu
destination=$1
shift
archive=$(mktemp)
trap 'rm -f "$archive"' EXIT HUP INT TERM
if tar -cf "$archive" "$@"; then :; else
 rc=$?
 echo "FAIL snapshot: source archive creation failed (exit $rc)" >&2
 exit "$rc"
fi
if tar -xf "$archive" -C "$destination"; then :; else
 rc=$?
 echo "FAIL snapshot: archive extraction failed (exit $rc)" >&2
 exit "$rc"
fi
