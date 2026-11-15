#!/bin/sh
# initech-8l4kp: desktop-trace.sh's press-and-hold step. The emitted trace is
# the contract both frontends read: press, wait for the app's marker (or a
# fixed settle), capture while held (QEMU only), release.
set -u
. "$(dirname "$0")/../testlib/shell-checks.sh"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
. "$(dirname "$0")/../desktop-trace.sh"
command -v hold_capture >/dev/null 2>&1 || fail "trace-hold.sh: desktop-trace.sh has no hold_capture"
out=/run/out markers=$work/markers
: > "$markers"
emulator=qemu
got=$(hold_capture key7-held 'CALC-KEY-DOWN key=7')
[ "$got" = 'l1,WCALC-KEY-DOWN key=7,D/run/out/key7-held.ppm,l0,' ] || fail "trace-hold.sh: qemu trace [$got]"
ok
grep -Fxq 'CALC-KEY-DOWN key=7' "$markers" || fail "trace-hold.sh: marker not expected"
ok
emulator=bochs
got=$(hold_capture key7-held 'CALC-KEY-DOWN key=7')
[ "$got" = 'l1,WCALC-KEY-DOWN key=7,l0,' ] || fail "trace-hold.sh: bochs trace [$got]"
ok
emulator=qemu
got=$(hold_capture idle)
[ "$got" = 'l1,w500,D/run/out/idle.ppm,l0,' ] || fail "trace-hold.sh: settle-only trace [$got]"
ok
summary
