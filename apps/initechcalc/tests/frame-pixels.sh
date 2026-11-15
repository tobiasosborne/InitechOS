#!/bin/sh
# The chart of a booted screen against goldens/frame: every expected point and
# box is read here from the reference package, never from InitechCalc.
#   $1 the frame-pixels program, $2 the screen (640 x 480 P6), $3 goldens/frame
# - PIE / PIEDEPTH (geometry.tsv, 640 columns): where the pie's pixels are.
# - the PIE ellipse and the chart labels (transcription.tsv): a slice face at
#   80% out at each value/sum middle (README question 10, recommended).
# - colours.tsv's chart-field samples, mapped through the still's visible
#   raster (geometry.tsv SCREEN): black. Its slice-top samples are not used:
#   the still's slices are not value/sum of a turn (question 10), so those
#   points fall on other slices or gaps of a value/sum pie.
# - the region searched for navy: the still's chart window (CHARTWIN), widened
#   to the screen's right edge for the Platinum chart window's extra width.
set -eu
export LC_ALL=C
bin=$1 shot=$2 ref=$3
g=$ref/geometry.tsv
pie=$(awk -F'\t' '$1=="PIE" { l=$11; t=$12; r=$13 } $1=="PIEDEPTH" { b=$14 } END { print l, t, r, b }' "$g")
ellipse=$(awk -F'\t' '$1=="PIE" { print ($11+$13)/2, ($12+$14)/2, ($13-$11)/2, ($14-$12)/2 }' "$g")
region=$(awk -F'\t' '$1=="CHARTWIN" { print $11, $12, 640, $14 }' "$g")
series=$(awk -F'\t' 'NR>1 && $2=="chart" && $4 ~ /^[0-9]+%$/ { v=$4; sub(/%$/,"",v); printf "%s ", v }' "$ref/transcription.tsv")
fields=$(awk -F'\t' -v g="$g" '
    BEGIN { while((getline l < g) > 0) { split(l,f,"\t"); if(f[1]=="SCREEN") { x0=f[3]; y0=f[4]; x1=f[5]; y1=f[6] } } }
    NR>1 && $3=="chart field (near black)" { printf "AT %d %d field ", (($4+$6)/2-x0)*640/(x1-x0), (($5+$7)/2-y0)*480/(y1-y0) }' "$ref/colours.tsv")
[ -n "$pie" ] && [ -n "$ellipse" ] && [ -n "$region" ] && [ -n "$series" ] && [ -n "$fields" ] || { echo 'FAIL frame pixels: reference package incomplete'; exit 1; }
# shellcheck disable=SC2086
"$bin" "$shot" REGION $region PIE $pie ELLIPSE $ellipse SERIES $series $fields
