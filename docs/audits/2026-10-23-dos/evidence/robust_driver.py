from pathlib import Path
import sys,time,re
R=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(R));from audit_driver import Driver
class Robust(Driver):
 def waitfor(self,pattern,start=0,timeout=15):
  until=time.monotonic()+timeout
  while time.monotonic()<until:
   tail=self.serial.read_bytes()[start:]
   if re.search(pattern,tail):return tail
   time.sleep(.03)
  self.log(['wait-timeout',str(pattern),start,timeout]);raise TimeoutError((pattern,tail[-200:]))
 def command(self,text,timeout=15):
  start=self.serial.stat().st_size;self.type(text);return self.waitfor(rb'[A-Z]:\\[^\r\n>]*>\Z',start,timeout)
 def enter(self,text,pattern,timeout=15):
  start=self.serial.stat().st_size;self.type(text);return self.waitfor(pattern,start,timeout)
 def keywait(self,key,pattern,timeout=15):
  start=self.serial.stat().st_size;self.key(key);return self.waitfor(pattern,start,timeout)
