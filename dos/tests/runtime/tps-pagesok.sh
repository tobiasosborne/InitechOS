#!/bin/sh
TPS_SCENARIOS=pages;export TPS_SCENARIOS
exec sh "$1/dos/tests/runtime/tps.sh" "$@"
