#!/bin/sh
# Two makes that rebuild one output at once (mutation copies of different
# components building a shared dependency, as Note Pad's baseline did in the
# strict certificate) must both succeed: each keeps a private command file.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
command_mk=${RUN_COMMAND_MK:-$root/mk/command.mk}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cat > "$work/Makefile" <<MAKE
include $command_mk
\$(OUT)/tool: FORCE
	\$(call run-command,sleep 1 && touch \$@)
MAKE
rc1=0 rc2=0
make -s -C "$work" OUT="$work/out" "$work/out/tool" > "$work/a.log" 2>&1 & a=$!
sleep 0.3
make -s -C "$work" OUT="$work/out" "$work/out/tool" > "$work/b.log" 2>&1 || rc2=$?
wait "$a" || rc1=$?
if [ "$rc1" -ne 0 ] || [ "$rc2" -ne 0 ] || [ ! -f "$work/out/tool.cmd" ]; then
    echo "FAIL run-command race: concurrent rebuilds exited $rc1 and $rc2"; cat "$work/a.log" "$work/b.log"; exit 1
fi
leftover=$(find "$work/out" -name '*.tmp' | wc -l)
[ "$leftover" -eq 0 ] || { echo "FAIL run-command race: $leftover temporary files left"; exit 1; }
echo 'ok run-command: concurrent rebuilds of one output both succeed'
