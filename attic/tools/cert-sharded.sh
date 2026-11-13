#!/bin/bash
# Parallel certificate: docs/design/parallel-certificate.md Sec 3, initech-9bgg6.
# Laws 1/2: the make vector is the oracle; every red stays red, with both logs.
# Law 3: thin shell around unchanged C/make gates. Law 4: no fidelity changes.
set -euo pipefail
export LC_ALL=C
# A parent make's jobserver/options must not parallelise inside a worktree.
unset MAKEFLAGS MFLAGS MAKELEVEL MAKEOVERRIDES GNUMAKEFLAGS
jobs=8 ref=HEAD out=build/cert only= gates= diff= resume=0 keep=0
die() { echo "cert-sharded: $*" >&2; exit 2; }
usage() {
    echo 'Usage: cert-sharded.sh [-j N] [-r REF] [--out DIR] [--only unit|emu]'
    echo '       [--resume] [--keep] [--gates FILE] [--diff OTHER_RESULTS]'
}
while (($#)); do
    case $1 in
        -j|-r|--out|--only|--gates|--diff)
            (($# >= 2)) || die "missing value for $1"
            case $1 in -j) jobs=$2;; -r) ref=$2;; --out) out=$2;; --only) only=$2;; --gates) gates=$2;; --diff) diff=$2;; esac
            shift 2;;
        --resume) resume=1; shift;; --keep) keep=1; shift;;
        -h|--help) usage; exit 0;; *) die "unknown option: $1";;
    esac
done
[[ $jobs =~ ^[1-9][0-9]*$ ]] || die '-j must be a positive integer'
[[ -z $only || $only == unit || $only == emu ]] || die '--only must be unit or emu'
if [[ -n $diff ]]; then
    [[ -f $diff && -f $out/results.txt ]] || die 'diff requires two results.txt files'
    # Last row wins: initial FAIL becomes FAIL-CONFIRMED or FLAKY. Also show
    # added/removed gates; durations and worker assignments are immaterial.
    awk '
        $1 ~ /^(PASS|FAIL|FAIL-CONFIRMED|FLAKY)$/ {
            k=$2 SUBSEP $3; if (!(k in seen)) {seen[k]=1; order[++n]=k; label[k]=$2 " " $3}
            if (phase == "old") old[k]=$1; else now[k]=$1
        }
        END {for (i=1;i<=n;i++) {k=order[i]; if (old[k]!=now[k]) {
            print label[k], (old[k] ? old[k] : "MISSING") " -> " (now[k] ? now[k] : "MISSING"); bad=1
        }} exit bad}
    ' phase=old "$diff" phase=new "$out/results.txt"
    exit $?
