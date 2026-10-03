from pathlib import Path
import subprocess,shutil,json,hashlib
R=Path(__file__).resolve().parent
log=[]
def run(*args):
 p=subprocess.run(args,capture_output=True,text=True); log.append({'command':list(args),'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr})
 if p.returncode: raise RuntimeError(log[-1])
 return p.stdout
seed=R/'images/flair_data.img'
p=R/'evidence/tiny.txt'; p.write_text('Audit file.\n')
many=R/'images/many-data.img';shutil.copyfile(seed,many)
for i in range(64): run('mcopy','-i',str(many),str(p),f'::/X{i:03d}.TXT')
run('mmd','-i',str(many),'::/AAAAA')
(R/'evidence/many-before.txt').write_text(run('mdir','-i',str(many),'::/'))
folders=R/'images/folders-data.img';shutil.copyfile(seed,folders)
for s in ['A','B','C','D','E','A/ONE','A/ONE/TWO','A/ONE/TWO/THREE']: run('mmd','-i',str(folders),'::/'+s)
(R/'evidence/folders-before.txt').write_text(run('mdir','-i',str(folders),'::/'))
full=R/'images/full-data.img';shutil.copyfile(seed,full)
free=1455616
filler=R/'evidence/filler.bin'; filler.write_bytes(b'F'*free)
run('mcopy','-i',str(full),str(filler),'::/FILLER.BIN')
(R/'evidence/full-before.txt').write_text(run('mdir','-i',str(full),'::/'))
filler.unlink()
(R/'evidence/disk-preparation.json').write_text(json.dumps({'operations':log,'images':{str(p.relative_to(R)):{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size} for p in [many,folders,full]}},indent=2))
print('Created many-files, many-folders, and zero-free-space disk copies.')
