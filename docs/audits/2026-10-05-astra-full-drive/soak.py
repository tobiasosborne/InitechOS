from audit_driver import Driver,ROOT
import time,json
# A real QMP workload, 210 disk app lifecycles plus native window geometry.
d=Driver();counts={};start=time.monotonic()
try:
 d.start('soak','images/flair_tenants_interactive.img','images/flair_data.img')
 d.click(600,64,2);d.click(123,101,2)
 for i in range(210):
  kind=i%3;x=[75,143,211][kind];d.click(x,124,2)
  if kind==0:
   d.menu(140,28,174,50);d.drag(240,170,30,25);d.drag(270,195,-30,-25)
  elif kind==1:
   d.type('q9',False);d.click(250,250)
   d.click(459,180);d.click(459,180)
  else:
   d.type(str(i));d.key('right');d.type('Cycle '+str(i));d.key('esc');d.key('home')
  d.key('ctrl-q');counts[kind]=counts.get(kind,0)+1
  if i%21==20:
   d.pause(.4);d.move(500,380);d.shot('soak-%03d'%(i+1));print('CYCLES',i+1,flush=True)
 # Long menu pointer history ending on a named visible row.
 d.move(119,28);d.button(True);d.rel(45,186)
 for i in range(3):d.pause(15)
 d.shot('soak-held-45s');d.button(False);d.pause(.5);d.shot('soak-arrange-release')
 d.keyheld('ctrl',True);d.keyheld('shift',True);d.keyheld('ctrl',False);d.keyheld('shift',False);d.key('n');d.key('w');d.pause(.5);d.shot('soak-modifiers-final')
finally:
 if d.proc:d.quit()
 (ROOT/'evidence/soak-summary.json').write_text(json.dumps({'seconds':time.monotonic()-start,'cycles':counts},indent=2))
