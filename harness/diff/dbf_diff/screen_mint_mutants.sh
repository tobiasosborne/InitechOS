#!/bin/sh
# PM review reproducers. Every RED must be the expected rejection, not a timeout.
set -eu
export LC_ALL=C TZ=UTC
root=$(pwd)
base="$root/harness/diff/dbf_diff/xbase_prog_diff/screen-mint"
work=$(mktemp -d "$root/build/screen-mint/review-tests.XXXXXX")
printf 'Review evidence: %s\n' "$work"
mint="$root/harness/diff/dbf_diff/screen_mint.sh"
tool="$root/build/screen_tool"
reject() {
    label=$1; expected=$2; pattern=$3; shift 3
    rc=0
    "$@" > "$work/$label.log" 2>&1 || rc=$?
    if [ "$rc" != "$expected" ] || ! grep -q "$pattern" "$work/$label.log" ||
       grep -q '^GREEN: real III+ mint\|^GREEN: SAMIR emu screen capture' "$work/$label.log"; then
        cat "$work/$label.log"
        printf 'FAIL: %s expected exit %s, got %s\n' "$label" "$expected" "$rc" >&2
        exit 1
    fi
    printf 'RED: %s rejected (exit %s)\n' "$label" "$rc"
}

# The review's original stale-output reproducer, including a valid old page.
cp -rf "$base/dot" "$work/stale"
"$tool" decode "$base/dot/structure.screen" "$work/stale/SETUP/DUMP0.BIN"
printf 'QUIT\r\n' > "$work/stale/SETUP/SETUP.PRG"
reject stale-capture 2 'reserved SETUP name' sh "$mint" "$work/stale" "$work/stale-out"
rm -f "$work/stale/SETUP/DUMP0.BIN"
for reserved in dump7.bin capture.ok dbase.exe DBASEINL.OVL screen.com events.bin MINT.PRG; do
    printf 'payload\n' > "$work/stale/SETUP/$reserved"
    reject "reserved-$reserved" 2 'reserved SETUP name' "$tool" check-setup "$work/stale/SETUP"
    rm -f "$work/stale/SETUP/$reserved"
done
# Early QUIT returns DOSBox success but never installs the TSR or saves a page.
reject missing-save-marker 1 'no fresh successful-save marker' sh "$mint" "$work/stale" "$work/early-out"
# Make DOS create fail for DUMP0.BIN after the real helper has captured it.
cp -rf "$base/dot" "$work/save-failure"
printf 'RUN MD DUMP0.BIN\r\nRETURN\r\n' > "$work/save-failure/SETUP/SETUP.PRG"
reject failed-save-marker 1 'no fresh successful-save marker' sh "$mint" "$work/save-failure" "$work/save-failure-out"

# Historical pin, actual independent executable, compiled events, and helper.
oracle="$root/harness/diff/dbf_diff/screen_mint_oracle.sh"
cp -rf "$base/dot" "$work/oracle"
sed '1s/^[0-9a-f]\{64\}/0000000000000000000000000000000000000000000000000000000000000000/' \
    "$work/oracle/ORACLE.sha256" > "$work/pin"
mv -f "$work/pin" "$work/oracle/ORACLE.sha256"
reject oracle-hash 1 '^RED: oracle provenance mismatch' sh "$oracle" "$work/oracle"
mkdir "$work/altered-store"
cp -rf "$work/oracle" "$work/altered-store/dot"
reject oracle-self-test-gate 1 '^RED: oracle provenance mismatch' sh \
    harness/diff/dbf_diff/screen_mint_test.sh "$work/altered-store"
reject oracle-repro-gate 1 '^RED: oracle provenance mismatch' sh \
    harness/diff/dbf_diff/screen_mint_repro.sh "$work/altered-store"
ref=$(cd "${DBASE3_DECOMP:-../dbase3-decomp}" && pwd)
mkdir -p "$work/ref/mint/work"
for f in DBASE.EXE DBASEINL.OVL DBASE.MSG DBASE.OVL; do cp -f "$ref/mint/work/$f" "$work/ref/mint/work/$f"; done
printf X | dd of="$work/ref/mint/work/DBASE.EXE" bs=1 count=1 conv=notrunc status=none
reject oracle-binary 1 '^RED: oracle provenance mismatch' env DBASE3_DECOMP="$work/ref" sh "$oracle" "$base/dot"
cp -f "$base/dot/ORACLE.sha256" "$work/oracle/ORACLE.sha256"
sed 's/TEXT DISPLAY STRUCTURE/TEXT DISPLAY/' "$base/dot/KEYS" > "$work/oracle/KEYS"
reject oracle-events 1 '^RED: oracle provenance mismatch' sh "$oracle" "$work/oracle"
# Isolate the helper mutation in a miniature root; never modify shared builds.
mkdir -p "$work/clone/build" "$work/clone/harness/diff/dbf_diff"
cp -f build/screen_tool build/SCREEN.COM "$work/clone/build/"
cp -f "$mint" "$oracle" "$work/clone/harness/diff/dbf_diff/"
printf X | dd of="$work/clone/build/SCREEN.COM" bs=1 count=1 conv=notrunc status=none
(cd "$work/clone" && reject oracle-helper 1 '^RED: oracle provenance mismatch' env DBASE3_DECOMP="$ref" sh "$oracle" "$base/dot")

