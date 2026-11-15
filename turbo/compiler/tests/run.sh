#!/bin/sh
set -eu
out=$1 entry=$2
. "$TOP/turbo/tests/checks.sh"
scratch=$out/runs/$entry
rm -rf "$scratch"
setup mkdir -p "$scratch"
run_compiler() {
    dest=$1 source=$2
    setup mkdir -p "$dest"
    setup cp "$source" "$dest/TURBOIN.PAS"
    (cd "$dest" && setup "$out/turbo_fpc" > raw)
    setup tr -d '\r' < "$dest/raw" > "$dest/normalized"
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
    setup mkdir -p "$self_dest"
    setup cp "$shared"/* "$self_dest/"
}
bracket() {
    phase=$1 dir=$2
    assert "$entry:required artifact or count" test "$(grep -c "^TURBO-$phase-BEGIN\$" "$dir/normalized")" -eq 1
    assert "$entry:required artifact or count" test "$(grep -c "^TURBO-$phase-END\$" "$dir/normalized")" -eq 1
    sed -n "/^TURBO-$phase-BEGIN\$/,/^TURBO-$phase-END\$/p" "$dir/normalized" > "$dir/bracket-$phase"
}
links() {
    dir=$1
    assert "$entry:unique generated labels" awk -f tests/labels.awk "$dir/TURBOOUT.S"
    assert "$entry:cdecl header has no direct stores" awk -f "$TOP/turbo/tests/frame-layout.awk" "$dir/TURBOOUT.S"
    (cd "$dir" && setup "$NASM" -f elf32 TURBOOUT.S -o program.o)
    for mode in bare dos; do
        if [ "$mode" = bare ]; then start=start; script=seed; else start=start_dos; script=seed_dos; fi
        setup "$TARGET_LD" -m elf_i386 --build-id=none -T "$RT_SRC/$script.ld" -o "$dir/$mode.elf" \
            "$SEED_OUT/rt/$start.o" "$SEED_OUT/rt/fileio.o" "$dir/program.o"
    done
    setup "$OBJCOPY" -O binary "$dir/dos.elf" "$dir/program.com"
}
case $entry in
execution-frame|execution-var|execution-flat|execution-io)
    name=gen_tiny; paths=hosted
    if [ "$entry" = execution-io ]; then name=gen_ioresult; fi
    if [ "$entry" = execution-var ]; then name=gen_func_deep; fi
    if [ "$entry" = execution-flat ]; then paths='hosted hosted-flat'; fi
    TURBO_HOSTED="$out/turbo_fpc" TURBO_EXEC_PATHS="$paths" \
      TURBO_EXEC_SOURCES="$TOP/turbo/compiler/fixtures/$name.pas" \
      TURBO_EXEC_OUT="$scratch/execution" \
      exec sh "$TOP/turbo/tests/execution/run.sh"
    ;;
isolated-seed)
    # CLAUDE.md, Every component is self-contained: an isolated compiler OUT
    # must not redirect its seed prerequisite away from the requested SEED_OUT.
    setup make --no-print-directory seed-tools TOP="$TOP" OUT="$scratch/compiler" \
        SEED_OUT="$scratch/seed" TESTS= MAKEOVERRIDES=
    assert "$entry:required artifact or count" test -x "$scratch/seed/initechc"
    for object in start start_dos fileio; do assert "$entry:isolated object" test -s "$scratch/seed/rt/$object.o"; done
    assert "$entry:required artifact or count" test ! -e "$scratch/compiler/initechc"
    ;;
runtime-table)
    exec sh tests/runtime.sh "$TOP" "$scratch"
    ;;
lex*|parse*|type*)
    case $entry in lex*) phase=LEX ;; parse*) phase=PARSE ;; type*) phase=TYPE ;; esac
    run_compiler "$scratch" "fixtures/$entry.pas"
    bracket "$phase" "$scratch"
    case $entry in
    lex*|parse_rich|type_rich) setup cp "$scratch/bracket-$phase" "$scratch/actual" ;;
    *) assert "$entry:expected error" grep "^TURBO-$phase-ERROR" "$scratch/bracket-$phase" > "$scratch/actual"
       assert "$entry:required artifact or count" test "$(wc -l < "$scratch/actual")" -eq 1 ;;
    esac
    assert "$entry:protocol or FPC stdout" diff -u "fixtures/$entry.golden" "$scratch/actual"
    ;;
self-protocols)
    for pass in a b; do
        run_self_compiler "$scratch/$pass" "$pass"
        for phase in LEX PARSE TYPE; do
            bracket "$phase" "$scratch/$pass"
            assert_not "$entry:unexpected diagnostic or symbol" grep -q "^TURBO-$phase-ERROR" "$scratch/$pass/bracket-$phase"
            if [ "$phase" = LEX ]; then ok=EOF; else ok=TURBO-$phase-OK; fi
            assert "$entry:required artifact or count" test "$(grep -c "^$ok\$" "$scratch/$pass/bracket-$phase")" -eq 1
        done
    done
    for phase in LEX PARSE TYPE; do assert "$entry:$phase determinism" cmp "$scratch/a/bracket-$phase" "$scratch/b/bracket-$phase"; done
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
        assert_not "$entry:unexpected diagnostic or symbol" grep -q '^TURBO-GEN-ERROR' "$scratch/$pass/bracket-GEN"
        assert "$entry:required artifact or count" test "$(grep -c '^TURBO-GEN-OK bytes=[0-9][0-9]* labels=[0-9][0-9]*$' "$scratch/$pass/bracket-GEN")" -eq 1
        assert "$entry:required artifact or count" test -s "$scratch/$pass/TURBOOUT.S"
        links "$scratch/$pass"
        setup sh tests/package.sh "$scratch/$pass/TURBOOUT.S" "$scratch/$pass/extended" "$scratch/$pass/program.o"
        assert "$entry:required artifact or count" test -s "$scratch/$pass/TURBOOUT.EXE" || { echo "FAIL $entry: no native container emitted"; exit 1; }
        assert "$entry:independent bytes or determinism" cmp "$scratch/$pass/extended/program.exe" "$scratch/$pass/TURBOOUT.EXE"
        setup "$TOP/build/factory/mz/mzcheck" "$scratch/$pass/TURBOOUT.EXE"
        if [ "$entry" != self-generation ]; then
            assert "$entry:independent bytes or determinism" cmp "$scratch/$pass/program.com" "$scratch/$pass/TURBOOUT.COM"
        fi
    done
    for artifact in TURBOOUT.S program.o bare.elf dos.elf program.com; do
        assert "$entry:independent bytes or determinism" cmp "$scratch/a/$artifact" "$scratch/b/$artifact"
    done
    if [ "$entry" = self-generation ]; then
        setup nm -n "$scratch/a/dos.elf" > "$scratch/symbols"
        end=$(awk '$3 == "__bss_end" {print $1}' "$scratch/symbols")
        setup sh tests/package.sh "$scratch/a/TURBOOUT.S" "$scratch/extended" "$scratch/a/program.o"
        echo "self assembly=$(wc -c < "$scratch/a/TURBOOUT.S") DOS end=0x$end"
    else
        setup mkdir -p "$scratch/fpc"
        setup cp "$source" "$scratch/fpc/input.pas"
        setup "$FPC" -B -O- -v0 -FU"$scratch/fpc" -FE"$scratch/fpc" -o"$scratch/fpc/reference" "$scratch/fpc/input.pas"
        (cd "$scratch/fpc" && setup ./reference > stdout)
        assert "$entry:protocol or FPC stdout" diff -u "fixtures/$entry.golden" "$scratch/fpc/stdout"
    fi
    ;;
seed-build)
    # Legacy build contracts, not behavioural execution or an assembly golden.
    asm=$out/turbo_seed.s
    for pattern in '^v_readchunk: resb 256$' '^v_nextchunk: resb 256$' '^pf_advancesourcewindow:' \
        '^pf_parseprogram:' '^v_symnameoffset: resd 2048$' '^pf_runtypechecker:' '^pf_runcodegenerator:' \
        'call rtl_file_blockread$' 'call rtl_file_blockwrite$'; do assert "$entry:ABI declaration" grep -q "$pattern" "$asm"; done
    assert_not "$entry:unexpected diagnostic or symbol" grep -q '^v_sourcewords:' "$asm"
    assert "$entry:required diagnostic or symbol" grep -q "Assign(OutputFile, 'TURBOOUT.S')" turbo.pas
    setup nm -n "$out/extended/zero.elf" > "$scratch/symbols"
    start=$(awk '$3 == "__bss_start" {print $1}' "$scratch/symbols")
    end=$(awk '$3 == "__bss_end" {print $1}' "$scratch/symbols")
    assert "$entry:required artifact or count" test -n "$start" && assert "$entry:required symbol" test -n "$end"
    setup sh tests/package.sh "$asm" "$scratch/extended"
    setup "$SEED_OUT/initechc" --emit-asm -o "$scratch/again.s" turbo.pas
    assert "$entry:independent bytes or determinism" cmp "$asm" "$scratch/again.s"
    assert "$entry:required artifact or count" test -s "$out/TURBO.COM" && assert "$entry:required target" test -s "$out/turbo_seed_bare.elf"
    echo "COM=$(wc -c < "$out/TURBO.COM") BSS=$((0x$end-0x$start)) end=0x$end"
    ;;
*) echo "unknown test: $entry" >&2; exit 1 ;;
esac
finish
