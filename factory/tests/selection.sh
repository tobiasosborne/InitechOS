#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
make -s smoke-selection
for args in 'SMOKE_COMPONENTS=' 'SMOKE_COMPONENTS=turbo'; do
 if make -s smoke-selection "$args" > "$work/result" 2>&1; then echo "FAIL selection accepted $args"; exit 1; fi
 grep -E 'FAIL smoke: (empty|incomplete)' "$work/result"
done
for cpus in ' ' qemu bogus 'qemu qemu bochs'; do
 if IBASE_SMOKE_CPUS="$cpus" make -s smoke-selection > "$work/result" 2>&1; then echo "FAIL selection accepted CPUs [$cpus]"; exit 1; fi
 grep -E 'FAIL smoke: (empty|incomplete|unknown)' "$work/result"
done
INITECH_FULL_SMOKE=0 sh factory/smoke-selection.sh emulators qemu 'qemu bochs' > "$work/subset"
grep -F 'SUBSET SMOKE:' "$work/subset"
echo 'ok smoke coverage: full default, incomplete/empty/unknown/duplicate selection rejection, labelled subset'
