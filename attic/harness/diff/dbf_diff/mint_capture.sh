#!/bin/sh
# Shared reference-only DOSBox capture and exact ordered error validation.
# Ref: PRD 6.6; fns_mint.sh's real III+ 1.1 capture contract.
set -eu
work=$1
input=$2
end=$3
case "$work" in /*) ;; *) echo 'RED: MINT requires absolute work directory'; exit 2;; esac
cat > "$work/mint.conf" <<CONF
[sdl]
output=surface
[dosbox]
machine=svga_s3
memsize=8
[cpu]
core=auto
cputype=386
cycles=max
[printer]
docpath=$work
CONF
if [ "${MINT_PRINTER_CAPTURE:-0}" = 1 ]; then
    # DOSBox-X reference.full.conf [parallel]: file append records raw LPT.
    printf '[parallel]\nparallel1=file append:%s/PRINTER.OUT timeout:1\n' "$work" >> "$work/mint.conf"
fi
printf '[autoexec]\n' >> "$work/mint.conf"
if [ -n "${MINT_FREESIZE:-}" ]; then
    printf 'mount c "%s" -freesize %s\n' "$work" "$MINT_FREESIZE" >> "$work/mint.conf"
else
    printf 'mount c "%s"\n' "$work" >> "$work/mint.conf"
fi
if [ -d "$work/DRIVE" ]; then printf 'mount d "%s/DRIVE" -freesize 64\n' "$work" >> "$work/mint.conf"; fi
printf 'c:\nDATE 07/24/1986\nDBASE MINT\nexit\n' >> "$work/mint.conf"
rm -f "$work/GOLD.OUT"
if ! SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout --foreground -k 2 "${MINT_TIMEOUT:-90}" dosbox-x -defaultconf -fastlaunch -nopromptfolder -conf "$work/mint.conf" -exit > "$work/mint.log" 2>&1; then
    echo "RED: MINT reference process failed/timed out: $work"; exit 1
fi
test -s "$work/GOLD.OUT" || { echo "RED: MINT impossible: $work has no capture"; exit 1; }
grep -Fq "$end" "$work/GOLD.OUT" || { echo "RED: MINT incomplete: $work"; cat "$work/GOLD.OUT"; exit 1; }
test "$(tail -c 1 "$work/GOLD.OUT" | od -An -tu1 | tr -d ' ')" = 26 || { echo "RED: MINT unflushed: $work"; exit 1; }
expected=$(sed -n 's/^\* MINT-ERRORS: *//p' "$input" | tr -d '\r' | awk '{$1=$1;print}')
actual=$(awk '/MINT-ERROR/ { if ($1 != "MINT-ERROR" || $2 !~ /^[0-9]+$/) {print "MALFORMED";exit} printf "%s%s",sep,$2;sep=" " }' "$work/GOLD.OUT")
if [ "${MINT_PROBE:-0}" = 1 ]; then
    printf 'PROBE %s: errors [%s]; not promoted\n' "$input" "$actual"
    exit 0
fi
if [ "$actual" != "$expected" ]; then
    printf 'RED: MINT unexpected errors: %s (expected [%s], got [%s])\n' "$input" "$expected" "$actual"
    cat "$work/GOLD.OUT";exit 1
fi
