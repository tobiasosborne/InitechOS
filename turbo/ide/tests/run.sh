#!/bin/sh
# Tier-0 entries for turbo/ide (README.md). $1 = OUT, $2 = entry.
# Setup problems exit 2 (an error, never a kill); each check prints PASS or
# FAIL and the run ends with "N checks, M failures" (factory/testlib).
set -u
out=$1 entry=$2
top=${TOP:?}
corpus=${TURBOPASCAL_DECOMP:?}
mint=$corpus/goldens/minted/MINT06
scratch=$out/runs/$entry
rm -rf "$scratch"
mkdir -p "$scratch" || exit 2
checks=0 failures=0
setup() { "$@" || { echo "setup failed: $*"; exit 2; }; }
check() {
    name=$1; shift
    checks=$((checks+1)); code=0
    if [ "$1" = sh ] && [ "$2" != -c ]; then
        # Producer scripts must supply their own completed assertions. A
        # compiler/tool/setup failure cannot be relabelled by this adapter.
        sh "$top/turbo/tests/entry.sh" "$2" "$3" "$name" "$out/$name.producer.log" || code=$?
        cat "$out/$name.producer.log"
    else
        "$@" || code=$?
    fi
    case $code in
    0) echo "PASS $name";;
    1) echo "FAIL $name"; failures=$((failures+1));;
    *) echo "ERROR $name (exit $code)" >&2; exit 2;;
    esac
}
file_bytes() {
    label=$1 expected=$2 actual=$3
    if [ -f "$actual" ]; then check "$label" cmp "$expected" "$actual"
    else check "$label:output exists" test -f "$actual"; fi
}

case $entry in
unnamed)
    check unnamed sh tests/unnamed.sh "$out"
    ;;
tab-width)
    check tab-width sh tests/tab-width.sh "$out"
    ;;
save-path)
    check save-path sh tests/save-path.sh "$out"
    ;;
first-save)
    check first-save sh tests/first-save.sh "$out"
    ;;
save-identity)
    check save-identity sh tests/save-identity.sh "$out"
    ;;
save-failure)
    check save-failure sh tests/save-failure.sh "$out"
    ;;
page-failure)
    check page-failure sh tests/page-failure.sh "$out"
    ;;
journey)
    # The IDE built by Free Pascal replays the journey; every screen is
    # compared with the TP 5.5 capture it names, and the saved files with
    # the bytes TP 5.5 saved in the same journey.
    setup sh -c "cd '$mint' && sha256sum --quiet -c SHA256SUMS"
    setup cp "$out/idehost" "$scratch/"
    setup cp "$corpus/mint/src/MINT06/LONG.PAS" "$scratch/LONG.PAS"
    setup cp "$corpus/mint/src/MINT06/RO.PAS" "$scratch/RO.PAS"
    setup chmod 444 "$scratch/RO.PAS"
    setup "$out/keys" tests/journey.steps "$scratch/IDEKEYS.BIN" "$scratch/shots.list"
    (cd "$scratch" && ./idehost > stdout)
    check completed grep -qx "IDE-HOST-END shots=$(wc -l < "$scratch/shots.list")" "$scratch/stdout"
    check screens "$out/screens" "$mint" "$scratch" tests/journey.expect
    file_bytes saved-source "$mint/disk/DEMO.PAS" "$scratch/DEMO.PAS"
    file_bytes saved-backup "$mint/disk/DEMO.BAK" "$scratch/DEMO.BAK"
    file_bytes unsaved-file-unchanged "$corpus/mint/src/MINT06/LONG.PAS" "$scratch/LONG.PAS"
    file_bytes read-only-untouched "$corpus/mint/src/MINT06/RO.PAS" "$scratch/RO.PAS"
    check refused-saves-leave-nothing sh -c "! ls '$scratch' | grep -q -E '^(RO[.]BAK|RO[.][$]{3}|DEMO[.][$]{3}|X[.]PAS|.*NODIR.*)\$'"
    ;;
paging)
    # A file far larger than the 16 KiB page: edits at the start, line 2500
    # and the end must survive paging; awk computes the expected bytes.
    setup cp "$out/idehost" "$scratch/"
    awk 'BEGIN { for (i = 1; i <= 5000; i++) printf "line %d of the paging fixture, tab\there\r\n", i }' > "$scratch/BIG.PAS"
    setup cp "$scratch/BIG.PAS" "$scratch/original"
    setup "$out/keys" tests/paging.steps "$scratch/IDEKEYS.BIN" "$scratch/shots.list"
    (cd "$scratch" && ./idehost > stdout)
    awk 'BEGIN { RS = "\r\n" }
         NR == 1 { printf "TOP %s\r\n", $0; next }
         NR == 2500 { printf "%sMID %s\r\n", substr($0, 1, 4), substr($0, 5); next }
         NR == 2501 { next }
         { printf "%s\r\n", $0 }
         END { printf "END\032" }' "$scratch/original" > "$scratch/expected"
    file_bytes saved-pages "$scratch/expected" "$scratch/BIG.PAS"
    file_bytes backup "$scratch/original" "$scratch/BIG.BAK"
    ;;
one-program)
    # One TURBO.COM holds the compiler and the IDE: the compiler's
    # declarations, the IDE's, one main. Both compilers must accept that
    # program within their capacities (turbo/README.md, limits).
    main=$(grep -n '^begin$' "$top/turbo/compiler/turbo.pas" | tail -1 | cut -d: -f1)
    head -n $((main - 1)) "$top/turbo/compiler/turbo.pas" > "$scratch/ONE.PAS"
    cat screen.pas buffer.pas editor.pas >> "$scratch/ONE.PAS"
    printf 'begin\n  IdeStart()\nend.\n' >> "$scratch/ONE.PAS"
    check seed-capacity "$top/build/turbo/seed/initechc" --emit-asm -o "$scratch/one.s" "$scratch/ONE.PAS"
    setup cp "$scratch/ONE.PAS" "$scratch/TURBOIN.PAS"
    (cd "$scratch" && "$top/build/turbo/compiler/turbo_fpc" /DUMP > raw)
    grep -a -E 'TURBO-[A-Z]+-(OK|ERROR)' "$scratch/raw" | head -5
    check turbo-capacity grep -q '^TURBO-NATIVE-OK ' "$scratch/raw"
    ;;
seed)
    # The C seed accepts the IDE source (subset check, no execution).
    check seed-accepts "$top/build/turbo/seed/initechc" --emit-asm -o "$scratch/idehost.s" "$out/idehost.pas"
    ;;
turbo)
    # Turbo Initech itself (the FPC-hosted compiler) compiles the IDE.
    setup cp "$out/idehost.pas" "$scratch/TURBOIN.PAS"
    (cd "$scratch" && "$top/build/turbo/compiler/turbo_fpc" /DUMP > raw)
    check native-ok grep -q '^TURBO-NATIVE-OK ' "$scratch/raw"
    check no-error sh -c "! grep -a -E 'TURBO-.*ERROR' '$scratch/raw'"
    check container test -s "$scratch/TURBOOUT.EXE"
    ;;
*) echo "unknown entry $entry" >&2; exit 2 ;;
esac
echo "$checks checks, $failures failures"
test "$failures" -eq 0