# Alter host metadata and available host space between otherwise identical runs.
cp -rf "$base/dot" "$work/dir-case"
cat > "$work/dir-case/KEYS" <<'EOF'
WAIT 90
TEXT RUN DIR CLIENTS.DBF
KEY ENTER
WAIT 36
DUMP dir
TEXT DIR LIKE *.PRG
KEY ENTER
WAIT 36
DUMP space
TEXT QUIT
KEY ENTER
WAIT 18
EOF
printf 'dir\nspace\n' > "$work/dir-case/DUMPS"
find "$work/dir-case/SETUP" -exec touch -h -t 200101010101.00 {} +
sh "$mint" "$work/dir-case" "$work/dir-run1"
find "$work/dir-case/SETUP" -exec touch -h -t 202510250938.00 {} +
dd if=/dev/zero of="$work/host-space" bs=1048576 count=1 status=none
sh "$mint" "$work/dir-case" "$work/dir-run2"
for dump in dir space; do
    cmp "$work/dir-run1/$dump.screen" "$work/dir-run2/$dump.screen"
done
for manifest in SHA256SUMS INPUTS.sha256 ORACLE.sha256; do
    cmp "$work/dir-run1/$manifest" "$work/dir-run2/$manifest"
done
printf 'RUN DIR run 1\n'; cat "$work/dir-run1/SHA256SUMS"
printf 'RUN DIR run 2\n'; cat "$work/dir-run2/SHA256SUMS"
printf 'GREEN: RUN DIR and native DIR byte-identical despite changed host timestamps and space\n'

# A completed capture followed by nonzero helper status must never be GREEN.
cat > "$work/fail-after-save" <<'EOF'
#!/bin/sh
set -eu
build/screen_capture "$@"
cp -f "$4" "$SCREEN_PROBE_SAVED"
exit 1
EOF
chmod 755 "$work/fail-after-save"
reject helper-failed-after-save 1 '^RED: SAMIR harness failed' env SCREEN_CAPTURE_TOOL="$work/fail-after-save" \
    SCREEN_PROBE_SAVED="$work/completed.screen" SCREEN_SAMIR_OUT="$work/failed-helper" \
    sh harness/diff/dbf_diff/screen_mint_samir.sh
test -s "$work/completed.screen"
# Exit 0 and a formatted file alone are insufficient without capture completion.
cat > "$work/no-capture" <<'EOF'
#!/bin/sh
cp -f harness/diff/dbf_diff/xbase_prog_diff/screen-mint/dot/structure.screen "$4"
EOF
chmod 755 "$work/no-capture"
reject incomplete-capture 1 '^RED: SAMIR screen capture did not complete' env SCREEN_CAPTURE_TOOL="$work/no-capture" \
    SCREEN_SAMIR_OUT="$work/incomplete" sh harness/diff/dbf_diff/screen_mint_samir.sh
# Exercise the full harness, in addition to the 120ms fork/kill/reap unit gate.
printf '#!/bin/sh\nexec sleep 20\n' > "$work/stall"
chmod 755 "$work/stall"
reject stalled-helper 1 'helper deadline expired (killed and reaped)' env SCREEN_CAPTURE_TOOL="$work/stall" \
    SCREEN_SAMIR_TIMEOUT_MS=5000 SCREEN_SAMIR_OUT="$work/stalled" \
    timeout -k 1 9 sh harness/diff/dbf_diff/screen_mint_samir.sh
mkdir "$work/record-stalled"
cp -f build/samir_list.img "$work/record-stalled/data.img"
reject stalled-record-helper 1 'helper deadline expired (killed and reaped)' env \
    INITECH_QMP_STDIO=1 INITECH_TEXT_SCREEN_TOOL="$work/stall" \
    INITECH_TEXT_SCREEN_ELF=build/kernel_shell_secondary.elf \
    timeout -k 1 9 build/qemu_harness --disk build/tracer_secondary.img \
    --disk2 "$work/record-stalled/data.img" --name stalled --out "$work/record-stalled" \
    --timeout-ms 5000 --keys e,x,i,t,ret --keys-after SHELL-READY --record
printf 'GREEN: all five review blockers mutation-proven\n'
