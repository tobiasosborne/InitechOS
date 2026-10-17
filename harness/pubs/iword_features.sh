#!/bin/sh
# iword_features.sh -- CP-17 leg of test-pubs-iword (bead initech-ipu5.10):
# every feature a Workbook lesson lists (\LessonFeatures -> main.wpfeatures;
# the Feature List appendices are built from the same lists) is an entry of
# the InitechWord Reference (\begin{Feature} -> main.wpentries).
# usage: iword_features.sh <reference .wpentries> <workbook .wpfeatures>
set -eu
entries=$1; feats=$2
[ -s "$entries" ] || { printf '!!! test-pubs RED FEATURES: %s lists no entries\n' "$entries"; exit 1; }
[ -s "$feats" ] || { printf '!!! test-pubs RED FEATURES: %s lists no lesson features\n' "$feats"; exit 1; }
while IFS= read -r f; do
    [ -n "$f" ] || continue
    grep -qxF "$f" "$entries" || { printf '!!! test-pubs RED FEATURES: Workbook feature "%s" is not an entry of the Reference\n' "$f"; exit 1; }
done < "$feats"
printf '>>> test-pubs-iword: features green (%s lesson features, each an entry of the Reference)\n' "$(wc -l < "$feats")"
