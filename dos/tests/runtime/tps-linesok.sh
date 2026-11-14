#!/bin/sh
TPS_SCENARIOS=lines;export TPS_SCENARIOS
exec sh "$1/dos/tests/runtime/tps.sh" "$@"
