#!/bin/sh
# tps_replay.sh -- GC-03 for the Turbo Initech manuals (bead initech-ipu5.12;
# PUB-REF-0001 NT-17, GC-03, CP-19).  Every displayed program, program
# output, compiler output, assembly excerpt and development-system run that
# a Turbo Initech book prints is registered by initech-borland.sty in
# <job>.tpsexamples and is replayed here against the shipped tools; the
# printed bytes must equal the replayed bytes.
#
#   program|NAME     examples/NAME.pas is compiled three ways and run:
#                    Free Pascal (complete evaluation, ShortStrings: the
#                    independent root, ADR-0007 DEC-07), the seed
#                    INITECHC, and Turbo Initech itself (tps.pas built by
#                    Free Pascal; test-tps-*-os prove TPS.COM prints the same).
#                    Programs without file I/O run as bare images under the
#                    QEMU harness; EVERY Turbo Initech-generated program is
#                    also linked as a .COM and run on InitechDOS in one
#                    bounded boot.  All outputs == examples/NAME.out.
#   output|NAME      examples/NAME.out is printed; NAME must be a program.
#   compiler|NAME|R  the TPS-R-BEGIN..TPS-R-END region Turbo Initech prints
#                    for examples/NAME.pas == examples/NAME.R.txt.
#   assembly|NAME|L  TPSOUT.S for examples/NAME.pas from `L:' to the next
#                    top-level label == examples/NAME.L.s.
#   make|NAME        `make $(cat examples/NAME.cmd)': its lines that begin
#                    ">>> ", "!!! " or carry a pass rate == examples/NAME.txt.
# Inputs come from the environment (pubs/tps.mk exports them).  Thin shell.
set -eu
: "${TPS_BIN:?}" "${SEED_BIN:?}" "${NASM:?}" "${LD:?}" "${OBJCOPY:?}" "${SEED_RT_OBJ:?}" \
  "${SEED_DOS_RT_OBJ:?}" "${SEED_FILEIO_RT_OBJ:?}" "${SEED_RT_LD:?}" "${SEED_DOS_LD:?}" \
  "${HARNESS_BIN:?}" "${TRACER_IMG:?}" "${GATE:?}" "${BUILT:?}" "${OUT:?}" "${MAKE:?}"
