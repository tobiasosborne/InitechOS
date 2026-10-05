#!/usr/bin/env python3
"""Prepare valid disposable FAT fixtures using mtools, never repo images."""
import pathlib, subprocess, shutil, json, hashlib, re
R=pathlib.Path(__file__).resolve().parent
log=[]
def run(*a,ok=True):
 p=subprocess.run(a,capture_output=True,text=True)
 log.append(dict(argv=list(a),rc=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if ok and p.returncode: raise RuntimeError(log[-1])
 return p.stdout
def copy(base,name):
 p=R/'images'/name; shutil.copyfile(R/'images'/base,p); return str(p)
def put(img,name,data):
 p=R/'fixtures'/name.replace('/','_'); p.write_bytes(data if isinstance(data,bytes) else data.encode())
 run('mcopy','-o','-i',img,str(p),'::'+name)
dos=copy('flair_data.img','dos-fixture.img')
for d in ['FILLED','DELTEST','ALLTEST','OTHER','EMPTY']: run('mmd','-i',dos,'::'+d)
put(dos,'SELF.BIN',bytes(range(256))*32)
put(dos,'KEEP.TXT','ROOT-SENTINEL\r\n')
put(dos,'FILLED/SAVE.TXT','SUBDIR-SENTINEL\r\n')
put(dos,'DELTEST/KEEP.TXT','SUB-SENTINEL\r\n')
for i in range(40): put(dos,f'DELTEST/Z{i:02}.TXT',f'delete {i}\r\n')
for i in range(3): put(dos,f'ALLTEST/A{i}.TXT',f'all {i}\r\n')
put(dos,'READONLY.TXT','READONLY-SENTINEL\r\n'); run('mattrib','-i',dos,'+r','::READONLY.TXT')
put(dos,'CTRLZ.TXT',b'BEFORE\x1aAFTER\r\n')
put(dos,'CHECK.BAT','@ECHO OFF\r\nECHO ARG1=%1\r\nIF EXIST KEEP.TXT ECHO FOUND\r\nIF NOT EXIST ABSENT.TXT ECHO ABSENT\r\nFOR %%F IN (ONE TWO THREE) DO ECHO ITEM=%%F\r\nCALL CHILD.BAT TWO\r\nSHIFT\r\nECHO SHIFT=%1\r\nGOTO END\r\nECHO WRONG\r\n:END\r\nECHO FINISHED\r\n')
put(dos,'CHILD.BAT','@ECHO OFF\r\nECHO CHILD=%1\r\n')
gui=copy('flair_data.img','gui-fixture.img')
for d in ['A','B','C','D','E','A/CHILD','B/CHILD']:run('mmd','-i',gui,'::'+d)
for name,data in [('SAME.TXT','root'),('B/SAME.TXT','child'),('WWWWWWWW.WWW','wide'),('MMMMMMMM.MMM','wide')]:put(gui,name,data)
put(gui,'LOCKED.TXT','locked trash file');run('mattrib','-i',gui,'+r','::LOCKED.TXT')
for i in range(70):put(gui,f'F{i:02}.TXT',f'file {i}\r\n')
full=copy('flair_data.img','full-fixture.img')
free=int(re.search(r'([\d ]+) bytes free',run('mdir','-i',full,'::')).group(1).replace(' ',''))
put(full,'FILLER.BIN',bytes(free))
db=copy('samir_list.img','db-fixture.img')
run('mmd','-i',db,'::APPS')
for name in ['TENANTFX.EXE','CTENANT.EXE','123.EXE']:
 p=R/'fixtures'/name;run('mcopy','-o','-i',str(R/'images/flair_data.img'),'::APPS/'+name,str(p));run('mcopy','-o','-i',db,str(p),'::APPS/'+name)
put(db,'SIMPLE.PRG','STORE 5 TO X\r\n? X * 2\r\nRETURN\r\n')
put(db,'UPDATE.PRG','GO TOP\r\nREPLACE BAL WITH 42\r\nRETURN\r\n')
for name in ['dos-fixture.img','gui-fixture.img','full-fixture.img','db-fixture.img']:
 s=run('mdir','-i',str(R/'images'/name),'::');(R/'evidence'/f'{name}-before.txt').write_text(s)
(R/'evidence/fixture-preparation.json').write_text(json.dumps(log,indent=2))
(R/'evidence/image-manifest.json').write_text(json.dumps({p.name:dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in (R/'images').glob('*.img')},indent=2))
print('Fixtures ready')
