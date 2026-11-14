#!/bin/sh
TPS_SCENARIOS=full;export TPS_SCENARIOS
exec sh "$1/dos/tests/runtime/tps.sh" "$@"
