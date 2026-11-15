#!/bin/sh
# initech-kkqbw: a wrapper that turns any exit 1 into assertion text must not
# let a build tool or an emulator run that never completed kill a mutant.
# The fixture mirrors that wrapper shape and builds its product at test time.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
runner=${MUTATION_RUNNER:-$root/factory/testlib/run-mutants.sh}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
c=$work/component
mkdir -p "$c/mutants" "$work/inject"
cat > "$c/Makefile" <<'MAKE'
COMPONENT := fixture
include $(TOP)/mk/config.mk
.PHONY: test test-build test-run
test: test-run
test-build:
	@mkdir -p $(OUT)/host
test-run: test-build
	@rm -f $(OUT)/host/*.result; fail=0; \
	sh $(TOP)/factory/testlib/run-test.sh probe 20 '' $(OUT)/host/probe sh wrap.sh $(OUT) || fail=1; \
	echo 'INITECH_TEST_RUN 1'; test $$fail -eq 0
MAKE
# The wrapper fabricates an assertion and a summary from exit 1 alone.
cat > "$c/wrap.sh" <<'SH'
out=$1 rc=0
sh run.sh "$out" > "$out/host/probe.log" 2>&1 || rc=$?
if [ "$rc" -eq 0 ]; then echo '1 checks, 0 failures' >> "$out/host/probe.log"; exit 0; fi
if [ "$rc" -eq 1 ]; then echo 'FAIL probe' >> "$out/host/probe.log"; echo '1 checks, 1 failures' >> "$out/host/probe.log"; fi
exit "$rc"
SH
cat > "$c/run.sh" <<'SH'
set -eu
out=$1
if [ -n "${MKTEMP_CC:-}" ]; then
 # Review follow-up (xreview-frame-factory F1): generated temp sources and
 # one reused output name; the baseline's expected refusal is a different
 # input with the same command shape as the mutant's broken compile.
 gen=$(mktemp ./tmp.XXXXXX); cp product.c "$gen"
 cc -x c -c "$gen" -o "$out/x.o"; cc -o "$out/product" "$out/x.o"
 gen2=$(mktemp ./tmp.XXXXXX); cp invalid.c "$gen2"
 if cc -x c -c "$gen2" -o "$out/x.o" 2>/dev/null; then exit 2; fi
 rm -f "$gen" "$gen2"
else
cc -o "$out/product" product.c
fi
# An independent negative compiler check: green when the compiler refuses.
if [ -n "${NEGATIVE_CC:-}" ] && cc -c invalid.c -o "$out/invalid.o" 2>/dev/null; then exit 2; fi
got=$("$out/product")
[ "$got" = 7 ] || { echo "FAIL run.sh:5: product printed $got, want 7"; exit 1; }
# A guest leg whose script ignores the harness status, as some do.
"$EMU_HARNESS" > "$out/serial" 2>/dev/null || :
grep -qx READY "$out/serial" || { echo 'FAIL run.sh:8: guest never printed READY'; exit 1; }
SH
printf 'not valid C\n' > "$c/invalid.c"
printf '#include <stdio.h>\nint main(void) { printf("%%d\\n", 7); return 0; } /* seven */\n' > "$c/product.c"
mkpatch() { # name, replacement line for product.c
 printf -- '--- a/product.c\n+++ b/product.c\n@@ -2 +2 @@\n-int main(void) { printf("%%d\\n", 7); return 0; } /* seven */\n+%s\n' "$2" > "$c/mutants/$1.patch"
}
mkpatch comment 'int main(void) { printf("%d\n", 7); return 0; } /* still seven */'
mkpatch broken 'int main(void) { printf("%d\n", 7) return 0; } /* seven */'
mkpatch semantic 'int main(void) { printf("%d\n", 8); return 0; } /* seven */'
# Stand-ins fail only inside a mutant's copy, the way load or a missing tool would.
real_cc=$(command -v cc)
cat > "$work/inject/cc" <<SH
#!/bin/sh
case \$PWD in *baseline-*) ;; *) [ -z "\${INJECT_CC:-}" ] || { echo 'cc: internal compiler error (injected)' >&2; exit 1; };; esac
exec "$real_cc" "\$@"
SH
# The emulator stand-in follows the harness contract (factory/README.md,
# Emulator exit status): a run that never completed exits 3 and says so.
cat > "$work/inject/harness" <<'SH'
#!/bin/sh
case $PWD in *baseline-*) echo READY; exit 0;; esac
[ -n "${INJECT_EMU:-}" ] || { echo READY; exit 0; }
[ -z "${INITECH_INFRA_LOG:-}" ] || echo 'qemu_harness exit 3 (launched=1 timed_out=1)' >> "$INITECH_INFRA_LOG"
exit 3
SH
chmod +x "$work/inject/cc" "$work/inject/harness"
failed=0
expect() { # label, wanted summary, patch, environment...
 label=$1 want=$2 patch=$3; shift 3
 rc=0
 (cd "$c" && env PATH="$work/inject:$PATH" EMU_HARNESS="$work/inject/harness" "$@" \
  sh "$runner" "$root" fixture "$work/out-$label" make "mutants/$patch.patch") > "$work/$label.log" 2>&1 || rc=$?
 if grep -q "fixture: 1 mutants, $want" "$work/$label.log"; then echo "ok infra mutants: $label"
 else echo "FAIL infra mutants: $label: want '$want'"; tail -n 8 "$work/$label.log"; failed=1; fi
}
expect comment-only-control '0 killed, 1 survived, 0 errors' comment
expect comment-only-compiler-failure '0 killed, 0 survived, 1 errors' comment INJECT_CC=1
expect comment-only-emulator-incomplete '0 killed, 0 survived, 1 errors' comment INJECT_EMU=1
expect build-breaking-at-test-time '0 killed, 0 survived, 1 errors' broken
expect semantic-still-killed '1 killed, 0 survived, 0 errors' semantic
# initech-kkqbw follow-up (review F1): the baseline's expected compiler
# refusal of invalid.c must not cancel the mutant's new failure on product.c.
expect build-breaking-behind-expected-refusal '0 killed, 0 survived, 1 errors' broken NEGATIVE_CC=1
expect semantic-killed-with-expected-refusal '1 killed, 0 survived, 0 errors' semantic NEGATIVE_CC=1
expect build-breaking-behind-generated-refusal '0 killed, 0 survived, 1 errors' broken MKTEMP_CC=1
expect semantic-killed-with-generated-refusal '1 killed, 0 survived, 0 errors' semantic MKTEMP_CC=1
# initech-piuvw: FPC finds ld on PATH and then runs ld.bfd from the same
# directory. A real FPC hello program compiled at test time is the
# independent control: its baseline must pass through the recording shims.
p=$work/pascal
mkdir -p "$p/mutants"
command -v fpc >/dev/null || { echo "FAIL infra mutants: fpc not installed (setup)"; exit 2; }
cat > "$p/Makefile" <<'MAKE'
COMPONENT := fixture
include $(TOP)/mk/config.mk
.PHONY: test test-build test-run
test: test-run
test-build:
	@mkdir -p $(OUT)/host
