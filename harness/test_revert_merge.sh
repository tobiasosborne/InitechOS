#!/bin/sh
# test_revert_merge.sh -- oracle for tools/revert-merge.sh (initech-ror2g, F10
# of epic initech-0uphq; policy docs/process/MERGE-REVERT.md).
#
#   sh harness/test_revert_merge.sh TOOL SCRATCH
#
# TOOL     the revert-merge.sh under test (the mutant gate passes a sed-patched
#          copy; the original is never edited).
# SCRATCH  private scratch dir; wiped on entry. Every case builds its OWN
#          throwaway git repository (a local bare 'origin', a main with three
#          no-ff merges M1 M2 M3, a tiny Makefile with a fake green and a fake
#          red gate), so no case can see another's state and nothing here
#          touches the real checkout or the real origin.
#
# Failure lines read `FAIL <case>: <why>`; the mutant gate greps for them.
# Exit 0 only if every case passes.
set -u

tool=$1
scratch=$2
case $tool in /*) ;; *) tool=$(pwd)/$tool ;; esac
case $scratch in /*) ;; *) scratch=$(pwd)/$scratch ;; esac

# Nothing from the invoking make / git environment may leak into the fixture.
unset MAKEFLAGS MAKELEVEL MFLAGS GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE SMOKE
unset HAVE_GATES_FOR
export GIT_CONFIG_NOSYSTEM=1 GIT_CONFIG_GLOBAL=/dev/null
export GIT_AUTHOR_NAME=Oracle GIT_COMMITTER_NAME=Oracle
export GIT_AUTHOR_EMAIL=oracle@example.invalid GIT_COMMITTER_EMAIL=oracle@example.invalid
export GIT_TERMINAL_PROMPT=0

rm -rf "$scratch"
mkdir -p "$scratch"

bad=0
fail() { printf 'FAIL %s: %s\n' "$case_name" "$*" >&2; bad=$((bad + 1)); }
ok()   { printf 'ok   %s\n' "$case_name"; }

# ---------------------------------------------------------------- fixture --
# fixture NAME : sets $o (bare origin), $w (work clone), $mark (smoke marker).
mkfixture() {
    case_name=$1
    d=$scratch/$1
    o=$d/origin.git
    w=$d/work
    mark=$d/smoke.mark
    RS=; RG=
    mkdir -p "$d"
    git init -q --bare "$o"
    git -C "$o" symbolic-ref HEAD refs/heads/main
    git init -q "$w"
    git -C "$w" checkout -q -b main
    git -C "$w" remote add origin "$o"
    cat > "$w/Makefile" <<'EOF'
# fixture: a fake green gate and a fake red gate. Each gate that runs
# records the subject of HEAD at that moment in $SMOKE_MARK.
fake-green:
	@printf 'fake-green HEAD=%s\n' "$$(git log -1 --format=%s)" >> "$$SMOKE_MARK"
fake-red:
	@printf 'fake-red HEAD=%s\n' "$$(git log -1 --format=%s)" >> "$$SMOKE_MARK"
	@false
test-kernel-repro test-kernel-flair-repro test-ovl-repro test-merge-samir:
	@printf '%s\n' "$@" >> "$$SMOKE_MARK"
ifeq ($(HAVE_GATES_FOR),1)
gates-for:
	@printf 'gates-for DIFF=%s\n' "$(DIFF)" >> "$$SMOKE_MARK"
endif
EOF
    printf 'base\n' > "$w/log.txt"
    git -C "$w" add .
    git -C "$w" commit -q -m 'base'
    git -C "$w" push -q origin main
    for i in 1 2 3; do addmerge "$i" "M$i.txt"; done
    git -C "$w" push -q origin main
    : > "$mark"
}

# addmerge N FILE : branch lane/MN off main, touch FILE and append to log.txt
# (so reverting M2 alone, with M3 above it, conflicts: ordering matters),
# then merge it back with --no-ff.
addmerge() {
    git -C "$w" checkout -q -b "lane/M$1" main
    mkdir -p "$(dirname "$w/$2")"
    printf 'from M%s\n' "$1" > "$w/$2"
    [ "${3-}" = nolog ] || printf 'M%s\n' "$1" >> "$w/log.txt"
    git -C "$w" add .
    git -C "$w" commit -q -m "lane M$1 work"
    git -C "$w" checkout -q main
    git -C "$w" merge -q --no-ff -m "Merge lane M$1" "lane/M$1"
}

head_of() { git -C "$w" rev-parse HEAD; }
origin_main() { git -C "$o" rev-parse refs/heads/main; }
msha() { git -C "$w" log --first-parent --format=%H --grep="^Merge lane $1\$" -n1; }

# run EXPECTED_RC ARGS... : run the tool in $w, output -> $d/out, rc -> $rc.
run() {
    want=$1; shift
    rc=0
    (cd "$w" && { [ -z "${RS-}" ] || SMOKE=$RS; [ -z "${RG-}" ] || HAVE_GATES_FOR=$RG; export SMOKE HAVE_GATES_FOR; SMOKE_MARK=$mark; export SMOKE_MARK; exec sh "$tool" "$@"; }) > "$d/out" 2>&1 || rc=$?
    if [ "$rc" -ne "$want" ]; then
        fail "expected exit $want, got $rc: $*"
        sed 's/^/    | /' "$d/out" >&2
        return 1
    fi
    return 0
}
has() { grep -q -- "$1" "$d/out"; }

# ------------------------------------------------------------------ cases --

# 1. dirty tree -> refuse, nothing changes.
mkfixture dirty-tree
h=$(head_of); s=$(msha M2)
printf 'local edit\n' >> "$w/log.txt"
if run 2 "$s" && has dirty; then
    [ "$(head_of)" = "$h" ] && [ ! -s "$mark" ] && ok || fail 'smoke ran or HEAD moved'
else
    has dirty || fail 'refusal does not name "dirty"'
fi

# 2. not on main -> refuse.
mkfixture non-main
s=$(msha M3); h=$(head_of)
git -C "$w" checkout -q -b side
if run 2 "$s" && has 'not main'; then
    [ "$(head_of)" = "$h" ] && ok || fail 'HEAD moved'
else
    has 'not main' || fail 'refusal does not say "not main"'
fi

# 3. not a first-parent merge: a plain commit, garbage, and a merge that is
#    only reachable through a second parent (a side merge, then merged in).
mkfixture non-merge
h=$(head_of)
plain=$(git -C "$w" rev-parse "$h^2")   # M3 lane commit: plain, 1 parent
if run 2 "$plain" && has 'not a merge'; then :; else fail 'plain commit not refused as non-merge'; fi
if run 2 deadbeefdeadbeef && has 'not a commit'; then :; else fail 'garbage sha not refused'; fi
git -C "$w" checkout -q -b lane/inner main
printf 'in\n' > "$w/inner.txt"; git -C "$w" add .; git -C "$w" commit -q -m 'inner work'
git -C "$w" checkout -q -b lane/sub main
printf 'sub\n' > "$w/sub.txt"; git -C "$w" add .; git -C "$w" commit -q -m 'sub work'
git -C "$w" merge -q --no-ff -m 'Merge inner into sub' lane/inner
git -C "$w" checkout -q main
git -C "$w" merge -q --no-ff -m 'Merge lane SUB' lane/sub
git -C "$w" push -q origin main
inner=$(git -C "$w" log --format=%H --grep='^Merge inner into sub$' -n1)
h=$(head_of)
if run 2 "$inner" && has 'first-parent'; then
    [ "$(head_of)" = "$h" ] && ok || fail 'HEAD moved'
else
    has 'first-parent' || fail 'second-parent merge not refused with "first-parent"'
fi

# 4. origin/main ahead of HEAD -> refuse. The tool must fetch by itself.
mkfixture origin-ahead
s=$(msha M3); h=$(head_of)
git clone -q "$o" "$d/other"
printf 'x\n' > "$d/other/other.txt"
git -C "$d/other" add .
git -C "$d/other" commit -q -m 'someone else landed'
git -C "$d/other" push -q origin main
if run 2 "$s" -y && has 'origin/main is ahead'; then
    [ "$(head_of)" = "$h" ] && [ ! -s "$mark" ] && ok || fail 'HEAD moved or smoke ran'
else
    has 'origin/main is ahead' || fail 'refusal does not say "origin/main is ahead"'
fi

# 5. -n on M2 with M3 above it: prints the plan (M3 then M2), changes nothing.
mkfixture plan-dry-run
s=$(msha M2); h=$(head_of); oh=$(origin_main)
if run 0 -n "$s"; then
    l3=$(grep -n 'Merge lane M3' "$d/out" | head -n1 | cut -d: -f1)
    l2=$(grep -n 'Merge lane M2' "$d/out" | head -n1 | cut -d: -f1)
    if [ -z "$l3" ] || [ -z "$l2" ] || [ "$l3" -ge "$l2" ]; then
        fail "plan order wrong (M3 line '${l3:-none}', M2 line '${l2:-none}')"
    elif [ "$(head_of)" != "$h" ] || [ "$(origin_main)" != "$oh" ]; then
        fail '-n moved HEAD or origin'
    elif [ -n "$(git -C "$w" status --porcelain --untracked-files=no)" ] || [ -s "$mark" ]; then
        fail '-n left the tree dirty or ran smoke'
    else
        ok
        cp "$d/out" "$scratch/plan-dry-run.out"
    fi
fi

# 6. newer merges exist and neither -y nor -n -> print plan, refuse, no change.
mkfixture needs-yes
s=$(msha M2); h=$(head_of)
if run 2 "$s" && has "-y"; then
    [ "$(head_of)" = "$h" ] && ok || fail 'HEAD moved without -y'
else
    has "-y" || fail 'refusal does not tell the operator to pass -y'
fi

# 7. -y on M2 with M3 above: exactly two reverts (M3 then M2), linear, smoke
#    ran after them, origin updated, bead note carries subject/reason/log.
mkfixture revert-two
s=$(msha M2); m3=$(msha M3)
RS='make -s fake-green'
if run 0 "$s" -y -r 'M2 broke the kernel repro' -l /logs/red-gate.log; then
    n=$(git -C "$w" rev-list --count "$m3..HEAD")
    mg=$(git -C "$w" rev-list --count --merges "$m3..HEAD")
    subj=$(git -C "$w" log --format=%s "$m3..HEAD" | tr '\n' '|')
    if [ "$n" != 2 ]; then fail "expected exactly 2 new commits, got $n"
    elif [ "$mg" != 0 ]; then fail 'revert range contains a merge commit (not linear)'
    elif [ "$subj" != 'Revert "Merge lane M2"|Revert "Merge lane M3"|' ]; then
        fail "revert order wrong: $subj"
    elif [ "$(git -C "$w" rev-parse HEAD~2)" != "$m3" ]; then fail 'reverts are not directly above M3'
    elif [ "$(wc -l < "$mark")" != 1 ] || ! grep -q 'HEAD=Revert "Merge lane M2"' "$mark"; then
        fail "smoke did not run exactly once after both reverts: $(cat "$mark")"
    elif [ "$(origin_main)" != "$(head_of)" ]; then fail 'origin main was not updated'
    elif [ -e "$w/M2.txt" ] || [ -e "$w/M3.txt" ] || [ ! -e "$w/M1.txt" ]; then fail 'wrong tree after reverts'
    elif [ "$(cat "$w/log.txt")" != "$(printf 'base\nM1')" ]; then fail 'log.txt not back to M1 state'
    elif ! has 'Merge lane M2' || ! has 'M2 broke the kernel repro' || ! has '/logs/red-gate.log'; then
        fail 'bead note lacks subject / reason / log path'
        sed 's/^/    | /' "$d/out" >&2
    else
        ok
        # 7b. the merge is now reverted: asking again is refused.
        case_name=revert-two-again
        h=$(head_of)
        if run 2 "$s" -y && has 'already reverted'; then
            [ "$(head_of)" = "$h" ] && ok || fail 'HEAD moved'
        else
            has 'already reverted' || fail 'second revert of the same merge not refused'
        fi
    fi
fi

# 8. smoke red: no push, non-zero exit, origin unchanged, says what to do.
mkfixture smoke-red
s=$(msha M2); oh=$(origin_main)
RS='make -s fake-red'
if run 3 "$s" -y -r r -l l; then
    if [ "$(origin_main)" != "$oh" ]; then fail 'origin moved although smoke was red'
    elif ! has 'NOT pushed'; then fail 'no "NOT pushed" message'
    elif ! has 'git reset --hard'; then fail 'no recovery instructions'
    else ok; fi
fi

# 9. target is the newest merge: no -y needed, exactly one revert, pushed.
mkfixture revert-newest
s=$(msha M3); m2=$(msha M2)
RS='make -s fake-green'
if run 0 "$s"; then
    if [ "$(git -C "$w" rev-list --count "$m2..HEAD" --first-parent)" != 2 ] \
       || [ "$(git -C "$w" log -1 --format=%s)" != 'Revert "Merge lane M3"' ]; then
        fail 'expected merge M3 plus exactly one revert above M2'
    elif [ "$(origin_main)" != "$(head_of)" ]; then fail 'origin not updated'
    else ok; fi
fi

# 10. default smoke selection (SMOKE unset).
#   a) no gates-for, ordinary diff        -> the repro trio
#   b) no gates-for, samir-only diff      -> test-merge-samir
#   c) gates-for target exists            -> gates-for DIFF=<merge>^1..<merge>
mkfixture smoke-trio
s=$(msha M3)
if run 0 "$s"; then
    if [ "$(tr '\n' ' ' < "$mark")" = 'test-kernel-repro test-kernel-flair-repro test-ovl-repro ' ]; then ok
    else fail "trio not selected: $(cat "$mark")"; fi
fi

mkfixture smoke-samir
addmerge 4 os/samir/m4.c nolog
git -C "$w" push -q origin main
s=$(msha M4)
if run 0 "$s"; then
    if [ "$(cat "$mark")" = 'test-merge-samir' ]; then ok
    else fail "samir-only diff did not select test-merge-samir: $(cat "$mark")"; fi
fi

mkfixture smoke-gates-for
s=$(msha M3)
RG=1
if run 0 "$s"; then
    if [ "$(cat "$mark")" = "gates-for DIFF=$s^1..$s" ]; then ok
    else fail "gates-for not selected with the merge range: $(cat "$mark")"; fi
fi

# ----------------------------------------------------------------- verdict --
if [ "$bad" -ne 0 ]; then
    printf '!!! test-revert-merge: %s case failure(s)\n' "$bad" >&2
    exit 1
fi
printf '>>> test-revert-merge: green (refusals, plan, two-step revert, smoke gate, push gate, smoke selection)\n'
