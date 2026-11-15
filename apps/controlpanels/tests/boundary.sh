#!/bin/sh
# CLAUDE.md, Two interfaces: host tests obey the app's public dependencies too.
set -eu
top=$1; flags=$2; shift 2
checks=0
for flag in $flags; do
 case $flag in -I*) checks=$((checks + 1));; esac
 case $flag in
 -I.) ;; -Iinclude) ;;
 -I"$top"/sdk/*) ;;
 -I"$top"/flair/include|-I"$top"/flair/include/*|-I"$top"/flair/region/include) ;;
 -I"$top"/factory/include) ;;
 -I*) echo "FAIL private include search path: $flag" >&2; exit 1;;
 esac
done
# Inspect only the application's own translation units. The public hosted
# library is built and audited by its owner, with its own private dependencies.
for dependency do
 test -f "$dependency" || { echo "FAIL missing dependency file: $dependency" >&2; exit 1; }
 n=$(awk -v top="$top" '
 {for(i=1;i<=NF;i++) {
   p=$i
   if(p !~ /\.h$/)continue
   if(p ~ /^\.\.\//)bad=1
   if(p ~ /^\// && index(p,top "/apps/controlpanels/")!=1 &&
      index(p,top "/sdk/")!=1 && index(p,top "/flair/include/")!=1 &&
      index(p,top "/flair/region/include/")!=1 &&
      index(p,top "/factory/include/")!=1)bad=1
   if(bad){print "FAIL private header dependency: " p > "/dev/stderr";exit 1}
   n++
 }} END {if(!bad)print n+0}' "$dependency")
 checks=$((checks + n))
done
# The factory's counted form (factory/testlib/README.md, Kill contract).
[ "$checks" -gt 0 ] || { echo "FAIL Control Panels boundary checked nothing" >&2; exit 1; }
echo "Control Panels boundary: $checks checks, 0 failures (only own, SDK and declared public headers)"
