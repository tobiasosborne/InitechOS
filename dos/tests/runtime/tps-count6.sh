#!/bin/sh
TPS_SCENARIOS=ritual;export TPS_SCENARIOS
TPS_COUNT_CONTROL=1;export TPS_COUNT_CONTROL
exec sh "$1/dos/tests/runtime/tps.sh" "$@"
