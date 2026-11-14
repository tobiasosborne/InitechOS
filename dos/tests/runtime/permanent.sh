#!/bin/sh
DOS_INTERACTIVE=0;DOS_EXIT_CONTROL=1;export DOS_INTERACTIVE DOS_EXIT_CONTROL
exec sh "$1/dos/tests/runtime/session.sh" "$@"
