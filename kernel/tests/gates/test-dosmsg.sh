set -eu
"${PYTHON_MESSAGE_REFERENCE}" -c "import json,re; \
d=json.load(open('kernel/include/dos_messages.json')); \
m=d['messages'] if isinstance(d,dict) and 'messages' in d else d; \
hdr=open('${SOURCE_ROOT}/build/kernel/dos_messages.h').read(); \
esc=lambda s: s.replace(chr(92),chr(92)*2).replace(chr(34),chr(92)+chr(34)).replace(chr(13),chr(92)+'r').replace(chr(10),chr(92)+'n'); \
miss=[]; \
[ miss.append('MSG-DOS-%04d'%i) for i in range(1,64) if ('#define MSG_DOS_%04d \"%s\"'%(i,esc(m['MSG-DOS-%04d'%i]))) not in hdr ]; \
assert not miss, 'header does NOT encode spec verbatim for: %r'%miss; \
print('    ${SOURCE_ROOT}/build/kernel/dos_messages.h has all 63 #define MSG_DOS_NNNN verbatim from spec')" \
	|| { printf '!!! test-dosmsg FAIL: ${SOURCE_ROOT}/build/kernel/dos_messages.h diverges from kernel/include/dos_messages.json (regen + recheck)\n'; exit 1; }

for source in kernel/*.c dos/command/*.c; do
 build/dosmsg_scan --image "${SOURCE_ROOT}/build/kernel/dos_messages.h" "$source" "${SOURCE_ROOT}/build/kernel/INITDOS.SYS"
done
build/dosmsg_scan --known "${SOURCE_ROOT}/build/kernel/dos_messages.h" kernel/*.c dos/command/*.c
