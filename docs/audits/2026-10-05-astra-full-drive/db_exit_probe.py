from audit_driver import Driver,ROOT
import time,json
cases={'db-exit-control':['USE CLIENTS.DBF','LIST'], 'db-exit-two':['SELECT 1','USE CLIENTS.DBF ALIAS ONE','SELECT 2','USE CLIENTS.DBF ALIAS TWO'], 'db-exit-zap-two':['SELECT 1','USE CLIENTS.DBF ALIAS ONE','SELECT 2','USE CLIENTS.DBF ALIAS TWO','SELECT 1','SET SAFETY ON','ZAP','APPEND BLANK','REPLACE NAME WITH "NEW"']}
for label,cs in cases.items():
 d=Driver()
 try:
  d.start(label,'images/flair_tenants_interactive.img','images/db-fixture.img');d.key('backslash');time.sleep(.5)
  for c in cs:
   d.type(c);time.sleep(.4)
  d.shot(label+'-before');d.type('QUIT');d.pause(3)
  if b'FLAIR-RESUME' not in d.serial.read_bytes():d.pause(15)
  d.shot(label+'-after')
  (ROOT/'evidence'/f'{label}-qmp-state.json').write_text(json.dumps({'status':d.cmd('query-status'),'registers':d.cmd('human-monitor-command',{'command-line':'info registers'})},indent=2))
  d.key('ctrl-c');d.key('ret');d.pause(.5);d.shot(label+'-recovery')
 finally:d.quit()
