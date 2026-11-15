#!/bin/sh
# Existing RUN.BAT protocol: each end-Pnnn is printed after that case exits.
# W consumes those complete lines in order; dump bytes are not progress.
set -eu
base=$1 cases=$2
base_keys=$(cat "$base") || exit 2
printf '%s' "$base_keys"
n=0
while read -r id name path extra; do
    n=$((n+1)); wanted=$(printf 'P%03d' "$n")
    if [ "$id" != "$wanted" ] || [ -z "$name" ] || [ -n "$extra" ]; then
        echo 'Invalid execution case order' >&2; exit 1
    fi
    case $path in seed|hosted|hosted-flat|resident|resident-flat) ;;
        *) echo 'Invalid execution path' >&2; exit 1;; esac
    printf ',Wend-%s' "$id"
done < "$cases"
[ "$n" -gt 0 ] || { echo 'Empty execution cases' >&2; exit 1; }
printf ',Wturbo-execution-complete\n'
