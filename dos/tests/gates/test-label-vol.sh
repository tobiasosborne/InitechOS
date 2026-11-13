set -eu
build/dosutil_vol
sh dos/common/tests/dosutil_diff.sh label build/dosutil_host label
