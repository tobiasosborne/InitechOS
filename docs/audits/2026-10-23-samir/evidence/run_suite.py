import sys,json,time,pathlib,traceback
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from audit_driver import Driver
p=pathlib.Path(sys.argv[1]); plan=json.loads(p.read_text());d=Driver()
try:
 d.start(plan['name'],'images/'+plan.get('boot','flair_tenants_interactive.img'),'images/'+plan.get('data','rich-fixture.img'))
 if plan.get('route','desktop')=='desktop': d.key('ctrl-alt-backslash');d.pause(.7)
 else:
  if b'Enter new date' in d.serial.read_bytes()[-300:]: d.key('ret',.8)
  if b'Enter new time' in d.serial.read_bytes()[-300:]: d.key('ret',.8)
  for line in plan.get('shell_commands',[]): d.type(line);d.pause(.5)
  d.type('SAMIR.COM');d.pause(1)
 d.pause(.5)
 d.shot(plan['name']+'-startup')
 if not d.serial.read_bytes().rstrip().endswith(b'.'):
  raise RuntimeError('SAMIR prompt not reached; inspect startup screenshot/serial')
 for i,a in enumerate(plan['actions']):
  start=d.serial.stat().st_size
  if isinstance(a,str):
   d.type(a);d.pause(.45)
   last=d.serial.stat().st_size; stable=time.monotonic(); until=stable+5
   while time.monotonic()<until:
    time.sleep(.1); now=d.serial.stat().st_size
    if now!=last: last=now; stable=time.monotonic()
    if time.monotonic()-stable>.45: break
  else:
   if a[0]=='shot': d.pause(.7)
   d.run(a)
  end=d.serial.stat().st_size
  with (ROOT/'evidence'/f"{plan['name']}-steps.jsonl").open('a') as f:f.write(json.dumps({'step':i,'action':a,'start':start,'end':end})+'\n')
  if isinstance(a,str):
   print(a,repr(d.serial.read_bytes()[start:end].decode(errors='replace')[-1200:]),flush=True)
finally:
 if d.proc:
  state={'status':d.cmd('query-status'),'registers':d.cmd('human-monitor-command',{'command-line':'info registers'})}
  (ROOT/'evidence'/(plan['name']+'-qmp-state.json')).write_text(json.dumps(state,indent=2))
  d.quit()
