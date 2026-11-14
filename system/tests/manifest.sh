#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
manifest=${1:-working-volume.list}
# Independent delivery inventory: GOLDEN-MASTER items 2, 5, 7 and 8;
# phase-2 design-delivery.md, Complete installed inputs.
for path in COMMAND.COM CONFIG.SYS AUTOEXEC.BAT HELLO.TXT \
 CHKDSK.COM FORMAT.COM SYS.COM FDISK.COM EDLIN.COM XCOPY.COM TREE.COM \
 ATTRIB.COM LABEL.COM FIND.COM SORT.COM MORE.COM DISKCOPY.COM DISKCOMP.COM TPSREP.COM \
 TURBO/TURBO.COM TURBO/TURBO.PAS TURBO/HELLO.PAS TURBO/TURBOIN.PAS \
 SAMPLES/CLIENTS.DBF SAMPLES/PARTS.DBF SAMPLES/INVOICE.DBF SAMPLES/Y2KACCT.PRG \
 SAMPLES/TPS.DBF SAMPLES/TPSPARM.DAT README.TXT; do
 awk -v path="$path" '$1 !~ /^#/ && $2 == path {found++} END {exit found != 1}' "$manifest" || {
  echo "FAIL installed inventory: missing or repeated $path" >&2; exit 1;
 }
done
awk '
 /^#/ || NF == 0 {next}
 NF != 2 || $1 !~ /^build\// || $2 ~ /(^\/|\.\.|\\)/ {bad=1}
 seen[toupper($2)]++ {bad=1}
 $2 ~ /(TIMER|IDXTEST|VALUES|TPSREPT|TURBOOUT|\.DBT$|\.NDX$)/ {bad=1}
 END {exit bad}' "$manifest" || { echo 'FAIL installed inventory: invalid, duplicate or test-only entry' >&2; exit 1; }
echo 'system: installed workflow inventory green'
