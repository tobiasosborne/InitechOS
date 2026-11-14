#!/bin/sh
# Exact, expiring debt from initech-q9p80. No broad component exemptions.
set -eu
component=$1 declarations=$2
check() {
 name=$1 issue=$2 file=$3 pattern=$4
 live=0
 if [ -f "$file" ]; then
  awk -v pattern='a^' -v emit=1 -f "$(dirname "$0")/source-audit.awk" "$file" > "$declarations.source"
  if grep -Eq "$pattern" "$declarations.source"; then live=1; fi
 fi
 if [ "$live" -eq 1 ]; then
  grep -qx "Interface migration: $name $issue" "$declarations" || { echo "FAIL interface: wrong issue for $name"; exit 1; }
  echo "interface migration: $name ($issue), $file; expires when this crossing is removed"
 elif grep -q "^Interface migration: $name " "$declarations"; then
  echo "FAIL interface: stale migration $name; remove its README declaration"; exit 1
 fi
}
case $component in
 dos)
 check chkdsk-arena initech-q9p80 chkdsk/chkdsk_program.c 'io[.]arena[.]base='
 check fdisk-reset initech-q9p80 fdisk/fdisk_program.c '[(]void[)]kernel_reset[(][)]'
 ;;
 apps/initechbase) check base-console39 initech-qu7rx pal/pal_dos.c '"int \$0x39; setc %0"';;
 apps/notepad) check notes-diagnostics21 initech-q9p80 notepad.c '"int \$0x21"';;
esac
while read -r first kind name issue; do
 [ "$kind" = migration: ] || continue
 case $component:$name:$issue in
 dos:chkdsk-arena:initech-q9p80|dos:fdisk-reset:initech-q9p80|apps/initechbase:base-console39:initech-qu7rx|apps/notepad:notes-diagnostics21:initech-q9p80) :;;
 *) echo "FAIL interface: unknown migration $component/$name/$issue"; exit 1;;
 esac
done < "$declarations"
