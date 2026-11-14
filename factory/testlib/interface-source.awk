# Comment/literal-stripped own production C identifiers against the provider's
# published resident services. This also catches header-inlined direct calls.
BEGIN {n=split(shell_services,a," ");for(i=1;i<=n;i++)shell[a[i]]=1}
FILENAME==ARGV[1] {for(i=4;i<=NF;i++)service[$i]=1;next}
FILENAME==ARGV[2] {for(i=1;i<=NF;i++)pure[$i]=1;next}
{
 line=$0;gsub(/"([^"\\]|\\.)*"|\047([^\047\\]|\\.)*\047/,"",line)
 n=split(line,a,/[^a-zA-Z0-9_]+/)
 for(i=1;i<=n;i++)if(service[a[i]] && !pure[a[i]] && !seen[a[i]]++) {
  if(role=="resident" || role=="startup" || (component=="dos" && source ~ /\/command\// && shell[a[i]]))continue
  if(component=="dos" && source ~ /\/fdisk\/fdisk_program.c$/ && fdisk_reset && (a[i]=="kernel_reset" || a[i]=="kernel_halt"))continue
  print "FAIL interface: " source ": direct resident service " a[i];bad=1
 }
}
END {exit bad}
