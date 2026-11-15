#!/bin/sh
export FILTER_PROGRAM=SORT
exec sh "$1/dos/tests/runtime/filterlimits.sh" "$@"
