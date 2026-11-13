#!/bin/sh
# Compatibility entry for the original vertical-slice checker.
set -eu
exec sh "$(dirname "$0")/../apps/initech123/tests/smoke/verify.sh" "$@"
