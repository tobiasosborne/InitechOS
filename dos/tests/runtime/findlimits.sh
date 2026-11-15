#!/bin/sh
export FILTER_PROGRAM=FIND
exec sh "$1/dos/tests/runtime/filterlimits.sh" "$@"
