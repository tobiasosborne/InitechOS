set -eu
if ! sh kernel/tests/disk/copy_stamp.sh host; then echo 'FAIL dos-stamp-copy: copy_stamp host check failed (output above)'; echo 'INITECH_CHECKS 1 failures=1'; exit 1; fi
