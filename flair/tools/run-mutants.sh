#!/bin/sh
# Parallel exact selections; the shared runner owns patch/build/kill validation.
set -eu
top=$1 out=$2 make_cmd=$3 jobs=$4
shift 4
[ "$#" -gt 0 ] || { echo 'FAIL: no mutation patches selected' >&2; exit 1; }
case $jobs in ''|*[!0-9]*|0) echo 'FAIL: MUTANT_JOBS must be positive' >&2; exit 1;; esac
mkdir -p "$out/mutants"
run=$(mktemp -d "$out/mutants/parallel.XXXXXX")
start=$(date +%s)
export FLAIR_MUTANT_TOP="$top" FLAIR_MUTANT_OUT="$run" FLAIR_MUTANT_MAKE="$make_cmd"
printf '.PHONY: all\nall:\n' > "$run/groups.mk"
# Registered program names and repository patch paths contain no shell syntax.
for patch_file do
    case $patch_file in mutants/*[!a-zA-Z0-9_./-]*|*[!a-zA-Z0-9_./-]*)
        echo "FAIL: unsafe patch path: $patch_file" >&2; exit 1;; esac
done
# Read headers separately: optional test headers need not be the second line.
for patch_file do
    selection=$(sed -n 's/^# test: //p' "$patch_file")
    printf '%s\t%s\n' "$selection" "$patch_file"
done | awk -F '\t' '
{ if (!($1 in ids)) ids[$1] = ++n; id = ids[$1]; patches[id] = patches[id] " " $2 }
END {
    for (i = 1; i <= n; i++) {
        print "all: group-" i
        print ".PHONY: group-" i
        print "group-" i ":"
        print "\t@sh " ENVIRON["FLAIR_MUTANT_TOP"] "/factory/testlib/run-mutants.sh " \
              ENVIRON["FLAIR_MUTANT_TOP"] " flair " ENVIRON["FLAIR_MUTANT_OUT"] \
              " " ENVIRON["FLAIR_MUTANT_MAKE"] patches[i] " > " \
              ENVIRON["FLAIR_MUTANT_OUT"] "/group-" i ".log 2>&1"
    }
}' >> "$run/groups.mk"
rc=0
"$make_cmd" --no-print-directory -k -j"$jobs" -f "$run/groups.mk" || rc=$?
cat "$run"/group-*.log
awk -v elapsed="$(($(date +%s)-start))" '
/^flair: [0-9]+ mutants,/ { n += $2; killed += $4; survived += $6; errors += $8 }
END { printf "flair sweep: %d patches, %d killed, %d survived, %d errors; %ds wall\n", n, killed, survived, errors, elapsed }
' "$run"/group-*.log
if [ "$rc" -eq 0 ]; then rm -rf "$run"; else echo "sweep retained: $run"; fi
exit "$rc"
