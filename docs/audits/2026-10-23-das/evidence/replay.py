#!/usr/bin/env python3
"""Replay one recorded QMP session from actions.jsonl with its measured timing.
Usage: python3 evidence/replay.py SESSION NEW_LABEL
Runs one emulator, makes image copies, preserves only low-level input events.
Do not reuse a recorded label: that would overwrite the original evidence.
Prepared input images are retained in ../images; prepare*.py explain their bytes.
"""
import json,sys,time,datetime,pathlib
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from audit_driver import Driver
name,label=sys.argv[1:3]
if (ROOT/'evidence'/f'{label}-session.json').exists():raise SystemExit('Use a new label')
rows=[json.loads(s) for s in (ROOT/'evidence/actions.jsonl').read_text().splitlines()]
rows=[r for r in rows if r['session']==name and r['serial_bytes'] is not None and r['action'][0] in ['start','rel','button','key','keyheld','shot','pause','quit']]
d=Driver();wall=None;record=None
try:
 for row in rows:
  a=row['action'];ts=datetime.datetime.fromisoformat(row['utc']).timestamp()
  if a[0]=='start':
   d.start(label,*a[2:]);wall=time.monotonic();record=ts;continue
  delay=wall+(ts-record)-time.monotonic()
  while delay>0:
   time.sleep(min(delay,1));delay=wall+(ts-record)-time.monotonic()
  if a[0]=='shot':a=['shot',label+'-'+a[1]]
  if a[0]=='key':a=['key',a[1],0]
  if a[0]=='pause':
   # dalife is the sole recorded explicit QMP reset: its 12-second pause
   # follows the first shutdown screenshot. The original reset method
   # recorded its following pause but not system_reset itself.
   if name=='dalife' and a[1]==12:d.cmd('system_reset')
   continue # spacing already represented by timestamps
  d.run(a)
finally:
 if d.proc is not None:d.quit()
