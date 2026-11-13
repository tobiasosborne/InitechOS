# Inspect actual make graph paths (including generated-header prerequisites).
# Longest component path wins: flair/desktop is not flair.
BEGIN {n=split(components,c," "); split(declared,d," "); for(i in d) allow[d[i]]=1; allow[component]=1}
{
 line=$0
 while((at=index(line,root "/build/"))>0) {
   line=substr(line,at+length(root)+7); path=line
   sub(/[^A-Za-z0-9_\/.-].*$/, "", path)
   owner=""
   for(i=1;i<=n;i++) if((path==c[i] || index(path,c[i] "/")==1) && length(c[i])>length(owner)) owner=c[i]
   if(owner!="" && !allow[owner] && !seen[owner]++) {
     print "FAIL dependency: " component " reads build/" owner " without declaring it";bad=1
   }
 }
}
END {exit bad}
