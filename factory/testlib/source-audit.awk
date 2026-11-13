# Strip C comments lexically, preserving quoted include paths and line numbers.
FNR==1 {block=0}
{ out=""; quote=""; escape=0
  for(i=1;i<=length($0);i++) {
    c=substr($0,i,1); pair=substr($0,i,2)
    if(block) {
      if((block==1 && pair=="*/") || (block==2 && pair=="*)")) {block=0;i++}
      else if(block==3 && c=="}") block=0
      continue
    }
    if(quote!="") {out=out c; if(escape) escape=0; else if(c=="\\") escape=1; else if(c==quote) quote=""; continue}
    if(pair=="/*") {block=1;i++;out=out " ";continue}
    if(FILENAME ~ /\.pas$/ && pair=="(*") {block=2;i++;continue}
    if(FILENAME ~ /\.pas$/ && c=="{" && substr($0,i+1,1)!="$") {block=3;continue}
    if(pair=="//" || (FILENAME ~ /\.asm$/ && c==";")) break
    if(c=="\"" || c=="\047") quote=c
    out=out c
  }
  if(out ~ /^[ \t]*#[ \t]*include[ \t]*["<]\.\.\//) {
    print FILENAME ":" FNR ":" out; print "FAIL includes: product include escapes component"; bad=1
  }
  if(out ~ /^[ \t]*#[ \t]*include.*attic\//) {
    print FILENAME ":" FNR ":" out; print "FAIL attic: product include references attic"; bad=1
  }
  # Literal text and comments naming an old hook are not executable hooks.
  gsub(/"([^"\\]|\\.)*"|\047([^\047\\]|\\.)*\047/, "", out)
  if(out ~ pattern) {print FILENAME ":" FNR ":" out; print "FAIL mutation: product mutation hook"; bad=1}
}
END {exit bad}
