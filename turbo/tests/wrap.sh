#!/bin/sh
set -eu
printf '#!/bin/sh\nexec env TOP="%s" TURBO_TESTS_ROOT="%s" sh "%s/verdict-test.sh" "%s" > "%s/verdict.log" 2>&1\n' "$2" "$3" "$3" "$4" "$5" > "$1"
chmod +x "$1"
