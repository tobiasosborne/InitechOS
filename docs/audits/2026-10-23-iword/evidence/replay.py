#!/usr/bin/env python3
"""Replay one recorded QEMU session into a NEW output directory.
Uses primitive records only, so composite type/click/move records do not double input.
Example: python3 evidence/replay.py s02 /tmp/iword-replay-s02
"""
import sys,json,pathlib,shutil,time,datetime
SRC=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(SRC))
import audit_driver
label,out=sys.argv[1],pathlib.Path(sys.argv[2]).resolve()
if out.exists(): raise SystemExit('Refusing an existing output directory')
(out/'images').mkdir(parents=True);(out/'shots').mkdir();(out/'evidence').mkdir()
rows=[json.loads(x) for x in (SRC/'evidence/actions.jsonl').read_text().splitlines()]
rows=[r for r in rows if r['session']==label]
if not rows:raise SystemExit('Unknown session')
for r in rows:
 if r['action'][0]=='start':
  for p in r['action'][2:4]:
   if p:shutil.copyfile(SRC/p,out/p)
audit_driver.ROOT=out
d=audit_driver.Driver()
primitives={'start','key','keyheld','rel','button','pause','shot','quit','note'}
try:
 origin=None; started=None
 for r in rows:
  if r['action'][0] not in primitives:continue
  when=datetime.datetime.fromisoformat(r['utc']).timestamp()
  if origin is None:origin=when;started=time.monotonic()
  deadline=started+when-origin
  while True:
   remaining=deadline-time.monotonic()
   if remaining<=0:break
   time.sleep(min(30,remaining))
  d.run(r['action'])
finally:
 if d.proc is not None:d.quit()
