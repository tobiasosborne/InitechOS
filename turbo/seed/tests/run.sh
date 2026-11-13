#!/bin/sh
set -eu
out=$1 entry=$2
case $entry in
lexer|parser) exec "$out/test_$entry" ;;
esac
scratch=$out/runs/$entry
rm -rf "$scratch"
mkdir -p "$scratch"
link_bare() {
    "$NASM" -f elf32 "$1" -o "$2.o"
    "$TARGET_LD" -m elf_i386 --build-id=none -T "$PWD/rt/seed.ld" -o "$2.elf" \
        "$out/rt/start.o" "$out/rt/fileio.o" "$2.o"
}
case $entry in
repro)
    n=0
    for source in examples/hello.pas examples/smoke.pas examples/arith/*.pas; do
        name=$(basename "$source" .pas)
        extra_rt=
        if [ "$name" = fileio ]; then extra_rt=$out/rt/fileio.o; fi
        for pass in a b; do
            mkdir -p "$scratch/$pass"
            "$out/initechc" --emit-asm -o "$scratch/$pass/$name.s" "$source"
            (cd "$scratch/$pass" && "$NASM" -f elf32 "$name.s" -o "$name.o")
            "$TARGET_LD" -m elf_i386 --build-id=none -T rt/seed.ld -o "$scratch/$pass/$name.elf" \
                "$out/rt/start.o" ${extra_rt:+"$extra_rt"} "$scratch/$pass/$name.o"
        done
        for stage in s o elf; do cmp "$scratch/a/$name.$stage" "$scratch/b/$name.$stage"; done
        n=$((n+1))
    done
    awk '/^static void gen_binop/,/^}/' codegen.c > "$scratch/gen_binop.c"
    ! grep -qE '\bj[a-z]+\b' "$scratch/gen_binop.c"
    echo "$n fixtures, $((n*3)) equal artifact pairs"
    ;;
fileio-build)
    "$out/initechc" --emit-asm -o "$scratch/fileio.s" examples/arith/fileio.pas
    link_bare "$scratch/fileio.s" "$scratch/fileio"
    test "$(grep -c 'call rtl_file_blockwrite$' "$scratch/fileio.s")" -eq 2
    grep -q '^v_targetfile: resd 65$' "$scratch/fileio.s"
    grep -q '^    sub esp, 260$' "$scratch/fileio.s"
    grep -q '^    mov dword \[ebp-260\], 0$' "$scratch/fileio.s"
    grep -q '^    mov byte \[ebp-256\], 0$' "$scratch/fileio.s"
    grep -q '^    call pf_exerciselocalfile$' "$scratch/fileio.s"
    nm "$out/rt/fileio.o" > "$scratch/symbols"
    for fn in assign reset rewrite blockread blockwrite blockwrite_short blockwrite_wrong_handle; do
        grep -q " rtl_file_$fn\$" "$scratch/symbols"
    done
    nm "$out/rt/start.o" > "$scratch/start-symbols"
    ! grep -q 'rtl_file_' "$scratch/start-symbols"
    ;;
fileio-fpc|fpc-shared)
    if [ "$entry" = fileio-fpc ]; then names=file_shared; else names='func_shared array_shared record_shared string_shared'; fi
    for name in $names; do
        run=$scratch/$name
        mkdir -p "$run"
        cp "examples/fpc/$name.pas" "$run/input.pas"
        "$FPC" -B -O- -v0 -FU"$run" -FE"$run" -o"$run/reference" "$run/input.pas"
        (cd "$run" && ./reference > stdout)
        case $name in
        func_shared) expected='120 TRUE FALSE 2 1' ;;
        array_shared) expected='SUM=15 REV=54321 NESTED=2 BSUM=510 CSUM=20 F1=TRUE F2=FALSE LSUM=45' ;;
        record_shared) expected='KIND=7 VAL=40 OK=FALSE BKIND=17 BOK=TRUE T2KIND=99 TKIND=17 T2VAL=12 KSUM=118' ;;
        string_shared) expected='Hello, World! 13 H EQ LT' ;;
        file_shared)
            expected='FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR'
            printf NORTHSTAR | cmp - "$run/B8FILE.BIN"
            test ! -s "$run/B8SINK.BIN" && test -f "$run/B8SINK.BIN"
            test ! -s "$run/B8LOCAL.BIN" && test -f "$run/B8LOCAL.BIN"
            ;;
        esac
        printf '%s\n' "$expected" | cmp - "$run/stdout"
    done
    ;;
*) echo "unknown test: $entry" >&2; exit 1 ;;
esac
