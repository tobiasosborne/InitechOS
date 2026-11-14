#!/bin/sh
TPS_SCENARIOS=backup;export TPS_SCENARIOS
exec sh "$1/dos/tests/runtime/tps.sh" "$@"
