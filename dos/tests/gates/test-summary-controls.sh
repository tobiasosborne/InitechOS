set -eu
check_suite=summary-controls
. dos/tests/assertions.sh
mkdir -p build/summary-controls
counted() {
 name=$1; shift
 rc=0
 "$@" > "build/summary-controls/$name.log" 2>&1 || rc=$?
 [ "$rc" = 0 ] || error "$name healthy producer exited $rc"
 verdict=$(awk -f factory/testlib/summary-contract.awk "build/summary-controls/$name.log")
 check "$name completes a positive successful check summary" test "$verdict" = ok
}
counted batch-capacity "$HOST_TEST_OUTPUT/batch-capacity"
counted fdisk-session "$HOST_TEST_OUTPUT/fdisk-session"
counted dir-look build/test_dir_look dos/command/tests/dos33_dir.golden
counted tokenizer build/test_cmdline_fuzz --seed 1 --ops 320
counted IF-EXIST sh dos/tests/gates/test-if-exist-compat33.sh
# Real independent-reference failure: the DIR oracle must not claim a kill.
rc=0
build/test_dir_look build/summary-controls/no-such-golden > build/summary-controls/missing.log 2>&1 || rc=$?
check 'missing DIR reference remains setup exit 2' test "$rc" = 2
if grep '^ *FAIL' build/summary-controls/missing.log; then fail 'missing DIR reference emitted assertion'; fi
checks=$((checks+1))
# Exercise the actual batch host wrapper, after successful mtools setup.
printf '#!/bin/sh\nexit 2\n' > build/summary-controls/producer-error
chmod +x build/summary-controls/producer-error
rc=0
BATCH2_HOST="$PWD/build/summary-controls/producer-error" sh dos/command/tests/batch2.sh host > build/summary-controls/producer.log 2>&1 || rc=$?
check 'batch producer setup error remains exit 2' test "$rc" = 2
if grep '^ *FAIL' build/summary-controls/producer.log; then fail 'batch producer setup error emitted assertion'; fi
checks=$((checks+1))
# EDLIN consumes real independent model fixtures but rejects an uncompleted
# setup-failed producer before interpreting any missing transcript as a kill.
rc=0
EDLIN_ONLY=FIRST sh dos/edlin/tests/edlin.sh host build/summary-controls/edlin "$PWD/build/summary-controls/producer-error" "$PWD/build/edlin_model" > build/summary-controls/edlin-error.log 2>&1 || rc=$?
check 'EDLIN setup producer remains exit 2' test "$rc" = 2
if grep '^ *FAIL' build/summary-controls/edlin-error.log; then fail 'EDLIN setup producer emitted semantic assertion'; fi
checks=$((checks+1))
# Actual FORMAT driver and mtools setup, with only the host producer replaced.
component=$(realpath dos)
format_case=$PWD/build/summary-controls/format-case
mkdir -p "$format_case/build"
ln -s "$component" "$format_case/dos"
printf '#!/bin/sh\n: > producer-called\nexit 2\n' > "$format_case/build/formatsys_host"
chmod +x "$format_case/build/formatsys_host"
rc=0
(cd "$format_case"; sh dos/tests/gates/test-formatsys.sh) > build/summary-controls/format-error.log 2>&1 || rc=$?
check 'FORMAT setup producer was reached' test -f "$format_case/producer-called"
check 'FORMAT setup producer remains exit 2' test "$rc" = 2
check 'FORMAT setup error is explicit' grep -q -Fx 'ERROR formatsys: host producer did not complete' build/summary-controls/format-error.log
if grep '^ *FAIL' build/summary-controls/format-error.log; then fail 'FORMAT setup producer emitted semantic assertion'; fi
checks=$((checks+1))
rc=0
DIR_MATCH_HOST="$PWD/build/summary-controls/producer-error" sh dos/command/tests/dir_match.sh host > build/summary-controls/matcher-error.log 2>&1 || rc=$?
check 'DIR matcher setup failure survives sorting pipeline' test "$rc" = 2
if grep '^ *FAIL' build/summary-controls/matcher-error.log; then fail 'DIR matcher setup failure emitted assertion'; fi
checks=$((checks+1))
cat > build/summary-controls/invalid-producer <<'CASE'
#!/bin/sh
case $3 in /X*) exit 2;; esac
exec "$DOSUTIL_REAL" "$@"
CASE
chmod +x build/summary-controls/invalid-producer
rc=0
DOSUTIL_REAL="$PWD/build/dosutil_host" sh dos/common/tests/dosutil_diff.sh attrib "$PWD/build/summary-controls/invalid-producer" producer-control > build/summary-controls/invalid-producer.log 2>&1 || rc=$?
check 'ATTRIB invalid-argument setup failure is not accepted as refusal' test "$rc" = 2
if grep '^ *FAIL' build/summary-controls/invalid-producer.log; then fail 'ATTRIB invalid-argument setup failure emitted assertion'; fi
checks=$((checks+1))
complete
