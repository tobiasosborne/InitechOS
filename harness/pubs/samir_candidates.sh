#!/bin/sh
# PUB-REF-0001 GC-01 / CP-11 (beads initech-ipu5.8, initech-u01i): the
# candidate words of the shipped InitechBase surface, read from the very
# sources the dot-prompt dispatcher compiles -- never a second list.
# harness/pubs/samir_inventory.c asks the REAL dispatcher which of them it
# accepts; this script only proposes.
#
# A candidate is every element of a C string literal of the named sources
# that is one upper-case word ("ZAP") or two ("CLEAR MEMORY", which gives
# both its words), where a literal's elements are separated by \0.  Since
# SAMIR size round 3 (0d5e6bbf, initech-vn4i) the verb, keyword, function
# and SET-option names live in packed grammar tables walked by
# rt_kw_index / rt_kw_word / rt_kw_match (os/samir/core/rt.c), e.g.
#   rt_kw_index(verb, "TYPE\0RENAME\0ERASE\0...")      cmd/mutate.c
#   static const char names[] = "RECSIZE\0ERROR\0..."  core/fn_builtins.c
# so a whole-literal match ('"[A-Z][A-Z0-9]*"') saw only 76 words and lost
# ACCEPT, APPEND, DELETE, LIST... (initech-u01i).  Character literals are
# dropped first so that '"' cannot unbalance a line's string literals.
# The symbol commands (? ?? @ ! & && *) are not words; they are added here.
#
# usage: samir_candidates.sh <source.c|.h>...   (sorted, one per line)
set -eu
[ "$#" -gt 0 ] || { printf '!!! samir_candidates: no sources\n' >&2; exit 2; }
for f in "$@"; do [ -f "$f" ] || { printf '!!! samir_candidates: %s missing\n' "$f" >&2; exit 2; }; done
{
    sed -e "s/'\\\\.'//g" -e "s/'[^'\\\\]'//g" "$@" |
        grep -oE '"([^"\\]|\\.)*"' |
        sed -e 's/^"//' -e 's/"$//' -e 's/\\0/\n/g' |
        grep -E '^[A-Z][A-Z0-9]*( [A-Z][A-Z0-9]*)?$' | tr ' ' '\n' || true
    printf '%s\n' '?' '??' '@' '!' '&' '&&' '*'
} | LC_ALL=C sort -u
