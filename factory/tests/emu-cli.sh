#!/bin/sh
# Review F2 (initech-kkqbw follow-up): the real harness CLIs, linked to a
# scripted run, exit 3 and record infrastructure when the emulator died after
# partial output; a completed run's verdict stays 0/1 and a triple fault stays
# product evidence. Usage: emu-cli.sh QEMU_CLI BOCHS_CLI
set -u
. "$(dirname "$0")/../testlib/shell-checks.sh"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
case_() { # cli label scenario want-exit want-infra(0/1)
    : > "$work/infra"
    rc=0
    EMU_STUB=$3 INITECH_INFRA_LOG=$work/infra "$1" --disk unused --expect READY --quit-after READY \
        --out "$work" --name stub > "$work/out" 2> "$work/err" || rc=$?
    if [ "$rc" -ne "$4" ]; then fail "emu cli $2/$3: exit $rc, want $4"; return; fi
    if [ "$5" = 1 ] && ! grep -q "exit 3 (.*abnormal_end=1)" "$work/infra"; then fail "emu cli $2/$3: no infrastructure record"; return; fi
    if [ "$5" = 0 ] && [ -s "$work/infra" ]; then fail "emu cli $2/$3: unexpected infrastructure record"; return; fi
    ok
}
for cli in "qemu:$1" "bochs:$2"; do
    label=${cli%%:*} bin=${cli#*:}
    case_ "$bin" "$label" complete 0 0
    case_ "$bin" "$label" verdict 1 0
    case_ "$bin" "$label" stopped 1 0
    case_ "$bin" "$label" triple 1 0
    case_ "$bin" "$label" sigkill 3 1
    case_ "$bin" "$label" sigsegv 3 1
    case_ "$bin" "$label" badexit 3 1
done
# Review F2 follow-up: the QMP session failing on the harness's own channel
# (transport, send, capture, screendump) after partial output is no verdict;
# a missing marker ('stopped' above) still is.
case_ "$1" qemu qmpfail 3 1
summary
