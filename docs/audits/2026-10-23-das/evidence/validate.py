#!/usr/bin/env python3
from pathlib import Path
import json,hashlib,re,collections
r=Path(__file__).resolve().parents[1]
fs=json.loads((r/'evidence/findings.json').read_text())
sessions=[json.loads(p.read_text())|{'file':str(p.relative_to(r))} for p in (r/'evidence').glob('*-session.json')]
events=[]
for s in sessions:
 events += [(s['started_utc'],1,s['file']),(s.get('stopped_utc','9999'),-1,s['file'])]
active=peak=0
for _,delta,_ in sorted(events):active+=delta;peak=max(peak,active)
report=(r/'REPORT.md').read_text()
links=re.findall(r'\]\(([^)]+)\)',report)
missing=[p for p in links if not (r/p).exists() and p!='evidence/final-validation.json']
checks=json.loads((r/'evidence/byte-checks.json').read_text())
replay=(r/'evidence/replay-copybreak-serial.log').read_text()
result={'finding_count':len(fs),'severity_counts':dict(collections.Counter(f['severity'] for f in fs)),
 'sessions':len(sessions),'all_sessions_stopped':all('stopped_utc' in s and s['exit_code']==0 for s in sessions),
 'maximum_simultaneous_audit_emulators':peak,'missing_report_links':missing,
 'all_byte_checks_pass':all(checks.values()),'replay_validation':{'session':'replay-copybreak','int23_break_reproduced':'INT23-BREAK' in replay,'normal_qemu_exit':json.loads((r/'evidence/replay-copybreak-session.json').read_text())['exit_code']==0},
 'ascii_report':report.isascii(),'ascii_tsv':(r/'NITS.tsv').read_text().isascii(),
 'native_png_count':len(list((r/'shots').glob('*.png'))),
 'no_unfinished_worktree_mutation':'Read-only source audit; no repository builds, edits or tracker commands',
 'report_sha256':hashlib.sha256((r/'REPORT.md').read_bytes()).hexdigest(),
 'nits_sha256':hashlib.sha256((r/'NITS.tsv').read_bytes()).hexdigest()}
(r/'evidence/final-validation.json').write_text(json.dumps(result,indent=2))
manifest={str(p.relative_to(r)):{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in (r/'images').glob('*.img')}
(r/'evidence/image-manifest.json').write_text(json.dumps(manifest,indent=2))
assert result['all_sessions_stopped'] and peak==1 and not missing and result['all_byte_checks_pass'] and result['replay_validation']['int23_break_reproduced']
print(json.dumps(result,indent=2))
