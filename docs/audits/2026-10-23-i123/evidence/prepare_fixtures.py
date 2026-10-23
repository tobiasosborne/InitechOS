#!/usr/bin/env python3
# Rebuild copied data fixtures only. Never runs or modifies the source repository.
import pathlib,shutil,subprocess,re,struct,sys
from drive123 import ROOT,fixture
mode=sys.argv[1]
if mode=='base':
 fixture()
 rec=lambda op,b:struct.pack('<HH',op,len(b))+b
 code=b'\x02'+struct.pack('<HHHH',1,0,255,8191)+b'\x50\x01\x03'
 b=rec(0,b'\x06\x04')+rec(2,b'\x00')+b''.join(rec(16,struct.pack('<BHHdH',0x7f,0,i,0,len(code))+code) for i in range(2048))+rec(1,b'')
 p=ROOT/'evidence/HEAVY.WK1';p.write_bytes(b)
 subprocess.run(['mcopy','-i',str(ROOT/'images/fixture.img'),str(p),'::HEAVY.WK1'],check=True)
 # Dense labels occupy 32535 bytes of the 32768-byte cell pool.
 b=rec(0,b'\x06\x04')+b''.join(rec(15,struct.pack('<BHH',0x7f,0,i)+b"'"+b'x'*239+b'Z\0') for i in range(135))+rec(1,b'')
 p=ROOT/'evidence/POOL135.WK1';p.write_bytes(b)
 subprocess.run(['mcopy','-i',str(ROOT/'images/fixture.img'),str(p),'::POOL135.WK1'],check=True)
elif mode=='full':
 # Run phases.py long first so the metadata exists before the disk is filled.
 source=ROOT/'images/long-data.img';dest=ROOT/'images/full-fixture2.img';shutil.copyfile(source,dest)
 subprocess.run(['mcopy','-i',str(dest),'/home/tobias/Projects/lotus123-decomp/goldens/minted/MINT01.WK1','::FULL.WK1'],check=True)
 s=subprocess.check_output(['mdir','-i',str(dest),'::'],text=True)
 n=int(re.search(r'([\d ]+) bytes free',s)[1].replace(' ',''));p=ROOT/'evidence/FILLER2.BIN';p.write_bytes(bytes(n))
 subprocess.run(['mcopy','-i',str(dest),str(p),'::FILLER.BIN'],check=True)
 (ROOT/'evidence/full2-before.txt').write_text(subprocess.check_output(['mdir','-i',str(dest),'::'],text=True))
else:raise SystemExit('usage: prepare_fixtures.py base|full')
