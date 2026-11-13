#!/bin/sh
# test_revert_merge_mutant.sh -- mutation proof for test_revert_merge.sh
# (initech-ror2g; CLAUDE.md Rule 6).
#
#   sh harness/test_revert_merge_mutant.sh TOOL TEST SCRATCH
#
# Each mutant is a sed-patched copy of TOOL under SCRATCH. The tool marks the
# guard and selector lines with `# CHECK:<name>`; a mutant replaces that line
# by `:` or a print-only selector. TEST must FAIL, and the FAIL line names the case
# that guards the mutated line.
set -u
tool=$1
test_sh=$2
scratch=$3
rm -rf "$scratch"
mkdir -p "$scratch"
bad=0

# mutant NAME MARKER CASE [REPLACEMENT]: patch the marker; expect FAIL <CASE>.
mutant() {
    name=$1; marker=$2; want=$3
    replacement=${4-:}
    m=$scratch/$name.sh
    sed "/# CHECK:$marker\$/s/.*/$replacement/" "$tool" > "$m"
    if cmp -s "$tool" "$m"; then
        printf '!!! mutant %s: marker CHECK:%s not found in %s\n' "$name" "$marker" "$tool" >&2
        bad=$((bad + 1)); return
    fi
    if sh "$test_sh" "$m" "$scratch/$name.work" > "$scratch/$name.log" 2>&1; then
        printf '!!! mutant %s SURVIVED: the oracle stayed green\n' "$name" >&2
        bad=$((bad + 1)); return
    fi
    if ! grep -q "^FAIL $want" "$scratch/$name.log"; then
        printf '!!! mutant %s went RED for the wrong reason (wanted FAIL %s):\n' "$name" "$want" >&2
        sed 's/^/    | /' "$scratch/$name.log" >&2
        bad=$((bad + 1)); return
    fi
    printf '>>> mutant %s: killed (FAIL %s)\n' "$name" "$want"
}

mutant skip-origin-ahead origin-ahead origin-ahead
mutant push-on-red-smoke smoke-red smoke-red
mutant skip-executing-selected-gates selected-gates smoke-selected-red \
    '  smoke_cmd="make -s gates-for DIFF=$target^1..$newest"'

if [ "$bad" -ne 0 ]; then
    printf '!!! test-revert-merge-mutant: %s mutant(s) not killed\n' "$bad" >&2
    exit 1
fi
printf '>>> test-revert-merge-mutant: green (all three mutants killed)\n'
