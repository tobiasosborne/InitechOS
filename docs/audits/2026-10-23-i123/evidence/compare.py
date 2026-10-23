import json,pathlib,hashlib,sys
from extract import parse
root=pathlib.Path(__file__).resolve().parents[1]
ref=pathlib.Path('/home/tobias/Projects/lotus123-decomp/goldens/minted')
res=[]
for label,native,golden in [('long','M1ROUND','MINT01'),('capacity','M3ROUND','MINT03'),('capacity','M4ROUND','MINT04'),('capacity','M5ROUND','MINT05'),('transfers','COPYB','COPY01B'),('transfers','COPYC','COPY01C'),('transfers','COPYD','COPY01D'),('transfers','MOVEB','MOVE01B'),('transfers','MOVEC','MOVE01C'),('transfers','MOVED','MOVE01D')]:
 p=root/'evidence/extracted'/label/(native+'.WK1')
 if not p.exists():continue
 a=parse(p);b=parse(ref/(golden+'.WK1'))
 aa={c['cell']:c for c in a['cells']};bb={c['cell']:c for c in b['cells']}
 diff=[{'cell':k,'native':aa.get(k),'real':bb.get(k)} for k in sorted(aa.keys()|bb.keys()) if aa.get(k)!=bb.get(k)]
 res.append({'session':label,'native':native,'real':golden,'native_bytes':a['bytes'],'real_bytes':b['bytes'],'file_identical':a['sha256']==b['sha256'],'cell_records_identical':not diff,'cell_differences':diff})
(root/'evidence/corpus-comparison.json').write_text(json.dumps(res,indent=2))
for r in res:print(r['native'],r['real'],'identical',r['file_identical'],'cell diffs',len(r['cell_differences']))
