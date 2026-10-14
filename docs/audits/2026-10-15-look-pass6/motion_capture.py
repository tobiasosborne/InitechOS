#!/usr/bin/env python3
from audit_driver import Driver,ROOT
from PIL import Image
import time,json,hashlib

def frames(d,prefix,count=12,interval=.025):
 out=[];t=time.monotonic()
 for i in range(count):
  p=ROOT/'shots'/f'{prefix}-{i:02}.ppm'
  d.cmd('screendump',{'filename':str(p)})
  elapsed=(time.monotonic()-t)*1000
  im=Image.open(p);im.save(p.with_suffix('.png'));p.unlink()
  out.append({'file':p.with_suffix('.png').name,'ms':round(elapsed,3),'sha256':hashlib.sha256(im.tobytes()).hexdigest()})
  time.sleep(interval)
 return out

if __name__ == '__main__':
 d=Driver();results={}
 try:
  d.start('motion','images/flair_tenants_interactive.img','images/many_source.img')
  d.click(600,64,2);d.move(38,30);d.shot('098-menu-before')
  d.button(True);results['menu_down']=frames(d,'motion-menu',10,.02)
  d.rel(462,350);d.button(False);time.sleep(.3)
  d.move(200,70);d.button(True);d.rel(80,45);results['window_drag_held']=frames(d,'motion-drag',6,.025)
  d.button(False);results['window_drag_release']=frames(d,'motion-release',10,.025)
  d.move(500,380);d.shot('099-after-motion-drag')
  d.click(447,297);results['scroll_after_click']=frames(d,'motion-scroll',10,.025)
  d.move(500,380);d.shot('100-after-motion-scroll')
 finally:
  if d.proc is not None:d.quit()
 (ROOT/'evidence/motion-samples.json').write_text(json.dumps(results,indent=2))
 print('Motion capture complete',flush=True)
