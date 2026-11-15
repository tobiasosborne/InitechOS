#!/bin/sh
# Kill contract item 4 (testlib/README.md): append one failed build-tool run to
# $INITECH_INFRA_LOG. Usage: infra-record.sh TOOL STATUS ARGS...
# The line names the command and its inputs, not only the tool, so a
# baseline's expected refusal (a negative test compiling invalid source)
# cannot cancel a mutant's new failure of another compile (initech-kkqbw,
# reviews F1 and its follow-up). The run's own directory ($INITECH_INFRA_ROOT)
# and mktemp names are normalised so the same command matches between the
# baseline copy and a mutant copy; every existing input file argument also
# carries a hash of its contents, so two generated sources behind the same
# temp-name shape stay distinct. Output arguments (after -o, -MF, -MT, -MQ)
# are not hashed: a semantic mutant legitimately changes its outputs.
[ -n "${INITECH_INFRA_LOG:-}" ] || exit 0
tool=$1 rc=$2
shift 2
line="$tool exit $rc in $(pwd):"
output=0
for a do
    if [ "$output" -eq 0 ] && [ -f "$a" ] && [ -r "$a" ]; then
        a="$a#$(sha256sum < "$a" | cut -c1-16)"
    fi
    case $a in -o|-MF|-MT|-MQ) output=1;; *) output=0;; esac
    line="$line $a"
done
printf '%s\n' "$line" | tr '\n' ' ' |
    awk -v r="${INITECH_INFRA_ROOT:-}" '{
        if (r != "") while ((i = index($0, r)) > 0) $0 = substr($0, 1, i - 1) "@" substr($0, i + length(r))
        gsub(/tmp\.[A-Za-z0-9]+/, "tmp.*"); sub(/ +$/, ""); print }' >> "$INITECH_INFRA_LOG"
