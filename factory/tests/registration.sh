#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
if make -s -C flair/region test-run HOST_TESTS= > "$work/region" 2>&1; then
 echo 'FAIL empty region registration accepted'; exit 1
fi
grep -F 'empty test registration/selection' "$work/region"
make -s -C flair/region test-build HOST_TESTS=
if sh factory/testlib/run-entries.sh empty "$work/out" /dev/null > "$work/entries" 2>&1; then
 echo 'FAIL empty run-entries accepted'; exit 1
fi
grep -F 'empty test registration/selection' "$work/entries"
for line in 'flair/region: 0 tests, 0 failures' 'no tests ran'; do
 if printf '%s\n' "$line" | awk -v component=flair/region -f factory/testlib/component-result.awk; then
  echo 'FAIL missing positive receipt accepted'; exit 1
 fi
done
printf 'flair/region: 2 tests, 0 failures\n' | awk -v component=flair/region -f factory/testlib/component-result.awk
echo 'ok registration: empty runners and missing/zero receipts rejected; build-only usable'
