#!/bin/sh
# Full smoke coverage is fixed by the registry, not by caller selectors.
set -eu
kind=$1; selected=$2; registered=$3
normalize() { printf '%s\n' "$1" | tr ' \t\n' '\n' | sed '/^$/d' | sort; }
names=$(normalize "$registered" | tr '\n' ' ')
[ -n "$(normalize "$selected")" ] || { echo "FAIL smoke: empty $kind selection" >&2; exit 1; }
for entry in $selected; do
 case " $names " in *" $entry "*) ;; *) echo "FAIL smoke: unknown $kind [$entry]" >&2; exit 1;; esac
done
if [ "${INITECH_FULL_SMOKE:-0}" = 1 ] && [ "$(normalize "$selected")" != "$(normalize "$registered")" ]; then
 echo "FAIL smoke: incomplete $kind selection [$selected]; required [$registered]" >&2; exit 1
fi
if [ "$(normalize "$selected")" != "$(normalize "$registered")" ]; then
 echo "SUBSET SMOKE: $kind [$selected] (not full completion)"
fi
