#!/usr/bin/env python3
from pathlib import Path
import subprocess as s, shutil, json, struct, hashlib
R=Path(__file__).resolve().parent.parent
E=R/'evidence'; I=R/'images'; F=E/'fixtures'; F.mkdir(exist_ok=True)
log=[]
def run(*a):
 p=s.run(list(map(str,a)),stdout=s.PIPE,stderr=s.STDOUT); log.append({'argv':list(map(str,a)),'rc':p.returncode,'output':p.stdout.decode(errors='replace')})
 if p.returncode: raise RuntimeError(log[-1])
 return p.stdout
def put(img,name,data):
 p=F/name.replace('/','_');p.write_bytes(data);run('mcopy','-o','-m','-i',img,p,'::'+name)
def mkdir(img,name):run('mmd','-i',img,'::'+name)
img=I/'edge-fixture.img';shutil.copyfile(I/'flair_data.img',img)
for d in ['T','T/SUB','T/EMPTY','T/DEST','MANY','D16','D32','D64','DEEP'] :mkdir(img,d)
files={'ONE.TXT':b'ALPHA\r\n','TWO.TXT':b'BETA\r\n','EOF.TXT':b'BEFORE\x1aAFTER\r\n','ZERO.TXT':b'','NOCR.TXT':b'one\ntwo\nlast','BINARY.BIN':bytes(range(256))*4,'EXACT.BIN':bytes(range(256))*2,'SELF.BIN':bytes(range(256))*32,'READONLY.TXT':b'PROTECTED\r\n','HIDDEN.TXT':b'HIDDEN\r\n','SYSTEM.TXT':b'SYSTEM\r\n','LINES.TXT':b'zebra\r\nAlpha\r\nbeta\r\nALPHA\r\n01 omega\r\n02 aardvark\r\nlast','LONG.TXT':b''.join(f'LINE {i:05d} alphabet abcdefghijklmnopqrstuvwxyz\r\n'.encode() for i in range(1600)),'NOLF.TXT':b'NO-NEWLINE','OLD.TXT':b'1980\r\n','FUTURE.TXT':b'2099\r\n'}
for n,b in files.items():put(img,'T/'+n,b)
put(img,'T/SUB/ONE.TXT',b'SUB-SENTINEL\r\n')
put(img,'T/SUB/TWO.TXT',b'SUB-TWO\r\n')
put(img,'T/SUB/RO.TXT',b'SUB-READONLY\r\n')
put(img,'T/DEST/KEEP.TXT',b'DEST-SENTINEL\r\n')
for n in ['T/READONLY.TXT','T/SUB/RO.TXT']:run('mattrib','-i',img,'+r','::'+n)
run('mattrib','-i',img,'+h','::T/HIDDEN.TXT');run('mattrib','-i',img,'+s','::T/SYSTEM.TXT')
for i in range(300):put(img,f'MANY/F{i:03d}.TXT',f'file {i:03d}\r\n'.encode())
for n in [16,32,64]:
 for i in range(n-2):put(img,f'D{n}/F{i:03d}.TXT',b'x')
p='DEEP'
for i in range(12):p+=f'/D{i:02d}';mkdir(img,p)
put(img,p+'/LEAF.TXT',b'DEEP-END\r\n')
# Set raw directory timestamps for precise historical/future COPY preservation.
b=bytearray(img.read_bytes());bps=struct.unpack_from('<H',b,11)[0];spf=struct.unpack_from('<H',b,22)[0];root=(b[14]+b[16]*spf)*bps;data=root+struct.unpack_from('<H',b,17)[0]*32
for off in range(root,root+224*32,32):
 if b[off:off+11]==b'T          ':tc=struct.unpack_from('<H',b,off+26)[0];break
at=data+(tc-2)*512
# T fits more than one cluster, traverse directory FAT.
fat=b[512:512+spf*512];c=tc
while c<0xff8:
 base=data+(c-2)*512
 for off in range(base,base+512,32):
  if b[off:off+11] in [b'OLD     TXT',b'FUTURE  TXT']:
   yr=1980 if b[off:off+3]==b'OLD' else 2099;date=((yr-1980)<<9)|(12<<5)|31;tm=(23<<11)|(59<<5)|29
   struct.pack_into('<HH',b,off+22,tm,date)
 v=struct.unpack_from('<H',fat,c+c//2)[0];c=(v>>4 if c&1 else v&4095)
img.write_bytes(b)
# Add a second primary FAT12 partition to a copied boot disk for C/D tests.
extra=I/'extra-fixture.img';extra.write_bytes(bytes(2880*512));run('mformat','-i',extra,'-f','1440','-v','EXTRA','::');mkdir(extra,'DHOME');put(extra,'DHOME/SAME.TXT',b'D-DRIVE\r\n')
master=I/'multi-boot-fixture.img';b=bytearray((I/'tracer_boot.img').read_bytes());b.extend(bytes(8192*512-len(b)));base=4096
x=bytearray(extra.read_bytes());struct.pack_into('<I',x,28,base);x[21]=0xf8;x[512]=0xf8;x[(1+9)*512]=0xf8;extra.write_bytes(x)
b[462:478]=struct.pack('<B3sB3sII',0,b'\xfe\xff\xff',1,b'\xfe\xff\xff',base,2880);b[base*512:(base+2880)*512]=extra.read_bytes();master.write_bytes(b)
# Add C fixtures with mtools at the real MBR offset.
ci=str(master)+'@@32256';mkdir(ci,'CHOME');put(ci,'CHOME/SAME.TXT',b'C-DRIVE\r\n')
(E/'preparation.json').write_text(json.dumps(log,indent=2))
(E/'fixture-manifest.json').write_text(json.dumps({p.name:{'size':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in I.glob('*fixture.img')},indent=2))
print('Prepared',len(log),'mtools operations')