fi
repo=$(git rev-parse --show-toplevel)
commit=$(git rev-parse --verify "$ref^{commit}")
[[ -z $gates ]] || gates=$(cd "$(dirname "$gates")" && pwd)/$(basename "$gates")
mkdir -p "$out"
out=$(cd "$out" && pwd)
exec 6> "$out/run.lock"
flock -n 6 || die "another runner owns $out"
[[ $resume == 1 || ! -e $out/run.meta ]] || die 'output already used; use --resume or another --out'
started=$(date +%s)
trees=() pids=()
cleanup() {
    local rc=$?
    trap - EXIT INT TERM
    for pid in "${pids[@]}"; do kill -- "-$pid" 2>/dev/null || :; done
    for pid in "${pids[@]}"; do wait "$pid" 2>/dev/null || :; done
    if [[ $keep == 0 ]]; then
        for tree in "${trees[@]}"; do git -C "$repo" worktree remove --force "$tree" || rc=2; done
    fi
    exit "$rc"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
# Give each background warm/worker its own process group for signal cleanup.
set -m
mkdir -p "$out/logs"
for ((i=1;i<=jobs;i++)); do
    tree=$out/w$i
    if [[ -e $tree ]]; then
        [[ $resume == 1 ]] && git -C "$repo" worktree list --porcelain | grep -Fxq "worktree $tree" || die "worktree already exists: $tree"
        [[ $(git -C "$tree" rev-parse HEAD) == "$commit" ]] || die "wrong HEAD in $tree"
    else
        git -C "$repo" worktree add --quiet --detach "$tree" "$commit"
    fi
    trees+=("$tree")
done
# Preserve even the hard-coded ../corpus paths used by C oracles and pubs.
for sibling in "$(dirname "$repo")"/*-decomp "$(dirname "$repo")/initech-manuals-ref"; do
    [[ -d $sibling ]] || continue
    link=$out/$(basename "$sibling")
    if [[ -e $link || -L $link ]]; then
        [[ $link -ef $sibling ]] || die "corpus link collision: $link"
    else ln -s "$sibling" "$link"; fi
done
cd "$out/w1"
make -pn > "$out/make-db.txt" 2> "$out/make-db.err"
make --no-print-directory -s --eval='.PHONY: cert-runner-count' \
    --eval='cert-runner-count: ; @printf "%s %s\n" "$(words $(TEST_UNIT_GATES))" "$(words $(TEST_EMU_GATES))"' \
    cert-runner-count > "$out/counts.txt" 2>> "$out/make-db.err"
awk '
    /^TEST_(UNIT|EMU)_GATES :?=/ {
        if (got[$1]++) next
        kind=($1=="TEST_UNIT_GATES" ? "unit" : "emu")
        for (i=3;i<=NF;i++) {if ($i !~ /^[A-Za-z0-9_][A-Za-z0-9_.-]*$/) exit 2; list[kind,++n[kind]]=$i}
    }
    END {split("unit emu",k); for (j=1;j<=2;j++) for(i=1;i<=n[k[j]];i++) {
        g=list[k[j],i]; if (!seen[g]++) print k[j],g
    }}
' "$out/make-db.txt" > "$out/vector.txt"
read -r host emu < "$out/counts.txt"
read -r actual_host actual_emu < <(awk '{n[$1]++} END {print n["unit"]+0,n["emu"]+0}' "$out/vector.txt")
[[ $host == "$actual_host" && $emu == "$actual_emu" ]] || die "gate count mismatch: banner $host/$emu, deduplicated $actual_host/$actual_emu"
echo "Gate vector: $host host + $emu emu (deduplicated; make banner counts match)"
awk -v only="$only" 'only=="" || $1==only' "$out/vector.txt" > "$out/selected.txt"
if [[ -n $gates ]]; then
    awk 'NR==FNR {if(NF) want[$1]=1; next} $2 in want {print; delete want[$2]} END {for(g in want) {print "unknown/excluded gate: " g > "/dev/stderr"; bad=1} exit bad}' \
        "$gates" "$out/selected.txt" > "$out/subset.txt" || die 'invalid --gates selection'
    mv -f "$out/subset.txt" "$out/selected.txt"
fi
[[ -s $out/selected.txt ]] || die 'empty gate selection'
{ printf '%s %s\n' "$commit" "$jobs"; cat "$out/selected.txt"; } > "$out/run.next"
if [[ $resume == 1 ]]; then
    cmp -s "$out/run.meta" "$out/run.next" || die 'resume requires the same commit, workers and gate selection'
else
    mv -f "$out/run.next" "$out/run.meta"
    : > "$out/results.txt"
fi
# Bochs is always last, including non-emulator host tests named *bochs*.
awk 'FILENAME==ARGV[1] {done[$3]=1; next} !($2 in done) {if($2 ~ /bochs/) b[++n]=$0; else print} END {for(i=1;i<=n;i++) print b[i]}' \
    "$out/results.txt" "$out/selected.txt" > "$out/queue.txt"
cd "$repo"
for tree in "${trees[@]}"; do
    (cd "$tree"; make --no-print-directory clean && make --no-print-directory factory image) > "$out/logs/$(basename "$tree").warm.log" 2>&1 &
    pids+=("$!")
done
warm_bad=0
for pid in "${pids[@]}"; do wait "$pid" || warm_bad=1; done
pids=()
[[ $warm_bad == 0 ]] || die "warm build failed; see $out/logs/w*.warm.log"
run_gate() {
    local kind=$1 gate=$2 worker=$3 suffix=$4 state start seconds
    start=$(date +%s); state=PASS
    (cd "$out/$worker"; INITECH_QMP_STDIO=1 make --no-print-directory "$gate" 8>&- 9>&-) > "$out/logs/$gate$suffix.log" 2>&1 || state=FAIL
    seconds=$(( $(date +%s)-start ))
    if [[ -n $suffix ]]; then
        if [[ $state == PASS ]]; then state=FLAKY; else state=FAIL-CONFIRMED; fi
    fi
    flock 9
    printf '%s %s %s %s %s\n' "$state" "$kind" "$gate" "$seconds" "$worker" | tee -a "$out/results.txt"
    flock -u 9
}
worker() {
    local kind gate row
    exec 9> "$out/queue.lock" 8> "$out/bochs.lock"
    while :; do
        flock 9
        if ! read -r kind gate < "$out/queue.txt"; then flock -u 9; break; fi
        if [[ $gate == *bochs* ]]; then
            # Reserve locally, probe the host lock, then RELEASE the probe.
            # The harness takes that lock itself; holding it would deadlock.
            if ! flock -n 8 || ! (exec 7>/tmp/initech-bochs.lock; flock -n 7); then
                flock -u 8
                row=$(awk '$2 !~ /bochs/ {print; exit}' "$out/queue.txt")
                if [[ -z $row ]]; then flock -u 9; sleep 0.1; continue; fi
                read -r kind gate <<< "$row"
            fi
        fi
        awk -v gate="$gate" '$2 != gate' "$out/queue.txt" > "$out/queue.next"
        mv -f "$out/queue.next" "$out/queue.txt"
        flock -u 9
        run_gate "$kind" "$gate" "$1" ''
        flock -u 8
    done
}
touch "$out/workers.ready"
for ((i=1;i<=jobs;i++)); do worker "w$i" & pids+=("$!"); done
worker_bad=0
for pid in "${pids[@]}"; do wait "$pid" || worker_bad=1; done
pids=()
[[ $worker_bad == 0 ]] || die 'worker failed; resume to finish unrecorded gates'
# All workers are now idle. Re-run each unclassified FAIL once, serially.
awk '{state[$3]=$1; row[$3]=$0} END {for(g in state) if(state[g]=="FAIL") print row[g]}' "$out/results.txt" > "$out/solo.txt"
exec 9> "$out/queue.lock"
while read -r state kind gate seconds worker; do run_gate "$kind" "$gate" "$worker" .solo; done < "$out/solo.txt"
awk 'FILENAME==ARGV[1] {want[$2]=1; next} {if(!($3 in want)) exit 1; if($1=="PASS" || $1=="FAIL") n[$3]++} END {for(g in want) if(n[g]!=1) exit 1}' \
    "$out/selected.txt" "$out/results.txt" || die 'incomplete or duplicate gate coverage'
read -r host emu < <(awk '{n[$1]++} END {print n["unit"]+0,n["emu"]+0}' "$out/selected.txt")
elapsed=$(( $(date +%s)-started ))
echo '=== certificate verdict ==='
awk '$1=="PASS" || $1=="FAIL" {n[$5]++} END {for(w in n) print "Worker " w ": " n[w] " gates"}' "$out/results.txt" | sort
echo "Wall clock: $elapsed seconds"
if awk '$1!="PASS" {bad=1} END {exit bad}' "$out/results.txt"; then
    awk -v h="$host" -v e="$emu" -v n="$jobs" -v t="$elapsed" 'BEGIN {printf "ALL GREEN -- %s host + %s emu gates, %s workers, %.2f min\n",h,e,n,t/60}'
else
    echo "RED -- $host host + $emu emu gates, $jobs workers"
    awk -v out="$out" '$1=="FAIL-CONFIRMED" || $1=="FLAKY" {print $1 " " $2 " " $3 " " out "/logs/" $3 ".log " out "/logs/" $3 ".solo.log"}' "$out/results.txt"
    exit 1
fi
