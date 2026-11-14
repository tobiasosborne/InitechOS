#!/bin/sh
set -eu
out=$1 entry=$2
scratch=$out/runs/$entry
rm -rf "$scratch"
mkdir -p "$scratch"
run_compiler() {
    dest=$1 source=$2
    mkdir -p "$dest"
    cp "$source" "$dest/TURBOIN.PAS"
    (cd "$dest" && "$out/turbo_fpc" > raw)
    tr -d '\r' < "$dest/raw" > "$dest/normalized"
}
run_self_compiler() {
    self_dest=$1 self_pass=$2
    # Both self entries assert different properties of the same two fresh
    # translations. Share only inside this invocation, never across make runs.
    # Generation stays inside the timed entry runner; every assertion below
    # still runs, including both protocol and artifact determinism checks.
    if [ -z "${TURBO_SELF_RUNS:-}" ]; then
        run_compiler "$self_dest" turbo.pas
        return
    fi
    shared=$TURBO_SELF_RUNS/$self_pass
    if [ ! -f "$shared/complete" ]; then
        run_compiler "$shared" turbo.pas
        : > "$shared/complete"
    fi
    mkdir -p "$self_dest"
    cp "$shared"/* "$self_dest/"
}
bracket() {
    phase=$1 dir=$2
    test "$(grep -c "^TURBO-$phase-BEGIN\$" "$dir/normalized")" -eq 1
    test "$(grep -c "^TURBO-$phase-END\$" "$dir/normalized")" -eq 1
    sed -n "/^TURBO-$phase-BEGIN\$/,/^TURBO-$phase-END\$/p" "$dir/normalized" > "$dir/bracket-$phase"
}
links() {
    dir=$1
    (cd "$dir" && "$NASM" -f elf32 TURBOOUT.S -o program.o)
    for mode in bare dos; do
        if [ "$mode" = bare ]; then start=start; script=seed; else start=start_dos; script=seed_dos; fi
        "$TARGET_LD" -m elf_i386 --build-id=none -T "$RT_SRC/$script.ld" -o "$dir/$mode.elf" \
            "$SEED_OUT/rt/$start.o" "$SEED_OUT/rt/fileio.o" "$dir/program.o"
    done
    "$OBJCOPY" -O binary "$dir/dos.elf" "$dir/program.com"
}
case $entry in
execution-frame|execution-var|execution-flat)
    name=gen_tiny; paths=hosted
    if [ "$entry" = execution-var ]; then name=gen_func_deep; fi
    if [ "$entry" = execution-flat ]; then paths='hosted hosted-flat'; fi
    TURBO_HOSTED="$out/turbo_fpc" TURBO_EXEC_PATHS="$paths" \
      TURBO_EXEC_SOURCES="$TOP/turbo/compiler/fixtures/$name.pas" \
      TURBO_EXEC_OUT="$scratch/execution" \
      sh "$TOP/turbo/tests/execution/run.sh"
    ;;
isolated-seed)
    # CLAUDE.md, Every component is self-contained: an isolated compiler OUT
    # must not redirect its seed prerequisite away from the requested SEED_OUT.
    make --no-print-directory seed-tools TOP="$TOP" OUT="$scratch/compiler" \
        SEED_OUT="$scratch/seed" TESTS= MAKEOVERRIDES=
    test -x "$scratch/seed/initechc"
    for object in start start_dos fileio; do test -s "$scratch/seed/rt/$object.o"; done
    test ! -e "$scratch/compiler/initechc"
    ;;
runtime-table)
    sh tests/runtime.sh "$TOP" "$scratch"
    ;;
lex*|parse*|type*)
    case $entry in lex*) phase=LEX ;; parse*) phase=PARSE ;; type*) phase=TYPE ;; esac
    run_compiler "$scratch" "fixtures/$entry.pas"
    bracket "$phase" "$scratch"
    case $entry in
    lex*|parse_rich|type_rich) cp "$scratch/bracket-$phase" "$scratch/actual" ;;
    *) grep "^TURBO-$phase-ERROR" "$scratch/bracket-$phase" > "$scratch/actual"
       test "$(wc -l < "$scratch/actual")" -eq 1 ;;
    esac
    diff -u "fixtures/$entry.golden" "$scratch/actual"
    ;;
self-protocols)
    for pass in a b; do
        run_self_compiler "$scratch/$pass" "$pass"
        for phase in LEX PARSE TYPE; do
            bracket "$phase" "$scratch/$pass"
            ! grep -q "^TURBO-$phase-ERROR" "$scratch/$pass/bracket-$phase"
            if [ "$phase" = LEX ]; then ok=EOF; else ok=TURBO-$phase-OK; fi
            test "$(grep -c "^$ok\$" "$scratch/$pass/bracket-$phase")" -eq 1
        done
    done
    for phase in LEX PARSE TYPE; do cmp "$scratch/a/bracket-$phase" "$scratch/b/bracket-$phase"; done
    ;;
gen*|self-generation)
    if [ "$entry" = self-generation ]; then source=turbo.pas; else source=fixtures/$entry.pas; fi
    for pass in a b; do
        if [ "$entry" = self-generation ]; then
            run_self_compiler "$scratch/$pass" "$pass"
        else
            run_compiler "$scratch/$pass" "$source"
        fi
        bracket GEN "$scratch/$pass"
        ! grep -q '^TURBO-GEN-ERROR' "$scratch/$pass/bracket-GEN"
        test "$(grep -c '^TURBO-GEN-OK bytes=[0-9][0-9]* labels=[0-9][0-9]*$' "$scratch/$pass/bracket-GEN")" -eq 1
        test -s "$scratch/$pass/TURBOOUT.S"
        links "$scratch/$pass"
        sh tests/package.sh "$scratch/$pass/TURBOOUT.S" "$scratch/$pass/extended" "$scratch/$pass/program.o"
        cmp "$scratch/$pass/extended/program.exe" "$scratch/$pass/TURBOOUT.EXE"
        "$TOP/build/factory/mz/mzcheck" "$scratch/$pass/TURBOOUT.EXE"
        if [ "$entry" != self-generation ]; then
            cmp "$scratch/$pass/program.com" "$scratch/$pass/TURBOOUT.COM"
        fi
    done
    for artifact in TURBOOUT.S program.o bare.elf dos.elf program.com; do
        cmp "$scratch/a/$artifact" "$scratch/b/$artifact"
    done
    if [ "$entry" = self-generation ]; then
        nm -n "$scratch/a/dos.elf" > "$scratch/symbols"
        end=$(awk '$3 == "__bss_end" {print $1}' "$scratch/symbols")
        sh tests/package.sh "$scratch/a/TURBOOUT.S" "$scratch/extended" "$scratch/a/program.o"
        echo "self assembly=$(wc -c < "$scratch/a/TURBOOUT.S") DOS end=0x$end"
    else
        mkdir -p "$scratch/fpc"
        cp "$source" "$scratch/fpc/input.pas"
        "$FPC" -B -O- -v0 -FU"$scratch/fpc" -FE"$scratch/fpc" -o"$scratch/fpc/reference" "$scratch/fpc/input.pas"
        (cd "$scratch/fpc" && ./reference > stdout)
        diff -u "fixtures/$entry.golden" "$scratch/fpc/stdout"
    fi
    ;;
seed-build)
    # Legacy build contracts, not behavioural execution or an assembly golden.
    asm=$out/turbo_seed.s
    for pattern in '^v_readchunk: resb 256$' '^v_nextchunk: resb 256$' '^pf_advancesourcewindow:' \
        '^pf_parseprogram:' '^v_symnameoffset: resd 1024$' '^pf_runtypechecker:' '^pf_runcodegenerator:' \
        'call rtl_file_blockread$' 'call rtl_file_blockwrite$'; do grep -q "$pattern" "$asm"; done
    ! grep -q '^v_sourcewords:' "$asm"
    grep -q "Assign(OutputFile, 'TURBOOUT.S')" turbo.pas
    nm -n "$out/extended/zero.elf" > "$scratch/symbols"
    start=$(awk '$3 == "__bss_start" {print $1}' "$scratch/symbols")
    end=$(awk '$3 == "__bss_end" {print $1}' "$scratch/symbols")
    test -n "$start" && test -n "$end"
    sh tests/package.sh "$asm" "$scratch/extended"
    "$SEED_OUT/initechc" --emit-asm -o "$scratch/again.s" turbo.pas
    cmp "$asm" "$scratch/again.s"
    test -s "$out/TURBO.COM" && test -s "$out/turbo_seed_bare.elf"
    echo "COM=$(wc -c < "$out/TURBO.COM") BSS=$((0x$end-0x$start)) end=0x$end"
    ;;
*) echo "unknown test: $entry" >&2; exit 1 ;;
esac
