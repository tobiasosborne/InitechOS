#!/bin/sh
# The complete generated-execution matrix owns the Turbo product-image smoke.
set -eu
exec sh "$(dirname "$0")/../execution/run.sh" "$@"
