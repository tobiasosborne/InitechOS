#!/bin/sh
# CLAUDE.md, How the tree is organised: imported is a checked condition.
set -eu
pattern=$1 database=$2 artifacts=$3 flags=$4
shift 4
bad=0
if printf '%s\n' "$flags" | grep -q attic || grep -n attic "$@"; then
    echo 'FAIL attic: build files/include paths reference attic'; bad=1
fi
files=$(find . -type d \( -name tests -o -name mutants -o -name build -o -name .git \) -prune -o -type f \( -name '*.c' -o -name '*.h' -o -name '*.asm' -o -name '*.pas' \) -print)
if [ -n "$files" ]; then
    if grep -nE '^[[:space:]]*#[[:space:]]*include.*attic' $files; then echo 'FAIL attic: product include references attic'; bad=1; fi
    if grep -nE "$pattern" $files; then echo 'FAIL mutation: product mutation hook'; bad=1; fi
    if grep -nE '^[[:space:]]*#[[:space:]]*include[[:space:]]*["<]\.\./' $files; then
        echo 'FAIL includes: product include escapes component'; bad=1
    fi
fi
# Walk the actual make graph, rather than trusting a declared artifact list.
if [ -z "$artifacts" ] || ! awk -v wanted="$artifacts" '
/^[^#\t ][^=]*: / { key=$1; sub(/:$/, "", key); deps[key]=deps[key] " " substr($0,index($0,":")+1) }
function visit(key, n,a,i) { if (seen[key]++) return; n=split(deps[key],a," "); for(i=1;i<=n;i++) if(a[i]!="|") visit(a[i]) }
END { visit("all"); n=split(wanted,a," "); for(i=1;i<=n;i++) if(!seen[a[i]]) exit 1 }
' "$database"; then
    echo 'FAIL target: declare TARGET_ARTIFACTS and make all depend on them'; bad=1
fi
exit "$bad"
