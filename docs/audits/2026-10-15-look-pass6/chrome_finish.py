#!/usr/bin/env python3
from audit_driver import Driver,ROOT
import time,json
from PIL import Image
import numpy as np
r={};d=Driver()
try:
 d.start('collapse-check','images/flair_tenants_interactive.img','images/flair_data.img')
 d.click(600,64,2);d.move(500,380);d.shot('113-root-before-collapse')
 d.click(370,70);time.sleep(.4);d.move(500,380);d.shot('114-root-collapsed')
 d.click(370,70);time.sleep(.4);d.move(500,380);d.shot('115-root-expanded')
 d.drag(370,270,-200,-120);d.click(151,147);d.drag(170,150,200,120);d.move(500,380);d.shot('116-resize-stale-bar')
 d.click(352,267);d.move(500,380);d.shot('117-stale-bar-click')
 d.quit()
finally:
 if d.proc:d.quit()
for n in ['113-root-before-collapse','114-root-collapsed','115-root-expanded']:
 a=np.array(Image.open(ROOT/'shots'/(n+'.png')).convert('RGB'));r[n]= {'title_sha':__import__('hashlib').sha256(a[60:82,20:380].tobytes()).hexdigest()}
(ROOT/'evidence/collapse-check.json').write_text(json.dumps(r,indent=2))
