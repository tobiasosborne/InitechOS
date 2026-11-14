#!/bin/sh
DOS_INTERACTIVE=0;export DOS_INTERACTIVE
exec sh "$1/dos/tests/runtime/session.sh" "$@"