test-run: test-build
	@rm -f $(OUT)/host/*.result; fail=0; \
	sh $(TOP)/factory/testlib/run-test.sh probe 20 '' $(OUT)/host/probe sh run.sh $(OUT) || fail=1; \
	echo 'INITECH_TEST_RUN 1'; test $$fail -eq 0
MAKE
cat > "$p/run.sh" <<'SH'
out=$1
mkdir -p "$out/fpc"
fpc -v0 -FE"$out/fpc" probe.pas > "$out/host/probe.log" 2>&1 || exit 2
got=$("$out/fpc/probe")
if [ "$got" = 7 ]; then echo '1 checks, 0 failures' >> "$out/host/probe.log"; exit 0; fi
echo "FAIL run.sh: probe printed $got, want 7" >> "$out/host/probe.log"
echo '1 checks, 1 failures' >> "$out/host/probe.log"; exit 1
SH
printf 'program Probe;\nbegin\n  writeln(7) { seven }\nend.\n' > "$p/probe.pas"
printf -- '--- a/probe.pas\n+++ b/probe.pas\n@@ -3 +3 @@\n-  writeln(7) { seven }\n+  writeln(7) { still seven }\n' > "$p/mutants/comment.patch"
printf -- '--- a/probe.pas\n+++ b/probe.pas\n@@ -3 +3 @@\n-  writeln(7) { seven }\n+  writeln(8) { seven }\n' > "$p/mutants/semantic.patch"
for case in comment:'0 killed, 1 survived, 0 errors' semantic:'1 killed, 0 survived, 0 errors'; do
 patch=${case%%:*} want=${case#*:}
 rc=0
 (cd "$p" && sh "$runner" "$root" fixture "$work/out-fpc-$patch" make "mutants/$patch.patch") > "$work/fpc-$patch.log" 2>&1 || rc=$?
 if grep -q "fixture: 1 mutants, $want" "$work/fpc-$patch.log"; then echo "ok infra mutants: real-fpc-$patch"
 else echo "FAIL infra mutants: real-fpc-$patch: want '$want'"; tail -n 8 "$work/fpc-$patch.log"; failed=1; fi
done
exit "$failed"
