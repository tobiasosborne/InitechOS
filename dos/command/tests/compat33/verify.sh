#!/bin/sh
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cd "${1:-$src}"
sha256sum --quiet -c EVIDENCE.sha256
echo 'GREEN compat33 retained evidence hashes'
