#!/usr/bin/env python3
from audit_driver import Driver
import time

d=Driver()
try:
 d.start('bars-proof','images/flair_tenants_interactive.img','images/many_source.img')
 d.click(600,64,2);d.drag(370,270,-200,-120);d.click(151,147);d.drag(170,150,200,120);d.move(500,380);d.shot('118-grown-stale-bar')
 d.click(352,267);d.move(500,380);d.shot('119-ignored-stale-bar');d.quit()
 d.start('film-cursor','images/flair_live_interactive.img','images/flair_data.img')
 d.move(470,228);time.sleep(.4);d.shot('120-film-arrow-on-modal');d.quit()
finally:
 if d.proc:d.quit()
