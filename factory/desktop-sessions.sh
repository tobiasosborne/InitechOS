#!/bin/sh
# One journey per boot; adjacent Calculator/Note Pad retain shared clipboard work.
set -eu
[ "$#" -gt 0 ] || { echo 'FAIL desktop sessions: empty selection' >&2; exit 1; }
pending=
for dir do
 if [ -n "$pending" ]; then
  if [ "$pending" = apps/calculator/tests/smoke ] && [ "$dir" = apps/notepad/tests/smoke ]; then
   printf '%s %s\n' "$pending" "$dir"
   pending=
   continue
  fi
  printf '%s\n' "$pending"
 fi
 pending=$dir
done
[ -z "$pending" ] || printf '%s\n' "$pending"
