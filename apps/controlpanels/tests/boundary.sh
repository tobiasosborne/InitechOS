#!/bin/sh
# CLAUDE.md, Two interfaces: host tests obey the app's public dependencies too.
set -eu
top=$1; flags=$2; shift 2
for flag in $flags; do
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
 test -f "$dependency"
 awk -v top="$top" '
 {for(i=1;i<=NF;i++) {
   p=$i
   if(p !~ /\.h$/)continue
   if(p ~ /^\.\.\//)bad=1
   if(p ~ /^\// && index(p,top "/apps/controlpanels/")!=1 &&
      index(p,top "/sdk/")!=1 && index(p,top "/flair/include/")!=1 &&
      index(p,top "/flair/region/include/")!=1 &&
      index(p,top "/factory/include/")!=1)bad=1
   if(bad){print "FAIL private header dependency: " p;exit 1}
 }}' "$dependency"
done
echo 'Control Panels boundary: only own, SDK and declared public headers'
