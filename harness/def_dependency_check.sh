#!/bin/sh
# Ref: initech-q8j; PRD 6.6; CLAUDE.md Rules 1/6/11.
# Reproduce the stale COLOR oracle with the current source tree in a temp copy.
set -eu
work=$(mktemp -d "${TMPDIR:-/tmp}/initech-def-deps-XXXXXX")
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM
git ls-files -z --cached --others --exclude-standard > "$work/files"
tar --null -T "$work/files" -cf "$work/source.tar"
tar -xf "$work/source.tar" -C "$work"
rm -f "$work/source.tar"
make=${MAKE:-make}
cd "$work"
"$make" --no-print-directory test-samir-set-color > baseline.log 2>&1 || {
    cat baseline.log; exit 1;
}
# Also exercise a FUTURE consumer and a kernel-pattern object, not a hand list
# of current consumers. Their only input is an independent little C fixture.
cat > consumer.mk <<'EOF'
$(BUILD)/future-def-consumer: future.c
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD)/kmain_def_probe.o: future.c
	$(CC) $(CFLAGS) -c -o $@ $<
EOF
printf 'int main(void) { return 0; }\n' > future.c
"$make" --no-print-directory -f Makefile -f consumer.mk build/future-def-consumer build/kmain_def_probe.o > consumers.log 2>&1 || {
    cat consumers.log; exit 1;
}
# Ordinary touch (not -B) must invalidate the hidden shared prerequisite.
sleep 1
touch os/samir/cmd/set_options.def
"$make" --no-print-directory -f Makefile -f consumer.mk build/future-def-consumer build/kmain_def_probe.o > touched.log 2>&1 || {
    cat touched.log; exit 1;
}
grep -qF -- '-o build/future-def-consumer' touched.log &&
grep -qF -- '-o build/kmain_def_probe.o' touched.log || {
    cat touched.log
    printf '!!! def-deps RED: touching the table did not rebuild future/kernel consumers\n' >&2
    exit 1
}
# Bring the original COLOR binary fully current after the touch, so the next
# check is invalidated by the changed keyword itself, not a previous rebuild.
"$make" --no-print-directory test-samir-set-color > after-touch.log 2>&1 || {
    cat after-touch.log; exit 1;
}
# Change only the keyword. The ordinary oracle must recompile and go RED.
sed 's/SET_OPT("COLOR", set_opt_color)/SET_OPT("COLOUR", set_opt_color)/' \
    os/samir/cmd/set_options.def > options.tmp
mv -f options.tmp os/samir/cmd/set_options.def
rc=0
"$make" --no-print-directory test-samir-set-color > colour.log 2>&1 || rc=$?
[ "$rc" -eq 2 ] && grep -qF -- '-o build/test_samir_set_color' colour.log &&
grep -qF 'FAIL' colour.log && grep -qF 'SET COLOR standard attribute reaches PAL' colour.log || {
    cat colour.log
    printf '!!! def-deps RED: COLOR->COLOUR stayed green or failed infrastructure (exit %s)\n' "$rc" >&2
    exit 1
}
printf '>>> def-deps: ordinary COLOR->COLOUR gate correctly RED (make exit %s)\n' "$rc"
grep 'checks,.*failures' colour.log
printf '>>> test-samir-def-deps: green (touch rebuilds future/kernel consumers; changed keyword rebuilds oracle)\n'
