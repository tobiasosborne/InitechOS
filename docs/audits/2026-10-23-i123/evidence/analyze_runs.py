import pathlib,json,re
from PIL import Image
root=pathlib.Path(__file__).resolve().parents[1]
records={}
for session in ['functions','function_errors','formats','formats_wide','typeerrors']:
 p=root/'evidence'/f'{session}-serial.log'
 if not p.exists():continue
 log=p.read_bytes();actions=[json.loads(x) for x in (root/'evidence/actions.jsonl').read_text().splitlines()]
 rows=[]
 for a in actions:
  if a['session']!=session or a['action'][0]!='shot':continue
  end=a['serial_bytes'];text=log[max(0,end-20000):end]
  lines={}
  for m in re.finditer(rb'TENANT-GATE ax=0x0033 win=1 x=(\d+) y=(\d+) s="([^\r\n]*)" fg=(\d+) bg=(\d+)',text):
   x,y=int(m[1]),int(m[2]);lines[str((x,y))]=m[3].decode('ascii')
  rows.append({'shot':a['action'][1],'last_rendered_lines':lines})
 records[session]=rows
(root/'evidence/rendered-case-lines.json').write_text(json.dumps(records,indent=2))
print({s:len(v) for s,v in records.items()})
