#!/bin/sh
# Run from repository root. Uses the actual editor, interpreter and host PAL.
set -eu
. tools/lane-env.sh
cc -std=c11 -O1 -g -Wall -Wextra -Werror -DSAMIR_W4 -DUI_HAVE_create -DUI_HAVE_modcmd \
    -Ios/samir/include -Ios/samir/ui -Ispec -o build/host_CREATE \
    harness/diff/dbf_diff/xbase_prog_diff/oneshot/create/host_replay.c \
    os/samir/samir_main.c os/samir/core/rt.c os/samir/core/value.c \
    os/samir/core/lex.c os/samir/core/parse.c os/samir/core/eval.c \
    os/samir/core/fn_builtins.c os/samir/fs/dbf.c os/samir/fs/dbt.c \
    os/samir/fs/ndx.c os/samir/cmd/workarea.c os/samir/cmd/nav.c \
    os/samir/cmd/flow.c os/samir/cmd/query.c os/samir/cmd/mutate.c \
    os/samir/cmd/set.c os/samir/cmd/proc.c os/samir/cmd/report.c \
    os/samir/pal/pal_host.c os/samir/ui/ui.c os/samir/ui/kit.c \
    os/samir/ui/create.c os/samir/ui/modcmd.c -lm
root=$PWD
for app in create modcmd; do
    out=build/oneshot/$app-host
    mkdir -p "$out/files"
    rm -f "$out/files/WIDGETS.DBF" "$out/files/WIDGETS.BAK" \
        "$out/files/HELLO.PRG" "$out/files/HELLO.BAK"
    (cd "$out/files" && ../../../host_CREATE "$app" > ../after.txt)
    (cd "$out" && OLDPWD="$root" sh "$root/harness/diff/dbf_diff/xbase_prog_diff/oneshot/$app/CHECK")
done
out=build/oneshot/create-extended
rm -rf "$out"
mkdir -p "$out"
python3 - <<'PY'
from pathlib import Path
import shutil
p = Path('build/oneshot/create-extended')
for case in ('memo', 'abort', 'prompt', 'structure', 'text', 'abandon', 'empty', 'file', 'line-limit', 'file-limit'):
    (p / case).mkdir()
shutil.copyfile('build/oneshot/create-host/files/WIDGETS.DBF', p / 'structure' / 'WIDGETS.DBF')
for case in ('text', 'abandon'):
    (p / case / 'EDITING.PRG').write_bytes(b'abc\r\ndef\r\nlast\x1a')
(p/'line-limit'/'KEYS.RAW').write_bytes(b'a'*254 + b'x \x17')
(p/'file-limit'/'KEYS.RAW').write_bytes((b'a'*254+b'\r')*19 + b'b'*135 + b'x \x17')
PY
for pair in memo:b abort:a prompt:p structure:s text:t abandon:x empty:e file:f line-limit:r file-limit:r; do
    case_name=${pair%:*}
    mode=${pair#*:}
    (cd "$out/$case_name" && ../../../host_CREATE "$mode" > output.txt)
done
python3 - "$root" <<'PY'
import importlib.util
from pathlib import Path
import sys
s = importlib.util.spec_from_file_location('ref', Path(sys.argv[1])/'harness/diff/dbf_diff/dbf_ref.py')
m = importlib.util.module_from_spec(s); s.loader.exec_module(m)
p = Path('build/oneshot/create-extended')
d = m.DbfFile(str(p/'memo'/'MEMOS.DBF'))
assert d.fields == [('NOTE', 'M', 10, 0)] and d.record_count == 0 and d.version == 0x83
b = (p/'memo'/'MEMOS.DBT').read_bytes()
assert len(b) == 512 and b[:4] == b'\x01\0\0\0'
assert not (p/'abort'/'ABORTED.DBF').exists()
d = m.DbfFile(str(p/'prompt'/'PROMPTED.DBF'))
assert d.fields == [('NAME','C',12,0)] and d.record_count == 0
orig = m.DbfFile(str(p/'structure'/'WIDGETS.BAK'))
new = m.DbfFile(str(p/'structure'/'WIDGETS.DBF'))
assert orig.fields[1][0] == 'QTY' and new.fields[1][0] == 'COUNT'
assert orig.record_count == new.record_count == 1
raw = (p/'structure'/'WIDGETS.DBF').read_bytes()
assert raw[new.header_length:new.header_length+6] == b'*ALPHA'
assert (p/'text'/'EDITING.PRG').read_bytes() == b'!Zbcdef\r\nlast\x1a'
assert (p/'text'/'EDITING.BAK').read_bytes() == b'abc\r\ndef\r\nlast\x1a'
assert (p/'abandon'/'EDITING.PRG').read_bytes() == b'abc\r\ndef\r\nlast\x1a'
assert not (p/'abandon'/'EDITING.BAK').exists()
assert (p/'empty'/'EMPTY.PRG').read_bytes() == b'\x1a'
assert (p/'file'/'PLAIN').read_bytes() == b'plain text\x1a'
assert not (p/'file'/'PLAIN.PRG').exists()
assert (p/'line-limit'/'EDITING.PRG').read_bytes() == b'a'*254 + b'\x1a'
expected = (b'a'*254+b'\r\n')*19 + b'b'*135 + b'\x1a'
assert len(expected) == 5000
assert (p/'file-limit'/'EDITING.PRG').read_bytes() == expected
print('Extended replay PASS: DBT, prompt, abort, migration, deletion flag, backups, text editing, abandon, MODIFY FILE, 254/5000 limits')
PY
(cd "$out/structure" && ../../../host_CREATE k > keepmemo.txt)
python3 harness/diff/dbf_diff/dbf_ref.py --memo "$out/structure/WIDGETS.DBF" > "$out/memo.txt"
python3 - <<'PY'
from pathlib import Path
p=Path('build/oneshot/create-extended')
assert "memo survives" in (p/'memo.txt').read_text()
assert (p/'structure'/'WIDGETS.TBK').read_bytes() == (p/'structure'/'WIDGETS.DBT').read_bytes()
print('Memo contents and DBT backup survive MODIFY STRUCTURE')
PY
