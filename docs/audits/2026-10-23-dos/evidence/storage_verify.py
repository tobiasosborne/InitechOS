#!/usr/bin/env python3
from pathlib import Path
import json,subprocess as sp,hashlib,struct,os
R=Path(__file__).resolve().parent.parent;E=R/'evidence';I=R/'images';report={}
def get(img,path):return sp.run(['mcopy','-i',str(img),'::'+path,'-'],stdout=sp.PIPE,stderr=sp.PIPE)
for name in ['re-full0','re-full512','re-dirty','re-root64','re-root224']+[f're-spc{x}' for x in [2,4,8,16,32,64]]+['re-fat16']:
 img=I/f'{name}-data.img';a={};report[name]=a
 for tool,args in [('minfo',['-i',str(img)]),('mdir',['-i',str(img),'-s','::']),('fsck.fat',['-n',str(img)])]:
  p=sp.run([tool]+args,stdout=sp.PIPE,stderr=sp.STDOUT);(E/f'{name}-{tool}.txt').write_bytes(p.stdout);a[tool]={'rc':p.returncode}
 rc=E/f'{name}-mtoolsrc';rc.write_text(f'drive u: file="{img}"\n');env=dict(os.environ,MTOOLSRC=str(rc));p=sp.run(['mcheck','u:'],env=env,stdout=sp.PIPE,stderr=sp.STDOUT);(E/f'{name}-mcheck.txt').write_bytes(p.stdout);a['mcheck_rc']=p.returncode
 paths=['T/FULL.OUT','T/FULL.BIN','T/ZERO2.TXT','T/SELF.BIN'] if 'full' in name else ['FILE0000.CHK','FILE0001.CHK','POST.TXT'] if 'dirty' in name else ['EXACT.BIN','PLUS.BIN','RENAMED.BIN','TYPE.BIN','SUB/EXACT.BIN'] if 'spc' in name else ['NEW.TXT']
 out=E/'extracted'/name;out.mkdir(parents=True,exist_ok=True);a['files']={}
 for path in paths:
  p=get(img,path);f=out/path.replace('/','_')
  if p.returncode==0:f.write_bytes(p.stdout)
  a['files'][path]={'rc':p.returncode,'size':len(p.stdout),'sha256':hashlib.sha256(p.stdout).hexdigest(),'hex':p.stdout.hex() if len(p.stdout)<100 else None}
 if 'spc' in name:
  a['copy_equal']=get(img,'EXACT.BIN').stdout==get(img,'RENAMED.BIN').stdout==get(img,'SUB/EXACT.BIN').stdout;a['type_equal']=get(img,'PLUS.BIN').stdout==get(img,'TYPE.BIN').stdout
 if name=='re-fat16':
  boot=str(I/f'{name}-boot.img')+'@@32256';src=get(img,'CHAIN.TXT').stdout
  for pth in ['FAT16.OUT','FATCOPY.TXT']:
   p=get(boot,pth);(out/pth).write_bytes(p.stdout);a[pth]={'rc':p.returncode,'size':len(p.stdout),'equals_source':p.stdout==src}
(E/'storage-verification.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:{z:v for z,v in x.items() if z not in ['files']} for k,x in report.items()},indent=2))
