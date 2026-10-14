#!/usr/bin/env python3
from pathlib import Path
from PIL import Image
import numpy as np,json,hashlib,re
R=Path(__file__).resolve().parent
fail=[];checks=0
def check(ok,msg):
 global checks
 checks+=1
 if not ok:fail.append(msg)
def a(p):return np.array(Image.open(R/p).convert('RGB'))
def rgbcount(p,r,c):
 x=a(p)[r[1]:r[3],r[0]:r[2]];return int(np.count_nonzero(np.all(x==c,2)))
rep=(R/'REPORT.md').read_bytes();check(all(b<128 for b in rep),'REPORT must be ASCII')
f=json.loads((R/'evidence/findings.json').read_text());rows=(R/'NITS.tsv').read_text().splitlines()[1:]
check(len(f)==28 and len(rows)==28,'28 findings/TSV rows')
for i,x in enumerate(f,1):
 check(x['id']==f'M{i:03}',f'ID {i}')
 check(rows[i-1].split('\t')==[x['id'],x['area'],x['severity'],x['summary']],x['id']+' TSV mismatch')
 for p,r in [(x['screenshot'],x['initech_crop']),(x['reference_file'],x['reference_crop'])]:
  im=Image.open(R/p);check(bool(im.width and im.height),p+' image')
  if r:check(0<=r[0]<r[2]<=im.width and 0<=r[1]<r[3]<=im.height,p+' crop bounds')
 for p in [x['diff'],'diffs/'+x['id']+'-initech.png','refs/'+x['id']+'-reference.png']:
  im=Image.open(R/p);im.verify();check(True,p+' decodes')
check(rgbcount('shots/042-many-root.png',[360,97,374,112],(141,220,220))==205,'thumb face count')
check(rgbcount('shots/042-many-root.png',[360,97,374,112],(243,243,243))==5,'thumb caps')
check(rgbcount('refs/s8_doc_window_inactive.png',[608,311,622,326],(84,84,179))==28,'reference grips')
check(rgbcount('shots/016-view-menu.png',[100,40,111,55],(0,0,0))==26,'v pixels')
check(rgbcount('refs/chicago-checkmark.png',[0,0,11,15],(0,0,0))==18,'checkmark pixels')
check(rgbcount('shots/022-root-small.png',[130,63,134,78],(0,0,0))==4,'period pixels')
check(rgbcount('refs/chicago-ellipsis.png',[0,0,14,15],(0,0,0))==12,'ellipsis pixels')
check(rgbcount('shots/031-123-save-alert.png',[365,160,419,161],(255,255,255))==11,'button highlight')
check(rgbcount('refs/s8_alert_modal.png',[365,160,419,161],(255,255,255))==54,'reference button highlight')
check(rgbcount('shots/031-123-save-alert.png',[365,175,419,176],(192,192,192))==10,'button shadow')
check(rgbcount('refs/s8_alert_modal.png',[365,175,419,176],(192,192,192))==53,'reference button shadow')
check(rgbcount('shots/087-dos-settled-dir.png',[200,128,640,160],(111,160,142))==14080,'DIR empty timestamp columns')
check(rgbcount('shots/079-db-status.png',[0,432,640,480],(111,160,142))==30720,'DB blank bottom')
check(rgbcount('shots/079-db-status.png',[0,352,640,400],(111,160,142))==30720,'DB historical status rows blank')
check(rgbcount('shots/105-film-steady.png',[306,142,538,198],(255,255,255))==12992,'film chart blank')
check(rgbcount('shots/105-film-steady.png',[66,102,299,198],(255,255,255))==22368,'film worksheet blank')
check(rgbcount('shots/120-film-arrow-on-modal.png',[469,227,485,243],(0,0,0))==61,'film arrow ink')
check(rgbcount('shots/120-film-arrow-on-modal.png',[469,227,485,243],(255,255,255))==53,'film arrow mask')
fields=json.loads((R/'evidence/dbf-input-fields.json').read_text());check(fields['record_2_BAL']['stored_ascii']=='  -42.00','stored BAL precision');check(fields['reference_UNITCOST']['decimals']==2,'reference numeric precision')
cycles=json.loads((R/'evidence/cursor-cycle.json').read_text());check(len(set(x['sha256'] for x in cycles))==1 and cycles[-1]['ms']>2000,'cursor sampled across 2 s')
check(np.array_equal(a('shots/092-safe-before-click.png'),a('shots/093-safe-after-text-click.png')),'reject false shutdown disappearance')
collapse=json.loads((R/'evidence/collapse-check.json').read_text());check(len(set(x['title_sha'] for x in collapse.values()))==1,'collapsed title unchanged')
for p in sorted((R/'evidence').glob('*-session.json')):
 x=json.loads(p.read_text());check(x.get('exit_code')==0,p.name+' QEMU must be stopped')
 check('-display' in x['command'] and x['command'][x['command'].index('-display')+1]=='none',p.name+' headless')
 check('-qmp' in x['command'] and x['command'][x['command'].index('-qmp')+1]=='stdio',p.name+' QMP stdio')
 for arg in x['command']:
  if arg.startswith('format=raw,file=') or arg.startswith('file='):
   check('-boot.img' in arg or '-data.img' in arg,p.name+' disk must be copy')
for group in ['source-image-manifest.json','screenshot-index.json']:
 data=json.loads((R/'evidence'/group).read_text());data=data['images'] if isinstance(data,dict) else data
 for x in data:
  p=R/x.get('path',x.get('file',''));check(hashlib.sha256(p.read_bytes()).hexdigest()==x['sha256'],str(p)+' hash')
for href in re.findall(r'\]\(([^)]+)\)',rep.decode('ascii')):
 check((R/href).exists(),'broken report link '+href)
result={'checks':checks,'failures':fail,'finding_count':len(f),'screenshots':len(list((R/'shots').glob('*.png'))),'session_records':len(list((R/'evidence').glob('*-session.json'))),'all_session_records_stopped':not any('QEMU must be stopped' in x for x in fail)}
(R/'evidence/final-validation.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2))
if fail:raise SystemExit(1)
