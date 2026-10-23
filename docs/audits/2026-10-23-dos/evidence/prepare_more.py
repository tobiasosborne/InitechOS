#!/usr/bin/env python3
from pathlib import Path
import subprocess as s,struct,shutil
R=Path(__file__).resolve().parent.parent;I=R/'images';E=R/'evidence';F=E/'more-fixtures';F.mkdir(exist_ok=True)
img=I/'more-fixture.img';shutil.copyfile(I/'program-fixture.img',img)
def put(n,b):
 p=F/n;p.write_bytes(b);s.run(['mcopy','-o','-i',str(img),str(p),'::T/'+n],check=True)
for c in [0,7]:put(f'ERR{c}.COM',b'\xb8'+struct.pack('<I',0x4c00+c)+b'\xcd\x21')
put('OPEN.TXT',b'OPEN-SENTINEL\r\n')
put('TEN.BAT',b'@ECHO OFF\r\nECHO A=%1 B=%9\r\nSHIFT\r\nECHO ZERO=%0 A=%1 B=%9\r\nSHIFT\r\nECHO A=%1 B=%9\r\n')
put('ELARGE.TXT',b''.join(f'RECORD {i:04d} abcdefghijklmnopqrstuvwxyz\r\n'.encode() for i in range(1600)))
put('MERGE.TXT',b'MERGED-A\r\nMERGED-B\r\n')
put('TINY.TXT',b'one\r\ntwo\r\nthree\r\nfour\r\n')
# Correct CHS fields for QEMU's 4 MiB disk geometry (16 heads, 63 sectors).
b=bytearray((I/'multi-boot-fixture.img').read_bytes())
def chs(lba):
 c,rem=divmod(lba,16*63);h,sec=divmod(rem,63);sec+=1;return bytes([h,(sec&63)|((c>>2)&0xc0),c&255])
for i in range(2):
 o=446+i*16;start,size=struct.unpack_from('<II',b,o+8);b[o+1:o+4]=chs(start);b[o+5:o+8]=chs(start+size-1)
# Hidden sectors and fixed media retain DEC-18 consistency.
(I/'geometry-boot-fixture.img').write_bytes(b)
print('more fixtures ready')
