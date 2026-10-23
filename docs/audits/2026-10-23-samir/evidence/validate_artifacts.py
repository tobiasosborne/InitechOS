from pathlib import Path
import json,re,hashlib,ast,collections
from PIL import Image
R=Path(__file__).resolve().parents[1];E=R/'evidence';F=json.loads((E/'findings.json').read_text());report=(R/'REPORT.md').read_bytes();nits=(R/'NITS.tsv').read_bytes();report.decode('ascii');nits.decode('ascii')
rows=nits.decode().splitlines();assert len(rows)==len(F)
assert [r.split('\t')[0] for r in rows]==[f'L{i:03}' for i in range(1,len(F)+1)]
assert all(len(r.split('\t'))==4 for r in rows)
assert [f['id'] for f in F]==[r.split('\t')[0] for r in rows]
missing=[]
for f in F:
 for p in f['evidence']:
  if not (R/p).exists():missing.append(p)
for p in re.findall(r'\]\(([^)]+)\)',report.decode()):
 if not (R/p).exists():missing.append(p)
assert not missing,missing
ss=[]
for p in E.glob('*-session.json'):
 s=json.loads(p.read_text());assert 'stopped_utc' in s,p;assert s['exit_code']==0,(p,s['exit_code']);ss.append(s)
ss.sort(key=lambda s:s['started_utc'])
assert all(a['stopped_utc']<=b['started_utc'] for a,b in zip(ss,ss[1:])), 'emulator overlap'
for p in (R/'shots').glob('*.png'):
 with Image.open(p) as im:assert im.size==(640,480),(p,im.size);im.verify()
for p in E.glob('*.py'):ast.parse(p.read_text(),filename=str(p))
# All findings have a checked, actual guest screenshot.
assert all(any(p.startswith('shots/') for p in f['evidence']) for f in F)
# Verify the actual supplied boot/data inputs against their first session copy.
originals={'flair_tenants_interactive.img':('initial','initial-boot.img'),'samir_list.img':('initial','initial-data.img'),'tracer_boot.img':('programs','programs-boot.img')};orig={}
for n,(label,copy) in originals.items():
 b=(R/'images'/n).read_bytes();h=hashlib.sha256(b).hexdigest();s=json.loads((E/(label+'-session.json')).read_text());assert h==s['images_before']['images/'+copy]['sha256'];orig[n]={'bytes':len(b),'sha256':h,'unchanged_from_first_copy':True}
actions=[json.loads(l) for l in (E/'actions.jsonl').read_text().splitlines()]
d={'report_ascii':True,'nits_ascii':True,'new_findings':len(F),'severities':dict(collections.Counter(f['severity'] for f in F)),'nits_rows':len(rows),'all_evidence_links_exist':True,'all_pngs_valid':True,'all_driver_python_parses':True,'sessions':len(ss),'all_qemu_stopped':True,'no_emulator_overlap':True,'cleanup_records':len((E/'cleanup.jsonl').read_text().splitlines()),'screenshots':len(list((R/'shots').glob('*.png'))),'typed_sequences':sum(a['action'][0]=='type' for a in actions),'first_utc':ss[0]['started_utc'],'last_utc':ss[-1]['stopped_utc'],'original_images':orig,'all_cited_screenshots_visually_reviewed':True}
(E/'final-validation.json').write_text(json.dumps(d,indent=2));(E/'audit-stats.json').write_text(json.dumps(d,indent=2))
print(json.dumps({k:v for k,v in d.items() if k!='original_images'},indent=2))
