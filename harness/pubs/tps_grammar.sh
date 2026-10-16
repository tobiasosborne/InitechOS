#!/bin/sh
# tps_grammar.sh -- the printed Turbo Initech grammar against the shipped
# parser, both directions (PUB-REF-0001 CP-18; bead initech-ipu5.12).
#   grammar => parser: harness/pubs/railroad.c derives N programs from the
#     printed rules (every alternative, option and repetition exercised);
#     the shipped parser (tps.pas, built by Free Pascal) must print
#     TPS-PARSE-OK for every one.
#   parser => grammar: the TPS-LEX dumps of real programs the parser accepts
#     (the corpus os/tps/fixtures/gen_*.pas, parse_rich.pas, tps.pas itself
#     and every program printed in the books) must be derived by the rules.
# Usage: tps_grammar.sh RAILROAD GRAMMAR TPS_BIN OUTDIR N PROGRAM.pas...
set -eu
rr=$1; grammar=$2; tps=$3; out=$4; n=$5; shift 5
root=$(pwd)
case $tps in /*) ;; *) tps=$root/$tps;; esac
rm -rf "$out"; mkdir -p "$out/sent" "$out/rec"
"$rr" sentences "$grammar" "$n" "$out/sent"
ok=0
for s in "$out"/sent/s*.pas; do
    d=$out/run; rm -rf "$d"; mkdir -p "$d"; cp "$s" "$d/TPSIN.PAS"
    (cd "$d" && "$tps" > out.txt) || true
    if ! grep -q '^TPS-PARSE-OK$' "$d/out.txt"; then
        grep 'TPS-LEX-ERROR\|TPS-PARSE-ERROR' "$d/out.txt" >&2 || true
        printf '!!! test-pubs RED GRAMMAR-PARSER: %s: the shipped parser rejects a program the printed grammar derives\n' "$s" >&2
        exit 1
    fi
    ok=$((ok + 1))
done
printf '>>> test-pubs-tps-grammar: %s generated programs accepted by the shipped parser (grammar => parser)\n' "$ok"
k=0
for p in "$@"; do
    k=$((k + 1)); d=$out/rec/$k; mkdir -p "$d"; cp "$p" "$d/TPSIN.PAS"
    (cd "$d" && "$tps" > lex.txt) || true
    grep -q '^TPS-PARSE-OK$' "$d/lex.txt" || { printf '!!! test-pubs RED GRAMMAR-INPUT: %s does not parse\n' "$p" >&2; exit 1; }
done
"$rr" recognize "$grammar" "$out"/rec/*/lex.txt
printf '>>> test-pubs-tps-grammar: green (%s accepted programs derived by the printed rules: parser => grammar)\n' "$k"
