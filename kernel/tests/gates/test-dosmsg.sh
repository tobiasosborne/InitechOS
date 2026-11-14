set -eu
"${PYTHON_MESSAGE_REFERENCE}" -c "import json,re; \
d=json.load(open('kernel/include/dos_messages.json')); \
m=d['messages'] if isinstance(d,dict) and 'messages' in d else d; \
hdr=open('${SOURCE_ROOT}/build/kernel/dos_messages.h').read(); \
esc=lambda s: s.replace(chr(92),chr(92)*2).replace(chr(34),chr(92)+chr(34)).replace(chr(13),chr(92)+'r').replace(chr(10),chr(92)+'n'); \
miss=[]; \
[ miss.append('MSG-DOS-%04d'%i) for i in range(1,67) if ('#define MSG_DOS_%04d \"%s\"'%(i,esc(m['MSG-DOS-%04d'%i]))) not in hdr ]; \
assert not miss, 'header does NOT encode spec verbatim for: %r'%miss; \
print('    ${SOURCE_ROOT}/build/kernel/dos_messages.h has all 66 #define MSG_DOS_NNNN verbatim from spec')" \
	|| { printf '!!! test-dosmsg FAIL: ${SOURCE_ROOT}/build/kernel/dos_messages.h diverges from kernel/include/dos_messages.json (regen + recheck)\n'; exit 1; }

for source in kernel/*.c dos/command/*.c; do
 build/dosmsg_scan --image "${SOURCE_ROOT}/build/kernel/dos_messages.h" "$source" "${SOURCE_ROOT}/build/kernel/INITDOS.SYS"
done
build/dosmsg_scan --known "${SOURCE_ROOT}/build/kernel/dos_messages.h" kernel/*.c dos/command/*.c

# Genuine DOS 3.30 VGA transcriptions supply the wording, independently of
# both the catalogue and its generator (initech-pnjw6).
for pair in 0064:SWITCH 0065:BATCH 0066:FULL; do
 id=${pair%:*}
 capture=${pair#*:}
 wording=$(sed -n '3{s/[[:space:]]*$//;p;}' "dos/command/tests/compat33/expected/$capture/result.txt")
 test -n "$wording"
 grep -Fx "#define MSG_DOS_$id \"$wording\"" "${SOURCE_ROOT}/build/kernel/dos_messages.h" || {
  printf '!!! captured DOS 3.30 message MSG_DOS_%s is absent or changed\n' "$id"
  exit 1
 }
done
