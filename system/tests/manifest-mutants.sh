#!/bin/sh
# Patch only isolated manifest copies; product sources are never changed.
# Every patch is graded: strict apply, then the inventory check must fail on
# the inventory signature. Prints the shared factory summary line.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
start=$(date +%s)
[ "$#" -gt 0 ] || set -- mutants/*.patch
[ -f "$1" ] || { echo 'FAIL: no mutation patches selected' >&2; exit 1; }
n=0 killed=0 survived=0 errors=0
if ! { sh tests/manifest.sh && sh tests/data-volume.sh; } > "$work/baseline" 2>&1; then
    cat "$work/baseline"; echo '!!! BASELINE FAILED'
    errors=$#; n=$#
else
    for fault do
        n=$((n+1))
        rm -rf "$work/copy"; mkdir "$work/copy"
        cp working-volume.list data-volume.list install-data.sh "$work/copy/"
        if ! patch --batch --fuzz=0 -s -d "$work/copy" -p1 < "$fault" > "$work/patch.log" 2>&1; then
            echo "!!! PATCH FAILED: $fault"; errors=$((errors+1)); continue
        fi
        result=kill
        for repeat in 1 2; do
            rc=0
            if grep -q '^# test: data-volume$' "$fault"; then
                sh tests/data-volume.sh "$work/copy/install-data.sh" "$work/copy/data-volume.list" > "$work/verdict" 2>&1 || rc=$?
                signature='^FAIL data volume:'
            else
                sh tests/manifest.sh "$work/copy/working-volume.list" > "$work/verdict" 2>&1 || rc=$?
                signature='^FAIL installed inventory:'
            fi
            if test "$rc" = 0; then result=survive; break
            elif test "$rc" != 1 || ! grep -q "$signature" "$work/verdict"; then result=error; break
            fi
        done
        case $result in
         kill) echo "  killed twice $fault: $(sed -n '/^FAIL /p' "$work/verdict")"; killed=$((killed+1));;
         survive) echo "!!! SURVIVED: $fault"; survived=$((survived+1));;
         error) echo "!!! WRONG FAILURE (not a kill): $fault: $(cat "$work/verdict")"; errors=$((errors+1));;
        esac
    done
fi
echo "system: $n mutants, $killed killed, $survived survived, $errors errors; $(($(date +%s)-start))s"
[ "$survived" -eq 0 ] && [ "$errors" -eq 0 ]