root=$(pwd)
abs() { case $1 in /*) printf '%s' "$1";; *) printf '%s/%s' "$root" "$1";; esac; }
TPS_BIN=$(abs "$TPS_BIN"); SEED_BIN=$(abs "$SEED_BIN")
red() { printf '!!! test-pubs RED %s: %s: %s\n' "$1" "$2" "$3" >&2; exit 1; }
same() {  # tag printed actual
    if ! cmp -s "$2" "$3"; then diff -u "$2" "$3" | head -20 >&2 || true; red "$1" "$2" "differs from the replay ($3)"; fi
}
rm -rf "$OUT"; mkdir -p "$OUT/dos"
: > "$OUT/dos.bat"; printf '@ECHO OFF\r\n' > "$OUT/dos/AUTOEXEC.BAT"
n=0; ndos=0; nbare=0
for index in "$BUILT"/turbo-initech-*/main.tpsexamples; do
    [ -f "$index" ] || continue
    book=$(basename "$(dirname "$index")")
    ex=${EXROOT:-pubs/manuals}/$book/examples
    sort -u "$index" > "$OUT/$book.list"
    while IFS='|' read -r kind name arg; do
        [ -n "$kind" ] || continue
        src=$ex/$name.pas
        w=$OUT/$book-$name
        case $kind in
        program)
            [ -f "$src" ] && [ -f "$ex/$name.out" ] || red TRANSCRIPT-PAIR "$src" "needs $name.pas and $name.out"
            mkdir -p "$w/fpc" "$w/tps" "$w/seed"
            # Free Pascal: the independent root under the DEC-02 pins.
            printf '{$MODE DELPHI}{$B+}{$H-}\n' > "$w/fpc/$name.pas"; cat "$src" >> "$w/fpc/$name.pas"
            fpc -B -O- -v0 -FU"$w/fpc" -FE"$w/fpc" -o"$w/fpc/prog" "$w/fpc/$name.pas" > "$w/fpc/log" 2>&1 \
                || { cat "$w/fpc/log" >&2; red FPC-REJECTS "$src" "Free Pascal did not compile the printed program"; }
            (cd "$w/fpc" && ./prog > out.txt) || red FPC-RUN "$src" "the Free Pascal program failed"
            same TRANSCRIPT-DRIFT "$ex/$name.out" "$w/fpc/out.txt"
            # Turbo Initech: compile, assemble, link.
            cp "$src" "$w/tps/TPSIN.PAS"
            (cd "$w/tps" && "$TPS_BIN" > tps.out) || red TPS-RUN "$src" "Turbo Initech exited non-zero"
            grep -q '^TPS-GEN-OK bytes=' "$w/tps/tps.out" || { grep 'ERROR' "$w/tps/tps.out" >&2 || true; red TPS-REJECTS "$src" "Turbo Initech did not compile the printed program"; }
            "$NASM" -f elf32 "$w/tps/TPSOUT.S" -o "$w/tps/p.o"
            "$LD" -m elf_i386 -T "$SEED_DOS_LD" -o "$w/tps/dos.elf" "$SEED_DOS_RT_OBJ" "$SEED_FILEIO_RT_OBJ" "$w/tps/p.o" 2>/dev/null
            "$OBJCOPY" -O binary "$w/tps/dos.elf" "$w/tps/P.COM"
            ndos=$((ndos + 1)); dos=$(printf 'E%03d' "$ndos")
            cp "$w/tps/P.COM" "$OUT/dos/$dos.COM"
            printf 'ECHO PUBS-EX-BEGIN %s\r\n%s\r\nECHO PUBS-EX-END %s\r\n' "$dos" "$dos" "$dos" >> "$OUT/dos/AUTOEXEC.BAT"
            printf '%s|%s\n' "$dos" "$ex/$name.out" >> "$OUT/dos.bat"
            if ! grep -q 'rtl_file_' "$w/tps/TPSOUT.S"; then
                "$LD" -m elf_i386 -T "$SEED_RT_LD" -o "$w/tps/bare.elf" "$SEED_RT_OBJ" "$w/tps/p.o" 2>/dev/null
                "$HARNESS_BIN" --kernel "$w/tps/bare.elf" --serial-stdout --name "pubs_tps_$ndos" \
                    --out "$OUT" --timeout-ms 10000 2>/dev/null > "$w/tps/out.txt" || true
                same TRANSCRIPT-DRIFT "$ex/$name.out" "$w/tps/out.txt"
                # The seed (the genesis compiler) must agree too.
                "$SEED_BIN" --emit-asm -o "$w/seed/p.s" "$src" 2> "$w/seed/log" \
                    || { cat "$w/seed/log" >&2; red SEED-REJECTS "$src" "INITECHC did not compile the printed program"; }
                "$NASM" -f elf32 "$w/seed/p.s" -o "$w/seed/p.o"
                "$LD" -m elf_i386 -T "$SEED_RT_LD" -o "$w/seed/bare.elf" "$SEED_RT_OBJ" "$w/seed/p.o" 2>/dev/null
                "$HARNESS_BIN" --kernel "$w/seed/bare.elf" --serial-stdout --name "pubs_seed_$ndos" \
                    --out "$OUT" --timeout-ms 10000 2>/dev/null > "$w/seed/out.txt" || true
                same TRANSCRIPT-DRIFT "$ex/$name.out" "$w/seed/out.txt"
                nbare=$((nbare + 1))
            fi
            ;;
        output)
            grep -qx "program|$name" "$OUT/$book.list" || red TRANSCRIPT-PAIR "$ex/$name.out" "printed output of a program that is not printed"
            ;;
        compiler)
            [ -f "$src" ] && [ -f "$ex/$name.$arg.txt" ] || red TRANSCRIPT-PAIR "$src" "needs $name.$arg.txt"
            mkdir -p "$w/c"; cp "$src" "$w/c/TPSIN.PAS"
            (cd "$w/c" && "$TPS_BIN" > tps.out) || red TPS-RUN "$src" "Turbo Initech exited non-zero"
            sed -n "/^TPS-$arg-BEGIN\$/,/^TPS-$arg-END\$/p" "$w/c/tps.out" > "$w/c/$arg.txt"
            [ -s "$w/c/$arg.txt" ] || red COMPILER-REGION "$src" "no TPS-$arg region"
            same TRANSCRIPT-DRIFT "$ex/$name.$arg.txt" "$w/c/$arg.txt"
            ;;
        assembly)
            [ -f "$src" ] && [ -f "$ex/$name.$arg.s" ] || red TRANSCRIPT-PAIR "$src" "needs $name.$arg.s"
            mkdir -p "$w/a"; cp "$src" "$w/a/TPSIN.PAS"
            (cd "$w/a" && "$TPS_BIN" > tps.out) || red TPS-RUN "$src" "Turbo Initech exited non-zero"
            awk -v l="$arg:" '$0 == l { on = 1; print; next } on && /^[A-Za-z_][A-Za-z0-9_]*:$/ { exit } on && /^section / { exit } on { print }' \
                "$w/a/TPSOUT.S" > "$w/a/$arg.s"
            [ -s "$w/a/$arg.s" ] || red ASSEMBLY-LABEL "$src" "no label $arg in TPSOUT.S"
            same TRANSCRIPT-DRIFT "$ex/$name.$arg.s" "$w/a/$arg.s"
            ;;
        make)
            [ -f "$ex/$name.cmd" ] && [ -f "$ex/$name.txt" ] || red TRANSCRIPT-PAIR "$ex/$name" "needs $name.cmd and $name.txt"
            target=$(cat "$ex/$name.cmd")
            mkdir -p "$w"
            "$MAKE" --no-print-directory $target > "$w/make.log" 2>&1 || true
            grep -E '^(>>> |!!! |[a-z_]+_pass_rate=)' "$w/make.log" > "$w/make.txt" || true
            same TRANSCRIPT-DRIFT "$ex/$name.txt" "$w/make.txt"
            ;;
        *) red TRANSCRIPT-KIND "$index" "$kind" ;;
        esac
        n=$((n + 1))
    done < "$OUT/$book.list"
