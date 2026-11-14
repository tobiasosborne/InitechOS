# all/artifact reachability comes from GNU make, including compiler .d edges.
/^[^#\t ][^=]*: / {
 key=$1;sub(/:$/,"",key);deps[key]=deps[key] " " substr($0,index($0,":")+1)
}
function visit(key, n,a,i) {
 if(seen[active SUBSEP key]++)return
 if(key ~ /\.(c|h|asm|pas|o|a|ld)$/) {
  if(membership)print key "\t" active;else if(!printed[key]++)print key
 }
 n=split(deps[key],a," ");for(i=1;i<=n;i++)if(a[i]!="|")visit(a[i])
}
END {if(roots==""){active="all";visit("all")}else {n=split(roots,r," ");for(j=1;j<=n;j++){active=r[j];visit(r[j])}}}
