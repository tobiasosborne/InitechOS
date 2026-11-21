#!/bin/sh
# A hunk with no context lines (`@@ -N,0 +M @@` and nothing else) applies at
# a bare line number wherever that line now falls, so the factory's dry run
# cannot see when the source under it moves. Twenty-four such patches drifted
# into other functions (initech-4sqj2, 6ld9x, 98eaw, 07r1b, f84lh). Every
# hunk must carry context. The target grader's controls are not mutants and
# are outside the factory's check, so they are dry-run here.
set -eu
bad=0
for p in $(find mutants tests/target-controls -type f -name '*.patch' | sort); do
 # An empty old side (`-N,0`, N > 0) is a pure insertion with nothing to
 # match. A hunk with context or removed lines is located by them.
 if grep -Eq '^@@ -[1-9][0-9]*,0 ' "$p" || ! grep -q '^@@ ' "$p"; then
  echo "FAIL patch hunk without context: $p"; bad=1
 fi
done
for p in tests/target-controls/*.patch; do
 if ! out=$(patch --dry-run --batch --forward --fuzz=0 -p1 < "$p" 2>&1); then
  printf 'FAIL stale target control: %s\n%s\n' "$p" "$out"; bad=1
 fi
done
[ "$bad" -eq 0 ] && echo 'patch anchors: every hunk has context; target controls apply'
