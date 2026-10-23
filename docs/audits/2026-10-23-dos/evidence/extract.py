#!/usr/bin/env python3
from pathlib import Path
import subprocess as s,sys,struct,json,hashlib
R=Path(__file__).resolve().parent.parent;E=R/'evidence'
label=sys.argv[1];img=R/'images'/f'{label}-data.img';out=E/'extracted'/label;out.mkdir(parents=True,exist_ok=True)
checks={}
for tool,args in [('minfo',[]),('mdir',['-s','::']),('fsck.fat',['-n'])]:
 cmd=([tool,'-i',str(img)]+args) if tool!='fsck.fat' else [tool]+args+[str(img)]
 p=s.run(cmd,stdout=s.PIPE,stderr=s.STDOUT);(E/f'{label}-{tool}.txt').write_bytes(p.stdout);checks[tool]={'rc':p.returncode,'argv':cmd}
p=s.run(['mcopy','-s','-m','-i',str(img),'::T',str(out)],stdout=s.PIPE,stderr=s.STDOUT);checks['mcopy']={'rc':p.returncode,'out':p.stdout.decode(errors='replace')}
b=img.read_bytes();bps=struct.unpack_from('<H',b,11)[0];spc=b[13];f0=struct.unpack_from('<H',b,14)[0]*bps;spf=struct.unpack_from('<H',b,22)[0];root=f0+spf*b[16]*bps;data=root+struct.unpack_from('<H',b,17)[0]*32
entries=[]
def directory(start,path):
 chunks=[(root,struct.unpack_from('<H',b,17)[0]*32)] if start==0 else []
 c=start;seen=set()
 while c>=2 and c<0xff8 and c not in seen:
  seen.add(c);chunks.append((data+(c-2)*bps*spc,bps*spc));v=struct.unpack_from('<H',b,f0+c+c//2)[0];c=(v>>4 if c&1 else v&4095)
 for base,size in chunks:
  for o in range(base,base+size,32):
   e=b[o:o+32]
   if len(e)<32 or e[0]==0:return
   if e[0] in [0xe5,0x2e] or e[11]==15:continue
   name=e[:8].decode('ascii',errors='replace').rstrip();ext=e[8:11].decode('ascii',errors='replace').rstrip();name+=('.'+ext if ext else '')
   c=struct.unpack_from('<H',e,26)[0];t,dt=struct.unpack_from('<HH',e,22)
   item={'path':path+name,'attr':e[11],'cluster':c,'size':struct.unpack_from('<I',e,28)[0],'raw_hex':e.hex(),'date':f'{1980+(dt>>9):04d}-{(dt>>5)&15:02d}-{dt&31:02d}','time':f'{t>>11:02d}:{(t>>5)&63:02d}:{(t&31)*2:02d}'};entries.append(item)
   if e[11]&16:directory(c,path+name+'/')
directory(0,'/')
checks['fat_mirrors_equal']=b[f0:f0+spf*bps]==b[f0+spf*bps:f0+2*spf*bps]
checks['entries']=entries;checks['extracted']={str(p.relative_to(out)):{'size':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'hex':p.read_bytes().hex() if p.stat().st_size<100 else None} for p in out.rglob('*') if p.is_file()}
(E/f'{label}-filesystem.json').write_text(json.dumps(checks,indent=2));print(label,checks['fsck.fat'],'files',len(checks['extracted']))
