#!/bin/sh
# Independent serial oracle shared by the clean kernel and every mutant.
# Ref: ADR-0003 DEC-08a.1/.4; MSG-DOS-0035; spec/tsr_contract.h.
set -eu
stem=$1
tr -d '\r' < "$stem.serial" > "$stem.repl"
fail() { tail -45 "$stem.repl"; printf '!!! test-tsr-exec RED: %s\n' "$1"; exit 1; }
! grep -Eq 'triple_fault=1' "$stem.report" || fail 'triple fault'
! grep -Eq '^PANIC' "$stem.repl" || fail 'guest panic'
grep -qx 'SHELL-READY' "$stem.repl" || fail 'no shell'
grep -qx 'SK-INSTALLED' "$stem.repl" || fail 'no resident'
[ "$(grep -c '^MZEXEC-OK$' "$stem.repl")" -eq 2 ] || fail 'MZEXEC missing after KEEP (BAD_FORMAT on unchanged main)'
[ "$(grep -c '^Cannot edit .BAK file--rename file$' "$stem.repl")" -eq 1 ] || fail 'fixed-base EDLIN did not run exactly once before KEEP'
grep -qx 'LOADER-REFUSE-FLAT-RESIDENT name=EDLIN.COM' "$stem.repl" || fail 'missing named fixed-base refusal'
grep -qxF 'A:\EDLIN.COM: Insufficient memory' "$stem.repl" || fail 'missing MSG-DOS-0035 with filename'
awk '
/^Cannot edit .BAK file--rename file$/ { before=NR }
/^SK-INSTALLED$/ { resident=NR }
/^MZEXEC-OK$/ { if (!first) first=NR; last=NR; count++ }
/^LOADER-REFUSE-FLAT-RESIDENT name=EDLIN.COM$/ { refusal=NR }
/^WAITKEY-WAITING$/ { wait=NR }
/^SK-POPUP INDOS=1 SCREEN=SAME$/ { popup=NR }
/^WAITKEY-GOT=x$/ { got=NR }
/^SHELL-DONE$/ { done=NR }
END { exit !(before < resident && resident < first && first < refusal &&
             refusal < last && count==2 && last < wait && wait < popup &&
             popup < got && got < done) }
' "$stem.repl" || fail 'execution/refusal/popup order'
printf 'tsr-exec-check: green\n'
