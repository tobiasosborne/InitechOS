#!/bin/sh
set -eu
disk=$1 out=$2
mcopy -o -i "$disk" ::APPS/TABLES.TXT "$out.txt"
printf 'INITECH LEDGER\r\n73               TODO_FRAME\r\nMatte Cutters    TODO_FRAME\r\nExacto Blades    TODO_FRAME\r\n570- format      570-\r\nTOTAL            $112,276.87\r\n' > "$out.expected"
cmp "$out.expected" "$out.txt"
mcopy -i "$disk" ::APPS/LEDGER.TXT - | cmp - apps/initechcalc/data/LEDGER.TXT
if mdir -b -i "$disk" ::APPS | grep -E 'TABLE.*\.TMP$'; then exit 1; fi
fsck.fat -n "$disk" > "$out.fsck"
