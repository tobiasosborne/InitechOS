#!/usr/bin/env python3
"""Audit QMP-only driver. Run from workspace; input is one JSON action per line."""
import sys,json,time,pathlib
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]))
from audit_driver import Driver,ROOT
class Audit(Driver):
 def shot(self,name):
  time.sleep(.35)
  super().shot(name)
 def repeat(self,n,action):
  for _ in range(n): self.run(action)
 def slowtype(self,s,enter=True,delay=.13):
  for c in s:
   if c in '\r\n': self.key('ret',delay)
   else: self.type(c,False);time.sleep(delay)
  if enter:self.key('ret',.5)
 def play(self,path):
  for a in json.loads((ROOT/path).read_text()): self.run(a)
 def reset(self):
  self.log(['reset']);self.cmd('system_reset');self.pause(12)
d=Audit()
try:
 for line in sys.stdin:
  try:
   a=json.loads(line)
   with open(ROOT/'evidence/commands.jsonl','a') as f:f.write(json.dumps(a)+'\n')
   d.run(a);print('OK',flush=True)
  except Exception as e: print('ERROR',repr(e),flush=True)
finally:
 if d.proc is not None:d.quit()
