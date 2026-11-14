# Input is objdump -d output from real ELF32 target objects, never literals.
function reject(why) {print "FAIL interface: " object ": " why;bad=1}
function only_programs(pattern, n,a,i,p) {
 n=split(artifacts,a,"[ \n]+");if(!n)return 0
 for(i=1;i<=n;i++){p=a[i];sub(/^.*\//,"",p);if(p !~ pattern)return 0}
 return 1
}
{
 line=tolower($0)
 if((input_kind=="asm" && line ~ /^[ \t]*(int3|into)([ \t;]|$)/) ||
    (input_kind!="asm" && line ~ /\t(int3|into)([ \t]|$)/)) {
  if(role!="startup")reject("forbidden interrupt opcode")
  next
 }
 if(input_kind=="asm") {
  sub(/;.*/,"",line)
  if(line ~ /^[ \t]*[a-z_][a-z_0-9]*[ \t]+equ[ \t]+/) {split(line,a," ");aliases[a[1]]=a[3];next}
  if(line !~ /^[ \t]*(int)[ \t]+/)next
 } else if(line !~ /int[ \t]+\$0x[0-9a-f]+/)next
 text=line;sub(/^.*int[ \t]+\$?/,"",text);sub(/[ \t].*$/,"",text)
 if(text in aliases)text=aliases[text]
 if(text ~ /^0x/)sub(/^0x/,"",text)
 else if(text ~ /^[0-9a-f]+h$/)sub(/h$/,"",text)
 else if(text ~ /^[0-9]+$/)text=sprintf("%x",text+0)
 v=text
 if(role=="startup")next
 if(v=="81" && (role=="desktop" || role=="gate" || role=="resident"))next
 if((v=="21" || v=="20") && (role=="dos" || role=="command"))next
 if(v=="13" && component=="dos" && only_programs("^(FORMAT|FDISK|SYS|DISKCOPY|DISKCOMP)[.]COM$"))next
 if((v=="25" || v=="26") && component=="dos" && only_programs("^(CHKDSK|LABEL)[.]COM$"))next
 if(v=="39" && component=="apps/initechbase" && object ~ /\/pal\/pal_dos\.o$/ && base39) {base_seen++;next}
 if(v=="21" && component=="apps/notepad" && object ~ /\/tenant\/notepad\.o$/ && notes21) {notes_seen++;next}
 reject("forbidden INT " v "h for " role)
}
END {
 if(base_seen>1 || notes_seen>1)reject("migration covers one diagnostic/console site only")
 exit bad
}
