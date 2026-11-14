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
        for n in 1 2 3; do printf 'Unrelated file %s\r\n' "$n" > "$disk/APPS/TABLE~$n.TMP"; done
    fi
    cp -R "$disk/APPS" "$tmp/expected/$scenario"
    if ! "$bin" "$scenario" "$disk" > "$tmp/out"; then fail=1; fi
    cat "$tmp/out"
    c=$(sed -n 's/^test_calc_save_safety: \([0-9]*\) checks, \([0-9]*\) failures.*/\1 \2/p' "$tmp/out")
    checks=$((checks + ${c%% *})); failed=$((failed + ${c##* }))
    case $scenario in
        stage-open-error|collision) cp "$tmp/edited" "$tmp/expected/$scenario/TABLES.TXT" ;;
    esac
    if ! diff -r "$tmp/expected/$scenario" "$disk/APPS"; then
        echo "FAIL save safety disk readback: $scenario"; fail=1; failed=$((failed + 1))
    else
        echo "ok independent disk readback: $scenario"
    fi
    checks=$((checks + 1))
done
echo "test_calc_save_safety: $checks checks, $failed failures (all scenarios)"
exit "$fail"
