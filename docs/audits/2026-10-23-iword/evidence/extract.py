#!/usr/bin/env python3
"""Extract root files from a STOPPED audit copy into a new directory.
Usage: python3 evidence/extract.py images/s02-data.img /tmp/iword-extracted
Do not use on a disk currently mounted by QEMU.
"""
import sys,pathlib,subprocess,json,hashlib
image=pathlib.Path(sys.argv[1]).resolve(); out=pathlib.Path(sys.argv[2]).resolve()
if out.exists():raise SystemExit('Choose a new output directory')
out.mkdir(parents=True)
subprocess.run(['mcopy','-i',str(image),'::*',str(out)+'/'],check=True)
(out/'directory.txt').write_text(subprocess.check_output(['mdir','-i',str(image),'::'],text=True))
files={}
for p in out.iterdir():
 if p.is_file():
  b=p.read_bytes(); files[p.name]={'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
  if b[:4]==b'\xffWPC':
   start=int.from_bytes(b[4:8],'little');files[p.name].update(docstart=start,body_hex=b[start:].hex())
(out/'bytes.json').write_text(json.dumps(files,indent=2))
