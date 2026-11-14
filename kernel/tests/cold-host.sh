#!/bin/sh
# Explicit build regression, outside tier 0: independent empty worktrees at
# both job counts. Never clean or reuse the caller's product build directory.
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
revision=$(git -C "$root" rev-parse "${1:-HEAD}^{commit}")
mkdir -p "$root/build/kernel/cold-host"
out=$(mktemp -d "$root/build/kernel/cold-host/run.XXXXXX")
printf 'Cold host evidence: %s\nSource: %s\n' "$out" "$revision"
for jobs in 12 1; do
    source=$out/j$jobs
    git -C "$root" worktree add --quiet --detach "$source" "$revision"
    test ! -e "$source/build"
    if (unset MAKEFLAGS MFLAGS MAKEOVERRIDES TESTS;
        nice -n 15 make -C "$source/kernel" -j"$jobs" test) > "$out/j$jobs.log" 2>&1; then
        tail -3 "$out/j$jobs.log"
        printf 'GREEN empty kernel build -j%s\n' "$jobs"
    else
        tail -20 "$out/j$jobs.log"
        printf 'FAIL empty kernel build -j%s (%s)\n' "$jobs" "$out/j$jobs.log" >&2
        exit 1
    fi
done
