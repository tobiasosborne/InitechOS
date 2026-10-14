#!/usr/bin/env python3
from audit_driver import Driver
import time

d=Driver()
try:
 d.start('upper-menu-check','images/flair_tenants_interactive.img','images/flair_data.img')
 d.move(38,10);d.button(True);time.sleep(.4);d.shot('121-upper-file-held');d.rel(462,370);d.button(False);time.sleep(.3);d.move(500,380);d.shot('122-upper-file-dismissed');d.quit()
finally:
 if d.proc:d.quit()
