#!/usr/bin/env python3
from audit_driver import Driver,ROOT
from PIL import Image
import time,json,hashlib
r=[];d=Driver()
try:
 d.start('cursor-cycle','images/tracer_boot.img','images/flair_data.img')
 d.type('CLS');time.sleep(1)
 t=time.monotonic()
 for i in range(14):
  p=ROOT/'shots'/f'cursor-cycle-{i:02}.ppm';d.cmd('screendump',{'filename':str(p)})
  im=Image.open(p);im.save(p.with_suffix('.png'));p.unlink()
  r.append({'file':p.with_suffix('.png').name,'ms':round((time.monotonic()-t)*1000,2),'sha256':hashlib.sha256(im.tobytes()).hexdigest()})
  time.sleep(.16)
finally:
 if d.proc:d.quit()
(ROOT/'evidence/cursor-cycle.json').write_text(json.dumps(r,indent=2))
