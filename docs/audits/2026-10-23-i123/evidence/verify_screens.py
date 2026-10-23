import json,pathlib,hashlib
from PIL import Image
root=pathlib.Path(__file__).resolve().parents[1]
f=json.load(open(root/'evidence/final-findings.json'))
rows=[]
for x in f:
 for s in x['shots']:
  p=root/'shots'/(s+'.png');im=Image.open(p);im.verify()
  rows.append({'finding':x['id'],'screenshot':str(p.relative_to(root)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'check':'PNG decoded; observation validation is in menu-verified.json or manual screenshot review'})
(root/'evidence/screenshot-manifest.json').write_text(json.dumps(rows,indent=2))
print('Referenced valid PNGs',len(rows))
