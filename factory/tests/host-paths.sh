#!/bin/sh
# CLAUDE.md, The constitution: scripts must not depend on a user's home path.
set -eu
cd "$(dirname "$0")/../.."
find factory -type f -name '*.sh' -exec awk '
 /\/(home|Users)\/[^/[:space:]]+/ {
  printf "%s:%d: host-specific home path: %s\n", FILENAME, FNR, $0
  bad=1
 }
 END { exit bad }
' {} +
echo 'Factory scripts: no host-specific home paths'
