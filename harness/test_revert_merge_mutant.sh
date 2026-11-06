#!/bin/sh
# test_revert_merge_mutant.sh -- mutation proof for test_revert_merge.sh
# (initech-ror2g; CLAUDE.md Rule 6).
#
#   sh harness/test_revert_merge_mutant.sh TOOL TEST SCRATCH
#
# Each mutant is a sed-patched copy of TOOL under SCRATCH. The tool marks the
# two guard lines with `# CHECK:<name>`; a mutant replaces that whole line by
# `:`. The oracle TEST must then FAIL, and the FAIL line must name the case
# that guards the mutated line.
set -u
tool=$1
test_sh=$2
scratch=$3
rm -rf "$scratch"
mkdir -p "$scratch"
bad=0

# mutant NAME MARKER CASE : drop the marked guard; expect `FAIL <CASE>`.
mutant() {
    name=$1; marker=$2; want=$3
    m=$scratch/$name.sh
    sed "/# CHECK:$marker\$/s/.*/:/" "$tool" > "$m"
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

if [ "$bad" -ne 0 ]; then
    printf '!!! test-revert-merge-mutant: %s mutant(s) not killed\n' "$bad" >&2
    exit 1
fi
printf '>>> test-revert-merge-mutant: green (skip-origin-ahead and push-on-red-smoke both killed)\n'
