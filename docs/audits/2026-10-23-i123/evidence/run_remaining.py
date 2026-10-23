import pathlib,subprocess,time,json,os,sys
root=pathlib.Path(__file__).resolve().parents[1]
# Wait for this audit's menu process to close its recorded emulator before starting another.
p=root/'evidence/menus-session.json'
while not p.exists() or 'stopped_utc' not in json.loads(p.read_text()):time.sleep(1)
for phase in ['files','capacity','transfers','functions','formats','full','keyslook']:
 print('START_PHASE',phase,flush=True)
 with open(root/'evidence'/f'{phase}-run.log','w') as f:
  r=subprocess.run([sys.executable,str(root/'evidence/phases.py'),phase],cwd=root,stdout=f,stderr=subprocess.STDOUT)
 if r.returncode:print('PHASE_FAILED',phase,r.returncode,flush=True);break
 subprocess.run([sys.executable,str(root/'evidence/extract.py'),phase],cwd=root,check=True)
 print('DONE_PHASE',phase,flush=True)
