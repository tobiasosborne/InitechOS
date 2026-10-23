import pathlib,json,re,hashlib
from PIL import Image
from drive123 import ROOT,MENU,REPO,levels
refs=(REPO/'spec/i123/menu_ri_offsets.tsv').read_text().splitlines()
ri=pathlib.Path('/home/tobias/Projects/lotus123-decomp/goldens/lotus-123-2.2-installed-ref/extracted/installed/123.RI').read_bytes()
rows=json.loads((ROOT/'evidence/menu-results.json').read_text())
log=(ROOT/'evidence/menus-serial.log').read_bytes()
actions=[json.loads(x) for x in (ROOT/'evidence/actions.jsonl').read_text().splitlines()]
shots={a['action'][1]:a['serial_bytes'] for a in actions if a['session']=='menus' and a['action'][0]=='shot'}
refim=Image.open(ROOT/'shots/menu-040-result.png').crop((7,78,615,110)).tobytes()
for r in rows:
 level=0
 for depth,idx in enumerate(r['indices']):
  first,count=levels[level]; item=MENU[level][idx]
  if depth<len(r['indices'])-1:level=item['child']
  else:ordinal=first+idx
 path,no,do=refs[ordinal].split('\t');no=int(no,16);do=int(do,16)
 r['ri_name_offset']=no;r['ri_description_offset']=do
 n=ri[no:ri.index(b'\0',no)].decode('ascii');d=ri[do:ri.index(b'\0',do)].decode('ascii')
 r['ri_name']=n;r['ri_description']=d;r['resource_matches_reference']=(n==r['path'][-1] and d==r['description'])
 tag=f"menu-{r['n']:03}";end=shots[tag+'-selected']
 traces=re.findall(rb'TENANT-GATE ax=0x0033 win=1 x=1 y=32 s="([^\r\n]*)" fg=',log[max(0,end-20000):end])
 visible=traces[-1].decode('ascii') if traces else ''
 r['visible_description']=visible;r['description_matches_visible_prefix']=(visible.rstrip()==d[:76].rstrip())
 r['description_clipped']=len(d)>76
 p=ROOT/'shots'/(tag+'-result.png')
 r['visible_unsupported']=p.exists() and Image.open(p).crop((7,78,615,110)).tobytes()==refim
 r['is_leaf']=item['child']==65535
(ROOT/'evidence/menu-verified.json').write_text(json.dumps(rows,indent=2))
print(json.dumps({'count':len(rows),'unsupported':sum(r['visible_unsupported'] for r in rows),'reference_mismatches':[r['n'] for r in rows if not r['resource_matches_reference']],'render_mismatches':[r['n'] for r in rows if not r['description_matches_visible_prefix']],'clipped':[r['n'] for r in rows if r['description_clipped']]},indent=2))
