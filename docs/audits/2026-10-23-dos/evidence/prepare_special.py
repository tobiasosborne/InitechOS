#!/usr/bin/env python3
from pathlib import Path
import sys,subprocess as s,shutil,struct,json
R=Path(__file__).resolve().parent.parent; I=R/'images';E=R/'evidence';F=E/'special-fixtures';F.mkdir(exist_ok=True)
log=[]
def run(*a,check=True):
 p=s.run(list(map(str,a)),stdout=s.PIPE,stderr=s.STDOUT);log.append({'argv':list(map(str,a)),'rc':p.returncode,'out':p.stdout.decode(errors='replace')})
 if check and p.returncode:raise RuntimeError(log[-1])
 return p.stdout

def put(img,name,b):
 f=F/name.replace('/','_');f.write_bytes(b);run('mcopy','-o','-i',img,f,'::'+name)
def free(img):
 b=run('mdir','-i',img,'::').decode();return int(b.split('bytes free')[0].splitlines()[-1].strip().replace(' ',''))
def layout(b):
 spf=struct.unpack_from('<H',b,22)[0];fat=struct.unpack_from('<H',b,14)[0]*512;root=fat+b[16]*spf*512;data=root+struct.unpack_from('<H',b,17)[0]*32;return fat,spf,root,data

def fatset(b,n,v):
 fat,spf,_,_=layout(b)
 for k in range(b[16]):
  o=fat+k*spf*512+n+n//2;old=struct.unpack_from('<H',b,o)[0];value=(old&15)|(v<<4) if n&1 else (old&0xf000)|v;struct.pack_into('<H',b,o,value)
for left in [0,512]:
 img=I/f'full{left}-fixture.img';shutil.copyfile(I/'edge-fixture.img',img);n=free(img)-left;put(img,'FILLER.BIN',b'F'*n);print(img.name,'free',free(img))
for n in [64,224]:
 img=I/f'root{n}-fixture.img';img.write_bytes(bytes(1474560));run('mformat','-i',img,'-f','1440','::')
 for j in range(n):put(img,f'F{j:03d}.TXT',b'')
# Minimal code: 32-bit native exit, invalid opcode, zero length, 16-bit MZ.
img=I/'program-fixture.img';shutil.copyfile(I/'suite-fixture.img',img)
exitcode=bytes.fromhex('b8004c0000cd21')
put(img,'T/ZERO.COM',b'');put(img,'T/GARBAGE.COM',bytes.fromhex('0f0b'));put(img,'T/MAX64.COM',exitcode+b'\x90'*(65536-len(exitcode)))
mz=bytearray(32);mz[:2]=b'MZ';struct.pack_into('<HHHHHHHHHHHHH',mz,2,39,1,0,2,0,0xffff,0,0xfffe,0,0,0,0x1c,0);put(img,'T/DOS16.EXE',mz+bytes.fromhex('b8004ccd21'))
# Valid native open / delete while open / read / close probe, hand assembled later.
# Corruption: two files sharing same cluster and one deliberately orphaned end cluster.
img=I/'dirty-fixture.img';shutil.copyfile(I/'edge-fixture.img',img);b=bytearray(img.read_bytes());fat,spf,root,data=layout(b)
for o in range(root,root+224*32,32):
 if b[o:o+11]==b'T          ':c=struct.unpack_from('<H',b,o+26)[0];break
base=data+(c-2)*512;slots={bytes(b[o:o+11]):o for o in range(base,base+512,32)}
one=slots[b'ONE     TXT'];two=slots[b'TWO     TXT'];struct.pack_into('<H',b,two+26,struct.unpack_from('<H',b,one+26)[0]);fatset(b,2800,0xfff);b[data+(2800-2)*512:data+(2800-1)*512]=b'ORPHAN-RECOVERY\r\n'.ljust(512,b'!');img.write_bytes(b)
for name,patch in [('badbpb',lambda b:b.__setitem__(13,0)),('nofat',lambda b:b.__setitem__(slice(0,512),bytes(512)))]:
 img=I/f'{name}-fixture.img';b=bytearray((I/'edge-fixture.img').read_bytes());patch(b);img.write_bytes(b)
# FAT12 cluster sizes 2..64 and FAT16.
for spc in [2,4,8,16,32,64]:
 img=I/f'spc{spc}-fixture.img';img.write_bytes(bytes(1474560));run('mformat','-i',img,'-f','1440','-c',str(spc),'-v',f'CLUSTER{spc}','::')
 put(img,'AUTOEXEC.BAT',b'@ECHO OFF\r\n');put(img,'EXACT.BIN',bytes(range(256))*(spc*2));put(img,'PLUS.BIN',bytes(range(256))*(spc*2)+b'X');put(img,'ONE.TXT',b'ONE\r\n')
img=I/'fat16-fixture.img';img.write_bytes(bytes(8*1024*1024));run('mformat','-i',img,'-T','16384','-h','16','-s','63','-c','1','-v','FAT16TEST','::');put(img,'AUTOEXEC.BAT',b'@ECHO OFF\r\n');put(img,'CHAIN.TXT',b''.join(f'FAT16 LINE {i:04d}\r\n'.encode() for i in range(1400)))
(E/'special-preparation.json').write_text(json.dumps(log,indent=2));print('special fixtures ready')
