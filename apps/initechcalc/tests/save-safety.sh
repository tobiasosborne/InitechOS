#!/bin/sh
# Independent disk reader: POSIX diff inspects files exported by sdk/host.
# Expected bytes come from pre-operation fixtures and a literal edited ledger
# (the shipped data/LEDGER.TXT with rows inserted by awk), never from Calc's
# serializer or FILE COPY's transfer code.
set -eu
bin=$1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
# insert_after_rows FILE LINES...: the ledger with LINES (CR LF added) after ROWS.
insert_after_rows() {
    file=$1; shift
    awk -v add="$*" 'BEGIN { n=split(add,a,"|") } { print } index($0,"ROWS\t")==1 { for(i=1;i<=n;i++) printf "%s\r\n", a[i] }' "$file"
}
# The previous save: a readable ledger with a sentinel row the user typed.
insert_after_rows data/LEDGER.TXT "2	'Keep under TABLES.TXT" > "$tmp/previous"
insert_after_rows data/LEDGER.TXT "1	(G) 73|2	'Keep under TABLES.TXT" > "$tmp/edited"
fail=0 checks=0 failed=0
for scenario in stage-open-error collision all-collisions create-error write-error write-zero write-short stage-close-error destination-open-error destination-close-error copy-rejected copy-error copy-cancel replace-no; do
    disk=$tmp/$scenario
    mkdir -p "$disk/APPS" "$tmp/expected"
    cp data/LEDGER.TXT "$disk/APPS/LEDGER.TXT"
    cp "$tmp/previous" "$disk/APPS/TABLES.TXT"
    case $scenario in
        stage-open-error|collision|all-collisions)
            printf 'Unrelated user file\000\377\r\n' > "$disk/APPS/TABLES.TMP" ;;
    esac
    if [ "$scenario" = all-collisions ]; then
        for n in 1 2 3 4 5 6 7 8 9; do printf 'Unrelated file %s\r\n' "$n" > "$disk/APPS/TABLE~$n.TMP"; done
    fi
    cp -R "$disk/APPS" "$tmp/expected/$scenario"
    if ! "$bin" "$scenario" "$disk" > "$tmp/out"; then fail=1; fi
    cat "$tmp/out"
    c=$(sed -n 's/^test_calc_save_safety: \([0-9]*\) checks, \([0-9]*\) failures.*/\1 \2/p' "$tmp/out")
    checks=$((checks + ${c%% *})); failed=$((failed + ${c##* }))
    case $scenario in
        stage-open-error|collision|all-collisions) cp "$tmp/edited" "$tmp/expected/$scenario/TABLES.TXT" ;;
    esac
    if ! diff -r "$tmp/expected/$scenario" "$disk/APPS"; then
        echo "FAIL save safety disk readback: $scenario"; fail=1; failed=$((failed + 1))
    else
        echo "ok independent disk readback: $scenario"
    fi
    checks=$((checks + 1))
done
# One run of the binary on DISK, its checks added to the totals.
run() {
    if ! "$bin" "$1" "$2" > "$tmp/out"; then fail=1; fi
    cat "$tmp/out"
    c=$(sed -n 's/^test_calc_save_safety: \([0-9]*\) checks, \([0-9]*\) failures.*/\1 \2/p' "$tmp/out")
    checks=$((checks + ${c%% *})); failed=$((failed + ${c##* }))
}
readback() {
    checks=$((checks + 1))
    if ! diff -r "$1" "$2"; then
        echo "FAIL save safety disk readback: $3"; fail=1; failed=$((failed + 1))
    else
        echo "ok independent disk readback: $3"
    fi
}
fresh() {
    mkdir -p "$1/APPS"
    cp data/LEDGER.TXT "$1/APPS/LEDGER.TXT"
    cp "$tmp/previous" "$1/APPS/TABLES.TXT"
}
# initech-zev40: a cancelled copy's stage that DOS would not delete, then
# replaced by the user with an unrelated file, survives the next Save.
fresh "$tmp/cleanup-replaced"
run cleanup-replaced "$tmp/cleanup-replaced"
mkdir -p "$tmp/expected/cleanup-replaced"
cp data/LEDGER.TXT "$tmp/edited" "$tmp/expected/cleanup-replaced/"
mv "$tmp/expected/cleanup-replaced/edited" "$tmp/expected/cleanup-replaced/TABLES.TXT"
printf 'Unrelated user file\000\377\r\n' > "$tmp/expected/cleanup-replaced/TABLES.TMP"
readback "$tmp/expected/cleanup-replaced" "$tmp/cleanup-replaced/APPS" cleanup-replaced
# initech-40kpb: eleven sessions each Quit > Save, each stage's DELETE
# refused; a twelfth session without the fault still saves. The eleven
# stages are left to the user as they were written (each the saved 73
# ledger), under the names DOS order gives them.
fresh "$tmp/restart"
for session in 1 2 3 4 5 6 7 8 9 10 11; do run cleanup-refused "$tmp/restart"; done
run after-leftovers "$tmp/restart"
mkdir -p "$tmp/expected/restart"
cp data/LEDGER.TXT "$tmp/expected/restart/LEDGER.TXT"
insert_after_rows data/LEDGER.TXT "1	(G) 74|2	'Keep under TABLES.TXT" > "$tmp/expected/restart/TABLES.TXT"
for name in TABLES TABLE~1 TABLE~2 TABLE~3 TABLE~4 TABLE~5 TABLE~6 TABLE~7 TABLE~8 TABLE~9 TABL~10; do cp "$tmp/edited" "$tmp/expected/restart/$name.TMP"; done
readback "$tmp/expected/restart" "$tmp/restart/APPS" restart-after-leftovers
echo "test_calc_save_safety: $checks checks, $failed failures (all scenarios)"
exit "$fail"
