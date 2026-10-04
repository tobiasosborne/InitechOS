#!/usr/bin/env python3
import subprocess,json,time,sys,pathlib,datetime,hashlib,os,select,shutil
from PIL import Image
ROOT=pathlib.Path(__file__).resolve().parent
class Driver:
 def __init__(self): self.proc=None; self.label=None; self.num=0
 def stamp(self): return datetime.datetime.now(datetime.timezone.utc).isoformat()
 def hashes(self):
  return {str(p.relative_to(ROOT)):{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in self.files}
 def log(self,action):
  with open(ROOT/'evidence/actions.jsonl','a') as f:
   f.write(json.dumps({'utc':self.stamp(),'session':self.label,'action':action,'serial_bytes':self.serial.stat().st_size if self.proc else None})+'\n')
 def start(self,label,image,data=None,post=None):
  if self.proc is not None: raise RuntimeError('Stop existing emulator first')
  self.label=label; self.serial=ROOT/'evidence'/f'{label}-serial.log'
  boot=ROOT/'images'/f'{label}-boot.img'; shutil.copyfile(ROOT/image,boot)
  self.files=[boot]
  cmd=['qemu-system-i386','-cpu','486','-m','16','-drive','format=raw,file='+str(boot),'-serial','file:'+str(self.serial),'-display','none','-qmp','stdio','-no-shutdown']
  if data:
   disk=ROOT/'images'/f'{label}-data.img'; shutil.copyfile(ROOT/data,disk); self.files.append(disk)
   cmd+=['-drive','file='+str(disk)+',format=raw,if=ide,index=1']
  self.stderr=open(ROOT/'evidence'/f'{label}-qemu-stderr.log','wb')
  self.proc=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=self.stderr)
  self.fin=self.proc.stdout; self.fout=self.proc.stdin; self.read(); self.cmd('qmp_capabilities')
  self.session={'started_utc':self.stamp(),'driver_pid':os.getpid(),'qemu_pid':self.proc.pid,'command':cmd,'images_before':self.hashes()}
  (ROOT/'evidence'/f'{label}-session.json').write_text(json.dumps(self.session,indent=2))
  self.log(['start',label,image,data])
  until=time.monotonic()+12
  while time.monotonic()<until:
   b=self.serial.read_bytes()
   if b'FLAIR-LIVE-READY' in b or b'FLAIR-DESKTOP' in b and 'desktop' in image: break
   time.sleep(.01)
  print('STARTED',label,'pid',self.proc.pid,flush=True)
  if post:
   for a in post: self.run(a)
 def read(self):
  if not select.select([self.fin],[],[],10)[0]: raise TimeoutError('QMP response')
  line=self.fin.readline()
  if not line: raise RuntimeError('QEMU exited '+str(self.proc.poll()))
  return json.loads(line)
 def cmd(self,name,args=None):
  self.fout.write((json.dumps({'execute':name,'arguments':args or {}})+'\n').encode()); self.fout.flush()
  while True:
   r=self.read()
   if 'error' in r: raise RuntimeError(r)
   if 'return' in r: return r['return']
 def rel(self,x,y,delay=.012,step=80):
  self.log(['rel',x,y,delay,step])
  n=max(1,(max(abs(x),abs(y))+step-1)//step)
  for i in range(n):
   dx=round((i+1)*x/n)-round(i*x/n); dy=round((i+1)*y/n)-round(i*y/n)
   self.cmd('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':dx}},{'type':'rel','data':{'axis':'y','value':dy}}]})
   if delay: time.sleep(delay)
 def move(self,x,y):
  self.log(['move',x,y]); self.rel(-800,-600); self.rel(x,y); time.sleep(.05)
 def button(self,down,button='left'):
  self.log(['button',down,button]); self.cmd('input-send-event',{'events':[{'type':'btn','data':{'button':button,'down':down}}]})
 def click(self,x,y,count=1,button='left'):
  self.log(['click',x,y,count,button]); self.move(x,y)
  for i in range(count): self.button(True,button); time.sleep(.05); self.button(False,button); time.sleep(.08)
  time.sleep(.12)
 def drag(self,x,y,dx,dy):
  self.log(['drag',x,y,dx,dy]); self.move(x,y); self.button(True); time.sleep(.08); self.rel(dx,dy); time.sleep(.08); self.button(False); time.sleep(.15)
 def menu(self,x,y,tx,ty):
  self.log(['menu',x,y,tx,ty]); self.move(x,y); self.button(True); time.sleep(.06); self.rel(tx-x,ty-y); time.sleep(.08); self.button(False); time.sleep(.15)
 def key(self,k,delay=.15):
  self.log(['key',k,delay]); r=self.cmd('human-monitor-command',{'command-line':'sendkey '+k+' 20'});
  if r: raise RuntimeError('sendkey rejected: '+str(r))
  time.sleep(delay)
 def keyheld(self,k,down):
  self.log(['keyheld',k,down]); self.cmd('input-send-event',{'events':[{'type':'key','data':{'down':down,'key':{'type':'qcode','data':k}}}]}); time.sleep(.02)
 def shot(self,name):
  self.log(['shot',name]); p=ROOT/'shots'/f'{name}.ppm'; self.cmd('screendump',{'filename':str(p)}); Image.open(p).save(p.with_suffix('.png')); p.unlink(); print('SHOT',name,flush=True)
 def type(self,text,enter=True):
  self.log(['type',text,enter])
  chars={' ':'spc','.':'dot',':':'shift-semicolon',';':'semicolon','/':'slash','\\':'backslash','>':'shift-dot','<':'shift-comma','=':'equal','-':'minus','_':'shift-minus','*':'shift-8','?':'shift-slash','"':'shift-apostrophe',"'":'apostrophe','(':'shift-9',')':'shift-0',',':'comma','!':'shift-1','|':'shift-backslash','%':'shift-5','+':'shift-equal','&':'shift-7','$':'shift-4','@':'shift-2'}
  for ch in text:
   k=chars.get(ch, ('shift-'+ch.lower()) if ch.isupper() else ch)
   self.key(k,.045)
  if enter: self.key('ret',.3)
 def pause(self,seconds):
  if seconds>60: raise ValueError('Use shorter waits')
  self.log(['pause',seconds]); time.sleep(seconds)
 def note(self,n): self.log(['note',n])
 def tail(self,n=30): print('\n'.join(self.serial.read_text(errors='replace').splitlines()[-n:]),flush=True)
 def quit(self):
  self.log(['quit']); pid=self.proc.pid
  try: self.cmd('quit')
  finally:
   self.proc.wait(timeout=10); self.stderr.close(); self.session.update(stopped_utc=self.stamp(),exit_code=self.proc.returncode,images_after=self.hashes())
   (ROOT/'evidence'/f'{self.label}-session.json').write_text(json.dumps(self.session,indent=2))
   with open(ROOT/'evidence/cleanup.jsonl','a') as f: f.write(json.dumps({'utc':self.stamp(),'session':self.label,'pid':pid,'exit_code':self.proc.returncode})+'\n')
   self.proc=None
  print('STOPPED',pid,flush=True)
 def run(self,a):
  if a[0]=='batch':
   for b in a[1]: self.run(b)
  else: getattr(self,a[0])(*a[1:])
if __name__=='__main__':
 d=Driver()
 try:
  for line in sys.stdin:
   try: d.run(json.loads(line)); print('OK',flush=True)
   except Exception as e: print('ERROR',repr(e),flush=True)
 finally:
  if d.proc is not None: d.quit()
