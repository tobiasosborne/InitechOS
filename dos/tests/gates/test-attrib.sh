set -eu
sh dos/common/tests/dosutil_diff.sh attrib build/dosutil_host attrib
printf '>>> test-attrib: green (oracle completed)\n'
