#!/bin/sh
# Real III+ samples are the oracle; no generated substitute. DEC-03a.10.
set -eu
export LC_ALL=C
ref=${DBASE3_DECOMP:?absolute DBASE3_DECOMP required}
case $ref in /*) ;; *) echo 'RED: DBASE3_DECOMP must be absolute'; exit 2;; esac
out=build/w4-files
mkdir -p "$out"
samples=$ref/goldens/dbase-iii-plus-1.1-pristine/files
real=$samples/Sample_Programs_and_Utilities
red() {
    kind=$1; file=$2; offset=$3; reason=$4
    build/w4_mutate --xor "$offset" "$file" "$out/$kind.mut"
    rc=0; "build/${kind}_diff" "$file" "$out/$kind.mut" > "$out/$kind-$reason.log" 2>&1 || rc=$?
    [ "$rc" = 1 ] && grep -q "RED: ${kind}_diff" "$out/$kind-$reason.log" || {
        cat "$out/$kind-$reason.log"; echo "RED: $kind $reason mutant survived / infrastructure failed"; exit 1;
    }
    printf 'RED proven: %s %s @%s: ' "$kind" "$reason" "$offset"
    head -1 "$out/$kind-$reason.log"
}
green_pad() {
    kind=$1; file=$2; offset=$3
    build/w4_mutate --xor "$offset" "$file" "$out/$kind.pad"
    "build/${kind}_diff" "$file" "$out/$kind.pad"
    printf 'GREEN proven: %s spec padding/stale byte @%s\n' "$kind" "$offset"
}
one() {
    kind=$1; file=$2; off=$3
    [ -s "$file" ] || { echo "RED: missing REAL III+ sample $file"; exit 1; }
    # Differential TDD: meaningful mutation is tested BEFORE self equality.
    red "$kind" "$file" "$off" meaningful
    "build/${kind}_diff" "$file" "$file"
    build/w4_mutate --truncate 1 "$file" "$out/$kind.short"
    if "build/${kind}_diff" "$file" "$out/$kind.short" > "$out/$kind-short.log" 2>&1; then
        echo "RED: $kind accepted truncation"; exit 1
    fi
}
one frm "$real/FLT_INFO.FRM" 0xe0
green_pad frm "$real/FLT_INFO.FRM" 0x1d8
green_pad frm "$real/FLT_INFO.FRM" 0x68b
green_pad frm "$real/FLT_INFO.FRM" 0x6e0
green_pad frm "$real/FLT_INFO.FRM" 0x7b5
red frm "$real/FLT_INFO.FRM" 0x1d6 heap-last-char
red frm "$real/FLT_INFO.FRM" 0x68c first-column
red frm "$real/FLT_INFO.FRM" 0x690 uncertain-column-byte
red frm "$real/FLT_INFO.FRM" 0x6df last-column
red frm "$real/FLT_INFO.FRM" 0x7b6 footer
for name in TRAVELER TRIP; do build/frm_diff "$real/$name.FRM" "$real/$name.FRM"; done
bad=$ref/goldens/dbase-iii-plus-1.1/extracted/disk2/UNIVERS.FRM
if build/frm_diff "$bad" "$bad" > "$out/frm-corrupt.log" 2>&1; then echo 'RED: corrupt UNIVERS accepted'; exit 1; fi
one lbl "$real/ADDRESS.LBL" 0x3f
green_pad lbl "$real/ADDRESS.LBL" 0x175
green_pad lbl "$real/ADDRESS.LBL" 0x408
red lbl "$real/ADDRESS.LBL" 0x174 last-active-slot
red lbl "$real/ADDRESS.LBL" 1 remarks
one qry "$real/NOTPAID.QRY" 0xb
red qry "$real/NOTPAID.QRY" 3 length-high-byte
red qry "$real/NOTPAID.QRY" 0xa operator-high-byte
one scr "$real/CLIENTS.SCR" 0x36
for off in 9 0x16 0x18 0x1b 0x2c 0x33 0x22e 0x235; do green_pad scr "$real/CLIENTS.SCR" "$off"; done
for off in 1 0x12 0x17 0x1c 0x20 0x34 0x35 0x3a 0x223 0x22c 0x250; do red scr "$real/CLIENTS.SCR" "$off" "durable-$off"; done
for name in TOURS TRAVEL; do build/scr_diff "$real/$name.SCR" "$real/$name.SCR"; done
one fmt "$real/CLIENTS.FMT" 3
for name in TOURS TRAVEL RESERVE; do build/fmt_diff "$real/$name.FMT" "$real/$name.FMT"; done
one config "$samples/System_1/CONFIG.DB" 7
one prg "$real/ADD.PRG" 3
one vue "$real/RESERVE.VUE" 0x1a
one exact "$real/ADD.PRG" 3
red exact "$real/FLT_INFO.FRM" 0x1d8 unchanged-resave-scratch
printf 'GREEN: test-w4-files: real III+ samples; meaningful mutants RED; documented padding GREEN; framing/truncation RED\n'
