#!/bin/sh
# Independent disk reader: POSIX diff inspects files exported by sdk/host.
# Expected bytes come from pre-operation fixtures and a literal edited ledger,
# never from Calc's serializer or FILE COPY's transfer code.
set -eu
bin=$1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
fail=0
for scenario in stage-open-error collision all-collisions create-error write-error write-zero write-short stage-close-error destination-open-error destination-close-error copy-rejected copy-error copy-cancel replace-no; do
    disk=$tmp/$scenario
    mkdir -p "$disk/APPS" "$tmp/expected"
    cp data/LEDGER.TXT "$disk/APPS/LEDGER.TXT"
    printf 'Previous saved ledger\r\nKeep under TABLES.TXT\r\n' > "$disk/APPS/TABLES.TXT"
    case $scenario in
        stage-open-error|collision|all-collisions)
            printf 'Unrelated user file\000\377\r\n' > "$disk/APPS/TABLES.TMP" ;;
    esac
    if [ "$scenario" = all-collisions ]; then
        for n in 1 2 3; do printf 'Unrelated file %s\r\n' "$n" > "$disk/APPS/TABLE~$n.TMP"; done
    fi
    cp -R "$disk/APPS" "$tmp/expected/$scenario"
    if ! "$bin" "$scenario" "$disk"; then fail=1; fi
    case $scenario in
        stage-open-error|collision)
            printf 'INITECH LEDGER\r\n73               TODO_FRAME\r\nMatte Cutters    TODO_FRAME\r\nExacto Blades    TODO_FRAME\r\n570- format      570-\r\nTOTAL            $112,276.87\r\n' > "$tmp/expected/$scenario/TABLES.TXT" ;;
    esac
    if ! diff -r "$tmp/expected/$scenario" "$disk/APPS"; then
        echo "FAIL save safety disk readback: $scenario" >&2; fail=1
    else
        echo "ok independent disk readback: $scenario"
    fi
done
exit "$fail"
