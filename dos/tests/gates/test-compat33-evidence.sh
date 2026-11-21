set -eu
# Verify provenance bytes only. Pending incompatibility comparisons are manual
(cd dos/find/tests/compat33 && sha256sum --quiet -c EVIDENCE.sha256) || exit 2
# until each fixed case has its own independent red-first product gate.
sh dos/command/tests/compat33/verify.sh
