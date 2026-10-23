import pathlib,subprocess,time,json,sys
root=pathlib.Path(__file__).resolve().parents[1]
p=root/'evidence/full-session.json'
while not p.exists() or 'stopped_utc' not in json.loads(p.read_text()):time.sleep(1)
for phase in ['keyslook','hotkeys','typeerrors']:
 print('START_EXTRA',phase,flush=True)
 with open(root/'evidence'/f'{phase}-run.log','w') as f:
  r=subprocess.run([sys.executable,str(root/'evidence/phases.py'),phase],cwd=root,stdout=f,stderr=subprocess.STDOUT)
 if r.returncode:print('EXTRA_FAILED',phase,r.returncode,flush=True);break
 subprocess.run([sys.executable,str(root/'evidence/extract.py'),phase],cwd=root,check=True)
 print('DONE_EXTRA',phase,flush=True)
