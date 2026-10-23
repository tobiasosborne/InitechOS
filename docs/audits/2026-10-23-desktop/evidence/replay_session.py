#!/usr/bin/env python3
"""Replay recorded QMP inputs, one private emulator at a time.

Usage from the audit directory:
  python3 evidence/replay_session.py keys01
  python3 evidence/replay_session.py edge01 --skip-idle

The default preserves recorded inter-event times. --skip-idle caps gaps at
two seconds, so do not use it to grade long holds or double-click timing.
Outputs use replay- prefixes and never replace the audit evidence.
"""
import argparse, datetime, importlib.util, json, pathlib, time
ROOT=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(); p.add_argument('session'); p.add_argument('--skip-idle',action='store_true')
args=p.parse_args()
spec=importlib.util.spec_from_file_location('audit_driver',ROOT/'audit_driver.py')
mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
rows=[json.loads(s) for s in (ROOT/'evidence/actions.jsonl').read_text().splitlines()]
rows=[r for r in rows if r['session']==args.session]
start=next(r['action'] for r in rows if r['action'][0]=='start')
atomic={'rel','button','key','keyheld','shot','quit'}
rows=[r for r in rows if r['action'][0] in atomic]
if not rows or rows[-1]['action'][0]!='quit': raise SystemExit('Session is incomplete')
d=mod.Driver()
try:
 d.start('replay-'+args.session,*start[2:])
 previous=None; deadline=time.monotonic()
 for r in rows:
  stamp=datetime.datetime.fromisoformat(r['utc']).timestamp()
  if previous is not None:
   gap=stamp-previous
   deadline+=min(gap,2) if args.skip_idle else gap
  previous=stamp
  while time.monotonic()<deadline:
   time.sleep(min(.2,deadline-time.monotonic()))
  action=list(r['action'])
  if action[0]=='shot': action[1]='replay-'+args.session+'-'+action[1]
  d.run(action)
finally:
 if d.proc is not None: d.quit()
