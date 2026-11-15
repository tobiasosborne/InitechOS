#!/bin/sh
# Full shared-driver controls, kept outside the fast host suite. Host only.
set -eu
root=${TOP:?} out=$1 controls=$2
. "$controls/checks.sh"
work=$out/noop-controls
setup rm -rf "$work"
setup mkdir -p "$work"
cat > "$work/tool-fails" <<'STUB'
#!/bin/sh
echo 'injected tool failure before a completed assertion' >&2
: > "$TURBO_TOOL_VISITED"
exit 1
STUB
cat > "$work/inject-make" <<'WRAPPER'
#!/bin/sh
for arg do last=$arg; done
if [ "$last" = test-run ]; then
    case $TURBO_TOOL in
    nasm) set -- "$@" NASM="$TURBO_TOOL_FAILURE";;
    fpc) set -- "$@" FPC="$TURBO_TOOL_FAILURE";;
    *) exit 2;;
    esac
fi
exec make -j1 "$@"
WRAPPER
setup chmod +x "$work/tool-fails" "$work/inject-make"
for variant in healthy-nop nasm-failure fpc-failure genuine-property; do
    component=seed; patchfile=$controls/fixtures/seed-noop.patch; maker=make
    if [ "$variant" = genuine-property ]; then patchfile=$root/turbo/seed/mutants/nondet.patch; fi
    if [ "$variant" = nasm-failure ] || [ "$variant" = fpc-failure ]; then
        maker=$work/inject-make
        TURBO_TOOL=nasm
        if [ "$variant" = fpc-failure ]; then
            component=compiler; TURBO_TOOL=fpc; patchfile=$controls/fixtures/compiler-noop.patch
        fi
        TURBO_TOOL_FAILURE=$work/tool-fails TURBO_TOOL_VISITED=$work/$variant.visited
        export TURBO_TOOL TURBO_TOOL_FAILURE TURBO_TOOL_VISITED
    fi
    status=0
    (cd "$root/turbo/$component" && sh "$root/factory/testlib/run-mutants.sh" \
      "$root" "turbo/$component" "$work/$variant" "$maker" "$patchfile") \
      > "$work/$variant.log" 2>&1 || status=$?
    case $variant in
    healthy-nop)
        assert 'NOOP must survive' test "$status" -eq 1
        assert 'NOOP is never killed' grep -F '1 mutants, 0 killed, 1 survived, 0 errors;' "$work/$variant.log";;
    *-failure)
        assert "$variant:driver rejects setup error" test "$status" -eq 1
        assert "$variant:zero false kills" grep -F '1 mutants, 0 killed, 0 survived, 1 errors;' "$work/$variant.log"
        assert "$variant:tool reached" test -f "$work/$variant.visited";;
    genuine-property)
        assert 'real determinism fault dies' test "$status" -eq 0
        assert 'real assertion counted as a kill' grep -F '1 mutants, 1 killed, 0 survived, 0 errors;' "$work/$variant.log";;
    esac
done
finish