done
[ "$n" -gt 0 ] || red TRANSCRIPT-EMPTY "$BUILT" "no Turbo Initech examples registered"
# One bounded InitechDOS boot runs every Turbo Initech-generated program.
if [ "$ndos" -gt 0 ]; then
    img=$OUT/examples.img
    dd if=/dev/zero of="$img" bs=512 count=2880 status=none
    mformat -i "$img" -f 1440 ::
    mcopy -i "$img" "$OUT/dos/AUTOEXEC.BAT" ::AUTOEXEC.BAT
    for f in "$OUT"/dos/E*.COM; do mcopy -i "$img" "$f" "::$(basename "$f")"; done
    "$HARNESS_BIN" --disk "$TRACER_IMG" --disk2 "$img" --name pubs_tps_dos --out "$OUT" \
        --timeout-ms 240000 --keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
        --keys-after-ms 60000 --quit-after "SHELL-EXIT" 2> "$OUT/dos.report" || true
    grep -q 'triple_fault=1' "$OUT/dos.report" && red DOS-REPLAY "$img" "triple fault"
    tr -d '\r' < "$OUT/pubs_tps_dos.serial" > "$OUT/dos.serial"
    grep -q '^SHELL-EXIT$' "$OUT/dos.serial" || red DOS-REPLAY "$img" "the batch did not finish (no SHELL-EXIT)"
    while IFS='|' read -r dos expected; do
        sed -n "/^PUBS-EX-BEGIN $dos\$/,/^PUBS-EX-END $dos\$/p" "$OUT/dos.serial" | sed '1d;$d' > "$OUT/dos/$dos.out"
        same TRANSCRIPT-DRIFT "$expected" "$OUT/dos/$dos.out"
    done < "$OUT/dos.bat"
fi
printf '>>> test-pubs-tps-examples: green (%s printed items replayed; %s programs: Free Pascal == printed, %s also seed and Turbo Initech bare images == printed, %s Turbo Initech .COM files == printed on InitechDOS)\n' "$n" "$ndos" "$nbare" "$ndos"
