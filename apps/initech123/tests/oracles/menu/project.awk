# Independent full-RI minus reviewed omissions; never reads the product model.
# Inputs: omission pairs, reviewed literal descriptions, unchanged full RI dump.
BEGIN {FS=OFS="\t"}
FILENAME==ARGV[1] {omit[$1 SUBSEP $2]=1;next}
FILENAME==ARGV[2] {literal[$1 SUBSEP $2]=$3;next}
{parent[++n]=$1;name[n]=$2;desc[n]=$3;levels[$1]=1;ordinal[n]=siblings[$1]++;}
function path(p,n) {return p=="/"?p n:p "/" n}
function live(p, i,c) {
 if(p in cache)return cache[p];cache[p]=0;
 for(i=1;i<=n;i++)if(parent[i]==p&&!omit[p SUBSEP name[i]]) {
  c=path(p,name[i]);if((c in levels)?live(c):name[i]!="Quit")cache[p]=1;
 }
 return cache[p];
}
function kept(i, c) {
 c=path(parent[i],name[i]);
 return live(parent[i])&&!omit[parent[i] SUBSEP name[i]]&&(!(c in levels)||live(c));
}
function children(p, surviving, i,s) {
 s="";for(i=1;i<=n;i++)if(parent[i]==p&&(!surviving||kept(i)))s=s (s==""?"":"  ") name[i];return s;
}
END {
 for(key in omit) {
  found=0;for(i=1;i<=n;i++)if(key==parent[i] SUBSEP name[i])found++;
  if(found!=1){print "invalid reviewed omission: " key > "/dev/stderr";exit 1;}
 }
 for(i=1;i<=n;i++)if(kept(i)) {
  c=path(parent[i],name[i]);d=desc[i];key=parent[i] SUBSEP name[i];
  if((c in levels)&&d==children(c,0)&&d!=children(c,1)) {
   d=children(c,1);
   if(!(key in literal)||literal[key]!=d){print "unreviewed description: " c > "/dev/stderr";exit 1;}
   used[key]=1;
  } else if(key in literal){print "unnecessary description override: " c > "/dev/stderr";exit 1;}
  if(ids)print parent[i],name[i],d,ordinal[i];else print parent[i],name[i],d;
 }
 for(key in literal)if(!used[key]){print "unused description override: " key > "/dev/stderr";exit 1;}
}
