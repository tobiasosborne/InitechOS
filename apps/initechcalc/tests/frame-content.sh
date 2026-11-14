#!/bin/sh
# The frame's content, graded against goldens/frame (README.md "Checks").
# Expected values come only from transcription.tsv and geometry.tsv, the hand
# reading of the still; the operator's note literal is quoted from the ruling.
# Three things are graded with the same rules:
#   1. the shipped ledger document, read here with awk (no product code);
#   2. what the real tenant drew, as tests/test_calc_frame.c reports it;
#   3. TABLES.TXT saved through the tenant's ordinary Save request.
set -eu
export LC_ALL=C
bin=$1
tsv=${FRAME_TSV:?set FRAME_TSV to goldens/frame/transcription.tsv}
geom=$(dirname "$tsv")/geometry.tsv
doc=${FRAME_DOC:-data/LEDGER.TXT}
# Operator ruling 2026-11-14, gap treatment A.
note='Blank entries were not recovered from the surviving material.'
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

# Expected cells: document row = fragment row + 9 (nine ruled rows above the
# fragment, transcription README question 7), label column L = 0.
awk -F'\t' 'NR>1 && $1 ~ /^LED\.R[0-9][0-9]\.C.$/ {
    c=substr($1,10,1); if(c=="L") c=0
    print substr($1,6,2)+9 "\t" c "\t" $4 "\t" $5 }' "$tsv" > "$tmp/cells"
awk -F'\t' 'NR>1 && $2=="chart" && $4 ~ /^[0-9]+%$/ { print $4 }' "$tsv" > "$tmp/labels"
awk -F'\t' '$1=="CANON.PIE116" { print $4 }' "$tsv" > "$tmp/canon"
awk -F'\t' '$1=="LEDWIN" || $1=="CHARTWIN" { print $11 "\t" $12 }' "$geom" > "$tmp/origins"

# 1. The document as the product's formats display it.
read_document() {
tr -d '\r' < "$1" | awk -F'\t' -v q="'" '
function grouped(s,   i, o, n) { n=length(s); o=""; for(i=1;i<=n;i++) { o=o substr(s,i,1); if((n-i)%3==0 && i<n) o=o "," } return o }
function shown(f,   d, v, neg, s, p) {
    if(substr(f,1,1)==q) return substr(f,2)
    if(substr(f,1,1)=="\\") return substr(f,2)=="=" ? "(double rule)" : "(repeat " substr(f,2) ")"
    if(f ~ /^\(G\) [0-9.]+-?$/) return substr(f,5)
    if(f !~ /^\([FC][0-9]\) [0-9.]+-?$/) return "(bad field " f ")"
    d=substr(f,3,1)+0; v=substr(f,6); neg=(substr(v,length(v))=="-"); if(neg) v=substr(v,1,length(v)-1)
    s=sprintf("%." d "f", v+0)
    if(substr(f,2,1)=="C") { p=index(s,"."); s="$" (p ? grouped(substr(s,1,p-1)) substr(s,p) : grouped(s)) }
    return neg ? s "-" : s }
NR==1 { if($0!="INITECH LEDGER") print "OTHER\tbad header" ; next }
$1=="WINDOW" { print "WINDOW\t1\t" $2 "\t" $3 "\t" $4 "\t" $5; next }
$1=="CHART"  { print "WINDOW\t2\t" $2 "\t" $3 "\t" $4 "\t" $5; next }
$1=="SERIES" { for(i=2;i<=NF;i++) print "LABEL\t" $i "%"; next }
$1=="NOTE"   { print "NOTE\t" substr($0,6); next }
$1=="ROWS"   { next }
$1 ~ /^[0-9]+$/ { for(i=2;i<=NF;i++) if($i!="") print "CELL\t" $1 "\t" i-2 "\t" shown($i); next }
{ print "OTHER\t" $0 }'
}
read_document "$doc" > "$tmp/document"

# 2. What the tenant drew.
"$bin" "$doc" "$tmp/TABLES.TXT" > "$tmp/drawn"
read_document "$tmp/TABLES.TXT" > "$tmp/saved"

