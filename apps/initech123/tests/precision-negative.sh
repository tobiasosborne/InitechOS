#!/bin/sh
# Grade the deliberate 53-bit x87 refusal; retain the child log separately.
# Ref: real MINT05 literal bytes, ADR-0009 DEC-01a, test_i123_formula.c.
set -eu
log=${1:?precision log required}
rc=${2:?producer status required}
case $rc in 0|1) ;; *) echo "precision producer error: exit $rc; log $log" >&2; exit 2;; esac
test -r "$log" || exit 2
# A missing/partial producer is an error, never a precision assertion failure.
if ! counts=$(awk '
 /^INITECH_CHECKS [0-9]+ failures=[0-9]+$/ {n++; checks=$2; split($3,f,"="); failures=f[2]}
 END {if(n!=1 || checks+0<=0)exit 1; print checks,failures}' "$log"); then
 echo "precision producer has no completed verdict: $log" >&2; exit 2
fi
checks=0
need() {
 message=$1; shift; checks=$((checks+1))
 status=0; "$@" || status=$?
 if test "$status" -gt 1; then
  echo "precision reader error: exit $status; log $log" >&2; exit 2
 fi
 if test "$status" -eq 1; then
  echo "FAIL precision control: $message (raw log $log)"
  echo "INITECH_CHECKS $checks failures=1"
  exit 1
 fi
}
need '53-bit arithmetic must be refused' test "$rc" -eq 1
set -- $counts
need 'only the two recorded precision assertions fail' test "$2" -eq 2
need 'the producer used the deliberately wrong precision' grep -Fxq '  x87 arithmetic, control word 0x027f' "$log"
need 'the minted literal differs in the typed byte comparison' grep -Fxq '  FAIL A: MINT05 "1E99*10"' "$log"
need 'the minted literal has the recorded decompile refusal' grep -Fxq '  FAIL A: MINT05 (0,11) decompiles to "*10", typed "1E99*10"' "$log"
need 'the same literal cannot round trip at 53 bits' grep -Fxq '  FAIL B: MINT05 (0,11) "*10" does not recompile' "$log"
need 'no other typed or round-trip diagnostic is accepted' awk '/FAIL [AB]:/ {n++} END {exit n!=3}' "$log"
need 'only the recorded failure categories occur' awk '
 /RED-REASON/ {
  if($0=="  RED-REASON typed: 1 typed formulas differ from 123.EXE")typed++;
  else if($0=="  RED-REASON round-trip: 1 golden formulas do not round-trip")roundtrip++;
  else bad=1
 }
 END {exit bad || typed!=1 || roundtrip!=1}' "$log"
echo "53-bit x87 negative control verified; raw producer log: $log"
echo "INITECH_CHECKS $checks failures=0"
