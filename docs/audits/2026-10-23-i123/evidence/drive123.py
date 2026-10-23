#!/usr/bin/env python3
import sys,pathlib,time,json,shutil,re,ast,subprocess,struct
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from audit_driver import Driver
REPO=pathlib.Path('/home/tobias/Projects/initech-os')
class Audit(Driver):
 def key(self,k,delay=.06):
  before=self.serial.stat().st_size if self.proc else 0
  out=super().key(k,max(delay,.055))
  deadline=time.monotonic()+3
  while self.proc and time.monotonic()<deadline:
   with open(self.serial,'rb') as f:f.seek(before);tail=f.read()
   if b'FLAIR-EVT what=4' in tail:break
   time.sleep(.015)
  return out
 def open123(self):
  self.click(600,64,2); self.move(500,380); self.click(123,101,2); self.click(211,124,2); self.pause(.5)
 def launch(self,label,data='images/fixture.img'):
  self.start(label,'images/flair_tenants_interactive.img',data); self.open123()
 def goto(self,addr): self.key('f5');self.type(addr)
 def cell(self,addr,value): self.goto(addr);self.type(value)
 def slash(self,keys): self.type('/'+keys,False)
 def clear(self): self.key('ctrl-scroll_lock');self.slash('wey')
 def saveas(self,name): self.slash('fs');self.type(name)
 def get(self,name): self.key('ctrl-scroll_lock');self.slash('fr');self.type(name)
 def state(self,name):
  self.shot(name)
  out=subprocess.run(['tesseract',str(ROOT/'shots'/f'{name}.png'),'stdout','--psm','6'],capture_output=True,text=True).stdout
  (ROOT/'evidence'/f'{name}.txt').write_text(out)
  print(out[:1800],flush=True)
 def keys(self,ks):
  for k in ks:self.key(k)
 def menu_path(self,path,execute=True):
  node=MENU[0];self.key('ctrl-scroll_lock');self.key('slash')
  for pos,part in enumerate(path.split('/')[1:]):
   idx=next(i for i,it in enumerate(node) if it['name']==part)
   self.key('home')
   for _ in range(idx):self.key('right')
   if pos<len(path.split('/')[1:])-1 or execute:
    self.key('ret')
   if node[idx]['child']!=65535:node=MENU[node[idx]['child']]
 def run(self,a): return super().run(a)
s=(REPO/'os/i123/core/menu_data.h').read_text()
strings=[ast.literal_eval(x).rstrip('\0') for x in re.findall(r'"(?:[^"\\]|\\.)*"',s.split('menu_template_strings[] =')[1].split('static const uint16_t menu_template_offsets')[0])]
items=[list(map(int,m)) for m in re.findall(r'\{(\d+),(\d+),(\d+)\}',s.split('menu_template_items[] =')[1].split(';')[0])]
levels=[list(map(int,m)) for m in re.findall(r'\{(\d+),(\d+)\}',s.split('menu_template_levels[] =')[1].split(';')[0])]
MENU=[[{'name':strings[n],'description':strings[d],'child':c} for n,d,c in items[first:first+count]] for first,count in levels]
def fixture():
 shutil.copyfile(ROOT/'images/flair_data.img',ROOT/'images/fixture.img')
 src=pathlib.Path('/home/tobias/Projects/lotus123-decomp/goldens/minted')
 for name in ['MINT01','MINT02','MINT03','MINT04','MINT05','CIRC01','COPY01A','COPY01B','COPY01C','COPY01D','MOVE01A','MOVE01B','MOVE01C','MOVE01D','WRAP01A','WRAP01B','WRAP01C']:
  p=src/(name+'.WK1')
  if p.exists():subprocess.run(['mcopy','-i',str(ROOT/'images/fixture.img'),str(p),'::'+p.name],check=True)
 def rec(op,b):return struct.pack('<HH',op,len(b))+b
 for n in [2047,2048,2049]:
  b=rec(0,b'\x06\x04')+b''.join(rec(13,struct.pack('<BHHh',0x7f,0,i,1)) for i in range(n))+rec(1,b'')
  p=ROOT/'evidence'/f'CAP{n}.WK1';p.write_bytes(b)
  subprocess.run(['mcopy','-i',str(ROOT/'images/fixture.img'),str(p),'::'+p.name],check=True)
 p=ROOT/'evidence/BAD.WK1';p.write_bytes(b'not a worksheet\0')
 subprocess.run(['mcopy','-i',str(ROOT/'images/fixture.img'),str(p),'::BAD.WK1'],check=True)
 (ROOT/'evidence/menu-tree.json').write_text(json.dumps(MENU,indent=2))
if __name__=='__main__':
 if len(sys.argv)>1 and sys.argv[1]=='fixture':fixture();sys.exit()
 a=Audit()
 try:
  for line in sys.stdin:
   try:a.run(json.loads(line));print('OK',flush=True)
   except Exception as e:print('ERROR',repr(e),flush=True)
 finally:
  if a.proc is not None:a.quit()
