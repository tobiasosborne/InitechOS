#!/bin/sh
# Ref: PRD sections 8/9/14; CLAUDE.md Rules 1/6; initech-xses3.
set -eu
LC_ALL=C
export LC_ALL
refuse() { printf 'proof-replay: %s\n' "$1" >&2; exit 2; }
root=$(git rev-parse --show-toplevel 2>/dev/null) || refuse 'not a git worktree'
cd "$root"
manifest=build/proof/manifest
[ -s "$manifest" ] || refuse 'manifest missing or empty'
if grep -q '[^ -~]' "$manifest"; then refuse 'non-ASCII manifest'; fi
number=0
failed=0
while IFS= read -r record || [ -n "$record" ]; do
    number=$((number + 1))
    kind=${record%%|*}; rest=${record#*|}
    sha=${rest%%|*}; rest=${rest#*|}
    command=${rest%%|*}; expect=${rest#*|}
    # Exactly four nonempty fields, no escape language or implicit eval data.
    case $kind in RED|GREEN|MUTANT) ;; *) refuse "record $number: invalid KIND";; esac
    [ "$record" = "$kind|$sha|$command|$expect" ] || refuse "record $number: malformed fields"
    case $expect in ''|*'|'*) refuse "record $number: invalid EXPECT";; esac
    case $sha in ''|*[!0-9a-f]*) refuse "record $number: invalid SHA";; esac
    [ "${#sha}" -eq 40 ] && [ -n "$command" ] || refuse "record $number: incomplete record"
    resolved=$(git rev-parse --verify "$sha^{commit}" 2>/dev/null) || refuse "record $number: commit unavailable"
    [ "$resolved" = "$sha" ] || refuse "record $number: SHA must name the commit itself"
    wt=$root/build/proof/wt-$sha
    log=$root/build/proof/record-$number.log
    if [ -e "$wt" ]; then
        [ "$(git -C "$wt" rev-parse --show-toplevel 2>/dev/null)" = "$wt" ] || refuse "record $number: invalid worktree"
        [ "$(git -C "$wt" rev-parse HEAD)" = "$sha" ] || refuse "record $number: worktree HEAD mismatch"
        git -C "$wt" diff --quiet && git -C "$wt" diff --cached --quiet || refuse "record $number: dirty proof worktree"
        [ -z "$(git -C "$wt" ls-files --others --exclude-standard)" ] || refuse "record $number: untracked proof inputs"
    else
        git worktree add --quiet --detach "$wt" "$sha" || refuse "record $number: worktree creation failed"
    fi
    rc=0
    (cd "$wt" && sh -c "$command") < /dev/null > "$log" 2>&1 || rc=$?
    status_ok=no
    case $kind in GREEN) [ "$rc" -ne 0 ] || status_ok=yes;;
        RED|MUTANT) [ "$rc" -eq 0 ] || status_ok=yes;; esac
    if [ "$status_ok" = yes ] && grep -qF -- "$expect" "$log"; then
        printf 'PASS %s %s %s (exit %s)\n' "$number" "$kind" "$sha" "$rc"
    else
        printf 'FAIL %s %s %s (exit %s; EXPECT: %s)\n' "$number" "$kind" "$sha" "$rc" "$expect"
        failed=1
    fi
done < "$manifest"
exit "$failed"
