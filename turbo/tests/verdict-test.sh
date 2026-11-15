#!/bin/sh
# Host-only regression for initech-kkqbw. Runs the real adapters and guest
# producer, replacing only external tools/harnesses; never starts an emulator.
set -eu
out=$1
root=${TOP:?}
controls=${TURBO_TESTS_ROOT:-$root/turbo/tests}
subject_out=$out
if [ ! -x "$subject_out/initechc" ]; then subject_out=$root/build/turbo/seed; fi
. "$controls/checks.sh"
work=$out/runs/test-verdict
setup rm -rf "$work"
setup mkdir -p "$work"
case_run() {
    label=$1 expected_state=$2
    shift 2
    status=0
    sh "$root/factory/testlib/run-test.sh" "$label" 10 '' "$work/$label" \
      "$@" > "$work/$label.driver" 2>&1 || status=$?
    if [ "$expected_state" = SEMANTIC ]; then
        assert "$label:real assertion receipt" grep -Eq '^SEMANTIC [0-9a-f]{64}$' "$work/$label.result"
        assert "$label:semantic exit" test "$status" -eq 1
    else
        assert "$label:classification" grep -Fx "$expected_state" "$work/$label.result"
        if [ "$expected_state" = PASS ]; then assert "$label:exit" test "$status" -eq 0
        else assert "$label:nonzero exit" test "$status" -ne 0; fi
    fi
}
for variant in empty failed incomplete zero-count pass semantic inconsistent timeout startup; do
    runner=$work/$variant.sh
    case $variant in
    empty) printf 'exit 0\n' > "$runner"; state=ERROR;;
    failed) printf 'echo "FAIL without completed assertions"\nexit 1\n' > "$runner"; state=ERROR;;
    incomplete) printf 'echo "1 checks, 0 failures"\nexit 1\n' > "$runner"; state=ERROR;;
    zero-count) printf 'echo "0 checks, 0 failures"\n' > "$runner"; state=ERROR;;
    pass) printf 'echo "1 checks, 0 failures"\n' > "$runner"; state=PASS;;
    semantic) printf 'echo "FAIL literal property"\necho "1 checks, 1 failures"\nexit 1\n' > "$runner"; state=SEMANTIC;;
    inconsistent) printf 'echo "FAIL literal property"\necho "1 checks, 1 failures"\n' > "$runner"; state=ERROR;;
    timeout) printf 'echo timed_out=1\nexit 124\n' > "$runner"; state=ERROR;;
    startup) printf 'echo launched=0\nexit 2\n' > "$runner"; state=ERROR;;
    esac
    case_run "$variant" "$state" sh "$controls/entry.sh" "$runner" "$out" "$variant" "$work/$variant.log"
done
cd "$root/turbo/seed"
printf 'bits 32\npush ebp\nmov ebp,esp\nmov [ebp-4],eax\nmov eax,[ebp+8]\nleave\nret\n' > "$work/frame-valid.s"
assert 'cdecl local and argument slots allowed' awk -f "$controls/frame-layout.awk" "$work/frame-valid.s"
for offset in 0 4; do
    printf 'mov [ebp+%s],eax\n' "$offset" > "$work/frame-invalid.s"
    assert_not "cdecl header word $offset is immutable" awk -f "$controls/frame-layout.awk" "$work/frame-invalid.s"
done
printf '#!/bin/sh\necho injected-tool-failure >&2\n: > "$TOOL_VISITED"\nexit 1\n' > "$work/tool-fails"
setup chmod +x "$work/tool-fails"
case_run nasm ERROR env TOOL_VISITED="$work/nasm.called" NASM="$work/tool-fails" sh "$controls/entry.sh" tests/run.sh "$subject_out" repro "$work/nasm.log"
case_run fpc ERROR env TOOL_VISITED="$work/fpc.called" FPC="$work/tool-fails" sh "$controls/entry.sh" tests/run.sh "$subject_out" fileio-fpc "$work/fpc.log"
assert 'NASM control reached the assembler' test -f "$work/nasm.called"
assert 'FPC control reached the reference compiler' test -f "$work/fpc.called"
# Exercise the actual inherited producer and the new independent validator.
# All guest resources live in this scratch root. Only the harness is a stub.
fake=$work/no-emulator
setup mkdir -p "$fake/factory" "$fake/turbo/tests" "$fake/build/factory" "$fake/build/dos"
setup cp "$root/factory/seed-guest.sh" "$fake/factory/"
setup cp "$controls/checks.sh" "$fake/turbo/tests/"
setup cp "$controls/frame-layout.awk" "$fake/turbo/tests/"
printf 'unused host harness fixture\n' > "$fake/build/dos/COMMAND.COM"
printf '#!/bin/sh\ncat\n' > "$fake/build/factory/keys"
setup chmod +x "$fake/build/factory/keys"
for variant in missing-serial harness-timeout harness-startup; do
    cat > "$fake/build/factory/qemu_harness" <<'STUB'
#!/bin/sh
printf '%s\n' "$HARNESS_CONTROL" > "$HARNESS_VISITED"
case $HARNESS_CONTROL in
missing-serial)
    echo '[harness] launched=1 timed_out=0 exit=0 signal=0' >&2
    echo '[harness] triple_fault=0 cpu_reset=2 guest_errors=0' >&2
    echo '[harness] quit_after_found=1' >&2
    exit 0;;
harness-timeout) echo '[harness] launched=1 timed_out=1 exit=0 signal=0' >&2; exit 1;;
harness-startup) echo '[harness] launched=0 timed_out=0 exit=1 signal=0' >&2; exit 2;;
esac
exit 2
STUB
    setup chmod +x "$fake/build/factory/qemu_harness"
    # A tiny forwarding runner preserves the real entry adapter's interface.
    cat > "$work/guest-runner.sh" <<'RUNNER'
#!/bin/sh
exec sh "$REAL_ROOT/turbo/seed/tests/guest.sh" "$FAKE_ROOT" "$1" bool
RUNNER
    setup rm -f "$work/harness.called"
    case_run "$variant" ERROR env REAL_ROOT="$root" FAKE_ROOT="$fake" HARNESS_CONTROL="$variant" HARNESS_VISITED="$work/harness.called" \
      sh "$controls/entry.sh" "$work/guest-runner.sh" "$subject_out" guest-bool "$work/$variant.log"
    assert "$variant:reached the harness" grep -Fx "$variant" "$work/harness.called"
done
finish
