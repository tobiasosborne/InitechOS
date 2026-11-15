#!/bin/sh
set -eu
entry=${6:-verdict}
printf '#!/bin/sh\nexec env TOP="%s" TURBO_TESTS_ROOT="%s" sh "%s/%s-test.sh" "%s" > "%s/%s.log" 2>&1\n' "$2" "$3" "$3" "$entry" "$4" "$5" "$entry" > "$1"
chmod +x "$1"
