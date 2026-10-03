import subprocess,json,time,sys,pathlib,datetime,hashlib,os
from PIL import Image
ROOT=pathlib.Path(__file__).resolve().parent
class Driver:
 def __init__(self):
  cmd=['qemu-system-i386','-cpu','486','-m','16','-drive','format=raw,file=boot.img','-drive','file=data.img,format=raw,if=ide,index=1','-serial','file:evidence/serial.log','-display','none','-qmp','stdio']
  self.proc=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=open(ROOT/'evidence/qemu-stderr.log','wb'))
  self.fin=self.proc.stdout; self.fout=self.proc.stdin
  self.read(); self.cmd('qmp_capabilities')
  files={n:{'bytes':(ROOT/n).stat().st_size,'sha256':hashlib.sha256((ROOT/n).read_bytes()).hexdigest()} for n in ['boot.img','data.img']}
  (ROOT/'evidence/session.json').write_text(json.dumps({'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'qemu_pid_in_namespace':self.proc.pid,'driver_pid_in_namespace':os.getpid(),'command':cmd,'images_before':files},indent=2))
  print('Private QEMU started',self.proc.pid,flush=True)
 def read(self):
  return json.loads(self.fin.readline())
 def cmd(self,name,args=None):
  self.fout.write((json.dumps({'execute':name,'arguments':args or {}})+'\n').encode()); self.fout.flush()
  while True:
   r=self.read()
   if 'error' in r: raise RuntimeError(r)
   if 'return' in r: return r['return']
 def log(self,action):
  with open(ROOT/'evidence/actions.jsonl','a') as f: f.write(json.dumps({'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'action':action,'serial_bytes':(ROOT/'evidence/serial.log').stat().st_size})+'\n')
 def rel(self,x,y):
  n=max(1,(max(abs(x),abs(y))+79)//80)
  for i in range(n):
   dx=round((i+1)*x/n)-round(i*x/n); dy=round((i+1)*y/n)-round(i*y/n)
   self.cmd('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':dx}},{'type':'rel','data':{'axis':'y','value':dy}}]}); time.sleep(.025)
 def move(self,x,y):
  self.log(['move',x,y]); self.rel(-800,-600); self.rel(x,y); time.sleep(.15)
 def button(self,down,button='left'):
  self.cmd('input-send-event',{'events':[{'type':'btn','data':{'button':button,'down':down}}]})
 def click(self,x,y,count=1,button='left'):
  self.log(['click',x,y,count,button]); self.move(x,y)
  for i in range(count): self.button(True,button); time.sleep(.06); self.button(False,button); time.sleep(.1)
  time.sleep(.3)
 def drag(self,x,y,dx,dy):
  self.log(['drag',x,y,dx,dy]); self.move(x,y); self.button(True); time.sleep(.1); self.rel(dx,dy); time.sleep(.15); self.button(False); time.sleep(.3)
 def key(self,k):
  self.log(['key',k]); self.cmd('human-monitor-command',{'command-line':'sendkey '+k}); time.sleep(.25)
 def shot(self,name):
  self.log(['shot',name]); time.sleep(.2); p=ROOT/'shots'/f'{name}.ppm'; self.cmd('screendump',{'filename':str(p)}); Image.open(p).save(p.with_suffix('.png')); p.unlink(); print(str(p.with_suffix('.png')))
if __name__=='__main__':
 d=Driver()
 for line in sys.stdin:
  try:
   a=json.loads(line); op=a.pop(0)
   if op=='quit': d.log(['quit']); d.cmd('quit'); d.proc.wait(timeout=5); print('QEMU stopped',flush=True); break
   result=getattr(d,op)(*a); print('OK',op,result,flush=True)
  except Exception as e: print('ERROR',repr(e),flush=True)
