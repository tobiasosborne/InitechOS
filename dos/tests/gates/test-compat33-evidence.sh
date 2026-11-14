set -eu
# Verify provenance bytes only. Pending incompatibility comparisons are manual
# until each fixed case has its own independent red-first product gate.
sh dos/command/tests/compat33/verify.sh
