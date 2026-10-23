#!/usr/bin/env python3
"""Read-only guest-disk checks after all recorded emulators have stopped."""
import pathlib,json,re,hashlib,subprocess,collections
from PIL import Image
R=pathlib.Path(__file__).resolve().parents[1]
report=(R/'REPORT.md').read_bytes();report.decode('ascii')
tsv=(R/'NITS.tsv').read_text().splitlines();assert len(tsv)==10
assert all(len(x.split('\t'))==4 for x in tsv)
links=re.findall(r'\]\(((?:shots|evidence)/[^)]+)\)',report.decode())
missing=[p for p in links if not (R/p).is_file()]
# Also check literal filename references in narrative/table text.
names=re.findall(r'(?<![/\w.-])(?:shots|evidence)/[A-Za-z0-9_./-]+\.(?:png|jsonl|json|txt|py|log|BAT|TXT|tex)(?![A-Za-z])',report.decode())
missing+= [p for p in names if p!='evidence/final-validation.json' and not (R/p).is_file()]
assert not missing,missing
pngs=list((R/'shots').glob('*.png'))
for p in pngs:
 with Image.open(p) as im:assert im.size==(640,480);im.verify()
sessions={}
for p in sorted((R/'evidence').glob('*-session.json')):
 j=json.loads(p.read_text());assert 'stopped_utc' in j and j['exit_code']==0,(p,j)
 label=p.name.removesuffix('-session.json');s=(R/'evidence'/f'{label}-serial.log').read_text(errors='replace')
 bad=[line for line in s.splitlines() if re.search(r'FLAIR-EVT-DROP|PANIC|PC LOAD LETTER',line)]
 sessions[label]={'qemu_pid':j['qemu_pid'],'exit_code':j['exit_code'],'serial_bad_markers':bad,'serial_bytes':len(s.encode())}
 data=R/'images'/f'{label}-data.img'
 if data.exists():
  c=subprocess.run(['fsck.fat','-n',str(data)],capture_output=True,text=True)
  (R/'evidence'/f'{label}-fsck.txt').write_text(c.stdout+c.stderr)
  sessions[label]['fsck_exit']=c.returncode
actions=[json.loads(x) for x in (R/'evidence/actions.jsonl').read_text().splitlines()]
counts={}
for label in sessions:
 c=collections.Counter(r['action'][0] for r in actions if r['session']==label)
 counts[label]={k:c[k] for k in ['click','drag','key','keyheld','menu','selectmenu','shot']}
result={'report_ascii':True,'new_findings':9,'pngs_verified':len(pngs),'cited_paths_missing':missing,'sessions':sessions,'high_level_action_counts':counts,'report_sha256':hashlib.sha256(report).hexdigest()}
(R/'evidence/final-validation.json').write_text(json.dumps(result,indent=2))
print(json.dumps({'new_findings':9,'pngs':len(pngs),'sessions':len(sessions),'missing':missing,'bad_markers':sum(len(s['serial_bad_markers']) for s in sessions.values()),'fsck_nonzero':{n:s['fsck_exit'] for n,s in sessions.items() if s.get('fsck_exit',0)}},indent=2))
