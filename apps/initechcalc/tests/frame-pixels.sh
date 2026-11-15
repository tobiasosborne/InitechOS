#!/bin/sh
# A booted screen against goldens/frame: every expected point and box is read
# here from the reference package, never from InitechCalc or FLAIR.
#   $1 the frame-pixels program, $2 the screen (640 x 480 P6), $3 goldens/frame,
#   $4 the moment: "chart" (Calc open, before Save; the default) or "copy"
#      (FILE COPY running in front of it).
# chart:
# - CHARTWIN (geometry.tsv, 640 columns): the chart window's outline, exact.
# - PIE / PIEDEPTH: where the pie's pixels are, searched for inside CHARTWIN.
# - the PIE ellipse and the chart labels (transcription.tsv): a slice face at
#   80% out at each value/sum middle (README question 10, recommended).
# - colours.tsv's chart-field samples, mapped through the still's visible
#   raster (geometry.tsv SCREEN): black. Its slice-top samples are not used:
#   the still's slices are not value/sum of a turn (question 10), so those
#   points fall on other slices or gaps of a value/sum pie.
# copy:
# - CHARTWIN behind COPY, both outlines exact where not covered. COPY's foot
#   is the still's plus 11 rows: the live box keeps a Cancel button below the
#   bar, which the still lacks (GM3; flair/README.md, the stated departure).
# - COPYCAPTION: the caption's black text starts at the still's place.
# - COPYBAR: the progress bar's outline at the still's place.
# - no slice navy shows through COPY: it is in front of the chart.
# Not graded, stated departures (goldens/frame/README.md): colours.tsv's
# progress_fill blue and filecopy_content white are FLAIR's Platinum bar and
# dialog grey; the slice blue is the Toolbox's navy (question 12).
set -eu
export LC_ALL=C
bin=$1 shot=$2 ref=$3 moment=${4:-chart}
g=$ref/geometry.tsv
box() { awk -F'\t' -v id="$1" -v foot="${2:-0}" '$1==id { print $11, $12, $13, $14+foot }' "$g"; }
chartwin=$(box CHARTWIN)
case $moment in
chart)
    pie=$(awk -F'\t' '$1=="PIE" { l=$11; t=$12; r=$13 } $1=="PIEDEPTH" { b=$14 } END { print l, t, r, b }' "$g")
    ellipse=$(awk -F'\t' '$1=="PIE" { print ($11+$13)/2, ($12+$14)/2, ($13-$11)/2, ($14-$12)/2 }' "$g")
    series=$(awk -F'\t' 'NR>1 && $2=="chart" && $4 ~ /^[0-9]+%$/ { v=$4; sub(/%$/,"",v); printf "%s ", v }' "$ref/transcription.tsv")
    fields=$(awk -F'\t' -v g="$g" '
        BEGIN { while((getline l < g) > 0) { split(l,f,"\t"); if(f[1]=="SCREEN") { x0=f[3]; y0=f[4]; x1=f[5]; y1=f[6] } } }
        NR>1 && $3=="chart field (near black)" { printf "AT %d %d field ", (($4+$6)/2-x0)*640/(x1-x0), (($5+$7)/2-y0)*480/(y1-y0) }' "$ref/colours.tsv")
    [ -n "$chartwin" ] && [ -n "$pie" ] && [ -n "$ellipse" ] && [ -n "$series" ] && [ -n "$fields" ] || { echo 'FAIL frame pixels: reference package incomplete'; exit 1; }
    # shellcheck disable=SC2086
    "$bin" "$shot" FRAME $chartwin REGION $chartwin PIE $pie ELLIPSE $ellipse SERIES $series $fields ;;
copy)
    copy=$(box COPY 11) caption=$(box COPYCAPTION) bar=$(box COPYBAR)
    [ -n "$chartwin" ] && [ -n "$copy" ] && [ -n "$caption" ] && [ -n "$bar" ] || { echo 'FAIL frame pixels: reference package incomplete'; exit 1; }
    # shellcheck disable=SC2086
    "$bin" "$shot" FRAME $chartwin FRAME $copy TEXT $caption BAR $bar NONAVY $copy ;;
*) echo "frame-pixels.sh: unknown moment $moment" >&2; exit 2 ;;
esac
