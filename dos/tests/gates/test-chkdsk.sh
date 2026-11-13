set -eu
sh dos/chkdsk/tests/chkdsk_label.sh build/chkdsk/labels build/chkdsk_host
sh dos/chkdsk/tests/chkdsk.sh host build build/chkdsk_host build/chkdsk_fixture
