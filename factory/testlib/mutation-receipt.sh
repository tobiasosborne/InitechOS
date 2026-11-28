#!/bin/sh
# Call only after the grader confirms its repeated semantic verdict.
set -eu
path=$(realpath --relative-base="$1" -e -- "$2")
printf 'INITECH_MUTANT_KILLED %s\n' "$path"
