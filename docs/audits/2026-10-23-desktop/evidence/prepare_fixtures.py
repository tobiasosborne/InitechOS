#!/usr/bin/env python3
"""Local copies only; never alter supplied images or the repository."""
import pathlib, shutil, subprocess, json, hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
def run(*a):
    return subprocess.run(a,check=True,capture_output=True,text=True).stdout
edge=ROOT/'images/edge-data.img'
shutil.copyfile(ROOT/'images/flair_data.img',edge)
def md(p): run('mmd','-i',str(edge),'::'+p)
def put(p,b):
    tmp=ROOT/'evidence/fixture-payload.tmp'; tmp.write_bytes(b)
    run('mcopy','-i',str(edge),str(tmp),'::'+p)
for p in ['A','A/CHILD','B','MANY','NEST']+[f'D{i:02}' for i in range(1,21)]: md(p)
put('A/CHILD/KEEP.TXT',b'KEEP THIS TREE\r\n')
put('A/SAME.TXT',b'AAA source payload\r\n')
put('B/SAME.TXT',b'BBB target payload\r\n')
put('A/LOCKED.TXT',b'LOCKED SENTINEL\r\n')
run('mattrib','-i',str(edge),'+r','::A/LOCKED.TXT')
for i in range(80): put(f'MANY/F{i:03}.TXT',f'fixture {i}\r\n'.encode())
put('MANY/WWWWWWWW.WWW',b'wide 1'); put('MANY/MMMMMMMM.MMM',b'wide 2')
path='NEST'
for i in range(1,17):
    path+=f'/N{i:02}'; md(path)
put(path+'/BOTTOM.TXT',b'deep sentinel\r\n')
full=ROOT/'images/full-data.img'; shutil.copyfile(ROOT/'images/flair_data.img',full)
big=ROOT/'evidence/fill.bin'; big.write_bytes(b'F'*1163776)
run('mcopy','-i',str(full),str(big),'::FILL.BIN')
big.unlink(); (ROOT/'evidence/fixture-payload.tmp').unlink()
manifest={p.name:{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size} for p in [edge,full]}
(ROOT/'evidence/fixture-manifest.json').write_text(json.dumps(manifest,indent=2))
for p in [edge,full]:
    (ROOT/'evidence'/f'{p.stem}-inventory.txt').write_text(run('mdir','-i',str(p),'-s','::'))
