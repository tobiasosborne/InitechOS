#!/usr/bin/env python3
from audit_driver import Driver,ROOT
from motion_capture import frames
import time,json
# The helper import has no side effects. Each leg below has a unique label.
d=Driver();r={}
try:
 d.start('lotus-controls','images/flair_tenants_interactive.img','images/flair_data.img')
 d.click(600,64,2);d.click(123,101,2);d.click(211,124,2)
 d.type('/',False);time.sleep(.5);d.move(500,380);d.shot('101-123-main-menu')
 d.type('fs',False);time.sleep(.5);d.shot('102-123-file-save')
 # Escape Save -> File -> Main -> READY before committing an edit.
 d.key('esc');d.key('esc');d.key('esc');d.type('abc');d.key('ctrl-q');time.sleep(.3)
 d.move(393,168);d.button(True);time.sleep(.2);d.shot('103-quit-cancel-mousedown');d.button(False);d.move(500,380);d.shot('104-quit-cancel-release')
 d.quit()
 d.ready_seconds=0
 d.start('film-boot','images/flair_live_interactive.img','images/flair_data.img')
 r['film_boot']=frames(d,'film-boot',35,.035);time.sleep(.5);d.move(500,380);d.shot('105-film-steady')
 d.quit()
finally:
 if d.proc is not None:d.quit()
(ROOT/'evidence/final-samples.json').write_text(json.dumps(r,indent=2))
