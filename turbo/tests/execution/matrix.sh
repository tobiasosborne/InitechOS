#!/bin/sh
# Keep each Bochs invocation within the existing 300-second bound, including
# the old resident compiler's serial trace. Every source/path still runs on
# both emulators and is compared independently with FPC by run.sh.
set -eu
cd "$(dirname "$0")/../../.."
if [ -n "${TURBO_EXEC_SOURCES:-}" ]; then
    exec sh turbo/tests/execution/run.sh
fi
batch= count=0 groups=0
for source in turbo/compiler/fixtures/gen*.pas turbo/tests/execution/*.pas; do
    batch="$batch $source"; count=$((count+1))
    if [ "$count" -eq 6 ]; then
        groups=$((groups+1)); echo "Execution group $groups:$batch"
        TURBO_EXEC_SOURCES=$batch sh turbo/tests/execution/run.sh
        batch= count=0
    fi
done
if [ "$count" -gt 0 ]; then
    groups=$((groups+1)); echo "Execution group $groups:$batch"
    TURBO_EXEC_SOURCES=$batch sh turbo/tests/execution/run.sh
fi
