#!/bin/sh
# Every declared external binding must resolve from the shared host archive.
set -eu
printf '#include "tbx.h"\n'
awk '!/^#/ && $3 >= 0 { printf "_Static_assert(%s == %s, \"stable trap\");\n", $1,$2;
 name=$1; sub(/^TBX_/,"TBX_ARGC_",name);
 printf "_Static_assert(%s == %s, \"stable arity\");\n",name,$3;
}' "$1"
# Link every external declaration, including the raw trap and clock alias.
awk '/^(TBX_TRAP_ABI )?int32_t tbx_[a-z_]+\(/ {
 match($0,/tbx_[a-z_]+\(/); line=substr($0,RSTART,RLENGTH-1);
 printf "__typeof__(&%s) volatile ref_%s = &%s;\n",line,line,line;
}' "$2"
printf 'int main(void) { return 0; }\n'
