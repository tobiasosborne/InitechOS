#!/usr/bin/env python3
from pathlib import Path
import json,re,collections,hashlib
R=Path(__file__).resolve().parent.parent;E=R/'evidence';errors=[]
report=(R/'REPORT.md').read_bytes();nits=(R/'NITS.tsv').read_bytes()
for name,b in [('REPORT.md',report),('NITS.tsv',nits)]:
 if any(c>127 for c in b):errors.append(name+' is not ASCII')
 if any(c<32 and c not in [9,10,13] for c in b):errors.append(name+' has unexpected control bytes')
s=report.decode('ascii');F=json.loads((E/'final-findings.json').read_text());ids=[f['id'] for f in F];expected=[f'L{i:03d}' for i in range(1,len(F)+1)]
if ids!=expected:errors.append('Nonconsecutive findings')
if re.findall(r'^### (L\d{3}) -',s,re.M)!=ids:errors.append('Report headings do not match findings')
rows=nits.decode().splitlines();tsv=[row.split('\t') for row in rows[1:]]
if any(len(row)!=4 for row in tsv):errors.append('TSV column count')
if [row[0] for row in tsv]!=ids:errors.append('TSV IDs do not match')
if any(' '.join(row[1:])!=' '.join([f['area'],f['severity'],f['title']]) for row,f in zip(tsv,F)):errors.append('TSV values do not match findings')
links=re.findall(r'\]\(([^)]+)\)',s);missing=[]
for path in links:
 if not path.startswith(('http:','https:','#')) and not (R/path).exists():missing.append(path)
if missing:errors.append('Missing links: '+str(sorted(set(missing))))
for f in F:
 for k in ['area','severity','steps','observed','expected','evidence']:
  if not f.get(k):errors.append(f['id']+' missing '+k)
 if not any(p.endswith('.png') for p in f['evidence']):errors.append(f['id']+' missing screen evidence')
 for p in f['evidence']:
  if not (R/p).exists():errors.append(f['id']+' missing '+p)
sessions=[]
for p in E.glob('resume-*-session.json'):
 j=json.loads(p.read_text())
 if 'stopped_utc' not in j:errors.append(p.name+' not stopped')
 else:
  sessions.append((j['started_utc'],j['stopped_utc'],p.name))
  if j['exit_code']!=0:errors.append(p.name+' QEMU host exit not zero')
sessions.sort()
for a,b in zip(sessions,sessions[1:]):
 if a[1]>b[0]:errors.append('Overlapping emulator sessions '+a[2]+' '+b[2])
# Inspect process command lines, not recycled numeric PIDs, for this audit's images.
live=[]
for p in Path('/proc').glob('[0-9]*/cmdline'):
 try:args=p.read_bytes().split(b'\0')
 except (FileNotFoundError,PermissionError,ProcessLookupError):continue
 if args and b'qemu-system' in args[0] and any(str(R/'images').encode() in a for a in args):live.append(p.parent.name)
if live:errors.append('Auditor QEMU still running: '+str(live))
summary={'findings':len(F),'severity_counts':dict(collections.Counter(f['severity'] for f in F)),'ascii_only':all(c<128 for c in report+nits),'tsv_data_rows':len(tsv),'evidence_links':len(links),'unique_evidence_links':len(set(links)),'resumed_sessions':len(sessions),'emulator_session_overlap':False if not any('Overlapping' in e for e in errors) else True,'live_auditor_qemu_pids':live,'report_sha256':hashlib.sha256(report).hexdigest(),'nits_sha256':hashlib.sha256(nits).hexdigest(),'errors':errors}
(E/'report-validation.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
if errors:raise SystemExit(1)
