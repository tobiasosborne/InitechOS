#!/bin/sh
# CLAUDE.md "Two interfaces": declarations document authority; they cannot
# grant disk applications a resident binding or a private kernel service.
set -eu
root=$1 component=$2 database=$3 out=$4 artifacts=${5:-}
mkdir -p "$out"
helper=$root/factory/testlib
bad=0
fail() { echo "FAIL interface: $*"; bad=1; }
own_path() {
 # Longest component ownership: FLAIR's two registered children are providers.
 case $component:$1 in
  flair:"$root/flair/region/"*|flair:"$root/build/flair/region/"*|flair:"$root/flair/desktop/"*|flair:"$root/build/flair/desktop/"*) return 1;;
 esac
 case $1 in "$root/$component/"*|"$root/build/$component/"*) return 0;; esac
 return 1
}
section() {
 awk '/^<!-- interface-declarations -->$/ {on=1;begins++;next} /^<!-- end-interface-declarations -->$/ {on=0;ends++} on && /^Interface [a-z-]+:/ {print}
 END {if(begins!=1 || ends!=1 || on){print "FAIL interface: require one complete declaration section in " FILENAME > "/dev/stderr";exit 1}}' "$1"
}
section README.md > "$out/declarations"
[ -s "$out/declarations" ] || fail "$component missing README interface declarations"
# Roles are fixed by product ownership, not chosen by the caller's README.
case $component in
 boot|kernel|system) role=startup;;
 flair|flair/desktop) role=resident;;
 flair/region) role=pure;;
 sdk) role=gate;;
 dos|apps/initechbase|turbo|turbo/seed|turbo/compiler) role=dos;;
 apps/*) role=desktop;;
 *) fail "unknown product component $component"; role=unknown;;
esac
grep -qx "Interface role: $role" "$out/declarations" || fail "$component role must be $role"
sh "$helper/interface-migrations.sh" "$component" "$out/declarations" || bad=1
shell_services='fileio_fat_volume_label int21_batch_checkpoint int21_current_drive int21_directory_exists int21_directory_path int21_get_psp int21_io_drive int21_same_file int21_select_path int21_set_break_recovery int21_write_fault kernel_graphical_environment'
if [ "$component" = dos ]; then
 grep -qx "Interface shell-services: $shell_services" "$out/declarations" || fail 'resident shell must list exactly its existing temporary service calls'
fi
: > "$out/symbols.allowed"
: > "$out/bindings.allowed"
: > "$out/headers.allowed"
: > "$out/sources.allowed"
: > "$out/link-layout"
: > "$out/product-elf"
: > "$out/services"
for provider in kernel flair; do
 section "$root/$provider/README.md" > "$out/provider"
 awk '$2=="export-binding:" {print}' "$out/provider" >> "$out/services"
done
while read -r word kind contract rest; do
 case $kind in
 role:) continue;;
 use-pure:|use-binding:|use-records:|use-source:|use-gate:)
  case $contract in /*|*..*|*[!a-zA-Z0-9_./-]*) fail "invalid contract $contract"; continue;; esac
  provider=${contract%%/include/*}
  case $contract in build/flair/generated/*) provider=flair;; build/kernel/*) provider=kernel;; esac
  case $kind in
   use-source:) provider=${contract%/*};;
   use-gate:) [ "$contract" = sdk/include/tbx.h ] || fail "unknown gate $contract";;
  esac
  case $kind:$contract in use-binding:*.c|use-binding:*.asm) provider=${contract%/*};; esac
  if [ "$kind" = use-binding: ]; then
   case $role:$component in startup:*|resident:*|dos:dos) :;; *) fail "$component cannot use resident binding $contract";; esac
  fi
  [ -f "$root/$contract" ] || { fail "missing contract $contract"; continue; }
  case $kind:$contract in
   use-source:*|use-binding:*.c|use-binding:*.asm|*:*/include/*.h|*:build/flair/generated/*.h|*:build/kernel/dos_messages.h) :;;
   *) fail "private contract $contract"; continue;;
  esac
  if [ "$kind" = use-gate: ]; then
   find "$root/sdk/include" -name '*.h' >> "$out/headers.allowed"
   cat "$root"/sdk/include/*.h >> "$out/symbols.allowed"
   continue
  fi
  export_kind=$(printf '%s' "$kind" | sed 's/use-/export-/')
  [ -f "$root/$provider/README.md" ] || { fail "missing provider $provider"; continue; }
  section "$root/$provider/README.md" > "$out/provider"
  # Provider exports use a root-relative path and an exact symbol list.
  entry=$(awk -v kind="$export_kind" -v path="$contract" '$2==kind && $3==path {print}' "$out/provider")
  [ -n "$entry" ] || { fail "unpublished $kind $contract"; continue; }
  if [ "$kind" = use-source: ]; then
   printf '%s\n' "$root/$contract" >> "$out/sources.allowed"
  else
   printf '%s\n' "$root/$contract" >> "$out/headers.allowed"
   case $kind in
    use-records:) :;;
    use-binding:) printf '%s\n' "$entry" | cut -d' ' -f4- >> "$out/bindings.allowed";;
    *) printf '%s\n' "$entry" | cut -d' ' -f4- >> "$out/symbols.allowed";;
   esac
  fi;;
 callback:)
  case $component:$contract in
   turbo/seed:pas_main) :;;
   sdk:tbx_app_event) [ "$rest" = 'tbx_app_main tbx_app_name' ] || fail 'SDK callbacks must match published ABI';;
   *) fail "unknown callback $contract";;
  esac
  printf '%s %s\n' "$contract" "$rest" >> "$out/symbols.allowed";;
 link-layout:)
  [ "$component:$contract" = sdk:sdk/rt/tenant.ld ] || fail "unknown library link layout $contract"
  printf '%s\n' "$root/$contract" >> "$out/link-layout";;
 product-elf:)
  [ "$component:$contract" = turbo:build/turbo/compiler/extended/zero.elf ] || fail "unknown packaged product ELF $contract"
  printf '%s\n' "$root/$contract" >> "$out/product-elf";;
 compiler-runtime:)
  [ "$component:$contract" = apps/initechbase:__divdi3 ] || fail "unknown compiler runtime $contract"
  printf '%s\n' "$contract" >> "$out/symbols.allowed";;
 export-pure:|export-binding:|export-records:|export-source:|temporary:|migration:|shell-services:) :;;
 *) fail "unknown declaration $word $kind";;
 esac
done < "$out/declarations"
# The two law-authorized temporary exceptions are separate from migration debt.
if [ "$component" = kernel ]; then
 grep -qx 'Interface temporary: resident-command initech-1frx2' "$out/declarations" || fail 'resident command entry must declare its removal event'
 printf '%s\n' "$root/dos/command/entry.h" >> "$out/headers.allowed"
 cat "$root/dos/command/entry.h" >> "$out/symbols.allowed"
 echo 'interface exception: resident command entry (initech-1frx2; ends at separately loaded COMMAND.COM, Release 2)'
fi
if [ "$component" = flair/desktop ] && grep -Eq '^Interface use-binding: kernel/include/(fat12|fileio_fat)[.]h$' "$out/declarations"; then
 grep -qx 'Interface temporary: finder-fat initech-02siz' "$out/declarations" || fail 'Finder FAT binding must declare its removal event'
 echo 'interface exception: Finder published FAT (initech-02siz; ends when Finder operations use DOS)'
fi
if [ "$component" = dos ]; then
 artifacts="$artifacts $root/build/dos/command-resident.o $root/build/dos/batch.o $root/build/dos/env.o"
fi
awk -v roots="$artifacts" -f "$helper/interface-graph.awk" "$database" > "$out/graph.paths"
awk -v roots="$artifacts" -v membership=1 -f "$helper/interface-graph.awk" "$database" > "$out/graph.members"
[ ! -f "$out/link-layout" ] || cat "$out/link-layout" >> "$out/graph.paths"
cat "$out/product-elf" >> "$out/graph.paths"
 : > "$out/objects"; : > "$out/headers"; : > "$out/defs"; : > "$out/refs"
while read -r path; do
 original=$path
 case $path in /*) :;; *) path=$PWD/$path;; esac
 path=$(realpath -m "$path")
 if [ "${path##*.}" = ld ] && [ -f "$path" ]; then
  awk '{line=$0;while(match(line,/[A-Za-z_][A-Za-z_0-9]*[ \t]*=/)){s=substr(line,RSTART,RLENGTH);sub(/[ \t]*=$/,"",s);print s;line=substr(line,RSTART+RLENGTH)}}' "$path" >> "$out/symbols.allowed"
 fi
 own_path "$path" || continue
 case $path in
 *.c)
  awk -v pattern='a^' -v emit=1 -v component="$component" -v here="$PWD" -f "$helper/source-audit.awk" "$path" > "$out/source" || bad=1
  awk -v component="$component" -v role="$role" -v source="$path" \
   -v shell_services="$shell_services" \
   -v fdisk_reset="$(grep -c '^Interface migration: fdisk-reset initech-q9p80$' "$out/declarations" || :)" \
   -f "$helper/interface-source.awk" "$out/services" "$out/symbols.allowed" "$out/source" || bad=1
  ;;
 *.asm)
  [ "$role" != startup ] || continue
  # Turbo's wildcard runtime prerequisite also names the Multiboot judging
  # startup. The shipped program is checked from its actual packaged ELF.
  if [ "$component" = turbo ]; then
   [ -s "$out/product-elf" ] || fail 'Turbo must name its packaged product ELF'
   continue
  fi
  nasm -E -I"$root/build/kernel/" "$path" > "$out/assembly" || { fail "cannot preprocess $path"; continue; }
  members=$(awk -F '\t' -v p="$path" -v original="$original" '$1==p || $1==original {print $2}' "$out/graph.members")
  awk -v component="$component" -v role="$role" -v object="$path" -v input_kind=asm -v artifacts="$members" \
   -f "$helper/interface-vectors.awk" "$out/assembly" || bad=1
  ;;
 *.o|*.elf)
  [ -f "$path" ] || { fail "missing target object $path"; continue; }
  objdump -f "$path" > "$out/format"
  grep -q 'file format elf32-i386' "$out/format" || continue
  printf '%s\n' "$path" >> "$out/objects"
  nm -A -u "$path" >> "$out/refs"
  object_role=$role
  case $component:$path in dos:*/command-resident.o|dos:*/batch.o|dos:*/env.o) object_role=command;; esac
  objdump -d "$path" > "$out/disassembly"
  members=$(awk -F '\t' -v p="$path" -v original="$original" '$1==p || $1==original {print $2}' "$out/graph.members")
  awk -v component="$component" -v role="$object_role" -v object="$path" \
   -v artifacts="$members" \
   -v base39="$(grep -c '^Interface migration: base-console39 initech-qu7rx$' "$out/declarations" || :)" \
   -v notes21="$(grep -c '^Interface migration: notes-diagnostics21 initech-q9p80$' "$out/declarations" || :)" \
   -v fdisk_reset="$(grep -c '^Interface migration: fdisk-reset initech-q9p80$' "$out/declarations" || :)" \
   -f "$helper/interface-vectors.awk" "$out/disassembly" || bad=1
  # Foreign code copied into a consumer's output remains foreign.
  foreign=0
  if [ -f "${path%.o}.d" ]; then
   origin=$(awk 'NR==1 {sub(/^[^:]*:[[:space:]]*/,"")} {gsub(/\\/,"");for(i=1;i<=NF;i++)if($i ~ /\.(c|asm)$/){print $i;exit}}' "${path%.o}.d")
   if [ -n "$origin" ]; then
    case $origin in /*) :;; *) origin=$PWD/$origin;; esac
    origin=$(realpath -m "$origin")
    if ! own_path "$origin" && [ "$origin" != "$root/sdk/rt/tbx_rt.c" ]; then
     foreign=1; grep -qxF "$origin" "$out/sources.allowed" || fail "undeclared compiled source $origin"
    fi
   fi
   if [ "$foreign" -eq 0 ]; then
    awk '{gsub(/\\/,"");for(i=1;i<=NF;i++)if($i ~ /\.h$/)print $i}' "${path%.o}.d" |
    while read -r header; do
     case $header in /*) :;; *) header=$PWD/$header;; esac
     header=$(realpath -m "$header")
     own_path "$header" || printf '%s\n' "$header"
    done >> "$out/headers"
   fi
  fi
  [ "$foreign" -eq 1 ] || nm --defined-only -g "$path" >> "$out/defs"
  ;;
 esac
done < "$out/graph.paths"
sort -u "$out/headers" -o "$out/headers"
while read -r path; do
 own_path "$path" && continue
 grep -qxF "$path" "$out/headers.allowed" || fail "undeclared external header $path"
done < "$out/headers"
# Undefined object references must resolve within this component or an exact
# provider-published list. Even a hand-written extern cannot bypass this check.
awk -v role="$role" -v component="$component" -v shell_services="$shell_services" '
 BEGIN {n=split(shell_services,a," ");for(i=1;i<=n;i++)shell[a[i]]=1}
 FILENAME==ARGV[1] {if(NF>=3)defined[$NF]=1;next}
 FILENAME==ARGV[2] {line=$0;gsub(/[^a-zA-Z0-9_]/," ",line);n=split(line,a," ");for(i=1;i<=n;i++)allowed[a[i]]=1;next}
 FILENAME==ARGV[3] {n=split($0,a," ");for(i=1;i<=n;i++)binding[a[i]]=1;next}
 {s=$NF;obj=$1;sub(/:$/,"",obj)
  resident=(role=="startup" || role=="resident" || (component=="dos" && obj ~ /\/(command-resident|batch|env)\.o$/ && shell[s]))
  reset=(component=="dos" && obj ~ /\/fdisk\/fdisk_program\.o$/ && (s=="kernel_reset" || s=="kernel_halt"))
  if(s!="" && !defined[s] && !allowed[s] && !(binding[s] && (resident || reset)) && !seen[s]++) {print "FAIL interface: undeclared external symbol " s " in " obj;bad=1}}
 END {exit bad}
' "$out/defs" "$out/symbols.allowed" "$out/bindings.allowed" "$out/refs" || bad=1
exit "$bad"
