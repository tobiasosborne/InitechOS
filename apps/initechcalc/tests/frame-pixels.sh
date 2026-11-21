#!/bin/sh
# A booted screen against goldens/frame: every expected point and box is read
# here from the reference package, never from InitechCalc or FLAIR.
#   $1 the frame-pixels program, $2 the screen (640 x 480 P6), $3 goldens/frame,
#   $4 the moment: "chart" (Calc open, before Save; the default), "copy"
#      (FILE COPY running in front of it) or "hourglass" (the busy cursor).
# chart:
# - CHARTWIN (geometry.tsv, 640 columns): the chart window's outline, exact.
# - PIE / PIEDEPTH: where the pie's pixels are, searched for inside CHARTWIN.
# - the PIE ellipse and the chart labels (transcription.tsv): a slice face at
#   80% out at each value/sum middle (README question 10, recommended).
# - colours.tsv's chart-field samples, mapped through the still's visible
#   raster (geometry.tsv SCREEN): black.
# - colours.tsv's slice samples (roles "slice blue" and "slice side (darker
#   blue)"), mapped the same way: each box's per-channel median is its
#   role's colour, the area-weighted median of the role's samples relative to
#   the photographed white (about #4282FF for faces, #334A87 for sides).
#   Which slice a box falls on does not matter (the still's slices are not
#   value/sum of a turn, question 10): every face is one blue, every side
#   the other. The same colours are what PIE, ELLIPSE and NOSLICE look for.
# copy:
# - CHARTWIN behind COPY, both outlines exact where not covered. COPY's foot
#   is the still's plus 11 rows: the live box keeps a Cancel button below the
#   bar, which the still lacks (GM3; flair/README.md, the stated departure).
# - COPYCAPTION: the caption's black text starts at the still's place.
# - COPYBAR: the progress bar's outline at the still's place.
# - no slice colour shows through COPY: it is in front of the chart.
# - colours.tsv filecopy_content and progress_fill, mapped through the still's
#   visible raster (geometry.tsv SCREEN): the per-channel median of each box
#   is the sample's colour relative to the photographed white, at full white
#   (white #FFFFFF; blue 0.27 0.35 1.00, about #4559FF). The still's fill
#   fraction cannot be read (README question 13) and the moment is taken at
#   half way (TENANT-FILECOPY-MID v>=50), so the fill is graded over the left
#   quarter of its sample, which the bar's fill covers by then.
# hourglass (a frame taken while the copy runs):
# - COPYHOURGLASS: the busy cursor is an hourglass at the still's place
#   (transcription CANON.HOURGLASS; the journey parks the pointer there).
#   FLAIR shields the cursor while it repaints the bar, so one frame may
#   catch it erased; tests/smoke/verify.sh asks this of several frames and
#   needs the hourglass in one.
set -eu
export LC_ALL=C
bin=$1 shot=$2 ref=$3 moment=${4:-chart}
g=$ref/geometry.tsv
box() { awk -F'\t' -v id="$1" -v foot="${2:-0}" '$1==id { print $11, $12, $13, $14+foot }' "$g"; }
chartwin=$(box CHARTWIN)
# The slice samples as $1 (SAMPLE or SLICEREF) arguments: the box on screen,
# the colour at full white (a channel brighter than the photographed white is
# full), the box's area on the still and the role.
slices() { awk -F'\t' -v g="$g" -v kw="$1" '
    BEGIN { while((getline l < g) > 0) { split(l,f,"\t"); if(f[1]=="SCREEN") { x0=f[3]; y0=f[4]; x1=f[5]; y1=f[6] } } }
    function full(v) { v=int(v*255+0.5); return v>255 ? 255 : v }
    NR>1 && ($3=="slice blue" || $3=="slice side (darker blue)") { split($9,r," ")
        printf "%s %d %d %d %d %d %d %d %d %s %s ", kw, ($4-x0)*640/(x1-x0), ($5-y0)*480/(y1-y0), ($6-x0)*640/(x1-x0), ($7-y0)*480/(y1-y0),
               full(r[1]), full(r[2]), full(r[3]), ($6-$4)*($7-$5), ($3=="slice blue") ? "face" : "side", $1 }' "$ref/colours.tsv"; }
case $moment in
chart)
    pie=$(awk -F'\t' '$1=="PIE" { l=$11; t=$12; r=$13 } $1=="PIEDEPTH" { b=$14 } END { print l, t, r, b }' "$g")
    ellipse=$(awk -F'\t' '$1=="PIE" { print ($11+$13)/2, ($12+$14)/2, ($13-$11)/2, ($14-$12)/2 }' "$g")
    series=$(awk -F'\t' 'NR>1 && $2=="chart" && $4 ~ /^[0-9]+%$/ { v=$4; sub(/%$/,"",v); printf "%s ", v }' "$ref/transcription.tsv")
    fields=$(awk -F'\t' -v g="$g" '
        BEGIN { while((getline l < g) > 0) { split(l,f,"\t"); if(f[1]=="SCREEN") { x0=f[3]; y0=f[4]; x1=f[5]; y1=f[6] } } }
        NR>1 && $3=="chart field (near black)" { printf "AT %d %d field ", (($4+$6)/2-x0)*640/(x1-x0), (($5+$7)/2-y0)*480/(y1-y0) }' "$ref/colours.tsv")
    samples=$(slices SAMPLE)
    [ -n "$chartwin" ] && [ -n "$pie" ] && [ -n "$ellipse" ] && [ -n "$series" ] && [ -n "$fields" ] && [ -n "$samples" ] || { echo 'FAIL frame pixels: reference package incomplete'; exit 1; }
    # shellcheck disable=SC2086
    "$bin" "$shot" $samples FRAME $chartwin REGION $chartwin PIE $pie ELLIPSE $ellipse SERIES $series $fields ;;
copy)
    copy=$(box COPY 11) caption=$(box COPYCAPTION) bar=$(box COPYBAR)
    # A sample's box in 640 x 480 and its relative colour at full white;
    # $2 is the fraction of its width graded, from the left.
    sample() { awk -F'\t' -v g="$g" -v id="$1" -v part="$2" '
        BEGIN { while((getline l < g) > 0) { split(l,f,"\t"); if(f[1]=="SCREEN") { x0=f[3]; y0=f[4]; x1=f[5]; y1=f[6] } } }
        $1==id { split($9,r," "); l=($4-x0)*640/(x1-x0); rt=($6-x0)*640/(x1-x0)
                 printf "%d %d %d %d %d %d %d %s", l, ($5-y0)*480/(y1-y0), l+(rt-l)*part, ($7-y0)*480/(y1-y0),
                        r[1]*255+0.5, r[2]*255+0.5, r[3]*255+0.5, id }' "$ref/colours.tsv"; }
    body=$(sample filecopy_content 1) fill=$(sample progress_fill 0.25) refs=$(slices SLICEREF)
    [ -n "$chartwin" ] && [ -n "$copy" ] && [ -n "$caption" ] && [ -n "$bar" ] && [ -n "$body" ] && [ -n "$fill" ] && [ -n "$refs" ] || { echo 'FAIL frame pixels: reference package incomplete'; exit 1; }
    # shellcheck disable=SC2086
    "$bin" "$shot" $refs FRAME $chartwin FRAME $copy TEXT $caption BAR $bar NOSLICE $copy COLOUR $body COLOUR $fill ;;
hourglass)
    glass=$(box COPYHOURGLASS)
    [ -n "$glass" ] || { echo 'FAIL frame pixels: reference package incomplete'; exit 1; }
    # shellcheck disable=SC2086
    "$bin" "$shot" HOURGLASS $glass ;;
*) echo "frame-pixels.sh: unknown moment $moment" >&2; exit 2 ;;
esac
