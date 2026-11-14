#!/bin/sh
# Write the host-test program for one tests/run.sh entry.
# Usage: wrap.sh PROGRAM TOP CORPUS COMPONENT-DIR OUT ENTRY
set -eu
printf '#!/bin/sh\nexec env TOP=%s TURBOPASCAL_DECOMP=%s sh %s/tests/run.sh %s %s\n' "$2" "$3" "$4" "$5" "$6" > "$1"
chmod +x "$1"
