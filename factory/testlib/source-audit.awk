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
  # CLAUDE.md "Two interfaces": sole private resident entry header, until
  # separately loaded COMMAND.COM (Release 2, initech-1frx2).
  shell_entry=(component=="kernel" && (FILENAME=="./main.c" || FILENAME=="./startup.c" || FILENAME==here "/main.c" || FILENAME==here "/startup.c") &&
      out ~ /^[ \t]*#[ \t]*include[ \t]*"\.\.\/dos\/command\/entry.h"[ \t]*$/)
  if(out ~ /^[ \t]*#[ \t]*include[ \t]*["<]\.\.\// && !shell_entry) {
    print FILENAME ":" FNR ":" out; print "FAIL includes: product include escapes component"; bad=1
  }
  if(out ~ /^[ \t]*#[ \t]*include.*attic\//) {
    print FILENAME ":" FNR ":" out; print "FAIL attic: product include references attic"; bad=1
  }
  # Literal text and comments naming an old hook are not executable hooks.
  if(emit)print out
  gsub(/"([^"\\]|\\.)*"|\047([^\047\\]|\\.)*\047/, "", out)
  if(out ~ pattern) {print FILENAME ":" FNR ":" out; print "FAIL mutation: product mutation hook"; bad=1}
  if(macros!="") fault_branch(out)
}
# A conditional whose macro no checked-in build defines can only be switched
# on by a patch: a fault hook, whatever its name. A constant condition hides
# a branch the same way. macros names definable-macros.sh output.
BEGIN {
  if(macros!="") {while((getline name < macros) > 0) definable[name]=1; close(macros)}
  # Temporary rows (fault-branches.known): removed as each issue closes.
  FS0=FS; FS="\t"
  if(known!="") while((getline row < known) > 0) {
    if(row ~ /^#/ || split(row, f, "\t") != 4 || f[1]!=component) continue
    ledger[f[2] SUBSEP f[3]]=f[4]
  }
  FS=FS0
}
function fault_branch(line,  rest,n,t,i,names,pascal,word,file) {
  pascal=(FILENAME ~ /\.pas$/)
  if(line ~ /^[ \t]*#[ \t]*(if|ifdef|ifndef|elif|elifdef|elifndef)([ \t(!]|$)/) {
    rest=line; sub(/^[ \t]*#[ \t]*[a-z]+/, "", rest)
  } else if(line ~ /^[ \t]*%(ifdef|ifndef|elifdef|elifndef)[ \t]/) {
    rest=line; sub(/^[ \t]*%[a-z]+/, "", rest)
  } else if(pascal && toupper(line) ~ /\{\$(IFDEF|IFNDEF|IF|ELSEIF)[ \t]/) {
    rest=toupper(line); sub(/.*\{\$[A-Z]+/, "", rest); sub(/\}.*/, "", rest)
  } else return
  n=split(rest, t, /[^A-Za-z0-9_]+/); names=0
  for(i=1;i<=n;i++) {
    word=t[i]
    if(word !~ /^[A-Za-z_]/) continue
    if(word=="defined" || word ~ /^__has_/ || (pascal && word=="DEFINED")) continue
    names++
    if(!(word in definable)) {
      print FILENAME ":" FNR ":" line
      file=FILENAME; sub(/^\.\//, "", file); if(index(file, here "/")==1) file=substr(file, length(here)+2)
      if((file SUBSEP word) in ledger) {
        print "KNOWN fault branch: " word " (" ledger[file SUBSEP word] "; factory/testlib/fault-branches.known)"
        used[file SUBSEP word]=1; continue
      }
      print "FAIL fault branch: " word " is defined by no checked-in build; only a patch can enable it"
      bad=1
    }
  }
  if(!names) {print FILENAME ":" FNR ":" line; print "FAIL fault branch: constant condition"; bad=1}
}
END {
  for(k in ledger) if(!(k in used)) {
    split(k, f, SUBSEP)
    print "NOTE stale fault-branch row: " component " " f[1] " " f[2] " (" ledger[k] "): delete it"
  }
  exit bad
}
