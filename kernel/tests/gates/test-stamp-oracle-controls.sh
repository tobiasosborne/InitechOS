set -eu
. kernel/tests/assertions.sh
grader=kernel/tests/disk/grade-stamps.sh
work=build/stamp-controls
mkdir -p "$work"
for name in data reference; do
 truncate -s 1474560 "$work/$name.img"
 mformat -i "$work/$name.img" -f 1440 ::
done
printf 'independent stamp fixture\n' > "$work/payload"
touch -t 198001010000.00 "$work/payload"
mcopy -m -i "$work/reference.img" "$work/payload" ::EARLY.TXT
mcopy -m -i "$work/data.img" "$work/payload" ::NEW.TXT
mmd -i "$work/data.img" ::SUB
mcopy -m -i "$work/data.img" "$work/payload" ::SUB/CHILD.TXT
touch -t 198802292359.58 "$work/payload"
mcopy -m -i "$work/reference.img" "$work/payload" ::LATE.TXT
mcopy -m -i "$work/data.img" "$work/payload" ::OLD.TXT
mcopy -m -i "$work/data.img" "$work/payload" ::TRUNC.TXT
printf 'STAMP HOST: real INT21 create/write/truncate/mkdir/read-close completed\n' > "$work/completed.log"
: > "$work/incomplete.log"
run_case() {
 expected_status=$1; semantic=$2; log=$3; reader=$4
 status=0
 PYTHON_FAT_REFERENCE="$reader" sh "$grader" "$work/data.img" "$work/reference.img" "$log" > "$work/result.log" 2>&1 || status=$?
 assert test "$status" -eq "$expected_status"
 evidence=0
 awk -f factory/testlib/host-verdict.awk "$work/result.log" > "$work/assertions" || evidence=$?
 if [ "$semantic" = yes ]; then assert test "$evidence" -eq 0; else assert test "$evidence" -ne 0; fi
}
run_case 0 no "$work/completed.log" "${PYTHON_FAT_REFERENCE:-python3}"
# A real independent timestamp mismatch must be a completed assertion failure.
mcopy -o -m -i "$work/data.img" "$work/payload" ::NEW.TXT
run_case 1 yes "$work/completed.log" "${PYTHON_FAT_REFERENCE:-python3}"
# Even a damaged image is not graded until its real producer has completed.
run_case 2 no "$work/incomplete.log" "${PYTHON_FAT_REFERENCE:-python3}"
run_case 1 no "$work/completed.log" /bin/false
# Enter the REAL fixture producer/helper; refusal after its completed real
# operations must remain an error even when all independent properties pass.
gate=kernel/tests/gates/test-dos-stamp-host.sh
cat > "$work/after-helper" <<'WRAPPER'
#!/bin/sh
build/dos_parity_host "$@" || exit 2
exit 7
WRAPPER
chmod +x "$work/after-helper"
status=0
DOS_PARITY_HOST="$PWD/$work/after-helper" sh "$gate" > "$work/producer-result.log" 2>&1 || status=$?
assert test "$status" -eq 2
assert grep -qx 'STAMP HOST: real INT21 create/write/truncate/mkdir/read-close completed' "$work/producer-result.log"
assert grep -qx 'INITECH_CHECKS 5 failures=0' "$work/producer-result.log"
evidence=0
awk -f factory/testlib/host-verdict.awk "$work/producer-result.log" > "$work/assertions" || evidence=$?
assert test "$evidence" -ne 0
finish
