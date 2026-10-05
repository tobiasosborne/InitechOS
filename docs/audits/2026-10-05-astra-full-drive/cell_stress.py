from audit_driver import Driver,ROOT
import time,json
D=Driver();t=time.monotonic()
try:
 D.start('cell-stress','images/flair_tenants_interactive.img','images/flair_data.img');D.click(600,64,2);D.click(123,101,2);D.click(211,124,2)
 for i in range(2050):
  D.key('1',.04);D.key('down',.04)
  if i in [511,1023,1535,2046,2047,2048,2049]:
   D.pause(.5);D.shot('cell-%04d'%(i+1));print('CELL',i+1,flush=True)
 D.key('esc');D.key('home');D.type('2');D.pause(.5);D.shot('cell-existing-edit');D.key('ctrl-q')
finally:
 if D.proc:D.quit()
 (ROOT/'evidence/cell-stress-summary.json').write_text(json.dumps({'seconds':time.monotonic()-t,'attempted_entries':2050},indent=2))
