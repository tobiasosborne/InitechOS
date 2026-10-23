from pathlib import Path
import subprocess,shutil,json
r=Path(__file__).resolve().parents[1];f=r/'evidence/copy-fixtures';f.mkdir(exist_ok=True)
def run(*a):return subprocess.run(a,cwd=r,check=True,capture_output=True).stdout
cases={'zero':(b'PROGRESS_BAR_RATE=0\r\n',b'zero rate sentinel\r\n'),'hundred':(b'PROGRESS_BAR_RATE=100\r\n',b''.join(('Ledger row %04d\r\n'%i).encode() for i in range(1000))),'bad':(b'PROGRESS_BAR_RATE=banana\r\n',b'bad rate sentinel\r\n'),'empty':(b'',b''),'missing':(None,None)}
A=[]
for i,(name,(cfg,ledger)) in enumerate(cases.items()):
 dst='images/copy-'+name+'-source.img';shutil.copyfile(r/'images/flair_data.img',r/dst)
 for fn,b in [('MICHAEL.CFG',cfg),('LEDGER.TXT',ledger)]:
  run('mdel','-i',dst,'::/APPS/'+fn)
  if b is not None:
   p=f/(name+'-'+fn);p.write_bytes(b);run('mcopy','-i',dst,str(p),'::/APPS/'+fn)
 # Existing output to expose overwrite and missing-input transactional behavior.
 old=f/'old-table.txt';old.write_bytes(b'ORIGINAL TABLES MUST SURVIVE FAILED COPY\r\n')
 run('mcopy','-i',dst,str(old),'::/APPS/TABLES.TXT')
 A += [['start','copy-'+name,'images/flair_tenants_interactive.img',dst],['click',600,64,2],['click',123,101,2],['click',150,172,2],['key','ctrl-s'],['shot',str(170+i*2)+'-copy-'+name+'-start'],['pause',7],['shot',str(171+i*2)+'-copy-'+name+'-end'],['quit']]
(r/'evidence/copy-actions.json').write_text(json.dumps(A,indent=2))
