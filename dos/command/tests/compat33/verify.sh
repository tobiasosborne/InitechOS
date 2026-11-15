#!/bin/sh
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cd "${1:-$src}"
sha256sum --quiet -c EVIDENCE.sha256 || { echo "ERROR compat33: unavailable or corrupt reference capture" >&2; exit 2; }
echo 'GREEN compat33 retained evidence hashes'
echo 'INITECH_CHECKS 1 failures=0'
