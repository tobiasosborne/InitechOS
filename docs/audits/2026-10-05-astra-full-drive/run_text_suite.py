#!/usr/bin/env python3
"""Keyboard-driven suite; one guest at a time, stop even on errors."""
import json,sys,time
from audit_driver import Driver,ROOT
d=Driver()
suite=json.loads((ROOT/sys.argv[1]).read_text())
try:
 d.start(suite['label'],suite['boot'],suite['data'])
 for a in suite['actions']:
  before=d.serial.stat().st_size
  d.run(a)
  if suite.get('sync') and a[0]=='type' and (len(a)<3 or a[2]):
   start=time.monotonic()
   while time.monotonic()-start<20:
    data=d.serial.read_bytes()
    if len(data)>before and (data.endswith(b'. ') or b'FLAIR-RESUME' in data[before:]): break
    time.sleep(.1)
   d.log(['command-settle',a[1],round(time.monotonic()-start,3),d.serial.read_bytes()[-150:].decode(errors='replace')])
   if time.monotonic()-start>=20:
    d.shot(suite['label']+'-timeout'); raise RuntimeError('No prompt after '+a[1])
finally:
 if d.proc: d.quit()
