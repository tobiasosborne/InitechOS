#!/bin/sh
# Lotus independent reader's complete record contract (wk1_ref.py).
# Check framing too: that reader permits short bodies and trailing bytes.
set -eu
file=$1 dump=$2
od -An -v -tu1 "$file" | awk '
 {for(i=1;i<=NF;i++) b[n++]=$i}
 END {
   if(n<10 || b[0]!=0 || b[1]!=0 || b[2]!=2 || b[3]!=0 || b[4]!=6 || b[5]!=4) exit 1
   p=0; eof=0
   while(p+4<=n) {
     op=b[p]+256*b[p+1]; len=b[p+2]+256*b[p+3]; p+=4
     if(p+len>n) exit 1
     if(op==0 && (p!=4 || len!=2)) exit 1
     if(op==13 && len!=7) exit 1
     if(op==14 && len!=13) exit 1
     if(op==16 && (len<15 || len!=15+b[p+13]+256*b[p+14])) exit 1
     p+=len
     if(op==1) {if(len!=0 || p!=n) exit 1; eof=1; break}
   }
   if(!eof || p!=n) exit 1
 }' || { echo 'FAIL worksheet: malformed record framing or trailing bytes' >&2; exit 1; }
awk '
 /^=== / {if($0 !~ / OK$/) bad=1; headers++;next}
 /^  INTEGER +len=7 +P:Default c0 r0 int=123$/ {a++;next}
 /^  NUMBER +len=13 +P:Default c1 r0 num=1\.5$/ {b++;next}
 /^  FORMULA +len=27 +P:Default c2 r0 cached=124\.5 len=12 rpn: RANGE\(Cr-2,Rr\+0\.\.Cr-1,Rr\+0\) @SUM\/1 RETURN$/ {c++;next}
 /^  (INTEGER|NUMBER|FORMULA|BLANK|LABEL|STRING) / {bad=1}
 /^  EOF +len=0 *$/ {eof++}
 /\[LEN MISMATCH\]|\[TRAIL |TRAILING BYTES|RPN-DECODE-FAILED/ {bad=1}
 END {exit (bad || headers!=1 || a!=1 || b!=1 || c!=1 || eof!=1)}
' "$dump" || { echo 'FAIL worksheet: wrong cells or reader diagnostics' >&2; exit 1; }
