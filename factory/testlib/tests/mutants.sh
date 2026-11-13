#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cd "$work"
mkdir -p good/mutants pending/mutants/pending bad/mutants silent/mutants
: > good/mutants/a.patch
: > pending/mutants/pending/a.patch
: > bad/mutants/a.patch
: > silent/mutants/a.patch
printf '.PHONY: mutants\nmutants:\n\t@:\n' > silent/Makefile
printf '.PHONY: mutants\nmutants:\n\t@echo "good: 1 mutants, 1 killed, 0 survived, 0 errors"\n' > good/Makefile
printf '.PHONY: mutants\nmutants:\n\t@exit 1\n' > pending/Makefile
printf '.PHONY: mutants\nmutants:\n\t@echo "bad: 1 mutants, 0 killed, 1 survived, 0 errors"; exit 1\n' > bad/Makefile
if sh "$root/factory/mutants.sh" "$work/log" > red; then echo 'FAIL survivor accepted'; exit 1; fi
grep -q 'bad: 1 mutants, 0 killed, 1 survived' red
grep -q 'FAIL silent: mutation target reported no counts' red
rm -rf bad silent log
sh "$root/factory/mutants.sh" "$work/log" > green
grep -q 'good: 1 mutants, 1 killed' green
grep -q 'pending: 1 pending patches' green
echo 'mutation summary fixtures: survivor red; killed and pending-only green'
