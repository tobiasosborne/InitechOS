#!/usr/bin/env python3
from pathlib import Path
import shutil,subprocess,json,hashlib
root=Path(__file__).resolve().parent
p=root/'images/many_source.img';shutil.copyfile(root/'images/flair_data.img',p)
f=root/'work/EMPTY.TXT';f.parent.mkdir(exist_ok=True);f.write_bytes(b'visual audit fixture\r\n')
for i in range(30):subprocess.run(['mcopy','-o','-i',str(p),str(f),f'::F{i:02}.TXT'],check=True)
listing=subprocess.check_output(['mdir','-i',str(p),'::']).decode()
(root/'evidence/many-fixture-directory.txt').write_text(listing)
(root/'evidence/many-fixture.json').write_text(json.dumps({'source':'images/flair_data.img','output':str(p.relative_to(root)),'added_files':[f'F{i:02}.TXT' for i in range(30)],'added_contents_sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()},indent=2))
