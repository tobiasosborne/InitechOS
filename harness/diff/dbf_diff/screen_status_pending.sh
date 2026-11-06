#!/bin/sh
# PRD 6.6; initech-ebwsw. Real III+ cells/attributes/cursor, without masks.
set -eu
dir=harness/diff/dbf_diff
log=build/samir-status-screen-pending.log
case_dir=$dir/xbase_prog_diff/screen-mint/at-status-real
(cd "$case_dir" && sha256sum -c SHA256SUMS && sha256sum -c INPUTS.sha256)
sh "$dir/screen_mint_oracle.sh" "$case_dir"
rc=0
SCREEN_FORMS_SUITE=status-screen-pending SCREEN_FORMS_GATED=$dir/screen_status_pending.txt \
    sh "$dir/screen_forms_emu.sh" > "$log" 2>&1 || rc=$?
cat "$log"
# An incomplete guest or missing capture is an infrastructure failure, not
# evidence of the known layout mismatch. Require a comparison of each dump.
for dump in status off; do
    grep -Eq "^(GREEN: at-status-real/$dump$|RED: at-status-real/$dump: RED: first difference )" "$log" || {
        echo "!!! test-samir-status-screen-pending: $dump comparison missing (exit $rc)"
        exit 1
    }
done
red=$(grep -c '^RED: at-status-real/.*: RED: first difference ' "$log" || :)
printf '>>> test-samir-status-screen-pending PENDING (initech-ebwsw): %s dumps RED\n' "$red"
# Pending gates are opt-in, non-zero until accepted with mutation proof.
exit 1
