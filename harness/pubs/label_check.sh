#!/bin/sh
# initech-77am: MS-DOS 3.3 User's Guide, LABEL pp.86-87 / PDF171-172;
# shipped drivext (initech-q45q), dosutil.c label_main and DEC-15a.
# GC-01/03/04: the entry must describe the drive operand, and its printed
# example must actually label A: while C: remains the current drive.
# Expected facts are independent literals; capture.sh supplies live pixels.
set -eu
entry=$1; keys=$2; screen=$3
red() { printf '!!! test-pubs RED LABEL-%s: %s\n' "$1" "$2"; exit 1; }
text=$(awk '/^\\begin\{DosEntry\}\{LABEL\}/ { on=1 }
    on && !/^%/ { printf "%s ", $0 }
    on && /^\\end\{DosEntry\}/ { exit }' "$entry" | tr -s '[:space:]' ' ')
case $text in
*'Only the current drive'*) red DRIVE "LABEL still restricts the drive operand to the current drive";;
esac
for fact in 'You can specify any mounted drive' \
    'If you omit it, the current drive is used' \
    'An invalid drive letter displays' \
    'Invalid drive specification'; do
    printf '%s\n' "$text" | grep -Fq "$fact" || red DRIVE "entry omits: $fact"
done
[ "$(cat "$keys")" = 'CLS\nC:\nLABEL A:REPORTS\nVOL A:\nA:\nLABEL BAD.NAME\nVOL\n' ] || \
    red TRACE "labelset must exercise A: from C:, return to A:, and reject a period"
for fact in 'A:\>C:' 'C:\>LABEL A:REPORTS' 'C:\>VOL A:' \
    ' Volume in drive A is REPORTS' 'C:\>A:' 'A:\>LABEL BAD.NAME' 'A:\>VOL' \
    'Invalid characters in volume label' 'A:\>'; do
    grep -qxF "$fact" "$screen" || red SCREEN "labelset omits: $fact"
done
[ "$(grep -cxF ' Volume in drive A is REPORTS' "$screen")" -eq 2 ] || \
    red SCREEN "A: must retain REPORTS after the invalid label"
if grep -qF 'Invalid drive specification' "$screen"; then
    red SCREEN "labelset rejected the mounted A: drive"
fi
printf '>>> test-pubs-label: green (mounted/default/invalid drive text; A: labeled from C:, current drive preserved, invalid label rejected)\n'