grade() {   # $1 = name, $2 = observation file
    awk -F'\t' -v name="$1" -v note="$note" -v canon="$(cat "$tmp/canon")" \
        -v cells="$tmp/cells" -v labels="$tmp/labels" -v origins="$tmp/origins" '
    BEGIN {
        while((getline l < cells) > 0) { split(l,f,"\t"); k=f[1] SUBSEP f[2]
            if((f[4]=="certain" || f[4]=="probable") && f[3]!="") want[k]=f[3]; else empty[k]=f[4] }
        while((getline l < labels) > 0) { nl++; lab[nl]=l; v=l; sub(/%$/,"",v); val[nl]=v+0; sum+=v }
        while((getline l < origins) > 0) { no++; split(l,f,"\t"); ox[no]=f[1]; oy[no]=f[2] }
        bad=0 }
    function fail(s) { print "FAIL " name ": " s; bad++ }
    $1=="CELL" { k=$2 SUBSEP $3; seen[k]=1
        if(!(k in want)) { fail("row " $2 " col " $3 " shows \"" $4 "\" where the still shows " ((k in empty) ? empty[k] : "nothing (row above the fragment)")); next }
        got=$4; if(want[k]=="(double rule)" && got ~ /^===+$/) got="(double rule)"
        if(got!=want[k]) fail("row " $2 " col " $3 " shows \"" $4 "\", the still reads \"" want[k] "\"") }
    $1=="LABEL" { ng++; if(lab[ng]!=$2) fail("chart label " ng " is \"" $2 "\", the still reads \"" lab[ng] "\"") }
    $1=="NOTE" { notes++; if($2!=note) fail("note reads \"" $2 "\"") }
    $1=="WINDOW" { d1=$3-ox[$2]; d2=$4-oy[$2]; if(d1<-4 || d1>4 || d2<-4 || d2>4) fail("window " $2 " opens at " $3 "," $4 "; the still puts it at " ox[$2] "," oy[$2]) }
    $1=="RING" { ring=$2 }
    $1=="OTHER" || $1=="ALERT" || $1=="LAUNCH" || $1=="DROPPED" { fail("unexpected " $0) }
    END {
        for(k in want) if(!(k in seen)) { split(k,f,SUBSEP); fail("row " f[1] " col " f[2] " is empty; the still reads \"" want[k] "\"") }
        if(ng!=nl) fail(ng " chart labels, the still has " nl)
        if(sum "%" != canon) fail("the still'"'"'s labels sum to " sum "%, canon " canon)
        if(notes!=1) fail(notes+0 " notes")
        if(ring!="") {   # slices are value/sum of a turn: a face (N) at each middle,
                         # a gap (black B or a side S) at each boundary, within 4 degrees
            if(index(ring,"o")) fail("ring has colours other than navy and black")
            a=0
            for(i=1;i<=nl;i++) { b=a+360*val[i]/sum; m=int((a+b)/2+0.5)%360
                if(substr(ring,m+1,1)!="N") fail("no slice at " m " degrees, the middle of " lab[i])
                e=int(b+0.5); hit=0; for(j=e-4;j<=e+4;j++) if(substr(ring,(j+360)%360+1,1)!="N") hit=1
                if(!hit) fail("no gap at " e " degrees, after " lab[i])
                a=b } }
        else if(name=="drawn") fail("no ring")
        exit bad>0 }' "$2"
}
rc=0
grade document "$tmp/document" || rc=1
grade drawn "$tmp/drawn" || rc=1
grade saved "$tmp/saved" || rc=1
# Product sources, shipped document and actual draw records must all omit
# the retired placeholder. Construct the test token without a self-match.
forbidden='TODO_''FRAME'
for source in calc.c include/*.h include/*.json data/LEDGER.TXT; do
    if grep -F "$forbidden" "$source" >/dev/null; then
        echo "FAIL treatment A: placeholder in $source"
        rc=1
    fi
done
if grep -F "$forbidden" "$tmp/drawn" >/dev/null; then
    echo "FAIL treatment A: placeholder in screen draw records"
    rc=1
fi
cells=$(grep -c '^CELL' "$tmp/drawn" || :)
labels=$(grep -c '^LABEL' "$tmp/drawn" || :)
if [ "$rc" -eq 0 ]; then
    echo "ok frame content: shipped document, saved TABLES.TXT and drawing agree ($cells cells, $labels labels)"
fi
echo "test_calc_frame: $((cells + labels + 2)) checks, $rc failures"
exit "$rc"
