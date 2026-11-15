#!/bin/sh
# Kill contract item 4 (testlib/README.md): append one failed build-tool run to
# $INITECH_INFRA_LOG. Usage: infra-record.sh TOOL STATUS ARGS...
# The line names the command, not only the tool, so a baseline's expected
# refusal (say, a negative test compiling invalid.c) cannot cancel a mutant's
# new failure of another command (initech-kkqbw, review F1). The run's own
# directory ($INITECH_INFRA_ROOT) and mktemp names are normalised so the same
# command matches between the baseline copy and a mutant copy.
[ -n "${INITECH_INFRA_LOG:-}" ] || exit 0
tool=$1 rc=$2
shift 2
line="$tool exit $rc in $(pwd):"
for a do line="$line $a"; done
printf '%s\n' "$line" | tr '\n' ' ' |
    awk -v r="${INITECH_INFRA_ROOT:-}" '{
        if (r != "") while ((i = index($0, r)) > 0) $0 = substr($0, 1, i - 1) "@" substr($0, i + length(r))
        gsub(/tmp\.[A-Za-z0-9]+/, "tmp.*"); sub(/ +$/, ""); print }' >> "$INITECH_INFRA_LOG"
