#!/usr/bin/env python3
"""Recreate resumed test inputs from retained fixtures. Do not run during QEMU."""
from pathlib import Path
import shutil,subprocess as sp,struct,json
R=Path(__file__).resolve().parent.parent;I=R/'images';E=R/'evidence'
image=I/'resume-fixture.img';shutil.copyfile(I/'more-fixture.img',image)
for p in sorted((E/'resume-fixtures').iterdir()):
 if p.is_file():sp.run(['mcopy','-o','-i',str(image),str(p),'::T/'+p.name],check=True)
b=bytearray((I/'geometry-boot-fixture.img').read_bytes());b[462:478]=bytes(16);(I/'single-boot-fixture.img').write_bytes(b)
image=I/'root64real-fixture.img';image.write_bytes(bytes(1474560));sp.run(['mformat','-i',str(image),'-f','1440','-r','4','::'],check=True)
zero=E/'resume-fixtures/ZERO.TXT';zero.write_bytes(b'')
for n in range(64):sp.run(['mcopy','-o','-i',str(image),str(zero),f'::F{n:03d}.TXT'],check=True)
assert struct.unpack_from('<H',image.read_bytes(),17)[0]==64
print('Resumed fixture images prepared')
